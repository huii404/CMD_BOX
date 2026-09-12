#ifndef NETWORK_SCANNER_H
#define NETWORK_SCANNER_H

#include <winsock2.h>
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>
#include "SystemCore.h"

struct DiscoveredDevice {
    uint32_t ipInt;
    std::string ip;
    std::string mac;
    std::string hostname;
    std::string deviceType;
    bool isSelf;
    bool isGateway;
};

class NetworkScanner {
private:
    SystemCore &sc;

    static std::string getLocalIP(SystemCore &core);
    static bool isRandomizedPrivateMac(const std::string& mac);
    static std::pair<std::string, std::string> lookupVendorFromMac(const std::string& mac);
    static std::string queryNetBiosName(const std::string& ipStr);
    static std::string queryLocalDnsPtr(const std::string& ipStr, const std::string& routerIp, int timeoutMs = 50);
    static std::pair<std::string, std::string> classifyDevice(const std::string& ip, const std::string& mac, bool isLocal, bool isGw,
                                                             const std::string& localHost, const std::string& netBios, const std::string& dnsHost);
    static int utf8DisplayLen(const std::string& str);
    static std::string utf8PadRight(const std::string& str, int targetWidth);

public:
    NetworkScanner(SystemCore &core);
    ~NetworkScanner();

    // Menu quét và hiển thị tương tác người dùng
    void scanConnectedDevices();

    // Thuật toán quét mạng lõi trả về danh sách thiết bị (phục vụ nâng cấp & tái sử dụng)
    std::vector<DiscoveredDevice> performScan(double &outElapsedSec);
};

#endif // NETWORK_SCANNER_H
