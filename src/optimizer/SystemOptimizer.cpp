#include "FileSafety.h"
#include "EmbeddedScripts.h"
#include "SystemOptimizer.h"
#include "CleanerCore.h"
#include "TempCleaner.h"
#include "BrowserCleaner.h"
#include "SystemDeepCleaner.h"
#include "DevCleaner.h"
#include "DownloadsCleaner.h"
#include <iostream>
#include <algorithm>
#include <windows.h>
#include <filesystem>
#include <vector>
#include <string>
#include <iomanip> 
using namespace std;
namespace fs = std::filesystem;

SystemOptimizer::SystemOptimizer(SystemCore &s) : sc(s) {}

// Quản lý ứng dụng khởi động: bảo vệ bộ gõ, driver, antivirus;
// sao lưu sang Run_Disabled hoặc đổi đuôi .disabled trước khi tắt.

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
    transform(combined.begin(), combined.end(), combined.begin(), [](unsigned char c) { return char(std::tolower(c)); });

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

    return {true, "Chưa nhận diện (giữ nguyên)"};
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
                    auto terminator = std::find(valDataBuf.begin(), valDataBuf.begin() + valDataSize, char(0));
                    if (terminator == valDataBuf.begin() + valDataSize) { index++; continue; }
                    string cmdStr(valDataBuf.begin(), terminator);
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
                        transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });
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
        if (RegCreateKeyExA(item.hKeyRoot, backupKeyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, NULL, &hBackup, NULL) != ERROR_SUCCESS) {
            return false;
        }
        DWORD backupSize = 0;
        LONG exists = RegQueryValueExA(hBackup, item.name.c_str(), nullptr, nullptr, nullptr, &backupSize);
        if (exists != ERROR_FILE_NOT_FOUND) { RegCloseKey(hBackup); return false; }
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
    if (!SystemCore::confirm("Reset Windows Update: dừng dịch vụ tạm thời, giữ cache cũ làm bản dự phòng?")) return;
    const string script = R"PS($ErrorActionPreference='Stop'
$running = @(); $moved = @(); $failed = $false
try {
    $services = @('wuauserv','cryptSvc','bits','msiserver') | ForEach-Object { Get-Service -Name $_ }
    foreach ($service in $services) {
        if ($service.Status -eq 'Running') { $running += $service.Name }
        elseif ($service.Status -ne 'Stopped') { throw 'Service is transitioning; try again later' }
    }
    foreach ($name in $running) { Stop-Service -Name $name; (Get-Service -Name $name).WaitForStatus('Stopped',[TimeSpan]::FromSeconds(30)) }
    foreach ($service in $services) { if ((Get-Service -Name $service.Name).Status -ne 'Stopped') { throw 'Service did not stop' } }
    foreach ($path in @((Join-Path $env:SystemRoot 'SoftwareDistribution'),(Join-Path $env:SystemRoot 'System32\catroot2'))) {
        if (Test-Path -LiteralPath $path) {
            $item = Get-Item -LiteralPath $path -Force
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Cache is a reparse point' }
            $backup = $path + '.cmd-box-backup-' + [guid]::NewGuid().ToString('N')
            Move-Item -LiteralPath $path -Destination $backup
            $moved += [pscustomobject]@{Original=$path; Backup=$backup}
        }
    }
} catch {
    $failed = $true; Write-Error $_ -ErrorAction Continue
    foreach ($move in $moved) { try { if (-not (Test-Path -LiteralPath $move.Original)) { Move-Item -LiteralPath $move.Backup -Destination $move.Original } } catch { Write-Error $_ -ErrorAction Continue } }
} finally {
    foreach ($name in $running) {
        try { Start-Service -Name $name; (Get-Service -Name $name).WaitForStatus('Running',[TimeSpan]::FromSeconds(30)) }
        catch { $failed = $true; Write-Error $_ -ErrorAction Continue }
    }
}
if ($failed) { exit 1 }; exit 0
)PS";
    FileSafety::LockedScript file(script, L".ps1");
    bool ok = file && sc.runAdmin("powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" + file.path().u8string() + "\"", true);
    cout << (ok ? "Đã reset; cache cũ vẫn được giữ dưới tên .cmd-box-backup-*.\n" : "Reset thất bại/chưa hoàn tất; kiểm tra dịch vụ và cache dự phòng.\n");
    sc.waitEnter();
}

// --- ỦY QUYỀN DỌN RÁC SANG DISKCLEANER (ENGINE BIN v2 - STATIC API) ---
void SystemOptimizer::runClean() {
    DiskCleaner::runAutomaticCleanup(true);
}

void SystemOptimizer::runCleanChoice(int /*choice*/) {
    runClean();
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

// Nhiệm vụ 3: Tối ưu Giao diện, Taskbar & Độ nhạy Windows (Ghi file tạm thực thi script nhúng)
bool SystemOptimizer::optimizeVisualEffectsAndUI() {
    bool userSettingsOk = SystemCore::runEmbeddedBatch(EmbeddedScripts::OPTIMIZE_REGISTRY_BAT, "user", false);
    bool machineSettingsOk = SystemCore::runEmbeddedBatch(EmbeddedScripts::OPTIMIZE_REGISTRY_BAT, "machine", true);
    return userSettingsOk && machineSettingsOk;
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
        "Tắt Maps, Wallet, Telemetry, Error Reporting và một số dịch vụ nền",
        "Tinh chỉnh Taskbar, hiệu ứng và có thể khởi động lại Explorer",
        "Thực hiện cả ba nhóm tối ưu trên"
    };
    cout << "\n \x1b[38;2;195;165;255m╭── XEM TRƯỚC TỐI ƯU ──────────────────────────────╮\x1b[0m\n"
         << "   Phạm vi: " << scopes[choice] << "\n"
         << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n";
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
        bool optimized = optimizeVisualEffectsAndUI();
        if (optimized) {
            cout << "     └── [✓] Đã kiểm tra và áp dụng các tinh chỉnh Registry.\n\n";
        } else {
            cout << "     └── [!] Không áp dụng được đầy đủ; kiểm tra quyền Admin hoặc thư mục scripts.\n\n";
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



// Điều khiển dịch vụ Windows qua Win32 Service Control Manager (SCM API)
bool SystemOptimizer::ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService) {
    if (serviceName.empty() || serviceName.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != string::npos ||
        (startupType != SERVICE_DISABLED && startupType != SERVICE_DEMAND_START && startupType != SERVICE_AUTO_START)) return false;
    if (!SystemCore::isElevated()) {
        string mode = startupType == SERVICE_DISABLED ? "Disabled" : startupType == SERVICE_AUTO_START ? "Automatic" : "Manual";
        string script = "$ErrorActionPreference='Stop'\ntry {\nSet-Service -Name '" + serviceName + "' -StartupType " + mode + ";\n";
        if (stopService) script += "$s = Get-Service -Name '" + serviceName + "'; if ($s.Status -ne 'Stopped') { Stop-Service -Name $s.Name -ErrorAction Stop; $s.WaitForStatus('Stopped',[TimeSpan]::FromSeconds(30)) };\n";
        script += "} catch { Write-Error $_ -ErrorAction Continue; exit 1 }\n";
        FileSafety::LockedScript file(script, L".ps1");
        if (!file || !sc.runAdmin("powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" + file.path().u8string() + "\"", true)) return false;
    }
    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!manager) return false;
    DWORD access = SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS;
    if (SystemCore::isElevated()) access |= SERVICE_CHANGE_CONFIG | (stopService ? SERVICE_STOP : 0);
    SC_HANDLE service = OpenServiceA(manager, serviceName.c_str(), access);
    if (!service) { CloseServiceHandle(manager); return false; }
    bool ok = true;
    if (SystemCore::isElevated()) {
        ok = ChangeServiceConfigA(service, SERVICE_NO_CHANGE, startupType, SERVICE_NO_CHANGE,
                                  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) != FALSE;
        if (ok && stopService) {
            SERVICE_STATUS status{};
            ok = QueryServiceStatus(service, &status) != FALSE;
            if (ok && status.dwCurrentState != SERVICE_STOPPED && status.dwCurrentState != SERVICE_STOP_PENDING)
                ok = ControlService(service, SERVICE_CONTROL_STOP, &status) != FALSE;
            ULONGLONG deadline = GetTickCount64() + 30000;
            while (ok && status.dwCurrentState != SERVICE_STOPPED && GetTickCount64() < deadline) {
                Sleep(100); ok = QueryServiceStatus(service, &status) != FALSE;
            }
            ok = ok && status.dwCurrentState == SERVICE_STOPPED;
        }
    }
    DWORD required = 0;
    QueryServiceConfigA(service, nullptr, 0, &required);
    vector<unsigned char> buffer(required);
    if (required < sizeof(QUERY_SERVICE_CONFIGA)) ok = false;
    else {
        auto config = reinterpret_cast<QUERY_SERVICE_CONFIGA*>(buffer.data());
        ok = ok && QueryServiceConfigA(service, config, required, &required) && config->dwStartType == startupType;
    }
    if (ok && stopService) {
        SERVICE_STATUS status{};
        ok = QueryServiceStatus(service, &status) && status.dwCurrentState == SERVICE_STOPPED;
    }
    CloseServiceHandle(service); CloseServiceHandle(manager); return ok;
}

// Quản lý 40 dịch vụ theo từng mục; xem ảnh hưởng trước khi chọn Manual/Disabled.
void SystemOptimizer::turnOffServicesMenu() {
    sc.cls();
    struct SvcInfo { std::string name; std::string desc; };
    
    // Danh sách các dịch vụ Windows có thể tối ưu
    std::vector<SvcInfo> targetSvcs = {
        {"MapsBroker", "Bản đồ ngoại tuyến — Không tự quản lý/tải bản đồ offline."},
        {"WalletService", "Ví điện tử Windows — Ứng dụng dùng Windows Wallet có thể không hoạt động."},
        {"RetailDemo", "Chế độ trình diễn cửa hàng — Không dùng được chế độ Retail Demo."},
        {"DiagTrack", "Connected User Experiences / Telemetry — Giảm chức năng chẩn đoán và thu thập telemetry của dịch vụ này."},
        {"dmwappushservice", "WAP Push / quản lý thiết bị — Có thể ảnh hưởng máy được doanh nghiệp quản lý qua MDM."},
        {"WerSvc", "Báo cáo lỗi Windows — Không gửi báo cáo lỗi qua dịch vụ này; giảm hỗ trợ chẩn đoán."},
        {"RemoteRegistry", "Registry từ xa — Công cụ quản trị từ xa không sửa được Registry qua dịch vụ này."},
        {"wisvc", "Windows Insider — Ảnh hưởng chương trình nhận bản Windows thử nghiệm."},
        {"Fax", "Gửi và nhận fax — Không gửi/nhận fax trên máy này."},
        {"AJRouter", "AllJoyn Router — Ứng dụng/thiết bị AllJoyn không có router riêng có thể ngừng hoạt động."},
        {"XblAuthManager", "Xác thực Xbox Live — Có thể không đăng nhập Xbox Live được trong một số game."},
        {"XblGameSave", "Đồng bộ save Xbox Live — Save game không được đồng bộ Xbox Live."},
        {"XboxNetApiSvc", "Mạng Xbox Live — Ảnh hưởng multiplayer và chức năng mạng Xbox Live."},
        {"XboxGipSvc", "Phụ kiện Xbox — Có thể ảnh hưởng tay cầm/phụ kiện Xbox."},
        {"WMPNetworkSvc", "Chia sẻ media Windows Media Player — Không chia sẻ thư viện media qua mạng."},
        {"PhoneSvc", "Phone Service — Ảnh hưởng ứng dụng dùng chức năng điện thoại của Windows."},
        {"lfsvc", "Định vị địa lý — Ứng dụng không lấy được vị trí qua dịch vụ này."},
        {"CDPSvc", "Connected Devices Platform — Ảnh hưởng liên kết/chia sẻ giữa các thiết bị."},
        {"Spooler", "In ấn / Print Spooler — Mất in ấn, gồm cả một số máy in PDF ảo."},
        {"bthserv", "Bluetooth Support — Có thể mất kết nối/ghép đôi Bluetooth, gồm chuột và bàn phím."},
        {"icssvc", "Mobile Hotspot — Không dùng được điểm phát Wi-Fi di động của Windows."},
        {"SharedAccess", "Internet Connection Sharing — Ảnh hưởng chia sẻ Internet/hotspot và phần mềm phụ thuộc ICS."},
        {"WSearch", "Windows Search / lập chỉ mục — Tìm file và tìm kiếm nội dung/Outlook có thể chậm hoặc thiếu kết quả."},
        {"SensorDataService", "Dữ liệu cảm biến — Ứng dụng không nhận được dữ liệu cảm biến."},
        {"SensrSvc", "Theo dõi cảm biến — Ảnh hưởng thích ứng ánh sáng và các tính năng dùng cảm biến."},
        {"SensorService", "Quản lý cảm biến — Có thể mất tự xoay màn hình và cảm biến định hướng."},
        {"SSDPSRV", "Phát hiện thiết bị SSDP / UPnP — Không tìm được một số TV/thiết bị media/UPnP trong LAN."},
        {"upnphost", "UPnP Device Host — Không host được thiết bị UPnP trên máy."},
        {"workfolderssvc", "Work Folders — Không đồng bộ Work Folders của tổ chức."},
        {"WpcSvc", "Parental Controls — Ảnh hưởng kiểm soát trẻ em / quản lý gia đình."},
        {"CDPUserSvc", "Connected Devices theo người dùng (mẫu) — Ảnh hưởng liên kết thiết bị; instance có hậu tố là mục riêng, không đổi tự động."},
        {"OneSyncSvc", "Sync Host theo người dùng (mẫu) — Ảnh hưởng mail/liên hệ/lịch; instance có hậu tố không đổi tự động."},
        {"wuauserv", "Windows Update — Ngừng cập nhật qua Windows Update; có thể thiếu bản vá."},
        {"UsoSvc", "Update Orchestrator — Ảnh hưởng điều phối cập nhật Windows."},
        {"WaaSMedicSvc", "Windows Update Medic — Ảnh hưởng sửa chữa Windows Update; Windows có thể từ chối quyền thay đổi."},
        {"WpnService", "Thông báo Windows — Ảnh hưởng thông báo Windows và ứng dụng; không phải chỉ Widgets."},
        {"WpnUserService", "Thông báo theo người dùng (mẫu) — Ảnh hưởng toast/push; instance có hậu tố không đổi tự động."},
        {"edgeupdate", "Cập nhật Microsoft Edge — Ngừng cập nhật Edge qua dịch vụ này, gồm các bản vá bảo mật."},
        {"edgeupdatem", "Cập nhật Edge theo yêu cầu — Ảnh hưởng cập nhật Edge qua dịch vụ theo yêu cầu."},
        {"SysMain", "Tối ưu truy cập ứng dụng — Có thể giảm hiệu năng; SSD không phải lý do tự động tắt."},
    };

    while (true) {
        sc.cls();
        std::cout << "\n \x1b[38;2;195;165;255m╭── QUẢN LÝ DỊCH VỤ WINDOWS ────────────────────────╮\x1b[0m\n"
                  << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n";
        
        for (size_t i = 0; i < targetSvcs.size(); ++i) {
            std::cout << " [" << std::setw(2) << i + 1 << "] " 
                      << std::left << std::setw(30) << (targetSvcs[i].desc.substr(0, 45) + "...") 
                      << " [" << targetSvcs[i].name << "]\n";
        }
        std::cout << "\n [A] Hướng dẫn về cấu hình hàng loạt\n"
                  << " [0] Quay lại\n\n"
                  << "Chọn dịch vụ, hoặc [A]/[0]: ";
        std::string input;
        std::cin >> input;

        if (input == "0") return;

        DWORD startType = SERVICE_DISABLED;
        std::string modeName = "";

        // --- CẤU HÌNH HÀNG LOẠT TẤT CẢ DỊCH VỤ ---
        if (input == "A" || input == "a") {
            cout << "Cấu hình hàng loạt có thể tắt cập nhật/thông báo. Hãy chọn từng dịch vụ để kiểm tra trước.\n";
            cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
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
                size_t used = 0; int selected = std::stoi(input, &used);
                if (used != input.size() || selected < 1) throw std::invalid_argument("selection");
                int idx = selected - 1;
                if (idx >= 0 && idx < (int)targetSvcs.size()) {
                    sc.cls();
                    std::cout << "\n \x1b[38;2;195;165;255m╭── DỊCH VỤ: " << targetSvcs[idx].name << " ───────────────────────╮\x1b[0m\n"
                              << "   " << targetSvcs[idx].desc << "\n"
                              << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n"
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
                    if (ServiceControlAPI(targetSvcs[idx].name, startType, startType == SERVICE_DISABLED)) {
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


