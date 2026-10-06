#include "MediaMetadata.h"
#include "ProcessRunner.h"
#define private public
#include "MediaProcessor.h"
#undef private
#include <cassert>
#include <sstream>
#include <regex>
#include <conio.h>
#include "../src/media/MediaProcessor.cpp"
#include "../src/media/MediaAlbum.cpp"
void SystemCore::cls(){}
void SystemCore::waitEnter(){}
bool SystemCore::confirm(const std::string&){return true;}
bool SystemCore::runRawCommand(const std::string& c){return ProcessRunner::run(c);}
int SystemCore::readInt(const std::string&,int){std::string s;std::getline(std::cin,s);return s.empty()?0:std::stoi(s);}
std::string SystemCore::formatSize(long long s){return std::to_string(s);}
std::string SystemCore::trim(const std::string& s){return s;}
std::vector<std::string> SystemCore::parsePaths(const std::string& s){if(s.empty()||s=="0")return {};return {s.size()>1&&s.front()=='\"'?s.substr(1,s.size()-2):s};}
static bool run(std::vector<std::string> a){std::string s;return MediaMetadata::capture(a,s);}
static void convert(MediaProcessor& p,const fs::path& file,int choice){std::istringstream s("\""+file.u8string()+"\"\n"+std::to_string(choice)+"\n0\n");auto old=std::cin.rdbuf(s.rdbuf());p.processConvertFormatBatch();std::cin.rdbuf(old);}
int main(){
    using namespace MediaMetadata;
    auto temp=fs::temp_directory_path();
    if(temp.filename().empty())temp=temp.parent_path();
    auto root=temp/("cmd-box-media-test-"+std::to_string(GetCurrentProcessId()));
    assert(fs::create_directory(root));
    fs::create_directories(root/fs::u8path("kỷ niệm"));auto ff=binary(L"ffmpeg.exe");std::string error;
    auto photo=root/fs::u8path("kỷ niệm/ảnh.jpg");assert(run({ff.u8string(),"-v","error","-f","lavfi","-i","color=red:s=16x8","-frames:v","1",photo.u8string()}));
    auto edit=exifTool();edit.insert(edit.end(),{"-overwrite_original","-charset","filename=UTF8","-DateTimeOriginal=2020:08:15 17:30:22","-OffsetTimeOriginal=+07:00","-Orientation#=6","-Make=MemoryCam","-GPSLatitude=10.5","-GPSLatitudeRef=N","-GPSLongitude=106.5","-GPSLongitudeRef=E","-XMP-dc:Description=Family memory"});
    auto profile=fs::path(L"C:/Windows/System32/spool/drivers/color/sRGB Color Space Profile.icm");if(fs::exists(profile))edit.push_back("-ICC_Profile<="+profile.u8string());edit.push_back(photo.u8string());assert(run(edit));
    auto original=snapshot(photo,error);assert(original.valid);MediaProcessor media;convert(media,photo,2);convert(media,photo,3);
    for(const auto& ext:{".png",".webp"}){auto file=photo.parent_path()/L"CMD_BOX_Output"/fs::u8path("ảnh"+std::string(ext));Json actual;assert(tags(file,actual,error));
        for(const auto& key:{"DateTimeOriginal","OffsetTimeOriginal","GPSLatitude","GPSLongitude","Make","Orientation","Description","ICC_Profile"})if(!value(original.metadata,key).is_null())assert(value(original.metadata,key)==value(actual,key));
        assert(value(actual,"ImageWidth")==16&&value(actual,"ImageHeight")==8);assert(readTimes(file).written.dwLowDateTime==original.times.written.dwLowDateTime);assert(fs::exists(fs::path(file.wstring()+L".metadata.json")));
    }
    std::vector<MediaAlbum::Item> album;assert(MediaAlbum::collect(root,album,error)&&album.size()==1);assert(album[0].relative.generic_u8string()=="2020/08/kỷ niệm/ảnh.jpg"&&!album[0].fallback);assert(MediaAlbum::copy(album[0],root/L"CMD_BOX_Album",error));assert(!MediaAlbum::copy(album[0],root/L"CMD_BOX_Album",error));assert(MediaAlbum::collect(root,album,error)&&album.size()==1);
    auto video=root/L"two.mkv";assert(run({ff.u8string(),"-v","error","-f","lavfi","-i","color=s=320x240:d=0.2","-f","lavfi","-i","sine=frequency=440:duration=0.2","-f","lavfi","-i","sine=frequency=880:duration=0.2","-map","0:v","-map","1:a","-map","2:a","-c:v","libx264","-c:a","aac","-metadata:s:a:0","language=vie","-metadata:s:a:1","language=eng",video.u8string()}));
    convert(media,video,1);Json info;assert(probeMedia(root/L"CMD_BOX_Output"/L"two.mp4",info,error));assert(info["streams"].size()==3&&info["streams"][1]["tags"]["language"]=="vie"&&info["streams"][2]["tags"]["language"]=="eng");
    auto deep=root/L"deep.tiff";assert(run({ff.u8string(),"-v","error","-f","lavfi","-i","color=s=16x8","-pix_fmt","rgb48le","-frames:v","1",deep.u8string()}));convert(media,deep,2);Json bits;assert(tags(root/L"CMD_BOX_Output"/L"deep.png",bits,error)&&value(bits,"BitDepth")==16);
    auto payload=root/L"payload.txt";{std::ofstream f(payload);f<<"family-memory";}auto old=readTimes(payload);auto cover=root/L"hidden.jpg";assert(media.hideFileInImageCore(photo.u8string(),payload.u8string(),cover.u8string(),error));auto recovered=root/L"recovered.txt";assert(media.extractHiddenFromMediaCore(cover.u8string(),recovered.u8string(),error));assert(readTimes(recovered).written.dwLowDateTime==old.written.dwLowDateTime);
    {std::ifstream f(recovered);std::string s((std::istreambuf_iterator<char>(f)),{});assert(s=="family-memory");}
    auto legacy=root/L"old.bin";{std::ofstream f(legacy,std::ios::binary);f<<"cover";for(char c:std::string("old"))f.put(char(c^0xAA));const char tail[8]={0,0,0,3,'H','I','D','E'};f.write(tail,8);}assert(media.extractHiddenFromMediaCore(legacy.u8string(),(root/L"old.txt").u8string(),error));
    Json chapters={{"chapters",Json::array({{{"start_time","0.0"},{"end_time","2.0"},{"tags",{{"title","Memory"}}}}})}};auto scaled=root/L"scaled.ffmetadata";assert(writeScaledChapters(chapters,2,scaled));{std::ifstream f(scaled);std::string s((std::istreambuf_iterator<char>(f)),{});assert(s.find("END=1000000")!=std::string::npos);}
    auto originalChapters=root/L"original.ffmeta";{std::ofstream f(originalChapters);f<<";FFMETADATA1\n[CHAPTER]\nTIMEBASE=1/1000\nSTART=0\nEND=100\ntitle=First\n[CHAPTER]\nTIMEBASE=1/1000\nSTART=100\nEND=200\ntitle=Second\n";}
    auto withChapters=root/L"chaptered.mkv";assert(run({ff.u8string(),"-v","error","-i",video.u8string(),"-f","ffmetadata","-i",originalChapters.u8string(),"-map","0","-map_chapters","1","-c","copy",withChapters.u8string()}));
    auto faster=root/L"speed.mp4";assert(media.changeSpeedCore(withChapters.u8string(),faster.u8string(),2));Json fast;assert(probeMedia(faster,fast,error));assert(fast["chapters"].size()==2);
    assert(std::abs(std::stod(fast["chapters"][1]["start_time"].get<std::string>())-0.05)<0.000001);assert(fast["streams"][1]["tags"]["language"]=="vie"&&fast["streams"][2]["tags"]["language"]=="eng");
    auto audio=root/L"audio.mp3";assert(media.extractAudioCore(video.u8string(),audio.u8string())&&fs::file_size(audio)>0);
    Snapshot fallback;fallback.times=original.times;bool used=false;assert(albumMonth(fallback,used)!="UnknownDate"&&used);
    assert(root.parent_path()==temp && root.filename().u8string().rfind("cmd-box-media-test-",0)==0);
    fs::remove_all(root);
    std::cout<<"PASS: EXIF/XMP/ICC/GPS/orientation, Unicode, timestamps/sidecar, directory album/exclusions/collisions, all audio tracks/languages, PNG 16-bit, chapter scaling with real speed render, audio extraction, hidden v2/legacy.\n";
}
