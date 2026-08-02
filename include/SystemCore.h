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

public:
    SystemCore();
    ~SystemCore();

    // === UI ===
    void setColor(int color);
    void cls();
    static std::string getTime(bool includeDate = true);
    void waitEnter();

    static std::string trim(const std::string& str);
    static int readInt(const std::string &prompt);
    static std::string readString(const std::string &prompt);


    static std::string formatSize(long long b);
    static bool runRawCommand(const std::string& command);
    static std::vector<std::string> parsePaths(const std::string& rawInput);
    static std::string urlDecode(const std::string& str);
    static bool runBatchAsAdmin(const std::string& batContent, const std::string& description = "");

    // === SYSTEM ===
    void runCMD(const std::string &cmd);
    static bool runAdmin(const std::string &cmd, bool silent = false);
    
    // === MOUSE & KEYBOARD ===
    void leftClick();
    void pressEnter();
    void setClipboard(const std::string &text);
    void pressCtrlV();
};

#endif