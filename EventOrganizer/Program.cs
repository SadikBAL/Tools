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

app.MapStaticAssets();
app.MapRazorComponents<App>()
    .AddInteractiveServerRenderMode();

// gRPC QUIC bağlantısını uygulama başlarken arka planda ısıt.
// İlk kullanıcı isteğinde timeout yaşanmadan önce HTTP/2 fallback devreye girer.
_ = app.Services.GetRequiredService<FirestoreDb>()
    .Collection("events").Limit(1).GetSnapshotAsync()
    .ContinueWith(_ => { });

app.Run();
