#include "../include/UtilityTools.h"
#include <iostream>
#include <windows.h>
#include <vector>
#include <iomanip>
#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <random>
#include <chrono>

using namespace std;
namespace fs = std::filesystem;

// ==================== CONSTRUCTOR ====================
UtilityTools::UtilityTools(SystemCore &s) : sc(s) {}

// ==================== HÀM TIỆN ÍCH NỘI BỘ ====================

// Format dung lượng file
static string formatFileSize(long long bytes) {
    if (bytes >= 1024LL * 1024LL * 1024LL) {
        return to_string(bytes / (1024LL * 1024LL * 1024LL)) + " GB";
    } else if (bytes >= 1024LL * 1024LL) {
        return to_string(bytes / (1024LL * 1024LL)) + " MB";
    } else if (bytes >= 1024LL) {
        return to_string(bytes / 1024LL) + " KB";
    }
    return to_string(bytes) + " B";
}

// Lưu lịch sử download
static void saveDownloadHistory(const string& appName) {
    try {
        string historyFile = string(getenv("TEMP")) + "\\download_history.txt";
        ofstream history(historyFile, ios::app);
        if (history.is_open()) {
            time_t now = time(0);
            tm *t = localtime(&now);
            char buf[100];
            strftime(buf, sizeof(buf), "[%d/%m/%Y %H:%M:%S]", t);
            history << buf << " - Da tai: " << appName << "\n";
            history.close();
        }
    } catch (...) {}
}

// ==================== AUTO CLICK (NÂNG CẤP) ====================

void UtilityTools::autoClickPoint() {
    sc.cls();
    cout << "============================================================\n";
    cout << "           AUTO CLICK\n";
    cout << "============================================================\n";
    
    int times = sc.readInt("So lan click: ");
    if (times <= 0) {
        cout << "[!] So lan khong hop le!\n";
        return;
    }
    
    int intervalMs = sc.readInt("Delay giua cac lan (ms): ");
    if (intervalMs < 0) intervalMs = 100;
    
    int delaySec = sc.readInt("Di chuyen chuot den dich (giay): ");
    if (delaySec < 0) delaySec = 3;
    
    cout << "\nDua chuot den vi tri can click...\n";
    for (int i = delaySec; i > 0; i--) { 
        cout << " " << i << "... "; cout.flush(); 
        Sleep(1000); 
    }
    
    POINT p; 
    GetCursorPos(&p);
    cout << "\n\n[*] Bat dau click tai: (" << p.x << ", " << p.y << ")\n";
    cout << "[*] So lan: " << times << ", Delay: " << intervalMs << "ms\n\n";
    
    for (int i = 0; i < times; i++) { 
        SetCursorPos(p.x, p.y); 
        sc.leftClick();
        
        // Hiển thị tiến trình
        if (times > 20 && i % (times / 20) == 0) {
            cout << "\r[+] Tien do: " << (i * 100 / times) << "% ";
            cout.flush();
        }
        
        Sleep(intervalMs); 
    }
    
    cout << "\n\n[✓] Hoan thanh " << times << " lan click!\n";
}

// ==================== SPAM TEXT (NÂNG CẤP) ====================

void UtilityTools::spamText() {
    sc.cls();
    cout << "============================================================\n";
    cout << "           SPAM TEXT (1 dong + xuong dong)\n";
    cout << "============================================================\n";
    
    string content; 
    cout << "\nNhap text can spam: ";
    cin.ignore(); 
    getline(cin, content);
    
    if (content.empty()) { 
        cout << "[!] Text trong!\n"; 
        return; 
    }
    
    int times = sc.readInt("So lan spam: ");
    if (times <= 0) {
        cout << "[!] So lan khong hop le!\n";
        return;
    }
    
    int delayMs = sc.readInt("Delay giua cac lan (ms): ");
    if (delayMs < 0) delayMs = 100;
    
    cout << "\n[?] Tu dong click vao o nhap? (Y/N): ";
    string autoFocus;
    getline(cin, autoFocus);
    bool shouldClick = (autoFocus == "y" || autoFocus == "Y");
    
    cout << "\n[*] Chuan bi spam...\n";
    cout << "    - Text: " << content << "\n";
    cout << "    - So lan: " << times << "\n";
    cout << "    - Delay: " << delayMs << "ms\n";
    
    if (shouldClick) {
        cout << "    - Tu dong click vao vi tri con tro\n";
    }
    
    cout << "\nBat dau sau 3 giay...\n";
    for (int i = 3; i > 0; i--) { 
        cout << " " << i << "... "; cout.flush(); 
        Sleep(1000); 
    }
    cout << "\n";
    
    // Click để focus nếu cần
    if (shouldClick) {
        POINT p;
        GetCursorPos(&p);
        SetCursorPos(p.x, p.y);
        Sleep(50);
        sc.leftClick();
        Sleep(100);
    }
    
    // Spam
    for (int i = 0; i < times; i++) { 
        sc.setClipboard(content); 
        sc.pressCtrlV();    // Paste
        sc.pressEnter();    // Xuong dong
        
        // Hien thi tien trinh
        if ((i + 1) % 10 == 0 || i == times - 1) {
            cout << "\r[+] Da spam: " << (i + 1) << "/" << times << " ";
            cout.flush();
        }
        
        Sleep(delayMs); 
    }
    
    cout << "\n\n[✓] Hoan thanh spam " << times << " lan!\n";
}

// ==================== AUTO PASTE DATA ====================

void UtilityTools::autoPasteData() {
    sc.cls();
    cout << "============================================================\n";
    cout << "           AUTO PASTE DATA\n";
    cout << "============================================================\n";
    
    int n = sc.readInt("So dong du lieu: ");
    if (n <= 0) {
        cout << "[!] So dong khong hop le!\n";
        return;
    }
    
    int delayMs = sc.readInt("Delay giua cac dong (ms): ");
    if (delayMs < 0) delayMs = 200;
    
    cin.ignore();
    vector<string> dataList(n);
    
    cout << "\nNhap " << n << " dong du lieu:\n";
    for (int i = 0; i < n; i++) { 
        cout << "  [" << i + 1 << "]: "; 
        getline(cin, dataList[i]);
        if (dataList[i].empty()) {
            cout << "    [!] Dong trong, bo qua!\n";
            dataList[i] = "(empty)";
        }
    }
    
    cout << "\n[?] Tu dong click vao o nhap? (Y/N): ";
    string autoFocus;
    getline(cin, autoFocus);
    bool shouldClick = (autoFocus == "y" || autoFocus == "Y");
    
    cout << "\n[*] Chuan bi paste " << n << " dong...\n";
    cout << "Bat dau sau 3 giay...\n";
    for (int i = 3; i > 0; i--) { 
        cout << " " << i << "... "; cout.flush(); 
        Sleep(1000); 
    }
    cout << "\n";
    
    // Click để focus
    if (shouldClick) {
        POINT p;
        GetCursorPos(&p);
        SetCursorPos(p.x, p.y);
        Sleep(50);
        sc.leftClick();
        Sleep(100);
    }
    
    // Paste từng dòng
    for (int i = 0; i < n; i++) {
        sc.setClipboard(dataList[i]);
        sc.pressCtrlV();
        sc.pressEnter();
        
        cout << "\r[+] Da paste: " << (i + 1) << "/" << n << " ";
        cout.flush();
        
        Sleep(delayMs);
    }
    
    cout << "\n\n[✓] Hoan thanh paste " << n << " dong!\n";
}

// ==================== QR CODE ====================

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
    
    // URL encode spaces
    string encodedText = text;
    for (char &c : encodedText) {
        if (c == ' ') c = '+';
    }
    
    cout << "\n[*] Dang lay QR Code cho: " << text << "\n\n";
    sc.runCMD("curl -s qrenco.de/" + encodedText);
    cout << "\n";
}

void UtilityTools::ShowN_QR(int number) {
    if (number >= 15 || number <= 0) { 
        cout << "[!] So luong khong hop le (1-14)!\n"; 
        return; 
    }
    
    cin.ignore(); 
    vector<string> list_qr;
    
    cout << "\nNhap " << number << " noi dung QR:\n";
    for (int dem = 1; dem <= number; dem++) {
        string text; 
        cout << "  [" << dem << "/" << number << "]: ";
        if (!getline(cin, text) || text.empty()) { 
            cout << "    [!] Khong duoc de trong!\n"; 
            dem--; 
            continue; 
        }
        list_qr.push_back(text);
    }

    cout << "\n[*] Dang lay " << number << " ma QR...\n\n";
    
    for (size_t i = 0; i < list_qr.size(); i++) {
        string current_text = list_qr[i];
        cout << "  [" << i + 1 << "/" << list_qr.size() << "] " << current_text << "\n";
        
        // URL encode
        for (char &c : current_text) {
            if (c == ' ') c = '+';
        }
        
        sc.runCMD("curl -s qrenco.de/" + current_text);
        cout << "\n";
        
        if (i < list_qr.size() - 1) Sleep(300);
    }
    
    cout << "\n[✓] Hoan thanh " << number << " ma QR!\n";
}

// ==================== DOWNLOAD MANAGER (NÂNG CẤP) ====================

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
    cout << "============================================================\n";
    cout << "           TRÌNH TẢI & CÀI ĐẶT APP TỰ ĐỘNG\n";
    cout << "============================================================\n";
    
    // Hiển thị danh sách 2 cột
    for (size_t i = 0; i < appList.size(); i++) {
        cout << "[" << setw(2) << i + 1 << "] " << left << setw(28) << appList[i].name;
        if ((i + 1) % 2 == 0) cout << "\n";
    }
    if (appList.size() % 2 != 0) cout << "\n";
    
    cout << "============================================================\n";
    cout << " [A] TẢI TOÀN BỘ DANH SÁCH\n";
    cout << " [H] Xem lịch sử download\n";
    cout << " [0] Quay lại\n";
    cout << "============================================================\n";
    cout << " -> Nhập lựa chọn: "; 
    
    string input; 
    cin >> input;
    if (input == "0") return;

    if (input == "H" || input == "h") {
        showDownloadHistory();
        return;
    }

    if (input == "A" || input == "a") { 
        cout << "\n[*] Bat dau tai " << appList.size() << " app...\n";
        for (const auto& app : appList) {
            processDownload(app);
        }
        cout << "\n[✓] Hoan thanh tai tat ca!\n";
    } 
    else {
        try { 
            int idx = stoi(input) - 1; 
            if (idx >= 0 && idx < (int)appList.size()) {
                processDownload(appList[idx]);
            } else {
                cout << "[!] Lua chon khong hop le!\n";
            }
        } catch (...) { 
            cout << "[!] Vui long nhap so hoac chu cai hop le!\n"; 
        }
    }
}

void UtilityTools::processDownload(const AppInfo &app) {
    cout << "\n[+] Dang tai: " << app.name << "\n";
    cout << "    File: " << app.fileName << "\n";
    
    string tempPath = "%temp%\\" + app.fileName;
    string fullPath = string(getenv("TEMP")) + "\\" + app.fileName;
    
    // Xóa file cũ nếu tồn tại
    if (fs::exists(fullPath)) {
        try { fs::remove(fullPath); } catch (...) {}
    }
    
    // Tải file với progress bar
    string cmd = "curl -L --progress-bar \"" + app.url + "\" -o \"" + tempPath + "\"";
    
    // Chạy curl và hiển thị progress
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        cout << "    [✗] Khong the khoi tao download!\n";
        return;
    }
    
    char buffer[128];
    int lastProgress = -1;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        string line(buffer);
        if (line.find('%') != string::npos) {
            try {
                int progress = stoi(line.substr(0, line.find('%')));
                if (progress != lastProgress && progress % 5 == 0) {
                    cout << "\r    Tien do: " << progress << "% ";
                    cout.flush();
                    lastProgress = progress;
                }
            } catch (...) {}
        }
    }
    _pclose(pipe);
    
    // Kiểm tra kết quả
    if (fs::exists(fullPath) && fs::file_size(fullPath) > 0) {
        auto size = fs::file_size(fullPath);
        cout << "\n    [✓] Tai thanh cong! (" << formatFileSize(size) << ")\n";
        
        // Lưu lịch sử
        saveDownloadHistory(app.name);
        
        // Hỏi có chạy installer không
        cout << "    [?] Chạy file cài đặt? (Y/N): ";
        string answer;
        cin >> answer;
        
        if (answer == "y" || answer == "Y") {
            cout << "    [>] Dang chay installer...\n";
            sc.runCMD("start \"\" \"" + fullPath + "\"");
        } else {
            cout << "    [i] Da luu file tai: " << fullPath << "\n";
        }
    } else {
        cout << "\n    [✗] Tai that bai! Vui long kiem tra ket noi mang.\n";
    }
}

void UtilityTools::showDownloadHistory() {
    string historyFile = string(getenv("TEMP")) + "\\download_history.txt";
    ifstream history(historyFile);
    
    sc.cls();
    cout << "============================================================\n";
    cout << "           LICH SU DOWNLOAD\n";
    cout << "============================================================\n";
    
    if (!history.is_open()) {
        cout << "[i] Chua co lich su download.\n";
        return;
    }
    
    string line;
    int count = 1;
    while (getline(history, line)) {
        cout << " [" << count++ << "] " << line << "\n";
    }
    history.close();
    
    cout << "============================================================\n";
}

// ==================== UNINSTALL BLOATWARE ====================

void UtilityTools::uninstallBloatware() {
    sc.cls();
    cout << "============================================================\n";
    cout << "           XOA APP RAC (BLOATWARE)\n";
    cout << "============================================================\n";
    
    string listRac = "BingWeather|BingNews|SolitaireCollection|People|PowerAutomateDesktop|"
                     "Todo|GetHelp|Getstarted|OfficeHub|SkypeApp|YourPhone|FeedbackHub|"
                     "ZuneVideo|ZuneMusic|MixedReality.Portal|Clipchamp|Disney|"
                     "MicrosoftStickyNotes|WindowsAlarms|WindowsMaps|YourPhone|"
                     "MicrosoftSolitaireCollection|GamingApp|XboxGamingOverlay|"
                     "DevHome|OneNote|MicrosoftTeams|Cortana|ZuneMusic|ZuneVideo|"
                     "Copilot|CandyCrush|King\\.com|SpiderSolitaire|FreeCell|"
                     "Hearts|Solitaire|Hearts|Zone";
    
    cout << "[*] Dang quet va xoa app rac...\n";
    cout << "[!] Qua trinh nay can quyen Admin\n\n";
    
    string cmd = "powershell -Command \"";
    cmd += "Get-AppxPackage -AllUsers | Where-Object {$_.Name -match '" + listRac + "'} | Remove-AppxPackage -AllUsers; ";
    cmd += "Get-AppxProvisionedPackage -Online | Where-Object {$_.DisplayName -match '" + listRac + "'} | Remove-AppxProvisionedPackage -Online";
    cmd += "\"";
    
    sc.runAdmin(cmd, true);
    
    cout << "\n[✓] Hoan thanh xoa app rac!\n";
}

// ==================== HIỂN THỊ MENU ====================

void UtilityTools::showToolsMenu() {
    sc.cls();
    cout << "============================================================\n";
    cout << "           CONG CU TIEN ICH\n";
    cout << "============================================================\n";
    cout << " [1] Auto Click\n";
    cout << " [2] Spam Text (1 dong + xuong dong)\n";
    cout << " [3] Auto Paste Data\n";
    cout << " [4] Tao QR Code (1 cai)\n";
    cout << " [5] Tao QR Code (Nhieu cai)\n";
    cout << " [6] Download Manager\n";
    cout << " [7] Xem lich su Download\n";
    cout << " [8] Xoa app rac (Bloatware)\n";
    cout << " [0] Quay lai\n";
    cout << "============================================================\n";
    cout << " [Chon]: ";
}