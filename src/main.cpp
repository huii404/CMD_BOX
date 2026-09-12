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
                    cout << " [1] Dọn rác Đa Tầng\n"
                         << " [2] Tăng tốc & Tối ưu Đa Tầng\n"
                         << " [3] Sửa lỗi kẹt Windows Update\n"
                         << " [0] Quay lại\n\n"
                         << " [Chọn]: ";
                    sub = readInt("");
                    if (sub == 0) break;
                    
                    switch (sub) {
                    case 1:  getOptimizer().multiTierDiskClean(); break;
                    case 2:  getOptimizer().multiTierPerformanceOptimize(); break;
                    case 3:  getOptimizer().fixWindowsUpdate(); break;
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

#include "ImageEnhancerPro.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>

int runEnhanceTestPro(const std::string& folderPath) {
    fs::path inDir = fs::u8path(folderPath);
    if (!fs::exists(inDir) || !fs::is_directory(inDir)) {
        std::cerr << "Lỗi: Thư mục không tồn tại: " << folderPath << "\n";
        return 1;
    }

    fs::path outDir = inDir / "pro_output";
    if (!fs::exists(outDir)) {
        fs::create_directories(outDir);
    }

    std::vector<std::string> validExts = { ".jpg", ".jpeg", ".png", ".bmp", ".webp", ".tif", ".tiff" };
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(inDir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (std::find(validExts.begin(), validExts.end(), ext) != validExts.end()) {
                files.push_back(entry.path());
            }
        }
    }

    if (files.empty()) {
        std::cout << "Không tìm thấy file ảnh nào trong " << folderPath << "\n";
        return 0;
    }

    std::cout << "\n========================================================================================\n";
    std::cout << "       CMD BOX: BENCHMARK & EVALUATION SUITE FOR IMAGE ENHANCER PRO (7 GÓC ĐỘ)\n";
    std::cout << "========================================================================================\n";
    std::cout << "Tổng số ảnh phát hiện: " << files.size() << "\n\n";

    struct ImageBenchRecord {
        std::string filename;
        int origW, origH;
        int procW, procH;
        std::string detectedType;
        float origClarity, enhancedClarity, clarityGain;
        float origEdge, enhancedEdge;
        float origNoise, enhancedNoise, noiseMultiplier;
        float origBlur, enhancedBlur;
        float origDr, enhancedDr;
        float origShadowClip, enhancedShadowClip;
        float origHighlightClip, enhancedHighlightClip;
        float skinPercent;
        double elapsedMs;
        bool success;
    };

    std::vector<ImageBenchRecord> records;

    for (size_t i = 0; i < files.size(); ++i) {
        const auto& filePath = files[i];
        std::string fname = filePath.filename().string();
        std::string ext = filePath.extension().string();
        fs::path outFilePath = outDir / (filePath.stem().string() + "_pro" + ext);

        std::cout << "[" << (i + 1) << "/" << files.size() << "] Đang xử lý: " << fname << "... " << std::flush;

        ImageScorePro inScore;
        std::string errMsg;
        EnhanceErrorPro errCode = EnhanceErrorPro::Success;

        auto tStart = std::chrono::high_resolution_clock::now();
        bool ok = ImageEnhancerPro::enhanceImage(filePath.string(), outFilePath.string(), 0, &inScore, &errMsg, &errCode);
        auto tEnd = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

        if (!ok) {
            std::cout << "THẤT BẠI: " << errMsg << "\n";
            records.push_back({ fname, 0, 0, 0, 0, "ERROR", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ms, false });
            continue;
        }

        ImageScorePro outScore = ImageEnhancerPro::analyzeImageFile(outFilePath.string());

        float clarityGain = (inScore.clarityScore > 0.01f) ?
            ((outScore.clarityScore - inScore.clarityScore) / inScore.clarityScore * 100.0f) : 0.0f;
        float noiseMult = (inScore.noiseFloor > 0.01f) ? (outScore.noiseFloor / inScore.noiseFloor) : 1.0f;

        std::cout << "Xong (" << std::fixed << std::setprecision(1) << ms << "ms) | Ngữ cảnh: " 
                  << inScore.detectedType << " | Nét: " << inScore.clarityScore << " -> " << outScore.clarityScore 
                  << " (" << (clarityGain >= 0 ? "+" : "") << clarityGain << "%) | Nhiễu: " << noiseMult 
                  << "x | Da: " << (int)inScore.skinPercent << "% | Sat: " << (int)inScore.colorSaturation 
                  << " | Thin: " << std::setprecision(2) << inScore.thinFeatureRatio
                  << " | Blur: " << std::setprecision(1) << inScore.blurDegree
                  << " | DR: " << (int)inScore.dynamicRange << "\n";

        records.push_back({
            fname,
            inScore.origW, inScore.origH,
            inScore.procW, inScore.procH,
            inScore.detectedType,
            inScore.clarityScore, outScore.clarityScore, clarityGain,
            inScore.edgeSharpness, outScore.edgeSharpness,
            inScore.noiseFloor, outScore.noiseFloor, noiseMult,
            inScore.blurDegree, outScore.blurDegree,
            inScore.dynamicRange, outScore.dynamicRange,
            inScore.shadowClipPercent, outScore.shadowClipPercent,
            inScore.highlightClipPercent, outScore.highlightClipPercent,
            inScore.skinPercent,
            ms,
            true
        });
    }

    // In bảng tổng hợp 7 góc độ chi tiết
    std::cout << "\n====================================================================================================================================================================\n";
    std::cout << "                                               BẢNG ĐÁNH GIÁ 7 GÓC ĐỘ CHẤT LƯỢNG ẢNH CHI TIẾT (BEFORE -> AFTER)\n";
    std::cout << "====================================================================================================================================================================\n";
    std::cout << std::left << std::setw(28) << "Tên Tệp Ảnh"
              << std::setw(11) << "Kích Thước"
              << std::setw(26) << "Ngữ Cảnh Nhận Diện"
              << std::setw(7)  << "Da %"
              << std::setw(15) << "Acutance(Edge)"
              << std::setw(15) << "Độ Nhòe Blur"
              << std::setw(14) << "Noise (MAD)"
              << std::setw(14) << "Dải Sáng DR"
              << std::setw(14) << "Bết Tối Shadow"
              << std::setw(10) << "Thời Gian"
              << "\n";
    std::cout << "--------------------------------------------------------------------------------------------------------------------------------------------------------------------\n";

    double avgEdgeBefore = 0, avgEdgeAfter = 0;
    double avgNoiseBefore = 0, avgNoiseAfter = 0;
    double avgBlurBefore = 0, avgBlurAfter = 0;
    double avgMs = 0.0;
    int countOk = 0;

    for (const auto& r : records) {
        if (!r.success) continue;
        std::string resStr = std::to_string(r.origW) + "x" + std::to_string(r.origH);
        std::string edgeStr = std::to_string((int)r.origEdge) + "->" + std::to_string((int)r.enhancedEdge);
        std::string blurStr = std::to_string((int)r.origBlur) + "->" + std::to_string((int)r.enhancedBlur);
        std::string noiseStr = std::to_string(r.origNoise).substr(0,4) + "->" + std::to_string(r.enhancedNoise).substr(0,4);
        std::string drStr = std::to_string((int)r.origDr) + "->" + std::to_string((int)r.enhancedDr);
        std::string shadowStr = std::to_string(r.origShadowClip).substr(0,4) + "%->" + std::to_string(r.enhancedShadowClip).substr(0,4) + "%";

        std::cout << std::left << std::setw(28) << (r.filename.size() > 26 ? r.filename.substr(0, 25) + ".." : r.filename)
                  << std::setw(11) << resStr
                  << std::setw(26) << (r.detectedType.size() > 24 ? r.detectedType.substr(0, 23) + ".." : r.detectedType)
                  << std::setw(7)  << (std::to_string((int)r.skinPercent) + "%")
                  << std::setw(15) << edgeStr
                  << std::setw(15) << blurStr
                  << std::setw(14) << noiseStr
                  << std::setw(14) << drStr
                  << std::setw(14) << shadowStr
                  << std::setw(10) << (std::to_string((int)std::round(r.elapsedMs)) + "ms")
                  << "\n";

        avgEdgeBefore += r.origEdge; avgEdgeAfter += r.enhancedEdge;
        avgBlurBefore += r.origBlur; avgBlurAfter += r.enhancedBlur;
        avgNoiseBefore += r.origNoise; avgNoiseAfter += r.enhancedNoise;
        avgMs += r.elapsedMs;
        countOk++;
    }

    if (countOk > 0) {
        std::cout << "--------------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
        std::string edgeSum = std::to_string((int)(avgEdgeBefore/countOk)) + "->" + std::to_string((int)(avgEdgeAfter/countOk)) +
                              " (+" + std::to_string((int)std::round((avgEdgeAfter - avgEdgeBefore)/avgEdgeBefore * 100.0)) + "%)";
        std::string blurSum = std::to_string((int)(avgBlurBefore/countOk)) + "->" + std::to_string((int)(avgBlurAfter/countOk));
        std::string noiseSum = std::to_string(avgNoiseBefore/countOk).substr(0,4) + "->" + std::to_string(avgNoiseAfter/countOk).substr(0,4);

        std::cout << std::left << std::setw(28) << "TRUNG BÌNH TOÀN DIỆN"
                  << std::setw(11) << "-"
                  << std::setw(26) << "-"
                  << std::setw(7)  << "-"
                  << std::setw(15) << edgeSum
                  << std::setw(15) << blurSum
                  << std::setw(14) << noiseSum
                  << std::setw(14) << "-"
                  << std::setw(14) << "-"
                  << std::setw(10) << (std::to_string((int)std::round(avgMs / countOk)) + "ms")
                  << "\n";
    }
    std::cout << "====================================================================================================================================================================\n";

    return 0;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::ios::sync_with_stdio(false);

    if (argc >= 2 && std::string(argv[1]) == "--test-pro") {
        std::string targetDir = (argc >= 3) ? argv[2] : "images_test";
        return runEnhanceTestPro(targetDir);
    }

    AppUI app;
    app.run();
    
    return 0;
}
