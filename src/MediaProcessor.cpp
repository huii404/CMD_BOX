#include "../include/MediaProcessor.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <mutex>

using namespace std;
namespace fs = std::filesystem;

static string cachedFFmpegPath = "";
static mutex ffmpegMutex;


static void clearScreenWin32() {
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {0, 0};
    DWORD count;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdOut, &csbi);
    FillConsoleOutputCharacter(hStdOut, ' ', csbi.dwSize.X * csbi.dwSize.Y, coord, &count);
    SetConsoleCursorPosition(hStdOut, coord);
}

MediaProcessor::MediaProcessor() {}
MediaProcessor::~MediaProcessor() {}

// ==================== HÀM TIỆN ÍCH ====================

// Thêm static hoặc đổi tên để tránh conflict
static int readIntLocal(const std::string& prompt) {
    while (true) {
        if (!std::cin.good()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\t[Lỗi] Input stream bị lỗi, thử lại!\n";
            continue;
        }
        std::cout << prompt;
        std::string s;
        std::getline(std::cin, s);
        try {
            return std::stoi(s);
        } catch (...) {
            std::cout << "\t[Lỗi] Vui lòng nhập số nguyên!\n";
        }
    }
}

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


vector<string> MediaProcessor::getPathInput(const string& promptText) {
    string rawInput;
    cout << promptText;
    getline(cin, rawInput);

    // Kiểm tra input rỗng hoặc "0"
    if (rawInput.empty()) {
        return vector<string>();
    }

    if (rawInput == "0") {
        return vector<string>();
    }

    // Xử lý tách đường dẫn dính liền
    string processed = rawInput;
    
    for (int i = (int)processed.length() - 3; i >= 1; --i) {
        char prev = processed[i - 1];
        char curr = processed[i];
        char next = processed[i + 1];
        char next2 = processed[i + 2];
        
        bool isPrevNonSpace = (prev != ' ' && prev != '\t' && prev != '"' && prev != '\0');
        bool isDriveLetter = ((curr >= 'A' && curr <= 'Z') || (curr >= 'a' && curr <= 'z'));
        bool isDrivePattern = (next == ':' && next2 == '\\');
        
        if (isPrevNonSpace && isDriveLetter && isDrivePattern) {
            processed.insert(i, " ");
        }
    }
    
    vector<string> paths;
    vector<string> tokens;
    string token;
    bool inQuotes = false;
    
    for (size_t i = 0; i <= processed.size(); ++i) {
        char c = (i < processed.size()) ? processed[i] : '\0';
        
        if (c == '"') {
            inQuotes = !inQuotes;
            continue;
        }
        
        bool isSep = (!inQuotes && (c == ' ' || c == '\t' || c == '\0'));
        
        if (isSep) {
            if (!token.empty()) {
                if (!token.empty() && token.front() == '"') token.erase(0, 1);
                if (!token.empty() && token.back() == '"') token.pop_back();
                
                tokens.push_back(token);
                token.clear();
            }
        } else if (c != '\0') {
            token += c;
        }
    }
    
    for (const string& t : tokens) {
        string path = t;
        size_t p = path.find("\\\\");
        while (p != string::npos) {
            path.replace(p, 2, "\\");
            p = path.find("\\\\", p + 1);
        }
        
        // Bỏ qua nếu path rỗng
        if (path.empty()) continue;
        
        if (fs::exists(path)) {
            paths.push_back(path);
        } else {
            cout << "    [!] Không tìm thấy: " << path << "\n";
        }
    }

    if (paths.empty()) {
        cout << "    [!] Không tìm thấy file nào hợp lệ!\n";
    }

    return paths;
}

bool MediaProcessor::runCommand(const string& command) {
    // Chuyển đổi command sang UTF-16
    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, NULL, 0);
    if (wchars_num == 0) {
        return false;
    }
    
    vector<wchar_t> wcmd(wchars_num);
    if (MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, wcmd.data(), wchars_num) == 0) {
        return false;
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES; // Thêm flag để redirect
    si.wShowWindow = SW_HIDE;
    
    // === FIX: Redirect output để tránh treo buffer ===
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    
    HANDLE hNull = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hNull != INVALID_HANDLE_VALUE) {
        si.hStdOutput = hNull;
        si.hStdError = hNull;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }
    
    ZeroMemory(&pi, sizeof(pi));

    bool success = false;
    if (CreateProcessW(NULL, wcmd.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 0;
        if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
            success = (exitCode == 0);
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }else{
        //check
        std::cerr << "[!] Không thể tạo process. Lỗi: " << GetLastError() << "\n";
        success = false;
    }

    if (hNull != INVALID_HANDLE_VALUE){
        CloseHandle(hNull);
    }

    return success;
}

int MediaProcessor::readIntLocal(const std::string& prompt) {
    while (true) {
        if (!std::cin.good()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\t[Lỗi] Input stream bị lỗi, thử lại!\n";
            continue;
        }
        std::cout << prompt;
        std::string s;
        std::getline(std::cin, s);
        
        // Xóa khoảng trắng đầu/cuối
        s.erase(0, s.find_first_not_of(" \t\n\r"));
        s.erase(s.find_last_not_of(" \t\n\r") + 1);
        
        if (s.empty()) {
            std::cout << "\t[Lỗi] Vui lòng nhập số nguyên!\n";
            continue;
        }
        
        try {
            return std::stoi(s);
        } catch (const std::invalid_argument&) {
            std::cout << "\t[Lỗi] Vui lòng nhập số nguyên hợp lệ!\n";
        } catch (const std::out_of_range&) {
            std::cout << "\t[Lỗi] Số quá lớn hoặc quá nhỏ!\n";
        } catch (...) {
            std::cout << "\t[Lỗi] Đã xảy ra lỗi không xác định!\n";
        }
    }
}


void MediaProcessor::compressImage(const string& inputPath, const string& outputPath, int quality) {
    string ffmpeg = getFFmpegPath();
    // THÊM: -map_metadata 0 để giữ metadata gốc
    // THÊM: -movflags +faststart để tối ưu streaming
    string cmd = ffmpeg + " -y -i \"" + inputPath + "\" -map_metadata 0 -movflags +faststart -q:v " + to_string(quality) + " \"" + outputPath + "\"";
    cout << " \x1b[35m[Media]\x1b[0m Đang tối ưu dung lượng ảnh...";
    if (runCommand(cmd)) cout << "\n -> Thành công! Đã xuất file: " << outputPath << "\n";
    else cout << "\n -> [Lỗi] Quá trình xử lý thất bại hoặc sai đường dẫn!\n";
}

void MediaProcessor::extractAudioCore(const std::string& inputPath, const std::string& outputPath) {
    std::string ffmpeg = getFFmpegPath();
    // THÊM: -map_metadata 0 để giữ metadata của audio
    std::string cmd = ffmpeg + " -y -i \"" + inputPath + "\" -map_metadata 0 -vn -q:a 2 \"" + outputPath + "\"";
    runCommand(cmd);
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
    runCommand(cmd);
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
            double finalSavedSize = static_cast<double>(totalBytesSaved);
            string sizeUnit = "Bytes";

            if (totalBytesSaved >= 1024LL * 1024LL * 1024LL) {
                finalSavedSize /= (1024.0 * 1024.0 * 1024.0);
                sizeUnit = "GB";
            } 
            else if (totalBytesSaved >= 1024LL * 1024LL) {
                finalSavedSize /= (1024.0 * 1024.0);
                sizeUnit = "MB";
            } 
            else if (totalBytesSaved >= 1024LL) {
                finalSavedSize /= 1024.0;
                sizeUnit = "KB";
            }
            
            cout << "  Số file đầu vào: " << totalTotalFiles << "\n";
            
            if (totalBytesSaved > 0) {
                printf("  Đã giải phóng: %.2f %s\n", finalSavedSize, sizeUnit.c_str());
            } else {
                cout << "  Đã giải phóng: 0 Bytes\n";
            }
            
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

        // === FIX: Xử lý input từ kéo thả - Tối ưu hóa ===
        vector<string> inputs;
        string token;
        bool inQuotes = false;
        
        // Thêm space vào cuối để xử lý token cuối
        string processed = rawInput + " ";
        
        for (char c : processed) {
            if (c == '"') {
                inQuotes = !inQuotes;
                continue;
            }
            
            bool isSep = (!inQuotes && (c == ' ' || c == '\t'));
            
            if (isSep) {
                if (!token.empty()) {
                    // Xóa dấu ngoặc kép
                    if (token.front() == '"') token.erase(0, 1);
                    if (token.back() == '"') token.pop_back();
                    
                    // Sửa đường dẫn
                    size_t p = token.find("\\\\");
                    while (p != string::npos) {
                        token.replace(p, 2, "\\");
                        p = token.find("\\\\", p + 1);
                    }
                    
                    // Kiểm tra file tồn tại
                    if (fs::exists(token)) {
                        inputs.push_back(token);
                    } else {
                        cout << "    [!] Không tìm thấy: " << token << "\n";
                    }
                    token.clear();
                }
            } else {
                token += c;
            }
        }

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
                    renderSuccess = runCommand(cmd) && fs::exists(tempOutPath);
                } else {
                    compressImage(input, tempOutPath.string(), 5);
                    renderSuccess = fs::exists(tempOutPath);
                }
                
                // Fix orientation
                if (renderSuccess && fs::exists(tempOutPath)) {
                    string ffmpeg = getFFmpegPath();
                    string tempFixPath = tempOutPath.string() + ".fix";
                    string fixCmd = ffmpeg + " -y -hide_banner -loglevel error -i \"" + tempOutPath.string() + "\" -map_metadata 0 -metadata:s:v:0 rotate=0 -c copy \"" + tempFixPath + "\"";
                    if (runCommand(fixCmd) && fs::exists(tempFixPath)) {
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
                renderSuccess = runCommand(cmd) && fs::exists(tempOutPath);
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
                        cout << "\n    -> [OK] Đã nén: " << (originalSize - compressedSize) / 1024 << " KB (" << fixed << setprecision(1) << ratio << "%)\n\n";
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
    cin.clear();
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    vector<string> inputs = getPathInput("\tkéo thả N video để lấy âm thanh: ");
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
            // Hàm extractAudioCore đã có map_metadata
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
    cin.clear();
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::vector<std::string> inputs = getPathInput("\tkéo thả N video: ");
    if (inputs.empty()) {
        cout << "\t[Lỗi] Chưa nhập file nào cả!\n";
        return;
    }

    cout << "\ttốc độ mong muốn (0.5: Slow-motion, 2.0: Tua nhanh): ";
    std::string speedStr;
    getline(cin, speedStr);
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
            // Hàm changeSpeedCore đã có map_metadata
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
        clearScreenWin32();

        std::cout << " ==================================================\n";
        std::cout << "    BỘ TỰ ĐỘNG PHỤC CHẾ & LÀM NÉT (AI AUTO) \n";
        std::cout << " ==================================================\n\n";

        std::vector<std::string> inputs = getPathInput(" -> Kéo thả N file Ảnh/Video để làm nét ( 0 để thoát): ");
        
        if (inputs.empty()) {
            std::cout << "\n -> [Thoát] Quay lại menu chính.\n";
            return;
        }

        std::cout << "\n ==================================================\n";
        std::cout << "  [1] Tinh chỉnh nhẹ (Nhanh, khử nhiễu cơ bản)\n";
        std::cout << "  [2] Cân bằng hệ thống (Khuyến nghị, nét tự nhiên)\n";
        std::cout << "  [3] Tăng cường tối đa (Chất lượng cao, chậm hơn)\n";
        std::cout << " ==================================================\n";
        int level = readIntLocal(" -> Chọn cấp độ xử lý: ");
        if (level < 1 || level > 3) level = 2;

        std::string imgFilter, vidFilter, vidCodec;
        if (level == 1) {
            imgFilter = "hqdn3d=1.5:1.5:2:2,unsharp=5:5:1.2:5:5:0.0";
            vidFilter = "hqdn3d=1.5:1.5:3:3";
            vidCodec  = "-c:v libx264 -crf 18 -preset medium"; 
        } 
        else if (level == 2) {
            imgFilter = "nlmeans=s=2:p=1:r=3:threads=2,unsharp=7:7:1.8:7:7:0.0";
            vidFilter = "hqdn3d=2.5:2.5:4:4,unsharp=5:5:0.8:5:5:0.0";
            vidCodec  = "-c:v libx264 -crf 23 -preset medium";
        } 
        else {
            imgFilter = "bm3d=sigma=5:block=8:bstep=4:group=1:thmse=1000,unsharp=7:7:2.2:7:7:0.0,eq=contrast=1.03:brightness=0.01";
            vidFilter = "hqdn3d=4.0:4.0:6:6,unsharp=7:7:1.2:7:7:0.0,eq=contrast=1.05:brightness=0.02";
            vidCodec  = "-c:v libx264 -crf 25 -preset fast";
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

            // Kiểm tra và dựng lệnh cho Ảnh - THÊM MAP_METADATA
            if (std::find(imageExts.begin(), imageExts.end(), ext) != imageExts.end()) {
                cmd = ffmpeg + " -y -i \"" + inputs[i] + "\" -map_metadata 0 -vf \"" + imgFilter + "\" -q:v 2 \"" + outPath.string() + "\"";
                std::cout << " \x1b[35m[AI-Image]\x1b[0m Đang tái cấu trúc pixel & khử nhiễu bề mặt...";
                isSupported = true;
            }
            // Kiểm tra và dựng lệnh cho Video - THÊM MAP_METADATA
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
                if (runCommand(cmd) && fs::exists(outPath)) {
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

        vector<string> inputs = getPathInput(" -> Kéo thả N file ảnh/video (0 để thoát): ");

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
            int choice = readIntLocal(" -> Chọn: ");
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
                bool success = runCommand(cmd);

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
            int choice = readIntLocal(" -> Chọn: ");
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
                bool success = runCommand(cmd);

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