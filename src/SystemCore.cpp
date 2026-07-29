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
    
    // Thực hiện lệnh xóa màn hình tùy thuộc cách bạn đang viết:
    system("cls"); 
}
std::string SystemCore::getTime() {
    time_t now = time(0);
    tm *t = localtime(&now);
    std::stringstream ss;
    ss << "[" << std::setfill('0') << std::setw(2) << t->tm_mday << "/" 
       << std::setw(2) << t->tm_mon + 1 << "/" << t->tm_year + 1900 << "-" 
       << std::setw(2) << t->tm_hour << ":" << std::setw(2) << t->tm_min << ":" 
       << std::setw(2) << t->tm_sec << "]";
    return ss.str();
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
        line = trim(line);
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
    std::wstring params = L"/k " + wCmd;

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

// ==================== UTILITY ====================

std::string SystemCore::trim(const std::string &str) {
    std::string s = str;
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
        s.pop_back();
    }
    while (!s.empty() && s.front() == ' ') {
        s.erase(0, 1);
    }
    return s;
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