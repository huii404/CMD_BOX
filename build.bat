@echo off
setlocal
chcp 65001 >nul

:: Chuyen den thu muc goc chua file bat
cd /d "%~dp0"

echo ======================================================
echo           CMD BOX - HE THONG BIEN DICH C++
echo ======================================================
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

if "%GXX%"=="" (
    echo [LOI] Khong tim thay trinh bien dich g++.exe!
    echo Vui long kiem tra duong dan MSYS2 / MinGW-w64 hoac them g++ vao bien moi truong PATH.
    echo.
    pause
    exit /b 1
)

:: 2. Tao thu muc bin neu chua ton tai
if not exist "bin" mkdir "bin"

echo [*] Trinh bien dich : %GXX%
echo [*] Dang bien dich cac tep trong src\*.cpp...
echo [*] Che do         : Release (Toi uu -O2, rut gon -s)
echo.

:: 3. Chay lenh bien dich
"%GXX%" -std=c++17 -O2 -Iinclude src\*.cpp -o bin\main.exe -lws2_32 -liphlpapi -static-libgcc -static-libstdc++ -static -s

if %errorlevel% neq 0 goto :BUILD_FAIL

echo.
echo ======================================================
echo [THANH CONG] Da bien dich xong: bin\main.exe
echo ======================================================
echo.

set /p RUN_CHOICE="Ban co muon chay ung dung ngay khong? (y/n): "
if /i "%RUN_CHOICE%"=="y" goto :RUN_APP
goto :DONE

:RUN_APP
echo [*] Dang khoi chay chuong trinh...
echo ------------------------------------------------------
"bin\main.exe"
goto :END

:BUILD_FAIL
echo.
echo ======================================================
echo [THAT BAI] Qua trinh bien dich bi loi!
echo ======================================================
echo.

:DONE
pause

:END
