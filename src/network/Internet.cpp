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
#include <wlanapi.h>

namespace fs = std::filesystem;
using namespace std;

static string wideToUtf8(const wstring &value) {
    if (value.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                                   NULL, 0, NULL, NULL);
    if (size <= 0) return "";
    string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                        &result[0], size, NULL, NULL);
    return result;
}

static string xmlUnescape(string value) {
    const pair<const char*, const char*> entities[] = {
        {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&apos;", "'"}
    };
    for (const auto &entity : entities) {
        size_t pos = 0;
        while ((pos = value.find(entity.first, pos)) != string::npos) {
            value.replace(pos, strlen(entity.first), entity.second);
            pos += strlen(entity.second);
        }
    }
    return value;
}

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
    cout << "Đặt lại mạng, DNS/ARP và IP.\n";
    if (!sc.confirm("Tiếp tục sửa lỗi mạng? (y/n): ")) return;

    std::string batContent = 
        "@echo off\n"
        "set \"failed=0\"\n"
        "chcp 65001 >nul\n"
        "title SUA LOI MANG\n"
        "echo [1/6] Xoa bo dem DNS\n"
        "ipconfig /flushdns >nul 2>&1\n"
        "echo [2/6] Dat lai Winsock Catalog\n"
        "netsh winsock reset >nul 2>&1\n"
        "if errorlevel 1 set \"failed=1\"\n"
        "echo [3/6] Dat lai ngan xep TCP/IP\n"
        "netsh int ip reset >nul 2>&1\n"
        "if errorlevel 1 set \"failed=1\"\n"
        "echo [4/6] Xoa bang ARP Cache\n"
        "netsh interface ip delete arpcache >nul 2>&1\n"
        "if errorlevel 1 set \"failed=1\"\n"
        "echo [5/6] Lam moi dia chi IP (DHCP)\n"
        "ipconfig /release >nul 2>&1\n"
        "ipconfig /renew >nul 2>&1\n"
        "echo [6/6] Khoi dong lai WinNAT va HNS\n"
        "net stop winnat >nul 2>&1 & net start winnat >nul 2>&1\n"
        "net stop hns >nul 2>&1 & net start hns >nul 2>&1\n"
        "echo Hoan tat!\n"
        "exit /b %failed%\n";

    cout << "\nĐang sửa mạng (Admin)...\n";
    if (SystemCore::runBatchAsAdmin(batContent, "Sửa lỗi mạng")) {
        cout << "[✓] Đã sửa mạng.\n";
    } else {
        cout << "[!] Thất bại; cần quyền Admin.\n";
    }
    sc.waitEnter();
}

// ----------------------------------------------------------------------------------
// BƯỚC 2: TRA CỨU MẬT KHẨU WI-FI ĐÃ LƯU
// Cơ chế: Sử dụng netsh wlan truy xuất trực tiếp profile XML được Windows lưu trong máy.
// ----------------------------------------------------------------------------------
void Internet::wifiAudit() {
    sc.cls();
    HMODULE dll = LoadLibraryW(L"wlanapi.dll");
    if (!dll) { cout << "[!] Không mở được WLAN API.\n"; sc.waitEnter(); return; }
    auto openHandle = reinterpret_cast<decltype(&WlanOpenHandle)>(GetProcAddress(dll, "WlanOpenHandle"));
    auto closeHandle = reinterpret_cast<decltype(&WlanCloseHandle)>(GetProcAddress(dll, "WlanCloseHandle"));
    auto enumInterfaces = reinterpret_cast<decltype(&WlanEnumInterfaces)>(GetProcAddress(dll, "WlanEnumInterfaces"));
    auto getProfiles = reinterpret_cast<decltype(&WlanGetProfileList)>(GetProcAddress(dll, "WlanGetProfileList"));
    auto getProfile = reinterpret_cast<decltype(&WlanGetProfile)>(GetProcAddress(dll, "WlanGetProfile"));
    auto freeMemory = reinterpret_cast<decltype(&WlanFreeMemory)>(GetProcAddress(dll, "WlanFreeMemory"));
    if (!openHandle || !closeHandle || !enumInterfaces || !getProfiles || !getProfile || !freeMemory) {
        FreeLibrary(dll); cout << "[!] WLAN API không đầy đủ.\n"; sc.waitEnter(); return;
    }
    HANDLE client = NULL;
    DWORD negotiated = 0;
    if (openHandle(2, NULL, &negotiated, &client) != ERROR_SUCCESS) {
        FreeLibrary(dll); cout << "[!] Không truy cập được Wi-Fi.\n"; sc.waitEnter(); return;
    }
    PWLAN_INTERFACE_INFO_LIST interfaces = NULL;
    struct SavedProfile { GUID interfaceId; wstring name; };
    vector<SavedProfile> profiles;
    if (enumInterfaces(client, NULL, &interfaces) == ERROR_SUCCESS && interfaces) {
        for (DWORD i = 0; i < interfaces->dwNumberOfItems; ++i) {
            PWLAN_PROFILE_INFO_LIST list = NULL;
            const GUID &id = interfaces->InterfaceInfo[i].InterfaceGuid;
            if (getProfiles(client, &id, NULL, &list) == ERROR_SUCCESS && list) {
                for (DWORD j = 0; j < list->dwNumberOfItems; ++j)
                    profiles.push_back({id, list->ProfileInfo[j].strProfileName});
            }
            if (list) freeMemory(list);
        }
    }
    if (interfaces) freeMemory(interfaces);
    if (profiles.empty()) {
        cout << "[!] Không có profile Wi-Fi đã lưu.\n";
    } else {
        for (size_t i = 0; i < profiles.size(); ++i)
            cout << " [" << i + 1 << "] " << wideToUtf8(profiles[i].name) << "\n";
        cout << " [0] Quay lại\n";
        int choice = sc.readInt(" Chọn: ");
        if (choice > 0 && choice <= static_cast<int>(profiles.size())) {
            const SavedProfile &selected = profiles[choice - 1];
            LPWSTR xml = NULL;
            DWORD flags = WLAN_PROFILE_GET_PLAINTEXT_KEY;
            DWORD access = 0;
            DWORD result = getProfile(client, &selected.interfaceId, selected.name.c_str(), NULL,
                                      &xml, &flags, &access);
            cout << " Wi-Fi: " << wideToUtf8(selected.name) << "\n";
            if (result == ERROR_SUCCESS && xml) {
                wstring profileXml(xml);
                size_t start = profileXml.find(L"<keyMaterial>");
                size_t end = start == wstring::npos ? wstring::npos : profileXml.find(L"</keyMaterial>", start);
                if (end != wstring::npos)
                    cout << " Mật khẩu: " << xmlUnescape(wideToUtf8(profileXml.substr(start + 13, end - start - 13))) << "\n";
                else cout << " Mật khẩu: Không có hoặc không được phép xem.\n";
            } else cout << "[!] Không đọc được profile hoặc khóa Wi-Fi.\n";
            if (xml) freeMemory(xml);
        }
    }
    closeHandle(client, NULL);
    FreeLibrary(dll);
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
    if (res == ERROR_SUCCESS && type == REG_DWORD && size == sizeof(data)) {
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
        cout << "== BẢO MẬT HỆ THỐNG ==\n"
             << " [1] Bật lá chắn (Defender, Firewall, Chặn Port Telnet/RPC)\n"
             << "     * Thông suốt 100% truyền file Media, Smart View, LAN\n"
             << " [2] Gỡ rule CMD Box\n"
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
                "set \"failed=0\"\n"
                "chcp 65001 >nul\n"
                "title KICH HOAT LA CHAN BAO MAT HE THONG & MANG\n"
                "echo 1/5. Bat Windows Defender Realtime va cap nhat mau virus\n"
                "powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $false; Update-MpSignature\" >nul 2>&1\n"
                "if errorlevel 1 set \"failed=1\"\n"
                "echo 2/5. Kich hoat Tuong lua Windows (Tat ca profiles)\n"
                "netsh advfirewall set allprofiles state on >nul 2>&1\n"
                "if errorlevel 1 set \"failed=1\"\n"
                "echo 3/5. Bat chong ma doc tong tien (Controlled Folder Access)\n"
                "powershell -Command \"Set-MpPreference -EnableControlledFolderAccess Enabled\" >nul 2>&1\n"
                "if errorlevel 1 set \"failed=1\"\n"
                "echo 4/5. Chan cac cong nguy hiem de bi khai thac tu xa (Telnet: 23, TFTP: 69, RPC: 135)\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_23\" >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_23\" dir=in action=block protocol=TCP localport=23 >nul 2>&1\n"
                "if errorlevel 1 set \"failed=1\"\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_69\" >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_69\" dir=in action=block protocol=UDP localport=69 >nul 2>&1\n"
                "if errorlevel 1 set \"failed=1\"\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_135\" >nul 2>&1\n"
                "netsh advfirewall firewall add rule name=\"Block_Dangerous_Port_135\" dir=in action=block protocol=TCP localport=135 >nul 2>&1\n"
                "if errorlevel 1 set \"failed=1\"\n"
                "echo 5/5. Vo hieu hoa dich vu Remote Registry doc hai\n"
                "sc config RemoteRegistry start= disabled >nul 2>&1\n"
                "if errorlevel 1 set \"failed=1\"\n"
                "sc stop RemoteRegistry >nul 2>&1\n"
                "echo Hoan tat!\n"
                "exit /b %failed%\n";

            if (SystemCore::runBatchAsAdmin(batContent, "Kích hoạt lá chắn bảo mật")) {
                cout << "\n[OK] Đã kích hoạt lá chắn bảo mật thành công! Các cổng chia sẻ Media vẫn hoạt động bình thường.\n";
            } else {
                cout << "\n[Lỗi] Thất bại khi thực thi lá chắn bảo mật (Cần quyền Administrator).\n";
            }
            SystemCore::waitEnter();
        } 
        else if (choice == 2) {
            sc.cls();
            cout << "Đang gỡ rule CMD Box\n\n"
                 << "Đang khởi chạy quy trình trong cửa sổ quản trị\n";

            std::string batContent = 
                "@echo off\n"
                "set \"failed=0\"\n"
                "chcp 65001 >nul\n"
                "title KHOI PHUC CAI DAT MANG MAC DINH\n"
                "echo 1/2. Go bo quy tac chan cong tren tuong lua\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_23\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_69\" >nul 2>&1\n"
                "netsh advfirewall firewall delete rule name=\"Block_Dangerous_Port_135\" >nul 2>&1\n"
                "echo Hoan tat!\n"
                "exit /b %failed%\n";

            if (SystemCore::runBatchAsAdmin(batContent, "Khôi phục cài đặt mạng")) {
                cout << "\n[OK] Đã gỡ rule CMD Box.\n";
            } else {
                cout << "\n[Lỗi] Không gỡ được rule CMD Box.\n";
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
    cout << "== TRẠNG THÁI BẢO MẬT ==\n";

    // 1. Windows Defender Real-time
    cout << " 1. Windows Defender:\n";
    bool defService = isServiceRunningNative("WinDefend");
    DWORD defDisable = 0;
    bool hasDefDisable = readRegDword(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection", "DisableRealtimeMonitoring", defDisable);
    bool rtPolicyDisabled = hasDefDisable && defDisable != 0;
    cout << "    - Dịch vụ WinDefend  : " << (defService ? "\x1b[32mĐANG CHẠY\x1b[0m" : "\x1b[31mĐÃ DỪNG\x1b[0m") << "\n"
         << "    - Policy Real-Time   : " << (rtPolicyDisabled ? "\x1b[31mĐang tắt bảo vệ\x1b[0m" :
             "Không có policy tắt (chưa xác minh trạng thái thực)") << "\n";

    // 2. Tường lửa Windows (Firewall)
    cout << "\n 2. Tường lửa Windows:\n";
    FILE *pipeFw = _popen("netsh advfirewall show allprofiles state", "r");
    int fwOnCount = 0;
    int fwProfileCount = 0;
    if (pipeFw) {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipeFw)) {
            string l = sc.trim(string(buf));
            if (l.find("Profile") != string::npos || l.find("State") != string::npos || l.find("Trạng thái") != string::npos) {
                cout << "    - " << l << "\n";
                if (l.find("State") != string::npos || l.find("Trạng thái") != string::npos) {
                    ++fwProfileCount;
                    if (l.find("ON") != string::npos || l.find("BẬT") != string::npos) ++fwOnCount;
                }
            }
        }
        _pclose(pipeFw);
    }
    if (fwProfileCount == 0) {
        cout << "    [!] Không xác định được trạng thái firewall.\n";
    } else if (fwOnCount < fwProfileCount) {
        cout << "    \x1b[33m[!] Có profile firewall đang tắt.\x1b[0m\n";
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
        cout << "    - Rule port " << cp.first << " [" << cp.second << "]: "
             << (blocked ? "\x1b[32mCó rule CMD Box\x1b[0m" : "\x1b[33mKhông có rule CMD Box\x1b[0m") << "\n";
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
