#include "FileSafety.h"
#include <limits>
#include <cmath>
#include "MediaProcessor.h"
#include "SystemCore.h"
#include <iostream>
#include <conio.h>
#include <random>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <mutex>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace std;
namespace fs = std::filesystem;

static string cachedFFmpegPath = "";
static mutex ffmpegMutex;

static GpuCodecInfo cachedGpuInfo;
static bool hasDetectedGpu = false;
static mutex gpuDetectionMutex;

MediaProcessor::MediaProcessor() = default;
MediaProcessor::~MediaProcessor() {}

string MediaProcessor::getFFmpegPath() {
    lock_guard<mutex> lock(ffmpegMutex);
    if (!cachedFFmpegPath.empty()) {
        return cachedFFmpegPath;
    }
    
    wchar_t buffer[MAX_PATH];
    if (GetModuleFileNameW(NULL, buffer, MAX_PATH) > 0) {
        fs::path exePath(buffer);
        fs::path binDir = exePath.parent_path();
        fs::path ffmpegPath = binDir / "ffmpeg.exe";
        
        if (fs::exists(ffmpegPath)) {
            cachedFFmpegPath = "\"" + ffmpegPath.string() + "\"";
            return cachedFFmpegPath;
        }
    }

    // Thử tìm trong PATH
    char* pathEnv = getenv("PATH");
    if (pathEnv) {
        string pathStr(pathEnv);
        stringstream paths(pathStr); string token;
        while (getline(paths, token, ';')) {
            if (token.empty()) continue;
            if (token.size() > 1 && token.front() == '"' && token.back() == '"') token = token.substr(1, token.size() - 2);
            fs::path testPath = fs::u8path(token) / "ffmpeg.exe";
            std::error_code ec;
            if (fs::is_regular_file(testPath, ec) && !ec) {
                cachedFFmpegPath = "\"" + testPath.u8string() + "\""; return cachedFFmpegPath;
            }
        }
    }
    cachedFFmpegPath = "ffmpeg";
    return cachedFFmpegPath;
}

GpuCodecInfo MediaProcessor::getGpuEncoder() {
    lock_guard<mutex> lock(gpuDetectionMutex);
    if (hasDetectedGpu) return cachedGpuInfo;

    string ffmpeg = getFFmpegPath();

    // 1. Kiểm tra NVIDIA NVENC
    string testNvenc = ffmpeg + " -hide_banner -loglevel error -f lavfi -i color=s=64x64:d=0.1 -c:v h264_nvenc -f null -";
    if (SystemCore::runRawCommand(testNvenc)) {
        cachedGpuInfo.encoder = "h264_nvenc";
        cachedGpuInfo.compressParams = "-c:v h264_nvenc -preset p6 -cq 22 -b:v 0 -pix_fmt yuv420p";
        cachedGpuInfo.speedParams = "-c:v h264_nvenc -preset p4 -cq 23 -pix_fmt yuv420p";
        cachedGpuInfo.displayName = "NVIDIA NVENC (GPU Tăng tốc phần cứng)";
        hasDetectedGpu = true;
        return cachedGpuInfo;
    }

    // 2. Kiểm tra Intel QuickSync (QSV)
    string testQsv = ffmpeg + " -hide_banner -loglevel error -f lavfi -i color=s=64x64:d=0.1 -c:v h264_qsv -f null -";
    if (SystemCore::runRawCommand(testQsv)) {
        cachedGpuInfo.encoder = "h264_qsv";
        cachedGpuInfo.compressParams = "-c:v h264_qsv -preset medium -global_quality 22 -pix_fmt yuv420p";
        cachedGpuInfo.speedParams = "-c:v h264_qsv -global_quality 23 -pix_fmt yuv420p";
        cachedGpuInfo.displayName = "Intel QuickSync (GPU Tăng tốc phần cứng)";
        hasDetectedGpu = true;
        return cachedGpuInfo;
    }

    // 3. Kiểm tra AMD AMF
    string testAmf = ffmpeg + " -hide_banner -loglevel error -f lavfi -i color=s=64x64:d=0.1 -c:v h264_amf -f null -";
    if (SystemCore::runRawCommand(testAmf)) {
        cachedGpuInfo.encoder = "h264_amf";
        cachedGpuInfo.compressParams = "-c:v h264_amf -quality quality -rc cqp -qp_p 22 -qp_i 22 -pix_fmt yuv420p";
        cachedGpuInfo.speedParams = "-c:v h264_amf -rc cqp -qp_p 23 -qp_i 23 -pix_fmt yuv420p";
        cachedGpuInfo.displayName = "AMD AMF (GPU Tăng tốc phần cứng)";
        hasDetectedGpu = true;
        return cachedGpuInfo;
    }

    // 4. Fallback CPU
    cachedGpuInfo.encoder = "libx264";
    cachedGpuInfo.compressParams = "-c:v libx264 -crf 21 -preset medium -pix_fmt yuv420p";
    cachedGpuInfo.speedParams = "-c:v libx264 -crf 23 -preset fast -pix_fmt yuv420p";
    cachedGpuInfo.displayName = "CPU (libx264 Software Encoder)";
    hasDetectedGpu = true;
    return cachedGpuInfo;
}

static fs::path makeUniqueOutputPath(const fs::path& requested, const fs::path& source = {}) {
    std::error_code ec;
    if (requested != source && !fs::exists(requested, ec) && !ec) return requested;
    for (int i = 1; i < 10000; ++i) {
        fs::path candidate = requested.parent_path() /
            (requested.stem().string() + "_converted_" + to_string(i) + requested.extension().string());
        ec.clear();
        if (!fs::exists(candidate, ec)) return candidate;
    }
    return requested.parent_path() /
        (requested.stem().string() + "_converted_" + to_string(GetTickCount64()) + requested.extension().string());
}

static fs::path getMediaOutputDirectory(const fs::path& inputPath) {
    fs::path outputDir = inputPath.parent_path() / "CMD_BOX_Output";
    std::error_code ec;
    fs::create_directories(outputDir, ec);
    return ec ? fs::path() : outputDir;
}

static bool commitRenderedFile(const fs::path& tempPath, const fs::path& sourcePath,
                               const fs::path& requestedOutput, fs::path& actualOutput,
                               string& errorMessage, bool& originalRemoved,
                               bool preserveSource = true) {
    (void)preserveSource;
    originalRemoved = false;
    std::error_code ec;
    if (!fs::is_regular_file(tempPath, ec) || ec || fs::file_size(tempPath, ec) == 0 || ec) {
        errorMessage = "File kết quả rỗng hoặc không tồn tại"; return false;
    }
    actualOutput = makeUniqueOutputPath(requestedOutput, sourcePath);
    FileSafety::AncestorLocks parents;
    if (!parents.acquire(actualOutput) || !MoveFileW(tempPath.c_str(), actualOutput.c_str())) {
        errorMessage = "Không thể lưu kết quả; file gốc được giữ nguyên."; return false;
    }
    return true;
}


bool MediaProcessor::extractAudioCore(const std::string& inputPath, const std::string& outputPath) {
    std::string ffmpeg = getFFmpegPath();
    std::string cmd = ffmpeg + " -n -i \"" + inputPath + "\" -map_metadata 0 -vn -q:a 2 \"" + outputPath + "\"";
    return SystemCore::runRawCommand(cmd);
}

bool MediaProcessor::changeSpeedCore(const std::string& inputPath, const std::string& outputPath, float speedMultiplier) {
    if (!std::isfinite(speedMultiplier) || speedMultiplier < 0.5f || speedMultiplier > 2.0f) return false;

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
    
    std::string cmd = ffmpeg + " -n -i \"" + inputPath + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 " + filter + " " + gpu.speedParams + " -c:a aac \"" + outputPath + "\"";
    return SystemCore::runRawCommand(cmd);
}

void MediaProcessor::processMediaAuto() {
    bool hasPreviousRun = false;
    long long totalBytesSaved = 0;

    while (true) {
        std::cin.clear();
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        SystemCore::cls();

        if (hasPreviousRun) {
            cout << "\n\n";
            cout << "  Bản xuất giảm được: " << SystemCore::formatSize(totalBytesSaved) << "\n"
                 << "  File gốc           : Được giữ nguyên\n\n";
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
                 << "    Thử lại sau 2 giây\n";
            Sleep(2000);
            continue;
        }

        cout << "\nĐang phân tích " << inputs.size() << " file...\n";

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
            transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });

            bool isImage = (find(imageExts.begin(), imageExts.end(), ext) != imageExts.end());
            bool isVideo = (find(videoExts.begin(), videoExts.end(), ext) != videoExts.end());

            fs::path outputDir = getMediaOutputDirectory(inPath);
            if (outputDir.empty()) {
                cout << "    Không thể tạo thư mục CMD_BOX_Output!\n\n";
                continue;
            }
            FileSafety::TemporaryDirectory scratch(outputDir);
            if (!scratch) { cout << "Không tạo được thư mục render tạm an toàn.\n"; continue; }
            fs::path tempOutPath;
            fs::path finalOutPath;

            if (isImage) {
                // Tuyệt đối không xuất ra WebP theo yêu cầu; ưu tiên bảo toàn độ nét tối đa
                // Nếu gốc là PNG -> giữ định dạng PNG (nén lossless 100% không mất nét)
                // Các định dạng khác (JPG, HEIC, BMP, TIFF, WebP) -> chuẩn hóa sang JPG chất lượng cao
                if (ext == ".png") {
                    finalOutPath = outputDir / (inPath.stem().string() + "_compressed.png");
                    tempOutPath = scratch.path() / (inPath.stem().string() + "_temp_compressed.png");
                } else {
                    finalOutPath = outputDir / (inPath.stem().string() + "_compressed.jpg");
                    tempOutPath = scratch.path() / (inPath.stem().string() + "_temp_compressed.jpg");
                }
            } else if (isVideo) {
                // Video luôn ưu tiên xuất ra định dạng chuẩn MP4 tương thích cao nhất
                finalOutPath = outputDir / (inPath.stem().string() + "_compressed.mp4");
                tempOutPath = scratch.path() / (inPath.stem().string() + "_temp_compressed.mp4");
            } else {
                cout << "\nBỏ qua: Định dạng " << ext << " không hỗ trợ!\n\n";
                continue;
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

            if (isImage) {
                string ffmpeg = getFFmpegPath();
                string cmd;
                if (finalOutPath.extension() == ".png") {
                    // PNG Lossless compression: giữ nguyên 100% pixel, không mất nét, bảo toàn metadata
                    cmd = ffmpeg + " -n -hide_banner -loglevel error -i \"" + input + "\" -map_metadata 0 -c:v png -compression_level 9 -pred mixed \"" + tempOutPath.string() + "\"";
                    cout << " \x1b[35m[Media]\x1b[0m Đang tối ưu PNG";
                } else {
                    // JPG chất lượng cao (-q:v 2 tương đương 93-95% quality, bảo toàn chi tiết vi mô, giữ trọn vẹn EXIF/Metadata)
                    cmd = ffmpeg + " -n -hide_banner -loglevel error -i \"" + input + "\" -map_metadata 0 -movflags +faststart -q:v 2 \"" + tempOutPath.string() + "\"";
                    cout << " \x1b[35m[Media]\x1b[0m Đang tối ưu JPG";
                }
                renderSuccess = SystemCore::runRawCommand(cmd) && fs::exists(tempOutPath);
                
                // Fix orientation cho file JPG nếu cần
                if (renderSuccess && fs::exists(tempOutPath) && finalOutPath.extension() == ".jpg") {
                    string tempFixPath = (tempOutPath.parent_path() / (tempOutPath.stem().string() + "_fixed.jpg")).string();
                    string fixCmd = ffmpeg + " -n -hide_banner -loglevel error -i \"" + tempOutPath.string() + "\" -map_metadata 0 -metadata:s:v:0 rotate=0 -c copy \"" + tempFixPath + "\"";
                    if (SystemCore::runRawCommand(fixCmd) && fs::exists(tempFixPath)) {
                        fs::remove(tempOutPath);
                        fs::rename(tempFixPath, tempOutPath);
                    } else {
                        if (fs::exists(tempFixPath)) fs::remove(tempFixPath);
                    }
                }
            }
            else if (isVideo) {
                string ffmpeg = getFFmpegPath();
                // Bổ sung bộ lọc làm nét nhẹ luma unsharp (3:3:0.5:3:3:0.0) chống nhòe sau khi lượng tử hóa
                // Bảo toàn toàn bộ metadata gốc (-map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0)
                // Xuất chuẩn MP4
                string cmd = ffmpeg + " -n -hide_banner -loglevel error -i \"" + input + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 -vf \"unsharp=3:3:0.5:3:3:0.0\" " + gpu.compressParams + " -c:a aac -b:a 160k -movflags +faststart \"" + tempOutPath.string() + "\"";
                cout << " \x1b[35m[Media]\x1b[0m Đang tối ưu Video (" << gpu.encoder << ")";
                renderSuccess = SystemCore::runRawCommand(cmd) && fs::exists(tempOutPath);
            }

            // Xử lý kết quả
            if (renderSuccess && fs::exists(tempOutPath)) {
                try {
                    uintmax_t compressedSize = fs::file_size(tempOutPath);

                    if (compressedSize < originalSize) {
                        fs::path actualOutPath;
                        string commitError;
                        bool originalRemoved = false;
                        if (!commitRenderedFile(tempOutPath, inPath, finalOutPath, actualOutPath,
                                                commitError, originalRemoved, true)) {
                            cout << "\nLỗi: " << commitError << "\n\n";
                            continue;
                        }
                        currentBytesSaved += (originalSize - compressedSize);
                        currentOptimizedCount++;
                        
                        float ratio = (1.0f - (float)compressedSize / originalSize) * 100;
                        cout << "\nĐã nén: " << SystemCore::formatSize(originalSize - compressedSize) 
                             << " (" << fixed << setprecision(1) << ratio << "%) -> " 
                             << actualOutPath.filename().string() << "\n\n";
                        if (!commitError.empty()) cout << "Cảnh báo: " << commitError << "\n\n";
                    } 
                    else {
                        fs::remove(tempOutPath);
                        currentSkippedCount++;
                    cout << "\nBỏ qua: File đã tối ưu.\n";
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
        
        totalBytesSaved += currentBytesSaved; 
        hasPreviousRun = true;

        cout << "\n[✓] Đã xử lý xong " << inputs.size() << " file!\n"
             << "Quay lại sau 2 giây\n";
        cout.flush();
        
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        
        for (int i = 2; i > 0; i--) {
            Sleep(1000);
            cout << " " << i << " ";
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

    cout << "\nPhát hiện " << inputs.size() << " file cần trích âm thanh\n";
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
        transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });

        if (find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
            fs::path outputDir = getMediaOutputDirectory(inPath);
            if (outputDir.empty()) { cout << "    [!] Không tạo được CMD_BOX_Output!\n"; continue; }
            fs::path outPath = makeUniqueOutputPath(outputDir / (inPath.stem().string() + ".mp3"));
            bool rendered = extractAudioCore(inputs[i], outPath.string());
            if (rendered && fs::exists(outPath) && fs::file_size(outPath) > 0) {
                cout << "    [✓] " << outPath.filename().string() << "\n";
                successCount++;
            } else {
                cout << "    [!] Thất bại khi trích xuất âm thanh!\n";
            }
        } else {
            cout << "    Bỏ qua: Sai định dạng!\n";
        }
    }
    cout << "\n[✓] Đã trích " << successCount << "/" << inputs.size() << " file âm thanh.\n";
    SystemCore::waitEnter();
}

void MediaProcessor::processChangeSpeedBatch() {
    cout << "\n== ĐỔI TỐC ĐỘ VIDEO ==\n"
         << " Kéo thả video [kèm tốc độ nếu muốn, vd: video.mp4, 1.5]:\n"
         << " [>] ";
    string rawInput;
    getline(cin, rawInput);
    string trimmedInput = SystemCore::trim(rawInput);
    if (trimmedInput.empty() || trimmedInput == "0") return;

    float speed = -1.0f;
    string speedStr = "";

    // Dò tìm tham số tốc độ ở cuối chuỗi (vd: , 1.5 hoặc , 1.5x hoặc 1.5x)
    size_t lastComma = trimmedInput.find_last_of(',');
    if (lastComma != string::npos) {
        string cand = SystemCore::trim(trimmedInput.substr(lastComma + 1));
        if (!cand.empty() && (cand.back() == 'x' || cand.back() == 'X')) cand.pop_back();
        for (char &c : cand) { if (c == ',') c = '.'; }
        try {
            size_t consumed = 0; float val = stof(cand, &consumed);
            if (consumed == cand.size() && std::isfinite(val) && val >= 0.5f && val <= 2.0f) {
                speed = val;
                speedStr = cand;
                rawInput = trimmedInput.substr(0, lastComma);
            }
        } catch (...) {}
    } else {
        size_t lastSpace = trimmedInput.find_last_of(" \t");
        if (lastSpace != string::npos) {
            string cand = SystemCore::trim(trimmedInput.substr(lastSpace + 1));
            if (!cand.empty() && (cand.back() == 'x' || cand.back() == 'X')) cand.pop_back();
            for (char &c : cand) { if (c == ',') c = '.'; }
            try {
                size_t consumed = 0; float val = stof(cand, &consumed);
                if (consumed == cand.size() && std::isfinite(val) && val >= 0.5f && val <= 2.0f) {
                    speed = val;
                    speedStr = cand;
                    rawInput = trimmedInput.substr(0, lastSpace);
                }
            } catch (...) {}
        }
    }

    std::vector<std::string> inputs = SystemCore::parsePaths(rawInput);
    if (inputs.empty()) {
        cout << "Chưa nhập file hợp lệ!\n";
        SystemCore::waitEnter();
        return;
    }

    if (speed <= 0.0f) {
        cout << "Tốc độ mong muốn (0.5: Chậm, 2.0: Nhanh) [Mặc định: 1.5x]: ";
        string promptSpeed;
        getline(cin, promptSpeed);
        promptSpeed = SystemCore::trim(promptSpeed);
        if (promptSpeed.empty()) {
            speed = 1.5f;
            speedStr = "1.5";
        } else {
            if (!promptSpeed.empty() && (promptSpeed.back() == 'x' || promptSpeed.back() == 'X')) promptSpeed.pop_back();
            for (char &c : promptSpeed) { if (c == ',') c = '.'; }
            try { size_t consumed = 0; speed = stof(promptSpeed, &consumed);
                if (consumed != promptSpeed.size() || !std::isfinite(speed) || speed < 0.5f || speed > 2.0f) throw std::invalid_argument("speed");
                speedStr = promptSpeed;
            } catch(...) { cout << "Tốc độ phải là số hữu hạn từ 0.5 đến 2.0.\n"; return; }
        }
    }

    if (speed < 0.5f) speed = 0.5f;
    if (speed > 2.0f) speed = 2.0f;
    if (speedStr.empty()) {
        ostringstream oss;
        oss << fixed << setprecision(1) << speed;
        speedStr = oss.str();
    }

    cout << "\nĐang đổi tốc độ (" << speed << "x) cho " << inputs.size() << " video\n";
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
        transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });

        if (find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
            std::string speedSuffix = "_speed_" + speedStr + "x.mp4";
            fs::path outputDir = getMediaOutputDirectory(inPath);
            if (outputDir.empty()) { cout << "    [!] Không tạo được CMD_BOX_Output!\n"; continue; }
            fs::path outPath = makeUniqueOutputPath(outputDir / (inPath.stem().string() + speedSuffix));
            bool rendered = changeSpeedCore(inputs[i], outPath.string(), speed);
            if (rendered && fs::exists(outPath) && fs::file_size(outPath) > 0) {
                cout << "    [✓] " << outPath.filename().string() << "\n";
                successCount++;
            } else {
                cout << "    [!] Thất bại khi đổi tốc độ!\n";
            }
        } else {
            cout << "Bỏ qua: Sai định dạng!\n";
        }
    }
    cout << "\n[✓] Đã xử lý " << successCount << "/" << inputs.size() << " video.\n";
    SystemCore::waitEnter();
}

void MediaProcessor::processConvertFormatBatch() {
    while (true) {
        SystemCore::cls();

        cout<< "Kéo thả các file ảnh/video (0 để thoát): ";
        string rawInput;
        getline(cin, rawInput);
        vector<string> inputs = SystemCore::parsePaths(rawInput);

        if (inputs.empty()) {
            cout << "\nQuay lại menu chính\n";
            return;
        }

        cout << "\nPhát hiện " << inputs.size() << " file\n\n";

        vector<string> imageExts = { ".jpg", ".jpeg", ".png", ".bmp", ".webp", ".tiff", ".tif", ".heic", ".dng" };
        vector<string> videoExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm" };

        bool hasImage = false;
        bool hasVideo = false;
        for (const string& input : inputs) {
            fs::path p(input);
            string ext = p.extension().string();
            transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });
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

            cout << "\nĐang chuyển đổi ảnh sang " << targetExt << "\n\n";

            for (size_t i = 0; i < inputs.size(); ++i) {
                string input = inputs[i];
                fs::path inPath(input);
                cout << " [" << i + 1 << "/" << inputs.size() << "] " << inPath.filename().string() << "\n";

                if (!fs::exists(inPath)) {
                    cout << "    File không tồn tại!\n";
                    continue;
                }

                string ext = inPath.extension().string();
                transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });

                if (find(imageExts.begin(), imageExts.end(), ext) == imageExts.end()) {
                    cout << "    Bỏ qua: Không phải file ảnh!\n";
                    continue;
                }

                fs::path outputDir = getMediaOutputDirectory(inPath);
                if (outputDir.empty()) { cout << "    Không tạo được CMD_BOX_Output!\n"; continue; }
                fs::path outPath = outputDir / (inPath.stem().string() + targetExt);
                if (inPath == outPath || ext == targetExt) {
                    cout << "    Bỏ qua: File đã ở định dạng " << targetExt << "\n";
                    continue;
                }
                outPath = makeUniqueOutputPath(outPath, inPath);
                string cmd;

                if (ext == ".jpg" || ext == ".jpeg") {
                    if (targetExt == ".png" || targetExt == ".webp") {
                        cout << "Cảnh báo: JPG sang PNG/WEBP có thể mất EXIF metadata\n";
                    }
                }

                if (targetExt == ".jpg" || targetExt == ".jpeg") {
                    cmd = ffmpeg + " -n -i \"" + input + "\" -map_metadata 0 -pix_fmt yuvj420p -q:v 2 \"" + outPath.string() + "\"";
                } else if (targetExt == ".png") {
                    cmd = ffmpeg + " -n -i \"" + input + "\" -map_metadata 0 -pix_fmt rgba \"" + outPath.string() + "\"";
                } else { // .webp
                    cmd = ffmpeg + " -n -i \"" + input + "\" -map_metadata 0 -q:v 90 \"" + outPath.string() + "\"";
                }

                cout << "    Đang chuyển đổi";
                bool success = SystemCore::runRawCommand(cmd);

                std::error_code outputEc;
                bool validOutput = success && fs::exists(outPath, outputEc) &&
                                   fs::file_size(outPath, outputEc) > 0 && !outputEc;
                if (validOutput) {
                                        cout << " OK: " << outPath.filename().string();
                    cout << " (đã giữ file gốc)";
                    cout << "\n";
                } else {
                    // Keep an output we cannot prove this invocation created.
                    cout << "Chuyển đổi thất bại!\n";
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

            cout << "\nĐang chuyển đổi video sang " << targetExt << "\n\n";

            for (size_t i = 0; i < inputs.size(); ++i) {
                string input = inputs[i];
                fs::path inPath(input);
                cout << " [" << i + 1 << "/" << inputs.size() << "] " << inPath.filename().string() << "\n";

                if (!fs::exists(inPath)) {
                    cout << "File không tồn tại!\n";
                    continue;
                }

                string ext = inPath.extension().string();
                transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });

                if (find(videoExts.begin(), videoExts.end(), ext) == videoExts.end()) {
                    cout << "Bỏ qua: Không phải file video!\n";
                    continue;
                }

                fs::path outputDir = getMediaOutputDirectory(inPath);
                if (outputDir.empty()) { cout << "    Không tạo được CMD_BOX_Output!\n"; continue; }
                fs::path outPath = outputDir / (inPath.stem().string() + targetExt);
                if (inPath == outPath || ext == targetExt) {
                    cout << "    Bỏ qua: File đã ở định dạng " << targetExt << "\n";
                    continue;
                }
                outPath = makeUniqueOutputPath(outPath, inPath);
                
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
                    cout << "    Đang chuyển đổi codec (" << gpu.encoder << ")\n";
                    cmd = ffmpeg + " -n -i \"" + input + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 " + gpu.speedParams + " -c:a aac \"" + outPath.string() + "\"";
                } else {
                    cmd = ffmpeg + " -n -i \"" + input + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 -c copy \"" + outPath.string() + "\"";
                }

                cout << "    Đang chuyển đổi";
                bool success = SystemCore::runRawCommand(cmd);

                std::error_code outputEc;
                bool validOutput = success && fs::exists(outPath, outputEc) &&
                                   fs::file_size(outPath, outputEc) > 0 && !outputEc;
                if (validOutput) {
                                        cout << " OK: " << outPath.filename().string();
                    cout << " (đã giữ file gốc)";
                    cout << "\n";
                } else {
                    // Keep an output we cannot prove this invocation created.
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
    SystemCore::cls();
    std::cout << "\n== CHUẨN HÓA TÊN MEDIA ==\n"
              << "Nhập đường dẫn thư mục (0 để quay lại): ";
    std::string dirPath;
    std::getline(std::cin, dirPath);
    dirPath = SystemCore::trim(dirPath);
    if (dirPath == "0" || dirPath.empty()) return;
    
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
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        
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

    std::cout << "\nBắt đầu chuẩn hóa\n";
    int successCount = 0;

    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    std::mt19937_64 rng(timestamp);

    for (const auto& oldPath : filesToRename) {
        std::string ext = oldPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        
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
    errorMsg.clear();
    try {
        const auto coverPath = fs::u8path(containerPath), payloadPath = fs::u8path(hiddenFilePath);
        std::error_code ec;
        if (!fs::is_regular_file(coverPath, ec) || ec || !fs::is_regular_file(payloadPath, ec) || ec) {
            errorMsg = "File đầu vào không tồn tại hoặc không phải file thường."; return false;
        }
        const uintmax_t coverSize = fs::file_size(coverPath, ec);
        if (ec || coverSize > maxContainerSize) { errorMsg = "File nền vượt giới hạn hoặc không đọc được."; return false; }
        const uintmax_t payloadSize = fs::file_size(payloadPath, ec);
        if (ec || payloadSize == 0 || payloadSize > UINT32_MAX) {
            errorMsg = "File ẩn phải có 1..4294967295 bytes."; return false;
        }
        std::ifstream cover(coverPath, std::ios::binary), payload(payloadPath, std::ios::binary);
        if (!cover || !payload) { errorMsg = "Không mở được file đầu vào."; return false; }
        FileSafety::ExclusiveOutput out(fs::u8path(outputPath));
        if (!out) { errorMsg = "Đầu ra đã tồn tại hoặc đường dẫn không an toàn; hãy chọn tên mới."; return false; }
        const uint32_t size = static_cast<uint32_t>(payloadSize);
        const char footer[8] = {char(size >> 24), char(size >> 16), char(size >> 8), char(size), 'H','I','D','E'};
        if (!FileSafety::copyExact(cover, out, coverSize, false) ||
            !FileSafety::copyExact(payload, out, payloadSize, true) ||
            !out.write(footer, sizeof(footer)) || !out.commit()) {
            errorMsg = "Lỗi đọc/ghi file; đầu ra chưa hoàn tất đã được hủy."; return false;
        }
        return true;
    } catch (const std::exception&) { errorMsg = "Không thể ghép file."; return false; }
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
    errorMsg.clear();
    try {
        const auto path = fs::u8path(containerPath);
        std::error_code ec;
        if (!fs::is_regular_file(path, ec) || ec) { errorMsg = "Không đọc được file chứa."; return false; }
        const uintmax_t fileSize = fs::file_size(path, ec);
        if (ec || fileSize < 8 || fileSize > uintmax_t((std::numeric_limits<std::streamoff>::max)())) {
            errorMsg = "Kích thước file không hợp lệ."; return false;
        }
        std::ifstream in(path, std::ios::binary);
        unsigned char footer[8]{};
        in.seekg(static_cast<std::streamoff>(fileSize - 8));
        in.read(reinterpret_cast<char*>(footer), 8);
        if (!in || footer[4] != 'H' || footer[5] != 'I' || footer[6] != 'D' || footer[7] != 'E') {
            errorMsg = "Không tìm thấy file ẩn hợp lệ."; return false;
        }
        const uint32_t size = (uint32_t(footer[0]) << 24) | (uint32_t(footer[1]) << 16) |
                              (uint32_t(footer[2]) << 8) | uint32_t(footer[3]);
        // Subtract in the wide type first: 8 + uint32_t can wrap.
        if (!size || uintmax_t(size) > fileSize - 8) { errorMsg = "Footer bị hỏng."; return false; }
        in.seekg(static_cast<std::streamoff>(fileSize - 8 - size));
        FileSafety::ExclusiveOutput out(fs::u8path(outputPath));
        if (!out) { errorMsg = "Đầu ra đã tồn tại hoặc đường dẫn không an toàn; hãy chọn tên mới."; return false; }
        if (!FileSafety::copyExact(in, out, size, true) || !out.commit()) {
            errorMsg = "Lỗi đọc/ghi file; đầu ra chưa hoàn tất đã được hủy."; return false;
        }
        return true;
    } catch (const std::exception&) { errorMsg = "Không thể trích xuất file."; return false; }
}

// 1. Giấu file bí mật vào Ảnh
void MediaProcessor::hideFileInImage() {
    SystemCore::cls();
    std::cout << "\n== ẨN FILE TRONG ẢNH (≤10 MB) ==\n"
              << "Nhập đường dẫn Ảnh nền (jpg/png): ";
    std::string imagePath;
    std::getline(std::cin, imagePath);
    imagePath = SystemCore::trim(imagePath);
    if (!imagePath.empty() && imagePath.front() == '"' && imagePath.back() == '"') {
        imagePath = imagePath.substr(1, imagePath.length() - 2);
    }

    std::cout << "Nhập đường dẫn File cần ẩn (zip/txt/exe): ";
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

    std::cout << "\nĐang nhúng file\n";
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
    SystemCore::cls();
    std::cout << "\n== ẨN FILE TRONG VIDEO (≤100 MB) ==\n"
              << "Nhập đường dẫn Video nền (mp4/mkv): ";
    std::string videoPath;
    std::getline(std::cin, videoPath);
    videoPath = SystemCore::trim(videoPath);
    if (!videoPath.empty() && videoPath.front() == '"' && videoPath.back() == '"') {
        videoPath = videoPath.substr(1, videoPath.length() - 2);
    }

    std::cout << "Nhập đường dẫn File cần ẩn (zip/txt/exe): ";
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

    std::cout << "\nĐang nhúng file\n";
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
    SystemCore::cls();
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

    std::cout << "\nĐang phân tích & trích xuất\n";
    std::string errorMsg;
    if (extractHiddenFromMediaCore(in, outputPath, errorMsg)) {
        std::cout << "File lưu tại : " << outputPath << "\n";
        std::cout << "Dung lượng   : " << SystemCore::formatSize(fs::file_size(outputPath)) << "\n\n";
    } else {
        std::cout << "\nLỗi: " << errorMsg << "\n";
    }
}

// HÀM MẸ: ẨN FILE TRONG FILE & TRÍCH XUẤT THÔNG MINH
void MediaProcessor::processAnFileTrongFile() {
    SystemCore::cls();
    std::cout << "\n== ẨN & TRÍCH FILE MEDIA ==\n"
              << " * Giấu file  : Kéo thả [File nền], [File cần ẩn] (vd: anh.jpg, data.zip)\n"
              << " * Trích xuất : Kéo thả [File đã giấu] để tự động lấy lại file ẩn\n"
              << " (0 để quay lại)\n\n"
              << " [>] ";

    std::string rawInput;
    if (!std::getline(std::cin, rawInput)) return;
    rawInput = SystemCore::trim(rawInput);
    if (rawInput.empty() || rawInput == "0") return;

    std::vector<std::string> paths = SystemCore::parsePaths(rawInput);
    if (paths.empty()) {
        std::cout << "\nChưa nhập file hợp lệ!\n";
        SystemCore::waitEnter();
        return;
    }

    // Trường hợp 1 file: Kiểm tra có ẩn dữ liệu không để trích xuất, hoặc hỏi file cần ẩn
    if (paths.size() == 1) {
        fs::path p = fs::u8path(paths[0]);
        if (!fs::exists(p)) {
            std::cout << "\nFile không tồn tại!\n";
            SystemCore::waitEnter();
            return;
        }

        // Kiểm tra chữ ký HIDE ở 4 byte cuối
        bool hasHidden = false;
        try {
            uintmax_t sz = fs::file_size(p);
            if (sz >= 8) {
                std::ifstream f(p, std::ios::binary);
                f.seekg(-4, std::ios::end);
                char tag[4] = {0};
                f.read(tag, 4);
                if (tag[0] == 'H' && tag[1] == 'I' && tag[2] == 'D' && tag[3] == 'E') {
                    hasHidden = true;
                }
            }
        } catch (...) {}

        if (hasHidden) {
            std::cout << "\n[*] Phát hiện dữ liệu ẩn trong file: " << p.filename().string() << "\n"
                      << "[*] Đang tiến hành trích xuất...\n";
            long long now = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            std::string outPath = (p.parent_path() / ("extracted_" + std::to_string(now) + ".bin")).string();
            std::string err;
            if (extractHiddenFromMediaCore(paths[0], outPath, err)) {
                std::cout << "\n[✓] Trích xuất thành công!\n"
                          << "    File lưu tại : " << outPath << "\n"
                          << "    Dung lượng   : " << SystemCore::formatSize(fs::file_size(outPath)) << "\n";
            } else {
                std::cout << "\n[!] Thất bại: " << err << "\n";
            }
            SystemCore::waitEnter();
            return;
        }

        // Nếu chưa có dữ liệu ẩn -> Người dùng muốn giấu file vào đây
        std::cout << "\n[*] File nền: " << p.filename().string() << "\n"
                  << "Kéo thả File cần ẩn (zip/txt/exe): ";
        std::string hiddenPath;
        if (!std::getline(std::cin, hiddenPath)) return;
        std::vector<std::string> hiddenPaths = SystemCore::parsePaths(hiddenPath);
        if (hiddenPaths.empty()) {
            std::cout << "\nChưa nhập file cần ẩn!\n";
            SystemCore::waitEnter();
            return;
        }
        paths.push_back(hiddenPaths[0]);
    }

    // Trường hợp giấu file (paths[0] = nền, paths[1] = file cần ẩn)
    if (paths.size() >= 2) {
        fs::path containerPath = fs::u8path(paths[0]);
        fs::path hiddenPath = fs::u8path(paths[1]);

        if (!fs::exists(containerPath)) {
            std::cout << "\nFile nền không tồn tại!\n";
            SystemCore::waitEnter();
            return;
        }
        if (!fs::exists(hiddenPath)) {
            std::cout << "\nFile cần ẩn không tồn tại!\n";
            SystemCore::waitEnter();
            return;
        }

        std::string ext = containerPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });

        std::vector<std::string> imgExts = { ".jpg", ".jpeg", ".png", ".bmp", ".webp" };
        std::vector<std::string> vidExts = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm" };

        bool isImg = (std::find(imgExts.begin(), imgExts.end(), ext) != imgExts.end());
        bool isVid = (std::find(vidExts.begin(), vidExts.end(), ext) != vidExts.end());

        if (!isImg && !isVid) {
            std::cout << "\n[!] Định dạng file nền (" << ext << ") không được hỗ trợ (chỉ nhận Ảnh hoặc Video)!\n";
            SystemCore::waitEnter();
            return;
        }

        // Tự động sinh tên file đầu ra: <tên_gốc>_hidden<ext>
        std::string outPath = (containerPath.parent_path() / (containerPath.stem().string() + "_hidden" + ext)).string();

        std::cout << "\n[*] Đang nhúng '" << hiddenPath.filename().string() 
                  << "' vào '" << containerPath.filename().string() << "'...\n";

        std::string err;
        bool ok = false;
        if (isImg) {
            ok = hideFileInImageCore(containerPath.string(), hiddenPath.string(), outPath, err);
        } else {
            ok = hideFileInVideoCore(containerPath.string(), hiddenPath.string(), outPath, err);
        }

        if (ok && fs::exists(outPath)) {
            std::cout << "\n[✓] Nhúng file thành công!\n"
                      << "    File xuất   : " << outPath << "\n"
                      << "    Dung lượng  : " << SystemCore::formatSize(fs::file_size(outPath)) << "\n"
                      << "    Ghi chú     : File xem bình thường. Khi cần lấy lại, kéo thả file này vào đây để trích xuất!\n";
        } else {
            std::cout << "\n[!] Lỗi: " << err << "\n";
        }
        SystemCore::waitEnter();
    }
}
