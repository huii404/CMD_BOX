// UpdateManager.cpp
#include "../include/UpdateManager.h"
#include "../include/SystemCore.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <ctime>

using namespace std;

const string UpdateManager::CURRENT_VERSION = "0.1.0";
const string UpdateManager::GITHUB_REPO = "huii404/CMD_BOX";
const string UpdateManager::API_RELEASES_URL = "https://api.github.com/repos/huii404/CMD_BOX/releases/latest";

static const long long UPDATE_COOLDOWN_SECONDS = 2 * 24 * 3600; // 2 ngày (48 giờ)

static atomic<bool> g_checkFinished(false);
static atomic<bool> g_hasNewVersion(false);
static string g_remoteVersion = "";
static string g_releaseUrl = "https://github.com/huii404/CMD_BOX/releases";
static mutex g_versionMutex;

static string getCacheFilePath() {
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) > 0) {
        string path(exePath);
        size_t lastSlash = path.find_last_of("\\/");
        if (lastSlash != string::npos) {
            return path.substr(0, lastSlash + 1) + ".update_cache";
        }
    }
    return ".update_cache";
}

static bool loadCache(long long& outTime, string& outVersion, string& outUrl) {
    ifstream file(getCacheFilePath());
    if (!file.is_open()) return false;
    string lineTime;
    if (getline(file, lineTime) && getline(file, outVersion)) {
        try {
            outTime = stoll(lineTime);
            getline(file, outUrl);
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
}

static void saveCache(long long checkTime, const string& version, const string& url) {
    ofstream file(getCacheFilePath());
    if (file.is_open()) {
        file << checkTime << "\n" << version << "\n" << url << "\n";
    }
}

static string extractJsonField(const string& json, const string& field) {
    string key = "\"" + field + "\"";
    size_t pos = json.find(key);
    if (pos == string::npos) return "";
    pos = json.find(":", pos + key.length());
    if (pos == string::npos) return "";
    pos = json.find("\"", pos + 1);
    if (pos == string::npos) return "";
    size_t endPos = json.find("\"", pos + 1);
    if (endPos == string::npos) return "";
    return json.substr(pos + 1, endPos - pos - 1);
}

static string extractCleanVersion(const string& raw) {
    if (raw.empty()) return "";

    // Nếu chuỗi có dạng cmd_base3 hoặc tương tự
    string lower = raw;
    for (char &c : lower) c = (char)tolower((unsigned char)c);
    
    // Tìm vị trí chữ số đầu tiên
    size_t start = string::npos;
    for (size_t i = 0; i < raw.length(); ++i) {
        if (isdigit((unsigned char)raw[i])) {
            start = i;
            break;
        }
    }
    if (start == string::npos) return raw;

    string ver = "";
    for (size_t i = start; i < raw.length(); ++i) {
        char c = raw[i];
        if (isdigit((unsigned char)c) || c == '.') {
            ver += c;
        } else {
            break;
        }
    }
    return ver;
}

static vector<int> parseVersionParts(string v) {
    if (!v.empty() && (v[0] == 'v' || v[0] == 'V')) v = v.substr(1);
    
    // Xử lý tiền tố cmd_base nếu còn sót
    string lower = v;
    for (char &c : lower) c = (char)tolower((unsigned char)c);
    size_t basePos = lower.find("cmd_base");
    if (basePos != string::npos) {
        v = v.substr(basePos + 8);
    }

    vector<int> parts;
    stringstream ss(v);
    string token;
    while (getline(ss, token, '.')) {
        try {
            parts.push_back(stoi(token));
        } catch (...) {
            parts.push_back(0);
        }
    }
    while (parts.size() < 3) parts.push_back(0);
    return parts;
}

bool UpdateManager::isNewer(const string& currentVer, const string& remoteVer) {
    if (remoteVer.empty()) return false;
    auto c = parseVersionParts(currentVer);
    auto r = parseVersionParts(remoteVer);
    for (size_t i = 0; i < 3; ++i) {
        if (r[i] > c[i]) return true;
        if (r[i] < c[i]) return false;
    }
    return false;
}

ReleaseInfo UpdateManager::fetchLatestRelease() {
    ReleaseInfo info;
    string cmd = "curl.exe -s -H \"User-Agent: CMD_BOX\" --max-time 4 \"" + API_RELEASES_URL + "\"";
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return info;

    string json = "";
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe)) {
        json += buf;
    }
    _pclose(pipe);

    if (json.empty() || json.find("\"tag_name\"") == string::npos) {
        return info;
    }

    info.releaseName = extractJsonField(json, "name");
    if (info.releaseName.empty()) {
        info.releaseName = extractJsonField(json, "tag_name");
    }
    info.version = extractCleanVersion(info.releaseName);
    if (info.version.empty()) {
        info.version = extractCleanVersion(extractJsonField(json, "tag_name"));
    }
    info.htmlUrl = extractJsonField(json, "html_url");
    info.publishedAt = extractJsonField(json, "published_at");
    info.downloadUrl = extractJsonField(json, "browser_download_url");
    info.body = extractJsonField(json, "body");
    info.valid = (!info.version.empty());

    return info;
}

void UpdateManager::checkUpdateAsync() {
    // 1. Tải cache phiên bản từ lần kiểm tra trước (tức thì 0ms, không tốn tài nguyên)
    long long lastCheckTime = 0;
    string cachedVer = "";
    string cachedUrl = "";
    long long now = static_cast<long long>(time(nullptr));

    if (loadCache(lastCheckTime, cachedVer, cachedUrl)) {
        {
            lock_guard<mutex> lock(g_versionMutex);
            g_remoteVersion = cachedVer;
            if (!cachedUrl.empty()) g_releaseUrl = cachedUrl;
            if (isNewer(CURRENT_VERSION, cachedVer)) {
                g_hasNewVersion = true;
            }
            g_checkFinished = true;
        }

        // Nếu chưa đủ 2 ngày kể từ lần kiểm tra gần nhất -> giữ nguyên dữ liệu cache, không gọi GitHub
        if ((now - lastCheckTime) < UPDATE_COOLDOWN_SECONDS && (now >= lastCheckTime)) {
            return;
        }
    }

    // 2. Chưa có cache hoặc đã quá 2 ngày -> khởi chạy thread ngầm kiểm tra GitHub
    thread([]() {
        ReleaseInfo rel = fetchLatestRelease();
        if (rel.valid) {
            lock_guard<mutex> lock(g_versionMutex);
            g_remoteVersion = rel.version;
            if (!rel.htmlUrl.empty()) g_releaseUrl = rel.htmlUrl;
            if (isNewer(CURRENT_VERSION, rel.version)) {
                g_hasNewVersion = true;
            } else {
                g_hasNewVersion = false;
            }
            // Lưu cache mới kèm mốc thời gian
            saveCache(static_cast<long long>(time(nullptr)), rel.version, g_releaseUrl);
        }
        g_checkFinished = true;
    }).detach();
}

bool UpdateManager::isCheckingFinished() {
    return g_checkFinished.load();
}

bool UpdateManager::hasNewVersion() {
    return g_hasNewVersion.load();
}

string UpdateManager::getRemoteVersion() {
    lock_guard<mutex> lock(g_versionMutex);
    return g_remoteVersion;
}

string UpdateManager::getVersionStatusText() {
    if (hasNewVersion()) {
        return "v" + CURRENT_VERSION + " \x1b[33m(🚀 v" + getRemoteVersion() + ")\x1b[0m";
    }
    return "v" + CURRENT_VERSION;
}

void UpdateManager::showUpdateMenu() {
    while (true) {
        system("cls");
        cout << "\n";

        ReleaseInfo rel = fetchLatestRelease();
        if (!rel.valid) {
            cout << "  Phiên bản hiện tại : \x1b[36mv" << CURRENT_VERSION << "\x1b[0m\n"
                 << "  Trạng thái         : \x1b[31mKhông thể kết nối Internet\x1b[0m\n\n";
        } else {
            lock_guard<mutex> lock(g_versionMutex);
            g_remoteVersion = rel.version;
            if (!rel.htmlUrl.empty()) g_releaseUrl = rel.htmlUrl;

            if (isNewer(CURRENT_VERSION, rel.version)) {
                g_hasNewVersion = true;
                cout << "  Phiên bản hiện tại : \x1b[36mv" << CURRENT_VERSION << "\x1b[0m\n"
                     << "  Phiên bản mới nhất : \x1b[33mv" << rel.version << " (Có bản cập nhật mới!)\x1b[0m\n\n";
            } else {
                g_hasNewVersion = false;
                cout << "  Phiên bản hiện tại : \x1b[32mv" << CURRENT_VERSION << " (Đang là mới nhất)\x1b[0m\n\n";
            }
            saveCache(static_cast<long long>(time(nullptr)), rel.version, g_releaseUrl);
        }

        cout << " [1] Tải bản mới (Mở GitHub)\n"
             << " [2] Cập nhật tự động (Git pull)\n"
             << " [3] Kiểm tra lại\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";

        int choice = SystemCore::readInt("");
        if (choice == 0) return;

        if (choice == 1) {
            string openUrl = g_releaseUrl;
            ShellExecuteA(NULL, "open", openUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
            Sleep(800);
        } else if (choice == 2) {
            cout << "\n[*] Đang kéo mã nguồn mới nhất từ GitHub...\n\n";
            if (SystemCore::runRawCommand("git rev-parse --is-inside-work-tree >nul 2>&1")) {
                SystemCore::runRawCommand("git pull origin main");
                cout << "\n[✓] Hoàn tất! Bạn có muốn biên dịch lại ứng dụng ngay? (y/n): ";
                string ans;
                getline(cin, ans);
                if (ans == "y" || ans == "Y") {
                    cout << "\n[*] Đang biên dịch lại qua build.bat...\n";
                    SystemCore::runRawCommand("call build.bat");
                }
            } else {
                cout << "[!] Không tìm thấy kho lưu trữ Git cục bộ.\n";
            }
            SystemCore::waitEnter();
        } else if (choice == 3) {
            continue;
        }
    }
}
