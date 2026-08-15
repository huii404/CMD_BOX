// Internet.cpp
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

// Cache IP
static string cachedIP = "";
static time_t lastIPCheck = 0;
static const int IP_CACHE_TTL = 60;

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

Internet::Internet(SystemCore &s) : sc(s), listenSocket(INVALID_SOCKET), httpPort(8080) {
}

Internet::~Internet() {
    if (listenSocket != INVALID_SOCKET)
        closesocket(listenSocket);
}

// Lấy IP nội bộ
string Internet::getLocalIP() {
    time_t now = time(nullptr);
    
    if (!cachedIP.empty() && (now - lastIPCheck) < IP_CACHE_TTL) {
        return cachedIP;
    }
    
    // Thử qua gethostbyname
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
    
    // Thử qua GetAdaptersInfo
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD dwSize = sizeof(adapterInfo);
    DWORD dwRetVal = GetAdaptersInfo(adapterInfo, &dwSize);
    
    if (dwRetVal == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapter = adapterInfo;
        while (pAdapter) {
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
    
    // Fallback qua PowerShell
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

string Internet::getContentType(const string &fpath) {
    size_t dotPos = fpath.find_last_of(".");
    if (dotPos == string::npos) return "application/octet-stream";
    
    string ext = fpath.substr(dotPos);
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto it = contentTypeMap.find(ext);
    return (it != contentTypeMap.end()) ? it->second : "application/octet-stream";
}

string Internet::getField(const string &line) {
    size_t pos = line.find(":");
    if (pos != string::npos && pos + 2 < line.size()) return sc.trim(line.substr(pos + 2));
    return "";
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

void Internet::showNetworkInfo() {
    sc.cls();

    cout << "[*] Địa chỉ IP nội bộ (LAN): " << getLocalIP() << "\n";
    
    cout << "[*] Đang truy vấn Public IP (Internet)... ";
    FILE *pipe = _popen("curl -s --max-time 3 https://api.ipify.org", "r");
    if (pipe) {
        char buf[64] = {0};
        if (fgets(buf, sizeof(buf), pipe)) {
            string pubIp = sc.trim(string(buf));
            if (!pubIp.empty()) cout << pubIp << "\n";
            else cout << "(Không có kết nối Internet)\n";
        } else {
            cout << "(Không có kết nối Internet)\n";
        }
        _pclose(pipe);
    } else {
        cout << "(Không thể kết nối)\n";
    }

    cout << "\n[*] Cấu hình chi tiết các Card mạng (Network Adapters):\n";
    sc.runCMD("chcp 437 >nul & ipconfig /all | findstr /i \"Description IPv4 Subnet Default DNS Lease DHCP\" & chcp 65001 >nul");
    cout << "\n";
}

void Internet::repairNetwork() {
    sc.cls();

    cout << "[1/5] Xóa bộ đệm DNS (Flush DNS)...\n";
    sc.runCMD("ipconfig /flushdns >nul 2>&1");
    cout << "      -> [OK]\n";

    cout << "[2/5] Đặt lại Winsock Catalog...\n";
    sc.runAdmin("netsh winsock reset", true);
    cout << "      -> [OK]\n";

    cout << "[3/5] Đặt lại ngăn xếp giao thức TCP/IP...\n";
    sc.runAdmin("netsh int ip reset", true);
    cout << "      -> [OK]\n";

    cout << "[4/5] Xóa bảng ARP Cache...\n";
    sc.runAdmin("netsh interface ip delete arpcache", true);
    cout << "      -> [OK]\n";

    cout << "[5/5] Làm mới địa chỉ IP (Release & Renew)...\n";
    sc.runCMD("ipconfig /release >nul 2>&1 & ipconfig /renew >nul 2>&1");
    cout << "      -> [OK]\n";

    cout << "\n[✓] Đã hoàn tất sửa lỗi và khôi phục toàn bộ cài đặt mạng!\n";
}

void Internet::fullSecurityShield() {
    sc.cls();

    string batContent = "";
    batContent += "powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $false\"\n";
    batContent += "\"%ProgramFiles%\\Windows Defender\\MpCmdRun.exe\" -SignatureUpdate\n";
    batContent += "netsh advfirewall set allprofiles state on\n";
    batContent += "powershell -Command \"Set-MpPreference -EnableControlledFolderAccess Enabled\"\n";
    batContent += "sc config RemoteRegistry start= disabled\n";
    batContent += "sc stop RemoteRegistry\n";
    batContent += "sc config TermService start= disabled\n";
    batContent += "sc stop TermService\n";
    batContent += "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters\" /v SMB1 /t REG_DWORD /d 0 /f\n";
    batContent += "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient\" /v EnableMulticast /t REG_DWORD /d 0 /f\n";
    batContent += "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-NetAdapter -Name $_.Name -NetLuid $_.NetLuid -NetBIOSSetting Disabled }\"\n";

    vector<int> ports = {445, 139, 135, 137, 138};
    for (int p : ports) {
        string ruleName = "Block_Dangerous_Port_" + to_string(p);
        batContent += "netsh advfirewall firewall delete rule name=\"" + ruleName + "\"\n";
        batContent += "netsh advfirewall firewall delete rule name=\"" + ruleName + "_out\"\n";
        batContent += "netsh advfirewall firewall add rule name=\"" + ruleName + "\" dir=in action=block protocol=TCP localport=" + to_string(p) + "\n";
        batContent += "netsh advfirewall firewall add rule name=\"" + ruleName + "_out\" dir=out action=block protocol=TCP localport=" + to_string(p) + "\n";
    }

    batContent += "powershell -Command \"Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ServerAddresses ('1.1.1.1','1.0.0.1') }\"\n";
    batContent += "reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters\" /v EnableAutoDoh /t REG_DWORD /d 2 /f\n";
    batContent += "reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection\" /v AllowTelemetry /t REG_DWORD /d 0 /f\n";

    SystemCore::runBatchAsAdmin(batContent, "Tăng cường bảo mật");

    checkSecurityStatus();
    cout << "\n[✓] Đã kích hoạt toàn bộ lá chắn bảo mật hệ thống & mạng!\n";
}

void Internet::wifiAudit() {
    sc.cls();

    FILE *pipe = _popen("netsh wlan show profiles", "r");
    if (!pipe) {
        cout << "[!] Không thể truy cập cấu hình Wi-Fi.\n";
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
        cout << "[i] Không tìm thấy cấu hình Wi-Fi nào trên máy.\n";
        return;
    }

    for (size_t i = 0; i < wifiList.size(); ++i) {
        cout << " [" << i + 1 << "] " << wifiList[i] << "\n";
    }
    cout << " [0] Quay lại\n\n";

    int choice = sc.readInt(" -> Nhập số thứ tự Wi-Fi muốn xem mật khẩu: ");
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
    cout << " Tên Wi-Fi  : " << selectedWifi << "\n";
    cout << " Mật khẩu   : " << pass << "\n";
    cout << " Bảo mật    : " << auth << "\n";
    cout << " Mã hóa     : " << cipher << "\n\n";
}

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
    cout << "[Chat Server]: http://" << ip << ":" << httpPort << "\n";
    cout << "Các thiết bị trong cùng mạng Wi-Fi có thể truy cập link trên để chat.\n";
    cout << "Nhấn Ctrl+C để đóng server.\n\n";

    while (true) {
        SOCKET client = accept(listenSocket, nullptr, nullptr);
        if (client != INVALID_SOCKET) {
            thread([this, client]() { handleChatClient(client); }).detach();
        }
    }
}

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

            chatHistory.push_back(SystemCore::getTime(false) + " : " + decodedMsg);
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

void Internet::enableWindowsDefender() {
    sc.cls();
    sc.runAdmin("powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $false\"", true);
    sc.runCMD("cmd /c \"\"%ProgramFiles%\\Windows Defender\\MpCmdRun.exe\" -SignatureUpdate\"");
    cout << "[+] Windows Defender – bảo vệ thời gian thực đã bật.\n";
}

void Internet::enableFirewall() {
    sc.cls();
    sc.runAdmin("netsh advfirewall set allprofiles state on", true);
    cout << "[+] Tường lửa Windows đã bật cho tất cả profile.\n";
}

void Internet::enableControlledFolderAccess() {
    sc.cls();
    sc.runAdmin("powershell -Command \"Set-MpPreference -EnableControlledFolderAccess Enabled\"", true);
    cout << "[+] Truy cập thư mục có kiểm soát (Controlled Folder Access) đã bật.\n";
}

void Internet::disableInsecureProtocols() {
    sc.cls();
    sc.runAdmin("reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters\" /v SMB1 /t REG_DWORD /d 0 /f", true);
    sc.runAdmin("reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient\" /v EnableMulticast /t REG_DWORD /d 0 /f", true);
    string psNetBios = "Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-NetAdapter -Name $_.Name -NetLuid $_.NetLuid -NetBIOSSetting Disabled }";
    sc.runAdmin("powershell -Command \"" + psNetBios + "\"", true);
    cout << "[+] Đã vô hiệu hóa các giao thức không an toàn (SMB1, LLMNR, NetBIOS).\n";
}

void Internet::blockDangerousPorts() {
    sc.cls();
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
    sc.cls();
    string psDns = "Get-NetAdapter -Physical | Where-Object {$_.Status -eq 'Up'} | ForEach-Object { Set-DnsClientServerAddress -InterfaceIndex $_.InterfaceIndex -ServerAddresses ('1.1.1.1','1.0.0.1') }";
    sc.runAdmin("powershell -Command \"" + psDns + "\"", true);
    sc.runAdmin("reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters\" /v EnableAutoDoh /t REG_DWORD /d 2 /f", true);
    cout << "[+] Đã cấu hình DNS over HTTPS (Cloudflare 1.1.1.1 & 1.0.0.1).\n";
}

void Internet::checkSecurityStatus() {
    sc.cls();
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