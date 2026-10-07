#include "FileSafety.h"
#include "TempCleaner.h"
#include "SystemCore.h"
#include <iostream>
#include <vector>

CleanStats TempCleaner::clean(bool dryRun) {
    CleanStats stats;

    std::string localApp = FileSafety::knownFolder(FOLDERID_LocalAppData);
    std::string tempDir = localApp.empty() ? "" : (fs::u8path(localApp) / "Temp").u8string();
    std::string appData = FileSafety::knownFolder(FOLDERID_RoamingAppData);
    std::string sysRoot = FileSafety::windowsDirectory();
    std::string progData = FileSafety::knownFolder(FOLDERID_ProgramData);
    if (sysRoot.empty() || progData.empty()) { stats.errorsCount++; return stats; }

    // 1. Thư mục Temp người dùng
    if (!tempDir.empty()) {
        CleanerCore::wipeFolderContents(tempDir, dryRun, stats);
    }

    // 2. Thư mục Temp hệ thống & Prefetch (Nếu có quyền Admin)
    if (SystemCore::isElevated()) {
        CleanerCore::wipeFolderContents(sysRoot + "\\Temp", dryRun, stats);
        CleanerCore::wipeFolderContents(sysRoot + "\\Prefetch", dryRun, stats);
    }

    // 3. Recent Items & Quick Access Cache
    if (!appData.empty()) {
        CleanerCore::wipeFolderContents(appData + "\\Microsoft\\Windows\\Recent", dryRun, stats);
    }

    // 4. Caches đồ họa & Bảo mật Windows
    if (!localApp.empty()) {
        CleanerCore::wipeFolderContents(localApp + "\\D3DSCache", dryRun, stats);
        CleanerCore::wipeFolderContents(localApp + "\\Low\\Microsoft\\CryptnetUrlCache", dryRun, stats);
        CleanerCore::wipeFolderContents(localApp + "\\CrashDumps", dryRun, stats);
        CleanerCore::wipeFolderContents(localApp + "\\Microsoft\\Windows\\WER\\Temp", dryRun, stats);
        CleanerCore::wipeFolderContents(localApp + "\\Microsoft\\Windows\\WER\\ReportArchive", dryRun, stats);
        CleanerCore::wipeFolderContents(localApp + "\\Microsoft\\Windows\\WER\\ReportQueue", dryRun, stats);
        CleanerCore::wipeFolderContents(localApp + "\\Microsoft\\Windows\\INetCache", dryRun, stats);

        // Thumbnail cache
        std::error_code ec;
        fs::path explorerDir = fs::path(localApp) / "Microsoft" / "Windows" / "Explorer";
        if (fs::exists(explorerDir, ec)) {
            for (const auto& entry : fs::directory_iterator(explorerDir, fs::directory_options::skip_permission_denied, ec)) {
                if (ec) { ec.clear(); continue; }
                if (entry.is_regular_file(ec)) {
                    std::string fn = entry.path().filename().string();
                    if (fn.rfind("thumbcache_", 0) == 0 && fn.size() >= 3 && fn.substr(fn.size() - 3) == ".db") {
                        CleanerCore::safeDeleteFile(entry.path(), dryRun, stats);
                    }
                }
            }
        }
    }

    // 5. ProgramData WER Temp
    if (SystemCore::isElevated() && !progData.empty()) {
        CleanerCore::wipeFolderContents(progData + "\\Microsoft\\Windows\\WER\\Temp", dryRun, stats);
    }

    // 6. Xóa cache phân giải tên miền DNS
    if (!dryRun) {
        CleanerCore::flushDns();
    }

    return stats;
}
