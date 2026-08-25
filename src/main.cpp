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
            if (!internet)  internet = std::make_unique<Internet>(*this);
        }
        return *internet;
    }
    
    SystemOptimizer& getOptimizer() {
        if (!opt) {
            std::lock_guard<std::mutex> lock(optMutex);
            if (!opt) opt = std::make_unique<SystemOptimizer>(*this, getInternet());
        }
        return *opt;
    }
    
    UtilityTools& getTools() {
        if (!tools) {
            std::lock_guard<std::mutex> lock(toolsMutex);
            if (!tools) tools = std::make_unique<UtilityTools>(*this);
        }
        return *tools;
    }
    
    MediaProcessor& getMedia() {
        if (!media) {
            std::lock_guard<std::mutex> lock(mediaMutex);
            if (!media) media = std::make_unique<MediaProcessor>();
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
        cout << " ┌─ [ TRẠNG THÁI ] ─────────────────────────\n"
             << " │ Quyền hạn : " << (admin ? "Administrator" : "User") << "\n"
             << " │ Thiết bị  : " << devInfo << "\n"
             << " └──────────────────────────────────────────\n";
    }

    void mainMenu() {
        renderStatusBox();
        cout << "\n"
             << " [1] Bảo trì & Tối ưu hệ thống\n"
             << " [2] Mạng & Bảo mật\n"
             << " [3] Công cụ tự động & Tiện ích\n"
             << " [4] Xử lý Media\n"
             << " [5] Trợ lý ảo AI\n"
             << " [0] Thoát\n\n"
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
                    cout << " [1] Dọn rác nhanh \n"
                         << " [2] Dọn rác chuyên sâu \n"
                         << " [3] Tắt ứng dụng khởi động\n"
                         << " [4] Tối ưu dịch vụ Windows\n"
                         << " [5] Chỉnh giao diện & Taskbar Win 11\n"
                         << " [6] Sửa lỗi Windows Update\n"
                         << " [7] Tối ưu hóa tổng thể hệ thống\n"
                         << " [8] Dọn rác môi trường Dev (Python, Node, Java...)\n"
                         << " [0] Quay lại\n\n"
                         << " [Chọn]: ";
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    switch (sub) {
                    case 1:  getOptimizer().cleanDiskQuick(); break;
                    case 2:  getOptimizer().cleanDiskPro(); break;
                    case 3:  getOptimizer().disableAllStartupApps(); break;
                    case 4:  getOptimizer().turnOffServicesMenu(); break;
                    case 5:  getOptimizer().optimizeTaskbar(); break;
                    case 6:  getOptimizer().fixWindowsUpdate(); break;
                    case 7:  getOptimizer().optimizeSystemPRO(); break;
                    case 8:  getOptimizer().cleanDevCaches(true); break;
                    default: Sleep(300); break;
                    }
                }
                break;

            // Mạng & Bảo mật
            case 2:
                while (true) {
                    cls();
                    cout << " [1] Xem thông tin mạng chi tiết\n"
                         << " [2] Sửa lỗi & Khôi phục mạng toàn diện\n"
                         << " [3] Kích hoạt Lá chắn bảo mật toàn diện\n"
                         << " [4] Kiểm tra trạng thái bảo mật hệ thống\n"
                         << " [5] Xem danh sách mật khẩu Wi-Fi đã lưu\n"
                         << " [6] Quét & Bảo vệ tập tin Hosts\n"
                         << " [0] Quay lại\n\n"
                         << " [Chọn]: ";
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    switch (sub) {
                    case 1:  getInternet().showNetworkInfo(); break;
                    case 2:  getInternet().repairNetwork(); break;
                    case 3:  getInternet().fullSecurityShield(); break;
                    case 4:  getInternet().checkSecurityStatus(); break;
                    case 5:  getInternet().wifiAudit(); break;
                    case 6:  getInternet().checkHostsFileSecurity(); break;
                    default: Sleep(300); break;
                    }
                }
                break;

            // Công cụ tự động & Tiện ích
            case 3:
                while (true) {
                    cls();
                    cout << " [1] Auto Click chuột\n"
                         << " [2] Spam Text\n"
                         << " [3] Auto Paste dữ liệu nhiều dòng\n"
                         << " [4] Tải & Cài đặt phần mềm tự động\n"
                         << " [5] Gỡ bỏ ứng dụng rác windows\n"
                         << " [6] Kiểm tra chai Pin Laptop\n"
                         << " [0] Quay lại\n\n"
                         << " [Chọn]: ";
                    sub = readInt("");
                    if (sub == 0) break;

                    switch (sub) {
                    case 1:  getTools().autoClickPoint(); break;
                    case 2:  getTools().spamText(); break;
                    case 3:  getTools().autoPasteData(); break;
                    case 4:  getTools().downloadManager(); break;
                    case 5:  getTools().uninstallBloatware(); break;
                    case 6:  getTools().batteryHealthDiagnostic(); break;
                    default: Sleep(300); break;
                    }
                }
                break;

            // Xử lý Media (FFmpeg)
            case 4:
                while (true) {
                    cls(); 
                    cout << " [1] Nén dung lượng Video/Ảnh\n"
                         << " [2] Làm nét Video /Ảnh\n"
                         << " [3] Mp4 -> Mp3\n"
                         << " [4] Tốc độ Video\n"
                         << " [5] Đổi định dạng Media\n"
                         << " [6] Chuẩn hóa tên file trong thư mục\n"
                         << " [7] Ẩn file trong file\n"
                         << " [0] Quay lại\n\n"
                         << " [Chọn]: ";
                    sub = readInt("");
                    if (sub == 0) break; 

                    switch (sub) {
                    case 1:  getMedia().processMediaAuto(); break;
                    case 2:  getMedia().processMediaEnhancementAuto(); break;
                    case 3:  getMedia().processExtractAudioBatch(); break;
                    case 4:  getMedia().processChangeSpeedBatch(); break;
                    case 5:  getMedia().processConvertFormatBatch(); break;
                    case 6:  getMedia().normalizeMediaFilenames(); break;
                    case 7:  getMedia().processAnFileTrongFile(); break;
                    default: Sleep(300); break;
                    }
                }
                break;

            // Trợ lý ảo AI
            case 5: {
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
                    break;
                }

                string pyExe = SystemCore::runRawCommand("where python >nul 2>nul") ? "python" :
                               SystemCore::runRawCommand("where py >nul 2>nul") ? "py" : "";

                if (pyExe.empty()) {
                    cls();
                    cout << " [!] Trợ lý ảo AI yêu cầu Python (3.11+) để hoạt động.\n\n";
                    SystemCore::waitEnter();
                    break;
                }

                system(("start \"CMD BOX - Tro Ly Ao AI\" " + pyExe + " \"" + scriptPath.string() + "\"").c_str());
                Sleep(1000);
                break;
            }
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
