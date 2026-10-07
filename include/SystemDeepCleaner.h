#ifndef SYSTEM_DEEP_CLEANER_H
#define SYSTEM_DEEP_CLEANER_H

#include "CleanerCore.h"

// Dọn dẹp hệ thống chuyên sâu: Windows Update cache, DeliveryOptimization, log/dump, DISM component cleanup
class SystemDeepCleaner {
public:
    static CleanStats clean(bool dryRun = false, bool runDismCleanup = false);
};

#endif // SYSTEM_DEEP_CLEANER_H
