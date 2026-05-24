#include "../include/Chatbox.h"
#include <iostream>
#include <random>
#include <iomanip>

using namespace std;

Chatbox::Chatbox() : isRunning(true), isChatboxActive(false) {
    chatQuotes = {
        "GITHUB:\thuii404",
        "hi chào các bồ.",
        "bồ đã dọn rác máy chưa?",
        "thử tính năng QUICKSHARE đi hay lắm",
        "gặp lỗi gì thì nhắn cho chủ tool nhó",
        "ăn gì chưa em iu?"
    };
}

Chatbox::~Chatbox() {
    stop();
}

void Chatbox::start() {
    isRunning = true;
    isChatboxActive = true;
    chatboxThread = std::thread(&Chatbox::chatboxWorker, this);
}

void Chatbox::stop() {
    isRunning = false;
    isChatboxActive = false;
    if (chatboxThread.joinable()) {
        chatboxThread.join();
    }
}

void Chatbox::pause() {
    isChatboxActive = false;
}

void Chatbox::resume() {
    isChatboxActive = true;
}

void Chatbox::drawBorder(HANDLE hConsole, const string& quote) {
    // 92 ký tự bọc đoạn text
    int totalWidth = 92; 
    
    // Vẽ viền trên (Màu tím đen - Color 5)
    SetConsoleTextAttribute(hConsole, 5);
    cout << "┌" << string(totalWidth - 2, '-') << "┐\n";
    
    // Vẽ nội dung thân hộp
    cout << "│ ";
    SetConsoleTextAttribute(hConsole, 13); // Màu tím sáng(ASSISTANT)
    cout << "🤖 ASSISTANT: ";
    SetConsoleTextAttribute(hConsole, 7);  // Màu trắng cho text
    
    // LOGIC CĂN CHỈNH KHOẢNG TRỐNG: Tự động bù trừ khoảng trống chính xác
    // "🤖 ASSISTANT: " chiếm 15 ký tự, cộng với 4 ký tự biên cấu trúc "│ " và " │"
    int reservedSpace = 15 + 4;
    int maxTextWidth = totalWidth - reservedSpace;
    
    cout << left << setw(maxTextWidth) << quote;
    
    // Vẽ viền phải đóng hộp
    SetConsoleTextAttribute(hConsole, 5);
    cout << "│\n";
    
    // Vẽ viền dưới
    cout << "└" << string(totalWidth - 2, '-') << "┘\n";
    SetConsoleTextAttribute(hConsole, 7); // màu chữ mặc định
}

void Chatbox::chatboxWorker() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, chatQuotes.size() - 1);

    bool isFirstRun = true;

    while (isRunning) {
        if (isChatboxActive) {
            unique_lock<mutex> lock(uiMutex);
            
            // Lưu lại vị trí con trỏ hiện tại (nơi người dùng đang gõ nhập lệnh)
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            GetConsoleScreenBufferInfo(hConsole, &csbi);
            
            // ÉP TỌA ĐỘ CỐ ĐỊNH: Dòng số 5 là khoảng không gian trống dành riêng cho Chatbox
            COORD chatPos = {0, 2}; 
            SetConsoleCursorPosition(hConsole, chatPos);
            
            // Tiến hành vẽ hộp Status Bar
            drawBorder(hConsole, chatQuotes[dis(gen)]);
            
            // TRẢ CON TRỎ CHUỘT: Quay lại đúng vị trí nhập lệnh [Chọn]: ở dưới cùng
            SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);
            lock.unlock();

            if (isFirstRun) {
                isFirstRun = false;
                Sleep(100);
                continue;
            }

            for (int i = 0; i < 30 && isChatboxActive && isRunning; i++) {
                Sleep(100); 
            }
        } else {
            isFirstRun = true; 
            Sleep(150);
        }
    }
}