#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <array>
#include <string>
#include <vector>
#include <cwctype>
#include <limits>

namespace FileSafety {
namespace fs = std::filesystem;
inline std::string knownFolder(REFKNOWNFOLDERID id) {
    PWSTR buffer = nullptr;
    if (FAILED(SHGetKnownFolderPath(id, 0, nullptr, &buffer))) return {};
    std::string result = fs::path(buffer).u8string(); CoTaskMemFree(buffer); return result;
}
inline std::string windowsDirectory() {
    wchar_t buffer[MAX_PATH]{};
    UINT count = GetWindowsDirectoryW(buffer, MAX_PATH);
    return count && count < MAX_PATH ? fs::path(buffer).u8string() : std::string();
}
inline bool within(const fs::path& path, const fs::path& root) {
    auto p = path.lexically_normal(), r = root.lexically_normal();
    auto a = p.begin();
    for (auto b = r.begin(); b != r.end(); ++b, ++a) {
        if (a == p.end()) return false;
        std::wstring x = a->wstring(), y = b->wstring();
        for (auto& c : x) c = std::towlower(c);
        for (auto& c : y) c = std::towlower(c);
        if (x != y) return false;
    }
    return true;
}

// Hold every ancestor against rename/delete while the caller operates on a leaf.
// OPEN_REPARSE_POINT checks the link itself instead of opening its target.
class AncestorLocks {
    std::vector<HANDLE> handles;
public:
    AncestorLocks() = default;
    AncestorLocks(const AncestorLocks&) = delete;
    AncestorLocks& operator=(const AncestorLocks&) = delete;
    ~AncestorLocks() { for (HANDLE h : handles) CloseHandle(h); }
    bool acquire(const fs::path& leaf) {
        std::error_code ec;
        fs::path absolute = fs::absolute(leaf, ec).lexically_normal();
        if (ec || absolute.empty() || absolute.root_name().wstring().size() != 2) return false;
        fs::path part = absolute.root_path();
        auto relative = absolute.parent_path().lexically_relative(part);
        std::vector<fs::path> parents{part};
        for (const auto& component : relative) { part /= component; parents.push_back(part); }
        for (const auto& parent : parents) {
            HANDLE h = CreateFileW(parent.c_str(), FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            if (h == INVALID_HANDLE_VALUE) return false;
            handles.push_back(h);
            BY_HANDLE_FILE_INFORMATION info{};
            if (!GetFileInformationByHandle(h, &info) ||
                !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
        }
        return true;
    }
};

// CREATE_NEW refuses existing files, hard-link aliases and output=input.
// Failed writes delete the opened file by handle, never an unrelated replacement.
class ExclusiveOutput {
    AncestorLocks parents;
    HANDLE handle = INVALID_HANDLE_VALUE;
    bool good = false;
public:
    explicit ExclusiveOutput(const fs::path& path) {
        if (!parents.acquire(path)) return;
        handle = CreateFileW(path.c_str(), GENERIC_WRITE | DELETE, 0, nullptr,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        good = handle != INVALID_HANDLE_VALUE;
    }
    ExclusiveOutput(const ExclusiveOutput&) = delete;
    ExclusiveOutput& operator=(const ExclusiveOutput&) = delete;
    ~ExclusiveOutput() { discard(); }
    explicit operator bool() const { return good; }
    bool write(const char* data, size_t count) {
        if (!good) return false;
        while (count) {
            DWORD written = 0, chunk = static_cast<DWORD>((std::min)(count, size_t(65536)));
            if (!WriteFile(handle, data, chunk, &written, nullptr) || written != chunk) {
                good = false; return false;
            }
            data += written; count -= written;
        }
        return true;
    }
    void discard() {
        if (handle == INVALID_HANDLE_VALUE) return;
        FILE_DISPOSITION_INFO remove{TRUE};
        SetFileInformationByHandle(handle, FileDispositionInfo, &remove, sizeof(remove));
        CloseHandle(handle); handle = INVALID_HANDLE_VALUE; good = false;
    }
    bool commit() {
        if (!good || !FlushFileBuffers(handle)) { discard(); return false; }
        bool ok = CloseHandle(handle) != FALSE;
        handle = INVALID_HANDLE_VALUE; good = false;
        return ok;
    }
};

inline bool copyExact(std::ifstream& in, ExclusiveOutput& out, uintmax_t bytes, bool obfuscate) {
    std::array<char, 65536> buffer{};
    while (bytes) {
        size_t count = static_cast<size_t>((std::min)(bytes, uintmax_t(buffer.size())));
        in.read(buffer.data(), static_cast<std::streamsize>(count));
        if (in.gcount() != static_cast<std::streamsize>(count) || in.bad()) return false;
        if (obfuscate) for (size_t i = 0; i < count; ++i) buffer[i] ^= char(0xAA);
        if (!out.write(buffer.data(), count)) return false;
        bytes -= count;
    }
    return true;
}

class TemporaryDirectory {
    AncestorLocks parents;
    HANDLE directory = INVALID_HANDLE_VALUE;
    fs::path location;
public:
    explicit TemporaryDirectory(const fs::path& preferredRoot = {}) {
        wchar_t temp[MAX_PATH]{};
        DWORD count = GetTempPathW(MAX_PATH, temp);
        GUID id{}; wchar_t name[40]{};
        if ((preferredRoot.empty() && (!count || count >= MAX_PATH)) || FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id, name, 40)) return;
        auto candidate = (preferredRoot.empty() ? fs::path(temp) : preferredRoot) / (std::wstring(L"cmd-box-") + name);
        if (candidate.wstring().find_first_of(L"%\r\n\"!") != std::wstring::npos || !parents.acquire(candidate)) return;
        if (!CreateDirectoryW(candidate.c_str(), nullptr)) return;
        directory = CreateFileW(candidate.c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (directory == INVALID_HANDLE_VALUE) { RemoveDirectoryW(candidate.c_str()); return; }
        BY_HANDLE_FILE_INFORMATION info{};
        if (!GetFileInformationByHandle(directory, &info) || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
            CloseHandle(directory); directory = INVALID_HANDLE_VALUE; return;
        }
        location = candidate;
    }
    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
    ~TemporaryDirectory() {
        if (directory == INVALID_HANDLE_VALUE) return;
        // Only remove known immediate files; never recursively follow a newly added directory.
        std::error_code ec;
        for (fs::directory_iterator it(location, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
            DWORD attributes = GetFileAttributesW(it->path().c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
                DeleteFileW(it->path().c_str());
        }
        CloseHandle(directory); RemoveDirectoryW(location.c_str());
    }
    explicit operator bool() const { return !location.empty(); }
    const fs::path& path() const { return location; }
};

class LockedScript {
    TemporaryDirectory folder;
    HANDLE reader = INVALID_HANDLE_VALUE;
    fs::path script;
public:
    LockedScript(const std::string& text, const std::wstring& extension, const fs::path& preferredRoot = {})
        : folder(preferredRoot) {
        if (!folder || (extension != L".bat" && extension != L".ps1")) return;
        const auto candidate = folder.path() / (L"task" + extension);
        ExclusiveOutput writer(candidate);
        if (!writer || !writer.write(text.data(), text.size()) || !writer.commit()) return;
        reader = CreateFileW(candidate.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (reader == INVALID_HANDLE_VALUE) return;
        BY_HANDLE_FILE_INFORMATION info{};
        LARGE_INTEGER size{};
        bool valid = GetFileInformationByHandle(reader, &info) &&
            !(info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) &&
            GetFileSizeEx(reader, &size) && size.QuadPart >= 0 && uintmax_t(size.QuadPart) == text.size();
        std::array<char, 65536> buffer{}; size_t offset = 0;
        while (valid && offset < text.size()) {
            DWORD count = static_cast<DWORD>((std::min)(buffer.size(), text.size() - offset)), read = 0;
            valid = ReadFile(reader, buffer.data(), count, &read, nullptr) && read == count &&
                    text.compare(offset, count, buffer.data(), count) == 0;
            offset += count;
        }
        if (valid) script = candidate;
        else { CloseHandle(reader); reader = INVALID_HANDLE_VALUE; }
    }
    LockedScript(const LockedScript&) = delete;
    ~LockedScript() { if (reader != INVALID_HANDLE_VALUE) CloseHandle(reader); }
    explicit operator bool() const { return !script.empty(); }
    const fs::path& path() const { return script; }
};
}
