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
#include "ImageEnhancerPro.h"
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
    ~AppUI() = default;

    void renderStatusBox() {
        bool admin = SystemCore::isElevated();
        std::string devInfo = SystemCore::getDeviceStatus();
        std::string verStatus = UpdateManager::getVersionStatusText();
        cout << " ┌─ TRẠNG THÁI ──────────────────────────────┐\n"
             << " │ Phiên bản : " << verStatus << "\n"
             << " │ Quyền     : " << (admin ? "Admin" : "User") << "\n"
             << " │ Thiết bị  : " << devInfo << "\n"
             << " └───────────────────────────────────────────┘\n";
    }

    void mainMenu() {
        renderStatusBox();
        cout << "\n"
             << " [1] Tối ưu hệ thống\n"
             << " [2] Mạng & Bảo mật\n"
             << " [3] Công cụ tự động\n"
             << " [4] Xử lý Media\n"
             << " [5] Cập nhật phần mềm\n"
             << " [0] Thoát\n\n"
             << " [Chọn]: ";
    }


    void run() {
        SetConsoleTitleA("CMD BOX");
        Sleep(50);

        while (true) {
            cls();
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
                    cout << "== DỌN RÁC ==\n"
                         << " [1] Temp & cache người dùng\n"
                         << " [2] Cache trình duyệt & ứng dụng\n"
                         << " [3] Dọn hệ thống chuyên sâu\n"
                         << " [4] Cache lập trình\n"
                         << " [5] Bộ cài trong Downloads\n"
                         << " [6] Dọn hệ thống (1, 2, 3, 5)\n"
                         << " [7] Dọn tất cả (1–5)\n\n"
                         << "== TỐI ƯU ==\n"
                         << " [8] Ứng dụng khởi động\n"
                         << " [9] Dịch vụ nền\n"
                         << " [10] Giao diện & Taskbar\n"
                         << " [11] Tối ưu tất cả (8–10)\n\n"
                         << "== HỆ THỐNG ==\n"
                         << " [12] Sửa Windows Update\n"
                         << " [13] Quản lý dịch vụ\n"
                         << " [0] Quay lại\n"
                         << " [Chọn]: ";
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    switch (sub) {
                    case 1:  getOptimizer().runCleanChoice(1); break;
                    case 2:  getOptimizer().runCleanChoice(2); break;
                    case 3:  getOptimizer().runCleanChoice(3); break;
                    case 4:  getOptimizer().runCleanChoice(4); break;
                    case 5:  getOptimizer().runCleanChoice(5); break;
                    case 6:  getOptimizer().runCleanChoice(6); break;
                    case 7:  getOptimizer().runCleanChoice(7); break;
                    case 8:  getOptimizer().runOptimizeChoice(1); break;
                    case 9:  getOptimizer().runOptimizeChoice(2); break;
                    case 10: getOptimizer().runOptimizeChoice(3); break;
                    case 11: getOptimizer().runOptimizeChoice(4); break;
                    case 12: getOptimizer().fixWindowsUpdate(); break;
                    case 13: getOptimizer().turnOffServicesMenu(); break;
                    default: Sleep(300); break;
                    }
                }
                break;

            // Mạng & Bảo mật
            case 2:
                while (true) {
                    cls();
                    cout << "== MẠNG & BẢO MẬT ==\n"
                         << " [1] Sửa mạng\n"
                         << " [2] Bật bảo vệ\n"
                         << " [3] Trạng thái bảo mật\n"
                         << " [4] Mật khẩu Wi-Fi đã lưu\n"
                         << " [5] Thiết bị Wi-Fi\n"
                         << " [6] Truyền file LAN\n"
                         << " [0] Quay lại\n"
                         << " [Chọn]: ";
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
                    cout << "== CÔNG CỤ ==\n"
                         << " [1] Tự động click\n"
                         << " [2] Gửi văn bản\n"
                         << " [3] Dán nhiều dòng\n"
                         << " [4] Tải phần mềm\n"
                         << " [5] Gỡ ứng dụng rác\n"
                         << " [6] Kiểm tra pin\n"
                         << " [0] Quay lại\n"
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
                    cout << "== MEDIA ==\n"
                         << " [1] Nén video/ảnh\n"
                         << " [2] Làm nét ảnh\n"
                         << " [3] MP4 → MP3\n"
                         << " [4] Đổi tốc độ video\n"
                         << " [5] Đổi định dạng\n"
                         << " [6] Chuẩn hóa tên file\n"
                         << " [7] Ẩn file trong media\n"
                         << " [0] Quay lại\n"
                         << " [Chọn]: ";
                    sub = readInt("");
                    if (sub == 0) break; 

                    switch (sub) {
                    case 1:  getMedia().processMediaAuto(); break;
                    case 2:  getMedia().processMediaEnhancement(); break;
                    case 3:  getMedia().processExtractAudioBatch(); break;
                    case 4:  getMedia().processChangeSpeedBatch(); break;
                    case 5:  getMedia().processConvertFormatBatch(); break;
                    case 6:  getMedia().normalizeMediaFilenames(); break;
                    case 7:  getMedia().processAnFileTrongFile(); break;
                    default: Sleep(300); break;
                    }
                }
                break;

            // Kiểm tra cập nhật
            case 5:
                UpdateManager::showUpdateMenu();
                break;
            }
        }
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

    // Hỗ trợ chế độ dòng lệnh (CLI Mode) cho Image Enhancer và Kiểm thử Benchmark
    if (argc >= 4 && std::string(argv[1]) == "--enhance") {
        std::string inPath = argv[2];
        std::string outPath = argv[3];
        int level = (argc >= 5) ? std::stoi(argv[4]) : 0;
        ImageScorePro score;
        std::string errMsg;
        EnhanceErrorPro errCode = EnhanceErrorPro::Success;
        auto tStart = std::chrono::high_resolution_clock::now();
        bool ok = ImageEnhancerPro::enhanceImage(inPath, outPath, level, &score, &errMsg, &errCode);
        auto tEnd = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        if (ok) {
            std::cout << "[CPP_PRO] SUCCESS | Time: " << elapsedMs << " ms | Clarity: " << score.clarityScore
                      << " | Edge: " << score.edgeSharpness << " | Blur: " << score.blurDegree
                      << " | Noise: " << score.noiseFloor << " | Out: " << outPath << "\n";
            return 0;
        } else {
            std::cerr << "[CPP_PRO] FAILED | Error: " << errMsg << "\n";
            return 1;
        }
    }

    AppUI app;
    app.run();
    
    return 0;
}
