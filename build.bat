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
echo %C_GREEN%%C_BOLD%     [ [   ] [\/] [ ] ] ]    ]  _ \[ [ ] ]\  /    %C_PINK%CHẠY LÀ PHẢI MƯỢT%C_RESET%
echo %C_GREEN%%C_BOLD%     [ [___] [  ] [ ]_] ]    ] ]_) ] [_] ]/  \    %C_BLUE%KHÔNG MƯỢT THÌ FIX%C_RESET%
echo %C_YELLOW%%C_BOLD%      \____]_]  [_]____/     ]____/ \___//_/\_\   %C_RED%PRO V2 ULTRA MAX%C_RESET%
echo %C_PINK%%C_BOLD%  ========================================================================%C_RESET%
echo       %BG_PURPLE%  * TOOLKIT PRO SYSTEM BUILDER - PHIÊN BẢN HOÀNG GIA LÒE LOẸT *  %C_RESET%
echo.

:: 1. Soi trinh bien dich g++
echo %C_BLUE%[?] Đang soi xem máy đại ca có cài g++ xịn không hay toàn tải virus...%C_RESET%
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
    echo %BG_RED% [!] TOANG RỒI ÔNG GIÁO ƠI! %C_RESET% %C_RED%%C_BOLD%Không tìm thấy g++ đâu cả! Cài MinGW hoặc MSYS2 gấp đi!%C_RESET%
    echo.
    pause
    exit /b 1
)

echo %C_GREEN%%C_BOLD%  [v] Phát hiện trình biên dịch uy tín 100%%: %C_YELLOW%!GXX! %C_GREEN%[Chuẩn cơm mẹ nấu]%C_RESET%

:: 2. Tao thu muc bin neu chua co
if not exist "bin" (
    echo %C_CYAN%  [*] Chưa có thư mục bin? Để tao tạo luôn cho nóng hổi...%C_RESET%
    mkdir "bin"
)

:: 3. Chay lenh bien dich
echo.
echo %C_PINK%%C_BOLD%  [O_o] Đang vận 100%% công lực biên dịch toàn bộ src\*.cpp...%C_RESET%
echo %C_CYAN%  [+] Bơm doping cực nặng: %C_YELLOW%-O3 %C_GREEN%-mavx2 %C_BLUE%-mfma %C_PINK%-fopenmp %C_WHITE%[CPU quay tít mù]%C_RESET%
echo %C_BLUE%  [i] Giữ chặt ghế, đừng manh động kẻo cháy chip nhà hàng xóm...%C_RESET%
echo.

"!GXX!" -std=c++17 -O3 -fopenmp -mavx2 -mfma -Iinclude src\*.cpp -o bin\main.exe -lws2_32 -liphlpapi -lole32 -lwindowscodecs -loleaut32 -luuid -static-libgcc -static-libstdc++ -static -s

if %errorlevel% neq 0 (
    echo.
    echo %BG_RED%  [X] BÙMM! TOANG RỒI BU EM ƠI!  %C_RESET%
    echo %C_RED%%C_BOLD%  Có lỗi cú pháp tanh bành té bẹ trong code kìa!%C_RESET%
    echo %C_YELLOW%  Mau mở lại code mà sửa đi, đừng đổ tại trời mưa hay tại máy lag! [-_-]%C_RESET%
    echo.
    pause
    exit /b 1
)

if exist "src\apps.txt" copy /y "src\apps.txt" "bin\apps.txt" >nul

echo.
echo %BG_GREEN%  [OK] ẢO MA CANADA - BUILD THÀNH CÔNG RỰC RỠ KHÔNG MỘT VẾT XƯỚC  %C_RESET%
echo %C_GREEN%%C_BOLD%  [+] Hàng nóng ra lò: %C_YELLOW%bin\main.exe %C_GREEN%[Siêu nhẹ, siêu mượt, bảo hành 100 năm]%C_RESET%
echo.

:: 4. Tùy chọn mở app: Y = mở, Enter = không mở
set "RUN_APP="
echo %C_PINK%%C_BOLD%========================================================================%C_RESET%
set /p "RUN_APP=%C_CYAN%[?] Muốn phóng xe vào main.exe luôn không bro? [%C_YELLOW%y%C_CYAN% = Mở ngay, %C_WHITE%Enter%C_CYAN% = Thôi]: %C_RESET%"

if /i "!RUN_APP!"=="y" (
    echo.
    echo %C_GREEN%  [O_O] Đang phóng xe vào app... Vèo vèo!%C_RESET%
    start "" "bin\main.exe"
) else (
    echo.
    echo %C_YELLOW%  [O_O] Ok luôn, hẹn gặp lại đại ca lần sau nhé! Bye!%C_RESET%
)

echo.
