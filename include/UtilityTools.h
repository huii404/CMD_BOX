#ifndef UTILITYTOOLS_H
#define UTILITYTOOLS_H

#include "SystemCore.h"
#include <string>

struct AppInfo {
    std::string name;
    std::string url;
    std::string fileName;
};

class UtilityTools {
private:
    SystemCore &sc;
public:
    UtilityTools(SystemCore &s);

    void autoClickPoint();
    void spamText();
    void autoPasteData();
    void downloadManager();
    void processDownload(const AppInfo& app);
    void showDownloadHistory();
    void uninstallBloatware();
};

#endif 