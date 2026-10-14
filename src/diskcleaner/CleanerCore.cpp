#include "ProcessRunner.h"
#include "FileSafety.h"
#include "SystemCore.h"
#include <climits>
#include "CleanerCore.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <cwctype>
#include <cstring>
#include <shellapi.h>
#include <shlobj.h>

#include <cwchar>

const char* CleanerCore::C_RESET   = "\033[0m";
const char* CleanerCore::C_BOLD    = "\033[1m";
const char* CleanerCore::C_DIM     = "\033[2m";
const char* CleanerCore::C_RED     = "\033[91m";
const char* CleanerCore::C_GREEN   = "\033[92m";
const char* CleanerCore::C_YELLOW  = "\033[93m";
const char* CleanerCore::C_CYAN    = "\033[96m";

namespace {
bool isExpectedCleanupFailure(const std::error_code& ec) {
    return ec == std::errc::permission_denied ||
           ec.value() == ERROR_ACCESS_DENIED ||
           ec.value() == ERROR_SHARING_VIOLATION ||
           ec.value() == ERROR_LOCK_VIOLATION ||
           ec.value() == ERROR_CURRENT_DIRECTORY;
}

void recordCleanupFailure(const std::error_code& ec, CleanStats& stats) {
    if (isExpectedCleanupFailure(ec)) stats.itemsSkipped++;
    else stats.errorsCount++;
}

struct CachedEnvPaths {
    fs::path drive;
    fs::path windows;
    std::vector<fs::path> allowedCaches;
    std::vector<fs::path> protectedRoots;
    std::vector<fs::path> userRoots;
    bool valid = false;
};

// Cache đường dẫn hệ thống 1 lần duy nhất thay vì gọi Win32 API hàng trăm nghìn lần
const CachedEnvPaths& getEnvPaths() {
    static const CachedEnvPaths env = [] {
        CachedEnvPaths e;
        wchar_t winBuf[MAX_PATH]{};
        UINT count = GetWindowsDirectoryW(winBuf, MAX_PATH);
        if (!count || count >= MAX_PATH) return e;
        e.windows = fs::path(winBuf).lexically_normal();
        e.drive = e.windows.root_path();

        e.allowedCaches = {
            e.windows / L"Temp", e.windows / L"Prefetch",
            e.windows / L"SoftwareDistribution" / L"Download",
            e.windows / L"LiveKernelReports", e.windows / L"Minidump",
            e.windows / L"Logs" / L"CBS", e.windows / L"Logs" / L"DISM",
            e.windows / L"Panther",
            e.drive / L"ProgramData" / L"Microsoft" / L"Windows" / L"WER" / L"Temp",
            e.drive / L"ProgramData" / L"Microsoft" / L"Windows" / L"DeliveryOptimization" / L"Cache"
        };
        for (const auto& name : {L"Program Files", L"Program Files (x86)", L"Recovery",
                                 L"System Volume Information", L"Windows.old", L"$WINDOWS.~BT", L"$WINDOWS.~WS"}) {
            e.protectedRoots.push_back(e.drive / name);
        }
        wchar_t profBuf[MAX_PATH]{};
        DWORD len = GetEnvironmentVariableW(L"USERPROFILE", profBuf, MAX_PATH);
        if (len > 0 && len < MAX_PATH) {
            fs::path home(profBuf);
            e.userRoots = {home, home / L"Desktop", home / L"Documents", home / L"Downloads", home / L"AppData"};
        }
        e.valid = true;
        return e;
    }();
    return env;
}

// Kiem tra nhanh ten bao ve tai cho ma khong quet nguoc len o dia goc
inline bool isProtectedName(const wchar_t* name) {
    return _wcsicmp(name, L".git") == 0 ||
           _wcsicmp(name, L".github") == 0 ||
           _wcsicmp(name, L".cmd-box-protect") == 0;
}

// Xóa file siêu tốc khi thư mục cha ĐÃ ĐƯỢC KHÓA & XÁC THỰC AN TOÀN
bool fastDeleteFileInLockedDir(const std::wstring& fullPath, const WIN32_FIND_DATAW& fd,
                               bool dryRun, CleanStats& stats) {
    if ((fd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ||
        isProtectedName(fd.cFileName)) {
        stats.itemsSkipped++;
        return false;
    }
    if (!dryRun) {
        HANDLE h = CreateFileW(fullPath.c_str(), DELETE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               nullptr, OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) return true;
            stats.itemsSkipped++;
            return false;
        }
        FILE_DISPOSITION_INFO remove{TRUE};
        bool ok = SetFileInformationByHandle(h, FileDispositionInfo, &remove, sizeof(remove)) != FALSE;
        CloseHandle(h);
        if (!ok) {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                SetFileAttributesW(fullPath.c_str(), fd.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
                HANDLE h2 = CreateFileW(fullPath.c_str(), DELETE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        nullptr, OPEN_EXISTING,
                                        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
                if (h2 != INVALID_HANDLE_VALUE) {
                    ok = SetFileInformationByHandle(h2, FileDispositionInfo, &remove, sizeof(remove)) != FALSE;
                    CloseHandle(h2);
                }
            }
        }
        if (!ok) {
            stats.itemsSkipped++;
            return false;
        }
    }
    uintmax_t size = (uintmax_t(fd.nFileSizeHigh) << 32) | fd.nFileSizeLow;
    stats.bytesFreed += static_cast<long long>((std::min)(size, uintmax_t(LLONG_MAX - stats.bytesFreed)));
    stats.filesDeleted++;
    return true;
}

// Duyệt và xóa đệ quy bên trong thư mục ĐÃ ĐƯỢC KHÓA bằng FindFirstFileExW (LARGE_FETCH)
bool fastWipeLockedSubtree(const std::wstring& dirPath, bool deleteSelf,
                           bool dryRun, CleanStats& stats) {
    if (GetFileAttributesW((dirPath + L"\\.git").c_str()) != INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesW((dirPath + L"\\.cmd-box-protect").c_str()) != INVALID_FILE_ATTRIBUTES) {
        stats.itemsSkipped++;
        return false;
    }

    std::wstring searchPattern = dirPath + L"\\*";
    WIN32_FIND_DATAW fd{};
    HANDLE hFind = FindFirstFileExW(searchPattern.c_str(), FindExInfoBasic, &fd,
                                    FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) return true;
        stats.itemsSkipped++;
        return false;
    }

    bool allSuccess = true;
    do {
        if (fd.cFileName[0] == L'.' &&
            (fd.cFileName[1] == L'\0' || (fd.cFileName[1] == L'.' && fd.cFileName[2] == L'\0'))) {
            continue;
        }
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) || isProtectedName(fd.cFileName)) {
            stats.itemsSkipped++;
            allSuccess = false;
            continue;
        }

        std::wstring childPath = dirPath + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!fastWipeLockedSubtree(childPath, true, dryRun, stats)) {
                allSuccess = false;
            }
        } else {
            if (!fastDeleteFileInLockedDir(childPath, fd, dryRun, stats)) {
                allSuccess = false;
            }
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    if (deleteSelf && allSuccess) {
        if (!dryRun) {
            if (!RemoveDirectoryW(dirPath.c_str())) {
                DWORD err = GetLastError();
                if (err == ERROR_ACCESS_DENIED) {
                    SetFileAttributesW(dirPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                    if (!RemoveDirectoryW(dirPath.c_str())) {
                        err = GetLastError();
                    } else {
                        err = ERROR_SUCCESS;
                    }
                }
                if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
                    stats.itemsSkipped++;
                    return false;
                }
            }
        }
        stats.dirsDeleted++;
    }
    return allSuccess;
}
} // namespace

void CleanerCore::initConsole() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
}

std::string CleanerCore::toLower(const std::string& str) {
    std::string res = str;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return res;
}

std::string CleanerCore::getSystemDriveRoot() {
    wchar_t windows[MAX_PATH]{};
    UINT count = GetWindowsDirectoryW(windows, MAX_PATH);
    if (!count || count >= MAX_PATH) return "C:\\";
    return fs::path(windows).root_path().string();
}

long long CleanerCore::getAvailableDiskSpace(const std::string& drivePath) {
    std::string path = drivePath.empty() ? getSystemDriveRoot() : drivePath;
    std::error_code ec;
    auto spaceInfo = fs::space(path, ec);
    if (ec) return 0;
    return static_cast<long long>(spaceInfo.available);
}

bool CleanerCore::isCriticalPath(const fs::path& p) {
    const auto& env = getEnvPaths();
    if (!env.valid) return true;

    std::error_code ec;
    const fs::path absolute = fs::absolute(p, ec).lexically_normal();
    if (ec || absolute.empty() || absolute == absolute.root_path()) return true;
    const fs::path normalized = fs::weakly_canonical(absolute, ec);
    if (ec) return true;

    bool cache = false;
    for (const auto& allowed : env.allowedCaches) {
        if (FileSafety::within(normalized, allowed)) { cache = true; break; }
    }
    if (FileSafety::within(normalized, env.windows) && !cache &&
        normalized != env.windows / L"MEMORY.DMP" && normalized != env.windows / L"WindowsUpdate.log") {
        return true;
    }
    for (const auto& prot : env.protectedRoots) {
        if (FileSafety::within(normalized, prot)) return true;
    }
    if (FileSafety::within(normalized, env.drive / L"ProgramData") && !cache) return true;
    if (normalized == env.drive / L"Users") return true;
    if (GetFileAttributesW((absolute / L".git").c_str()) != INVALID_FILE_ATTRIBUTES) return true;

    for (fs::path parent = absolute; !parent.empty();) {
        std::wstring name = parent.filename().wstring();
        if (isProtectedName(name.c_str())) return true;
        auto marker = parent / L".cmd-box-protect";
        DWORD attrs = GetFileAttributesW(marker.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES) return true;
        if (GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND) return true;
        const auto next = parent.parent_path();
        if (next == parent) break;
        parent = next;
    }

    for (const auto& root : env.userRoots) {
        if (FileSafety::within(normalized, root) && FileSafety::within(root, normalized)) return true;
    }
    return false;
}



bool CleanerCore::restartAsAdmin(const std::string& args) {
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    SHELLEXECUTEINFOA sei{};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = "runas";
    sei.lpFile = exePath;
    sei.lpParameters = args.c_str();
    sei.nShow = SW_NORMAL;

    if (ShellExecuteExA(&sei)) {
        return true;
    }
    return false;
}

bool CleanerCore::runCommand(const std::string& cmd, bool hideWindow, DWORD timeoutMs) { return ProcessRunner::run("cmd.exe /d /c " + cmd, hideWindow, timeoutMs); }

bool CleanerCore::wipeFolderContents(const fs::path& dirPath, bool dryRun, CleanStats& stats) {
    try {
        std::error_code ec;
        fs::path absPath = fs::absolute(dirPath, ec).lexically_normal();
        if (ec) { stats.errorsCount++; return false; }

        // Kiểm tra tồn tại cực nhanh trước khi khóa AncestorLocks
        DWORD rootAttr = GetFileAttributesW(absPath.c_str());
        if (rootAttr == INVALID_FILE_ATTRIBUTES) {
            DWORD err = GetLastError();
            if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) return true;
            stats.itemsSkipped++; return false;
        }
        if (!(rootAttr & FILE_ATTRIBUTE_DIRECTORY) || (rootAttr & FILE_ATTRIBUTE_REPARSE_POINT)) {
            stats.itemsSkipped++; return false;
        }

        // Khóa toàn bộ nhánh cha 1 LẦN DUY NHẤT cho cả cây thư mục
        FileSafety::AncestorLocks parents;
        if (!parents.acquire(absPath) || isCriticalPath(absPath)) {
            stats.itemsSkipped++; return false;
        }
        HANDLE directory = CreateFileW(absPath.c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (directory == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) return true;
            stats.itemsSkipped++; return false;
        }
        struct Guard { HANDLE h; ~Guard() { CloseHandle(h); } } guard{directory};
        BY_HANDLE_FILE_INFORMATION info{};
        if (!GetFileInformationByHandle(directory, &info) ||
            !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
            stats.itemsSkipped++; return false;
        }

        std::wstring cleanPath = absPath.wstring();
        while (!cleanPath.empty() && (cleanPath.back() == L'\\' || cleanPath.back() == L'/')) {
            cleanPath.pop_back();
        }
        return fastWipeLockedSubtree(cleanPath, false, dryRun, stats);
    } catch (const std::exception&) { stats.errorsCount++; return false; }
}

bool CleanerCore::forceDeleteFolder(const fs::path& dirPath, bool dryRun, CleanStats& stats) {
    try {
        if (!wipeFolderContents(dirPath, dryRun, stats)) return false;
        FileSafety::AncestorLocks parents;
        if (!parents.acquire(dirPath) || isCriticalPath(dirPath)) { stats.itemsSkipped++; return false; }
        HANDLE h = CreateFileW(dirPath.c_str(), FILE_READ_ATTRIBUTES | (dryRun ? 0 : DELETE),
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) return true;
            stats.itemsSkipped++; return false;
        }
        struct Guard { HANDLE h; ~Guard() { CloseHandle(h); } } guard{h};
        BY_HANDLE_FILE_INFORMATION info{};
        if (!GetFileInformationByHandle(h, &info) || !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) { stats.itemsSkipped++; return false; }
        if (!dryRun) {
            FILE_DISPOSITION_INFO remove{TRUE};
            if (!SetFileInformationByHandle(h, FileDispositionInfo, &remove, sizeof(remove))) {
                stats.itemsSkipped++; return false;
            }
        }
        stats.dirsDeleted++; return true;
    } catch (const std::exception&) { stats.errorsCount++; return false; }
}

bool CleanerCore::safeDeleteFile(const fs::path& filePath, bool dryRun, CleanStats& stats) {
    try {
        FileSafety::AncestorLocks parents;
        if (!parents.acquire(filePath) || isCriticalPath(filePath)) { stats.itemsSkipped++; return false; }
        HANDLE h = CreateFileW(filePath.c_str(), FILE_READ_ATTRIBUTES | (dryRun ? 0 : DELETE),
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) return true;
            stats.itemsSkipped++; return false;
        }
        struct Guard { HANDLE h; ~Guard() { CloseHandle(h); } } guard{h};
        BY_HANDLE_FILE_INFORMATION info{};
        if (!GetFileInformationByHandle(h, &info) ||
            (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))) {
            stats.itemsSkipped++; return false;
        }
        // Keep attributes unchanged: hard-linked caches can share attributes with project files.
        if (!dryRun) {
            FILE_DISPOSITION_INFO remove{TRUE};
            if (!SetFileInformationByHandle(h, FileDispositionInfo, &remove, sizeof(remove))) {
                stats.itemsSkipped++; return false;
            }
        }
        uintmax_t size = (uintmax_t(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
        stats.bytesFreed += static_cast<long long>((std::min)(size, uintmax_t(LLONG_MAX - stats.bytesFreed)));
        stats.filesDeleted++; return true;
    } catch (const std::exception&) { stats.errorsCount++; return false; }
}

bool CleanerCore::moveToRecycleBin(const fs::path& filePath, bool dryRun, CleanStats& stats) {
    FileSafety::AncestorLocks parents;
    if (!parents.acquire(filePath) || isCriticalPath(filePath)) { stats.itemsSkipped++; return false; }
    DWORD attributes = GetFileAttributesW(filePath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))) {
        stats.itemsSkipped++; return false;
    }

    std::error_code ec;
    if (!fs::exists(filePath, ec)) return true;
    uintmax_t sz = fs::file_size(filePath, ec);
    if (ec) { sz = 0; ec.clear(); }

    if (dryRun) {
        stats.bytesRecycled += sz;
        stats.filesRecycled++;
        return true;
    }

    std::wstring pathStr = filePath.wstring();
    pathStr.push_back(L'\0'); // Double null-terminated cho SHFILEOPSTRUCTW

    SHFILEOPSTRUCTW fileOp{};
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = pathStr.c_str();
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    int res = SHFileOperationW(&fileOp);
    if (res == 0 && !fileOp.fAnyOperationsAborted) {
        stats.bytesRecycled += sz;
        stats.filesRecycled++;
        return true;
    }
    if (res == ERROR_ACCESS_DENIED || res == ERROR_SHARING_VIOLATION ||
        res == ERROR_LOCK_VIOLATION) {
        stats.itemsSkipped++;
    } else {
        stats.errorsCount++;
    }
    return false;
}

bool CleanerCore::flushDns() {
    HMODULE dnsApi = LoadLibraryW(L"dnsapi.dll");
    if (dnsApi) {
        using FlushResolverCacheFn = BOOL (WINAPI*)();
        FARPROC rawFunction = GetProcAddress(dnsApi, "DnsFlushResolverCache");
        FlushResolverCacheFn flushResolverCache = nullptr;
        static_assert(sizeof(flushResolverCache) == sizeof(rawFunction));
        std::memcpy(&flushResolverCache, &rawFunction, sizeof(flushResolverCache));
        if (flushResolverCache && flushResolverCache()) {
            FreeLibrary(dnsApi);
            return true;
        }
        FreeLibrary(dnsApi);
    }
    return runCommand("ipconfig /flushdns >nul 2>&1", true);
}

bool CleanerCore::filesHaveSameContent(const fs::path& first, const fs::path& second) {
    std::error_code ec1, ec2;
    uintmax_t sz1 = fs::file_size(first, ec1);
    uintmax_t sz2 = fs::file_size(second, ec2);
    if (ec1 || ec2 || sz1 != sz2) return false;
    std::ifstream a(first, std::ios::binary);
    std::ifstream b(second, std::ios::binary);
    if (!a || !b) return false;
    std::vector<char> aBuf(64 * 1024), bBuf(64 * 1024);
    while (a && b) {
        a.read(aBuf.data(), static_cast<std::streamsize>(aBuf.size()));
        b.read(bBuf.data(), static_cast<std::streamsize>(bBuf.size()));
        if (a.gcount() != b.gcount() ||
            !std::equal(aBuf.begin(), aBuf.begin() + a.gcount(), bBuf.begin())) return false;
    }
    return a.eof() && b.eof();
}
