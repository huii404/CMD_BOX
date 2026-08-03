#include "../include/SystemCore.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <limits>
#include <intrin.h>
#include <winternl.h>

#pragma comment(lib, "ws2_32.lib")
namespace fs = std::filesystem;

std::string SystemCore::trim(const std::string& str) {
    std::string s = str;
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
        s.pop_back();
    }
    while (!s.empty() && s.front() == ' ') {
        s.erase(0, 1);
    }
    return s;
}

// ==================== CONSTRUCTOR & DESTRUCTOR ====================

SystemCore::SystemCore() {
    hJob = CreateJobObjectA(NULL, NULL);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {0};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
}

SystemCore::~SystemCore() {
    if (hJob) CloseHandle(hJob);
}

// ==================== BASIC UTILITIES ====================

void SystemCore::setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void SystemCore::cls() {
    std::cout << std::flush; // Buộc đẩy hết dữ liệu cout cũ ra màn hình
    std::fflush(stdout);     // Dọn sạch luồng stdout của hệ thống
    
    system("cls"); 
}

std::string SystemCore::getTime(bool includeDate) {
    time_t now = time(0);
    tm *t = localtime(&now);
    std::stringstream ss;
    if (includeDate) {
        ss << "[" << std::setfill('0') << std::setw(2) << t->tm_mday << "/" 
           << std::setw(2) << t->tm_mon + 1 << "/" << t->tm_year + 1900 << "-";
    } else {
        ss << "[";
    }
    ss << std::setw(2) << t->tm_hour << ":" << std::setw(2) << t->tm_min << ":" 
       << std::setw(2) << t->tm_sec << "]";
    return ss.str();
}


std::string SystemCore::formatSize(long long b) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double sz = (double)b;
    int i = 0;
    while (sz >= 1024.0 && i < 4) {
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


bool SystemCore::runRawCommand(const std::string& command) {
    // Chuyển đổi command sang UTF-16
    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, NULL, 0);
    if (wchars_num == 0) return false;
    
    std::vector<wchar_t> wcmd(wchars_num);
    if (MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, wcmd.data(), wchars_num) == 0) return false;

    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE hNull = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hNull != INVALID_HANDLE_VALUE) {
        si.hStdOutput = hNull;
        si.hStdError = hNull;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }
    
    bool success = false;
    if (CreateProcessW(NULL, wcmd.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 0;
        if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
            success = (exitCode == 0);
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    if (hNull != INVALID_HANDLE_VALUE) CloseHandle(hNull);
    return success;
}


std::vector<std::string> SystemCore::parsePaths(const std::string& rawInput) {
    if (rawInput.empty() || rawInput == "0") return {};

    // Tách chuỗi (giữ nguyên logic cũ của MediaProcessor)
    std::string processed = rawInput;
    for (int i = (int)processed.length() - 3; i >= 1; --i) {
        char prev = processed[i - 1];
        char curr = processed[i];
        char next = processed[i + 1];
        char next2 = processed[i + 2];
        bool isPrevNonSpace = (prev != ' ' && prev != '\t' && prev != '"' && prev != '\0');
        bool isDriveLetter = ((curr >= 'A' && curr <= 'Z') || (curr >= 'a' && curr <= 'z'));
        bool isDrivePattern = (next == ':' && next2 == '\\');
        if (isPrevNonSpace && isDriveLetter && isDrivePattern) {
            processed.insert(i, " ");
        }
    }
    
    std::vector<std::string> paths;
    std::string token;
    bool inQuotes = false;
    for (size_t i = 0; i <= processed.size(); ++i) {
        char c = (i < processed.size()) ? processed[i] : '\0';
        if (c == '"') { inQuotes = !inQuotes; continue; }
        bool isSep = (!inQuotes && (c == ' ' || c == '\t' || c == '\0'));
        if (isSep) {
            if (!token.empty()) {
                if (!token.empty() && token.front() == '"') token.erase(0, 1);
                if (!token.empty() && token.back() == '"') token.pop_back();
                
                // Chuẩn hóa đường dẫn (sửa \\ thành \)
                size_t p = token.find("\\\\");
                while (p != std::string::npos) {
                    token.replace(p, 2, "\\");
                    p = token.find("\\\\", p + 1);
                }
                
                if (std::filesystem::exists(token)) {
                    paths.push_back(token);
                } else {
                    std::cout << "    [!] Không tìm thấy: " << token << "\n";
                }
                token.clear();
            }
        } else if (c != '\0') {
            token += c;
        }
    }
    return paths;
}


std::string SystemCore::urlDecode(const std::string& str) {
    std::string decoded;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%') {
            if (i + 2 < str.length()) {
                int value;
                sscanf(str.substr(i + 1, 2).c_str(), "%x", &value);
                decoded += static_cast<char>(value);
                i += 2;
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
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string batPath = std::string(tempPath) + "SystemCoreBatch_" + std::to_string(GetCurrentProcessId()) + ".bat";
    
    std::ofstream batFile(batPath);
    if (!batFile) return false;
    batFile << "@echo off\nchcp 65001 >nul\n" << batContent << "\nexit\n";
    batFile.close();

    bool result = SystemCore::runAdmin("\"" + batPath + "\"", true);
    std::filesystem::remove(batPath);
    return result;
}


void SystemCore::waitEnter() {
    std::cout << "\nNhấn Enter để tiếp tục...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int SystemCore::readInt(const std::string &prompt) {
    std::string line;
    while (true) {
        if (!prompt.empty()) {
            std::cout << prompt;
        }
        if (!std::getline(std::cin, line)) {
            // Nếu stream bị lỗi (EOF), clear và reset lại stream
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        
        // Trim khoảng trắng thừa để xử lý trường hợp người dùng vô tình bấm Space + Enter
        line = SystemCore::trim(line);
        if (line.empty()) continue;  
        
        try {
            return std::stoi(line);
        } catch (...) {
            std::cout << "[!] Vui lòng nhập số hợp lệ.\n";
        }
    }
}

// ==================== SYSTEM COMMANDS ====================
void SystemCore::runCMD(const std::string &cmd) {
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    std::string fullCmd = "cmd.exe /c " + cmd;
    std::vector<char> commandLine(fullCmd.begin(), fullCmd.end());
    commandLine.push_back('\0');

    if (CreateProcessA(NULL, commandLine.data(), NULL, NULL, FALSE, 
                       CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        AssignProcessToJobObject(hJob, pi.hProcess);
        ResumeThread(pi.hThread);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

bool SystemCore::runAdmin(const std::string &cmd, bool silent) {
    if (!silent) {
        std::string answer;
        std::cout << "Chạy quyền Admin cho lệnh [" << cmd << "] (Y/N): ";
        std::cin >> answer;
        std::cin.ignore();
        if (answer != "y" && answer != "Y") {
            std::cout << "Bỏ qua lệnh.\n";
            return false;
        }
    }
    
    int wlen = MultiByteToWideChar(CP_UTF8, 0, cmd.c_str(), -1, NULL, 0);
    std::wstring wCmd(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, cmd.c_str(), -1, &wCmd[0], wlen);
    std::wstring params = L"/c " + wCmd;

    SHELLEXECUTEINFOW sei = {sizeof(sei)};
    sei.lpVerb    = L"runas";
    sei.lpFile    = L"cmd.exe";
    sei.lpParameters = params.c_str();
    sei.nShow     = SW_SHOWNORMAL;
    sei.fMask     = SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteExW(&sei)) {
        std::cout << "[OK] Đang chạy lệnh với quyền Admin...\n";
        if (sei.hProcess) {
            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }
        return true;
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) {
            std::cout << "[!] Người dùng từ chối cấp quyền Admin.\n";
        } else {
            std::cout << "[!] Không thể lấy quyền Admin. (Mã lỗi: " << err << ")\n";
        }
        return false;
    }
}



// ==================== MOUSE & KEYBOARD ====================
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
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hMem) {
        memcpy(GlobalLock(hMem), text.c_str(), text.size() + 1);
        GlobalUnlock(hMem);
        if (!SetClipboardData(CF_TEXT, hMem)) GlobalFree(hMem);
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
    keybd_event(VK_RETURN, 0, 0, 0);
    keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
}

std::string SystemCore::readString(const std::string &prompt) {
    std::string line;
    std::cout << prompt;
    std::getline(std::cin, line);
    return SystemCore::trim(line);
}