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

    // 1. Dọn rác nhanh Plus (User Temp, System Temp, Recent, Thùng rác, DNS, WER Temp - Tốc độ siêu nhanh 1-3s)
    void cleanDiskQuick();

    // 2. Dọn rác ổ đĩa chuyên sâu PRO (User Cache, Temp, System Temp, Prefetch, Log, Thùng rác, WinSxS, DISM...)
    void cleanDiskPro();

    // 2. Quét & tắt ứng dụng tự khởi động cùng Windows qua Registry (có Whitelist an toàn)
    void disableAllStartupApps();

    // 3. Sửa lỗi kẹt cập nhật Windows Update (Dừng service, xóa SoftwareDistribution, reset catroot2)
    void fixWindowsUpdate();

    // 4. Dọn dẹp cache của các trình duyệt web Chromium (Chrome, Edge, Brave...) và Firefox
    void clearBrowserCache();

    // 5. Dọn dẹp cache môi trường lập trình (Node/npm/pnpm/yarn, Python pip, VS Code, NuGet, Gradle, Rust...)
    void cleanDevCaches();

    // 6. Tối ưu hóa hệ thống tổng thể (DISM Component Cleanup, Tweak Registry, tắt Telemetry, tăng tốc tắt máy)
    void optimizeSystemPRO();

    // Hàm phụ trợ: Thay đổi trạng thái khởi động và dừng/chạy Windows Service thông qua Win32 Service Manager API
    bool ServiceControlAPI(std::string serviceName, DWORD startupType, bool stopService);

    // 7. Menu quản lý & tắt/chuyển Manual các dịch vụ Windows không cần thiết (Telemetry, Xbox, Windows Update...)
    void turnOffServicesMenu();

    // 8. Tối ưu thanh Taskbar Windows 11/10 (Tắt Searchbox, Widgets, Chat Teams, Task View, Feeds, Copilot)
    void optimizeTaskbar();  
};

#endif 