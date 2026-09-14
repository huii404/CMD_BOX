#include "TaskLog.h"
#include "SystemCore.h"
#include <windows.h>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>

namespace fs = std::filesystem;
namespace {
std::mutex logMutex;

std::string oneLine(std::string value) {
    for (char& c : value) {
        if (c == '\r' || c == '\n' || c == '\t') c = ' ';
    }
    return value;
}
}

fs::path TaskLog::path() {
    wchar_t localAppData[32768] = {};
    DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, 32768);
    if (length > 0 && length < 32768) {
        return fs::path(localAppData) / L"CMDBox" / L"logs" / L"tasks.log";
    }
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    return fs::path(exePath).parent_path() / L"tasks.log";
}

void TaskLog::write(const std::string& action, const std::string& result,
                    const std::string& detail, const std::string& outputPath) noexcept {
    try {
        std::lock_guard<std::mutex> lock(logMutex);
        fs::path file = path();
        fs::create_directories(file.parent_path());
        std::ofstream out(file, std::ios::app | std::ios::binary);
        if (!out) return;
        out << SystemCore::getTime() << '\t' << oneLine(action) << '\t'
            << oneLine(result) << '\t' << oneLine(detail) << '\t'
            << oneLine(outputPath) << '\n';
    } catch (...) {
        // Logging must never interrupt the user's task.
    }
}

void TaskLog::showRecent() {
    SystemCore::cls();
    std::cout << "NHẬT KÝ TÁC VỤ (tối đa 30 dòng gần nhất)\n"
              << "Đường dẫn: " << path().u8string() << "\n\n";
    std::ifstream in(path(), std::ios::binary);
    if (!in) {
        std::cout << "Chưa có nhật ký.\n";
    } else {
        std::deque<std::string> recent;
        std::string line;
        while (std::getline(in, line)) {
            if (recent.size() == 30) recent.pop_front();
            recent.push_back(line);
        }
        for (const auto& entry : recent) std::cout << entry << '\n';
    }
    SystemCore::waitEnter();
}
