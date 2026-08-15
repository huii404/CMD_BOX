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

UtilityTools::UtilityTools(SystemCore &s) : sc(s) {}

// Lưu lịch sử tải
static void saveDownloadHistory(const string& appName) {
    try {
        string historyFile = string(getenv("TEMP")) + "\\download_history.txt";
        ofstream history(historyFile, ios::app);
        if (history.is_open()) {
            history << SystemCore::getTime(true) << " - Đã tải: " << appName << "\n";
            history.close();
        }
    } catch (...) {}
}

static bool sleepWithEmergencyCheck(int totalMs) {
    int elapsed = 0;
    while (elapsed < totalMs) {
        if (SystemCore::checkEmergencyStop()) return true;
        int step = (totalMs - elapsed > 20) ? 20 : (totalMs - elapsed);
        Sleep(step);
        elapsed += step;
    }
    return SystemCore::checkEmergencyStop();
}

// Auto click chuột theo vị trí
void UtilityTools::autoClickPoint() {
    sc.cls();
    cout << "[i] Nhấn phím ESC hoặc F6 bất kỳ lúc nào để DỪNG KHẨN CẤP\n\n";
    
    int times = sc.readInt("Số lần click: ");
    if (times <= 0) {
        cout << "[!] Số lần không hợp lệ!\n";
        return;
    }
    
    int intervalMs = sc.readInt("Delay giữa các lần (ms): ");
    if (intervalMs < 0) intervalMs = 100;
    
    int delaySec = sc.readInt("Thời gian di chuyển chuột đến đích (giây): ");
    if (delaySec < 0) delaySec = 3;
    
    cout << "\nDi chuyển chuột đến vị trí cần click...\n";
    for (int i = delaySec; i > 0; i--) { 
        cout << " " << i << "... "; cout.flush(); 
        if (sleepWithEmergencyCheck(1000)) {
            cout << "\n[!] Đã hủy bởi người dùng.\n";
            return;
        }
    }
    
    POINT p; 
    GetCursorPos(&p);
    cout << "\n\n[*] Bắt đầu click tại tọa độ: (" << p.x << ", " << p.y << ")\n";
    cout << "[*] Số lần: " << times << " | Delay: " << intervalMs << "ms\n";
    cout << "[*] (Nhấn ESC / F6 để dừng ngay lập tức)\n\n";
    
    bool stopped = false;
    int executed = 0;
    for (int i = 0; i < times; i++) { 
        if (SystemCore::checkEmergencyStop()) {
            stopped = true;
            break;
        }

        SetCursorPos(p.x, p.y); 
        sc.leftClick();
        executed++;
        
        if (times > 20 && i % (times / 20) == 0) {
            cout << "\r[+] Tiến độ: " << (i * 100 / times) << "% ";
            cout.flush();
        }
        
        if (sleepWithEmergencyCheck(intervalMs)) {
            stopped = true;
            break;
        }
    }
    
    if (stopped) {
        cout << "\n\n[!] ĐÃ DỪNG KHẨN CẤP (Đã click " << executed << "/" << times << " lần)\n";
    } else {
        cout << "\n\n[✓] Hoàn thành " << times << " lần click!\n";
    }
}

// Spam văn bản tự động
void UtilityTools::spamText() {
    sc.cls();
    cout << "[i] Nhấn phím ESC hoặc F6 bất kỳ lúc nào để DỪNG KHẨN CẤP\n\n";
    
    string content; 
    cout << "Nhập text cần gửi: ";
    cin.ignore(); 
    getline(cin, content);
    
    if (content.empty()) { 
        cout << "[!] Nội dung trống!\n"; 
        return; 
    }
    
    int times = sc.readInt("Số lần gửi: ");
    if (times <= 0) {
        cout << "[!] Số lần không hợp lệ!\n";
        return;
    }
    
    int delayMs = sc.readInt("Delay giữa các lần (ms): ");
    if (delayMs < 0) delayMs = 100;
    
    cout << "\n[?] Tự động click vào ô nhập? (Y/N): ";
    string autoFocus;
    getline(cin, autoFocus);
    bool shouldClick = (autoFocus == "y" || autoFocus == "Y");
    
    cout << "\n[*] Chuẩn bị gửi...\n";
    cout << "    - Nội dung: " << content << "\n";
    cout << "    - Số lần: " << times << "\n";
    cout << "    - Delay: " << delayMs << "ms\n";
    cout << "    - Phím ngắt: ESC / F6\n";
    
    cout << "\nBắt đầu sau 3 giây...\n";
    for (int i = 3; i > 0; i--) { 
        cout << " " << i << "... "; cout.flush(); 
        if (sleepWithEmergencyCheck(1000)) {
            cout << "\n[!] Đã hủy bởi người dùng.\n";
            return;
        }
    }
    cout << "\n";
    
    if (shouldClick) {
        POINT p;
        GetCursorPos(&p);
        SetCursorPos(p.x, p.y);
        Sleep(50);
        sc.leftClick();
        Sleep(100);
    }
    
    bool stopped = false;
    int executed = 0;
    for (int i = 0; i < times; i++) { 
        if (SystemCore::checkEmergencyStop()) {
            stopped = true;
            break;
        }

        sc.setClipboard(content); 
        sc.pressCtrlV();
        sc.pressEnter();
        executed++;
        
        if ((i + 1) % 10 == 0 || i == times - 1) {
            cout << "\r[+] Đã gửi: " << (i + 1) << "/" << times << " ";
            cout.flush();
        }
        
        if (sleepWithEmergencyCheck(delayMs)) {
            stopped = true;
            break;
        }
    }
    
    if (stopped) {
        cout << "\n\n[!] ĐÃ DỪNG KHẨN CẤP (Đã gửi " << executed << "/" << times << " lần)\n";
    } else {
        cout << "\n\n[✓] Hoàn thành gửi " << times << " lần!\n";
    }
}

// Tự động paste danh sách dữ liệu
void UtilityTools::autoPasteData() {
    sc.cls();
    cout << "[i] Nhấn phím ESC hoặc F6 bất kỳ lúc nào để DỪNG KHẨN CẤP\n\n";
    
    int n = sc.readInt("Số dòng dữ liệu: ");
    if (n <= 0) {
        cout << "[!] Số dòng không hợp lệ!\n";
        return;
    }
    
    int delayMs = sc.readInt("Delay giữa các dòng (ms): ");
    if (delayMs < 0) delayMs = 200;
    
    cin.ignore();
    vector<string> dataList(n);
    
    cout << "\nNhập " << n << " dòng dữ liệu:\n";
    for (int i = 0; i < n; i++) { 
        cout << "  [" << i + 1 << "]: "; 
        getline(cin, dataList[i]);
        if (dataList[i].empty()) {
            dataList[i] = "(empty)";
        }
    }
    
    cout << "\n[?] Tự động click vào ô nhập? (Y/N): ";
    string autoFocus;
    getline(cin, autoFocus);
    bool shouldClick = (autoFocus == "y" || autoFocus == "Y");
    
    cout << "\nBắt đầu sau 3 giây...\n";
    for (int i = 3; i > 0; i--) { 
        cout << " " << i << "... "; cout.flush(); 
        if (sleepWithEmergencyCheck(1000)) {
            cout << "\n[!] Đã hủy bởi người dùng.\n";
            return;
        }
    }
    cout << "\n";
    
    if (shouldClick) {
        POINT p;
        GetCursorPos(&p);
        SetCursorPos(p.x, p.y);
        Sleep(50);
        sc.leftClick();
        Sleep(100);
    }
    
    bool stopped = false;
    int executed = 0;
    for (int i = 0; i < n; i++) {
        if (SystemCore::checkEmergencyStop()) {
            stopped = true;
            break;
        }

        sc.setClipboard(dataList[i]);
        sc.pressCtrlV();
        sc.pressEnter();
        executed++;
        
        cout << "\r[+] Đã dán: " << (i + 1) << "/" << n << " ";
        cout.flush();
        
        if (sleepWithEmergencyCheck(delayMs)) {
            stopped = true;
            break;
        }
    }
    
    if (stopped) {
        cout << "\n\n[!] ĐÃ DỪNG KHẨN CẤP (Đã dán " << executed << "/" << n << " dòng)\n";
    } else {
        cout << "\n\n[✓] Hoàn thành dán " << n << " dòng!\n";
    }
}

// Trình tải & Cài đặt phần mềm tự động
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
        {"7-Zip (Giải nén)", "https://www.7-zip.org/a/7z2408-x64.exe", "7zipSetup.exe"},
        {"WinRAR", "https://www.rarlab.com/rar/winrar-x64-701.exe", "WinRARSetup.exe"},
        {"WARP 1.1.1.1", "https://1111-releases.cloudflareclient.com/windows/Cloudflare_WARP_Release-x64.msi", "CloudflareWARP.msi"},
        {"VS Code", "https://code.visualstudio.com/sha/download?build=stable&os=win32-x64-user", "VSCodeSetup.exe"},
        {"Notepad++", "https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v8.6.7/npp.8.6.7.Installer.x64.exe", "NotepadPlusPlusSetup.exe"},
        {"Git for Windows", "https://github.com/git-for-windows/git/releases/download/v2.45.1.windows.1/Git-2.45.1-64-bit.exe", "GitSetup.exe"},
    };

    sc.cls();
    
    for (size_t i = 0; i < appList.size(); i++) {
        cout << " [" << setw(2) << i + 1 << "] " << left << setw(28) << appList[i].name;
        if ((i + 1) % 2 == 0) cout << "\n";
    }
    if (appList.size() % 2 != 0) cout << "\n";
    
    cout << "\n [A] Tải toàn bộ danh sách\n";
    cout << " [H] Xem lịch sử tải\n";
    cout << " [0] Quay lại\n\n";
    cout << " -> Nhập lựa chọn: "; 
    
    string input; 
    cin >> input;
    if (input == "0") return;

    if (input == "H" || input == "h") {
        showDownloadHistory();
        return;
    }

    if (input == "A" || input == "a") { 
        cout << "\n[*] Bắt đầu tải " << appList.size() << " ứng dụng...\n";
        for (const auto& app : appList) {
            processDownload(app);
        }
        cout << "\n[✓] Hoàn thành tải tất cả!\n";
    } 
    else {
        try { 
            int idx = stoi(input) - 1; 
            if (idx >= 0 && idx < (int)appList.size()) {
                processDownload(appList[idx]);
            } else {
                cout << "[!] Lựa chọn không hợp lệ!\n";
            }
        } catch (...) { 
            cout << "[!] Lựa chọn không hợp lệ!\n"; 
        }
    }
}

void UtilityTools::processDownload(const AppInfo &app) {
    cout << "\n[+] Đang tải: " << app.name << "\n";
    cout << "    File: " << app.fileName << "\n";
    
    string tempPath = "%temp%\\" + app.fileName;
    string fullPath = string(getenv("TEMP")) + "\\" + app.fileName;
    
    if (fs::exists(fullPath)) {
        try { fs::remove(fullPath); } catch (...) {}
    }
    
    string cmd = "curl -L --progress-bar \"" + app.url + "\" -o \"" + tempPath + "\"";
    
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        cout << "    [✗] Không thể kết nối để tải!\n";
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
                    cout << "\r    Tiến độ: " << progress << "% ";
                    cout.flush();
                    lastProgress = progress;
                }
            } catch (...) {}
        }
    }
    _pclose(pipe);
    
    if (fs::exists(fullPath) && fs::file_size(fullPath) > 0) {
        auto size = fs::file_size(fullPath);
        cout << "\n    [✓] Tải thành công! (" << SystemCore::formatSize(size) << ")\n";
        
        saveDownloadHistory(app.name);
        
        cout << "    [?] Mở file cài đặt ngay? (Y/N): ";
        string answer;
        cin >> answer;
        
        if (answer == "y" || answer == "Y") {
            sc.runCMD("start \"\" \"" + fullPath + "\"");
        } else {
            cout << "    [i] Đã lưu file tại: " << fullPath << "\n";
        }
    } else {
        cout << "\n    [✗] Tải thất bại! Vui lòng kiểm tra lại kết nối mạng.\n";
    }
}

void UtilityTools::showDownloadHistory() {
    string historyFile = string(getenv("TEMP")) + "\\download_history.txt";
    ifstream history(historyFile);
    
    sc.cls();
    
    if (!history.is_open()) {
        cout << "[i] Chưa có lịch sử tải nào.\n";
        return;
    }
    
    string line;
    int count = 1;
    while (getline(history, line)) {
        cout << " [" << count++ << "] " << line << "\n";
    }
    history.close();
    cout << "\n";
}

// Gỡ bỏ ứng dụng rác Bloatware
void UtilityTools::uninstallBloatware() {
    sc.cls();
    
    string listRac = "BingWeather|BingNews|SolitaireCollection|People|PowerAutomateDesktop|"
                     "Todo|GetHelp|Getstarted|OfficeHub|SkypeApp|YourPhone|FeedbackHub|"
                     "ZuneVideo|ZuneMusic|MixedReality.Portal|Clipchamp|Disney|"
                     "MicrosoftStickyNotes|WindowsAlarms|WindowsMaps|"
                     "MicrosoftSolitaireCollection|GamingApp|XboxGamingOverlay|"
                     "DevHome|OneNote|MicrosoftTeams|Cortana|"
                     "Copilot|CandyCrush|King\\.com|SpiderSolitaire|FreeCell|"
                     "Hearts|Zone";
    
    cout << "[*] Đang quét và gỡ bỏ các ứng dụng rác mặc định...\n";
    cout << "[!] Quá trình này yêu cầu quyền Administrator\n\n";
    
    string cmd = "powershell -Command \"";
    cmd += "Get-AppxPackage -AllUsers | Where-Object {$_.Name -match '" + listRac + "'} | Remove-AppxPackage -AllUsers; ";
    cmd += "Get-AppxProvisionedPackage -Online | Where-Object {$_.DisplayName -match '" + listRac + "'} | Remove-AppxProvisionedPackage -Online";
    cmd += "\"";
    
    sc.runAdmin(cmd, true);
    cout << "\n[✓] Đã dọn sạch toàn bộ Bloatware khỏi hệ thống!\n";
}
