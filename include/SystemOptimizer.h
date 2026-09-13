#ifndef SYSTEMOPTIMIZER_H
#define SYSTEMOPTIMIZER_H

#include "SystemCore.h"
#include "DiskCleaner.h"
#include <string>

/**
 * @brief Module Tối Ưu Hệ Thống (System Optimizer)
 * Quản lý các tính năng tinh chỉnh Registry, quản lý Service, 
 * vô hiệu hóa ứng dụng khởi động và tối ưu giao diện Windows.
 * Các tác vụ dọn rác được ủy quyền sang lớp chuyên trách DiskCleaner.
 */
class SystemOptimizer {
private:
    SystemCore &sc;
    DiskCleaner cleaner;
public:
    SystemOptimizer(SystemCore &s);

    // --- ỦY QUYỀN SANG DISKCLEANER (BẢO TOÀN 100% TƯƠNG THÍCH NGƯỢC) ---
    inline void runCleanChoice(int choice) { cleaner.runCleanChoice(choice); }
    inline long long cleanSurfaceAndUserTemp() { return cleaner.cleanSurfaceAndUserTemp(); }
    inline long long cleanBrowserAndAppCache() { return cleaner.cleanBrowserAndAppCache(); }
    inline long long cleanDeepSystemAndUpdates() { return cleaner.cleanDeepSystemAndUpdates(); }
    inline long long cleanDevArtifactsAndCaches() { return cleaner.cleanDevArtifactsAndCaches(); }
    inline long long cleanDownloadsExesAndDuplicates() { return cleaner.cleanDownloadsExesAndDuplicates(); }
    inline void cleanDevCaches(bool interactive = false) { cleaner.cleanDevCaches(interactive); }
    inline void clearBrowserCache() { cleaner.clearBrowserCache(); }

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
    void runOptimizeChoice(int choice);
    void multiTierOptimize();

    // 3. Sửa lỗi kẹt cập nhật Windows Update
    void fixWindowsUpdate();

    // 4. Các tiện ích phụ trợ & Quản lý dịch vụ
    bool ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService);
    void turnOffServicesMenu();
};

#endif 