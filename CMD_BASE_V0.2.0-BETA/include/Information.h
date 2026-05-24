#ifndef INFORMATION_H
#define INFORMATION_H

#include "SystemCore.h"
#include <string>

class Information {
private:
    SystemCore &sc;
    void drawHorizontalLine(char start, char middle, char end, const std::vector<int>& widths);

public:
    Information(SystemCore &s);
    ~Information();
    
    // Các hàm lấy thông tin phần cứng cũ...
    std::string getCPUModel();
    std::string getGPUModel();
    std::string getDiskCStatus();
    void getRAMInfo(double &total, double &free);
    std::string getUptime();
    std::string getHostname();
    std::string getIPv4Address();
    std::string getWindowsVersion();
    int getCPUCores();
    double getCPUSpeed();
    std::string getGPUMemory();
    std::string getDeviceType(); 

    // Các hàm hiển thị
    void showDashboard();
    std::string getWindowsLicenseStatus(); 
    std::string getAppLicenses();
    void showNetworkDetails();            
    void showInstalledSoftware();         

    // ==================== HÀM KHÁI QUÁT TOÀN BỘ MỚI ====================
    void showAllInfo(); 
};

#endif // INFORMATION_H