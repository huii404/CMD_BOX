#ifndef CHATBOX_H
#define CHATBOX_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <windows.h>

class Chatbox {
private:
    std::thread chatboxThread;
    std::mutex uiMutex;
    bool isRunning;
    bool isChatboxActive;
    std::vector<std::string> chatQuotes;

    void chatboxWorker();
    void drawBorder(HANDLE hConsole, const std::string& quote); // Hàm vẽ khung đồng bộ

public:
    Chatbox();
    ~Chatbox();

    void start();
    void stop();
    void pause();
    void resume();
};

#endif // CHATBOX_H