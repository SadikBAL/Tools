using System;
using System.IO;
using System.Text;
using System.Linq;

public class UAssetBatchParser
{
    /// <summary>
    /// Bir .uasset dosyasından Asset Adını ve Sınıf Adını okumayı dener.
    /// </summary>
    /// <param name="filePath">.uasset dosyasının tam yolu.</param>
    /// <returns>Asset adı ve sınıfını içeren bir tuple. Bulunamazsa null döner.</returns>
    public static (string AssetName, string AssetClass)? ParseAssetNameAndClass(string filePath)
    {
        if (!File.Exists(filePath))
        {
            Console.WriteLine($"Hata: {filePath} bulunamadı.");
            return null;
        }

        try
        {
            byte[] fileBytes = File.ReadAllBytes(filePath);

            string assetName = ExtractAssetNameFromPath(fileBytes);
            string assetClass = FindTopLevelClass(fileBytes);
            
            if (assetName != null && assetClass != null)
            {
                return (assetName, assetClass);
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Dosya okunurken bir hata oluştu: {Path.GetFileName(filePath)} - {ex.Message}");
        }

        return null;
    }

    /// <summary>
    /// Dosya içindeki /Game/ ile başlayan asset yolunu bulur ve buradan asset adını çıkarır.
    /// </summary>
    private static string? ExtractAssetNameFromPath(byte[] fileBytes)
    {
        byte[] searchTerm = Encoding.ASCII.GetBytes("/Game/");
        int pathStartIndex = FindByteSequence(fileBytes, searchTerm);

        if (pathStartIndex != -1)
        {
            int pathEndIndex = Array.IndexOf(fileBytes, (byte)0x00, pathStartIndex);
            if (pathEndIndex != -1)
            {
                string fullPath = Encoding.ASCII.GetString(fileBytes, pathStartIndex, pathEndIndex - pathStartIndex);
                return Path.GetFileNameWithoutExtension(fullPath);
            }
        }
        return null;
    }

    /// <summary>
    /// Dosya içindeki en üst seviye sınıf referansını bulur (örn: BlueprintGeneratedClass).
    /// </summary>
    private static string? FindTopLevelClass(byte[] fileBytes)
    {
        // Aranacak Sınıf İsimlerinin Kapsamlı Listesi
        string[] commonClassNames = {
            // Blueprint'ler ve Mantık
            "BlueprintGeneratedClass", "AnimBlueprint", "WidgetBlueprint", "BehaviorTree", "BlackboardData", "Blueprint",
            
            // 3D Modeller ve Geometri
            "StaticMesh", "SkeletalMesh", "Skeleton", "PhysicsAsset", "GeometryCache",
            
            // Materyaller ve Dokular
            "Material", "MaterialInstanceConstant", "MaterialFunction", "Texture2D", "TextureCube", "RenderTarget",
            
            // Animasyon
            "AnimSequence", "AnimMontage", "BlendSpace", "AnimComposite",
            
            // Ses
            "SoundWave", "SoundCue", "SoundAttenuation", "AkAudioEvent",
            
            // Veri ve Yapılandırma
            "DataTable", "DataAsset", "PrimaryDataAsset", "CurveFloat", "CurveVector", "InputMappingContext",
            
            // Efektler (VFX)
            "NiagaraSystem", "NiagaraEmitter", "ParticleSystem",
            
            // Kullanıcı Arayüzü (UMG)
            "Font", "SlateBrushAsset",

            // Seviyeler ve Sinematik
            "World", "LevelSequence"
        };

        foreach (var className in commonClassNames)
        {
            byte[] searchTerm = Encoding.ASCII.GetBytes(className);
            if (FindByteSequence(fileBytes, searchTerm) != -1)
            {
                // Okunabilirliği artırmak için bazı isimleri daha kullanıcı dostu hale getirelim
                switch (className)
                {
                    case "BlueprintGeneratedClass":
                        return "Blueprint";
                    case "AnimBlueprint":
                        return "Animation Blueprint";
                    case "WidgetBlueprint":
                        return "Widget Blueprint";
                    case "MaterialInstanceConstant":
                        return "Material Instance";
                    default:
                        return className;
                }
            }
        }
        
        return "Bilinmiyor"; // Eğer bilinen tiplerden biri bulunamazsa.
    }

    /// <summary>
    /// Bir byte dizisi içinde başka bir byte dizisi arar.
    /// </summary>
    private static int FindByteSequence(byte[] haystack, byte[] needle)
    {
        for (int i = 0; i <= haystack.Length - needle.Length; i++)
        {
            if (haystack.Skip(i).Take(needle.Length).SequenceEqual(needle))
            {
                return i;
            }
        }
        return -1;
    }

    // --- Ana Program ---
    public static void Main(string[] args)
    {
        // --- BURAYI DEĞİŞTİRİN ---
        // Analiz edilecek ana klasörün yolunu belirtin.
        // Genellikle bir Unreal projesinin "Content" klasörü olur.
        string rootPath = @"E:\Works\Unreal\Calypso_LyraFramework\Content"; 

        Console.WriteLine($"Belirtilen yol taranıyor: {rootPath}");
        
        if (!Directory.Exists(rootPath))
        {
            Console.WriteLine("Hata: Belirtilen klasör yolu bulunamadı.");
            return;
        }

        // Directory.GetFiles metodu, alt klasörleri de taramak için SearchOption.AllDirectories kullanır.
        // Sadece .uasset uzantılı dosyaları arar.
        string[] uassetFiles = Directory.GetFiles(rootPath, "*.uasset", SearchOption.AllDirectories);

        if (uassetFiles.Length == 0)
        {
            Console.WriteLine("Belirtilen yolda ve alt klasörlerinde hiç .uasset dosyası bulunamadı.");
            return;
        }

        Console.WriteLine($"Toplam {uassetFiles.Length} adet .uasset dosyası bulundu. Analiz ediliyor...\n");

        int successCount = 0;
        foreach (var filePath in uassetFiles)
        {
            var assetInfo = ParseAssetNameAndClass(filePath);

            if (assetInfo.HasValue)
            {
                Console.WriteLine($"Dosya:<{Path.GetFileName(filePath)}>");
                Console.Write($"->Name:[{assetInfo.Value.AssetName}] \t");
                Console.Write($"->Type:[{assetInfo.Value.AssetClass}]");
                successCount++;
            }
            else
            {
                Console.WriteLine($"Dosya: {Path.GetFileName(filePath)}");
                Console.Write("  -> Bilgiler okunamadı.\n");
            }
        }

        Console.WriteLine($"--- Analiz Tamamlandı ---");
        Console.Write($"{successCount} adet dosya başarıyla işlendi.");
    }
}