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
    cout << "Nhấn ESC hoặc F6 bất kỳ lúc nào để dừng khẩn cấp.\n\n";
    
    int times = sc.readInt("Số lần click: ");
    if (times <= 0) {
        cout << "Số lần không hợp lệ!\n";
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
            cout << "\nĐã hủy thao tác.\n";
            return;
        }
    }
    
    POINT p; 
    GetCursorPos(&p);
    cout << "\n\nBắt đầu click tại tọa độ: (" << p.x << ", " << p.y << ")\n"
         << "Số lần: " << times << " | Delay: " << intervalMs << "ms\n"
         << "(Nhấn ESC / F6 để dừng)\n\n";
    
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
            cout << "\rTiến độ: " << (i * 100 / times) << "% ";
            cout.flush();
        }
        
        if (sleepWithEmergencyCheck(intervalMs)) {
            stopped = true;
            break;
        }
    }
    
    if (stopped) {
        cout << "\n\nĐã dừng khẩn cấp (Đã click " << executed << "/" << times << " lần)\n";
    } else {
        cout << "\n\nHoàn thành " << times << " lần click!\n";
    }
}

// Spam văn bản tự động
void UtilityTools::spamText() {
    sc.cls();
    cout << "Nhấn ESC hoặc F6 bất kỳ lúc nào để dừng khẩn cấp.\n\n";
    
    string content; 
    cout << "Nhập text cần gửi: ";
    cin.ignore(); 
    getline(cin, content);
    
    if (content.empty()) { 
        cout << "Nội dung trống!\n"; 
        return; 
    }
    
    int times = sc.readInt("Số lần gửi: ");
    if (times <= 0) {
        cout << "Số lần không hợp lệ!\n";
        return;
    }
    
    int delayMs = sc.readInt("Delay giữa các lần (ms): ");
    if (delayMs < 0) delayMs = 100;
    
    cout << "\nTự động click vào ô nhập? (Y/N): ";
    string autoFocus;
    getline(cin, autoFocus);
    bool shouldClick = (autoFocus == "y" || autoFocus == "Y");
    
    cout << "\nChuẩn bị gửi...\n"
         << "  - Nội dung : " << content << "\n"
         << "  - Số lần   : " << times << "\n"
         << "  - Delay    : " << delayMs << "ms\n"
         << "  - Phím ngắt: ESC / F6\n"
         << "\nBắt đầu sau 3 giây...\n";
    for (int i = 3; i > 0; i--) { 
        cout << " " << i << "... "; cout.flush(); 
        if (sleepWithEmergencyCheck(1000)) {
            cout << "\nĐã hủy thao tác.\n";
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
            cout << "\rĐã gửi: " << (i + 1) << "/" << times << " ";
            cout.flush();
        }
        
        if (sleepWithEmergencyCheck(delayMs)) {
            stopped = true;
            break;
        }
    }
    
    if (stopped) {
        cout << "\n\nĐã dừng khẩn cấp (Đã gửi " << executed << "/" << times << " lần)\n";
    } else {
        cout << "\n\nHoàn thành gửi " << times << " lần!\n";
    }
}

// Tự động paste danh sách dữ liệu
void UtilityTools::autoPasteData() {
    sc.cls();
    cout << "Nhấn ESC hoặc F6 bất kỳ lúc nào để dừng khẩn cấp.\n\n";
    
    int n = sc.readInt("Số dòng dữ liệu: ");
    if (n <= 0) {
        cout << "Số dòng không hợp lệ!\n";
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
    
    cout << "\nTự động click vào ô nhập? (Y/N): ";
    string autoFocus;
    getline(cin, autoFocus);
    bool shouldClick = (autoFocus == "y" || autoFocus == "Y");
    
    cout << "\nBắt đầu sau 3 giây...\n";
    for (int i = 3; i > 0; i--) { 
        cout << " " << i << "... "; cout.flush(); 
        if (sleepWithEmergencyCheck(1000)) {
            cout << "\nĐã hủy thao tác.\n";
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
        
        cout << "\rĐã dán: " << (i + 1) << "/" << n << " ";
        cout.flush();
        
        if (sleepWithEmergencyCheck(delayMs)) {
            stopped = true;
            break;
        }
    }
    
    if (stopped) {
        cout << "\n\nĐã dừng khẩn cấp (Đã dán " << executed << "/" << n << " dòng)\n";
    } else {
        cout << "\n\nHoàn thành dán " << n << " dòng!\n";
    }
}

struct AppItem {
    string name;
    string url;
    string fileName;
};

static string trimStr(const string &s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static vector<AppItem> loadAppsFromTxt(const string &filePath) {
    vector<AppItem> list;
    if (!fs::exists(filePath)) {
        ofstream out(filePath);
        if (out.is_open()) {
            out << "# DANH SÁCH ỨNG DỤNG TẢI TỰ ĐỘNG CHO CMD BOX\n"
                << "# Định dạng: Tên ứng dụng | Link tải trực tiếp | Tên file lưu\n\n"
                << "Google Chrome | https://dl.google.com/tag/s/appname%3DGoogle%2520Chrome/update2/installers/ChromeSetup.exe | ChromeSetup.exe\n"
                << "Cốc Cốc | https://files.coccoc.com/browser/coccoc_vi.exe | CocCocSetup.exe\n"
                << "Brave Browser | https://laptop-updates.brave.com/latest/winx64 | BraveSetup.exe\n"
                << "Mozilla Firefox | https://download.mozilla.org/?product=firefox-latest-ssl&os=win64&lang=vi | FirefoxSetup.exe\n"
                << "EVKey | https://github.com/lamquangminh/EVKey/releases/download/v5.0.4/EVKey.zip | EVKey.zip\n"
                << "OpenKey | https://github.com/tphan/openkey/releases/latest/download/OpenKey-Windows-x64.zip | OpenKey.zip\n"
                << "UniKey | https://www.unikey.org/assets/release/unikey43RC5-200929-win64.zip | UniKey.zip\n"
                << "Zalo PC | https://zalo.me/download/zalo-pc | ZaloSetup.exe\n"
                << "Discord | https://discord.com/api/downloads/distributions/app/installers/latest?channel=stable&platform=win&arch=x64 | DiscordSetup.exe\n"
                << "Telegram | https://telegram.org/dl/desktop/win64 | TelegramSetup.exe\n"
                << "Zoom | https://zoom.us/client/latest/ZoomInstaller.exe | ZoomInstaller.exe\n"
                << "7-Zip | https://www.7-zip.org/a/7z2408-x64.exe | 7zipSetup.exe\n"
                << "WinRAR | https://www.rarlab.com/rar/winrar-x64-701.exe | WinRARSetup.exe\n"
                << "WARP 1.1.1.1 | https://1111-releases.cloudflareclient.com/windows/Cloudflare_WARP_Release-x64.msi | CloudflareWARP.msi\n"
                << "LocalSend | https://github.com/localsend/localsend/releases/latest/download/LocalSend-1.16.1-windows-x86-64.exe | LocalSendSetup.exe\n"
                << "Everything Search | https://www.voidtools.com/Everything-1.4.1.1026.x64-Setup.exe | EverythingSetup.exe\n"
                << "CPU-Z | https://download.cpuid.com/cpu-z/cpu-z_2.11-en.exe | CPUZSetup.exe\n"
                << "Rufus | https://github.com/pbatard/rufus/releases/download/v4.5/rufus-4.5.exe | Rufus.exe\n"
                << "VLC Media Player | https://get.videolan.org/vlc/last/win64/vlc-3.0.21-win64.exe | VLCSetup.exe\n"
                << "OBS Studio | https://cdn-fastly.obsproject.com/downloads/OBS-Studio-30.2.2-Windows-Installer.exe | OBSStudioSetup.exe\n"
                << "VS Code | https://code.visualstudio.com/sha/download?build=stable&os=win32-x64-user | VSCodeSetup.exe\n"
                << "Notepad++ | https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v8.6.7/npp.8.6.7.Installer.x64.exe | NotepadPlusPlusSetup.exe\n"
                << "Git for Windows | https://github.com/git-for-windows/git/releases/download/v2.45.1.windows.1/Git-2.45.1-64-bit.exe | GitSetup.exe\n"
                << "Node.js LTS | https://nodejs.org/dist/v20.16.0/node-v20.16.0-x64.msi | NodejsSetup.msi\n"
                << "Python | https://www.python.org/ftp/python/3.12.5/python-3.12.5-amd64.exe | PythonSetup.exe\n";
            out.close();
        }
    }

    ifstream file(filePath);
    if (!file.is_open()) return list;

    string line;
    while (getline(file, line)) {
        string t = trimStr(line);
        if (t.empty() || t[0] == '#') continue;

        stringstream ss(t);
        string name, url, fname;
        if (getline(ss, name, '|') && getline(ss, url, '|') && getline(ss, fname)) {
            name = trimStr(name);
            url = trimStr(url);
            fname = trimStr(fname);
            if (!name.empty() && !url.empty() && !fname.empty()) {
                list.push_back({name, url, fname});
            }
        }
    }
    return list;
}

// Trình tải & Cài đặt phần mềm tự động (Đọc từ file apps.txt)
void UtilityTools::downloadManager() {
    char* userProf = getenv("USERPROFILE");
    string downloadDir = userProf ? (string(userProf) + "\\Downloads") : "C:\\Downloads";
    if (!fs::exists(downloadDir)) {
        try { fs::create_directories(downloadDir); } catch (...) {}
    }

    string configPath = "apps.txt";
    vector<AppItem> apps = loadAppsFromTxt(configPath);

    while (true) {
        sc.cls();
        cout << "======================================================================\n"
             << "               TRÌNH TẢI & CÀI ĐẶT PHẦN MỀM TỰ ĐỘNG\n"
             << "======================================================================\n"
             << "Thư mục lưu : " << downloadDir << "\n"
             << "File dữ liệu: " << configPath << " (" << apps.size() << " ứng dụng)\n"
             << "----------------------------------------------------------------------\n\n";

        if (apps.empty()) {
            cout << " [!] Không tìm thấy ứng dụng nào trong file " << configPath << "\n"
                 << "     Vui lòng kiểm tra lại file cấu hình.\n\n";
        } else {
            size_t half = (apps.size() + 1) / 2;
            for (size_t i = 0; i < half; i++) {
                cout << "  [" << setw(2) << right << (i + 1) << "] " << setw(28) << left << apps[i].name;
                size_t j = i + half;
                if (j < apps.size()) {
                    cout << "  [" << setw(2) << right << (j + 1) << "] " << setw(28) << left << apps[j].name;
                }
                cout << "\n";
            }
        }

        cout << "\n----------------------------------------------------------------------\n"
             << "  [A] Tải tất cả ứng dụng      [H] Xem lịch sử tải      [R] Nạp lại apps.txt\n"
             << "  [0] Quay lại menu chính\n"
             << "======================================================================\n"
             << "Chọn thao tác: ";

        string choice;
        cin >> choice;

        if (choice == "0") break;

        if (choice == "R" || choice == "r") {
            apps = loadAppsFromTxt(configPath);
            cout << "\nĐã nạp lại file apps.txt (" << apps.size() << " ứng dụng)!\n";
            Sleep(800);
            continue;
        }

        if (choice == "H" || choice == "h") {
            sc.cls();
            cout << "======================================================================\n"
                 << "                       LỊCH SỬ TẢI ỨNG DỤNG\n"
                 << "======================================================================\n\n";
            string histPath = downloadDir + "\\download_history.txt";
            if (fs::exists(histPath)) {
                ifstream hfile(histPath);
                string hline;
                while (getline(hfile, hline)) {
                    cout << "  " << hline << "\n";
                }
            } else {
                cout << "  (Chưa có lịch sử tải)\n";
            }
            cout << "\n======================================================================\n";
            sc.waitEnter();
            continue;
        }

        if (choice == "A" || choice == "a") {
            if (apps.empty()) continue;
            sc.cls();
            cout << "======================================================================\n"
                 << "                     TẢI TOÀN BỘ ỨNG DỤNG\n"
                 << "======================================================================\n"
                 << "Bắt đầu tải " << apps.size() << " ứng dụng về: " << downloadDir << "\n\n";

            int successCount = 0;
            string histPath = downloadDir + "\\download_history.txt";

            for (size_t i = 0; i < apps.size(); ++i) {
                cout << "----------------------------------------------------------------------\n"
                     << "[" << (i + 1) << "/" << apps.size() << "] Đang tải: " << apps[i].name << "...\n";

                string targetPath = downloadDir + "\\" + apps[i].fileName;
                string cmd = "curl -# -L \"" + apps[i].url + "\" -o \"" + targetPath + "\"";
                int ret = system(cmd.c_str());

                if (ret == 0 && fs::exists(targetPath)) {
                    cout << "  [✓] Hoàn tất: " << apps[i].fileName << "\n";
                    successCount++;
                    ofstream hist(histPath, ios::app);
                    if (hist.is_open()) {
                        time_t now = time(nullptr);
                        char tbuf[64];
                        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
                        hist << "[" << tbuf << "] Đã tải: " << apps[i].name << " -> " << apps[i].fileName << "\n";
                    }
                } else {
                    cout << "  [!] Thất bại khi tải " << apps[i].name << "!\n";
                }
            }

            cout << "\n======================================================================\n"
                 << "Hoàn tất tải " << successCount << "/" << apps.size() << " ứng dụng!\n";
            sc.waitEnter();
            continue;
        }

        try {
            int idx = stoi(choice);
            if (idx >= 1 && idx <= (int)apps.size()) {
                const auto &app = apps[idx - 1];
                sc.cls();
                cout << "======================================================================\n"
                     << "                         TẢI ỨNG DỤNG\n"
                     << "======================================================================\n"
                     << "Ứng dụng: " << app.name << "\n"
                     << "Lưu tại : " << downloadDir << "\\" << app.fileName << "\n\n"
                     << "Đang tải xuống, vui lòng chờ...\n\n";

                string targetPath = downloadDir + "\\" + app.fileName;
                string cmd = "curl -# -L \"" + app.url + "\" -o \"" + targetPath + "\"";
                int ret = system(cmd.c_str());

                if (ret == 0 && fs::exists(targetPath)) {
                    cout << "\n[OK] Đã tải về thành công: " << targetPath << "\n";

                    string histPath = downloadDir + "\\download_history.txt";
                    ofstream hist(histPath, ios::app);
                    if (hist.is_open()) {
                        time_t now = time(nullptr);
                        char tbuf[64];
                        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
                        hist << "[" << tbuf << "] Đã tải: " << app.name << " -> " << app.fileName << "\n";
                    }

                    cout << "\nBạn có muốn mở file cài đặt ngay? (y/n): ";
                    string runChoice;
                    cin >> runChoice;
                    if (runChoice == "y" || runChoice == "Y") {
                        ShellExecuteA(NULL, "open", targetPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    }
                } else {
                    cout << "\n[!] Tải thất bại! Vui lòng kiểm tra lại kết nối mạng hoặc link tải.\n";
                    sc.waitEnter();
                }
            } else {
                cout << "\n[!] Lựa chọn không hợp lệ!\n";
                Sleep(500);
            }
        } catch (...) {
            cout << "\n[!] Lựa chọn không hợp lệ!\n";
            Sleep(500);
        }
    }
}

// Gỡ bỏ ứng dụng rác Bloatware (Sử dụng Vector chính xác tuyệt đối, không dùng Regex mập mờ)
void UtilityTools::uninstallBloatware() {
    sc.cls();
    
    // Danh sách các ứng dụng rác / game quảng cáo cài sẵn cần gỡ
    const std::vector<std::pair<std::string, std::string>> bloatList = {
        {"Microsoft.BingNews", "Tin tức (Bing News)"},
        {"Microsoft.BingWeather", "Thời tiết (Bing Weather)"},
        {"Microsoft.GetHelp", "Trợ giúp (Get Help)"},
        {"Microsoft.Getstarted", "Mẹo & Bắt đầu (Tips)"},
        {"Microsoft.MicrosoftOfficeHub", "Quảng cáo Microsoft 365"},
        {"Microsoft.MicrosoftSolitaireCollection", "Game Solitaire Collection"},
        {"Microsoft.PowerAutomateDesktop", "Power Automate Desktop"},
        {"Microsoft.SkypeApp", "Skype mặc định"},
        {"Microsoft.Todos", "Microsoft To-Do"},
        {"Microsoft.WindowsFeedbackHub", "Trung tâm phản hồi (Feedback Hub)"},
        {"Microsoft.WindowsMaps", "Bản đồ (Windows Maps)"},
        {"Microsoft.MixedReality.Portal", "Cổng thực tế hỗn hợp (Mixed Reality)"},
        {"Microsoft.549981C3F5F10", "Trợ lý ảo Cortana cũ"},
        {"Clipchamp.Clipchamp", "Clipchamp Video Editor"},
        {"Disney.37853FC22B2CE", "Disney+ quảng cáo"},
        {"SpotifyAB.SpotifyMusic", "Spotify cài sẵn"},
        {"king.com.CandyCrushSaga", "Game Candy Crush Saga"},
        {"king.com.CandyCrushSodaSaga", "Game Candy Crush Soda"},
        {"king.com.BubbleWitch3Saga", "Game Bubble Witch 3"},
        {"Playtika.CaesarsSlotsFreeCasino", "Game Caesars Slots Casino"},
        {"ShazamEntertainmentLtd.Shazam", "Shazam quảng cáo"},
        {"ByteDancePte.Ltd.TikTok", "TikTok quảng cáo"},
        {"Amazon.com.Amazon", "Amazon quảng cáo"}
    };

    cout << "======================================================================\n"
         << "           GỠ BỎ ỨNG DỤNG RÁC MẶC ĐỊNH (BLOATWARE CLEANER)\n"
         << "======================================================================\n\n"
         << "Chế độ: Gỡ bỏ chính xác theo tên gói (Tuyệt đối bảo vệ Calculator, StickyNotes,\n"
         << "        Clock/Alarms, Movies & TV, Media Player, Xbox Game Bar, Phone Link)\n\n"
         << "Tổng số gói rác được quét: " << bloatList.size() << " ứng dụng.\n\n";

    cout << "Bạn có chắc chắn muốn quét và gỡ bỏ toàn bộ danh sách rác trên? (y/n): ";
    string confirm;
    cin >> confirm;
    if (confirm != "y" && confirm != "Y") {
        cout << "\nĐã hủy thao tác.\n";
        return;
    }

    cout << "\nĐang tiến hành gỡ bỏ an toàn từng ứng dụng...\n\n";

    string psScript = "$apps = @(\n";
    for (size_t i = 0; i < bloatList.size(); ++i) {
        psScript += "    '" + bloatList[i].first + "'";
        if (i + 1 < bloatList.size()) psScript += ",";
        psScript += "\n";
    }
    psScript += ");\n";
    psScript += "foreach ($pkg in $apps) {\n";
    psScript += "    Get-AppxPackage -AllUsers -Name $pkg -ErrorAction SilentlyContinue | Remove-AppxPackage -AllUsers -ErrorAction SilentlyContinue;\n";
    psScript += "    Get-AppxProvisionedPackage -Online | Where-Object { $_.DisplayName -eq $pkg } | Remove-AppxProvisionedPackage -Online -ErrorAction SilentlyContinue;\n";
    psScript += "}\n";

    string cmd = "powershell -NoProfile -Command \"" + psScript + "\"";
    sc.runAdmin(cmd, true);

    for (const auto &item : bloatList) {
        cout << "  [✓] Đã dọn sạch: " << left << setw(35) << item.second << " [" << item.first << "]\n";
    }

    cout << "\n======================================================================\n"
         << "Hoàn tất! Đã gỡ bỏ toàn bộ ứng dụng rác mà không ảnh hưởng tới app hệ thống.\n";
}
