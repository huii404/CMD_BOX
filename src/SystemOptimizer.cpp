#include "../include/SystemOptimizer.h"
#include <iostream>
#include <algorithm>
#include <windows.h>
#include <filesystem>
#include <vector>
#include <string>
#include <iomanip> 
using namespace std;
namespace fs = std::filesystem;

// =========================================================================================
// MODULE: SYSTEM OPTIMIZER (Tối ưu hóa và dọn dẹp hệ thống)
// =========================================================================================

SystemOptimizer::SystemOptimizer(SystemCore &s, Internet &net) : sc(s), n(net) {}

/**
 * @brief Hàm phụ trợ: Xóa sạch toàn bộ file & thư mục con bên trong một thư mục chỉ định
 * @param dirPath Đường dẫn thư mục cần dọn sạch nội dung
 * 
 * LƯU Ý KHI MỞ RỘNG:
 * - Dùng try/catch để bỏ qua các file đang bị khóa (in-use/locked) bởi tiến trình khác mà không làm crash app.
 */
static void wipeFolderContents(const fs::path &dirPath) {
    if (!fs::exists(dirPath)) return;
    try {
        for (const auto &entry : fs::directory_iterator(dirPath)) {
            try {
                fs::remove_all(entry.path());
            } catch (...) {}
        }
    } catch (...) {}
}

// Forward declaration hàm dọn dẹp thư mục dev artifacts (được định nghĩa chi tiết ở phần dưới)
static int cleanDirectoryArtifacts(const fs::path &rootPath, const std::vector<std::string> &targetDirNames, const std::vector<std::string> &targetExtensions, long long &freedBytes);

// --- HỆ THỐNG DỌN RÁC ĐA TẦNG (MULTI-TIER DISK CLEAN) ---

// Tầng 1: Rác bề mặt siêu tốc (Temp, Recent, Thùng rác, Flush DNS)
long long SystemOptimizer::runCleanTier1() {
    long long before = 0, after = 0;
    try { before = fs::space("C:\\").available; } catch (...) {}

    vector<thread> threads;
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /s /f /q \"%temp%\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%systemroot%\\temp\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Microsoft\\Windows\\Recent\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\D3DSCache\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Low\\Microsoft\\CryptnetUrlCache\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Microsoft\\Windows\\WER\\Temp\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\Temp\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("powershell -NoProfile -Command \"Clear-RecycleBin -Force -ErrorAction SilentlyContinue\""); });
    threads.emplace_back([this]() { sc.runCMD("ipconfig /flushdns >nul 2>&1"); });

    for (auto &t : threads) t.join();

    try { after = fs::space("C:\\").available; } catch (...) {}
    return (after > before) ? (after - before) : 0;
}

// Tầng 2: Rác Trình duyệt & Ứng dụng (Chrome, Edge, Brave, Discord, Shader)
long long SystemOptimizer::runCleanTier2() {
    long long before = 0, after = 0;
    try { before = fs::space("C:\\").available; } catch (...) {}

    clearBrowserCache();

    vector<thread> threads;
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\NVIDIA\\GLCache\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Microsoft\\Windows\\Explorer\\thumbcache_*.db\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%AppData%\\discord\\Cache\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%AppData%\\discord\\Code Cache\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Telegram Desktop\\tdata\\user_data\\cache\\*\" 2>nul"); });

    for (auto &t : threads) t.join();

    try { after = fs::space("C:\\").available; } catch (...) {}
    return (after > before) ? (after - before) : 0;
}

// Tầng 3: Chuyên sâu Hệ thống (DISM WinSxS, Update kẹt, Logs, Event Viewer)
long long SystemOptimizer::runCleanTier3() {
    long long before = 0, after = 0;
    try { before = fs::space("C:\\").available; } catch (...) {}

    string batContent = "@echo off\nchcp 65001 >nul\n";
    batContent += "mkdir \"%SystemDrive%\\EmptyFolderTmp\" 2>nul\n";
    batContent += "start /b robocopy \"%SystemDrive%\\EmptyFolderTmp\" \"%systemroot%\\temp\" /mir /w:0 /r:0 /log:nul\n";
    batContent += "start /b robocopy \"%SystemDrive%\\EmptyFolderTmp\" \"%systemroot%\\Prefetch\" /mir /w:0 /r:0 /log:nul\n";
    batContent += "dism /online /cleanup-image /startcomponentcleanup /resetbase\n";
    batContent += "del /f /s /q \"%systemroot%\\SoftwareDistribution\\Download\\*\" 2>nul\n";
    batContent += "del /f /s /q \"%systemroot%\\Logs\\CBS\\*.*\" 2>nul\n";
    batContent += "del /f /q %windir%\\WindowsUpdate.log 2>nul\n";
    batContent += "del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\ReportQueue\\*\" 2>nul\n";
    batContent += "del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\ReportArchive\\*\" 2>nul\n";
    batContent += "powershell -Command \"Get-DeliveryOptimizationStatus | Remove-DeliveryOptimizationCache -Confirm:$false\" 2>nul\n";
    batContent += "wevtutil el 2>nul | foreach { wevtutil cl \"$_\" 2>nul }\n";
    batContent += "powercfg -h off\n";
    batContent += "cleanmgr /sagerun:1\n";
    batContent += "rmdir \"%SystemDrive%\\EmptyFolderTmp\" 2>nul\n";

    SystemCore::runBatchAsAdmin(batContent, "Dọn dẹp hệ thống chuyên sâu Tầng 3");

    try { after = fs::space("C:\\").available; } catch (...) {}
    return (after > before) ? (after - before) : 0;
}

// Tầng 4: Rác Môi trường lập trình (node_modules, pip, gradle, VS Code...)
long long SystemOptimizer::runCleanTier4(const std::string &customPath) {
    long long before = 0, after = 0;
    try { before = fs::space("C:\\").available; } catch (...) {}

    cleanDevCaches(false);

    if (!customPath.empty() && fs::exists(customPath)) {
        long long extraFreed = 0;
        cleanDirectoryArtifacts(customPath, 
            {"node_modules", "node_module", ".turbo", ".next", ".nuxt", ".parcel-cache", ".svelte-kit", ".cache"}, 
            {".pyc", ".pyo"}, extraFreed);
    }

    try { after = fs::space("C:\\").available; } catch (...) {}
    return (after > before) ? (after - before) : 0;
}

// Menu điều phối Dọn rác Đa Tầng
void SystemOptimizer::multiTierDiskClean() {
    while (true) {
        sc.cls();
        cout << "\n\n"
             << " [1] Tầng 1: Dọn rác bề mặt\n"
             << " [2] Tầng 2: Dọn rác Trình duyệt & Ứng dụng\n"
             << " [3] Tầng 3: Dọn dẹp Chuyên sâu Hệ thống\n"
             << " [4] Tầng 4: Dọn rác Môi trường lập trình\n"
             << " [5] [⚡] Dọn liên hoàn Tầng 1 + 2 + 3\n"
             << " [6] [🚀] Dọn toàn bộ cả 4 Tầng\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";

        int choice = sc.readInt("");
        if (choice == 0) break;

        sc.cls();
        cout << "\n\n";
        long long totalFreed = 0;

        if (choice == 1 || choice == 5 || choice == 6) {
            cout << " [*] Tầng 1: Đang dọn rác tạm & cache bề mặt...\n"
                 << "     ├── Dọn Temp, Recent, ShaderCache, Cryptnet, WER Temp...\n"
                 << "     ├── Xóa sạch Thùng rác (Recycle Bin) & Flush DNS...\n";
            long long f1 = runCleanTier1();
            totalFreed += f1;
            cout << "     └── [✓ Xong] " << (f1 > 0 ? ("Giải phóng " + SystemCore::formatSize(f1)) : "Đã sạch sẽ từ trước") << "\n\n";
        }

        if (choice == 2 || choice == 5 || choice == 6) {
            cout << " [*] Tầng 2: Đang dọn rác Trình duyệt & Ứng dụng...\n"
                 << "     ├── Dọn cache Chrome, Edge, Brave, CocCoc, Opera, Firefox...\n"
                 << "     ├── Dọn cache Discord, Telegram, NVIDIA GLCache, Thumbnails...\n";
            long long f2 = runCleanTier2();
            totalFreed += f2;
            cout << "     └── [✓ Xong] " << (f2 > 0 ? ("Giải phóng " + SystemCore::formatSize(f2)) : "Đã sạch sẽ từ trước") << "\n\n";
        }

        if (choice == 3 || choice == 5 || choice == 6) {
            cout << " [*] Tầng 3: Đang dọn dẹp Chuyên sâu Hệ thống...\n"
                 << "     ├── Chạy DISM WinSxS ResetBase, dọn Windows Update kẹt...\n"
                 << "     ├── Xóa Delivery Optimization, CBS Logs, Windows Error Reports...\n"
                 << "     ├── Xóa toàn bộ Windows Event Logs, giải phóng file ngủ đông...\n";
            long long f3 = runCleanTier3();
            totalFreed += f3;
            cout << "     └── [✓ Xong] " << (f3 > 0 ? ("Giải phóng " + SystemCore::formatSize(f3)) : "Đã sạch sẽ từ trước") << "\n\n";
        }

        if (choice == 4 || choice == 6) {
            cout << " [*] Tầng 4: Đang dọn rác Môi trường lập trình (Dev Caches)...\n"
                 << "     ├── Dọn cache npm, yarn, pnpm, pip, nuget, gradle, cargo, go...\n"
                 << "     ├── Dọn cache VS Code, Cursor workspace storage...\n";
            long long f4 = runCleanTier4();
            totalFreed += f4;
            cout << "     └── [✓ Xong] " << (f4 > 0 ? ("Giải phóng " + SystemCore::formatSize(f4)) : "Đã sạch sẽ từ trước") << "\n\n";
        }

        cout << "\n\n";
        if (totalFreed > 0) {
            cout << "[✓] Đã giải phóng: " << SystemCore::formatSize(totalFreed) << "\n\n";
        } else {
            cout << "[✓] Hệ thống đã rất sạch sẽ.\n\n";
        }
        sc.waitEnter();
    }
}

void SystemOptimizer::cleanDiskQuick() {
    sc.cls();
    cout << "Đang dọn rác nhanh (Tầng 1)\n";
    long long freed = runCleanTier1();
    cout << "\n[✓] Đã xong!";
    if (freed > 0) cout << " (Giải phóng: " << SystemCore::formatSize(freed) << ")";
    cout << "\n\n";
    sc.waitEnter();
}

void SystemOptimizer::cleanDiskPro() {
    sc.cls();
    cout << "Đang dọn rác chuyên sâu (Tầng 1 + 2 + 3)\n";
    long long freed = runCleanTier1() + runCleanTier2() + runCleanTier3();
    cout << "\n[✓] Đã xong!";
    if (freed > 0) cout << " (Giải phóng: " << SystemCore::formatSize(freed) << ")";
    cout << "\n\n";
    sc.waitEnter();
}

/**
 * =========================================================================================
 * 2. HÀM QUẢN LÝ & TẮT ỨNG DỤNG KHỞI ĐỘNG THÔNG MINH (disableAllStartupApps)
 * =========================================================================================
 * NÂNG CẤP THÔNG MINH:
 * - Quét toàn diện: Registry Run (HKCU, HKLM, WOW64) và Thư mục Startup người dùng.
 * - Nhận diện thông minh (Smart Categorization):
 *   + TUYỆT ĐỐI BẢO VỆ: Bộ gõ tiếng Việt (Unikey, EVKey, OpenKey), Defender, Driver âm thanh,
 *     Driver GPU (NVIDIA, AMD, Intel), Chuột/Phím gaming gear, Touchpad/Hotkeys Laptop, Cloud (OneDrive).
 *   + NHẬN DIỆN APP KHUYÊN TẮT: Spotify, Discord, Steam, Epic Games, uTorrent, IDM, Skype...
 * - Cơ chế an toàn (Non-destructive):
 *   + Tự động sao lưu sang `Run_Disabled` thay vì xóa vĩnh viễn, cho phép KHÔI PHỤC 1-CLICK bất kỳ lúc nào.
 *   + File trong thư mục Startup được đổi tên thành `.disabled` để dễ dàng bật lại.
 */

struct StartupAppInfo {
    string name;
    string command;
    string locationName;
    HKEY hKeyRoot;
    string subKey;
    string filePath;
    bool isFolderFile;
    bool isSafe;
    string category;
};

// Bộ phân tích ứng dụng khởi động thông minh
static pair<bool, string> analyzeStartupApp(const string &name, const string &cmd) {
    string combined = name + " " + cmd;
    transform(combined.begin(), combined.end(), combined.begin(), ::tolower);

    // 1. Bộ gõ tiếng Việt (Ưu tiên số 1 - Tuyệt đối không tắt)
    if (combined.find("unikey") != string::npos || combined.find("evkey") != string::npos ||
        combined.find("openkey") != string::npos || combined.find("gotiengviet") != string::npos) {
        return {true, "Bộ gõ tiếng Việt"};
    }

    // 2. Bảo mật & Antivirus
    if (combined.find("securityhealth") != string::npos || combined.find("windowsdefender") != string::npos ||
        combined.find("msmpeng") != string::npos || combined.find("kaspersky") != string::npos ||
        combined.find("bitdefender") != string::npos || combined.find("avast") != string::npos ||
        combined.find("avg") != string::npos || combined.find("malwarebytes") != string::npos ||
        combined.find("eset") != string::npos) {
        return {true, "Bảo mật & Antivirus"};
    }

    // 3. Driver Âm thanh
    if (combined.find("rtkaud") != string::npos || combined.find("realtek") != string::npos ||
        combined.find("rthdvcpl") != string::npos || combined.find("ravcpl") != string::npos ||
        combined.find("wavessvc") != string::npos || combined.find("wavesmaxxaudio") != string::npos ||
        combined.find("nahimic") != string::npos || combined.find("ctaud") != string::npos) {
        return {true, "Driver âm thanh"};
    }

    // 4. Driver Card màn hình (GPU)
    if (combined.find("nvbackend") != string::npos || combined.find("nvtaskbar") != string::npos ||
        combined.find("nvcpldaemon") != string::npos || combined.find("nvmediacenter") != string::npos ||
        combined.find("nvcontainer") != string::npos || combined.find("nvidia") != string::npos ||
        combined.find("radeon") != string::npos || combined.find("amdlink") != string::npos ||
        combined.find("igfxtray") != string::npos || combined.find("igfxem") != string::npos ||
        combined.find("igfxhk") != string::npos || combined.find("intel") != string::npos) {
        return {true, "Driver Card màn hình"};
    }

    // 5. Driver Chuột, Bàn phím & Gaming Gear
    if (combined.find("lcore") != string::npos || combined.find("lghub") != string::npos ||
        combined.find("logioptions") != string::npos || combined.find("razer") != string::npos ||
        combined.find("steelseries") != string::npos || combined.find("icue") != string::npos ||
        combined.find("corsair") != string::npos || combined.find("ngenuity") != string::npos ||
        combined.find("hyperx") != string::npos) {
        return {true, "Phần mềm Chuột/Phím cơ"};
    }

    // 6. Touchpad, Phím nóng Laptop OEM
    if (combined.find("syntp") != string::npos || combined.find("synaptics") != string::npos ||
        combined.find("elan") != string::npos || combined.find("etdctrl") != string::npos ||
        combined.find("hcontrol") != string::npos || combined.find("atkosd") != string::npos ||
        combined.find("asus") != string::npos || combined.find("armourycrate") != string::npos ||
        combined.find("lenovoutility") != string::npos || combined.find("lenovovantage") != string::npos ||
        combined.find("dellsupportassist") != string::npos || combined.find("hphotkey") != string::npos ||
        combined.find("predatorsense") != string::npos || combined.find("nitrosense") != string::npos) {
        return {true, "Touchpad & Phím nóng Laptop"};
    }

    // 7. Đồng bộ đám mây
    if (combined.find("onedrive") != string::npos || combined.find("googledrive") != string::npos ||
        combined.find("dropbox") != string::npos) {
        return {true, "Đồng bộ đám mây"};
    }

    // 8. Các ứng dụng bên thứ 3 làm chậm máy (Khuyên tắt)
    if (combined.find("spotify") != string::npos) return {false, "Spotify (Làm chậm máy)"};
    if (combined.find("steam") != string::npos) return {false, "Steam (Làm chậm máy)"};
    if (combined.find("discord") != string::npos) return {false, "Discord (Làm chậm máy)"};
    if (combined.find("epicgames") != string::npos) return {false, "Epic Games Launcher"};
    if (combined.find("utorrent") != string::npos || combined.find("bittorrent") != string::npos) return {false, "Phần mềm Torrent ngầm"};
    if (combined.find("idman") != string::npos || combined.find("internet download manager") != string::npos) return {false, "IDM (Tải file ngầm)"};
    if (combined.find("skype") != string::npos) return {false, "Skype"};
    if (combined.find("zalo") != string::npos) return {false, "Zalo"};
    if (combined.find("telegram") != string::npos) return {false, "Telegram"};
    if (combined.find("chrome") != string::npos || combined.find("msedge") != string::npos) return {false, "Trình duyệt chạy ngầm"};

    return {false, "Ứng dụng bên thứ ba"};
}

// Quét toàn bộ ứng dụng khởi động từ Registry & Thư mục Startup
static vector<StartupAppInfo> scanAllStartupApps() {
    vector<StartupAppInfo> list;
    const struct { HKEY hKeyRoot; string subKey; string name; } targets[] = {
        {HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", "HKCU"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "HKLM"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run", "HKLM_WOW64"}
    };

    // 1. Quét Registry Run
    for (const auto &t : targets) {
        HKEY hKey;
        if (RegOpenKeyExA(t.hKeyRoot, t.subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char valName[256];
            char valData[1024];
            DWORD valNameSize, valDataSize, type;
            DWORD index = 0;

            while (true) {
                valNameSize = sizeof(valName);
                valDataSize = sizeof(valData);
                if (RegEnumValueA(hKey, index, valName, &valNameSize, NULL, &type, (LPBYTE)valData, &valDataSize) == ERROR_SUCCESS) {
                    if (type == REG_SZ || type == REG_EXPAND_SZ) {
                        string nameStr(valName);
                        string cmdStr(valData);
                        auto analysis = analyzeStartupApp(nameStr, cmdStr);

                        StartupAppInfo info;
                        info.name = nameStr;
                        info.command = cmdStr;
                        info.locationName = t.name;
                        info.hKeyRoot = t.hKeyRoot;
                        info.subKey = t.subKey;
                        info.isFolderFile = false;
                        info.isSafe = analysis.first;
                        info.category = analysis.second;
                        list.push_back(info);
                    }
                    index++;
                } else break;
            }
            RegCloseKey(hKey);
        }
    }

    // 2. Quét Thư mục Startup của người dùng
    char *appData = std::getenv("APPDATA");
    if (appData) {
        string startupFolder = string(appData) + "\\Microsoft\\Windows\\Start Menu\\Programs\\Startup";
        if (fs::exists(startupFolder)) {
            try {
                for (const auto &entry : fs::directory_iterator(startupFolder)) {
                    if (entry.is_regular_file()) {
                        string ext = entry.path().extension().string();
                        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                        if (ext == ".lnk" || ext == ".bat" || ext == ".cmd") {
                            string fName = entry.path().filename().string();
                            auto analysis = analyzeStartupApp(fName, entry.path().string());

                            StartupAppInfo info;
                            info.name = fName;
                            info.command = entry.path().string();
                            info.locationName = "Startup Folder";
                            info.filePath = entry.path().string();
                            info.isFolderFile = true;
                            info.isSafe = analysis.first;
                            info.category = analysis.second;
                            list.push_back(info);
                        }
                    }
                }
            } catch (...) {}
        }
    }

    return list;
}

// Tắt an toàn 1 ứng dụng (Sao lưu sang Run_Disabled hoặc đổi đuôi file)
static bool disableSingleStartupApp(const StartupAppInfo &item) {
    if (item.isFolderFile) {
        try {
            fs::path src(item.filePath);
            fs::path dst = item.filePath + ".disabled";
            fs::rename(src, dst);
            return true;
        } catch (...) { return false; }
    } else {
        // Sao lưu sang subKey + "_Disabled"
        string backupKeyPath = item.subKey + "_Disabled";
        HKEY hBackup;
        if (RegCreateKeyExA(item.hKeyRoot, backupKeyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hBackup, NULL) == ERROR_SUCCESS) {
            RegSetValueExA(hBackup, item.name.c_str(), 0, REG_SZ, (const BYTE*)item.command.c_str(), (DWORD)(item.command.length() + 1));
            RegCloseKey(hBackup);
        }
        // Xóa khỏi key Run gốc
        HKEY hOriginal;
        if (RegOpenKeyExA(item.hKeyRoot, item.subKey.c_str(), 0, KEY_WRITE, &hOriginal) == ERROR_SUCCESS) {
            LONG res = RegDeleteValueA(hOriginal, item.name.c_str());
            RegCloseKey(hOriginal);
            return (res == ERROR_SUCCESS);
        }
    }
    return false;
}

void SystemOptimizer::disableAllStartupApps() {
    sc.cls();
    cout << "Đang quét và tắt các ứng dụng khởi động làm chậm máy...\n\n";

    vector<StartupAppInfo> appList = scanAllStartupApps();
    if (appList.empty()) {
        cout << " [✓] Không tìm thấy ứng dụng khởi động nào.\n";
        sc.waitEnter();
        return;
    }

    int disabledCount = 0;
    int keptCount = 0;

    for (const auto &app : appList) {
        if (!app.isSafe) {
            if (disableSingleStartupApp(app)) {
                cout << "  [✓] Đã tắt:  " << left << setw(28) << app.name << " (" << app.locationName << ")\n";
                disabledCount++;
            }
        } else {
            cout << "  [-] Giữ lại: " << left << setw(28) << app.name << " [" << app.category << "]\n";
            keptCount++;
        }
    }

    cout << "\n--------------------------------------------------------------\n";
    cout << "[✓] Hoàn tất! Đã tắt " << disabledCount << " ứng dụng làm chậm máy (Bảo vệ " << keptCount << " ứng dụng hệ thống & bộ gõ).\n";
    sc.waitEnter();
}

/**
 * =========================================================================================
 * 3. HÀM SỬA LỖI WINDOWS UPDATE (fixWindowsUpdate)
 * =========================================================================================
 * TÍNH NĂNG:
 * - Khắc phục lỗi kẹt cập nhật 0%, không tải được update, lỗi mã hex (0x800.....).
 * - Bước 1: Dừng các dịch vụ liên quan: wuauserv (Windows Update), cryptSvc (Cryptographic),
 *   bits (Background Intelligent Transfer), msiserver (Windows Installer).
 * - Bước 2: Xóa bộ đệm cập nhật kẹt tại SoftwareDistribution và catroot2.
 * - Bước 3: Khởi động lại toàn bộ dịch vụ để Windows tải lại bản update mới nguyên bản.
 */
void SystemOptimizer::fixWindowsUpdate() {
    cout << "1/3. Dừng dịch vụ Windows Update...\n";
    sc.runAdmin("net stop wuauserv", true); 
    sc.runAdmin("net stop cryptSvc", true); 
    sc.runAdmin("net stop bits", true); 
    sc.runAdmin("net stop msiserver", true);

    cout << "2/3. Xóa cache cập nhật kẹt...\n";
    sc.runCMD("del /f /q %windir%\\SoftwareDistribution\\*.*"); 
    sc.runAdmin("rd /s /q %windir%\\SoftwareDistribution", true); 
    sc.runAdmin("rd /s /q %windir%\\system32\\catroot2", true);

    cout << "3/3. Khởi động lại dịch vụ...\n";
    sc.runAdmin("net start wuauserv", true); 
    sc.runAdmin("net start cryptSvc", true); 
    sc.runAdmin("net start bits", true); 
    sc.runAdmin("net start msiserver", true);

    cout << "\nĐã reset Windows Update thành công!\n";
    sc.waitEnter();
}

/**
 * =========================================================================================
 * 4. HÀM DỌN DẸP BỘ ĐỆM TRÌNH DUYỆT (clearBrowserCache)
 * =========================================================================================
 * TÍNH NĂNG:
 * - Quét tất cả hồ sơ người dùng (Profiles: Default, Profile 1, Profile 2, Guest...) của:
 *   + Google Chrome, Microsoft Edge, Cốc Cốc, Brave Browser, Vivaldi, Opera, Opera GX.
 *   + Mozilla Firefox (hỗ trợ tất cả profile xxxxx.default-release trong AppData và LocalAppData).
 * - Xóa sạch các thư mục cache ngốn dung lượng:
 *   Cache, Code Cache, GPUCache, DawnCache, ShaderCache, Service Worker Caches...
 * - KHÔNG xóa Cookies hay Lịch sử duyệt web (chỉ xóa cache tạm để giải phóng ổ đĩa).
 * 
 * CÁCH BỔ SUNG TRÌNH DUYỆT MỚI:
 * - Thêm đường dẫn User Data vào vector `chromiumBases`.
 * - Thêm tên thư mục cache vào vector `cacheFolderNames` nếu trình duyệt có loại cache mới.
 */
void SystemOptimizer::clearBrowserCache() {
    char *localAppData = std::getenv("LOCALAPPDATA");
    char *appData = std::getenv("APPDATA");
    if (!localAppData) return;
    string baseLocal = string(localAppData);

    // Danh sách đường dẫn User Data của các trình duyệt nhân Chromium
    vector<string> chromiumBases = {
        baseLocal + "\\Google\\Chrome\\User Data",
        baseLocal + "\\Microsoft\\Edge\\User Data",
        baseLocal + "\\CocCoc\\Browser\\User Data",
        baseLocal + "\\BraveSoftware\\Brave-Browser\\User Data",
        baseLocal + "\\Vivaldi\\User Data",
        baseLocal + "\\Opera Software\\Opera Stable",
        baseLocal + "\\Opera Software\\Opera GX Stable"
    };

    if (appData) {
        string baseApp = string(appData);
        chromiumBases.push_back(baseApp + "\\Opera Software\\Opera Stable");
        chromiumBases.push_back(baseApp + "\\Opera Software\\Opera GX Stable");
    }

    // Các thư mục cache thành phần bên trong mỗi profile trình duyệt
    vector<string> cacheFolderNames = {
        "Cache", "Code Cache", "GPUCache", "DawnCache", "ShaderCache", 
        "GrShaderCache", "GraphiteDawnCache", "Service Worker\\CacheStorage", 
        "Service Worker\\ScriptCache"
    };

    cout << "Đang dọn cache trình duyệt...\n";

    // Duyệt qua từng trình duyệt Chromium và dọn từng Profile
    for (const string &baseDir : chromiumBases) {
        if (!fs::exists(baseDir)) continue;

        try {
            for (const auto &entry : fs::directory_iterator(baseDir)) {
                if (!entry.is_directory()) continue;
                string dirName = entry.path().filename().string();
                
                // Nhận diện thư mục Profile
                bool isProfile = (dirName == "Default" || dirName.rfind("Profile", 0) == 0 || 
                                  dirName == "Guest Profile" || dirName == "System Profile");
                
                if (isProfile) {
                    for (const auto &cacheName : cacheFolderNames) {
                        fs::path targetCache = entry.path() / cacheName;
                        wipeFolderContents(targetCache);
                    }
                } else if (dirName == "ShaderCache" || dirName == "GrShaderCache" || dirName == "DawnCache") {
                    wipeFolderContents(entry.path());
                }
            }
        } catch (...) {}
    }

    // Dọn cache Mozilla Firefox (AppData Roaming & Local)
    if (appData) {
        string ffPath = string(appData) + "\\Mozilla\\Firefox\\Profiles";
        if (fs::exists(ffPath)) {
            try {
                for (const auto &profile : fs::directory_iterator(ffPath)) {
                    if (profile.is_directory()) {
                        wipeFolderContents(profile.path() / "cache2");
                        wipeFolderContents(profile.path() / "startupCache");
                        wipeFolderContents(profile.path() / "jumpListCache");
                    }
                }
            } catch (...) {}
        }
    }

    string ffLocal = baseLocal + "\\Mozilla\\Firefox\\Profiles";
    if (fs::exists(ffLocal)) {
        try {
            for (const auto &profile : fs::directory_iterator(ffLocal)) {
                if (profile.is_directory()) {
                    wipeFolderContents(profile.path() / "cache2");
                    wipeFolderContents(profile.path() / "startupCache");
                }
            }
        } catch (...) {}
    }
}

/**
 * =========================================================================================
 * 5. HÀM DỌN DẸP BỘ ĐỆM DÀNH CHO LẬP TRÌNH VIÊN (cleanDevCaches)
 * =========================================================================================
 * TÍNH NĂNG:
 * - Giải phóng hàng chục GB rác tích tụ trong quá trình build code và cài thư viện:
 *   + Node.js / Web: npm-cache, Yarn Cache, pnpm store.
 *   + Python: pip cache.
 *   + .NET / C#: NuGet v3-cache, .nuget packages.
 *   + Golang: go-build cache.
 *   + Java / Android: .gradle caches, .gradle daemon, .android cache.
 *   + Rust: .cargo registry cache.
 *   + Visual Studio Code: Code Cache, CachedData, CachedExtensionVSIXs, workspaceStorage.
 * 
 * CÁCH BỔ SUNG MÔI TRƯỜNG DEV MỚI:
 * - Thêm đường dẫn thư mục cache vào vector `devCachePaths` theo các biến môi trường tương ứng.
 */
static int cleanDirectoryArtifacts(const fs::path &rootPath, const std::vector<std::string> &targetDirNames, const std::vector<std::string> &targetExtensions, long long &freedBytes) {
    if (!fs::exists(rootPath)) return 0;
    int deletedCount = 0;
    std::vector<fs::path> dirsToDelete;
    std::vector<fs::path> filesToDelete;

    std::error_code ec;
    try {
        for (auto it = fs::recursive_directory_iterator(rootPath, fs::directory_options::skip_permission_denied, ec);
             it != fs::recursive_directory_iterator();) {
            if (ec) { ec.clear(); try { it++; } catch (...) { break; } continue; }

            try {
                const auto &entry = *it;
                std::string name = entry.path().filename().string();

                // 1. TUYỆT ĐỐI BẢO VỆ .git & .github (Không bao giờ xóa, không duyệt sâu để giữ nguyên trạng thái)
                if (name == ".git" || name == ".github" || name == ".gitignore" || name == ".gitattributes") {
                    if (entry.is_directory(ec)) {
                        it.disable_recursion_pending();
                    }
                    it.increment(ec);
                    continue;
                }

                if (entry.is_directory(ec)) {
                    bool isTarget = false;
                    for (const auto &td : targetDirNames) {
                        // Không bao giờ cho phép target là .git
                        if (td == ".git" || td == ".github") continue;
                        if (name == td) {
                            isTarget = true;
                            break;
                        }
                    }

                    if (isTarget) {
                        dirsToDelete.push_back(entry.path());
                        it.disable_recursion_pending(); // Không cần đệ quy vào trong thư mục sắp xóa
                    }
                } else if (entry.is_regular_file(ec)) {
                    std::string ext = entry.path().extension().string();
                    for (const auto &te : targetExtensions) {
                        if (ext == te) {
                            filesToDelete.push_back(entry.path());
                            break;
                        }
                    }
                }
            } catch (...) {}
            it.increment(ec);
        }
    } catch (...) {}

    // Xóa an toàn các file artifacts đơn lẻ (.pyc, .pyo...)
    for (const auto &f : filesToDelete) {
        try {
            std::error_code sizeEc;
            auto sz = fs::file_size(f, sizeEc);
            if (!sizeEc) freedBytes += sz;

            SetFileAttributesA(f.string().c_str(), FILE_ATTRIBUTE_NORMAL);
            if (fs::remove(f, ec)) {
                deletedCount++;
            }
        } catch (...) {}
    }

    // Xóa an toàn các thư mục artifacts (node_modules, node_module, __pycache__, .turbo...)
    for (const auto &d : dirsToDelete) {
        try {
            // Tính tổng dung lượng thư mục trước khi xóa
            try {
                for (const auto &sub : fs::recursive_directory_iterator(d, fs::directory_options::skip_permission_denied, ec)) {
                    if (!ec && sub.is_regular_file(ec)) {
                        freedBytes += sub.file_size(ec);
                    }
                }
            } catch (...) {}

            // Gỡ bỏ thuộc tính Read-Only và xóa tận gốc thư mục (khắc phục lỗi Access Denied trên Windows)
            string cmd = "cmd /c \"attrib -r -s -h /s /d \"" + d.string() + "\\*\" >nul 2>&1 & rd /s /q \"" + d.string() + "\" >nul 2>&1\"";
            system(cmd.c_str());

            if (!fs::exists(d)) {
                deletedCount++;
            } else {
                fs::remove_all(d, ec);
                if (!fs::exists(d)) deletedCount++;
            }
        } catch (...) {}
    }

    return deletedCount;
}

void SystemOptimizer::cleanDevCaches(bool interactive) {
    char *localAppData = std::getenv("LOCALAPPDATA");
    char *appData = std::getenv("APPDATA");
    char *userProfile = std::getenv("USERPROFILE");

    string baseLocal = localAppData ? string(localAppData) : "";
    string baseApp   = appData ? string(appData) : "";
    string baseUser  = userProfile ? string(userProfile) : "";

    // Xác định thư mục dự án / workspace để quét
    fs::path scanRoot = fs::current_path();
    if (scanRoot.filename() == "bin" && fs::exists(scanRoot.parent_path())) {
        scanRoot = scanRoot.parent_path();
    }

    if (interactive) {
        sc.cls();
        cout << "DỌN RÁC MÔI TRƯỜNG DEV\n"
             << "Mục tiêu quét: " << scanRoot.string() << "\n\n";
    } else {
        cout << "Đang dọn rác Dev Caches...\n";
    }

    // 1. KIỂM TRA & DỌN DẸP PYTHON
    bool hasPython = SystemCore::runRawCommand("where python >nul 2>nul") || 
                     SystemCore::runRawCommand("where py >nul 2>nul") ||
                     (!baseLocal.empty() && fs::exists(baseLocal + "\\pip\\cache"));

    if (hasPython) {
        if (!baseLocal.empty()) wipeFolderContents(baseLocal + "\\pip\\cache");

        long long pyFreed = 0;
        int pyDeleted = cleanDirectoryArtifacts(scanRoot, 
            {"__pycache__", ".pytest_cache", ".mypy_cache", ".ruff_cache", ".tox"}, 
            {".pyc", ".pyo"}, 
            pyFreed);

        if (interactive) {
            cout << " [✓] Python : Đã dọn Pip cache, " << pyDeleted << " mục (__pycache__/.pyc) - " << SystemCore::formatSize(pyFreed) << "\n";
        }
    }

    // 2. KIỂM TRA & DỌN DẸP NODE.JS / JAVASCRIPT
    bool hasNode = SystemCore::runRawCommand("where node >nul 2>nul") ||
                   SystemCore::runRawCommand("where npm >nul 2>nul") ||
                   (!baseLocal.empty() && (fs::exists(baseLocal + "\\npm-cache") || fs::exists(baseLocal + "\\pnpm\\store")));

    if (hasNode) {
        if (!baseLocal.empty()) {
            wipeFolderContents(baseLocal + "\\npm-cache");
            wipeFolderContents(baseLocal + "\\Yarn\\Cache");
            wipeFolderContents(baseLocal + "\\pnpm\\store");
            wipeFolderContents(baseLocal + "\\electron\\Cache");
            wipeFolderContents(baseLocal + "\\Microsoft\\TypeScript");
            wipeFolderContents(baseLocal + "\\deno\\deps");
        }
        if (!baseApp.empty()) {
            wipeFolderContents(baseApp + "\\npm-cache");
        }
        if (!baseUser.empty()) {
            wipeFolderContents(baseUser + "\\.turbo");
            wipeFolderContents(baseUser + "\\.npm");
            wipeFolderContents(baseUser + "\\.yarn");
            wipeFolderContents(baseUser + "\\.pnpm-store");
        }

        long long nodeFreed = 0;
        int nodeDeleted = cleanDirectoryArtifacts(scanRoot, 
            {"node_modules", "node_module", ".turbo", ".next", ".nuxt", ".parcel-cache", ".svelte-kit", ".cache"}, 
            {}, 
            nodeFreed);

        if (interactive) {
            cout << " [✓] Node.js: Đã dọn npm/yarn/pnpm, " << nodeDeleted << " thư mục (node_modules/cache) - " << SystemCore::formatSize(nodeFreed) << "\n";
        }
    }

    // 3. KIỂM TRA & DỌN DẸP JAVA / ANDROID
    bool hasJava = SystemCore::runRawCommand("where java >nul 2>nul") ||
                   SystemCore::runRawCommand("where javac >nul 2>nul") ||
                   (!baseUser.empty() && (fs::exists(baseUser + "\\.gradle") || fs::exists(baseUser + "\\.android")));

    if (hasJava) {
        if (!baseUser.empty()) {
            wipeFolderContents(baseUser + "\\.gradle\\caches");
            wipeFolderContents(baseUser + "\\.gradle\\daemon");
            wipeFolderContents(baseUser + "\\.android\\cache");
            wipeFolderContents(baseUser + "\\.m2\\repository");
        }
        if (interactive) {
            cout << " [✓] Java   : Đã dọn .gradle caches, daemon, .m2\n";
        }
    }

    // 4. DỌN DẸP CÁC BỘ ĐỆM DEV KHÁC (VS Code, Cursor, Go, Rust, .NET NuGet)
    if (!baseApp.empty()) {
        wipeFolderContents(baseApp + "\\Code\\Cache");
        wipeFolderContents(baseApp + "\\Code\\CachedData");
        wipeFolderContents(baseApp + "\\Code\\CachedExtensionVSIXs");
        wipeFolderContents(baseApp + "\\Code\\User\\workspaceStorage");
        wipeFolderContents(baseApp + "\\Cursor\\Cache");
        wipeFolderContents(baseApp + "\\Cursor\\CachedData");
        wipeFolderContents(baseApp + "\\Cursor\\User\\workspaceStorage");
    }
    if (!baseLocal.empty()) {
        wipeFolderContents(baseLocal + "\\NuGet\\v3-cache");
        wipeFolderContents(baseLocal + "\\go-build");
    }
    if (!baseUser.empty()) {
        wipeFolderContents(baseUser + "\\.cargo\\registry\\cache");
        wipeFolderContents(baseUser + "\\.cargo\\registry\\src");
        wipeFolderContents(baseUser + "\\.nuget\\packages");
        wipeFolderContents(baseUser + "\\.rustup\\downloads");
    }

    if (interactive) {
        cout << " [✓] Khác   : Đã dọn VS Code, Cursor, NuGet, Go, Cargo...\n"
             << "\nHoàn tất dọn dẹp!\n";
        sc.waitEnter();
    }
}

// --- HỆ THỐNG TĂNG TỐC & TỐI ƯU ĐA TẦNG (MULTI-TIER PERFORMANCE TUNING) ---

// Tầng 1: Tối ưu Khởi động (Tắt app bên thứ ba làm chậm máy, bảo vệ 100% Bộ gõ & Driver)
int SystemOptimizer::runOptimizeTier1() {
    vector<StartupAppInfo> appList = scanAllStartupApps();
    int disabledCount = 0;
    int safeCount = 0;
    for (const auto &app : appList) {
        if (!app.isSafe) {
            if (disableSingleStartupApp(app)) {
                cout << "     ├── [Tắt] " << left << setw(24) << app.name << " (" << app.locationName << ")\n";
                disabledCount++;
            }
        } else {
            safeCount++;
        }
    }
    if (safeCount > 0) {
        cout << "     ├── [Bảo vệ] " << safeCount << " ứng dụng hệ thống & bộ gõ tiếng Việt\n";
    }
    return disabledCount;
}

// Tầng 2: Tối ưu Dịch vụ ngầm (Maps, Wallet, Telemetry, Demo, ErrorReporting...)
int SystemOptimizer::runOptimizeTier2() {
    struct SvcCheck { string name; string desc; };
    vector<SvcCheck> svcs = {
        {"MapsBroker", "Bản đồ ngoại tuyến Windows"},
        {"WalletService", "Ví điện tử & Thanh toán Windows"},
        {"RetailDemo", "Chế độ demo trình diễn tại cửa hàng"},
        {"DiagTrack", "Dịch vụ thu thập Telemetry ngầm"},
        {"dmwappushservice", "Định tuyến trắc lượng WAP Push"},
        {"WerSvc", "Báo cáo sự cố về Microsoft"},
        {"RemoteRegistry", "Chỉnh sửa Registry từ xa qua mạng"},
        {"wisvc", "Chương trình thử nghiệm Windows Insider"}
    };

    int disabledCount = 0;
    for (const auto &s : svcs) {
        if (ServiceControlAPI(s.name, SERVICE_DISABLED, true)) {
            cout << "     ├── [Tối ưu] " << left << setw(18) << s.name << " (" << s.desc << ")\n";
            disabledCount++;
        }
    }
    return disabledCount;
}

// Tầng 3: Tối ưu Giao diện, Taskbar & Độ nhạy Windows (Kiểm tra trước khi thay đổi, chỉ khởi động lại Explorer nếu cần)
bool SystemOptimizer::runOptimizeTier3() {
    struct TaskbarSetting {
        string keyPath;
        string valueName;
        DWORD targetValue;
        string desc;
    };

    vector<TaskbarSetting> settings = {
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Search", "SearchboxTaskbarMode", 0, "Searchbox (Thu gọn tìm kiếm)"},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "TaskbarDa", 0, "Widgets (Tắt tin tức widget)"},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "TaskbarMn", 0, "Chat Teams (Tắt biểu tượng chat)"},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "ShowTaskViewButton", 0, "Task View (Tắt xem tác vụ)"},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Feeds", "ShellFeedsTaskbarViewMode", 2, "Feeds News (Tắt tin tức Win 10)"},
        {"Software\\Policies\\Microsoft\\Windows\\WindowsCopilot", "TurnOffWindowsCopilot", 1, "Copilot AI (Tắt nút Copilot)"},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", "EnableTransparency", 0, "Transparency (Tắt trong suốt tiết kiệm GPU)"},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "SnapAssist", 0, "Snap Assist (Tắt gợi ý chia đôi cửa sổ)"},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", "SilentInstalledAppsEnabled", 0, "Silent Apps (Chặn Store cài app rác ngầm)"},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", "SubscribedContent-310093Enabled", 0, "Start Ads (Tắt quảng cáo Start Menu)"},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", "SubscribedContent-338388Enabled", 0, "Settings Tips (Tắt mẹo gợi ý Settings)"},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", "SubscribedContent-338389Enabled", 0, "Explorer Ads (Tắt quảng cáo trong Explorer)"},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "ShowSyncProviderNotifications", 0, "Sync Notifications (Tắt quảng cáo OneDrive)"}
    };

    int newlyChanged = 0;
    for (const auto &item : settings) {
        HKEY hKey;
        DWORD currentVal = 0;
        DWORD dataSize = sizeof(DWORD);
        bool alreadySet = false;

        if (RegOpenKeyExA(HKEY_CURRENT_USER, item.keyPath.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExA(hKey, item.valueName.c_str(), NULL, NULL, (LPBYTE)&currentVal, &dataSize) == ERROR_SUCCESS) {
                if (currentVal == item.targetValue) alreadySet = true;
            }
            if (!alreadySet) {
                if (RegSetValueExA(hKey, item.valueName.c_str(), 0, REG_DWORD, (const BYTE*)&item.targetValue, sizeof(DWORD)) == ERROR_SUCCESS) {
                    cout << "     ├── [Tinh chỉnh] " << item.desc << "\n";
                    newlyChanged++;
                }
            }
            RegCloseKey(hKey);
        } else {
            if (RegCreateKeyExA(HKEY_CURRENT_USER, item.keyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                if (RegSetValueExA(hKey, item.valueName.c_str(), 0, REG_DWORD, (const BYTE*)&item.targetValue, sizeof(DWORD)) == ERROR_SUCCESS) {
                    cout << "     ├── [Tinh chỉnh] " << item.desc << "\n";
                    newlyChanged++;
                }
                RegCloseKey(hKey);
            }
        }
    }

    // Bỏ độ trễ mở menu chuột phải (Desktop MenuShowDelay = 0)
    HKEY hDesktop;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ | KEY_WRITE, &hDesktop) == ERROR_SUCCESS) {
        char valBuf[32] = {0};
        DWORD valSz = sizeof(valBuf);
        bool delayIsZero = false;
        if (RegQueryValueExA(hDesktop, "MenuShowDelay", NULL, NULL, (LPBYTE)valBuf, &valSz) == ERROR_SUCCESS) {
            if (string(valBuf) == "0") delayIsZero = true;
        }
        if (!delayIsZero) {
            const char zeroStr[] = "0";
            RegSetValueExA(hDesktop, "MenuShowDelay", 0, REG_SZ, (const BYTE*)zeroStr, 2);
            cout << "     ├── [Tinh chỉnh] Giảm độ trễ Menu chuột phải về 0ms\n";
            newlyChanged++;
        }
        RegCloseKey(hDesktop);
    }

    // Tắt Telemetry chẩn đoán ngầm trong HKLM (yêu cầu Admin)
    HKEY hLM;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", 0, KEY_READ | KEY_WRITE, &hLM) == ERROR_SUCCESS) {
        DWORD curTelem = 1, szTelem = sizeof(DWORD);
        if (RegQueryValueExA(hLM, "AllowTelemetry", NULL, NULL, (LPBYTE)&curTelem, &szTelem) != ERROR_SUCCESS || curTelem != 0) {
            DWORD zeroVal = 0;
            RegSetValueExA(hLM, "AllowTelemetry", 0, REG_DWORD, (const BYTE*)&zeroVal, sizeof(DWORD));
        }
        RegCloseKey(hLM);
    }

    // CHỈ KHỞI ĐỘNG LẠI EXPLORER NẾU CÓ THAY ĐỔI MỚI THỰC SỰ!
    if (newlyChanged > 0) {
        sc.runCMD("taskkill /f /im explorer.exe >nul 2>&1 & start explorer.exe");
        return true;
    }
    return false;
}

// Menu điều phối Tăng tốc & Tối ưu Đa Tầng
void SystemOptimizer::multiTierPerformanceOptimize() {
    while (true) {
        sc.cls();
        cout << "HỆ THỐNG TĂNG TỐC & TỐI ƯU ĐA TẦNG\n\n"
             << " [1] Tầng 1: Tối ưu Khởi động (Tắt app làm chậm máy, bảo vệ Bộ gõ & Driver)\n"
             << " [2] Tầng 2: Tối ưu Dịch vụ ngầm (Tắt Maps, Ví điện tử, Telemetry, Demo...)\n"
             << " [3] Tầng 3: Tối ưu Giao diện & Độ nhạy Windows (Taskbar, bỏ độ trễ UI)\n"
             << " [4] Tối ưu liên hoàn cả 3 Tầng\n"
             << " [5] Quản lý dịch vụ Windows nâng cao\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";

        int choice = sc.readInt("");
        if (choice == 0) break;

        if (choice == 5) {
            turnOffServicesMenu();
            continue;
        }

        sc.cls();
        cout << "=== TIẾN TRÌNH TĂNG TỐC & TỐI ƯU ĐA TẦNG ===\n\n";

        if (choice == 1 || choice == 4) {
            cout << " [*] Tầng 1: Đang quét và tắt ứng dụng khởi động làm chậm máy...\n";
            int count = runOptimizeTier1();
            cout << "     └── [✓ Xong] " << (count > 0 ? ("Đã tắt " + to_string(count) + " app làm chậm máy") : "Tất cả ứng dụng khởi động đã tối ưu") << "\n\n";
        }

        if (choice == 2 || choice == 4) {
            cout << " [*] Tầng 2: Đang vô hiệu hóa các dịch vụ chạy ngầm vô ích...\n";
            int count = runOptimizeTier2();
            cout << "     └── [✓ Xong] Đã tối ưu " << count << " dịch vụ ngầm (Maps, Wallet, Telemetry, ErrorReporting)\n\n";
        }

        if (choice == 3 || choice == 4) {
            cout << " [*] Tầng 3: Kiểm tra và tối ưu Giao diện & Taskbar...\n";
            bool restarted = runOptimizeTier3();
            if (restarted) {
                cout << "     └── [✓ Xong] Đã áp dụng tinh chỉnh mới và làm mới Explorer.\n\n";
            } else {
                cout << "     └── [✓ Xong] Taskbar & Giao diện đã tinh gọn từ trước (Bỏ qua reset Explorer, tránh chớp màn hình).\n\n";
            }
        }

        cout << "==============================================================\n";
        cout << "[✓] Hoàn tất quy trình tối ưu! Hệ thống đã sẵn sàng với hiệu năng tối đa.\n\n";
        sc.waitEnter();
    }
}

void SystemOptimizer::optimizeSystemPRO() {
    multiTierPerformanceOptimize();
}

/**
 * =========================================================================================
 * 7. HÀM ĐIỀU KHIỂN DỊCH VỤ WINDOWS QUA WIN32 SCM API (ServiceControlAPI)
 * =========================================================================================
 * TÍNH NĂNG:
 * - Sử dụng trực tiếp API Service Control Manager của Windows (OpenSCManager, OpenService, ChangeServiceConfig)
 *   thay vì gọi lệnh `sc config` bên ngoài, giúp tốc độ thực thi nhanh vượt trội và kiểm soát lỗi chính xác.
 * 
 * @param serviceName Tên định danh của dịch vụ (ví dụ: "wuauserv", "DiagTrack", "SysMain")
 * @param startupType Kiểu khởi động (SERVICE_AUTO_START, SERVICE_DEMAND_START/Manual, SERVICE_DISABLED)
 * @param stopService Có dừng ngay lập tức nếu dịch vụ đang chạy hay không
 * @return bool true nếu cấu hình thành công, false nếu thất bại (thiếu quyền Admin hoặc service không tồn tại)
 */
bool SystemOptimizer::ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService) {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) return false;

    SC_HANDLE svc = OpenServiceA(scm, serviceName.c_str(), SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG | SERVICE_STOP | SERVICE_START);
    if (!svc) { CloseServiceHandle(scm); return false; }

    // Kiểm tra cấu hình hiện tại để tránh can thiệp nếu đã đúng
    DWORD bytesNeeded = 0;
    QueryServiceConfigA(svc, NULL, 0, &bytesNeeded);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        std::vector<BYTE> buf(bytesNeeded);
        LPQUERY_SERVICE_CONFIGA pConfig = (LPQUERY_SERVICE_CONFIGA)buf.data();
        if (QueryServiceConfigA(svc, pConfig, bytesNeeded, &bytesNeeded)) {
            if (pConfig->dwStartType == startupType) {
                SERVICE_STATUS_PROCESS ssp;
                DWORD sspNeeded = 0;
                if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &sspNeeded)) {
                    if (!stopService || ssp.dwCurrentState == SERVICE_STOPPED) {
                        CloseServiceHandle(svc);
                        CloseServiceHandle(scm);
                        return true; // Đã chuẩn từ trước, không cần ghi đè
                    }
                }
            }
        }
    }

    bool configSuccess = ChangeServiceConfigA(svc, SERVICE_NO_CHANGE, startupType, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if (stopService) {
        SERVICE_STATUS status;
        ControlService(svc, SERVICE_CONTROL_STOP, &status);
    }
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return configSuccess;
}

/**
 * =========================================================================================
 * 8. MENU QUẢN LÝ DỊCH VỤ WINDOWS (turnOffServicesMenu)
 * =========================================================================================
 * TÍNH NĂNG:
 * - Cung cấp danh sách 20 dịch vụ chạy ngầm phổ biến có thể tắt an toàn hoặc chuyển Manual:
 *   + Windows Update (wuauserv, UsoSvc, WaaSMedicSvc)
 *   + Báo cáo lỗi (WerSvc) & Thông báo ngầm (WpnService, WpnUserService)
 *   + Xbox Live services (XblAuthManager, XblGameSave, XboxNetApiSvc)
 *   + Telemetry thu thập dữ liệu (DiagTrack, dmwappushservice)
 *   + Trình cập nhật ngầm Edge (EdgeUpdate), Thử nghiệm Insider (wisvc)
 *   + Dịch vụ in ấn Spooler, Bluetooth (BthServ), Maps ngoại tuyến, Remote Registry, SysMain (Superfetch), Wallet...
 * - Cho phép chọn [A] Cấu hình tất cả thành Manual / Disabled hoặc cấu hình từng dịch vụ riêng lẻ.
 * 
 * CÁCH BỔ SUNG THÊM SERVICE MỚI:
 * - Thêm 1 dòng `{"tên_service", "Mô tả tiếng Việt và ghi chú"}` vào vector `targetSvcs`.
 */
void SystemOptimizer::turnOffServicesMenu() {
    sc.cls();
    struct SvcInfo { std::string name; std::string desc; };
    
    // Danh sách các dịch vụ Windows có thể tối ưu
    std::vector<SvcInfo> targetSvcs = {
        {"CDPSvc", "Connected Devices Platform (Nền tảng Phone Link / Cross Device)"},
        {"CDPUserSvc", "CDP User Service (Tiến trình liên kết Phone Link chạy ngầm)"},
        {"OneSyncSvc", "Sync Host (Đồng bộ dữ liệu ngầm Phone / Mail / People)"},
        {"wuauserv", "Windows Update (Ngăn tự động cập nhật hệ thống)"},
        {"UsoSvc", "Update Orchestrator Service (Điều phối cập nhật Windows)"},
        {"WaaSMedicSvc", "Windows Update Medic Service (Ngăn tự động bật lại Update)"},
        {"WerSvc", "Windows Error Reporting Service (Báo cáo lỗi về Microsoft)"},
        {"WpnService", "Windows Push Notifications System (Hệ thống thông báo/Widgets)"},
        {"WpnUserService", "Windows Push Notifications User Service (Tắt WebView2 ngầm)"},
        {"WpcSvc", "Parental Controls (Tính năng quản lý trẻ em gia đình)"},
        {"XblAuthManager", "Xbox Live AuthManager (Xác thực tài khoản Xbox)"},
        {"XblGameSave", "Xbox Live Game Save (Đồng bộ dữ liệu game)"},
        {"XboxNetApiSvc", "Xbox Live Networking Service (Mạng Xbox)"},
        {"DiagTrack", "Connected User Experiences and Telemetry (Thu thập dữ liệu ngầm)"},
        {"dmwappushservice", "WAP Push Message Routing Service (Định tuyến trắc lượng)"},
        {"EdgeUpdate", "Microsoft Edge Update Service (Cập nhật trình duyệt ngầm)"},
        {"wisvc", "Windows Insider Service (Dịch vụ chương trình thử nghiệm)"},
        {"BthServ", "Bluetooth Support Service (Tắt nếu PC không có Bluetooth)"},
        {"MapsBroker", "Downloaded Maps Manager (Quản lý bản đồ ngoại tuyến)"},
        {"RemoteRegistry", "Remote Registry (Cho phép sửa Registry từ xa)"},
        {"SysMain", "Superfetch / SysMain (Nên tắt hoàn toàn nếu dùng SSD)"},
        {"WalletService", "Wallet Service (Ví điện tử và thanh toán Windows)"},
        {"RetailDemo", "Retail Demo Service (Chế độ demo cửa hàng trưng bày)"},
        {"lfsvc", "Geolocation Service (Dịch vụ định vị vị trí ngầm)"}
    };

    while (true) {
        sc.cls();
        
        for (size_t i = 0; i < targetSvcs.size(); ++i) {
            std::cout << " [" << std::setw(2) << i + 1 << "] " 
                      << std::left << std::setw(30) << (targetSvcs[i].desc.substr(0, 45) + "...") 
                      << " [" << targetSvcs[i].name << "]\n";
        }
        std::cout << "\n [A] Cấu hình tất cả\n"
                  << " [0] Quay lại\n\n"
                  << "Chọn dịch vụ, hoặc [A]/[0]: ";
        std::string input;
        std::cin >> input;

        if (input == "0") return;

        DWORD startType = SERVICE_DISABLED;
        std::string modeName = "";

        // --- CẤU HÌNH HÀNG LOẠT TẤT CẢ DỊCH VỤ ---
        if (input == "A" || input == "a") {
            std::cout << "\nCấu hình tất cả dịch vụ:\n"
                      << " [1] Manual   (Chỉ chạy khi cần)\n"
                      << " [2] Disabled (Tắt hoàn toàn)\n"
                      << " [0] Hủy\n\n";
            int action = sc.readInt("Chọn: ");
            if (action == 0) continue;

            startType = (action == 1) ? SERVICE_DEMAND_START : SERVICE_DISABLED;
            modeName = (action == 1) ? "MANUAL" : "DISABLED";

            std::cout << "\nĐang thực thi cấu hình...\n";
            int successCount = 0;
            for (const auto &s : targetSvcs) {
                if (ServiceControlAPI(s.name, startType, true)) {
                    std::cout << "  " << modeName << ": " << s.name << "\n";
                    successCount++;
                } else {
                    std::cout << "  Thất bại: " << s.name << "\n";
                }
            }
            std::cout << "\nĐã cấu hình " << successCount << "/" << targetSvcs.size() << " dịch vụ.\n";
            sc.waitEnter();
        }
        // --- CẤU HÌNH TỪNG DỊCH VỤ RIÊNG LẺ THEO SỐ THỨ TỰ ---
        else {
            if (input.empty()) {
                std::cout << "Chưa nhập lựa chọn!\n";
                Sleep(800);
                continue;
            }
            try {
                int idx = std::stoi(input) - 1;
                if (idx >= 0 && idx < (int)targetSvcs.size()) {
                    sc.cls();
                    std::cout << "Dịch vụ: " << targetSvcs[idx].desc << " [" << targetSvcs[idx].name << "]\n\n"
                              << " [1] Manual   (Chỉ chạy khi cần)\n"
                              << " [2] Disabled (Tắt hẳn)\n"
                              << " [0] Hủy\n\n";
                    int action = sc.readInt("Chọn: ");
                    if (action == 0) continue;

                    startType = (action == 1) ? SERVICE_DEMAND_START : SERVICE_DISABLED;
                    modeName = (action == 1) ? "MANUAL" : "DISABLED";

                    std::cout << "\nĐang xử lý " << targetSvcs[idx].name << "...\n";
                    if (ServiceControlAPI(targetSvcs[idx].name, startType, true)) {
                        std::cout << "Đã chuyển sang: " << modeName << "\n";
                    } else {
                        std::cout << "Thất bại! Cần quyền Administrator.\n";
                    }
                    sc.waitEnter();
                } else {
                    std::cout << "Lựa chọn không hợp lệ!\n";
                    Sleep(800);
                }
            }
            catch (...) {
                std::cout << "Lựa chọn không hợp lệ!\n";
                Sleep(800);
            }
        }
    }
}

/**
 * =========================================================================================
 * 9. HÀM TỐI ƯU HÓA GIAO DIỆN TASKBAR (optimizeTaskbar)
 * =========================================================================================
 * TÍNH NĂNG:
 * - Kiểm tra và tắt các icon/tính năng gây tốn tài nguyên trên thanh Taskbar Windows 11 & Windows 10:
 *   + SearchboxTaskbarMode = 0: Tắt / thu gọn thanh tìm kiếm Search.
 *   + TaskbarDa = 0: Tắt biểu tượng Widgets (Tin tức/Thời tiết).
 *   + TaskbarMn = 0: Tắt biểu tượng Chat (Microsoft Teams).
 *   + ShowTaskViewButton = 0: Tắt nút Task View (Xem tác vụ).
 *   + ShellFeedsTaskbarViewMode = 2: Tắt News & Interests trên Windows 10.
 *   + TurnOffWindowsCopilot = 1: Tắt nút trợ lý ảo Copilot AI trên Taskbar.
 * - Kiểm tra trạng thái hiện tại trong Registry (tránh ghi đè nếu đã tắt từ trước).
 * - Nếu có thay đổi mới: Tự động khởi động lại explorer.exe để cập nhật giao diện ngay tức thì.
 * 
 * CÁCH BỔ SUNG TÙY CHỈNH TASKBAR MỚI:
 * - Thêm struct `TaskbarSetting` mới vào vector `settings`.
 */
void SystemOptimizer::optimizeTaskbar() {
    sc.cls();
    cout << "Đang kiểm tra và tối ưu Taskbar & Giao diện...\n\n";
    bool changed = runOptimizeTier3();
    if (changed) {
        cout << "\n[✓] Đã áp dụng tinh chỉnh mới và làm mới Taskbar thành công.\n\n";
    } else {
        cout << "\n[✓] Taskbar & Giao diện đã ở trạng thái tối ưu chuẩn từ trước (Bỏ qua, không reset Explorer).\n\n";
    }
    sc.waitEnter();
}
