#include "FileSafety.h"
#include "MediaMetadata.h"
#include <limits>
#include <cmath>
#include "MediaProcessor.h"
#include "SystemCore.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <mutex>
#include <random>
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
MediaProcessor::~MediaProcessor() = default;

string MediaProcessor::getFFmpegPath() {
    lock_guard<mutex> lock(ffmpegMutex);
    if (!cachedFFmpegPath.empty()) {
        return cachedFFmpegPath;
    }

    auto binary = MediaMetadata::binary(L"ffmpeg.exe");
    cachedFFmpegPath = binary.empty() ? "ffmpeg" : MediaMetadata::quote(binary.u8string());
    return cachedFFmpegPath;
}

GpuCodecInfo MediaProcessor::getGpuEncoder() {
    lock_guard<mutex> lock(gpuDetectionMutex);
    if (hasDetectedGpu) return cachedGpuInfo;

    string ffmpeg = getFFmpegPath();

    const GpuCodecInfo candidates[] = {
        {"h264_nvenc", "-c:v h264_nvenc -preset p6 -cq 22 -b:v 0 -pix_fmt yuv420p",
         "-c:v h264_nvenc -preset p4 -cq 23 -pix_fmt yuv420p", "NVIDIA NVENC (GPU Tăng tốc phần cứng)"},
        {"h264_qsv", "-c:v h264_qsv -preset medium -global_quality 22 -pix_fmt yuv420p",
         "-c:v h264_qsv -global_quality 23 -pix_fmt yuv420p", "Intel QuickSync (GPU Tăng tốc phần cứng)"},
        {"h264_amf", "-c:v h264_amf -quality quality -rc cqp -qp_p 22 -qp_i 22 -pix_fmt yuv420p",
         "-c:v h264_amf -rc cqp -qp_p 23 -qp_i 23 -pix_fmt yuv420p", "AMD AMF (GPU Tăng tốc phần cứng)"}
    };
    cachedGpuInfo = {"libx264", "-c:v libx264 -crf 21 -preset medium -pix_fmt yuv420p",
                     "-c:v libx264 -crf 23 -preset fast -pix_fmt yuv420p", "CPU (libx264 Software Encoder)"};
    for (const auto& candidate : candidates) {
        string command = ffmpeg + " -hide_banner -loglevel error -f lavfi -i color=s=64x64:d=0.1 -c:v " +
                         candidate.encoder + " -f null -";
        if (SystemCore::runRawCommand(command)) {
            cachedGpuInfo = candidate;
            break;
        }
    }
    hasDetectedGpu = true;
    return cachedGpuInfo;
}

static fs::path makeUniqueOutputPath(const fs::path& requested, const fs::path& source = {}) {
    std::error_code ec;
    if (requested != source && !fs::exists(requested, ec) && !ec) return requested;
    for (int i = 1; i < 10000; ++i) {
        fs::path candidate = requested.parent_path() /
            (requested.stem().u8string() + "_converted_" + to_string(i) + requested.extension().u8string());
        ec.clear();
        if (!fs::exists(candidate, ec)) return candidate;
    }
    return requested.parent_path() /
        (requested.stem().u8string() + "_converted_" + to_string(GetTickCount64()) + requested.extension().u8string());
}

static fs::path getMediaOutputDirectory(const fs::path& inputPath) {
    fs::path outputDir = inputPath.parent_path() / "CMD_BOX_Output";
    std::error_code ec;
    fs::create_directories(outputDir, ec);
    return ec ? fs::path() : outputDir;
}

static bool probeMedia(const fs::path& path,MediaMetadata::Json& data,string& error) {
    auto tool=MediaMetadata::binary(L"ffprobe.exe");if(tool.empty()){error="Thiếu ffprobe.exe (đặt cạnh FFmpeg hoặc trong PATH).";return false;}
    string output;if(!MediaMetadata::capture({tool.u8string(),"-v","error","-show_format","-show_streams","-show_chapters","-of","json",fs::absolute(path).u8string()},output)){error="Không đọc được cấu trúc media bằng ffprobe.";return false;}
    try{data=MediaMetadata::Json::parse(output);return data.contains("streams") && data["streams"].is_array();}catch(...){error="ffprobe trả JSON không hợp lệ.";return false;}
}
static string containerMetadataFlags(const fs::path& output) {
    auto ext=output.extension().u8string();return ext==".mp4" || ext==".mov" ? " -movflags +faststart+use_metadata_tags " : " ";
}
static string streamFlags(const fs::path& output) {
    return " -map 0:v:0 -map 0:a? -map 0:s? "+string(output.extension()==L".mkv" ? "-map 0:t? -c:t copy -c:s copy " : "-c:s mov_text ");
}
static bool safeVideoEncoder(const MediaMetadata::Json& info,const string& normal,string& options,string& error) {
    options=normal;
    int videos=0;for(const auto& stream:info["streams"]){auto type=stream.value("codec_type","");videos+=type=="video";if(type=="data" || type=="attachment"){error="Nguồn có telemetry/attachment; chỉ remux để không bỏ stream.";return false;}}
    if(videos>1){error="Nguồn có nhiều video stream; chỉ remux để không bỏ stream.";return false;}
    for(const auto& stream:info["streams"]){if(stream.value("codec_type","")!="video")continue;
        auto transfer=stream.value("color_transfer","");
        if(transfer=="smpte2084" || transfer=="arib-std-b67"){error="Nguồn HDR: giữ file gốc / chuyển container bằng stream copy. Chưa render HDR để tránh mất dải màu.";return false;}
        auto format=stream.value("pix_fmt","");
        if(format.find("10")!=string::npos || format.find("12")!=string::npos || format.find("16")!=string::npos){
            error="Nguồn video trên 8-bit: chỉ chuyển container bằng stream copy; chưa render giảm bit-depth.";return false;
        }break;
    }return true;
}
static bool finalizeMedia(const fs::path& temporary,const MediaMetadata::Snapshot& source,const fs::path& requested,fs::path& actual,string& error) {
    error.clear();error_code ec;
    if(!source.valid || !fs::is_regular_file(temporary,ec) || ec || fs::file_size(temporary,ec)==0 || ec){error="File render rỗng hoặc snapshot metadata không hợp lệ.";return false;}
    vector<string> missing;
    if(!MediaMetadata::preserve(source,temporary,missing)){
        error="Không xuất: định dạng đích chưa giữ được metadata nhúng.";
        for(const auto& tag:missing)error+="\n  "+tag;
        return false;
    }
    actual=makeUniqueOutputPath(requested,fs::u8path(source.source));
    FileSafety::AncestorLocks parents;if(!parents.acquire(actual)){error="Thư mục xuất không an toàn.";return false;}
    struct Publication {
        HANDLE media=INVALID_HANDLE_VALUE;bool committed=false;
        ~Publication(){
            if(media==INVALID_HANDLE_VALUE)return;
            if(!committed){FILE_DISPOSITION_INFO remove{TRUE};SetFileInformationByHandle(media,FileDispositionInfo,&remove,sizeof(remove));}
            CloseHandle(media);
        }
    } file;
    file.media=CreateFileW(temporary.c_str(),FILE_WRITE_ATTRIBUTES|DELETE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(file.media==INVALID_HANDLE_VALUE || !MediaMetadata::publishHandle(file.media,actual)){
        error="Không lưu được file; nguồn giữ nguyên.";return false;
    }
    if(!SetFileTime(file.media,&source.times.created,&source.times.accessed,&source.times.written)){error="Không đồng bộ được timestamp Windows; đã hủy bản xuất.";return false;}
    file.committed=true;return true;
}

static string escapeChapterText(const string& value) {
    string result;for(char c:value){if(c=='\r' || c=='\n'){result+=' ';continue;}if(c=='\\' || c=='=' || c==';' || c=='#')result+='\\';result+=c;}return result;
}
static bool writeScaledChapters(const MediaMetadata::Json& info,double speed,const fs::path& output) {
    string text=";FFMETADATA1\n";
    if(info.contains("chapters"))for(const auto& chapter:info["chapters"]){
        double start=stod(chapter.value("start_time","0")),end=stod(chapter.value("end_time","0"));
        text+="[CHAPTER]\nTIMEBASE=1/1000000\nSTART="+to_string(llround(start*1000000/speed))+"\nEND="+to_string(llround(end*1000000/speed))+"\n";
        if(chapter.contains("tags"))for(auto it=chapter["tags"].begin();it!=chapter["tags"].end();++it)if(it.value().is_string())text+=escapeChapterText(it.key())+"="+escapeChapterText(it.value().get<string>())+"\n";
    }
    FileSafety::ExclusiveOutput file(output);return file && file.write(text.data(),text.size()) && file.commit();
}

static string audioMemoryFlags(const fs::path& source) {
    MediaMetadata::Json tags;string error;
    if(!MediaMetadata::tags(source,tags,error))return {};
    string flags;
    for(const auto& key:{"DateTimeOriginal","CreateDate","CreationDate","CreationTime","TrackCreateDate","MediaCreateDate",
                        "Make","Model","LensModel","LensMake","SerialNumber","LensSerialNumber","GPSCoordinates",
                        "GPSLatitude","GPSLatitudeRef","GPSLongitude","GPSLongitudeRef","GPSAltitude","GPSAltitudeRef",
                        "GPSDateStamp","GPSTimeStamp","OffsetTimeOriginal","OffsetTimeDigitized","SubSecTimeOriginal","SubSecTimeDigitized",
                        "Artist","Author","Copyright","Description","Title","ImageDescription","UserComment","Keywords","Subject","Rating"}){
        auto value=MediaMetadata::value(tags,key);if(value.is_null() || (!value.is_string() && !value.is_number()))continue;
        if(MediaMetadata::emptyDate(key,value))continue;
        flags+=" -metadata "+MediaMetadata::quote(string(key)+"="+(value.is_string()?value.get<string>():value.dump()));
    }
    auto created=MediaMetadata::value(tags,"CreationTime");
    if(created.is_string())flags+=" -metadata "+MediaMetadata::quote("creation_time="+created.get<string>());
    MediaMetadata::Json probe;
    if(MediaMetadata::probeFile(source,probe))for(const auto& stream:probe.value("streams",MediaMetadata::Json::array())){
        if(stream.value("codec_type","")!="audio")continue;
        auto metadata=stream.value("tags",MediaMetadata::Json::object());
        for(auto it=metadata.begin();it!=metadata.end();++it){
            auto key=MediaMetadata::lower(it.key());
            if(key=="encoder" || key=="handler_name" || key=="vendor_id" || key=="duration" || key=="bps" ||
               key=="number_of_frames" || key=="number_of_bytes" || key.rfind("_statistics_",0)==0 || !it.value().is_string())continue;
            flags+=" -metadata "+MediaMetadata::quote("audio_track_"+key+"="+it.value().get<string>());
        }
        break;
    }
    return flags;
}

bool MediaProcessor::extractAudioCore(const string& inputPath,const string& outputPath) {
    auto cmd=getFFmpegPath()+" -n -hide_banner -loglevel error -i "+MediaMetadata::quote(inputPath)+" -map 0:a:0 -map_metadata 0 -vn -id3v2_version 4 -q:a 2 "+audioMemoryFlags(fs::u8path(inputPath))+" "+MediaMetadata::quote(outputPath);
    return SystemCore::runRawCommand(cmd);
}
bool MediaProcessor::changeSpeedCore(const string& inputPath,const string& outputPath,float speed) {
    if(!std::isfinite(speed) || speed<0.5f || speed>2)return false;
    MediaMetadata::Json info;string error;if(!probeMedia(fs::u8path(inputPath),info,error)){cout<<error<<'\n';return false;}
    string encoder;if(!safeVideoEncoder(info,getGpuEncoder().speedParams,encoder,error)){cout<<error<<'\n';return false;}
    auto chapters=fs::u8path(outputPath).parent_path()/L"scaled-chapters.ffmetadata";
    if(!writeScaledChapters(info,speed,chapters))return false;
    ostringstream s,pts;s<<setprecision(9)<<speed;pts<<setprecision(12)<<(1.0/double(speed));
    string command=getFFmpegPath()+" -n -hide_banner -loglevel error -i "+MediaMetadata::quote(inputPath)+" -f ffmetadata -i "+MediaMetadata::quote(chapters.u8string())+
        streamFlags(fs::u8path(outputPath))+" -map_metadata 0 -map_chapters 1 -vf "+MediaMetadata::quote("setpts="+pts.str()+"*PTS")+" -af "+MediaMetadata::quote("atempo="+s.str())+
        " "+encoder+" -c:a aac "+containerMetadataFlags(fs::u8path(outputPath))+MediaMetadata::quote(outputPath);
    return SystemCore::runRawCommand(command);
}

void MediaProcessor::processMediaAuto() {
    if (!SystemCore::requireFeature(Feature::Compress)) { SystemCore::waitEnter(); return; }
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
            fs::path inPath=fs::u8path(input);
            cout << " [" << i + 1 << "/" << inputs.size() << "] Xử lý: " << inPath.filename().u8string() << "\n";

            if (!fs::exists(inPath)) {
                cout << "    File không tồn tại!\n\n";
                continue;
            }

            string ext = inPath.extension().u8string();
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
                if (ext == ".png") {
                    finalOutPath = outputDir / (inPath.stem().u8string() + "_compressed.png");
                    tempOutPath = scratch.path() / (inPath.stem().u8string() + "_temp_compressed.png");
                } else {
                    finalOutPath = outputDir / (inPath.stem().u8string() + "_compressed.jpg");
                    tempOutPath = scratch.path() / (inPath.stem().u8string() + "_temp_compressed.jpg");
                }
            } else if (isVideo) {
                finalOutPath = outputDir / (inPath.stem().u8string() + "_compressed.mp4");
                tempOutPath = scratch.path() / (inPath.stem().u8string() + "_temp_compressed.mp4");
            } else {
                cout << "\nBỏ qua: Định dạng " << ext << " không hỗ trợ!\n\n";
                continue;
            }

            string metadataError;
            auto originalMetadata=MediaMetadata::snapshot(inPath,metadataError);
            MediaMetadata::Json mediaInfo;
            if(!originalMetadata.valid || !probeMedia(inPath,mediaInfo,metadataError)){cout << "Metadata: " << metadataError << "\n";continue;}
            if(isImage){
                bool highDepth=false,floating=false;
                for(const auto& stream:mediaInfo["streams"]){auto fmt=stream.value("pix_fmt","");highDepth|=fmt.find("16")!=string::npos || fmt.find("48")!=string::npos || fmt.find("64")!=string::npos || fmt.find("10")!=string::npos || fmt.find("12")!=string::npos;floating|=fmt.find("f32")!=string::npos || fmt.find("f64")!=string::npos;}
                if(floating){cout<<"Ảnh float: giữ nguồn, chưa tự giảm độ chính xác khi nén.\n";continue;}
                if(highDepth && finalOutPath.extension()!=L".png"){
                    finalOutPath=outputDir/fs::u8path(inPath.stem().u8string()+"_compressed.png");
                    tempOutPath=scratch.path()/L"high-depth.png";
                    cout<<"Nguồn trên 8-bit: dùng PNG thay cho JPEG để giữ bit-depth.\n";
                }
            }
            bool renderSuccess = false;

            uintmax_t originalSize = 0;
            try {
                originalSize = fs::file_size(inPath);
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
                    cmd = ffmpeg + " -n -hide_banner -loglevel error -noautorotate -i \"" + input + "\" -map_metadata 0 -c:v png -compression_level 9 -pred mixed \"" + tempOutPath.u8string() + "\"";
                    cout << " \x1b[35m[Media]\x1b[0m Đang tối ưu PNG";
                } else {
                    cmd = ffmpeg + " -n -hide_banner -loglevel error -noautorotate -i \"" + input + "\" -map_metadata 0 -q:v 2 \"" + tempOutPath.u8string() + "\"";
                    cout << " \x1b[35m[Media]\x1b[0m Đang tối ưu JPG";
                }
                renderSuccess = SystemCore::runRawCommand(cmd) && fs::exists(tempOutPath);
            }
            else if (isVideo) {
                string ffmpeg = getFFmpegPath();
                string encoder;
                if(!safeVideoEncoder(mediaInfo,gpu.compressParams,encoder,metadataError)){cout<<metadataError<<"\n";continue;}
                string cmd=ffmpeg+" -n -hide_banner -loglevel error -i "+MediaMetadata::quote(input)+streamFlags(tempOutPath)+" -map_metadata 0 -map_chapters 0 "+encoder+" -c:a aac -b:a 160k "+containerMetadataFlags(tempOutPath)+MediaMetadata::quote(tempOutPath.u8string());
                cout << " \x1b[35m[Media]\x1b[0m Đang tối ưu Video (" << gpu.encoder << ")";
                renderSuccess = SystemCore::runRawCommand(cmd) && fs::exists(tempOutPath);
            }

            if (renderSuccess && fs::exists(tempOutPath)) {
                try {
                    uintmax_t compressedSize = fs::file_size(tempOutPath);

                    if (compressedSize < originalSize) {
                        fs::path actualOutPath;
                        string commitError;

                        if (!finalizeMedia(tempOutPath,originalMetadata,finalOutPath,actualOutPath,commitError)) {
                            cout << "\nLỗi: " << commitError << "\n\n";
                            continue;
                        }
                        compressedSize=fs::file_size(actualOutPath);
                        auto saved=originalSize>compressedSize ? originalSize-compressedSize : uintmax_t(0);
                        currentBytesSaved += saved;
                        currentOptimizedCount++;

                        float ratio = (1.0f - (float)(originalSize-saved) / originalSize) * 100;
                        cout << "\nĐã nén: " << SystemCore::formatSize(saved)
                             << " (" << fixed << setprecision(1) << ratio << "%) -> "
                             << actualOutPath.filename().u8string() << "\n\n";
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
    if (!SystemCore::requireFeature(Feature::ExtractAudio)) { SystemCore::waitEnter(); return; }
    cout << "Trích track audio đầu tiên. Kéo thả các video: ";
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
        fs::path inPath=fs::u8path(inputs[i]);
        cout << " [" << i + 1 << "/" << inputs.size() << "] Đang trích: " << inPath.filename().u8string() << "\n";

        if (!fs::exists(inPath)) {
            cout << "    File không tồn tại!\n";
            continue;
        }

        string ext = inPath.extension().u8string();
        transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });

        if (find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
            fs::path outputDir = getMediaOutputDirectory(inPath);
            if (outputDir.empty()) { cout << "    [!] Không tạo được CMD_BOX_Output!\n"; continue; }
            fs::path outPath = makeUniqueOutputPath(outputDir / (inPath.stem().u8string() + ".mp3"));
            string metadataError;auto originalMetadata=MediaMetadata::snapshot(inPath,metadataError);
            FileSafety::TemporaryDirectory scratch(outputDir);if(!originalMetadata.valid || !scratch){cout<<metadataError<<"\n";continue;}
            auto temporary=scratch.path()/L"audio.mp3";
            bool rendered=extractAudioCore(inputs[i],temporary.u8string());
            fs::path actual;
            rendered=rendered && finalizeMedia(temporary,originalMetadata,outPath,actual,metadataError);
            if(rendered){outPath=actual;if(!metadataError.empty())cout<<"Metadata: "<<metadataError<<"\n";}
            if (rendered && fs::exists(outPath) && fs::file_size(outPath) > 0) {

                cout << "    [✓] " << outPath.filename().u8string() << "\n";
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
    if (!SystemCore::requireFeature(Feature::Speed)) { SystemCore::waitEnter(); return; }
    cout << "\n \x1b[38;2;195;165;255m╭── ĐỔI TỐC ĐỘ VIDEO ──────────────────────────────╮\x1b[0m\n"
         << "   Kéo thả video [kèm tốc độ nếu muốn, vd: video.mp4, 1.5]\n"
         << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n"
         << " [>] ";
    string rawInput;
    getline(cin, rawInput);
    string trimmedInput = SystemCore::trim(rawInput);
    if (trimmedInput.empty() || trimmedInput == "0") return;

    float speed = -1.0f;
    string speedStr = "";

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
        fs::path inPath=fs::u8path(inputs[i]);
        cout << " [" << i + 1 << "/" << inputs.size() << "] Đang render: " << inPath.filename().u8string() << "\n";

        if (!fs::exists(inPath)) {
            cout << "File không tồn tại!\n";
            continue;
        }

        std::string ext = inPath.extension().u8string();
        transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });

        if (find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
            std::string speedSuffix = "_speed_" + speedStr + "x.mp4";
            fs::path outputDir = getMediaOutputDirectory(inPath);
            if (outputDir.empty()) { cout << "    [!] Không tạo được CMD_BOX_Output!\n"; continue; }
            fs::path outPath = makeUniqueOutputPath(outputDir / (inPath.stem().u8string() + speedSuffix));
            string metadataError;auto originalMetadata=MediaMetadata::snapshot(inPath,metadataError);
            FileSafety::TemporaryDirectory scratch(outputDir);if(!originalMetadata.valid || !scratch){cout<<metadataError<<"\n";continue;}
            auto temporary=scratch.path()/L"speed.mp4";
            bool rendered=changeSpeedCore(inputs[i],temporary.u8string(),speed);
            fs::path actual;
            rendered=rendered && finalizeMedia(temporary,originalMetadata,outPath,actual,metadataError);
            if(rendered){outPath=actual;if(!metadataError.empty())cout<<"Metadata: "<<metadataError<<"\n";}
            if (rendered && fs::exists(outPath) && fs::file_size(outPath) > 0) {

                cout << "    [✓] " << outPath.filename().u8string() << "\n";
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
    if (!SystemCore::requireFeature(Feature::Convert)) { SystemCore::waitEnter(); return; }
    while(true){
        SystemCore::cls();cout<<"Kéo thả ảnh/video (0: Hủy): ";string raw;getline(cin,raw);if(raw.empty() || raw=="0")return;
        auto inputs=SystemCore::parsePaths(raw);if(inputs.empty())continue;
        bool photos=MediaMetadata::image(fs::u8path(inputs[0])),mixed=false;
        for(const auto& path:inputs)if(MediaMetadata::image(fs::u8path(path))!=photos)mixed=true;
        if(mixed){cout<<"Chọn riêng ảnh hoặc video.\n";continue;}
        cout<<(photos ? "[1] JPG  [2] PNG  [3] WebP  [0] Hủy\n" : "[1] MP4  [2] MKV  [3] MOV  [0] Hủy\n");int choice=SystemCore::readInt("Định dạng đích: ");if(!choice)continue;if(choice<1 || choice>3)continue;
        string extension=photos ? vector<string>{".jpg",".png",".webp"}[choice-1] : vector<string>{".mp4",".mkv",".mov"}[choice-1];
        for(const auto& input:inputs){
            auto source=fs::u8path(input);if(!fs::is_regular_file(source)){cout<<"Không phải file thường.\n";continue;}
            auto originalExt=source.extension().u8string();transform(originalExt.begin(),originalExt.end(),originalExt.begin(),[](unsigned char c){return char(tolower(c));});
            if(originalExt==extension){cout<<"Bỏ qua: đã đúng định dạng.\n";continue;}
            string error;auto snapshot=MediaMetadata::snapshot(source,error);MediaMetadata::Json info;
            if(!snapshot.valid || !probeMedia(source,info,error)){cout<<error<<'\n';continue;}
            auto dir=getMediaOutputDirectory(source);FileSafety::TemporaryDirectory scratch(dir);if(dir.empty() || !scratch){cout<<"Không tạo được thư mục xuất.\n";continue;}
            auto temporary=scratch.path()/fs::u8path("render"+extension);auto requested=dir/fs::u8path(source.stem().u8string()+extension);
            string prefix=getFFmpegPath()+" -n -hide_banner -loglevel error ";bool rendered=false;
            if(photos){
                string pixel;
                // PNG preserves sample depth; JPG/WebP may reduce it.
                bool high=false;for(const auto& stream:info["streams"]){auto fmt=stream.value("pix_fmt","");high|=fmt.find("16")!=string::npos || fmt.find("48")!=string::npos || fmt.find("64")!=string::npos || fmt.find("10")!=string::npos || fmt.find("12")!=string::npos;}
                if(extension==".png")pixel=high ? " -pix_fmt rgba64be " : " -pix_fmt rgba ";
                else if(extension==".jpg")pixel=" -pix_fmt yuvj444p -q:v 2 ";else pixel=" -quality 90 ";
                if(high && extension!=".png")cout<<"Chú ý: định dạng đích là bản 8-bit; giữ file nguồn.\n";
                string command=prefix+"-noautorotate -i "+MediaMetadata::quote(input)+" -map 0:v:0 -frames:v 1 -map_metadata 0 "+pixel+MediaMetadata::quote(temporary.u8string());
                rendered=SystemCore::runRawCommand(command);
            }else{
                // Remux all streams first; render only if the container rejects them.
                string command=prefix+"-i "+MediaMetadata::quote(input)+" -map 0 -map_metadata 0 -map_chapters 0 -c copy "+containerMetadataFlags(temporary)+MediaMetadata::quote(temporary.u8string());
                rendered=SystemCore::runRawCommand(command);
                if(!rendered){
                    DeleteFileW(temporary.c_str());string encoder;
                    if(!safeVideoEncoder(info,getGpuEncoder().speedParams,encoder,error)){cout<<error<<'\n';continue;}
                    bool extra=false;int videos=0;for(const auto& stream:info["streams"]){auto type=stream.value("codec_type","");extra|=type=="data" || (type=="attachment" && extension!=".mkv");videos+=type=="video";}
                    if(extra || videos>1){cout<<"Container không nhận đủ stream nguồn; giữ nguyên file gốc, không tự bỏ stream.\n";continue;}
                    command=prefix+"-i "+MediaMetadata::quote(input)+streamFlags(temporary)+" -map_metadata 0 -map_chapters 0 "+encoder+" -c:a aac "+containerMetadataFlags(temporary)+MediaMetadata::quote(temporary.u8string());
                    rendered=SystemCore::runRawCommand(command);
                }else cout<<"Đổi container bằng stream copy, không render lại.\n";
            }
            fs::path actual;
            if(rendered && finalizeMedia(temporary,snapshot,requested,actual,error)){cout<<"OK: "<<actual.u8string()<<"\n";if(!error.empty())cout<<"Metadata: "<<error<<'\n';}
            else cout<<"Thất bại: "<<error<<"; nguồn giữ nguyên.\n";
        }
    }
}

void MediaProcessor::normalizeMediaFilenames() {
    if (!SystemCore::requireFeature(Feature::Rename)) { SystemCore::waitEnter(); return; }
    SystemCore::cls();
    std::cout << "\n \x1b[38;2;195;165;255m╭── CHUẨN HÓA TÊN MEDIA THEO NGÀY ─────────────────╮\x1b[0m\n"
              << "   Nhập đường dẫn thư mục (0 để quay lại)\n"
              << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n"
              << " [>] ";
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

        std::string ext = entry.path().extension().u8string();
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
        std::string ext = oldPath.extension().u8string();
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
            std::cout << "  " << oldPath.filename().u8string() << " -> " << newName << "\n";
            successCount++;
        } catch (const std::exception& e) {
            std::cout << "  Lỗi đổi tên: " << oldPath.filename().u8string() << " (" << e.what() << ")\n";
        }
    }

    std::cout << "\nHoàn thành! Đã chuẩn hóa " << successCount << "/" << filesToRename.size() << " file.\n\n";
    SystemCore::waitEnter();
}

static MediaMetadata::Json hiddenFileInfo(const fs::path& path) {
    auto times=MediaMetadata::readTimes(path);auto ticks=[](FILETIME t){return (uint64_t(t.dwHighDateTime)<<32)|t.dwLowDateTime;};
    return {{"schema","cmd-box-hidden/2"},{"originalName",path.filename().u8string()},{"created",ticks(times.created)},{"accessed",ticks(times.accessed)},{"written",ticks(times.written)},{"timesValid",times.valid}};
}
static void putWideSize(char* output,uint64_t value){for(int i=7;i>=0;--i){output[i]=char(value);value>>=8;}}
static uint64_t wideSize(const unsigned char* input){uint64_t value=0;for(int i=0;i<8;++i)value=(value<<8)|input[i];return value;}
static bool restoreHiddenTimes(const MediaMetadata::Json& info,const fs::path& path) {
    if(!info.value("timesValid",false))return false;
    MediaMetadata::Times times;times.valid=true;
    auto time=[](uint64_t v){return FILETIME{DWORD(v),DWORD(v>>32)};};times.created=time(info.at("created").get<uint64_t>());times.accessed=time(info.at("accessed").get<uint64_t>());times.written=time(info.at("written").get<uint64_t>());
    return MediaMetadata::setTimes(times,path);
}
bool MediaProcessor::embedFileIntoContainerCore(const string& containerPath,const string& hiddenFilePath,const string& outputPath,uintmax_t maxContainerSize,string& errorMsg) {
    errorMsg.clear();
    try{
        auto coverPath=fs::u8path(containerPath),payloadPath=fs::u8path(hiddenFilePath),output=fs::u8path(outputPath);error_code ec;
        if(!fs::is_regular_file(coverPath,ec) || ec || !fs::is_regular_file(payloadPath,ec) || ec){errorMsg="File đầu vào không hợp lệ.";return false;}
        auto coverTimes=MediaMetadata::readTimes(coverPath);auto info=hiddenFileInfo(payloadPath);if(!coverTimes.valid || !info.value("timesValid",false)){errorMsg="Không đọc được timestamp nguồn.";return false;}
        auto coverSize=fs::file_size(coverPath),payloadSize=fs::file_size(payloadPath);if(coverSize>maxContainerSize || !payloadSize || payloadSize>UINT32_MAX){errorMsg="Kích thước file vượt giới hạn.";return false;}
        auto manifest=info.dump();char footer[32]{};putWideSize(footer,coverSize);putWideSize(footer+8,payloadSize);putWideSize(footer+16,manifest.size());memcpy(footer+24,"CBOXHID2",8);
        std::ifstream cover(coverPath,std::ios::binary),payload(payloadPath,std::ios::binary);FileSafety::ExclusiveOutput out(output);
        if(!cover || !payload || !out || !FileSafety::copyExact(cover,out,coverSize,false) || !FileSafety::copyExact(payload,out,payloadSize,true) || !out.write(manifest.data(),manifest.size()) || !out.write(footer,sizeof(footer)) || !out.commit()){errorMsg="Không ghép được file; không ghi đè nguồn.";return false;}
        if(!MediaMetadata::setTimes(coverTimes,output)){errorMsg="Đã ghép nhưng không đồng bộ được timestamp file nền.";return false;}return true;
    }catch(...){errorMsg="Không thể ghép file.";return false;}
}

bool MediaProcessor::hideFileInImageCore(const std::string& imagePath, const std::string& hiddenFilePath,
                                        const std::string& outputPath, std::string& errorMsg) {
    const uintmax_t MAX_IMG_SIZE = 10ULL * 1024 * 1024; // 10MB
    return embedFileIntoContainerCore(imagePath, hiddenFilePath, outputPath, MAX_IMG_SIZE, errorMsg);
}

bool MediaProcessor::hideFileInVideoCore(const std::string& videoPath, const std::string& hiddenFilePath,
                                        const std::string& outputPath, std::string& errorMsg) {
    const uintmax_t MAX_VIDEO_SIZE = 100ULL * 1024 * 1024; // 100MB
    return embedFileIntoContainerCore(videoPath, hiddenFilePath, outputPath, MAX_VIDEO_SIZE, errorMsg);
}

bool MediaProcessor::extractHiddenFromMediaCore(const string& containerPath,const string& outputPath,string& errorMsg) {
    errorMsg.clear();
    try{
        auto path=fs::u8path(containerPath),output=fs::u8path(outputPath);error_code ec;auto size=fs::file_size(path,ec);
        if(ec || size<8 || size>uintmax_t((numeric_limits<streamoff>::max)())){errorMsg="File chứa không hợp lệ.";return false;}
        std::ifstream in(path,std::ios::binary);unsigned char end[32]{};in.seekg(streamoff(size-8));in.read(reinterpret_cast<char*>(end+24),8);if(!in){errorMsg="Không đọc được footer.";return false;}
        uint64_t coverSize=0,payloadSize=0;MediaMetadata::Json info;bool modern=memcmp(end+24,"CBOXHID2",8)==0;
        if(modern){
            if(size<32){errorMsg="Footer v2 bị hỏng.";return false;}in.seekg(streamoff(size-32));in.read(reinterpret_cast<char*>(end),32);if(!in)return false;
            coverSize=wideSize(end);payloadSize=wideSize(end+8);auto metaSize=wideSize(end+16);
            if(!payloadSize || metaSize>1024*1024 || coverSize>size-32 || payloadSize>size-32-coverSize || metaSize!=size-32-coverSize-payloadSize){errorMsg="Footer v2 sai kích thước.";return false;}
            in.seekg(streamoff(coverSize+payloadSize));string metadata(size_t(metaSize),'\0');in.read(metadata.data(),streamsize(metaSize));if(!in)return false;
            info=MediaMetadata::Json::parse(metadata);if(info.value("schema","")!="cmd-box-hidden/2"){errorMsg="Metadata file ẩn không hợp lệ.";return false;}
            // Validate timestamps before creating the output.
            if(!info.value("timesValid",false))return false;
            for(const auto& key:{"created","accessed","written"})if(!info.contains(key) || !info[key].is_number_unsigned())return false;
        }else{
            in.seekg(streamoff(size-8));unsigned char legacy[8];in.read(reinterpret_cast<char*>(legacy),8);
            if(!in || memcmp(legacy+4,"HIDE",4)){errorMsg="Không có file ẩn hợp lệ.";return false;}
            payloadSize=(uint32_t(legacy[0])<<24)|(uint32_t(legacy[1])<<16)|(uint32_t(legacy[2])<<8)|legacy[3];if(!payloadSize || payloadSize>size-8){errorMsg="Footer cũ bị hỏng.";return false;}coverSize=size-8-payloadSize;
            info={{"schema","cmd-box-hidden/1"},{"note","Footer cũ không lưu timestamp hoặc tên nguồn."}};
        }
        in.seekg(streamoff(coverSize));FileSafety::ExclusiveOutput out(output);
        if(!out || !FileSafety::copyExact(in,out,payloadSize,true) || !out.commit()){errorMsg="Không trích được file; không ghi đè đầu ra.";return false;}
        if(modern && !restoreHiddenTimes(info,output)){errorMsg="Đã trích nhưng không khôi phục được timestamp.";return false;}return true;
    }catch(...){errorMsg="Không thể trích xuất file ẩn.";return false;}
}

void MediaProcessor::hideFileInImage() {
    if (!SystemCore::requireFeature(Feature::Steganography)) { SystemCore::waitEnter(); return; }
    SystemCore::cls();
    std::cout << "\n \x1b[38;2;195;165;255m╭── GIẤU FILE TRONG ẢNH (STEGANOGRAPHY ≤10 MB) ───╮\x1b[0m\n"
              << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n"
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

void MediaProcessor::hideFileInVideo() {
    if (!SystemCore::requireFeature(Feature::Steganography)) { SystemCore::waitEnter(); return; }
    SystemCore::cls();
    std::cout << "\n \x1b[38;2;195;165;255m╭── GIẤU FILE TRONG VIDEO (≤100 MB) ───────────────╮\x1b[0m\n"
              << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n"
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

void MediaProcessor::extractHiddenFromMedia() {
    if (!SystemCore::requireFeature(Feature::Steganography)) { SystemCore::waitEnter(); return; }
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
    std::string outputPath = (containerPath.parent_path() / ("extracted_" + std::to_string(now) + ".bin")).u8string();

    std::cout << "\nĐang phân tích & trích xuất\n";
    std::string errorMsg;
    if (extractHiddenFromMediaCore(in, outputPath, errorMsg)) {
        std::cout << "File lưu tại : " << outputPath << "\n";
        std::cout << "Dung lượng   : " << SystemCore::formatSize(fs::file_size(outputPath)) << "\n\n";
    } else {
        std::cout << "\nLỗi: " << errorMsg << "\n";
    }
}

void MediaProcessor::processAnFileTrongFile() {
    if (!SystemCore::requireFeature(Feature::Steganography)) { SystemCore::waitEnter(); return; }
    SystemCore::cls();
    std::cout << "\n \x1b[38;2;195;165;255m╭── ẨN & TRÍCH XUẤT TẬP TIN TRONG MEDIA ────────────╮\x1b[0m\n"
              << "   * Giấu file  : Kéo thả [File nền], [File cần ẩn] (vd: anh.jpg, data.zip)\n"
              << "   * Trích xuất : Kéo thả [File đã giấu] để tự động lấy lại file ẩn\n"
              << "   * 0 để quay lại\n"
              << " \x1b[38;2;195;165;255m╰──────────────────────────────────────────────────╯\x1b[0m\n\n"
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

    if (paths.size() == 1) {
        fs::path p = fs::u8path(paths[0]);
        if (!fs::exists(p)) {
            std::cout << "\nFile không tồn tại!\n";
            SystemCore::waitEnter();
            return;
        }

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
            std::cout << "\n[*] Phát hiện dữ liệu ẩn trong file: " << p.filename().u8string() << "\n"
                      << "[*] Đang tiến hành trích xuất...\n";
            long long now = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            std::string outPath = (p.parent_path() / ("extracted_" + std::to_string(now) + ".bin")).u8string();
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

        std::cout << "\n[*] File nền: " << p.filename().u8string() << "\n"
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

        std::string ext = containerPath.extension().u8string();
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

        std::string outPath = (containerPath.parent_path() / (containerPath.stem().u8string() + "_hidden" + ext)).u8string();

        std::cout << "\n[*] Đang nhúng '" << hiddenPath.filename().u8string()
                  << "' vào '" << containerPath.filename().u8string() << "'...\n";

        std::string err;
        bool ok = false;
        if (isImg) {
            ok = hideFileInImageCore(containerPath.u8string(), hiddenPath.u8string(), outPath, err);
        } else {
            ok = hideFileInVideoCore(containerPath.u8string(), hiddenPath.u8string(), outPath, err);
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
