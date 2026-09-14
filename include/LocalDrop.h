#ifndef LOCALDROP_H
#define LOCALDROP_H

#include <winsock2.h>
#include <windows.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <atomic>
#include <memory>
#include "SystemCore.h"

class LocalDrop {
private:
    SystemCore &sc;
    std::atomic<bool> isRunning{false};
    SOCKET serverTcpSocket{INVALID_SOCKET};
    SOCKET beaconUdpSocket{INVALID_SOCKET};
    std::string firewallRuleName;

    static std::string detectBestLANIP();
    static std::unordered_set<std::string> getAllLocalIPs();
    static bool addFirewallRule(int port, const std::string &ruleName);
    static bool removeFirewallRule(const std::string &ruleName);
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
