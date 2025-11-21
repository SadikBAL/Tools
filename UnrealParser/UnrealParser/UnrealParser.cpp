#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <optional>

// Dosya sistemi işlemleri için namespace kısaltması
namespace fs = std::filesystem;

// Parser fonksiyonlarını bir namespace içivnde gruplayalım
namespace UAssetParser 
{
    // Bir dosyayı byte vektörü olarak okuyan yardımcı fonksiyon
    std::vector<char> ReadFileToBytes(const fs::path& filePath) 
    {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return {}; // Dosya açılamazsa boş vektör döndür
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size)) {
            return buffer;
        }

        return {}; // Okuma başarısız olursa boş vektör döndür
    }

    // Byte vektörü içinde belirli bir string'i (byte dizisi olarak) arar
    std::vector<char>::const_iterator FindByteSequence(
        const std::vector<char>& haystack, 
        const std::string& needle)
    {
        return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end());
    }

    // Asset yolunu bulup adını çıkarır
    std::optional<std::string> ExtractAssetNameFromPath(const std::vector<char>& fileBytes) 
    {
        const std::string searchTerm = "/Game/";
        auto it = FindByteSequence(fileBytes, searchTerm);

        if (it != fileBytes.end()) 
        {
            // Null sonlandırıcıyı (0x00) bul
            auto end_it = std::find(it, fileBytes.end(), '\0');
            if (end_it != fileBytes.end()) 
            {
                std::string fullPath(it, end_it);
                return fs::path(fullPath).stem().string(); // .uasset uzantısını kaldırır
            }
        }
        return std::nullopt;
    }

    // Asset sınıfını bulur
    std::optional<std::string> FindTopLevelClass(const std::vector<char>& fileBytes) 
    {
        std::vector<std::string> commonClassNames = {
            // Blueprint'ler ve Mantık (Blueprints & Logic)
            "BlueprintGeneratedClass",
            "AnimBlueprint",
            "WidgetBlueprint",
            "BehaviorTree",
            "BlackboardData",
            "BlueprintFunctionLibrary",
            "BlueprintMacroLibrary",
            "Interface",
            "Enum",
            "ScriptStruct",
            "AIController",

            // 3D Modeller ve Geometri (3D Models & Geometry)
            "StaticMesh",
            "SkeletalMesh",
            "Skeleton",
            "PhysicsAsset",
            "GeometryCache",
            "ControlRig",

            // Materyaller ve Dokular (Materials & Textures)
            "Material",
            "MaterialInstanceConstant",
            "MaterialFunction",
            "Texture2D",
            "TextureCube",
            "RenderTarget",
            "MaterialParameterCollection",
            "SubsurfaceProfile",

            // Animasyon (Animation)
            "AnimSequence",
            "AnimMontage",
            "BlendSpace",
            "BlendSpace1D",
            "AnimComposite",
            "AimOffsetBlendSpace",

            // Ses (Audio)
            "SoundWave",
            "SoundCue",
            "SoundAttenuation",
            "ReverbEffect",
            "SoundConcurrency",
            "SoundMix",
            "DialogueWave",
            "DialogueVoice",
            "AkAudioEvent", // Wwise Plugin

            // Veri ve Yapılandırma (Data & Configuration)
            "DataTable",
            "DataAsset",
            "PrimaryDataAsset",
            "CurveFloat",
            "CurveVector",
            "CurveLinearColor",
            "CurveTable",
            "InputMappingContext",
            "InputAction",

            // Efektler (VFX)
            "NiagaraSystem",
            "NiagaraEmitter",
            "NiagaraScript",
            "ParticleSystem", // Cascade

            // Kullanıcı Arayüzü (UMG)
            "Font",
            "SlateBrushAsset",
    
            // Seviyeler ve Sinematik (Levels & Cinematics)
            "World",
            "LevelSequence",
            "CameraAnim",

            // Diğer (Miscellaneous)
            "PhysicsMaterial",
            "FoliageType",
            "LandscapeGrassType",
            "ForceFeedbackEffect",
            "ObjectLibrary"
        };

        for (const auto& className : commonClassNames) 
        {
            if (FindByteSequence(fileBytes, className) != fileBytes.end())
            {
                if (className == "BlueprintGeneratedClass") return "Blueprint";
                if (className == "AnimBlueprint") return "Animation Blueprint";
                if (className == "WidgetBlueprint") return "Widget Blueprint";
                if (className == "MaterialInstanceConstant") return "Material Instance";
                return className;
            }
        }
        return "Bilinmiyor";
    }

    // Ana parser fonksiyonu
    std::optional<std::pair<std::string, std::string>> ParseAssetNameAndClass(const fs::path& filePath)
    {
        std::vector<char> fileBytes = ReadFileToBytes(filePath);
        if (fileBytes.empty()) {
            std::cerr << "Hata: Dosya okunamadı veya boş: " << filePath.filename().string() << std::endl;
            return std::nullopt;
        }

        auto assetNameOpt = ExtractAssetNameFromPath(fileBytes);
        auto assetClassOpt = FindTopLevelClass(fileBytes);

        if (assetNameOpt.has_value() && assetClassOpt.has_value()) {
            return std::make_pair(assetNameOpt.value(), assetClassOpt.value());
        }

        return std::nullopt;
    }
}

// --- Ana Program ---
int main() 
{
    // --- BURAYI DEĞİŞTİRİN ---
    // C++'da raw string literali (R"()") ters slash'larla uğraşmamak için kullanışlıdır.
    fs::path rootPath = R"(E:\Works\Unreal\Calypso_LyraFramework\Content)";

    std::cout << "Belirtilen yol taranıyor: " << rootPath << std::endl;

    if (!fs::is_directory(rootPath)) {
        std::cerr << "Hata: Belirtilen klasör yolu bulunamadı veya bir klasör değil." << std::endl;
        return 1;
    }

    std::vector<fs::path> uassetFiles;
    // recursive_directory_iterator ile tüm alt klasörleri gez
    for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".uasset") {
            uassetFiles.push_back(entry.path());
        }
    }

    if (uassetFiles.empty()) {
        std::cout << "Belirtilen yolda ve alt klasörlerinde hiç .uasset dosyası bulunamadı." << std::endl;
        return 0;
    }

    std::cout << "Toplam " << uassetFiles.size() << " adet .uasset dosyası bulundu. Analiz ediliyor...\n" << std::endl;

    int counter = 0;
    int successCount = 0;
    for (const auto& filePath : uassetFiles) {
        counter++;
            auto assetInfo = UAssetParser::ParseAssetNameAndClass(filePath);

            if (assetInfo.has_value()) {
                std::cout << counter << " ) Dosya: <" << filePath.filename().string() << ">";
                std::cout << "->Name: [" << assetInfo->first << "] \t";
                std::cout << "->Type: [" << assetInfo->second << "]" << std::endl;
                successCount++;
            } else {
                std::cout << counter << " ) Dosya: " << filePath.filename().string();
                std::cout << "  -> Bilgiler okunamadı." << std::endl;
            }
    }
    
    std::cout << "--- Analiz Tamamlandı ---" << std::endl;
    std::cout << successCount << " adet dosya başarıyla işlendi." << std::endl;

    return 0;
}