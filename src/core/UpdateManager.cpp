// UpdateManager.cpp
#include "UpdateManager.h"
#include "MenuStyle.h"
#include "SystemCore.h"
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

const string UpdateManager::CURRENT_VERSION = "2.1.1";
const string UpdateManager::API_RELEASES_URL = "https://api.github.com/repos/huii404/CMD_BOX/releases/latest";

static const long long UPDATE_COOLDOWN_SECONDS = 2 * 24 * 3600; // 2 ngày (48 giờ)

static atomic<bool> g_checkFinished(false);
static atomic<bool> g_hasNewVersion(false);
static string g_remoteVersion = "";
static string g_releaseUrl = "https://github.com/huii404/CMD_BOX/releases";
static mutex g_versionMutex;
static thread g_checkThread;

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
    // Walk string tokens; a null/number value must not consume the next key.
    for (size_t pos = 0; pos < json.size();) {
        if (json[pos++] != '"') continue;
        string key; bool complete = false;
        while (pos < json.size()) {
            char c = json[pos++];
            if (c == '"') { complete = true; break; }
            if (c == '\\') { if (pos == json.size()) return ""; key += json[pos++]; }
            else key += c;
        }
        if (!complete) return "";
        size_t value = pos;
        while (value < json.size() && isspace(static_cast<unsigned char>(json[value]))) ++value;
        if (key != field || value == json.size() || json[value] != ':') continue;
        ++value;
        while (value < json.size() && isspace(static_cast<unsigned char>(json[value]))) ++value;
        if (value == json.size() || json[value++] != '"') return "";
        string result;
        while (value < json.size()) {
            unsigned char c = json[value++];
            if (c == '"') return result;
            if (c < 32) return "";
            if (c != '\\') { result += char(c); continue; }
            if (value == json.size()) return "";
            char escape = json[value++];
            switch (escape) {
                case '"': case '\\': case '/': result += escape; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'u': {
                    auto hex = [](char h) -> int {
                        if (h >= '0' && h <= '9') return h - '0';
                        if (h >= 'a' && h <= 'f') return h - 'a' + 10;
                        if (h >= 'A' && h <= 'F') return h - 'A' + 10;
                        return -1;
                    };
                    auto word = [&](unsigned& code) {
                        code = 0;
                        if (value + 4 > json.size()) return false;
                        for (int i = 0; i < 4; ++i) { int n = hex(json[value++]); if (n < 0) return false; code = code * 16 + unsigned(n); }
                        return true;
                    };
                    unsigned code = 0;
                    if (!word(code)) return "";
                    if (code >= 0xD800 && code <= 0xDBFF) {
                        if (value + 2 > json.size() || json.substr(value, 2) != "\\u") return "";
                        value += 2; unsigned low = 0;
                        if (!word(low) || low < 0xDC00 || low > 0xDFFF) return "";
                        code = 0x10000 + (code - 0xD800) * 1024 + low - 0xDC00;
                    } else if (code >= 0xDC00 && code <= 0xDFFF) return "";
                    if (code < 0x80) result += char(code);
                    else if (code < 0x800) { result += char(0xC0 | (code >> 6)); result += char(0x80 | (code & 63)); }
                    else if (code < 0x10000) {
                        result += char(0xE0 | (code >> 12)); result += char(0x80 | ((code >> 6) & 63)); result += char(0x80 | (code & 63));
                    } else {
                        result += char(0xF0 | (code >> 18)); result += char(0x80 | ((code >> 12) & 63));
                        result += char(0x80 | ((code >> 6) & 63)); result += char(0x80 | (code & 63));
                    }
                    break;
                }
                default: return "";
            }
        }
        return ""; // Unterminated string.
    }
    return "";
}

static string extractCleanVersion(const string& raw) {
    string text = raw;
    size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == string::npos) return "";
    text = text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
    if (text.rfind("cmd_base", 0) == 0) text.erase(0, 8);
    if (!text.empty() && (text[0] == 'v' || text[0] == 'V')) text.erase(0, 1);
    if (text.empty() || text.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-+") != string::npos) return "";
    return text; // Retain prerelease identifiers; comparison validates the core.
}

static vector<int> parseVersionParts(string v) {
    v = extractCleanVersion(v);
    auto suffix = v.find_first_of("-+");
    string core = v.substr(0, suffix);
    if (core.empty() || core.back() == '.') return {};
    vector<int> parts; stringstream stream(core); string token;
    while (getline(stream, token, '.')) {
        if (token.empty() || token.find_first_not_of("0123456789") != string::npos || parts.size() == 3) return {};
        try { parts.push_back(stoi(token)); } catch (...) { return {}; }
    }
    while (parts.size() < 3) parts.push_back(0);
    return parts;
}

bool UpdateManager::isNewer(const string& currentVer, const string& remoteVer) {
    const string current = extractCleanVersion(currentVer), remote = extractCleanVersion(remoteVer);
    const auto c = parseVersionParts(current), r = parseVersionParts(remote);
    if (c.size() != 3 || r.size() != 3) return false;
    for (size_t i = 0; i < 3; ++i) {
        if (r[i] != c[i]) return r[i] > c[i];
    }
    auto pre = [](const string& v) {
        size_t metadata = v.find('+'), dash = v.find('-');
        if (dash == string::npos || (metadata != string::npos && dash > metadata)) return string();
        return v.substr(dash + 1, metadata == string::npos ? string::npos : metadata - dash - 1);
    };
    string cp = pre(current), rp = pre(remote);
    if (cp.empty() || rp.empty()) return !cp.empty() && rp.empty();
    stringstream cs(cp), rs(rp); string ct, rt;
    while (true) {
        bool hasC = bool(getline(cs, ct, '.')), hasR = bool(getline(rs, rt, '.'));
        if (!hasC || !hasR) return hasR && !hasC;
        if (ct.empty() || rt.empty()) return false;
        if (ct == rt) continue;
        bool cn = ct.find_first_not_of("0123456789") == string::npos;
        bool rn = rt.find_first_not_of("0123456789") == string::npos;
        if (cn != rn) return !rn;
        if (cn) {
            auto strip = [](string x) { size_t first = x.find_first_not_of('0'); return first == string::npos ? string("0") : x.substr(first); };
            ct = strip(ct); rt = strip(rt);
            if (ct.size() != rt.size()) return rt.size() > ct.size();
        }
        return rt > ct;
    }
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

    string tagName = extractJsonField(json, "tag_name");
    info.releaseName = extractJsonField(json, "name");
    if (info.releaseName.empty()) {
        info.releaseName = tagName;
    }
    info.version = extractCleanVersion(tagName);
    if (info.version.empty()) {
        info.version = extractCleanVersion(info.releaseName);
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
            if (cachedUrl.rfind("https://github.com/huii404/CMD_BOX/releases", 0) == 0) g_releaseUrl = cachedUrl;
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
    if (g_checkThread.joinable()) g_checkThread.join();
    g_checkThread = thread([]() {
        ReleaseInfo rel = fetchLatestRelease();
        lock_guard<mutex> lock(g_versionMutex);
        if (rel.valid) {
            g_remoteVersion = rel.version;
            if (rel.htmlUrl.rfind("https://github.com/huii404/CMD_BOX/releases", 0) == 0) g_releaseUrl = rel.htmlUrl;
            g_hasNewVersion = isNewer(CURRENT_VERSION, rel.version);
            // Lưu cache mới kèm mốc thời gian
            saveCache(static_cast<long long>(time(nullptr)), rel.version, g_releaseUrl);
        }
        g_checkFinished = true; // Inside lock: ensures happens-before with string reads
    });
}

void UpdateManager::shutdown() {
    if (g_checkThread.joinable()) g_checkThread.join();
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
        SystemCore::cls();
        MenuStyle::header("CẬP NHẬT PHẦN MỀM",MenuStyle::ROSE);

        ReleaseInfo rel = fetchLatestRelease();
        if (!rel.valid) {
            MenuStyle::info("Phiên bản hiện tại","v"+CURRENT_VERSION,MenuStyle::SKY);
            MenuStyle::info("Trạng thái","Không thể kết nối Internet",MenuStyle::ROSE);
        } else {
            lock_guard<mutex> lock(g_versionMutex);
            g_remoteVersion = rel.version;
            if (rel.htmlUrl.rfind("https://github.com/huii404/CMD_BOX/releases", 0) == 0) g_releaseUrl = rel.htmlUrl;

            if (isNewer(CURRENT_VERSION, rel.version)) {
                g_hasNewVersion = true;
                MenuStyle::info("Phiên bản hiện tại","v"+CURRENT_VERSION,MenuStyle::SKY);
                MenuStyle::info("Phiên bản mới nhất","v"+rel.version+" (Có bản cập nhật mới!)",MenuStyle::AMBER);
            } else {
                g_hasNewVersion = false;
                MenuStyle::info("Phiên bản hiện tại","v"+CURRENT_VERSION+" (Đang là mới nhất)",MenuStyle::MINT);
            }
            saveCache(static_cast<long long>(time(nullptr)), rel.version, g_releaseUrl);
        }

        MenuStyle::item(1,"Mở GitHub",MenuStyle::ROSE);
        MenuStyle::item(2,"Cập nhật bằng Git",MenuStyle::MINT);
        MenuStyle::item(3,"Kiểm tra lại",MenuStyle::AMBER);
        MenuStyle::footer("Quay lại",MenuStyle::ROSE);

        int choice = SystemCore::readInt("");
        if (choice == 0) return;

        if (choice == 1) {
            string openUrl;
            {
                lock_guard<mutex> lock(g_versionMutex);
                openUrl = g_releaseUrl;
            }
            ShellExecuteA(NULL, "open", openUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
            Sleep(800);
        } else if (choice == 2) {
            cout << "\n[*] Đang kéo mã nguồn mới nhất từ GitHub...\n\n";
            if (SystemCore::runRawCommand("git rev-parse --is-inside-work-tree")) {
                bool pullOk = SystemCore::runRawCommand("git pull --ff-only");
                if (!pullOk) {
                    cout << "\n[!] Cập nhật thất bại hoặc nhánh cục bộ có thay đổi xung đột.\n";
                    SystemCore::waitEnter();
                    continue;
                }
                cout << "\n[✓] Hoàn tất! Bạn có muốn biên dịch lại ứng dụng ngay? (y/n): ";
                string ans;
                getline(cin, ans);
                if (ans == "y" || ans == "Y") {
                    cout << "\n[*] Đang biên dịch lại qua build.bat...\n";
                    if (!SystemCore::runRawCommand("cmd.exe /c call build.bat")) {
                        cout << "\n[!] Build thất bại. Mã nguồn đã cập nhật nhưng file chạy chưa được thay thế.\n";
                    }
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
