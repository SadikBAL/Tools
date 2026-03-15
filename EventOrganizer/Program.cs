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

    if (path.StartsWith("/event/", StringComparison.OrdinalIgnoreCase))
    {
        var log = context.RequestServices.GetRequiredService<ILoggerFactory>().CreateLogger("OGCrawler");
        log.LogWarning("[OG] /event/ request: isCrawler={IsCrawler} UA={UA}", IsSocialCrawler(ua), ua);
    }

    if (path.StartsWith("/event/", StringComparison.OrdinalIgnoreCase) && IsSocialCrawler(ua))
    {
        var log2    = context.RequestServices.GetRequiredService<ILoggerFactory>().CreateLogger("OGCrawler");
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

                    // EventCard.razor ile aynı proxy mantığı — Firebase URL → /img/ proxy
                    // Resim yoksa kategori default resmi (Unsplash, herkese açık)
                    var defaultImages = new Dictionary<string, string>
                    {
                        ["music"] = "https://images.unsplash.com/photo-1470229722913-7c0e2dbbafd3?w=1200&h=630&fit=crop",
                        ["tech"]  = "https://images.unsplash.com/photo-1540575467063-178a50c2df87?w=1200&h=630&fit=crop",
                        ["art"]   = "https://images.unsplash.com/photo-1547826039-bfc35e0f1ea8?w=1200&h=630&fit=crop",
                        ["sport"] = "https://images.unsplash.com/photo-1571008887538-b36bb32f4571?w=1200&h=630&fit=crop",
                    };

                    string imageUrl;
                    var raw = evt.ImageUrl ?? "";
                    if (string.IsNullOrWhiteSpace(raw))
                    {
                        imageUrl = defaultImages.GetValueOrDefault(evt.Category ?? "", "https://images.unsplash.com/photo-1540575467063-178a50c2df87?w=1200&h=630&fit=crop");
                    }
                    else if (raw.StartsWith("/img/"))
                    {
                        // Yeni format: /img/events/yyyyMMdd/guid.ext → direkt public GCS URL
                        var cfg    = context.RequestServices.GetRequiredService<IConfiguration>();
                        var bucket = cfg["Firebase:StorageBucket"]!;
                        imageUrl = $"https://storage.googleapis.com/{bucket}/{raw["/img/".Length..]}";
                    }
                    else if (raw.Contains("firebasestorage.googleapis.com"))
                    {
                        // Eski Firebase Storage format → proxy üzerinden
                        var oIdx = raw.IndexOf("/o/", StringComparison.Ordinal);
                        if (oIdx >= 0)
                        {
                            var encoded = raw[(oIdx + 3)..];
                            var qIdx    = encoded.IndexOf('?');
                            if (qIdx >= 0) encoded = encoded[..qIdx];
                            imageUrl = $"{baseUrl}/img/{Uri.UnescapeDataString(encoded)}";
                        }
                        else imageUrl = raw;
                    }
                    else
                    {
                        imageUrl = raw.StartsWith("http") ? raw : $"{baseUrl}{raw}";
                    }

                    var tr = new System.Globalization.CultureInfo("tr-TR");
                    var datePart = evt.Date.ToString("dd MMMM yyyy · HH:mm", tr);
                    var ogDesc   = string.IsNullOrEmpty(evt.Location)
                        ? datePart
                        : $"{datePart}\n{evt.Location}";

                    var html = $"""
                        <!DOCTYPE html>
                        <html lang="tr">
                        <head>
                        <meta charset="utf-8"/>
                        <title>{E(evt.Title)}</title>
                        <meta name="description" content="{E(ogDesc)}"/>
                        <meta property="og:type"             content="website"/>
                        <meta property="og:url"              content="{pageUrl}"/>
                        <meta property="og:title"            content="{E(evt.Title)}"/>
                        <meta property="og:description"      content="{E(ogDesc)}"/>
                        <meta property="og:image"            content="{imageUrl}"/>
                        <meta property="og:image:secure_url" content="{imageUrl}"/>
                        <meta property="og:image:type"       content="image/jpeg"/>
                        <meta property="og:image:width"      content="1200"/>
                        <meta property="og:image:height"     content="630"/>
                        <meta property="og:image:alt"        content="{E(evt.Title)}"/>
                        <meta property="og:site_name"        content="EventOrganizer"/>
                        <meta name="twitter:card"            content="summary_large_image"/>
                        <meta name="twitter:title"           content="{E(evt.Title)}"/>
                        <meta name="twitter:description"     content="{E(ogDesc)}"/>
                        <meta name="twitter:image"           content="{imageUrl}"/>
                        </head>
                        <body>
                        <h1>{E(evt.Title)}</h1>
                        <p>{E(ogDesc)}</p>
                        </body>
                        </html>
                        """;

                    log2.LogWarning("[OG] Serving HTML: title={Title} imageRaw={Raw} imageUrl={ImageUrl}",
                        evt.Title, evt.ImageUrl, imageUrl);

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
