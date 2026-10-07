#ifndef BROWSER_CLEANER_H
#define BROWSER_CLEANER_H

#include "CleanerCore.h"

// Dọn dẹp cache trình duyệt (Chromium, Firefox) và cache/media tạm của Zalo PC
class BrowserCleaner {
public:
    static CleanStats clean(bool dryRun = false);
};

#endif // BROWSER_CLEANER_H
