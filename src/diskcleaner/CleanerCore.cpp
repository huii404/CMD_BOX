#include "ProcessRunner.h"
#include "FileSafety.h"
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
}

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

void CleanerCore::cls() {
    std::cout << "\033[2J\033[1;1H" << std::flush;
}

void CleanerCore::waitEnter() {
    std::cout << C_YELLOW << "\nNhấn [Enter] để tiếp tục..." << C_RESET;
    std::string dummy;
    std::getline(std::cin, dummy);
}

std::string CleanerCore::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string CleanerCore::toLower(const std::string& str) {
    std::string res = str;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return res;
}

std::string CleanerCore::formatSize(long long bytes) {
    if (bytes <= 0) return "0 B";
    static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    double sz = static_cast<double>(bytes);
    int unitIdx = 0;
    while (sz >= 1024.0 && unitIdx < 5) {
        sz /= 1024.0;
        unitIdx++;
    }
    std::ostringstream ss;
    if (unitIdx == 0) {
        ss << static_cast<long long>(sz) << " " << units[unitIdx];
    } else {
        ss << std::fixed << std::setprecision(2) << sz << " " << units[unitIdx];
    }
    return ss.str();
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
    std::error_code ec;
    const fs::path absolute = fs::absolute(p, ec).lexically_normal();
    if (ec || absolute.empty() || absolute == absolute.root_path()) return true;
    const fs::path normalized = fs::weakly_canonical(absolute, ec);
    if (ec) return true; // Cannot establish a safe path.
    const auto drive = fs::path(getSystemDriveRoot());
    wchar_t windowsBuffer[MAX_PATH]{};
    UINT count = GetWindowsDirectoryW(windowsBuffer, MAX_PATH);
    if (!count || count >= MAX_PATH) return true;
    const fs::path windows(windowsBuffer);
    const std::vector<fs::path> caches = {
        windows / L"Temp", windows / L"Prefetch", windows / L"SoftwareDistribution" / L"Download",
        windows / L"LiveKernelReports", windows / L"Minidump", windows / L"Logs" / L"CBS",
        windows / L"Logs" / L"DISM", windows / L"Panther",
        drive / L"ProgramData" / L"Microsoft" / L"Windows" / L"WER" / L"Temp",
        drive / L"ProgramData" / L"Microsoft" / L"Windows" / L"DeliveryOptimization" / L"Cache"
    };
    bool cache = false;
    for (const auto& allowed : caches) if (FileSafety::within(normalized, allowed)) cache = true;
    if (FileSafety::within(normalized, windows) && !cache &&
        normalized != windows / L"MEMORY.DMP" && normalized != windows / L"WindowsUpdate.log") return true;
    for (const auto& name : {L"Program Files", L"Program Files (x86)", L"Recovery",
                            L"System Volume Information", L"Windows.old", L"$WINDOWS.~BT", L"$WINDOWS.~WS"})
        if (FileSafety::within(normalized, drive / name)) return true;
    if (FileSafety::within(normalized, drive / L"ProgramData") && !cache) return true;
    if (normalized == drive / L"Users") return true;
    if (GetFileAttributesW((absolute / L".git").c_str()) != INVALID_FILE_ATTRIBUTES) return true;
    // Never operate on a repository, protected subtree, or a complete user profile.
    for (fs::path parent = absolute; !parent.empty();) {
        std::wstring name = parent.filename().wstring();
        for (auto& c : name) c = std::towlower(c);
        if (name == L".git" || name == L".github") return true;
        auto marker = parent / L".cmd-box-protect";
        DWORD attrs = GetFileAttributesW(marker.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES) return true;
        if (GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND) return true;
        const auto next = parent.parent_path();
        if (next == parent) break;
        parent = next;
    }
    std::wstring profile;
    DWORD length = GetEnvironmentVariableW(L"USERPROFILE", nullptr, 0);
    if (length) {
        profile.resize(length);
        DWORD read = GetEnvironmentVariableW(L"USERPROFILE", profile.data(), length);
        if (read && read < length) {
            profile.resize(read);
            const fs::path home(profile);
            for (const auto& root : {home, home / L"Desktop", home / L"Documents", home / L"Downloads", home / L"AppData"})
                if (FileSafety::within(normalized, root) && FileSafety::within(root, normalized)) return true;
        }
    }
    return false;
}

bool CleanerCore::isElevated() {
    bool elevated = false;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize)) {
            elevated = (elevation.TokenIsElevated != 0);
        }
        CloseHandle(hToken);
    }
    return elevated;
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
        FileSafety::AncestorLocks parents;
        if (!parents.acquire(dirPath) || isCriticalPath(dirPath)) { stats.itemsSkipped++; return false; }
        HANDLE directory = CreateFileW(dirPath.c_str(), FILE_READ_ATTRIBUTES,
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
            (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) { stats.itemsSkipped++; return false; }
        std::error_code ec;
        auto it = fs::directory_iterator(dirPath, ec);
        if (ec) { recordCleanupFailure(ec, stats); return false; }
        bool success = true;
        while (it != fs::directory_iterator()) {
            const auto path = it->path();
            DWORD attributes = GetFileAttributesW(path.c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                stats.itemsSkipped++; success = false;
            } else if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!forceDeleteFolder(path, dryRun, stats)) success = false;
            } else if (!safeDeleteFile(path, dryRun, stats)) success = false;
            it.increment(ec);
            if (ec) { recordCleanupFailure(ec, stats); return false; }
        }
        return success;
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

bool CleanerCore::emptyRecycleBin(bool dryRun, CleanStats& stats) {
    SHQUERYRBINFO rbi{};
    rbi.cbSize = sizeof(rbi);
    if (SUCCEEDED(SHQueryRecycleBinW(NULL, &rbi))) {
        if (dryRun) {
            stats.bytesFreed += rbi.i64Size;
            stats.filesDeleted += static_cast<int>(rbi.i64NumItems);
            return true;
        }
        HRESULT hr = SHEmptyRecycleBinW(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
        if (SUCCEEDED(hr)) {
            stats.bytesFreed += rbi.i64Size;
            stats.filesDeleted += static_cast<int>(rbi.i64NumItems);
            return true;
        }
    }
    // Fallback qua powershell nếu API lỗi
    if (!dryRun && runCommand("powershell -NoProfile -Command \"Clear-RecycleBin -Force -ErrorAction Stop\"", true)) {
        return true;
    }
    stats.errorsCount++;
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

bool CleanerCore::takeOwnershipAndGrantAdmin(const fs::path& targetPath) {
    (void)targetPath;
    return false; // No recursive ACL takeover in a cache cleaner.
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
