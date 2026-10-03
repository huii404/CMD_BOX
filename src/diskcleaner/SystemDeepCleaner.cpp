#include "FileSafety.h"
#include "SystemDeepCleaner.h"
#include <iostream>
#include <vector>

namespace {
bool queryServiceRunning(const wchar_t* serviceName, bool& running) {
    running = false;
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;
    SC_HANDLE service = OpenServiceW(scm, serviceName, SERVICE_QUERY_STATUS);
    if (!service) {
        CloseServiceHandle(scm);
        return false;
    }

    SERVICE_STATUS_PROCESS status{};
    DWORD bytesNeeded = 0;
    bool ok = QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
                                   reinterpret_cast<LPBYTE>(&status), sizeof(status),
                                   &bytesNeeded) != FALSE;
    if (ok) {
        running = status.dwCurrentState == SERVICE_RUNNING;
        ok = running || status.dwCurrentState == SERVICE_STOPPED;
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return ok;
}
}

CleanStats SystemDeepCleaner::clean(bool dryRun, bool runDismCleanup) {
    CleanStats stats;

    if (!dryRun && !CleanerCore::isElevated()) {
        std::cout << CleanerCore::C_RED << " [!] LƯU Ý: Module dọn dẹp hệ thống chuyên sâu yêu cầu quyền Administrator!\n" << CleanerCore::C_RESET;
        return stats;
    }

    std::string sysRoot = FileSafety::windowsDirectory();
    std::string progData = FileSafety::knownFolder(FOLDERID_ProgramData);
    if (sysRoot.empty() || progData.empty()) { stats.errorsCount++; return stats; }

    // 1. Dọn dẹp cache tải về của Windows Update
    std::string wuDownloadPath = sysRoot + "\\SoftwareDistribution\\Download";
    
    // Abort cache deletion if service state cannot be established/stopped.
    {
        struct WuServiceGuard {
            CleanStats& stats;
            bool bits = false, update = false;
            ~WuServiceGuard() {
                if (bits && !CleanerCore::runCommand("net start bits >nul 2>&1", true)) stats.errorsCount++;
                if (update && !CleanerCore::runCommand("net start wuauserv >nul 2>&1", true)) stats.errorsCount++;
            }
        } guard{stats};
        bool ready = true;
        if (!dryRun) {
            bool update = false, bits = false;
            ready = queryServiceRunning(L"wuauserv", update) && queryServiceRunning(L"bits", bits);
            if (ready && update) {
                guard.update = true;
                ready = CleanerCore::runCommand("net stop wuauserv >nul 2>&1", true);
            }
            if (ready && bits) {
                guard.bits = true;
                ready = CleanerCore::runCommand("net stop bits >nul 2>&1", true);
            }
            bool stillUpdate = true, stillBits = true;
            ready = ready && queryServiceRunning(L"wuauserv", stillUpdate) &&
                    queryServiceRunning(L"bits", stillBits) && !stillUpdate && !stillBits;
        }
        if (ready) CleanerCore::wipeFolderContents(wuDownloadPath, dryRun, stats);
        else stats.itemsSkipped++;
    } // Restore services before subsequent cleanup and DISM.

    // 2. Dọn Delivery Optimization Cache
    CleanerCore::wipeFolderContents(progData + "\\Microsoft\\Windows\\DeliveryOptimization\\Cache", dryRun, stats);

    // Giữ Windows.old và các thư mục rollback; dọn cache không được phá khả năng khôi phục.

    // 4. Log cài đặt Windows & Kernel Dumps
    std::vector<std::string> logAndDumpFolders = {
        sysRoot + "\\Panther",
        sysRoot + "\\LiveKernelReports",
        sysRoot + "\\Minidump",
        sysRoot + "\\Logs\\CBS",
        sysRoot + "\\Logs\\DISM"
    };

    for (const auto& folder : logAndDumpFolders) {
        CleanerCore::wipeFolderContents(folder, dryRun, stats);
    }

    // Single dump and log files
    std::vector<std::string> singleFiles = {
        sysRoot + "\\MEMORY.DMP",
        sysRoot + "\\WindowsUpdate.log"
    };

    for (const auto& file : singleFiles) {
        CleanerCore::safeDeleteFile(file, dryRun, stats);
    }

    // 5. Chạy DISM Component Store Cleanup an toàn (vẫn cho phép gỡ bản cập nhật).
    if (runDismCleanup && !dryRun) {
        std::cout << CleanerCore::C_CYAN << " [*] Đang thực thi DISM Component Cleanup... Có thể mất vài phút...\n" << CleanerCore::C_RESET;
        if (!CleanerCore::runCommand(
                "dism.exe /online /cleanup-image /startcomponentcleanup", false,
                CleanerCore::DISM_CMD_TIMEOUT_MS)) {
            stats.errorsCount++;
        }
    }

    return stats;
}
