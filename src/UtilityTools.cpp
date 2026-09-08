#include "../include/UtilityTools.h"
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <iomanip>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

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
    
    int times = sc.readInt("Số lần click: ");
    if (times <= 0) return;
    
    int intervalMs = sc.readInt("Delay giữa các lần (ms) [100]: ");
    if (intervalMs <= 0) intervalMs = 100;
    
    int clickType = sc.readInt("Kiểu click: [1] Trái | [2] Phải | [3] Đúp trái: ");
    if (clickType < 1 || clickType > 3) clickType = 1;

    int delaySec = sc.readInt("Thời gian chờ di chuột (giây) [3]: ");
    if (delaySec <= 0) delaySec = 3;
    
    cout << "\nDi chuột đến vị trí cần click...\n";
    for (int i = delaySec; i > 0; i--) { 
        cout << " " << i << "... "; cout.flush(); 
        if (sleepWithEmergencyCheck(1000)) {
            cout << "\nĐã hủy.\n";
            return;
        }
    }
    
    POINT p; 
    GetCursorPos(&p);
    cout << "\n\nTọa độ: (" << p.x << ", " << p.y << ") | " << times << " lần | Delay: " << intervalMs << "ms (Ngắt: ESC/F6)\n";
    
    bool stopped = false;
    int executed = 0;
    for (int i = 0; i < times; i++) { 
        if (SystemCore::checkEmergencyStop()) {
            stopped = true;
            break;
        }

        SetCursorPos(p.x, p.y); 
        if (clickType == 1) {
            sc.leftClick();
        } else if (clickType == 2) {
            INPUT inp[2] = {};
            inp[0].type = INPUT_MOUSE;
            inp[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            inp[1].type = INPUT_MOUSE;
            inp[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            SendInput(2, inp, sizeof(INPUT));
        } else if (clickType == 3) {
            sc.leftClick();
            Sleep(30);
            sc.leftClick();
        }
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
        cout << "\n\n[!] Đã dừng khẩn cấp (" << executed << "/" << times << " lần)\n";
    } else {
        cout << "\n\n[✓] Hoàn thành " << times << " lần click!\n";
    }
    sc.waitEnter();
}

// Spam văn bản tự động
void UtilityTools::spamText() {
    sc.cls();
    
    cout << "Nhập text cần gửi: ";
    string content; 
    getline(cin, content);
    while (content.empty()) {
        getline(cin, content);
    }
    
    int times = sc.readInt("Số lần gửi: ");
    if (times <= 0) return;
    
    int delayMs = sc.readInt("Delay (ms) [100]: ");
    if (delayMs <= 0) delayMs = 100;
    
    cout << "Tự động click vào ô nhập? (y/n): ";
    string autoFocus;
    getline(cin, autoFocus);
    bool shouldClick = (autoFocus == "y" || autoFocus == "Y");
    
    cout << "Phím ngắt: ESC / F6\n"
         << "Bắt đầu sau 3 giây...\n";
    for (int i = 3; i > 0; i--) { 
        cout << " " << i << "... "; cout.flush(); 
        if (sleepWithEmergencyCheck(1000)) {
            cout << "\nĐã hủy.\n";
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
        cout << "\n\n[!] Đã dừng khẩn cấp (" << executed << "/" << times << ")\n";
    } else {
        cout << "\n\n[✓] Hoàn tất " << times << " lần!\n";
    }
    sc.waitEnter();
}

// Tự động paste danh sách dữ liệu
void UtilityTools::autoPasteData() {
    sc.cls();
    
    int n = sc.readInt("Số dòng dữ liệu: ");
    if (n <= 0) return;
    
    int delayMs = sc.readInt("Delay giữa các dòng (ms) [200]: ");
    if (delayMs <= 0) delayMs = 200;
    
    vector<string> dataList(n);
    cout << "\nNhập " << n << " dòng dữ liệu:\n";
    for (int i = 0; i < n; i++) { 
        cout << "  [" << i + 1 << "]: "; 
        getline(cin, dataList[i]);
        if (dataList[i].empty()) {
            dataList[i] = "(empty)";
        }
    }
    
    cout << "Tự động click vào ô nhập? (y/n): ";
    string autoFocus;
    getline(cin, autoFocus);
    bool shouldClick = (autoFocus == "y" || autoFocus == "Y");
    
    cout << "Phím ngắt: ESC / F6\n"
         << "Bắt đầu sau 3 giây...\n";
    for (int i = 3; i > 0; i--) { 
        cout << " " << i << "... "; cout.flush(); 
        if (sleepWithEmergencyCheck(1000)) {
            cout << "\nĐã hủy.\n";
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
        cout << "\n\n[!] Đã dừng khẩn cấp (" << executed << "/" << n << ")\n";
    } else {
        cout << "\n\n[✓] Hoàn tất dán " << n << " dòng!\n";
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
    // 1. Tìm theo đường dẫn thực tế của file thực thi .exe
    char buffer[MAX_PATH];
    if (GetModuleFileNameA(NULL, buffer, MAX_PATH) > 0) {
        fs::path exePath(buffer);
        fs::path exeDir = exePath.parent_path();

        if (fs::exists(exeDir / "apps.txt")) return (exeDir / "apps.txt").string();
        if (fs::exists(exeDir / "src" / "apps.txt")) return (exeDir / "src" / "apps.txt").string();
        if (fs::exists(exeDir.parent_path() / "src" / "apps.txt")) return (exeDir.parent_path() / "src" / "apps.txt").string();
        if (fs::exists(exeDir.parent_path() / "apps.txt")) return (exeDir.parent_path() / "apps.txt").string();
    }

    // 2. Tìm theo thư mục làm việc hiện tại (CWD)
    if (fs::exists("src/apps.txt")) return "src/apps.txt";
    if (fs::exists("apps.txt")) return "apps.txt";
    if (fs::exists("../src/apps.txt")) return "../src/apps.txt";

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
        cout << "[Lưu: " << downloadDir << "]\n\n";

        if (apps.empty()) {
            cout << " [!] Không tìm thấy danh sách trong " << configPath << "\n\n";
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

        cout << "\n [A] Tải tất cả | [R] Nạp lại | [0] Quay lại\n\n"
             << " Chọn số (hoặc nhiều số, vd: 1 5 8): ";

        string inputLine;
        getline(cin, inputLine);
        inputLine = SystemCore::trim(inputLine);

        if (inputLine.empty()) continue;
        if (inputLine == "0") break;

        if (inputLine == "R" || inputLine == "r") {
            configPath = resolveAppConfigPath();
            apps = loadAppsFromTxt(configPath);
            cout << "\nĐã nạp lại (" << apps.size() << " ứng dụng)!\n";
            Sleep(600);
            continue;
        }

        if (inputLine == "A" || inputLine == "a") {
            if (apps.empty()) continue;
            sc.cls();
            cout << "=== TẢI TOÀN BỘ " << apps.size() << " ỨNG DỤNG ===\n\n";

            int successCount = 0;
            for (size_t i = 0; i < apps.size(); ++i) {
                cout << "[" << (i + 1) << "/" << apps.size() << "] Đang tải: " << apps[i].name << "...\n";
                string targetPath = downloadDir + "\\" + apps[i].fileName;
                string cmd = "curl -# -L \"" + apps[i].url + "\" -o \"" + targetPath + "\"";
                int ret = system(cmd.c_str());

                if (ret == 0 && fs::exists(targetPath)) {
                    cout << "  [✓] Xong: " << apps[i].fileName << "\n";
                    successCount++;
                } else {
                    cout << "  [!] Thất bại: " << apps[i].name << "\n";
                }
            }

            cout << "\n[✓] Hoàn tất " << successCount << "/" << apps.size() << " ứng dụng!\n";
            sc.waitEnter();
            continue;
        }

        // Phân tích danh sách số được chọn (ví dụ: "1", "1 5 8", "1,5,8")
        vector<int> selectedIndices;
        stringstream ss(inputLine);
        string token;
        while (ss >> token) {
            for (char &c : token) { if (c == ',') c = ' '; }
            stringstream subSs(token);
            string num;
            while (subSs >> num) {
                try {
                    int idx = stoi(num);
                    if (idx >= 1 && idx <= (int)apps.size()) {
                        selectedIndices.push_back(idx - 1);
                    }
                } catch (...) {}
            }
        }

        if (selectedIndices.empty()) {
            cout << "\n[!] Lựa chọn không hợp lệ!\n";
            Sleep(500);
            continue;
        }

        // Nếu chỉ chọn 1 ứng dụng
        if (selectedIndices.size() == 1) {
            const auto &app = apps[selectedIndices[0]];
            sc.cls();
            cout << "[-] Đang tải " << app.name << " (" << app.fileName << ")...\n\n";

            string targetPath = downloadDir + "\\" + app.fileName;
            string cmd = "curl -# -L \"" + app.url + "\" -o \"" + targetPath + "\"";
            int ret = system(cmd.c_str());

            if (ret == 0 && fs::exists(targetPath)) {
                cout << "\n[✓] Đã tải về: " << targetPath << "\n";
                cout << "Mở file cài đặt ngay? (y/n): ";
                string runChoice;
                getline(cin, runChoice);
                if (runChoice == "y" || runChoice == "Y") {
                    ShellExecuteA(NULL, "open", targetPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
            } else {
                cout << "\n[!] Tải thất bại! Kiểm tra kết nối mạng.\n";
                sc.waitEnter();
            }
        } else {
            // Tải nhiều ứng dụng được chọn theo lô
            sc.cls();
            cout << "=== TẢI " << selectedIndices.size() << " ỨNG DỤNG ĐÃ CHỌN ===\n\n";

            int successCount = 0;
            for (size_t i = 0; i < selectedIndices.size(); ++i) {
                const auto &app = apps[selectedIndices[i]];
                cout << "[" << (i + 1) << "/" << selectedIndices.size() << "] Đang tải: " << app.name << "...\n";
                string targetPath = downloadDir + "\\" + app.fileName;
                string cmd = "curl -# -L \"" + app.url + "\" -o \"" + targetPath + "\"";
                int ret = system(cmd.c_str());

                if (ret == 0 && fs::exists(targetPath)) {
                    cout << "  [✓] Xong: " << app.fileName << "\n";
                    successCount++;
                } else {
                    cout << "  [!] Thất bại: " << app.name << "\n";
                }
            }

            cout << "\n[✓] Hoàn tất " << successCount << "/" << selectedIndices.size() << " ứng dụng!\n";
            cout << "Mở thư mục Downloads? (y/n): ";
            string openChoice;
            getline(cin, openChoice);
            if (openChoice == "y" || openChoice == "Y") {
                ShellExecuteA(NULL, "open", downloadDir.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
        }
    }
}

static bool isProcessRunning(const string &procName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32First(hSnap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, procName.c_str()) == 0) {
                found = true;
                break;
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return found;
}

struct BloatAppInfo {
    string id;
    string name;
};

// 1. Nhóm App rác thứ cấp (Store Apps, Game, Ads cài sẵn - không hook sâu hệ thống)
static const vector<BloatAppInfo> g_secondaryBloat = {
    {"Microsoft.BingNews", "Bing News"},
    {"Microsoft.BingWeather", "Bing Weather"},
    {"Microsoft.GetHelp", "Get Help"},
    {"Microsoft.Getstarted", "Tips (Mẹo & Bắt đầu)"},
    {"Microsoft.MicrosoftOfficeHub", "Quảng cáo Office 365"},
    {"Microsoft.MicrosoftSolitaireCollection", "Game Solitaire"},
    {"Microsoft.SkypeApp", "Skype mặc định"},
    {"Microsoft.Todos", "Microsoft To-Do"},
    {"Microsoft.WindowsFeedbackHub", "Feedback Hub"},
    {"Microsoft.WindowsMaps", "Windows Maps"},
    {"Microsoft.MixedReality.Portal", "Mixed Reality Portal"},
    {"Clipchamp.Clipchamp", "Clipchamp"},
    {"Disney.37853FC22B2CE", "Disney+"},
    {"SpotifyAB.SpotifyMusic", "Spotify"},
    {"king.com.CandyCrushSaga", "Candy Crush Saga"},
    {"king.com.CandyCrushSodaSaga", "Candy Crush Soda"},
    {"king.com.BubbleWitch3Saga", "Bubble Witch 3"},
    {"Playtika.CaesarsSlotsFreeCasino", "Caesars Slots"},
    {"ShazamEntertainmentLtd.Shazam", "Shazam"},
    {"ByteDancePte.Ltd.TikTok", "TikTok"},
    {"Amazon.com.Amazon", "Amazon"}
};

// 2. Trạng thái App rác nâng cao (Bám rễ sâu: Chạy ngầm Taskmgr, Dịch vụ Service, File exe trong C:\, Registry)
struct AdvancedBloatStatus {
    bool hasOneDrive = false;
    bool oneDriveRunning = false;
    bool hasPhoneLink = false;
    bool phoneLinkRunning = false;
    bool hasCortana = false;
    bool hasTeams = false;
    bool teamsRunning = false;
};

// Dò quét thực tế trên máy tính
static void scanBloatware(vector<BloatAppInfo> &outSecondary, AdvancedBloatStatus &outAdv) {
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    string scanFile = string(tempPath) + "cmd_installed_apps.txt";
    string scanCmd = "powershell -NoProfile -Command \"Get-AppxPackage | Select-Object -ExpandProperty Name | Out-File -FilePath '" + scanFile + "' -Encoding ascii\"";
    system(scanCmd.c_str());

    string installed = "";
    if (fs::exists(scanFile)) {
        ifstream f(scanFile);
        if (f.is_open()) {
            stringstream ss;
            ss << f.rdbuf();
            installed = ss.str();
            f.close();
        }
        try { fs::remove(scanFile); } catch (...) {}
    }

    outSecondary.clear();
    for (const auto &app : g_secondaryBloat) {
        if (installed.find(app.id) != string::npos) {
            outSecondary.push_back(app);
        }
    }

    char* userProf = getenv("USERPROFILE");
    string oneDriveLocal = userProf ? (string(userProf) + "\\AppData\\Local\\Microsoft\\OneDrive") : "";
    outAdv.oneDriveRunning = isProcessRunning("OneDrive.exe");
    outAdv.hasOneDrive = outAdv.oneDriveRunning || (fs::exists(oneDriveLocal) && !fs::is_empty(oneDriveLocal)) || fs::exists("C:\\ProgramData\\Microsoft OneDrive");

    outAdv.phoneLinkRunning = isProcessRunning("PhoneExperienceHost.exe") || isProcessRunning("CrossDeviceService.exe");
    outAdv.hasPhoneLink = outAdv.phoneLinkRunning || (installed.find("Microsoft.YourPhone") != string::npos) || (installed.find("CrossDevice") != string::npos);

    outAdv.hasCortana = isProcessRunning("Cortana.exe") || (installed.find("Microsoft.549981C3F5F10") != string::npos);
    outAdv.teamsRunning = isProcessRunning("ms-teams.exe") || isProcessRunning("msteams.exe");
    outAdv.hasTeams = outAdv.teamsRunning || (installed.find("MicrosoftTeams") != string::npos);
}

// Luồng 1: Xử lý gỡ sạch app rác thứ cấp
static void cleanSecondaryBloat(SystemCore &sc, const vector<BloatAppInfo> &list) {
    if (list.empty()) {
        cout << " [✓] Không có ứng dụng rác thứ cấp nào cần gỡ.\n";
        return;
    }
    cout << " [-] Đang gỡ bỏ " << list.size() << " ứng dụng rác thứ cấp...\n";
    string psScript = "$apps = @(\n";
    for (size_t i = 0; i < list.size(); ++i) {
        psScript += "    '" + list[i].id + "'";
        if (i + 1 < list.size()) psScript += ",";
        psScript += "\n";
    }
    psScript += ");\n";
    psScript += "foreach ($pkg in $apps) {\n";
    psScript += "    Get-AppxPackage -AllUsers -Name ('*' + $pkg + '*') -ErrorAction SilentlyContinue | Remove-AppxPackage -AllUsers -ErrorAction SilentlyContinue;\n";
    psScript += "    Get-AppxProvisionedPackage -Online | Where-Object { $_.DisplayName -like ('*' + $pkg + '*') -or $_.PackageName -like ('*' + $pkg + '*') } | Remove-AppxProvisionedPackage -Online -ErrorAction SilentlyContinue;\n";
    psScript += "}\n";

    string cmd = "powershell -NoProfile -Command \"" + psScript + "\"";
    sc.runAdmin(cmd, true);
    cout << " [✓] Đã dọn sạch " << list.size() << " ứng dụng rác thứ cấp thành công!\n";
}

// Luồng 2: Xử lý tận gốc app rác nâng cao (Chỉ chạy khi phát hiện có trên máy)
static void cleanAdvancedBloat(SystemCore &sc, const AdvancedBloatStatus &adv) {
    if (!adv.hasOneDrive && !adv.hasPhoneLink && !adv.hasCortana && !adv.hasTeams) {
        cout << " [✓] Các ứng dụng rác nâng cao đều sạch sẽ, không có gì cần gỡ!\n";
        return;
    }

    cout << " [-] Đang xử lý gỡ bỏ các ứng dụng nâng cao được phát hiện...\n";
    string psScript = "";

    // 1. Kiểm tra & Gỡ Microsoft OneDrive nếu có
    if (adv.hasOneDrive) {
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
    }

    // 2. Kiểm tra & Gỡ Phone Link, vô hiệu hóa Service CDPUserSvc nếu có
    if (adv.hasPhoneLink) {
        psScript += "Stop-Process -Name 'CrossDeviceService','PhoneExperienceHost' -Force -ErrorAction SilentlyContinue;\n";
        psScript += "Get-AppxPackage -AllUsers -Name '*YourPhone*','*CrossDevice*' -ErrorAction SilentlyContinue | Remove-AppxPackage -AllUsers -ErrorAction SilentlyContinue;\n";
        psScript += "Get-AppxProvisionedPackage -Online | Where-Object { $_.DisplayName -like '*YourPhone*' -or $_.DisplayName -like '*CrossDevice*' } | Remove-AppxProvisionedPackage -Online -ErrorAction SilentlyContinue;\n";
        psScript += "Stop-Service -Name 'CDPUserSvc*' -Force -ErrorAction SilentlyContinue;\n";
        psScript += "Set-ItemProperty -Path 'HKLM:\\SYSTEM\\CurrentControlSet\\Services\\CDPUserSvc' -Name 'Start' -Value 4 -Type DWord -Force -ErrorAction SilentlyContinue;\n";
        psScript += "New-Item -Path 'HKLM:\\SOFTWARE\\Policies\\Microsoft\\Windows\\System' -Force -ErrorAction SilentlyContinue | Out-Null;\n";
        psScript += "Set-ItemProperty -Path 'HKLM:\\SOFTWARE\\Policies\\Microsoft\\Windows\\System' -Name 'EnableMmx' -Value 0 -Type DWord -Force -ErrorAction SilentlyContinue;\n";
    }

    // 3. Kiểm tra & Gỡ Cortana nếu có
    if (adv.hasCortana) {
        psScript += "Stop-Process -Name 'Cortana' -Force -ErrorAction SilentlyContinue;\n";
        psScript += "Get-AppxPackage -AllUsers -Name '*549981C3F5F10*' -ErrorAction SilentlyContinue | Remove-AppxPackage -AllUsers -ErrorAction SilentlyContinue;\n";
        psScript += "New-Item -Path 'HKLM:\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search' -Force -ErrorAction SilentlyContinue | Out-Null;\n";
        psScript += "Set-ItemProperty -Path 'HKLM:\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search' -Name 'AllowCortana' -Value 0 -Type DWord -Force -ErrorAction SilentlyContinue;\n";
    }

    // 4. Kiểm tra & Gỡ Teams cá nhân nếu có
    if (adv.hasTeams) {
        psScript += "Stop-Process -Name 'ms-teams','msteams' -Force -ErrorAction SilentlyContinue;\n";
        psScript += "Get-AppxPackage -AllUsers -Name '*MicrosoftTeams*' -ErrorAction SilentlyContinue | Remove-AppxPackage -AllUsers -ErrorAction SilentlyContinue;\n";
        psScript += "Set-ItemProperty -Path 'HKCU:\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced' -Name 'TaskbarMn' -Value 0 -Type DWord -Force -ErrorAction SilentlyContinue;\n";
    }

    if (!psScript.empty()) {
        string cmd = "powershell -NoProfile -Command \"" + psScript + "\"";
        sc.runAdmin(cmd, true);
    }

    if (adv.hasOneDrive) cout << " [✓] Đã gỡ triệt để Microsoft OneDrive (tiến trình, thư mục C:, icon Explorer).\n";
    if (adv.hasPhoneLink) cout << " [✓] Đã gỡ Phone Link & vô hiệu hóa dịch vụ ngầm CDPUserSvc.\n";
    if (adv.hasCortana) cout << " [✓] Đã gỡ Cortana & chặn Policy tìm kiếm.\n";
    if (adv.hasTeams) cout << " [✓] Đã gỡ Teams cá nhân & ẩn icon Chat trên Taskbar.\n";
}

// Gỡ bỏ ứng dụng rác Bloatware (Dọn dẹp toàn diện cả 2 luồng: Thứ cấp & Nâng cao)
void UtilityTools::uninstallBloatware() {
    sc.cls();
    cout << "\n[-] Đang dò quét ứng dụng rác trên hệ thống...\n\n";

    vector<BloatAppInfo> detectedSec;
    AdvancedBloatStatus advStatus;
    scanBloatware(detectedSec, advStatus);

    cout << " ┌─ [ KẾT QUẢ DÒ QUÉT ] ──────────────────────────────────────────\n";
    
    // Hiển thị app thứ cấp
    cout << " │ [1] App rác thứ cấp: Phát hiện " << detectedSec.size() << "/" << g_secondaryBloat.size() << " ứng dụng\n";
    if (!detectedSec.empty()) {
        cout << " │     ↳ ";
        for (size_t i = 0; i < detectedSec.size(); ++i) {
            cout << detectedSec[i].name;
            if (i + 1 < detectedSec.size()) cout << ", ";
            if ((i + 1) % 4 == 0 && i + 1 < detectedSec.size()) cout << "\n │       ";
        }
        cout << "\n";
    } else {
        cout << " │     ↳ \x1b[32m(Hệ thống sạch, không có app thứ cấp)\x1b[0m\n";
    }

    // Hiển thị app nâng cao
    cout << " │\n │ [2] App rác nâng cao (Bám rễ sâu):\n";
    cout << " │   - Microsoft OneDrive   : " << (advStatus.hasOneDrive ? (advStatus.oneDriveRunning ? "\x1b[33m[Phát hiện - Đang chạy ngầm]\x1b[0m" : "\x1b[33m[Phát hiện file/folder C:]\x1b[0m") : "\x1b[32m[Sạch]\x1b[0m") << "\n";
    cout << " │   - Phone Link & Dịch vụ : " << (advStatus.hasPhoneLink ? (advStatus.phoneLinkRunning ? "\x1b[33m[Phát hiện - Tiến trình đang chạy]\x1b[0m" : "\x1b[33m[Phát hiện gói/dịch vụ]\x1b[0m") : "\x1b[32m[Sạch]\x1b[0m") << "\n";
    cout << " │   - Cortana Assistant    : " << (advStatus.hasCortana ? "\x1b[33m[Phát hiện gói Cortana]\x1b[0m" : "\x1b[32m[Sạch]\x1b[0m") << "\n";
    cout << " │   - Teams Chat Taskbar   : " << (advStatus.hasTeams ? "\x1b[33m[Phát hiện Teams cá nhân]\x1b[0m" : "\x1b[32m[Sạch]\x1b[0m") << "\n";
    cout << " └────────────────────────────────────────────────────────────────\n\n";

    bool hasAnySec = !detectedSec.empty();
    bool hasAnyAdv = advStatus.hasOneDrive || advStatus.hasPhoneLink || advStatus.hasCortana || advStatus.hasTeams;

    if (!hasAnySec && !hasAnyAdv) {
        cout << " \x1b[32m[✓] Hệ thống đã hoàn toàn sạch sẽ, không phát hiện ứng dụng rác nào cần xử lý!\x1b[0m\n\n";
        sc.waitEnter();
        return;
    }

    if (!SystemCore::confirm("Xác nhận thực hiện dọn dẹp toàn diện cả 2 luồng?")) {
        cout << "Đã hủy thao tác.\n";
        Sleep(800);
        return;
    }

    cout << "\n";
    cleanSecondaryBloat(sc, detectedSec);
    cleanAdvancedBloat(sc, advStatus);

    cout << "\n[✓] Hoàn tất quá trình dọn dẹp toàn diện!\n";
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
        cout << "--- THÔNG TIN PIN LAPTOP ---\n"
             << "Đang đọc dữ liệu ACPI...\n";

        SYSTEM_POWER_STATUS sps;
        bool hasSps = GetSystemPowerStatus(&sps);

        // Kiểm tra thiết bị có pin không
        if (hasSps && (sps.BatteryFlag == 128 || sps.BatteryFlag == 255) && sps.BatteryLifePercent == 255) {
            sc.cls();
            cout << "--- THÔNG TIN PIN LAPTOP ---\n\n"
                 << " [!] Máy tính bàn (PC) hoặc không có Pin.\n"
                 << "     Nguồn: Cắm sạc trực tiếp (AC Online).\n\n";
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
        cout << "=== THÔNG TIN PIN LAPTOP ===\n";

        if (!sysMfg.empty() && sysMfg != "N/A") {
            cout << "  Thiết bị     : " << sysMfg << " " << sysModel << " (BIOS: " << biosVer << ")\n";
        }
        if (!deviceName.empty() && deviceName != "N/A") {
            cout << "  Pin          : " << chemistry << " - " << manufacturer << " [" << deviceName << "]\n";
        }
        cout << "----------------------------------------------------------------------\n";

        if (designCap > 0 && fullCap > 0) {
            double healthPercent = ((double)fullCap / (double)designCap) * 100.0;
            if (healthPercent > 100.0) healthPercent = 100.0;
            double wearPercent = 100.0 - healthPercent;
            long long lostCap = designCap - fullCap;
            if (lostCap < 0) lostCap = 0;

            string colorCode = "\x1b[32m";
            string wearLevel = "(Rất tốt)";
            if (wearPercent < 5.0) {
                colorCode = "\x1b[32m";
                wearLevel = "(Như mới)";
            } else if (wearPercent < 15.0) {
                colorCode = "\x1b[32m";
                wearLevel = "(Tốt)";
            } else if (wearPercent < 30.0) {
                colorCode = "\x1b[33m";
                wearLevel = "(Bình thường)";
            } else if (wearPercent < 50.0) {
                colorCode = "\x1b[31m";
                wearLevel = "(Chai đáng kể)";
            } else {
                colorCode = "\x1b[31m";
                wearLevel = "(Chai nặng - Nên thay pin)";
            }

            cout << "  Dung lượng gốc : " << setw(8) << right << formatNumber(designCap) << " mWh\n"
                 << "  Khi nạp đầy    : " << setw(8) << right << formatNumber(fullCap) << " mWh (Hao hụt: " << formatNumber(lostCap) << " mWh)\n"
                 << "  Chu kỳ sạc     : " << setw(8) << right << cycleCount << " lần\n"
                 << "  Sức khỏe Pin   : " << colorCode << fixed << setprecision(1) << healthPercent << "%\x1b[0m  " << renderBar(healthPercent) << "\n"
                 << "  Độ chai Pin    : " << colorCode << fixed << setprecision(1) << wearPercent << "%\x1b[0m  " << wearLevel << "\n";
        } else {
            cout << "  [!] Không đọc được ACPI nâng cao.\n";
        }

        if (hasSps) {
            string powerSource = (sps.ACLineStatus == 1) ? "Cắm sạc (AC ⚡)" : (sps.ACLineStatus == 0) ? "Dùng Pin (DC 🔋)" : "Không rõ";
            int batPct = (int)sps.BatteryLifePercent;
            cout << "  Nguồn điện     : " << powerSource << "\n";
            if (batPct >= 0 && batPct <= 100) {
                cout << "  Mức sạc hiện tại: " << batPct << "%  " << renderBar(batPct) << "\n";
            }

            string chargeStatus = "Bình thường";
            if (sps.BatteryFlag & 8) chargeStatus = "Đang sạc ⚡";
            else if (sps.ACLineStatus == 1 && batPct >= 95) chargeStatus = "Đã đầy";
            else if (sps.BatteryFlag & 4) chargeStatus = "Cực yếu";
            else if (sps.BatteryFlag & 2) chargeStatus = "Yếu";
            cout << "  Trạng thái sạc : " << chargeStatus << "\n";

            if (sps.BatteryLifeTime != (DWORD)-1 && sps.ACLineStatus == 0) {
                int hours = sps.BatteryLifeTime / 3600;
                int mins = (sps.BatteryLifeTime % 3600) / 60;
                cout << "  Thời lượng còn : ~" << hours << " giờ " << mins << " phút\n";
            }
        }

        cout << "----------------------------------------------------------------------\n"
             << " [1] Báo cáo HTML | [2] Cài đặt Pin | [R] Làm mới | [0] Quay lại\n"
             << " Chọn: ";

        string opt;
        getline(cin, opt);
        opt = SystemCore::trim(opt);

        if (opt.empty()) continue;
        if (opt == "0") break;

        if (opt == "1") {
            cout << "\nĐang xuất báo cáo HTML...\n";
            char tempHtml[MAX_PATH];
            GetTempPathA(MAX_PATH, tempHtml);
            string htmlPath = string(tempHtml) + "battery_report.html";
            string genCmd = "powercfg /batteryreport /output \"" + htmlPath + "\" >nul 2>&1";
            system(genCmd.c_str());

            if (fs::exists(htmlPath)) {
                ShellExecuteA(NULL, "open", htmlPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                cout << "  [✓] Đã mở: " << htmlPath << "\n";
            } else {
                cout << "  [!] Không thể xuất file HTML.\n";
            }
            Sleep(1000);
            continue;
        }

        if (opt == "2") {
            ShellExecuteA(NULL, "open", "ms-settings:batterysaver", NULL, NULL, SW_SHOWNORMAL);
            cout << "\nĐã mở cài đặt Pin...\n";
            Sleep(800);
            continue;
        }

        if (opt == "R" || opt == "r") {
            continue;
        }
    }
}

