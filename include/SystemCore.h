#ifndef SYSTEMCORE_H
#define SYSTEMCORE_H

#include <winsock2.h>
#include <windows.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

class SystemCore {
private:
    HANDLE hJob;
    
    // === CẤU HÌNH LOG ==
    bool enableFileLogging = false;
    bool enableConsoleLogging = true;
    std::string logDirectory = "logs";
    size_t maxLogLines = 1000;
    std::mutex logMutex;
    
    void rotateLogFile();
    void writeToFile(const std::string& content);

public:
    SystemCore();
    ~SystemCore();

    // === UI ===
    void setColor(int color);
    void cls();
    std::string getTime();
    void waitEnter();
    int readInt(const std::string &prompt);

    // === SYSTEM ===
    void runCMD(const std::string &cmd);
    bool runAdmin(const std::string &cmd, bool silent = false);
    
    // === UTILITY ===
    std::string trim(const std::string& str);

    // === MOUSE & KEYBOARD ===
    void leftClick();
    void pressEnter();
    void setClipboard(const std::string &text);
    void pressCtrlV();

    // === LOG CONFIG ===
    void setFileLogging(bool enable) { enableFileLogging = enable; }
    void setConsoleLogging(bool enable) { enableConsoleLogging = enable; }
    void setMaxLogLines(size_t lines) { maxLogLines = lines; }
    
    // === LOG TEMPLATE ===
    template <typename... Args>
    void log(Args... args) {
        if (!enableConsoleLogging && !enableFileLogging) return;
        
        std::string timeStr = getTime();
        std::string logContent = timeStr + " : ";
        logContent += buildLogString(args...);
        
        if (enableConsoleLogging) {
            std::cout << logContent << "\n";
        }
        
        if (enableFileLogging) {
            std::lock_guard<std::mutex> lock(logMutex);
            writeToFile(logContent);
        }
    }

private:
    // Helper để build log string
    template <typename T>
    std::string buildLogString(T&& arg) {
        std::stringstream ss;
        ss << arg;
        return ss.str();
    }
    
    template <typename T, typename... Rest>
    std::string buildLogString(T&& arg, Rest&&... rest) {
        std::stringstream ss;
        ss << arg << buildLogString(std::forward<Rest>(rest)...);
        return ss.str();
    }
};

#endif // SYSTEMCORE_H