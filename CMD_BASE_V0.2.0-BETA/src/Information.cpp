#include "../include/Information.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <intrin.h>
#include <psapi.h> // Cho hàm driverList

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")

using namespace std;

Information::Information(SystemCore &s) : sc(s) {}
Information::~Information() {}

void Information::showNetworkDetails() {
    sc.cls();
    cout << "┌────────────────────────────────────────────────────────┐\n";
    cout << "│               THÔNG TIN CẤU HÌNH MẠNG SÂU              │\n";
    cout << "└────────────────────────────────────────────────────────┘\n";
    cout << "[1] Xem toàn bộ cấu hình Adapter & DNS chi tiết (ipconfig)\n";
    cout << "[2] Kiểm tra các Cổng (Port) đang kết nối và Lắng nghe (netstat)\n";
    cout << "[3] Xem bảng định tuyến mạng hiện tại (route print)\n";
    cout << "[0] Quay lại\n";
    int choice = sc.readInt("Chọn tính năng: ");

    if (choice == 1) {
        // Lệnh xem chi tiết Mac Address, DNS Server, DHCP Status
        sc.runCMD("ipconfig /all");
    }
    else if (choice == 2) {
        // Lệnh hiển thị các port đang kết nối mạng (ESTABLISHED/LISTENING) kèm theo PID
        cout << "[*] Đang thống kê các kết nối mạng hoạt động...\n";
        sc.runCMD("netstat -an | findstr /i \"ESTABLISHED LISTENING\"");
    }
    else if (choice == 3) {
        // Lệnh in bảng định tuyến mạng từ file tài liệu
        sc.runCMD("route print");
    }
}


void Information::showDashboard() {
    double totalRAM, freeRAM;
    getRAMInfo(totalRAM, freeRAM);
    double usedPercent = ((totalRAM - freeRAM) / totalRAM) * 100;

    // Chuẩn bị dữ liệu để đưa vào khung
    vector<pair<string, string>> sysData = {
        {"THÀNH PHẦN", "THÔNG TIN CHI TIẾT SYSTEM"},
        {"CPU Model", getCPUModel()},
        {"CPU Cores", to_string(getCPUCores()) + " Cores | " + to_string(getCPUSpeed()).substr(0,4) + " GHz"},
        {"GPU Model", getGPUModel() + " (" + getGPUMemory() + ")"},
        {"RAM Status", to_string(totalRAM).substr(0,4) + " GB (Đã dùng: " + to_string(usedPercent).substr(0,4) + "%)"},
        {"Ổ đĩa C", getDiskCStatus()},
        {"OS Version", getWindowsVersion()},
        {"Uptime", getUptime()}
    };

    // Độ rộng cố định cho 2 cột: Cột 1 rộng 15 ký tự, Cột 2 rộng 50 ký tự
    vector<int> widths = {15, 50}; 

    sc.cls();
    // Vẽ hàng góc trên cùng ┌───────────────┬──────────────────────────────────────────────────┐
    sc.setColor(3); // Đổi sang màu Cyan cho đẹp
    drawHorizontalLine((char)218, (char)194, (char)191, widths);

    // Dòng tiêu đề
    cout << "│ " << left << setw(13) << sysData[0].first 
         << " │ " << left << setw(48) << sysData[0].second << " │\n";

    // Vẽ đường ngăn cách tiêu đề ├───────────────┼──────────────────────────────────────────────────┤
    drawHorizontalLine((char)195, (char)197, (char)180, widths);
    sc.setColor(7); // Trả về màu chữ trắng cho nội dung bên trong

    // Duyệt qua các phần tử phần cứng để in ra kèm bọc khung dọc │
    for (size_t i = 1; i < sysData.size(); ++i) {
        sc.setColor(3); cout << "│ "; sc.setColor(7);
        cout << left << setw(13) << sysData[i].first;
        sc.setColor(3); cout << " │ "; sc.setColor(7);
        cout << left << setw(48) << sysData[i].second;
        sc.setColor(3); cout << " │\n";
    }

    // Vẽ hàng đáy dưới cùng └───────────────┴──────────────────────────────────────────────────┘
    sc.setColor(3);
    drawHorizontalLine((char)192, (char)193, (char)217, widths);
    sc.setColor(7);
}


// ================= CÁC HÀM API CHUYỂN TỪ SYSTEMCORE =================

string Information::getCPUModel() {
    int cpuInfo[4] = {-1};
    char cpuBrand[49];
    __cpuid(cpuInfo, 0x80000002);
    memcpy(cpuBrand, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000003);
    memcpy(cpuBrand + 16, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000004);
    memcpy(cpuBrand + 32, cpuInfo, sizeof(cpuInfo));
    return string(cpuBrand);
}

string Information::getGPUModel() {
    HKEY hKey;
    char value[256] = "Integrated Graphics";
    DWORD vSize = sizeof(value);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinSAT", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "PrimaryAdapterString", NULL, NULL, (LPBYTE)value, &vSize);
        RegCloseKey(hKey);
    }
    return string(value);
}

string Information::getDiskCStatus() {
    ULARGE_INTEGER free, total, tFree;
    if (GetDiskFreeSpaceExA("C:\\", &free, &total, &tFree)) {
        double totalGB = (double)total.QuadPart / (1024 * 1024 * 1024);
        double usedGB = totalGB - ((double)tFree.QuadPart / (1024 * 1024 * 1024));
        stringstream ss;
        ss << fixed << setprecision(1) << "C: " << usedGB << "/" << totalGB << " GB";
        return ss.str();
    }
    return "C: Unknown";
}

void Information::getRAMInfo(double &total, double &free) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    total = (double)memInfo.ullTotalPhys / (1024 * 1024 * 1024);
    free = (double)memInfo.ullAvailPhys / (1024 * 1024 * 1024);
}

string Information::getUptime() {
    ULONGLONG ms = GetTickCount64();
    int days = ms / (24 * 3600 * 1000);
    int hours = (ms / (3600 * 1000)) % 24;
    int mins = (ms / (60 * 1000)) % 60;
    return to_string(days) + "d " + to_string(hours) + "h " + to_string(mins) + "m";
}

string Information::getHostname() {
    static string cached = "";
    if (!cached.empty()) return cached;
    char hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(hostname);
    if (GetComputerNameA(hostname, &size)) {
        cached = string(hostname);
        return cached;
    }
    cached = "Unknown";
    return cached;
}

string Information::getIPv4Address() {
    static string cached = "";
    if (!cached.empty()) return cached;

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        cached = "0.0.0.0";
        return cached;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    addr.sin_addr.s_addr = inet_addr("8.8.8.8");

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0) {
        sockaddr_in local;
        int len = sizeof(local);
        if (getsockname(sock, (sockaddr*)&local, &len) == 0) {
            cached = inet_ntoa(local.sin_addr);
            closesocket(sock);
            return cached;
        }
    }
    closesocket(sock);
    cached = "127.0.0.1";
    return cached;
}

string Information::getWindowsVersion() {
    static string cached = "";
    if (!cached.empty()) return cached;

    HKEY hKey;
    char productName[256] = "Unknown Windows";
    char currentBuild[64] = "0";
    DWORD size = sizeof(productName);
    DWORD buildSize = sizeof(currentBuild);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "ProductName", NULL, NULL, (LPBYTE)productName, &size);
        RegQueryValueExA(hKey, "CurrentBuild", NULL, NULL, (LPBYTE)currentBuild, &buildSize);
        RegCloseKey(hKey);
    }

    string name = string(productName);
    int buildNum = stoi(currentBuild);

    // Logic quan trọng: Nếu Build >= 22000 thì là Windows 11
    if (buildNum >= 22000) {
        size_t pos = name.find("Windows 10");
        if (pos != string::npos) {
            name.replace(pos, 10, "Windows 11");
        }
    }

    cached = name;
    return cached;
}

int Information::getCPUCores() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
}

double Information::getCPUSpeed() {
    HKEY hKey;
    DWORD speed = 0;
    DWORD vSize = sizeof(speed);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "~MHz", NULL, NULL, (LPBYTE)&speed, &vSize);
        RegCloseKey(hKey);
    }
    return speed > 0 ? (double)speed / 1000.0 : 0.0;
}

string Information::getGPUMemory() {
    HKEY hKey;
    DWORD memory = 0;
    DWORD vSize = sizeof(memory);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Intel\\GMM", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "DedicatedSegmentSize", NULL, NULL, (LPBYTE)&memory, &vSize);
        RegCloseKey(hKey);
    }

    if (memory > 0) {
        double memGB = (double)memory / (1024 * 1024 * 1024);
        stringstream ss;
        ss << fixed << setprecision(1) << memGB << " GB";
        return ss.str();
    }
    return "Shared";
}

string Information::getDeviceType() {
    static string cached = "";
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        // 128 (0x80) nghĩa là "No System Battery" - Thường là Desktop
        if (sps.BatteryFlag & 128) { 
            cached = "Desktop (AC)"; 
            return cached; 
        }
        stringstream ss;
        string status = (sps.ACLineStatus == 1) ? "Charging" : "Discharging";

        if (sps.BatteryLifePercent != 255) {
            ss << "Laptop [" << status << "]: " << (int)sps.BatteryLifePercent << "%";
        } else {
            ss << "Laptop [" << status << "]: Battery Unknown";
        }   
        cached = ss.str();
        return cached;
    }
    cached = "Unknown Device";
    return cached;
}

string Information::getAppLicenses() {
    vector<string> apps;
    
    // Check Office
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Office\\ClickToRun\\Configuration", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        apps.push_back("Office");
        RegCloseKey(hKey);
    }

    // Check Adobe (Photoshop/Illustrator...)
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Adobe\\Adobe Acrobat", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        apps.push_back("Adobe");
        RegCloseKey(hKey);
    }

    // Check AutoCAD
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Autodesk\\AutoCAD", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        apps.push_back("AutoCAD");
        RegCloseKey(hKey);
    }

    if (apps.empty()) return "None Detected";
    
    string finalStr = "";
    for (size_t i = 0; i < apps.size(); ++i) {
        finalStr += apps[i] + (i == apps.size() - 1 ? "" : ", ");
    }
    return finalStr + " (Installed)";
}


string Information::getWindowsLicenseStatus() {
    // Sử dụng cscript để gọi slmgr.vbs lấy trạng thái chi tiết
    // Lệnh này chạy ngầm và xuất ra file tạm
    string cmd = "cscript //nologo %systemroot%\\system32\\slmgr.vbs /dli > license_check.txt";
    system(cmd.c_str());

    ifstream file("license_check.txt");
    string line, result = "Unlicensed";
    bool found = false;

    if (file.is_open()) {
        while (getline(file, line)) {
            // Kiểm tra dòng "License Status"
            if (line.find("License Status:") != string::npos) {
                if (line.find("Licensed") != string::npos) {
                    result = "Licensed (Chính thức)";
                } else if (line.find("Initial grace period") != string::npos) {
                    result = "Grace Period (Dùng thử)";
                } else {
                    result = "Unlicensed/Expired";
                }
                found = true;
                break;
            }
        }
        file.close();
        remove("license_check.txt"); 
    }

    return found ? result : "Unknown Status";
}

// Hàm bổ trợ để vẽ các đường viền góc cạnh đóng khung hộp
void Information::drawHorizontalLine(char start, char middle, char end, const vector<int>& widths) {
    cout << start;
    for (size_t i = 0; i < widths.size(); ++i) {
        for (int j = 0; j < widths[i]; ++j) cout << "─";
        if (i < widths.size() - 1) cout << middle;
    }
    cout << end << "\n";
}


void Information::showAllInfo() {
    sc.cls(); 

    // 1. In Dashboard phần cứng (Khung bo vuông)
    showDashboard();
    cout << "\n";

    sc.setColor(3);
    cout << "┌────────────────────────────────────────────────────────┐\n";
    cout << "│               TRẠNG THÁI BẢN QUYỀN OS                  │\n";
    cout << "└────────────────────────────────────────────────────────┘\n";
    sc.setColor(7);
    cout << getWindowsLicenseStatus() << "\n";
}