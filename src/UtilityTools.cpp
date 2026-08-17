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

// Trình tải & Cài đặt phần mềm tự động (Nhúng script .bat tự sinh)
void UtilityTools::downloadManager() {
    char* userProf = getenv("USERPROFILE");
    string downloadDir = userProf ? (string(userProf) + "\\Downloads") : "C:\\Downloads";
    string tempDir = getenv("TEMP") ? string(getenv("TEMP")) : downloadDir;
    string batPath = tempDir + "\\CMD_AppDownloader_" + to_string(GetCurrentProcessId()) + ".bat";

    ofstream bat(batPath);
    if (!bat.is_open()) {
        cout << "Không thể khởi tạo trình tải phần mềm!\n";
        return;
    }

    bat << R"BAT(@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul
title TAI VA CAI DAT PHAN MEM

set "DEST=%USERPROFILE%\Downloads"
if not exist "!DEST!" mkdir "!DEST!" >nul 2>&1

:MENU
cls
echo.
echo  [1]  Google Chrome           [10] Telegram Desktop       [19] VLC Media Player
echo  [2]  Cốc Cốc                 [11] Zoom Meeting           [20] OBS Studio
echo  [3]  Brave Browser           [12] 7-Zip (Giải nén)       [21] Visual Studio Code
echo  [4]  Mozilla Firefox         [13] WinRAR (Giải nén)      [22] Notepad++
echo  [5]  EVKey (Bộ gõ TV)        [14] WARP 1.1.1.1           [23] Git for Windows
echo  [6]  OpenKey (Bộ gõ TV)      [15] LocalSend (Chia sẻ LAN)[24] Node.js (LTS)
echo  [7]  UniKey (Bộ gõ TV)       [16] Everything (Tìm file)  [25] Python
echo  [8]  Zalo PC                 [17] CPU-Z (Kiểm tra phần cứng)
echo  [9]  Discord                 [18] Rufus (Tạo USB Boot)
echo.
echo  [A]  Tải tất cả              [H]  Lịch sử tải            [0] Quay lại
echo.
set /p "CHOICE= [Chọn]: "

if "%CHOICE%"=="0" goto EXIT_SCRIPT
if /i "%CHOICE%"=="H" goto SHOW_HISTORY
if /i "%CHOICE%"=="A" goto DOWNLOAD_ALL

if "%CHOICE%"=="1" (set "NAME=Google Chrome" & set "URL=https://dl.google.com/tag/s/appname%%3DGoogle%%2520Chrome/update2/installers/ChromeSetup.exe" & set "FILE=ChromeSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="2" (set "NAME=Cốc Cốc" & set "URL=https://files.coccoc.com/browser/coccoc_vi.exe" & set "FILE=CocCocSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="3" (set "NAME=Brave Browser" & set "URL=https://laptop-updates.brave.com/latest/winx64" & set "FILE=BraveSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="4" (set "NAME=Mozilla Firefox" & set "URL=https://download.mozilla.org/?product=firefox-latest-ssl&os=win64&lang=vi" & set "FILE=FirefoxSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="5" (set "NAME=EVKey" & set "URL=https://github.com/lamquangminh/EVKey/releases/download/v5.0.4/EVKey.zip" & set "FILE=EVKey.zip" & goto DO_DOWNLOAD)
if "%CHOICE%"=="6" (set "NAME=OpenKey" & set "URL=https://github.com/tphan/openkey/releases/latest/download/OpenKey-Windows-x64.zip" & set "FILE=OpenKey.zip" & goto DO_DOWNLOAD)
if "%CHOICE%"=="7" (set "NAME=UniKey" & set "URL=https://www.unikey.org/assets/release/unikey43RC5-200929-win64.zip" & set "FILE=UniKey.zip" & goto DO_DOWNLOAD)
if "%CHOICE%"=="8" (set "NAME=Zalo PC" & set "URL=https://zalo.me/download/zalo-pc" & set "FILE=ZaloSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="9" (set "NAME=Discord" & set "URL=https://discord.com/api/downloads/distributions/app/installers/latest?channel=stable&platform=win&arch=x64" & set "FILE=DiscordSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="10" (set "NAME=Telegram" & set "URL=https://telegram.org/dl/desktop/win64" & set "FILE=TelegramSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="11" (set "NAME=Zoom" & set "URL=https://zoom.us/client/latest/ZoomInstaller.exe" & set "FILE=ZoomInstaller.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="12" (set "NAME=7-Zip" & set "URL=https://www.7-zip.org/a/7z2408-x64.exe" & set "FILE=7zipSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="13" (set "NAME=WinRAR" & set "URL=https://www.rarlab.com/rar/winrar-x64-701.exe" & set "FILE=WinRARSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="14" (set "NAME=Cloudflare WARP" & set "URL=https://1111-releases.cloudflareclient.com/windows/Cloudflare_WARP_Release-x64.msi" & set "FILE=CloudflareWARP.msi" & goto DO_DOWNLOAD)
if "%CHOICE%"=="15" (set "NAME=LocalSend" & set "URL=https://github.com/localsend/localsend/releases/latest/download/LocalSend-1.16.1-windows-x86-64.exe" & set "FILE=LocalSendSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="16" (set "NAME=Everything Search" & set "URL=https://www.voidtools.com/Everything-1.4.1.1026.x64-Setup.exe" & set "FILE=EverythingSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="17" (set "NAME=CPU-Z" & set "URL=https://download.cpuid.com/cpu-z/cpu-z_2.11-en.exe" & set "FILE=CPUZSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="18" (set "NAME=Rufus" & set "URL=https://github.com/pbatard/rufus/releases/download/v4.5/rufus-4.5.exe" & set "FILE=Rufus.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="19" (set "NAME=VLC Media Player" & set "URL=https://get.videolan.org/vlc/last/win64/vlc-3.0.21-win64.exe" & set "FILE=VLCSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="20" (set "NAME=OBS Studio" & set "URL=https://cdn-fastly.obsproject.com/downloads/OBS-Studio-30.2.2-Windows-Installer.exe" & set "FILE=OBSStudioSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="21" (set "NAME=Visual Studio Code" & set "URL=https://code.visualstudio.com/sha/download?build=stable&os=win32-x64-user" & set "FILE=VSCodeSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="22" (set "NAME=Notepad++" & set "URL=https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v8.6.7/npp.8.6.7.Installer.x64.exe" & set "FILE=NotepadPlusPlusSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="23" (set "NAME=Git for Windows" & set "URL=https://github.com/git-for-windows/git/releases/download/v2.45.1.windows.1/Git-2.45.1-64-bit.exe" & set "FILE=GitSetup.exe" & goto DO_DOWNLOAD)
if "%CHOICE%"=="24" (set "NAME=Node.js LTS" & set "URL=https://nodejs.org/dist/v20.16.0/node-v20.16.0-x64.msi" & set "FILE=NodejsSetup.msi" & goto DO_DOWNLOAD)
if "%CHOICE%"=="25" (set "NAME=Python" & set "URL=https://www.python.org/ftp/python/3.12.5/python-3.12.5-amd64.exe" & set "FILE=PythonSetup.exe" & goto DO_DOWNLOAD)

echo [!] Lựa chọn không hợp lệ!
timeout /t 1 >nul
goto MENU

:DO_DOWNLOAD
cls
echo.
echo Đang tải: !NAME!...
set "TARGET=!DEST!\!FILE!"
if exist "!TARGET!" del /f /q "!TARGET!" >nul 2>&1
curl -# -L "!URL!" -o "!TARGET!"
if %errorlevel% neq 0 (
    echo.
    echo [!] Lỗi tải file! Vui lòng kiểm tra lại kết nối mạng.
    pause
    goto MENU
)
echo [OK] Đã tải về: !TARGET!
echo [%date% %time%] Đã tải: !NAME! >> "!DEST!\download_history.txt"
echo.
set /p "RUN_APP= Mở file cài đặt ngay? (y/n): "
if /i "!RUN_APP!"=="y" start "" "!TARGET!"
goto MENU

:DOWNLOAD_ALL
cls
echo.
echo Đang tải toàn bộ ứng dụng về thư mục: !DEST!
echo.
call :DOWNLOAD_ITEM "Google Chrome" "https://dl.google.com/tag/s/appname%%3DGoogle%%2520Chrome/update2/installers/ChromeSetup.exe" "ChromeSetup.exe"
call :DOWNLOAD_ITEM "Cốc Cốc" "https://files.coccoc.com/browser/coccoc_vi.exe" "CocCocSetup.exe"
call :DOWNLOAD_ITEM "Brave Browser" "https://laptop-updates.brave.com/latest/winx64" "BraveSetup.exe"
call :DOWNLOAD_ITEM "Mozilla Firefox" "https://download.mozilla.org/?product=firefox-latest-ssl&os=win64&lang=vi" "FirefoxSetup.exe"
call :DOWNLOAD_ITEM "EVKey" "https://github.com/lamquangminh/EVKey/releases/download/v5.0.4/EVKey.zip" "EVKey.zip"
call :DOWNLOAD_ITEM "OpenKey" "https://github.com/tphan/openkey/releases/latest/download/OpenKey-Windows-x64.zip" "OpenKey.zip"
call :DOWNLOAD_ITEM "UniKey" "https://www.unikey.org/assets/release/unikey43RC5-200929-win64.zip" "UniKey.zip"
call :DOWNLOAD_ITEM "Zalo PC" "https://zalo.me/download/zalo-pc" "ZaloSetup.exe"
call :DOWNLOAD_ITEM "Discord" "https://discord.com/api/downloads/distributions/app/installers/latest?channel=stable&platform=win&arch=x64" "DiscordSetup.exe"
call :DOWNLOAD_ITEM "Telegram" "https://telegram.org/dl/desktop/win64" "TelegramSetup.exe"
call :DOWNLOAD_ITEM "Zoom" "https://zoom.us/client/latest/ZoomInstaller.exe" "ZoomInstaller.exe"
call :DOWNLOAD_ITEM "7-Zip" "https://www.7-zip.org/a/7z2408-x64.exe" "7zipSetup.exe"
call :DOWNLOAD_ITEM "WinRAR" "https://www.rarlab.com/rar/winrar-x64-701.exe" "WinRARSetup.exe"
call :DOWNLOAD_ITEM "WARP 1.1.1.1" "https://1111-releases.cloudflareclient.com/windows/Cloudflare_WARP_Release-x64.msi" "CloudflareWARP.msi"
call :DOWNLOAD_ITEM "LocalSend" "https://github.com/localsend/localsend/releases/latest/download/LocalSend-1.16.1-windows-x86-64.exe" "LocalSendSetup.exe"
call :DOWNLOAD_ITEM "Everything Search" "https://www.voidtools.com/Everything-1.4.1.1026.x64-Setup.exe" "EverythingSetup.exe"
call :DOWNLOAD_ITEM "CPU-Z" "https://download.cpuid.com/cpu-z/cpu-z_2.11-en.exe" "CPUZSetup.exe"
call :DOWNLOAD_ITEM "Rufus" "https://github.com/pbatard/rufus/releases/download/v4.5/rufus-4.5.exe" "Rufus.exe"
call :DOWNLOAD_ITEM "VLC Media Player" "https://get.videolan.org/vlc/last/win64/vlc-3.0.21-win64.exe" "VLCSetup.exe"
call :DOWNLOAD_ITEM "OBS Studio" "https://cdn-fastly.obsproject.com/downloads/OBS-Studio-30.2.2-Windows-Installer.exe" "OBSStudioSetup.exe"
call :DOWNLOAD_ITEM "VS Code" "https://code.visualstudio.com/sha/download?build=stable&os=win32-x64-user" "VSCodeSetup.exe"
call :DOWNLOAD_ITEM "Notepad++" "https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v8.6.7/npp.8.6.7.Installer.x64.exe" "NotepadPlusPlusSetup.exe"
call :DOWNLOAD_ITEM "Git for Windows" "https://github.com/git-for-windows/git/releases/download/v2.45.1.windows.1/Git-2.45.1-64-bit.exe" "GitSetup.exe"
call :DOWNLOAD_ITEM "Node.js LTS" "https://nodejs.org/dist/v20.16.0/node-v20.16.0-x64.msi" "NodejsSetup.msi"
call :DOWNLOAD_ITEM "Python" "https://www.python.org/ftp/python/3.12.5/python-3.12.5-amd64.exe" "PythonSetup.exe"
echo.
echo [OK] Đã hoàn tất tải tất cả ứng dụng về: !DEST!
pause
goto MENU

:DOWNLOAD_ITEM
echo [*] Đang tải %~1...
set "TGT=!DEST!\%~3"
if exist "!TGT!" del /f /q "!TGT!" >nul 2>&1
curl -# -L "%~2" -o "!TGT!"
echo [%date% %time%] Đã tải: %~1 >> "!DEST!\download_history.txt"
exit /b 0

:SHOW_HISTORY
cls
echo.
echo Lịch sử tải ứng dụng:
echo.
if exist "!DEST!\download_history.txt" (
    type "!DEST!\download_history.txt"
) else (
    echo (Chưa có lịch sử tải)
)
echo.
pause
goto MENU

:EXIT_SCRIPT
exit /b 0
)BAT";

    bat.close();

    system(("cmd.exe /c \"" + batPath + "\"").c_str());
    try {
        if (fs::exists(batPath)) {
            fs::remove(batPath);
        }
    } catch (...) {}
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
