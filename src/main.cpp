#include "../include/Internet.h"
#include <windows.h>
#include <iostream>
#include <limits>
#include <vector>
#include <memory>
#include <mutex>
#include "../include/SystemCore.h"
#include "../include/SystemOptimizer.h"
#include "../include/UtilityTools.h"
#include "../include/MediaProcessor.h"

using namespace std;
namespace fs = std::filesystem;

class AppUI : public SystemCore {
private:
    // Khởi tạo trễ (lazy) với unique_ptr giúp giải phóng tài nguyên lúc mở app
    std::unique_ptr<Internet> internet;
    std::unique_ptr<SystemOptimizer> opt;
    std::unique_ptr<UtilityTools> tools;
    std::unique_ptr<MediaProcessor> media;
    
    // TỐI ƯU CỐT LÕI: Tách riêng biệt từng Mutex để triệt tiêu lỗi Deadlock khi gọi lồng nhau
    std::mutex internetMutex;
    std::mutex optMutex;
    std::mutex toolsMutex;
    std::mutex mediaMutex;

    // Các hàm Getter thông minh - Khóa nào đi việc nấy
    Internet& getInternet() {
        if (!internet) {
            std::lock_guard<std::mutex> lock(internetMutex);
            if (!internet) {
                internet = std::make_unique<Internet>(*this);
            }
        }
        return *internet;
    }
    
    SystemOptimizer& getOptimizer() {
        if (!opt) {
            std::lock_guard<std::mutex> lock(optMutex);
            if (!opt) {
                // An toàn tuyệt đối: Gọi getInternet() tại đây không bị lock chéo nữa
                opt = std::make_unique<SystemOptimizer>(*this, getInternet());
            }
        }
        return *opt;
    }
    
    UtilityTools& getTools() {
        if (!tools) {
            std::lock_guard<std::mutex> lock(toolsMutex);
            if (!tools) {
                tools = std::make_unique<UtilityTools>(*this);
            }
        }
        return *tools;
    }
    
    MediaProcessor& getMedia() {
        if (!media) {
            std::lock_guard<std::mutex> lock(mediaMutex);
            if (!media) {
                media = std::make_unique<MediaProcessor>();
            }
        }
        return *media;
    }

public:
    AppUI() {
    }

    ~AppUI() = default;

    void mainMenu() {
        cout << " [1] BẢO TRÌ & TỐI ƯU\n";
        cout << " [2] MẠNG & CHIA SẺ\n";
        cout << " [3] CÔNG CỤ TỰ ĐỘNG\n";
        cout << " [4] ỨNG DỤNG & TIỆN ÍCH\n";
        cout << " [5] MEDIA\n";
        cout << " [0] THOÁT\n";
        cout << "\n [Chọn]: ";
    }

    void menuBaoTriToiUu() {
        cls();
        cout << " [1] Dọn rác (PRO)\n";
        cout << " [2] Dọn rác (BASE)\n";
        cout << " [3] Quét virus\n";
        cout << " [4] Kiểm tra file hệ thống\n";
        cout << " [5] Kiểm tra ổ đĩa\n";
        cout << " [6] Tắt cài đặt app ngầm\n";
        cout << " [7] Tắt Hibernate\n";
        cout << " [8] Tắt Telemetry\n";
        cout << " [9] Tối ưu hệ thống (PRO)\n";
        cout << " [10] Tối ưu mạng + bảo mật (PRO)\n";
        cout << " [11] Tắt dịch vụ không cần thiết\n";
        cout << " [12] Tắt app khởi động cùng Windows\n";
        cout << " [13] Cập nhật tất cả ứng dụng\n";
        cout << " [14] Sửa lỗi Windows Update (Kẹt 0%)\n";
        cout << " [15] Tối ưu Taskbar\n";
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
        cout << " [8] Bật Windows Defender\n";
        cout << " [9] Bật tường lửa\n";
        cout << " [10] Bật Controlled Folder Access\n";
        cout << " [11] Vô hiệu hóa giao thức không an toàn\n";
        cout << " [12] Chặn cổng nguy hiểm\n";
        cout << " [13] Cấu hình DNS over HTTPS\n";
        cout << " [14] Kiểm tra trạng thái bảo mật\n";
        cout << " [15] Tăng cường bảo mật (PRO)\n";
        cout << " [16] Tối ưu mạng & bảo mật (PRO)\n";
        cout << " [0] Quay lại\n";
        cout << " [Chọn]: ";
    }

    void menuCongCuTuDong() {
        cls();
        cout << " [1] Auto Click\n";
        cout << " [2] Spam Text\n";
        cout << " [3] Auto Paste\n";
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

    void Media() {
        cls();
        cout << " [1] Nén dung lượng\n";
        cout << " [2] Phục chế & Làm nét\n"; 
        cout << " [3] Mp4->Mp3\n";
        cout << " [4] Tốc độ Video\n";
        cout << " [5] Đổi đuôi file (giữ nguyên chất lượng)\n";
        cout << " [0] Quay lại\n\n";
        cout << " [Chọn]: ";
    }

    void run() {
        int mainChoice;
        SetConsoleTitleA("Github: huii404");
        Sleep(50);

        while (true) {
            cls();
            cout<<"\n\n\n";
            mainMenu();
            mainChoice = readInt("");
            
            if (mainChoice == 0) {
                break;
            }
            if (mainChoice < 1 || mainChoice > 5) continue;

            int sub;
            switch (mainChoice) {

            // CASE 1: BẢO TRÌ & TỐI ƯU 
            case 1:
                while (true) {
                    cls();
                    menuBaoTriToiUu();
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    if (sub == 1) getOptimizer().cleanDiskPro();
                    else if (sub == 2) getOptimizer().cleanDiskBase();
                    else if (sub == 3) {
                        int ans = readInt("[1] Quét nhanh    [2] Quét toàn bộ     [0] Back: ");
                        if (ans == 0) continue;
                        if (ans == 1) getOptimizer().QuickScanVirus();
                        else getOptimizer().FullScanVirus();
                    }
                    else if (sub == 4) runAdmin("sfc /scannow");
                    else if (sub == 5) runAdmin("chkdsk C: /f /r");
                    else if (sub == 6) getOptimizer().Consumer_Content();
                    else if (sub == 7) getOptimizer().Hibernate();
                    else if (sub == 8) getOptimizer().windowsTelemetry();
                    else if (sub == 9) getOptimizer().optimizeSystemPRO();
                    else if (sub == 10) {
                        getOptimizer().optimizeNetworkPRO();
                        getOptimizer().enableSecurityPRO();
                    }
                    else if (sub == 11) getOptimizer().turnOffServicesMenu();
                    else if (sub == 12) getOptimizer().disableAllStartupApps();
                    else if (sub == 13) getOptimizer().updateAllApps();
                    else if (sub == 14) getOptimizer().fixWindowsUpdate();
                    else if (sub == 15) getOptimizer().optimizeTaskbar();

                    waitEnter();
                }
                break;

            // CASE 2: MẠNG & CHIA SẺ
            case 2:
                while (true) {
                    cls();
                    menuMangChiaSe();
                    sub = readInt("");
                    if (sub == 0) break;
                

                    if (sub == 1) getInternet().showIP();
                    else if (sub == 2) getInternet().renewIP();
                    else if (sub == 3) getInternet().wifiAudit();
                    else if (sub == 4) getInternet().flushdns();
                    else if (sub == 5) getInternet().netsh_tcpIP();
                    else if (sub == 6) getInternet().quickSharePRO();
                    else if (sub == 7) getInternet().startLocalChat();
                    else if (sub == 8) {
                        cls();
                        getInternet().enableWindowsDefender();
                    }
                    else if (sub == 9) {
                        cls();
                        getInternet().enableFirewall();
                    }
                    else if (sub == 10) {
                        cls();
                        getInternet().enableControlledFolderAccess();
                    }
                    else if (sub == 11) {
                        cls();
                        getInternet().disableInsecureProtocols();
                    }
                    else if (sub == 12) {
                        cls();
                        getInternet().blockDangerousPorts();
                    }
                    else if (sub == 13) {
                        cls();
                        getInternet().configureDNSoverHTTPS();
                    }
                    else if (sub == 14) {
                        cls();
                        getInternet().checkSecurityStatus();
                    }
                    else if (sub == 15) {
                        cls();
                        getInternet().enableWindowsDefender();
                        getInternet().enableFirewall();
                        getInternet().enableControlledFolderAccess();
                        getInternet().disableInsecureProtocols();
                        getInternet().blockDangerousPorts();
                        getInternet().configureDNSoverHTTPS();
                        getInternet().checkSecurityStatus();
                    }
                    else if (sub == 17) {
                        cls();
                        getInternet().flushdns();
                        getInternet().netsh_tcpIP();
                        getInternet().disableInsecureProtocols();
                        getInternet().blockDangerousPorts();
                        getInternet().configureDNSoverHTTPS();
                        getInternet().checkSecurityStatus();
                    }
                    waitEnter();
                }
                break;

            // CASE 3: CÔNG CỤ TỰ ĐỘNG 
            case 3:
                while (true) {
                    cls();
                    menuCongCuTuDong();
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    if (sub == 1) getTools().autoClickPoint();
                    else if (sub == 2) getTools().spamText();
                    else if (sub == 3) getTools().autoPasteData();
                    else if (sub == 4) {
                        cls();
                        int qrChoice = readInt("\n [1] Tạo 1 mã QR\n [2] Tạo nhiều mã QR\n [0] Back\n [Chọn]: ");
                        if (qrChoice == 0) continue;
                        if (qrChoice == 1) {
                            string line;
                            cin.ignore();
                            cout << " Nhập text: ";
                            getline(cin, line);
                            getTools().ShowQR(line);
                        }
                        if (qrChoice == 2) {
                            int n = readInt(" Số lượng QR: ");
                            getTools().ShowN_QR(n);
                        }
                    }
                    waitEnter();
                }
                break;

            // CASE 4: ỨNG DỤNG & TIỆN ÍCH 
            case 4:
                while (true) {
                    cls();
                    menuUngDungTienIch();
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    if (sub == 1) getTools().downloadManager();
                    else if (sub == 2) getTools().uninstallBloatware();
                    else if (sub == 3) getOptimizer().clearBrowserCache();
                    else if (sub == 4) getOptimizer().upgradeWindowsEditionPRO();

                    waitEnter();
                }
                break;

            // CASE 5: XỬ LÝ MEDIA (FFMPEG)
            case 5:
                while (true) {
                    cls(); 
                    Media();
                    sub = readInt("");
                    if (sub == 0) break; 

                    if (sub == 1)
                        getMedia().processMediaAuto(); 
                    else if (sub == 2)
                        getMedia().processMediaEnhancementAuto();
                    else if (sub == 3)
                        getMedia().processExtractAudioBatch();
                    else if (sub == 4)
                        getMedia().processChangeSpeedBatch();
                    else if (sub == 5)
                        getMedia().processConvertFormatBatch();

                    if (sub != 1 && sub != 2)
                        waitEnter();
                }
                break;
            }
        }
    }
};

int main() {
    // === TỐI ƯU I/O ===
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // Tăng tốc C++ I/O 
    std::ios::sync_with_stdio(false);
    AppUI app;
    app.run();
    
    return 0;
}