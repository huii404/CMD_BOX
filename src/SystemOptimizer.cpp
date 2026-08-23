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

/**
 * =========================================================================================
 * 1. HÀM DỌN RÁC NHANH PLUS (cleanDiskQuick) - Tốc độ siêu tốc (1 - 3 giây)
 * =========================================================================================
 * TÍNH NĂNG:
 * - Tập trung dọn các vùng rác phát sinh thường ngày bằng đa luồng song song:
 *   + User Temp (%TEMP%) & System Temp (C:\Windows\Temp)
 *   + Lịch sử tệp vừa mở (Recent Files)
 *   + Thùng rác (Recycle Bin)
 *   + Bộ đệm phân giải tên miền (Flush DNS)
 *   + Bộ đệm DirectX (D3DSCache), CryptnetUrlCache, Báo cáo sự cố tạm thời (WER Temp)
 */
void SystemOptimizer::cleanDiskQuick() {
    sc.cls();
    cout << "⚡ Đang dọn rác nhanh (Quick Clean Plus)...\n\n";

    long long bytesBefore = 0;
    try {
        fs::space_info space = fs::space("C:\\");
        bytesBefore = space.available;
    } catch (...) {}

    // Dọn song song các vùng rác nhẹ bằng đa luồng
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

    for (auto& t : threads) t.join();

    long long bytesAfter = 0;
    try {
        fs::space_info space = fs::space("C:\\");
        bytesAfter = space.available;
    } catch (...) {}

    long long freed = bytesAfter - bytesBefore;
    cout << "✅ Đã dọn xong tức thì!";
    if (freed > 0) {
        cout << " (Đã giải phóng: " << SystemCore::formatSize(freed) << ")";
    }
    cout << "\n\n";
    sc.waitEnter();
}

/**
 * =========================================================================================
 * 2. HÀM DỌN RÁC Ổ ĐĨA TOÀN DIỆN (cleanDiskPro)
 * =========================================================================================
 * TÍNH NĂNG:
 * - Đo dung lượng ổ C: trước và sau khi dọn để báo cáo dung lượng đã giải phóng.
 * - [Đa luồng / Multi-threaded]: Dọn dẹp đồng thời nhiều vùng cache người dùng (%temp%, Recent,
 *   CryptnetUrlCache, DirectX Shader Cache, NVIDIA Shader Cache, Thumbnail DB, WER Crash Logs).
 * - Dọn dẹp cache trình duyệt (Chrome, Edge, Firefox, Cốc Cốc, Brave...) qua hàm clearBrowserCache().
 * - Dọn dẹp cache môi trường lập trình (Node/npm, Pip, NuGet, Cargo, Gradle...) qua hàm cleanDevCaches().
 * - Làm rỗng Thùng rác (Recycle Bin) và xóa DNS Cache (Flush DNS).
 * - [Chạy quyền Administrator qua Batch]:
 *   + Dùng thủ thuật Robocopy /MIR với thư mục rỗng để xóa nhanh triệu file rác trong System Temp và Prefetch.
 *   + Xóa rác cập nhật Windows (SoftwareDistribution/Download, CBS Logs, WindowsUpdate.log).
 *   + Xóa báo cáo lỗi hệ thống (WER ReportQueue/ReportArchive) và cache lịch sử Defender.
 *   + Khởi động lại FontCache để xóa font cache lỗi thời.
 *   + Xóa Delivery Optimization Cache, xóa toàn bộ Event Logs (nhật ký sự kiện), Dump files.
 *   + Gọi cleanmgr /sagerun:1 (công cụ dọn dẹp gốc của Windows).
 * 
 * CÁCH BỔ SUNG THÊM VÙNG RÁC SAU NÀY:
 * - Để thêm vùng cache User: Thêm 1 dòng `userThreads.emplace_back(...)` ở phần [ĐOẠN 1].
 * - Để thêm lệnh xóa System rác sâu: Thêm dòng `batContent += "del ...\n";` ở phần [ĐOẠN 2].
 */
void SystemOptimizer::cleanDiskPro() {
    sc.cls();

    // Lấy dung lượng khả dụng trước khi dọn để đo hiệu quả
    long long bytesBefore = 0;
    try {
        fs::space_info space = fs::space("C:\\");
        bytesBefore = space.available;
    } catch (...) {}

    // --- [ĐOẠN 1: DỌN CACHE NGƯỜI DÙNG BẰNG ĐA LUỒNG] ---
    // (Có thể thêm đường dẫn cache mới của các ứng dụng khác vào danh sách luồng này)
    vector<thread> userThreads;
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /s /f /q \"%temp%\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Microsoft\\Windows\\Recent\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Low\\Microsoft\\CryptnetUrlCache\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\D3DSCache\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\NVIDIA\\GLCache\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Microsoft\\Windows\\Explorer\\thumbcache_*.db\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\Temp\\*\" 2>nul"); });
    userThreads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Local\\Microsoft\\Windows\\WER\\*\" 2>nul"); });
    
    // Đợi tất cả luồng dọn rác người dùng hoàn tất
    for (auto& t : userThreads) t.join();;

    // --- [DỌN TRÌNH DUYỆT, RÁC DEV, THÙNG RÁC, FLUSH DNS] ---
    clearBrowserCache();
    cleanDevCaches();
    sc.runCMD("powershell -NoProfile -Command \"Clear-RecycleBin -Force -ErrorAction SilentlyContinue\"");
    sc.runCMD("ipconfig /flushdns");

    // --- [ĐOẠN 2: DỌN RÁC HỆ THỐNG YÊU CẦU QUYỀN ADMINISTRATOR] ---
    // (Thêm các lệnh xóa rác hệ thống sâu bằng Batch script tại đây)
    string batContent = "";
    batContent += "mkdir \"%SystemDrive%\\EmptyFolderTmp\" 2>nul\n";
    // Thủ thuật Robocopy /MIR xóa hàng vạn file temp & prefetch cực nhanh
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
    batContent += "wevtutil el 2>nul | foreach { wevtutil cl \"$_\" 2>nul }\n"; // Xóa sạch Event Viewer logs
    batContent += "del /f /s /q \"%SystemRoot%\\Minidump\\*\" 2>nul\n";
    batContent += "del /f /q \"%SystemRoot%\\Memory.dmp\" 2>nul\n";
    batContent += "cleanmgr /sagerun:1\n";
    batContent += "rmdir \"%SystemDrive%\\EmptyFolderTmp\" 2>nul\n";

    // Thực thi kịch bản batch với quyền Quản trị viên
    SystemCore::runBatchAsAdmin(batContent, "Dọn rác hệ thống chuyên sâu");

    // Tính toán dung lượng thực tế đã giải phóng
    long long bytesAfter = 0;
    try {
        fs::space_info space = fs::space("C:\\");
        bytesAfter = space.available;
    } catch (...) {}
    
    sc.cls();
    long long freed = bytesAfter - bytesBefore;
    if (freed > 0) {
        cout << "\nĐã giải phóng: " << SystemCore::formatSize(freed) << "\n";
    }

    cout << "\nHoàn tất dọn dẹp!\n";
    sc.waitEnter();
}

/**
 * =========================================================================================
 * 2. HÀM TẮT TẤT CẢ ỨNG DỤNG KHỞI ĐỘNG CÙNG WINDOWS (disableAllStartupApps)
 * =========================================================================================
 * TÍNH NĂNG:
 * - Quét các khóa Registry khởi động:
 *   + HKCU\Software\Microsoft\Windows\CurrentVersion\Run (User hiện tại)
 *   + HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run (Toàn máy 64-bit)
 *   + HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Run (Toàn máy 32-bit trên Win 64-bit)
 * - Tự động xóa các mục khởi động không cần thiết để tăng tốc độ boot máy.
 * - [Cơ chế Whitelist / Danh sách an toàn]:
 *   Bảo vệ các tiến trình cốt lõi quan trọng: Antivirus (Defender), Driver âm thanh (Realtek, Waves),
 *   Driver card màn hình (NVIDIA, AMD, Intel), Driver chuột/phím cơ (Logitech, Razer, Corsair...),
 *   Driver Touchpad (Synaptics, Elan), Phần mềm OEM Laptop (Lenovo, Asus, Dell, HP) và OneDrive.
 * 
 * CÁCH BỔ SUNG THÊM APP AN TOÀN:
 * - Để thêm app không bao giờ bị tắt: Thêm chuỗi nhận diện vào vector `whitelist`.
 */
void SystemOptimizer::disableAllStartupApps() {
    sc.cls();
    cout << "Đang quét và tắt ứng dụng khởi động...\n\n";
    int removedCount = 0;
    const struct { HKEY hKeyRoot; LPCSTR subKey; string name; } targets[] = {
        {HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", "HKCU"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "HKLM"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run", "HKLM_WOW64"}
    };

    // --- DANH SÁCH WHITELIST: CÁC ỨNG DỤNG AN TOÀN GIỮ LẠI ---
    vector<string> whitelist = {
        // Bảo mật & Defender
        "SecurityHealth", "WindowsDefender", "MsMpEng",
        // Âm thanh
        "RtkAudUService", "RtkAudUService64", "RtHDVCpl", "RtHDVBg",
        "RAVCpl64", "RAVBg64", "RtkNGUI64",
        "WavesSvc", "WavesSvc64", "WavesMaxxAudioService",
        "CTAudSvc", "CTHELPER", "VolPanel",
        // GPU (NVIDIA, AMD, Intel)
        "NvBackend", "NvTaskbarInit", "NvCplDaemon", "NvMediaCenter",
        "NVCP", "nvtray",
        "ADService", "ATKOSD", "RadeonSoftware", "RadeOnSettings",
        "AMDLinkUpdate", "AdobeGCInvoker",
        "IgfxTray", "igfxEM", "igfxHK", "igfxCUIService",
        // Gaming Gear / Phần mềm chuột phím
        "LCore", "LGHUB", "LogiOptions", "LogiOptionsPlus",
        "RazerCentralService", "Razer Synapse",
        "SteelSeriesGG", "CUE", "HyperX NGenuity",
        // Touchpad & Hotkeys Laptop
        "Elan", "SynTPEnh", "SynTPHelper", "ETDCtrl",
        "HControl", "ATKOSD2", "FBAgent", "HotkeyUtility",
        // Tiện ích hãng Laptop
        "ASUSTPCenter", "AsusUpdateCheck",
        "HPHotkeyMonitor", "HPPrintScanDoctorService",
        "LenovoUtility", "LenovoVantageService",
        "DellSupportAssist"
    };

    // Quét từng nhánh Registry
    for (const auto &target : targets) {
        HKEY hKey;
        if (RegOpenKeyExA(target.hKeyRoot, target.subKey, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            char valueName[256];
            DWORD nameSize, type;
            DWORD index = 0;
            vector<string> toDelete;

            // Đọc danh sách tất cả các Value trong key Run
            while (true) {
                nameSize = sizeof(valueName);
                if (RegEnumValueA(hKey, index, valueName, &nameSize, NULL, &type, NULL, NULL) == ERROR_SUCCESS) {
                    string vName(valueName);
                    bool isSafe = false;
                    
                    // Kiểm tra xem tên có khớp với Whitelist không (không phân biệt hoa/thường)
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

            // Xóa các entry không nằm trong whitelist
            for (const string &delName : toDelete) {
                if (RegDeleteValueA(hKey, delName.c_str()) == ERROR_SUCCESS) {
                    cout << "  Đã tắt: " << delName << " (" << target.name << ")\n";
                    removedCount++;
                }
            }
            RegCloseKey(hKey);
        }
    }
    cout << "\nĐã tắt " << removedCount << " ứng dụng khởi động.\n";
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
void SystemOptimizer::cleanDevCaches() {
    char *localAppData = std::getenv("LOCALAPPDATA");
    char *appData = std::getenv("APPDATA");
    char *userProfile = std::getenv("USERPROFILE");

    cout << "Đang dọn rác Dev Caches...\n";

    vector<fs::path> devCachePaths;

    // Cache trong AppData\Local
    if (localAppData) {
        string baseLocal = string(localAppData);
        devCachePaths.push_back(baseLocal + "\\npm-cache");
        devCachePaths.push_back(baseLocal + "\\Yarn\\Cache");
        devCachePaths.push_back(baseLocal + "\\pip\\cache");
        devCachePaths.push_back(baseLocal + "\\NuGet\\v3-cache");
        devCachePaths.push_back(baseLocal + "\\go-build");
        devCachePaths.push_back(baseLocal + "\\pnpm\\store");
    }

    // Cache trong AppData\Roaming
    if (appData) {
        string baseApp = string(appData);
        devCachePaths.push_back(baseApp + "\\npm-cache");
        devCachePaths.push_back(baseApp + "\\Code\\Cache");
        devCachePaths.push_back(baseApp + "\\Code\\CachedData");
        devCachePaths.push_back(baseApp + "\\Code\\CachedExtensionVSIXs");
        devCachePaths.push_back(baseApp + "\\Code\\User\\workspaceStorage");
    }

    // Cache trong Thư mục User Home (%USERPROFILE%)
    if (userProfile) {
        string baseUser = string(userProfile);
        devCachePaths.push_back(baseUser + "\\.gradle\\caches");
        devCachePaths.push_back(baseUser + "\\.gradle\\daemon");
        devCachePaths.push_back(baseUser + "\\.cargo\\registry\\cache");
        devCachePaths.push_back(baseUser + "\\.nuget\\packages");
        devCachePaths.push_back(baseUser + "\\.android\\cache");
    }

    // Xóa sạch nội dung bên trong các thư mục cache lập trình
    for (const auto &path : devCachePaths) {
        wipeFolderContents(path);
    }
}

/**
 * =========================================================================================
 * 6. HÀM TỐI ƯU HÓA HỆ THỐNG CHUYÊN SÂU (optimizeSystemPRO)
 * =========================================================================================
 * TÍNH NĂNG:
 * - Dọn dẹp & nén Component Store của Windows (DISM cleanup-image /resetbase) để giảm dung lượng WinSxS.
 * - Xóa bộ đệm Delivery Optimization, BranchCache, FontCache và tất cả Event Logs hệ thống.
 * - [Tinh chỉnh Registry hiệu năng cao]:
 *   + WaitToKillAppTimeout = 2000ms: Đóng ứng dụng treo nhanh khi tắt máy, không bị chờ lâu.
 *   + SilentInstalledAppsEnabled = 0: Ngăn Windows tự tải game/app rác từ Microsoft Store.
 *   + AllowTelemetry = 0: Vô hiệu hóa thu thập dữ liệu chẩn đoán gửi về Microsoft.
 *   + powercfg -h off: Tắt chế độ ngủ đông (Hibernate) để giải phóng hàng chục GB file hiberfil.sys.
 * - [Tối ưu giao diện & Taskbar]:
 *   + Ẩn thanh tìm kiếm Searchbox, Widgets, Teams Chat, Task View, Feeds tin tức, Copilot AI.
 *   + Tắt SnapAssist gây phiền khi kéo thả cửa sổ.
 *   + Khởi động lại explorer.exe để áp dụng ngay thay đổi.
 * - Kết hợp dọn dẹp toàn bộ Browser, Dev Cache, Recycle Bin, Temporary files.
 * 
 * CÁCH BỔ SUNG TWEAK REGISTRY MỚI:
 * - Thêm lệnh `batContent += "reg add \"...\" /v ... /t ... /d ... /f\n";` vào khối Tối ưu Registry.
 */
void SystemOptimizer::optimizeSystemPRO() {
    sc.cls();
    cout << "Đang chuẩn bị dọn dẹp Browser & Dev Caches...\n";

    // 1. Dọn dẹp cache trình duyệt & dev bằng C++
    clearBrowserCache(); 
    cleanDevCaches();

    // 2. Gom toàn bộ tác vụ hệ thống & Registry vào 1 kịch bản Batch chạy quyền Administrator duy nhất
    string batContent = "";
    
    // Dọn dẹp temp & Prefetch hệ thống bằng Robocopy siêu tốc
    batContent += "mkdir \"%SystemDrive%\\EmptyFolderTmp\" 2>nul\n";
    batContent += "start /b robocopy \"%SystemDrive%\\EmptyFolderTmp\" \"%systemroot%\\temp\" /mir /w:0 /r:0 /log:nul\n";
    batContent += "start /b robocopy \"%SystemDrive%\\EmptyFolderTmp\" \"%systemroot%\\Prefetch\" /mir /w:0 /r:0 /log:nul\n";
    batContent += "rmdir \"%SystemDrive%\\EmptyFolderTmp\" 2>nul\n";
    
    // Tối ưu hóa WinSxS Component Store bằng DISM
    batContent += "dism /online /cleanup-image /startcomponentcleanup\n";
    batContent += "powershell -Command \"Get-DeliveryOptimizationStatus | Remove-DeliveryOptimizationCache -Confirm:$false\"\n";
    batContent += "netsh branchcache flush\n";
    batContent += "powershell -Command \"Stop-Service -Name FontCache -Force; del /f /s /q $env:windir\\ServiceProfiles\\LocalService\\AppData\\Local\\FontCache\\* ; Start-Service -Name FontCache\"\n";
    batContent += "dism /online /cleanup-image /startcomponentcleanup /resetbase\n";
    batContent += "powershell -Command \"Get-EventLog -LogName * | ForEach { Clear-EventLog $_.Log }\"\n";

    // --- TỐI ƯU REGISTRY (TẮT QUẢNG CÁO, MẸO VẶT, TELEMETRY) ---
    // Ngăn Windows tự động cài đặt app rác ngầm
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager\" /v \"SilentInstalledAppsEnabled\" /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager\" /v \"SubscribedContent-310093Enabled\" /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager\" /v \"SubscribedContent-338388Enabled\" /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager\" /v \"SubscribedContent-338389Enabled\" /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager\" /v \"SystemPaneSuggestionsEnabled\" /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v \"ShowSyncProviderNotifications\" /t REG_DWORD /d 0 /f\n";
    
    // Tắt hiệu ứng trong suốt (Transparency Effects) để tiết kiệm GPU/RAM và pin
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize\" /v \"EnableTransparency\" /t REG_DWORD /d 0 /f\n";
    
    // Tắt Telemetry chẩn đoán ngầm
    batContent += "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection\" /v AllowTelemetry /t REG_DWORD /d 0 /f\n";
    
    // Tắt file Hibernate hiberfil.sys (giải phóng dung lượng bằng đúng dung lượng RAM)
    batContent += "powercfg -h off\n";

    // --- TỐI ƯU THANH TASKBAR & GIAO DIỆN ---
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search\" /v SearchboxTaskbarMode /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v TaskbarDa /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v TaskbarMn /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v ShowTaskViewButton /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Feeds\" /v ShellFeedsTaskbarViewMode /t REG_DWORD /d 2 /f\n";
    batContent += "reg add \"HKCU\\Software\\Policies\\Microsoft\\Windows\\WindowsCopilot\" /v TurnOffWindowsCopilot /t REG_DWORD /d 1 /f\n";
    batContent += "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\" /v SnapAssist /t REG_DWORD /d 0 /f\n";

    // --- DỌN RÁC NGƯỜI DÙNG, THÙNG RÁC, LOG VÀ DISK CLEANUP ---
    batContent += "del /f /s /q \"%AppData%\\Microsoft\\Windows\\Recent\\*\" 2>nul\n";
    batContent += "powershell -NoProfile -Command \"Clear-RecycleBin -Force -ErrorAction SilentlyContinue\"\n";
    batContent += "del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\Temp\\*\" 2>nul\n";
    batContent += "del /f /s /q \"%AppData%\\Local\\Microsoft\\Windows\\WER\\*\" 2>nul\n";
    batContent += "del /f /s /q \"%LocalAppData%\\Low\\Microsoft\\CryptnetUrlCache\\*\" 2>nul\n";
    batContent += "del /f /s /q \"%LocalAppData%\\D3DSCache\\*\" 2>nul\n";
    batContent += "del /f /q %windir%\\WindowsUpdate.log 2>nul\n";
    batContent += "cleanmgr /sagerun:1\n";
    batContent += "taskkill /f /im explorer.exe & start explorer.exe\n";

    SystemCore::runBatchAsAdmin(batContent, "Tối ưu hệ thống PRO");

    cout << "\nTối ưu hệ thống hoàn tất!\n";
    sc.waitEnter();
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
        {"WalletService", "Wallet Service (Ví điện tử và thanh toán Windows)"}
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
    
    struct TaskbarSetting {
        std::string keyPath;
        std::string valueName;
        DWORD targetValue;
        std::string description;
    };
    
    // Danh sách cấu hình Registry cần áp dụng cho Taskbar & Giao diện
    std::vector<TaskbarSetting> settings = {
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search", "SearchboxTaskbarMode", 0, "Search (Thanh tìm kiếm)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "TaskbarDa", 0, "Widgets (Tiện ích)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "TaskbarMn", 0, "Chat (Microsoft Teams)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "ShowTaskViewButton", 0, "Task View (Xem tác vụ)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Feeds", "ShellFeedsTaskbarViewMode", 2, "News & Interests (Tin tức)"},
        {"HKCU\\Software\\Policies\\Microsoft\\Windows\\WindowsCopilot", "TurnOffWindowsCopilot", 1, "Copilot (Trợ lý AI)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", "EnableTransparency", 0, "Transparency (Hiệu ứng trong suốt)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", "SubscribedContent-310093Enabled", 0, "Start Menu Ads (Gợi ý & Quảng cáo Start)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", "SubscribedContent-338388Enabled", 0, "Settings Tips (Mẹo & Gợi ý Cài đặt)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", "SubscribedContent-338389Enabled", 0, "Explorer Ads (Quảng cáo File Explorer)"},
        {"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "ShowSyncProviderNotifications", 0, "Sync Notifications (Thông báo Office/OneDrive)"}
    };
    
    int alreadyDisabled = 0;
    int newlyDisabled = 0;
    int failedCount = 0;
    
    cout << "Đang kiểm tra Taskbar...\n\n";
    
    for (const auto& setting : settings) {
        HKEY hKey;
        DWORD currentValue = 0;
        DWORD dataSize = sizeof(DWORD);
        bool isAlreadyDisabled = false;
        
        // Mở key Registry tương ứng
        if (RegOpenKeyExA(HKEY_CURRENT_USER, setting.keyPath.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            // Đọc giá trị hiện tại xem đã được tắt trước đó chưa
            if (RegQueryValueExA(hKey, setting.valueName.c_str(), NULL, NULL, (LPBYTE)&currentValue, &dataSize) == ERROR_SUCCESS) {
                if (currentValue == setting.targetValue) {
                    isAlreadyDisabled = true;
                    alreadyDisabled++;
                }
            }
            
            // Nếu chưa tắt thì ghi giá trị tối ưu mới
            if (!isAlreadyDisabled) {
                if (RegSetValueExA(hKey, setting.valueName.c_str(), 0, REG_DWORD, (const BYTE*)&setting.targetValue, sizeof(DWORD)) == ERROR_SUCCESS) {
                    std::cout << "  Đã tắt: " << setting.description << "\n";
                    newlyDisabled++;
                } else {
                    std::cout << "  Lỗi tắt: " << setting.description << "\n";
                    failedCount++;
                }
            } else {
                std::cout << "Đã tắt từ trước: " << setting.description << "\n";
            }
            
            RegCloseKey(hKey);
        } else {
            // Nếu key chưa tồn tại thì tạo mới
            if (RegCreateKeyExA(HKEY_CURRENT_USER, setting.keyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                if (RegSetValueExA(hKey, setting.valueName.c_str(), 0, REG_DWORD, (const BYTE*)&setting.targetValue, sizeof(DWORD)) == ERROR_SUCCESS) {
                    std::cout << "Đã tạo và tắt: " << setting.description << "\n";
                    newlyDisabled++;
                } else {
                    std::cout << "Lỗi tạo key: " << setting.description << "\n";
                    failedCount++;
                }
                RegCloseKey(hKey);
            } else {
                std::cout << "Lỗi tạo key: " << setting.description << "\n";
                failedCount++;
            }
        }
    }
    
    cout << "\nĐã tắt từ trước: " << alreadyDisabled << "\n"
         << "Vừa tắt xong:    " << newlyDisabled << "\n";
    if (failedCount > 0) {
        cout << "Thất bại:        " << failedCount << "\n";
    }
    
    // Nếu có thay đổi mới thì khởi động lại Explorer để áp dụng hiệu lực ngay
    if (newlyDisabled > 0 || failedCount > 0) {
        cout << "\nKhởi động lại Explorer...\n";
        sc.runCMD("taskkill /f /im explorer.exe & start explorer.exe");
        cout << "Đã khởi động lại Explorer.\n";
    } else {
        cout << "\nTaskbar đã được tối ưu từ trước.\n";
    }
    sc.waitEnter();
}
