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

    // Native Win32 Registry & Service Helpers (Chỉ giữ hàm thực sự sử dụng)
    static bool readRegDword(HKEY hRoot, const std::string &subKey, const std::string &valueName, DWORD &outVal);
    static bool isServiceRunningNative(const std::string &serviceName);

public:
    Internet(SystemCore &s);
    ~Internet();

    void repairNetwork();
    void wifiAudit();
    void checkSecurityStatus();
    void fullSecurityShield();
    void scanConnectedDevices();
    void localDropMenu();
};

#endif 