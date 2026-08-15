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
    std::unique_ptr<Internet> internet;
    std::unique_ptr<SystemOptimizer> opt;
    std::unique_ptr<UtilityTools> tools;
    std::unique_ptr<MediaProcessor> media;
    
    std::mutex internetMutex;
    std::mutex optMutex;
    std::mutex toolsMutex;
    std::mutex mediaMutex;

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
    AppUI() = default;
    ~AppUI() = default;

    void mainMenu() {
        bool admin = SystemCore::isElevated();
        cout << "--- CMD BOX - SYSTEM TOOLKIT ---\n";
        if (admin) {
            cout << " [Trạng thái: Administrator ✓ (Đầy đủ quyền)]\n\n";
        } else {
            cout << " [Trạng thái: User ⚠️ (Khuyên chạy Run as Administrator)]\n\n";
        }
        cout << " [1] Bảo trì & Tối ưu hệ thống\n";
        cout << " [2] Mạng & Bảo mật\n";
        cout << " [3] Công cụ tự động & Tiện ích\n";
        cout << " [4] Xử lý Media (FFmpeg)\n";
        cout << " [0] Thoát\n\n";
        cout << " [Chọn]: ";
    }

    void menuBaoTriToiUu() {
        cls();
        cout << "--- BẢO TRÌ & TỐI ƯU HỆ THỐNG ---\n\n";
        cout << " [1] Dọn rác chuyên sâu PRO (Browser, Temp, Logs, Cache)\n";
        cout << " [2] Quản lý & Tắt ứng dụng khởi động cùng Windows\n";
        cout << " [3] Quản lý & Tối ưu dịch vụ Windows (Services Control)\n";
        cout << " [4] Tinh chỉnh giao diện & Tối ưu Taskbar Win 11\n";
        cout << " [5] Sửa lỗi Windows Update (Kẹt 0% / Lỗi dịch vụ)\n";
        cout << " [6] Tối ưu hóa tổng thể hệ thống (PRO 1-Click)\n";
        cout << " [0] Quay lại\n\n";
        cout << " [Chọn]: ";
    }

    void menuMangBaoMat() {
        cls();
        cout << "--- MẠNG & BẢO MẬT ---\n\n";
        cout << " [1] Xem thông tin mạng chi tiết (LAN, Public IP, DNS)\n";
        cout << " [2] Sửa lỗi & Khôi phục mạng (Reset TCP/IP, Winsock, DNS)\n";
        cout << " [3] Kích hoạt bảo mật toàn diện (Defender, Firewall, Port)\n";
        cout << " [4] Kiểm tra trạng thái bảo mật hệ thống\n";
        cout << " [5] Xem danh sách mật khẩu Wi-Fi đã lưu\n";
        cout << " [6] Phòng Chat nội bộ mạng LAN (Web Chat)\n";
        cout << " [0] Quay lại\n\n";
        cout << " [Chọn]: ";
    }

    void menuCongCuTienIch() {
        cls();
        cout << "--- CÔNG CỤ TỰ ĐỘNG & TIỆN ÍCH ---\n\n";
        cout << " [1] Auto Click chuột\n";
        cout << " [2] Spam Text (Tự động gửi tin nhắn/văn bản)\n";
        cout << " [3] Auto Paste dữ liệu nhiều dòng\n";
        cout << " [4] Trình tải & Cài đặt phần mềm tự động\n";
        cout << " [5] Gỡ bỏ ứng dụng rác mặc định (Bloatware Windows)\n";
        cout << " [0] Quay lại\n\n";
        cout << " [Chọn]: ";
    }

    void menuMedia() {
        cls();
        cout << "--- BỘ XỬ LÝ MEDIA (FFMPEG) ---\n\n";
        cout << " [1] Nén dung lượng Video / Ảnh\n";
        cout << " [2] Phục chế & Làm nét Video / Ảnh\n"; 
        cout << " [3] Chuyển đổi định dạng Mp4 -> Mp3 (Tách âm thanh)\n";
        cout << " [4] Thay đổi tốc độ Video\n";
        cout << " [5] Đổi đuôi định dạng Media\n";
        cout << " [6] Chuẩn hóa tên file trong thư mục\n";
        cout << " [7] Giấu file bí mật vào Ảnh\n"; 
        cout << " [8] Giấu file bí mật vào Video\n";
        cout << " [9] Dò tìm & Trích xuất file ẩn từ Media\n"; 
        cout << " [0] Quay lại\n\n";
        cout << " [Chọn]: ";
    }

    void run() {
        SetConsoleTitleA("CMD BOX - System Toolkit");
        Sleep(50);

        while (true) {
            cls();
            cout << "\n\n";
            mainMenu();
            int mainChoice = readInt("");
            
            if (mainChoice == 0) break;      
            if (mainChoice < 1 || mainChoice > 4) continue;

            int sub;
            switch (mainChoice) {

            // Bảo trì & Tối ưu
            case 1:
                while (true) {
                    cls();
                    menuBaoTriToiUu();
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    if (sub == 1)      getOptimizer().cleanDiskPro();
                    else if (sub == 2) getOptimizer().disableAllStartupApps();
                    else if (sub == 3) getOptimizer().turnOffServicesMenu();
                    else if (sub == 4) getOptimizer().optimizeTaskbar();
                    else if (sub == 5) getOptimizer().fixWindowsUpdate();
                    else if (sub == 6) getOptimizer().optimizeSystemPRO();
                    else {
                        cout << "\n[!] Lựa chọn không hợp lệ!\n";
                        Sleep(800);
                        continue;
                    }
                    waitEnter();
                }
                break;

            // Mạng & Bảo mật
            case 2:
                while (true) {
                    cls();
                    menuMangBaoMat();
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    if (sub == 1)      getInternet().showNetworkInfo();
                    else if (sub == 2) getInternet().repairNetwork();
                    else if (sub == 3) getInternet().fullSecurityShield();
                    else if (sub == 4) getInternet().checkSecurityStatus();
                    else if (sub == 5) getInternet().wifiAudit();
                    else if (sub == 6) getInternet().startLocalChat();
                    else {
                        cout << "\n[!] Lựa chọn không hợp lệ!\n";
                        Sleep(800);
                        continue;
                    }
                    waitEnter();
                }
                break;

            // Công cụ tự động & Tiện ích
            case 3:
                while (true) {
                    cls();
                    menuCongCuTienIch();
                    sub = readInt("");
                    if (sub == 0) break;

                    if (sub == 1)      getTools().autoClickPoint();
                    else if (sub == 2) getTools().spamText();
                    else if (sub == 3) getTools().autoPasteData();      
                    else if (sub == 4) getTools().downloadManager();
                    else if (sub == 5) getTools().uninstallBloatware();   
                    else {
                        cout << "\n[!] Lựa chọn không hợp lệ!\n";
                        Sleep(800);
                        continue;
                    }
                    waitEnter();
                }
                break;

            // Xử lý Media (FFmpeg)
            case 4:
                while (true) {
                    cls(); 
                    menuMedia();
                    sub = readInt("");
                    if (sub == 0) break; 

                    if (sub == 1)      getMedia().processMediaAuto(); 
                    else if (sub == 2) getMedia().processMediaEnhancementAuto();
                    else if (sub == 3) getMedia().processExtractAudioBatch();
                    else if (sub == 4) getMedia().processChangeSpeedBatch();
                    else if (sub == 5) getMedia().processConvertFormatBatch();
                    else if (sub == 6) getMedia().normalizeMediaFilenames();
                    else if (sub == 7) getMedia().hideFileInImage();   
                    else if (sub == 8) getMedia().hideFileInVideo();
                    else if (sub == 9) getMedia().extractHiddenFromMedia();
                    else {
                        cout << "\n[!] Lựa chọn không hợp lệ!\n";
                        Sleep(800);
                        continue;
                    }

                    if (sub != 1 && sub != 2)
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
    std::ios::sync_with_stdio(false);
    AppUI app;
    app.run();
    
    return 0;
}
