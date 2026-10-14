#ifndef DISK_CLEANER_H
#define DISK_CLEANER_H

#include "CleanerCore.h"
#include "TempCleaner.h"
#include "BrowserCleaner.h"
#include "SystemDeepCleaner.h"
#include "DevCleaner.h"
#include "DownloadsCleaner.h"

class DiskCleaner {
public:
    static void printBanner();


    // Luồng một nút: tự kiểm tra, dọn từng tiêu chí và cập nhật dashboard.
    static void runAutomaticCleanup(bool waitAtEnd = true);


};

#endif // DISK_CLEANER_H
