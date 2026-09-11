#ifndef SYSTEMOPTIMIZER_H
#define SYSTEMOPTIMIZER_H

#include "SystemCore.h"
#include <string>

/**
 * @brief Module Tối Ưu Hệ Thống (System Optimizer)
 * Quản lý các tính năng dọn dẹp bộ nhớ/ổ đĩa, tinh chỉnh Registry, 
 * quản lý Service, vô hiệu hóa ứng dụng khởi động và tối ưu giao diện Windows.
 */
class SystemOptimizer {
private:
    SystemCore &sc;
public:
    SystemOptimizer(SystemCore &s);

    // --- HỆ THỐNG ĐA TẦNG MỚI ---
    // 1. Hệ thống Dọn rác Đa Tầng (Tầng 1 -> Tầng 4)
    void multiTierDiskClean();

    // 2. Hệ thống Tăng tốc & Tối ưu Đa Tầng (Boot -> Services -> UI/Taskbar)
    void multiTierPerformanceOptimize();

    // Các hàm phụ trợ thực thi từng tầng
    long long runCleanTier1();
    long long runCleanTier2();
    long long runCleanTier3();
    long long runCleanTier4();

    int runOptimizeTier1();
    int runOptimizeTier2();
    bool runOptimizeTier3();

    // 3. Sửa lỗi kẹt cập nhật Windows Update
    void fixWindowsUpdate();

    // 4. Các tiện ích phụ trợ & Quản lý dịch vụ
    void clearBrowserCache();
    void cleanDevCaches(bool interactive = false);
    bool ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService);
    void turnOffServicesMenu();
};

#endif 