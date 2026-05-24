#include "../include/UtilityTools.h"
#include <iostream>
#include <windows.h>
#include <vector>
#include <iomanip>
#include <string>

using namespace std;

UtilityTools::UtilityTools(SystemCore &s) : sc(s) {}

// ==================== AUTO ACTIONS (TỰ ĐỘNG HÓA) ====================

void UtilityTools::autoClickPoint() {
    cout << "--- AUTO CLICK TAI VI TRI ---\n";
    int times = sc.readInt("So lan click: ");
    int intervalMs = sc.readInt("Delay giua cac lan (ms): ");
    int delaySec = sc.readInt("Di chuyen chuot den dich (giay): ");
    
    if (times <= 0 || intervalMs < 0 || delaySec < 0) { 
        cout << "[!] Gia tri khong hop le.\n"; 
        return; 
    }
    
    cout << "\nDua chuot den vi tri can click...\n";
    for (int i = delaySec; i > 0; i--) { 
        cout << i << "... "; cout.flush(); Sleep(1000); 
    }
    
    POINT p; 
    GetCursorPos(&p);
    cout << "\nBat dau click tai: (" << p.x << ", " << p.y << ")\n";
    
    for (int i = 0; i < times; i++) { 
        SetCursorPos(p.x, p.y); 
        sc.leftClick(); 
        Sleep(intervalMs); 
    }
    cout << "Da xong!\n";
}

void UtilityTools::spamText() {
    string content; 
    cout << "\nText: ";
    cin.ignore(); 
    getline(cin, content);
    
    if (content.empty()) { 
        cout << "[!] Text trong.\n"; 
        return; 
    }
    
    int times = sc.readInt("So lan: "); 
    int delayMs = sc.readInt("Delay (ms): ");
    
    cout << "\nClick vao o nhap lieu trong 3 giay...\n";
    for (int i = 3; i > 0; i--) { 
        cout << i << "... "; cout.flush(); Sleep(1000); 
    }
    
    for (int i = 0; i < times; i++) { 
        sc.setClipboard(content); 
        sc.pressCtrlV(); 
        sc.pressEnter(); 
        Sleep(delayMs); 
    }
}

void UtilityTools::autoPasteData() {
    int n = sc.readInt("So dong du lieu: "); 
    int delayMs = sc.readInt("Delay (ms): "); 
    cin.ignore();
    
    if (n <= 0) return;
    vector<string> dataList(n);
    
    for (int i = 0; i < n; i++) { 
        cout << "Dong [" << i + 1 << "]: "; 
        getline(cin, dataList[i]); 
    }
    
    cout << "\nClick vao o nhap lieu trong 3 giay...\n";
    for (int i = 3; i > 0; i--) { 
        cout << i << "... "; cout.flush(); Sleep(1000); 
    }
    
    for (const string &data : dataList) { 
        sc.setClipboard(data); 
        sc.pressCtrlV(); 
        sc.pressEnter(); 
        Sleep(delayMs); 
    }
}

// ==================== EXTENSION (TIỆN ÍCH) ====================

bool UtilityTools::text_processing(const string &text) {
    if (text.empty() || text.length() > 99) { 
        return true; 
    }
    return false;
}

void UtilityTools::ShowQR(string text) {
    if (text_processing(text)) {
        cout << "[!] Van ban khong hop le (Max 99 ky tu).\n";
        return;
    }
    // Thay khoảng trắng bằng dấu + để curl không bị lỗi
    for (char &c : text) if (c == ' ') c = '+';
    sc.runCMD("curl qrenco.de/" + text);
}

void UtilityTools::ShowN_QR(int number) {
    if (number >= 15 || number <= 0) { 
        cout << "[!] So luong khong hop le!\n"; 
        return; 
    }
    cin.ignore(); 
    vector<string> list_qr;
    
    for (int dem = 1; dem <= number; dem++) {
        string text; 
        cout << "[" << dem << "/" << number << "] Nhap noi dung QR: ";
        if (!getline(cin, text) || text.empty()) { 
            cout << "[!] Khong duoc de trong!\n"; 
            dem--; 
            continue; 
        }
        list_qr.push_back(text);
    }

    for (size_t i = 0; i < list_qr.size(); i++) {
        string current_text = list_qr[i];
        cout << "Dang lay ma QR thu " << i + 1 << "...\n"; 
        Sleep(4000); 
        for (char &c : current_text) if (c == ' ') c = '+';
        sc.runCMD("curl qrenco.de/" + current_text);
        cout << "\n--------------------------------------\n";
    }
}

void UtilityTools::uninstallBloatware() {
    // Danh sách app rác cần xóa
    string listRac = "BingWeather|BingNews|SolitaireCollection|People|PowerAutomateDesktop|Todo|GetHelp|Getstarted|OfficeHub|SkypeApp|YourPhone|FeedbackHub|ZuneVideo|ZuneMusic|MixedReality.Portal|Clipchamp|Disney";
    
    cout << "[SYSTEM] Dang tien hanh xoa app rac (Bloatware)...\n";
    string cmd1 = "powershell -Command \"Get-AppxPackage -AllUsers | Where-Object {$_.Name -match '" + listRac + "'} | Remove-AppxPackage -AllUsers\"";
    string cmd2 = "powershell -Command \"Get-AppxProvisionedPackage -Online | Where-Object {$_.DisplayName -match '" + listRac + "'} | Remove-AppxProvisionedPackage -Online\"";
    
    sc.runAdmin(cmd1, true);
    sc.runAdmin(cmd2, true);
    cout << "\n[SUCCESS] Hệ thống đã dọn dẹp xong ứng dụng mặc định!\n";
}

void UtilityTools::downloadManager() {
    vector<AppInfo> appList = {
        {"Google Chrome", "https://dl.google.com/tag/s/appname%3DGoogle%2520Chrome/update2/installers/ChromeSetup.exe", "ChromeSetup.exe"},
        {"Coc Coc", "https://files.coccoc.com/browser/coccoc_vi.exe", "CocCocSetup.exe"},
        {"Zalo PC", "https://zalo.me/download/zalo-pc", "ZaloSetup.exe"},
        {"Discord", "https://discord.com/api/downloads/distributions/app/installers/latest?channel=stable&platform=win&arch=x86", "DiscordSetup.exe"},
        {"VS Code", "https://code.visualstudio.com/sha/download?build=stable&os=win32-x64-user", "VSCodeSetup.exe"}
    };

    sc.cls();
    cout << "====================================================\n";
    cout << "          TRÌNH TẢI & CÀI ĐẶT APP TỰ ĐỘNG           \n";
    cout << "====================================================\n";
    
    for (size_t i = 0; i < appList.size(); i++) {
        cout << "[" << i + 1 << "] " << left << setw(15) << appList[i].name << "\n";
    }
    cout << "[A] TẢI TÒAN BỘ\n[0] Quay lại\n----------------------------------------------------\n[Chon]: "; 
    
    string input; 
    cin >> input;
    if (input == "0") return;

    if (input == "A" || input == "a") { 
        for (const auto& app : appList) processDownload(app); 
    } 
    else {
        try { 
            int idx = stoi(input) - 1; 
            if (idx >= 0 && idx < (int)appList.size()) processDownload(appList[idx]); 
            else cout << "[!] Lua chon khong hop le!\n";
        } catch (...) { 
            cout << "[!] Vui lòng nhập số hoặc chữ cái hợp lệ!\n"; 
        }
    }
}

void UtilityTools::processDownload(const AppInfo &app) {
    string tempPath = "%temp%\\" + app.fileName;
    cout << "\n[+] Đang tải " << app.name << "...\n";
    // Sử dụng curl -L để theo link redirect
    sc.runCMD("curl -L \"" + app.url + "\" -o \"" + tempPath + "\"");
    cout << "[>] Đang khởi chạy installer: " << app.fileName << "\n";
    sc.runCMD("start \"\" \"" + tempPath + "\"");
}