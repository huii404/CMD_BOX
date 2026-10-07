#include "SystemCore.h"
#include "ProcessRunner.h"
#include "FileSafety.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <limits>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
namespace fs = std::filesystem;

std::string SystemCore::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Constructor / Destructor
SystemCore::SystemCore() {
    hJob = CreateJobObjectA(NULL, NULL);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli{};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
}

SystemCore::~SystemCore() {
    if (hJob) CloseHandle(hJob);
}

// Tiện ích cơ bản
void SystemCore::cls() {
    std::cout << std::flush;
    system("cls");
    std::cout << "\x1b[3J\x1b[H" << std::flush;
}

std::string SystemCore::getTime(bool includeDate) {
    time_t now = time(nullptr);
    tm t;
    localtime_s(&t, &now);
    std::stringstream ss;
    if (includeDate) {
        ss << "[" << std::setfill('0') << std::setw(2) << t.tm_mday << "/" 
           << std::setw(2) << t.tm_mon + 1 << "/" << t.tm_year + 1900 << " ";
    } else {
        ss << "[";
    }
    ss << std::setfill('0') << std::setw(2) << t.tm_hour << ":" 
       << std::setw(2) << t.tm_min << ":" 
       << std::setw(2) << t.tm_sec << "]";
    return ss.str();
}

std::string SystemCore::formatSize(long long b) {
    if (b < 0) b = 0;
    static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    double sz = static_cast<double>(b);
    int i = 0;
    while (sz >= 1024.0 && i < 5) {
        sz /= 1024.0;
        i++;
    }
    char buf[32];
    if (i == 0) {
        sprintf_s(buf, sizeof(buf), "%.0f %s", sz, units[i]);
    } else {
        sprintf_s(buf, sizeof(buf), "%.2f %s", sz, units[i]);
    }
    return std::string(buf);
}

bool SystemCore::runRawCommand(const std::string& command) { return ProcessRunner::run(command); }

std::vector<std::string> SystemCore::parsePaths(const std::string& rawInput) {
    std::string str = trim(rawInput);
    if (str.empty() || str == "0") return {};

    std::vector<std::string> paths;
    paths.reserve(8);

    std::string current;
    current.reserve(260);
    bool inQuotes = false;

    auto addPathIfValid = [&](const std::string& rawP) {
        std::string p = trim(rawP);
        while (!p.empty() && (p.front() == ',' || p.front() == ';')) p = trim(p.substr(1));
        while (!p.empty() && (p.back() == ',' || p.back() == ';')) p = trim(p.substr(0, p.length() - 1));
        if (!p.empty()) {
            std::error_code ec;
            if (fs::exists(fs::u8path(p), ec)) paths.push_back(p);
            else std::cout << "    Không tìm thấy: " << p << "\n";
        }
    };

    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        if (c == '"') {
            inQuotes = !inQuotes;
            continue;
        }

        // Tách đường dẫn liền nhau khi kéo thả nhiều file (vd: C:\a.mp4D:\b.mp4)
        if (!inQuotes && i + 2 < str.length()) {
            char curr = str[i];
            char next = str[i + 1];
            char next2 = str[i + 2];
            bool isDriveLetter = ((curr >= 'A' && curr <= 'Z') || (curr >= 'a' && curr <= 'z'));
            if (isDriveLetter && next == ':' && (next2 == '\\' || next2 == '/')) {
                if (!current.empty() && current.back() != ' ' && current.back() != '\t') {
                    addPathIfValid(current);
                    current.clear();
                }
            }
        }

        // Hỗ trợ phân tách bằng khoảng trắng, dấu phẩy ',' hoặc chấm phẩy ';'
        if (!inQuotes && (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',' || c == ';')) {
            if (!current.empty()) {
                addPathIfValid(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        addPathIfValid(current);
    }

    return paths;
}

std::string SystemCore::urlDecode(const std::string& str) {
    std::string decoded;
    decoded.reserve(str.length());
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%') {
            if (i + 2 < str.length() && 
                isxdigit(static_cast<unsigned char>(str[i + 1])) && 
                isxdigit(static_cast<unsigned char>(str[i + 2]))) {
                int value = 0;
                if (sscanf(str.substr(i + 1, 2).c_str(), "%x", &value) == 1 && value >= 0 && value <= 255) {
                    decoded += static_cast<char>(static_cast<unsigned char>(value));
                } else {
                    decoded += '%';
                }
                i += 2;
            } else {
                decoded += '%';
            }
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}

bool SystemCore::runBatchAsAdmin(const std::string& batContent, const std::string& description) {
    (void)description;
    FileSafety::LockedScript script("@echo off\r\nchcp 65001 >nul\r\n" + batContent + "\r\nexit /b %ERRORLEVEL%\r\n", L".bat");
    if (!script) return false;
    return runAdmin("call \"" + script.path().u8string() + "\"", true);
}

bool SystemCore::runEmbeddedBatch(const std::string& batContent,
                                  const std::string& arguments,
                                  bool requireAdmin) {
    FileSafety::LockedScript script(batContent, L".bat");
    if (!script) return false;
    std::string invocation = "call \"" + script.path().u8string() + "\"";
    if (!arguments.empty()) invocation += " " + arguments;
    return requireAdmin ? runAdmin(invocation, true) : runRawCommand("cmd.exe /d /c " + invocation);
}


void SystemCore::waitEnter() {
    std::cout << "\nEnter để tiếp tục";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

bool SystemCore::confirm(const std::string& prompt) {
    std::cout << prompt;
    std::string choice;
    std::getline(std::cin, choice);
    choice = trim(choice);
    return (choice == "y" || choice == "Y");
}

int SystemCore::readInt(const std::string &prompt, int defaultValue) {
    std::string line;
    while (true) {
        if (!prompt.empty()) {
            std::cout << prompt;
        }
        if (!std::getline(std::cin, line)) {
            if (std::cin.eof() || std::cin.bad()) {
                return (defaultValue != std::numeric_limits<int>::min()) ? defaultValue : 0;
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        
        line = SystemCore::trim(line);
        if (line.empty()) {
            if (defaultValue != std::numeric_limits<int>::min()) {
                return defaultValue;
            }
            continue;
        }
        
        try {
            size_t consumed = 0;
            int value = std::stoi(line, &consumed);
            if (consumed != line.size()) throw std::invalid_argument("trailing input");
            return value;
        } catch (...) {
            std::cout << "Số không hợp lệ.\n";
        }
    }
}

// Thực thi lệnh hệ thống
void SystemCore::runCMD(const std::string &cmd) { ProcessRunner::run("cmd.exe /d /c " + cmd, false); }

bool SystemCore::isElevated() {
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

std::string SystemCore::getDeviceStatus() {
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        bool hasBattery = !(sps.BatteryFlag & 128) && (sps.BatteryLifePercent != 255);
        if (hasBattery) {
            int percent = static_cast<int>(sps.BatteryLifePercent);
            std::string status = "Laptop "+ std::to_string(percent) + "%";
            if (sps.ACLineStatus == 1) {
                status += "⚡";
            } else if (sps.ACLineStatus == 0) {
                status += "🔌";
            }
            return status;
        } else {
            return "PC";
        }
    }
    return "PC";
}

bool SystemCore::checkEmergencyStop() {
    if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) || (GetAsyncKeyState(VK_F6) & 0x8000)) {
        GetAsyncKeyState(VK_ESCAPE);
        GetAsyncKeyState(VK_F6);
        return true;
    }
    return false;
}

bool SystemCore::runAdmin(const std::string &cmd, bool silent) {
    if (isElevated()) {
        return SystemCore::runRawCommand("cmd.exe /c " + cmd);
    }

    if (!silent) {
        std::cout << "Chạy quyền Admin cho lệnh [" << cmd << "] (Y/N): ";
        std::string answer;
        std::getline(std::cin, answer);
        answer = trim(answer);
        if (answer != "y" && answer != "Y") {
            std::cout << "Bỏ qua lệnh\n";
            return false;
        }
    }
    
    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, cmd.c_str(), -1, NULL, 0);
    if (!wlen) return false;
    std::wstring wCmd(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, cmd.c_str(), -1, &wCmd[0], wlen);
    std::wstring params = L"/d /c " + wCmd;

    SHELLEXECUTEINFOW sei{};
    sei.cbSize    = sizeof(sei);
    sei.lpVerb    = L"runas";
    sei.lpFile    = L"cmd.exe";
    sei.lpParameters = params.c_str();
    sei.nShow     = SW_SHOWNORMAL;
    sei.fMask     = SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteExW(&sei)) {
        std::cout << "Đang chạy lệnh với quyền Admin\n";
        bool success = false;
        if (sei.hProcess) {
            if (WaitForSingleObject(sei.hProcess, 45 * 60 * 1000) != WAIT_OBJECT_0) {
                std::cout << "Lệnh Admin quá thời gian; chưa xác nhận hoàn tất.\n";
                CloseHandle(sei.hProcess); return false;
            }
            DWORD exitCode = 0;
            if (GetExitCodeProcess(sei.hProcess, &exitCode)) {
                success = (exitCode == 0);
            }
            CloseHandle(sei.hProcess);
        }
        return success;
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) {
            std::cout << "Người dùng từ chối cấp quyền Admin\n";
        } else {
            std::cout << "Không thể lấy quyền Admin (Mã lỗi: " << err << ")\n";
        }
        return false;
    }
}

// Giả lập bàn phím & chuột
void SystemCore::leftClick() {
    INPUT input[2] = {};
    input[0].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    input[1].type = INPUT_MOUSE;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, input, sizeof(INPUT));
}

void SystemCore::setClipboard(const std::string &text) {
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();

    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
    if (wlen > 0) {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(wchar_t));
        if (hMem) {
            wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
            if (pMem) {
                MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, pMem, wlen);
                GlobalUnlock(hMem);
                if (!SetClipboardData(CF_UNICODETEXT, hMem)) {
                    GlobalFree(hMem);
                }
            } else {
                GlobalFree(hMem);
            }
        }
    }
    CloseClipboard();
}

void SystemCore::pressCtrlV() {
    INPUT inputs[4] = {};
    for (int i = 0; i < 4; i++) inputs[i].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].ki.wVk = 'V';
    inputs[2].ki.wVk = 'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
}

void SystemCore::pressEnter() {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_RETURN;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_RETURN;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
}

