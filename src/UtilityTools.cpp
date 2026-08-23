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
    cout << "Nhấn ESC hoặc F6 để dừng khẩn cấp.\n\n";
    
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
    sc.waitEnter();
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
        sc.waitEnter();
        return; 
    }
    
    int times = sc.readInt("Số lần gửi: ");
    if (times <= 0) {
        cout << "Số lần không hợp lệ!\n";
        sc.waitEnter();
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
            sc.waitEnter();
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
    sc.waitEnter();
}

// Tự động paste danh sách dữ liệu
void UtilityTools::autoPasteData() {
    sc.cls();
    cout << "Nhấn ESC hoặc F6 bất kỳ lúc nào để dừng khẩn cấp.\n\n";
    
    int n = sc.readInt("Số dòng dữ liệu: ");
    if (n <= 0) {
        cout << "Số dòng không hợp lệ!\n";
        sc.waitEnter();
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
            sc.waitEnter();
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
    sc.waitEnter();
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

static string resolveAppConfigPath() {
    if (fs::exists("src/apps.txt")) return "src/apps.txt";
    if (fs::exists("apps.txt")) return "apps.txt";
    return "src/apps.txt";
}

static vector<AppItem> loadAppsFromTxt(const string &filePath) {
    vector<AppItem> list;
    if (!fs::exists(filePath)) return list;

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

// Trình tải & Cài đặt phần mềm tự động (Đọc từ file src/apps.txt)
void UtilityTools::downloadManager() {
    char* userProf = getenv("USERPROFILE");
    string downloadDir = userProf ? (string(userProf) + "\\Downloads") : "C:\\Downloads";
    if (!fs::exists(downloadDir)) {
        try { fs::create_directories(downloadDir); } catch (...) {}
    }

    string configPath = resolveAppConfigPath();
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
             << "  [A] Tải tất cả ứng dụng          [R] Nạp lại danh sách\n"
             << "  [0] Quay lại menu chính\n"
             << "======================================================================\n"
             << "Chọn thao tác: ";

        string choice;
        cin >> choice;

        if (choice == "0") break;

        if (choice == "R" || choice == "r") {
            configPath = resolveAppConfigPath();
            apps = loadAppsFromTxt(configPath);
            cout << "\nĐã nạp lại file " << configPath << " (" << apps.size() << " ứng dụng)!\n";
            Sleep(800);
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

            for (size_t i = 0; i < apps.size(); ++i) {
                cout << "----------------------------------------------------------------------\n"
                     << "[" << (i + 1) << "/" << apps.size() << "] Đang tải: " << apps[i].name << "...\n";

                string targetPath = downloadDir + "\\" + apps[i].fileName;
                string cmd = "curl -# -L \"" + apps[i].url + "\" -o \"" + targetPath + "\"";
                int ret = system(cmd.c_str());

                if (ret == 0 && fs::exists(targetPath)) {
                    cout << "  [✓] Hoàn tất: " << apps[i].fileName << "\n";
                    successCount++;
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

// Gỡ bỏ ứng dụng rác Bloatware (Bao gồm Phone Link, Cross Device & game/app quảng cáo cài sẵn)
void UtilityTools::uninstallBloatware() {
    sc.cls();
    
    // Danh sách các ứng dụng rác
    const std::vector<std::pair<std::string, std::string>> bloatList = {
        {"Microsoft.YourPhone", "Liên kết điện thoại (Phone Link)"},
        {"MicrosoftWindows.CrossDevice", "Trải nghiệm liên kết thiết bị (Cross Device)"},
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
         << "Chế độ: Gỡ bỏ chính xác theo tên gói & khóa dịch vụ ngầm\n"
         << "        (Bảo vệ Calculator, StickyNotes, Clock/Alarms, Media Player, Xbox Game Bar)\n\n"
         << "Tổng số gói rác được quét: " << bloatList.size() << " ứng dụng.\n\n";

    cout << "Bạn có chắc chắn muốn quét và gỡ bỏ toàn bộ danh sách rác trên? (y/n): ";
    string confirm;
    cin >> confirm;
    if (confirm != "y" && confirm != "Y") {
        cout << "\nĐã hủy thao tác.\n";
        return;
    }

    cout << "\nĐang tiến hành gỡ bỏ an toàn từng ứng dụng và khóa dịch vụ ngầm...\n\n";

    string psScript = "";
    // 1. Tắt tiến trình Phone Link & Cross Device đang chạy
    psScript += "Stop-Process -Name 'CrossDeviceService','PhoneExperienceHost' -Force -ErrorAction SilentlyContinue;\n";

    // 2. Gỡ bỏ các gói Appx và Provisioned Package
    psScript += "$apps = @(\n";
    for (size_t i = 0; i < bloatList.size(); ++i) {
        psScript += "    '" + bloatList[i].first + "'";
        if (i + 1 < bloatList.size()) psScript += ",";
        psScript += "\n";
    }
    psScript += ");\n";
    psScript += "foreach ($pkg in $apps) {\n";
    psScript += "    Get-AppxPackage -AllUsers -Name ('*' + $pkg + '*') -ErrorAction SilentlyContinue | Remove-AppxPackage -AllUsers -ErrorAction SilentlyContinue;\n";
    psScript += "    Get-AppxProvisionedPackage -Online | Where-Object { $_.DisplayName -like ('*' + $pkg + '*') -or $_.PackageName -like ('*' + $pkg + '*') } | Remove-AppxProvisionedPackage -Online -ErrorAction SilentlyContinue;\n";
    psScript += "}\n";

    // 3. Khóa hoàn toàn Service và Group Policy liên kết thiết bị
    psScript += "Stop-Service -Name 'CDPUserSvc*' -Force -ErrorAction SilentlyContinue;\n";
    psScript += "Set-ItemProperty -Path 'HKLM:\\SYSTEM\\CurrentControlSet\\Services\\CDPUserSvc' -Name 'Start' -Value 4 -Type DWord -Force -ErrorAction SilentlyContinue;\n";
    psScript += "New-Item -Path 'HKLM:\\SOFTWARE\\Policies\\Microsoft\\Windows\\System' -Force -ErrorAction SilentlyContinue | Out-Null;\n";
    psScript += "Set-ItemProperty -Path 'HKLM:\\SOFTWARE\\Policies\\Microsoft\\Windows\\System' -Name 'EnableMmx' -Value 0 -Type DWord -Force -ErrorAction SilentlyContinue;\n";

    // 4. Gỡ bỏ triệt để Microsoft OneDrive & xóa tàn dư / icon Explorer
    psScript += "Stop-Process -Name 'OneDrive' -Force -ErrorAction SilentlyContinue;\n";
    psScript += "if (Test-Path \"$env:SystemRoot\\SysWOW64\\OneDriveSetup.exe\") {\n";
    psScript += "    Start-Process \"$env:SystemRoot\\SysWOW64\\OneDriveSetup.exe\" -ArgumentList '/uninstall' -Wait -NoNewWindow -ErrorAction SilentlyContinue;\n";
    psScript += "} elseif (Test-Path \"$env:SystemRoot\\System32\\OneDriveSetup.exe\") {\n";
    psScript += "    Start-Process \"$env:SystemRoot\\System32\\OneDriveSetup.exe\" -ArgumentList '/uninstall' -Wait -NoNewWindow -ErrorAction SilentlyContinue;\n";
    psScript += "} elseif (Test-Path \"$env:LocalAppData\\Microsoft\\OneDrive\\Update\\OneDriveSetup.exe\") {\n";
    psScript += "    Start-Process \"$env:LocalAppData\\Microsoft\\OneDrive\\Update\\OneDriveSetup.exe\" -ArgumentList '/uninstall' -Wait -NoNewWindow -ErrorAction SilentlyContinue;\n";
    psScript += "}\n";
    psScript += "Remove-Item -Path \"$env:LocalAppData\\Microsoft\\OneDrive\" -Recurse -Force -ErrorAction SilentlyContinue;\n";
    psScript += "Remove-Item -Path \"$env:ProgramData\\Microsoft OneDrive\" -Recurse -Force -ErrorAction SilentlyContinue;\n";
    psScript += "Remove-Item -Path \"C:\\OneDriveTemp\" -Recurse -Force -ErrorAction SilentlyContinue;\n";
    psScript += "Remove-Item -Path 'HKCR:\\CLSID\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}' -Recurse -Force -ErrorAction SilentlyContinue;\n";
    psScript += "Remove-Item -Path 'HKCR:\\Wow6432Node\\CLSID\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}' -Recurse -Force -ErrorAction SilentlyContinue;\n";

    string cmd = "powershell -NoProfile -Command \"" + psScript + "\"";
    sc.runAdmin(cmd, true);

    for (const auto &item : bloatList) {
        cout << "  [✓] Đã dọn sạch: " << left << setw(45) << item.second << " [" << item.first << "]\n";
    }
    cout << "  [✓] Đã dọn sạch: " << left << setw(45) << "Gỡ bỏ tận gốc Microsoft OneDrive" << " [OneDriveSetup /uninstall]\n";
    cout << "  [✓] Đã khóa:   " << left << setw(45) << "Dịch vụ liên kết CDPUserSvc & Group Policy" << " [CDPUserSvc/EnableMmx]\n";
    
    sc.waitEnter();
}

static string getXmlTag(const string &xml, const string &tag) {
    string openTag = "<" + tag + ">";
    string closeTag = "</" + tag + ">";
    size_t start = xml.find(openTag);
    if (start == string::npos) return "";
    start += openTag.length();
    size_t end = xml.find(closeTag, start);
    if (end == string::npos) return "";
    return xml.substr(start, end - start);
}

static string renderBar(double percent, int width = 20) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    int filled = (int)((percent / 100.0) * width);
    string bar = "[";
    for (int i = 0; i < filled; ++i) bar += "■";
    for (int i = filled; i < width; ++i) bar += " ";
    bar += "]";
    return bar;
}

static string formatNumber(long long n) {
    string s = to_string(n);
    int insertPosition = (int)s.length() - 3;
    while (insertPosition > 0) {
        s.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    return s;
}

// Soi thông tin & Độ chai Pin Laptop chuyên sâu
void UtilityTools::batteryHealthDiagnostic() {
    while (true) {
        sc.cls();
        cout << "======================================================================\n"
             << "               CHẨN ĐOÁN & SOI ĐỘ CHAI PIN LAPTOP\n"
             << "======================================================================\n"
             << "Đang đọc dữ liệu vi điều khiển Pin từ Windows ACPI...\n";

        SYSTEM_POWER_STATUS sps;
        bool hasSps = GetSystemPowerStatus(&sps);

        // Kiểm tra thiết bị có pin không
        if (hasSps && (sps.BatteryFlag == 128 || sps.BatteryFlag == 255) && sps.BatteryLifePercent == 255) {
            sc.cls();
            cout << "======================================================================\n"
                 << "               CHẨN ĐOÁN & SOI ĐỘ CHAI PIN LAPTOP\n"
                 << "======================================================================\n\n"
                 << " [!] THIẾT BỊ KHÔNG CÓ PIN (MÁY BÀN / PC DESKTOP)\n\n"
                 << "  - Hệ thống nhận diện đây là máy tính bàn hoặc Pin Laptop đã bị tháo rời.\n"
                 << "  - Nguồn điện hiện tại: Cắm nguồn trực tiếp qua Adapter / Bộ nguồn AC.\n\n"
                 << "======================================================================\n";
            sc.waitEnter();
            return;
        }

        // Tạo file XML báo cáo pin tạm thời
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        string xmlPath = string(tempPath) + "cmd_battery_report.xml";
        string cmd = "powercfg /batteryreport /xml /output \"" + xmlPath + "\" >nul 2>&1";
        system(cmd.c_str());

        string manufacturer = "N/A", deviceName = "N/A", serial = "N/A", chemistry = "Li-ion";
        string sysMfg = "N/A", sysModel = "N/A", biosVer = "N/A";
        long long designCap = 0, fullCap = 0, cycleCount = 0;

        if (fs::exists(xmlPath)) {
            ifstream f(xmlPath);
            if (f.is_open()) {
                stringstream ss;
                ss << f.rdbuf();
                string xml = ss.str();
                f.close();

                sysMfg = getXmlTag(xml, "SystemManufacturer");
                sysModel = getXmlTag(xml, "SystemProductName");
                biosVer = getXmlTag(xml, "BIOSVersion");

                size_t batPos = xml.find("<Batteries>");
                if (batPos != string::npos) {
                    string batXml = xml.substr(batPos);
                    deviceName = getXmlTag(batXml, "Id");
                    manufacturer = getXmlTag(batXml, "Manufacturer");
                    serial = getXmlTag(batXml, "SerialNumber");
                    string chem = getXmlTag(batXml, "Chemistry");
                    if (!chem.empty()) chemistry = chem;

                    string dcStr = getXmlTag(batXml, "DesignCapacity");
                    string fcStr = getXmlTag(batXml, "FullChargeCapacity");
                    string ccStr = getXmlTag(batXml, "CycleCount");

                    if (!dcStr.empty()) try { designCap = stoll(dcStr); } catch (...) {}
                    if (!fcStr.empty()) try { fullCap = stoll(fcStr); } catch (...) {}
                    if (!ccStr.empty()) try { cycleCount = stoll(ccStr); } catch (...) {}
                }
            }
            try { fs::remove(xmlPath); } catch (...) {}
        }

        sc.cls();
        cout << "======================================================================\n"
             << "               CHẨN ĐOÁN & SOI ĐỘ CHAI PIN LAPTOP CHUYÊN SÂU\n"
             << "======================================================================\n";

        if (!sysMfg.empty() && sysMfg != "N/A") {
            cout << "Thiết bị        : " << sysMfg << " " << sysModel << " (BIOS: " << biosVer << ")\n";
        }
        if (!deviceName.empty() && deviceName != "N/A") {
            cout << "Loại Pin        : " << chemistry << " - " << manufacturer << " [" << deviceName << "]\n";
            if (!serial.empty() && serial != "N/A" && serial != "") {
                cout << "Số Seri Pin     : " << serial << "\n";
            }
        }
        cout << "----------------------------------------------------------------------\n";

        if (designCap > 0 && fullCap > 0) {
            double healthPercent = ((double)fullCap / (double)designCap) * 100.0;
            if (healthPercent > 100.0) healthPercent = 100.0;
            double wearPercent = 100.0 - healthPercent;
            long long lostCap = designCap - fullCap;
            if (lostCap < 0) lostCap = 0;

            cout << "[THÔNG SỐ DUNG LƯỢNG & ĐỘ CHAI PIN]\n"
                 << "  + Dung lượng thiết kế (Design)    : " << setw(10) << right << formatNumber(designCap) << " mWh\n"
                 << "  + Dung lượng khi nạp đầy (Full)   : " << setw(10) << right << formatNumber(fullCap) << " mWh\n"
                 << "  + Dung lượng bị hao hụt           : " << setw(10) << right << formatNumber(lostCap) << " mWh\n"
                 << "  + Số chu kỳ sạc (Cycle Count)     : " << setw(10) << right << cycleCount << " lần\n"
                 << "  + Sức khỏe Pin (Battery Health)   : " << fixed << setprecision(1) << healthPercent << "%  " << renderBar(healthPercent) << "\n"
                 << "  + Độ chai Pin (Wear Level)        : " << fixed << setprecision(1) << wearPercent << "%  ";

            if (wearPercent < 5.0) {
                cout << "(Hoàn hảo - Như pin mới 100%)\n";
            } else if (wearPercent < 15.0) {
                cout << "(Rất tốt - Hoạt động lý tưởng)\n";
            } else if (wearPercent < 30.0) {
                cout << "(Bình thường - Bắt đầu có dấu hiệu lão hóa)\n";
            } else if (wearPercent < 50.0) {
                cout << "(Chai đáng kể - Thời lượng dùng giảm rõ rệt)\n";
            } else {
                cout << "(Chai nặng / Hư hại - Nên cân nhắc thay thế Pin)\n";
            }
        } else {
            cout << "[THÔNG SỐ DUNG LƯỢNG]\n"
                 << "  [!] Không thể đọc chỉ số ACPI nâng cao (Có thể do Driver hoặc thiết bị ảo).\n";
        }

        cout << "----------------------------------------------------------------------\n"
             << "[TRẠNG THÁI NGUỒN HIỆN TẠI (REALTIME)]\n";

        if (hasSps) {
            string powerSource = "Không xác định";
            if (sps.ACLineStatus == 1) powerSource = "Đang cắm sạc (AC Online)";
            else if (sps.ACLineStatus == 0) powerSource = "Đang dùng nguồn Pin (Battery/DC)";

            int batPct = (int)sps.BatteryLifePercent;
            cout << "  + Nguồn điện                      : " << powerSource << "\n";
            if (batPct >= 0 && batPct <= 100) {
                cout << "  + Mức pin hiện tại                : " << batPct << "%  " << renderBar(batPct) << "\n";
            }

            string chargeStatus = "Bình thường";
            if (sps.BatteryFlag & 8) chargeStatus = "Đang sạc pin (Charging...)";
            else if (sps.ACLineStatus == 1 && batPct >= 95) chargeStatus = "Đã sạc đầy (Fully Charged)";
            else if (sps.BatteryFlag & 4) chargeStatus = "Pin cực yếu (Critical)";
            else if (sps.BatteryFlag & 2) chargeStatus = "Pin yếu (Low)";
            cout << "  + Trạng thái sạc                  : " << chargeStatus << "\n";

            if (sps.BatteryLifeTime != (DWORD)-1 && sps.ACLineStatus == 0) {
                int hours = sps.BatteryLifeTime / 3600;
                int mins = (sps.BatteryLifeTime % 3600) / 60;
                cout << "  + Thời lượng ước tính còn lại     : ~ " << hours << " giờ " << mins << " phút\n";
            }
        }

        cout << "----------------------------------------------------------------------\n"
             << "[LỜI KHUYÊN DÀNH CHO BẠN]\n";
        if (designCap > 0 && fullCap > 0) {
            double wear = 100.0 - (((double)fullCap / (double)designCap) * 100.0);
            if (wear < 15.0) {
                cout << "  ✓ Pin trong tình trạng xuất sắc. Để giữ pin bền lâu, tránh để máy quá nóng\n"
                     << "    và duy trì mức sạc từ 20% - 80% khi cắm sạc làm việc liên tục.\n";
            } else if (wear < 40.0) {
                cout << "  ! Pin đã có độ chai tự nhiên theo thời gian. Nên bật chế độ Battery Saver\n"
                     << "    khi làm việc di động và hạn chế vừa chơi game nặng vừa sạc.\n";
            } else {
                cout << "  ⚠ Cảnh báo: Pin đã chai trên " << (int)wear << "%. Có thể sập nguồn đột ngột khi tải nặng.\n"
                     << "    Khuyến nghị kiểm tra và thay thế cell pin mới tại trung tâm bảo hành.\n";
            }
        } else {
            cout << "  ✓ Sử dụng bộ sạc chính hãng kèm máy để đảm bảo dòng điện và bảo vệ mạch sạc.\n";
        }

        cout << "======================================================================\n"
             << "  [1] Xuất & mở báo cáo đồ thị Battery Report (HTML) trên Trình duyệt\n"
             << "  [2] Mở cài đặt Quản lý Pin & Nguồn của Windows (Power & Battery)\n"
             << "  [R] Làm mới lại dữ liệu chẩn đoán\n"
             << "  [0] Quay lại menu chính\n"
             << "======================================================================\n"
             << "Chọn thao tác: ";

        string opt;
        cin >> opt;

        if (opt == "0") break;

        if (opt == "1") {
            cout << "\nĐang xuất báo cáo đồ thị HTML chuẩn của Windows...\n";
            char tempHtml[MAX_PATH];
            GetTempPathA(MAX_PATH, tempHtml);
            string htmlPath = string(tempHtml) + "battery_report.html";
            string genCmd = "powercfg /batteryreport /output \"" + htmlPath + "\" >nul 2>&1";
            system(genCmd.c_str());

            if (fs::exists(htmlPath)) {
                ShellExecuteA(NULL, "open", htmlPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                cout << "  [✓] Đã mở báo cáo: " << htmlPath << "\n";
            } else {
                cout << "  [!] Không thể xuất file báo cáo HTML.\n";
            }
            Sleep(1200);
            continue;
        }

        if (opt == "2") {
            ShellExecuteA(NULL, "open", "ms-settings:batterysaver", NULL, NULL, SW_SHOWNORMAL);
            cout << "\nĐã mở cài đặt Pin của Windows...\n";
            Sleep(1000);
            continue;
        }

        if (opt == "R" || opt == "r") {
            continue;
        }
    }
}

