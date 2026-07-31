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
};

#endif