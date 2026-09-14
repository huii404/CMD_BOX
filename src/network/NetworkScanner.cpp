#include "NetworkScanner.h"
#include "OuiDatabase.h"
#include <iphlpapi.h>
#include <icmpapi.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <omp.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Cache IP nội bộ
static string cachedScannerIP = "";
static time_t lastScannerIPCheck = 0;
static const int IP_SCANNER_CACHE_TTL = 60;

NetworkScanner::NetworkScanner(SystemCore &core) : sc(core) {
}

NetworkScanner::~NetworkScanner() {
}

static bool isPrivateIPv4(const string& ip) {
    if (ip.rfind("192.168.", 0) == 0 || ip.rfind("10.", 0) == 0) return true;
    if (ip.rfind("172.", 0) == 0) {
        int o1 = 0, o2 = 0;
        if (sscanf(ip.c_str(), "%d.%d", &o1, &o2) == 2) {
            if (o1 == 172 && o2 >= 16 && o2 <= 31) return true;
        }
    }
    return false;
}

string NetworkScanner::getLocalIP(SystemCore &core) {
    time_t now = time(nullptr);
    
    if (!cachedScannerIP.empty() && (now - lastScannerIPCheck) < IP_SCANNER_CACHE_TTL) {
        return cachedScannerIP;
    }
    
    vector<BYTE> adapterInfo(sizeof(IP_ADAPTER_INFO));
    DWORD dwSize = static_cast<DWORD>(adapterInfo.size());
    DWORD dwRetVal = GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(adapterInfo.data()), &dwSize);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        adapterInfo.resize(dwSize);
        dwRetVal = GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(adapterInfo.data()), &dwSize);
    }
    
    if (dwRetVal == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapter = reinterpret_cast<PIP_ADAPTER_INFO>(adapterInfo.data());
        while (pAdapter) {
            string ip = pAdapter->IpAddressList.IpAddress.String;
            if (isPrivateIPv4(ip)) {
                if (ip != "0.0.0.0" && ip != "127.0.0.1") {
                    cachedScannerIP = ip;
                    lastScannerIPCheck = now;
                    return cachedScannerIP;
                }
            }
            pAdapter = pAdapter->Next;
        }
    }
    
    // Fallback qua UDP socket routing (cực nhanh < 1ms, không spawn process ngoài)
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s != INVALID_SOCKET) {
        sockaddr_in target;
        memset(&target, 0, sizeof(target));
        target.sin_family = AF_INET;
        target.sin_addr.s_addr = inet_addr("8.8.8.8");
        target.sin_port = htons(80);
        if (connect(s, (sockaddr*)&target, sizeof(target)) == 0) {
            sockaddr_in local;
            int len = sizeof(local);
            if (getsockname(s, (sockaddr*)&local, &len) == 0) {
                char ipStr[INET_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET, &local.sin_addr, ipStr, sizeof(ipStr));
                string res = ipStr;
                if (!res.empty() && res != "0.0.0.0" && res != "127.0.0.1") {
                    closesocket(s);
                    cachedScannerIP = res;
                    lastScannerIPCheck = now;
                    return cachedScannerIP;
                }
            }
        }
        closesocket(s);
    }
    
    return "127.0.0.1";
}

bool NetworkScanner::isRandomizedPrivateMac(const string& mac) {
    if (mac.length() < 2) return false;
    char secondHex = (char)toupper((unsigned char)mac[1]);
    return (secondHex == '2' || secondHex == '6' || secondHex == 'A' || secondHex == 'E');
}

pair<string, string> NetworkScanner::lookupVendorFromMac(const string& mac) {
    if (mac.length() < 6) return {"Thiết bị không định danh", "Thiết bị mạng"};

    string clean = "";
    for (char c : mac) {
        if (c != ':' && c != '-') clean += (char)toupper((unsigned char)c);
    }
    if (clean.length() < 6) return {"Thiết bị không định danh", "Thiết bị mạng"};
    string oui = clean.substr(0, 6);

    const auto& ouiMap = getOuiDatabase();
    auto it = ouiMap.find(oui);
    if (it != ouiMap.end()) {
        return it->second;
    }

    if (isRandomizedPrivateMac(mac)) {
        return {"Ẩn danh (Bảo mật MAC)", "Điện thoại / Tablet (iOS / Android)"};
    }

    return {"Thiết bị mạng (OEM)", "Thiết bị kết nối Wi-Fi"};
}

// 1. Truy vấn tên máy Windows qua NetBIOS Name Service (UDP 137, timeout siêu tốc 50ms)
string NetworkScanner::queryNetBiosName(const string& ipStr) {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return "";

    DWORD timeout = 50; // 50ms timeout cục bộ trong LAN
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(137);
    dest.sin_addr.s_addr = inet_addr(ipStr.c_str());

    static const unsigned char nbtQuery[] = {
        0x80, 0x90, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x20, 0x43, 0x4B, 0x41,
        0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41, 0x41, 0x00, 0x00, 0x21,
        0x00, 0x01
    };

    sendto(s, (const char*)nbtQuery, sizeof(nbtQuery), 0, (sockaddr*)&dest, sizeof(dest));

    char buf[1024];
    int recvLen = recv(s, buf, sizeof(buf), 0);
    closesocket(s);

    if (recvLen > 56) {
        int numNames = (unsigned char)buf[56];
        int offset = 57;
        for (int i = 0; i < numNames && offset + 18 <= recvLen; i++) {
            unsigned char nameType = (unsigned char)buf[offset + 15];
            unsigned short flags = ntohs(*(unsigned short*)(buf + offset + 16));
            bool isGroup = (flags & 0x8000) != 0;
            if (!isGroup && nameType == 0x00) {
                char name[16] = {0};
                memcpy(name, buf + offset, 15);
                string res = name;
                while (!res.empty() && (res.back() == ' ' || res.back() == '\0')) res.pop_back();
                if (!res.empty() && res.find("__MSBROWSE__") == string::npos) {
                    return res;
                }
            }
            offset += 18;
        }
    }
    return "";
}

// 2. Truy vấn tên thiết bị qua DNS PTR nội bộ tới Router (UDP 53, timeout 50ms)
string NetworkScanner::queryLocalDnsPtr(const string& ipStr, const string& routerIp, int timeoutMs) {
    if (routerIp.empty()) return "";

    int o1, o2, o3, o4;
    if (sscanf(ipStr.c_str(), "%d.%d.%d.%d", &o1, &o2, &o3, &o4) != 4) return "";

    string qname = to_string(o4) + "." + to_string(o3) + "." + to_string(o2) + "." + to_string(o1) + ".in-addr.arpa";

    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return "";

    DWORD timeout = timeoutMs;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    dest.sin_addr.s_addr = inet_addr(routerIp.c_str());

    unsigned char packet[512];
    memset(packet, 0, sizeof(packet));

    packet[0] = 0x24; packet[1] = 0x68; // Transaction ID
    packet[2] = 0x01; packet[3] = 0x00; // Standard query
    packet[4] = 0x00; packet[5] = 0x01; // QDCOUNT = 1

    int idx = 12;
    size_t start = 0;
    while (start < qname.length()) {
        size_t dot = qname.find('.', start);
        if (dot == string::npos) dot = qname.length();
        int labelLen = (int)(dot - start);
        packet[idx++] = (unsigned char)labelLen;
        for (int i = 0; i < labelLen; ++i) packet[idx++] = qname[start + i];
        start = dot + 1;
    }
    packet[idx++] = 0x00; // End of QNAME

    // QTYPE = PTR (0x000C), QCLASS = IN (0x0001)
    packet[idx++] = 0x00; packet[idx++] = 0x0C;
    packet[idx++] = 0x00; packet[idx++] = 0x01;

    sendto(s, (const char*)packet, idx, 0, (sockaddr*)&dest, sizeof(dest));

    char recvBuf[512];
    int recvLen = recv(s, recvBuf, sizeof(recvBuf), 0);
    closesocket(s);

    if (recvLen <= idx) return "";

    // Xác thực Transaction ID (0x24 0x68)
    if ((unsigned char)recvBuf[0] != 0x24 || (unsigned char)recvBuf[1] != 0x68) return "";

    unsigned short flags = ntohs(*(unsigned short*)(recvBuf + 2));
    if ((flags & 0x8000) == 0 || (flags & 0x000F) != 0) return "";

    unsigned short anCount = ntohs(*(unsigned short*)(recvBuf + 6));
    if (anCount == 0) return "";

    int ansIdx = idx;
    if (ansIdx >= recvLen) return "";

    if ((recvBuf[ansIdx] & 0xC0) == 0xC0) {
        ansIdx += 2;
    } else {
        while (ansIdx < recvLen && recvBuf[ansIdx] != 0) ansIdx += ((unsigned char)recvBuf[ansIdx]) + 1;
        ansIdx++;
    }

    if (ansIdx + 10 > recvLen) return "";
    unsigned short aType = ntohs(*(unsigned short*)(recvBuf + ansIdx));
    ansIdx += 8;
    unsigned short dataLen = ntohs(*(unsigned short*)(recvBuf + ansIdx));
    ansIdx += 2;

    if (aType != 12 || ansIdx + dataLen > recvLen) return "";

    string hostname = "";
    int rIdx = ansIdx;
    int hops = 0;
    while (rIdx < recvLen && hops < 10) {
        unsigned char len = (unsigned char)recvBuf[rIdx++];
        if (len == 0) break;
        if ((len & 0xC0) == 0xC0) {
            if (rIdx >= recvLen) break;
            int ptrOffset = ((len & 0x3F) << 8) | (unsigned char)recvBuf[rIdx++];
            if (ptrOffset < recvLen) {
                rIdx = ptrOffset;
                hops++;
                continue;
            } else {
                break;
            }
        }
        if (!hostname.empty()) hostname += ".";
        for (int i = 0; i < len && rIdx < recvLen; ++i) {
            hostname += recvBuf[rIdx++];
        }
    }

    size_t firstDot = hostname.find('.');
    if (firstDot != string::npos) {
        string tld = hostname.substr(firstDot);
        if (tld == ".lan" || tld == ".local" || tld == ".home" || tld == ".station") {
            hostname = hostname.substr(0, firstDot);
        }
    }

    return hostname;
}

pair<string, string> NetworkScanner::classifyDevice(const string& ip, const string& mac, bool isLocal, bool isGw,
                                                   const string& localHost, const string& netBios, const string& dnsHost) {
    if (isLocal) {
        return {localHost, "Máy tính này (This PC)"};
    }

    if (isGw) {
        auto gwVendor = lookupVendorFromMac(mac);
        string gwName = !dnsHost.empty() ? dnsHost : "Router Wi-Fi (" + gwVendor.first + ")";
        return {gwName, "Router / Modem Wi-Fi"};
    }

    if (!netBios.empty()) {
        return {netBios, "Máy tính Windows (LAN)"};
    }

    if (!dnsHost.empty()) {
        string lower = dnsHost;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("iphone") != string::npos || lower.find("ipad") != string::npos) {
            return {dnsHost, "Điện thoại / iPad (Apple)"};
        }
        if (lower.find("galaxy") != string::npos || lower.find("samsung") != string::npos) {
            return {dnsHost, "Điện thoại Samsung Galaxy"};
        }
        if (lower.find("redmi") != string::npos || lower.find("xiaomi") != string::npos) {
            return {dnsHost, "Điện thoại Xiaomi / Redmi"};
        }
        if (lower.find("desktop") != string::npos || lower.find("laptop") != string::npos) {
            return {dnsHost, "Máy tính (PC / Laptop)"};
        }
        if (lower.find("tapo") != string::npos || lower.find("cam") != string::npos) {
            return {dnsHost, "Camera IP an ninh"};
        }
        return {dnsHost, "Thiết bị mạng (Đã định danh)"};
    }

    if (isRandomizedPrivateMac(mac)) {
        return {"Ẩn danh (Bảo mật MAC)", "Điện thoại / Tablet (iOS/Android)"};
    }

    return lookupVendorFromMac(mac);
}

int NetworkScanner::utf8DisplayLen(const string& str) {
    int len = 0;
    for (size_t i = 0; i < str.length(); ++i) {
        unsigned char c = (unsigned char)str[i];
        if ((c & 0xC0) != 0x80) {
            len++;
        }
    }
    return len;
}

string NetworkScanner::utf8PadRight(const string& str, int targetWidth) {
    int curWidth = utf8DisplayLen(str);
    if (curWidth > targetWidth) {
        int needed = targetWidth - 3;
        if (needed < 1) needed = 1;
        string res = "";
        int w = 0;
        for (size_t i = 0; i < str.length(); ++i) {
            unsigned char c = (unsigned char)str[i];
            if ((c & 0xC0) != 0x80) {
                if (w >= needed) break;
                w++;
            }
            res += str[i];
        }
        res += "...";
        int pad = targetWidth - utf8DisplayLen(res);
        if (pad > 0) res.append(pad, ' ');
        return res;
    }
    if (curWidth < targetWidth) {
        return str + string(targetWidth - curWidth, ' ');
    }
    return str;
}

vector<DiscoveredDevice> NetworkScanner::performScan(double &outElapsedSec) {
    // 1. Lấy thông tin Card mạng chính & dải Subnet
    string myIP = "";
    string myMask = "255.255.255.0";
    string gatewayIP = "";
    string myMAC = "";

    ULONG outBufLen = sizeof(IP_ADAPTER_INFO);
    vector<BYTE> adapterBuffer(outBufLen);
    DWORD adapterResult = GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(adapterBuffer.data()), &outBufLen);
    if (adapterResult == ERROR_BUFFER_OVERFLOW) {
        adapterBuffer.resize(outBufLen);
        adapterResult = GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(adapterBuffer.data()), &outBufLen);
    }

    DWORD preferredIndex = 0;
    GetBestInterface(inet_addr("8.8.8.8"), &preferredIndex);
    if (adapterResult == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = reinterpret_cast<PIP_ADAPTER_INFO>(adapterBuffer.data());
        while (pAdapter) {
            string ip = pAdapter->IpAddressList.IpAddress.String;
            string gw = pAdapter->GatewayList.IpAddress.String;
            if (!ip.empty() && ip != "0.0.0.0" && ip != "127.0.0.1" && !gw.empty() && gw != "0.0.0.0") {
                myIP = ip;
                myMask = pAdapter->IpAddressList.IpMask.String;
                gatewayIP = gw;
                char macBuf[32];
                sprintf(macBuf, "%02X:%02X:%02X:%02X:%02X:%02X",
                        pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2],
                        pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);
                myMAC = macBuf;
                if (pAdapter->Index == preferredIndex) break;
            }
            pAdapter = pAdapter->Next;
        }
    }

    if (myIP.empty()) {
        myIP = getLocalIP(sc);
    }

    if (myIP == "127.0.0.1" || myIP.empty()) {
        outElapsedSec = 0;
        return {};
    }

    IN_ADDR ipAddr{}, maskAddr{};
    if (inet_pton(AF_INET, myIP.c_str(), &ipAddr) != 1 ||
        inet_pton(AF_INET, myMask.c_str(), &maskAddr) != 1) {
        outElapsedSec = 0;
        return {};
    }
    uint32_t ipNumber = ntohl(ipAddr.S_un.S_addr);
    uint32_t maskNumber = ntohl(maskAddr.S_un.S_addr);
    uint32_t networkNumber = ipNumber & maskNumber;
    uint32_t broadcastNumber = networkNumber | ~maskNumber;
    if (broadcastNumber <= networkNumber + 1 ||
        static_cast<uint64_t>(broadcastNumber) - networkNumber > 4096) {
        outElapsedSec = 0;
        return {};
    }
    if (gatewayIP.empty()) {
        IN_ADDR gatewayAddr{};
        gatewayAddr.S_un.S_addr = htonl(networkNumber + 1);
        char gatewayText[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &gatewayAddr, gatewayText, sizeof(gatewayText));
        gatewayIP = gatewayText;
    }

    char localHostName[256] = {0};
    DWORD hostSz = sizeof(localHostName);
    if (!GetComputerNameA(localHostName, &hostSz) || hostSz == 0) {
        char* envComp = getenv("COMPUTERNAME");
        if (envComp && strlen(envComp) > 0) {
            strncpy(localHostName, envComp, sizeof(localHostName) - 1);
        } else {
            gethostname(localHostName, sizeof(localHostName));
        }
    }
    if (strlen(localHostName) == 0) {
        strcpy(localHostName, "This-PC");
    }

    auto scanStart = chrono::high_resolution_clock::now();

    // Quét các địa chỉ host thuộc subnet thật; giới hạn 4096 địa chỉ.
    #pragma omp parallel for schedule(dynamic, 1) num_threads(32)
    for (int64_t address = static_cast<int64_t>(networkNumber) + 1;
         address < static_cast<int64_t>(broadcastNumber); ++address) {
        if (static_cast<uint32_t>(address) == ipNumber) continue;

        IPAddr destIp = htonl(static_cast<uint32_t>(address));
        HANDLE hIcmp = IcmpCreateFile();
        if (hIcmp != INVALID_HANDLE_VALUE) {
            char sendData[] = "Q";
            BYTE replyBuf[sizeof(ICMP_ECHO_REPLY) + 32];
            IcmpSendEcho(hIcmp, destIp, sendData, sizeof(sendData), NULL, replyBuf, sizeof(replyBuf), 70);
            IcmpCloseHandle(hIcmp);
        }
    }

    unordered_map<string, string> activeMap;
    activeMap[myIP] = (!myMAC.empty() ? myMAC : "00:00:00:00:00:00");

    // Đọc bảng ARP Cache hệ thống Windows
    ULONG tableSize = 0;
    GetIpNetTable(NULL, &tableSize, FALSE);
    if (tableSize > 0) {
        PMIB_IPNETTABLE pIpNetTable = (PMIB_IPNETTABLE)malloc(tableSize);
        if (pIpNetTable && GetIpNetTable(pIpNetTable, &tableSize, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < pIpNetTable->dwNumEntries; i++) {
                MIB_IPNETROW row = pIpNetTable->table[i];
                if (row.dwType != 2) { // 2 = MIB_IPNET_TYPE_INVALID
                    in_addr inAddr;
                    inAddr.s_addr = row.dwAddr;
                    string ip = inet_ntoa(inAddr);
                    uint32_t arpAddress = ntohl(row.dwAddr);
                    if ((arpAddress & maskNumber) == networkNumber &&
                        arpAddress != networkNumber && arpAddress != broadcastNumber &&
                        row.dwPhysAddrLen == 6) {
                        char macBuf[24];
                        sprintf(macBuf, "%02X:%02X:%02X:%02X:%02X:%02X",
                                row.bPhysAddr[0], row.bPhysAddr[1], row.bPhysAddr[2],
                                row.bPhysAddr[3], row.bPhysAddr[4], row.bPhysAddr[5]);
                        activeMap[ip] = macBuf;
                    }
                }
            }
        }
        if (pIpNetTable) free(pIpNetTable);
    }

    // Bổ sung quét SendARP nhanh cho Gateway nếu chưa có trong bảng
    if (activeMap.find(gatewayIP) == activeMap.end()) {
        IPAddr gwDest = inet_addr(gatewayIP.c_str());
        ULONG gwMac[2] = {0};
        ULONG gwLen = 6;
        if (SendARP(gwDest, 0, gwMac, &gwLen) == NO_ERROR && gwLen == 6) {
            BYTE* b = (BYTE*)gwMac;
            char macBuf[24];
            sprintf(macBuf, "%02X:%02X:%02X:%02X:%02X:%02X", b[0], b[1], b[2], b[3], b[4], b[5]);
            activeMap[gatewayIP] = macBuf;
        }
    }

    // 3. GIAI ĐOẠN 2: PHÂN LOẠI & ĐỊNH DANH CHI TIẾT (SONG SONG NETBIOS + DNS PTR + OUI)
    vector<pair<string, string>> activeEntries(activeMap.begin(), activeMap.end());
    vector<DiscoveredDevice> tempDevices(activeEntries.size());

    #pragma omp parallel for schedule(dynamic, 1) num_threads(16)
    for (int i = 0; i < (int)activeEntries.size(); ++i) {
        string ip = activeEntries[i].first;
        string mac = activeEntries[i].second;
        bool isLocal = (ip == myIP);
        bool isGw = (ip == gatewayIP);

        string netBios = "";
        string dnsHost = "";

        if (!isLocal) {
            dnsHost = queryLocalDnsPtr(ip, gatewayIP, 50);
            if (dnsHost.empty() && !isGw) {
                netBios = queryNetBiosName(ip);
            }
        }

        auto res = classifyDevice(ip, mac, isLocal, isGw, localHostName, netBios, dnsHost);

        DiscoveredDevice dev;
        dev.ip = ip;
        dev.ipInt = ntohl(inet_addr(ip.c_str()));
        dev.mac = mac;
        dev.hostname = res.first;
        dev.deviceType = res.second;
        dev.isSelf = isLocal;
        dev.isGateway = isGw;

        tempDevices[i] = dev;
    }

    vector<DiscoveredDevice> deviceList = tempDevices;

    // Sắp xếp: Router -> Máy tính này -> Các IP khác tăng dần
    sort(deviceList.begin(), deviceList.end(), [](const DiscoveredDevice& a, const DiscoveredDevice& b) {
        if (a.isGateway != b.isGateway) return a.isGateway > b.isGateway;
        if (a.isSelf != b.isSelf) return a.isSelf > b.isSelf;
        return a.ipInt < b.ipInt;
    });

    auto scanEnd = chrono::high_resolution_clock::now();
    outElapsedSec = chrono::duration<double>(scanEnd - scanStart).count();

    return deviceList;
}

void NetworkScanner::scanConnectedDevices() {
    while (true) {
        sc.cls();
        cout << "\n[*] Đang quét mạng...\n";
        cout.flush();

        double elapsedSec = 0.0;
        vector<DiscoveredDevice> deviceList = performScan(elapsedSec);

        if (deviceList.empty()) {
            sc.cls();
            cout << "[!] Không có kết nối LAN đang hoạt động.\n";
            sc.waitEnter();
            return;
        }

        // Hiển thị bảng danh sách thiết bị căn chỉnh chuẩn UTF-8
        sc.cls();
        cout << "\n\n";
        cout << "+-----+-----------------+-------------------+-------------------------------+-----------------------------------+\n"
             << "| STT |   Địa chỉ IP    |    Địa chỉ MAC    |   Loại thiết bị / Phân loại   |      Tên thiết bị (Hostname)      |\n"
             << "+-----+-----------------+-------------------+-------------------------------+-----------------------------------+\n";

        for (size_t i = 0; i < deviceList.size(); ++i) {
            const auto& d = deviceList[i];
            string typeDisplay = utf8PadRight(d.deviceType, 29);
            string hostDisplay = utf8PadRight(d.hostname, 33);
            string ipDisplay = utf8PadRight(d.ip, 15);
            string macDisplay = utf8PadRight(d.mac, 17);

            // Router: Xanh lá, Máy tính này: Cyan, Thiết bị khác: Mặc định
            string colorCode = "";
            string resetCode = "\x1b[0m";
            if (d.isGateway) colorCode = "\x1b[32m";
            else if (d.isSelf) colorCode = "\x1b[36m";

            cout << "| " << setw(3) << left << (i + 1) << " | "
                 << colorCode << ipDisplay << resetCode << " | "
                 << macDisplay << " | "
                 << typeDisplay << " | "
                 << hostDisplay << " |\n";
        }
        cout << "+-----+-----------------+-------------------+-------------------------------+-----------------------------------+\n";
        cout << " [*] Thời gian quét: \x1b[93m" << fixed << setprecision(2) << elapsedSec << "s\x1b[0m | "
             << "Tìm thấy \x1b[32m" << deviceList.size() << "\x1b[0m thiết bị đang kết nối mạng.\n\n";

        cout << " Enter: quay lại | 1/r: quét lại: ";
        string opt;
        getline(cin, opt);
        opt = SystemCore::trim(opt);
        if (opt == "1" || opt == "r" || opt == "R") {
            continue;
        }
        return;
    }
}
