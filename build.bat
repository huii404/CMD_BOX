@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: ========================================================
:: MA MAU ANSI NEON
:: ========================================================
for /f %%a in ('echo prompt $e ^| cmd') do set "ESC=%%a"
for /f %%a in ('copy /z "%~dpf0" nul') do set "CR=%%a"
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

:: 3. Bien dich voi hieu ung loading thoi gian thuc
echo.
echo %C_PINK%  [☕] Bắt đầu nấu code (-O3, AVX2, FMA, OpenMP)...%C_RESET%

del /f /q "%TEMP%\cmd_build_done.txt" "%TEMP%\cmd_build_err.log" 2>nul

start /b "" cmd /c ""!GXX!" -std=c++17 -O3 -fopenmp -mavx2 -mfma -Iinclude src\*.cpp -o bin\main.exe -lws2_32 -liphlpapi -lole32 -lwindowscodecs -loleaut32 -luuid -static-libgcc -static-libstdc++ -static -s > "%TEMP%\cmd_build_err.log" 2>&1 & echo %%errorlevel%% > "%TEMP%\cmd_build_done.txt""

set /a STEP=0
set "SPIN_CHARS=/-\|"

:WAIT_LOOP
if exist "%TEMP%\cmd_build_done.txt" goto DONE_BUILD

set /a "IDX=STEP %% 4"
set "CH=!SPIN_CHARS:~%IDX%,1!"
set /a "T_SEC=STEP / 10"
set /a "T_DEC=STEP %% 10"

<nul set /p "=!CR!%C_PINK%  [!CH!] %C_CYAN%Đang nấu code... %C_YELLOW%!T_SEC!.!T_DEC!s%C_RESET%   "

powershell -nop -c "Start-Sleep -m 100" >nul 2>nul
set /a STEP+=1
goto WAIT_LOOP

:DONE_BUILD
set /a "T_SEC=STEP / 10"
set /a "T_DEC=STEP %% 10"
set /p BUILD_STATUS=<"%TEMP%\cmd_build_done.txt"
set "BUILD_STATUS=!BUILD_STATUS: =!"

if not "!BUILD_STATUS!"=="0" (
    echo.
    echo.
    echo %BG_RED%  [X] BÙMM! TOANG RỒI BU EM ƠI!  %C_RESET%
    echo %C_RED%%C_BOLD%  Lỗi biên dịch trong code kia kìa:%C_RESET%
    echo %C_YELLOW%
    if exist "%TEMP%\cmd_build_err.log" type "%TEMP%\cmd_build_err.log"
    echo %C_RESET%
    del /f /q "%TEMP%\cmd_build_done.txt" "%TEMP%\cmd_build_err.log" 2>nul
    pause
    exit /b 1
)

del /f /q "%TEMP%\cmd_build_done.txt" "%TEMP%\cmd_build_err.log" 2>nul
if exist "src\apps.txt" copy /y "src\apps.txt" "bin\apps.txt" >nul

echo.
echo !CR!%BG_GREEN%  [OK] BUILD XONG TRONG !T_SEC!.!T_DEC!s! SIÊU MƯỢT  %C_RESET%           
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
