#include "DiskCleaner.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <vector>
#include <string>
#include <regex>
#include <iomanip>
#include <thread>
#include <chrono>
#include <unordered_set>

using namespace std;
namespace fs = std::filesystem;

DiskCleaner::DiskCleaner(SystemCore &core) : sc(core) {}

void DiskCleaner::wipeFolderContents(const fs::path &dirPath) {
    if (!fs::exists(dirPath)) return;
    try {
        for (const auto &entry : fs::directory_iterator(dirPath)) {
            try {
                fs::remove_all(entry.path());
            } catch (...) {}
        }
    } catch (...) {}
}

bool DiskCleaner::forceDeleteFolder(const fs::path &path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) return false;

    fs::remove_all(path, ec);
    if (!ec && !fs::exists(path, ec)) return true;

    // Fallback qua lệnh rd /s /q
    string cmd = "cmd /c rd /s /q \"" + path.string() + "\" 2>nul";
    SystemCore::runRawCommand(cmd);
    return !fs::exists(path, ec);
}

int DiskCleaner::cleanDirectoryArtifacts(const fs::path &rootPath, 
                                        const vector<string> &targetDirNames, 
                                        const vector<string> &targetExtensions, 
                                        long long &freedBytes) {
    std::error_code ec;
    if (!fs::exists(rootPath, ec) || !fs::is_directory(rootPath, ec)) return 0;

    int deletedCount = 0;
    vector<fs::path> dirsToDelete;
    vector<fs::path> filesToDelete;

    try {
        auto it = fs::recursive_directory_iterator(rootPath, 
            fs::directory_options::skip_permission_denied, ec);
        auto end = fs::recursive_directory_iterator();

        while (it != end && !ec) {
            const auto &entry = *it;
            string filename = entry.path().filename().string();

            if (entry.is_directory(ec)) {
                bool matchDir = false;
                for (const auto &td : targetDirNames) {
                    if (_stricmp(filename.c_str(), td.c_str()) == 0) {
                        matchDir = true;
                        break;
                    }
                }
                if (matchDir) {
                    dirsToDelete.push_back(entry.path());
                    it.disable_recursion_pending();
                }
            } else if (entry.is_regular_file(ec)) {
                string ext = entry.path().extension().string();
                for (const auto &te : targetExtensions) {
                    if (_stricmp(ext.c_str(), te.c_str()) == 0) {
                        filesToDelete.push_back(entry.path());
                        break;
                    }
                }
            }
            it.increment(ec);
        }
    } catch (...) {}

    for (const auto &f : filesToDelete) {
        try {
            uintmax_t sz = fs::file_size(f, ec);
            if (fs::remove(f, ec)) {
                freedBytes += sz;
                deletedCount++;
            }
        } catch (...) {}
    }

    for (const auto &d : dirsToDelete) {
        try {
            uintmax_t dirSize = 0;
            try {
                for (const auto &sub : fs::recursive_directory_iterator(d, fs::directory_options::skip_permission_denied, ec)) {
                    if (sub.is_regular_file(ec)) dirSize += fs::file_size(sub, ec);
                }
            } catch (...) {}

            if (forceDeleteFolder(d)) {
                freedBytes += dirSize;
                deletedCount++;
            }
        } catch (...) {}
    }

    return deletedCount;
}

// ----------------------------------------------------------------------------------
// HELPERS DỌN DẸP DOWNLOADS (EXE & DUPLICATES)
// ----------------------------------------------------------------------------------

string DiskCleaner::getDownloadsPath() {
    PWSTR path = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &path))) {
        char buf[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, path, -1, buf, MAX_PATH, NULL, NULL);
        CoTaskMemFree(path);
        return string(buf);
    }
    const char *userProf = getenv("USERPROFILE");
    if (userProf) return string(userProf) + "\\Downloads";
    return "";
}

string DiskCleaner::cleanAppName(const string &raw) {
    string s = raw;
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    
    // Gỡ bỏ các từ khóa đuôi phổ biến trong file cài đặt
    static const vector<string> junkTokens = {
        "setup", "installer", "install", "x64", "x86", "win64", "win32", 
        "64bit", "32bit", "user", "portable", "standalone", "full"
    };
    for (const auto &jt : junkTokens) {
        size_t pos = s.find(jt);
        if (pos != string::npos) {
            s.replace(pos, jt.length(), " ");
        }
    }

    string res = "";
    for (char c : s) {
        if (isalnum((unsigned char)c)) res += c;
        else if (res.empty() || res.back() != ' ') res += ' ';
    }
    return SystemCore::trim(res);
}

// Quét toàn bộ DisplayName của các phần mềm đã cài đặt trên hệ thống qua Registry
unordered_set<string> DiskCleaner::getInstalledAppNames() {
    unordered_set<string> installed;

    auto scanRegistryKey = [&](HKEY root, const char *subKey) {
        HKEY hKey;
        if (RegOpenKeyExA(root, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char keyName[256];
            DWORD keyIndex = 0;
            DWORD keyNameSize = sizeof(keyName);

            while (RegEnumKeyExA(hKey, keyIndex++, keyName, &keyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                HKEY hSubKey;
                if (RegOpenKeyExA(hKey, keyName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                    char displayName[512] = {0};
                    DWORD dataSize = sizeof(displayName);
                    if (RegQueryValueExA(hSubKey, "DisplayName", NULL, NULL, (LPBYTE)displayName, &dataSize) == ERROR_SUCCESS) {
                        string name = string(displayName);
                        transform(name.begin(), name.end(), name.begin(), ::tolower);
                        name = SystemCore::trim(name);
                        if (!name.empty()) {
                            installed.insert(name);
                            installed.insert(cleanAppName(name));
                        }
                    }
                    RegCloseKey(hSubKey);
                }
                keyNameSize = sizeof(keyName);
            }
            RegCloseKey(hKey);
        }
    };

    // 1. Quét 64-bit & 32-bit System Uninstall
    scanRegistryKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
    scanRegistryKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall");

    // 2. Quét User-level Uninstall
    scanRegistryKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall");

    return installed;
}

// Lấy ProductName từ Version Resource của tệp exe qua version.dll
string DiskCleaner::getExeProductName(const string &exePath) {
    HMODULE hVer = LoadLibraryA("version.dll");
    if (!hVer) return "";

    typedef DWORD (WINAPI *pfnGetFileVersionInfoSizeA)(LPCSTR, LPDWORD);
    typedef BOOL (WINAPI *pfnGetFileVersionInfoA)(LPCSTR, DWORD, DWORD, LPVOID);
    typedef BOOL (WINAPI *pfnVerQueryValueA)(LPCVOID, LPCSTR, LPVOID*, PUINT);

    auto pGetSize = (pfnGetFileVersionInfoSizeA)GetProcAddress(hVer, "GetFileVersionInfoSizeA");
    auto pGetInfo = (pfnGetFileVersionInfoA)GetProcAddress(hVer, "GetFileVersionInfoA");
    auto pQueryVal = (pfnVerQueryValueA)GetProcAddress(hVer, "VerQueryValueA");

    if (!pGetSize || !pGetInfo || !pQueryVal) {
        FreeLibrary(hVer);
        return "";
    }

    DWORD dummy = 0;
    DWORD size = pGetSize(exePath.c_str(), &dummy);
    if (size == 0) {
        FreeLibrary(hVer);
        return "";
    }

    vector<BYTE> data(size);
    if (!pGetInfo(exePath.c_str(), 0, size, data.data())) {
        FreeLibrary(hVer);
        return "";
    }

    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate = NULL;
    UINT cbTranslate = 0;

    string productName = "";
    if (pQueryVal(data.data(), "\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate) && cbTranslate >= sizeof(struct LANGANDCODEPAGE)) {
        char subBlock[128];
        sprintf_s(subBlock, sizeof(subBlock), "\\StringFileInfo\\%04x%04x\\ProductName", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
        LPVOID lpBuffer = NULL;
        UINT sizeStr = 0;
        if (pQueryVal(data.data(), subBlock, &lpBuffer, &sizeStr) && lpBuffer && sizeStr > 0) {
            productName = (char*)lpBuffer;
        }
    }

    if (productName.empty()) {
        static const char* fallbackBlocks[] = {
            "\\StringFileInfo\\040904b0\\ProductName",
            "\\StringFileInfo\\040904e4\\ProductName",
            "\\StringFileInfo\\000004b0\\ProductName"
        };
        for (const char* fb : fallbackBlocks) {
            LPVOID lpBuffer = NULL;
            UINT sizeStr = 0;
            if (pQueryVal(data.data(), fb, &lpBuffer, &sizeStr) && lpBuffer && sizeStr > 0) {
                productName = (char*)lpBuffer;
                break;
            }
        }
    }

    FreeLibrary(hVer);
    return SystemCore::trim(productName);
}

// ----------------------------------------------------------------------------------
// CÁC NHIỆM VỤ DỌN RÁC CHÍNH
// ----------------------------------------------------------------------------------

// 1. Dọn rác tạm bề mặt & Cache người dùng
long long DiskCleaner::cleanSurfaceAndUserTemp() {
    long long before = 0, after = 0;
    try { before = fs::space("C:\\").available; } catch (...) {}

    vector<thread> threads;
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /s /f /q \"%temp%\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%systemroot%\\temp\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%AppData%\\Microsoft\\Windows\\Recent\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\D3DSCache\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Low\\Microsoft\\CryptnetUrlCache\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\CrashDumps\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Microsoft\\Windows\\WER\\Temp\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Microsoft\\Windows\\WER\\ReportArchive\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Microsoft\\Windows\\WER\\ReportQueue\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\Temp\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("cmd /c del /f /s /q \"%LocalAppData%\\Microsoft\\Windows\\INetCache\\*\" 2>nul"); });
    threads.emplace_back([this]() { sc.runCMD("powershell -NoProfile -Command \"Clear-RecycleBin -Force -ErrorAction SilentlyContinue\""); });
    threads.emplace_back([this]() { sc.runCMD("ipconfig /flushdns >nul 2>&1"); });

    for (auto &t : threads) t.join();

    try { after = fs::space("C:\\").available; } catch (...) {}
    return (after > before) ? (after - before) : 0;
}

// 2. Dọn rác Trình duyệt & Ứng dụng
long long DiskCleaner::cleanBrowserAndAppCache() {
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

// 3. Dọn dẹp Chuyên sâu & Tồn dư Cập nhật
long long DiskCleaner::cleanDeepSystemAndUpdates() {
    long long before = 0, after = 0;
    try { before = fs::space("C:\\").available; } catch (...) {}

    string batContent = "";
    batContent += "net stop wuauserv 2>nul\n";
    batContent += "net stop bits 2>nul\n";
    batContent += "del /f /s /q %windir%\\SoftwareDistribution\\Download\\* 2>nul\n";
    batContent += "net start bits 2>nul\n";
    batContent += "net start wuauserv 2>nul\n";
    batContent += "del /f /s /q %windir%\\Logs\\CBS\\* 2>nul\n";
    batContent += "del /f /s /q %windir%\\Logs\\DISM\\* 2>nul\n";
    batContent += "del /f /s /q %windir%\\LiveKernelReports\\* 2>nul\n";
    batContent += "del /f /s /q %windir%\\Minidump\\* 2>nul\n";
    batContent += "del /f /q %windir%\\MEMORY.DMP 2>nul\n";
    batContent += "takeown /F \"%SystemDrive%\\$WINDOWS.~BT\" /A /R /D Y 2>nul\n";
    batContent += "icacls \"%SystemDrive%\\$WINDOWS.~BT\" /grant Administrators:F /T /C /Q 2>nul\n";
    batContent += "rd /s /q \"%SystemDrive%\\$WINDOWS.~BT\" 2>nul\n";
    batContent += "takeown /F \"%SystemDrive%\\$WINDOWS.~WS\" /A /R /D Y 2>nul\n";
    batContent += "icacls \"%SystemDrive%\\$WINDOWS.~WS\" /grant Administrators:F /T /C /Q 2>nul\n";
    batContent += "rd /s /q \"%SystemDrive%\\$WINDOWS.~WS\" 2>nul\n";
    batContent += "takeown /F \"%SystemDrive%\\Windows.old\" /A /R /D Y 2>nul\n";
    batContent += "icacls \"%SystemDrive%\\Windows.old\" /grant Administrators:F /T /C /Q 2>nul\n";
    batContent += "rd /s /q \"%SystemDrive%\\Windows.old\" 2>nul\n";
    batContent += "del /f /q %windir%\\WindowsUpdate.log 2>nul\n";
    batContent += "del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\ReportQueue\\*\" 2>nul\n";
    batContent += "del /f /s /q \"%ProgramData%\\Microsoft\\Windows\\WER\\ReportArchive\\*\" 2>nul\n";
    batContent += "powershell -Command \"Get-DeliveryOptimizationStatus | Remove-DeliveryOptimizationCache -Confirm:$false\" 2>nul\n";
    batContent += "for /f \"tokens=*\" %%a in ('wevtutil el 2^>nul') do wevtutil cl \"%%a\" 2>nul\n";
    batContent += "powercfg -h off\n";
    batContent += "cleanmgr /sagerun:1\n";
    batContent += "rmdir \"%SystemDrive%\\EmptyFolderTmp\" 2>nul\n";

    SystemCore::runBatchAsAdmin(batContent, "Dọn dẹp hệ thống chuyên sâu & Tồn dư cập nhật");

    try { after = fs::space("C:\\").available; } catch (...) {}
    return (after > before) ? (after - before) : 0;
}

// 4. Dọn rác Môi trường lập trình (Dev Artifacts)
long long DiskCleaner::cleanDevArtifactsAndCaches() {
    auto getTotalDrivesFreeSpace = []() -> long long {
        long long total = 0;
        DWORD mask = GetLogicalDrives();
        for (char c = 'C'; c <= 'Z'; ++c) {
            if (mask & (1 << (c - 'A'))) {
                string r = string(1, c) + ":\\";
                if (GetDriveTypeA(r.c_str()) == DRIVE_FIXED) {
                    try { total += fs::space(r).available; } catch (...) {}
                }
            }
        }
        return total;
    };

    long long before = getTotalDrivesFreeSpace();
    cleanDevCaches(false);
    long long after = getTotalDrivesFreeSpace();

    return (after > before) ? (after - before) : 0;
}

// 5. Dọn file cài đặt Exe & Rác tải về trong Downloads (CHỈ ÁP DỤNG CHO EXE/MSI & CRDOWNLOAD)
long long DiskCleaner::cleanDownloadsExesAndDuplicates() {
    string dlPath = getDownloadsPath();
    if (dlPath.empty() || !fs::exists(dlPath)) {
        cout << "     └── [!] Không tìm thấy thư mục Downloads!\n";
        return 0;
    }

    std::error_code ec;
    long long freedBytes = 0;
    int deletedExeCount = 0;
    int deletedDuplicateCount = 0;
    int deletedCorruptCount = 0;

    cout << "     ├── Đang quét danh mục phần mềm đã cài đặt trên Windows...\n";
    unordered_set<string> installed = getInstalledAppNames();

    // Regex phát hiện file trùng lặp: tên (1).exe, tên (2).msi
    regex dupRegex(R"(^(.+)\s\(([0-9]+)\)\.(exe|msi)$)", regex::icase);

    vector<fs::path> allFiles;
    for (const auto &entry : fs::directory_iterator(dlPath, ec)) {
        if (entry.is_regular_file(ec)) {
            allFiles.push_back(entry.path());
        }
    }

    // A. Dọn file tải hỏng / dở dang (.crdownload, .part, .tmp cũ hơn 24 giờ)
    auto now = fs::file_time_type::clock::now();
    for (const auto &p : allFiles) {
        string ext = p.extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".crdownload" || ext == ".part" || ext == ".tmp") {
            try {
                auto writeTime = fs::last_write_time(p, ec);
                auto ageHours = std::chrono::duration_cast<std::chrono::hours>(now - writeTime).count();
                if (ageHours >= 24) {
                    uintmax_t sz = fs::file_size(p, ec);
                    if (fs::remove(p, ec)) { // Xóa trực tiếp vĩnh viễn (không dùng Recycle Bin)
                        freedBytes += sz;
                        deletedCorruptCount++;
                    }
                }
            } catch (...) {}
        }
    }

    // B. Dọn các file cài đặt .exe / .msi trùng lặp & đã cài đặt
    for (const auto &p : allFiles) {
        string ext = p.extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".exe" && ext != ".msi") continue;

        string filename = p.filename().string();
        string stem = p.stem().string();
        uintmax_t sz = fs::file_size(p, ec);

        bool isDuplicate = false;
        string baseStem = stem;
        smatch match;
        if (regex_match(filename, match, dupRegex)) {
            isDuplicate = true;
            baseStem = match[1].str();
        }

        // 1. Kiểm tra xem phần mềm này đã cài đặt trên máy chưa
        string prodName = getExeProductName(p.string());
        string cleanedProd = cleanAppName(prodName);
        string cleanedStem = cleanAppName(baseStem);

        bool isAlreadyInstalled = false;

        // So khớp với danh sách phần mềm đã cài trong Registry
        for (const auto &inst : installed) {
            if (!cleanedProd.empty() && cleanedProd.length() >= 3 && inst.find(cleanedProd) != string::npos) {
                isAlreadyInstalled = true;
                break;
            }
            if (!cleanedStem.empty() && cleanedStem.length() >= 3 && inst.find(cleanedStem) != string::npos) {
                isAlreadyInstalled = true;
                break;
            }
        }

        // Logic xử lý (Theo đúng yêu cầu của người dùng):
        // - Nếu đã cài đặt -> Xóa sạch file cài đặt thừa này!
        // - Nếu chưa cài đặt nhưng là file lặp (dạng file (1).exe, file (2).exe) -> Xóa bản sao lặp, giữ lại file gốc!
        if (isAlreadyInstalled) {
            if (fs::remove(p, ec)) {
                freedBytes += sz;
                deletedExeCount++;
            }
        } else if (isDuplicate) {
            // Kiểm tra xem file gốc có tồn tại không: vd Setup.exe bên cạnh Setup (1).exe
            fs::path originalFile = p.parent_path() / (baseStem + ext);
            if (fs::exists(originalFile, ec)) {
                if (fs::remove(p, ec)) {
                    freedBytes += sz;
                    deletedDuplicateCount++;
                }
            }
        }
    }

    cout << "     ├── [✓] Đã xóa " << deletedExeCount << " file cài đặt (.exe/.msi) đã hoàn tất cài đặt\n";
    cout << "     ├── [✓] Đã dọn " << deletedDuplicateCount << " file cài đặt tải trùng lặp (1), (2)\n";
    if (deletedCorruptCount > 0) {
        cout << "     ├── [✓] Đã dọn " << deletedCorruptCount << " file tải dở dang bị kẹt (.crdownload)\n";
    }

    return freedBytes;
}

// ----------------------------------------------------------------------------------
// DỌN CACHE DEV NÂNG CAO
// ----------------------------------------------------------------------------------
void DiskCleaner::cleanDevCaches(bool interactive) {
    char *localAppData = std::getenv("LOCALAPPDATA");
    char *appData = std::getenv("APPDATA");
    char *userProfile = std::getenv("USERPROFILE");

    string baseLocal = localAppData ? string(localAppData) : "";
    string baseApp   = appData ? string(appData) : "";
    string baseUser  = userProfile ? string(userProfile) : "";

    std::vector<fs::path> scanRoots;
    fs::path currentRoot = fs::current_path();
    if (currentRoot.filename() == "bin" && fs::exists(currentRoot.parent_path())) {
        currentRoot = currentRoot.parent_path();
    }
    scanRoots.push_back(currentRoot);

    DWORD driveMask = GetLogicalDrives();
    for (char c = 'C'; c <= 'Z'; ++c) {
        if (driveMask & (1 << (c - 'A'))) {
            string rootDrive = string(1, c) + ":\\";
            if (GetDriveTypeA(rootDrive.c_str()) == DRIVE_FIXED) {
                static const vector<string> commonDevFolders = {
                    "Code", "Projects", "Source", "Repos", "Dev", "Web", "Workspace"
                };
                for (const auto &df : commonDevFolders) {
                    fs::path devPath = rootDrive + df;
                    std::error_code ec;
                    if (fs::exists(devPath, ec) && fs::is_directory(devPath, ec)) {
                        bool duplicate = false;
                        for (const auto &sr : scanRoots) {
                            if (fs::equivalent(sr, devPath, ec)) { duplicate = true; break; }
                        }
                        if (!duplicate) scanRoots.push_back(devPath);
                    }
                }
            }
        }
    }

    if (!baseUser.empty()) {
        static const vector<string> userDevFolders = {
            "Desktop", "Documents", "Projects", "source\\repos"
        };
        for (const auto &uf : userDevFolders) {
            fs::path uPath = fs::path(baseUser) / uf;
            std::error_code ec;
            if (fs::exists(uPath, ec) && fs::is_directory(uPath, ec)) {
                bool duplicate = false;
                for (const auto &sr : scanRoots) {
                    if (fs::equivalent(sr, uPath, ec)) { duplicate = true; break; }
                }
                if (!duplicate) scanRoots.push_back(uPath);
            }
        }
    }

    // Python
    bool hasPython = SystemCore::runRawCommand("where python >nul 2>nul") || 
                     SystemCore::runRawCommand("where py >nul 2>nul") ||
                     (!baseLocal.empty() && fs::exists(baseLocal + "\\pip\\cache"));

    if (hasPython) {
        if (!baseLocal.empty()) wipeFolderContents(baseLocal + "\\pip\\cache");
        long long pyFreed = 0;
        for (const auto &sr : scanRoots) {
            cleanDirectoryArtifacts(sr, 
                {"__pycache__", ".pytest_cache", ".mypy_cache", ".ruff_cache", ".tox"}, 
                {".pyc", ".pyo"}, 
                pyFreed);
        }
    }

    // Node.js & npm
    if (!baseLocal.empty()) {
        wipeFolderContents(baseLocal + "\\npm-cache");
        wipeFolderContents(baseLocal + "\\pnpm\\cache");
    }
    if (!baseApp.empty()) {
        wipeFolderContents(baseApp + "\\npm-cache");
    }
    if (!baseUser.empty()) {
        wipeFolderContents(baseUser + "\\.yarn\\cache");
    }

    // Java Gradle & Maven
    if (!baseUser.empty()) {
        wipeFolderContents(baseUser + "\\.gradle\\caches");
        wipeFolderContents(baseUser + "\\.gradle\\daemon");
        wipeFolderContents(baseUser + "\\.m2\\repository");
    }

    // VS Code, Cursor, NuGet, Rust, Go
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
}

void DiskCleaner::clearBrowserCache() {
    char *localAppData = std::getenv("LOCALAPPDATA");
    char *appData = std::getenv("APPDATA");
    if (!localAppData && !appData) return;

    string baseLocal = localAppData ? string(localAppData) : "";
    string baseApp   = appData ? string(appData) : "";

    vector<string> cacheTargets = {
        baseLocal + "\\Google\\Chrome\\User Data\\Default\\Cache",
        baseLocal + "\\Google\\Chrome\\User Data\\Default\\Code Cache",
        baseLocal + "\\Microsoft\\Edge\\User Data\\Default\\Cache",
        baseLocal + "\\Microsoft\\Edge\\User Data\\Default\\Code Cache",
        baseLocal + "\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Cache",
        baseLocal + "\\CocCoc\\Browser\\User Data\\Default\\Cache",
        baseApp + "\\Opera Software\\Opera Stable\\Cache"
    };

    for (const auto &path : cacheTargets) {
        wipeFolderContents(path);
    }
}

// ----------------------------------------------------------------------------------
// ĐIỀU PHỐI THỰC THI DỌN RÁC
// ----------------------------------------------------------------------------------
void DiskCleaner::runCleanChoice(int choice) {
    if (choice < 1 || choice > 7) return;

    sc.cls();
    cout << "\n\n";
    long long totalFreed = 0;

    if (choice == 1 || choice == 6 || choice == 7) {
        cout << " [*] Nhiệm vụ 1: Đang dọn rác Bề mặt & Cache người dùng (Temp, WER, CrashDumps, DNS)...\n";
        long long f1 = cleanSurfaceAndUserTemp();
        totalFreed += f1;
        cout << "     └── [✓] " << (f1 > 0 ? ("Giải phóng " + SystemCore::formatSize(f1)) : "Đã sạch sẽ từ trước") << "\n\n";
    }

    if (choice == 2 || choice == 6 || choice == 7) {
        cout << " [*] Nhiệm vụ 2: Đang dọn rác Trình duyệt & Ứng dụng (Chrome, Edge, Discord, Telegram)...\n";
        long long f2 = cleanBrowserAndAppCache();
        totalFreed += f2;
        cout << "     └── [✓] " << (f2 > 0 ? ("Giải phóng " + SystemCore::formatSize(f2)) : "Đã sạch sẽ từ trước") << "\n\n";
    }

    if (choice == 3 || choice == 6 || choice == 7) {
        cout << " [*] Nhiệm vụ 3: Đang dọn dẹp Chuyên sâu & Tồn dư cập nhật (DISM, Windows.old, EventLogs)...\n";
        long long f3 = cleanDeepSystemAndUpdates();
        totalFreed += f3;
        cout << "     └── [✓] " << (f3 > 0 ? ("Giải phóng " + SystemCore::formatSize(f3)) : "Đã sạch sẽ từ trước") << "\n\n";
    }

    if (choice == 4 || choice == 7) {
        cout << " [*] Nhiệm vụ 4: Đang dọn rác Môi trường lập trình (node_modules, Pip, Gradle, VS Code)...\n";
        long long f4 = cleanDevArtifactsAndCaches();
        totalFreed += f4;
        cout << "     └── [✓] " << (f4 > 0 ? ("Giải phóng " + SystemCore::formatSize(f4)) : "Đã sạch sẽ từ trước") << "\n\n";
    }

    if (choice == 5 || choice == 6 || choice == 7) {
        cout << " [*] Nhiệm vụ 5: Đang quét & dọn dẹp thư mục Downloads (Exe đã cài đặt, Exe trùng lặp)...\n";
        long long f5 = cleanDownloadsExesAndDuplicates();
        totalFreed += f5;
        cout << "     └── [✓] " << (f5 > 0 ? ("Giải phóng " + SystemCore::formatSize(f5)) : "Đã sạch sẽ từ trước") << "\n\n";
    }

    cout << "\n\n";
    if (totalFreed > 0) {
        cout << " [✓] TỔNG DUNG LƯỢNG ĐÃ GIẢI PHÓNG: \x1b[92m" << SystemCore::formatSize(totalFreed) << "\x1b[0m\n\n";
    } else {
        cout << " [✓] Hệ thống đã rất sạch sẽ.\n\n";
    }
    sc.waitEnter();
}
