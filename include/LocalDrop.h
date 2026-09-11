#ifndef LOCALDROP_H
#define LOCALDROP_H

#include <winsock2.h>
#include <windows.h>
#include <string>
#include <vector>
#include <atomic>
#include <memory>
#include "SystemCore.h"

class LocalDrop {
private:
    SystemCore &sc;
    std::atomic<bool> isRunning{false};
    SOCKET serverTcpSocket{INVALID_SOCKET};
    SOCKET beaconUdpSocket{INVALID_SOCKET};

    static std::string detectBestLANIP();
    static void addFirewallRule(int port);
    static void removeFirewallRule();
    static std::string parseDeviceName(const std::string &userAgent, const std::string &clientIP);
    static std::string sanitizeFilename(const std::string &filename);
    static std::string getDownloadsFolder();

public:
    LocalDrop(SystemCore &core);
    ~LocalDrop();

    void menu();
    void startPublicDrop(const std::string &defaultFile = "");
    void startSecureSender(const std::string &defaultFile = "");
    void startSecureReceiver();
    void startSender(const std::string &defaultFile = "", bool isSecure = false);
    void startReceiver();
};

#endif // LOCALDROP_H
