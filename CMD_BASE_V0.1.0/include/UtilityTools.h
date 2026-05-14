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

    // --- AutoActions ---
    void autoClickPoint();
    void spamText();
    void autoPasteData();

    // --- Extension ---
    bool text_processing(const std::string &text);
    void ShowQR(std::string text);
    void ShowN_QR(int number);
    void uninstallBloatware();
    void downloadManager();
    void processDownload(const AppInfo& app);
};

#endif