#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <algorithm>
#include <cctype>
#include <string>

using json = nlohmann::json;
struct GameInfo {
    std::string name;
    double hours;
};
const std::vector<std::string> IGNORED_KEYWORDS = {
    "demo",
    "teaser",
    "prologue",
    "trial",
    "playable teaser",
    "soundtrack",
    "beta",
    "alpha",
    "test",
    "playtest",
    "server"
};
std::string ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return str;
}
bool ShouldSkipGame(const std::string& gameName) {
    
    std::string lowerName = ToLower(gameName);

    for (const auto& keyword : IGNORED_KEYWORDS) {
        size_t pos = lowerName.find(keyword);

        while (pos != std::string::npos) {
            bool startOk = (pos == 0) || !std::isalnum(static_cast<unsigned char>(lowerName[pos - 1]));
            size_t endPos = pos + keyword.length();
            bool endOk = (endPos == lowerName.length()) || !std::isalnum(static_cast<unsigned char>(lowerName[endPos]));
            if (startOk && endOk) {
                return true;
            }
            pos = lowerName.find(keyword, pos + 1);
        }
    }
    return false;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void PrintLine(int width) {
    std::cout << " " << std::string(width, '-') << std::endl;
}

int main()
{
    std::string apiKey = "D69DA3022BC0A8836311D2B5349F436B"; // Steam Web API Key
    std::string steamId = "76561198060886364";   // Steam ID 64 (örn: 7656119xxxx...)

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string url = "http://api.steampowered.com/IPlayerService/GetOwnedGames/v0001/?key=" + apiKey +
        "&steamid=" + steamId +
        "&format=json&include_appinfo=1&include_played_free_games=1";

    curl = curl_easy_init();
    if (curl)
    {
        std::cout << "Steam connect succesfully." << std::endl;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            std::cerr << "Steam connect failed." << curl_easy_strerror(res) << std::endl;
        }
        else
        {
            try
            {
                auto jsonData = json::parse(readBuffer);

                if (jsonData.contains("response") && jsonData["response"].contains("games"))
                {
                    std::vector<GameInfo> gameList;

                    for (const auto& game : jsonData["response"]["games"])
                    {
                        std::string gameName = "Unknown Game";
                        if (game.contains("name")) gameName = game["name"];

                        if (ShouldSkipGame(gameName)) continue;

                        int playTime = 0;
                        if (game.contains("playtime_forever")) playTime = game["playtime_forever"];
                        double hours = playTime / 60.0;

                        gameList.push_back({ gameName, hours });
                    }

                    std::sort(gameList.begin(), gameList.end(), [](const GameInfo& a, const GameInfo& b) {
                        return a.name < b.name;
                        });

                    const int wIndex = 5;
                    const int wName = 50;
                    const int wTime = 15;
                    const int totalWidth = wIndex + wName + wTime + 7; // Boşluklar ve dikey çizgiler için ek pay

                    std::cout << "\n";
                    PrintLine(totalWidth);
                    std::cout << " | " << std::left << std::setw(wIndex) << "#"
                        << " | " << std::left << std::setw(wName) << "Game"
                        << " | " << std::right << std::setw(wTime) << "Time"
                        << " |" << std::endl;

                    PrintLine(totalWidth);
                    int listedCounter = 0;
                    for (const auto& game : gameList)
                    {
                        std::string displayName = game.name;
                        if (displayName.length() > wName) {
                            displayName = displayName.substr(0, wName - 3) + "...";
                        }

                        listedCounter++;
                        std::cout << " | " << std::left << std::setw(wIndex) << listedCounter
                            << " | " << std::left << std::setw(wName) << displayName
                            << " | " << std::right << std::setw(wTime) << std::fixed << std::setprecision(1) << game.hours
                            << " |" << std::endl;
                    }
                    PrintLine(totalWidth);
                }
                else
                {
                    std::cout << "There is no game or Profile hidden." << std::endl;
                    std::cout << "Data : " << readBuffer << std::endl;
                }
            }
            catch (json::parse_error& e)
            {
                std::cerr << "JSON Error : " << e.what() << std::endl;
            }
        }

        curl_easy_cleanup(curl);
    }
    else
    {
        std::cerr << "CURL coud not start." << std::endl;
    }

    std::cout << "\n...";
    std::cin.get();
    return 0;
}