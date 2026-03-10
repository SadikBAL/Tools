#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <optional>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <map>
#include <iomanip>
#include <chrono>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Binary parsing
// ---------------------------------------------------------------------------
namespace UAssetParser
{
    std::vector<char> ReadFileToBytes(const fs::path& filePath)
    {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return {};
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size)) return buffer;
        return {};
    }

    std::vector<char>::const_iterator FindByteSequence(
        const std::vector<char>& haystack, const std::string& needle)
    {
        return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end());
    }

    std::optional<std::string> ExtractAssetNameFromPath(const std::vector<char>& fileBytes)
    {
        const std::string searchTerm = "/Game/";
        auto it = FindByteSequence(fileBytes, searchTerm);
        if (it != fileBytes.end())
        {
            auto end_it = std::find(it, fileBytes.end(), '\0');
            if (end_it != fileBytes.end())
                return fs::path(std::string(it, end_it)).stem().string();
        }
        return std::nullopt;
    }

    std::optional<std::string> FindTopLevelClass(const std::vector<char>& fileBytes)
    {
        // Daha spesifik stringler önce (substring çakışmalarını önlemek için)
        static const std::vector<std::pair<std::string, std::string>> classMap = {
            // Blueprint
            { "AnimBlueprint",                  "Animation Blueprint"           },
            { "WidgetBlueprint",                "Widget Blueprint"              },
            { "BlueprintGeneratedClass",        "Blueprint"                     },
            { "BlueprintFunctionLibrary",       "Blueprint Function Library"    },
            { "BlueprintMacroLibrary",          "Blueprint Macro Library"       },
            // AI
            { "BehaviorTree",                   "Behavior Tree"                 },
            { "BlackboardData",                 "Blackboard"                    },
            { "EnvQuery",                       "EQS Query"                     },
            { "AIController",                   "AI Controller"                 },
            // Kullanici Tanimli
            { "UserDefinedEnum",                "Enum"                          },
            { "UserDefinedStruct",              "Struct"                        },
            // 3D Modeller
            { "SkeletalMesh",                   "Skeletal Mesh"                 },
            { "StaticMesh",                     "Static Mesh"                   },
            { "GeometryCache",                  "Geometry Cache"                },
            // Fizik
            { "PhysicsAsset",                   "Physics Asset"                 },
            { "PhysicalMaterial",               "Physical Material"             },
            // Animasyon (spesifik once)
            { "AimOffsetBlendSpace",            "Aim Offset Blend Space"        },
            { "BlendSpace1D",                   "Blend Space 1D"                },
            { "BlendSpace",                     "Blend Space"                   },
            { "AnimSequence",                   "Anim Sequence"                 },
            { "AnimMontage",                    "Anim Montage"                  },
            { "AnimComposite",                  "Anim Composite"                },
            { "PoseAsset",                      "Pose Asset"                    },
            { "Skeleton",                       "Skeleton"                      },
            { "ControlRig",                     "Control Rig"                   },
            // Materyaller (spesifik once)
            { "MaterialInstanceConstant",       "Material Instance"             },
            { "MaterialInstanceDynamic",        "Material Instance Dynamic"     },
            { "MaterialParameterCollection",    "Material Parameter Collection" },
            { "MaterialFunction",               "Material Function"             },
            { "Material",                       "Material"                      },
            { "SubsurfaceProfile",              "Subsurface Profile"            },
            // Dokular (spesifik once)
            { "TextureRenderTargetCube",        "Render Target Cube"            },
            { "TextureRenderTarget2D",          "Render Target"                 },
            { "Texture2DArray",                 "Texture 2D Array"              },
            { "TextureCube",                    "Texture Cube"                  },
            { "VolumeTexture",                  "Volume Texture"                },
            { "MediaTexture",                   "Media Texture"                 },
            { "Texture2D",                      "Texture 2D"                    },
            // Ses (spesifik once)
            { "MetaSoundSource",                "MetaSound Source"              },
            { "SoundAttenuation",               "Sound Attenuation"             },
            { "SoundConcurrency",               "Sound Concurrency"             },
            { "SoundClass",                     "Sound Class"                   },
            { "SoundMix",                       "Sound Mix"                     },
            { "SoundWave",                      "Sound Wave"                    },
            { "SoundCue",                       "Sound Cue"                     },
            { "ReverbEffect",                   "Reverb Effect"                 },
            { "DialogueWave",                   "Dialogue Wave"                 },
            { "DialogueVoice",                  "Dialogue Voice"                },
            { "AkAudioEvent",                   "Wwise Audio Event"             },
            // VFX (spesifik once)
            { "NiagaraParameterCollection",     "Niagara Parameter Collection"  },
            { "NiagaraSystem",                  "Niagara System"                },
            { "NiagaraEmitter",                 "Niagara Emitter"               },
            { "NiagaraScript",                  "Niagara Script"                },
            { "ParticleSystem",                 "Particle System (Cascade)"     },
            // Veri (spesifik once)
            { "PrimaryDataAsset",               "Primary Data Asset"            },
            { "DataTable",                      "Data Table"                    },
            { "CurveTable",                     "Curve Table"                   },
            { "CurveLinearColor",               "Curve Linear Color"            },
            { "CurveVector",                    "Curve Vector"                  },
            { "CurveFloat",                     "Curve Float"                   },
            { "DataAsset",                      "Data Asset"                    },
            { "StringTable",                    "String Table"                  },
            // Input
            { "InputMappingContext",            "Input Mapping Context"         },
            { "InputAction",                    "Input Action"                  },
            // UI / Font (spesifik once)
            { "FontFace",                       "Font Face"                     },
            { "Font",                           "Font"                          },
            { "SlateBrushAsset",                "Slate Brush Asset"             },
            { "SlateWidgetStyleAsset",          "Slate Widget Style"            },
            // Sinematik
            { "LevelSequence",                  "Level Sequence"                },
            { "TemplateSequence",               "Template Sequence"             },
            // Dunya
            { "World",                          "World / Level"                 },
            // Medya
            { "FileMediaSource",                "File Media Source"             },
            { "MediaPlayer",                    "Media Player"                  },
            // Cevre
            { "LandscapeLayerInfoObject",       "Landscape Layer"               },
            { "LandscapeGrassType",             "Landscape Grass Type"          },
            { "FoliageType",                    "Foliage Type"                  },
            // Diger
            { "ForceFeedbackEffect",            "Force Feedback Effect"         },
            { "TouchInterface",                 "Touch Interface"               },
            { "ObjectLibrary",                  "Object Library"                },
            { "CameraAnim",                     "Camera Anim"                   },
        };

        for (const auto& [key, label] : classMap)
            if (FindByteSequence(fileBytes, key) != fileBytes.end())
                return label;

        return std::nullopt;
    }

    std::optional<std::pair<std::string, std::string>> ParseAssetNameAndClass(const fs::path& filePath)
    {
        std::vector<char> fileBytes = ReadFileToBytes(filePath);
        if (fileBytes.empty()) return std::nullopt;
        auto name  = ExtractAssetNameFromPath(fileBytes);
        auto klass = FindTopLevelClass(fileBytes);
        if (name && klass) return std::make_pair(*name, *klass);
        return std::nullopt;
    }
}

// ---------------------------------------------------------------------------
// Naming convention
// ---------------------------------------------------------------------------
struct NamingRule { std::string prefix; std::string suffix; };

static const std::map<std::string, NamingRule> k_namingRules = {
    // AI
    { "AI Controller",               { "AIC_",    ""       } },
    { "Behavior Tree",               { "BT_",     ""       } },
    { "Blackboard",                  { "BB_",     ""       } },
    { "EQS Query",                   { "EQS_",    ""       } },
    // Animation
    { "Animation Blueprint",         { "ABP_",    ""       } },
    { "Anim Composite",              { "AC_",     ""       } },
    { "Anim Montage",                { "AM_",     ""       } },
    { "Anim Sequence",               { "AS_",     ""       } },
    { "Blend Space",                 { "BS_",     ""       } },
    { "Blend Space 1D",              { "BS_",     ""       } },
    { "Aim Offset Blend Space",      { "AO_",     ""       } },
    { "Level Sequence",              { "LS_",     ""       } },
    { "Skeletal Mesh",               { "SK_",     ""       } },
    { "Skeleton",                    { "SKEL_",   ""       } },
    { "Control Rig",                 { "CR_",     ""       } },
    // Blueprint
    { "Blueprint",                   { "BP_",     ""       } },
    { "Widget Blueprint",            { "WBP_",    ""       } },
    { "Blueprint Function Library",  { "BPFL_",   ""       } },
    { "Blueprint Macro Library",     { "BPML_",   ""       } },
    { "Enum",                        { "E",       ""       } },
    { "Struct",                      { "S",       ""       } },
    // Audio
    { "Dialogue Voice",              { "DV_",     ""       } },
    { "Dialogue Wave",               { "DW_",     ""       } },
    { "Reverb Effect",               { "Reverb_", ""       } },
    { "Sound Attenuation",           { "ATT_",    ""       } },
    { "Sound Concurrency",           { "",        "_SC"    } },
    { "Sound Cue",                   { "A_",      "_Cue"   } },
    { "Sound Mix",                   { "Mix_",    ""       } },
    { "Sound Wave",                  { "A_",      ""       } },
    // Enhanced Input
    { "Input Action",                { "IA_",     ""       } },
    { "Input Mapping Context",       { "IMC_",    ""       } },
    // Material & Texture
    { "Texture 2D",                  { "T_",      ""       } },
    { "Material",                    { "M_",      ""       } },
    { "Material Function",           { "MF_",     ""       } },
    { "Material Instance",           { "MI_",     ""       } },
    { "Material Parameter Collection",{ "MPC_",   ""       } },
    { "Subsurface Profile",          { "SP_",     ""       } },
    // Niagara
    { "Niagara System",              { "NS_",     ""       } },
    { "Niagara Emitter",             { "NE_",     ""       } },
    { "Niagara Parameter Collection",{ "NPC_",    ""       } },
    // Data
    { "Data Table",                  { "DT_",     ""       } },
    { "Data Asset",                  { "DA_",     ""       } },
    { "Primary Data Asset",          { "DA_",     ""       } },
    { "Curve Table",                 { "Curve_",  "_Table" } },
    { "Curve Linear Color",          { "Curve_",  "_Color" } },
    { "Curve Vector",                { "Curve_",  "_Vector"} },
    { "Curve Float",                 { "Curve_",  ""       } },
    // Misc
    { "Camera Anim",                 { "CA_",     ""       } },
    { "File Media Source",           { "FMS_",    ""       } },
    { "Foliage Type",                { "FT_",     ""       } },
    { "Force Feedback Effect",       { "FFE_",    ""       } },
    { "Landscape Grass Type",        { "LG_",     ""       } },
    { "Landscape Layer",             { "LL_",     ""       } },
    { "World / Level",               { "L_",      ""       } },
    { "Media Player",                { "MP_",     ""       } },
    { "Object Library",              { "OL_",     ""       } },
    { "Static Mesh",                 { "SM_",     ""       } },
    { "Physics Asset",               { "PHYS_",   ""       } },
    { "Touch Interface",             { "TL",      ""       } },
};

bool CheckNaming(const std::string& stem, const NamingRule& rule)
{
    bool prefixOk = rule.prefix.empty() ||
                    stem.rfind(rule.prefix, 0) == 0;
    bool suffixOk = rule.suffix.empty() ||
                    (stem.size() >= rule.suffix.size() &&
                     stem.compare(stem.size() - rule.suffix.size(),
                                  rule.suffix.size(), rule.suffix) == 0);
    return prefixOk && suffixOk;
}

// ---------------------------------------------------------------------------
// Console helpers
// ---------------------------------------------------------------------------
void PrintProgressBar(int current, int total, int barWidth = 50)
{
    float progress = total > 0 ? (float)current / total : 1.0f;
    int filled = (int)(progress * barWidth);
    std::cout << "\r[";
    for (int i = 0; i < barWidth; i++) {
        if (i < filled)       std::cout << '=';
        else if (i == filled) std::cout << '>';
        else                  std::cout << ' ';
    }
    std::cout << "] " << std::setw(3) << (int)(progress * 100) << "%"
              << " (" << current << "/" << total << ")";
    std::cout.flush();
}

void PrintSummaryTable(const std::map<std::string, int>& typeCounts, int total)
{
    size_t maxLen = std::string("Asset Tipi").size();
    for (const auto& [type, _] : typeCounts)
        maxLen = std::max(maxLen, type.size());

    std::string line = "+" + std::string(maxLen + 2, '-') + "+--------+--------+";
    std::cout << "\n\n" << line << "\n";
    std::cout << "| " << std::left << std::setw((int)maxLen) << "Asset Tipi"
              << " | " << std::setw(6) << "Adet"
              << " | " << std::setw(6) << "%    |\n";
    std::cout << line << "\n";

    for (const auto& [type, count] : typeCounts) {
        float pct = total > 0 ? (float)count / total * 100.0f : 0.0f;
        std::cout << "| " << std::left  << std::setw((int)maxLen) << type
                  << " | " << std::right << std::setw(6) << count
                  << " | " << std::right << std::setw(5)
                  << std::fixed << std::setprecision(1) << pct << "% |\n";
    }
    std::cout << line << "\n";
    std::cout << "| " << std::left << std::setw((int)maxLen) << "TOPLAM"
              << " | " << std::right << std::setw(6) << total
              << " | " << std::right << std::setw(5) << "100.0% |\n";
    std::cout << line << "\n";
}

struct BadAsset
{
    std::string filename;
    std::string relativePath;
    std::string assetType;
    std::string expectedPrefix;
    std::string expectedSuffix;
};

void PrintNamingErrors(const std::vector<BadAsset>& errors, const fs::path& rootPath)
{
    if (errors.empty()) {
        std::cout << "\nTum dosyalar isimlendirme kuralina uygun!\n";
        return;
    }

    // Sutun genislikleri
    size_t wFile = std::string("Dosya Adi").size();
    size_t wType = std::string("Asset Tipi").size();
    size_t wExp  = std::string("Beklenen").size();
    size_t wPath = std::string("Yol").size();

    for (const auto& e : errors) {
        wFile = std::max(wFile, e.filename.size());
        wType = std::max(wType, e.assetType.size());
        std::string exp = e.expectedPrefix + "*" + e.expectedSuffix;
        wExp  = std::max(wExp, exp.size());
        wPath = std::max(wPath, e.relativePath.size());
    }

    std::string line = "+" + std::string(wFile + 2, '-')
                     + "+" + std::string(wType + 2, '-')
                     + "+" + std::string(wExp  + 2, '-')
                     + "+" + std::string(wPath + 2, '-') + "+";

    std::cout << "\n\n=== Yanlis Isimlendirilen Dosyalar: " << errors.size() << " adet ===\n";
    std::cout << line << "\n";
    std::cout << "| " << std::left << std::setw((int)wFile) << "Dosya Adi"
              << " | " << std::setw((int)wType) << "Asset Tipi"
              << " | " << std::setw((int)wExp)  << "Beklenen"
              << " | " << std::setw((int)wPath)  << "Yol"
              << " |\n";
    std::cout << line << "\n";

    for (const auto& e : errors) {
        std::string exp = e.expectedPrefix + "*" + e.expectedSuffix;
        std::cout << "| " << std::left << std::setw((int)wFile) << e.filename
                  << " | " << std::setw((int)wType) << e.assetType
                  << " | " << std::setw((int)wExp)  << exp
                  << " | " << std::setw((int)wPath)  << e.relativePath
                  << " |\n";
    }
    std::cout << line << "\n";
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    fs::path exeDir   = fs::path(argv[0]).parent_path();
    fs::path rootPath = exeDir / "Content";

    std::cout << "Taranan dizin: " << rootPath << "\n";

    if (!fs::is_directory(rootPath)) {
        std::cerr << "Hata: Content klasoru bulunamadi: " << rootPath << "\n";
        return 1;
    }

    // Dosyalari topla
    std::vector<fs::path> uassetFiles;
    for (const auto& entry : fs::recursive_directory_iterator(rootPath))
        if (entry.is_regular_file() && entry.path().extension() == ".uasset")
            uassetFiles.push_back(entry.path());

    if (uassetFiles.empty()) {
        std::cout << "Content klasorunde .uasset dosyasi bulunamadi.\n";
        return 0;
    }

    int total = (int)uassetFiles.size();
    std::cout << total << " adet .uasset dosyasi bulundu. Analiz ediliyor...\n\n";

    // Paralel islem
    unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
    std::atomic<int> processedCount{ 0 };
    std::mutex resultsMutex;

    std::map<std::string, int> typeCounts;
    std::vector<BadAsset> badAssets;

    int chunkSize = (total + (int)threadCount - 1) / (int)threadCount;
    std::vector<std::future<void>> futures;

    for (int t = 0; t < (int)threadCount; t++) {
        int start = t * chunkSize;
        int end   = std::min(start + chunkSize, total);
        if (start >= total) break;

        futures.push_back(std::async(std::launch::async, [&, start, end]() {
            for (int i = start; i < end; i++) {
                const auto& path   = uassetFiles[i];
                auto result        = UAssetParser::ParseAssetNameAndClass(path);
                std::string stem   = path.stem().string();
                std::string relPath = fs::relative(path, rootPath).string();

                {
                    std::lock_guard<std::mutex> lock(resultsMutex);
                    std::string type = result.has_value() ? result->second : "Bilinmiyor";
                    typeCounts[type]++;

                    // Isimlendirme kontrolu
                    auto ruleIt = k_namingRules.find(type);
                    if (ruleIt != k_namingRules.end()) {
                        if (!CheckNaming(stem, ruleIt->second)) {
                            badAssets.push_back({
                                stem, relPath, type,
                                ruleIt->second.prefix,
                                ruleIt->second.suffix
                            });
                        }
                    }
                }
                processedCount++;
            }
        }));
    }

    // Progress bar
    while (processedCount < total) {
        PrintProgressBar(processedCount.load(), total);
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
    PrintProgressBar(total, total);
    for (auto& f : futures) f.get();

    // Asset tipi ozet tablosu
    PrintSummaryTable(typeCounts, total);

    // Isimlendirme hatalari
    PrintNamingErrors(badAssets, rootPath);

    std::cout << "\nCikmak icin bir tusa basin...";
    std::cin.get();
    return 0;
}
