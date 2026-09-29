#ifndef SYSTEMOPTIMIZER_H
#define SYSTEMOPTIMIZER_H

#include "SystemCore.h"
#include "DiskCleaner.h"
#include <string>

/**
 * @brief Module Tối Ưu Hệ Thống (System Optimizer)
 * Quản lý các tính năng tinh chỉnh Registry, quản lý Service, 
 * vô hiệu hóa ứng dụng khởi động và tối ưu giao diện Windows.
 * Các tác vụ dọn rác được ủy quyền sang hệ thống DiskCleaner mới (static API).
 */
class SystemOptimizer {
private:
    SystemCore &sc;
public:
    SystemOptimizer(SystemCore &s);

    // --- ỦY QUYỀN SANG DISKCLEANER (API MỚI - STATIC) ---
    void runCleanChoice(int choice);

    // --- CÁC HÀM TỐI ƯU HIỆU NĂNG THEO NHIỆM VỤ (TASK-BASED) ---
    // Tối ưu ứng dụng khởi động (Tắt app bên thứ ba làm chậm máy, bảo vệ 100% Bộ gõ & Driver)
    int optimizeStartupApps();

    // Tối ưu dịch vụ chạy ngầm vô ích (Maps, Wallet, Telemetry, ErrorReporting...)
    int optimizeBackgroundServices();

    // Tối ưu giao diện & Taskbar (Tắt widget, thu gọn taskbar, tối ưu độ nhạy UI)
    bool optimizeVisualEffectsAndUI();

    // Alias tương thích ngược
    inline int runOptimizeTier1() { return optimizeStartupApps(); }
    inline int runOptimizeTier2() { return optimizeBackgroundServices(); }
    inline bool runOptimizeTier3() { return optimizeVisualEffectsAndUI(); }
    void runOptimizeChoice(int choice);
    void multiTierPerformanceOptimize();

    // 3. Sửa lỗi kẹt cập nhật Windows Update
    void fixWindowsUpdate();

    // 4. Các tiện ích phụ trợ & Quản lý dịch vụ
    bool ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService);
    void turnOffServicesMenu();
};

#endif 