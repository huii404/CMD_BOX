@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: ========================================================
:: MA MAU ANSI SIEU LOE LOET (RGB NEON)
:: ========================================================
for /f %%a in ('echo prompt $e ^| cmd') do set "ESC=%%a"
set "C_RESET=%ESC%[0m"
set "C_BOLD=%ESC%[1m"

:: Mau chu neon
set "C_RED=%ESC%[91m"
set "C_GREEN=%ESC%[92m"
set "C_YELLOW=%ESC%[93m"
set "C_BLUE=%ESC%[94m"
set "C_PINK=%ESC%[95m"
set "C_CYAN=%ESC%[96m"
set "C_WHITE=%ESC%[97m"

:: Mau nen + chu
set "BG_PURPLE=%ESC%[45;97m"
set "BG_BLUE=%ESC%[44;93m"
set "BG_GREEN=%ESC%[42;30m"
set "BG_RED=%ESC%[41;97m"
set "BG_CYAN=%ESC%[46;30m"

cls
echo.
echo %C_PINK%%C_BOLD%  ========================================================================%C_RESET%
echo %C_CYAN%%C_BOLD%       ____ __  __ ____       ____   ______  __   %C_YELLOW%[B-]%C_RESET%
echo %C_CYAN%%C_BOLD%      / ___]  \/  ]  _ \     ] __ ) / _ \ \/ /   %C_GREEN%BUILD SYSTEM%C_RESET%
echo %C_GREEN%%C_BOLD%     [ [   ] [\/] [ ] ] ]    ]  _ \[ [ ] ]\  /    %C_PINK%CHAY LA PHAI MUOT%C_RESET%
echo %C_GREEN%%C_BOLD%     [ [___] [  ] [ ]_] ]    ] ]_) ] [_] ]/  \    %C_BLUE%KHONG MUOT THI FIX%C_RESET%
echo %C_YELLOW%%C_BOLD%      \____]_]  [_]____/     ]____/ \___//_/\_\   %C_RED%PRO V2 ULTRA MAX%C_RESET%
echo %C_PINK%%C_BOLD%  ========================================================================%C_RESET%
echo       %BG_PURPLE%  * TOOLKIT PRO SYSTEM BUILDER - PHIEN BAN HOANG GIA LOE LOET *  %C_RESET%
echo.

:: 1. Soi trinh bien dich g++
echo %C_BLUE%[?] Dang soi xem may dai ca co cai g++ xin khong hay toan virus...%C_RESET%
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
    echo %BG_RED% [!] TOANG ROI ONG GIAO OI! %C_RESET% %C_RED%%C_BOLD%Khong tim thay g++ dau ca! Cai MinGW hoac MSYS2 gap di!%C_RESET%
    echo.
    pause
    exit /b 1
)

echo %C_GREEN%%C_BOLD%  [v] Phat hien trinh bien dich uy tin 100%%: %C_YELLOW%!GXX! %C_GREEN%[Chuan com me nau!]%C_RESET%

:: 2. Tao thu muc bin neu chua co
if not exist "bin" (
    echo %C_CYAN%  [*] Chua co thu muc bin? De bo may tao luon cho nong hoi...%C_RESET%
    mkdir "bin"
)

:: 3. Chay lenh bien dich
echo.
echo %C_PINK%%C_BOLD%  [O_o] Dang van 100%% cong luc bien dich toan bo src\*.cpp...%C_RESET%
echo %C_CYAN%  [+] Bom doping cuc nang: %C_YELLOW%-O3 %C_GREEN%-mavx2 %C_BLUE%-mfma %C_PINK%-fopenmp %C_WHITE%[CPU quay tit mu]%C_RESET%
echo %C_BLUE%  [i] Giu chat ghe, dung manh dong keo chay chip nha hang xom...%C_RESET%
echo.

"!GXX!" -std=c++17 -O3 -fopenmp -mavx2 -mfma -Iinclude src\*.cpp -o bin\main.exe -lws2_32 -liphlpapi -lole32 -lwindowscodecs -loleaut32 -luuid -static-libgcc -static-libstdc++ -static -s

if %errorlevel% neq 0 (
    echo.
    echo %BG_RED%  [X] BUMMM! TOANG ROI BU EM OI!  %C_RESET%
    echo %C_RED%%C_BOLD%  Co loi cu phap lanh tanh banh trong code kia kia!%C_RESET%
    echo %C_YELLOW%  Mau mo lai code ma fix di, dung do tai troi mua hay tai may lag! [-_-]%C_RESET%
    echo.
    pause
    exit /b 1
)

if exist "src\apps.txt" copy /y "src\apps.txt" "bin\apps.txt" >nul

echo.
echo %BG_GREEN%  [OK] AO MA CANADA - BUILD THANH CONG RUC RO KHONG MOT VET XUOC  %C_RESET%
echo %C_GREEN%%C_BOLD%  [>>] Hang nong ra lo: %C_YELLOW%bin\main.exe %C_GREEN%[Sieu nhe, sieu muot, bao hanh 100 nam]%C_RESET%
echo.

:: 4. Tùy chọn mở app: Y = mở, Enter = không mở
set "RUN_APP="
echo %C_PINK%%C_BOLD%========================================================================%C_RESET%
set /p "RUN_APP=%C_CYAN%>>> Muon phong xe vao main.exe luon khong bro? [%C_YELLOW%y%C_CYAN% = Mo ngay, %C_WHITE%Enter%C_CYAN% = Thoi]: %C_RESET%"

if /i "!RUN_APP!"=="y" (
    echo.
    echo %C_GREEN%  [^>_^>] Phong xe vao app... Veo veo!%C_RESET%
    start "" "bin\main.exe"
) else (
    echo.
    echo %C_YELLOW%  [-_-] Ok luon, hen gap lai dai ca lan sau nhe! Bye!%C_RESET%
)

echo.
