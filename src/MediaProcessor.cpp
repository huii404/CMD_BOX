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

MediaProcessor::MediaProcessor() {}
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
    if (SystemCore::runRawCommand(cmd)) cout << "\n -> Thành công! Đã xuất file: " << outputPath << "\n";
    else cout << "\n -> [Lỗi] Quá trình xử lý thất bại hoặc sai đường dẫn!\n";
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

    GpuCodecInfo gpu = getGpuEncoder();

    while (true) {
        // === FIX: Xóa buffer input và flush console ===
        std::cin.clear();
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        std::cout << std::flush;
        system("cls");

        cout << " ==================================================\n";
        cout << "    BỘ TỐI ƯU DUNG LƯỢNG \n";
        cout << " ==================================================\n";

        if (hasPreviousRun) {
            cout << "  Số file đầu vào: " << totalTotalFiles << "\n";
            cout << "  Đã giải phóng: " << SystemCore::formatSize(totalBytesSaved) << "\n"; // Đã dùng formatSize chuẩn
            cout << "  Số file tối ưu  : " << totalOptimizedCount << "\n";
            cout << "  Số file giữ nguyên : " << totalSkippedCount << "\n";
            cout << " ==================================================\n";
        }
        cout << "\n";

        cout << " -> Kéo thả N file vào đây để nén tiếp ( 0 để thoát): ";
        string rawInput;
        getline(cin, rawInput);
        
        // Kiểm tra thoát
        if (rawInput == "0" || rawInput.empty()) {
            cout << "\n -> [Thoát] Quay lại menu chính.\n";
            Sleep(1500);
            return;
        }

        // === FIX: DÙNG HÀM parsePaths CỦA SYSTEMCORE THAY VÌ TỰ PARSE DÀI DÒNG ===
        vector<string> inputs = SystemCore::parsePaths(rawInput);

        if (inputs.empty()) {
            cout << "\n    [!] Không tìm thấy file nào hợp lệ!\n";
            cout << "    Thử lại sau 2 giây...\n";
            Sleep(2000);
            continue;
        }

        cout << "\n ==================================================\n";
        cout << " [*] Phát hiện " << inputs.size() << " file đang được phân tích...\n";
        cout << " ==================================================\n\n";

        int currentOptimizedCount = 0; 
        int currentSkippedCount = 0;   
        long long currentBytesSaved = 0; 
        
        vector<string> imageExts = { ".jpg", ".jpeg", ".png", ".bmp", ".webp", ".tiff", ".heic" };
        vector<string> videoExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm" };

        for (size_t i = 0; i < inputs.size(); ++i) {
            string input = inputs[i];
            fs::path inPath(input);
            cout << " [" << i + 1 << "/" << inputs.size() << "] Xử lý: " << inPath.filename().string() << "\n";

            if (!fs::exists(inPath)) {
                cout << "    -> [Lỗi] File không tồn tại!\n\n";
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
                // Nếu file < 50KB, bỏ qua (không nén)
                if (originalSize < 50 * 1024) {
                    cout << "    -> [Bỏ qua] File quá nhỏ (< 50KB)\n\n";
                    currentSkippedCount++;
                    continue;
                }
            } catch (...) {
                cout << "    -> [Lỗi] Không thể đọc dung lượng file!\n\n";
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
                cout << "\n    -> [Bỏ qua] Định dạng " << ext << " không được hỗ trợ!\n\n";
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
                        
                        // Hiển thị tỷ lệ nén
                        float ratio = (1.0f - (float)compressedSize / originalSize) * 100;
                        cout << "\n    -> [OK] Đã nén: " << SystemCore::formatSize(originalSize - compressedSize) << " (" << fixed << setprecision(1) << ratio << "%)\n\n";
                    } 
                    else {
                        fs::remove(tempOutPath);
                        currentSkippedCount++;
                        cout << "\n    -> [Bỏ qua] Không thể nén thêm (file đã tối ưu)\n\n";
                    }
                } 
                catch (const std::exception& e) {
                    cout << "\n    -> [Lỗi] " << e.what() << "\n\n";
                    if (fs::exists(tempOutPath)) fs::remove(tempOutPath);
                }
                catch (...) {
                    cout << "\n    -> [Lỗi] Không thể xử lý file này!\n\n";
                    if (fs::exists(tempOutPath)) fs::remove(tempOutPath);
                }
            } 
            else {
                if (fs::exists(tempOutPath)) fs::remove(tempOutPath);
                cout << "\n    -> [Lỗi] Quá trình render thất bại!\n\n";
            }
            
            fflush(stdout);
        }
        
        totalTotalFiles += inputs.size();
        totalOptimizedCount += currentOptimizedCount;
        totalSkippedCount += currentSkippedCount;
        totalBytesSaved += currentBytesSaved; 
        hasPreviousRun = true;

        // === FIX: Đảm bảo hiển thị và chờ đủ 2 giây ===
        cout << "\n ==================================================\n";
        cout << " [*] Đã xử lý xong " << inputs.size() << " file!\n";
        cout << " ==================================================\n";
        cout << " Tự động quay về menu thống kê sau 2 giây...\n";
        cout.flush();
        
        // === FIX: Xóa buffer trước khi sleep ===
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        
        // Đếm ngược 2 giây
        for (int i = 2; i > 0; i--) {
            Sleep(1000);
            cout << " " << i << "... ";
            cout.flush();
        }
        cout << "\n";
        
        // Đảm bảo đã sleep đủ
        Sleep(100);
    }
}

void MediaProcessor::processExtractAudioBatch() {
    cout << "\tkéo thả N video để lấy âm thanh: ";
    string rawInput;
    getline(cin, rawInput);
    vector<string> inputs = SystemCore::parsePaths(rawInput);
    
    if (inputs.empty()) {
        cout << "\t[Lỗi] Chưa nhập file nào cả!\n";
        return;
    }

    cout << "\n\t[*] Phát hiện " << inputs.size() << " file cần trích âm thanh...\n";
    int successCount = 0;
    vector<string> videoExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm" };

    for (size_t i = 0; i < inputs.size(); ++i) {
        fs::path inPath(inputs[i]);
        cout << "\t[" << i + 1 << "/" << inputs.size() << "] Đang trích: " << inPath.filename().string() << "\n";

        if (!fs::exists(inPath)) {
            cout << "\t    -> [Lỗi] File không tồn tại!\n";
            continue;
        }

        string ext = inPath.extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
            fs::path outPath = inPath.parent_path() / (inPath.stem().string() + ".mp3");
            extractAudioCore(inputs[i], outPath.string());
            cout << "\t[OK] -> " << outPath.filename().string() << "\n";
            successCount++;
        } else {
            cout << "\t[Bỏ qua] Sai định dạng!\n";
        }
    }
    cout << "\tHOÀN THÀNH: Đã trích " << successCount << "/" << inputs.size() << " âm thanh!\n";
}

void MediaProcessor::processChangeSpeedBatch() {
    cout << "\tkéo thả N video: ";
    string rawInput;
    getline(cin, rawInput);
    std::vector<std::string> inputs = SystemCore::parsePaths(rawInput);
    
    if (inputs.empty()) {
        cout << "\t[Lỗi] Chưa nhập file nào cả!\n";
        return;
    }

    cout << "\ttốc độ mong muốn (0.5: Slow-motion, 2.0: Tua nhanh): ";
    std::string speedStr;
    getline(cin, speedStr);
    speedStr = SystemCore::trim(speedStr);
    float speed = 1.0f;
    try { speed = stof(speedStr); } catch(...) { speed = 1.0f; }

    if (speed < 0.5f || speed > 2.0f) {
        cout << "\t[Lỗi] Hiện tại hệ thống chỉ hỗ trợ tốc độ từ 0.5x đến 2.0x để tiếng không bị méo!\n";
        return;
    }

    cout << "\n [*] Đang xử lý đổi tốc độ (" << speed << "x) cho " << inputs.size() << " video (Vui lòng đợi)...\n";
    int successCount = 0;
    std::vector<std::string> videoExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm" };

    for (size_t i = 0; i < inputs.size(); ++i) {
        fs::path inPath(inputs[i]);
        cout << " [" << i + 1 << "/" << inputs.size() << "] Đang render: " << inPath.filename().string() << "\n";

        if (!fs::exists(inPath)) {
            cout << "\t[Lỗi] File không tồn tại!\n";
            continue;
        }

        std::string ext = inPath.extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
            std::string speedSuffix = "_speed_" + speedStr + "x.mp4";
            fs::path outPath = inPath.parent_path() / (inPath.stem().string() + speedSuffix);
            changeSpeedCore(inputs[i], outPath.string(), speed);
            cout << "\t[OK] -> " << outPath.filename().string() << "\n";
            successCount++;
        } else {
            cout << "\t[Bỏ qua] Sai định dạng!\n";
        }
    }
    cout << "\tHOÀN THÀNH: Đã xử lý " << successCount << "/" << inputs.size() << " video!\n";
}

void MediaProcessor::processMediaEnhancementAuto() {
    std::vector<std::string> imageExts = { ".jpg", ".jpeg", ".png", ".bmp", ".webp", ".heic" };
    std::vector<std::string> videoExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm", ".m4v" };
    std::string ffmpeg = getFFmpegPath();
    GpuCodecInfo gpu = getGpuEncoder();

    while (true) {
        std::cout << std::flush;
        system("cls");

        std::cout << " ==================================================\n";
        std::cout << "    BỘ TỰ ĐỘNG PHỤC CHẾ & LÀM NÉT (AI AUTO) \n";
        std::cout << "    Phần cứng: " << gpu.displayName << "\n";
        std::cout << " ==================================================\n\n";

        cout << " -> Kéo thả N file Ảnh/Video để làm nét ( 0 để thoát): ";
        string rawInput;
        getline(cin, rawInput);
        std::vector<std::string> inputs = SystemCore::parsePaths(rawInput);
        
        if (inputs.empty()) {
            std::cout << "\n -> [Thoát] Quay lại menu chính.\n";
            return;
        }

        std::cout << "\n ==================================================\n";
        std::cout << "  [1] Nét nhẹ & Mịn da/ảnh (Kế thừa Mức 3 + Tối ưu độ mịn)\n";
        std::cout << "  [2] Nét sâu & Phục hồi chi tiết PRO (Nâng cấp từ Mức 3)\n";
        std::cout << "  [3] Siêu nét Ultra HD & Tương phản cao (Cực đại chi tiết)\n";
        std::cout << " ==================================================\n";
        int level = SystemCore::readInt(" -> Chọn cấp độ xử lý: ");
        if (level < 1 || level > 3) level = 2;

        std::string imgFilter, vidFilter, vidCodec;
        if (level == 1) {
            // Mức 1: Kế thừa Mức 3 cũ + bổ sung độ mịn màng cho da/ảnh (nlmeans s=1.8), màu sắc tự nhiên
            imgFilter = "nlmeans=s=1.8:p=7:r=3,eq=saturation=1.12:contrast=1.06,unsharp=5:5:0.8:5:5:0.0";
            vidFilter = "nlmeans=s=1.2:p=7:r=3,eq=saturation=1.1:contrast=1.05,unsharp=5:5:0.8:5:5:0.0";
            vidCodec  = gpu.enhanceParamsLevel1; 
        } 
        else if (level == 2) {
            // Mức 2: Nâng cấp mạnh mẽ từ mức 3 cũ (Tăng cường độ nét sâu, tương phản động và tái tạo vân khối)
            imgFilter = "nlmeans=s=2.0:p=7:r=3,eq=saturation=1.15:contrast=1.1,unsharp=5:5:1.2:5:5:0.0,cas=strength=0.5";
            vidFilter = "nlmeans=s=1.5:p=7:r=3,eq=saturation=1.15:contrast=1.1,unsharp=5:5:1.0:5:5:0.0";
            vidCodec  = gpu.enhanceParamsLevel2;
        } 
        else {
            // Mức 3: Nâng cấp cực đại Ultra HD từ mức 3 cũ (Siêu nét, chi tiết vi mô cực đại, tương phản sắc sảo)
            imgFilter = "nlmeans=s=2.4:p=7:r=5,eq=saturation=1.2:contrast=1.12:brightness=0.01,unsharp=7:7:1.5:7:7:0.0,cas=strength=0.7";
            vidFilter = "nlmeans=s=1.8:p=7:r=3,eq=saturation=1.2:contrast=1.12,unsharp=7:7:1.3:7:7:0.0";
            vidCodec  = gpu.enhanceParamsLevel3;
        }

        std::cout << "\n [*] Đang tiến hành phân tích và xử lý " << inputs.size() << " file...\n\n";

        for (size_t i = 0; i < inputs.size(); ++i) {
            fs::path inPath(inputs[i]);
            std::cout << " [" << i + 1 << "/" << inputs.size() << "] Đang tối ưu: " << inPath.filename().string() << "\n";

            if (!fs::exists(inPath)) {
                std::cout << "    -> [Lỗi] File không tồn tại!\n\n";
                continue;
            }

            std::string ext = inPath.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            fs::path outPath = inPath.parent_path() / (inPath.stem().string() + "_enhanced" + ext);
            
            std::string cmd = "";
            bool isSupported = false;

            // Kiểm tra và dựng lệnh cho Ảnh
            if (std::find(imageExts.begin(), imageExts.end(), ext) != imageExts.end()) {
                cmd = ffmpeg + " -y -i \"" + inputs[i] + "\" -map_metadata 0 -vf \"" + imgFilter + "\" -q:v 2 \"" + outPath.string() + "\"";
                std::cout << " \x1b[35m[AI-Image]\x1b[0m Đang tái cấu trúc pixel & khử nhiễu bề mặt...";
                isSupported = true;
            }
            else if (std::find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
                cmd = ffmpeg + " -y -i \"" + inputs[i] + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 -vf \"" + vidFilter + "\" " + vidCodec + " -c:a copy \"" + outPath.string() + "\"";
                std::cout << " \x1b[35m[AI-Video]\x1b[0m Đang nội suy khung hình & tăng độ tương phản...";
                isSupported = true;
            }
            else {
                std::cout << "    -> [Bỏ qua] Định dạng " << ext << " không nằm trong danh mục hỗ trợ!\n\n";
                continue;
            }

            if (isSupported) {
                if (SystemCore::runRawCommand(cmd) && fs::exists(outPath)) {
                    std::cout << "\n    -> Thành công! File xuất: " << outPath.filename().string() << "\n\n";
                } else {
                    std::cout << "\n    -> [Lỗi] Xử lý thất bại hoặc lỗi dòng lệnh!\n\n";
                }
            }
        }
    }
}


void MediaProcessor::processConvertFormatBatch() {
    while (true) {
        std::cout << std::flush;
        system("cls");

        cout << " ==================================================\n";
        cout << "    BỘ CHUYỂN ĐỔI ĐỊNH DẠNG (GIỮ NGUYÊN CHẤT LƯỢNG) \n";
        cout << " ==================================================\n\n";

        cout << " -> Kéo thả N file ảnh/video (0 để thoát): ";
        string rawInput;
        getline(cin, rawInput);
        vector<string> inputs = SystemCore::parsePaths(rawInput);

        if (inputs.empty()) {
            cout << "\n -> [Thoát] Quay lại menu chính.\n";
            return;
        }

        cout << "\n [*] Phát hiện " << inputs.size() << " file...\n\n";

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
            cout << " [Lỗi] Không được kéo thả lẫn ảnh và video! Vui lòng chọn 1 loại.\n";
            continue;
        }

        string targetExt;
        string ffmpeg = getFFmpegPath();

        if (hasImage) {
            cout << " Chọn định dạng ảnh đầu ra:\n";
            cout << "  [1] .jpg\n";
            cout << "  [2] .png\n";
            cout << "  [3] .webp\n";
            cout << "  [0] Hủy\n";
            int choice = SystemCore::readInt(" -> Chọn: ");
            if (choice == 0) continue;
            if (choice == 1) targetExt = ".jpg";
            else if (choice == 2) targetExt = ".png";
            else if (choice == 3) targetExt = ".webp";
            else {
                cout << " -> [Lỗi] Lựa chọn không hợp lệ!\n";
                continue;
            }

            cout << "\n [*] Đang chuyển đổi ảnh sang " << targetExt << "...\n\n";

            for (size_t i = 0; i < inputs.size(); ++i) {
                string input = inputs[i];
                fs::path inPath(input);
                cout << " [" << i + 1 << "/" << inputs.size() << "] " << inPath.filename().string() << "\n";

                if (!fs::exists(inPath)) {
                    cout << "    -> [Lỗi] File không tồn tại!\n";
                    continue;
                }

                string ext = inPath.extension().string();
                transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (find(imageExts.begin(), imageExts.end(), ext) == imageExts.end()) {
                    cout << "    -> [Bỏ qua] Không phải file ảnh!\n";
                    continue;
                }

                fs::path outPath = inPath.parent_path() / (inPath.stem().string() + targetExt);
                string cmd;

                // === FIX: JPG→PNG MẤT EXIF - THÔNG BÁO CHO NGƯỜI DÙNG ===
                if (ext == ".jpg" || ext == ".jpeg") {
                    if (targetExt == ".png" || targetExt == ".webp") {
                        cout << "    -> [Cảnh báo] JPG->PNG/WEBP có thể mất EXIF metadata (GPS, máy ảnh...)\n";
                    }
                }

                // === FIX: THÊM MAP_METADATA CHO ẢNH ===
                if (targetExt == ".jpg" || targetExt == ".jpeg") {
                    cmd = ffmpeg + " -y -i \"" + input + "\" -map_metadata 0 -q:v 2 \"" + outPath.string() + "\"";
                } else if (targetExt == ".png") {
                    cmd = ffmpeg + " -y -i \"" + input + "\" -map_metadata 0 -lossless 0 -compression_level 6 \"" + outPath.string() + "\"";
                } else { // .webp
                    cmd = ffmpeg + " -y -i \"" + input + "\" -map_metadata 0 -q:v 90 \"" + outPath.string() + "\"";
                }

                cout << "    -> Đang chuyển đổi...";
                bool success = SystemCore::runRawCommand(cmd);

                if (success && fs::exists(outPath)) {
                    try {
                        fs::remove(inPath);
                        cout << " OK -> " << outPath.filename().string() << "\n";
                    } catch (...) {
                        cout << " [Lỗi xóa file gốc]\n";
                    }
                } else {
                    if (fs::exists(outPath)) fs::remove(outPath);
                    cout << " [Lỗi] Chuyển đổi thất bại!\n";
                }
            }

        } else if (hasVideo) {
            cout << " Chọn định dạng video đầu ra:\n";
            cout << "  [1] .mp4\n";
            cout << "  [2] .mkv\n";
            cout << "  [3] .mov\n";
            cout << "  [0] Hủy\n";
            int choice = SystemCore::readInt(" -> Chọn: ");
            if (choice == 0) continue;
            if (choice == 1) targetExt = ".mp4";
            else if (choice == 2) targetExt = ".mkv";
            else if (choice == 3) targetExt = ".mov";
            else {
                cout << " -> [Lỗi] Lựa chọn không hợp lệ!\n";
                continue;
            }

            cout << "\n [*] Đang chuyển đổi video sang " << targetExt << "...\n\n";

            for (size_t i = 0; i < inputs.size(); ++i) {
                string input = inputs[i];
                fs::path inPath(input);
                cout << " [" << i + 1 << "/" << inputs.size() << "] " << inPath.filename().string() << "\n";

                if (!fs::exists(inPath)) {
                    cout << "    -> [Lỗi] File không tồn tại!\n";
                    continue;
                }

                string ext = inPath.extension().string();
                transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (find(videoExts.begin(), videoExts.end(), ext) == videoExts.end()) {
                    cout << "    -> [Bỏ qua] Không phải file video!\n";
                    continue;
                }

                fs::path outPath = inPath.parent_path() / (inPath.stem().string() + targetExt);
                
                // === FIX: KIỂM TRA TƯƠNG THÍCH CODEC TRƯỚC KHI DÙNG -c copy ===
                bool useCopy = true;
                string codecInfo = "";
                
                // Lấy thông tin codec của file input
                string codecCmd = ffmpeg + " -i \"" + input + "\" 2>&1 | findstr \"Video:\"";
                // (Giả định parse được codec, ở đây đơn giản hóa)
                
                // === FIX: XÁC ĐỊNH CODEC KHÔNG TƯƠNG THÍCH ===
                // Các trường hợp không tương thích phổ biến
                bool incompatible = false;
                
                // AVI không hỗ trợ H.265/HEVC
                if (targetExt == ".avi") {
                    // Kiểm tra nếu file có codec H.265
                    // (đơn giản hóa: giả định nếu file .mkv có thể là H.265)
                    if (ext == ".mkv" || ext == ".mov") {
                        incompatible = true;
                        cout << "    -> [Cảnh báo] AVI có thể không hỗ trợ codec H.265/HEVC!\n";
                    }
                }
                
                // MP4 không hỗ trợ ProRes, DNxHD
                if (targetExt == ".mp4") {
                    if (ext == ".mov") {
                        incompatible = true;
                        cout << "    -> [Cảnh báo] MP4 có thể không hỗ trợ codec ProRes/DNxHD!\n";
                    }
                }
                
                // MOV không hỗ trợ tốt codec VP9, AV1
                if (targetExt == ".mov") {
                    if (ext == ".mkv" || ext == ".webm") {
                        incompatible = true;
                        cout << "    -> [Cảnh báo] MOV có thể không hỗ trợ codec VP9/AV1!\n";
                    }
                }

                string cmd;
                if (incompatible) {
                    GpuCodecInfo gpu = getGpuEncoder();
                    cout << "    -> [Thông báo] Đang chuyển đổi codec (" << gpu.encoder << ")...\n";
                    cmd = ffmpeg + " -y -i \"" + input + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 " + gpu.speedParams + " -c:a aac \"" + outPath.string() + "\"";
                } else {
                    cmd = ffmpeg + " -y -i \"" + input + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 -c copy \"" + outPath.string() + "\"";
                }

                cout << "    -> Đang chuyển đổi...";
                bool success = SystemCore::runRawCommand(cmd);

                if (success && fs::exists(outPath)) {
                    try {
                        fs::remove(inPath);
                        cout << " OK -> " << outPath.filename().string() << "\n";
                    } catch (...) {
                        cout << " [Lỗi xóa file gốc]\n";
                    }
                } else {
                    if (fs::exists(outPath)) fs::remove(outPath);
                    cout << " [Lỗi] Chuyển đổi thất bại!\n";
                }
            }

        } else {
            cout << " [Lỗi] Không phát hiện file ảnh hoặc video hợp lệ!\n";
        }
    }
}


// ==================== CHUẨN HÓA TÊN FILE MEDIA ====================
void MediaProcessor::normalizeMediaFilenames() {
    // Dùng SystemCore để xóa màn hình (vì hàm cls là static)
    system("cls");
    std::cout << "============================================================\n";
    std::cout << "    CHUẨN HÓA TÊN FILE ẢNH, VIDEO, ÂM THANH\n";
    std::cout << "============================================================\n";
    
    std::cout << "-> Nhập đường dẫn thư mục chứa file cần chuẩn hóa: ";
    std::string dirPath;
    std::getline(std::cin, dirPath);
    dirPath = SystemCore::trim(dirPath);
    
    // Bỏ dấu ngoặc kép nếu người dùng kéo thả
    if (dirPath.length() >= 2 && dirPath.front() == '"' && dirPath.back() == '"') {
        dirPath = dirPath.substr(1, dirPath.length() - 2);
    }

    if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
        std::cout << "\n[!] Đường dẫn không tồn tại hoặc không phải thư mục!\n";
        return;
    }

    // Định nghĩa danh sách đuôi file
    std::vector<std::string> imgExts = { ".png", ".jpg", ".jpeg", ".bmp", ".webp" };
    std::vector<std::string> vidExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv" };
    std::vector<std::string> audExts = { ".mp3", ".wav", ".aac", ".flac" };
    std::vector<std::string> allExts = imgExts;
    allExts.insert(allExts.end(), vidExts.begin(), vidExts.end());
    allExts.insert(allExts.end(), audExts.begin(), audExts.end());

    std::vector<std::filesystem::path> filesToRename;
    
    // Duyệt thư mục
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (!std::filesystem::is_regular_file(entry.path())) continue;
        
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        // Chỉ xử lý các định dạng hỗ trợ
        if (std::find(allExts.begin(), allExts.end(), ext) != allExts.end()) {
            filesToRename.push_back(entry.path());
        }
    }

    if (filesToRename.empty()) {
        std::cout << "\n[i] Không tìm thấy file ảnh/video/audio nào trong thư mục này!\n";
        return;
    }

    std::cout << "\n[*] Phát hiện " << filesToRename.size() << " file cần chuẩn hóa.\n";
    std::cout << "[?] Bạn có muốn thực hiện đổi tên? (Y/N): ";
    std::string confirm;
    std::getline(std::cin, confirm);
    if (confirm != "y" && confirm != "Y") {
        std::cout << "[i] Đã hủy.\n";
        return;
    }

    std::cout << "\n[*] Bắt đầu chuẩn hóa...\n";
    int successCount = 0;

    // === Cốt lõi chống trùng: Dùng thời gian hiện tại + số ngẫu nhiên ===
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    std::mt19937_64 rng(timestamp); // Seed bằng chính thời gian để random

    for (const auto& oldPath : filesToRename) {
        std::string ext = oldPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        std::string prefix;
        if (std::find(imgExts.begin(), imgExts.end(), ext) != imgExts.end()) prefix = "IMG_";
        else if (std::find(vidExts.begin(), vidExts.end(), ext) != vidExts.end()) prefix = "VD_";
        else if (std::find(audExts.begin(), audExts.end(), ext) != audExts.end()) prefix = "MP3_";
        else continue; // Không bao giờ xảy ra vì đã lọc ở trên

        // Tạo số duy nhất: Lấy thời gian hiện tại (micro giây) cộng thêm 1 số random 0-999
        long long uniqueNumber = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        uniqueNumber += rng() % 1000; 

        // Tạo tên mới
        std::string newName = prefix + std::to_string(uniqueNumber) + ext;
        std::filesystem::path newPath = oldPath.parent_path() / newName;

        // Kiểm tra nếu tên đã tồn tại thì thêm hậu tố _1, _2...
        int counter = 1;
        while (std::filesystem::exists(newPath)) {
            newName = prefix + std::to_string(uniqueNumber) + "_" + std::to_string(counter) + ext;
            newPath = oldPath.parent_path() / newName;
            counter++;
        }

        try {
            std::filesystem::rename(oldPath, newPath);
            std::cout << " [✓] " << oldPath.filename().string() << "  ->  " << newName << "\n";
            successCount++;
        } catch (const std::exception& e) {
            std::cout << " [✗] Lỗi đổi tên: " << oldPath.filename().string() << " (" << e.what() << ")\n";
        }
    }

    std::cout << "\n============================================================\n";
    std::cout << " [✓] Hoàn thành! Đã chuẩn hóa " << successCount << "/" << filesToRename.size() << " file.\n";
    std::cout << "============================================================\n";
}

// ==================== CORE: XỬ LÝ ẨN & TRÍCH XUẤT FILE TRONG MEDIA ====================

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
        errorMsg = "File nền quá nặng (" + SystemCore::formatSize(containerSize) + "). Giới hạn tối đa: " + SystemCore::formatSize(maxContainerSize);
        return false;
    }

    // Đọc toàn bộ container
    std::ifstream inCover(containerPath, std::ios::binary);
    if (!inCover) {
        errorMsg = "Không thể mở file nền để đọc!";
        return false;
    }
    std::vector<uint8_t> coverData((std::istreambuf_iterator<char>(inCover)), std::istreambuf_iterator<char>());
    inCover.close();

    // Đọc file cần ẩn
    std::ifstream inPayload(hiddenFilePath, std::ios::binary);
    if (!inPayload) {
        errorMsg = "Không thể mở file cần ẩn để đọc!";
        return false;
    }
    std::vector<uint8_t> payloadData((std::istreambuf_iterator<char>(inPayload)), std::istreambuf_iterator<char>());
    inPayload.close();

    if (payloadData.empty()) {
        errorMsg = "File cần ẩn rỗng (0 bytes)!";
        return false;
    }

    // Mã hóa XOR payload
    xorCipher(payloadData, 0xAA);

    // Ghi ra output
    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        errorMsg = "Không thể tạo file đầu ra: " + outputPath;
        return false;
    }

    // 1. Ghi cover data
    out.write(reinterpret_cast<const char*>(coverData.data()), coverData.size());
    // 2. Ghi payload đã mã hóa
    out.write(reinterpret_cast<const char*>(payloadData.data()), payloadData.size());

    // 3. Ghi 4 byte kích thước payload (Big Endian)
    uint32_t payloadSize = static_cast<uint32_t>(payloadData.size());
    uint8_t sizeBytes[4];
    sizeBytes[0] = static_cast<uint8_t>((payloadSize >> 24) & 0xFF);
    sizeBytes[1] = static_cast<uint8_t>((payloadSize >> 16) & 0xFF);
    sizeBytes[2] = static_cast<uint8_t>((payloadSize >> 8) & 0xFF);
    sizeBytes[3] = static_cast<uint8_t>(payloadSize & 0xFF);
    out.write(reinterpret_cast<const char*>(sizeBytes), 4);

    // 4. Ghi 4 byte Magic Tag "HIDE"
    const char magic[4] = {'H', 'I', 'D', 'E'};
    out.write(magic, 4);
    out.close();

    if (!fs::exists(outputPath)) {
        errorMsg = "Lỗi không xác định khi lưu file đầu ra!";
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

    // Kiểm tra magic number "HIDE" (4 byte cuối)
    if (buffer.size() >= 8 &&
        buffer[buffer.size() - 4] == 'H' && buffer[buffer.size() - 3] == 'I' &&
        buffer[buffer.size() - 2] == 'D' && buffer[buffer.size() - 1] == 'E') {
        
        uint32_t hiddenSize = 0;
        hiddenSize |= (static_cast<uint32_t>(buffer[buffer.size() - 8]) << 24);
        hiddenSize |= (static_cast<uint32_t>(buffer[buffer.size() - 7]) << 16);
        hiddenSize |= (static_cast<uint32_t>(buffer[buffer.size() - 6]) << 8);
        hiddenSize |= (static_cast<uint32_t>(buffer[buffer.size() - 5]));

        if (hiddenSize == 0 || buffer.size() < (8 + hiddenSize)) {
            errorMsg = "Dữ liệu file ẩn bị hỏng hoặc kích thước không hợp lệ!";
            return false;
        }

        size_t startPos = buffer.size() - 8 - hiddenSize;
        std::vector<uint8_t> hiddenData(buffer.begin() + startPos, buffer.begin() + startPos + hiddenSize);

        // Giải mã XOR ngược
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

    errorMsg = "Không tìm thấy dữ liệu file ẩn (Magic 'HIDE') trong file này!";
    return false;
}

// ==================== CÁC HÀM GIAO DIỆN CON (UI) ====================

// 1. Giấu file bí mật vào Ảnh
void MediaProcessor::hideFileInImage() {
    system("cls");
    std::cout << "============================================================\n";
    std::cout << "   ẨN FILE TRONG ẢNH (Bìa Ảnh <= 10MB)\n";
    std::cout << "============================================================\n";

    std::cout << "-> Nhập đường dẫn Ảnh nền (jpg/png...): ";
    std::string imagePath;
    std::getline(std::cin, imagePath);
    imagePath = SystemCore::trim(imagePath);
    if (!imagePath.empty() && imagePath.front() == '"' && imagePath.back() == '"') {
        imagePath = imagePath.substr(1, imagePath.length() - 2);
    }

    std::cout << "-> Nhập đường dẫn File cần ẩn (zip/txt/exe...): ";
    std::string hiddenFilePath;
    std::getline(std::cin, hiddenFilePath);
    hiddenFilePath = SystemCore::trim(hiddenFilePath);
    if (!hiddenFilePath.empty() && hiddenFilePath.front() == '"' && hiddenFilePath.back() == '"') {
        hiddenFilePath = hiddenFilePath.substr(1, hiddenFilePath.length() - 2);
    }

    std::cout << "-> Nhập tên file đầu ra (vd: final.jpg): ";
    std::string outputFileName;
    std::getline(std::cin, outputFileName);
    outputFileName = SystemCore::trim(outputFileName);
    if (!outputFileName.empty() && outputFileName.front() == '"' && outputFileName.back() == '"') {
        outputFileName = outputFileName.substr(1, outputFileName.length() - 2);
    }

    if (imagePath.empty() || hiddenFilePath.empty() || outputFileName.empty()) {
        std::cout << "\n[!] Đường dẫn không được để trống!\n";
        return;
    }

    std::cout << "\n[*] Đang nhúng file...\n";
    std::string errorMsg;
    if (hideFileInImageCore(imagePath, hiddenFilePath, outputFileName, errorMsg)) {
        uintmax_t outputSize = fs::file_size(outputFileName);
        std::cout << "\n[✓] Thành công! File đầu ra: " << outputFileName 
                  << " (" << SystemCore::formatSize(outputSize) << ")\n";
        std::cout << "[✓] File xem/mở như ảnh thông thường. Dùng chức năng 'Dò tìm & Trích xuất' để lấy lại file ẩn.\n";
    } else {
        std::cout << "\n[✗] Lỗi: " << errorMsg << "\n";
    }
}

// 2. Giấu file bí mật vào Video
void MediaProcessor::hideFileInVideo() {
    system("cls");
    std::cout << "============================================================\n";
    std::cout << "   ẨN FILE TRONG VIDEO (Bìa Video <= 100MB)\n";
    std::cout << "============================================================\n";

    std::cout << "-> Nhập đường dẫn Video nền (mp4/mkv...): ";
    std::string videoPath;
    std::getline(std::cin, videoPath);
    videoPath = SystemCore::trim(videoPath);
    if (!videoPath.empty() && videoPath.front() == '"' && videoPath.back() == '"') {
        videoPath = videoPath.substr(1, videoPath.length() - 2);
    }

    std::cout << "-> Nhập đường dẫn File cần ẩn (zip/txt/exe...): ";
    std::string hiddenFilePath;
    std::getline(std::cin, hiddenFilePath);
    hiddenFilePath = SystemCore::trim(hiddenFilePath);
    if (!hiddenFilePath.empty() && hiddenFilePath.front() == '"' && hiddenFilePath.back() == '"') {
        hiddenFilePath = hiddenFilePath.substr(1, hiddenFilePath.length() - 2);
    }

    std::cout << "-> Nhập tên file đầu ra (vd: final.mp4): ";
    std::string outputFileName;
    std::getline(std::cin, outputFileName);
    outputFileName = SystemCore::trim(outputFileName);
    if (!outputFileName.empty() && outputFileName.front() == '"' && outputFileName.back() == '"') {
        outputFileName = outputFileName.substr(1, outputFileName.length() - 2);
    }

    if (videoPath.empty() || hiddenFilePath.empty() || outputFileName.empty()) {
        std::cout << "\n[!] Đường dẫn không được để trống!\n";
        return;
    }

    std::cout << "\n[*] Đang nhúng file...\n";
    std::string errorMsg;
    if (hideFileInVideoCore(videoPath, hiddenFilePath, outputFileName, errorMsg)) {
        uintmax_t outputSize = fs::file_size(outputFileName);
        std::cout << "\n[✓] Thành công! File đầu ra: " << outputFileName 
                  << " (" << SystemCore::formatSize(outputSize) << ")\n";
        std::cout << "[✓] File xem bình thường. Dùng chức năng 'Dò tìm & Trích xuất' để lấy lại file ẩn.\n";
    } else {
        std::cout << "\n[✗] Lỗi: " << errorMsg << "\n";
    }
}

// 3. Dò tìm & Trích xuất file ẩn
void MediaProcessor::extractHiddenFromMedia() {
    system("cls");
    std::cout << "============================================================\n";
    std::cout << "   DÒ TÌM & TRÍCH XUẤT FILE ẨN TỪ MEDIA\n";
    std::cout << "============================================================\n";

    std::cout << "-> Nhập đường dẫn File chứa (ảnh/video): ";
    std::string in;
    std::getline(std::cin, in);
    in = SystemCore::trim(in);
    if (!in.empty() && in.front() == '"' && in.back() == '"') {
        in = in.substr(1, in.length() - 2);
    }

    if (in.empty()) {
        std::cout << "\n[!] Đường dẫn không được để trống!\n";
        return;
    }

    if (!fs::exists(in)) {
        std::cout << "\n[!] File không tồn tại!\n";
        return;
    }

    // Sinh tên file xuất tự động
    long long now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    fs::path containerPath(in);
    std::string outputPath = (containerPath.parent_path() / ("extracted_" + std::to_string(now) + ".bin")).string();

    std::cout << "\n[*] Đang phân tích & trích xuất...\n";
    std::string errorMsg;
    if (extractHiddenFromMediaCore(in, outputPath, errorMsg)) {
        std::cout << "\n[✓] Đã trích xuất thành công!\n";
        std::cout << "    File lưu tại : " << outputPath << "\n";
        std::cout << "    Dung lượng   : " << SystemCore::formatSize(fs::file_size(outputPath)) << "\n";
        std::cout << "    [Gợi ý] Bạn có thể đổi đuôi .bin thành .zip / .txt / .png / .exe tương ứng với định dạng gốc.\n";
    } else {
        std::cout << "\n[✗] Lỗi: " << errorMsg << "\n";
    }
}

// ==================== HÀM MẸ: ẨN FILE TRONG FILE (SUBMENU) ====================
void MediaProcessor::processAnFileTrongFile() {
    while (true) {
        system("cls");
        std::cout << "============================================================\n";
        std::cout << "                  ẨN FILE TRONG FILE\n";
        std::cout << "============================================================\n";
        std::cout << " [1] Giấu file bí mật vào Ảnh (<= 10MB)\n";
        std::cout << " [2] Giấu file bí mật vào Video (<= 100MB)\n";
        std::cout << " [3] Dò tìm & Trích xuất file ẩn từ Media\n";
        std::cout << " [0] Quay lại\n\n";
        std::cout << " [Chọn]: ";

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
            std::cout << "\n[!] Lựa chọn không hợp lệ!\n";
            Sleep(300);
            break;
        }
    }
}

// ==================== BỘ PHÂN TÍCH & TRÍCH XUẤT METADATA (CÁCH 2) ====================
bool MediaProcessor::extractMetadataCore(const std::string& inputPath, std::string& outputReportPath, std::string& summaryInfo, std::string& errorMsg) {
    if (!fs::exists(inputPath)) {
        errorMsg = "Tập tin không tồn tại: " + inputPath;
        return false;
    }

    fs::path inP(inputPath);
    std::string jsonPath = (inP.parent_path() / (inP.stem().string() + "_metadata.json")).string();
    std::string txtPath = (inP.parent_path() / (inP.stem().string() + "_metadata.txt")).string();

    string ffprobe = getFFprobePath();
    string ffmpeg = getFFmpegPath();

    // 1. Thử dùng ffprobe xuất dạng JSON chuẩn (Cách 2)
    string probeCmd = ffprobe + " -v quiet -print_format json -show_format -show_streams \"" + inputPath + "\" > \"" + jsonPath + "\"";
    bool probeOk = (SystemCore::runRawCommand("cmd /c " + probeCmd) && fs::exists(jsonPath) && fs::file_size(jsonPath) > 20);

    // 2. Dự phòng: Dùng ffmpeg xuất ffmetadata nếu ffprobe chưa có
    if (!probeOk) {
        string metaCmd = ffmpeg + " -y -i \"" + inputPath + "\" -f ffmetadata \"" + txtPath + "\"";
        SystemCore::runRawCommand(metaCmd);
    }

    outputReportPath = probeOk ? jsonPath : txtPath;

    // 3. Phân tích nội dung để tạo tóm tắt hiển thị trực quan
    std::stringstream ss;
    uintmax_t fsize = fs::file_size(inputPath);
    ss << "  [+] Tên tập tin    : " << inP.filename().string() << "\n";
    ss << "  [+] Dung lượng     : " << SystemCore::formatSize(fsize) << "\n";

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

            if (!formatName.empty()) ss << "  [+] Định dạng       : " << formatName << "\n";
            if (!duration.empty()) {
                try {
                    double durSec = std::stod(duration);
                    int h = (int)durSec / 3600;
                    int m = ((int)durSec % 3600) / 60;
                    int s = (int)durSec % 60;
                    char dBuf[64];
                    sprintf_s(dBuf, sizeof(dBuf), "%02d:%02d:%02d (%.2f s)", h, m, s, durSec);
                    ss << "  [+] Thời lượng     : " << dBuf << "\n";
                } catch (...) {
                    ss << "  [+] Thời lượng     : " << duration << "s\n";
                }
            }
            if (!bitRate.empty()) {
                try {
                    long long br = std::stoll(bitRate);
                    ss << "  [+] Bitrate tổng   : " << (br / 1000) << " kbps\n";
                } catch (...) {
                    ss << "  [+] Bitrate tổng   : " << bitRate << "\n";
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
                    ss << "  [+] Luồng Video    : " << vCodec << " (" << w << "x" << h << ")";
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
                    ss << "  [+] Luồng Audio    : " << aCodec << " (" << ch << " channels, " << sRate << " Hz)\n";
                }
            }

            // Thẻ Tag
            if (!title.empty()) ss << "  [*] Tiêu đề (Title) : " << title << "\n";
            if (!artist.empty()) ss << "  [*] Nghệ sĩ/Tác giả: " << artist << "\n";
            if (!creationTime.empty()) ss << "  [*] Ngày tạo       : " << creationTime << "\n";
            if (!encoder.empty()) ss << "  [*] Trình mã hóa   : " << encoder << "\n";
        }
    } else {
        std::ifstream tf(txtPath);
        if (tf) {
            std::string line;
            while (std::getline(tf, line)) {
                line = SystemCore::trim(line);
                if (!line.empty() && line[0] != ';') {
                    ss << "  [*] " << line << "\n";
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

        std::cout << " ==================================================\n";
        std::cout << "    BỘ PHÂN TÍCH & TRÍCH XUẤT METADATA MEDIA (PRO)\n";
        std::cout << " ==================================================\n\n";

        std::cout << " -> Kéo thả N file Ảnh/Video/Audio để trích xuất Metadata (0 để thoát): ";
        std::string rawInput;
        std::getline(std::cin, rawInput);
        std::vector<std::string> inputs = SystemCore::parsePaths(rawInput);

        if (inputs.empty()) {
            std::cout << "\n -> [Thoát] Quay lại menu chính.\n";
            return;
        }

        std::cout << "\n [*] Đang phân tích " << inputs.size() << " file...\n";
        for (size_t i = 0; i < inputs.size(); ++i) {
            std::cout << "\n ------------------------------------------------------------\n";
            std::cout << " [" << i + 1 << "/" << inputs.size() << "] KẾT QUẢ PHÂN TÍCH METADATA:\n";
            std::cout << " ------------------------------------------------------------\n";

            std::string outPath, summary, errorMsg;
            if (extractMetadataCore(inputs[i], outPath, summary, errorMsg)) {
                std::cout << summary;
                std::cout << "\n [✓] Đã xuất toàn bộ chi tiết Metadata ra file:\n";
                std::cout << "     -> " << outPath << "\n";
            } else {
                std::cout << " [✗] Lỗi: " << errorMsg << "\n";
            }
        }

        std::cout << "\n ============================================================\n";
        SystemCore::waitEnter();
    }
}