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

    bool text_processing(const std::string &text);
    void ShowQR();
    void ShowN_QR();
    void uninstallBloatware();
    void downloadManager();
    void processDownload(const AppInfo& app);
    void showToolsMenu();
    void showDownloadHistory();
};

#endif 