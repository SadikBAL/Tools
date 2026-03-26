using Google.Apis.Auth.OAuth2;
using Google.Cloud.Storage.V1;
using SixLabors.ImageSharp;
using SixLabors.ImageSharp.Processing;

namespace EventOrganizer.Services;

public class StorageService
{
    private readonly StorageClient _client;
    private readonly string        _bucket;

    public StorageService(IConfiguration config, IWebHostEnvironment env)
    {
        var credentialsPath = config["Firebase:CredentialsPath"];

        GoogleCredential credential;
        if (!string.IsNullOrEmpty(credentialsPath))
        {
            var fullPath = Path.IsPathRooted(credentialsPath)
                ? credentialsPath
                : Path.Combine(env.ContentRootPath, credentialsPath);
            credential = GoogleCredential.FromFile(fullPath);
        }
        else
        {
            credential = GoogleCredential.GetApplicationDefault();
        }

        _client = StorageClient.Create(credential);
        _bucket = config["Firebase:StorageBucket"]!;
    }

    // Yükler ve /img/{objectName} proxy URL'si döner.
    // crop verilmişse önce o alan kırpılır, sonra 1200×630'a resize edilir.
    public async Task<string> UploadImageAsync(Stream stream, string fileName, string contentType,
        int cropX = 0, int cropY = 0, int cropW = 0, int cropH = 0)
    {
        var objectName = $"events/{DateTime.UtcNow:yyyyMMdd}/{Guid.NewGuid():N}.jpg";

        using var image = await Image.LoadAsync(stream);

        image.Mutate(x =>
        {
            if (cropW > 0 && cropH > 0)
            {
                // Koordinatların resim sınırları dışına taşmamasını garantile
                var safeX = Math.Max(0, Math.Min(cropX, image.Width  - 1));
                var safeY = Math.Max(0, Math.Min(cropY, image.Height - 1));
                var safeW = Math.Min(cropW, image.Width  - safeX);
                var safeH = Math.Min(cropH, image.Height - safeY);
                x.Crop(new Rectangle(safeX, safeY, safeW, safeH));
            }
            x.Resize(new ResizeOptions { Size = new Size(1200, 630), Mode = ResizeMode.Crop });
        });

        using var ms = new MemoryStream();
        await image.SaveAsJpegAsync(ms);
        ms.Position = 0;

        await _client.UploadObjectAsync(_bucket, objectName, "image/jpeg", ms);

        return $"/img/{objectName}";
    }

    // /img/{path} endpoint'i için Storage'dan stream döner
    public async Task<(Stream stream, string contentType)> DownloadAsync(string objectName)
    {
        var obj = await _client.GetObjectAsync(_bucket, objectName);
        var ms  = new MemoryStream();
        await _client.DownloadObjectAsync(_bucket, objectName, ms);
        ms.Position = 0;
        return (ms, obj.ContentType ?? "image/jpeg");
    }

    // Kart arka yüzü iç alan oranına göre kırpar: 245px kart - 2×3 border - 2×10 padding = 219px geniş,
    // 342px - 2×3 - 2×10 = 316px; header ~42px çıkar → ~274px yüksek.
    // 2× kalitede hedef: 438×548.
    public async Task<string> UploadTemplateImageAsync(Stream stream, string fileName)
    {
        const int W = 438, H = 548;
        var objectName = $"templates/{Guid.NewGuid():N}.png";

        using var image = await Image.LoadAsync(stream);
        image.Mutate(x =>
        {
            // Oranı koruyarak ortadan kırp (event görselleriyle aynı mantık)
            var safeX = 0; var safeY = 0; var safeW = image.Width; var safeH = image.Height;
            var targetRatio = (double)W / H;
            var srcRatio    = (double)image.Width / image.Height;
            if (srcRatio > targetRatio)
            {
                safeW = (int)(image.Height * targetRatio);
                safeX = (image.Width - safeW) / 2;
            }
            else
            {
                safeH = (int)(image.Width / targetRatio);
                safeY = (image.Height - safeH) / 2;
            }
            x.Crop(new Rectangle(safeX, safeY, safeW, safeH));
            x.Resize(new ResizeOptions { Size = new Size(W, H), Mode = ResizeMode.Stretch });
        });

        using var ms = new MemoryStream();
        await image.SaveAsPngAsync(ms);
        ms.Position = 0;
        await _client.UploadObjectAsync(_bucket, objectName, "image/png", ms);
        return $"/img/{objectName}";
    }

    public async Task DeleteImageAsync(string? imageUrl)
    {
        if (string.IsNullOrEmpty(imageUrl)) return;

        string objectName;

        if (imageUrl.StartsWith("/img/"))
        {
            // Yeni format: /img/events/20260312/abc.jpg
            objectName = imageUrl[5..];
        }
        else if (imageUrl.Contains("firebasestorage.googleapis.com"))
        {
            // Eski format: https://firebasestorage.googleapis.com/v0/b/.../o/{encoded}?alt=media
            var absPath = new Uri(imageUrl).AbsolutePath;
            var oIndex  = absPath.IndexOf("/o/", StringComparison.Ordinal);
            if (oIndex < 0) return;
            objectName = Uri.UnescapeDataString(absPath[(oIndex + 3)..]);
        }
        else return;

        try { await _client.DeleteObjectAsync(_bucket, objectName); }
        catch { /* Bulunamazsa ya da zaten silinmişse yoksay */ }
    }
}
