@echo off
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

:: 1. Soi trinh bien dich g++
echo %C_CYAN%[📿] Đang thỉnh pháp bảo Compiler...%C_RESET%
set "GXX="
where g++ >nul 2>nul
if %errorlevel% equ 0 (
    set "GXX=g++"
) else if exist "C:\msys64\ucrt64\bin\g++.exe" (
    set "GXX=C:\msys64\ucrt64\bin\g++.exe"
) else if exist "C:\msys64\mingw64\bin\g++.exe" (
    set "GXX=C:\msys64\mingw64\bin\g++.exe"
)

if "!GXX!"=="" (
    echo.
    echo %BG_RED% [!] NGHIỆP TỤ! %C_RESET% %C_RED%%C_BOLD%Chưa có g++! Hãy cài MinGW/MSYS2 để tích đức tu luyện!%C_RESET%
    echo.
    pause
    exit /b 1
)

echo %C_GREEN%  [🙏] Pháp bảo: %C_YELLOW%!GXX! %C_GREEN%[Đã khai quang và chứng giám]%C_RESET%

:: 2. Thu muc bin
if not exist "bin" mkdir "bin"

:: 3. Bien dich voi loading thoi gian thuc tai cho (1 file duy nhat, chu dung im so nhay)
echo.
if exist "%TEMP%\cmd_build_err.log" del /f /q "%TEMP%\cmd_build_err.log" 2>nul

powershell -NoProfile -ExecutionPolicy Bypass -Command "$e=[string][char]27; $sw=[System.Diagnostics.Stopwatch]::StartNew(); $p=Start-Process -FilePath '!GXX!' -ArgumentList '-std=c++17 -O3 -fopenmp -mavx2 -mfma -Iinclude src\*.cpp src\core\*.cpp src\optimizer\*.cpp src\network\*.cpp src\tools\*.cpp src\media\*.cpp -o bin\main.exe -lws2_32 -liphlpapi -lole32 -lwindowscodecs -loleaut32 -luuid -static-libgcc -static-libstdc++ -static -s' -NoNewWindow -PassThru -RedirectStandardError $env:TEMP\cmd_build_err.log; $frames=@('|','/','-','\'); $i=0; Write-Host -NoNewline ('  ' + $e + '[93m[🪷] Đang tụng kinh độ code:' + $e + '[0m ' + $e + '[s'); while(-not $p.HasExited){ $s=$sw.Elapsed.TotalSeconds.ToString('0.0'); $f=$frames[$i%%4]; $disp=$e+'[u'+$e+'[95m['+$f+']'+$e+'[0m '+$e+'[93m'+$s+'s'+$e+'[0m'+$e+'[K'; Write-Host -NoNewline $disp; Start-Sleep -Milliseconds 80; $i++ }; $p.WaitForExit(); $tot=$sw.Elapsed.TotalSeconds.ToString('0.0'); $ec=$p.ExitCode; if($null -eq $ec -or $ec -eq 0){ Write-Host ($e+'[u'+$e+'[42;30m [ĐẮC ĐẠO] '+$tot+'s! CODE THÀNH CHÍNH QUẢ, KHÔNG BUG '+$e+'[0m'+$e+'[K'); exit 0 } else { Write-Host ($e+'[u'+$e+'[41;97m [NGHIỆP QUẢ] ('+$tot+'s) CODE VƯỚNG BỤI TRẦN, CÒN BUG '+$e+'[0m'+$e+'[K'); exit $ec }"

set BUILD_RET=%errorlevel%

if not "!BUILD_RET!"=="0" (
    echo.
    echo %C_RED%%C_BOLD%  Nghiệp báo biên dịch hiện hình ở đây:%C_RESET%
    echo %C_YELLOW%
    if exist "%TEMP%\cmd_build_err.log" type "%TEMP%\cmd_build_err.log"
    echo %C_RESET%
    del /f /q "%TEMP%\cmd_build_err.log" 2>nul
    pause
    exit /b 1
)

del /f /q "%TEMP%\cmd_build_err.log" 2>nul
if exist "src\apps.txt" copy /y "src\apps.txt" "bin\apps.txt" >nul

echo %C_GREEN%  [☸] Đã độ thành công: %C_YELLOW%bin\main.exe %C_GREEN%[Viên mãn - Vạn bug tiêu tán]%C_RESET%
echo.

:: 4. Tuy chon mo app
set "RUN_APP="
echo %C_PINK%  ========================================================================%C_RESET%
set /p "RUN_APP=%C_CYAN%[🧘] Xuất quan khởi động main.exe luôn không? [%C_YELLOW%y%C_CYAN% = Khởi động, %C_WHITE%Enter%C_CYAN% = Thôi]: %C_RESET%"

if /i "!RUN_APP!"=="y" (
    echo.
    echo %C_GREEN%  [⚡] Vạn sự hanh thông! Đang phóng vào app... A Di Đà Phật!%C_RESET%
    start "" "bin\main.exe"
) else (
    echo.
    echo %C_YELLOW%  [🙏] Thiện tai thiện tai! Chúc thí chủ vạn dặm bình an!%C_RESET%
)

echo.
