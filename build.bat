@echo off
cd /d "%~dp0"
chcp 65001 >nul
setlocal enabledelayedexpansion

:: ========================================================
:: MA MAU ANSI NEON
:: ========================================================
for /f %%a in ('powershell -nop -c [char]27') do set "ESC=%%a"
set "C_RESET=%ESC%[0m"
set "C_BOLD=%ESC%[1m"
set "C_RED=%ESC%[91m"
set "C_GREEN=%ESC%[92m"
set "C_YELLOW=%ESC%[93m"
set "C_BLUE=%ESC%[94m"
set "C_PINK=%ESC%[95m"
set "C_CYAN=%ESC%[96m"
set "C_WHITE=%ESC%[97m"

set "BG_PURPLE=%ESC%[45;97m"
set "BG_GREEN=%ESC%[42;30m"
set "BG_RED=%ESC%[41;97m"

cls
echo.
echo %C_YELLOW%%C_BOLD%  ========================================================================%C_RESET%
echo %C_YELLOW%%C_BOLD%                   _ooOoo_                                                %C_RESET%
echo %C_YELLOW%%C_BOLD%                  o8888888o               %C_CYAN%%C_BOLD%NAM MÔ A DI ĐÀ PHẬT            %C_RESET%
echo %C_YELLOW%%C_BOLD%                  88" . "88           %C_YELLOW%%C_BOLD%[ PHẬT TỔ ĐỘ TRÌ BUILDER ]       %C_RESET%
echo %C_YELLOW%%C_BOLD%                  (│ -_- │)                                               %C_RESET%
echo %C_YELLOW%%C_BOLD%                  O\  =  /O            %C_GREEN%Phật quang phổ chiếu              %C_RESET%
echo %C_YELLOW%%C_BOLD%               ____/`---'\____         %C_PINK%Độ code không Warning, sạch Bug    %C_RESET%
echo %C_YELLOW%%C_BOLD%             .'  \\│     │//  `.       %C_CYAN%Độ app chạy êm, mượt không Lag      %C_RESET%
echo %C_YELLOW%%C_BOLD%            /  \\│││  :  │││//  \      %C_GREEN%G++ biên dịch vèo vèo qua môn     %C_RESET%
echo %C_YELLOW%%C_BOLD%           /  _│││││ -:- │││││-  \     %C_WHITE%RAM sạch thông thoáng, không rò rỉ %C_RESET%
echo %C_YELLOW%%C_BOLD%           │   │ \\\  -  /// │   │                                         %C_RESET%
echo %C_YELLOW%%C_BOLD%           │ \_│  ''\---/''  │   │     %C_YELLOW%* Khẩu quyết giải nghiệp:        %C_RESET%
echo %C_YELLOW%%C_BOLD%           \  .-\__  `-`  ___/-. /     %C_WHITE%"Tâm bất biến giữa dòng đời vạn bug%C_RESET%
echo %C_YELLOW%%C_BOLD%         ___`. .'  /--.--\  `. . ___   %C_WHITE% Vạn sự tùy duyên, compile tùy đức!"%C_RESET%
echo %C_YELLOW%%C_BOLD%      .─"" '‹  `.___\_~_/___.'  ›'"─.                                    %C_RESET%
echo %C_YELLOW%%C_BOLD%     │ │ :  `- \`.;`\ _ /`;.`/ - ` : │ │                                  %C_RESET%
echo %C_YELLOW%%C_BOLD%     \  \ `-.   \_ __\ /__ _/   .-` /  /                                  %C_RESET%
echo %C_YELLOW%%C_BOLD%  =====`-.____`-.___\_____/___.-`____.-'=====                             %C_RESET%
echo %C_YELLOW%%C_BOLD%                    `=---='                                               %C_RESET%
echo %C_YELLOW%%C_BOLD%  ========================================================================%C_RESET%
echo       %BG_PURPLE%  * THẦN CHÚ ĐỘ CODE: NAM MÔ A DI ĐÀ PHẬT - COMPILE KHÔNG LỖI *  %C_RESET%
echo.

:: Build engine: cache object/header dependencies, cached icon, bounded parallelism.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build.ps1" %*
if errorlevel 1 goto build_failed
exit /b 0

:build_failed
for %%A in (%*) do if /i "%%~A"=="-NoRun" exit /b 1
pause
exit /b 1
