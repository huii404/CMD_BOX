#include "Internet.h"
#include <windows.h>
#include <iostream>
#include <limits>
#include <vector>
#include <memory>
#include <mutex>
#include "SystemCore.h"
#include "SystemOptimizer.h"
#include "UtilityTools.h"
#include "MediaProcessor.h"
#include "UpdateManager.h"
#include <chrono>

using namespace std;
namespace fs = std::filesystem;

namespace UI {
    inline const char* RESET   = "\x1b[0m";
    inline const char* BOLD    = "\x1b[1m";
    inline const char* DIM     = "\x1b[90m";
    inline const char* RED     = "\x1b[91m";
    inline const char* GREEN   = "\x1b[92m";
    inline const char* YELLOW  = "\x1b[93m";
    inline const char* BLUE    = "\x1b[94m";
    inline const char* MAGENTA = "\x1b[95m";
    inline const char* CYAN    = "\x1b[96m";
    inline const char* WHITE   = "\x1b[97m";
}

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
            if (!opt) opt = std::make_unique<SystemOptimizer>(*this);
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
        UpdateManager::checkUpdateAsync();
    }
    ~AppUI() {
        UpdateManager::shutdown();
    }

    void renderStatusBox() {
        bool admin = SystemCore::isElevated();
        std::string devInfo = SystemCore::getDeviceStatus();
        std::string verStatus = UpdateManager::getVersionStatusText();

        cout << "\n"
             << UI::CYAN << "  ── THÔNG TIN HỆ THỐNG ────────────────────────────────\n"
             << UI::DIM  << "   Phiên bản : " << UI::WHITE << verStatus << "\n"
             << UI::DIM  << "   Quyền hạn : " << (admin ? (string(UI::GREEN) + "Administrator") : (string(UI::YELLOW) + "Standard User")) << "\n"
             << UI::DIM  << "   Thiết bị  : " << UI::WHITE << devInfo << "\n"
             << UI::CYAN << "  ──────────────────────────────────────────────────────\n"
             << UI::RESET;
    }

    void mainMenu() {
        renderStatusBox();
        cout << "\n"
             << UI::CYAN << "  ┌── MENU CHÍNH ───────────────────────────────────────\n"
             << "  │ " << UI::YELLOW << " [1] " << UI::WHITE << "Tối ưu & Dọn dẹp hệ thống\n"
             << "  │ " << UI::YELLOW << " [2] " << UI::WHITE << "Quản trị mạng & Bảo mật\n"
             << "  │ " << UI::YELLOW << " [3] " << UI::WHITE << "Công cụ tự động & Tiện ích\n"
             << "  │ " << UI::YELLOW << " [4] " << UI::WHITE << "Xử lý Media (FFmpeg)\n"
             << "  │ " << UI::YELLOW << " [5] " << UI::WHITE << "Kiểm tra cập nhật phần mềm\n"
             << "  │ " << UI::YELLOW << " [6] " << UI::WHITE << "Biên dịch lại ứng dụng\n"
             << "  │ " << UI::RED    << " [0] " << UI::DIM   << "Thoát chương trình\n"
             << UI::CYAN << "  └─────────────────────────────────────────────────────\n"
             << UI::RESET << "\n"
             << UI::BOLD << UI::CYAN << "  [Chọn]: " << UI::RESET;
    }

    void run() {
        SetConsoleTitleA("CMD BOX");
        Sleep(50);

        while (true) {
            cls();
            mainMenu();
            int mainChoice = readInt("");
            
            if (mainChoice == 0) break;      
            if (mainChoice < 1 || mainChoice > 6) continue;

            int sub;
            switch (mainChoice) {

            // Bảo trì & Tối ưu
            case 1:
                while (true) {
                    cls();
                    cout << "\n"
                         << UI::CYAN << "  ┌── TỐI ƯU & DỌN DẸP HỆ THỐNG ────────────────────────\n"
                         << "  │ " << UI::DIM    << "--- Dọn rác ---\n"
                         << "  │ " << UI::YELLOW << " [1] " << UI::WHITE << "Dọn dẹp hệ thống toàn diện\n"
                         << "  │\n"
                         << "  │ " << UI::DIM    << "--- Hiệu năng ---\n"
                         << "  │ " << UI::YELLOW << " [2] " << UI::WHITE << "Tối ưu ứng dụng khởi động\n"
                         << "  │ " << UI::YELLOW << " [3] " << UI::WHITE << "Tắt dịch vụ nền vô ích\n"
                         << "  │ " << UI::YELLOW << " [4] " << UI::WHITE << "Tối ưu giao diện & Taskbar\n"
                         << "  │ " << UI::YELLOW << " [5] " << UI::GREEN << "Tối ưu toàn bộ (Khuyên dùng)\n"
                         << "  │\n"
                         << "  │ " << UI::DIM    << "--- Bảo trì ---\n"
                         << "  │ " << UI::YELLOW << " [6] " << UI::WHITE << "Sửa lỗi kẹt Windows Update\n"
                         << "  │ " << UI::YELLOW << " [7] " << UI::WHITE << "Quản lý dịch vụ hệ thống\n"
                         << "  │\n"
                         << "  │ " << UI::RED    << " [0] " << UI::DIM   << "Quay lại menu chính\n"
                         << UI::CYAN << "  └─────────────────────────────────────────────────────\n"
                         << UI::RESET << "\n"
                         << UI::BOLD << UI::CYAN << "  [Chọn]: " << UI::RESET;
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    switch (sub) {
                    case 1:  getOptimizer().runClean(); break;
                    case 2:  getOptimizer().runOptimizeChoice(1); break;
                    case 3:  getOptimizer().runOptimizeChoice(2); break;
                    case 4:  getOptimizer().runOptimizeChoice(3); break;
                    case 5:  getOptimizer().runOptimizeChoice(4); break;
                    case 6:  getOptimizer().fixWindowsUpdate(); break;
                    case 7:  getOptimizer().turnOffServicesMenu(); break;
                    default: Sleep(300); break;
                    }
                }
                break;

            // Mạng & Bảo mật
            case 2:
                while (true) {
                    cls();
                    cout << "\n"
                         << UI::CYAN << "  ┌── QUẢN TRỊ MẠNG & BẢO MẬT ──────────────────────────\n"
                         << "  │ " << UI::YELLOW << " [1] " << UI::WHITE << "Sửa lỗi mạng & Reset kết nối\n"
                         << "  │ " << UI::YELLOW << " [2] " << UI::WHITE << "Kích hoạt tường lửa bảo vệ\n"
                         << "  │ " << UI::YELLOW << " [3] " << UI::WHITE << "Kiểm tra an ninh mạng\n"
                         << "  │ " << UI::YELLOW << " [4] " << UI::WHITE << "Xem mật khẩu Wi-Fi đã lưu\n"
                         << "  │ " << UI::YELLOW << " [5] " << UI::WHITE << "Quét thiết bị trong mạng LAN\n"
                         << "  │ " << UI::YELLOW << " [6] " << UI::WHITE << "Chia sẻ file cục bộ (LocalDrop)\n"
                         << "  │\n"
                         << "  │ " << UI::RED    << " [0] " << UI::DIM   << "Quay lại menu chính\n"
                         << UI::CYAN << "  └─────────────────────────────────────────────────────\n"
                         << UI::RESET << "\n"
                         << UI::BOLD << UI::CYAN << "  [Chọn]: " << UI::RESET;
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    switch (sub) {
                    case 1:  getInternet().repairNetwork(); break;
                    case 2:  getInternet().fullSecurityShield(); break;
                    case 3:  getInternet().checkSecurityStatus(); break;
                    case 4:  getInternet().wifiAudit(); break;
                    case 5:  getInternet().scanConnectedDevices(); break;
                    case 6:  getInternet().localDropMenu(); break;
                    default: Sleep(300); break;
                    }
                }
                break;

            // Công cụ tự động & Tiện ích
            case 3:
                while (true) {
                    cls();
                    cout << "\n"
                         << UI::CYAN << "  ┌── CÔNG CỤ TỰ ĐỘNG & TIỆN ÍCH ───────────────────────\n"
                         << "  │ " << UI::YELLOW << " [1] " << UI::WHITE << "Auto Clicker\n"
                         << "  │ " << UI::YELLOW << " [2] " << UI::WHITE << "Gửi văn bản tự động\n"
                         << "  │ " << UI::YELLOW << " [3] " << UI::WHITE << "Dán dữ liệu nhiều dòng\n"
                         << "  │ " << UI::YELLOW << " [4] " << UI::WHITE << "Tải phần mềm nhanh\n"
                         << "  │ " << UI::YELLOW << " [5] " << UI::WHITE << "Gỡ ứng dụng rác (Bloatware)\n"
                         << "  │ " << UI::YELLOW << " [6] " << UI::WHITE << "Kiểm tra pin laptop\n"
                         << "  │\n"
                         << "  │ " << UI::RED    << " [0] " << UI::DIM   << "Quay lại menu chính\n"
                         << UI::CYAN << "  └─────────────────────────────────────────────────────\n"
                         << UI::RESET << "\n"
                         << UI::BOLD << UI::CYAN << "  [Chọn]: " << UI::RESET;
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
                    cout << "\n"
                         << UI::CYAN << "  ┌── XỬ LÝ MEDIA (FFMPEG) ─────────────────────────────\n"
                         << "  │ " << UI::YELLOW << " [1] " << UI::WHITE << "Nén video / hình ảnh\n"
                         << "  │ " << UI::YELLOW << " [2] " << UI::WHITE << "Tách âm thanh (MP3)\n"
                         << "  │ " << UI::YELLOW << " [3] " << UI::WHITE << "Đổi tốc độ phát video\n"
                         << "  │ " << UI::YELLOW << " [4] " << UI::WHITE << "Đổi định dạng tệp\n"
                         << "  │ " << UI::YELLOW << " [5] " << UI::WHITE << "Chuẩn hóa tên file\n"
                         << "  │ " << UI::YELLOW << " [6] " << UI::WHITE << "Ẩn file trong media\n"
                         << "  │\n"
                         << "  │ " << UI::RED    << " [0] " << UI::DIM   << "Quay lại menu chính\n"
                         << UI::CYAN << "  └─────────────────────────────────────────────────────\n"
                         << UI::RESET << "\n"
                         << UI::BOLD << UI::CYAN << "  [Chọn]: " << UI::RESET;
                    sub = readInt("");
                    if (sub == 0) break; 

                    switch (sub) {
                    case 1:  getMedia().processMediaAuto(); break;
                    case 2:  getMedia().processExtractAudioBatch(); break;
                    case 3:  getMedia().processChangeSpeedBatch(); break;
                    case 4:  getMedia().processConvertFormatBatch(); break;
                    case 5:  getMedia().normalizeMediaFilenames(); break;
                    case 6:  getMedia().processAnFileTrongFile(); break;
                    default: Sleep(300); break;
                    }
                }
                break;

            // Kiểm tra cập nhật
            case 5:
                UpdateManager::showUpdateMenu();
                break;

            // Biên dịch lại & khởi động lại qua build.bat
            case 6: {
                cls();
                cout << "\n [*] Đang khởi chạy build.bat để biên dịch lại...\n";
                fs::path batPath;
                if (fs::exists("build.bat")) {
                    batPath = fs::absolute("build.bat");
                } else if (fs::exists("..\\build.bat")) {
                    batPath = fs::absolute("..\\build.bat");
                }

                if (!batPath.empty()) {
                    string batStr = batPath.string();
                    string dirStr = batPath.parent_path().string();
                    ShellExecuteA(NULL, "open", batStr.c_str(), NULL, dirStr.c_str(), SW_SHOWNORMAL);
                    return;
                } else {
                    cout << "\n [!] Không tìm thấy file build.bat!\n";
                    SystemCore::waitEnter();
                }
                break;
            }
            }
        }
    }

    int runCommandLine(int argc, char* argv[]) {
        const string command = argc > 1 ? argv[1] : "";
        if (command == "--version" || command == "-v") {
            cout << "CMD BOX v" << UpdateManager::CURRENT_VERSION << "\n";
            return 0;
        }
        if (command == "--help" || command == "-h" || command == "help") {
            cout << "CMD BOX v" << UpdateManager::CURRENT_VERSION << "\n\n"
                 << "Cách dùng:\n"
                 << "  main.exe                         Mở menu tương tác\n"
                 << "  main.exe clean <1-2>             Xóa rác: 1 = Plus, 2 = Pro (có xác nhận)\n"
                 << "  main.exe optimize <1-4>          Tối ưu, có xem trước và xác nhận\n"
                 << "  main.exe scan-network            Quét thiết bị trong LAN\n"
                 << "  main.exe security-status         Kiểm tra trạng thái bảo mật\n"
                 << "  main.exe media                   Mở công cụ nén media\n"
                 << "  main.exe --version               Xem phiên bản\n";
            return 0;
        }
        if (command == "clean") {
            getOptimizer().runClean();
            return 0;
        }
        if (command == "optimize" && argc >= 3) {
            int tier = 0;
            try { tier = stoi(argv[2]); } catch (...) {}
            if (tier < 1 || tier > 4) { cerr << "Mức tối ưu phải từ 1 đến 4.\n"; return 2; }
            getOptimizer().runOptimizeChoice(tier);
            return 0;
        }
        if (command == "scan-network") { getInternet().scanConnectedDevices(); return 0; }
        if (command == "security-status") { getInternet().checkSecurityStatus(); return 0; }
        if (command == "media") { getMedia().processMediaAuto(); return 0; }
        cerr << "Lệnh không hợp lệ. Dùng --help để xem hướng dẫn.\n";
        return 2;
    }
};

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::ios::sync_with_stdio(false);

    // Kích hoạt Virtual Terminal Processing cho Console Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE && hOut != NULL) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }

    AppUI app;
    if (argc > 1) return app.runCommandLine(argc, argv);
    app.run();
    
    return 0;
}
