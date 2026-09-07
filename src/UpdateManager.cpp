// UpdateManager.cpp
#include "../include/UpdateManager.h"
#include "../include/SystemCore.h"
#include <windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cctype>

using namespace std;

const string UpdateManager::CURRENT_VERSION = "0.1.0";
const string UpdateManager::GITHUB_REPO = "huii404/CMD_BOX";
const string UpdateManager::API_RELEASES_URL = "https://api.github.com/repos/huii404/CMD_BOX/releases/latest";

static atomic<bool> g_checkFinished(false);
static atomic<bool> g_hasNewVersion(false);
static string g_remoteVersion = "";
static string g_releaseUrl = "https://github.com/huii404/CMD_BOX/releases";
static mutex g_versionMutex;

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
    thread([]() {
        ReleaseInfo rel = fetchLatestRelease();
        if (rel.valid) {
            lock_guard<mutex> lock(g_versionMutex);
            g_remoteVersion = rel.version;
            if (!rel.htmlUrl.empty()) g_releaseUrl = rel.htmlUrl;
            if (isNewer(CURRENT_VERSION, rel.version)) {
                g_hasNewVersion = true;
            }
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
        return "v" + CURRENT_VERSION + " \x1b[33m(Có bản mới v" + getRemoteVersion() + "!)\x1b[0m";
    }
    if (isCheckingFinished()) {
        if (!getRemoteVersion().empty()) {
            return "v" + CURRENT_VERSION + " \x1b[32m(Mới nhất)\x1b[0m";
        }
        return "v" + CURRENT_VERSION;
    }
    return "v" + CURRENT_VERSION + " (Đang kiểm tra...)";
}

void UpdateManager::showUpdateMenu() {
    while (true) {
        system("cls");
        cout << "\n=========================================================================================\n"
             << "                           KIỂM TRA CẬP NHẬT TỪ GITHUB RELEASES                          \n"
             << "=========================================================================================\n\n";

        cout << "  - Phiên bản hiện tại : \x1b[36mv" << CURRENT_VERSION << "\x1b[0m\n"
             << "  - Kho lưu trữ GitHub : https://github.com/" << GITHUB_REPO << "\n"
             << "  - API Releases       : " << API_RELEASES_URL << "\n\n";

        cout << "  - Đang truy vấn GitHub API... ";
        cout.flush();

        ReleaseInfo rel = fetchLatestRelease();
        if (!rel.valid) {
            cout << "\x1b[31m(Không thể kết nối tới GitHub Releases API)\x1b[0m\n\n";
            cout << " [!] Hãy kiểm tra kết nối mạng của bạn.\n\n";
        } else {
            lock_guard<mutex> lock(g_versionMutex);
            g_remoteVersion = rel.version;
            if (!rel.htmlUrl.empty()) g_releaseUrl = rel.htmlUrl;

            cout << "\x1b[32m[✓] Thành công!\x1b[0m\n\n";
            cout << "  + Bản phát hành mới  : \x1b[36m" << rel.releaseName << "\x1b[0m\n"
                 << "  + Phiên bản nhận diện: \x1b[32mv" << rel.version << "\x1b[0m\n";
            if (!rel.publishedAt.empty()) {
                cout << "  + Thời gian phát hành: " << rel.publishedAt << "\n";
            }
            if (!rel.htmlUrl.empty()) {
                cout << "  + Trang tải trực tiếp: " << rel.htmlUrl << "\n";
            }
            cout << "\n";

            if (isNewer(CURRENT_VERSION, rel.version)) {
                g_hasNewVersion = true;
                cout << "  ┌────────────────────────────────────────────────────────────────────────┐\n"
                     << "  │ \x1b[33m[!] PHÁT HIỆN PHIÊN BẢN MỚI: v" << rel.version << " (Bản hiện tại của bạn: v" << CURRENT_VERSION << ")\x1b[0m       │\n"
                     << "  │ Bạn nên cập nhật để có thêm các tính năng và bản vá lỗi mới nhất.      │\n"
                     << "  └────────────────────────────────────────────────────────────────────────┘\n\n";
            } else {
                g_hasNewVersion = false;
                cout << "  [✓] \x1b[32mBạn đang sử dụng phiên bản mới nhất! (v" << CURRENT_VERSION << ")\x1b[0m\n\n";
            }
        }

        cout << " [1] Mở trang GitHub Release trên trình duyệt\n"
             << " [2] Cập nhật mã nguồn tự động qua Git (git pull)\n"
             << " [3] Kiểm tra lại\n"
             << " [0] Quay lại menu chính\n\n"
             << " [Chọn]: ";

        int choice = SystemCore::readInt("");
        if (choice == 0) return;

        if (choice == 1) {
            string openUrl = g_releaseUrl;
            cout << "\n[*] Đang mở trang: " << openUrl << " trên trình duyệt...\n";
            ShellExecuteA(NULL, "open", openUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
            Sleep(1500);
        } else if (choice == 2) {
            cout << "\n[*] Đang kiểm tra Git repository...\n";
            if (SystemCore::runRawCommand("git rev-parse --is-inside-work-tree >nul 2>&1")) {
                cout << "[*] Đang kéo mã nguồn mới nhất từ GitHub (git pull)...\n\n";
                SystemCore::runRawCommand("git pull origin main");
                cout << "\n[✓] Kéo mã nguồn hoàn tất!\n"
                     << "[?] Bạn có muốn chạy build lại ngay bây giờ? (y/n): ";
                string ans;
                getline(cin, ans);
                if (ans == "y" || ans == "Y") {
                    cout << "\n[*] Đang biên dịch lại ứng dụng qua build.bat...\n";
                    SystemCore::runRawCommand("call build.bat");
                }
            } else {
                cout << "[!] Không tìm thấy thư mục .git. Vui lòng tải mã nguồn hoặc file từ GitHub.\n";
            }
            SystemCore::waitEnter();
        } else if (choice == 3) {
            continue;
        }
    }
}
