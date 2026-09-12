#include "LocalDrop.h"
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

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace fs = std::filesystem;
using namespace std;

static const int DEFAULT_TCP_PORT = 8888;
static const int DEFAULT_UDP_BEACON_PORT = 53318;
static const int CHUNK_SIZE = 262144; // 256 KB (Tối ưu thông lượng LAN & Wi-Fi)

// Gọi API miễn phí (qrenco.de) để lấy mã QR hiển thị trên console (timeout 1-2s tránh nghẽn)
static string fetchQrCodeApi(const string &targetUrl) {
    string cmd = "curl.exe -s --connect-timeout 1 --max-time 2 \"qrenco.de/" + targetUrl + "\"";
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

// Hàm nhận diện và xử lý đường dẫn file chính xác (hỗ trợ kéo thả, nháy kép, nháy đơn, Unicode UTF-8)
static bool resolveFilePath(string rawInput, string &outPath) {
    rawInput = SystemCore::trim(rawInput);
    if (rawInput.empty() || rawInput == "0" || rawInput == "q" || rawInput == "Q") {
        return false;
    }

    // Gỡ tiền tố drag-and-drop của PowerShell nếu có: & '...' hoặc & "..."
    if (rawInput.rfind("& ", 0) == 0) {
        rawInput = SystemCore::trim(rawInput.substr(2));
    }

    // Gỡ dấu ngoặc kép hoặc dấu nháy đơn khi kéo thả trên Windows / PowerShell
    while (rawInput.size() >= 2 && 
        ((rawInput.front() == '"' && rawInput.back() == '"') ||
         (rawInput.front() == '\'' && rawInput.back() == '\''))) {
        rawInput = rawInput.substr(1, rawInput.length() - 2);
        rawInput = SystemCore::trim(rawInput);
    }

    // 1. Kiểm tra với fs::u8path (hỗ trợ tiếng Việt có dấu)
    std::error_code ec;
    fs::path p1 = fs::u8path(rawInput);
    if (fs::exists(p1, ec) && !fs::is_directory(p1, ec)) {
        outPath = p1.u8string();
        return true;
    }

    // 2. Kiểm tra với fs::path thông thường
    fs::path p2(rawInput);
    if (fs::exists(p2, ec) && !fs::is_directory(p2, ec)) {
        outPath = p2.u8string();
        return true;
    }

    // 3. Fallback qua SystemCore::parsePaths
    vector<string> parsed = SystemCore::parsePaths(rawInput);
    if (!parsed.empty()) {
        fs::path p3 = fs::u8path(parsed[0]);
        if (fs::exists(p3, ec) && !fs::is_directory(p3, ec)) {
            outPath = p3.u8string();
            return true;
        }
    }

    return false;
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
    removeFirewallRule();
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

            // Kiểm tra tên mô tả card mạng để loại bỏ card ảo
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

            // Kiểm tra có Gateway hay không
            bool hasGateway = (pCurr->FirstGatewayAddress != NULL);
            if (!hasGateway) continue;

            // Duyệt danh sách địa chỉ Unicast IPv4
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

// ----------------------------------------------------------------------------------
// Quản lý Windows Firewall tự động
// ----------------------------------------------------------------------------------
void LocalDrop::addFirewallRule(int port) {
    string cmd = "netsh advfirewall firewall add rule name=\"CMDBOX_DROP\" dir=in action=allow protocol=TCP localport=" 
                 + to_string(port);
    SystemCore::runRawCommand(cmd);
}

void LocalDrop::removeFirewallRule() {
    SystemCore::runRawCommand("netsh advfirewall firewall delete rule name=\"CMDBOX_DROP\"");
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
        name = "CMD Box Client";
    } else {
        name = "Thiết bị mạng";
    }
    return clientIP + " (" + name + ")";
}

string LocalDrop::sanitizeFilename(const string &filename) {
    string safe = filename;
    for (char &c : safe) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    return safe;
}

string LocalDrop::getDownloadsFolder() {
    PWSTR path = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &path))) {
        char buf[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, path, -1, buf, MAX_PATH, NULL, NULL);
        CoTaskMemFree(path);
        return string(buf);
    }
    const char *userProf = getenv("USERPROFILE");
    if (userProf) return string(userProf) + "\\Downloads";
    return ".";
}

// ----------------------------------------------------------------------------------
// MENU CHÍNH CỦA LOCAL DROP
// ----------------------------------------------------------------------------------
void LocalDrop::menu() {
    while (true) {
        sc.cls();
        cout << "\n\n\n"
             << " [1] Public (Web QR)\n\n"
             << " Private (P2P)\n"
             << "   ├── [2] Send File\n"
             << "   └── [3] Receive File\n\n"
             << " [0] Return\n\n"
             << " [Chọn]: ";

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
    startSender(defaultFile, true); // Bảo mật: bắn beacon ngầm, không lộ QR/link
}

void LocalDrop::startSecureReceiver() {
    startReceiver();
}

// ----------------------------------------------------------------------------------
// MÁY PHÁT (SENDER): HTTP Chunked Stream + UDP Broadcast Beacon
// ----------------------------------------------------------------------------------
void LocalDrop::startSender(const string &defaultFile, bool isSecure) {
    sc.cls();
    string targetPath = defaultFile;

    if (targetPath.empty()) {
        while (true) {
            cout << "\n [CHỌN FILE TRUYỀN]\n"
                 << " Kéo thả file hoặc nhập đường dẫn (0: Hủy):\n"
                 << " [>] " << std::flush;

            string rawInput;
            if (!getline(cin, rawInput)) return;
            rawInput = SystemCore::trim(rawInput);
            if (rawInput.empty()) continue;
            if (rawInput == "0" || rawInput == "q" || rawInput == "Q") return;

            string resolved;
            if (resolveFilePath(rawInput, resolved)) {
                targetPath = resolved;
                break;
            } else {
                string cleanInput = rawInput;
                if (cleanInput.rfind("& ", 0) == 0) cleanInput = SystemCore::trim(cleanInput.substr(2));
                while (cleanInput.size() >= 2 && 
                    ((cleanInput.front() == '"' && cleanInput.back() == '"') ||
                     (cleanInput.front() == '\'' && cleanInput.back() == '\''))) {
                    cleanInput = cleanInput.substr(1, cleanInput.length() - 2);
                    cleanInput = SystemCore::trim(cleanInput);
                }
                std::error_code ec;
                fs::path dirP = fs::u8path(cleanInput);
                if (fs::exists(dirP, ec) && fs::is_directory(dirP, ec)) {
                    cout << " [!] Bạn vừa chọn một THƯ MỤC. Vui lòng chọn một FILE cụ thể để truyền!\n";
                } else {
                    cout << " [!] Không tìm thấy file: " << rawInput << "\n"
                         << " [!] Vui lòng kéo thả file vào cửa sổ hoặc kiểm tra lại đường dẫn.\n";
                }
            }
        }
    } else {
        string resolved;
        if (resolveFilePath(targetPath, resolved)) {
            targetPath = resolved;
        } else {
            cout << " [!] File không hợp lệ: " << targetPath << "\n";
            Sleep(1000);
            return;
        }
    }

    cout << "\n [*] Đang mở trạm phát" << std::flush;

    uintmax_t fileSizeBytes = fs::file_size(targetPath);
    string fileName = fs::path(targetPath).filename().u8string();
    string fileSizeFormatted = SystemCore::formatSize(fileSizeBytes);

    // Phát hiện IP LAN chuẩn
    string localIP = detectBestLANIP();
    int tcpPort = DEFAULT_TCP_PORT;

    // Tự động mở rule Windows Firewall
    addFirewallRule(tcpPort);

    // Khởi tạo Socket TCP Server (Bind 0.0.0.0 / INADDR_ANY)
    serverTcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverTcpSocket == INVALID_SOCKET) {
        cout << " [!] Lỗi khởi tạo socket TCP! Code: " << WSAGetLastError() << "\n";
        removeFirewallRule();
        sc.waitEnter();
        return;
    }

    BOOL opt = TRUE;
    setsockopt(serverTcpSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // BẮT BUỘC: 0.0.0.0, KHÔNG DÙNG 127.0.0.1
    serverAddr.sin_port = htons(tcpPort);

    if (::bind(serverTcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        // Nếu port 8888 bị bận, thử cổng tiếp theo
        tcpPort = 8889;
        serverAddr.sin_port = htons(tcpPort);
        if (::bind(serverTcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cout << " [!] Cổng " << DEFAULT_TCP_PORT << " & " << tcpPort << " đều đang bận!\n";
            closesocket(serverTcpSocket);
            serverTcpSocket = INVALID_SOCKET;
            removeFirewallRule();
            sc.waitEnter();
            return;
        }
        addFirewallRule(tcpPort);
    }

    if (listen(serverTcpSocket, 5) == SOCKET_ERROR) {
        cout << " [!] Lỗi listen trên socket TCP!\n";
        closesocket(serverTcpSocket);
        serverTcpSocket = INVALID_SOCKET;
        removeFirewallRule();
        sc.waitEnter();
        return;
    }

    // Khởi tạo UDP Socket để phát Beacon Broadcast
    beaconUdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (beaconUdpSocket != INVALID_SOCKET) {
        BOOL bcOpt = TRUE;
        setsockopt(beaconUdpSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&bcOpt, sizeof(bcOpt));
        DWORD udpTimeout = 200;
        setsockopt(beaconUdpSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&udpTimeout, sizeof(udpTimeout));
    }

    isRunning = true;
    string downloadUrl = "http://" + localIP + ":" + to_string(tcpPort) + "/download";
    string homeUrl = "http://" + localIP + ":" + to_string(tcpPort) + "/";

    string qrDisplay = "";
    if (!isSecure) {
        cout << "\n [*] Đang tạo mã QR" << std::flush;
        qrDisplay = fetchQrCodeApi(homeUrl);
        if (qrDisplay.empty()) {
            qrDisplay = "  [!] Không thể kết nối API tạo QR (Không sao cả!)\n"
                        "  [!] Mở trực tiếp link bên trên trên trình duyệt điện thoại/máy khác.\n";
        }
    }

    // Thread phát UDP Beacon CHỈ kích hoạt khi ở chế độ Bảo mật (isSecure)
    std::thread beaconThread;
    if (isSecure) {
        beaconThread = std::thread([&]() {
            sockaddr_in bcastAddr = {0};
            bcastAddr.sin_family = AF_INET;
            bcastAddr.sin_port = htons(DEFAULT_UDP_BEACON_PORT);
            bcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST); // 255.255.255.255

            string beaconMsg = "CMDBOX_BEACON|FILE|" + fileName + "|" + to_string(fileSizeBytes) + "|" 
                               + fileSizeFormatted + "|" + downloadUrl;

            while (isRunning) {
                if (beaconUdpSocket != INVALID_SOCKET) {
                    sendto(beaconUdpSocket, beaconMsg.c_str(), (int)beaconMsg.size(), 0, 
                           (sockaddr*)&bcastAddr, sizeof(bcastAddr));
                }
                for (int i = 0; i < 15 && isRunning; ++i) {
                    Sleep(100);
                }
            }
        });
    }

    // Vẽ giao diện Dashboard ban đầu (Chỉ vẽ khi có sự kiện lớn, không vẽ lại từng frame tránh chớp nháy)
    auto renderDashboard = [&](const string &connectedDevice = "Chờ kết nối", 
                                const string &statusText = "Chờ kết nối") {
        sc.cls();
        if (!isSecure) {
            cout << "\n --- CÔNG KHAI: WEB DROP QR ---\n"
                 << " [*] File : \x1b[93m" << fileName << " (" << fileSizeFormatted << ")\x1b[0m\n"
                 << " [*] Link : \x1b[96m" << homeUrl << "\x1b[0m\n"
                 << " [!] Quét QR hoặc mở link để tải (Phím 0: Thoát)\n\n"
                 << qrDisplay << "\n"
                 << " [*] Thiết bị: \x1b[96m" << connectedDevice << "\x1b[0m\n"
                 << " [*] Tiến độ : " << statusText << "\n" << std::flush;
        } else {
            cout << "\n --- BẢO MẬT: BẮN FILE P2P ---\n"
                 << " [*] File : \x1b[93m" << fileName << " (" << fileSizeFormatted << ")\x1b[0m\n"
                 << " [*] Kênh : Sóng Beacon ngầm (Chỉ tool CMD Box nhận diện)\n"
                 << " [!] Đang phát tín hiệu ngầm tới máy nhận (Phím 0: Thoát)\n\n"
                 << " [*] Thiết bị nhận: \x1b[96m" << connectedDevice << "\x1b[0m\n"
                 << " [*] Tiến độ : " << statusText << "\n" << std::flush;
        }
    };

    renderDashboard();

    // Vòng lặp lắng nghe kết nối TCP
    while (isRunning) {
        // Kiểm tra phím bấm không chặn (Non-blocking keyboard check)
        if (_kbhit()) {
            int key = _getch();
            if (key == '0' || key == 27 || key == 'q' || key == 'Q') {
                cout << "\n [!] Người dùng bấm dừng truyền file.\n" << std::flush;
                break;
            }
        }

        // Dùng select() kiểm tra kết nối với timeout 100ms
        // KHÔNG gọi accept() trực tiếp vì Winsock SO_RCVTIMEO không hỗ trợ accept()
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverTcpSocket, &readSet);
        timeval tv = {0, 100000}; // 100ms
        int selRet = select(0, &readSet, NULL, NULL, &tv);
        if (selRet <= 0) {
            continue; // Hết 100ms không có kết nối, tiếp tục vòng lặp để _kbhit() bắt phím 0
        }

        sockaddr_in clientAddr = {0};
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverTcpSocket, (sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET) {
            continue;
        }

        DWORD sockTimeout = 8000;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&sockTimeout, sizeof(sockTimeout));
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sockTimeout, sizeof(sockTimeout));

        // Tối ưu hóa bộ đệm Socket: 2MB send buffer + tắt thuật toán Nagle để tốc độ Wi-Fi/LAN tối đa
        int sndBuf = 2 * 1024 * 1024;
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sndBuf, sizeof(sndBuf));
        BOOL noDelay = TRUE;
        setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

        // Đọc HTTP Request Header
        char reqBuf[4096] = {0};
        int bytesRead = recv(clientSocket, reqBuf, sizeof(reqBuf) - 1, 0);
        if (bytesRead <= 0) {
            closesocket(clientSocket);
            continue;
        }
        string req(reqBuf, bytesRead);

        // Trích xuất User-Agent
        string userAgent = "";
        size_t uaPos = req.find("User-Agent:");
        if (uaPos != string::npos) {
            size_t uaEnd = req.find("\r\n", uaPos);
            userAgent = req.substr(uaPos + 11, (uaEnd != string::npos ? uaEnd - (uaPos + 11) : string::npos));
        }
        string deviceStr = parseDeviceName(userAgent, clientIP);

        // Kiểm tra đường dẫn yêu cầu HTTP
        bool isDownload = (req.find("GET /download") != string::npos || 
                           req.find("GET /get") != string::npos ||
                           req.find("CMDBOX_GET") != string::npos);
        
        // Nếu người dùng truy cập trang chủ / bằng trình duyệt điện thoại -> phục vụ Web Portal
        if (!isDownload && req.find("GET / ") != string::npos) {
            renderDashboard(deviceStr, "Đang xem trang chủ");

            std::ostringstream html;
            html << "<!DOCTYPE html><html lang=\"vi\"><head><meta charset=\"UTF-8\">"
                 << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                 << "<title>CMD Box - Local Web Drop</title>"
                 << "<style>"
                 << "body{margin:0;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,sans-serif;"
                 << "background:#0f172a;color:#f8fafc;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:16px;box-sizing:border-box;}"
                 << ".card{background:#1e293b;border:1px solid #334155;border-radius:24px;padding:32px 24px;max-width:480px;width:100%;box-shadow:0 25px 50px -12px rgba(0,0,0,0.5);text-align:center;}"
                 << ".icon{width:80px;height:80px;background:linear-gradient(135deg,#06b6d4,#3b82f6);border-radius:20px;margin:0 auto 20px;display:flex;align-items:center;justify-content:center;font-size:36px;box-shadow:0 10px 20px -5px rgba(59,130,246,0.5);}"
                 << "h1{font-size:22px;margin:0 0 8px;font-weight:700;word-break:break-word;color:#ffffff;}"
                 << ".badge{display:inline-block;background:#0369a1;color:#bae6fd;padding:6px 14px;border-radius:999px;font-size:13px;font-weight:600;margin-bottom:24px;}"
                 << ".btn{display:block;background:linear-gradient(135deg,#10b981,#059669);color:#ffffff;text-decoration:none;font-size:18px;font-weight:700;padding:18px 24px;border-radius:16px;box-shadow:0 10px 25px -5px rgba(16,185,129,0.4);transition:transform 0.1s ease;}"
                 << ".btn:active{transform:scale(0.98);}"
                 << ".note{font-size:12px;color:#94a3b8;margin-top:20px;line-height:1.5;}"
                 << "</style></head><body>"
                 << "<div class=\"card\">"
                 << "<div class=\"icon\">📦</div>"
                 << "<h1>" << fileName << "</h1>"
                 << "<div class=\"badge\">" << fileSizeFormatted << " • Tốc độ LAN siêu tốc</div>"
                 << "<a href=\"/download\" class=\"btn\" download>⚡ BẤM ĐỂ TẢI VỀ NGAY</a>"
                 << "<div class=\"note\">Truyền trực tiếp từ máy tính qua Wi-Fi nội bộ.<br>Không tốn dung lượng 4G/Internet ngoài.</div>"
                 << "</div></body></html>";

            string body = html.str();
            std::ostringstream resp;
            resp << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: text/html; charset=UTF-8\r\n"
                 << "Content-Length: " << body.size() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << body;

            string respStr = resp.str();
            send(clientSocket, respStr.c_str(), (int)respStr.size(), 0);
            closesocket(clientSocket);
            continue;
        }

        // BẮT ĐẦU TRUYỀN FILE THEO KHỐI 256KB (CHUNKED STREAM)
        std::ifstream file(targetPath, std::ios::binary);
        if (!file.is_open()) {
            string notFound = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            send(clientSocket, notFound.c_str(), (int)notFound.size(), 0);
            closesocket(clientSocket);
            continue;
        }

        // Vẽ Dashboard 1 lần duy nhất khi bắt đầu tải file
        renderDashboard(deviceStr, "Bắt đầu truyền");

        // Gửi HTTP Response Headers
        std::ostringstream respHeader;
        respHeader << "HTTP/1.1 200 OK\r\n"
                   << "Content-Type: application/octet-stream\r\n"
                   << "Content-Disposition: attachment; filename=\"" << fileName << "\"\r\n"
                   << "Content-Length: " << fileSizeBytes << "\r\n"
                   << "Connection: close\r\n"
                   << "Accept-Ranges: bytes\r\n\r\n";

        string headerStr = respHeader.str();
        send(clientSocket, headerStr.c_str(), (int)headerStr.size(), 0);

        // Buffer 256KB chống tràn RAM & tối ưu hóa thông lượng
        std::vector<char> buffer(CHUNK_SIZE);
        uintmax_t totalTransferred = 0;
        auto startTime = std::chrono::steady_clock::now();
        auto lastUiUpdate = startTime;
        uintmax_t lastBytes = 0;
        double currentSpeedMBps = 0.0;

        while (isRunning && (file.read(buffer.data(), CHUNK_SIZE) || file.gcount() > 0)) {
            // Kiểm tra phím hủy
            if (_kbhit()) {
                int key = _getch();
                if (key == '0' || key == 27) {
                    isRunning = false;
                    break;
                }
            }

            int toSend = (int)file.gcount();
            int sentSoFar = 0;

            // Vòng lặp gửi đảm bảo không bị partial send (theo Section V)
            while (sentSoFar < toSend && isRunning) {
                int bytesSent = send(clientSocket, buffer.data() + sentSoFar, toSend - sentSoFar, 0);
                if (bytesSent <= 0) {
                    goto client_disconnected;
                }
                sentSoFar += bytesSent;
                totalTransferred += bytesSent;
            }

            // Cập nhật giao diện tốc độ mỗi 150ms bằng \r (IN ĐÈ TẠI CHỖ, KHÔNG XÓA MÀN HÌNH -> KHÔNG CHỚP NHÁY)
            auto now = std::chrono::steady_clock::now();
            auto durationSinceLast = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUiUpdate).count();
            if (durationSinceLast >= 150 || totalTransferred == fileSizeBytes) {
                double sec = durationSinceLast / 1000.0;
                if (sec > 0) {
                    currentSpeedMBps = (totalTransferred - lastBytes) / (1024.0 * 1024.0 * sec);
                }
                lastUiUpdate = now;
                lastBytes = totalTransferred;
                double percent = (fileSizeBytes > 0) ? (totalTransferred * 100.0 / fileSizeBytes) : 100.0;
                if (percent > 100.0) percent = 100.0;

                int barWidth = 20;
                int filled = (int)(percent / 100.0 * barWidth);
                if (filled > barWidth) filled = barWidth;
                string bar = "";
                for (int i = 0; i < filled; ++i) bar += "█";
                for (int i = filled; i < barWidth; ++i) bar += "░";

                cout << "\r [*] Tiến độ : [" << bar << "] " 
                     << fixed << setprecision(1) << percent << "% (" 
                     << fixed << setprecision(1) << currentSpeedMBps << " MB/s)   " << std::flush;
            }
        }

client_disconnected:
        file.close();
        closesocket(clientSocket);

        if (totalTransferred >= fileSizeBytes) {
            cout << "\r [*] Tiến độ : [████████████████████] 100.0% \x1b[92m[HOÀN TẤT]\x1b[0m ("
                 << fixed << setprecision(1) << currentSpeedMBps << " MB/s)   \n"
                 << " \x1b[92m[✓] Đã gửi thành công tới " << deviceStr << "!\x1b[0m\n\n"
                 << " [*] Sẵn sàng cho lượt tải tiếp theo (Phím 0: Thoát)\n" << std::flush;
        } else {
            cout << "\n \x1b[93m[*] Ngắt kết nối. Sẵn sàng nhận lượt mới (Phím 0: Thoát).\x1b[0m\n" << std::flush;
        }
    }

    // Dọn dẹp an toàn khi dừng
    isRunning = false;
    if (beaconThread.joinable()) {
        beaconThread.join();
    }
    if (beaconUdpSocket != INVALID_SOCKET) {
        closesocket(beaconUdpSocket);
        beaconUdpSocket = INVALID_SOCKET;
    }
    if (serverTcpSocket != INVALID_SOCKET) {
        closesocket(serverTcpSocket);
        serverTcpSocket = INVALID_SOCKET;
    }
    removeFirewallRule();

    cout << "\n [*] Đã đóng trạm phát.\n";
    Sleep(500);
}

// ----------------------------------------------------------------------------------
// MÁY NHẬN (RECEIVER): Lắng nghe sóng UDP Beacon & Tải file trực tiếp vào ổ đĩa
// ----------------------------------------------------------------------------------
void LocalDrop::startReceiver() {
    sc.cls();
    cout << "\n --- BẢO MẬT: NHẬN FILE P2P ---\n"
         << " [*] Đang dò sóng Beacon ngầm từ tool phát (Phím 0: Thoát)\n\n";

    SOCKET recvUdp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (recvUdp == INVALID_SOCKET) {
        cout << " [!] Lỗi socket UDP! Code: " << WSAGetLastError() << "\n";
        sc.waitEnter();
        return;
    }

    BOOL opt = TRUE;
    setsockopt(recvUdp, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    DWORD timeoutMs = 400;
    setsockopt(recvUdp, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

    sockaddr_in bindAddr = {0};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port = htons(DEFAULT_UDP_BEACON_PORT);
    bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (::bind(recvUdp, (sockaddr*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR) {
        cout << " [!] Cổng UDP " << DEFAULT_UDP_BEACON_PORT << " đang bận.\n";
        closesocket(recvUdp);
        sc.waitEnter();
        return;
    }

    string localIP = detectBestLANIP();
    int animFrame = 0;
    const char *radarFrames[] = {"[ 📡 .   ]", "[  . 📡  ]", "[   . 📡 ]", "[  . 📡  ]"};

    char buffer[2048];
    sockaddr_in senderAddr = {0};
    int senderAddrLen = sizeof(senderAddr);

    string detectedFileName = "";
    uintmax_t detectedSize = 0;
    string detectedSizeStr = "";
    string detectedUrl = "";
    string senderIP = "";

    bool found = false;

    while (!found) {
        if (_kbhit()) {
            int key = _getch();
            if (key == '0' || key == 27) {
                cout << "\n [*] Đã dừng quét Beacon.\n";
                closesocket(recvUdp);
                Sleep(500);
                return;
            }
        }

        senderAddrLen = sizeof(senderAddr);
        int bytes = recvfrom(recvUdp, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&senderAddr, &senderAddrLen);

        if (bytes > 0) {
            buffer[bytes] = '\0';
            string msg(buffer);

            char sIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(senderAddr.sin_addr), sIP, sizeof(sIP));
            string fromIP(sIP);

            // Bỏ qua gói tin từ chính máy mình nếu IP trùng khớp
            if (fromIP == localIP && fromIP != "127.0.0.1") {
                continue;
            }

            // Phân tích cú pháp: CMDBOX_BEACON|FILE|<filename>|<bytes>|<size_str>|<url>
            if (msg.find("CMDBOX_BEACON|FILE|") == 0) {
                std::vector<string> parts;
                std::stringstream ss(msg);
                string item;
                while (getline(ss, item, '|')) {
                    parts.push_back(item);
                }

                if (parts.size() >= 6) {
                    detectedFileName = parts[2];
                    detectedSize = strtoull(parts[3].c_str(), NULL, 10);
                    detectedSizeStr = parts[4];
                    detectedUrl = parts[5];
                    senderIP = fromIP;
                    found = true;
                    break;
                }
            }
        }

        cout << "\r " << radarFrames[animFrame % 4] << " Đang dò sóng (Phím 0: Thoát)" << std::flush;
        animFrame++;
        Sleep(100);
    }

    closesocket(recvUdp);

    cout << "\n\n [!] TÌM THẤY THIẾT BỊ PHÁT:\n"
         << " [*] Máy : \x1b[96m" << senderIP << "\x1b[0m\n"
         << " [*] File: \x1b[93m" << detectedFileName << " (" << detectedSizeStr << ")\x1b[0m\n"
         << " Chấp nhận nhận file? [\x1b[92my\x1b[0m/N]: ";

    string ans;
    getline(cin, ans);
    ans = SystemCore::trim(ans);

    if (ans != "y" && ans != "Y") {
        cout << " [*] Đã hủy.\n";
        Sleep(500);
        return;
    }

    // Gửi UDP Unicast phản hồi CMDBOX_ACCEPT|OK
    SOCKET acceptSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (acceptSock != INVALID_SOCKET) {
        sockaddr_in respAddr = {0};
        respAddr.sin_family = AF_INET;
        respAddr.sin_port = htons(DEFAULT_UDP_BEACON_PORT);
        inet_pton(AF_INET, senderIP.c_str(), &respAddr.sin_addr);
        string ack = "CMDBOX_ACCEPT|OK";
        sendto(acceptSock, ack.c_str(), (int)ack.size(), 0, (sockaddr*)&respAddr, sizeof(respAddr));
        closesocket(acceptSock);
    }

    // Xác định đường dẫn lưu file trong Downloads
    string safeName = sanitizeFilename(detectedFileName);
    string downloadsDir = getDownloadsFolder();
    fs::path savePath = fs::path(downloadsDir) / safeName;

    // Tránh ghi đè file nếu đã tồn tại
    if (fs::exists(savePath)) {
        string stem = savePath.stem().u8string();
        string ext = savePath.extension().u8string();
        int counter = 1;
        while (fs::exists(fs::path(downloadsDir) / (stem + "_" + to_string(counter) + ext))) {
            counter++;
        }
        savePath = fs::path(downloadsDir) / (stem + "_" + to_string(counter) + ext);
    }

    cout << "\n [*] Đang tải về: \x1b[96m" << savePath.u8string() << "\x1b[0m\n\n";

    // Phân tích Host và Port từ URL (vd: http://192.168.1.100:8888/download)
    string host = senderIP;
    int port = DEFAULT_TCP_PORT;
    string path = "/download";

    size_t protoPos = detectedUrl.find("://");
    if (protoPos != string::npos) {
        string rem = detectedUrl.substr(protoPos + 3);
        size_t slashPos = rem.find('/');
        string hostPort = (slashPos != string::npos) ? rem.substr(0, slashPos) : rem;
        path = (slashPos != string::npos) ? rem.substr(slashPos) : "/download";

        size_t colonPos = hostPort.find(':');
        if (colonPos != string::npos) {
            host = hostPort.substr(0, colonPos);
            port = stoi(hostPort.substr(colonPos + 1));
        } else {
            host = hostPort;
        }
    }

    // Kết nối TCP tới máy phát
    SOCKET tcpClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcpClient == INVALID_SOCKET) {
        cout << " [!] Không thể tạo socket TCP!\n";
        sc.waitEnter();
        return;
    }

    sockaddr_in serverSockAddr = {0};
    serverSockAddr.sin_family = AF_INET;
    serverSockAddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serverSockAddr.sin_addr);

    if (connect(tcpClient, (sockaddr*)&serverSockAddr, sizeof(serverSockAddr)) == SOCKET_ERROR) {
        cout << " [!] Kết nối tới máy phát thất bại! Code: " << WSAGetLastError() << "\n";
        closesocket(tcpClient);
        sc.waitEnter();
        return;
    }

    // Gửi HTTP GET request
    std::ostringstream getReq;
    getReq << "GET " << path << " HTTP/1.1\r\n"
           << "Host: " << host << ":" << port << "\r\n"
           << "User-Agent: CMDBOX_CLI/1.0\r\n"
           << "Connection: close\r\n\r\n";

    string reqStr = getReq.str();
    send(tcpClient, reqStr.c_str(), (int)reqStr.size(), 0);

    // Mở file ghi trực tiếp vào ổ đĩa (Zero-Temp-File)
    std::ofstream outFile(savePath, std::ios::binary);
    if (!outFile.is_open()) {
        cout << " [!] Không thể tạo file đích để ghi: " << savePath.u8string() << "\n";
        closesocket(tcpClient);
        sc.waitEnter();
        return;
    }

    // Đọc header HTTP trước
    string headerBuffer;
    char ch;
    bool foundHeaderEnd = false;
    while (recv(tcpClient, &ch, 1, 0) == 1) {
        headerBuffer += ch;
        if (headerBuffer.size() >= 4 && 
            headerBuffer.substr(headerBuffer.size() - 4) == "\r\n\r\n") {
            foundHeaderEnd = true;
            break;
        }
    }

    if (!foundHeaderEnd) {
        cout << " [!] Không nhận được phản hồi HTTP hợp lệ từ máy phát.\n";
        outFile.close();
        fs::remove(savePath);
        closesocket(tcpClient);
        sc.waitEnter();
        return;
    }

    // Bắt đầu đọc stream dữ liệu và ghi thẳng vào ổ đĩa theo khối 64KB
    std::vector<char> recvBuffer(CHUNK_SIZE);
    uintmax_t receivedBytes = 0;
    auto startTime = std::chrono::steady_clock::now();
    auto lastUpdate = startTime;
    uintmax_t lastBytes = 0;
    double currentSpeedMBps = 0.0;

    while (true) {
        int bytes = recv(tcpClient, recvBuffer.data(), CHUNK_SIZE, 0);
        if (bytes <= 0) break;

        outFile.write(recvBuffer.data(), bytes);
        receivedBytes += bytes;

        auto now = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count();
        if (diff >= 200 || receivedBytes == detectedSize) {
            double sec = diff / 1000.0;
            if (sec > 0) currentSpeedMBps = (receivedBytes - lastBytes) / (1024.0 * 1024.0 * sec);
            lastUpdate = now;
            lastBytes = receivedBytes;

            double pct = (detectedSize > 0) ? (receivedBytes * 100.0 / detectedSize) : 0.0;
            if (pct > 100.0) pct = 100.0;
            int barWidth = 25;
            int filled = (int)(pct / 100.0 * barWidth);
            string bar = "";
            for (int i = 0; i < filled; ++i) bar += "█";
            for (int i = filled; i < barWidth; ++i) bar += "░";

            cout << "\r [*] Tiến độ: [" << bar << "] " 
                 << fixed << setprecision(1) << pct << "% (" 
                 << SystemCore::formatSize(receivedBytes) << ") "
                 << fixed << setprecision(1) << currentSpeedMBps << " MB/s   " << std::flush;
        }
    }

    outFile.close();
    closesocket(tcpClient);

    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
    double totalSec = totalTime / 1000.0;
    double avgSpeed = (totalSec > 0) ? (receivedBytes / (1024.0 * 1024.0 * totalSec)) : 0.0;

    cout << "\n\n [✓] TẢI XONG: \x1b[93m" << savePath.u8string() << "\x1b[0m (" 
         << SystemCore::formatSize(receivedBytes) << " - " 
         << fixed << setprecision(1) << avgSpeed << " MB/s)\n\n"
         << " [1] Mở thư mục chứa file\n"
         << " [0] Quay lại\n\n"
         << " [Chọn]: ";

    int postChoice = sc.readInt("");
    if (postChoice == 1) {
        string param = "/select,\"" + savePath.string() + "\"";
        ShellExecuteA(NULL, "open", "explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
    }
}
