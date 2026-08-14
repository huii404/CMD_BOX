#ifndef SYSTEMOPTIMIZER_H
#define SYSTEMOPTIMIZER_H

#include "SystemCore.h"
#include "Internet.h"
#include <string>

class SystemOptimizer {
private:
    SystemCore &sc;
    Internet &n;
public:
    SystemOptimizer(SystemCore &s, Internet &net);

    void cleanDiskPro();
    void disableAllStartupApps();
    void fixWindowsUpdate();
    void clearBrowserCache();
    void optimizeSystemPRO();
    bool ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService);
    void turnOffServicesMenu();
    void optimizeTaskbar();  
};

#endif 