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
    for (char &c : text) if (c == ' ') c = '+';
    sc.runCMD("curl -s qrenco.de/" + text); // Thêm -s để silent
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
        for (char &c : current_text) if (c == ' ') c = '+';
        sc.runCMD("curl -s qrenco.de/" + current_text); // Thêm -s
        cout << "\n--------------------------------------\n";
        if (i < list_qr.size() - 1) Sleep(500); // Giảm thời gian chờ
    }
}

void UtilityTools::uninstallBloatware() {
    string listRac = "BingWeather|BingNews|SolitaireCollection|People|PowerAutomateDesktop|"
                     "Todo|GetHelp|Getstarted|OfficeHub|SkypeApp|YourPhone|FeedbackHub|"
                     "ZuneVideo|ZuneMusic|MixedReality.Portal|Clipchamp|Disney|"
                     "MicrosoftStickyNotes|WindowsAlarms|WindowsMaps|YourPhone|"
                     "MicrosoftSolitaireCollection|GamingApp|XboxGamingOverlay|"
                     "DevHome|OneNote|MicrosoftTeams|Cortana|ZuneMusic|ZuneVideo|"
                     "Copilot|CandyCrush|King\\.com|SpiderSolitaire|FreeCell|"
                     "Hearts|Solitaire|Hearts|Zone";
    
    cout << "[SYSTEM] Dang tien hanh xoa app rac (Bloatware) mo rong...\n";
    
    // GỘP 2 LỆNH VÀO 1
    string cmd = "powershell -Command \"";
    cmd += "Get-AppxPackage -AllUsers | Where-Object {$_.Name -match '" + listRac + "'} | Remove-AppxPackage -AllUsers; ";
    cmd += "Get-AppxProvisionedPackage -Online | Where-Object {$_.DisplayName -match '" + listRac + "'} | Remove-AppxProvisionedPackage -Online";
    cmd += "\"";
    
    sc.runAdmin(cmd, true);
    cout << "\n[SUCCESS] Hệ thống đã dọn dẹp xong ứng dụng mặc định!\n";
}


void UtilityTools::downloadManager() {
    vector<AppInfo> appList = {
        {"Google Chrome", "https://dl.google.com/tag/s/appname%3DGoogle%2520Chrome/update2/installers/ChromeSetup.exe", "ChromeSetup.exe"},
        {"Coc Coc", "https://files.coccoc.com/browser/coccoc_vi.exe", "CocCocSetup.exe"},
        {"Brave Browser", "https://laptop-updates.brave.com/latest/winx64", "BraveSetup.exe"},
        {"EVKey (Gõ TV)", "https://github.com/lamquangminh/EVKey/releases/download/v5.0.4/EVKey.zip", "EVKey.zip"},
        {"OpenKey (Gõ TV)", "https://github.com/tphan/openkey/releases/latest/download/OpenKey-Windows-x64.zip", "OpenKey.zip"},
        {"Zalo PC", "https://zalo.me/download/zalo-pc", "ZaloSetup.exe"},
        {"Discord", "https://discord.com/api/downloads/distributions/app/installers/latest?channel=stable&platform=win&arch=x64", "DiscordSetup.exe"},
        {"Telegram PC", "https://telegram.org/dl/desktop/win64", "TelegramSetup.exe"},
        {"Spotify", "https://download.scdn.co/SpotifySetup.exe", "SpotifySetup.exe"},
        {"7-Zip (Giải nén)", "https://www.7-zip.org/a/7z2408-x64.exe", "7zipSetup.exe"},
        {"WinRAR", "https://www.rarlab.com/rar/winrar-x64-701.exe", "WinRARSetup.exe"},
        {"Geek Uninstaller", "https://geekuninstaller.com/geek.zip", "GeekUninstaller.zip"},
        {"WARP 1.1.1.1", "https://1111-releases.cloudflareclient.com/windows/Cloudflare_WARP_Release-x64.msi", "CloudflareWARP.msi"},
        {"VS Code", "https://code.visualstudio.com/sha/download?build=stable&os=win32-x64-user", "VSCodeSetup.exe"},
        {"Notepad++", "https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v8.6.7/npp.8.6.7.Installer.x64.exe", "NotepadPlusPlusSetup.exe"},
        {"Git for Windows", "https://github.com/git-for-windows/git/releases/download/v2.45.1.windows.1/Git-2.45.1-64-bit.exe", "GitSetup.exe"},
        {"Epic Games", "https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/installer/download/EpicGamesLauncherInstaller.msi", "EpicGamesSetup.msi"}
    };

    sc.cls();
    cout << "====================================================================\n";
    cout << "               TRÌNH TẢI & CÀI ĐẶT APP TỰ ĐỘNG (EXPANDED)           \n";
    cout << "====================================================================\n";
    
    for (size_t i = 0; i < appList.size(); i++) {
        cout << "[" << setw(2) << i + 1 << "] " << left << setw(25) << appList[i].name;
        if ((i + 1) % 2 == 0) cout << "\n";
    }
    if (appList.size() % 2 != 0) cout << "\n";
    
    cout << "====================================================================\n";
    cout << " [A] TẢI TOÀN BỘ DANH SÁCH\n [0] Quay lại\n";
    cout << "====================================================================\n";
    cout << " -> Nhập lựa chọn của bạn: "; 
    
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
    // Thêm --silent --progress-bar để hiển thị tiến trình đẹp hơn
    sc.runCMD("curl -L --silent --progress-bar \"" + app.url + "\" -o \"" + tempPath + "\"");
    cout << "[>] Đang khởi chạy installer: " << app.fileName << "\n";
    sc.runCMD("start \"\" \"" + tempPath + "\"");
}