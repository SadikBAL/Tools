#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <algorithm>
#include <cctype>
#include <iomanip>

using json = nlohmann::json;
namespace fs = std::filesystem;

// ============================================================
//  PLATFORM TANIMI
// ============================================================

enum class Platform { Steam, Epic, PlayStation, Nintendo, Xbox, GOG, Ubisoft };

struct PlatformInfo {
    Platform id;
    std::string key;    // platforms.json'daki anahtar
    std::string name;   // Görünen ad
    std::string tag;    // Tablodaki kısa etiket
};

const std::vector<PlatformInfo> PLATFORMS = {
    { Platform::Steam,       "steam",       "Steam",           "[STM]" },
    { Platform::Epic,        "epic",        "Epic Games",      "[EPC]" },
    { Platform::PlayStation, "playstation", "PlayStation",     "[PS ] " },
    { Platform::Nintendo,    "nintendo",    "Nintendo Switch", "[NSW]" },
    { Platform::Xbox,        "xbox",        "Xbox",            "[XBX]" },
    { Platform::GOG,         "gog",         "GOG",             "[GOG]" },
    { Platform::Ubisoft,     "ubisoft",     "Ubisoft Connect", "[UBI]" },
};

const PlatformInfo& GetPlatformInfo(Platform p) {
    for (const auto& pi : PLATFORMS)
        if (pi.id == p) return pi;
    return PLATFORMS[0];
}

// ============================================================
//  GAME INFO
// ============================================================

struct GameInfo {
    std::string name;
    double hours;
    Platform platform;
    std::string franchise;
    int franchiseOrder;
};

// ============================================================
//  FRANCHISE KURALLARI
//  Daha spesifik (uzun) keyword'ler üstte olmalı.
//  İlk eşleşen kural kullanılır.
// ============================================================

struct FranchiseRule {
    std::string keyword;
    std::string franchise;
    int order;
};

const std::vector<FranchiseRule> FRANCHISE_RULES = {
    // Batman: Arkham
    { "Batman: Arkham Asylum",          "Batman: Arkham",     1 },
    { "Batman: Arkham City",            "Batman: Arkham",     2 },
    { "Batman: Arkham Origins",         "Batman: Arkham",     3 },
    { "Batman: Arkham Knight",          "Batman: Arkham",     4 },

    // The Witcher
    { "The Witcher 2",                  "The Witcher",        2 },
    { "The Witcher 3",                  "The Witcher",        3 },
    { "The Witcher",                    "The Witcher",        1 },

    // Soulsborne
    { "Demon's Souls",                  "Soulsborne",         1 },
    { "Dark Souls Remastered",          "Soulsborne",         2 },
    { "Dark Souls II",                  "Soulsborne",         3 },
    { "Dark Souls III",                 "Soulsborne",         4 },
    { "Bloodborne",                     "Soulsborne",         5 },
    { "Sekiro",                         "Soulsborne",         6 },
    { "Elden Ring",                     "Soulsborne",         7 },

    // Assassin's Creed
    { "Assassin's Creed II",            "Assassin's Creed",   2 },
    { "Assassin's Creed: Brotherhood",  "Assassin's Creed",   3 },
    { "Assassin's Creed: Revelations",  "Assassin's Creed",   4 },
    { "Assassin's Creed III",           "Assassin's Creed",   5 },
    { "Assassin's Creed IV",            "Assassin's Creed",   6 },
    { "Assassin's Creed Rogue",         "Assassin's Creed",   7 },
    { "Assassin's Creed Unity",         "Assassin's Creed",   8 },
    { "Assassin's Creed Syndicate",     "Assassin's Creed",   9 },
    { "Assassin's Creed Origins",       "Assassin's Creed",  10 },
    { "Assassin's Creed Odyssey",       "Assassin's Creed",  11 },
    { "Assassin's Creed Valhalla",      "Assassin's Creed",  12 },
    { "Assassin's Creed Mirage",        "Assassin's Creed",  13 },
    { "Assassin's Creed Shadows",       "Assassin's Creed",  14 },
    { "Assassin's Creed",               "Assassin's Creed",   1 },

    // Buraya yeni franchise'lar ekle...
};

// ============================================================
//  IGNORED KEYWORDS (demo, beta, vs.)
// ============================================================

const std::vector<std::string> IGNORED_KEYWORDS = {
    "demo", "teaser", "prologue", "trial", "playable teaser",
    "soundtrack", "beta", "alpha", "test", "playtest", "server"
};

// ============================================================
//  YARDIMCI FONKSİYONLAR
// ============================================================

std::string ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return str;
}

bool ShouldSkipGame(const std::string& gameName) {
    std::string lower = ToLower(gameName);
    for (const auto& kw : IGNORED_KEYWORDS) {
        size_t pos = lower.find(kw);
        while (pos != std::string::npos) {
            bool startOk = (pos == 0) || !std::isalnum((unsigned char)lower[pos - 1]);
            size_t end   = pos + kw.length();
            bool endOk   = (end == lower.length()) || !std::isalnum((unsigned char)lower[end]);
            if (startOk && endOk) return true;
            pos = lower.find(kw, pos + 1);
        }
    }
    return false;
}

void AssignFranchise(GameInfo& game) {
    std::string lower = ToLower(game.name);
    for (const auto& rule : FRANCHISE_RULES) {
        if (lower.find(ToLower(rule.keyword)) != std::string::npos) {
            game.franchise     = rule.franchise;
            game.franchiseOrder = rule.order;
            return;
        }
    }
    game.franchise      = "Other";
    game.franchiseOrder = 0;
}

// ============================================================
//  CONFIG (platforms.json)
// ============================================================

const std::string CONFIG_FILE = "platforms.json";
json gConfig;

void LoadConfig() {
    std::ifstream f(CONFIG_FILE);
    if (f.is_open()) {
        try { gConfig = json::parse(f); }
        catch (...) { gConfig = json::object(); }
    } else {
        gConfig = json::object();
    }
}

void SaveConfig() {
    std::ofstream f(CONFIG_FILE);
    f << gConfig.dump(4);
}

bool IsLoggedIn(const PlatformInfo& pi) {
    return gConfig.contains(pi.key) && gConfig[pi.key].value("logged_in", false);
}

// ============================================================
//  HTTP / CURL
// ============================================================

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string HttpGet(const std::string& url, const std::vector<std::string>& headers = {}) {
    CURL* curl = curl_easy_init();
    std::string result;
    if (curl) {
        struct curl_slist* headerList = nullptr;
        for (const auto& h : headers)
            headerList = curl_slist_append(headerList, h.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        if (headerList) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

        curl_easy_perform(curl);
        curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);
    }
    return result;
}

// ============================================================
//  AUTH FLOWLARI
// ============================================================

void AuthSteam() {
    std::cout << "\n  [Steam] Baglanti Kurma\n";
    std::cout << "  API key icin: https://steamcommunity.com/dev/apikey\n\n";

    std::cout << "  API Key: ";
    std::string apiKey; std::cin >> apiKey;

    std::cout << "  Steam ID (64-bit): ";
    std::string steamId; std::cin >> steamId;

    std::cout << "\n  Baglanti test ediliyor...\n";
    std::string url = "http://api.steampowered.com/IPlayerService/GetOwnedGames/v0001/?key="
        + apiKey + "&steamid=" + steamId + "&format=json&include_appinfo=1";
    std::string resp = HttpGet(url);

    try {
        auto j = json::parse(resp);
        if (j.contains("response") && j["response"].contains("games")) {
            int count = j["response"]["game_count"];
            gConfig["steam"] = {
                {"logged_in", true},
                {"api_key",   apiKey},
                {"steam_id",  steamId}
            };
            SaveConfig();
            std::cout << "  [OK] Steam baglandi! " << count << " oyun bulundu.\n";
        } else {
            std::cout << "  [HATA] Gecersiz API key veya Steam ID.\n";
        }
    } catch (...) {
        std::cout << "  [HATA] Baglanti saglanamadi.\n";
    }
}

void AuthEpic() {
    std::cout << "\n  [Epic Games] Baglanti Kurma\n";
    std::cout << "  (Giris gerekmez, yerel dosyalar okunur)\n\n";

    std::string defaultPath = "C:\\ProgramData\\Epic\\EpicGamesLauncher\\Data\\Manifests";

    if (fs::exists(defaultPath)) {
        gConfig["epic"] = {
            {"logged_in",      true},
            {"manifest_path",  defaultPath}
        };
        SaveConfig();
        std::cout << "  [OK] Epic Games kutuphanesi bulundu.\n";
    } else {
        std::cout << "  Varsayilan konum bulunamadi. Manifests klasor yolunu gir:\n  > ";
        std::string path;
        std::cin.ignore();
        std::getline(std::cin, path);

        if (fs::exists(path)) {
            gConfig["epic"] = {
                {"logged_in",     true},
                {"manifest_path", path}
            };
            SaveConfig();
            std::cout << "  [OK] Epic Games kutuphanesi bulundu.\n";
        } else {
            std::cout << "  [HATA] Klasor bulunamadi.\n";
        }
    }
}

void AuthPSN() {
    std::cout << "\n  [PlayStation] Baglanti Kurma\n\n";
    std::cout << "  1. Tarayicinizda su adrese gidin ve PSN'e giris yapin:\n";
    std::cout << "     https://store.playstation.com/\n\n";
    std::cout << "  2. Giris yaptiktan sonra su adrese gidin:\n";
    std::cout << "     https://ca.account.sony.com/api/v1/ssocookie\n\n";
    std::cout << "  3. Sayfada gorunen JSON icindeki 'npsso' degerini kopyalayin.\n\n";
    std::cout << "  NPSSO: ";

    std::string npsso;
    std::cin.ignore();
    std::getline(std::cin, npsso);

    if (npsso.empty()) {
        std::cout << "  [HATA] Deger bos.\n";
        return;
    }

    gConfig["playstation"] = {
        {"logged_in", true},
        {"npsso",     npsso}
    };
    SaveConfig();
    std::cout << "  [OK] PSN bilgileri kaydedildi.\n";
    std::cout << "  (Oyun cekme destegi yakin zamanda eklenecek)\n";
}

void AuthComingSoon(const PlatformInfo& pi) {
    std::cout << "\n  [" << pi.name << "] entegrasyonu yakin zamanda eklenecek.\n";
}

void RunAuth(const PlatformInfo& pi) {
    switch (pi.id) {
        case Platform::Steam:       AuthSteam();           break;
        case Platform::Epic:        AuthEpic();            break;
        case Platform::PlayStation: AuthPSN();             break;
        default:                    AuthComingSoon(pi);    break;
    }
}

// ============================================================
//  GAME PROVIDERS
// ============================================================

std::vector<GameInfo> FetchSteamGames() {
    std::vector<GameInfo> games;

    std::string apiKey  = gConfig["steam"]["api_key"];
    std::string steamId = gConfig["steam"]["steam_id"];

    std::string url = "http://api.steampowered.com/IPlayerService/GetOwnedGames/v0001/?key="
        + apiKey + "&steamid=" + steamId
        + "&format=json&include_appinfo=1&include_played_free_games=1";

    std::string resp = HttpGet(url);

    try {
        auto j = json::parse(resp);
        if (!j.contains("response") || !j["response"].contains("games")) return games;

        for (const auto& game : j["response"]["games"]) {
            std::string name = game.value("name", "Unknown Game");
            if (ShouldSkipGame(name)) continue;

            int playtime = game.value("playtime_forever", 0);
            GameInfo info{ name, playtime / 60.0, Platform::Steam, "", 0 };
            AssignFranchise(info);
            games.push_back(info);
        }
    } catch (...) {}

    return games;
}

std::vector<GameInfo> FetchEpicGames() {
    std::vector<GameInfo> games;

    std::string manifestPath = gConfig["epic"]["manifest_path"];
    if (!fs::exists(manifestPath)) return games;

    for (const auto& entry : fs::directory_iterator(manifestPath)) {
        if (entry.path().extension() != ".item") continue;

        std::ifstream f(entry.path());
        try {
            auto j = json::parse(f);

            // Sadece oyunlar (tool, DLC degil)
            if (!j.value("bIsApplication", false)) continue;

            std::string name = j.value("DisplayName", "");
            if (name.empty() || ShouldSkipGame(name)) continue;

            GameInfo info{ name, 0.0, Platform::Epic, "", 0 };
            AssignFranchise(info);
            games.push_back(info);
        } catch (...) {}
    }

    return games;
}

// ============================================================
//  DISPLAY
// ============================================================

const int W_INDEX = 4;
const int W_TAG   = 6;
const int W_NAME  = 48;
const int W_TIME  = 8;
const int TOTAL_W = W_INDEX + W_TAG + W_NAME + W_TIME + 11;

void PrintLine() {
    std::cout << " " << std::string(TOTAL_W, '-') << "\n";
}

void PrintFranchiseHeader(const std::string& franchise) {
    std::string label = "  " + franchise + "  ";
    int pad = TOTAL_W - (int)label.size() - 2;
    if (pad < 0) pad = 0;
    std::cout << " |" << label << std::string(pad, ' ') << "|\n";
}

void DisplayGames(std::vector<GameInfo>& gameList) {
    // Sıralama: franchise A-Z → order → oyun adı (Other en sona)
    std::sort(gameList.begin(), gameList.end(), [](const GameInfo& a, const GameInfo& b) {
        bool aOther = (a.franchise == "Other");
        bool bOther = (b.franchise == "Other");
        if (aOther != bOther) return !aOther;
        if (a.franchise != b.franchise) return a.franchise < b.franchise;
        if (a.franchiseOrder != b.franchiseOrder) return a.franchiseOrder < b.franchiseOrder;
        return a.name < b.name;
    });

    std::cout << "\n";
    PrintLine();
    std::cout << " | " << std::left  << std::setw(W_INDEX) << "#"
              << " | " << std::left  << std::setw(W_TAG)   << "Platf"
              << " | " << std::left  << std::setw(W_NAME)  << "Oyun"
              << " | " << std::right << std::setw(W_TIME)  << "Saat"
              << " |\n";
    PrintLine();

    std::string currentFranchise;
    int counter = 0;

    for (const auto& game : gameList) {
        if (game.franchise != currentFranchise) {
            if (!currentFranchise.empty()) PrintLine();
            PrintFranchiseHeader(game.franchise);
            PrintLine();
            currentFranchise = game.franchise;
        }

        std::string displayName = game.name;
        if (displayName.length() > W_NAME)
            displayName = displayName.substr(0, W_NAME - 3) + "...";

        const std::string& tag = GetPlatformInfo(game.platform).tag;

        std::cout << " | " << std::left  << std::setw(W_INDEX) << ++counter
                  << " | " << std::left  << std::setw(W_TAG)   << tag
                  << " | " << std::left  << std::setw(W_NAME)  << displayName
                  << " | " << std::right << std::setw(W_TIME)
                  << std::fixed << std::setprecision(1) << game.hours
                  << " |\n";
    }

    PrintLine();
    std::cout << "  Toplam: " << counter << " oyun\n";
}

// ============================================================
//  ANA MENU
// ============================================================

void ShowMenu() {
    std::cout << "\n ================================\n";
    std::cout << "  GamerLibrary\n";
    std::cout << " ================================\n\n";
    std::cout << "  Platform Ekle / Yonet:\n\n";

    for (int i = 0; i < (int)PLATFORMS.size(); i++) {
        const auto& pi = PLATFORMS[i];
        std::string status = IsLoggedIn(pi) ? "[v Bagli  ]" : "[  -      ]";
        std::cout << "  " << (i + 1) << ". "
                  << std::left << std::setw(18) << pi.name
                  << " " << status << "\n";
    }

    std::cout << "\n  0. Kütüphaneyi Göster\n";
    std::cout << "  q. Cikis\n";
    std::cout << "\n  Secin: ";
}

// ============================================================
//  MAIN
// ============================================================

int main() {
    LoadConfig();

    std::string input;
    while (true) {
        ShowMenu();
        std::cin >> input;

        if (input == "q" || input == "Q") break;

        if (input == "0") {
            std::vector<GameInfo> allGames;

            if (IsLoggedIn(GetPlatformInfo(Platform::Steam))) {
                std::cout << "\n  Steam oyunlari yukluyor...\n";
                auto g = FetchSteamGames();
                allGames.insert(allGames.end(), g.begin(), g.end());
            }
            if (IsLoggedIn(GetPlatformInfo(Platform::Epic))) {
                std::cout << "  Epic oyunlari yukluyor...\n";
                auto g = FetchEpicGames();
                allGames.insert(allGames.end(), g.begin(), g.end());
            }

            if (allGames.empty()) {
                std::cout << "\n  Hicbir platforma baglanilmamis.\n";
            } else {
                DisplayGames(allGames);
            }

            std::cout << "\n  Devam etmek icin Enter'a basin...";
            std::cin.ignore(); std::cin.get();
            continue;
        }

        try {
            int choice = std::stoi(input) - 1;
            if (choice >= 0 && choice < (int)PLATFORMS.size()) {
                RunAuth(PLATFORMS[choice]);
                std::cout << "\n  Devam etmek icin Enter'a basin...";
                std::cin.ignore(); std::cin.get();
            } else {
                std::cout << "  Gecersiz secim.\n";
            }
        } catch (...) {
            std::cout << "  Gecersiz giris.\n";
        }
    }

    return 0;
}
