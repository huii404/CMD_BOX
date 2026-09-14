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
        cout << " ┌─ [ TRẠNG THÁI ] ─────────────────────────\n"
             << " │ Phiên bản : " << verStatus << "\n"
             << " │ Quyền hạn : " << (admin ? "Administrator" : "User") << "\n"
             << " │ Thiết bị  : " << devInfo << "\n"
             << " └──────────────────────────────────────────\n";
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
                    cout << "\n"
                         << " ─── [ DỌN RÁC & LÀM SẠCH ] ────────────────────────────────────────\n\n"
                         << "  [1]  Dọn rác bề mặt & Cache người dùng (Temp, CrashDumps, WER, DNS)\n"
                         << "  [2]  Dọn rác Trình duyệt & Ứng dụng (Chrome, Edge, Discord...)\n"
                         << "  [3]  Dọn dẹp Chuyên sâu & Tồn dư Cập nhật (DISM, Windows.old, Logs)\n"
                         << "  [4]  Dọn rác Môi trường lập trình (node_modules, Pip, VS Code...)\n"
                         << "  [5]  Dọn tệp cài đặt Downloads (Exe đã cài đặt, installer trùng lặp)\n"
                         << "  [6]  [⚡] Dọn toàn diện Hệ thống (Mục 1 + 2 + 3 + 5)\n"
                         << "  [7]  [🚀] Dọn tất cả (Cả 5 mục - Bao gồm cả rác Dev)\n\n"
                         << " ─── [ TĂNG TỐC & TỐI ƯU ] ─────────────────────────────────────────\n\n"
                         << "  [8]  Tối ưu Khởi động (Tắt app làm chậm, giữ Bộ gõ & Driver)\n"
                         << "  [9]  Tối ưu Dịch vụ ngầm (Telemetry, DiagTrack, Maps, Wallet)\n"
                         << "  [10] Tối ưu Giao diện & Taskbar (Bỏ trễ UI, tinh gọn Taskbar)\n"
                         << "  [11] [⚡] Tối ưu liên hoàn hiệu năng (Mục 8 + 9 + 10)\n\n"
                         << " ─── [ CÔNG CỤ HỆ THỐNG ] ──────────────────────────────────────────\n\n"
                         << "  [12] Sửa lỗi kẹt Windows Update\n"
                         << "  [13] Quản lý Dịch vụ Windows nâng cao\n"
                         << " ───────────────────────────────────────────────────────────────────\n\n"
                         << "  [0]  Quay lại\n\n"
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
                    cout << " [1] Sửa lỗi & Khôi phục mạng toàn diện\n"
                         << " [2] Kích hoạt Lá chắn bảo mật toàn diện\n"
                         << " [3] Kiểm tra trạng thái bảo mật\n"
                         << " [4] Xem danh sách mật khẩu Wi-Fi đã lưu\n"
                         << " [5] Quét thiết bị kết nối Wi-Fi\n"
                         << " [6] Local Web Drop (Truyền file P2P)\n"
                         << " [0] Quay lại\n\n"
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
                    cout << " [1] Auto Click\n"
                         << " [2] Spam Text\n"
                         << " [3] Auto Paste\n"
                         << " [4] Install Software\n"
                         << " [5] Uninstall Bloatware\n"
                         << " [6] Check Pin Laptop\n"
                         << " [0] Return\n\n"
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
                         << " [2] Làm nét Ảnh\n"
                         << " [3] Mp4 -> Mp3\n"
                         << " [4] Tốc độ Video\n"
                         << " [5] Đổi định dạng Video/Ảnh\n"
                         << " [6] Chuẩn hóa tên file Video/Ảnh\n"
                         << " [7] Ẩn file vào file\n"
                         << " [0] Quay lại\n\n"
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
