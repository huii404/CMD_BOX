#ifndef SYSTEMOPTIMIZER_H
#define SYSTEMOPTIMIZER_H

#include "SystemCore.h"
#include "DiskCleaner.h"
#include <string>

// Tối ưu hệ thống: Registry, dịch vụ Windows, ứng dụng khởi động và giao diện
class SystemOptimizer {
private:
    SystemCore &sc;
public:
    SystemOptimizer(SystemCore &s);

    // Dọn rác hệ thống (ủy quyền sang DiskCleaner)
    void runClean();
    void runCleanChoice(int choice = 0);

    // Tối ưu hiệu năng
    int optimizeStartupApps();
    int optimizeBackgroundServices();
    bool optimizeVisualEffectsAndUI();

    // Alias tương thích ngược
    inline int runOptimizeTier1() { return optimizeStartupApps(); }
    inline int runOptimizeTier2() { return optimizeBackgroundServices(); }
    inline bool runOptimizeTier3() { return optimizeVisualEffectsAndUI(); }
    void runOptimizeChoice(int choice);
    void multiTierPerformanceOptimize();

    // 3. Sửa lỗi kẹt cập nhật Windows Update
    void fixWindowsUpdate();

    // 4. Quản lý 40 dịch vụ theo từng mục (Manual/Disabled)
    bool ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService);
    void turnOffServicesMenu();
};

#endif 