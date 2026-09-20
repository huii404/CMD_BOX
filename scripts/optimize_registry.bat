@echo off
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
