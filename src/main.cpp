#include "Internet.h"
#include "MenuStyle.h"
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

        using namespace MenuStyle;
        cout << "\n"
             << MUTED << "  Phiên bản  " << TEXT << verStatus << RESET << "\n"
             << MUTED << "  Quyền hạn  " << (admin ? MINT.text : AMBER.text)
             << (admin ? "● Administrator" : "● Standard User") << RESET << "\n"
             << MUTED << "  Thiết bị   " << TEXT << devInfo << RESET << "\n";
    }

    void mainMenu() {
        using namespace MenuStyle;
        renderStatusBox();
        header("MENU CHÍNH");
        item(1,"Tối ưu & Dọn dẹp hệ thống",MINT);
        item(2,"Quản trị mạng & Bảo mật",SKY);
        item(3,"Công cụ tự động & Tiện ích",AMBER);
        item(4,"Media",ORCHID);
        item(5,"Kiểm tra cập nhật phần mềm",ROSE);
        item(6,"Biên dịch ứng dụng",MINT);
        footer("Thoát chương trình");
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
                    MenuStyle::header("TỐI ƯU & DỌN DẸP",MenuStyle::MINT);
                    MenuStyle::section("DỌN RÁC");
                    MenuStyle::item(1,"Dọn dẹp hệ thống toàn diện",MenuStyle::MINT);
                    MenuStyle::section("HIỆU NĂNG");
                    MenuStyle::item(2,"Tối ưu ứng dụng khởi động",MenuStyle::MINT);
                    MenuStyle::item(3,"Tắt dịch vụ nền vô ích",MenuStyle::MINT);
                    MenuStyle::item(4,"Tối ưu giao diện & Taskbar",MenuStyle::MINT);
                    MenuStyle::item(5,"Tối ưu toàn bộ",MenuStyle::AMBER,"  · Khuyên dùng");
                    MenuStyle::section("BẢO TRÌ");
                    MenuStyle::item(6,"Sửa lỗi kẹt Windows Update",MenuStyle::MINT);
                    MenuStyle::item(7,"Quản lý dịch vụ hệ thống",MenuStyle::MINT);
                    MenuStyle::footer("Quay lại menu chính",MenuStyle::MINT);
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
                    MenuStyle::header("QUẢN TRỊ MẠNG & BẢO MẬT",MenuStyle::SKY);
                    MenuStyle::item(1,"Sửa lỗi mạng & Reset kết nối",MenuStyle::SKY);
                    MenuStyle::item(2,"Kích hoạt tường lửa bảo vệ",MenuStyle::SKY);
                    MenuStyle::item(3,"Kiểm tra an ninh mạng",MenuStyle::SKY);
                    MenuStyle::item(4,"Xem mật khẩu Wi-Fi đã lưu",MenuStyle::SKY);
                    MenuStyle::item(5,"Quét thiết bị trong mạng LAN",MenuStyle::SKY);
                    MenuStyle::item(6,"Chia sẻ file cục bộ (LocalDrop)",MenuStyle::SKY);
                    MenuStyle::footer("Quay lại menu chính",MenuStyle::SKY);
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
                    MenuStyle::header("CÔNG CỤ TỰ ĐỘNG & TIỆN ÍCH",MenuStyle::AMBER);
                    MenuStyle::item(1,"Auto Clicker",MenuStyle::AMBER);
                    MenuStyle::item(2,"Gửi văn bản tự động",MenuStyle::AMBER);
                    MenuStyle::item(3,"Dán dữ liệu nhiều dòng",MenuStyle::AMBER);
                    MenuStyle::item(4,"Tải phần mềm nhanh",MenuStyle::AMBER);
                    MenuStyle::item(5,"Gỡ ứng dụng rác (Bloatware)",MenuStyle::AMBER);
                    MenuStyle::item(6,"Kiểm tra pin laptop",MenuStyle::AMBER);
                    MenuStyle::footer("Quay lại menu chính",MenuStyle::AMBER);
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
                    MenuStyle::header("XỬ LÝ MEDIA",MenuStyle::ORCHID);
                    MenuStyle::item(1,"Nén video / hình ảnh",MenuStyle::ORCHID);
                    MenuStyle::item(2,"Tách âm thanh (MP3)",MenuStyle::ORCHID);
                    MenuStyle::item(3,"Đổi tốc độ phát video",MenuStyle::ORCHID);
                    MenuStyle::item(4,"Đổi định dạng tệp",MenuStyle::ORCHID);
                    MenuStyle::item(5,"Chuẩn hóa tên file",MenuStyle::ORCHID);
                    MenuStyle::item(6,"Ẩn file trong media",MenuStyle::ORCHID);
                    MenuStyle::item(7,"Sắp album từ thư mục (năm / tháng)",MenuStyle::ORCHID);
                    MenuStyle::footer("Quay lại menu chính",MenuStyle::ORCHID);
                    sub = readInt("");
                    if (sub == 0) break; 

                    switch (sub) {
                    case 1:  getMedia().processMediaAuto(); break;
                    case 2:  getMedia().processExtractAudioBatch(); break;
                    case 3:  getMedia().processChangeSpeedBatch(); break;
                    case 4:  getMedia().processConvertFormatBatch(); break;
                    case 5:  getMedia().normalizeMediaFilenames(); break;
                    case 6:  getMedia().processAnFileTrongFile(); break;
                    case 7:  getMedia().organizeAlbumFolder(); break;
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
                 << "  main.exe clean                   Dọn cache/Downloads (có xác nhận)\n"
                 << "  main.exe optimize <1-4>          Tối ưu, có xem trước và xác nhận\n"
                 << "  main.exe scan-network            Quét thiết bị trong LAN\n"
                 << "  main.exe security-status         Kiểm tra trạng thái bảo mật\n"
                 << "  main.exe media                   Mở công cụ nén media\n"
                 << "  main.exe --version               Xem phiên bản\n";
            return 0;
        }
        if (command == "clean") {
            if (argc != 2) { cerr << "Dùng main.exe clean; không có mức 1/2.\n"; return 2; }
            getOptimizer().runClean();
            return 0;
        }
        if (command == "optimize" && argc >= 3) {
            int tier = 0;
            try { size_t used = 0; string value = argv[2]; tier = stoi(value, &used); if (used != value.size()) tier = 0; } catch (...) {}
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
