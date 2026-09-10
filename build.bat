@echo off
setlocal enabledelayedexpansion

:: Khoi tao ma mau ANSI
for /f %%a in ('echo prompt $e ^| cmd') do set "ESC=%%a"
set "C_RESET=%ESC%[0m"
set "C_CYAN=%ESC%[96m"
set "C_GREEN=%ESC%[92m"
set "C_YELLOW=%ESC%[93m"
set "C_PURPLE=%ESC%[95m"
set "C_RED=%ESC%[91m"
set "C_BOLD=%ESC%[1m"

cls
echo.
echo %C_PURPLE%%C_BOLD%  ======================================================%C_RESET%
echo %C_CYAN%%C_BOLD%       ____ __  __ ____       ____   ______  __%C_RESET%
echo %C_CYAN%%C_BOLD%      / ___]  \/  ]  _ \     ] __ ) / _ \ \/ /%C_RESET%
echo %C_GREEN%%C_BOLD%     [ [   ] [\/] [ ] ] ]    ]  _ \[ [ ] ]\  / %C_RESET%
echo %C_GREEN%%C_BOLD%     [ [___] [  ] [ ]_] ]    ] ]_) ] [_] ]/  \ %C_RESET%
echo %C_YELLOW%%C_BOLD%      \____]_]  [_]____/     ]____/ \___//_/\_\%C_RESET%
echo %C_PURPLE%%C_BOLD%  ======================================================%C_RESET%
echo %C_YELLOW%          :: TOOLKIT PRO SYSTEM BUILDER ::%C_RESET%
echo.

:: 1. Kiem tra trinh bien dich g++
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
    echo %C_RED%  [x] Toi roi! Khong tim thay g++ o bat cu dau ca!%C_RESET%
    pause
    exit /b 1
)

echo %C_CYAN%  [v] Phat hien trinh bien dich: %C_BOLD%!GXX!%C_RESET%

:: 2. Tao thu muc bin neu chua co
if not exist "bin" (
    echo %C_YELLOW%  [*] Tao thu muc bin...%C_RESET%
    mkdir "bin"
)

:: 3. Chay lenh bien dich
echo.
echo %C_PURPLE%  [☕] Dang pha ca phe va van cong luc bien dich toan bo src\*.cpp...%C_RESET%
echo %C_CYAN%  [*] Kich hoat sieu toi uu: -O3, AVX2, FMA, OpenMP, WIC Metadata...%C_RESET%
echo.

"!GXX!" -std=c++17 -O3 -fopenmp -mavx2 -mfma -Iinclude src\*.cpp -o bin\main.exe -lws2_32 -liphlpapi -lole32 -lwindowscodecs -loleaut32 -luuid -static-libgcc -static-libstdc++ -static -s

if %errorlevel% neq 0 (
    echo.
    echo %C_RED%%C_BOLD%  [x] BUM! Bien dich that bai roi! Co loi cu phap trong code kia!%C_RESET%
    pause
    exit /b 1
)

if exist "src\apps.txt" copy /y "src\apps.txt" "bin\apps.txt" >nul
echo %C_GREEN%%C_BOLD%  [🚀] XONG PHIM! Build thanh cong ruc ro: bin\main.exe%C_RESET%
echo %C_GREEN%  [✓] He thong da san sang cat canh!%C_RESET%
echo.
pause
