@echo off
setlocal
chcp 65001 >nul

cd /d "%~dp0"

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

if "%GXX%"=="" (
    echo [!] Khong tim thay g++!
    pause
    exit /b 1
)

:: 2. Tao thu muc bin neu chua ton tai
if not exist "bin" mkdir "bin"

:: 3. Chay lenh bien dich
echo [*] Dang bien dich src\*.cpp...
"%GXX%" -std=c++17 -O2 -Iinclude src\*.cpp -o bin\main.exe -lws2_32 -liphlpapi -static-libgcc -static-libstdc++ -static -s

if %errorlevel% neq 0 (
    echo.
    echo [x] Bien dich that bai!
    pause
    exit /b 1
)

echo [v] Thanh cong: bin\main.exe
echo.

set /p RUN_CHOICE="Chay ung dung ngay? (y/n): "
if /i "%RUN_CHOICE%"=="y" (
    cls
    "bin\main.exe"
)

