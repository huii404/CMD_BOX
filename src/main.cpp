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
    AppUI() {
        std::thread([this]() {
            getMedia();
        }).detach();
    }
    ~AppUI() = default;

    void renderStatusBox() {
        bool admin = SystemCore::isElevated();
        std::string devInfo = SystemCore::getDeviceStatus();
        cout << "Quyền hạn : " << (admin ? "Administrator" : "User") << "\n"
             << "Thiết bị  : " << devInfo << "\n";
    }

    void mainMenu() {
        renderStatusBox();
        cout << "\n\n\n"
             << " [1] Bảo trì & Tối ưu hệ thống\n"
             << " [2] Mạng & Bảo mật\n"
             << " [3] Công cụ tự động & Tiện ích\n"
             << " [4] Xử lý Media\n"
             << " [5] Trợ lý ảo AI (Virtual Assistant)\n"
             << " [0] Thoát\n\n"
             << " [Chọn]: ";
    }

    void launchAssistant() {
        fs::path scriptPath;
        for (const auto& p : {
            fs::current_path() / "scripts" / "assistant.py",
            fs::current_path().parent_path() / "scripts" / "assistant.py",
            fs::path("scripts/assistant.py")
        }) {
            if (fs::exists(p)) { scriptPath = p; break; }
        }

        if (scriptPath.empty()) {
            cls();
            cout << "\n [!] Không tìm thấy tập tin scripts/assistant.py!\n";
            Sleep(1500);
            return;
        }

        string pyExe = SystemCore::runRawCommand("where python >nul 2>nul") ? "python" :
                       SystemCore::runRawCommand("where py >nul 2>nul") ? "py" : "";

        if (pyExe.empty()) {
            cls();
            cout << " [!] Trợ lý ảo AI yêu cầu Python (3.8+) để hoạt động.\n"
                 << " Bạn có thể cài đặt từ Microsoft Store hoặc qua Menu [3] -> [4].\n\n";
            SystemCore::waitEnter();
            return;
        }

        system(("start \"CMD BOX - Tro Ly Ao AI\" " + pyExe + " \"" + scriptPath.string() + "\"").c_str());
        cout << "\n [✓] Đã mở cửa sổ Trợ lý ảo AI song song.\n\n";
        Sleep(1000);
    }

    void menuBaoTriToiUu() {
        cls();
        cout << " [1] Dọn rác nhanh \n"
             << " [2] Dọn rác chuyên sâu \n"
             << " [3] Tắt ứng dụng khởi động\n"
             << " [4] Tối ưu dịch vụ Windows\n"
             << " [5] Chỉnh giao diện & Taskbar Win 11\n"
             << " [6] Sửa lỗi Windows Update\n"
             << " [7] Tối ưu hóa tổng thể hệ thống\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";
    }

    void menuMangBaoMat() {
        cls();
        cout << " [1] Xem thông tin mạng chi tiết\n"
             << " [2] Sửa lỗi & Khôi phục mạng toàn diện\n"
             << " [3] Kích hoạt Lá chắn bảo mật toàn diện\n"
             << " [4] Kiểm tra trạng thái bảo mật hệ thống\n"
             << " [5] Xem danh sách mật khẩu Wi-Fi đã lưu\n"
             << " [6] Quét & Bảo vệ tập tin Hosts\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";
    }

    void menuCongCuTienIch() {
        cls();
        cout << " [1] Auto Click chuột\n"
             << " [2] Spam Text\n"
             << " [3] Auto Paste dữ liệu nhiều dòng\n"
             << " [4] Tải & Cài đặt phần mềm tự động\n"
             << " [5] Gỡ bỏ ứng dụng rác mặc định\n"
             << " [6] Kiểm tra độ chai Pin Laptop\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";
    }

    void menuMedia() {
        cls();
        cout << " [1] Nén dung lượng Video / Ảnh\n"
             << " [2] Làm nét Video / Ảnh\n"
             << " [3] Mp4 -> Mp3\n"
             << " [4] Thay đổi tốc độ Video\n"
             << " [5] Đổi đuôi định dạng Media\n"
             << " [6] Chuẩn hóa tên file trong thư mục\n"
             << " [7] Ẩn file trong file\n"
             << " [0] Quay lại\n\n"
             << " [Chọn]: ";
    }

    void run() {
        SetConsoleTitleA("CMD BOX");
        Sleep(50);

        while (true) {
            cls();
            cout << "\n\n";
            mainMenu();
            int mainChoice = readInt("");
            
            if (mainChoice == 0) break;      
            if (mainChoice < 1 || mainChoice > 5) continue;

            int sub;
            switch (mainChoice) {

            // Bảo trì & Tối ưu
            case 1:
                while (true) {
                    cls();
                    menuBaoTriToiUu();
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    if (sub == 1)      getOptimizer().cleanDiskQuick();
                    else if (sub == 2) getOptimizer().cleanDiskPro();
                    else if (sub == 3) getOptimizer().disableAllStartupApps();
                    else if (sub == 4) getOptimizer().turnOffServicesMenu();
                    else if (sub == 5) getOptimizer().optimizeTaskbar();
                    else if (sub == 6) getOptimizer().fixWindowsUpdate();
                    else if (sub == 7) getOptimizer().optimizeSystemPRO();
                    else {
                        cout << "\nLựa chọn không hợp lệ!\n";
                        Sleep(300);
                    }
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
                    else if (sub == 6) getInternet().checkHostsFileSecurity();
                    else {
                        cout << "\nLựa chọn không hợp lệ!\n";
                        Sleep(300);
                    }
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
                    else if (sub == 6) getTools().batteryHealthDiagnostic();
                    else {
                        cout << "\nLựa chọn không hợp lệ!\n";
                        Sleep(300);
                    }
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
                    else if (sub == 7) getMedia().processAnFileTrongFile();
                    else {
                        cout << "\nLựa chọn không hợp lệ!\n";
                        Sleep(300);
                    }
                }
                break;

            // Trợ lý ảo AI
            case 5:
                launchAssistant();
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
