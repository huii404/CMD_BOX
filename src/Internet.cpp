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
    
    // Thử qua gethostbyname
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent* host = gethostbyname(hostname);
        if (host && host->h_addr_list && host->h_addr_list[0]) {
            struct in_addr addr;
            addr.s_addr = *(u_long*)host->h_addr_list[0];
            char* ip = inet_ntoa(addr);
            if (ip && strcmp(ip, "127.0.0.1") != 0) {
                cachedIP = ip;
                lastIPCheck = now;
                return cachedIP;
            }
        }
    }
    
    // Thử qua GetAdaptersInfo
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD dwSize = sizeof(adapterInfo);
    DWORD dwRetVal = GetAdaptersInfo(adapterInfo, &dwSize);
    
    if (dwRetVal == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapter = adapterInfo;
        while (pAdapter) {
            string ip = pAdapter->IpAddressList.IpAddress.String;
            if (ip.find("192.168.") == 0 || ip.find("10.") == 0 || 
                ip.find("172.16.") == 0 || ip.find("172.17.") == 0 ||
                ip.find("172.18.") == 0 || ip.find("172.19.") == 0) {
                if (ip != "0.0.0.0" && ip != "127.0.0.1") {
                    cachedIP = ip;
                    lastIPCheck = now;
                    return cachedIP;
                }
            }
            pAdapter = pAdapter->Next;
        }
    }
    
    // Fallback qua PowerShell
    FILE *pipe = _popen("powershell -NoProfile -Command \"(Get-NetIPAddress -AddressFamily IPv4 | Where-Object {$_.IPAddress -like '192.168.*' -or $_.IPAddress -like '10.*'} | Select-Object -First 1).IPAddress\"","r");
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

    cout << "Địa chỉ IP nội bộ (LAN): " << getLocalIP() << "\n"
         << "Đang truy vấn Public IP (Internet)... ";
    FILE *pipe = _popen("curl -s --max-time 3 https://api.ipify.org", "r");
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

    cout << "\nCấu hình chi tiết các Card mạng (Network Adapters):\n";
    sc.runCMD("chcp 437 >nul & ipconfig /all | findstr /i \"Description IPv4 Subnet Default DNS Lease DHCP\" & chcp 65001 >nul");
    cout << "\n";
}

void Internet::repairNetwork() {
    sc.cls();
    std::string batContent = 
        "@echo off\n"
        "chcp 65001 >nul\n"
        "title SUA LOI & KHOI PHUC CAI DAT MANG TOAN DIEN\n"
        "color 0B\n"
        "echo ============================================================\n"
        "echo       QUY TRINH SUA LOI & KHOI PHUC CAI DAT MANG TOAN DIEN\n"
        "echo ============================================================\n"
        "echo.\n"
        "echo 1/8. Xoa bo dem DNS (Flush DNS)...\n"
        "ipconfig /flushdns >nul 2>&1\n"
        "echo       -> OK\n"
        "echo 2/8. Dat lai Winsock Catalog...\n"
        "netsh winsock reset >nul 2>&1\n"
        "echo       -> OK\n"
        "echo 3/8. Dat lai ngan xep giao thuc TCP/IP...\n"
        "netsh int ip reset >nul 2>&1\n"
        "echo       -> OK\n"
        "echo 4/8. Xoa bang ARP Cache...\n"
        "netsh interface ip delete arpcache >nul 2>&1\n"
        "echo       -> OK\n"
        "echo 5/8. Lam moi dia chi IP (Release & Renew)...\n"
        "ipconfig /release >nul 2>&1 & ipconfig /renew >nul 2>&1\n"
        "echo       -> OK\n"
        "echo 6/8. Khoi dong lai WinNAT & HNS (Giai phong port bi chiem/loi Socket 10013)...\n"
        "taskkill /f /im localsend.exe >nul 2>&1\n"
        "taskkill /f /im localsend_app.exe >nul 2>&1\n"
        "net stop winnat >nul 2>&1 & net start winnat >nul 2>&1\n"
        "net stop hns >nul 2>&1 & net start hns >nul 2>&1\n"
        "netsh int ipv4 set dynamicport tcp start=49152 num=16384 >nul 2>&1\n"
        "netsh int ipv4 set dynamicport udp start=49152 num=16384 >nul 2>&1\n"
        "echo       -> OK\n"
        "echo 7/8. Cau hinh Tuong lua cho phep ket noi HTTP LAN & LocalSend (TCP/UDP 53317)...\n"
        "netsh advfirewall firewall delete rule name=\"LocalSend_HTTP_TCP_53317\" >nul 2>&1\n"
        "netsh advfirewall firewall delete rule name=\"LocalSend_Discovery_UDP_53317\" >nul 2>&1\n"
        "netsh advfirewall firewall delete rule name=\"LocalSend_App_Allow\" >nul 2>&1\n"
        "netsh advfirewall firewall add rule name=\"LocalSend_HTTP_TCP_53317\" dir=in action=allow protocol=TCP localport=53317 >nul 2>&1\n"
        "netsh advfirewall firewall add rule name=\"LocalSend_HTTP_TCP_53317\" dir=out action=allow protocol=TCP localport=53317 >nul 2>&1\n"
        "netsh advfirewall firewall add rule name=\"LocalSend_Discovery_UDP_53317\" dir=in action=allow protocol=UDP localport=53317 >nul 2>&1\n"
        "netsh advfirewall firewall add rule name=\"LocalSend_Discovery_UDP_53317\" dir=out action=allow protocol=UDP localport=53317 >nul 2>&1\n"
        "powershell -Command \"Get-ChildItem -Path @($env:LOCALAPPDATA, $env:ProgramFiles, ${env:ProgramFiles(x86)}) -Filter 'localsend*.exe' -Recurse -ErrorAction SilentlyContinue | ForEach-Object { New-NetFirewallRule -DisplayName 'LocalSend_App_Allow' -Direction Inbound -Program $_.FullName -Action Allow -Profile Any -ErrorAction SilentlyContinue; New-NetFirewallRule -DisplayName 'LocalSend_App_Allow' -Direction Outbound -Program $_.FullName -Action Allow -Profile Any -ErrorAction SilentlyContinue }\" >nul 2>&1\n"
        "echo       -> OK\n"
        "echo 8/8. Dat mang sang che do Private Network (Cho phep truyen file noi bo)...\n"
        "powershell -Command \"Get-NetConnectionProfile | Set-NetConnectionProfile -NetworkCategory Private\" >nul 2>&1\n"
        "echo       -> OK\n"
        "echo.\n"
        "echo ============================================================\n"
        "echo DA HOAN TAT SUA LOI VA KHOI PHUC CAI DAT MANG TOAN DIEN!\n"
        "echo Cua so se tu dong dong sau 5 giay...\n"
        "timeout /t 5 >nul\n";

    cout << "Đang thực thi quy trình 8 bước sửa lỗi & khôi phục toàn diện mạng trong cửa sổ quản trị...\n";
    if (SystemCore::runBatchAsAdmin(batContent, "Sửa lỗi mạng toàn diện")) {
        cout << "\nĐã hoàn tất sửa lỗi và khôi phục toàn bộ cài đặt mạng, dịch vụ chia sẻ LAN & LocalSend thành công!\n";
    } else {
        cout << "\nThất bại khi thực thi sửa lỗi mạng (Cần cấp quyền Administrator).\n";
    }
}

void Internet::wifiAudit() {
    sc.cls();

    FILE *pipe = _popen("netsh wlan show profiles", "r");
    if (!pipe) {
        cout << "Không thể truy cập cấu hình Wi-Fi.\n";
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
        return;
    }

    string selectedWifi = wifiList[choice - 1];
    string cmd = "netsh wlan show profile \"" + selectedWifi + "\" key=clear";
    FILE *p2 = _popen(cmd.c_str(), "r");
    if (!p2) {
        cout << "Thất bại khi truy vấn mật khẩu.\n";
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

void Internet::toggleDefender(bool enable, int &alreadyCount, int &newlyCount, int &failedCount) {
    cout << "  Đang kiểm tra Windows Defender Realtime Protection...\n";
    DWORD val = 0;
    bool hasVal = readRegDword(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection", "DisableRealtimeMonitoring", val);
    
    bool isAlready = false;
    if (enable) {
        if (hasVal && val == 0) isAlready = true;
    } else {
        if (hasVal && val == 1) isAlready = true;
    }

    if (isAlready) {
        cout << "    Defender đã ở trạng thái mong muốn từ trước.\n";
        alreadyCount++;
    } else {
        string cmd = enable 
            ? "powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $false\" & \"%ProgramFiles%\\Windows Defender\\MpCmdRun.exe\" -SignatureUpdate"
            : "powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $true\"";
        if (sc.runAdmin(cmd, true)) {
            cout << "    Đã cập nhật trạng thái.\n";
            newlyCount++;
        } else {
            cout << "    Thất bại khi thay đổi trạng thái.\n";
            failedCount++;
        }
    }
}

void Internet::toggleFirewall(bool enable, int &alreadyCount, int &newlyCount, int &failedCount) {
    cout << "  Đang kiểm tra Tường lửa Windows (Firewall)...\n";
    FILE *pipe = _popen("netsh advfirewall show allprofiles state", "r");
    bool allOn = true;
    bool allOff = true;
    if (pipe) {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe)) {
            string line = buf;
            if (line.find("State") != string::npos || line.find("Trạng thái") != string::npos) {
                if (line.find("ON") == string::npos && line.find("BẬT") == string::npos) allOn = false;
                if (line.find("OFF") == string::npos && line.find("TẮT") == string::npos) allOff = false;
            }
        }
        _pclose(pipe);
    }

    if (enable && allOn) {
        cout << "    Tường lửa đã bật sẵn trên toàn bộ profile.\n";
        alreadyCount++;
    } else if (!enable && allOff) {
        cout << "    Tường lửa đã tắt sẵn từ trước.\n";
        alreadyCount++;
    } else {
        string cmd = enable ? "netsh advfirewall set allprofiles state on" : "netsh advfirewall set allprofiles state off";
        if (sc.runAdmin(cmd, true)) {
            cout << (enable ? "    Đã kích hoạt Tường lửa cho toàn bộ profile.\n" : "    Đã tắt Tường lửa.\n");
            newlyCount++;
        } else {
            cout << "    Thất bại khi thay đổi trạng thái Tường lửa.\n";
            failedCount++;
        }
    }
}

void Internet::toggleControlledFolderAccess(bool enable, int &alreadyCount, int &newlyCount, int &failedCount) {
    cout << "  Đang kiểm tra Bảo vệ chống Ransomware (Controlled Folder Access)...\n";
    DWORD val = 0;
    bool hasVal = readRegDword(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows Defender\\Windows Defender Exploit Guard\\Controlled Folder Access", "EnableControlledFolderAccess", val);
    
    if (enable && hasVal && val == 1) {
        cout << "    Chống Ransomware đã kích hoạt sẵn từ trước.\n";
        alreadyCount++;
    } else if (!enable && hasVal && val == 0) {
        cout << "    Chống Ransomware đã tắt sẵn từ trước.\n";
        alreadyCount++;
    } else {
        string cmd = enable 
            ? "powershell -Command \"Set-MpPreference -EnableControlledFolderAccess Enabled\""
            : "powershell -Command \"Set-MpPreference -EnableControlledFolderAccess Disabled\"";
        if (sc.runAdmin(cmd, true)) {
            cout << (enable ? "    Đã bật bảo vệ thư mục chống Ransomware.\n" : "    Đã tắt bảo vệ Controlled Folder Access.\n");
            newlyCount++;
        } else {
            cout << "    Không thể thay đổi thiết lập Controlled Folder Access.\n";
            failedCount++;
        }
    }
}

void Internet::toggleInsecureProtocols(bool block, int &alreadyCount, int &newlyCount, int &failedCount) {
    cout << "  Đang kiểm tra các giao thức mạng không an toàn (SMBv1, LLMNR, NetBIOS)...\n";
    DWORD smbVal = 1;
    DWORD llmnrVal = 1;
    readRegDword(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters", "SMB1", smbVal);
    readRegDword(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient", "EnableMulticast", llmnrVal);

    bool smbOk = block ? (smbVal == 0) : (smbVal == 1);
    bool llmnrOk = block ? (llmnrVal == 0) : (llmnrVal == 1);

    if (smbOk && llmnrOk) {
        cout << "    Các giao thức mạng đã ở trạng thái an toàn từ trước.\n";
        alreadyCount++;
    } else {
        bool ok1 = writeRegDword(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters", "SMB1", block ? 0 : 1);
        bool ok2 = writeRegDword(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient", "EnableMulticast", block ? 0 : 1);
        
        string psNetBios = block
            ? "Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-NetAdapter -Name $_.Name -NetLuid $_.NetLuid -NetBIOSSetting Disabled }"
            : "Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-NetAdapter -Name $_.Name -NetLuid $_.NetLuid -NetBIOSSetting Default }";
        sc.runAdmin("powershell -Command \"" + psNetBios + "\"", true);

        if (ok1 && ok2) {
            cout << (block ? "    Đã vô hiệu hóa SMBv1, LLMNR Multicast và NetBIOS.\n" : "    Đã khôi phục cài đặt mặc định cho các giao thức mạng.\n");
            newlyCount++;
        } else {
            cout << "    Thất bại khi ghi Registry cho giao thức mạng.\n";
            failedCount++;
        }
    }
}

void Internet::toggleDangerousPorts(bool block, int &alreadyCount, int &newlyCount, int &failedCount) {
    cout << "  Đang kiểm tra bộ quy tắc tường lửa chặn cổng (Ports 445, 135-139)...\n";
    vector<int> ports = {445, 139, 135, 137, 138};
    bool allExist = true;
    for (int p : ports) {
        string chk = "netsh advfirewall firewall show rule name=\"Block_Dangerous_Port_" + to_string(p) + "\" >nul 2>&1";
        if (system(chk.c_str()) != 0) {
            allExist = false;
            break;
        }
    }

    if (block && allExist) {
        cout << "    Các cổng nguy hiểm đã được chặn từ trước trong Firewall.\n";
        alreadyCount++;
    } else if (!block && !allExist) {
        cout << "    Các cổng mạng đang ở trạng thái mở thông thường.\n";
        alreadyCount++;
    } else {
        string batContent = "";
        for (int p : ports) {
            string ruleName = "Block_Dangerous_Port_" + to_string(p);
            batContent += "netsh advfirewall firewall delete rule name=\"" + ruleName + "\"\n";
            batContent += "netsh advfirewall firewall delete rule name=\"" + ruleName + "_out\"\n";
            if (block) {
                batContent += "netsh advfirewall firewall add rule name=\"" + ruleName + "\" dir=in action=block protocol=TCP localport=" + to_string(p) + "\n";
                batContent += "netsh advfirewall firewall add rule name=\"" + ruleName + "_out\" dir=out action=block protocol=TCP localport=" + to_string(p) + "\n";
            }
        }
        if (SystemCore::runBatchAsAdmin(batContent, block ? "Chặn cổng nguy hiểm" : "Mở lại cổng mạng")) {
            cout << (block ? "    Đã tạo quy tắc chặn 2 chiều các cổng TCP (445, 135-139).\n" : "    Đã gỡ bỏ quy tắc chặn cổng mạng.\n");
            newlyCount++;
        } else {
            cout << "    Thất bại khi cấu hình quy tắc chặn cổng.\n";
            failedCount++;
        }
    }
}

void Internet::toggleDNSoverHTTPS(bool enable, int &alreadyCount, int &newlyCount, int &failedCount) {
    cout << "  Đang kiểm tra cấu hình DNS over HTTPS (DoH Cloudflare 1.1.1.1)...\n";
    DWORD dohVal = 0;
    readRegDword(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", "EnableAutoDoh", dohVal);

    if (enable && dohVal == 2) {
        cout << "    DNS over HTTPS đã được kích hoạt từ trước.\n";
        alreadyCount++;
    } else if (!enable && dohVal == 0) {
        cout << "    DNS over HTTPS đang ở trạng thái mặc định.\n";
        alreadyCount++;
    } else {
        string psDns = enable 
            ? "Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ServerAddresses ('1.1.1.1','1.0.0.1') }"
            : "Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ResetServerAddresses }";
        
        bool okDns = sc.runAdmin("powershell -Command \"" + psDns + "\"", true);
        bool okDoh = writeRegDword(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", "EnableAutoDoh", enable ? 2 : 0);

        if (okDns && okDoh) {
            cout << (enable ? "    Đã kích hoạt DNS over HTTPS (Cloudflare 1.1.1.1 & 1.0.0.1).\n" : "    Đã khôi phục DNS tự động từ DHCP.\n");
            newlyCount++;
        } else {
            cout << "    Thất bại khi cấu hình DNS over HTTPS.\n";
            failedCount++;
        }
    }
}

void Internet::fullSecurityShield() {
    while (true) {
        sc.cls();
        cout << "============\n"
             << "         LÁ CHẮN BẢO MẬT HỆ THỐNG & MẠNG TOÀN DIỆN\n"
             << "============\n"
             << " [1] Kích hoạt toàn diện (Defender, Firewall, Chặn Port, DoH, Chống Ransomware)\n"
             << " [2] Khôi phục mặc định (Mở lại Port, Tắt DoH, Cho phép chia sẻ LAN)\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";

        int choice = sc.readInt("");
        if (choice == 0) break;

        if (choice == 1) {
            sc.cls();
            cout << "============\n"
                 << "      ĐANG KÍCH HOẠT LÁ CHẮN BẢO MẬT TOÀN DIỆN\n"
                 << "============\n\n"
                 << "Đang tổng hợp toàn bộ quy tắc bảo mật và khởi chạy trong cửa sổ quản trị...\n";

            std::string batContent = 
                "@echo off\n"
                "chcp 65001 >nul\n"
                "title KICH HOAT LA CHAN BAO MAT HE THONG & MANG\n"
                "color 0A\n"
                "echo       DANG THIET LAP LA CHAN BAO MAT HE THONG & MANG\n"
                "echo.\n"
                "echo 1/8. Bat Windows Defender Realtime & Cap nhat Signature...\n"
                "powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $false\" >nul 2>&1\n"
                "\"%ProgramFiles%\\Windows Defender\\MpCmdRun.exe\" -SignatureUpdate >nul 2>&1\n"
                "echo       -> OK\n"
                "echo 2/8. Kich hoat Tuong lua Windows tren tat ca Profile...\n"
                "netsh advfirewall set allprofiles state on >nul 2>&1\n"
                "echo       -> OK\n"
                "echo 3/8. Bat bao ve chong Ransomware (Controlled Folder Access)...\n"
                "powershell -Command \"Set-MpPreference -EnableControlledFolderAccess Enabled\" >nul 2>&1\n"
                "echo       -> OK\n"
                "echo 4/8. Vo hieu hoa SMBv1, LLMNR Multicast va NetBIOS...\n"
                "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters\" /v SMB1 /t REG_DWORD /d 0 /f >nul 2>&1\n"
                "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient\" /v EnableMulticast /t REG_DWORD /d 0 /f >nul 2>&1\n"
                "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-NetAdapter -Name $_.Name -NetLuid $_.NetLuid -NetBIOSSetting Disabled }\" >nul 2>&1\n"
                "echo       -> OK\n"
                "echo 5/8. Thiet lap quy tac chan cong nguy hiem (TCP: 445, 135, 137, 138, 139)...\n"
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
                "echo       -> OK\n"
                "echo 6/8. Cau hinh DNS over HTTPS (Cloudflare 1.1.1.1 & 1.0.0.1 DoH)...\n"
                "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ServerAddresses ('1.1.1.1','1.0.0.1') }\" >nul 2>&1\n"
                "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters\" /v EnableAutoDoh /t REG_DWORD /d 2 /f >nul 2>&1\n"
                "echo       -> OK\n"
                "echo 7/8. Khoa va vo hieu hoa cac dich vu Remote nguy hiem...\n"
                "sc config RemoteRegistry start= disabled >nul 2>&1\n"
                "sc stop RemoteRegistry >nul 2>&1\n"
                "sc config TermService start= disabled >nul 2>&1\n"
                "sc stop TermService >nul 2>&1\n"
                "echo       -> OK\n"
                "echo 8/8. Vo hieu hoa thu thap chan doan Windows Telemetry...\n"
                "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection\" /v AllowTelemetry /t REG_DWORD /d 0 /f >nul 2>&1\n"
                "echo       -> OK\n"
                "echo.\n"
                "echo DA KICH HOAT THANH CONG TOAN BO LA CHAN BAO MAT!\n"
                "echo Cua so se tu dong dong sau 5 giay...\n"
                "timeout /t 5 >nul\n";

            if (SystemCore::runBatchAsAdmin(batContent, "Kích hoạt lá chắn bảo mật")) {
                cout << "\nĐã kích hoạt toàn bộ lá chắn bảo mật hệ thống & mạng thành công!\n";
            } else {
                cout << "\nThất bại khi thực thi lá chắn bảo mật (Cần cấp quyền Administrator).\n";
            }
            SystemCore::waitEnter();
        } 
        else if (choice == 2) {
            sc.cls();
            cout << "============\n"
                 << "         ĐANG KHÔI PHỤC THIẾT LẬP MẠNG MẶC ĐỊNH\n"
                 << "============\n\n"
                 << "Đang khởi chạy trong cửa sổ quản trị để khôi phục mặc định...\n";

            std::string batContent = 
                "@echo off\n"
                "chcp 65001 >nul\n"
                "title KHOI PHUC CAI DAT MANG MAC DINH\n"
                "color 0E\n"
                "echo           DANG KHOI PHUC CAI DAT MANG MAC DINH\n"
                "echo.\n"
                "echo 1/3. Khoi phuc giao thuc chia se mang LAN (SMB1, Multicast, NetBIOS)...\n"
                "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters\" /v SMB1 /t REG_DWORD /d 1 /f >nul 2>&1\n"
                "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient\" /v EnableMulticast /t REG_DWORD /d 1 /f >nul 2>&1\n"
                "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-NetAdapter -Name $_.Name -NetLuid $_.NetLuid -NetBIOSSetting Default }\" >nul 2>&1\n"
                "echo       -> OK\n"
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
                "echo       -> OK\n"
                "echo 3/3. Dat lai DNS tu dong tu Router (DHCP) & Tat DoH...\n"
                "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ResetServerAddresses }\" >nul 2>&1\n"
                "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters\" /v EnableAutoDoh /t REG_DWORD /d 0 /f >nul 2>&1\n"
                "echo       -> OK\n"
                "echo.\n"
                "echo DA KHOI PHUC THANH CONG CAI DAT MANG MAC DINH!\n"
                "echo Cua so se tu dong dong sau 5 giay...\n"
                "timeout /t 5 >nul\n";

            if (SystemCore::runBatchAsAdmin(batContent, "Khôi phục cài đặt mạng")) {
                cout << "\nĐã hoàn tất khôi phục cài đặt mạng mặc định thành công!\n";
            } else {
                cout << "\nThất bại khi khôi phục cài đặt mạng (Cần cấp quyền Administrator).\n";
            }
            SystemCore::waitEnter();
        }
    }
}

void Internet::checkHostsFileSecurity() {
    sc.cls();
    cout << "============\n"
         << "     QUÉT & BẢO VỆ TẬP TIN HOSTS (CHỐNG CHUYỂN HƯỚNG WEB)\n"
         << "============\n\n";

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
    cout << "\n BÁO CÁO TRẠNG THÁI BẢO MẬT CHI TIẾT \n\n";

    // 1. Windows Defender
    cout << "1. Windows Defender (Antivirus & Real-time Protection):\n";
    sc.runCMD("powershell -Command \"Get-MpComputerStatus | Select-Object AntivirusEnabled, RealTimeProtectionEnabled, IoavProtectionEnabled, AntispywareEnabled, FullScanAge\"");

    // 2. Tường lửa
    cout << "\n2. Trạng thái Tường lửa Windows (All Profiles):\n";
    sc.runCMD("netsh advfirewall show allprofiles | findstr /i \"State Profile Bật Tắt ON OFF\"");

    // 3. Cổng nguy hiểm
    cout << "\n3. Trạng thái Quy tắc chặn Port nguy hiểm:\n";
    vector<int> ports = {445, 139, 135, 137, 138};
    for (int p : ports) {
        string chk = "netsh advfirewall firewall show rule name=\"Block_Dangerous_Port_" + to_string(p) + "\" >nul 2>&1";
        if (system(chk.c_str()) == 0) {
            cout << "    Port " << p << " : ĐÃ CHẶN (An toàn)\n";
        } else {
            cout << "    Port " << p << " : ĐANG MỞ (Chưa chặn)\n";
        }
    }

    // 4. Dịch vụ điều khiển từ xa
    cout << "\n4. Dịch vụ nhạy cảm & Điều khiển từ xa:\n";
    vector<pair<string, string>> svcs = {
        {"RemoteRegistry", "Truy cập Registry từ xa"},
        {"TermService", "Remote Desktop Service"},
        {"RasMan", "Remote Access Connection Manager"}
    };
    for (auto &s : svcs) {
        bool running = isServiceRunningNative(s.first);
        bool disabled = isServiceDisabledNative(s.first);
        cout << "    - " << s.second << " (" << s.first << "): " 
             << (running ? "\x1b[31mĐANG CHẠY\x1b[0m" : "ĐÃ DỪNG") 
             << " | Khởi động: " << (disabled ? "\x1b[32mDISABLED (Khóa)\x1b[0m" : "ENABLED (Bật)") << "\n";
    }

    // 5. Cấu hình DNS & DoH
    cout << "\n5. Cấu hình DNS & DNS over HTTPS (DoH):\n";
    DWORD dohVal = 0;
    readRegDword(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", "EnableAutoDoh", dohVal);
    cout << "    - Mã hóa DoH: " << (dohVal == 2 ? "\x1b[32mĐÃ BẬT (DoH Active)\x1b[0m" : "\x1b[33mCHƯA BẬT (DoH Disabled)\x1b[0m") << "\n"
         << "    - Máy chủ DNS hiện tại:\n";
    sc.runCMD("ipconfig /all | findstr /i \"DNS\"");

    cout << "\n======\n";
    SystemCore::waitEnter();
}