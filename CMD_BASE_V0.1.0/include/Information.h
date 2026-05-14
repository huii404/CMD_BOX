#ifndef INFORMATION_H
#define INFORMATION_H

#include "SystemCore.h"
#include <string>

class Information {
private:
    SystemCore &sc;

public:
    Information(SystemCore &s);
    ~Information();
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

    void showDashboard();
    std::string getWindowsLicenseStatus(); // Kiểm tra bản quyền Win
    std::string getAppLicenses();// Kiểm tra một số app như Office, Adobe...
  
};

#endif // INFORMATION_H