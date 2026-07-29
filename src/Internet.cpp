// Internet.cpp - PHẦN ĐẦU FILE
#include "../include/Internet.h"
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#include "../include/SystemCore.h"
#include <filesystem>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <thread>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <mutex>

namespace fs = std::filesystem;
using namespace std;

#pragma comment(lib, "ws2_32.lib")

// ==================== STATIC CACHE ====================
static string cachedIP = "";
static time_t lastIPCheck = 0;
static const int IP_CACHE_TTL = 60; // Cache IP trong 60 giây

static map<string, string> contentTypeMap = {
    {".pdf", "application/pdf"},
    {".txt", "text/plain"},
    {".html", "text/html"},
    {".zip", "application/zip"},
    {".mp4", "video/mp4"},
    {".mp3", "audio/mpeg"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".exe", "application/octet-stream"},
    {".gif", "image/gif"},
    {".webp", "image/webp"},
    {".json", "application/json"},
    {".xml", "application/xml"}
};

// ==================== CONSTRUCTOR/DESTRUCTOR ====================

Internet::Internet(SystemCore &s) : sc(s), listenSocket(INVALID_SOCKET), 
    httpPort(8080), shareSize(0), dlCount(0) {
    // KHÔNG làm gì nặng ở đây
}

Internet::~Internet() {
    if (listenSocket != INVALID_SOCKET)
        closesocket(listenSocket);
}

// ==================== TỐI ƯU getLocalIP() ====================

string Internet::getLocalIP() {
    time_t now = time(nullptr);
    
    // Cache IP trong 60 giây
    if (!cachedIP.empty() && (now - lastIPCheck) < IP_CACHE_TTL) {
        return cachedIP;
    }
    
    // Cách 1: Dùng API Windows (NHANH NHẤT)
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent* host = gethostbyname(hostname);
        if (host && host->h_addr_list && host->h_addr_list[0]) {
            struct in_addr addr;
            addr.s_addr = *(u_long*)host->h_addr_list[0];
            char* ip = inet_ntoa(addr);
            if (ip && strcmp(ip, "127.0.0.1") != 0) {
                cachedIP = ip;
                lastIPCheck = now;
                return cachedIP;
            }
        }
    }
    
    // Cách 2: Dùng GetAdaptersInfo (nhanh hơn PowerShell)
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD dwSize = sizeof(adapterInfo);
    DWORD dwRetVal = GetAdaptersInfo(adapterInfo, &dwSize);
    
    if (dwRetVal == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapter = adapterInfo;
        while (pAdapter) {
            // Chỉ lấy IP private (192.168.x.x, 10.x.x.x, 172.16.x.x)
            string ip = pAdapter->IpAddressList.IpAddress.String;
            if (ip.find("192.168.") == 0 || ip.find("10.") == 0 || 
                ip.find("172.16.") == 0 || ip.find("172.17.") == 0 ||
                ip.find("172.18.") == 0 || ip.find("172.19.") == 0) {
                if (ip != "0.0.0.0" && ip != "127.0.0.1") {
                    cachedIP = ip;
                    lastIPCheck = now;
                    return cachedIP;
                }
            }
            pAdapter = pAdapter->Next;
        }
    }
    
    // Fallback: PowerShell (CHẬM NHƯNG CHÍNH XÁC)
    FILE *pipe = _popen("powershell -NoProfile -Command \"(Get-NetIPAddress -AddressFamily IPv4 | Where-Object {$_.IPAddress -like '192.168.*' -or $_.IPAddress -like '10.*'} | Select-Object -First 1).IPAddress\"","r");
    if (pipe) {
        char buf[32] = {0};
        if (fgets(buf, sizeof(buf), pipe)) {
            string ip = sc.trim(string(buf));
            _pclose(pipe);
            if (!ip.empty() && ip != "0.0.0.0" && ip != "127.0.0.1") {
                cachedIP = ip;
                lastIPCheck = now;
                return cachedIP;
            }
        }
        _pclose(pipe);
    }
    
    return "127.0.0.1";
}

// ==================== TỐI ƯU getContentType() ====================

string Internet::getContentType(const string &fpath) {
    size_t dotPos = fpath.find_last_of(".");
    if (dotPos == string::npos) return "application/octet-stream";
    
    string ext = fpath.substr(dotPos);
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto it = contentTypeMap.find(ext);
    return (it != contentTypeMap.end()) ? it->second : "application/octet-stream";
}

// ==================== TỐI ƯU formatSize() ====================

string Internet::formatSize(long long b) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double sz = (double)b;
    int i = 0;
    while (sz >= 1024.0 && i < 4) {
        sz /= 1024.0;
        i++;
    }
    
    char buf[32];
    if (i == 0) {
        sprintf_s(buf, sizeof(buf), "%.0f %s", sz, units[i]);
    } else {
        sprintf_s(buf, sizeof(buf), "%.2f %s", sz, units[i]);
    }
    return string(buf);
}

// ==================== TỐI ƯU openFW() ====================

void Internet::openFW() {
    static bool firewallOpened = false;
    if (firewallOpened) return; // Chỉ mở 1 lần
    
    char cmd[256];
    sprintf_s(cmd, sizeof(cmd), "netsh advfirewall firewall add rule name=\"QuickShare_%d\" dir=in action=allow protocol=tcp localport=%d >nul 2>&1", httpPort, httpPort);
    system(cmd);
    firewallOpened = true;
}

// ==================== CÁC HÀM KHÁC (GIỮ NGUYÊN) ====================

string Internet::getField(const string &line) {
    size_t pos = line.find(":");
    if (pos != string::npos && pos + 2 < line.size()) return sc.trim(line.substr(pos + 2));
    return "";
}

string Internet::getTime() {
    time_t now = time(0);
    tm *t = localtime(&now);
    char buf[100];
    strftime(buf, sizeof(buf), "[%H:%M:%S]", t);
    return string(buf);
}

HTTPRequest Internet::parseReq(const string &raw) {
    HTTPRequest req;
    istringstream iss(raw);
    string line;
    if (getline(iss, line)) {
        istringstream ls(line);
        ls >> req.method >> req.path >> req.httpVersion;
    }
    while (getline(iss, line)) {
        line = sc.trim(line);
        if (line.empty()) break;
        size_t p = line.find(":");
        if (p != string::npos) {
            req.headers[sc.trim(line.substr(0, p))] = sc.trim(line.substr(p + 1));
        }
    }
    return req;
}

// ==================== sendFile() - TỐI ƯU ====================

void Internet::sendFile(SOCKET client) {
    ifstream f(sharePath, ios::binary);
    if (!f.good()) {
        string err = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 13\r\nConnection: close\r\n\r\nFile not found";
        send(client, err.c_str(), (int)err.length(), 0);
        return;
    }

    string ct = getContentType(sharePath);
    string hdr = "HTTP/1.1 200 OK\r\nContent-Type: " + ct + "\r\nContent-Length: " + to_string(shareSize) + 
                 "\r\nContent-Disposition: attachment; filename=\"" + shareName + "\"\r\nConnection: close\r\n\r\n";
    send(client, hdr.c_str(), (int)hdr.length(), 0);

    const int CHUNK = 1024 * 64;
    char buf[CHUNK];
    long long sent = 0;
    int prog = 0;
    cout << getTime() << " | Gửi: ";

    while (f.read(buf, CHUNK) || f.gcount() > 0) {
        int toSend = (int)f.gcount();
        if (send(client, buf, toSend, 0) <= 0) {
            cout << "\n⚠️  Gián đoạn\n";
            f.close();
            return;
        }
        sent += toSend;
        int np = (int)((sent * 100) / shareSize);
        if (np > prog && np % 10 == 0) {
            cout << np << "% ";
            cout.flush();
        }
        prog = np;
    }
    f.close();
    dlCount++;
    cout << "100% ✓ [" << dlCount << "]\n";
}

// ==================== quickSharePRO() - TỐI ƯU ====================

void Internet::quickSharePRO() {
    sc.cls();
    string path;
    cin.ignore();
    cout << "File (drag-drop or path): ";
    getline(cin, path);

    if (path.length() >= 2 && path[0] == '"' && path[path.length() - 1] == '"') {
        path = path.substr(1, path.length() - 2);
    }
    path = sc.trim(path);

    if (!getFileSizeInfoAndPrompt(path, shareSize)) {
        return;
    }

    sharePath = path;
    shareName = path.substr(path.find_last_of("/\\") + 1);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "WSAStartup failed\n";
        return;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        cout << "Socket failed\n";
        WSACleanup();
        return;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(httpPort);

    if (bind(listenSocket, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cout << "Bind failed: Port " << httpPort << " đang dùng\n";
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
        WSACleanup();
        return;
    }

    listen(listenSocket, SOMAXCONN);

    string ip = getLocalIP();
    openFW();

    cout << "\n" << string(70, '=') << "\n";
    cout << "Server chạy!\n";
    cout << "[IP]: " << ip << " | Port: " << httpPort << " | File: " << shareName << " (" << formatSize(shareSize) << ")\n\n";
    cout << "[URL]: http://" << ip << ":" << httpPort << "/" << shareName << "\n\n";
    cout << "\nCtrl+C để dừng\n";
    cout << string(70, '=') << "\n\n";

    while (true) {
        SOCKET client = accept(listenSocket, nullptr, nullptr);
        if (client != INVALID_SOCKET) {
            thread([this, client]() { handleClient(client); }).detach();
        }
    }
}

// ==================== CÁC HÀM KHÁC (GIỮ NGUYÊN) ====================

void Internet::handleClient(SOCKET client) {
    const int BUFSIZE = 4096;
    char buf[BUFSIZE];
    int rcv = recv(client, buf, BUFSIZE - 1, 0);

    if (rcv > 0) {
        buf[rcv] = '\0';
        HTTPRequest req = parseReq(buf);
        std::cout << getTime() << " | " << req.method << " " << req.path << "\n";

        if (req.method == "GET" && !sharePath.empty()) {
            // Decode URL: bỏ query string, decode %20, chuẩn hóa path
            string reqPath = req.path;
            size_t qPos = reqPath.find('?');
            if (qPos != string::npos) reqPath = reqPath.substr(0, qPos);

            // URL decode đơn giản
            string decoded;
            for (size_t i = 0; i < reqPath.size(); ++i) {
                if (reqPath[i] == '%' && i + 2 < reqPath.size()) {
                    int val;
                    sscanf(reqPath.substr(i + 1, 2).c_str(), "%x", &val);
                    decoded += static_cast<char>(val);
                    i += 2;
                } else if (reqPath[i] == '+') {
                    decoded += ' ';
                } else {
                    decoded += reqPath[i];
                }
            }

            // Chỉ cho phép đúng path của file đang chia sẻ
            string expectedPath = "/" + shareName;
            if (decoded == "/" || decoded == expectedPath) {
                sendFile(client);
            } else {
                string err = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/plain\r\nContent-Length: 9\r\nConnection: close\r\n\r\nForbidden";
                send(client, err.c_str(), (int)err.length(), 0);
                std::cout << getTime() << " | [!] Từ chối truy cập path: " << decoded << "\n";
            }
        }
    }
    closesocket(client);
}

bool Internet::checkFileSizeAndConfirm(const string &path, long long &outSize) {
    ifstream testFile(path, ios::binary);
    if (!testFile.good()) {
        cout << "[!] File không tồn tại!\n";
        return false;
    }

    testFile.seekg(0, ios::end);
    long long fileSize = testFile.tellg();
    testFile.close();

    if (fileSize == 0) {
        cout << "[!] File trống!\n";
        return false;
    }

    outSize = fileSize;

    if (fileSize > MAX_FILE_SIZE) {
        cout << "\n" << string(70, '=') << "\n";
        cout << "[⚠️  CẢNH BÁO] File vượt quá giới hạn an toàn!\n";
        cout << "Kích thước file: " << formatSize(fileSize) << "\n";
        cout << "Giới hạn tối đa:  " << formatSize(MAX_FILE_SIZE) << "\n";
        cout << string(70, '=') << "\n";

        string confirm;
        cout << "\n[?] Bạn vẫn muốn tiếp tục? (Y/N): ";
        cin >> confirm;
        cin.ignore();

        if (confirm != "y" && confirm != "Y") {
            cout << "[i] Hủy bỏ quá trình chia sẻ.\n";
            return false;
        }
    }

    return true;
}

bool Internet::getFileSizeInfoAndPrompt(const string &path, long long &outSize) {
    if (!checkFileSizeAndConfirm(path, outSize)) {
        return false;
    }

    cout << "\n[✓] Thông tin file:\n";
    cout << "    - Đường dẫn: " << path << "\n";
    cout << "    - Dung lượng: " << formatSize(outSize) << "\n";

    string proceedChoice;
    cout << "\n[?] Tiếp tục với quá trình chia sẻ? (Y/N): ";
    cin >> proceedChoice;
    cin.ignore();

    if (proceedChoice != "y" && proceedChoice != "Y") {
        cout << "[i] Người dùng đã hủy bỏ.\n";
        return false;
    }

    return true;
}

void Internet::showIP() {
    cout << "\n====================================================\n";
    cout << "             THÔNG TIN MẠNG IPV4 CỐT LÕI            \n";
    cout << "====================================================\n";
    sc.runCMD("chcp 437 >nul & ipconfig | findstr /i \"IPv4 Subnet Gateway\" & chcp 65001 >nul");
    cout << "====================================================\n";
}

void Internet::renewIP() { sc.runCMD("ipconfig /renew"); }
void Internet::flushdns() { sc.runCMD("ipconfig /flushdns"); }
void Internet::netsh_tcpIP() { sc.runAdmin("netsh int ip reset"); }

// ==================== wifiAudit() - TỐI ƯU ====================

void Internet::wifiAudit() {
    sc.cls();
    cout << "====================================================\n";
    cout << "          DANH SÁCH CẤU HÌNH WIFi ĐÃ LƯU            \n";
    cout << "====================================================\n";

    FILE *pipe = _popen("netsh wlan show profiles", "r");
    if (!pipe) {
        cout << "[!] Không thể truy cập cấu hình mạng không dây.\n";
        return;
    }

    char buf[512];
    vector<string> wifiList;
    while (fgets(buf, sizeof(buf), pipe)) {
        string line = buf;
        if (line.find("All User Profile") != string::npos) {
            string wifiName = getField(line);
            if (!wifiName.empty()) {
                wifiList.push_back(wifiName);
            }
        }
    }
    _pclose(pipe);

    if (wifiList.empty()) {
        cout << "[i] Không tìm thấy cấu hình WiFi nào được lưu trên máy.\n";
        return;
    }

    for (size_t i = 0; i < wifiList.size(); ++i) {
        cout << " [" << i + 1 << "] WiFi: " << wifiList[i] << "\n";
    }
    cout << " [0] Quay lại\n";
    cout << "====================================================\n";

    int choice = sc.readInt(" -> Nhập số thứ tự WiFi muốn xem mật khẩu: ");
    if (choice == 0) return;

    if (choice < 1 || choice > static_cast<int>(wifiList.size())) {
        cout << "[!] Lựa chọn không hợp lệ!\n";
        return;
    }

    string selectedWifi = wifiList[choice - 1];
    string cmd = "netsh wlan show profile \"" + selectedWifi + "\" key=clear";
    FILE *p2 = _popen(cmd.c_str(), "r");
    if (!p2) {
        cout << "[!] Thất bại khi truy vấn mật khẩu.\n";
        return;
    }

    char b2[512];
    string auth = "", cipher = "", pass = "(Mạng Mở/Open)";
    while (fgets(b2, sizeof(b2), p2)) {
        string inf = b2;
        if (inf.find("Authentication") != string::npos) auth = getField(inf);
        if (inf.find("Cipher") != string::npos) cipher = getField(inf);
        if (inf.find("Key Content") != string::npos) pass = getField(inf);
    }
    _pclose(p2);

    sc.cls();
    cout << "====================================================\n";
    cout << "          THÔNG TIN KẾT NỐI ĐƯỢC TRÍCH XUẤT         \n";
    cout << "====================================================\n";
    cout << " Tên WiFi   : " << selectedWifi << "\n";
    cout << " Mật khẩu   : " << pass << "\n";
    cout << " Bảo mật    : " << auth << "\n";
    cout << " Mã hóa     : " << cipher << "\n";
    cout << "====================================================\n";
}

// ==================== startLocalChat() - TỐI ƯU ====================

void Internet::startLocalChat() {
    sc.cls();
    httpPort = 9000;
    
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "[!] Không thể khởi tạo Winsock.\n";
        return;
    }
    
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        cout << "[!] Không thể tạo socket.\n";
        WSACleanup();
        return;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(httpPort);

    if (bind(listenSocket, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cout << "[!] Không thể bind port " << httpPort << "\n";
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
        WSACleanup();
        return;
    }
    
    listen(listenSocket, SOMAXCONN);

    string ip = getLocalIP();
    cout << "====================================================\n";
    cout << "[CHAT SERVER] DANG CHAY TAI: http://" << ip << ":" << httpPort << "\n";
    cout << "====================================================\n";
    cout << "Nguoi dung cung mang Wi-Fi co the truy cap link tren de chat.\n";
    cout << "Nhan [Ctrl + C] de dong Server khi ket thuc.\n";
    cout << "====================================================\n\n";

    while (true) {
        SOCKET client = accept(listenSocket, nullptr, nullptr);
        if (client != INVALID_SOCKET) {
            thread([this, client]() { handleChatClient(client); }).detach();
        }
    }
}

// ==================== handleChatClient() - TỐI ƯU ====================

void Internet::handleChatClient(SOCKET client) {
    char buf[4096];
    int rcv = recv(client, buf, sizeof(buf) - 1, 0);
    if (rcv <= 0) {
        closesocket(client);
        return;
    }
    buf[rcv] = '\0';

    HTTPRequest req = parseReq(buf);

    if (req.path == "/" || req.path == "/index.html") {
        // HTML minified để gửi nhanh hơn
        string html = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n"
                      "<html><head><title>C++ Local Chat</title>"
                      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                      "<style>body{font-family:sans-serif;background:#f0f2f5;display:flex;flex-direction:column;align-items:center;margin-top:20px}"
                      "#box{width:90%;max-width:400px;height:80vh;background:#fff;border-radius:10px;box-shadow:0 4px 10px rgba(0,0,0,0.1);display:flex;flex-direction:column}"
                      "#msgs{flex:1;overflow-y:auto;padding:15px;display:flex;flex-direction:column;gap:8px}"
                      ".msg{background:#e4e6eb;padding:8px 12px;border-radius:15px;width:fit-content;max-width:80%;word-wrap:break-word}"
                      "input{border:none;padding:15px;border-top:1px solid #ddd;outline:none;font-size:16px;border-bottom-left-radius:10px;border-bottom-right-radius:10px}</style></head>"
                      "<body><h2>Chat Local</h2><div id='box'><div id='msgs'></div>"
                      "<input type='text' id='inp' placeholder='Nhap tin nhan va nhan Enter...' onkeypress='send(event)'></div>"
                      "<script>let msgBox=document.getElementById('msgs');"
                      "function load(){fetch('/get').then(r=>r.text()).then(t=>{msgBox.innerHTML=t;msgBox.scrollTop=msgBox.scrollHeight;})}"
                      "function send(e){if(e.key==='Enter'&&e.target.value.trim()!==''){fetch('/send?m='+encodeURIComponent(e.target.value));e.target.value='';load()}}"
                      "setInterval(load,1500);</script></body></html>";
        send(client, html.c_str(), html.length(), 0);
    }
    else if (req.path.find("/send") == 0) {
        size_t pos = req.path.find("?m=");
        if (pos != string::npos) {
            string msg = req.path.substr(pos + 3);
            
            string decodedMsg = "";
            for (size_t i = 0; i < msg.length(); ++i) {
                if (msg[i] == '%') {
                    if (i + 2 < msg.length()) {
                        int value;
                        sscanf(msg.substr(i + 1, 2).c_str(), "%x", &value);
                        decodedMsg += static_cast<char>(value);
                        i += 2;
                    }
                } else if (msg[i] == '+') {
                    decodedMsg += ' ';
                } else {
                    decodedMsg += msg[i];
                }
            }

            chatHistory.push_back(getTime() + " : " + decodedMsg);
            cout << "[NEW MSG] " << decodedMsg << endl;
        }
        string ok = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
        send(client, ok.c_str(), ok.length(), 0);
    }
    else if (req.path == "/get") {
        string res = "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n";
        for (const auto& m : chatHistory) {
            res += "<div class='msg'>" + m + "</div>";
        }
        send(client, res.c_str(), res.length(), 0);
    }
    closesocket(client);
}

// ==================== BẢO MẬT NÂNG CAO (GIỮ NGUYÊN) ====================

void Internet::enableWindowsDefender() {
    sc.runAdmin("powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $false\"", true);
    sc.runCMD("cmd /c \"\"%ProgramFiles%\\Windows Defender\\MpCmdRun.exe\" -SignatureUpdate\"");
    cout << "[+] Windows Defender – bảo vệ thời gian thực đã bật.\n";
}

void Internet::enableFirewall() {
    sc.runAdmin("netsh advfirewall set allprofiles state on", true);
    cout << "[+] Tường lửa Windows đã bật cho tất cả profile.\n";
}

void Internet::enableControlledFolderAccess() {
    sc.runAdmin("powershell -Command \"Set-MpPreference -EnableControlledFolderAccess Enabled\"", true);
    cout << "[+] Truy cập thư mục có kiểm soát (Controlled Folder Access) đã bật.\n";
}

void Internet::disableInsecureProtocols() {
    sc.runAdmin("reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters\" /v SMB1 /t REG_DWORD /d 0 /f", true);
    sc.runAdmin("reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient\" /v EnableMulticast /t REG_DWORD /d 0 /f", true);
    string psNetBios = "Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-NetAdapter -Name $_.Name -NetLuid $_.NetLuid -NetBIOSSetting Disabled }";
    sc.runAdmin("powershell -Command \"" + psNetBios + "\"", true);
    cout << "[+] Đã vô hiệu hóa các giao thức không an toàn (SMB1, LLMNR, NetBIOS).\n";
}

void Internet::blockDangerousPorts() {
    // Các port nguy hiểm thông thường (chặn không hỏi)
    vector<int> normalPorts = {445, 139, 135, 137, 138};

    // Port 3389 (RDP) — hỏi riêng vì nhiều người đang dùng Remote Desktop
    cout << "\n[!] CẢNH BÁO: Port 3389 là Remote Desktop (RDP).\n";
    cout << "    Nếu bạn đang dùng Remote Desktop để điều khiển máy này từ xa,\n";
    cout << "    chặn port này sẽ NGẮT KẾT NỐI ngay lập tức!\n";
    cout << "[?] Bạn có muốn chặn port 3389 (RDP) không? (Y/N): ";
    string rdpAns;
    cin >> rdpAns;
    cin.ignore();
    bool blockRDP = (rdpAns == "y" || rdpAns == "Y");

    // Chặn các port thông thường
    for (int port : normalPorts) {
        string ruleName = "Block_Dangerous_Port_" + to_string(port);
        sc.runAdmin("netsh advfirewall firewall delete rule name=\"" + ruleName + "\"", true);
        sc.runAdmin("netsh advfirewall firewall delete rule name=\"" + ruleName + "_out\"", true);
        sc.runAdmin("netsh advfirewall firewall add rule name=\"" + ruleName + "\" dir=in action=block protocol=TCP localport=" + to_string(port), true);
        sc.runAdmin("netsh advfirewall firewall add rule name=\"" + ruleName + "_out\" dir=out action=block protocol=TCP localport=" + to_string(port), true);
    }
    cout << "[+] Đã chặn các cổng nguy hiểm (TCP: 445, 139, 135, 137, 138).\n";

    // Chặn RDP nếu người dùng xác nhận
    if (blockRDP) {
        string ruleName = "Block_Dangerous_Port_3389";
        sc.runAdmin("netsh advfirewall firewall delete rule name=\"" + ruleName + "\"", true);
        sc.runAdmin("netsh advfirewall firewall delete rule name=\"" + ruleName + "_out\"", true);
        sc.runAdmin("netsh advfirewall firewall add rule name=\"" + ruleName + "\" dir=in action=block protocol=TCP localport=3389", true);
        sc.runAdmin("netsh advfirewall firewall add rule name=\"" + ruleName + "_out\" dir=out action=block protocol=TCP localport=3389", true);
        cout << "[+] Đã chặn thêm port 3389 (RDP).\n";
    } else {
        cout << "[i] Bỏ qua port 3389 (RDP) — giữ nguyên cấu hình hiện tại.\n";
    }
}

void Internet::configureDNSoverHTTPS() {
    string psDns = "Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ServerAddresses ('1.1.1.1','1.0.0.1') }";
    sc.runAdmin("powershell -Command \"" + psDns + "\"", true);
    sc.runAdmin("reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters\" /v EnableAutoDoh /t REG_DWORD /d 2 /f", true);
    cout << "[+] Đã cấu hình DNS over HTTPS (Cloudflare 1.1.1.1 & 1.0.0.1).\n";
}

void Internet::checkSecurityStatus() {
    cout << "\n========== BÁO CÁO TRẠNG THÁI BẢO MẬT ==========\n";
    cout << "[*] Windows Defender:\n";
    sc.runCMD("powershell -Command \"Get-MpComputerStatus | Select-Object AntivirusEnabled, RealTimeProtectionEnabled, IoavProtectionEnabled\"");
    cout << "\n[*] Tường lửa Windows:\n";
    sc.runCMD("netsh advfirewall show allprofiles | findstr \"State\"");
    cout << "\n[*] Trạng thái các dịch vụ từ xa:\n";
    vector<string> svcs = {"RemoteRegistry", "TermService", "RasAuto", "RasMan"};
    for (auto &svc : svcs) {
        string cmd = "sc query " + svc + " | findstr STATE";
        sc.runCMD(cmd);
    }
    cout << "\n[*] Cấu hình DNS:\n";
    sc.runCMD("ipconfig /all | findstr \"DNS Servers\"");
    cout << "==================================================\n";
}