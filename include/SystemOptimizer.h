#ifndef SYSTEMOPTIMIZER_H
#define SYSTEMOPTIMIZER_H

#include "SystemCore.h"
#include "Internet.h"
#include <string>

/**
 * @brief Module Tối Ưu Hệ Thống (System Optimizer)
 * Quản lý các tính năng dọn dẹp bộ nhớ/ổ đĩa, tinh chỉnh Registry, 
 * quản lý Service, vô hiệu hóa ứng dụng khởi động và tối ưu giao diện Windows.
 */
class SystemOptimizer {
private:
    SystemCore &sc;
    Internet &n;
public:
    SystemOptimizer(SystemCore &s, Internet &net);

    // --- HỆ THỐNG ĐA TẦNG MỚI ---
    // 1. Hệ thống Dọn rác Đa Tầng (Tầng 1 -> Tầng 4)
    void multiTierDiskClean();

    // 2. Hệ thống Tăng tốc & Tối ưu Đa Tầng (Boot -> Services -> UI/Taskbar)
    void multiTierPerformanceOptimize();

    // Các hàm phụ trợ thực thi từng tầng
    long long runCleanTier1();
    long long runCleanTier2();
    long long runCleanTier3();
    long long runCleanTier4(const std::string &customPath = "");

    int runOptimizeTier1();
    int runOptimizeTier2();
    bool runOptimizeTier3();

    // 3. Sửa lỗi kẹt cập nhật Windows Update
    void fixWindowsUpdate();

    // --- CÁC HÀM RIÊNG LẺ / TƯƠNG THÍCH ---
    void cleanDiskQuick();
    void cleanDiskPro();
    void disableAllStartupApps();
    void clearBrowserCache();
    void cleanDevCaches(bool interactive = false);
    void optimizeSystemPRO();
    bool ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService);
    void turnOffServicesMenu();
    void optimizeTaskbar();
};

#endif 