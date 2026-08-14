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

SystemOptimizer::SystemOptimizer(SystemCore &s, Internet &net) : sc(s), n(net) {}

void SystemOptimizer::cleanDiskPro() {
    sc.cls();
    cout << "--- TIẾN TRÌNH DỌN RÁC CHUYÊN SÂU PRO ---\n\n";

    long long bytesBefore = 0;
    try {
        fs::space_info space = fs::space("C:\\");
        bytesBefore = space.available;
    } catch (...) {}

    cout << "[*] Đang quét và dọn rác toàn diện...\n\n";

    // Dọn cache người dùng bằng đa luồng
    vector<thread> userThreads;
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /s /f /q \"%temp%\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Microsoft\\Windows\\Recent\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Low\\Microsoft\\CryptnetUrlCache\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\D3DSCache\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\pip\\Cache\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\NVIDIA\\GLCache\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Microsoft\\Windows\\Explorer\\thumbcache_*.db\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\Temp\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Local\\Microsoft\\Windows\\WER\\*\" 2>nul"); });
    
    for (auto& t : userThreads) t.join();
    cout << "[✓] Đã dọn cache người dùng\n";

    // Dọn browser, thùng rác, dns
    clearBrowserCache();
    sc.runCMD("powershell -NoProfile -Command \"Clear-RecycleBin -Force -ErrorAction SilentlyContinue\"");
    sc.runCMD("ipconfig /flushdns");
    cout << "[✓] Đã dọn Browser, Recycle Bin, DNS\n";

    // Dọn rác hệ thống với quyền admin
    string batContent = "";
    batContent += "mkdir \"%SystemDrive%\\EmptyFolderTmp\" 2>nul\n";
    batContent += "start /b robocopy \"%SystemDrive%\\EmptyFolderTmp\" \"%systemroot%\\temp\" /mir /w:0 /r:0 /log:nul\n";
    batContent += "start /b robocopy \"%SystemDrive%\\EmptyFolderTmp\" \"%systemroot%\\Prefetch\" /mir /w:0 /r:0 /log:nul\n";
    batContent += "del /f /s /q \"%systemroot%\\SoftwareDistribution\\Download\\*\" 2>nul\n";
    batContent += "del /f /s /q \"%systemroot%\\Logs\\CBS\\*.*\" 2>nul\n";
    batContent += "del /f /q %windir%\\WindowsUpdate.log 2>nul\n";
    batContent += "del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\ReportQueue\\*\" 2>nul\n";
    batContent += "del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\ReportArchive\\*\" 2>nul\n";
    batContent += "del /f /s /q \"%ProgramData%\\Microsoft\\Windows Defender\\Scans\\History\\*\" 2>nul\n";
    batContent += "del /f /s /q \"%ProgramData%\\Microsoft\\Windows Defender\\LocalCopy\\*\" 2>nul\n";
    batContent += "net stop FontCache 2>nul\n";
    batContent += "del /f /s /q \"%WinDir%\\ServiceProfiles\\LocalService\\AppData\\Local\\FontCache\\*\" 2>nul\n";
    batContent += "net start FontCache 2>nul\n";
    batContent += "powershell -Command \"Get-DeliveryOptimizationStatus | Remove-DeliveryOptimizationCache -Confirm:$false\" 2>nul\n";
    batContent += "wevtutil el 2>nul | foreach { wevtutil cl \"$_\" 2>nul }\n";
    batContent += "del /f /s /q \"%SystemRoot%\\Minidump\\*\" 2>nul\n";
    batContent += "del /f /q \"%SystemRoot%\\Memory.dmp\" 2>nul\n";
    batContent += "cleanmgr /sagerun:1\n";
    batContent += "rmdir \"%SystemDrive%\\EmptyFolderTmp\" 2>nul\n";

    SystemCore::runBatchAsAdmin(batContent, "Dọn rác hệ thống chuyên sâu");

    long long bytesAfter = 0;
    try {
        fs::space_info space = fs::space("C:\\");
        bytesAfter = space.available;
    } catch (...) {}
    
    sc.cls();
    long long freed = bytesAfter - bytesBefore;
    if (freed > 0) {
        cout << "\n[✓] Đã giải phóng: " << SystemCore::formatSize(freed) << "\n";
    }

    cout << "\n[✓] Đã hoàn tất dọn dẹp hệ thống!\n";
}

void SystemOptimizer::disableAllStartupApps() {
    sc.cls();
    cout << "--- QUẢN LÝ ỨNG DỤNG KHỞI ĐỘNG CÙNG WINDOWS ---\n\n";
    cout << "[*] Đang quét và tắt các ứng dụng không thiết yếu...\n\n";
    int removedCount = 0;
    const struct { HKEY hKeyRoot; LPCSTR subKey; string name; } targets[] = {
        {HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", "HKCU"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "HKLM"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run", "HKLM_WOW64"}
    };

    // Danh sách ứng dụng an toàn không tắt
    vector<string> whitelist = {
        "SecurityHealth", "WindowsDefender", "MsMpEng",
        "RtkAudUService", "RtkAudUService64", "RtHDVCpl", "RtHDVBg",
        "RAVCpl64", "RAVBg64", "RtkNGUI64",
        "WavesSvc", "WavesSvc64", "WavesMaxxAudioService",
        "CTAudSvc", "CTHELPER", "VolPanel",
        "NvBackend", "NvTaskbarInit", "NvCplDaemon", "NvMediaCenter",
        "NVCP", "nvtray",
        "ADService", "ATKOSD", "RadeonSoftware", "RadeOnSettings",
        "AMDLinkUpdate", "AdobeGCInvoker",
        "IgfxTray", "igfxEM", "igfxHK", "igfxCUIService",
        "LCore", "LGHUB", "LogiOptions", "LogiOptionsPlus",
        "RazerCentralService", "Razer Synapse",
        "SteelSeriesGG", "CUE", "HyperX NGenuity",
        "Elan", "SynTPEnh", "SynTPHelper", "ETDCtrl",
        "HControl", "ATKOSD2", "FBAgent", "HotkeyUtility",
        "OneDrive",
        "ASUSTPCenter", "AsusUpdateCheck",
        "HPHotkeyMonitor", "HPPrintScanDoctorService",
        "LenovoUtility", "LenovoVantageService",
        "DellSupportAssist"
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
    cout << "\n[✓] Đã tắt thành công " << removedCount << " ứng dụng khởi động thừa.\n";
}

void SystemOptimizer::fixWindowsUpdate() {
    cout << "[1/3] Đang dừng dịch vụ Windows Update...\n";
    sc.runAdmin("net stop wuauserv", true); sc.runAdmin("net stop cryptSvc", true); sc.runAdmin("net stop bits", true); sc.runAdmin("net stop msiserver", true);
    cout << "[2/3] Đang xóa bộ nhớ đệm cập nhật kẹt...\n";
    sc.runCMD("del /f /q %windir%\\SoftwareDistribution\\*.*"); sc.runAdmin("rd /s /q %windir%\\SoftwareDistribution", true); sc.runAdmin("rd /s /q %windir%\\system32\\catroot2", true);
    cout << "[3/3] Đang khởi động lại dịch vụ...\n";
    sc.runAdmin("net start wuauserv", true); sc.runAdmin("net start cryptSvc", true); sc.runAdmin("net start bits", true); sc.runAdmin("net start msiserver", true);
    cout << "\n[✓] Đã khôi phục và reset Windows Update thành công!\n";
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

    cout << "[*] Đang dọn cache trình duyệt...\n";
    for (const string &path : cachePaths) {
        if (fs::exists(path)) {
            try {
                for (const auto &entry : fs::directory_iterator(path)) fs::remove_all(entry.path());
            } catch (...) {}
        }
    }
    if (appData) {
        string ffPath = string(appData) + "\\Mozilla\\Firefox\\Profiles";
        if (fs::exists(ffPath)) {
            for (const auto &profile : fs::directory_iterator(ffPath)) {
                string cacheDir = profile.path().string() + "\\cache2";
                if (fs::exists(cacheDir)) {
                    try {
                        for (const auto &entry : fs::directory_iterator(cacheDir)) fs::remove_all(entry.path());
                    } catch (...) {}
                }
            }
        }
    }
}

void SystemOptimizer::optimizeSystemPRO() {
    sc.cls();
    cout << "--- TỐI ƯU HÓA HỆ THỐNG TOÀN DIỆN (PRO 1-CLICK) ---\n\n";

    string batContent = "";
    batContent += "del /s /f /q \"%systemroot%\\temp\\*\" & rd /s /q \"%systemroot%\\temp\" & md \"%systemroot%\\temp\"\n";
    batContent += "del /s /f /q \"%systemroot%\\Prefetch\\*\"\n";
    batContent += "dism /online /cleanup-image /startcomponentcleanup\n";
    batContent += "powershell -Command \"Get-DeliveryOptimizationStatus | Remove-DeliveryOptimizationCache -Confirm:$false\"\n";
    batContent += "netsh branchcache flush\n";
    batContent += "winget uninstall \"Windows Web Experience Pack\" --silent --accept-source-agreements 2>nul\n";
    batContent += "powershell -Command \"Stop-Service -Name FontCache -Force; del /f /s /q $env:windir\\ServiceProfiles\\LocalService\\AppData\\Local\\FontCache\\* ; Start-Service -Name FontCache\"\n";
    batContent += "dism /online /cleanup-image /startcomponentcleanup /resetbase\n";
    batContent += "powershell -Command \"Get-EventLog -LogName * | ForEach { Clear-EventLog $_.Log }\"\n";

    // Tối ưu Registry
    batContent += "reg add \"HKCU\\Control Panel\\Desktop\" /v \"WaitToKillAppTimeout\" /t REG_SZ /d \"2000\" /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager\" /v \"SilentInstalledAppsEnabled\" /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection\" /v AllowTelemetry /t REG_DWORD /d 0 /f\n";
    batContent += "powercfg -h off\n";

    // Tối ưu Taskbar
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search\" /v SearchboxTaskbarMode /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v TaskbarDa /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v TaskbarMn /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v ShowTaskViewButton /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Feeds\" /v ShellFeedsTaskbarViewMode /t REG_DWORD /d 2 /f\n";
    batContent += "reg add \"HKCU\\Software\\Policies\\Microsoft\\Windows\\WindowsCopilot\" /v TurnOffWindowsCopilot /t REG_DWORD /d 1 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v SnapAssist /t REG_DWORD /d 0 /f\n";
    batContent += "taskkill /f /im explorer.exe & start explorer.exe\n";

    cout << "[*] Đang áp dụng các thiết lập tối ưu hệ thống...\n";
    SystemCore::runBatchAsAdmin(batContent, "Tối ưu hệ thống PRO");

    clearBrowserCache(); 

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

    cout << "\n[✓] Toàn bộ hệ thống đã được tối ưu hóa thành công!\n";
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
        {"wuauserv", "Windows Update (Ngăn tự động cập nhật hệ thống)"},
        {"UsoSvc", "Update Orchestrator Service (Điều phối cập nhật Windows)"},
        {"WaaSMedicSvc", "Windows Update Medic Service (Ngăn tự động bật lại Update)"},
        {"WerSvc", "Windows Error Reporting Service (Báo cáo lỗi về Microsoft)"},
        {"WpnService", "Windows Push Notifications System (Hệ thống thông báo/Widgets)"},
        {"WpnUserService", "Windows Push Notifications User Service (Tắt WebView2 ngầm)"},
        {"WpcSvc", "Parental Controls (Tính năng quản lý trẻ em gia đình)"},
        {"XblAuthManager", "Xbox Live Auth Manager (Xác thực tài khoản Xbox)"},
        {"XblGameSave", "Xbox Live Game Save (Đồng bộ dữ liệu game)"},
        {"XboxNetApiSvc", "Xbox Live Networking Service (Mạng Xbox)"},
        {"DiagTrack", "Connected User Experiences and Telemetry (Thu thập dữ liệu ngầm)"},
        {"dmwappushservice", "WAP Push Message Routing Service (Định tuyến trắc lượng)"},
        {"EdgeUpdate", "Microsoft Edge Update Service (Cập nhật trình duyệt ngầm)"},
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
        std::cout << "--- DANH SÁCH DỊCH VỤ HỆ THỐNG CÓ THỂ TỐI ƯU ---\n\n";
        
        for (size_t i = 0; i < targetSvcs.size(); ++i) {
            std::cout << " [" << std::setw(2) << i + 1 << "] " 
                      << std::left << std::setw(30) << (targetSvcs[i].desc.substr(0, 45) + "...") 
                      << " [" << targetSvcs[i].name << "]\n";
        }
        std::cout << "\n [A] Cấu hình tất cả dịch vụ cùng lúc\n";
        std::cout << " [0] Quay lại\n\n";
        
        std::cout << "Chọn số thứ tự dịch vụ muốn xử lý đơn lẻ, hoặc [A]/[0]: ";
        std::string input;
        std::cin >> input;

        if (input == "0") return;

        DWORD startType = SERVICE_DISABLED;
        std::string modeName = "";

        if (input == "A" || input == "a") {
            std::cout << "\n[?] Bạn muốn cấu hình TẤT CẢ các dịch vụ theo cách nào?\n";
            std::cout << " [1] Tối ưu thụ động (Manual - Chỉ chạy khi cần thiết)\n";
            std::cout << " [2] Vô hiệu hóa     (Disabled - Tắt hoàn toàn)\n";
            std::cout << " [0] Hủy bỏ\n\n";
            int action = sc.readInt("Chọn: ");
            if (action == 0) continue;

            startType = (action == 1) ? SERVICE_DEMAND_START : SERVICE_DISABLED;
            modeName = (action == 1) ? "MANUAL" : "DISABLED";

            std::cout << "\n[*] Đang thực thi cấu hình...\n";
            int successCount = 0;
            for (const auto &s : targetSvcs) {
                if (ServiceControlAPI(s.name, startType, true)) {
                    std::cout << "[OK] -> " << modeName << ": " << s.name << "\n";
                    successCount++;
                } else {
                    std::cout << "[!] Thất bại: " << s.name << "\n";
                }
            }
            std::cout << "\n[✓] Hoàn tất! Đã tối ưu " << successCount << "/" << targetSvcs.size() << " dịch vụ.\n";
            sc.waitEnter();
        }
        else {
            if (input.empty()) {
                std::cout << "[!] Vui lòng nhập lựa chọn!\n";
                Sleep(800);
                continue;
            }
            try {
                int idx = std::stoi(input) - 1;
                if (idx >= 0 && idx < (int)targetSvcs.size()) {
                    sc.cls();
                    std::cout << "--- CẤU HÌNH DỊCH VỤ ---\n\n";
                    std::cout << "Dịch vụ: " << targetSvcs[idx].desc << " [" << targetSvcs[idx].name << "]\n\n";
                    std::cout << " [1] Chuyển về MANUAL (Thụ động)\n";
                    std::cout << " [2] Chuyển về DISABLED (Tắt hẳn)\n";
                    std::cout << " [0] Hủy bỏ\n\n";
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
                } else {
                    std::cout << "[!] Lựa chọn không hợp lệ!\n";
                    Sleep(800);
                }
            }
            catch (...) {
                std::cout << "[!] Lựa chọn không hợp lệ!\n";
                Sleep(800);
            }
        }
    }
}

void SystemOptimizer::optimizeTaskbar() {
    sc.cls();
    cout << "--- TỐI ƯU TASKBAR WINDOWS 11 ---\n\n";
    
    struct TaskbarSetting {
        std::string keyPath;
        std::string valueName;
        DWORD targetValue;
        std::string description;
    };
    
    std::vector<TaskbarSetting> settings = {
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search", "SearchboxTaskbarMode", 0, "Search (Thanh tìm kiếm)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "TaskbarDa", 0, "Widgets (Tiện ích)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "TaskbarMn", 0, "Chat (Microsoft Teams)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "ShowTaskViewButton", 0, "Task View (Xem tác vụ)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Feeds", "ShellFeedsTaskbarViewMode", 2, "News & Interests (Tin tức)"},
        {"HKCU\\Software\\Policies\\Microsoft\\Windows\\WindowsCopilot", "TurnOffWindowsCopilot", 1, "Copilot (Trợ lý AI)"}
    };
    
    int alreadyDisabled = 0;
    int newlyDisabled = 0;
    int failedCount = 0;
    
    cout << "[*] Đang kiểm tra trạng thái Taskbar...\n\n";
    
    for (const auto& setting : settings) {
        HKEY hKey;
        DWORD currentValue = 0;
        DWORD dataSize = sizeof(DWORD);
        bool isAlreadyDisabled = false;
        
        if (RegOpenKeyExA(HKEY_CURRENT_USER, setting.keyPath.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExA(hKey, setting.valueName.c_str(), NULL, NULL, (LPBYTE)&currentValue, &dataSize) == ERROR_SUCCESS) {
                if (currentValue == setting.targetValue) {
                    isAlreadyDisabled = true;
                    alreadyDisabled++;
                }
            }
            
            if (!isAlreadyDisabled) {
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
    
    cout << "\n[*] Đã tắt sẵn từ trước: " << alreadyDisabled << " nút\n";
    cout << "[*] Vừa tắt thành công:   " << newlyDisabled << " nút\n";
    if (failedCount > 0) {
        cout << "[✗] Thất bại:            " << failedCount << " nút\n";
    }
    
    if (newlyDisabled > 0 || failedCount > 0) {
        cout << "\n[*] Khởi động lại Explorer để áp dụng thay đổi...\n";
        sc.runCMD("taskkill /f /im explorer.exe & start explorer.exe");
        cout << "[✓] Đã khởi động lại Explorer\n";
    } else {
        cout << "\n[○] Không có thay đổi nào. Taskbar đã được tối ưu.\n";
    }
}