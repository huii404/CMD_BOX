#ifndef INTERNET_H
#define INTERNET_H

#include <winsock2.h>
#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include "SystemCore.h"

class Internet {
private:
    SystemCore &sc;

    std::string getField(const std::string &line);
    std::string getLocalIP();

    // Native Win32 Registry & Service Helpers
    static bool readRegDword(HKEY hRoot, const std::string &subKey, const std::string &valueName, DWORD &outVal);
    static bool writeRegDword(HKEY hRoot, const std::string &subKey, const std::string &valueName, DWORD val);
    static bool isServiceRunningNative(const std::string &serviceName);
    static bool isServiceDisabledNative(const std::string &serviceName);

public:
    Internet(SystemCore &s);
    ~Internet();

    void showNetworkInfo();
    void repairNetwork();
    void wifiAudit();
    void checkSecurityStatus();
    void fullSecurityShield();
    void checkHostsFileSecurity();

    // Các hàm Toggle bảo mật có kiểm tra trạng thái trước & sau
    void toggleDefender(bool enable, int &alreadyCount, int &newlyCount, int &failedCount);
    void toggleFirewall(bool enable, int &alreadyCount, int &newlyCount, int &failedCount);
    void toggleControlledFolderAccess(bool enable, int &alreadyCount, int &newlyCount, int &failedCount);
    void toggleInsecureProtocols(bool block, int &alreadyCount, int &newlyCount, int &failedCount);
    void toggleDangerousPorts(bool block, int &alreadyCount, int &newlyCount, int &failedCount);
    void toggleDNSoverHTTPS(bool enable, int &alreadyCount, int &newlyCount, int &failedCount);
};

#endif 