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

static mutex batchMutex;

SystemOptimizer::SystemOptimizer(SystemCore &s, Internet &net) : sc(s), n(net) {}

void SystemOptimizer::QuickScanVirus() { 
    sc.runCMD("cmd /c \"\"%ProgramFiles%\\Windows Defender\\MpCmdRun.exe\" -Scan -ScanType 1\""); 
}

void SystemOptimizer::FullScanVirus() { 
    sc.runCMD("cmd /c \"\"%ProgramFiles%\\Windows Defender\\MpCmdRun.exe\" -Scan -ScanType 2\""); 
}
void SystemOptimizer::Consumer_Content() { sc.runCMD("reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager\" /v \"SilentInstalledAppsEnabled\" /t REG_DWORD /d 0 /f"); }
void SystemOptimizer::Hibernate() { sc.runAdmin("powercfg -h off", true); }
void SystemOptimizer::windowsTelemetry() { sc.runAdmin("reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection\" /v AllowTelemetry /t REG_DWORD /d 0 /f", true); }


void SystemOptimizer::cleanDiskPro() {
    sc.cls();
    cout << "             TIẾN TRÌNH DỌN RÁC CHUYÊN SÂU PRO\n\n";


    // === ĐO DUNG LƯỢNG TRƯỚC ===
    long long bytesBefore = 0;
    try {
        fs::space_info space = fs::space("C:\\");
        bytesBefore = space.available;
    } catch (...) {}

    cout << "[*] Đang quét và dọn rác toàn diện...\n\n";

    // === 1. DỌN RÁC USER (MULTI-THREAD - KHÔNG CẦN ADMIN) ===
    vector<thread> userThreads;
    
    userThreads.emplace_back([this]() {
        sc.runCMD("cmd /c del /s /f /q \"%temp%\\*\" 2>nul");
    });
    userThreads.emplace_back([this]() {
        sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Microsoft\\Windows\\Recent\\*\" 2>nul");
    });
    userThreads.emplace_back([this]() {
        sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Low\\Microsoft\\CryptnetUrlCache\\*\" 2>nul");
    });
    userThreads.emplace_back([this]() {
        sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\D3DSCache\\*\" 2>nul");
    });
    userThreads.emplace_back([this]() {
        sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\pip\\Cache\\*\" 2>nul");
    });
    userThreads.emplace_back([this]() {
        sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\NVIDIA\\GLCache\\*\" 2>nul");
    });
    userThreads.emplace_back([this]() {
        sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Microsoft\\Windows\\Explorer\\thumbcache_*.db\" 2>nul");
    });
    userThreads.emplace_back([this]() {
        sc.runCMD("cmd /c del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\Temp\\*\" 2>nul");
    });
    userThreads.emplace_back([this]() {
        sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Local\\Microsoft\\Windows\\WER\\*\" 2>nul");
    });
    
    for (auto& t : userThreads) t.join();
    cout << "[✓] Đã dọn cache người dùng\n";

    // Browser cache + Recycle Bin + DNS
    clearBrowserCache();
    sc.runCMD("powershell -NoProfile -Command \"Clear-RecycleBin -Force -ErrorAction SilentlyContinue\"");
    sc.runCMD("ipconfig /flushdns");
    cout << "[✓] Đã dọn Browser, Recycle Bin, DNS\n";

    //  DỌN RÁC HỆ THỐNG  ADMIN - GỘP 1 BATCH
    lock_guard<mutex> lock(batchMutex);
    
    fs::path absoluteBatPath = fs::absolute("CleanDiskPro_Advanced.bat");
    string tempBatPath = absoluteBatPath.string();

    ofstream batFile(tempBatPath);
    if (!batFile.is_open()) {
        cout << "[!] Không thể khởi tạo file batch\n";
        return;
    }

    batFile << "@echo off\n";
    batFile << "chcp 65001 > nul\n";

    // Temp & Prefetch (song song trong batch bằng start)
    batFile << "echo [1/9] Don temp he thong...\n";
    batFile << "mkdir \"%SystemDrive%\\EmptyFolderTmp\" 2>nul\n";
    batFile << "start /b robocopy \"%SystemDrive%\\EmptyFolderTmp\" \"%systemroot%\\temp\" /mir /w:0 /r:0 /log:nul\n";
    batFile << "start /b robocopy \"%SystemDrive%\\EmptyFolderTmp\" \"%systemroot%\\Prefetch\" /mir /w:0 /r:0 /log:nul\n";
    
    // Don Windows Update cache
    batFile << "del /f /s /q \"%systemroot%\\SoftwareDistribution\\Download\\*\" 2>nul\n";
    
    //Don CBS logs
    batFile << "del /f /s /q \"%systemroot%\\Logs\\CBS\\*.*\" 2>nul\n";
    batFile << "del /f /q %windir%\\WindowsUpdate.log 2>nul\n";
    
    //  Don Windows Error Reporting
    batFile << "del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\ReportQueue\\*\" 2>nul\n";
    batFile << "del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\ReportArchive\\*\" 2>nul\n";
    
    // Don Windows Defender history
    batFile << "del /f /s /q \"%ProgramData%\\Microsoft\\Windows Defender\\Scans\\History\\*\" 2>nul\n";
    batFile << "del /f /s /q \"%ProgramData%\\Microsoft\\Windows Defender\\LocalCopy\\*\" 2>nul\n";
    
    // Don Font cache
    batFile << "net stop FontCache 2>nul\n";
    batFile << "del /f /s /q \"%WinDir%\\ServiceProfiles\\LocalService\\AppData\\Local\\FontCache\\*\" 2>nul\n";
    batFile << "net start FontCache 2>nul\n";
    
    // Don Delivery Optimization
    batFile << "powershell -Command \"Get-DeliveryOptimizationStatus | Remove-DeliveryOptimizationCache -Confirm:$false\" 2>nul\n";
    
    // Don Event Logs
    batFile << "wevtutil el 2>nul | foreach { wevtutil cl \"$_\" 2>nul }\n";
    
    //  Don memory dumps
    batFile << "del /f /s /q \"%SystemRoot%\\Minidump\\*\" 2>nul\n";
    batFile << "del /f /q \"%SystemRoot%\\Memory.dmp\" 2>nul\n";
    
    // Cleanmgr
    batFile << "cleanmgr /sagerun:1\n";
    batFile << "rmdir \"%SystemDrive%\\EmptyFolderTmp\" 2>nul\n";

    batFile << "\necho [OK] Hòan tất!\n";
    batFile << "exit\n";
    batFile.close();

    cout << "\n[i] Yêu cầu quyền Admin để dọn rác hệ thống...\n";
    sc.runAdmin("\"" + tempBatPath + "\"", true);
    
    Sleep(500);
    fs::remove(absoluteBatPath);

    //TÍNH DUNG LƯỢNG GIẢI PHÓNG 
    long long bytesAfter = 0;
    try {
        fs::space_info space = fs::space("C:\\");
        bytesAfter = space.available;
    } catch (...) {}
    
    sc.cls();
    long long freed = bytesAfter - bytesBefore;
    if (freed > 0) {
        double size = freed;
        string unit = "Bytes";
        if (freed >= 1024LL * 1024LL * 1024LL) { size /= (1024.0 * 1024.0 * 1024.0); unit = "GB"; }
        else if (freed >= 1024LL * 1024LL) { size /= (1024.0 * 1024.0); unit = "MB"; }
        else if (freed >= 1024LL) { size /= 1024.0; unit = "KB"; }
        printf("\n\n[✓] Da giai phong: %.2f %s\n", size, unit.c_str());
    }

    cout << "\n[✓] THÀNH CÔNG\n";
}

void SystemOptimizer::cleanDiskBase() {   
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

    vector<string> whitelist = {
        // Bảo mật hệ thống
        "SecurityHealth", "WindowsDefender", "MsMpEng",
        // Âm thanh — Realtek (nhiều variant tên)
        "RtkAudUService", "RtkAudUService64", "RtHDVCpl", "RtHDVBg",
        "RAVCpl64", "RAVBg64", "RtkNGUI64",
        // Âm thanh — Waves, Creative, IDT
        "WavesSvc", "WavesSvc64", "WavesMaxxAudioService",
        "CTAudSvc", "CTHELPER", "VolPanel",
        // GPU — NVIDIA
        "NvBackend", "NvTaskbarInit", "NvCplDaemon", "NvMediaCenter",
        "NVCP", "nvtray",
        // GPU — AMD / ATI
        "ADService", "ATKOSD", "RadeonSoftware", "RadeOnSettings",
        "AMDLinkUpdate", "AdobeGCInvoker",
        // GPU — Intel
        "IgfxTray", "igfxEM", "igfxHK", "igfxCUIService",
        // Thiết bị ngoại vi — Logitech
        "LCore", "LGHUB", "LogiOptions", "LogiOptionsPlus",
        // Thiết bị ngoại vi — Razer
        "RazerCentralService", "Razer Synapse",
        // Thiết bị ngoại vi — SteelSeries, Corsair, HyperX
        "SteelSeriesGG", "CUE", "HyperX NGenuity",
        // Touchpad / Keyboard OEM
        "Elan", "SynTPEnh", "SynTPHelper", "ETDCtrl",
        "HControl", "ATKOSD2", "FBAgent", "HotkeyUtility",
        // Microsoft / OneDrive (giữ OneDrive vì tích hợp sâu)
        "OneDrive",
        // Laptop OEM tools (ASUS, HP, Lenovo, Dell)
        "ASUSTPCenter", "AsusUpdateCheck", "HControl",
        "HPHotkeyMonitor", "HPPrintScanDoctorService",
        "LenovoUtility", "LenovoVantageService",
        "DellSupportAssist",
    };

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
                        // So sánh không phân biệt hoa thường để tránh miss
                        string vLower = vName, sLower = safeApp;
                        transform(vLower.begin(), vLower.end(), vLower.begin(), ::tolower);
                        transform(sLower.begin(), sLower.end(), sLower.begin(), ::tolower);
                        if (vLower.find(sLower) != string::npos) { isSafe = true; break; }
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
    sc.runCMD("winget upgrade --all --silent");
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
    sc.cls();

    // Lấy đường dẫn tuyệt đối cho file .bat tạm thời để tránh lỗi lạc hướng thư mục của Admin
    fs::path absoluteBatPath = fs::absolute("OptimizeSystem.bat");
    string tempBatPath = absoluteBatPath.string();

    ofstream batFile(tempBatPath);
    
    if (batFile.is_open()) {
        batFile << "@echo off\n";
        batFile << "chcp 65001 > nul\n"; 

        // Dang don dep bo nho dem va file rac Pro
        batFile << "del /s /f /q \"%%systemroot%%\\temp\\*\" & rd /s /q \"%%systemroot%%\\temp\" & md \"%%systemroot%%\\temp\"\n";
        batFile << "del /s /f /q \"%%systemroot%%\\Prefetch\\*\"\n";
        batFile << "dism /online /cleanup-image /startcomponentcleanup\n";
        batFile << "powershell -Command \"Get-DeliveryOptimizationStatus | Remove-DeliveryOptimizationCache -Confirm:$false\"\n";
        batFile << "netsh branchcache flush\n";
        batFile << "winget uninstall \"Windows Web Experience Pack\" --silent --accept-source-agreements\n";
        batFile << "powershell -Command \"Stop-Service -Name FontCache -Force; del /f /s /q $env:windir\\ServiceProfiles\\LocalService\\AppData\\Local\\FontCache\\* ; Start-Service -Name FontCache\"\n";
        batFile << "dism /online /cleanup-image /analyzecomponentstore\n";
        batFile << "dism /online /cleanup-image /startcomponentcleanup /resetbase\n";
        batFile << "powershell -Command \"Get-EventLog -LogName * | ForEach { Clear-EventLog $_.Log }\"\n";

        // (Giảm thời gian chờ tắt máy)
        batFile << "echo [+] Dang toi uu toc do tat may...\n";
        batFile << "reg add \"HKCU\\Control Panel\\Desktop\" /v \"WaitToKillAppTimeout\" /t REG_SZ /d \"2000\" /f\n";

        // (Tắt cài đặt app quảng cáo ngầm)
        batFile << "echo [+] Dang tat tu dong cai dat app quang cao ngam...\n";
        batFile << "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager\" /v \"SilentInstalledAppsEnabled\" /t REG_DWORD /d 0 /f\n";

        // (Tắt theo dõi, thu thập dữ liệu của MS)
        batFile << "echo [+] Dang chan Windows Telemetry de tiet kiem tai nguyen...\n";
        batFile << "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection\" /v AllowTelemetry /t REG_DWORD /d 0 /f\n";

        // (Tắt chế độ ngủ đông để giải phóng file hiberfil.sys vài GB)
        batFile << "echo [+] Dang tat tinh nang Hibernate de lay lai dung luong o C...\n";
        batFile << "powercfg -h off\n";

        // BỔ SUNG: TỐI ƯU TASKBAR
        // 1. Tắt Search (0=Ẩn, 1=Icon, 2=Box)
        batFile << "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search\" /v SearchboxTaskbarMode /t REG_DWORD /d 0 /f\n";
        
        // 2. Tắt Widgets (0=Tắt, 1=Bật)
        batFile << "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v TaskbarDa /t REG_DWORD /d 0 /f\n";
        
        // 3. Tắt Chat/Teams (0=Tắt, 1=Bật)
        batFile << "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v TaskbarMn /t REG_DWORD /d 0 /f\n";
        
        // 4. Tắt Task View (0=Tắt, 1=Bật)
        batFile << "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v ShowTaskViewButton /t REG_DWORD /d 0 /f\n";
        
        // 5. Tắt News & Interests (0=Bật, 1=Icon, 2=Tắt)
        batFile << "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Feeds\" /v ShellFeedsTaskbarViewMode /t REG_DWORD /d 2 /f\n";
        
        // 6. Tắt Copilot (0=Bật, 1=Tắt)
        batFile << "reg add \"HKCU\\Software\\Policies\\Microsoft\\Windows\\WindowsCopilot\" /v TurnOffWindowsCopilot /t REG_DWORD /d 1 /f\n";
        
        // 7. Tắt Snap Assist (khi hover nút Maximize)
        batFile << "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v SnapAssist /t REG_DWORD /d 0 /f\n";
        
        // Restart Explorer để áp dụng
        batFile << "taskkill /f /im explorer.exe & start explorer.exe\n";

        batFile << "exit\n";

        
        batFile.close();

        cout << "[i] Dang yeu cau 1 quyen Admin" << endl;
        sc.runAdmin("\"" + tempBatPath + "\"", true);

        //Dang don dep bo nho dem Browser (Chrome, Edge, Firefox...)
        clearBrowserCache(); 

        // Đang giải phóng siêu tốc bộ nhớ đệm người dùng
        sc.runCMD("cmd /c \"mkdir \"%LocalAppData%\\EmptyFolderTmp\" 2>nul\"");
        sc.runCMD("cmd /c \"robocopy \"%LocalAppData%\\EmptyFolderTmp\" \"%temp%\" /mir /w:0 /r:0 /log:nul\"");
        sc.runCMD("cmd /c \"rmdir \"%LocalAppData%\\EmptyFolderTmp\" 2>nul\"");
        sc.runCMD("cmd /c \"md \"%temp%\" 2>nul\""); 
        sc.runCMD("cleanmgr /sagerun:1");
        sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Microsoft\\Windows\\Recent\\*\"");
        sc.runCMD("powershell -NoProfile -Command \"Clear-RecycleBin -Force -ErrorAction SilentlyContinue\"");
        sc.runCMD("del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\Temp\\*\"");
        sc.runCMD("del /f /s /q \"%AppData%\\Local\\Microsoft\\Windows\\WER\\*\"");
        sc.runCMD("del /f /s /q \"%LocalAppData%\\Low\\Microsoft\\CryptnetUrlCache\\*\"");
        sc.runCMD("del /f /s /q \"%LocalAppData%\\D3DSCache\\*\"");
        sc.runCMD("del /f /s /q %windir%\\WindowsUpdate.log");

        Sleep(800); 
        fs::remove(absoluteBatPath);

        cout << "\n[OK] Hệ thống đã được tối ưu hóa\n";
    } else {
        cout << "[!] Khong the khoi tao file cau hinh" << endl;
    }
}


void SystemOptimizer::enableSecurityPRO() {
    sc.cls();
    cout << "===== TIẾN TRÌNH TĂNG CƯỜNG BẢO MẬT HỆ THỐNG =====\n\n";

    string batContent = "";
    
    // 1. Bật Windows Defender và cập nhật định nghĩa
    batContent += "powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $false\"\n";
    batContent += "\"%ProgramFiles%\\Windows Defender\\MpCmdRun.exe\" -SignatureUpdate\n";
    
    // 2. Bật tường lửa
    batContent += "netsh advfirewall set allprofiles state on\n";
    
    // 3. Bật Controlled Folder Access
    batContent += "powershell -Command \"Set-MpPreference -EnableControlledFolderAccess Enabled\"\n";
    
    // 4. Vô hiệu hóa dịch vụ từ xa
    batContent += "sc config RemoteRegistry start= disabled\n";
    batContent += "sc stop RemoteRegistry\n";
    batContent += "sc config TermService start= disabled\n";
    batContent += "sc stop TermService\n";
    batContent += "sc config RasAuto start= disabled\n";
    batContent += "sc stop RasAuto\n";
    batContent += "sc config RasMan start= disabled\n";
    batContent += "sc stop RasMan\n";
    
    // 5. Vô hiệu hóa giao thức không an toàn (SMB1, LLMNR, NetBIOS)
    batContent += "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters\" /v SMB1 /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient\" /v EnableMulticast /t REG_DWORD /d 0 /f\n";
    batContent += "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-NetAdapter -Name $_.Name -NetLuid $_.NetLuid -NetBIOSSetting Disabled }\"\n";
    
    // 6. Chặn các cổng nguy hiểm (TCP: 445,139,135,137,138,3389)
    vector<int> ports = {445,139,135,137,138,3389};
    for (int p : ports) {
        string ruleName = "Block_Dangerous_Port_" + to_string(p);
        batContent += "netsh advfirewall firewall delete rule name=\"" + ruleName + "\"\n";
        batContent += "netsh advfirewall firewall delete rule name=\"" + ruleName + "_out\"\n";
        batContent += "netsh advfirewall firewall add rule name=\"" + ruleName + "\" dir=in action=block protocol=TCP localport=" + to_string(p) + "\n";
        batContent += "netsh advfirewall firewall add rule name=\"" + ruleName + "_out\" dir=out action=block protocol=TCP localport=" + to_string(p) + "\n";
    }
    
    // 7. Cấu hình DNS over HTTPS (Cloudflare)
    batContent += "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ServerAddresses ('1.1.1.1','1.0.0.1') }\"\n";
    batContent += "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters\" /v EnableAutoDoh /t REG_DWORD /d 2 /f\n";
    
    // 8. Tắt Telemetry (giữ nguyên)
    batContent += "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection\" /v AllowTelemetry /t REG_DWORD /d 0 /f\n";

    // Chạy batch
    runBatchAsAdmin(batContent, "Tăng cường bảo mật");
    
    // Hiển thị báo cáo trạng thái (không cần admin)
    Internet tempInternet(sc); // tạm tạo nếu cần, hoặc dùng biến n
    n.checkSecurityStatus();   // n là Internet& đã có
    cout << "\n[OK] Hoàn tất tăng cường bảo mật.\n";
}

void SystemOptimizer::optimizeNetworkPRO() {
    sc.cls();
    cout << "===== TIẾN TRÌNH TỐI ƯU MẠNG & BẢO MẬT =====\n\n";

    string batContent = "";
    
    // Flush DNS và reset TCP/IP
    batContent += "ipconfig /flushdns\n";
    batContent += "netsh int ip reset\n";
    
    // Vô hiệu hóa giao thức không an toàn
    batContent += "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters\" /v SMB1 /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient\" /v EnableMulticast /t REG_DWORD /d 0 /f\n";
    batContent += "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-NetAdapter -Name $_.Name -NetLuid $_.NetLuid -NetBIOSSetting Disabled }\"\n";
    
    // Chặn cổng nguy hiểm
    vector<int> ports = {445,139,135,137,138,3389};
    for (int p : ports) {
        string ruleName = "Block_Dangerous_Port_" + to_string(p);
        batContent += "netsh advfirewall firewall delete rule name=\"" + ruleName + "\"\n";
        batContent += "netsh advfirewall firewall delete rule name=\"" + ruleName + "_out\"\n";
        batContent += "netsh advfirewall firewall add rule name=\"" + ruleName + "\" dir=in action=block protocol=TCP localport=" + to_string(p) + "\n";
        batContent += "netsh advfirewall firewall add rule name=\"" + ruleName + "_out\" dir=out action=block protocol=TCP localport=" + to_string(p) + "\n";
    }
    
    // DNS over HTTPS
    batContent += "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ServerAddresses ('1.1.1.1','1.0.0.1') }\"\n";
    batContent += "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters\" /v EnableAutoDoh /t REG_DWORD /d 2 /f\n";
    
    // Tắt Telemetry
    batContent += "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection\" /v AllowTelemetry /t REG_DWORD /d 0 /f\n";

    runBatchAsAdmin(batContent, "Tối ưu mạng");
    
    n.checkSecurityStatus();
    cout << "\n[OK] Hoàn tất tối ưu mạng.\n";
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

bool SystemOptimizer::ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService) {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) return false;

    SC_HANDLE svc = OpenServiceA(scm, serviceName.c_str(), SERVICE_CHANGE_CONFIG | SERVICE_STOP | SERVICE_START);
    if (!svc) { CloseServiceHandle(scm); return false; }

    bool configSuccess = ChangeServiceConfigA(svc, SERVICE_NO_CHANGE, startupType, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if (stopService) {
        SERVICE_STATUS status;
        ControlService(svc, SERVICE_CONTROL_STOP, &status);
    }
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return configSuccess;
}

void SystemOptimizer::turnOffServicesMenu() {
    sc.cls();
    struct SvcInfo { std::string name; std::string desc; };
    std::vector<SvcInfo> targetSvcs = {
        // Windows Update & Bảo trì
        {"wuauserv", "Windows Update (Ngăn tự động cập nhật hệ thống)"},
        {"UsoSvc", "Update Orchestrator Service (Điều phối cập nhật Windows)"},
        {"WaaSMedicSvc", "Windows Update Medic Service (Ngăn tự động bật lại Update)"},
        {"WerSvc", "Windows Error Reporting Service (Báo cáo lỗi về Microsoft)"},
        // Widgets, Notifications & Web Services
        {"WpnService", "Windows Push Notifications System (Hệ thống thông báo/Widgets)"},
        {"WpnUserService", "Windows Push Notifications User Service (Tắt WebView2 ngầm)"},
        {"WpcSvc", "Parental Controls (Tính năng quản lý trẻ em gia đình)"},
        // Xbox & Gaming Services
        {"XblAuthManager", "Xbox Live Auth Manager (Xác thực tài khoản Xbox)"},
        {"XblGameSave", "Xbox Live Game Save (Đồng bộ dữ liệu game)"},
        {"XboxNetApiSvc", "Xbox Live Networking Service (Mạng Xbox)"},
        // Telemetry & Thu thập dữ liệu
        {"DiagTrack", "Connected User Experiences and Telemetry (Thu thập dữ liệu ngầm)"},
        {"dmwappushservice", "WAP Push Message Routing Service (Định tuyến trắc lượng)"},
        // Trình duyệt & Cập nhật bên thứ ba
        {"EdgeUpdate", "Microsoft Edge Update Service (Cập nhật trình duyệt ngầm)"},
        // Các dịch vụ phần cứng / Tính năng khác không cần thiết
        {"wisvc", "Windows Insider Service (Dịch vụ chương trình thử nghiệm)"},
        {"Spooler", "Print Spooler (Dịch vụ in ấn - Tắt nếu không dùng máy in)"},
        {"BthServ", "Bluetooth Support Service (Tắt nếu PC không có Bluetooth)"},
        {"MapsBroker", "Downloaded Maps Manager (Quản lý bản đồ ngoại tuyến)"},
        {"RemoteRegistry", "Remote Registry (Cho phép sửa Registry từ xa - Nguy cơ bảo mật)"},
        {"SysMain", "Superfetch / SysMain (Nên tắt hoàn toàn nếu dùng SSD)"},
        {"WalletService", "Wallet Service (Ví điện tử và thanh toán Windows)"}
    };

    while (true) {
        sc.cls();
        std::cout << "====================================================================\n";
        std::cout << "             DANH SÁCH DỊCH VỤ HỆ THỐNG CÓ THỂ TỐI ƯU\n";
        std::cout << "====================================================================\n";
        
        // In danh sách kèm số thứ tự [1], [2], ...
        for (size_t i = 0; i < targetSvcs.size(); ++i) {
            std::cout << " [" << std::setw(2) << i + 1 << "] " 
                      << std::left << std::setw(30) << (targetSvcs[i].desc.substr(0, 45) + "...") 
                      << " [" << targetSvcs[i].name << "]\n";
        }
        std::cout << "====================================================================\n";
        std::cout << " [A] Cấu hình TẤT CẢ dịch vụ cùng lúc\n";
        std::cout << " [0] Quay lại Menu chính\n";
        std::cout << "====================================================================\n";
        
        std::cout << "Chọn số thứ tự dịch vụ muốn xử lý đơn lẻ, hoặc [A]/[0]: ";
        std::string input;
        std::cin >> input;

        if (input == "0") return;

        // XỬ LÝ OPTIONS CHÍNH
        DWORD startType = SERVICE_DISABLED;
        std::string modeName = "";

        if (input == "A" || input == "a") {
            std::cout << "\n[?] Bạn muốn cấu hình TẤT CẢ các dịch vụ theo cách nào?\n";
            std::cout << " [1] Tối ưu thụ động (Manual - Chỉ chạy khi hệ thống yêu cầu)\n";
            std::cout << " [2] Vô hiệu hóa     (Disabled - Tắt hoàn toàn, giải phóng RAM/CPU)\n";
            std::cout << " [0] Hủy bỏ\n";
            int action = sc.readInt("Chọn: ");
            if (action == 0) continue;

            startType = (action == 1) ? SERVICE_DEMAND_START : SERVICE_DISABLED;
            modeName = (action == 1) ? "MANUAL" : "DISABLED";

            std::cout << "\n[*] Đang thực thi cấu hình hàng loạt...\n";
            int successCount = 0;
            for (const auto &s : targetSvcs) {
                if (ServiceControlAPI(s.name, startType, true)) {
                    std::cout << "[OK] -> " << modeName << ": " << s.name << "\n";
                    successCount++;
                } else {
                    std::cout << "[!] Thất bại: " << s.name << "\n";
                }
            }
            std::cout << "\n[SUCCESS] Hoàn tất! Đã tối ưu " << successCount << "/" << targetSvcs.size() << " dịch vụ.\n";
            sc.waitEnter();
        }
        else{
            if(input.empty()){
                std::cout << "[!] Vui lòng nhập lựa chọn!\n";
                Sleep(1000);
                continue;
            }try{
                int idx = std::stoi(input) - 1;
                if (idx >= 0 && idx < (int)targetSvcs.size())
                {
                    sc.cls();
                    std::cout << "=== CẤU HÌNH RIÊNG LẺ DỊCH VỤ ===\n";
                    std::cout << "Dịch vụ: " << targetSvcs[idx].desc << " [" << targetSvcs[idx].name << "]\n\n";
                    std::cout << " [1] Chuyển về MANUAL (Thụ động)\n";
                    std::cout << " [2] Chuyển về DISABLED (Tắt hẳn)\n";
                    std::cout << " [0] Hủy bỏ\n";
                    int action = sc.readInt("Chọn hướng xử lý: ");
                    if (action == 0) continue;

                    startType = (action == 1) ? SERVICE_DEMAND_START : SERVICE_DISABLED;
                    modeName = (action == 1) ? "MANUAL" : "DISABLED";

                    std::cout << "\n[*] Đang xử lý dịch vụ " << targetSvcs[idx].name << "...\n";
                    if (ServiceControlAPI(targetSvcs[idx].name, startType, true)) {
                        std::cout << "[OK] Đã chuyển trạng thái sang -> " << modeName << "\n";
                    } else {
                        std::cout << "[!] Thất bại! Cần chạy công cụ bằng quyền Administrator.\n";
                    }
                    sc.waitEnter();
                }
                else
                {
                    std::cout << "[!] Số thứ tự nằm ngoài phạm vi danh sách!\n";
                    Sleep(1000);
                }
            }
            catch (...)
            {
                std::cout << "[!] Vui lòng nhập số thứ tự hoặc ký tự hợp lệ!\n";
                Sleep(1000);
            }
        }
    }
}

void SystemOptimizer::runBatchAsAdmin(const std::string &batContent, const std::string &description) {
    lock_guard<mutex> lock(batchMutex);
    
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    string batPath = string(tempPath) + "SecurityOpt_" + to_string(GetCurrentProcessId()) + ".bat";
    
    ofstream batFile(batPath);
    if (!batFile) {
        cout << "[!] Không thể tạo file bat tạm.\n";
        return;
    }
    batFile << "@echo off\n";
    batFile << "chcp 65001 >nul\n";
    batFile << batContent;
    batFile << "\nexit\n";
    batFile.close();

    string cmd = "\"" + batPath + "\"";
    sc.runAdmin(cmd, true);
    
    Sleep(500);
    fs::remove(batPath);
}

void SystemOptimizer::optimizeTaskbar() {
    sc.cls();
    cout << "============================================================\n";
    cout << "          TỐI ƯU TASKBAR WINDOWS 11\n";
    cout << "============================================================\n\n";
    
    // === ĐỊNH NGHĨA CÁC KEY CẦN KIỂM TRA ===
    struct TaskbarSetting {
        std::string keyPath;      // Đường dẫn Registry
        std::string valueName;    // Tên giá trị
        DWORD targetValue;        // Giá trị cần set (0 = tắt)
        std::string description;  // Mô tả
    };
    
    // Mảng cấu hình các nút Taskbar
    std::vector<TaskbarSetting> settings = {
        // 1. Search Box - Giá trị: 0=Ẩn, 1=Icon, 2=Box
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search", 
         "SearchboxTaskbarMode", 0, "Search (Thanh tìm kiếm)"},
        
        // 2. Widgets - Giá trị: 0=Tắt, 1=Bật
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 
         "TaskbarDa", 0, "Widgets (Tiện ích)"},
        
        // 3. Chat/Teams - Giá trị: 0=Tắt, 1=Bật
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 
         "TaskbarMn", 0, "Chat (Microsoft Teams)"},
        
        // 4. Task View - Giá trị: 0=Tắt, 1=Bật
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 
         "ShowTaskViewButton", 0, "Task View (Xem tác vụ)"},
        
        // 5. News & Interests (Win 10) - Giá trị: 0=Bật, 1=Icon, 2=Tắt
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Feeds", 
         "ShellFeedsTaskbarViewMode", 2, "News & Interests (Tin tức)"},
        
        // 6. Copilot (Win 11) - Giá trị: 0=Bật, 1=Tắt
        {"HKCU\\Software\\Policies\\Microsoft\\Windows\\WindowsCopilot", 
         "TurnOffWindowsCopilot", 1, "Copilot (Trợ lý AI)"}
    };
    
    // === BIẾN ĐẾM TRẠNG THÁI ===
    int alreadyDisabled = 0;  // Đếm số cái đã tắt
    int newlyDisabled = 0;    // Đếm số cái vừa tắt
    int failedCount = 0;      // Đếm số cái lỗi
    
    cout << " [*] Đang kiểm tra trạng thái Taskbar...\n\n";
    
    // === DUYỆT TỪNG CÀI ĐẶT ===
    for (const auto& setting : settings) {
        HKEY hKey;
        DWORD currentValue = 0;
        DWORD dataSize = sizeof(DWORD);
        bool isAlreadyDisabled = false;
        
        // Mở key Registry
        if (RegOpenKeyExA(HKEY_CURRENT_USER, setting.keyPath.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            // Đọc giá trị hiện tại
            if (RegQueryValueExA(hKey, setting.valueName.c_str(), NULL, NULL, (LPBYTE)&currentValue, &dataSize) == ERROR_SUCCESS) {
                // Kiểm tra nếu đã ở trạng thái tắt
                if (currentValue == setting.targetValue) {
                    isAlreadyDisabled = true;
                    alreadyDisabled++;
                }
            }
            
            // Nếu chưa tắt -> tiến hành tắt
            if (!isAlreadyDisabled) {
                // Set giá trị mới
                if (RegSetValueExA(hKey, setting.valueName.c_str(), 0, REG_DWORD, (const BYTE*)&setting.targetValue, sizeof(DWORD)) == ERROR_SUCCESS) {
                    std::cout << "    [✓] Đã tắt: " << setting.description << "\n";
                    newlyDisabled++;
                } else {
                    std::cout << "    [✗] Lỗi tắt: " << setting.description << "\n";
                    failedCount++;
                }
            } else {
                std::cout << "    [○] Đã tắt từ trước: " << setting.description << "\n";
            }
            
            RegCloseKey(hKey);
        } else {
            // Key chưa tồn tại -> tạo mới và set giá trị
            if (RegCreateKeyExA(HKEY_CURRENT_USER, setting.keyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                if (RegSetValueExA(hKey, setting.valueName.c_str(), 0, REG_DWORD, (const BYTE*)&setting.targetValue, sizeof(DWORD)) == ERROR_SUCCESS) {
                    std::cout << "    [✓] Đã tạo và tắt: " << setting.description << "\n";
                    newlyDisabled++;
                } else {
                    std::cout << "    [✗] Lỗi tạo key: " << setting.description << "\n";
                    failedCount++;
                }
                RegCloseKey(hKey);
            } else {
                std::cout << "    [✗] Không thể tạo key: " << setting.description << "\n";
                failedCount++;
            }
        }
    }
    
    // === HIỂN THỊ BÁO CÁO ===
    cout << "\n============================================================\n";
    cout << "           BÁO CÁO TỐI ƯU TASKBAR\n";
    cout << "============================================================\n";
    cout << " [*] Đã tắt sẵn từ trước: " << alreadyDisabled << " nút\n";
    cout << " [*] Vừa tắt thành công:   " << newlyDisabled << " nút\n";
    if (failedCount > 0) {
        cout << " [✗] Thất bại:            " << failedCount << " nút\n";
    }
    cout << "============================================================\n";
    
    // === NẾU CÓ THAY ĐỔI, RESTART EXPLORER ===
    if (newlyDisabled > 0 || failedCount > 0) {
        cout << "\n [*] Khởi động lại Explorer để áp dụng thay đổi...\n";
        sc.runCMD("taskkill /f /im explorer.exe & start explorer.exe");
        cout << " [✓] Đã khởi động lại Explorer\n";
    } else {
        cout << "\n [○] Không có thay đổi nào. Taskbar đã được tối ưu.\n";
    }
    
    cout << "\n============================================================\n";
    cout << " [✓] HOÀN THÀNH TỐI ƯU TASKBAR!\n";
    cout << "============================================================\n";
}