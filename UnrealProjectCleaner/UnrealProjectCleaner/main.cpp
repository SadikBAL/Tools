#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <limits>
#include <sstream>
#include <future>
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

namespace fs = std::filesystem;

// ANSI color codes (requires Windows 10+)
#define COL_RESET   "\033[0m"
#define COL_BOLD    "\033[1m"
#define COL_GREY    "\033[90m"
#define COL_RED     "\033[91m"
#define COL_GREEN   "\033[92m"
#define COL_YELLOW  "\033[93m"
#define COL_CYAN    "\033[96m"
#define COL_WHITE   "\033[97m"

const std::vector<std::string> WHITELIST = {
    "ProjectFiles",
    "Wwise",
    "WwiseNiagara",
    "WwiseSoundEngine",
    "PlayFab",
    "Common",
    "CommonUsers",
    "CommonGame",
    "AsyncMixin",
    "GameFeatures",
    "UIExtension",
    "GameplayMessageRouter",
    "ModularGameplayActors",
    "MultiplayerSessions",
    "ProceduralLevelGraph",
    "TagBaseView"
};

// One entry per project/plugin — groups Binaries + Intermediate together
struct GroupInfo
{
    std::string name;             // project name or plugin name
    fs::path    binariesPath;     // empty if not present
    fs::path    intermediatePath; // empty if not present
    int         binariesCount    = 0;
    int         intermediateCount = 0;
    bool        selected         = true;

    int totalCount() const { return binariesCount + intermediateCount; }
};

// ----------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------

void EnableAnsiColors()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode  = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

fs::path GetExeDir()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
}

// Fast file counter using native Windows API
int CountFiles(const std::wstring& dir)
{
    int count = 0;
    std::wstring search = dir + L"\\*";

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileExW(
        search.c_str(), FindExInfoBasic, &fd,
        FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH
    );
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    std::vector<std::wstring> subdirs;
    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (fd.cFileName[0] != L'.')
                subdirs.push_back(dir + L"\\" + fd.cFileName);
        }
        else { ++count; }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    for (const auto& sub : subdirs)
        count += CountFiles(sub);
    return count;
}

bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle)
{
    return std::search(
        haystack.begin(), haystack.end(),
        needle.begin(),   needle.end(),
        [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); }
    ) != haystack.end();
}

bool IsProtected(const fs::path& path)
{
    std::string pathStr = path.string();
    for (const auto& item : WHITELIST)
        if (ContainsIgnoreCase(pathStr, item))
            return true;
    return false;
}

std::string GetProjectName(const fs::path& startPath)
{
    try
    {
        for (auto& e : fs::directory_iterator(startPath, fs::directory_options::skip_permission_denied))
            if (e.is_regular_file() && e.path().extension() == ".uproject")
                return e.path().stem().string();
    }
    catch (...) {}
    return startPath.filename().string();
}

// ----------------------------------------------------------------
// Scan
// ----------------------------------------------------------------

// Build a GroupInfo for a given owner directory (project root or plugin dir)
GroupInfo BuildGroup(const std::string& name, const fs::path& ownerDir)
{
    GroupInfo g;
    g.name = name;

    fs::path bin = ownerDir / "Binaries";
    fs::path mid = ownerDir / "Intermediate";

    if (fs::exists(bin) && fs::is_directory(bin))   g.binariesPath     = bin;
    if (fs::exists(mid) && fs::is_directory(mid))   g.intermediatePath = mid;

    // Count in parallel
    std::future<int> fb, fm;
    if (!g.binariesPath.empty())
        fb = std::async(std::launch::async, CountFiles, g.binariesPath.wstring());
    if (!g.intermediatePath.empty())
        fm = std::async(std::launch::async, CountFiles, g.intermediatePath.wstring());

    if (!g.binariesPath.empty())     g.binariesCount     = fb.get();
    if (!g.intermediatePath.empty()) g.intermediateCount = fm.get();

    return g;
}

std::vector<GroupInfo> FindTargetFolders(const fs::path& startPath)
{
    std::vector<GroupInfo> result;

    // 1. Project root
    std::string projectName = GetProjectName(startPath);
    auto rootGroup = BuildGroup(projectName, startPath);
    if (!rootGroup.binariesPath.empty() || !rootGroup.intermediatePath.empty())
        result.push_back(std::move(rootGroup));

    // 2. Each plugin (parallel)
    fs::path pluginsDir = startPath / "Plugins";
    std::vector<fs::path> pluginDirs;
    try
    {
        for (auto& e : fs::directory_iterator(pluginsDir, fs::directory_options::skip_permission_denied))
            if (e.is_directory())
                pluginDirs.push_back(e.path());
    }
    catch (...) {}

    std::vector<std::future<GroupInfo>> futures;
    futures.reserve(pluginDirs.size());
    for (auto& dir : pluginDirs)
        futures.push_back(std::async(std::launch::async, BuildGroup, dir.filename().string(), dir));

    for (auto& f : futures)
    {
        auto g = f.get();
        if (!g.binariesPath.empty() || !g.intermediatePath.empty())
            result.push_back(std::move(g));
    }

    return result;
}

// ----------------------------------------------------------------
// Clean
// ----------------------------------------------------------------

void DeleteFilesInDir(const fs::path& dir)
{
    if (!fs::exists(dir)) return;
    bool isBinaries = ContainsIgnoreCase(dir.string(), "Binaries");
    try
    {
        for (auto& e : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied))
        {
            if (!e.is_regular_file()) continue;
            std::string ext = e.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
            if (isBinaries) { if (ext == ".dll" || ext == ".pdb") { std::error_code ec; fs::remove(e.path(), ec); } }
            else            { std::error_code ec; fs::remove(e.path(), ec); }
        }
    }
    catch (...) {}
}

void CleanPath(const fs::path& root)
{
    if (root.empty() || !fs::exists(root)) return;
    try
    {
        for (auto& e : fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied))
        {
            if (!e.is_directory()) continue;
            if (!IsProtected(e.path())) DeleteFilesInDir(e.path());
        }
        if (!IsProtected(root)) DeleteFilesInDir(root);
    }
    catch (...) {}
}

void DeleteSlnFiles(const fs::path& startPath)
{
    try
    {
        for (auto& e : fs::directory_iterator(startPath, fs::directory_options::skip_permission_denied))
            if (e.is_regular_file() && e.path().extension() == ".sln")
            { std::error_code ec; fs::remove(e.path(), ec); }
    }
    catch (...) {}
}

void GenerateProjectFiles(const fs::path& uprojectPath)
{
    fs::path selectorPath =
        L"C:\\Program Files (x86)\\Epic Games\\Launcher\\Engine\\Binaries\\Win64\\UnrealVersionSelector.exe";
    if (!fs::exists(selectorPath))
    {
        std::cout << COL_RED << "  [!] UnrealVersionSelector.exe not found." << COL_RESET << "\n";
        return;
    }
    std::wstring args = L"/projectfiles \"" + uprojectPath.wstring() + L"\"";
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei); sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpFile = selectorPath.wstring().c_str(); sei.lpParameters = args.c_str(); sei.nShow = SW_HIDE;
    if (ShellExecuteExW(&sei))
    {
        std::cout << COL_YELLOW << "  Generating project files..." << COL_RESET;
        WaitForSingleObject(sei.hProcess, INFINITE);
        CloseHandle(sei.hProcess);
        std::cout << COL_GREEN << " Done." << COL_RESET << "\n";
    }
    else std::cout << COL_RED << "  [!] Failed to generate project files." << COL_RESET << "\n";
}

void OpenUproject(const fs::path& uprojectPath)
{
    ShellExecuteW(nullptr, L"open", uprojectPath.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void CleanSelected(std::vector<GroupInfo>& groups, const fs::path& startPath)
{
    std::cout << "\n" << COL_YELLOW << "Cleaning..." << COL_RESET << "\n";
    int cleaned = 0;
    for (auto& g : groups)
    {
        if (!g.selected) continue;
        CleanPath(g.binariesPath);
        CleanPath(g.intermediatePath);
        ++cleaned;
    }
    DeleteSlnFiles(startPath);
    std::cout << COL_GREEN << "  " << cleaned << " entry(s) cleaned." << COL_RESET << "\n";

    for (auto& e : fs::directory_iterator(startPath, fs::directory_options::skip_permission_denied))
        if (e.is_regular_file() && e.path().extension() == ".uproject")
        { GenerateProjectFiles(e.path()); break; }
}

// ----------------------------------------------------------------
// UI
// ----------------------------------------------------------------

void PrintMenu(const std::vector<GroupInfo>& groups, const fs::path& startPath)
{
    system("cls");

    std::cout << COL_CYAN << COL_BOLD
              << "=====================================================\n"
              << "           UNREAL PROJECT CLEANER (C++)\n"
              << "=====================================================\n"
              << COL_RESET;
    std::cout << COL_WHITE << "Project: " << COL_RESET << startPath.string() << "\n\n";

    if (groups.empty())
    {
        std::cout << COL_GREY << "  No Binaries or Intermediate folders found.\n\n" << COL_RESET;
    }
    else
    {
        std::cout << COL_WHITE << COL_BOLD << "Folders:\n" << COL_RESET;

        // Calculate alignment widths
        size_t idxWidth  = std::to_string(groups.size()).size();
        size_t nameWidth = 0;
        size_t binNumWidth = 0;
        size_t midNumWidth = 0;
        for (auto& g : groups)
        {
            nameWidth  = std::max(nameWidth,  g.name.size());
            binNumWidth = std::max(binNumWidth, std::to_string(g.binariesCount).size());
            midNumWidth = std::max(midNumWidth, std::to_string(g.intermediateCount).size());
        }

        for (size_t i = 0; i < groups.size(); ++i)
        {
            const auto& g = groups[i];

            std::string idxStr = std::to_string(i + 1);
            std::string namePad(nameWidth - g.name.size(), ' ');
            std::string idxPad(idxWidth - idxStr.size(), ' ');

            // Right-align counts to their column widths
            auto padNum = [](int n, size_t w) {
                std::string s = std::to_string(n);
                return std::string(w - s.size(), ' ') + s;
            };

            std::string binStr = !g.binariesPath.empty()
                ? "[Binaries: "     + padNum(g.binariesCount,     binNumWidth) + "]"
                : std::string(12 + binNumWidth + 1, ' '); // blank same width as "[Binaries: NNN]"

            std::string midStr = !g.intermediatePath.empty()
                ? "[Intermediate: " + padNum(g.intermediateCount, midNumWidth) + "]"
                : "";

            std::string countStr = binStr + (midStr.empty() ? "" : " " + midStr);

            if (g.selected)
            {
                std::cout << COL_RED
                          << "  [X] " << idxPad << idxStr << ". " << g.name << namePad << "  " << countStr
                          << COL_RESET << "\n";
            }
            else if (g.totalCount() == 0)
            {
                std::cout << COL_GREEN
                          << "  [\xE2\x9C\x93] " << idxPad << idxStr << ". " << g.name << namePad << "  [CLEAN]"
                          << COL_RESET << "\n";
            }
            else
            {
                std::cout << COL_GREY
                          << "  [ ] " << idxPad << idxStr << ". " << g.name << namePad << "  " << countStr
                          << COL_RESET << "\n";
            }
        }
        std::cout << "\n";
    }

    std::cout << COL_WHITE << COL_BOLD << "Commands:\n" << COL_RESET;
    std::cout << COL_YELLOW << "  a" << COL_RESET << "  - Select all\n";
    std::cout << COL_YELLOW << "  n" << COL_RESET << "  - Deselect all\n";
    std::cout << COL_YELLOW << "  c" << COL_RESET << "  - Clean selected\n";
    std::cout << COL_YELLOW << "  b" << COL_RESET << "  - Clean + open project (Clean Build)\n";
    std::cout << COL_YELLOW << "  r" << COL_RESET << "  - Refresh\n";
    std::cout << COL_YELLOW << "  q" << COL_RESET << "  - Quit\n";
    std::cout << "\n";
    std::cout << COL_GREY << "  Type a number to toggle (space-separated for multiple)\n" << COL_RESET;
    std::cout << "\n" << COL_WHITE << "Input: " << COL_RESET;
}

// ----------------------------------------------------------------
// Main loop
// ----------------------------------------------------------------

int main()
{
    EnableAnsiColors();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    fs::path startPath = GetExeDir();

    std::cout << COL_YELLOW << "Scanning, please wait...\n" << COL_RESET;
    auto groups = FindTargetFolders(startPath);

    std::string line;
    while (true)
    {
        PrintMenu(groups, startPath);
        std::getline(std::cin, line);

        if (line == "q" || line == "Q") break;

        if (line == "a" || line == "A") { for (auto& g : groups) g.selected = true;  continue; }
        if (line == "n" || line == "N") { for (auto& g : groups) g.selected = false; continue; }
        if (line == "r" || line == "R") { groups = FindTargetFolders(startPath); continue; }

        if (line == "c" || line == "C")
        {
            CleanSelected(groups, startPath);
            groups = FindTargetFolders(startPath);
            continue;
        }

        if (line == "b" || line == "B")
        {
            CleanSelected(groups, startPath);
            for (auto& e : fs::directory_iterator(startPath, fs::directory_options::skip_permission_denied))
            {
                if (e.is_regular_file() && e.path().extension() == ".uproject")
                {
                    std::cout << COL_CYAN << "  Opening project: " << e.path().filename().string() << COL_RESET << "\n";
                    OpenUproject(e.path());
                    break;
                }
            }
            groups = FindTargetFolders(startPath);
            continue;
        }

        // Numbers - toggle selection
        std::istringstream iss(line);
        std::string token;
        while (iss >> token)
        {
            try
            {
                int idx = std::stoi(token) - 1;
                if (idx >= 0 && idx < (int)groups.size())
                    groups[idx].selected = !groups[idx].selected;
            }
            catch (...) {}
        }
    }

    return 0;
}
