#include "../include/SystemOptimizer.h"
#include <iostream>
#include <windows.h>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

SystemOptimizer::SystemOptimizer(SystemCore &s, Internet &net) : sc(s), n(net) {}

void SystemOptimizer::QuickScanVirus() { sc.runCMD("\"%ProgramFiles%\\Windows Defender\\MpCmdRun.exe\" -Scan -ScanType 1"); }
void SystemOptimizer::FullScanVirus() { sc.runCMD("\"%ProgramFiles%\\Windows Defender\\MpCmdRun.exe\" -Scan -ScanType 2"); }
void SystemOptimizer::Consumer_Content() { sc.runCMD("reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager\" /v \"SilentInstalledAppsEnabled\" /t REG_DWORD /d 0 /f"); }
void SystemOptimizer::Hibernate() { sc.runAdmin("powercfg -h off", true); }
void SystemOptimizer::windowsTelemetry() { sc.runAdmin("reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection\" /v AllowTelemetry /t REG_DWORD /d 0 /f", true); }
void SystemOptimizer::reduceShutdownTime() { sc.runCMD("reg add \"HKCU\\Control Panel\\Desktop\" /v \"WaitToKillAppTimeout\" /t REG_SZ /d \"2000\" /f"); }

void SystemOptimizer::cleanDiskPro() {
    sc.runCMD("cmd /c del /s /f /q \"%temp%\\*\" & rd /s /q \"%temp%\" & md \"%temp%\"");
    sc.runCMD("cmd /c del /s /f /q \"%systemroot%\\temp\\*\" & rd /s /q \"%systemroot%\\temp\" & md \"%systemroot%\\temp\"");
    sc.runCMD("cmd /c del /s /f /q \"%systemroot%\\Prefetch\\*\"");
    sc.runCMD("cleanmgr /sagerun:1");
    sc.runAdmin("dism /online /cleanup-image /startcomponentcleanup", true);
    sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Microsoft\\Windows\\Recent\\*\"");
    sc.runCMD("powershell -NoProfile -Command \"Clear-RecycleBin -Force -ErrorAction SilentlyContinue\"");
    sc.runAdmin("powershell -Command \"Get-DeliveryOptimizationStatus | Remove-DeliveryOptimizationCache -Confirm:$false\"", true);
    sc.runCMD("del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\Temp\\*\"");
    sc.runCMD("del /f /s /q \"%AppData%\\Local\\Microsoft\\Windows\\WER\\*\"");
    sc.runCMD("del /f /s /q \"%LocalAppData%\\Low\\Microsoft\\CryptnetUrlCache\\*\"");
    sc.runCMD("del /f /s /q \"%LocalAppData%\\D3DSCache\\*\"");
    sc.runAdmin("netsh branchcache flush", true);
    sc.runAdmin("powershell -Command \"Stop-Service -Name FontCache -Force; del /f /s /q $env:windir\\ServiceProfiles\\LocalService\\AppData\\Local\\FontCache\\* ; Start-Service -Name FontCache\"", true);
    sc.runAdmin("dism /online /cleanup-image /analyzecomponentstore", true);
    sc.runAdmin("dism /online /cleanup-image /startcomponentcleanup /resetbase", true);
    sc.runCMD("del /f /s /q %windir%\\WindowsUpdate.log");
    sc.runAdmin("powershell -Command \"Get-EventLog -LogName * | ForEach { Clear-EventLog $_.Log }\"", true);
    sc.runCMD("taskkill /f /im explorer.exe & del /f /q %LocalAppData%\\IconCache.db & start explorer.exe");
}

void SystemOptimizer::cleanDiskBase() {
    cout << "[...] Dang don rac he thong...\n\n";    
    cout << "[1] Xoa temp nguoi dung...\n"; sc.runCMD("cmd /c del /s /f /q \"%temp%\\*\" 2>nul & rd /s /q \"%temp%\" 2>nul & md \"%temp%\" 2>nul");
    cout << "[2] Xoa temp he thong...\n"; sc.runCMD("cmd /c del /s /f /q \"%systemroot%\\temp\\*\" 2>nul & rd /s /q \"%systemroot%\\temp\" 2>nul & md \"%systemroot%\\temp\" 2>nul");
    cout << "[3] Xoa Prefetch...\n"; sc.runCMD("cmd /c del /s /f /q \"%systemroot%\\Prefetch\\*\" 2>nul");
    cout << "[4] Xoa Recent files...\n"; sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Microsoft\\Windows\\Recent\\*\" 2>nul");
    cout << "[5] Xoa thung rac...\n"; sc.runCMD("powershell -NoProfile -Command \"Clear-RecycleBin -Force -ErrorAction SilentlyContinue\"");
    cout << "[6] Xoa cache trinh duyet...\n"; sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Microsoft\\Windows\\INetCache\\*\" 2>nul");
    cout << "[7] Xoa log Windows cu...\n"; sc.runCMD("del /f /q \"%systemroot%\\*.log\" 2>nul");
    cout << "[8] Xoa file temp Windows Update...\n"; sc.runCMD("del /f /s /q \"%systemroot%\\SoftwareDistribution\\Download\\*\" 2>nul");
    cout << "\n[OK] Da don rac xong!\n";
}

void SystemOptimizer::restart() {
    string ans;
    cin.ignore();
    cout << "Xac nhan RESTART may? (Y/N): "; cin >> ans;
    if (ans == "y" || ans == "Y") sc.runCMD("shutdown /r /t 5");
    else cout << "Huy restart.\n";
}

void SystemOptimizer::disableAllStartupApps() {
    sc.cls();
    cout << "================ TỐI ƯU APP KHỞI ĐỘNG ================\n";
    cout << "[...] Đang quét và dọn dẹp...\n\n";
    int removedCount = 0;
    const struct { HKEY hKeyRoot; LPCSTR subKey; string name; } targets[] = {
        {HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", "HKCU"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "HKLM"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run", "HKLM_WOW64"}
    };
    vector<string> whitelist = { "SecurityHealth", "RtkAudUService", "WavesSvc", "IgfxTray", "NvBackend", "OneDrive" };

    for (const auto &target : targets) {
        HKEY hKey;
        if (RegOpenKeyExA(target.hKeyRoot, target.subKey, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            char valueName[256];
            DWORD nameSize, type;
            DWORD index = 0;
            vector<string> toDelete;
            while (true) {
                nameSize = sizeof(valueName);
                if (RegEnumValueA(hKey, index, valueName, &nameSize, NULL, &type, NULL, NULL) == ERROR_SUCCESS) {
                    string vName(valueName);
                    bool isSafe = false;
                    for (const string &safeApp : whitelist) {
                        if (vName.find(safeApp) != string::npos) { isSafe = true; break; }
                    }
                    if (!isSafe) toDelete.push_back(vName);
                    index++;
                } else break;
            }
            for (const string &delName : toDelete) {
                if (RegDeleteValueA(hKey, delName.c_str()) == ERROR_SUCCESS) {
                    cout << "[-] Đã tắt: " << delName << " (" << target.name << ")\n";
                    removedCount++;
                }
            }
            RegCloseKey(hKey);
        }
    }
    cout << "\n[SUCCESS] Đã tắt thành công " << removedCount << " tiến trình.\n";
}

void SystemOptimizer::updateAllApps() {
    cout << "[...] Đang cập nhật toàn bộ ứng dụng qua Winget...\n";
    sc.runCMD("winget upgrade --all --silent");
    cout << "[OK] Cập nhật hoàn tất.\n";
}

void SystemOptimizer::fixWindowsUpdate() {
    cout << "[1/3] Dang dung dich vu...\n";
    sc.runAdmin("net stop wuauserv", true); sc.runAdmin("net stop cryptSvc", true); sc.runAdmin("net stop bits", true); sc.runAdmin("net stop msiserver", true);
    cout << "[2/3] Dang xoa cache...\n";
    sc.runCMD("del /f /q %windir%\\SoftwareDistribution\\*.*"); sc.runAdmin("rd /s /q %windir%\\SoftwareDistribution", true); sc.runAdmin("rd /s /q %windir%\\system32\\catroot2", true);
    cout << "[3/3] Dang khoi dong lai...\n";
    sc.runAdmin("net start wuauserv", true); sc.runAdmin("net start cryptSvc", true); sc.runAdmin("net start bits", true); sc.runAdmin("net start msiserver", true);
    cout << "\n[OK] Da reset Windows Update!\n";
}

void SystemOptimizer::clearBrowserCache() {
    char *localAppData = std::getenv("LOCALAPPDATA");
    char *appData = std::getenv("APPDATA");
    if (!localAppData) return;
    string baseLocal = string(localAppData);
    vector<string> cachePaths = {
        baseLocal + "\\Google\\Chrome\\User Data\\Default\\Cache", baseLocal + "\\Google\\Chrome\\User Data\\Default\\Code Cache",
        baseLocal + "\\Microsoft\\Edge\\User Data\\Default\\Cache", baseLocal + "\\CocCoc\\Browser\\User Data\\Default\\Cache",
        baseLocal + "\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Cache", baseLocal + "\\Vivaldi\\User Data\\Default\\Cache",
        baseLocal + "\\Opera Software\\Opera Stable\\Cache", baseLocal + "\\Opera Software\\Opera GX Stable\\Cache"
    };

    cout << "[SYSTEM] DANG DON DEP CACHE MULTI-BROWSER...\n";
    for (const string &path : cachePaths) {
        if (fs::exists(path)) {
            cout << "[...] Dang don: " << path;
            try {
                for (const auto &entry : fs::directory_iterator(path)) fs::remove_all(entry.path());
                cout << " [OK]\n";
            } catch (...) { cout << " [!] Dang ban\n"; }
        }
    }
    if (appData) {
        string ffPath = string(appData) + "\\Mozilla\\Firefox\\Profiles";
        if (fs::exists(ffPath)) {
            for (const auto &profile : fs::directory_iterator(ffPath)) {
                string cacheDir = profile.path().string() + "\\cache2";
                if (fs::exists(cacheDir)) {
                    cout << "[...] Dang don Firefox: " << cacheDir;
                    try {
                        for (const auto &entry : fs::directory_iterator(cacheDir)) fs::remove_all(entry.path());
                        cout << " [OK]\n";
                    } catch (...) { cout << " [!] Dang ban\n"; }
                }
            }
        }
    }
    cout << "\n[SUCCESS] Hoan tat don dep Cache!\n";
}

void SystemOptimizer::optimizeSystemPRO() {
    cout << "[...] Bắt đầu tối ưu hệ thống...\n\n";
    cleanDiskPro();
    reduceShutdownTime();
    Consumer_Content();
    windowsTelemetry();
    Hibernate();
    clearBrowserCache();
    cout << "[OK] Hệ thống đã được tối ưu!\n";
}

void SystemOptimizer::enableSecurityPRO() {
    sc.runAdmin("netsh advfirewall set allprofiles state on", true);
    sc.runAdmin("powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $false\"", true);
    sc.runAdmin("powershell -Command \"Set-MpPreference -EnableControlledFolderAccess Enabled\"", true);
    cout << "[OK] Bao mat da duoc bat!\n";
}

void SystemOptimizer::optimizeNetworkPRO() {
    cout << "[...] Đang tối ưu mạng...\n";
    n.flushdns();
    n.netsh_tcpIP();
    windowsTelemetry();
    cout << "[OK] Mang da duoc thiet lap lai!\n";
}

string SystemOptimizer::getCurrentOS() {
    FILE *pipe = _popen("systeminfo | findstr /B /C:\"OS Name\"", "r");
    if (!pipe) return "Unknown";
    char buffer[256]; string result = "";
    if (fgets(buffer, sizeof(buffer), pipe)) result = buffer;
    _pclose(pipe);
    size_t pos = result.find("Windows");
    if (pos != string::npos) return sc.trim(result.substr(pos));
    return "Unknown";
}

void SystemOptimizer::upgradeWindowsEditionPRO() {
    string currentOS = getCurrentOS();
    sc.cls();
    cout << "PHIEN BAN HIEN TAI: " << currentOS << "\n\n";
    string key = ""; int choice;

    if (currentOS.find("Home") != string::npos) {
        choice = sc.readInt("1. Pro | 2. Edu | 3. Enterprise | 0. Back: ");
        if (choice == 1) key = "VK7JG-NPHTM-C97JM-9MPGT-3V66T"; else if (choice == 2) key = "YNMGQ-8RYV3-4PGQ3-C8XTP-7CFBY"; else if (choice == 3) key = "XGVPP-NMH47-7TTHJ-W3FW7-8HV2C";
    } else if (currentOS.find("Pro") != string::npos && currentOS.find("Workstation") == string::npos) {
        choice = sc.readInt("1. Enterprise | 2. Edu | 3. Pro Workstation | 0. Back: ");
        if (choice == 1) key = "NPPR9-FWDCX-D2C8J-H872K-2YT43"; else if (choice == 2) key = "NW6C2-QMPVW-D7KKK-3GKT6-VCFB2"; else if (choice == 3) key = "DXG7C-N36C4-C4HTG-X4T3X-2YV77";
    } else if (currentOS.find("Education") != string::npos) {
        choice = sc.readInt("1. Enterprise | 0. Back: ");
        if (choice == 1) key = "XGVPP-NMH47-7TTHJ-W3FW7-8HV2C";
    } else return;

    if (!key.empty()) {
        string confirm; cout << "XAC NHAN VOI KEY: [" << key << "]? (Y/N): "; cin >> confirm;
        if (confirm == "y" || confirm == "Y") sc.runAdmin("changepk.exe /ProductKey " + key, true);
    }
}