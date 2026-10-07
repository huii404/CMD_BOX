#ifndef DOWNLOADS_CLEANER_H
#define DOWNLOADS_CLEANER_H

#include "CleanerCore.h"
#include <unordered_set>
#include <string>

// Dọn dẹp an toàn thư mục Downloads:
// - Xóa file tải dở (.crdownload, .part, .tmp)
// - Đưa vào Thùng rác: file cài đặt (.exe, .msi) đã cài và bản sao trùng lặp
class DownloadsCleaner {
private:
    static std::string getDownloadsPath();
    static std::unordered_set<std::string> getInstalledAppNames();
    static std::string getExeProductName(const std::string& exePath);
    static std::string cleanAppName(const std::string& raw);

public:
    static bool isSupportedExecutable(const std::string& ext);
    static bool isSupportedImage(const std::string& ext);
    static bool isSupportedVideo(const std::string& ext);
    static bool isCorruptDownload(const std::string& ext);

    static CleanStats clean(bool dryRun = false);
};

#endif // DOWNLOADS_CLEANER_H
