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
    std::string sharePath;
    std::string shareName;
    long long shareSize;
    int dlCount;
    
    const long long MAX_FILE_SIZE = 1500000000LL; 
    std::vector<std::string> chatHistory;

    std::string getField(const std::string &line);
    std::string getContentType(const std::string &fpath);
    std::string getLocalIP();
    void openFW();
    std::string formatSize(long long b);
    std::string getTime();
    HTTPRequest parseReq(const std::string &raw);
    void sendFile(SOCKET client);
    void handleClient(SOCKET client);     
    void handleChatClient(SOCKET client); 
    bool checkFileSizeAndConfirm(const std::string &path, long long &outSize);
    bool getFileSizeInfoAndPrompt(const std::string &path, long long &outSize);

public:
    Internet(SystemCore &s);
    ~Internet();

    void showIP();
    void renewIP();
    void wifiAudit();
    void flushdns();
    void netsh_tcpIP();
    void quickSharePRO();
    void startLocalChat();
    void enableWindowsDefender();
    void enableFirewall();
    void enableControlledFolderAccess();
    void disableInsecureProtocols();
    void blockDangerousPorts();
    void configureDNSoverHTTPS();
    void checkSecurityStatus();
};

#endif 