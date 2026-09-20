@echo off
setlocal EnableExtensions
chcp 65001 >nul
set "failed=0"

echo [1/6] Xoa bo dem DNS...
ipconfig /flushdns >nul 2>&1
if errorlevel 1 set "failed=1"

echo [2/6] Dat lai Winsock Catalog...
netsh winsock reset >nul 2>&1
if errorlevel 1 set "failed=1"

echo [3/6] Dat lai ngan xep TCP/IP...
netsh int ip reset >nul 2>&1
if errorlevel 1 set "failed=1"

echo [4/6] Xoa bang ARP Cache...
netsh interface ip delete arpcache >nul 2>&1
if errorlevel 1 set "failed=1"

echo [5/6] Lam moi dia chi IP (DHCP)...
ipconfig /release >nul 2>&1
ipconfig /renew >nul 2>&1

echo [6/6] Khoi dong lai WinNAT va HNS...
net stop winnat >nul 2>&1
net start winnat >nul 2>&1
net stop hns >nul 2>&1
net start hns >nul 2>&1

exit /b %failed%
