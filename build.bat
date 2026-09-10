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
echo %C_PINK%%C_BOLD%  ======================================================%C_RESET%
echo %C_CYAN%%C_BOLD%       ____ __  __ ____       ____   ______  __%C_RESET%
echo %C_CYAN%%C_BOLD%      / ___]  \/  ]  _ \     ] __ ) / _ \ \/ /%C_RESET%
echo %C_GREEN%%C_BOLD%     [ [   ] [\/] [ ] ] ]    ]  _ \[ [ ] ]\  / %C_RESET%
echo %C_GREEN%%C_BOLD%     [ [___] [  ] [ ]_] ]    ] ]_) ] [_] ]/  \ %C_RESET%
echo %C_YELLOW%%C_BOLD%      \____]_]  [_]____/     ]____/ \___//_/\_\%C_RESET%
echo %C_PINK%%C_BOLD%  ======================================================%C_RESET%
echo       %BG_PURPLE%  * TOOLKIT PRO BUILDER - CHẠY LÀ MƯỢT *  %C_RESET%
echo.

:: 1. Soi trinh bien dich g++
echo %C_BLUE%[?] Đang soi compiler...%C_RESET%
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
    echo %BG_RED% [!] TOANG! %C_RESET% %C_RED%%C_BOLD%Không tìm thấy g++ đâu cả! Cài MinGW/MSYS2 gấp!%C_RESET%
    echo.
    pause
    exit /b 1
)

echo %C_GREEN%  [v] Compiler: %C_YELLOW%!GXX! %C_GREEN%[Uy tín]%C_RESET%

:: 2. Thu muc bin
if not exist "bin" mkdir "bin"

:: 3. Bien dich voi loading thoi gian thuc tai cho (1 file duy nhat, chu dung im so nhay)
echo.
if exist "%TEMP%\cmd_build_err.log" del /f /q "%TEMP%\cmd_build_err.log" 2>nul

powershell -NoProfile -ExecutionPolicy Bypass -Command "$e=[string][char]27; $sw=[System.Diagnostics.Stopwatch]::StartNew(); $p=Start-Process -FilePath '!GXX!' -ArgumentList '-std=c++17 -O3 -fopenmp -mavx2 -mfma -Iinclude src\*.cpp -o bin\main.exe -lws2_32 -liphlpapi -lole32 -lwindowscodecs -loleaut32 -luuid -static-libgcc -static-libstdc++ -static -s' -NoNewWindow -PassThru -RedirectStandardError $env:TEMP\cmd_build_err.log; $frames=@('|','/','-','\'); $i=0; Write-Host -NoNewline ('  ' + $e + '[96m[☕] Đang nấu code:' + $e + '[0m ' + $e + '[s'); while(-not $p.HasExited){ $s=$sw.Elapsed.TotalSeconds.ToString('0.0'); $f=$frames[$i%%4]; $disp=$e+'[u'+$e+'[95m['+$f+']'+$e+'[0m '+$e+'[93m'+$s+'s'+$e+'[0m'+$e+'[K'; Write-Host -NoNewline $disp; Start-Sleep -Milliseconds 80; $i++ }; $p.WaitForExit(); $tot=$sw.Elapsed.TotalSeconds.ToString('0.0'); if($p.ExitCode -eq 0){ Write-Host ($e+'[u'+$e+'[42;30m [OK] '+$tot+'s! SIÊU MƯỢT '+$e+'[0m'+$e+'[K') } else { Write-Host ($e+'[u'+$e+'[41;97m [X] TOANG! ('+$tot+'s) '+$e+'[0m'+$e+'[K') }; exit $p.ExitCode"

set BUILD_RET=%errorlevel%

if not "!BUILD_RET!"=="0" (
    echo.
    echo %C_RED%%C_BOLD%  Lỗi biên dịch trong code kia kìa:%C_RESET%
    echo %C_YELLOW%
    if exist "%TEMP%\cmd_build_err.log" type "%TEMP%\cmd_build_err.log"
    echo %C_RESET%
    del /f /q "%TEMP%\cmd_build_err.log" 2>nul
    pause
    exit /b 1
)

del /f /q "%TEMP%\cmd_build_err.log" 2>nul
if exist "src\apps.txt" copy /y "src\apps.txt" "bin\apps.txt" >nul

echo %C_GREEN%  [+] Ra lò: %C_YELLOW%bin\main.exe %C_GREEN%[Chạy là bay]%C_RESET%
echo.

:: 4. Tuy chon mo app
set "RUN_APP="
echo %C_PINK%  ======================================================%C_RESET%
set /p "RUN_APP=%C_CYAN%[?] Mở main.exe luôn không? [%C_YELLOW%y%C_CYAN% = Mở, %C_WHITE%Enter%C_CYAN% = Thôi]: %C_RESET%"

if /i "!RUN_APP!"=="y" (
    echo.
    echo %C_GREEN%  [O_O] Đang phóng vào app... Vèo vèo!%C_RESET%
    start "" "bin\main.exe"
) else (
    echo.
    echo %C_YELLOW%  [O_O] Hẹn gặp lại đại ca! Bye!%C_RESET%
)

echo.
