#include "LocalDrop.h"
#include "MenuStyle.h"
#include "FileSafety.h"
#include "LocalDropProtocol.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <conio.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <filesystem>
#include <shlobj.h>
#include <algorithm>
#include <cstdint>
#include <cctype>
#include <climits>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#endif

namespace fs = std::filesystem;
using namespace std;

static void closeDropSocket(SOCKET& socket) {
    if (socket != INVALID_SOCKET) { closesocket(socket); socket = INVALID_SOCKET; }
}
struct DropSocketGuard {
    SOCKET& socket;
    ~DropSocketGuard() { closeDropSocket(socket); }
};
static bool connectWithTimeout(SOCKET socket, const sockaddr_in& address) {
    u_long nonblocking = 1;
    if (ioctlsocket(socket, FIONBIO, &nonblocking)) return false;
    int result = connect(socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    bool ok = result == 0;
    if (!ok && WSAGetLastError() == WSAEWOULDBLOCK) {
        fd_set ready, failed; FD_ZERO(&ready); FD_ZERO(&failed); FD_SET(socket, &ready); FD_SET(socket, &failed);
        timeval timeout{8,0};
        if (select(0, nullptr, &ready, &failed, &timeout) > 0 && FD_ISSET(socket, &ready)) {
            int error = 1, size = sizeof(error);
            ok = !getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &size) && !error;
        }
    }
    nonblocking = 0;
    return !ioctlsocket(socket, FIONBIO, &nonblocking) && ok;
}

static const int DEFAULT_TCP_PORT = 8888;
static const int DEFAULT_UDP_BEACON_PORT = 53318;
static const int CHUNK_SIZE = 262144; // 256 KB (Tối ưu thông lượng LAN & Wi-Fi)

static string createSessionToken() {
    GUID id{};
    if (FAILED(CoCreateGuid(&id))) {
        return to_string(GetTickCount64()) + to_string(GetCurrentProcessId());
    }
    char token[33];
    snprintf(token, sizeof(token), "%08lX%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
             static_cast<unsigned long>(id.Data1), id.Data2, id.Data3,
             id.Data4[0], id.Data4[1], id.Data4[2], id.Data4[3],
             id.Data4[4], id.Data4[5], id.Data4[6], id.Data4[7]);
    return token;
}

static string htmlEscape(const string &value) {
    string escaped;
    for (char c : value) {
        switch (c) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += c;
        }
    }
    return escaped;
}

static string encodeHeaderFilename(const string &value) {
    static const char hex[] = "0123456789ABCDEF";
    string encoded;
    for (unsigned char c : value) {
        if (std::isalnum(c) && c < 128) encoded += static_cast<char>(c);
        else if (c == '-' || c == '_' || c == '.') encoded += static_cast<char>(c);
        else { encoded += '%'; encoded += hex[c >> 4]; encoded += hex[c & 15]; }
    }
    return encoded;
}

// QR remains API-based. The URL contains no pairing code or file details.
static string fetchQrCodeApi(const string &targetUrl) {
    string cmd = "curl.exe -f -s --connect-timeout 2 --max-time 5 \"https://qrenco.de/" + targetUrl + "\"";
    FILE *pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buf[512];
    string qr = "";
    while (fgets(buf, sizeof(buf), pipe)) {
        qr += "   ";
        qr += buf;
    }
    _pclose(pipe);
    return qr;
}

LocalDrop::LocalDrop(SystemCore &core) : sc(core) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

LocalDrop::~LocalDrop() {
    isRunning = false;
    if (serverTcpSocket != INVALID_SOCKET) {
        closesocket(serverTcpSocket);
        serverTcpSocket = INVALID_SOCKET;
    }
    if (beaconUdpSocket != INVALID_SOCKET) {
        closesocket(beaconUdpSocket);
        beaconUdpSocket = INVALID_SOCKET;
    }
    if (!firewallRuleName.empty()) removeFirewallRule(firewallRuleName);
    WSACleanup();
}

// ----------------------------------------------------------------------------------
// Win32 GetAdaptersAddresses: Lọc bỏ card ảo (VMware, VirtualBox, Hyper-V, vEthernet, TAP)
// Ưu tiên adapter Wi-Fi hoặc Ethernet đang có Default Gateway hoạt động
// ----------------------------------------------------------------------------------
string LocalDrop::detectBestLANIP() {
    ULONG outBufLen = 15000;
    std::vector<BYTE> buf(outBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());

    ULONG dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, NULL, pAddresses, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        buf.resize(outBufLen);
        pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());
        dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, NULL, pAddresses, &outBufLen);
    }

    string bestIP = "";
    bool isBestWifi = false;

    if (dwRetVal == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != NULL; pCurr = pCurr->Next) {
            if (pCurr->OperStatus != IfOperStatusUp) continue;
            if (pCurr->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

            // Kiểm tra tên card để loại bỏ card ảo
            wstring descW = pCurr->Description ? pCurr->Description : L"";
            wstring friendlyW = pCurr->FriendlyName ? pCurr->FriendlyName : L"";
            string desc(descW.begin(), descW.end());
            string friendly(friendlyW.begin(), friendlyW.end());
            for (auto &c : desc) c = tolower(c);
            for (auto &c : friendly) c = tolower(c);

            if (desc.find("vmware") != string::npos || friendly.find("vmware") != string::npos) continue;
            if (desc.find("virtualbox") != string::npos || friendly.find("virtualbox") != string::npos) continue;
            if (desc.find("vbox") != string::npos || friendly.find("vbox") != string::npos) continue;
            if (desc.find("hyper-v") != string::npos || friendly.find("hyper-v") != string::npos) continue;
            if (desc.find("vethernet") != string::npos || friendly.find("vethernet") != string::npos) continue;
            if (desc.find("tap") != string::npos || friendly.find("tap") != string::npos) continue;
            if (desc.find("npcap") != string::npos || friendly.find("npcap") != string::npos) continue;
            if (desc.find("wsl") != string::npos || friendly.find("wsl") != string::npos) continue;
            if (desc.find("bluetooth") != string::npos || friendly.find("bluetooth") != string::npos) continue;

            // Kiểm tra Gateway
            bool hasGateway = (pCurr->FirstGatewayAddress != NULL);
            if (!hasGateway) continue;

            // Lấy địa chỉ Unicast IPv4
            for (PIP_ADAPTER_UNICAST_ADDRESS pUni = pCurr->FirstUnicastAddress; pUni != NULL; pUni = pUni->Next) {
                if (pUni->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in *sa_in = reinterpret_cast<sockaddr_in*>(pUni->Address.lpSockaddr);
                    char ipStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(sa_in->sin_addr), ipStr, INET_ADDRSTRLEN);
                    string ip(ipStr);

                    if (ip.find("127.") == 0 || ip == "0.0.0.0" || ip.find("169.254.") == 0) continue;

                    // Nếu là Wi-Fi (IF_TYPE_IEEE80211) -> ưu tiên cao nhất
                    if (pCurr->IfType == IF_TYPE_IEEE80211) {
                        return ip;
                    }
                    if (bestIP.empty() || (!isBestWifi && pCurr->IfType == IF_TYPE_ETHERNET_CSMACD)) {
                        bestIP = ip;
                        isBestWifi = (pCurr->IfType == IF_TYPE_IEEE80211);
                    }
                }
            }
        }
    }

    if (!bestIP.empty()) return bestIP;

    // Fallback: socket connect test để lấy local IP kết nối ra mạng
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s != INVALID_SOCKET) {
        sockaddr_in loopback;
        loopback.sin_family = AF_INET;
        loopback.sin_addr.s_addr = inet_addr("8.8.8.8");
        loopback.sin_port = htons(53);
        if (connect(s, (sockaddr*)&loopback, sizeof(loopback)) != SOCKET_ERROR) {
            sockaddr_in localAddr;
            int addrLen = sizeof(localAddr);
            if (getsockname(s, (sockaddr*)&localAddr, &addrLen) != SOCKET_ERROR) {
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &localAddr.sin_addr, ipStr, sizeof(ipStr));
                closesocket(s);
                return string(ipStr);
            }
        }
        closesocket(s);
    }

    return "127.0.0.1";
}

unordered_set<string> LocalDrop::getAllLocalIPs() {
    unordered_set<string> ips;
    ips.insert("127.0.0.1");

    ULONG outBufLen = 15000;
    std::vector<BYTE> buf(outBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());

    ULONG dwRetVal = GetAdaptersAddresses(AF_INET, 0, NULL, pAddresses, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        buf.resize(outBufLen);
        pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());
        dwRetVal = GetAdaptersAddresses(AF_INET, 0, NULL, pAddresses, &outBufLen);
    }

    if (dwRetVal == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != NULL; pCurr = pCurr->Next) {
            if (pCurr->OperStatus != IfOperStatusUp) continue;
            for (PIP_ADAPTER_UNICAST_ADDRESS pUni = pCurr->FirstUnicastAddress; pUni != NULL; pUni = pUni->Next) {
                if (pUni->Address.lpSockaddr && pUni->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in *sa_in = reinterpret_cast<sockaddr_in*>(pUni->Address.lpSockaddr);
                    char ipStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(sa_in->sin_addr), ipStr, INET_ADDRSTRLEN);
                    ips.insert(string(ipStr));
                }
            }
        }
    }
    return ips;
}

// ----------------------------------------------------------------------------------
// Quản lý Windows Firewall tự động
// ----------------------------------------------------------------------------------
bool LocalDrop::addFirewallRule(int port, const string &ruleName) {
    string cmd = "netsh advfirewall firewall add rule name=\"" + ruleName + "\" dir=in action=allow protocol=TCP localport="
                 + to_string(port) + " profile=private remoteip=localsubnet";
    return SystemCore::runRawCommand(cmd);
}

bool LocalDrop::removeFirewallRule(const string &ruleName) {
    return SystemCore::runRawCommand("netsh advfirewall firewall delete rule name=\"" + ruleName + "\"");
}

// ----------------------------------------------------------------------------------
// Phân tích User-Agent để nhận diện thiết bị
// ----------------------------------------------------------------------------------
string LocalDrop::parseDeviceName(const string &userAgent, const string &clientIP) {
    string name = "";
    if (userAgent.find("iPhone") != string::npos) {
        name = "Apple iPhone";
    } else if (userAgent.find("iPad") != string::npos) {
        name = "Apple iPad";
    } else if (userAgent.find("Macintosh") != string::npos) {
        name = "Apple Mac";
    } else if (userAgent.find("Android") != string::npos) {
        name = "Android Device";
    } else if (userAgent.find("Windows") != string::npos) {
        name = "Windows PC";
    } else if (userAgent.find("Linux") != string::npos) {
        name = "Linux Device";
    } else if (userAgent.find("CMDBOX_CLI") != string::npos) {
        name = "CMD Box";
    } else {
        name = "Thiết bị mạng";
    }
    return clientIP + " (" + name + ")";
}

string LocalDrop::sanitizeFilename(const string &filename) {
    string safe = filename;
    for (char &c : safe) {
        if (static_cast<unsigned char>(c) < 32 || c == '/' || c == '\\' || c == ':' ||
            c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    while (!safe.empty() && (safe.back() == '.' || safe.back() == ' ')) safe.pop_back();
    if (safe.empty() || safe == "." || safe == "..") safe = "received_file";
    string stem = safe.substr(0, safe.find('.'));
    transform(stem.begin(), stem.end(), stem.begin(), [](unsigned char c) { return static_cast<char>(toupper(c)); });
    if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" ||
        (stem.size() == 4 && (stem.rfind("COM", 0) == 0 || stem.rfind("LPT", 0) == 0) &&
         stem[3] >= '1' && stem[3] <= '9')) safe = "received_" + safe;
    return safe;
}

string LocalDrop::getDownloadsFolder() {
    PWSTR path = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &path))) {
        int needed = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
        string result;
        if (needed > 0) {
            vector<char> buf(needed);
            if (WideCharToMultiByte(CP_UTF8, 0, path, -1, buf.data(), needed, NULL, NULL) > 0)
                result = buf.data();
        }
        CoTaskMemFree(path);
        if (!result.empty()) return result;
    }
    return ""; // Fail closed if Windows cannot resolve Downloads.
}

// ----------------------------------------------------------------------------------
// MENU CHÍNH CỦA LOCAL DROP
// ----------------------------------------------------------------------------------
void LocalDrop::menu() {
    while (true) {
        sc.cls();
        MenuStyle::header("LOCALDROP",MenuStyle::SKY);
        MenuStyle::item(1,"Chia sẻ qua Web QR",MenuStyle::SKY);
        MenuStyle::section("LAN P2P · Ghép đôi bằng mã · HTTP không mã hóa");
        MenuStyle::item(2,"Gửi file / thư mục",MenuStyle::MINT);
        MenuStyle::item(3,"Nhận file / thư mục",MenuStyle::ORCHID);
        MenuStyle::footer("Quay lại",MenuStyle::SKY);

        int choice = sc.readInt("");
        if (choice == 0) break;
        if (choice == 1) startPublicDrop();
        else if (choice == 2) startSecureSender();
        else if (choice == 3) startSecureReceiver();
    }
}

void LocalDrop::startPublicDrop(const string &defaultFile) {
    startSender(defaultFile, false); // Công khai: hiện QR và link, không bắn beacon ngầm
}

void LocalDrop::startSecureSender(const string &defaultFile) {
    startSender(defaultFile, true); // Pairing code is never advertised in the beacon.
}

void LocalDrop::startSecureReceiver() {
    startReceiver();
}

// ----------------------------------------------------------------------------------
// Sender: file manifest, pairing, HTTP byte ranges and LAN discovery.
// ----------------------------------------------------------------------------------

void LocalDrop::startSender(const string &defaultFile, bool isSecure) {
    using namespace DropV2;
    sc.cls();
    vector<string> inputs;
    string raw=defaultFile;
    if(raw.empty()) {
        cout << "\n Kéo thả nhiều file / thư mục (đường dẫn có khoảng trắng đặt trong dấu nháy).\n [0: Hủy] > ";
        if(!getline(cin,raw) || raw=="0" || raw.empty())return;
    }
    raw=SystemCore::trim(raw);if(raw.rfind("& ",0)==0)raw=SystemCore::trim(raw.substr(2));
    string single=raw;
    if(single.size()>=2 && ((single.front()=='\"' && single.back()=='\"') || (single.front()=='\'' && single.back()=='\'')))single=single.substr(1,single.size()-2);
    error_code ec;
    if(fs::exists(fs::u8path(single),ec))inputs.push_back(single);
    else inputs=SystemCore::parsePaths(raw);
    if(inputs.empty()){cout << " Không tìm thấy đường dẫn.\n";sc.waitEnter();return;}
    vector<Entry> entries;
    struct SourceLocks {vector<HANDLE> handles;~SourceLocks(){for(auto h:handles)CloseHandle(h);}} locks;
    try {
        auto add=[&](const fs::path& p,const string& name) {
            if(entries.size()>=MaxFiles || !validName(name))throw runtime_error("Tên file không hỗ trợ hoặc quá 10000 file.");
            FileSafety::AncestorLocks parents;if(!parents.acquire(p))throw runtime_error("Không gửi qua junction/symlink.");
            HANDLE h=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
            if(h==INVALID_HANDLE_VALUE)throw runtime_error("Không khóa được file nguồn; hãy đóng ứng dụng đang sửa file.");
            locks.handles.push_back(h);BY_HANDLE_FILE_INFORMATION info{};
            if(!GetFileInformationByHandle(h,&info) || (info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)))throw runtime_error("File nguồn không hợp lệ.");
            Entry e;e.source=p;e.name=name;e.size=(uint64_t(info.nFileSizeHigh)<<32)|info.nFileSizeLow;
            e.hash=sha256Handle(h);if(e.hash.empty())throw runtime_error("Không tính được SHA-256.");
            entries.push_back(e);cout << "\r [*] Đã kiểm tra SHA-256: " << entries.size() << " file" << flush;
        };
        for(const auto& input:inputs) {
            auto p=fs::absolute(fs::u8path(input)).lexically_normal();
            DWORD attrs=GetFileAttributesW(p.c_str());if(attrs==INVALID_FILE_ATTRIBUTES || (attrs&FILE_ATTRIBUTE_REPARSE_POINT))throw runtime_error("Đường dẫn không hợp lệ hoặc là liên kết.");
            if(fs::is_directory(p)) {
                auto rootName=p.filename().u8string();if(rootName.empty())throw runtime_error("Hãy chọn thư mục con thay vì toàn bộ ổ đĩa.");
                for(fs::recursive_directory_iterator it(p),end;it!=end;++it) {
                    DWORD a=GetFileAttributesW(it->path().c_str());
                    if(a==INVALID_FILE_ATTRIBUTES || (a&FILE_ATTRIBUTE_REPARSE_POINT))throw runtime_error("Thư mục chứa junction/symlink; hãy loại bỏ liên kết khỏi lựa chọn.");
                    if(it->is_regular_file())add(it->path(),rootName+"/"+it->path().lexically_relative(p).generic_u8string());
                }
            } else add(p,p.filename().u8string());
        }
    } catch(const exception& e){cout << "\n [!] " << e.what() << '\n';sc.waitEnter();return;}
    sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){return a.name<b.name;});
    auto list=manifest(entries);vector<Entry> checked;
    if(!parseManifest(list,checked)){cout << "\n [!] Danh sách trống, quá lớn, hoặc trùng tên đích.\n";sc.waitEnter();return;}
    uint64_t total=0;for(const auto& e:entries)total+=e.size;
    string code;
    if(isSecure) {
        ULONG random=0;
        if(BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&random),sizeof(random),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0){cout << "Không tạo được mã ghép đôi.\n";return;}
        // Rejection sampling avoids modulo bias in the six-digit code.
        while(random>=4294000000UL)if(BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&random),sizeof(random),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)return;
        ostringstream formatted;formatted << setw(6) << setfill('0') << random%1000000;code=formatted.str();
    }
    string session=createSessionToken(),ip=detectBestLANIP();int port=DEFAULT_TCP_PORT;
    serverTcpSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);DropSocketGuard tcpGuard{serverTcpSocket};
    if(serverTcpSocket==INVALID_SOCKET){cout << "Lỗi socket.\n";sc.waitEnter();return;}
    BOOL exclusive=TRUE;setsockopt(serverTcpSocket,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof(exclusive));
    sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_ANY);
    bool bound=false;for(;port<=8889;++port){addr.sin_port=htons(port);if(::bind(serverTcpSocket,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))==0){bound=true;break;}}
    if(!bound || listen(serverTcpSocket,8)){cout << "Không mở được cổng 8888/8889.\n";sc.waitEnter();return;}
    firewallRuleName="CMDBOX_DROP_"+to_string(GetCurrentProcessId())+"_"+session;
    if(!addFirewallRule(port,firewallRuleName)){firewallRuleName.clear();cout << "Không mở được firewall; kiểm tra quyền và mạng Private.\n";}
    struct Cleanup {LocalDrop* self;~Cleanup(){self->isRunning=false;if(!self->firewallRuleName.empty()){LocalDrop::removeFirewallRule(self->firewallRuleName);self->firewallRuleName.clear();}}} cleanup{this};
    string home="http://"+ip+":"+to_string(port)+"/";
    cout << "\n [*] " << entries.size() << " file, " << SystemCore::formatSize(total) << "\n [*] " << home << '\n';
    if(isSecure)cout << " [*] Mã ghép đôi: " << code << " (nhập trên máy nhận / trang web)\n";
    cout << " [*] HTTP không mã hóa. QR dùng API bên ngoài, chỉ chứa địa chỉ trang chủ.\n";
    string qr=fetchQrCodeApi(home);
    cout << (qr.empty() ? " [*] API QR chưa phản hồi; dùng đường dẫn ở trên.\n" : qr)
         << "\n [0 / ESC: Dừng trạm phát]\n" << flush;
    beaconUdpSocket=isSecure ? socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP) : INVALID_SOCKET;DropSocketGuard udpGuard{beaconUdpSocket};
    if(beaconUdpSocket!=INVALID_SOCKET){BOOL yes=TRUE;setsockopt(beaconUdpSocket,SOL_SOCKET,SO_BROADCAST,reinterpret_cast<char*>(&yes),sizeof(yes));}
    isRunning=true;
    auto lastBeacon=chrono::steady_clock::now()-chrono::seconds(3);
    // Global rate limit prevents trying every pairing code by changing source IP.
    int failedCodes=0;auto cooldown=chrono::steady_clock::now();
    while(isRunning) {
        if(_kbhit()){int key=_getch();if(key=='0' || key==27)break;}
        auto now=chrono::steady_clock::now();
        if(beaconUdpSocket!=INVALID_SOCKET && now-lastBeacon>=chrono::seconds(2)) {
            sockaddr_in dst{};dst.sin_family=AF_INET;dst.sin_port=htons(DEFAULT_UDP_BEACON_PORT);dst.sin_addr.s_addr=INADDR_BROADCAST;
            string beacon="CMDBOX2|"+to_string(port)+"|"+session+"|"+to_string(entries.size())+"|"+to_string(total);
            sendto(beaconUdpSocket,beacon.data(),int(beacon.size()),0,reinterpret_cast<sockaddr*>(&dst),sizeof(dst));lastBeacon=now;
        }
        fd_set ready;FD_ZERO(&ready);FD_SET(serverTcpSocket,&ready);timeval wait{0,200000};
        if(select(0,&ready,nullptr,nullptr,&wait)<=0)continue;
        Socket client(accept(serverTcpSocket,nullptr,nullptr));if(client.value==INVALID_SOCKET)continue;timeouts(client.value);
        string request,pending;if(!readHeader(client.value,request,pending))continue;
        string method,target,version;istringstream line(request);line>>method>>target>>version;
        bool head=method=="HEAD";if(method!="GET" && !head){response(client.value,405);continue;}
        size_t query=target.find('?');string path=target.substr(0,query),provided=query==string::npos ? "" : target.substr(query+1);
        bool authorized=!isSecure || provided=="code="+code;
        if(isSecure && (path!="/" || query!=string::npos)) {
            if(now<cooldown){response(client.value,429,"Thử lại sau 30 giây.");continue;}
            if(!authorized){if(++failedCodes>=5){cooldown=now+chrono::seconds(30);failedCodes=0;}response(client.value,403,"Sai mã ghép đôi.");continue;}
            // Count only failed attempts; successful requests do not reset the limiter.
        }
        if(path=="/" && (!isSecure || query==string::npos || authorized)) {
            string body="<!doctype html><html lang='vi'><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>CMD BOX LocalDrop</title><style>body{background:#0f172a;color:#e2e8f0;font:16px system-ui;max-width:720px;margin:40px auto;padding:20px}a{color:#38bdf8;overflow-wrap:anywhere}li{padding:10px}input,button{font:inherit;padding:12px;border-radius:8px}small{color:#94a3b8}</style><h1>📦 CMD BOX LocalDrop</h1>";
            if(isSecure && !authorized)body+="<form method='get'><label>Mã ghép đôi trên máy gửi: <input name='code' pattern='[0-9]{6}' maxlength='6' required autocomplete='off'></label> <button>Mở danh sách</button></form>";
            else {body+="<p>"+to_string(entries.size())+" file • "+SystemCore::formatSize(total)+"</p><ul>";
                for(size_t i=0;i<entries.size();++i)body+="<li><a href='/file/"+to_string(i)+(isSecure ? "?code="+code : "")+"' download>"+htmlEscape(entries[i].name)+"</a> — "+SystemCore::formatSize(entries[i].size)+"<br><small>SHA-256: "+entries[i].hash+"</small></li>";
                body+="</ul>";}
            body+="<p>Truyền trong LAN. Để nhận cả thư mục, tự tiếp tục và kiểm tra hash, dùng CMD BOX trên máy nhận.</p></html>";
            response(client.value,200,body,"text/html; charset=UTF-8",head);continue;
        }
        if(path=="/manifest"){response(client.value,200,list,"text/plain; charset=UTF-8",head);continue;}
        uint64_t id=0;
        if(path.rfind("/file/",0)!=0 || !number(path.substr(6),id) || id>=entries.size()){response(client.value,404);continue;}
        const auto& e=entries[size_t(id)];uint64_t begin=0,end=0;string requested=field(request,"range");
        if(!range(requested,e.size,begin,end)) {
            string h="HTTP/1.1 416 Range Not Satisfiable\r\nContent-Range: bytes */"+to_string(e.size)+"\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";sendBytes(client.value,h.data(),h.size());continue;
        }
        ifstream input(e.source,ios::binary);if(!input){response(client.value,409);continue;}input.seekg(streamoff(begin));
        uint64_t length=e.size ? end-begin+1 : 0;
        ostringstream h;h << "HTTP/1.1 " << (requested.empty()?"200 OK":"206 Partial Content") << "\r\nContent-Type: application/octet-stream\r\nContent-Disposition: attachment; filename=\"download\"; filename*=UTF-8''" << encodeHeaderFilename(fs::u8path(e.name).filename().u8string()) << "\r\nAccept-Ranges: bytes\r\nETag: \"" << e.hash << "\"\r\nX-CMDBOX-SHA256: " << e.hash << "\r\nContent-Length: " << length << "\r\n";
        if(!requested.empty())h << "Content-Range: bytes " << begin << '-' << end << '/' << e.size << "\r\n";
        h << "Cache-Control: no-store\r\nConnection: close\r\n\r\n";string header=h.str();
        if(!sendBytes(client.value,header.data(),header.size()) || head)continue;
        vector<char> buffer(CHUNK_SIZE);uint64_t sent=begin;auto started=chrono::steady_clock::now();
        cout << "\n [*] Gửi " << e.name << " từ byte " << begin << '\n';
        while(length && isRunning && chrono::steady_clock::now()-started<chrono::hours(1)) {
            if(_kbhit()){int key=_getch();if(key=='0' || key==27){isRunning=false;break;}}
            auto n=(std::min)(length,uint64_t(buffer.size()));input.read(buffer.data(),streamsize(n));
            if(input.gcount()!=streamsize(n) || !sendBytes(client.value,buffer.data(),size_t(n)))break;
            length-=n;sent+=n;cout << "\r [*] " << SystemCore::formatSize(sent) << " / " << SystemCore::formatSize(e.size) << "     " << flush;
        }
        cout << (length ? "\n [*] Kết nối ngắt; máy nhận có thể tiếp tục.\n" : "\n [✓] Đã gửi xong.\n");
    }
    cout << "\n [*] Đã đóng trạm phát.\n";
}

void LocalDrop::startReceiver() {
    using namespace DropV2;
    sc.cls();cout << "\n [*] Tìm phiên LocalDrop v2 trong LAN (0 / ESC: Hủy)...\n";
    Socket udp(socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP));if(udp.value==INVALID_SOCKET)return;
    timeouts(udp.value,400);BOOL exclusive=TRUE;setsockopt(udp.value,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof(exclusive));
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(DEFAULT_UDP_BEACON_PORT);address.sin_addr.s_addr=htonl(INADDR_ANY);
    if(::bind(udp.value,reinterpret_cast<sockaddr*>(&address),sizeof(address))){cout << "Cổng dò LAN đang bận.\n";sc.waitEnter();return;}
    string host;int port=0;auto own=getAllLocalIPs();
    while(host.empty()) {
        if(_kbhit()){int key=_getch();if(key=='0' || key==27)return;}
        char buffer[2048];sockaddr_in from{};int size=sizeof(from);int n=recvfrom(udp.value,buffer,sizeof(buffer),0,reinterpret_cast<sockaddr*>(&from),&size);
        if(n<=0)continue;
        char ip[INET_ADDRSTRLEN];inet_ntop(AF_INET,&from.sin_addr,ip,sizeof(ip));if(own.count(ip))continue;
        vector<string> parts;istringstream msg(string(buffer,n));string part;while(getline(msg,part,'|'))parts.push_back(part);
        uint64_t p=0,count=0,total=0;
        if(parts.size()!=5 || parts[0]!="CMDBOX2" || !number(parts[1],p) || p<1 || p>65535 ||
            parts[2].size()!=32 || parts[2].find_first_not_of("0123456789abcdefABCDEF")!=string::npos ||
            !number(parts[3],count) || !count || count>MaxFiles || !number(parts[4],total) || total>uint64_t(INT64_MAX))continue;
        host=ip;port=int(p);
        cout << " [*] Máy gửi: " << host << ':' << port << " — " << count << " file, " << SystemCore::formatSize(total) << '\n';
    }
    cout << " Nhập mã ghép đôi 6 số trên máy gửi (0: Hủy): ";string code;if(!getline(cin,code) || code=="0")return;
    if(code.size()!=6 || code.find_first_not_of("0123456789")!=string::npos){cout << "Mã phải gồm 6 số.\n";sc.waitEnter();return;}
    string auth="?code="+code;
    auto connectToSender=[&](Socket& client) {
        client.value=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(client.value==INVALID_SOCKET)return false;
        timeouts(client.value);sockaddr_in dst{};dst.sin_family=AF_INET;dst.sin_port=htons(port);
        return inet_pton(AF_INET,host.c_str(),&dst.sin_addr)==1 && connectWithTimeout(client.value,dst);
    };
    vector<Entry> entries;string body;
    {
        Socket client;if(!connectToSender(client)){cout << "Không kết nối được máy gửi.\n";sc.waitEnter();return;}
        string request="GET /manifest"+auth+" HTTP/1.1\r\nHost: "+host+":"+to_string(port)+"\r\nConnection: close\r\n\r\n";
        if(!sendBytes(client.value,request.data(),request.size()))return;
        string header,pending;uint64_t length=0;
        if(!readHeader(client.value,header,pending) || header.rfind("HTTP/1.1 200 ",0)!=0 || !number(field(header,"content-length"),length) || length>MaxManifest){cout << "Sai mã ghép đôi, phiên bị giới hạn hoặc máy gửi không tương thích.\n";sc.waitEnter();return;}
        body=pending;char buffer[4096];auto deadline=chrono::steady_clock::now()+chrono::seconds(30);
        while(body.size()<length && chrono::steady_clock::now()<deadline){int n=recv(client.value,buffer,int((std::min)(size_t(length-body.size()),sizeof(buffer))),0);if(n<=0)break;body.append(buffer,n);}
        if(body.size()!=length || !parseManifest(body,entries)){cout << "Danh sách file không hợp lệ.\n";sc.waitEnter();return;}
    }
    cout << "\n Danh sách nhận:\n";uint64_t total=0;
    for(const auto& e:entries){cout << "  " << e.name << " — " << SystemCore::formatSize(e.size) << '\n';total+=e.size;}
    if(!sc.confirm("Nhận các file trên vào Downloads?"))return;
    string downloads=getDownloadsFolder();if(downloads.empty()){cout << "Không xác định được Downloads.\n";return;}
    string bundleHash=sha256Bytes(body);
    if(bundleHash.empty()){cout << "Không tính được mã nhận diện danh sách file.\n";return;}
    auto root=fs::u8path(downloads)/fs::u8path("CMD_BOX_Receive_"+bundleHash.substr(0,32));error_code ec;
    // Create and check each directory without following links; held locks prevent ancestor replacement.
    auto ensureDirectory=[&](const fs::path& directory) {
        vector<fs::path> all;auto p=directory;while(!p.empty() && p!=p.root_path()){all.push_back(p);p=p.parent_path();}
        reverse(all.begin(),all.end());
        for(const auto& d:all){FileSafety::AncestorLocks lock;if(!lock.acquire(d))return false;
            DWORD attr=GetFileAttributesW(d.c_str());if(attr==INVALID_FILE_ATTRIBUTES){if(!CreateDirectoryW(d.c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS)return false;attr=GetFileAttributesW(d.c_str());}
            if(attr==INVALID_FILE_ATTRIBUTES || !(attr&FILE_ATTRIBUTE_DIRECTORY) || (attr&FILE_ATTRIBUTE_REPARSE_POINT))return false;
        }return true;
    };
    if(!ensureDirectory(root)){cout << "Thư mục đích không an toàn hoặc không tạo được.\n";sc.waitEnter();return;}
    FileSafety::AncestorLocks rootLocks;if(!rootLocks.acquire(root/L"placeholder")){cout << "Không khóa được thư mục nhận.\n";return;}
    cout << " [*] Lưu tại: " << root.u8string() << '\n';size_t complete=0;
    for(size_t id=0;id<entries.size();++id) {
        const auto& e=entries[id];auto finalPath=root/fs::u8path(e.name);
        if(!ensureDirectory(finalPath.parent_path())){cout << "Không tạo được thư mục con.\n";break;}
        FileSafety::AncestorLocks parents;if(!parents.acquire(finalPath)){cout << "Không khóa được thư mục con.\n";break;}
        // Open final files without following a link; never overwrite an existing destination.
        DWORD existing=GetFileAttributesW(finalPath.c_str());
        if(existing!=INVALID_FILE_ATTRIBUTES) {
            HANDLE h=CreateFileW(finalPath.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
            BY_HANDLE_FILE_INFORMATION info{};bool match=h!=INVALID_HANDLE_VALUE && GetFileInformationByHandle(h,&info) && !(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)) && sha256Handle(h)==e.hash;
            if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);
            if(match){++complete;cout << " [✓] Đã có: " << e.name << '\n';continue;}
            cout << " [!] File đích khác nội dung; không ghi đè: " << e.name << '\n';break;
        }
        // Reserved working directory avoids collisions with any names in the manifest.
        auto working=root/L".cmd-box-partials";
        if(!ensureDirectory(working)){cout << "Không tạo được thư mục tải dở.\n";break;}
        auto partial=working/fs::u8path(to_string(id)+"_"+e.hash+".part");
        FileSafety::AncestorLocks partialParents;if(!partialParents.acquire(partial))break;
        HANDLE file=CreateFileW(partial.c_str(),GENERIC_READ|GENERIC_WRITE|DELETE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
        if(file==INVALID_HANDLE_VALUE){cout << "Không mở được file tải dở.\n";break;}
        struct FileGuard {HANDLE h;~FileGuard(){CloseHandle(h);}} fileGuard{file};
        BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER current{};
        if(!GetFileInformationByHandle(file,&info) || (info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)) || info.nNumberOfLinks!=1 || !GetFileSizeEx(file,&current) || current.QuadPart<0 || uint64_t(current.QuadPart)>e.size){cout << "File tải dở không hợp lệ.\n";break;}
        uint64_t offset=uint64_t(current.QuadPart);bool canceled=false;
        cout << "\n [*] Nhận " << e.name << " (0 / ESC: Dừng và giữ tải dở)\n";
        for(int attempt=0;offset<e.size && attempt<8 && !canceled;++attempt) {
            if(_kbhit()){int key=_getch();if(key=='0' || key==27){canceled=true;break;}}
            auto space=fs::space(root,ec);if(ec || space.available<e.size-offset){cout << "Không đủ dung lượng trống.\n";break;}
            Socket client;bool connected=connectToSender(client);
            string header,pending;uint64_t length=0;
            if(connected) {
                string req="GET /file/"+to_string(id)+auth+" HTTP/1.1\r\nHost: "+host+":"+to_string(port)+"\r\nRange: bytes="+to_string(offset)+"-\r\nConnection: close\r\n\r\n";
                connected=sendBytes(client.value,req.data(),req.size()) && readHeader(client.value,header,pending);
            }
            string expected="bytes "+to_string(offset)+"-"+to_string(e.size-1)+"/"+to_string(e.size);
            if(connected && (header.rfind("HTTP/1.1 206 ",0)!=0 || field(header,"content-range")!=expected || field(header,"x-cmdbox-sha256")!=e.hash || !number(field(header,"content-length"),length) || length!=e.size-offset)) {
                cout << " [!] Phản hồi không khớp phiên / file; dừng nhận.\n";break;
            }
            if(connected) {
                LARGE_INTEGER pos{};pos.QuadPart=LONGLONG(offset);if(!SetFilePointerEx(file,pos,nullptr,FILE_BEGIN))break;
                bool diskError=false;
                auto writeChunk=[&](const char* data,size_t n) {
                    if(n>e.size-offset)return false;
                    DWORD written=0;
                    if(!WriteFile(file,data,DWORD(n),&written,nullptr) || written!=n){diskError=true;return false;}offset+=n;return true;
                };
                if(!writeChunk(pending.data(),pending.size()))break;
                vector<char> buffer(CHUNK_SIZE);auto deadline=chrono::steady_clock::now()+chrono::hours(1);
                while(offset<e.size && chrono::steady_clock::now()<deadline) {
                    if(_kbhit()){int key=_getch();if(key=='0' || key==27){canceled=true;break;}}
                    int n=recv(client.value,buffer.data(),int((std::min)(uint64_t(buffer.size()),e.size-offset)),0);
                    if(n<=0 || !writeChunk(buffer.data(),size_t(n)))break;
                    cout << "\r [*] " << SystemCore::formatSize(offset) << " / " << SystemCore::formatSize(e.size) << "     " << flush;
                }
                if(!FlushFileBuffers(file) || diskError){cout << "\nLỗi ghi ổ đĩa.\n";break;}
            }
            if(offset<e.size && !canceled) {
                cout << "\n [*] Mất kết nối; thử tiếp tục " << attempt+1 << "/8 từ byte " << offset << "...\n";
                // Short interruptible backoff, preserving the open partial file.
                for(int tick=0;tick<20;++tick){if(_kbhit()){int key=_getch();if(key=='0' || key==27){canceled=true;break;}}Sleep(100);}
            }
        }
        if(offset!=e.size || canceled){cout << "\n [*] Đã giữ file .part; vào Nhận lại khi máy gửi sẵn sàng.\n";break;}
        if(!FlushFileBuffers(file) || sha256Handle(file)!=e.hash) {
            LARGE_INTEGER zero{};SetFilePointerEx(file,zero,nullptr,FILE_BEGIN);SetEndOfFile(file);
            cout << "\n [!] SHA-256 không khớp. Đã đặt lại tải dở để tải lại; chưa công nhận hoàn tất.\n";break;
        }
        if(!publish(file,finalPath)){cout << "\n [!] Không đổi tên được file; giữ .part để thử lại.\n";break;}
        ++complete;cout << "\n [✓] SHA-256 khớp: " << e.name << '\n';
    }
    cout << "\n [*] Hoàn tất " << complete << '/' << entries.size() << " file.\n";sc.waitEnter();
}
