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

    // --- Chức năng của Maintenance cũ ---
    void cleanDiskPro();
    void cleanDiskBase();
    void QuickScanVirus();
    void FullScanVirus();
    void restart();
    void Consumer_Content();
    void Hibernate();
    void windowsTelemetry();
    void reduceShutdownTime();
    void disableAllStartupApps();
    void updateAllApps();
    void fixWindowsUpdate();

    // --- Chức năng của Explorer cũ ---
    void clearBrowserCache();

    // --- Chức năng của Optimal cũ ---
    void optimizeSystemPRO();
    void enableSecurityPRO();
    void optimizeNetworkPRO();
    std::string getCurrentOS();
    void upgradeWindowsEditionPRO();
};

#endif