#ifndef TEMP_CLEANER_H
#define TEMP_CLEANER_H

#include "CleanerCore.h"

// Dọn dẹp file tạm (%TEMP%, Windows\Temp, Prefetch), cache người dùng (D3DS, WER, thumbnails) và flush DNS
class TempCleaner {
public:
    static CleanStats clean(bool dryRun = false);
};

#endif // TEMP_CLEANER_H
