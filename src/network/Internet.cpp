#include "Internet.h"
#include "LocalDrop.h"
#include "NetworkScanner.h"
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#include "SystemCore.h"
#include <filesystem>
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <ctime>

namespace fs = std::filesystem;
using namespace std;

Internet::Internet(SystemCore &s) : sc(s) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

Internet::~Internet() {
    WSACleanup();
}

string Internet::getField(const string &line) {
    size_t pos = line.find(':');
    if (pos != string::npos) return SystemCore::trim(line.substr(pos + 1));
    return "";
}

// ----------------------------------------------------------------------------------
// BƯỚC 1: SỬA LỖI & KHÔI PHỤC MẠNG TOÀN DIỆN
// Cơ chế: Xóa sạch cache lỗi (DNS, ARP), reset ngăn xếp socket Winsock và TCP/IP,
// sau đó ép Router cấp mới địa chỉ IP (release/renew) và khởi động lại WinNAT.
// ----------------------------------------------------------------------------------
void Internet::repairNetwork() {
    sc.cls();
    cout << "Đặt lại Winsock, TCP/IP, xóa cache DNS/ARP và cấp mới IP.\n";
    if (!sc.confirm("Tiếp tục sửa lỗi mạng? (y/n): ")) return;

    std::string batContent = 
        "@echo off\n"
        "chcp 65001 >nul\n"
        "title SUA LOI MANG\n"
        "echo [1/6] Xoa bo dem DNS\n"
        "ipconfig /flushdns >nul 2>&1\n"
        "echo [2/6] Dat lai Winsock Catalog\n"
        "netsh winsock reset >nul 2>&1\n"
        "echo [3/6] Dat lai ngan xep TCP/IP\n"
        "netsh int ip reset >nul 2>&1\n"
        "echo [4/6] Xoa bang ARP Cache\n"
        "netsh interface ip delete arpcache >nul 2>&1\n"
        "echo [5/6] Lam moi dia chi IP (DHCP)\n"
        "ipconfig /release >nul 2>&1\n"
        "ipconfig /renew >nul 2>&1\n"
        "echo [6/6] Khoi dong lai WinNAT va HNS\n"
        "net stop winnat >nul 2>&1 & net start winnat >nul 2>&1\n"
        "net stop hns >nul 2>&1 & net start hns >nul 2>&1\n"
        "echo Hoan tat!\n";

    cout << "\nĐang sửa lỗi mạng (quyền Admin)\n";
    if (SystemCore::runBatchAsAdmin(batContent, "Sửa lỗi mạng")) {
        cout << "[✓] Đã khôi phục mạng thành công!\n";
    } else {
        cout << "[!] Thất bại (Cần quyền Administrator).\n";
    }
    sc.waitEnter();
}

// ----------------------------------------------------------------------------------
// BƯỚC 2: TRA CỨU MẬT KHẨU WI-FI ĐÃ LƯU
// Cơ chế: Sử dụng netsh wlan truy xuất trực tiếp profile XML được Windows lưu trong máy.
// ----------------------------------------------------------------------------------
void Internet::wifiAudit() {
    sc.cls();

    FILE *pipe = _popen("netsh wlan show profiles", "r");
    if (!pipe) {
        cout << "[!] Không thể truy cập cấu hình Wi-Fi.\n";
        sc.waitEnter();
        return;
    }

    char buf[512];
    vector<string> wifiList;
    while (fgets(buf, sizeof(buf), pipe)) {
        string line = buf;
        if (line.find("All User Profile") != string::npos || line.find("hồ sơ") != string::npos || line.find("Profile") != string::npos) {
            string wifiName = getField(line);
            if (!wifiName.empty()) {
                wifiList.push_back(wifiName);
            }
        }
    }
    _pclose(pipe);

    if (wifiList.empty()) {
        cout << "[!] Không tìm thấy cấu hình Wi-Fi nào trên máy.\n";
        sc.waitEnter();
        return;
    }

    for (size_t i = 0; i < wifiList.size(); ++i) {
        cout << " [" << i + 1 << "] " << wifiList[i] << "\n";
    }
    cout << " [0] Quay lại\n\n";

    int choice = sc.readInt(" -> Chọn số thứ tự: ");
    if (choice == 0) return;

    if (choice < 1 || choice > static_cast<int>(wifiList.size())) {
        cout << "[!] Lựa chọn không hợp lệ!\n";
        sc.waitEnter();
        return;
    }

    string selectedWifi = wifiList[choice - 1];
    string cmd = "netsh wlan show profile \"" + selectedWifi + "\" key=clear";
    FILE *p2 = _popen(cmd.c_str(), "r");
    if (!p2) {
        cout << "[!] Thất bại khi truy vấn mật khẩu.\n";
        sc.waitEnter();
        return;
    }

    char b2[512];
    string auth = "", cipher = "", pass = "(Mạng Mở/Không mật khẩu)";
    while (fgets(b2, sizeof(b2), p2)) {
        string inf = b2;
        if (inf.find("Authentication") != string::npos || inf.find("Xác thực") != string::npos) auth = getField(inf);
        if (inf.find("Cipher") != string::npos || inf.find("Mã hóa") != string::npos) cipher = getField(inf);
        if (inf.find("Key Content") != string::npos || inf.find("Nội dung khóa") != string::npos) pass = getField(inf);
    }
    _pclose(p2);

    sc.cls();
    cout << " Tên Wi-Fi : " << selectedWifi << "\n"
         << " Mật khẩu  : \x1b[32m" << pass << "\x1b[0m\n"
         << " Bảo mật   : " << auth << " (" << cipher << ")\n\n";
    sc.waitEnter();
}

// ----------------------------------------------------------------------------------
// BƯỚC 3: CÁC TIỆN ÍCH TRUY VẤN HỆ THỐNG NATIVE (Win32 Registry & Service API)
// ----------------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------------
// BƯỚC 4: LÁ CHẮN BẢO MẬT HỆ THỐNG & MẠNG
// Quan trọng: Chỉ khóa các cổng nguy hiểm (Telnet 23, TFTP 69, RPC 135).
// TUYỆT ĐỐI KHÔNG CHẶN 445, 137, 138, 139 để bảo toàn ứng dụng truyền file Media.
// ----------------------------------------------------------------------------------
void Internet::fullSecurityShield() {
    while (true) {
        sc.cls();
        cout << "LÁ CHẮN BẢO MẬT HỆ THỐNG & MẠNG\n\n"
             << " [1] Bật lá chắn (Defender, Firewall, Chặn Port Telnet/RPC)\n"
             << "     * Thông suốt 100% truyền file Media, Smart View, LAN\n"
             << " [2] Khôi phục mặc định\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";

        int choice = sc.readInt("");
        if (choice == 0) break;

        if (choice == 1) {
            sc.cls();
            cout << "Đang kích hoạt lá chắn bảo mật\n\n"
                 << "Đang tổng hợp quy tắc an toàn trong cửa sổ quản trị\n";

            std::string batContent = 
                "@echo off\n"
                "chcp 65001 >nul\n"
                "title KICH HOAT LA CHAN BAO MAT HE THONG & MANG\n"
                "echo 1/5. Bat Windows Defender Realtime va cap nhat mau virus\n"
                "powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $false; Update-MpSignature\" >nul 2>&1\n"
                "echo 2/5. Kich hoat Tuong lua Windows (Tat ca profiles)\n"
                "netsh advfirewall set allprofiles state on >nul 2>&1\n"
                "echo 3/5. Bat chong ma doc tong tien (Controlled Folder Access)\n"
                "powershell -Command \"Set-MpPreference -EnableControlledFolderAccess Enabled\" >nul 2>&1\n"
                "echo 4/5. Chan cac cong nguy hiem de bi khai thac tu xa (Telnet: 23, TFTP: 69, RPC: 135)\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_23\" >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_23\" dir=in action=block protocol=TCP localport=23 >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_69\" >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_69\" dir=in action=block protocol=UDP localport=69 >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_135\" >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_135\" dir=in action=block protocol=TCP localport=135 >nul 2>&1\n"
                "echo 5/5. Vo hieu hoa dich vu Remote Registry doc hai\n"
                "sc config RemoteRegistry start= disabled >nul 2>&1\n"
                "sc stop RemoteRegistry >nul 2>&1\n"
                "echo Hoan tat!\n";

            if (SystemCore::runBatchAsAdmin(batContent, "Kích hoạt lá chắn bảo mật")) {
                cout << "\n[OK] Đã kích hoạt lá chắn bảo mật thành công! Các cổng chia sẻ Media vẫn hoạt động bình thường.\n";
            } else {
                cout << "\n[Lỗi] Thất bại khi thực thi lá chắn bảo mật (Cần quyền Administrator).\n";
            }
            SystemCore::waitEnter();
        } 
        else if (choice == 2) {
            sc.cls();
            cout << "Đang khôi phục thiết lập mạng mặc định\n\n"
                 << "Đang khởi chạy quy trình trong cửa sổ quản trị\n";

            std::string batContent = 
                "@echo off\n"
                "chcp 65001 >nul\n"
                "title KHOI PHUC CAI DAT MANG MAC DINH\n"
                "echo 1/2. Go bo quy tac chan cong tren tuong lua\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_23\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_69\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_135\" >nul 2>&1\n"
                "echo 2/2. Dat lai DNS tu dong tu Router (DHCP)\n"
                "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ResetServerAddresses }\" >nul 2>&1\n"
                "echo Hoan tat!\n";

            if (SystemCore::runBatchAsAdmin(batContent, "Khôi phục cài đặt mạng")) {
                cout << "\n[OK] Đã hoàn tất khôi phục cài đặt mặc định thành công!\n";
            } else {
                cout << "\n[Lỗi] Thất bại khi khôi phục cài đặt (Cần quyền Administrator).\n";
            }
            SystemCore::waitEnter();
        }
    }
}

// ----------------------------------------------------------------------------------
// BƯỚC 5: KIỂM TRA TRẠNG THÁI BẢO MẬT HỆ THỐNG THIẾT YẾU
// Cơ chế: Kiểm tra 4 yếu tố cốt lõi (Defender Realtime, Firewall, Spyware Proxy, Port hiểm)
// ----------------------------------------------------------------------------------
void Internet::checkSecurityStatus() {
    sc.cls();
    cout << "TRẠNG THÁI BẢO MẬT HỆ THỐNG\n\n";

    // 1. Windows Defender Real-time
    cout << " 1. Windows Defender:\n";
    bool defService = isServiceRunningNative("WinDefend");
    DWORD defDisable = 0;
    bool hasDefDisable = readRegDword(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection", "DisableRealtimeMonitoring", defDisable);
    bool rtActive = defService && (!hasDefDisable || defDisable == 0);
    cout << "    - Dịch vụ WinDefend  : " << (defService ? "\x1b[32mĐANG CHẠY\x1b[0m" : "\x1b[31mĐÃ DỪNG\x1b[0m") << "\n"
         << "    - Real-Time Protect  : " << (rtActive ? "\x1b[32mBẬT (An toàn)\x1b[0m" : "\x1b[31mTẮT\x1b[0m") << "\n";

    // 2. Tường lửa Windows (Firewall)
    cout << "\n 2. Tường lửa Windows:\n";
    FILE *pipeFw = _popen("netsh advfirewall show allprofiles state", "r");
    bool fwAnyOn = false;
    if (pipeFw) {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipeFw)) {
            string l = sc.trim(string(buf));
            if (l.find("Profile") != string::npos || l.find("State") != string::npos || l.find("Trạng thái") != string::npos) {
                cout << "    - " << l << "\n";
                if (l.find("ON") != string::npos || l.find("BẬT") != string::npos) fwAnyOn = true;
            }
        }
        _pclose(pipeFw);
    }
    if (!fwAnyOn) {
        cout << "    \x1b[33m-> Cảnh báo: Tường lửa đang tắt!\x1b[0m\n";
    }

    // 3. Kiểm tra Proxy lén (Spyware hay dùng để nghe lén gói tin)
    cout << "\n 3. Cấu hình mạng Proxy (Kiểm tra phần mềm gián điệp):\n";
    DWORD proxyEnable = 0;
    readRegDword(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", "ProxyEnable", proxyEnable);
    if (proxyEnable == 1) {
        cout << "    - Proxy hệ thống     : \x1b[31mĐANG BẬT (Cần kiểm tra có bị cài lén không!)\x1b[0m\n";
    } else {
        cout << "    - Proxy hệ thống     : \x1b[32mTẮT (Kết nối trực tiếp an toàn)\x1b[0m\n";
    }

    // 4. Kiểm tra cổng rủi ro cao từ xa (Telnet, RPC)
    cout << "\n 4. Trạng thái các cổng nguy hiểm:\n";
    vector<pair<int, string>> critPorts = {
        {23, "Telnet (Cũ, không mã hóa)"},
        {135, "RPC Mapper (Dễ bị khai thác)"}
    };
    for (const auto &cp : critPorts) {
        bool blocked = SystemCore::runRawCommand("netsh.exe advfirewall firewall show rule name=\"Block_Dangerous_Port_" + to_string(cp.first) + "\"");
        cout << "    - Port " << cp.first << " [" << cp.second << "]: " 
             << (blocked ? "\x1b[32mĐÃ CHẶN (An toàn)\x1b[0m" : "\x1b[33mMỞ (Mặc định)\x1b[0m") << "\n";
    }

    // 5. Ghi chú bảo toàn Media
    cout << "\n * Lưu ý: Cổng chia sẻ Media (445, 137-139) được giữ mở để truyền file.\n";

    cout << "\n";
    SystemCore::waitEnter();
}

// ----------------------------------------------------------------------------------
// BƯỚC 6: QUÉT THIẾT BỊ KẾT NỐI WI-FI / LAN
// Chuyển tiếp (delegate) sang module NetworkScanner chuyên biệt
// ----------------------------------------------------------------------------------
void Internet::scanConnectedDevices() {
    NetworkScanner scanner(sc);
    scanner.scanConnectedDevices();
}

// ----------------------------------------------------------------------------------
// BƯỚC 7: LOCAL WEB DROP (P2P FILE SHARING)
// Chuyển tiếp (delegate) sang module LocalDrop chuyên biệt
// ----------------------------------------------------------------------------------
void Internet::localDropMenu() {
    LocalDrop ld(sc);
    ld.menu();
}
