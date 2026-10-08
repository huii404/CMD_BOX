#include "FileSafety.h"
#include <shlobj.h>
#include <shellapi.h>
#include "UtilityTools.h"
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <iomanip>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

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
    cout << "\n \x1b[38;2;195;165;255m╭── TỰ ĐỘNG CLICK CHUỘT ────────────────────────────╮\x1b[0m\n"
         << "   Nhập: Số lần, Delay ms, Kiểu (1: Trái, 2: Phải, 3: Đúp)\n"
         << "   Mặc định: 100, 50, 2 | 0: Hủy\n"
         << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n"
         << " [>] ";
    
    string line;
    if (!getline(cin, line)) return;
    line = SystemCore::trim(line);
    if (line == "0") return;

    int times = 100;
    int intervalMs = 50;
    int clickType = 2; // Mặc định: Bấm Chuột Phải
    int delaySec = 3;

    if (!line.empty()) {
        for (char &c : line) { if (c == ',') c = ' '; }
        stringstream ss(line);
        int t = 0, d = 0, k = 0;
        if (ss >> t && t > 0) times = t;
        if (ss >> d && d > 0) intervalMs = d;
        if (ss >> k && k >= 1 && k <= 3) clickType = k;
    }

    string typeName = (clickType == 1) ? "Chuột Trái" : ((clickType == 2) ? "Chuột Phải" : "Đúp Trái");
    cout << "\nDi chuột đến vị trí cần click (" << typeName << " | " << times << " lần | Delay: " << intervalMs << "ms)\n";
    for (int i = delaySec; i > 0; i--) { 
        cout << " " << i; cout.flush(); 
        if (sleepWithEmergencyCheck(1000)) {
            cout << "\nĐã hủy.\n";
            sc.waitEnter();
            return;
        }
    }
    
    POINT p; 
    GetCursorPos(&p);
    cout << "\n\nTọa độ: (" << p.x << ", " << p.y << ") | " << times << " lần | " << typeName << " | Delay: " << intervalMs << "ms (Ngắt: ESC/F6)\n";
    
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
        
        if (times >= 20) {
            if (i % (times / 20) == 0 || i == times - 1) {
                cout << "\rTiến độ: " << ((i + 1) * 100 / times) << "% ";
                cout.flush();
            }
        } else {
            cout << "\rĐã click: " << (i + 1) << "/" << times << " ";
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
    
    cout << "\n \x1b[38;2;195;165;255m╭── GỬI VĂN BẢN TỰ ĐỘNG ────────────────────────────╮\x1b[0m\n"
         << "   Nhập nội dung cần gửi (0 để hủy)\n"
         << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n"
         << " [>] ";
    string content; 
    if (!getline(cin, content)) return;
    content = SystemCore::trim(content);
    if (content.empty() || content == "0") return;
    
    cout << "\n Nhập: Số lần gửi [delay ms] (Mặc định: 10 lần, delay 100ms)\n"
         << "   * Ví dụ: Bấm Enter dùng luôn | hoặc gõ: 50 | hoặc: 50, 80\n"
         << " [>] ";
    string optLine;
    getline(cin, optLine);
    optLine = SystemCore::trim(optLine);

    int times = 10;
    int delayMs = 100;

    if (!optLine.empty()) {
        for (char &c : optLine) { if (c == ',') c = ' '; }
        stringstream ss(optLine);
        int t = 0, d = 0;
        if (ss >> t && t > 0) times = t;
        if (ss >> d && d > 0) delayMs = d;
    }
    
    bool shouldClick = true; // Tự động kích hoạt con trỏ chuột vào ô nhập
    
    cout << "\nPhím ngắt: ESC / F6\n"
         << "Di chuột vào ô nhập liệu (bắt đầu sau 3 giây)\n";
    for (int i = 3; i > 0; i--) { 
        cout << " " << i; cout.flush(); 
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
        Sleep(15);
        sc.pressEnter();
        executed++;
        
        if (times <= 10 || (i + 1) % 10 == 0 || i == times - 1) {
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
    cout << "\n \x1b[38;2;195;165;255m╭── DÁN DỮ LIỆU TỰ ĐỘNG ────────────────────────────╮\x1b[0m\n"
         << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n";
    
    int n = sc.readInt("Số dòng dữ liệu: ");
    if (n <= 0 || n > 100000) return;
    
    int delayMs = sc.readInt("Delay giữa các dòng (ms) [200]: ", 200);
    if (delayMs <= 0) delayMs = 200;
    if (delayMs > 60000) { cout << "Delay tối đa 60000 ms.\n"; return; }
    
    vector<string> dataList(n);
    cout << "\nNhập " << n << " dòng dữ liệu:\n";
    for (int i = 0; i < n; i++) { 
        cout << "  [" << i + 1 << "]: "; 
        getline(cin, dataList[i]);
    }
    
    cout << "Tự động click vào ô nhập? (y/n): ";
    string autoFocus;
    getline(cin, autoFocus);
    bool shouldClick = (autoFocus == "y" || autoFocus == "Y");
    
    cout << "Phím ngắt: ESC / F6\n"
         << "Bắt đầu sau 3 giây\n";
    for (int i = 3; i > 0; i--) { 
        cout << " " << i; cout.flush(); 
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
        Sleep(15);
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

// Trình tải & Cài đặt phần mềm tự động (ủy quyền qua scripts/download-apps.bat)
void UtilityTools::downloadManager() {
    fs::path scriptPath;
    wchar_t buffer[MAX_PATH];
    if (GetModuleFileNameW(NULL, buffer, MAX_PATH) > 0) {
        fs::path exeDir = fs::path(buffer).parent_path();
        if (fs::exists(exeDir / "scripts" / "download-apps.bat")) {
            scriptPath = exeDir / "scripts" / "download-apps.bat";
        } else if (fs::exists(exeDir.parent_path() / "scripts" / "download-apps.bat")) {
            scriptPath = exeDir.parent_path() / "scripts" / "download-apps.bat";
        }
    }
    if (scriptPath.empty()) {
        if (fs::exists("scripts/download-apps.bat")) scriptPath = "scripts/download-apps.bat";
        else if (fs::exists("../scripts/download-apps.bat")) scriptPath = "../scripts/download-apps.bat";
    }

    if (scriptPath.empty() || !fs::exists(scriptPath)) {
        cout << "\n  [!] Không tìm thấy kịch bản: scripts/download-apps.bat\n";
        sc.waitEnter();
        return;
    }

    string cmd = "\"" + scriptPath.string() + "\"";
    sc.runCMD(cmd);
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
    // Mạng xã hội & Giải trí bên thứ ba
    {"LinkedIn", "LinkedIn"},
    {"TikTok", "TikTok"},
    {"Instagram", "Instagram"},
    {"Facebook", "Facebook"},
    {"Netflix", "Netflix"},
    {"Spotify", "Spotify"},
    {"Amazon", "Amazon"},
    {"Shazam", "Shazam"},
    {"Fitbit", "Fitbit"},
    {"Twitter", "Twitter"},
    {"WhatsApp", "WhatsApp"},

    // Game rác cài sẵn
    {"CandyCrush", "Candy Crush (Saga / Soda)"},
    {"BubbleWitch", "Bubble Witch 3 Saga"},
    {"CaesarsSlots", "Caesars Slots Casino"},
    {"SolitaireCollection", "Game Solitaire"},

    // Tiện ích rác & Ứng dụng thừa Microsoft
    {"MicrosoftStickyNotes", "Sticky Notes (Ghi chú)"},
    {"BingNews", "Bing News (Tin tức)"},
    {"BingWeather", "Bing Weather (Thời tiết)"},
    {"BingSearch", "Bing Search"},
    {"GetHelp", "Get Help (Trợ giúp)"},
    {"Getstarted", "Tips (Mẹo & Bắt đầu)"},
    {"MicrosoftOfficeHub", "Quảng cáo Office 365"},
    {"SkypeApp", "Skype mặc định"},
    {"Todos", "Microsoft To-Do"},
    {"WindowsFeedbackHub", "Feedback Hub (Phản hồi)"},
    {"WindowsMaps", "Windows Maps (Bản đồ)"},
    {"MixedReality", "Mixed Reality Portal"},
    {"Clipchamp", "Clipchamp Video Editor"},
    {"Disney", "Disney+"},
    {"PowerAutomateDesktop", "Power Automate Desktop"},
    {"People", "Microsoft People (Danh bạ)"},
    {"WindowsAlarms", "Báo thức & Đồng hồ"},
    {"WindowsSoundRecorder", "Ghi âm giọng nói"},
    {"ZuneMusic", "Media Player / Groove Music"},
    {"ZuneVideo", "Phim & TV (Movies & TV)"},
    {"Paint3D", "Paint 3D"},
    {"3DBuilder", "3D Builder"},
    {"Microsoft3DViewer", "3D Viewer"},
    {"OneNoteForWindows10", "OneNote for Windows 10"},
    {"QuickAssist", "Quick Assist (Hỗ trợ nhanh)"},
    {"OutlookForWindows", "Outlook mới"},
    {"MicrosoftTeams", "Microsoft Teams (Chat)"},

    // Xbox bloatware
    {"Xbox", "Gia đình ứng dụng Xbox (Xbox App, Game Bar, TCUI)"},
    {"GamingApp", "Xbox App (GamingApp)"},
    {"XboxGamingOverlay", "Xbox Game Bar"},
    {"XboxGameOverlay", "Xbox Game Overlay"},
    {"XboxIdentityProvider", "Xbox Identity Provider"},
    {"XboxSpeechToTextOverlay", "Xbox Speech To Text"}
};

// 2. Trạng thái App rác nâng cao (Bám rễ sâu: Chạy ngầm Taskmgr, Dịch vụ Service, File exe trong C:\, Registry)
struct AdvancedBloatStatus {
    bool hasOneDrive = false;
    bool oneDriveRunning = false;
    bool hasPhoneLink = false;
    bool phoneLinkRunning = false;
    bool hasCortana = false;
    bool hasWidgets = false;
    bool widgetsRunning = false;
};

// Thực thi an toàn đoạn mã PowerShell quản trị thông qua file .ps1 tạm thời
static bool runPowerShellScriptAsAdmin(SystemCore &sc, const string &psScript) {
    const string wrapped = "\xEF\xBB\xBF$ErrorActionPreference = 'Stop'\ntry {\n" + psScript +
                           "\n} catch { Write-Error $_ -ErrorAction Continue; exit 1 }\nexit 0\n";
    FileSafety::LockedScript script(wrapped, L".ps1");
    if (!script) return false;
    return sc.runAdmin("powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" + script.path().u8string() + "\"", true);
}

// Dò quét thực tế trên máy tính (hỗ trợ cả Appx cài đặt cho All Users và Provisioned Package)
static bool scanBloatware(vector<BloatAppInfo> &outSecondary, AdvancedBloatStatus &outAdv) {
    FileSafety::TemporaryDirectory temporary;
    if (!temporary) return false;
    string scanFile = (temporary.path() / L"installed.txt").u8string();
    string escaped = scanFile;
    size_t quote = 0;
    while ((quote = escaped.find('\'', quote)) != string::npos) { escaped.insert(quote, 1, '\''); quote += 2; }
    const string scanScript = "$ErrorActionPreference='Stop'\ntry { & { (Get-AppxPackage -AllUsers).Name; (Get-AppxProvisionedPackage -Online).DisplayName; (Get-AppxProvisionedPackage -Online).PackageName } | Out-File -LiteralPath '" + escaped + "' -Encoding utf8 } catch { exit 1 }\nexit 0\n";
    FileSafety::LockedScript script(scanScript, L".ps1");
    if (!script || !SystemCore::runRawCommand("powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" + script.path().u8string() + "\"")) {
        cout << " [!] Quét Appx thất bại; cần chạy ứng dụng bằng quyền Admin để kiểm tra toàn bộ.\n";
        return false;
    }

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
    outAdv.widgetsRunning = isProcessRunning("Widgets.exe") || isProcessRunning("WidgetService.exe");
    outAdv.hasWidgets = outAdv.widgetsRunning || (installed.find("WebExperience") != string::npos);
    return true;
}

// Luồng 1: Xử lý gỡ sạch app rác thứ cấp
static bool cleanSecondaryBloat(SystemCore &sc, const vector<BloatAppInfo> &list) {
    if (list.empty()) {
        cout << " [✓] Không có ứng dụng rác thứ cấp nào cần gỡ.\n";
        return true;
    }
    cout << " [-] Đang gỡ bỏ " << list.size() << " ứng dụng rác thứ cấp\n";
    string psScript = "$apps = @(\n";
    for (size_t i = 0; i < list.size(); ++i) {
        psScript += "    '" + list[i].id + "'";
        if (i + 1 < list.size()) psScript += ",";
        psScript += "\n";
    }
    psScript += ");\n";
    psScript += "foreach ($pkg in $apps) {\n";
    psScript += "    Get-AppxPackage -AllUsers -Name ('*' + $pkg + '*') -ErrorAction SilentlyContinue | Remove-AppxPackage -AllUsers -ErrorAction Stop;\n";
    psScript += "    Get-AppxProvisionedPackage -Online | Where-Object { $_.DisplayName -like ('*' + $pkg + '*') -or $_.PackageName -like ('*' + $pkg + '*') } | Remove-AppxProvisionedPackage -Online -ErrorAction Stop;\n";
    psScript += "}\n";

    if (!runPowerShellScriptAsAdmin(sc, psScript)) { cout << " [!] Gỡ ứng dụng thất bại hoặc chưa hoàn tất.\n"; return false; }
    cout << " [✓] Đã dọn sạch " << list.size() << " ứng dụng rác thứ cấp thành công!\n";
    return true;
}

// Luồng 2: Xử lý tận gốc app rác nâng cao (Chỉ chạy khi phát hiện có trên máy)
static void cleanAdvancedBloat(SystemCore &sc, const AdvancedBloatStatus &adv) {
    string script;
    if (adv.hasOneDrive) {
        script += "$setup = @('SysWOW64\\OneDriveSetup.exe','System32\\OneDriveSetup.exe') | ForEach-Object { Join-Path $env:SystemRoot $_ } | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1;\n";
        script += "if (-not $setup) { $setup = Join-Path $env:LocalAppData 'Microsoft\\OneDrive\\Update\\OneDriveSetup.exe' };\n";
        script += "if (-not (Test-Path -LiteralPath $setup)) { throw 'OneDrive uninstaller missing' };\n";
        script += "$p = Start-Process -FilePath $setup -ArgumentList '/uninstall' -Wait -PassThru; if ($p.ExitCode -ne 0) { throw 'OneDrive uninstall failed' };\n";
        // Keep user data, sync folders and notifications intact.
    }
    vector<string> names;
    if (adv.hasPhoneLink) { names.push_back("*YourPhone*"); names.push_back("*CrossDevice*"); }
    if (adv.hasCortana) { names.push_back("*Cortana*"); names.push_back("*549981C3F5F10*"); }
    if (adv.hasWidgets) names.push_back("*WebExperience*");
    for (const auto& name : names) {
        script += "Get-AppxPackage -AllUsers -Name '" + name + "' | Remove-AppxPackage -AllUsers -ErrorAction Stop;\n";
        script += "Get-AppxProvisionedPackage -Online | Where-Object { $_.DisplayName -like '" + name + "' } | Remove-AppxProvisionedPackage -Online -ErrorAction Stop;\n";
        script += "if (Get-AppxPackage -AllUsers -Name '" + name + "') { throw 'Package remains installed' };\n";
    }
    if (script.empty()) return;
    if (!runPowerShellScriptAsAdmin(sc, script)) cout << " [!] Gỡ nâng cao thất bại hoặc chưa hoàn tất.\n";
    else cout << " [✓] Lệnh gỡ nâng cao đã hoàn tất.\n";
}

// Gỡ bỏ ứng dụng rác Bloatware (Dọn dẹp toàn diện cả 2 luồng: Thứ cấp & Nâng cao)
void UtilityTools::uninstallBloatware() {
    sc.cls();
    cout << "\n\n";

    vector<BloatAppInfo> detectedSec;
    AdvancedBloatStatus advStatus;
    if (!scanBloatware(detectedSec, advStatus)) { sc.waitEnter(); return; }

    cout << " \x1b[38;2;195;165;255m╭── KẾT QUẢ QUÉT ỨNG DỤNG RÁC ───────────────────────╮\x1b[0m\n";
    
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
    cout << " │   - Windows Widgets (Góc): " << (advStatus.hasWidgets ? (advStatus.widgetsRunning ? "\x1b[33m[Phát hiện - Đang chạy ngầm]\x1b[0m" : "\x1b[33m[Phát hiện gói WebExperience]\x1b[0m") : "\x1b[32m[Sạch]\x1b[0m") << "\n";
    cout << " \x1b[38;2;195;165;255m╰───────────────────────────────────────────────────╯\x1b[0m\n\n";

    bool hasAnySec = !detectedSec.empty();
    bool hasAnyAdv = advStatus.hasOneDrive || advStatus.hasPhoneLink || advStatus.hasCortana || advStatus.hasWidgets;

    if (!hasAnySec && !hasAnyAdv) {
        cout << " \x1b[32m[✓] Hệ thống đã hoàn toàn sạch sẽ\x1b[0m\n";
        sc.waitEnter();
        return;
    }

    if (!SystemCore::confirm("Xác nhận thực hiện dọn dẹp toàn diện cả 2 luồng?")) {
        cout << "Đã hủy thao tác.\n";
        Sleep(800);
        return;
    }

    cout << "\n";
    bool secondaryOk = cleanSecondaryBloat(sc, detectedSec);
    cleanAdvancedBloat(sc, advStatus);

    cout << (secondaryOk ? "\nĐã kết thúc các tác vụ; xem kết quả từng nhóm ở trên.\n" : "\nCó tác vụ thất bại; xem kết quả ở trên.\n");
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
    if (n < 0) return "-" + formatNumber(-n);
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
        cout << "\n \x1b[38;2;195;165;255m╭── THÔNG TIN PIN LAPTOP & SỨC KHỎE ────────────────╮\x1b[0m\n"
             << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n"
             << "Đang đọc dữ liệu ACPI...\n";

        SYSTEM_POWER_STATUS sps;
        bool hasSps = GetSystemPowerStatus(&sps);

        // Kiểm tra thiết bị có pin không (đồng bộ với SystemCore)
        bool hasBattery = hasSps && !(sps.BatteryFlag & 128) && (sps.BatteryLifePercent != 255);
        if (!hasBattery) {
            sc.cls();
            cout << "\n \x1b[38;2;195;165;255m╭── THÔNG TIN PIN LAPTOP & SỨC KHỎE ────────────────╮\x1b[0m\n"
                 << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n"
                 << " [!] Máy tính bàn (PC) hoặc không có Pin.\n"
                 << "     Nguồn: Cắm sạc trực tiếp (AC Online).\n\n";
            sc.waitEnter();
            return;
        }

        // Tạo file XML báo cáo pin tạm thời
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        string xmlPath = string(tempPath) + "cmd_battery_report_" + to_string(GetCurrentProcessId()) + ".xml";
        string cmd = "powercfg.exe /batteryreport /xml /output \"" + xmlPath + "\"";
        SystemCore::runRawCommand(cmd);

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
        cout << "\n \x1b[38;2;195;165;255m╭── THÔNG TIN PIN LAPTOP & SỨC KHỎE ────────────────╮\x1b[0m\n"
             << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n";

        if (!sysMfg.empty() && sysMfg != "N/A") {
            cout << "  Thiết bị     : " << sysMfg << " " << sysModel << " (BIOS: " << biosVer << ")\n";
        }
        if (!deviceName.empty() && deviceName != "N/A") {
            cout << "  Pin          : " << chemistry << " - " << manufacturer << " [" << deviceName << "]\n";
        }
        cout << "\n";

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

        cout << "\n"
             << " [1] Báo cáo HTML | [2] Cài đặt Pin | [R] Làm mới | [0] Quay lại\n"
             << " Chọn: ";

        string opt;
        getline(cin, opt);
        opt = SystemCore::trim(opt);

        if (opt.empty()) continue;
        if (opt == "0") break;

        if (opt == "1") {
            cout << "\nĐang xuất báo cáo HTML\n";
            char tempHtml[MAX_PATH];
            GetTempPathA(MAX_PATH, tempHtml);
            string htmlPath = string(tempHtml) + "battery_report.html";
            string genCmd = "powercfg.exe /batteryreport /output \"" + htmlPath + "\"";
            SystemCore::runRawCommand(genCmd);

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
            cout << "\nĐã mở cài đặt Pin\n";
            Sleep(800);
            continue;
        }

        if (opt == "R" || opt == "r") {
            continue;
        }
    }
}

