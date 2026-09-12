#include "NetworkScanner.h"
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

string NetworkScanner::getLocalIP(SystemCore &core) {
    time_t now = time(nullptr);
    
    if (!cachedScannerIP.empty() && (now - lastScannerIPCheck) < IP_SCANNER_CACHE_TTL) {
        return cachedScannerIP;
    }
    
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD dwSize = sizeof(adapterInfo);
    DWORD dwRetVal = GetAdaptersInfo(adapterInfo, &dwSize);
    
    if (dwRetVal == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapter = adapterInfo;
        while (pAdapter) {
            string ip = pAdapter->IpAddressList.IpAddress.String;
            if (ip.find("192.168.") == 0 || ip.find("10.") == 0 || 
                ip.find("172.16.") == 0 || ip.find("172.17.") == 0 ||
                ip.find("172.18.") == 0 || ip.find("172.19.") == 0 ||
                ip.find("172.2") == 0 || ip.find("172.30.") == 0 || ip.find("172.31.") == 0) {
                if (ip != "0.0.0.0" && ip != "127.0.0.1") {
                    cachedScannerIP = ip;
                    lastScannerIPCheck = now;
                    return cachedScannerIP;
                }
            }
            pAdapter = pAdapter->Next;
        }
    }
    
    FILE *pipe = _popen("powershell -NoProfile -Command \"(Get-NetIPAddress -AddressFamily IPv4 | Where-Object {$_.IPAddress -like '192.168.*' -or $_.IPAddress -like '10.*' -or $_.IPAddress -like '172.*'} | Select-Object -First 1).IPAddress\"", "r");
    if (pipe) {
        char buf[32] = {0};
        if (fgets(buf, sizeof(buf), pipe)) {
            string ip = core.trim(string(buf));
            _pclose(pipe);
            if (!ip.empty() && ip != "0.0.0.0" && ip != "127.0.0.1") {
                cachedScannerIP = ip;
                lastScannerIPCheck = now;
                return cachedScannerIP;
            }
        }
        _pclose(pipe);
    }
    
    return "127.0.0.1";
}

bool NetworkScanner::isRandomizedPrivateMac(const string& mac) {
    if (mac.length() < 2) return false;
    char secondHex = (char)toupper((unsigned char)mac[1]);
    return (secondHex == '2' || secondHex == '6' || secondHex == 'A' || secondHex == 'E');
}

// Bảng tra cứu Vendor OUI (Hash Map O(1) tốc độ cực cao, đầy đủ các hãng phổ biến tại VN & quốc tế)
static const unordered_map<string, pair<string, string>>& getOuiDatabase() {
    static const unordered_map<string, pair<string, string>> ouiMap = {
        // Apple (iPhone, iPad, Mac, Apple Watch, Apple TV)
        {"A0BD1D", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"F01898", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"ACBC32", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"F8FFC2", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"0017F2", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"001CB3", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"0026BB", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"040CCE", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"087045", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"0CBC9F", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"1093E9", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"14109F", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"147DC5", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"20EE28", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"24A2E1", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"286ABA", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"2CF0EE", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"3090AB", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"3408BC", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"38F9D3", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"3C0754", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"3C22FB", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"3CD0F8", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"406C8F", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"40B395", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"48605F", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"50BC96", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"542696", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"5C95AE", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"60F81D", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"64B0A6", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"68AE20", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"6C4008", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"701124", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"70ECE4", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"74B587", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"784F43", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"7C6D62", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"80BE05", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"8489AD", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"88665A", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"8C8590", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"90DD5D", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"9801A7", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"9C207B", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"A483E7", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"A85B78", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"B418D1", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"B817C2", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"B8782E", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"BCD074", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"C09AD0", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"C4B301", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"C869CD", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"CC08FB", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"D0034B", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"D4909C", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"D89695", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"DCA904", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"E0B55F", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"E498D6", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"E8802E", {"Apple Device", "Apple iPhone / iPad / Mac"}},
        {"F4F15A", {"Apple Device", "Apple iPhone / iPad / Mac"}},

        // Samsung (Galaxy S, A, Note, Z Fold/Flip, Smart TV Samsung)
        {"503275", {"Samsung Galaxy", "Samsung (Android)"}},
        {"A4C494", {"Samsung Galaxy", "Samsung (Android)"}},
        {"342387", {"Samsung Galaxy", "Samsung (Android)"}},
        {"88329B", {"Samsung Galaxy", "Samsung (Android)"}},
        {"CC07AB", {"Samsung Galaxy", "Samsung (Android)"}},
        {"E458E7", {"Samsung Galaxy", "Samsung (Android)"}},
        {"784B87", {"Samsung Galaxy", "Samsung (Android)"}},
        {"404E36", {"Samsung Galaxy", "Samsung (Android)"}},
        {"0007AB", {"Samsung Galaxy", "Samsung (Android)"}},
        {"001247", {"Samsung Galaxy", "Samsung (Android)"}},
        {"001599", {"Samsung Galaxy", "Samsung (Android)"}},
        {"001D25", {"Samsung Galaxy", "Samsung (Android)"}},
        {"0023D7", {"Samsung Galaxy", "Samsung (Android)"}},
        {"08373D", {"Samsung Galaxy", "Samsung (Android)"}},
        {"103047", {"Samsung Galaxy", "Samsung (Android)"}},
        {"1449E0", {"Samsung Galaxy", "Samsung (Android)"}},
        {"18227E", {"Samsung Galaxy", "Samsung (Android)"}},
        {"20D390", {"Samsung Galaxy", "Samsung (Android)"}},
        {"286D97", {"Samsung Galaxy", "Samsung (Android)"}},
        {"30CDA7", {"Samsung Galaxy", "Samsung (Android)"}},
        {"380B40", {"Samsung Galaxy", "Samsung (Android)"}},
        {"40163B", {"Samsung Galaxy", "Samsung (Android)"}},
        {"444E1A", {"Samsung Galaxy", "Samsung (Android)"}},
        {"4844F7", {"Samsung Galaxy", "Samsung (Android)"}},
        {"508569", {"Samsung Galaxy", "Samsung (Android)"}},
        {"549B12", {"Samsung Galaxy", "Samsung (Android)"}},
        {"5CF8A1", {"Samsung Galaxy", "Samsung (Android)"}},
        {"60AF6D", {"Samsung Galaxy", "Samsung (Android)"}},
        {"68EBAE", {"Samsung Galaxy", "Samsung (Android)"}},
        {"70288B", {"Samsung Galaxy", "Samsung (Android)"}},
        {"7840E4", {"Samsung Galaxy", "Samsung (Android)"}},
        {"805B65", {"Samsung Galaxy", "Samsung (Android)"}},
        {"842519", {"Samsung Galaxy", "Samsung (Android)"}},
        {"8C7712", {"Samsung Galaxy", "Samsung (Android)"}},
        {"90F1AA", {"Samsung Galaxy", "Samsung (Android)"}},
        {"94652D", {"Samsung Galaxy", "Samsung (Android)"}},
        {"9852B1", {"Samsung Galaxy", "Samsung (Android)"}},
        {"A0821F", {"Samsung Galaxy", "Samsung (Android)"}},
        {"AC5F3E", {"Samsung Galaxy", "Samsung (Android)"}},
        {"B072BF", {"Samsung Galaxy", "Samsung (Android)"}},
        {"B479A7", {"Samsung Galaxy", "Samsung (Android)"}},
        {"BC4486", {"Samsung Galaxy", "Samsung (Android)"}},
        {"C0BDD1", {"Samsung Galaxy", "Samsung (Android)"}},
        {"C4731E", {"Samsung Galaxy", "Samsung (Android)"}},
        {"C81479", {"Samsung Galaxy", "Samsung (Android)"}},
        {"D0B128", {"Samsung Galaxy", "Samsung (Android)"}},
        {"D487D8", {"Samsung Galaxy", "Samsung (Android)"}},
        {"D857EF", {"Samsung Galaxy", "Samsung (Android)"}},
        {"DC7144", {"Samsung Galaxy", "Samsung (Android)"}},
        {"E09971", {"Samsung Galaxy", "Samsung (Android)"}},
        {"E47CF9", {"Samsung Galaxy", "Samsung (Android)"}},
        {"E8508B", {"Samsung Galaxy", "Samsung (Android)"}},
        {"F0728C", {"Samsung Galaxy", "Samsung (Android)"}},
        {"F409D8", {"Samsung Galaxy", "Samsung (Android)"}},
        {"F8042E", {"Samsung Galaxy", "Samsung (Android)"}},

        // Xiaomi / Redmi / POCO
        {"54AF97", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"640980", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"502B73", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"7C49EB", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"9C99A0", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"3480B3", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"186590", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"009EC8", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"04CF8C", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"14F65A", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"185936", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"286C07", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"38A4ED", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"40313C", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"50642B", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"584498", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"64CE00", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"742344", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"74A34A", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"8CBEBE", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"98FAE3", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"A4C361", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"ACC1EE", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"ACF6F7", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"B0E235", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"C40BCB", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"D4970B", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"E446DA", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"F460E2", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},
        {"F8A2D6", {"Xiaomi / Redmi", "Điện thoại / Thiết bị Xiaomi"}},

        // Oppo / Vivo / Realme / OnePlus / iQOO
        {"8090D0", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"E0191D", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"9C7142", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"600CB8", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"C0B5D5", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"1C521D", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"244BFE", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"2C598A", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"307512", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"347E5C", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"48137E", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"508F4C", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"646E97", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"902BD2", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"982CBC", {"OnePlus Phone", "Điện thoại OnePlus"}},
        {"A45B3D", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"B40B44", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"D4619D", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"F013C3", {"Oppo / Realme", "Điện thoại Oppo / Realme"}},
        {"102A97", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"18F0E4", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"205D49", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"3859F9", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"440444", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"54369B", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"64DB43", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"74AC5F", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"84DBAC", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"981DFA", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"B0D59D", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"C83DD4", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"D022BE", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"E89E09", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},
        {"F8E903", {"Vivo / iQOO", "Điện thoại Vivo / iQOO"}},

        // Huawei / Honor
        {"001E10", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"042528", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"0819A6", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"104780", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"1C1D67", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"24DF6A", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"342EB6", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"40B034", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"4846FB", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"548998", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"60E327", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"70723C", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"786A89", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"84A8E4", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"8853D4", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"9C2840", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"A4C7DE", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"AC853D", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"B4CD27", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"C07009", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"D8490B", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"E0247F", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},
        {"F8E811", {"Huawei / Honor", "Thiết bị Huawei / Honor"}},

        // Google (Pixel Phone, Nest, Chromecast, Home)
        {"001A11", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"18D6C7", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"20DFB9", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"3C5AB4", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"48D6D5", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"546009", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"703EAC", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"94EBCD", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"A47733", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"B827EB", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"D8EB97", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"E4F042", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"F4F5DB", {"Google Device", "Google Pixel / Nest / Chromecast"}},
        {"F80FF9", {"Google Device", "Google Pixel / Nest / Chromecast"}},

        // TP-Link / Mercusys
        {"F81A67", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"3C8CF8", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"74DA88", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"C0C9E3", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"50C7BF", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"000AEB", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"001478", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"001D0F", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"002586", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"002719", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"14CC20", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"1C3BF3", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"30DE4B", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"3C846A", {"TP-Link Device", "Bộ phát Wi-Fi / Camera Tapo"}},
        {"50D4F7", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"6032B1", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"6CA6B4", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"704F57", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"8416F9", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"984827", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"98DAC4", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"A0F3C1", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"B04E26", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"B09575", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"B4B024", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"C006C3", {"TP-Link Tapo", "Camera / Smart Plug Tapo"}},
        {"C025E9", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"C04A00", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"D807B6", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"D80D17", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"D84732", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"E4C32A", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"EC086B", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"F48CEB", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},
        {"F4F26D", {"TP-Link Device", "Bộ phát Wi-Fi / Router TP-Link"}},

        // Tenda / Totolink
        {"00B00C", {"Tenda Device", "Bộ phát Wi-Fi Tenda"}},
        {"0495E6", {"Tenda Device", "Bộ phát Wi-Fi Tenda"}},
        {"0840F3", {"Tenda Device", "Bộ phát Wi-Fi Tenda"}},
        {"58D9D5", {"Tenda Device", "Bộ phát Wi-Fi Tenda"}},
        {"946C65", {"Tenda Device", "Bộ phát Wi-Fi Tenda"}},
        {"C83A35", {"Tenda Device", "Bộ phát Wi-Fi Tenda"}},
        {"CC3429", {"Tenda Device", "Bộ phát Wi-Fi Tenda"}},
        {"E865D4", {"Tenda Device", "Bộ phát Wi-Fi Tenda"}},
        {"784476", {"Totolink Device", "Bộ phát Wi-Fi Totolink"}},
        {"D8FEE3", {"Totolink Device", "Bộ phát Wi-Fi Totolink"}},
        {"00085C", {"Totolink Device", "Bộ phát Wi-Fi Totolink"}},
        {"F0B429", {"Totolink Device", "Bộ phát Wi-Fi Totolink"}},

        // Thiết bị mạng Viettel / VNPT / FPT / Doanh nghiệp
        {"0017C2", {"VNPT iGate / Dasan", "Modem cáp quang VNPT / Dasan"}},
        {"002534", {"VNPT iGate", "Modem cáp quang VNPT iGate"}},
        {"48EE0C", {"VNPT iGate", "Modem cáp quang VNPT iGate"}},
        {"A06518", {"VNPT Technology", "Modem / Mesh Wi-Fi VNPT"}},
        {"B0C554", {"VNPT Technology", "Modem / Switch VNPT"}},
        {"40F201", {"VNPT Technology", "Modem cáp quang VNPT"}},
        {"282CB2", {"VNPT Technology", "Modem / Mesh Wi-Fi VNPT"}},
        {"58971E", {"VNPT Technology", "Modem / Mesh Wi-Fi VNPT"}},
        {"A41242", {"Viettel Telecom", "Modem Cáp quang Viettel"}},
        {"68DB54", {"Viettel Telecom", "Modem Cáp quang Viettel"}},
        {"786A1F", {"Viettel Telecom", "Modem Cáp quang Viettel"}},
        {"A8F94B", {"Viettel VHT", "Modem / Mesh Wi-Fi Viettel"}},
        {"C4B8B4", {"Viettel VHT", "Modem / Mesh Wi-Fi Viettel"}},
        {"38437D", {"Viettel VHT", "Modem / Mesh Wi-Fi Viettel"}},
        {"EC388F", {"Viettel VHT", "Modem / Mesh Wi-Fi Viettel"}},
        {"001E73", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"200889", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"34E0CF", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"908D78", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"B49842", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"002293", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"284153", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"681AB2", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"708A09", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"8CE081", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"C87B5B", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"DC028E", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"F47960", {"ZTE Modem", "Modem nhà mạng ZTE (Viettel/FPT)"}},
        {"00271C", {"Dasan Zhone", "Modem mạng Dasan"}},
        {"60F189", {"Dasan Zhone", "Modem mạng Dasan"}},
        {"001DAA", {"DrayTek Vigor", "Router doanh nghiệp DrayTek"}},
        {"00507F", {"DrayTek Vigor", "Router doanh nghiệp DrayTek"}},
        {"000C42", {"MikroTik Router", "Router MikroTik RouterBOARD"}},
        {"488F5A", {"MikroTik Router", "Router MikroTik RouterBOARD"}},
        {"64D154", {"MikroTik Router", "Router MikroTik RouterBOARD"}},
        {"744D28", {"MikroTik Router", "Router MikroTik RouterBOARD"}},
        {"B869F4", {"MikroTik Router", "Router MikroTik RouterBOARD"}},
        {"C4AD34", {"MikroTik Router", "Router MikroTik RouterBOARD"}},
        {"CC2DE0", {"MikroTik Router", "Router MikroTik RouterBOARD"}},
        {"D4CA6D", {"MikroTik Router", "Router MikroTik RouterBOARD"}},
        {"E48D8C", {"MikroTik Router", "Router MikroTik RouterBOARD"}},
        {"00156D", {"Ubiquiti UniFi", "Bộ phát UniFi / EdgeRouter"}},
        {"002722", {"Ubiquiti UniFi", "Bộ phát UniFi / EdgeRouter"}},
        {"24A43C", {"Ubiquiti UniFi", "Bộ phát UniFi / EdgeRouter"}},
        {"68D79A", {"Ubiquiti UniFi", "Bộ phát UniFi / EdgeRouter"}},
        {"70A741", {"Ubiquiti UniFi", "Bộ phát UniFi / EdgeRouter"}},
        {"7483C2", {"Ubiquiti UniFi", "Bộ phát UniFi / EdgeRouter"}},
        {"788A20", {"Ubiquiti UniFi", "Bộ phát UniFi / EdgeRouter"}},
        {"B4FBE4", {"Ubiquiti UniFi", "Bộ phát UniFi / EdgeRouter"}},
        {"DC9FDB", {"Ubiquiti UniFi", "Bộ phát UniFi / EdgeRouter"}},
        {"F09FC2", {"Ubiquiti UniFi", "Bộ phát UniFi / EdgeRouter"}},
        {"001AA9", {"Ruijie / Reyee", "Bộ phát Wi-Fi Ruijie / Reyee"}},
        {"00D0F8", {"Ruijie / Reyee", "Bộ phát Wi-Fi Ruijie / Reyee"}},
        {"14144B", {"Ruijie / Reyee", "Bộ phát Wi-Fi Ruijie / Reyee"}},
        {"34CE00", {"Ruijie / Reyee", "Bộ phát Wi-Fi Ruijie / Reyee"}},
        {"70AF6A", {"Ruijie / Reyee", "Bộ phát Wi-Fi Ruijie / Reyee"}},
        {"84D81B", {"Ruijie / Reyee", "Bộ phát Wi-Fi Ruijie / Reyee"}},
        {"F8A742", {"Ruijie / Reyee", "Bộ phát Wi-Fi Ruijie / Reyee"}},

        // Camera An ninh (Hikvision, Dahua, Ezviz, Imou, Yoosee)
        {"38AF29", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"4419B6", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"48EA63", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"600308", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"64DB8B", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"8CE748", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"A0BDCD", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"AC83F3", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"B0F963", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"C42F90", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"BC1401", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"D46E5C", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"F84D89", {"Hikvision / Ezviz", "Camera IP an ninh Ezviz"}},
        {"001A07", {"Dahua / Imou", "Camera IP an ninh Imou"}},
        {"3CEF8C", {"Dahua / Imou", "Camera IP an ninh Imou"}},
        {"402C76", {"Dahua / Imou", "Camera IP an ninh Imou"}},
        {"54C415", {"Dahua / Imou", "Camera IP an ninh Imou"}},
        {"686DBC", {"Dahua / Imou", "Camera IP an ninh Imou"}},
        {"9002A9", {"Dahua / Imou", "Camera IP an ninh Imou"}},
        {"B0C559", {"Dahua / Imou", "Camera IP an ninh Imou"}},
        {"E0508B", {"Dahua / Imou", "Camera IP an ninh Imou"}},
        {"F4F5E8", {"Dahua / Imou", "Camera IP an ninh Imou"}},
        {"001212", {"Yoosee Camera", "Camera IP giá rẻ Yoosee"}},
        {"001215", {"Yoosee Camera", "Camera IP giá rẻ Yoosee"}},
        {"001216", {"Yoosee Camera", "Camera IP giá rẻ Yoosee"}},
        {"001217", {"Yoosee Camera", "Camera IP giá rẻ Yoosee"}},
        {"34BA9D", {"Yoosee Camera", "Camera IP giá rẻ Yoosee"}},
        {"58639A", {"Yoosee Camera", "Camera IP giá rẻ Yoosee"}},

        // Smart TV & Media Box & Máy chơi game
        {"00014A", {"Sony Bravia / PS", "Smart TV Sony / PlayStation"}},
        {"00041F", {"Sony Bravia / PS", "Smart TV Sony / PlayStation"}},
        {"001315", {"Sony Bravia / PS", "Smart TV Sony / PlayStation"}},
        {"001DBA", {"Sony Bravia / PS", "Smart TV Sony / PlayStation"}},
        {"00248D", {"Sony Bravia / PS", "Smart TV Sony / PlayStation"}},
        {"10F96F", {"Sony Bravia / PS", "Smart TV Sony / PlayStation"}},
        {"30074D", {"Sony Bravia / PS", "Smart TV Sony / PlayStation"}},
        {"709E29", {"Sony Bravia / PS", "Smart TV Sony / PlayStation"}},
        {"94E6F7", {"Sony Bravia / PS", "Smart TV Sony / PlayStation"}},
        {"AC9B0A", {"Sony Bravia / PS", "Smart TV Sony / PlayStation"}},
        {"F8461C", {"Sony Bravia / PS", "Smart TV Sony / PlayStation"}},
        {"001FE2", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"002483", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"10683F", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"1868CB", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"203D66", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"3CCD36", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"40B0FA", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"58A2B5", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"700514", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"88366C", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"A823FE", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"AC8B03", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"C4366C", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"E85B5B", {"LG WebOS TV", "Smart TV LG (WebOS)"}},
        {"083E0C", {"TCL Smart TV", "Smart TV TCL / Android TV"}},
        {"2876CD", {"TCL Smart TV", "Smart TV TCL / Android TV"}},
        {"408BF6", {"TCL Smart TV", "Smart TV TCL / Android TV"}},
        {"50338B", {"TCL Smart TV", "Smart TV TCL / Android TV"}},
        {"6CF37F", {"TCL Smart TV", "Smart TV TCL / Android TV"}},
        {"D0D783", {"TCL Smart TV", "Smart TV TCL / Android TV"}},
        {"00264A", {"Casper / Skyworth", "Smart TV Casper / Skyworth"}},
        {"14F879", {"Casper / Skyworth", "Smart TV Casper / Skyworth"}},
        {"440377", {"Casper / Skyworth", "Smart TV Casper / Skyworth"}},
        {"C89346", {"Casper / Skyworth", "Smart TV Casper / Skyworth"}},
        {"0009BF", {"Nintendo Switch", "Máy chơi game Nintendo"}},
        {"98B6E9", {"Nintendo Switch", "Máy chơi game Nintendo"}},
        {"B87826", {"Nintendo Switch", "Máy chơi game Nintendo"}},
        {"CCFB65", {"Nintendo Switch", "Máy chơi game Nintendo"}},
        {"E84ECE", {"Nintendo Switch", "Máy chơi game Nintendo"}},

        // Máy tính, Laptop, Card mạng PC (Dell, HP, Lenovo, Asus, Acer, Intel, Realtek)
        {"78BE81", {"Máy tính (PC/Laptop)", "Máy tính (Lite-On / Acer / PC)"}},
        {"000C6E", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"0015F2", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"001E8C", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"04D9F5", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"08606E", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"107B44", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"1831BF", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"2C4D54", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"3085A9", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"382C4A", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"40167E", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"50465D", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"6045CB", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"704D7B", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"74D02B", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"90E6BA", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"A85E45", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"BCEE7B", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"C86000", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"D850E6", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"E03F49", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"F07959", {"Asus Laptop/PC", "Bo mạch / Laptop Asus ROG"}},
        {"000124", {"Acer Laptop/PC", "Máy tính / Laptop Acer"}},
        {"006067", {"Acer Laptop/PC", "Máy tính / Laptop Acer"}},
        {"00A060", {"Acer Laptop/PC", "Máy tính / Laptop Acer"}},
        {"485D60", {"Acer Laptop/PC", "Máy tính / Laptop Acer"}},
        {"80C5F2", {"Acer Laptop/PC", "Máy tính / Laptop Acer"}},
        {"B8763F", {"Acer Laptop/PC", "Máy tính / Laptop Acer"}},
        {"C09879", {"Acer Laptop/PC", "Máy tính / Laptop Acer"}},
        {"C89CDC", {"Acer Laptop/PC", "Máy tính / Laptop Acer"}},
        {"E06995", {"Acer Laptop/PC", "Máy tính / Laptop Acer"}},
        {"00065B", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"000874", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"000BDB", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"000D56", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"001143", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"001422", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"00188B", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"180373", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"1866DA", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"24B6FD", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"3417EB", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"484D7E", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"544810", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"74867A", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"847BEB", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"90B11C", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"A44CC8", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"B8AC6F", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"C8F750", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"D481D7", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"E4F004", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"F8BC12", {"Dell Laptop/PC", "Máy tính / Laptop Dell"}},
        {"0001E6", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"000802", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"000E7F", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"001635", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"001E0B", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"10604B", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"18A958", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"2C59E5", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"308D99", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"3C5282", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"40A8F0", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"480FCF", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"5820B1", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"68B599", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"705A0F", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"80C16E", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"9457A5", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"A0D3C1", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"B499BA", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"C8D3FF", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"D48564", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"E4115B", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"F40343", {"HP Laptop/PC", "Máy tính / Laptop HP"}},
        {"001A64", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"00215C", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"04766E", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"083E8E", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"1008B1", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"14ABC5", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"207693", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"286F7F", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"3052CB", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"3C970E", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"482CA0", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"503EAA", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"54EE75", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"606720", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"6C8814", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"707781", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"78028F", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"802BF9", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"8C8D28", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"98FA9B", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"B0359F", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"C85B76", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"D8CE3A", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"E0D55E", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"F0D5BF", {"Lenovo Laptop/PC", "Máy tính / Laptop Lenovo"}},
        {"0002B3", {"Intel Network", "Card mạng máy tính Intel"}},
        {"000347", {"Intel Network", "Card mạng máy tính Intel"}},
        {"000423", {"Intel Network", "Card mạng máy tính Intel"}},
        {"000CF1", {"Intel Network", "Card mạng máy tính Intel"}},
        {"000E0C", {"Intel Network", "Card mạng máy tính Intel"}},
        {"001302", {"Intel Network", "Card mạng máy tính Intel"}},
        {"001676", {"Intel Network", "Card mạng máy tính Intel"}},
        {"001B21", {"Intel Network", "Card mạng máy tính Intel"}},
        {"001E67", {"Intel Network", "Card mạng máy tính Intel"}},
        {"00216A", {"Intel Network", "Card mạng máy tính Intel"}},
        {"081196", {"Intel Network", "Card mạng máy tính Intel"}},
        {"3413E8", {"Intel Network", "Card mạng máy tính Intel"}},
        {"4851B7", {"Intel Network", "Card mạng máy tính Intel"}},
        {"6805CA", {"Intel Network", "Card mạng máy tính Intel"}},
        {"7CB0C2", {"Intel Network", "Card mạng máy tính Intel"}},
        {"8086F2", {"Intel Network", "Card mạng máy tính Intel"}},
        {"AC87A3", {"Intel Network", "Card mạng máy tính Intel"}},
        {"E82A44", {"Intel Network", "Card mạng máy tính Intel"}},
        {"00070E", {"Realtek LAN", "Card mạng máy tính Realtek"}},
        {"00E04C", {"Realtek LAN", "Card mạng máy tính Realtek"}},
        {"52544C", {"Realtek LAN", "Card mạng máy tính Realtek"}},
        {"0003FF", {"Microsoft Device", "Microsoft Surface / Hyper-V"}},
        {"00155D", {"Microsoft Hyper-V", "Máy ảo Microsoft Hyper-V"}},
        {"0017FA", {"Microsoft Device", "Microsoft Surface / PC"}},
        {"281878", {"Microsoft Device", "Microsoft Surface / PC"}},
        {"6045BD", {"Microsoft Device", "Microsoft Surface / PC"}},
        {"000569", {"VMware Virtual PC", "Máy ảo VMware Workstation"}},
        {"000C29", {"VMware Virtual PC", "Máy ảo VMware Workstation"}},
        {"005056", {"VMware Virtual PC", "Máy ảo VMware Workstation"}},
        {"080027", {"VirtualBox PC", "Máy ảo Oracle VirtualBox"}},

        // Smart Home IoT (Espressif ESP32/ESP8266, Tuya, Raspberry Pi)
        {"18FE34", {"Espressif IoT", "Thiết bị Smart Home (ESP8266)"}},
        {"240AC4", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"2462AB", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"246F28", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"2CF432", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"30AEA4", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"4022D8", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"485519", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"545AA6", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"68C63A", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"7CDFA1", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"84F3EB", {"Espressif IoT", "Thiết bị Smart Home (ESP8266)"}},
        {"A4CF12", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"B4E62D", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"CC50E3", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"DC4F22", {"Espressif IoT", "Thiết bị Smart Home (ESP32)"}},
        {"28CDC1", {"Raspberry Pi", "Máy tính mini Raspberry Pi"}},
        {"DC2632", {"Raspberry Pi", "Máy tính mini Raspberry Pi"}},
        {"E45F01", {"Raspberry Pi", "Máy tính mini Raspberry Pi"}},
        {"10521C", {"Tuya Smart", "Thiết bị thông minh Tuya IoT"}},
        {"708976", {"Tuya Smart", "Thiết bị thông minh Tuya IoT"}},
        {"840D8E", {"Tuya Smart", "Thiết bị thông minh Tuya IoT"}},
        {"D81F12", {"Tuya Smart", "Thiết bị thông minh Tuya IoT"}},

        // Máy in văn phòng (Canon, Epson, Brother, HP)
        {"000085", {"Canon Printer", "Máy in văn phòng Canon"}},
        {"001E8F", {"Canon Printer", "Máy in văn phòng Canon"}},
        {"180CAC", {"Canon Printer", "Máy in văn phòng Canon"}},
        {"7085C2", {"Canon Printer", "Máy in văn phòng Canon"}},
        {"84BA3B", {"Canon Printer", "Máy in văn phòng Canon"}},
        {"ACBCB0", {"Canon Printer", "Máy in văn phòng Canon"}},
        {"000048", {"Epson Printer", "Máy in phun Epson"}},
        {"0021B7", {"Epson Printer", "Máy in phun Epson"}},
        {"0026AB", {"Epson Printer", "Máy in phun Epson"}},
        {"64EB8C", {"Epson Printer", "Máy in phun Epson"}},
        {"9CB654", {"Epson Printer", "Máy in phun Epson"}},
        {"008077", {"Brother Printer", "Máy in Brother"}},
        {"30055C", {"Brother Printer", "Máy in Brother"}},
        {"485D36", {"Brother Printer", "Máy in Brother"}},
        {"E89E0C", {"Brother Printer", "Máy in Brother"}}
    };
    return ouiMap;
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
            unsigned short flags = *(unsigned short*)(buf + offset + 16);
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
    while (rIdx < ansIdx + dataLen) {
        unsigned char len = (unsigned char)recvBuf[rIdx++];
        if (len == 0) break;
        if ((len & 0xC0) == 0xC0) break;
        if (!hostname.empty()) hostname += ".";
        for (int i = 0; i < len && rIdx < ansIdx + dataLen; ++i) {
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
    PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(outBufLen);
    if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(outBufLen);
    }

    if (pAdapterInfo && GetAdaptersInfo(pAdapterInfo, &outBufLen) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
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
                break;
            }
            pAdapter = pAdapter->Next;
        }
    }
    if (pAdapterInfo) free(pAdapterInfo);

    if (myIP.empty()) {
        myIP = getLocalIP(sc);
    }

    if (myIP == "127.0.0.1" || myIP.empty()) {
        outElapsedSec = 0;
        return {};
    }

    size_t lastDot = myIP.rfind('.');
    if (lastDot == string::npos) {
        outElapsedSec = 0;
        return {};
    }

    string baseSubnet = myIP.substr(0, lastDot + 1);
    if (gatewayIP.empty()) {
        gatewayIP = baseSubnet + "1";
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

    // 2. GIAI ĐOẠN 1: QUÉT NHANH 254 IP BẰNG ICMP PING ĐA LUỒNG (128 THREADS, TIMEOUT 70MS)
    #pragma omp parallel for schedule(dynamic, 1) num_threads(128)
    for (int i = 1; i <= 254; ++i) {
        string curIP = baseSubnet + to_string(i);
        if (curIP == myIP) continue;

        IPAddr destIp = inet_addr(curIP.c_str());
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
                    if (ip.rfind(baseSubnet, 0) == 0 && !ip.empty() &&
                        (ip.length() < 4 || ip.rfind(".255") != ip.length() - 4) && row.dwPhysAddrLen == 6) {
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
        cout << "\n [*] Vui lòng đợi trong giây lát...\n";
        cout.flush();

        double elapsedSec = 0.0;
        vector<DiscoveredDevice> deviceList = performScan(elapsedSec);

        if (deviceList.empty()) {
            sc.cls();
            cout << " [!] Không phát hiện kết nối mạng cục bộ nào đang hoạt động!\n";
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

        while (true) {
            cout << " [1] Quét lại\n"
                 << " [0] Quay lại\n\n"
                 << " [Chọn]: ";

            int choice = sc.readInt("");
            if (choice == 0) return;
            if (choice == 1) break;
        }
    }
}
