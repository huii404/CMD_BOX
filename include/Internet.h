#ifndef INTERNET_H
#define INTERNET_H

#include <winsock2.h>
#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include "SystemCore.h"

struct HTTPRequest {
    std::string method, path, httpVersion;
    std::map<std::string, std::string> headers;
};

class Internet {
private:
    SystemCore &sc;
    SOCKET listenSocket;
    int httpPort;
    std::vector<std::string> chatHistory;

    std::string getField(const std::string &line);
    std::string getContentType(const std::string &fpath);
    std::string getLocalIP();
    HTTPRequest parseReq(const std::string &raw);
    void handleChatClient(SOCKET client); 

public:
    Internet(SystemCore &s);
    ~Internet();

    void showNetworkInfo();
    void repairNetwork();
    void wifiAudit();
    void startLocalChat();
    void enableWindowsDefender();
    void enableFirewall();
    void enableControlledFolderAccess();
    void disableInsecureProtocols();
    void blockDangerousPorts();
    void configureDNSoverHTTPS();
    void checkSecurityStatus();
    void fullSecurityShield();
};

#endif 