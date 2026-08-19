#include "../include/MediaProcessor.h"
#include "../include/SystemCore.h"
#include <iostream>
#include <random>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <mutex>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;

static string cachedFFmpegPath = "";
static mutex ffmpegMutex;

static string cachedFFprobePath = "";
static mutex ffprobeMutex;

static GpuCodecInfo cachedGpuInfo;
static bool hasDetectedGpu = false;
static mutex gpuDetectionMutex;

MediaProcessor::MediaProcessor() {
    std::thread([this]() {
        this->getGpuEncoder();
    }).detach();
}
MediaProcessor::~MediaProcessor() {}

string MediaProcessor::getFFmpegPath() {
    if (!cachedFFmpegPath.empty()) {
        return cachedFFmpegPath;
    }
    
    lock_guard<mutex> lock(ffmpegMutex);
    
    // Double-check
    if (!cachedFFmpegPath.empty()) {
        return cachedFFmpegPath;
    }
    
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    fs::path exePath(buffer);
    fs::path binDir = exePath.parent_path();
    fs::path ffmpegPath = binDir / "ffmpeg.exe";
    
    if (fs::exists(ffmpegPath)) {
        cachedFFmpegPath = "\"" + ffmpegPath.string() + "\"";
    } else {
        // Thử tìm trong PATH
        char* pathEnv = getenv("PATH");
        if (pathEnv) {
            string pathStr(pathEnv);
            size_t pos = 0;
            string token;
            while ((pos = pathStr.find(';')) != string::npos) {
                token = pathStr.substr(0, pos);
                fs::path testPath = fs::path(token) / "ffmpeg.exe";
                if (fs::exists(testPath)) {
                    cachedFFmpegPath = "\"" + testPath.string() + "\"";
                    return cachedFFmpegPath;
                }
                pathStr.erase(0, pos + 1);
            }
        }
        cachedFFmpegPath = "ffmpeg";
    }
    return cachedFFmpegPath;
}

string MediaProcessor::getFFprobePath() {
    if (!cachedFFprobePath.empty()) return cachedFFprobePath;
    lock_guard<mutex> lock(ffprobeMutex);
    if (!cachedFFprobePath.empty()) return cachedFFprobePath;

    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    fs::path exePath(buffer);
    fs::path binDir = exePath.parent_path();
    fs::path probePath = binDir / "ffprobe.exe";

    if (fs::exists(probePath)) {
        cachedFFprobePath = "\"" + probePath.string() + "\"";
        return cachedFFprobePath;
    }

    char* pathEnv = getenv("PATH");
    if (pathEnv) {
        string pathStr(pathEnv);
        size_t pos = 0;
        string token;
        while ((pos = pathStr.find(';')) != string::npos) {
            token = pathStr.substr(0, pos);
            fs::path testPath = fs::path(token) / "ffprobe.exe";
            if (fs::exists(testPath)) {
                cachedFFprobePath = "\"" + testPath.string() + "\"";
                return cachedFFprobePath;
            }
            pathStr.erase(0, pos + 1);
        }
    }
    cachedFFprobePath = "ffprobe";
    return cachedFFprobePath;
}

GpuCodecInfo MediaProcessor::getGpuEncoder() {
    if (hasDetectedGpu) return cachedGpuInfo;
    lock_guard<mutex> lock(gpuDetectionMutex);
    if (hasDetectedGpu) return cachedGpuInfo;

    string ffmpeg = getFFmpegPath();

    // 1. Kiểm tra NVIDIA NVENC
    string testNvenc = ffmpeg + " -hide_banner -loglevel error -f lavfi -i color=s=64x64:d=0.1 -c:v h264_nvenc -f null -";
    if (SystemCore::runRawCommand(testNvenc)) {
        cachedGpuInfo.encoder = "h264_nvenc";
        cachedGpuInfo.compressParams = "-c:v h264_nvenc -preset p4 -cq 26 -pix_fmt yuv420p";
        cachedGpuInfo.speedParams = "-c:v h264_nvenc -preset p4 -cq 23 -pix_fmt yuv420p";
        cachedGpuInfo.enhanceParamsLevel1 = "-c:v h264_nvenc -preset p3 -cq 20 -pix_fmt yuv420p";
        cachedGpuInfo.enhanceParamsLevel2 = "-c:v h264_nvenc -preset p5 -cq 22 -pix_fmt yuv420p";
        cachedGpuInfo.enhanceParamsLevel3 = "-c:v h264_nvenc -preset p7 -cq 24 -pix_fmt yuv420p";
        cachedGpuInfo.displayName = "NVIDIA NVENC (GPU Tăng tốc phần cứng)";
        hasDetectedGpu = true;
        return cachedGpuInfo;
    }

    // 2. Kiểm tra Intel QuickSync (QSV)
    string testQsv = ffmpeg + " -hide_banner -loglevel error -f lavfi -i color=s=64x64:d=0.1 -c:v h264_qsv -f null -";
    if (SystemCore::runRawCommand(testQsv)) {
        cachedGpuInfo.encoder = "h264_qsv";
        cachedGpuInfo.compressParams = "-c:v h264_qsv -global_quality 26 -pix_fmt yuv420p";
        cachedGpuInfo.speedParams = "-c:v h264_qsv -global_quality 23 -pix_fmt yuv420p";
        cachedGpuInfo.enhanceParamsLevel1 = "-c:v h264_qsv -preset fast -global_quality 20 -pix_fmt yuv420p";
        cachedGpuInfo.enhanceParamsLevel2 = "-c:v h264_qsv -preset medium -global_quality 22 -pix_fmt yuv420p";
        cachedGpuInfo.enhanceParamsLevel3 = "-c:v h264_qsv -preset slow -global_quality 24 -pix_fmt yuv420p";
        cachedGpuInfo.displayName = "Intel QuickSync (GPU Tăng tốc phần cứng)";
        hasDetectedGpu = true;
        return cachedGpuInfo;
    }

    // 3. Kiểm tra AMD AMF
    string testAmf = ffmpeg + " -hide_banner -loglevel error -f lavfi -i color=s=64x64:d=0.1 -c:v h264_amf -f null -";
    if (SystemCore::runRawCommand(testAmf)) {
        cachedGpuInfo.encoder = "h264_amf";
        cachedGpuInfo.compressParams = "-c:v h264_amf -rc cqp -qp_p 26 -qp_i 26 -pix_fmt yuv420p";
        cachedGpuInfo.speedParams = "-c:v h264_amf -rc cqp -qp_p 23 -qp_i 23 -pix_fmt yuv420p";
        cachedGpuInfo.enhanceParamsLevel1 = "-c:v h264_amf -quality speed -rc cqp -qp_p 20 -qp_i 20 -pix_fmt yuv420p";
        cachedGpuInfo.enhanceParamsLevel2 = "-c:v h264_amf -quality balanced -rc cqp -qp_p 22 -qp_i 22 -pix_fmt yuv420p";
        cachedGpuInfo.enhanceParamsLevel3 = "-c:v h264_amf -quality quality -rc cqp -qp_p 24 -qp_i 24 -pix_fmt yuv420p";
        cachedGpuInfo.displayName = "AMD AMF (GPU Tăng tốc phần cứng)";
        hasDetectedGpu = true;
        return cachedGpuInfo;
    }

    // 4. Fallback CPU
    cachedGpuInfo.encoder = "libx264";
    cachedGpuInfo.compressParams = "-c:v libx264 -crf 24 -preset fast -pix_fmt yuv420p";
    cachedGpuInfo.speedParams = "-c:v libx264 -crf 23 -preset fast -pix_fmt yuv420p";
    cachedGpuInfo.enhanceParamsLevel1 = "-c:v libx264 -crf 18 -preset fast -pix_fmt yuv420p";
    cachedGpuInfo.enhanceParamsLevel2 = "-c:v libx264 -crf 20 -preset medium -pix_fmt yuv420p";
    cachedGpuInfo.enhanceParamsLevel3 = "-c:v libx264 -crf 22 -preset slow -pix_fmt yuv420p";
    cachedGpuInfo.displayName = "CPU (libx264 Software Encoder)";
    hasDetectedGpu = true;
    return cachedGpuInfo;
}

void MediaProcessor::compressImage(const string& inputPath, const string& outputPath, int quality) {
    string ffmpeg = getFFmpegPath();
    string cmd = ffmpeg + " -y -i \"" + inputPath + "\" -map_metadata 0 -movflags +faststart -q:v " + to_string(quality) + " \"" + outputPath + "\"";
    cout << " \x1b[35m[Media]\x1b[0m Đang tối ưu dung lượng ảnh...";
    if (SystemCore::runRawCommand(cmd)) cout << "\n Thành công: " << outputPath << "\n";
    else cout << "\n Xử lý thất bại hoặc sai đường dẫn!\n";
}

void MediaProcessor::extractAudioCore(const std::string& inputPath, const std::string& outputPath) {
    std::string ffmpeg = getFFmpegPath();
    std::string cmd = ffmpeg + " -y -i \"" + inputPath + "\" -map_metadata 0 -vn -q:a 2 \"" + outputPath + "\"";
    SystemCore::runRawCommand(cmd);
}

void MediaProcessor::changeSpeedCore(const std::string& inputPath, const std::string& outputPath, float speedMultiplier) {
    std::string ffmpeg = getFFmpegPath();
    GpuCodecInfo gpu = getGpuEncoder();
    float videoPts = 1.0f / speedMultiplier;
    
    std::ostringstream ossSpeed, ossPts;
    ossSpeed << std::fixed << std::setprecision(2) << speedMultiplier;
    ossPts << std::fixed << std::setprecision(4) << videoPts;
    
    std::string speedStr = ossSpeed.str();
    std::string ptsStr = ossPts.str();
    
    std::string audioFilter = "atempo=" + speedStr;
    if (speedMultiplier < 0.5f) audioFilter = "atempo=0.5";
    if (speedMultiplier > 2.0f) audioFilter = "atempo=2.0";

    std::string filter = "-vf \"setpts=" + ptsStr + "*PTS\" -af \"" + audioFilter + "\"";
    
    std::string cmd = ffmpeg + " -y -i \"" + inputPath + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 " + filter + " " + gpu.speedParams + " -c:a aac \"" + outputPath + "\"";
    SystemCore::runRawCommand(cmd);
}

void MediaProcessor::processMediaAuto() {
    bool hasPreviousRun = false;
    int totalTotalFiles = 0;
    int totalOptimizedCount = 0;
    int totalSkippedCount = 0;
    long long totalBytesSaved = 0;

    while (true) {
        std::cin.clear();
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        std::cout << std::flush;
        system("cls");

        if (hasPreviousRun) {
            cout << "\n\n";
            cout << "  Số file đầu vào   : " << totalTotalFiles << "\n"
                 << "  Đã giải phóng     : " << SystemCore::formatSize(totalBytesSaved) << "\n"
                 << "  Số file tối ưu    : " << totalOptimizedCount << "\n"
                 << "  Số file giữ nguyên: " << totalSkippedCount << "\n\n";
        }
        cout << "\nKéo thả các file (0 để thoát): ";
        string rawInput;
        getline(cin, rawInput);
        
        if (rawInput == "0" || rawInput.empty()) {
            cout << "\nQuay lại menu chính.\n";
            Sleep(1000);
            return;
        }

        vector<string> inputs = SystemCore::parsePaths(rawInput);

        if (inputs.empty()) {
            cout << "\n    Không tìm thấy file hợp lệ!\n"
                 << "    Thử lại sau 2 giây...\n";
            Sleep(2000);
            continue;
        }

        cout << "\nPhát hiện " << inputs.size() << " file đang được phân tích...\n";

        int currentOptimizedCount = 0; 
        int currentSkippedCount = 0;   
        long long currentBytesSaved = 0; 
        
        GpuCodecInfo gpu = getGpuEncoder();

        vector<string> imageExts = { ".jpg", ".jpeg", ".png", ".bmp", ".webp", ".tiff", ".heic" };
        vector<string> videoExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm" };

        for (size_t i = 0; i < inputs.size(); ++i) {
            string input = inputs[i];
            fs::path inPath(input);
            cout << " [" << i + 1 << "/" << inputs.size() << "] Xử lý: " << inPath.filename().string() << "\n";

            if (!fs::exists(inPath)) {
                cout << "    File không tồn tại!\n\n";
                continue;
            }

            string ext = inPath.extension().string();
            transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            fs::path tempOutPath;
            if (ext == ".heic") {
                tempOutPath = inPath.parent_path() / (inPath.stem().string() + "_temp_compressed.jpg");
            } else {
                tempOutPath = inPath.parent_path() / (inPath.stem().string() + "_temp_compressed" + ext);
            }
            bool renderSuccess = false;

            // Kiểm tra dung lượng file trước khi nén
            uintmax_t originalSize = 0;
            try {
                originalSize = fs::file_size(inPath);
                // Nếu file < 50KB, bỏ qua
                if (originalSize < 50 * 1024) {
                    cout << "Bỏ qua: File quá nhỏ (< 50KB)\n\n";
                    currentSkippedCount++;
                    continue;
                }
            } catch (...) {
                cout << "Lỗi đọc dung lượng file!\n\n";
                continue;
            }

            if (find(imageExts.begin(), imageExts.end(), ext) != imageExts.end()) {
                if (ext == ".heic") {
                    string ffmpeg = getFFmpegPath();
                    string cmd = ffmpeg + " -y -hide_banner -loglevel error -i \"" + input + "\" -map_metadata 0 -movflags +faststart -q:v 5 \"" + tempOutPath.string() + "\"";
                    cout << " \x1b[35m[Media]\x1b[0m Đang chuyển HEIC sang JPG...";
                    renderSuccess = SystemCore::runRawCommand(cmd) && fs::exists(tempOutPath);
                } else {
                    compressImage(input, tempOutPath.string(), 5);
                    renderSuccess = fs::exists(tempOutPath);
                }
                
                // Fix orientation
                if (renderSuccess && fs::exists(tempOutPath)) {
                    string ffmpeg = getFFmpegPath();
                    string tempFixPath = tempOutPath.string() + ".fix";
                    string fixCmd = ffmpeg + " -y -hide_banner -loglevel error -i \"" + tempOutPath.string() + "\" -map_metadata 0 -metadata:s:v:0 rotate=0 -c copy \"" + tempFixPath + "\"";
                    if (SystemCore::runRawCommand(fixCmd) && fs::exists(tempFixPath)) {
                        fs::remove(tempOutPath);
                        fs::rename(tempFixPath, tempOutPath);
                    } else {
                        if (fs::exists(tempFixPath)) fs::remove(tempFixPath);
                    }
                }
            }
            else if (find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
                string ffmpeg = getFFmpegPath();
                string cmd = ffmpeg + " -y -hide_banner -loglevel error -i \"" + input + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 " + gpu.compressParams + " -c:a aac -b:a 128k \"" + tempOutPath.string() + "\"";
                cout << " \x1b[35m[Media]\x1b[0m Đang tối ưu Video (" << gpu.encoder << ")...";
                renderSuccess = SystemCore::runRawCommand(cmd) && fs::exists(tempOutPath);
            }
            else {
                cout << "\nBỏ qua: Định dạng " << ext << " không hỗ trợ!\n\n";
                continue;
            }

            // Xử lý kết quả
            if (renderSuccess && fs::exists(tempOutPath)) {
                try {
                    uintmax_t compressedSize = fs::file_size(tempOutPath);

                    if (compressedSize < originalSize) {
                        currentBytesSaved += (originalSize - compressedSize);
                        
                        if (fs::exists(inPath)) {
                            fs::remove(inPath);
                        }
                        fs::rename(tempOutPath, inPath);
                        currentOptimizedCount++;
                        
                        float ratio = (1.0f - (float)compressedSize / originalSize) * 100;
                        cout << "\nĐã nén: " << SystemCore::formatSize(originalSize - compressedSize) << " (" << fixed << setprecision(1) << ratio << "%)\n\n";
                    } 
                    else {
                        fs::remove(tempOutPath);
                        currentSkippedCount++;
                        cout << "\nBỏ qua: File đã tối ưu\n\n";
                    }
                } 
                catch (const std::exception& e) {
                    cout << "\nLỗi: " << e.what() << "\n\n";
                    if (fs::exists(tempOutPath)) fs::remove(tempOutPath);
                }
                catch (...) {
                    cout << "\nFile bị lỗi!\n\n";
                    if (fs::exists(tempOutPath)) fs::remove(tempOutPath);
                }
            } 
            else {
                if (fs::exists(tempOutPath)) fs::remove(tempOutPath);
                cout << "\nRender thất bại!\n\n";
            }
            
            fflush(stdout);
        }
        
        totalTotalFiles += inputs.size();
        totalOptimizedCount += currentOptimizedCount;
        totalSkippedCount += currentSkippedCount;
        totalBytesSaved += currentBytesSaved; 
        hasPreviousRun = true;

        cout << "\n============\n"
             << "  Đã xử lý xong " << inputs.size() << " file!\n"
             << "============\n"
             << " Tự động quay về menu thống kê sau 2 giây...\n";
        cout.flush();
        
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        
        for (int i = 2; i > 0; i--) {
            Sleep(1000);
            cout << " " << i << "... ";
            cout.flush();
        }
        cout << "\n";
        Sleep(100);
    }
}

void MediaProcessor::processExtractAudioBatch() {
    cout << "Kéo thả các video để lấy âm thanh: ";
    string rawInput;
    getline(cin, rawInput);
    vector<string> inputs = SystemCore::parsePaths(rawInput);
    
    if (inputs.empty()) {
        cout << "Chưa nhập file!\n";
        SystemCore::waitEnter();
        return;
    }

    cout << "\nPhát hiện " << inputs.size() << " file cần trích âm thanh...\n";
    int successCount = 0;
    vector<string> videoExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm" };

    for (size_t i = 0; i < inputs.size(); ++i) {
        fs::path inPath(inputs[i]);
        cout << " [" << i + 1 << "/" << inputs.size() << "] Đang trích: " << inPath.filename().string() << "\n";

        if (!fs::exists(inPath)) {
            cout << "    File không tồn tại!\n";
            continue;
        }

        string ext = inPath.extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
            fs::path outPath = inPath.parent_path() / (inPath.stem().string() + ".mp3");
            extractAudioCore(inputs[i], outPath.string());
            cout << "    " << outPath.filename().string() << "\n";
            successCount++;
        } else {
            cout << "    Bỏ qua: Sai định dạng!\n";
        }
    }
    cout << "\nHoàn thành: Đã trích " << successCount << "/" << inputs.size() << " âm thanh!\n";
    SystemCore::waitEnter();
}

void MediaProcessor::processChangeSpeedBatch() {
    cout << "Kéo thả các video: ";
    string rawInput;
    getline(cin, rawInput);
    std::vector<std::string> inputs = SystemCore::parsePaths(rawInput);
    
    if (inputs.empty()) {
        cout << "Chưa nhập file!\n";
        SystemCore::waitEnter();
        return;
    }

    cout << "Tốc độ mong muốn (0.5: Chậm, 2.0: Nhanh): ";
    std::string speedStr;
    getline(cin, speedStr);
    speedStr = SystemCore::trim(speedStr);
    float speed = 1.0f;
    try { speed = stof(speedStr); } catch(...) { speed = 1.0f; }

    if (speed < 0.5f || speed > 2.0f) {
        cout << "Hệ thống hỗ trợ tốc độ từ 0.5x đến 2.0x để tiếng không bị méo!\n";
        SystemCore::waitEnter();
        return;
    }

    cout << "\nĐang đổi tốc độ (" << speed << "x) cho " << inputs.size() << " video...\n";
    int successCount = 0;
    std::vector<std::string> videoExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm" };

    for (size_t i = 0; i < inputs.size(); ++i) {
        fs::path inPath(inputs[i]);
        cout << " [" << i + 1 << "/" << inputs.size() << "] Đang render: " << inPath.filename().string() << "\n";

        if (!fs::exists(inPath)) {
            cout << "File không tồn tại!\n";
            continue;
        }

        std::string ext = inPath.extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
            std::string speedSuffix = "_speed_" + speedStr + "x.mp4";
            fs::path outPath = inPath.parent_path() / (inPath.stem().string() + speedSuffix);
            changeSpeedCore(inputs[i], outPath.string(), speed);
            cout << "    " << outPath.filename().string() << "\n";
            successCount++;
        } else {
            cout << "Bỏ qua: Sai định dạng!\n";
        }
    }
    cout << "\nHoàn thành: Đã xử lý " << successCount << "/" << inputs.size() << " video!\n";
    SystemCore::waitEnter();
}

void MediaProcessor::processMediaEnhancementAuto() {
    std::vector<std::string> imageExts = { ".jpg", ".jpeg", ".png", ".bmp", ".webp", ".heic" };
    std::vector<std::string> videoExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm", ".m4v" };
    std::string ffmpeg = getFFmpegPath();
    GpuCodecInfo gpu = getGpuEncoder();

    while (true) {
        std::cout << std::flush;
        system("cls");

        std::cout << "BỘ TỰ ĐỘNG PHỤC CHẾ & LÀM NÉT (AI AUTO)\n"
                  << "Phần cứng: " << gpu.displayName << "\n\n"
                  << "Kéo thả file Ảnh/Video để làm nét (0 để thoát): ";
        string rawInput;
        getline(cin, rawInput);
        std::vector<std::string> inputs = SystemCore::parsePaths(rawInput);
        
        if (inputs.empty()) {
            std::cout << "\nQuay lại menu chính.\n";
            return;
        }

        std::cout << "[1] Nét nhẹ & Mịn da\n"
                  << "[2] Phục hồi chi tiết\n"
                  << "[3] Siêu nét \n";
        int level = SystemCore::readInt("Chọn cấp độ: ");
        if (level < 1 || level > 3) level = 2;

        std::string imgFilter, vidFilter, vidCodec;
        if (level == 1) {
            imgFilter = "nlmeans=s=1.8:p=7:r=3,eq=saturation=1.12:contrast=1.06,unsharp=5:5:0.8:5:5:0.0";
            vidFilter = "nlmeans=s=1.2:p=7:r=3,eq=saturation=1.1:contrast=1.05,unsharp=5:5:0.8:5:5:0.0";
            vidCodec  = gpu.enhanceParamsLevel1; 
        } 
        else if (level == 2) {
            imgFilter = "nlmeans=s=2.0:p=7:r=3,eq=saturation=1.15:contrast=1.1,unsharp=5:5:1.2:5:5:0.0,cas=strength=0.5";
            vidFilter = "nlmeans=s=1.5:p=7:r=3,eq=saturation=1.15:contrast=1.1,unsharp=5:5:1.0:5:5:0.0";
            vidCodec  = gpu.enhanceParamsLevel2;
        } 
        else {
            imgFilter = "nlmeans=s=2.4:p=7:r=5,eq=saturation=1.2:contrast=1.12:brightness=0.01,unsharp=7:7:1.5:7:7:0.0,cas=strength=0.7";
            vidFilter = "nlmeans=s=1.8:p=7:r=3,eq=saturation=1.2:contrast=1.12,unsharp=7:7:1.3:7:7:0.0";
            vidCodec  = gpu.enhanceParamsLevel3;
        }

        std::cout << "\nĐang phân tích và xử lý " << inputs.size() << " file...\n\n";

        for (size_t i = 0; i < inputs.size(); ++i) {
            fs::path inPath(inputs[i]);
            std::cout << " [" << i + 1 << "/" << inputs.size() << "] Đang tối ưu: " << inPath.filename().string() << "\n";

            if (!fs::exists(inPath)) {
                std::cout << "    File không tồn tại!\n\n";
                continue;
            }

            std::string ext = inPath.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            fs::path outPath = inPath.parent_path() / (inPath.stem().string() + "_enhanced" + ext);
            
            std::string cmd = "";
            bool isSupported = false;

            if (std::find(imageExts.begin(), imageExts.end(), ext) != imageExts.end()) {
                cmd = ffmpeg + " -y -i \"" + inputs[i] + "\" -map_metadata 0 -vf \"" + imgFilter + "\" -q:v 2 \"" + outPath.string() + "\"";
                std::cout << " \x1b[35m[AI-Image]\x1b[0m Đang tái cấu trúc pixel & khử nhiễu...";
                isSupported = true;
            }
            else if (std::find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
                cmd = ffmpeg + " -y -i \"" + inputs[i] + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 -vf \"" + vidFilter + "\" " + vidCodec + " -c:a copy \"" + outPath.string() + "\"";
                std::cout << " \x1b[35m[AI-Video]\x1b[0m Đang nội suy khung hình & tăng tương phản...";
                isSupported = true;
            }
            else {
                std::cout << "Bỏ qua: Định dạng " << ext << " không hỗ trợ!\n\n";
                continue;
            }

            if (isSupported) {
                if (SystemCore::runRawCommand(cmd) && fs::exists(outPath)) {
                    std::cout << "\n    Thành công: " << outPath.filename().string() << "\n\n";
                } else {
                    std::cout << "\n    Xử lý thất bại!\n\n";
                }
            }
        }
    }
}


void MediaProcessor::processConvertFormatBatch() {
    while (true) {
        std::cout << std::flush;
        system("cls");
        
        cout << "BỘ CHUYỂN ĐỔI ĐỊNH DẠNG (GIỮ NGUYÊN CHẤT LƯỢNG)\n\n";

        cout<< "Kéo thả các file ảnh/video (0 để thoát): ";
        string rawInput;
        getline(cin, rawInput);
        vector<string> inputs = SystemCore::parsePaths(rawInput);

        if (inputs.empty()) {
            cout << "\nQuay lại menu chính\n";
            return;
        }

        cout << "\nPhát hiện " << inputs.size() << " file...\n\n";

        vector<string> imageExts = { ".jpg", ".jpeg", ".png", ".bmp", ".webp", ".tiff", ".heic" };
        vector<string> videoExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm" };

        bool hasImage = false;
        bool hasVideo = false;
        for (const string& input : inputs) {
            fs::path p(input);
            string ext = p.extension().string();
            transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (find(imageExts.begin(), imageExts.end(), ext) != imageExts.end()) hasImage = true;
            if (find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) hasVideo = true;
        }

        if (hasImage && hasVideo) {
            cout << "Không kéo thả lẫn ảnh và video!\n";
            continue;
        }

        string targetExt;
        string ffmpeg = getFFmpegPath();

        if (hasImage) {
            cout << "Chọn định dạng ảnh đầu ra:\n"
                 << "  [1] .jpg  [2] .png  [3] .webp  [0] Hủy\n";

            int choice = SystemCore::readInt("Chọn: ");
            if (choice == 0) continue;
            if (choice == 1) targetExt = ".jpg";
            else if (choice == 2) targetExt = ".png";
            else if (choice == 3) targetExt = ".webp";
            else {
                cout << "Lựa chọn không hợp lệ!\n";
                continue;
            }

            cout << "\nĐang chuyển đổi ảnh sang " << targetExt << "...\n\n";

            for (size_t i = 0; i < inputs.size(); ++i) {
                string input = inputs[i];
                fs::path inPath(input);
                cout << " [" << i + 1 << "/" << inputs.size() << "] " << inPath.filename().string() << "\n";

                if (!fs::exists(inPath)) {
                    cout << "    File không tồn tại!\n";
                    continue;
                }

                string ext = inPath.extension().string();
                transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (find(imageExts.begin(), imageExts.end(), ext) == imageExts.end()) {
                    cout << "    Bỏ qua: Không phải file ảnh!\n";
                    continue;
                }

                fs::path outPath = inPath.parent_path() / (inPath.stem().string() + targetExt);
                string cmd;

                if (ext == ".jpg" || ext == ".jpeg") {
                    if (targetExt == ".png" || targetExt == ".webp") {
                        cout << "Cảnh báo: JPG sang PNG/WEBP có thể mất EXIF metadata\n";
                    }
                }

                if (targetExt == ".jpg" || targetExt == ".jpeg") {
                    cmd = ffmpeg + " -y -i \"" + input + "\" -map_metadata 0 -q:v 2 \"" + outPath.string() + "\"";
                } else if (targetExt == ".png") {
                    cmd = ffmpeg + " -y -i \"" + input + "\" -map_metadata 0 -lossless 0 -compression_level 6 \"" + outPath.string() + "\"";
                } else { // .webp
                    cmd = ffmpeg + " -y -i \"" + input + "\" -map_metadata 0 -q:v 90 \"" + outPath.string() + "\"";
                }

                cout << "    Đang chuyển đổi...";
                bool success = SystemCore::runRawCommand(cmd);

                if (success && fs::exists(outPath)) {
                    try {
                        fs::remove(inPath);
                        cout << " OK: " << outPath.filename().string() << "\n";
                    } catch (...) {
                        cout << " (Lỗi xóa file gốc)\n";
                    }
                } else {
                    if (fs::exists(outPath)) fs::remove(outPath);
                    cout << " Chuyển đổi thất bại!\n";
                }
            }

        } else if (hasVideo) {
            cout << " [1] .mp4  [2] .mkv  [3] .mov  [0] Hủy\n\n";
            int choice = SystemCore::readInt("Định dạng video đầu ra: ");
            if (choice == 0) continue;
            if (choice == 1) targetExt = ".mp4";
            else if (choice == 2) targetExt = ".mkv";
            else if (choice == 3) targetExt = ".mov";
            else {
                cout << "Lựa chọn không hợp lệ!\n";
                continue;
            }

            cout << "\nĐang chuyển đổi video sang " << targetExt << "...\n\n";

            for (size_t i = 0; i < inputs.size(); ++i) {
                string input = inputs[i];
                fs::path inPath(input);
                cout << " [" << i + 1 << "/" << inputs.size() << "] " << inPath.filename().string() << "\n";

                if (!fs::exists(inPath)) {
                    cout << "File không tồn tại!\n";
                    continue;
                }

                string ext = inPath.extension().string();
                transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (find(videoExts.begin(), videoExts.end(), ext) == videoExts.end()) {
                    cout << "Bỏ qua: Không phải file video!\n";
                    continue;
                }

                fs::path outPath = inPath.parent_path() / (inPath.stem().string() + targetExt);
                
                bool incompatible = false;
                
                if (targetExt == ".avi") {
                    if (ext == ".mkv" || ext == ".mov") {
                        incompatible = true;
                        cout << "    Cảnh báo: AVI có thể không hỗ trợ codec H.265/HEVC!\n";
                    }
                }
                
                if (targetExt == ".mp4") {
                    if (ext == ".mov") {
                        incompatible = true;
                        cout << "    Cảnh báo: MP4 có thể không hỗ trợ codec ProRes/DNxHD!\n";
                    }
                }
                
                if (targetExt == ".mov") {
                    if (ext == ".mkv" || ext == ".webm") {
                        incompatible = true;
                        cout << "    Cảnh báo: MOV có thể không hỗ trợ codec VP9/AV1!\n";
                    }
                }

                string cmd;
                if (incompatible) {
                    GpuCodecInfo gpu = getGpuEncoder();
                    cout << "    Đang chuyển đổi codec (" << gpu.encoder << ")...\n";
                    cmd = ffmpeg + " -y -i \"" + input + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 " + gpu.speedParams + " -c:a aac \"" + outPath.string() + "\"";
                } else {
                    cmd = ffmpeg + " -y -i \"" + input + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 -c copy \"" + outPath.string() + "\"";
                }

                cout << "    Đang chuyển đổi...";
                bool success = SystemCore::runRawCommand(cmd);

                if (success && fs::exists(outPath)) {
                    try {
                        fs::remove(inPath);
                        cout << " OK: " << outPath.filename().string() << "\n";
                    } catch (...) {
                        cout << "(Lỗi xóa file gốc)\n";
                    }
                } else {
                    if (fs::exists(outPath)) fs::remove(outPath);
                    cout << "Chuyển đổi thất bại!\n";
                }
            }

        } else {
            cout << "Không phát hiện file ảnh hoặc video hợp lệ!\n";
        }
    }
}


//  CHUẨN HÓA TÊN FILE MEDIA
void MediaProcessor::normalizeMediaFilenames() {
    system("cls");
    std::cout << "\n    CHUẨN HÓA TÊN FILE ẢNH, VIDEO, ÂM THANH\n\n"
              << "Nhập đường dẫn thư mục: ";
    std::string dirPath;
    std::getline(std::cin, dirPath);
    dirPath = SystemCore::trim(dirPath);
    
    if (dirPath.length() >= 2 && dirPath.front() == '"' && dirPath.back() == '"') {
        dirPath = dirPath.substr(1, dirPath.length() - 2);
    }

    if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
        std::cout << "\nĐường dẫn không tồn tại hoặc không phải thư mục!\n";
        return;
    }

    std::vector<std::string> imgExts = { ".png", ".jpg", ".jpeg", ".bmp", ".webp" };
    std::vector<std::string> vidExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv" };
    std::vector<std::string> audExts = { ".mp3", ".wav", ".aac", ".flac" };
    std::vector<std::string> allExts = imgExts;
    allExts.insert(allExts.end(), vidExts.begin(), vidExts.end());
    allExts.insert(allExts.end(), audExts.begin(), audExts.end());

    std::vector<std::filesystem::path> filesToRename;
    
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (!std::filesystem::is_regular_file(entry.path())) continue;
        
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (std::find(allExts.begin(), allExts.end(), ext) != allExts.end()) {
            filesToRename.push_back(entry.path());
        }
    }

    if (filesToRename.empty()) {
        std::cout << "\nKhông tìm thấy file media trong thư mục!\n";
        SystemCore::waitEnter();
        return;
    }

    std::cout << "\nPhát hiện " << filesToRename.size() << " file cần chuẩn hóa.\n"
              << "Bạn có muốn thực hiện đổi tên? (Y/N): ";
    std::string confirm;
    std::getline(std::cin, confirm);
    if (confirm != "y" && confirm != "Y") {
        std::cout << "Đã hủy.\n";
        SystemCore::waitEnter();
        return;
    }

    std::cout << "\nBắt đầu chuẩn hóa...\n";
    int successCount = 0;

    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    std::mt19937_64 rng(timestamp);

    for (const auto& oldPath : filesToRename) {
        std::string ext = oldPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        std::string prefix;
        if (std::find(imgExts.begin(), imgExts.end(), ext) != imgExts.end()) prefix = "IMG_";
        else if (std::find(vidExts.begin(), vidExts.end(), ext) != vidExts.end()) prefix = "VD_";
        else if (std::find(audExts.begin(), audExts.end(), ext) != audExts.end()) prefix = "MP3_";
        else continue;

        long long uniqueNumber = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        uniqueNumber += rng() % 1000; 

        std::string newName = prefix + std::to_string(uniqueNumber) + ext;
        std::filesystem::path newPath = oldPath.parent_path() / newName;

        int counter = 1;
        while (std::filesystem::exists(newPath)) {
            newName = prefix + std::to_string(uniqueNumber) + "_" + std::to_string(counter) + ext;
            newPath = oldPath.parent_path() / newName;
            counter++;
        }

        try {
            std::filesystem::rename(oldPath, newPath);
            std::cout << "  " << oldPath.filename().string() << " -> " << newName << "\n";
            successCount++;
        } catch (const std::exception& e) {
            std::cout << "  Lỗi đổi tên: " << oldPath.filename().string() << " (" << e.what() << ")\n";
        }
    }

    std::cout << "\nHoàn thành! Đã chuẩn hóa " << successCount << "/" << filesToRename.size() << " file.\n\n";
    SystemCore::waitEnter();
}

// Hàm XOR mã hóa/giải mã (dùng key 0xAA)
static void xorCipher(std::vector<uint8_t>& data, uint8_t key = 0xAA) {
    for (auto& byte : data) {
        byte ^= key;
    }
}

// Core tổng quát để ghép file vào Container (Ảnh/Video)
bool MediaProcessor::embedFileIntoContainerCore(const std::string& containerPath, const std::string& hiddenFilePath, 
                                               const std::string& outputPath, uintmax_t maxContainerSize, std::string& errorMsg) {
    if (!fs::exists(containerPath)) {
        errorMsg = "File chứa không tồn tại: " + containerPath;
        return false;
    }
    if (!fs::exists(hiddenFilePath)) {
        errorMsg = "File cần ẩn không tồn tại: " + hiddenFilePath;
        return false;
    }

    uintmax_t containerSize = fs::file_size(containerPath);
    if (containerSize > maxContainerSize) {
        errorMsg = "File nền quá nặng (" + SystemCore::formatSize(containerSize) + "). Giới hạn: " + SystemCore::formatSize(maxContainerSize);
        return false;
    }

    std::ifstream inCover(containerPath, std::ios::binary);
    if (!inCover) {
        errorMsg = "Không thể mở file nền!";
        return false;
    }
    std::vector<uint8_t> coverData((std::istreambuf_iterator<char>(inCover)), std::istreambuf_iterator<char>());
    inCover.close();

    std::ifstream inPayload(hiddenFilePath, std::ios::binary);
    if (!inPayload) {
        errorMsg = "Không thể mở file ẩn!";
        return false;
    }
    std::vector<uint8_t> payloadData((std::istreambuf_iterator<char>(inPayload)), std::istreambuf_iterator<char>());
    inPayload.close();

    if (payloadData.empty()) {
        errorMsg = "File ẩn rỗng (0 bytes)!";
        return false;
    }

    xorCipher(payloadData, 0xAA);

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        errorMsg = "Không thể tạo file xuất: " + outputPath;
        return false;
    }

    out.write(reinterpret_cast<const char*>(coverData.data()), coverData.size());
    out.write(reinterpret_cast<const char*>(payloadData.data()), payloadData.size());

    uint32_t payloadSize = static_cast<uint32_t>(payloadData.size());
    uint8_t sizeBytes[4];
    sizeBytes[0] = static_cast<uint8_t>((payloadSize >> 24) & 0xFF);
    sizeBytes[1] = static_cast<uint8_t>((payloadSize >> 16) & 0xFF);
    sizeBytes[2] = static_cast<uint8_t>((payloadSize >> 8) & 0xFF);
    sizeBytes[3] = static_cast<uint8_t>(payloadSize & 0xFF);
    out.write(reinterpret_cast<const char*>(sizeBytes), 4);

    const char magic[4] = {'H', 'I', 'D', 'E'};
    out.write(magic, 4);
    out.close();

    if (!fs::exists(outputPath)) {
        errorMsg = "Lỗi lưu file xuất!";
        return false;
    }

    return true;
}

// Core giấu file vào ảnh (<= 10MB)
bool MediaProcessor::hideFileInImageCore(const std::string& imagePath, const std::string& hiddenFilePath, 
                                        const std::string& outputPath, std::string& errorMsg) {
    const uintmax_t MAX_IMG_SIZE = 10ULL * 1024 * 1024; // 10MB
    return embedFileIntoContainerCore(imagePath, hiddenFilePath, outputPath, MAX_IMG_SIZE, errorMsg);
}

// Core giấu file vào video (<= 100MB)
bool MediaProcessor::hideFileInVideoCore(const std::string& videoPath, const std::string& hiddenFilePath, 
                                        const std::string& outputPath, std::string& errorMsg) {
    const uintmax_t MAX_VIDEO_SIZE = 100ULL * 1024 * 1024; // 100MB
    return embedFileIntoContainerCore(videoPath, hiddenFilePath, outputPath, MAX_VIDEO_SIZE, errorMsg);
}

// Core trích xuất file ẩn từ Media
bool MediaProcessor::extractHiddenFromMediaCore(const std::string& containerPath, const std::string& outputPath, std::string& errorMsg) {
    if (!fs::exists(containerPath)) {
        errorMsg = "File chứa không tồn tại: " + containerPath;
        return false;
    }

    std::ifstream in(containerPath, std::ios::binary);
    if (!in) {
        errorMsg = "Không thể mở file chứa!";
        return false;
    }

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    if (buffer.size() >= 8 &&
        buffer[buffer.size() - 4] == 'H' && buffer[buffer.size() - 3] == 'I' &&
        buffer[buffer.size() - 2] == 'D' && buffer[buffer.size() - 1] == 'E') {
        
        uint32_t hiddenSize = 0;
        hiddenSize |= (static_cast<uint32_t>(buffer[buffer.size() - 8]) << 24);
        hiddenSize |= (static_cast<uint32_t>(buffer[buffer.size() - 7]) << 16);
        hiddenSize |= (static_cast<uint32_t>(buffer[buffer.size() - 6]) << 8);
        hiddenSize |= (static_cast<uint32_t>(buffer[buffer.size() - 5]));

        if (hiddenSize == 0 || buffer.size() < (8 + hiddenSize)) {
            errorMsg = "Dữ liệu file ẩn bị lỗi!";
            return false;
        }

        size_t startPos = buffer.size() - 8 - hiddenSize;
        std::vector<uint8_t> hiddenData(buffer.begin() + startPos, buffer.begin() + startPos + hiddenSize);

        xorCipher(hiddenData, 0xAA);

        std::ofstream out(outputPath, std::ios::binary);
        if (!out) {
            errorMsg = "Không thể tạo file xuất: " + outputPath;
            return false;
        }
        out.write(reinterpret_cast<const char*>(hiddenData.data()), hiddenData.size());
        out.close();
        return true;
    }

    errorMsg = "Không tìm thấy file ẩn!";
    return false;
}

// 1. Giấu file bí mật vào Ảnh
void MediaProcessor::hideFileInImage() {
    system("cls");
    std::cout << "\n   ẨN FILE TRONG ẢNH (Bìa Ảnh <= 10MB)\n\n"
              << "Nhập đường dẫn Ảnh nền (jpg/png...): ";
    std::string imagePath;
    std::getline(std::cin, imagePath);
    imagePath = SystemCore::trim(imagePath);
    if (!imagePath.empty() && imagePath.front() == '"' && imagePath.back() == '"') {
        imagePath = imagePath.substr(1, imagePath.length() - 2);
    }

    std::cout << "Nhập đường dẫn File cần ẩn (zip/txt/exe...): ";
    std::string hiddenFilePath;
    std::getline(std::cin, hiddenFilePath);
    hiddenFilePath = SystemCore::trim(hiddenFilePath);
    if (!hiddenFilePath.empty() && hiddenFilePath.front() == '"' && hiddenFilePath.back() == '"') {
        hiddenFilePath = hiddenFilePath.substr(1, hiddenFilePath.length() - 2);
    }

    std::cout << "Nhập tên file đầu ra (vd: final.jpg): ";
    std::string outputFileName;
    std::getline(std::cin, outputFileName);
    outputFileName = SystemCore::trim(outputFileName);
    if (!outputFileName.empty() && outputFileName.front() == '"' && outputFileName.back() == '"') {
        outputFileName = outputFileName.substr(1, outputFileName.length() - 2);
    }

    if (imagePath.empty() || hiddenFilePath.empty() || outputFileName.empty()) {
        std::cout << "\nĐường dẫn không được để trống!\n";
        return;
    }

    std::cout << "\nĐang nhúng file...\n";
    std::string errorMsg;
    if (hideFileInImageCore(imagePath, hiddenFilePath, outputFileName, errorMsg)) {
        uintmax_t outputSize = fs::file_size(outputFileName);
        std::cout << "\nThành công! File đầu ra: " << outputFileName 
                  << " (" << SystemCore::formatSize(outputSize) << ")\n"
                  << "File xem như ảnh thường. Dùng chức năng trích xuất để lấy file ẩn.\n";
    } else {
        std::cout << "\nLỗi: " << errorMsg << "\n";
    }
}

// 2. Giấu file bí mật vào Video
void MediaProcessor::hideFileInVideo() {
    system("cls");
    std::cout << "\n   ẨN FILE TRONG VIDEO (Bìa Video <= 100MB)\n\n"
              << "Nhập đường dẫn Video nền (mp4/mkv...): ";
    std::string videoPath;
    std::getline(std::cin, videoPath);
    videoPath = SystemCore::trim(videoPath);
    if (!videoPath.empty() && videoPath.front() == '"' && videoPath.back() == '"') {
        videoPath = videoPath.substr(1, videoPath.length() - 2);
    }

    std::cout << "Nhập đường dẫn File cần ẩn (zip/txt/exe...): ";
    std::string hiddenFilePath;
    std::getline(std::cin, hiddenFilePath);
    hiddenFilePath = SystemCore::trim(hiddenFilePath);
    if (!hiddenFilePath.empty() && hiddenFilePath.front() == '"' && hiddenFilePath.back() == '"') {
        hiddenFilePath = hiddenFilePath.substr(1, hiddenFilePath.length() - 2);
    }

    std::cout << "Nhập tên file đầu ra (vd: final.mp4): ";
    std::string outputFileName;
    std::getline(std::cin, outputFileName);
    outputFileName = SystemCore::trim(outputFileName);
    if (!outputFileName.empty() && outputFileName.front() == '"' && outputFileName.back() == '"') {
        outputFileName = outputFileName.substr(1, outputFileName.length() - 2);
    }

    if (videoPath.empty() || hiddenFilePath.empty() || outputFileName.empty()) {
        std::cout << "\nĐường dẫn không được để trống!\n";
        return;
    }

    std::cout << "\nĐang nhúng file...\n";
    std::string errorMsg;
    if (hideFileInVideoCore(videoPath, hiddenFilePath, outputFileName, errorMsg)) {
        uintmax_t outputSize = fs::file_size(outputFileName);
        std::cout << "\nThành công! File đầu ra: " << outputFileName 
                  << " (" << SystemCore::formatSize(outputSize) << ")\n"
                  << "File xem bình thường. Dùng chức năng trích xuất để lấy file ẩn.\n";
    } else {
        std::cout << "\nLỗi: " << errorMsg << "\n";
    }
}

// 3. Dò tìm & Trích xuất file ẩn
void MediaProcessor::extractHiddenFromMedia() {
    system("cls");
    std::cout << "\n   DÒ TÌM & TRÍCH XUẤT FILE ẨN TỪ MEDIA\n\n"
              << "Nhập đường dẫn File chứa (ảnh/video): ";
    std::string in;
    std::getline(std::cin, in);
    in = SystemCore::trim(in);
    if (!in.empty() && in.front() == '"' && in.back() == '"') {
        in = in.substr(1, in.length() - 2);
    }

    if (in.empty()) {
        std::cout << "\nĐường dẫn không được để trống!\n";
        return;
    }

    if (!fs::exists(in)) {
        std::cout << "\nFile không tồn tại!\n";
        return;
    }

    long long now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    fs::path containerPath(in);
    std::string outputPath = (containerPath.parent_path() / ("extracted_" + std::to_string(now) + ".bin")).string();

    std::cout << "\nĐang phân tích & trích xuất...\n";
    std::string errorMsg;
    if (extractHiddenFromMediaCore(in, outputPath, errorMsg)) {
        std::cout << "\nĐã trích xuất thành công!\n"
                  << "File lưu tại : " << outputPath << "\n"
                  << "Dung lượng   : " << SystemCore::formatSize(fs::file_size(outputPath)) << "\n"
                  << "(Gợi ý: Bạn có thể đổi đuôi .bin thành .zip / .txt / .png / .exe tương ứng với định dạng gốc)\n";
    } else {
        std::cout << "\nLỗi: " << errorMsg << "\n";
    }
}

// HÀM MẸ: ẨN FILE TRONG FILE (SUBMENU)
void MediaProcessor::processAnFileTrongFile() {
    while (true) {
        system("cls");
        std::cout << " [1] Giấu file bí mật vào Ảnh (<= 10MB)\n"
                  << " [2] Giấu file bí mật vào Video (<= 100MB)\n"
                  << " [3] Dò tìm & Trích xuất file ẩn từ Media\n"
                  << " [0] Quay lại\n\n"
                  << " [Chọn]: ";

        int choice = SystemCore::readInt("");
        if (choice == 0) break;

        switch (choice) {
        case 1:
            hideFileInImage();
            SystemCore::waitEnter();
            break;
        case 2:
            hideFileInVideo();
            SystemCore::waitEnter();
            break;
        case 3:
            extractHiddenFromMedia();
            SystemCore::waitEnter();
            break;
        default:
            std::cout << "\nLựa chọn không hợp lệ!\n";
            Sleep(300);
            break;
        }
    }
}

// BỘ PHÂN TÍCH & TRÍCH XUẤT METADATA (CÁCH 2)
bool MediaProcessor::extractMetadataCore(const std::string& inputPath, std::string& outputReportPath, std::string& summaryInfo, std::string& errorMsg) {
    if (!fs::exists(inputPath)) {
        errorMsg = "File không tồn tại: " + inputPath;
        return false;
    }

    fs::path inP(inputPath);
    std::string jsonPath = (inP.parent_path() / (inP.stem().string() + "_metadata.json")).string();
    std::string txtPath = (inP.parent_path() / (inP.stem().string() + "_metadata.txt")).string();

    string ffprobe = getFFprobePath();
    string ffmpeg = getFFmpegPath();

    string probeCmd = ffprobe + " -v quiet -print_format json -show_format -show_streams \"" + inputPath + "\" > \"" + jsonPath + "\"";
    bool probeOk = (SystemCore::runRawCommand("cmd /c " + probeCmd) && fs::exists(jsonPath) && fs::file_size(jsonPath) > 20);

    if (!probeOk) {
        string metaCmd = ffmpeg + " -y -i \"" + inputPath + "\" -f ffmetadata \"" + txtPath + "\"";
        SystemCore::runRawCommand(metaCmd);
    }

    outputReportPath = probeOk ? jsonPath : txtPath;

    std::stringstream ss;
    uintmax_t fsize = fs::file_size(inputPath);
    ss << "  Tên tập tin    : " << inP.filename().string() << "\n"
       << "  Dung lượng     : " << SystemCore::formatSize(fsize) << "\n";

    if (probeOk) {
        std::ifstream jf(jsonPath);
        if (jf) {
            std::string content((std::istreambuf_iterator<char>(jf)), std::istreambuf_iterator<char>());
            jf.close();

            auto extractValue = [&](const std::string& key) -> std::string {
                size_t p = content.find("\"" + key + "\"");
                if (p == std::string::npos) return "";
                p = content.find(":", p);
                if (p == std::string::npos) return "";
                p = content.find_first_not_of(" \t\r\n", p + 1);
                if (p == std::string::npos) return "";
                if (content[p] == '"') {
                    size_t endP = content.find('"', p + 1);
                    if (endP != std::string::npos) return content.substr(p + 1, endP - p - 1);
                } else {
                    size_t endP = content.find_first_of(",}\r\n", p);
                    if (endP != std::string::npos) return content.substr(p, endP - p);
                }
                return "";
            };

            std::string formatName = extractValue("format_long_name");
            if (formatName.empty()) formatName = extractValue("format_name");
            std::string duration = extractValue("duration");
            std::string bitRate = extractValue("bit_rate");
            std::string title = extractValue("title");
            std::string artist = extractValue("artist");
            std::string encoder = extractValue("encoder");
            std::string creationTime = extractValue("creation_time");

            if (!formatName.empty()) ss << "  Định dạng       : " << formatName << "\n";
            if (!duration.empty()) {
                try {
                    double durSec = std::stod(duration);
                    int h = (int)durSec / 3600;
                    int m = ((int)durSec % 3600) / 60;
                    int s = (int)durSec % 60;
                    char dBuf[64];
                    sprintf_s(dBuf, sizeof(dBuf), "%02d:%02d:%02d (%.2f s)", h, m, s, durSec);
                    ss << "  Thời lượng     : " << dBuf << "\n";
                } catch (...) {
                    ss << "  Thời lượng     : " << duration << "s\n";
                }
            }
            if (!bitRate.empty()) {
                try {
                    long long br = std::stoll(bitRate);
                    ss << "  Bitrate tổng   : " << (br / 1000) << " kbps\n";
                } catch (...) {
                    ss << "  Bitrate tổng   : " << bitRate << "\n";
                }
            }

            // Luồng Video
            size_t vPos = content.find("\"codec_type\": \"video\"");
            if (vPos != std::string::npos) {
                size_t streamStart = content.rfind("{", vPos);
                size_t streamEnd = content.find("}", vPos);
                if (streamStart != std::string::npos && streamEnd != std::string::npos) {
                    std::string streamBlock = content.substr(streamStart, streamEnd - streamStart);
                    auto extractStreamVal = [&](const std::string& k) -> std::string {
                        size_t kp = streamBlock.find("\"" + k + "\"");
                        if (kp == std::string::npos) return "";
                        kp = streamBlock.find(":", kp);
                        if (kp == std::string::npos) return "";
                        kp = streamBlock.find_first_not_of(" \t\r\n", kp + 1);
                        if (kp == std::string::npos) return "";
                        if (streamBlock[kp] == '"') {
                            size_t ep = streamBlock.find('"', kp + 1);
                            if (ep != std::string::npos) return streamBlock.substr(kp + 1, ep - kp - 1);
                        } else {
                            size_t ep = streamBlock.find_first_of(",}\r\n", kp);
                            if (ep != std::string::npos) return streamBlock.substr(kp, ep - kp);
                        }
                        return "";
                    };
                    std::string vCodec = extractStreamVal("codec_name");
                    std::string w = extractStreamVal("width");
                    std::string h = extractStreamVal("height");
                    std::string fps = extractStreamVal("r_frame_rate");
                    ss << "  Luồng Video    : " << vCodec << " (" << w << "x" << h << ")";
                    if (!fps.empty() && fps != "0/0") ss << " @ " << fps << " fps";
                    ss << "\n";
                }
            }

            // Luồng Audio
            size_t aPos = content.find("\"codec_type\": \"audio\"");
            if (aPos != std::string::npos) {
                size_t streamStart = content.rfind("{", aPos);
                size_t streamEnd = content.find("}", aPos);
                if (streamStart != std::string::npos && streamEnd != std::string::npos) {
                    std::string streamBlock = content.substr(streamStart, streamEnd - streamStart);
                    auto extractStreamVal = [&](const std::string& k) -> std::string {
                        size_t kp = streamBlock.find("\"" + k + "\"");
                        if (kp == std::string::npos) return "";
                        kp = streamBlock.find(":", kp);
                        if (kp == std::string::npos) return "";
                        kp = streamBlock.find_first_not_of(" \t\r\n", kp + 1);
                        if (kp == std::string::npos) return "";
                        if (streamBlock[kp] == '"') {
                            size_t ep = streamBlock.find('"', kp + 1);
                            if (ep != std::string::npos) return streamBlock.substr(kp + 1, ep - kp - 1);
                        } else {
                            size_t ep = streamBlock.find_first_of(",}\r\n", kp);
                            if (ep != std::string::npos) return streamBlock.substr(kp, ep - kp);
                        }
                        return "";
                    };
                    std::string aCodec = extractStreamVal("codec_name");
                    std::string sRate = extractStreamVal("sample_rate");
                    std::string ch = extractStreamVal("channels");
                    ss << "  Luồng Audio    : " << aCodec << " (" << ch << " channels, " << sRate << " Hz)\n";
                }
            }

            // Thẻ Tag
            if (!title.empty()) ss << "   Tiêu đề        : " << title << "\n";
            if (!artist.empty()) ss << "   Nghệ sĩ        : " << artist << "\n";
            if (!creationTime.empty()) ss << "   Ngày tạo       : " << creationTime << "\n";
            if (!encoder.empty()) ss << "   Trình mã hóa   : " << encoder << "\n";
        }
    } else {
        std::ifstream tf(txtPath);
        if (tf) {
            std::string line;
            while (std::getline(tf, line)) {
                line = SystemCore::trim(line);
                if (!line.empty() && line[0] != ';') {
                    ss << "   " << line << "\n";
                }
            }
            tf.close();
        }
    }

    summaryInfo = ss.str();
    return true;
}

void MediaProcessor::processExtractMetadata() {
    while (true) {
        std::cout << std::flush;
        system("cls");

        std::cout << "BỘ PHÂN TÍCH & TRÍCH XUẤT METADATA MEDIA (PRO)\n\n"
                  << "Kéo thả file Ảnh/Video/Audio (0 để thoát): ";
        std::string rawInput;
        std::getline(std::cin, rawInput);
        std::vector<std::string> inputs = SystemCore::parsePaths(rawInput);

        if (inputs.empty()) {
            std::cout << "\nQuay lại menu chính.\n";
            return;
        }

        std::cout << "\nĐang phân tích " << inputs.size() << " file...\n";
        for (size_t i = 0; i < inputs.size(); ++i) {
            std::cout << "\n------------------------------------------------------------\n"
                      << " [" << i + 1 << "/" << inputs.size() << "] KẾT QUẢ PHÂN TÍCH METADATA:\n"
                      << "------------------------------------------------------------\n";

            std::string outPath, summary, errorMsg;
            if (extractMetadataCore(inputs[i], outPath, summary, errorMsg)) {
                std::cout << summary
                          << "\nĐã xuất Metadata ra file:\n"
                          << "   " << outPath << "\n";
            } else {
                std::cout << "Lỗi: " << errorMsg << "\n";
            }
        }

        std::cout << "\n";
        SystemCore::waitEnter();
    }
}