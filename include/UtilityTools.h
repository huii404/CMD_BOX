#ifndef UTILITYTOOLS_H
#define UTILITYTOOLS_H

#include "SystemCore.h"
#include <string>

class UtilityTools {
private:
    SystemCore &sc;
public:
    UtilityTools(SystemCore &s);

    void autoClickPoint();
    void spamText();
    void autoPasteData();
    void downloadManager();
    void uninstallBloatware();
};

#endif 