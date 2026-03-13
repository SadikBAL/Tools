using EventOrganizer.Components;
using EventOrganizer.Services;
using Google.Cloud.Firestore;
using Microsoft.AspNetCore.Authentication;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddRazorComponents()
    .AddInteractiveServerComponents();

// Google Authentication
builder.Services.AddAuthentication(options =>
{
    options.DefaultScheme = "Cookies";
    options.DefaultChallengeScheme = "Google";
})
.AddCookie("Cookies")
.AddGoogle("Google", options =>
{
    options.ClientId     = builder.Configuration["Authentication:Google:ClientId"]!;
    options.ClientSecret = builder.Configuration["Authentication:Google:ClientSecret"]!;
    options.Scope.Add("profile");
    options.Scope.Add("email");
    options.ClaimActions.MapJsonKey("picture", "picture");
});

builder.Services.AddAuthorization();
builder.Services.AddCascadingAuthenticationState();
builder.Services.AddHttpContextAccessor();

// Firestore
var projectId       = builder.Configuration["Firebase:ProjectId"]!;
var credentialsPath = builder.Configuration["Firebase:CredentialsPath"];

var firestoreBuilder = new FirestoreDbBuilder { ProjectId = projectId };
if (!string.IsNullOrEmpty(credentialsPath))
{
    // Relative path ise proje klasörüne göre çöz
    var fullPath = Path.IsPathRooted(credentialsPath)
        ? credentialsPath
        : Path.Combine(builder.Environment.ContentRootPath, credentialsPath);
    firestoreBuilder.CredentialsPath = fullPath;
}

builder.Services.AddSingleton(_ => firestoreBuilder.Build());
builder.Services.AddScoped<FirestoreService>();
builder.Services.AddSingleton<StorageService>();

var app = builder.Build();

if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Error", createScopeForErrors: true);
    app.UseHsts();
}

app.UseStatusCodePagesWithReExecute("/not-found", createScopeForStatusCodePages: true);
app.UseHttpsRedirection();
app.UseAuthentication();
app.UseAuthorization();
app.UseAntiforgery();

// Google login/logout endpoint'leri
app.MapGet("/login", () => Results.Challenge(
    new Microsoft.AspNetCore.Authentication.AuthenticationProperties { RedirectUri = "/" },
    ["Google"]));

app.MapGet("/logout", async (HttpContext ctx) =>
{
    await ctx.SignOutAsync("Cookies");
    return Results.Redirect("/");
});

// Firebase Storage proxy — CORS/CORB sorununu ortadan kaldırır (same-origin)
app.MapGet("/img/{**path}", async (string path, StorageService storage, HttpContext ctx) =>
{
    try
    {
        var (stream, contentType) = await storage.DownloadAsync(path);
        ctx.Response.Headers.CacheControl = "public, max-age=31536000";
        return Results.Stream(stream, contentType);
    }
    catch { return Results.NotFound(); }
});

// Social media crawler middleware — OG preview için
// WhatsApp/Telegram/Discord/Slack gibi botlar /event/{id} çektiğinde
// Blazor shell yerine OG meta tagları olan minimal HTML döndürür.
app.Use(async (context, next) =>
{
    var path = context.Request.Path.Value ?? "";
    var ua   = context.Request.Headers.UserAgent.ToString();

    if (path.StartsWith("/event/", StringComparison.OrdinalIgnoreCase) && IsSocialCrawler(ua))
    {
        var eventId = path["/event/".Length..].Trim('/');
        if (!string.IsNullOrWhiteSpace(eventId))
        {
            try
            {
                var firestore = context.RequestServices.GetRequiredService<FirestoreService>();
                var evt = await firestore.GetEventByIdAsync(eventId);
                if (evt != null)
                {
                    var req     = context.Request;
                    var baseUrl = $"{req.Scheme}://{req.Host}";
                    var pageUrl = $"{baseUrl}/event/{eventId}";

                    // ImageUrl /img/... formatında ya da eski firebase URL olabilir
                    var imageUrl = string.IsNullOrEmpty(evt.ImageUrl)
                        ? $"{baseUrl}/favicon.png"
                        : evt.ImageUrl.StartsWith("http")
                            ? evt.ImageUrl
                            : $"{baseUrl}{evt.ImageUrl}";

                    var tr = new System.Globalization.CultureInfo("tr-TR");
                    var datePart = evt.Date.ToString("dd MMMM yyyy · HH:mm", tr);
                    var subtitle = string.IsNullOrEmpty(evt.Location)
                        ? datePart
                        : $"{datePart} · {evt.Location}";

                    var rawDesc  = evt.Description ?? "";
                    var truncated = rawDesc.Length > 140 ? rawDesc[..140] + "…" : rawDesc;
                    var ogDesc   = string.IsNullOrEmpty(truncated) ? subtitle : $"{subtitle}\n{truncated}";

                    var html = $"""
                        <!DOCTYPE html>
                        <html lang="tr">
                        <head>
                        <meta charset="utf-8"/>
                        <title>{E(evt.Title)}</title>
                        <meta name="description" content="{E(ogDesc)}"/>
                        <meta property="og:type"        content="website"/>
                        <meta property="og:url"         content="{pageUrl}"/>
                        <meta property="og:title"       content="{E(evt.Title)}"/>
                        <meta property="og:description" content="{E(ogDesc)}"/>
                        <meta property="og:image"       content="{imageUrl}"/>
                        <meta property="og:image:width"  content="1200"/>
                        <meta property="og:image:height" content="630"/>
                        <meta property="og:site_name"   content="EventOrganizer"/>
                        <meta name="twitter:card"        content="summary_large_image"/>
                        <meta name="twitter:title"       content="{E(evt.Title)}"/>
                        <meta name="twitter:description" content="{E(ogDesc)}"/>
                        <meta name="twitter:image"       content="{imageUrl}"/>
                        </head>
                        <body>
                        <h1>{E(evt.Title)}</h1>
                        <p>{E(subtitle)}</p>
                        </body>
                        </html>
                        """;

                    context.Response.ContentType = "text/html; charset=utf-8";
                    await context.Response.WriteAsync(html);
                    return;
                }
            }
            catch { /* hata olursa Blazor handle etsin */ }
        }
    }

    await next();
});

bool IsSocialCrawler(string ua)
{
    if (string.IsNullOrEmpty(ua)) return false;
    var u = ua.ToLowerInvariant();
    return u.Contains("whatsapp") ||
           u.Contains("telegrambot") ||
           u.Contains("discordbot") ||
           u.Contains("twitterbot") ||
           u.Contains("facebookexternalhit") ||
           u.Contains("slackbot") ||
           u.Contains("linkedinbot") ||
           u.Contains("applebot") ||
           u.Contains("googlebot") ||
           u.Contains("bingbot");
}

string E(string? s) => System.Net.WebUtility.HtmlEncode(s ?? "");

app.MapStaticAssets();
app.MapRazorComponents<App>()
    .AddInteractiveServerRenderMode();

// gRPC QUIC bağlantısını uygulama başlarken arka planda ısıt.
// İlk kullanıcı isteğinde timeout yaşanmadan önce HTTP/2 fallback devreye girer.
_ = app.Services.GetRequiredService<FirestoreDb>()
    .Collection("events").Limit(1).GetSnapshotAsync()
    .ContinueWith(_ => { });

app.Run();
