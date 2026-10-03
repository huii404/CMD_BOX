#ifndef EMBEDDED_SCRIPTS_H
#define EMBEDDED_SCRIPTS_H

#include <string>

namespace EmbeddedScripts {

// 1. Script sửa lỗi mạng (DNS, Winsock, TCP/IP, ARP, DHCP, WinNAT/HNS)
inline const char REPAIR_NETWORK_BAT[] = R"bat(@echo off
setlocal EnableExtensions
chcp 65001 >nul
set "failed=0"

echo [1/6] Xoa bo dem DNS...
ipconfig /flushdns >nul 2>&1
if errorlevel 1 set "failed=1"

echo [2/6] Dat lai Winsock Catalog...
netsh winsock reset >nul 2>&1
if errorlevel 1 set "failed=1"

echo [3/6] Dat lai ngan xep TCP/IP...
netsh int ip reset >nul 2>&1
if errorlevel 1 set "failed=1"

echo [4/6] Xoa bang ARP Cache...
netsh interface ip delete arpcache >nul 2>&1
if errorlevel 1 set "failed=1"

echo [5/6] Lam moi dia chi IP (DHCP)...
ipconfig /release >nul 2>&1
ipconfig /renew >nul 2>&1

echo [6/6] Khoi dong lai WinNAT va HNS...
net stop winnat >nul 2>&1
net start winnat >nul 2>&1
net stop hns >nul 2>&1
net start hns >nul 2>&1

exit /b %failed%
)bat";

// 2. Script làm sạch và khôi phục Windows Update
inline const char RESET_WINDOWS_UPDATE_BAT[] = R"bat(@echo off
setlocal EnableExtensions
chcp 65001 >nul
set "failed=0"

echo [1/3] Dung cac dich vu Windows Update...
net stop wuauserv >nul 2>&1
net stop cryptSvc >nul 2>&1
net stop bits >nul 2>&1
net stop msiserver >nul 2>&1

echo [2/3] Xoa cache cap nhat ton dong...
del /f /q "%windir%\SoftwareDistribution\*.*" >nul 2>&1
rd /s /q "%windir%\SoftwareDistribution" >nul 2>&1
rd /s /q "%windir%\System32\catroot2" >nul 2>&1

if exist "%windir%\SoftwareDistribution" set "failed=1"
if exist "%windir%\System32\catroot2" set "failed=1"

echo [3/3] Khoi dong lai cac dich vu...
net start msiserver >nul 2>&1
net start bits >nul 2>&1
net start cryptSvc >nul 2>&1
net start wuauserv >nul 2>&1

exit /b %failed%
)bat";

// 3. Script tinh chỉnh Registry & giao diện Taskbar
inline const char OPTIMIZE_REGISTRY_BAT[] = R"bat(@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul

if /i "%~1"=="user" goto :user_settings
if /i "%~1"=="machine" goto :machine_settings
echo [LOI] Tham so khong hop le. Dung: user hoac machine.
exit /b 2

:user_settings
set "failed=0"
set "explorer_changed=0"

call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\Search" "SearchboxTaskbarMode" 0 1
call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced" "TaskbarDa" 0 1
call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced" "TaskbarMn" 0 1
call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced" "ShowTaskViewButton" 0 1
call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\Feeds" "ShellFeedsTaskbarViewMode" 2 1
call :set_dword "HKCU\Software\Policies\Microsoft\Windows\WindowsCopilot" "TurnOffWindowsCopilot" 1 1
call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize" "EnableTransparency" 0 0
call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced" "SnapAssist" 0 1
call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\ContentDeliveryManager" "SilentInstalledAppsEnabled" 0 0
call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\ContentDeliveryManager" "SubscribedContent-310093Enabled" 0 0
call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\ContentDeliveryManager" "SubscribedContent-338388Enabled" 0 0
call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\ContentDeliveryManager" "SubscribedContent-338389Enabled" 0 0
call :set_dword "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced" "ShowSyncProviderNotifications" 0 0
call :set_string "HKCU\Control Panel\Desktop" "MenuShowDelay" "0"

if "!explorer_changed!"=="1" (
    taskkill /f /im explorer.exe >nul 2>&1
    start "" explorer.exe
)
exit /b !failed!

:machine_settings
set "failed=0"
call :set_dword "HKLM\SOFTWARE\Policies\Microsoft\Windows\DataCollection" "AllowTelemetry" 0 0
exit /b !failed!

:set_dword
set "reg_key=%~1"
set "reg_name=%~2"
set "target_value=%~3"
set "restart_explorer=%~4"
set "current_value="
set "current_number="

for /f "tokens=3" %%V in ('reg query "%reg_key%" /v "%reg_name%" 2^>nul ^| findstr /i /c:"%reg_name%"') do set "current_value=%%V"
if defined current_value (
    set /a current_number=!current_value! 2>nul
    if "!current_number!"=="%target_value%" exit /b 0
)

reg add "%reg_key%" /v "%reg_name%" /t REG_DWORD /d "%target_value%" /f >nul 2>&1
if errorlevel 1 (
    set "failed=1"
    exit /b 1
)
if "%restart_explorer%"=="1" set "explorer_changed=1"
exit /b 0

:set_string
set "reg_key=%~1"
set "reg_name=%~2"
set "target_value=%~3"
set "current_value="

for /f "tokens=3,*" %%V in ('reg query "%reg_key%" /v "%reg_name%" 2^>nul ^| findstr /i /c:"%reg_name%"') do set "current_value=%%V"
if "!current_value!"=="%target_value%" exit /b 0

reg add "%reg_key%" /v "%reg_name%" /t REG_SZ /d "%target_value%" /f >nul 2>&1
if errorlevel 1 (
    set "failed=1"
    exit /b 1
)
exit /b 0
)bat";

} // namespace EmbeddedScripts

#endif // EMBEDDED_SCRIPTS_H
