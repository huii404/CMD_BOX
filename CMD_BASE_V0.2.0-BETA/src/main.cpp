#include "../include/Internet.h"
#include <windows.h>
#include <iostream>
#include <limits>
#include <vector>
#include "../include/SystemCore.h"
#include "../include/Information.h"
#include "../include/Manager.h"
#include "../include/SystemOptimizer.h"
#include "../include/UtilityTools.h"
#include "../include/Chatbox.h"

using namespace std;
namespace fs = std::filesystem;

class AppUI : public SystemCore {
    Internet internet;
    Information info;
    Manager manager;
    SystemOptimizer opt;
    UtilityTools tools;
    Chatbox chatbox;
    

    bool titleSet = false;

public:
    AppUI(): internet(*this), info(*this), manager(*this), opt(*this, internet), tools(*this) {
        chatbox.start();
    }

    ~AppUI() {}
    

    void mainMenu() {
        cout << " [1] THÔNG TIN HỆ THỐNG\n";
        cout << " [2] BẢO TRÌ & TỐI ƯU\n";
        cout << " [3] MẠNG & CHIA SẺ\n";
        cout << " [4] CÔNG CỤ TỰ ĐỘNG\n";
        cout << " [5] ỨNG DỤNG & TIỆN ÍCH\n";
        cout << " [0] THOÁT\n";
        cout << "\n [Chọn]: ";
    }

    void menuThongTin(){
        cout << "\n [0] Quay lại\n";
        cout << " Chọn: ";
    }

    void menuBaoTriToiUu() { 
        cls(); 
        cout << " [1] Dọn rác hệ thống (PRO)\n";
        cout << " [2] Dọn rác hệ thống (BASE)\n";
        cout << " [3] Quét virus (Windows Defender)\n";
        cout << " [4] Kiểm tra file hệ thống (SFC)\n";
        cout << " [5] Kiểm tra ổ đĩa (CHKDSK)\n";
        cout << " [6] Tắt cài đặt app ngầm\n";
        cout << " [7] Tắt Hibernate\n";
        cout << " [8] Tắt Telemetry\n";
        cout << " [9] Tối ưu hệ thống (PRO)\n";
        cout << " [10] Tối ưu mạng + bảo mật (PRO)\n";
        cout << " [11] Tắt dịch vụ không cần thiết\n";
        cout << " [12] Tắt app khởi động cùng Windows\n";
        cout << " [13] Cập nhật tất cả ứng dụng\n";
        cout << " [14] Restart máy\n";
        cout << " [15] Sửa lỗi Windows Update (Kẹt 0%)\n";
        cout << " [0] Quay lại\n";
        cout << " [Chọn]: ";
    }
    
    void menuMangChiaSe() { 
        cls(); 
        cout << " [1] Xem thông tin IP\n";
        cout << " [2] Renew IP\n";
        cout << " [3] Xem mật khẩu WiFi đã lưu\n";
        cout << " [4] Flush DNS\n";
        cout << " [5] Reset TCP/IP\n";
        cout << " [6] Chia sẻ file qua Web (QuickShare)\n";
        cout << " [7] Chat Local trong mạng LAN\n";
        cout << " [0] Quay lại\n";
        cout << " [Chọn]: ";
    }
    
    void menuCongCuTuDong() { 
        cls(); 
        cout << " [1] Auto Click tại vị trí\n";
        cout << " [2] Spam Text\n";
        cout << " [3] Auto Paste dữ liệu (nhiều dòng)\n";
        cout << " [4] Tạo mã QR\n";
        cout << " [0] Quay lại\n";
        cout << " [Chọn]: ";
    }
    
    void menuUngDungTienIch() { 
        cls(); 
        cout << " [1] Tải và cài đặt ứng dụng (Chrome, Zalo, Discord...)\n";
        cout << " [2] Xóa app rác (Bloatware)\n";
        cout << " [3] Xóa cache trình duyệt (Chrome, Edge, CocCoc...)\n";
        cout << " [4] Chuyển bản Windows (Home -> Pro -> ...)\n";
        cout << " [0] Quay lại\n";
        cout << " [Chọn]: ";
    }

    void run(){
        int mainChoice;
        runCMD("title V0.2.0");
        while (true) {
            chatbox.resume(); // Bật chatbox trước
            cls();
            cout << "\n\n\n\n\n\n\n\n";
            mainMenu();
            mainChoice = readInt("");
            if (mainChoice == 0) break;
            if (mainChoice < 1 || mainChoice > 5) continue;

            chatbox.pause(); // Khóa ngầm ngay lập tức khi chọn Menu con
            Sleep(40);       // Chờ luồng phụ kịp đóng cổng vẽ trước khi xóa màn hình

            int sub;
            switch (mainChoice) {

            // ==================== CASE 1: THÔNG TIN HỆ THỐNG ====================
            case 1:
                while (true) {
                    info.showAllInfo();
                    menuThongTin(); 
                    sub = readInt(""); 
                    log(mainChoice, sub);
                    if (sub == 0) break;
                    waitEnter();
                } 
                break;

            // ==================== CASE 2: BẢO TRÌ & TỐI ƯU ====================
            case 2:
                while (true) {
                    menuBaoTriToiUu(); 
                    sub = readInt("");
                    log(mainChoice, sub);
                    if (sub == 0) break;
                    
                    if (sub == 1) opt.cleanDiskPro();
                    if (sub == 2) opt.cleanDiskBase();
                    if (sub == 3) {
                        int ans = readInt(" [1] Quét nhanh   [2] Quét toàn bộ   [0] Back: ");
                        if (ans == 0) continue;
                        if (ans == 1) opt.QuickScanVirus();
                        else opt.FullScanVirus();
                    }
                    if (sub == 4) runAdmin("sfc /scannow");
                    if (sub == 5) runAdmin("chkdsk C: /f /r");
                    if (sub == 6) opt.Consumer_Content();
                    if (sub == 7) opt.Hibernate();
                    if (sub == 8) opt.windowsTelemetry();
                    if (sub == 9) opt.optimizeSystemPRO();
                    if (sub == 10){opt.optimizeNetworkPRO(); opt.enableSecurityPRO();} 
                    if (sub == 11){cls();manager.turnOffServicesMenu();} 
                    if (sub == 12) opt.disableAllStartupApps();
                    if (sub == 13) opt.updateAllApps();
                    if (sub == 14) opt.restart();
                    if (sub == 15) opt.fixWindowsUpdate();
                    
                    waitEnter();
                } 
                break;

            // ==================== CASE 3: MẠNG & CHIA SẺ ====================
            case 3:
                while (true) {
                    menuMangChiaSe(); 
                    sub = readInt("");
                    log(mainChoice, sub);
                    if (sub == 0) break;
                    
                    if (sub == 1) internet.showIP();
                    if (sub == 2) internet.renewIP();
                    if (sub == 3) internet.wifiAudit();
                    if (sub == 4) internet.flushdns();
                    if (sub == 5) internet.netsh_tcpIP();
                    if (sub == 6) internet.quickSharePRO();
                    if (sub == 7) internet.startLocalChat();
                    
                    waitEnter();
                } 
                break;

            // ==================== CASE 4: CÔNG CỤ TỰ ĐỘNG ====================
            case 4:
                while (true) {
                    menuCongCuTuDong(); 
                    sub = readInt("");
                    log(mainChoice, sub);
                    if (sub == 0) break;
                    if (sub == 1) tools.autoClickPoint();
                    if (sub == 2) tools.spamText();
                    if (sub == 3) tools.autoPasteData();
                    if (sub == 4) {
                        cls();
                        int qrChoice = readInt("\n [1] Tạo 1 mã QR\n [2] Tạo nhiều mã QR\n [0] Back\n [Chọn]: ");
                        if (qrChoice == 0) continue;
                        if (qrChoice == 1) {
                            string line;
                            cin.ignore();
                            cout << " Nhập text: ";
                            getline(cin, line);
                            tools.ShowQR(line);
                        }
                        if (qrChoice == 2) {
                            int n = readInt(" Số lượng QR: ");
                            tools.ShowN_QR(n);
                        }
                    }
                    waitEnter();
                } 
                break;

            // ==================== CASE 5: ỨNG DỤNG & TIỆN ÍCH ====================
            case 5:
                while (true) {
                    menuUngDungTienIch(); 
                    sub = readInt("");
                    log(mainChoice, sub);
                    if (sub == 0) break;
                    if (sub == 1) tools.downloadManager();
                    if (sub == 2) tools.uninstallBloatware();
                    if (sub == 3) opt.clearBrowserCache();
                    if (sub == 4) opt.upgradeWindowsEditionPRO();
                    
                    waitEnter();
                } 
                break;
            }
        }
    }
};

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    AppUI app;
    app.run();
    return 0;
}