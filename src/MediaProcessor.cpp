#include "../include/MediaProcessor.h"
#include "../include/SystemCore.h"
#include <iostream>
#include <random>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <mutex>

using namespace std;
namespace fs = std::filesystem;

static string cachedFFmpegPath = "";
static mutex ffmpegMutex;


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


void MediaProcessor::compressImage(const string& inputPath, const string& outputPath, int quality) {
    string ffmpeg = getFFmpegPath();
    // THÊM: -map_metadata 0 để giữ metadata gốc
    // THÊM: -movflags +faststart để tối ưu streaming
    string cmd = ffmpeg + " -y -i \"" + inputPath + "\" -map_metadata 0 -movflags +faststart -q:v " + to_string(quality) + " \"" + outputPath + "\"";
    cout << " \x1b[35m[Media]\x1b[0m Đang tối ưu dung lượng ảnh...";
    if (SystemCore::runRawCommand(cmd)) cout << "\n -> Thành công! Đã xuất file: " << outputPath << "\n";
    else cout << "\n -> [Lỗi] Quá trình xử lý thất bại hoặc sai đường dẫn!\n";
}

void MediaProcessor::extractAudioCore(const std::string& inputPath, const std::string& outputPath) {
    std::string ffmpeg = getFFmpegPath();
    // THÊM: -map_metadata 0 để giữ metadata của audio
    std::string cmd = ffmpeg + " -y -i \"" + inputPath + "\" -map_metadata 0 -vn -q:a 2 \"" + outputPath + "\"";
    SystemCore::runRawCommand(cmd);
}


void MediaProcessor::changeSpeedCore(const std::string& inputPath, const std::string& outputPath, float speedMultiplier) {
    std::string ffmpeg = getFFmpegPath();
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
    
    // THÊM: -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 để giữ metadata cho từng stream
    std::string cmd = ffmpeg + " -y -i \"" + inputPath + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 " + filter + " -c:v libx264 -crf 23 -c:a aac \"" + outputPath + "\"";
    SystemCore::runRawCommand(cmd);
}

void MediaProcessor::processMediaAuto() {
    bool hasPreviousRun = false;
    int totalTotalFiles = 0;
    int totalOptimizedCount = 0;
    int totalSkippedCount = 0;
    long long totalBytesSaved = 0;

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
            cout << "  - Số file tối ưu  : " << totalOptimizedCount << "\n";
            cout << "  - Số file giữ nguyên : " << totalSkippedCount << "\n";
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
                string cmd = ffmpeg + " -y -hide_banner -loglevel error -i \"" + input + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 -c:v libx264 -crf 24 -pix_fmt yuv420p -c:a aac -b:a 128k \"" + tempOutPath.string() + "\"";
                cout << " \x1b[35m[Media]\x1b[0m Đang đóng gói Video mã hóa ẩn";
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

    while (true) {
        std::cout << std::flush;
        system("cls");

        std::cout << " ==================================================\n";
        std::cout << "    BỘ TỰ ĐỘNG PHỤC CHẾ & LÀM NÉT (AI AUTO) \n";
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
        std::cout << "  [1] Tinh chỉnh nhẹ (Nhanh, khử nhiễu cơ bản)\n";
        std::cout << "  [2] Cân bằng hệ thống (Khuyến nghị, nét tự nhiên)\n";
        std::cout << "  [3] Tăng cường tối đa (Chất lượng cao, chậm hơn)\n";
        std::cout << " ==================================================\n";
        int level = SystemCore::readInt(" -> Chọn cấp độ xử lý: ");
        if (level < 1 || level > 3) level = 2;

        std::string imgFilter, vidFilter, vidCodec;
        if (level == 1) {
            imgFilter = "hqdn3d=4:3:4:3,eq=saturation=1.1:contrast=1.05,cas=strength=0.5";
            vidFilter = "hqdn3d=4:3:4:3,eq=saturation=1.1:contrast=1.05,cas=strength=0.5";
            vidCodec  = "-c:v libx264 -crf 18 -preset fast"; 
        } 
        else if (level == 2) {
            imgFilter = "nlmeans=s=1.0:p=7:r=3,eq=saturation=1.1,cas=strength=0.6";
            vidFilter = "hqdn3d=5:4:5:4,eq=saturation=1.1:contrast=1.05,cas=strength=0.6";
            vidCodec  = "-c:v libx264 -crf 20 -preset medium";
        } 
        else {
            imgFilter = "nlmeans=s=1.5:p=7:r=3,eq=saturation=1.15:contrast=1.1,unsharp=5:5:0.8";
            vidFilter = "nlmeans=s=1.2:p=7:r=3,eq=saturation=1.1:contrast=1.05,unsharp=5:5:0.8";
            vidCodec  = "-c:v libx264 -crf 22 -preset slow";
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
                    // === FIX: NẾU KHÔNG TƯƠNG THÍCH, RENDER LẠI ===
                    cout << "    -> [Thông báo] Đang chuyển đổi codec để tương thích...\n";
                    cmd = ffmpeg + " -y -i \"" + input + "\" -map_metadata 0 -map_metadata:s:a 0 -map_metadata:s:v 0 -c:v libx264 -crf 23 -c:a aac \"" + outPath.string() + "\"";
                } else {
                    // === FIX: TƯƠNG THÍCH, DÙNG -c copy ===
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

// ==================== ẨN FILE TRONG ẢNH (GIỚI HẠN 10MB) ====================
void MediaProcessor::hideFileInImage() {
    system("cls");
    std::cout << "============================================================\n";
    std::cout << "   ẨN FILE TRONG ẢNH (Bìa Ảnh <= 10MB)\n";
    std::cout << "============================================================\n";

    std::cout << "-> Nhập đường dẫn Ảnh nền (jpg/png...): ";
    std::string imagePath;
    std::getline(std::cin, imagePath);
    imagePath = SystemCore::trim(imagePath);
    if (imagePath.front() == '"') imagePath = imagePath.substr(1, imagePath.length() - 2);

    std::cout << "-> Nhập đường dẫn File cần ẩn (zip/txt/exe...): ";
    std::string hiddenFilePath;
    std::getline(std::cin, hiddenFilePath);
    hiddenFilePath = SystemCore::trim(hiddenFilePath);
    if (hiddenFilePath.front() == '"') hiddenFilePath = hiddenFilePath.substr(1, hiddenFilePath.length() - 2);

    std::cout << "-> Nhập tên file đầu ra (vd: final.jpg): ";
    std::string outputFileName;
    std::getline(std::cin, outputFileName);
    outputFileName = SystemCore::trim(outputFileName);
    if (outputFileName.front() == '"') outputFileName = outputFileName.substr(1, outputFileName.length() - 2);

    if (!std::filesystem::exists(imagePath) || !std::filesystem::exists(hiddenFilePath)) {
        std::cout << "\n[!] Lỗi: 1 trong 2 file không tồn tại!\n";
        return;
    }

    // Kiểm tra dung lượng file Ảnh
    uintmax_t imgSize = std::filesystem::file_size(imagePath);
    const uintmax_t MAX_IMG_SIZE = 10ULL * 1024 * 1024; // 10MB
    if (imgSize > MAX_IMG_SIZE) {
        std::cout << "\n[!] Lỗi: Ảnh nền quá nặng (" << SystemCore::formatSize(imgSize) 
                  << "). Chỉ chấp nhận ảnh <= 10MB để tránh bị phát hiện!\n";
        return;
    }

    // Tạo lệnh CMD
    std::string cmd = "copy /b \"" + imagePath + "\" + \"" + hiddenFilePath + "\" \"" + outputFileName + "\"";
    
    std::cout << "\n[*] Đang ghép file...\n";
    system(cmd.c_str());

    uintmax_t outputSize = 0;
    if (std::filesystem::exists(outputFileName)) {
        outputSize = std::filesystem::file_size(outputFileName);
        std::cout << "\n[✓] Thành công! File đầu ra: " << outputFileName 
                  << " (" << SystemCore::formatSize(outputSize) << ")\n";
        std::cout << "[✓] Bạn có thể mở file bằng ảnh bình thường, hoặc đổi đuôi .zip/.txt để lấy nội dung ẩn.\n";
    } else {
        std::cout << "\n[✗] Lỗi khi tạo file đầu ra!\n";
    }
}

// ==================== ẨN FILE TRONG VIDEO (GIỚI HẠN 100MB) ====================
void MediaProcessor::hideFileInVideo() {
    system("cls");
    std::cout << "============================================================\n";
    std::cout << "   ẨN FILE TRONG VIDEO (Bìa Video <= 100MB)\n";
    std::cout << "============================================================\n";

    std::cout << "-> Nhập đường dẫn Video nền (mp4/mkv...): ";
    std::string videoPath;
    std::getline(std::cin, videoPath);
    videoPath = SystemCore::trim(videoPath);
    if (videoPath.front() == '"') videoPath = videoPath.substr(1, videoPath.length() - 2);

    std::cout << "-> Nhập đường dẫn File cần ẩn (zip/txt/exe...): ";
    std::string hiddenFilePath;
    std::getline(std::cin, hiddenFilePath);
    hiddenFilePath = SystemCore::trim(hiddenFilePath);
    if (hiddenFilePath.front() == '"') hiddenFilePath = hiddenFilePath.substr(1, hiddenFilePath.length() - 2);

    std::cout << "-> Nhập tên file đầu ra (vd: final.mp4): ";
    std::string outputFileName;
    std::getline(std::cin, outputFileName);
    outputFileName = SystemCore::trim(outputFileName);
    if (outputFileName.front() == '"') outputFileName = outputFileName.substr(1, outputFileName.length() - 2);

    if (!std::filesystem::exists(videoPath) || !std::filesystem::exists(hiddenFilePath)) {
        std::cout << "\n[!] Lỗi: 1 trong 2 file không tồn tại!\n";
        return;
    }

    // Kiểm tra dung lượng video
    uintmax_t videoSize = std::filesystem::file_size(videoPath);
    const uintmax_t MAX_VIDEO_SIZE = 100ULL * 1024 * 1024; // 100MB
    if (videoSize > MAX_VIDEO_SIZE) {
        std::cout << "\n[!] Lỗi: Video nền quá nặng (" << SystemCore::formatSize(videoSize) 
                  << "). Chỉ chấp nhận Video <= 100MB để đỡ tốn thời gian render!\n";
        return;
    }

    // Tạo lệnh CMD
    std::string cmd = "copy /b \"" + videoPath + "\" + \"" + hiddenFilePath + "\" \"" + outputFileName + "\"";
    
    std::cout << "\n[*] Đang ghép file...\n";
    system(cmd.c_str());

    uintmax_t outputSize = 0;
    if (std::filesystem::exists(outputFileName)) {
        outputSize = std::filesystem::file_size(outputFileName);
        std::cout << "\n[✓] Thành công! File đầu ra: " << outputFileName 
                  << " (" << SystemCore::formatSize(outputSize) << ")\n";
        std::cout << "[✓] Bạn có thể xem video bình thường, hoặc đổi đuôi .zip/.txt để lấy nội dung ẩn.\n";
    } else {
        std::cout << "\n[✗] Lỗi khi tạo file đầu ra!\n";
    }
}


// Hàm XOR giải mã ngược (dùng key 0xAA)
static void xorDecrypt(std::vector<uint8_t>& data, uint8_t key = 0xAA) {
    for (auto& byte : data) {
        byte ^= key;
    }
}


// Hàm tách & giải mã file ẩn (Dùng nội bộ)
static bool extractHiddenFile(const std::string& containerPath, const std::string& extractPath, uint8_t key = 0xAA) {
    std::ifstream in(containerPath, std::ios::binary);
    if (!in) return false;

    // Đọc toàn bộ file
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Tìm dấu hiệu kết thúc: 4 byte cuối là magic number "HIDE"
    if (buffer.size() < 4) return false;
    if (buffer[buffer.size() - 4] != 'H' || buffer[buffer.size() - 3] != 'I' || 
        buffer[buffer.size() - 2] != 'D' || buffer[buffer.size() - 1] != 'E') {
        return false;
    }

    // Lấy kích thước file ẩn (4 byte ngay trước magic number)
    uint32_t hiddenSize = 0;
    hiddenSize |= (buffer[buffer.size() - 8] << 24);
    hiddenSize |= (buffer[buffer.size() - 7] << 16);
    hiddenSize |= (buffer[buffer.size() - 6] << 8);
    hiddenSize |= (buffer[buffer.size() - 5]);

    // Trích xuất dữ liệu ẩn
    size_t startPos = buffer.size() - 8 - hiddenSize;
    std::vector<uint8_t> hiddenData(buffer.begin() + startPos, buffer.begin() + startPos + hiddenSize);

    // Giải mã XOR ngược
    xorDecrypt(hiddenData, key);

    // Ghi ra file
    std::ofstream out(extractPath, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(hiddenData.data()), hiddenData.size());
    out.close();
    return true;
}

//TRÍCH XUẤT FILE ẨN (TÊN TỰ ĐỘNG)
void MediaProcessor::extractHiddenFromMedia() {
    system("cls");
    std::cout << "-> File chứa (ảnh/video): ";
    std::string in;
    std::getline(std::cin, in);
    in = SystemCore::trim(in);
    if (in.front() == '"') in = in.substr(1, in.size() - 2);

    if (!std::filesystem::exists(in)) {
        std::cout << "[!] File không tồn tại!\n";
        return;
    }

    // Sinh tên file xuất ngẫu nhiên
    long long now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    std::filesystem::path containerPath(in);
    std::string outputPath = containerPath.parent_path().string() + "\\extracted_" + std::to_string(now) + ".bin";

    if (extractHiddenFile(in, outputPath, 0xAA)) {
        std::cout << "\n[✓] Đã trích xuất thành công!\n";
        std::cout << "    File lưu tại: " << outputPath << "\n";
        std::cout << "    Dung lượng: " << SystemCore::formatSize(std::filesystem::file_size(outputPath)) << "\n";
        std::cout << "    [Tip] Đổi đuôi .bin thành .zip/.txt/.exe nếu cần.\n";
    } else {
        std::cout << "\n[✗] Không tìm thấy file ẩn hoặc file bị hỏng!\n";
    }
}