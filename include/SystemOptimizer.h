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

    // --- HỆ THỐNG ĐIỀU PHỐI DỌN RÁC & TỐI ƯU ---
    // 1. Quản lý Dọn rác Hệ thống
    void multiTierDiskClean();

    // 2. Quản lý Tăng tốc & Tối ưu Hệ thống
    void multiTierPerformanceOptimize();

    // --- CÁC HÀM DỌN RÁC THEO NHIỆM VỤ (TASK-BASED) ---
    // Dọn rác tạm bề mặt & cache người dùng (Temp, CrashDumps, WER User, INetCache, RecycleBin, Flush DNS)
    long long cleanSurfaceAndUserTemp();

    // Dọn bộ đệm trình duyệt & ứng dụng giao tiếp (Chrome, Edge, Firefox, Discord, Telegram...)
    long long cleanBrowserAndAppCache();

    // Dọn dẹp chuyên sâu hệ thống & tồn dư cập nhật ($WINDOWS.~BT/WS, Windows.old, DISM, Logs, LiveKernelReports)
    long long cleanDeepSystemAndUpdates();

    // Dọn dẹp rác môi trường lập trình & artifacts dự án (node_modules, pip, gradle, VS Code, v.v.)
    long long cleanDevArtifactsAndCaches();

    // Alias tương thích ngược
    inline long long runCleanTier1() { return cleanSurfaceAndUserTemp(); }
    inline long long runCleanTier2() { return cleanBrowserAndAppCache(); }
    inline long long runCleanTier3() { return cleanDeepSystemAndUpdates(); }
    inline long long runCleanTier4() { return cleanDevArtifactsAndCaches(); }

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

    // 3. Sửa lỗi kẹt cập nhật Windows Update
    void fixWindowsUpdate();

    // 4. Các tiện ích phụ trợ & Quản lý dịch vụ
    void clearBrowserCache();
    void cleanDevCaches(bool interactive = false);
    bool ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService);
    void turnOffServicesMenu();
};

#endif 