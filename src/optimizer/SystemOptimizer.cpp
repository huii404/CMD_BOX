#include "SystemOptimizer.h"
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

SystemOptimizer::SystemOptimizer(SystemCore &s) : sc(s), cleaner(s) {}

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
    DWORD regType;
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
            DWORD maxValueNameLen = 256;
            DWORD maxValueLen = 1024;
            RegQueryInfoKeyA(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &maxValueNameLen, &maxValueLen, NULL, NULL);
            maxValueNameLen += 1;
            if (maxValueNameLen < 256) maxValueNameLen = 256;
            if (maxValueLen < 1024) maxValueLen = 1024;

            std::vector<char> valNameBuf(maxValueNameLen);
            std::vector<char> valDataBuf(maxValueLen);
            DWORD index = 0;

            while (true) {
                DWORD valNameSize = static_cast<DWORD>(valNameBuf.size());
                DWORD valDataSize = static_cast<DWORD>(valDataBuf.size());
                DWORD type = 0;
                LONG ret = RegEnumValueA(hKey, index, valNameBuf.data(), &valNameSize, NULL, &type, (LPBYTE)valDataBuf.data(), &valDataSize);
                if (ret == ERROR_NO_MORE_ITEMS) {
                    break;
                } else if (ret == ERROR_MORE_DATA) {
                    valDataBuf.resize(valDataSize + 1024);
                    valNameBuf.resize(valNameSize + 256);
                    continue;
                } else if (ret != ERROR_SUCCESS) {
                    index++;
                    continue;
                }

                if (type == REG_SZ || type == REG_EXPAND_SZ) {
                    string nameStr(valNameBuf.data(), valNameSize);
                    string cmdStr(valDataBuf.data());
                    auto analysis = analyzeStartupApp(nameStr, cmdStr);

                    StartupAppInfo info;
                    info.name = nameStr;
                    info.command = cmdStr;
                    info.locationName = t.name;
                    info.hKeyRoot = t.hKeyRoot;
                    info.subKey = t.subKey;
                    info.filePath = "";
                    info.isFolderFile = false;
                    info.isSafe = analysis.first;
                    info.category = analysis.second;
                    info.regType = type;
                    list.push_back(info);
                }
                index++;
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
                            info.hKeyRoot = NULL;
                            info.subKey = "";
                            info.filePath = entry.path().string();
                            info.isFolderFile = true;
                            info.isSafe = analysis.first;
                            info.category = analysis.second;
                            info.regType = 0;
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
        HKEY hBackup = NULL;
        if (RegCreateKeyExA(item.hKeyRoot, backupKeyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hBackup, NULL) != ERROR_SUCCESS) {
            return false;
        }
        DWORD rType = (item.regType == REG_EXPAND_SZ) ? REG_EXPAND_SZ : REG_SZ;
        LONG setRes = RegSetValueExA(hBackup, item.name.c_str(), 0, rType, (const BYTE*)item.command.c_str(), (DWORD)(item.command.length() + 1));
        RegCloseKey(hBackup);
        if (setRes != ERROR_SUCCESS) {
            return false;
        }

        // Chỉ khi sao lưu thành công mới xóa khỏi key Run gốc
        HKEY hOriginal = NULL;
        if (RegOpenKeyExA(item.hKeyRoot, item.subKey.c_str(), 0, KEY_WRITE, &hOriginal) == ERROR_SUCCESS) {
            LONG res = RegDeleteValueA(hOriginal, item.name.c_str());
            RegCloseKey(hOriginal);
            return (res == ERROR_SUCCESS);
        }
    }
    return false;
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
    cout << "\n[*] Đang reset Windows Update (Admin)...\n";
    string batContent = 
        "@echo off\n"
        "set \"failed=0\"\n"
        "chcp 65001 >nul\n"
        "echo [1/3] Dung cac dich vu Windows Update...\n"
        "net stop wuauserv >nul 2>&1\n"
        "net stop cryptSvc >nul 2>&1\n"
        "net stop bits >nul 2>&1\n"
        "net stop msiserver >nul 2>&1\n"
        "if errorlevel 2 set \"failed=1\"\n"
        "\n"
        "echo [2/3] Xoa cache cap nhat ton dong...\n"
        "del /f /q \"%windir%\\SoftwareDistribution\\*.*\" >nul 2>&1\n"
        "rd /s /q \"%windir%\\SoftwareDistribution\" >nul 2>&1\n"
        "rd /s /q \"%windir%\\system32\\catroot2\" >nul 2>&1\n"
        "if exist \"%windir%\\SoftwareDistribution\" set \"failed=1\"\n"
        "if exist \"%windir%\\system32\\catroot2\" set \"failed=1\"\n"
        "\n"
        "echo [3/3] Khoi dong lai cac dich vu...\n"
        "net start msiserver >nul 2>&1\n"
        "net start bits >nul 2>&1\n"
        "net start cryptSvc >nul 2>&1\n"
        "net start wuauserv >nul 2>&1\n"
        "if errorlevel 1 set \"failed=1\"\n"
        "exit /b %failed%\n";

    if (SystemCore::runBatchAsAdmin(batContent, "Reset Windows Update")) {
        cout << "\n[✓] Đã reset Windows Update.\n";
    } else {
        cout << "\n[!] Reset thất bại; cần quyền Admin.\n";
    }
    sc.waitEnter();
}

// --- HỆ THỐNG TĂNG TỐC & TỐI ƯU HIỆU NĂNG THEO NHIỆM VỤ ---

// Nhiệm vụ 1: Tối ưu Khởi động (Tắt app bên thứ ba làm chậm máy, bảo vệ 100% Bộ gõ & Driver)
int SystemOptimizer::optimizeStartupApps() {
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

// Nhiệm vụ 2: Tối ưu Dịch vụ ngầm (Maps, Wallet, Telemetry, Demo, ErrorReporting...)
int SystemOptimizer::optimizeBackgroundServices() {
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

// Nhiệm vụ 3: Tối ưu Giao diện, Taskbar & Độ nhạy Windows (Kiểm tra trạng thái 0/1 trước, chỉ khởi động lại Explorer khi có thay đổi thực sự)
bool SystemOptimizer::optimizeVisualEffectsAndUI() {
    struct TaskbarSetting {
        string keyPath;
        string valueName;
        DWORD targetValue;
        string desc;
        bool affectsExplorer; // Cần khởi động lại Explorer nếu thay đổi
    };

    vector<TaskbarSetting> settings = {
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Search", "SearchboxTaskbarMode", 0, "Searchbox (Thu gọn ô tìm kiếm)", true},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "TaskbarDa", 0, "Widgets (Tắt tin tức widget)", true},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "TaskbarMn", 0, "Chat Teams (Tắt biểu tượng chat)", true},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "ShowTaskViewButton", 0, "Task View (Tắt xem tác vụ)", true},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Feeds", "ShellFeedsTaskbarViewMode", 2, "Feeds News (Tắt tin tức Win 10)", true},
        {"Software\\Policies\\Microsoft\\Windows\\WindowsCopilot", "TurnOffWindowsCopilot", 1, "Copilot AI (Tắt nút Copilot)", true},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", "EnableTransparency", 0, "Transparency (Tắt trong suốt tiết kiệm GPU)", false},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "SnapAssist", 0, "Snap Assist (Tắt gợi ý chia đôi cửa sổ)", true},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", "SilentInstalledAppsEnabled", 0, "Silent Apps (Chặn Store cài app rác ngầm)", false},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", "SubscribedContent-310093Enabled", 0, "Start Ads (Tắt quảng cáo Start Menu)", false},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", "SubscribedContent-338388Enabled", 0, "Settings Tips (Tắt mẹo gợi ý Settings)", false},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", "SubscribedContent-338389Enabled", 0, "Explorer Ads (Tắt quảng cáo trong Explorer)", false},
        {"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "ShowSyncProviderNotifications", 0, "Sync Notifications (Tắt quảng cáo OneDrive)", false}
    };

    bool explorerNeedsRestart = false;
    int newlyChanged = 0;

    for (const auto &item : settings) {
        HKEY hKey;
        DWORD currentVal = 0;
        DWORD dataSize = sizeof(DWORD);
        DWORD valType = 0;
        bool alreadyOptimized = false;

        // BƯỚC 1: Đọc kiểm tra trạng thái hiện tại (0 hay 1) bằng quyền đọc an toàn (KEY_QUERY_VALUE)
        if (RegOpenKeyExA(HKEY_CURRENT_USER, item.keyPath.c_str(), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExA(hKey, item.valueName.c_str(), NULL, &valType, (LPBYTE)&currentVal, &dataSize) == ERROR_SUCCESS) {
                if (currentVal == item.targetValue) {
                    alreadyOptimized = true;
                }
            }
            RegCloseKey(hKey);
        }

        // BƯỚC 2: Nếu đã ở trạng thái tối ưu (0 hoặc targetValue) -> Bỏ qua, không ghi đè, không reset explorer!
        if (alreadyOptimized) {
            cout << "     ├── [✓ Đã tắt] " << item.desc << "\n";
            continue;
        }

        // BƯỚC 3: Nếu đang bật (1) hoặc chưa cấu hình -> Tiến hành tắt và đánh dấu cần làm mới
        bool changeSuccess = false;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, item.keyPath.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            if (RegSetValueExA(hKey, item.valueName.c_str(), 0, REG_DWORD, (const BYTE*)&item.targetValue, sizeof(DWORD)) == ERROR_SUCCESS) {
                changeSuccess = true;
            }
            RegCloseKey(hKey);
        } else {
            // Thử tạo key nếu key chưa tồn tại
            if (RegCreateKeyExA(HKEY_CURRENT_USER, item.keyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                if (RegSetValueExA(hKey, item.valueName.c_str(), 0, REG_DWORD, (const BYTE*)&item.targetValue, sizeof(DWORD)) == ERROR_SUCCESS) {
                    changeSuccess = true;
                }
                RegCloseKey(hKey);
            }
        }

        if (changeSuccess) {
            cout << "     ├── [Đã tắt mới] " << item.desc << "\n";
            newlyChanged++;
            if (item.affectsExplorer) {
                explorerNeedsRestart = true;
            }
        }
    }

    // Kiểm tra độ trễ mở menu chuột phải (Desktop MenuShowDelay = 0)
    HKEY hDesktop;
    bool delayIsZero = false;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_QUERY_VALUE, &hDesktop) == ERROR_SUCCESS) {
        char valBuf[32] = {0};
        DWORD valSz = sizeof(valBuf);
        if (RegQueryValueExA(hDesktop, "MenuShowDelay", NULL, NULL, (LPBYTE)valBuf, &valSz) == ERROR_SUCCESS) {
            if (string(valBuf) == "0") delayIsZero = true;
        }
        RegCloseKey(hDesktop);
    }

    if (delayIsZero) {
        cout << "     ├── [✓ Đã tối ưu] Độ trễ Menu chuột phải (0ms)\n";
    } else {
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_SET_VALUE, &hDesktop) == ERROR_SUCCESS) {
            const char zeroStr[] = "0";
            if (RegSetValueExA(hDesktop, "MenuShowDelay", 0, REG_SZ, (const BYTE*)zeroStr, 2) == ERROR_SUCCESS) {
                cout << "     ├── [Đã tối ưu mới] Giảm độ trễ Menu chuột phải về 0ms\n";
                newlyChanged++;
            }
            RegCloseKey(hDesktop);
        }
    }

    // Kiểm tra Telemetry chẩn đoán ngầm trong HKLM
    HKEY hLM;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", 0, KEY_QUERY_VALUE, &hLM) == ERROR_SUCCESS) {
        DWORD curTelem = 1, szTelem = sizeof(DWORD);
        bool telemZero = false;
        if (RegQueryValueExA(hLM, "AllowTelemetry", NULL, NULL, (LPBYTE)&curTelem, &szTelem) == ERROR_SUCCESS && curTelem == 0) {
            telemZero = true;
        }
        RegCloseKey(hLM);

        if (telemZero) {
            cout << "     ├── [✓ Đã tắt] Telemetry chẩn đoán ngầm Windows\n";
        } else {
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", 0, KEY_SET_VALUE, &hLM) == ERROR_SUCCESS) {
                DWORD zeroVal = 0;
                if (RegSetValueExA(hLM, "AllowTelemetry", 0, REG_DWORD, (const BYTE*)&zeroVal, sizeof(DWORD)) == ERROR_SUCCESS) {
                    cout << "     ├── [Đã tắt mới] Telemetry chẩn đoán ngầm Windows\n";
                    newlyChanged++;
                }
                RegCloseKey(hLM);
            } else {
                cout << "     ├── [⚠ Bỏ qua] Telemetry: Cần quyền Administrator để tắt\n";
            }
        }
    } else {
        cout << "     ├── [⚠ Bỏ qua] Telemetry: Key chưa tồn tại (chưa cần cấu hình)\n";
    }

    // CHỈ KHỞI ĐỘNG LẠI EXPLORER NẾU CÓ THAY ĐỔI TASKBAR MỚI THỰC SỰ!
    // Nếu tất cả đã tắt rồi -> Tuyệt đối không reset explorer.exe tránh giật lag/khó chịu
    if (explorerNeedsRestart) {
        sc.runCMD("taskkill /f /im explorer.exe >nul 2>&1 & start explorer.exe");
        return true;
    }
    return false;
}

// Thực thi một nhiệm vụ Tăng tốc & Tối ưu cụ thể
void SystemOptimizer::runOptimizeChoice(int choice) {
    if (choice == 5) {
        turnOffServicesMenu();
        return;
    }
    if (choice < 1 || choice > 4) return;

    sc.cls();
    static const char* scopes[] = {
        "", "Tắt ứng dụng khởi động không thuộc danh sách bảo vệ",
        "Tắt Maps, Wallet, Telemetry, Error Reporting và một số dịch vụ nền [Admin]",
        "Tinh chỉnh Taskbar, hiệu ứng và có thể khởi động lại Explorer",
        "Thực hiện cả ba nhóm tối ưu trên [Admin]"
    };
    cout << "== XEM TRƯỚC TỐI ƯU ==\n"
         << " Phạm vi: " << scopes[choice] << "\n\n";
    if (!sc.confirm(" Tiếp tục thực hiện? (y/N): ")) {
        cout << "\nĐã hủy, chưa có thay đổi nào được thực hiện.\n";
        Sleep(600);
        return;
    }
    cout << "\n";
    if (choice == 1 || choice == 4) {
        cout << "[*] Đang tối ưu ứng dụng khởi động...\n";
        int count = optimizeStartupApps();
        cout << "     └── [✓] " << (count > 0 ? ("Đã tắt " + to_string(count) + " app làm chậm máy") : "Tất cả ứng dụng khởi động đã tối ưu") << "\n\n";
    }

    if (choice == 2 || choice == 4) {
        cout << "[*] Đang tối ưu dịch vụ nền...\n";
        int count = optimizeBackgroundServices();
        cout << "     └── [✓] Đã tối ưu " << count << " dịch vụ ngầm (Maps, Wallet, Telemetry, ErrorReporting)\n\n";
    }

    if (choice == 3 || choice == 4) {
        cout << "[*] Đang tối ưu giao diện...\n";
        bool restarted = optimizeVisualEffectsAndUI();
        if (restarted) {
            cout << "     └── [✓] Đã áp dụng tinh chỉnh mới và làm mới Explorer.\n\n";
        } else {
            cout << "     └── [✓] Taskbar & Giao diện đã tinh gọn từ trước (Bỏ qua reset Explorer, tránh chớp màn hình).\n\n";
        }
    }

    cout << "\n[✓] Hoàn tất tối ưu.\n";
    sc.waitEnter();
}

// Menu điều phối Tăng tốc & Tối ưu Đa Tầng
void SystemOptimizer::multiTierPerformanceOptimize() {
    while (true) {
        sc.cls();
         
        cout << " [1] Tối ưu Khởi động (Tắt app làm chậm máy, bảo vệ Bộ gõ & Driver)\n"
             << " [2] Tối ưu Dịch vụ ngầm\n"
             << " [3] Tối ưu Giao diện & Độ nhạy Windows (Taskbar, bỏ độ trễ UI)\n"
             << " [4] Tối ưu liên hoàn cả 3\n"
             << " [5] Quản lý dịch vụ Windows nâng cao\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";

        int choice = sc.readInt("");
        if (choice == 0) break;
        runOptimizeChoice(choice);
    }
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
    DWORD regStart = (startupType == SERVICE_DISABLED) ? 4 : ((startupType == SERVICE_AUTO_START) ? 2 : 3);
    std::string subKey = "SYSTEM\\CurrentControlSet\\Services\\" + serviceName;

    auto stopServiceCmd = [&]() {
        std::string stopCmd = "net stop \"" + serviceName + "\" >nul 2>&1";
        system(stopCmd.c_str());
    };

    // 1. Kiểm tra cấu hình Registry xem đã ở đúng giá trị mong muốn chưa
    HKEY hKeyCheck;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKeyCheck) == ERROR_SUCCESS) {
        DWORD curVal = 0, sz = sizeof(curVal);
        if (RegQueryValueExA(hKeyCheck, "Start", NULL, NULL, (LPBYTE)&curVal, &sz) == ERROR_SUCCESS) {
            if (curVal == regStart) {
                RegCloseKey(hKeyCheck);
                if (stopService) {
                    stopServiceCmd();
                }
                return true;
            }
        }
        RegCloseKey(hKeyCheck);
    }

    // 2. Thử qua Win32 SCM API (nếu tiến trình có đủ quyền)
    bool scmSuccess = false;
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (!scm) scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (scm) {
        SC_HANDLE svc = OpenServiceA(scm, serviceName.c_str(), SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | SERVICE_STOP | SERVICE_START);
        if (svc) {
            if (ChangeServiceConfigA(svc, SERVICE_NO_CHANGE, startupType, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL)) {
                scmSuccess = true;
            }
            if (stopService) {
                SERVICE_STATUS status;
                ControlService(svc, SERVICE_CONTROL_STOP, &status);
            }
            CloseServiceHandle(svc);
        }
        CloseServiceHandle(scm);
    }
    if (scmSuccess) {
        if (stopService) stopServiceCmd();
        return true;
    }

    // 3. Thử ghi trực tiếp vào Registry
    HKEY hKeyWrite;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_SET_VALUE, &hKeyWrite) == ERROR_SUCCESS) {
        LONG setValRes = RegSetValueExA(hKeyWrite, "Start", 0, REG_DWORD, (const BYTE*)&regStart, sizeof(regStart));
        RegCloseKey(hKeyWrite);
        if (setValRes == ERROR_SUCCESS) {
            if (stopService) stopServiceCmd();
            return true;
        }
    }

    // 4. Nếu thiếu quyền Administrator, dùng cơ chế tự nâng quyền runAdmin của CMD Box
    std::string scStart = (startupType == SERVICE_DISABLED) ? "disabled" : ((startupType == SERVICE_AUTO_START) ? "auto" : "demand");
    std::string adminCmd = "sc config \"" + serviceName + "\" start= " + scStart + " & reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\" + serviceName + "\" /v Start /t REG_DWORD /d " + std::to_string(regStart) + " /f";
    if (stopService) {
        adminCmd += " & net stop \"" + serviceName + "\" >nul 2>&1";
    }
    if (sc.runAdmin(adminCmd, true)) {
        HKEY hKeyVerify;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKeyVerify) == ERROR_SUCCESS) {
            DWORD finalVal = 0, fsz = sizeof(finalVal);
            LONG qRes = RegQueryValueExA(hKeyVerify, "Start", NULL, NULL, (LPBYTE)&finalVal, &fsz);
            RegCloseKey(hKeyVerify);
            if (qRes == ERROR_SUCCESS) {
                return (finalVal == regStart);
            }
        }
        return false;
    }
    return false;
}

/**
 * =========================================================================================
 * 8. MENU QUẢN LÝ DỊCH VỤ WINDOWS (turnOffServicesMenu)
 * =========================================================================================
 * TÍNH NĂNG:
 * - Cung cấp danh sách 24 dịch vụ chạy ngầm phổ biến có thể tắt an toàn hoặc chuyển Manual:
 *   + Windows Update (wuauserv, UsoSvc, WaaSMedicSvc)
 *   + Báo cáo lỗi (WerSvc) & Thông báo ngầm (WpnService, WpnUserService)
 *   + Xbox Live services (XblAuthManager, XblGameSave, XboxNetApiSvc)
 *   + Telemetry thu thập dữ liệu (DiagTrack, dmwappushservice)
 *   + Trình cập nhật ngầm Edge (EdgeUpdate), Thử nghiệm Insider (wisvc)
 *   + Dịch vụ in ấn Spooler, Bluetooth (BthServ), Maps ngoại tuyến, Remote Registry, SysMain (Superfetch), Wallet...
 * - Cho phép chọn [A] Cấu hình tất cả thành Manual / Disabled hoặc cấu hình từng dịch vụ riêng lẻ.
 * - Tự động nâng quyền Administrator khi cần, hỗ trợ triệt để các User Service có hậu tố ngẫu nhiên.
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
                      << " [1] Manual   (Chỉ khi cần)\n"
                      << " [2] Disabled (Tắt hoàn toàn)\n"
                      << " [0] Hủy\n\n";
            int action = sc.readInt("Chọn: ");
            if (action == 0) continue;
            if (action != 1 && action != 2) {
                std::cout << "Lựa chọn không hợp lệ! Vui lòng chọn 1 hoặc 2.\n";
                Sleep(1000);
                continue;
            }

            startType = (action == 1) ? SERVICE_DEMAND_START : SERVICE_DISABLED;
            modeName = (action == 1) ? "MANUAL" : "DISABLED";
            DWORD regVal = (action == 1) ? 3 : 4;
            std::string scVal = (action == 1) ? "demand" : "disabled";

            std::cout << "\nĐang thực thi cấu hình dịch vụ (" << modeName << ")...\n";

            // Nếu chưa chạy quyền Admin, gom tất cả vào 1 batch script chạy quyền Admin duy nhất
            if (!sc.isElevated()) {
                std::string batContent = "@echo off\n";
                for (const auto &s : targetSvcs) {
                    batContent += "sc config \"" + s.name + "\" start= " + scVal + " >nul 2>&1\n";
                    batContent += "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\" + s.name + "\" /v Start /t REG_DWORD /d " + std::to_string(regVal) + " /f >nul 2>&1\n";
                    batContent += "net stop \"" + s.name + "\" >nul 2>&1\n";
                    batContent += "powershell -NoProfile -Command \"Get-Service -Name '" + s.name + "*' -ErrorAction SilentlyContinue | Stop-Service -Force -ErrorAction SilentlyContinue\" >nul 2>&1\n";
                }
                SystemCore::runBatchAsAdmin(batContent, "Cấu hình dịch vụ hệ thống");
            } else {
                for (const auto &s : targetSvcs) {
                    ServiceControlAPI(s.name, startType, true);
                }
            }

            // Vòng lặp báo cáo kết quả: CHỈ ĐỌC (READ-ONLY), không kích hoạt UAC
            int successCount = 0;
            for (const auto &s : targetSvcs) {
                DWORD curVal = 0;
                std::string subKey = "SYSTEM\\CurrentControlSet\\Services\\" + s.name;
                HKEY hCheck;
                bool ok = false;
                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hCheck) == ERROR_SUCCESS) {
                    DWORD sz = sizeof(curVal);
                    if (RegQueryValueExA(hCheck, "Start", NULL, NULL, (LPBYTE)&curVal, &sz) == ERROR_SUCCESS) {
                        ok = (curVal == regVal);
                    }
                    RegCloseKey(hCheck);
                }
                if (ok) {
                    std::cout << "  [✓] " << modeName << ": " << s.name << "\n";
                    successCount++;
                } else {
                    std::cout << "  [!] Thất bại: " << s.name << "\n";
                }
            }
            std::cout << "\n[✓] Đã cấu hình " << successCount << "/" << targetSvcs.size() << " dịch vụ.\n";
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
                              << " [1] Manual   (Chỉ khi cần)\n"
                              << " [2] Disabled (Tắt hẳn)\n"
                              << " [0] Hủy\n\n";
                    int action = sc.readInt("Chọn: ");
                    if (action == 0) continue;
                    if (action != 1 && action != 2) {
                        std::cout << "Lựa chọn không hợp lệ! Vui lòng chọn 1 hoặc 2.\n";
                        Sleep(1000);
                        continue;
                    }

                    startType = (action == 1) ? SERVICE_DEMAND_START : SERVICE_DISABLED;
                    modeName = (action == 1) ? "MANUAL" : "DISABLED";

                    std::cout << "\nĐang xử lý " << targetSvcs[idx].name << "\n";
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


