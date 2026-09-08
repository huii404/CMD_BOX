// Internet.cpp
#include "../include/Internet.h"
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#include "../include/SystemCore.h"
#include <filesystem>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <thread>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <omp.h>
#include <iomanip>

namespace fs = std::filesystem;
using namespace std;

// Cache IP
static string cachedIP = "";
static time_t lastIPCheck = 0;
static const int IP_CACHE_TTL = 60;

Internet::Internet(SystemCore &s) : sc(s) {
}

Internet::~Internet() {
}

// Lấy IP nội bộ
string Internet::getLocalIP() {
    time_t now = time(nullptr);
    
    if (!cachedIP.empty() && (now - lastIPCheck) < IP_CACHE_TTL) {
        return cachedIP;
    }
    
    // Win32 GetAdaptersInfo (Nhanh, chuẩn, không dùng gethostbyname lỗi thời)
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD dwSize = sizeof(adapterInfo);
    DWORD dwRetVal = GetAdaptersInfo(adapterInfo, &dwSize);
    
    if (dwRetVal == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapter = adapterInfo;
        while (pAdapter) {
            string ip = pAdapter->IpAddressList.IpAddress.String;
            if (ip.find("192.168.") == 0 || ip.find("10.") == 0 || 
                ip.find("172.16.") == 0 || ip.find("172.17.") == 0 ||
                ip.find("172.18.") == 0 || ip.find("172.19.") == 0 ||
                ip.find("172.2") == 0 || ip.find("172.30.") == 0 || ip.find("172.31.") == 0) {
                if (ip != "0.0.0.0" && ip != "127.0.0.1") {
                    cachedIP = ip;
                    lastIPCheck = now;
                    return cachedIP;
                }
            }
            pAdapter = pAdapter->Next;
        }
    }
    
    // Fallback qua PowerShell nếu không tìm thấy qua Win32
    FILE *pipe = _popen("powershell -NoProfile -Command \"(Get-NetIPAddress -AddressFamily IPv4 | Where-Object {$_.IPAddress -like '192.168.*' -or $_.IPAddress -like '10.*' -or $_.IPAddress -like '172.*'} | Select-Object -First 1).IPAddress\"", "r");
    if (pipe) {
        char buf[32] = {0};
        if (fgets(buf, sizeof(buf), pipe)) {
            string ip = sc.trim(string(buf));
            _pclose(pipe);
            if (!ip.empty() && ip != "0.0.0.0" && ip != "127.0.0.1") {
                cachedIP = ip;
                lastIPCheck = now;
                return cachedIP;
            }
        }
        _pclose(pipe);
    }
    
    return "127.0.0.1";
}

string Internet::getField(const string &line) {
    size_t pos = line.find(":");
    if (pos != string::npos && pos + 2 < line.size()) return sc.trim(line.substr(pos + 2));
    return "";
}

void Internet::showNetworkInfo() {
    sc.cls();
    cout << "\n\n";

    cout << " [LAN IP]    : " << getLocalIP() << "\n";
    cout << " [Public IP] : ";
    cout.flush();
    FILE *pipe = _popen("curl -s --max-time 2 https://api.ipify.org", "r");
    if (pipe) {
        char buf[64] = {0};
        if (fgets(buf, sizeof(buf), pipe)) {
            string pubIp = sc.trim(string(buf));
            if (!pubIp.empty()) cout << pubIp << "\n";
            else cout << "(Không có kết nối Internet)\n";
        } else {
            cout << "(Không có kết nối Internet)\n";
        }
        _pclose(pipe);
    } else {
        cout << "(Không thể kết nối)\n";
    }

    cout << "\n--- Danh sách Card mạng đang hoạt động ---\n";
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD dwSize = sizeof(adapterInfo);
    if (GetAdaptersInfo(adapterInfo, &dwSize) == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO p = adapterInfo;
        int count = 0;
        while (p) {
            string ip = p->IpAddressList.IpAddress.String;
            if (ip != "0.0.0.0" && ip != "127.0.0.1") {
                count++;
                cout << " * " << p->Description << "\n";
                cout << "   - IPv4     : " << ip << " (Subnet: " << p->IpAddressList.IpMask.String << ")\n";
                if (strlen(p->GatewayList.IpAddress.String) > 0 && strcmp(p->GatewayList.IpAddress.String, "0.0.0.0") != 0) {
                    cout << "   - Gateway  : " << p->GatewayList.IpAddress.String << "\n";
                }
                if (p->DhcpEnabled && strlen(p->DhcpServer.IpAddress.String) > 0) {
                    cout << "   - DHCP     : " << p->DhcpServer.IpAddress.String << "\n";
                }
            }
            p = p->Next;
        }
        if (count == 0) {
            cout << " (Không có card mạng IPv4 khả dụng)\n";
        }
    } else {
        cout << " (Không thể đọc thông tin card mạng)\n";
    }
    cout << "\n";
    sc.waitEnter();
}

void Internet::repairNetwork() {
    sc.cls();
    cout << "Tiến trình sẽ đặt lại Winsock, TCP/IP, xóa DNS cache và làm mới IP.\n";
    if (!sc.confirm("Xác nhận thực hiện sửa lỗi mạng? (y/n): ")) return;

    std::string batContent = 
        "@echo off\n"
        "chcp 65001 >nul\n"
        "title SUA LOI MANG\n"
        "echo 1/6. Xoa bo dem DNS (Flush DNS)...\n"
        "ipconfig /flushdns >nul 2>&1\n"
        "echo 2/6. Dat lai Winsock Catalog...\n"
        "netsh winsock reset >nul 2>&1\n"
        "echo 3/6. Dat lai ngan xep TCP/IP...\n"
        "netsh int ip reset >nul 2>&1\n"
        "echo 4/6. Xoa bang ARP Cache...\n"
        "netsh interface ip delete arpcache >nul 2>&1\n"
        "echo 5/6. Lam moi dia chi IP...\n"
        "ipconfig /release >nul 2>&1\n"
        "ipconfig /renew >nul 2>&1\n"
        "echo 6/6. Khoi dong lai WinNAT va HNS...\n"
        "net stop winnat >nul 2>&1 & net start winnat >nul 2>&1\n"
        "net stop hns >nul 2>&1 & net start hns >nul 2>&1\n"
        "echo Hoan tat!\n";

    cout << "\nĐang chạy tiến trình sửa lỗi mạng trong cửa sổ quản trị...\n";
    if (SystemCore::runBatchAsAdmin(batContent, "Sửa lỗi mạng")) {
        cout << "[OK] Đã sửa lỗi và khôi phục cài đặt mạng thành công!\n";
    } else {
        cout << "[Lỗi] Thất bại khi thực thi (Cần quyền Administrator).\n";
    }
    sc.waitEnter();
}

void Internet::wifiAudit() {
    sc.cls();

    FILE *pipe = _popen("netsh wlan show profiles", "r");
    if (!pipe) {
        cout << "Không thể truy cập cấu hình Wi-Fi.\n";
        sc.waitEnter();
        return;
    }

    char buf[512];
    vector<string> wifiList;
    while (fgets(buf, sizeof(buf), pipe)) {
        string line = buf;
        if (line.find("All User Profile") != string::npos) {
            string wifiName = getField(line);
            if (!wifiName.empty()) {
                wifiList.push_back(wifiName);
            }
        }
    }
    _pclose(pipe);

    if (wifiList.empty()) {
        cout << "Không tìm thấy cấu hình Wi-Fi nào trên máy.\n";
        sc.waitEnter();
        return;
    }

    for (size_t i = 0; i < wifiList.size(); ++i) {
        cout << " [" << i + 1 << "] " << wifiList[i] << "\n";
    }
    cout << " [0] Quay lại\n\n";

    int choice = sc.readInt(" -> Nhập số thứ tự Wi-Fi muốn xem mật khẩu: ");
    if (choice == 0) return;

    if (choice < 1 || choice > static_cast<int>(wifiList.size())) {
        cout << "Lựa chọn không hợp lệ!\n";
        sc.waitEnter();
        return;
    }

    string selectedWifi = wifiList[choice - 1];
    string cmd = "netsh wlan show profile \"" + selectedWifi + "\" key=clear";
    FILE *p2 = _popen(cmd.c_str(), "r");
    if (!p2) {
        cout << "Thất bại khi truy vấn mật khẩu.\n";
        sc.waitEnter();
        return;
    }

    char b2[512];
    string auth = "", cipher = "", pass = "(Mạng Mở/Open)";
    while (fgets(b2, sizeof(b2), p2)) {
        string inf = b2;
        if (inf.find("Authentication") != string::npos) auth = getField(inf);
        if (inf.find("Cipher") != string::npos) cipher = getField(inf);
        if (inf.find("Key Content") != string::npos) pass = getField(inf);
    }
    _pclose(p2);

    sc.cls();
    cout << " Tên Wi-Fi  : " << selectedWifi << "\n"
         << " Mật khẩu   : " << pass << "\n"
         << " Bảo mật    : " << auth << "\n"
         << " Mã hóa     : " << cipher << "\n\n";
    sc.waitEnter();
}

// Native Win32 Registry & Service Helpers
bool Internet::readRegDword(HKEY hRoot, const std::string &subKey, const std::string &valueName, DWORD &outVal) {
    HKEY hKey;
    if (RegOpenKeyExA(hRoot, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    DWORD type = 0;
    DWORD data = 0;
    DWORD size = sizeof(data);
    LONG res = RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, reinterpret_cast<LPBYTE>(&data), &size);
    RegCloseKey(hKey);
    if (res == ERROR_SUCCESS && (type == REG_DWORD || type == REG_QWORD)) {
        outVal = data;
        return true;
    }
    return false;
}

bool Internet::writeRegDword(HKEY hRoot, const std::string &subKey, const std::string &valueName, DWORD val) {
    HKEY hKey;
    if (RegCreateKeyExA(hRoot, subKey.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }
    LONG res = RegSetValueExA(hKey, valueName.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&val), sizeof(val));
    RegCloseKey(hKey);
    return (res == ERROR_SUCCESS);
}

bool Internet::isServiceRunningNative(const std::string &serviceName) {
    SC_HANDLE hSCM = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCM) return false;
    SC_HANDLE hService = OpenServiceA(hSCM, serviceName.c_str(), SERVICE_QUERY_STATUS);
    if (!hService) {
        CloseServiceHandle(hSCM);
        return false;
    }
    SERVICE_STATUS_PROCESS ssp;
    DWORD bytesNeeded;
    bool isRunning = false;
    if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &bytesNeeded)) {
        isRunning = (ssp.dwCurrentState == SERVICE_RUNNING);
    }
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    return isRunning;
}

bool Internet::isServiceDisabledNative(const std::string &serviceName) {
    SC_HANDLE hSCM = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCM) return false;
    SC_HANDLE hService = OpenServiceA(hSCM, serviceName.c_str(), SERVICE_QUERY_CONFIG);
    if (!hService) {
        CloseServiceHandle(hSCM);
        return false;
    }
    DWORD bytesNeeded = 0;
    QueryServiceConfigA(hService, NULL, 0, &bytesNeeded);
    bool isDisabled = false;
    if (bytesNeeded > 0) {
        std::vector<BYTE> buffer(bytesNeeded);
        LPQUERY_SERVICE_CONFIGA pConfig = (LPQUERY_SERVICE_CONFIGA)buffer.data();
        if (QueryServiceConfigA(hService, pConfig, bytesNeeded, &bytesNeeded)) {
            isDisabled = (pConfig->dwStartType == SERVICE_DISABLED);
        }
    }
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    return isDisabled;
}

void Internet::fullSecurityShield() {
    while (true) {
        sc.cls();
        cout << "=== LÁ CHẮN BẢO MẬT HỆ THỐNG & MẠNG ===\n"
             << " [1] Kích hoạt toàn diện (Defender, Firewall, Chặn Port, DoH)\n"
             << " [2] Khôi phục mặc định (Mở Port, Tắt DoH, Chia sẻ LAN)\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";

        int choice = sc.readInt("");
        if (choice == 0) break;

        if (choice == 1) {
            sc.cls();
            cout << "=== ĐANG KÍCH HOẠT LÁ CHẮN BẢO MẬT ===\n\n"
                 << "Đang tổng hợp quy tắc bảo mật trong cửa sổ quản trị...\n";

            std::string batContent = 
                "@echo off\n"
                "chcp 65001 >nul\n"
                "title KICH HOAT LA CHAN BAO MAT HE THONG & MANG\n"
                "echo 1/8. Bat Defender Realtime va cap nhat Signature...\n"
                "powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $false\" >nul 2>&1\n"
                "\"%ProgramFiles%\\Windows Defender\\MpCmdRun.exe\" -SignatureUpdate >nul 2>&1\n"
                "echo 2/8. Kich hoat Tuong lua Windows...\n"
                "netsh advfirewall set allprofiles state on >nul 2>&1\n"
                "echo 3/8. Bat chong Ransomware (Controlled Folder Access)...\n"
                "powershell -Command \"Set-MpPreference -EnableControlledFolderAccess Enabled\" >nul 2>&1\n"
                "echo 4/8. Vo hieu hoa SMBv1, LLMNR Multicast va NetBIOS...\n"
                "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters\" /v SMB1 /t REG_DWORD /d 0 /f >nul 2>&1\n"
                "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient\" /v EnableMulticast /t REG_DWORD /d 0 /f >nul 2>&1\n"
                "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-NetAdapter -Name $_.Name -NetLuid $_.NetLuid -NetBIOSSetting Disabled }\" >nul 2>&1\n"
                "echo 5/8. Thiet lap chan cong nguy hiem (TCP: 445, 135, 137, 138, 139)...\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_445\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_445_out\" >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_445\" dir=in action=block protocol=TCP localport=445 >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_445_out\" dir=out action=block protocol=TCP localport=445 >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_139\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_139_out\" >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_139\" dir=in action=block protocol=TCP localport=139 >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_139_out\" dir=out action=block protocol=TCP localport=139 >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_135\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_135_out\" >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_135\" dir=in action=block protocol=TCP localport=135 >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_135_out\" dir=out action=block protocol=TCP localport=135 >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_137\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_137_out\" >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_137\" dir=in action=block protocol=TCP localport=137 >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_137_out\" dir=out action=block protocol=TCP localport=137 >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_138\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_138_out\" >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_138\" dir=in action=block protocol=TCP localport=138 >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_138_out\" dir=out action=block protocol=TCP localport=138 >nul 2>&1\n"
                "echo 6/8. Cau hinh DNS over HTTPS (Cloudflare 1.1.1.1)...\n"
                "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ServerAddresses ('1.1.1.1','1.0.0.1') }\" >nul 2>&1\n"
                "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters\" /v EnableAutoDoh /t REG_DWORD /d 2 /f >nul 2>&1\n"
                "echo 7/8. Vo hieu hoa dich vu Remote nguy hiem...\n"
                "sc config RemoteRegistry start= disabled >nul 2>&1\n"
                "sc stop RemoteRegistry >nul 2>&1\n"
                "sc config TermService start= disabled >nul 2>&1\n"
                "sc stop TermService >nul 2>&1\n"
                "echo 8/8. Vo hieu hoa Windows Telemetry...\n"
                "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection\" /v AllowTelemetry /t REG_DWORD /d 0 /f >nul 2>&1\n"
                "echo Hoan tat!\n";

            if (SystemCore::runBatchAsAdmin(batContent, "Kích hoạt lá chắn bảo mật")) {
                cout << "\n[OK] Đã kích hoạt toàn bộ lá chắn bảo mật hệ thống & mạng thành công!\n";
            } else {
                cout << "\n[Lỗi] Thất bại khi thực thi lá chắn bảo mật (Cần quyền Administrator).\n";
            }
            SystemCore::waitEnter();
        } 
        else if (choice == 2) {
            sc.cls();
            cout << "=== ĐANG KHÔI PHỤC THIẾT LẬP MẠNG MẶC ĐỊNH ===\n\n"
                 << "Đang khởi chạy quy trình trong cửa sổ quản trị...\n";

            std::string batContent = 
                "@echo off\n"
                "chcp 65001 >nul\n"
                "title KHOI PHUC CAI DAT MANG MAC DINH\n"
                "echo 1/3. Khoi phuc giao thuc chia se mang LAN (Multicast, NetBIOS)...\n"
                "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient\" /v EnableMulticast /t REG_DWORD /d 1 /f >nul 2>&1\n"
                "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-NetAdapter -Name $_.Name -NetLuid $_.NetLuid -NetBIOSSetting Default }\" >nul 2>&1\n"
                "echo 2/3. Go bo quy tac chan cong mang (Mo lai TCP 445, 135-139)...\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_445\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_445_out\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_139\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_139_out\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_135\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_135_out\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_137\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_137_out\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_138\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_138_out\" >nul 2>&1\n"
                "echo 3/3. Dat lai DNS tu dong tu Router (DHCP) va tat DoH...\n"
                "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ResetServerAddresses }\" >nul 2>&1\n"
                "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters\" /v EnableAutoDoh /t REG_DWORD /d 0 /f >nul 2>&1\n"
                "echo Hoan tat!\n";

            if (SystemCore::runBatchAsAdmin(batContent, "Khôi phục cài đặt mạng")) {
                cout << "\n[OK] Đã hoàn tất khôi phục cài đặt mạng mặc định thành công!\n";
            } else {
                cout << "\n[Lỗi] Thất bại khi khôi phục cài đặt mạng (Cần quyền Administrator).\n";
            }
            SystemCore::waitEnter();
        }
    }
}

void Internet::checkHostsFileSecurity() {
    sc.cls();
    cout << "=== QUÉT & BẢO VỆ TẬP TIN HOSTS ===\n\n";

    std::string hostsPath = "C:\\Windows\\System32\\drivers\\etc\\hosts";
    if (!fs::exists(hostsPath)) {
        cout << "Không tìm thấy tập tin hosts tại: " << hostsPath << "\n";
        SystemCore::waitEnter();
        return;
    }

    std::ifstream hf(hostsPath);
    if (!hf) {
        cout << "Không thể mở đọc tập tin hosts (Cần quyền Administrator).\n";
        SystemCore::waitEnter();
        return;
    }

    std::vector<std::string> activeRules;
    std::vector<std::string> suspiciousRules;
    std::string line;

    std::vector<std::string> sensitiveDomains = {
        "google", "facebook", "youtube", "microsoft", "windowsupdate", 
        "bank", "paypal", "kaspersky", "bitdefender", "virustotal", "avast"
    };

    while (std::getline(hf, line)) {
        std::string trimmed = sc.trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        activeRules.push_back(trimmed);

        std::string lowerLine = trimmed;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
        for (const auto &domain : sensitiveDomains) {
            if (lowerLine.find(domain) != std::string::npos) {
                suspiciousRules.push_back(trimmed);
                break;
            }
        }
    }
    hf.close();

    cout << "Đường dẫn tập tin: " << hostsPath << "\n"
         << "Tổng số quy tắc điều hướng đang kích hoạt: " << activeRules.size() << "\n\n";

    if (!activeRules.empty()) {
        cout << "--- Danh sách các dòng điều hướng trong hosts ---\n";
        for (const auto &r : activeRules) {
            cout << "  -> " << r << "\n";
        }
        cout << "--------------------------------------------------\n\n";
    }

    if (!suspiciousRules.empty()) {
        cout << "CẢNH BÁO: Phát hiện " << suspiciousRules.size() << " quy tắc điều hướng đáng ngờ (chuyển hướng tên miền nhạy cảm):\n";
        for (const auto &sr : suspiciousRules) {
            cout << "  [Nghi vấn] " << sr << "\n";
        }
        cout << "\n";
    } else {
        cout << "Tập tin hosts an toàn, không phát hiện điều hướng độc hại.\n\n";
    }

    cout << " [1] Khôi phục tập tin hosts sạch gốc chuẩn Microsoft\n"
         << " [0] Quay lại\n\n"
         << " [Chọn]: ";

    int choice = sc.readInt("");
    if (choice == 1) {
        std::string defaultHosts = 
            "# Copyright (c) 1993-2009 Microsoft Corp.\n"
            "#\n"
            "# This is a sample HOSTS file used by Microsoft TCP/IP for Windows.\n"
            "#\n"
            "# localhost name resolution is handled within DNS itself.\n"
            "#\t127.0.0.1       localhost\n"
            "#\t::1             localhost\n";

        std::string tempBat = "@echo off\n";
        tempBat += "attrib -r -s -h \"C:\\Windows\\System32\\drivers\\etc\\hosts\" >nul 2>&1\n";
        tempBat += "(echo # Clean Hosts File & echo 127.0.0.1 localhost & echo ::1 localhost) > \"C:\\Windows\\System32\\drivers\\etc\\hosts\"\n";

        if (SystemCore::runBatchAsAdmin(tempBat, "Khôi phục tập tin hosts")) {
            cout << "\nĐã khôi phục tập tin hosts về trạng thái sạch gốc thành công!\n";
        } else {
            cout << "\nThất bại khi ghi đè tập tin hosts (Cần quyền Administrator).\n";
        }
        SystemCore::waitEnter();
    }
}

void Internet::checkSecurityStatus() {
    sc.cls();
    cout << "=== BÁO CÁO TRẠNG THÁI BẢO MẬT HỆ THỐNG ===\n\n";

    // 1. Windows Defender
    cout << " 1. Windows Defender:\n";
    bool defService = isServiceRunningNative("WinDefend");
    DWORD defDisable = 0;
    bool hasDefDisable = readRegDword(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection", "DisableRealtimeMonitoring", defDisable);
    bool rtActive = defService && (!hasDefDisable || defDisable == 0);
    cout << "    - Dịch vụ WinDefend : " << (defService ? "\x1b[32mĐANG CHẠY\x1b[0m" : "\x1b[31mĐÃ DỪNG\x1b[0m") << "\n"
         << "    - Real-Time Protect : " << (rtActive ? "\x1b[32mBẬT (An toàn)\x1b[0m" : "\x1b[31mTẮT\x1b[0m") << "\n";

    // 2. Tường lửa
    cout << "\n 2. Tường lửa Windows (Firewall):\n";
    FILE *pipeFw = _popen("netsh advfirewall show allprofiles state", "r");
    if (pipeFw) {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipeFw)) {
            string l = sc.trim(string(buf));
            if (l.find("Profile") != string::npos || l.find("State") != string::npos || l.find("Trạng thái") != string::npos) {
                cout << "    - " << l << "\n";
            }
        }
        _pclose(pipeFw);
    }

    // 3. Cổng nguy hiểm
    cout << "\n 3. Trạng thái chặn Port nguy hiểm:\n";
    vector<int> ports = {445, 139, 135, 137, 138};
    for (int p : ports) {
        string chk = "netsh advfirewall firewall show rule name=\"Block_Dangerous_Port_" + to_string(p) + "\" >nul 2>&1";
        bool blocked = (system(chk.c_str()) == 0);
        cout << "    - Port " << p << " : " << (blocked ? "\x1b[32mĐÃ CHẶN (An toàn)\x1b[0m" : "\x1b[33mMỞ (Chưa chặn)\x1b[0m") << "\n";
    }

    // 4. Dịch vụ điều khiển từ xa
    cout << "\n 4. Dịch vụ điều khiển từ xa:\n";
    vector<pair<string, string>> svcs = {
        {"RemoteRegistry", "Remote Registry"},
        {"TermService", "Remote Desktop (RDP)"},
        {"RasMan", "Remote Access"}
    };
    for (auto &s : svcs) {
        bool running = isServiceRunningNative(s.first);
        bool disabled = isServiceDisabledNative(s.first);
        cout << "    - " << s.second << ": " 
             << (running ? "\x1b[31mĐANG CHẠY\x1b[0m" : "ĐÃ DỪNG") 
             << " | Khởi động: " << (disabled ? "\x1b[32mDISABLED\x1b[0m" : "\x1b[33mENABLED\x1b[0m") << "\n";
    }

    // 5. Cấu hình DNS & DoH
    cout << "\n 5. Cấu hình DNS & DNS over HTTPS (DoH):\n";
    DWORD dohVal = 0;
    readRegDword(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", "EnableAutoDoh", dohVal);
    cout << "    - Mã hóa DoH: " << (dohVal == 2 ? "\x1b[32mĐÃ BẬT (DoH Active)\x1b[0m" : "\x1b[33mCHƯA BẬT\x1b[0m") << "\n";

    cout << "\n============================================\n";
    SystemCore::waitEnter();
}

// --- TÍNH NĂNG QUÉT THIẾT BỊ ĐANG DÙNG WI-FI / LAN (THEO CHUẨN C# NETWORK SERVICE) ---

static bool isRandomizedPrivateMac(const string& mac) {
    if (mac.length() < 2) return false;
    char secondHex = (char)toupper((unsigned char)mac[1]);
    // Chuẩn IEEE: Bit thứ 2 của byte đầu = 1 (2, 6, A, E) -> Private / Randomized MAC (iOS 14+, Android 10+)
    return (secondHex == '2' || secondHex == '6' || secondHex == 'A' || secondHex == 'E');
}

static pair<string, string> lookupVendorFromMac(const string& mac) {
    if (mac.length() < 6) return {"Thiết bị không định danh", "Thiết bị mạng"};

    string clean = "";
    for (char c : mac) {
        if (c != ':' && c != '-') clean += (char)toupper((unsigned char)c);
    }
    if (clean.length() < 6) return {"Thiết bị không định danh", "Thiết bị mạng"};
    string oui = clean.substr(0, 6);

    // Apple
    if (oui == "A0BD1D" || oui == "F01898" || oui == "ACBC32" || oui == "F8FFC2" ||
        oui == "0017F2" || oui == "3CD0F8" || oui == "D89695" || oui == "406C8F" ||
        oui == "8C8590" || oui == "B8782E" || oui == "38F9D3" || oui == "701124" ||
        oui == "BCD074" || oui == "DCA904" || oui == "F4F15A" || oui == "A85B78" ||
        oui == "60F81D" || oui == "B817C2" || oui == "90DD5D" || oui == "E8802E" ||
        oui == "286ABA" || oui == "3C22FB" || oui == "70ECE4" || oui == "A483E7")
        return {"Apple Device", "Apple iPhone / iPad / Mac"};

    // Samsung
    if (oui == "503275" || oui == "A4C494" || oui == "342387" || oui == "88329B" ||
        oui == "CC07AB" || oui == "E458E7" || oui == "784B87" || oui == "404E36" ||
        oui == "0007AB" || oui == "001247" || oui == "001599" || oui == "001D25" ||
        oui == "0023D7" || oui == "08373D" || oui == "103047" || oui == "286D97" ||
        oui == "40163B" || oui == "508569" || oui == "7840E4" || oui == "94652D" ||
        oui == "BC4486" || oui == "DC7144" || oui == "E47CF9" || oui == "F409D8" ||
        oui == "842519" || oui == "5CF8A1" || oui == "70288B" || oui == "A0821F")
        return {"Samsung Galaxy", "Samsung (Android)"};

    // Xiaomi / Redmi
    if (oui == "54AF97" || oui == "640980" || oui == "502B73" || oui == "7C49EB" ||
        oui == "9C99A0" || oui == "3480B3" || oui == "186590" || oui == "009EC8" ||
        oui == "04CF8C" || oui == "185936" || oui == "286C07" || oui == "50642B" ||
        oui == "742344" || oui == "8CBEBE" || oui == "ACC1EE" || oui == "C40BCB" ||
        oui == "D4970B" || oui == "E446DA" || oui == "F460E2")
        return {"Xiaomi / Redmi", "Xiaomi (Android)"};

    // Oppo / Vivo / Realme
    if (oui == "8090D0" || oui == "E0191D" || oui == "9C7142" || oui == "600CB8" || oui == "C0B5D5")
        return {"Oppo / Vivo / Realme", "Android"};

    // Huawei / Honor
    if (oui == "001E10" || oui == "042528" || oui == "0819A6" || oui == "104780" ||
        oui == "24DF6A" || oui == "4846FB" || oui == "70723C" || oui == "84A8E4" ||
        oui == "AC853D" || oui == "D8490B" || oui == "E0247F" || oui == "40B034")
        return {"Huawei Device", "Điện thoại / Thiết bị Huawei"};

    // TP-Link / Tenda / Totolink
    if (oui == "F81A67" || oui == "3C8CF8" || oui == "74DA88" || oui == "C0C9E3" || oui == "50C7BF" ||
        oui == "000AEB" || oui == "001478" || oui == "001D0F" || oui == "002586" ||
        oui == "14CC20" || oui == "1C3BF3" || oui == "30DE4B" || oui == "6032B1" ||
        oui == "704F57" || oui == "8416F9" || oui == "98DAC4" || oui == "A0F3C1" ||
        oui == "C04A00" || oui == "EC086B" || oui == "F48CEB" || oui == "6CA6B4")
        return {"TP-Link Device", "Thiết bị mạng (Wi-Fi/AP)"};

    if (oui == "00B00C" || oui == "502B73" || oui == "C83A35" || oui == "CC3429" || oui == "E865D4")
        return {"Tenda Device", "Bộ phát Wi-Fi Tenda"};

    // Camera IP (Hikvision / Dahua / Ezviz / Imou)
    if (oui == "38AF29" || oui == "BC1401" || oui == "C42F90" || oui == "4419B6" || oui == "B0F963" || oui == "AC83F3")
        return {"IP Camera An ninh", "Camera IP / Smart Home"};

    // Smart TV
    if (oui == "001FE2" || oui == "AC8B03" || oui == "3C15C2" || oui == "00248D" || oui == "10F96F" || oui == "203D66")
        return {"Smart TV", "Smart TV / TV Box"};

    // Máy tính / Laptop / Card mạng (Intel / Realtek / VMware / VirtualBox)
    if (oui == "000C29" || oui == "005056") return {"VMware Virtual PC", "Máy ảo VMware"};
    if (oui == "080027") return {"VirtualBox Virtual PC", "Máy ảo VirtualBox"};
    if (oui == "001A7D" || oui == "00216A" || oui == "106530" || oui == "54EE75" ||
        oui == "0002B3" || oui == "000347" || oui == "000423" || oui == "000CF1" ||
        oui == "081196" || oui == "3413E8" || oui == "4851B7" || oui == "7CB0C2" ||
        oui == "8086F2" || oui == "AC87A3" || oui == "C85B76" || oui == "E82A44" ||
        oui == "78BE81" || oui == "00E04C" || oui == "52544C")
        return {"Máy tính (PC/Laptop)", "Máy tính (PC/Laptop)"};

    // Espressif (Nhà thông minh IoT)
    if (oui == "18FE34" || oui == "240AC4" || oui == "2462AB" || oui == "2CF432" ||
        oui == "30AEA4" || oui == "84F3EB" || oui == "A4CF12" || oui == "CC50E3" || oui == "DC4F22")
        return {"Espressif IoT", "Thiết bị Smart Home / IoT"};

    // Modem nhà mạng (ZTE / Dasan VNPT / DrayTek)
    if (oui == "001E73" || oui == "200889" || oui == "34E0CF" || oui == "908D78" || oui == "B49842")
        return {"ZTE Modem", "Modem nhà mạng (Viettel/VNPT)"};
    if (oui == "0017C2" || oui == "002534" || oui == "48EE0C")
        return {"Dasan Modem", "Modem mạng VNPT"};

    return {"Thiết bị mạng (OEM)", "Thiết bị kết nối Wi-Fi"};
}

// Truy vấn tên máy Windows qua NetBIOS Name Service (UDP 137, timeout cực nhanh 120ms)
static string queryNetBiosName(const string& ipStr) {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return "";

    DWORD timeout = 120; // 120ms timeout
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(137);
    dest.sin_addr.s_addr = inet_addr(ipStr.c_str());

    static const unsigned char nbtQuery[] = {
        0x80, 0x90, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x20, 0x43, 0x4B, 0x41,
        0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41, 0x41, 0x00, 0x00, 0x21,
        0x00, 0x01
    };

    sendto(s, (const char*)nbtQuery, sizeof(nbtQuery), 0, (sockaddr*)&dest, sizeof(dest));

    char buf[1024];
    int recvLen = recv(s, buf, sizeof(buf), 0);
    closesocket(s);

    if (recvLen > 56) {
        int numNames = (unsigned char)buf[56];
        int offset = 57;
        for (int i = 0; i < numNames && offset + 18 <= recvLen; i++) {
            unsigned char nameType = (unsigned char)buf[offset + 15];
            unsigned short flags = *(unsigned short*)(buf + offset + 16);
            bool isGroup = (flags & 0x8000) != 0;
            if (!isGroup && nameType == 0x00) {
                char name[16] = {0};
                memcpy(name, buf + offset, 15);
                string res = name;
                while (!res.empty() && (res.back() == ' ' || res.back() == '\0')) res.pop_back();
                if (!res.empty() && res.find("__MSBROWSE__") == string::npos) {
                    return res;
                }
            }
            offset += 18;
        }
    }
    return "";
}

// Phân loại thiết bị hoàn toàn tương đương ClassifyDevice trong C# NetworkService
static pair<string, string> classifyDevice(const string& ip, const string& mac, bool isLocal, bool isGw, const string& localHost, const string& netBios, const string& dnsHost) {
    if (isLocal) {
        return {localHost, "Máy tính này (This PC)"};
    }

    if (isGw) {
        string gwName = !dnsHost.empty() ? dnsHost : "Router / Modem Wi-Fi";
        return {gwName, "Router / Modem Wi-Fi"};
    }

    if (!netBios.empty()) {
        return {netBios, "Máy tính Windows (LAN)"};
    }

    if (!dnsHost.empty()) {
        return {dnsHost, "Thiết bị mạng (Đã định danh)"};
    }

    // Kiểm tra MAC ngẫu nhiên (Private / Randomized MAC trên iPhone iOS 14+ và Android 10+)
    if (isRandomizedPrivateMac(mac)) {
        return {"Ẩn danh (Bảo mật MAC riêng tư)", "Điện thoại (iOS / Android)"};
    }

    // Tra cứu hãng sản xuất từ MAC (OUI)
    return lookupVendorFromMac(mac);
}

static int utf8_display_len(const string& str) {
    int len = 0;
    for (size_t i = 0; i < str.length(); ++i) {
        unsigned char c = (unsigned char)str[i];
        if ((c & 0xC0) != 0x80) {
            len++;
        }
    }
    return len;
}

static string utf8_pad_right(const string& str, int targetWidth) {
    int curWidth = utf8_display_len(str);
    if (curWidth > targetWidth) {
        int needed = targetWidth - 3;
        if (needed < 1) needed = 1;
        string res = "";
        int w = 0;
        for (size_t i = 0; i < str.length(); ++i) {
            unsigned char c = (unsigned char)str[i];
            if ((c & 0xC0) != 0x80) {
                if (w >= needed) break;
                w++;
            }
            res += str[i];
        }
        res += "...";
        int pad = targetWidth - utf8_display_len(res);
        if (pad > 0) res.append(pad, ' ');
        return res;
    }
    if (curWidth < targetWidth) {
        return str + string(targetWidth - curWidth, ' ');
    }
    return str;
}

struct DiscoveredDevice {
    uint32_t ipInt;
    string ip;
    string mac;
    string hostname;
    string deviceType;
    bool isSelf;
    bool isGateway;
};

void Internet::scanConnectedDevices() {
    while (true) {
        sc.cls();
        cout << "\n [*] Vui lòng đợi trong giây lát!\n";
        cout.flush();

        // 1. Lấy thông tin Card mạng chính & dải Subnet
        string myIP = "";
        string myMask = "255.255.255.0";
        string gatewayIP = "";
        string myMAC = "";

        ULONG outBufLen = sizeof(IP_ADAPTER_INFO);
        PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(outBufLen);
        if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
            free(pAdapterInfo);
            pAdapterInfo = (IP_ADAPTER_INFO*)malloc(outBufLen);
        }

        if (pAdapterInfo && GetAdaptersInfo(pAdapterInfo, &outBufLen) == NO_ERROR) {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            while (pAdapter) {
                string ip = pAdapter->IpAddressList.IpAddress.String;
                string gw = pAdapter->GatewayList.IpAddress.String;
                if (!ip.empty() && ip != "0.0.0.0" && ip != "127.0.0.1" && !gw.empty() && gw != "0.0.0.0") {
                    myIP = ip;
                    myMask = pAdapter->IpAddressList.IpMask.String;
                    gatewayIP = gw;
                    char macBuf[32];
                    sprintf(macBuf, "%02X:%02X:%02X:%02X:%02X:%02X",
                            pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2],
                            pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);
                    myMAC = macBuf;
                    break;
                }
                pAdapter = pAdapter->Next;
            }
        }
        if (pAdapterInfo) free(pAdapterInfo);

        if (myIP.empty()) {
            myIP = getLocalIP();
        }

        if (myIP == "127.0.0.1" || myIP.empty()) {
            sc.cls();
            cout << " [!] Không phát hiện kết nối mạng cục bộ nào đang hoạt động!\n";
            sc.waitEnter();
            return;
        }

        size_t lastDot = myIP.rfind('.');
        if (lastDot == string::npos) {
            sc.cls();
            cout << " [!] Địa chỉ IP không hợp lệ (" << myIP << ")!\n";
            sc.waitEnter();
            return;
        }

        string baseSubnet = myIP.substr(0, lastDot + 1); // ví dụ "192.168.10."
        if (gatewayIP.empty()) {
            gatewayIP = baseSubnet + "1";
        }

        // Lấy tên máy Windows chính xác
        char localHostName[256] = {0};
        DWORD hostSz = sizeof(localHostName);
        if (!GetComputerNameA(localHostName, &hostSz) || hostSz == 0) {
            char* envComp = getenv("COMPUTERNAME");
            if (envComp && strlen(envComp) > 0) {
                strncpy(localHostName, envComp, sizeof(localHostName) - 1);
            } else {
                gethostname(localHostName, sizeof(localHostName));
            }
        }
        if (strlen(localHostName) == 0) {
            strcpy(localHostName, "This-PC");
        }

        // 2. Quét nhanh cực hạn 254 IP bằng SendARP (OpenMP 40 luồng song song, KHÔNG chặn DNS trong vòng lặp)
        map<string, string> activeMap;
        mutex mapMutex;

        // Luôn ghi nhận máy hiện tại
        activeMap[myIP] = (!myMAC.empty() ? myMAC : "00:00:00:00:00:00");

        #pragma omp parallel for schedule(dynamic, 4) num_threads(40)
        for (int i = 1; i <= 254; ++i) {
            string curIP = baseSubnet + to_string(i);
            if (curIP == myIP) continue;

            IPAddr destIp = inet_addr(curIP.c_str());
            ULONG macAddr[2] = {0};
            ULONG physAddrLen = 6;

            DWORD ret = SendARP(destIp, 0, macAddr, &physAddrLen);
            if (ret == NO_ERROR && physAddrLen == 6) {
                BYTE* b = (BYTE*)macAddr;
                char macBuf[24];
                sprintf(macBuf, "%02X:%02X:%02X:%02X:%02X:%02X",
                        b[0], b[1], b[2], b[3], b[4], b[5]);

                lock_guard<mutex> lock(mapMutex);
                activeMap[curIP] = macBuf;
            }
        }

        // Đọc thêm bảng ARP cache hệ thống để đảm bảo không sót thiết bị
        ULONG tableSize = 0;
        GetIpNetTable(NULL, &tableSize, FALSE);
        if (tableSize > 0) {
            PMIB_IPNETTABLE pIpNetTable = (PMIB_IPNETTABLE)malloc(tableSize);
            if (pIpNetTable && GetIpNetTable(pIpNetTable, &tableSize, FALSE) == NO_ERROR) {
                for (DWORD i = 0; i < pIpNetTable->dwNumEntries; i++) {
                    MIB_IPNETROW row = pIpNetTable->table[i];
                    if (row.dwType != 2) { // 2 = MIB_IPNET_TYPE_INVALID
                        in_addr inAddr;
                        inAddr.s_addr = row.dwAddr;
                        string ip = inet_ntoa(inAddr);
                        if (ip.rfind(baseSubnet, 0) == 0 && !ip.empty() && 
                            (ip.length() < 4 || ip.rfind(".255") != ip.length() - 4) && row.dwPhysAddrLen == 6) {
                            char macBuf[24];
                            sprintf(macBuf, "%02X:%02X:%02X:%02X:%02X:%02X",
                                    row.bPhysAddr[0], row.bPhysAddr[1], row.bPhysAddr[2],
                                    row.bPhysAddr[3], row.bPhysAddr[4], row.bPhysAddr[5]);
                            if (activeMap.find(ip) == activeMap.end()) {
                                activeMap[ip] = macBuf;
                            }
                        }
                    }
                }
            }
            if (pIpNetTable) free(pIpNetTable);
        }

        // 3. Phân loại & nhận diện chi tiết các thiết bị online (Xử lý song song chỉ các thiết bị tìm được)
        vector<DiscoveredDevice> deviceList;
        vector<pair<string, string>> activeEntries(activeMap.begin(), activeMap.end());

        vector<DiscoveredDevice> tempDevices(activeEntries.size());

        #pragma omp parallel for schedule(dynamic, 1) num_threads(8)
        for (int i = 0; i < (int)activeEntries.size(); ++i) {
            string ip = activeEntries[i].first;
            string mac = activeEntries[i].second;
            bool isLocal = (ip == myIP);
            bool isGw = (ip == gatewayIP);

            string netBios = "";
            string dnsHost = "";

            if (!isLocal && !isGw) {
                // Thử lấy tên NetBIOS trước (nếu là máy tính Windows)
                netBios = queryNetBiosName(ip);
            }

            auto res = classifyDevice(ip, mac, isLocal, isGw, localHostName, netBios, dnsHost);

            DiscoveredDevice dev;
            dev.ip = ip;
            dev.ipInt = ntohl(inet_addr(ip.c_str()));
            dev.mac = mac;
            dev.hostname = res.first;
            dev.deviceType = res.second;
            dev.isSelf = isLocal;
            dev.isGateway = isGw;

            tempDevices[i] = dev;
        }

        deviceList = tempDevices;

        // 4. Sắp xếp chuẩn C#: Router đầu tiên, Máy tính này thứ hai, các thiết bị còn lại theo thứ tự IP
        sort(deviceList.begin(), deviceList.end(), [](const DiscoveredDevice& a, const DiscoveredDevice& b) {
            if (a.isGateway != b.isGateway) return a.isGateway > b.isGateway;
            if (a.isSelf != b.isSelf) return a.isSelf > b.isSelf;
            return a.ipInt < b.ipInt;
        });

        // 5. Hiển thị bảng danh sách thiết bị căn chỉnh hoàn hảo từng cột
        sc.cls();
        cout<<"\n\n";
        cout << "+-----+-----------------+-------------------+-------------------------------+-----------------------------------+\n"
             << "| STT |   Địa chỉ IP    |    Địa chỉ MAC    |   Loại thiết bị / Phân loại   |      Tên thiết bị (Hostname)      |\n"
             << "+-----+-----------------+-------------------+-------------------------------+-----------------------------------+\n";

        for (size_t i = 0; i < deviceList.size(); ++i) {
            const auto& d = deviceList[i];
            string typeDisplay = utf8_pad_right(d.deviceType, 29);
            string hostDisplay = utf8_pad_right(d.hostname, 33);
            string ipDisplay = utf8_pad_right(d.ip, 15);
            string macDisplay = utf8_pad_right(d.mac, 17);

            // Màu sắc làm nổi bật: Router (Xanh lá), Máy tính này (Cyan)
            string colorCode = "";
            string resetCode = "\x1b[0m";
            if (d.isGateway) colorCode = "\x1b[32m";
            else if (d.isSelf) colorCode = "\x1b[36m";

            cout << "| " << setw(3) << left << (i + 1) << " | "
                 << colorCode << ipDisplay << resetCode << " | "
                 << macDisplay << " | "
                 << typeDisplay << " | "
                 << hostDisplay << " |\n";
        }
        cout << "+-----+-----------------+-------------------+-------------------------------+-----------------------------------+\n";
        cout << " [*] Tìm thấy \x1b[33m" << deviceList.size() << "\x1b[0m thiết bị đang kết nối mạng.\n\n";

        cout << " [1] Quét lại\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";

        int choice = sc.readInt("");
        if (choice == 0) return;
        if (choice == 1) continue;
    }
}