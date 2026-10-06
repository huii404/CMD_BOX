#pragma once
#include "FileSafety.h"
#include "third_party/json.hpp"
#include <chrono>
#include <map>
#include <set>

namespace MediaMetadata {
namespace fs = std::filesystem;
using Json = nlohmann::json;
struct Times { FILETIME created{}, accessed{}, written{}; bool valid=false; };
inline Times readTimes(const fs::path& path) {
    Times t; HANDLE h=CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(h!=INVALID_HANDLE_VALUE){t.valid=GetFileTime(h,&t.created,&t.accessed,&t.written)!=FALSE;CloseHandle(h);}return t;
}
inline bool setTimes(const Times& t,const fs::path& path) {
    if(!t.valid)return false;
    HANDLE h=CreateFileW(path.c_str(),FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(h==INVALID_HANDLE_VALUE)return false;
    bool ok=SetFileTime(h,&t.created,&t.accessed,&t.written)!=FALSE;CloseHandle(h);return ok;
}
inline std::string quote(const std::string& arg) {
    std::string out="\"";size_t slashes=0;
    for(char c:arg){if(c=='\\'){++slashes;continue;}if(c=='\"'){out.append(slashes*2+1,'\\');out+='\"';}else{out.append(slashes,'\\');out+=c;}slashes=0;}
    out.append(slashes*2,'\\');return out+'\"';
}
// Bound output; terminate the child job on timeout.
inline bool capture(const std::vector<std::string>& args,std::string& output,DWORD timeout=180000) {
    output.clear();if(args.empty())return false;std::string command;
    for(const auto& arg:args){if(!command.empty())command+=' ';command+=quote(arg);}
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,command.c_str(),-1,nullptr,0);if(!count)return false;
    std::vector<wchar_t> line(count);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,command.c_str(),-1,line.data(),count);
    SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};HANDLE reader=nullptr,writer=nullptr;
    if(!CreatePipe(&reader,&writer,&sa,0))return false;
    SetHandleInformation(reader,HANDLE_FLAG_INHERIT,0);
    HANDLE nullFile=CreateFileW(L"NUL",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,nullptr);
    HANDLE job=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    bool setup=job && nullFile!=INVALID_HANDLE_VALUE && SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits));
    STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdOutput=writer;startup.hStdError=nullFile;startup.hStdInput=nullFile;PROCESS_INFORMATION process{};
    bool started=setup && CreateProcessW(nullptr,line.data(),nullptr,nullptr,TRUE,CREATE_SUSPENDED|CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process);
    CloseHandle(writer);bool ok=false;
    if(started){
        if(AssignProcessToJobObject(job,process.hProcess) && ResumeThread(process.hThread)!=DWORD(-1)){
            auto until=std::chrono::steady_clock::now()+std::chrono::milliseconds(timeout);bool bad=false;char buffer[65536];
            while(!bad && std::chrono::steady_clock::now()<until){
                DWORD available=0;if(!PeekNamedPipe(reader,nullptr,0,nullptr,&available,nullptr))break;
                if(available){DWORD n=0;if(!ReadFile(reader,buffer,(std::min)(available,DWORD(sizeof(buffer))),&n,nullptr))break;output.append(buffer,n);if(output.size()>64*1024*1024)bad=true;}
                else if(WaitForSingleObject(process.hProcess,20)==WAIT_OBJECT_0){
                    DWORD remaining=0;if(!PeekNamedPipe(reader,nullptr,0,nullptr,&remaining,nullptr) || !remaining)break;
                }
            }
            DWORD exit=1;ok=!bad && WaitForSingleObject(process.hProcess,5000)==WAIT_OBJECT_0 && GetExitCodeProcess(process.hProcess,&exit) && exit==0;
        }
        if(!ok){TerminateJobObject(job,1);TerminateProcess(process.hProcess,1);WaitForSingleObject(process.hProcess,5000);}
        CloseHandle(process.hThread);CloseHandle(process.hProcess);
    }
    if(job)CloseHandle(job);
    if(nullFile!=INVALID_HANDLE_VALUE)CloseHandle(nullFile);
    CloseHandle(reader);return ok;
}
inline fs::path binary(const std::wstring& name) {
    wchar_t module[32768]{};DWORD size=GetModuleFileNameW(nullptr,module,32768);
    if(size && size<32768){auto candidate=fs::path(module).parent_path()/name;if(fs::is_regular_file(candidate))return candidate;}
    auto local=fs::current_path()/L"bin"/name;if(fs::is_regular_file(local))return local;
    wchar_t found[32768]{};DWORD n=SearchPathW(nullptr,name.c_str(),nullptr,32768,found,nullptr);if(n && n<32768)return fs::path(found);
    for(const auto& root:{L"C:/msys64/ucrt64/bin",L"C:/msys64/mingw64/bin"}){auto candidate=fs::path(root)/name;if(fs::is_regular_file(candidate))return candidate;}
    return {};
}
inline std::vector<std::string> exifTool() {
    auto exe=binary(L"exiftool.exe");if(!exe.empty())return {exe.u8string()};
    // Development fallback: ExifTool source + Perl.
    auto script=binary(L"exiftool-source/exiftool-13.59/exiftool");auto perl=binary(L"perl.exe");
    if(perl.empty() && fs::is_regular_file(L"C:/msys64/usr/bin/perl.exe"))perl=L"C:/msys64/usr/bin/perl.exe";
    return !script.empty() && !perl.empty() ? std::vector<std::string>{perl.u8string(),script.u8string()} : std::vector<std::string>{};
}
inline bool tags(const fs::path& path,Json& result,std::string& error) {
    auto args=exifTool();if(args.empty()){error="Thiếu ExifTool: đặt exiftool.exe và exiftool_files cạnh main.exe.";return false;}
    args.insert(args.end(),{"-j","-G1","-struct","-b","-n","-all:all","-ICC_Profile","-MakerNotes","-charset","filename=UTF8","-api","QuickTimeUTC=1",fs::absolute(path).u8string()});std::string output;
    if(!capture(args,output)){error="Không đọc được metadata bằng ExifTool.";return false;}
    try{auto data=Json::parse(output);if(!data.is_array() || data.size()!=1 || !data[0].is_object())throw std::runtime_error("tags");result=data[0];
        for(auto it=result.begin();it!=result.end();++it)if(it.key()=="ExifTool:Error"){error=it.value().dump();return false;}
        return true;
    }catch(...){error="ExifTool trả dữ liệu không hợp lệ.";return false;}
}
inline Json value(const Json& tags,const std::string& name) {
    auto fold=[](std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return char(std::tolower(c));});return s;};
    for(auto it=tags.begin();it!=tags.end();++it){auto p=it.key().find_last_of(':');if(fold(it.key().substr(p==std::string::npos?0:p+1))==fold(name))return it.value();}return nullptr;
}
inline bool image(const fs::path& path) {
    auto ext=path.extension().u8string();std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return char(std::tolower(c));});
    static const std::set<std::string> supported={".jpg",".jpeg",".png",".webp",".bmp",".tif",".tiff",".heic",".heif",".dng",".avif"};return supported.count(ext)!=0;
}
struct Snapshot {Json metadata, probe;Times times;std::string source;bool valid=false;};
inline Snapshot snapshot(const fs::path& source,std::string& error) {
    Snapshot s;s.times=readTimes(source);s.source=fs::absolute(source).u8string();s.valid=s.times.valid && tags(source,s.metadata,error);if(!s.times.valid)error="Không đọc được timestamp nguồn.";
    auto ffprobe=binary(L"ffprobe.exe");std::string output;
    if(s.valid && !ffprobe.empty() && capture({ffprobe.u8string(),"-v","error","-show_format","-show_streams","-show_chapters","-of","json",fs::absolute(source).u8string()},output)){
        try{s.probe=Json::parse(output);}catch(...){}
    }return s;
}
inline std::string tagName(const std::string& key) {
    auto colon=key.find_last_of(':');return key.substr(colon==std::string::npos?0:colon+1);
}
inline bool technicalTag(const std::string& name) {
    static const std::set<std::string> excluded={
        "ThumbnailImage","PreviewImage","JpgFromRaw","OtherImage","ThumbnailOffset","ThumbnailLength",
        "PreviewImageStart","PreviewImageLength","StripOffsets","StripByteCounts","TileOffsets","TileByteCounts",
        "ImageWidth","ImageHeight","ExifImageWidth","ExifImageHeight","RelatedImageWidth","RelatedImageHeight",
        "BitsPerSample","SamplesPerPixel","PhotometricInterpretation","Compression","PlanarConfiguration",
        "RowsPerStrip","YCbCrSubSampling","YCbCrPositioning","Predictor","NewSubfileType","SubfileType",
        "Software","ProcessingSoftware","CreatorTool","History","MetadataDate","ModifyDate","CurrentIPTCDigest",
        "Encoder","EncodingTool","MajorBrand","MinorVersion","CompatibleBrands","HandlerType","XMPToolkit",
        "Duration","MediaDuration","TrackDuration","VideoFrameRate","FrameRate","VideoCodec","AudioCodec",
        "FileSize","ImageSize","Megapixels","EXIF","XMP","MakerNoteByteOrder","ExifByteOrder"
    };
    return excluded.count(name)!=0;
}
inline bool memoryTag(const std::string& key) {
    const auto colon=key.find(':');auto group=key.substr(0,colon);auto name=tagName(key);
    if(technicalTag(name) || group=="IFD1")return false;
    if(group.rfind("XMP-",0)==0 || group.rfind("ID3v",0)==0 || group=="Keys" || group=="ItemList" || group=="UserData" ||
       group=="IPTC" || group=="IFD0" || group=="ExifIFD" ||
       group=="GPS" || group=="InteropIFD")return true;
    static const std::set<std::string> important={
        "DateTimeOriginal","CreateDate","CreationDate","MediaCreateDate","TrackCreateDate",
        "OffsetTimeOriginal","OffsetTimeDigitized","SubSecTimeOriginal","SubSecTimeDigitized",
        "GPSLatitude","GPSLatitudeRef","GPSLongitude","GPSLongitudeRef","GPSAltitude","GPSAltitudeRef",
        "GPSCoordinates","GPSDateStamp","GPSTimeStamp","Make","Model","LensModel","LensMake",
        "SerialNumber","LensSerialNumber","Artist","Author","Copyright","Description","Title",
        "UserComment","Keywords","Subject","Rating","ImageDescription","Orientation","ICC_Profile","MakerNotes",
        "Album","AlbumArtist","Composer","Genre","Track","Year","Comment","LocationInformation"
    };
    return important.count(name)!=0;
}
inline bool emptyDate(const std::string& name,const Json& value) {
    return name.find("Date")!=std::string::npos && value.is_string() && value.get<std::string>().rfind("0000:",0)==0;
}
inline bool probeFile(const fs::path& path,Json& result) {
    auto tool=binary(L"ffprobe.exe");std::string output;
    if(tool.empty() || !capture({tool.u8string(),"-v","error","-show_format","-show_streams","-show_chapters","-of","json",fs::absolute(path).u8string()},output))return false;
    try{result=Json::parse(output);return result.is_object();}catch(...){return false;}
}
inline std::string lower(std::string value) {
    std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return char(std::tolower(c));});return value;
}
inline std::string normalizedDate(std::string value) {
    if(value.size()<19)return value;
    if(value[4]=='-')value[4]=':';
    if(value[7]=='-')value[7]=':';
    if(value[10]=='T')value[10]=' ';
    if(value.back()=='Z'){value.pop_back();value+="+00:00";}
    if(value.size()>19 && value[19]=='.'){
        auto end=value.find_first_not_of("0123456789",20);if(end==std::string::npos)end=value.size();
        auto last=end;while(last>20 && value[last-1]=='0')--last;
        if(last==20)--last;
        value.erase(last,end-last);
    }
    return value;
}
inline bool sameTag(const std::string& name,const Json& before,const Json& after) {
    if(before==after)return true;
    if(before.is_number() && after.is_string()){
        try{auto number=Json::parse(after.get<std::string>());if(number.is_number() && number==before)return true;}catch(...){}
    }
    if((lower(name).find("date")!=std::string::npos || lower(name)=="creation_time") && before.is_string() && after.is_string())
        return normalizedDate(before.get<std::string>())==normalizedDate(after.get<std::string>());
    return false;
}
inline Json nativeFormatValue(const Json& metadata,const std::string& key) {
    auto compact=[](std::string s){s=lower(s);s.erase(std::remove_if(s.begin(),s.end(),[](unsigned char c){return !std::isalnum(c);}),s.end());return s;};
    for(auto it=metadata.begin();it!=metadata.end();++it)if(compact(tagName(it.key()))==compact(key))return it.value();
    return nullptr;
}
inline bool verifyFormatTags(const Json& source,const Json& target,const Json& sourceTags,const Json& targetTags,std::vector<std::string>& errors) {
    auto before=source.value("format",Json::object()).value("tags",Json::object());
    auto after=target.value("format",Json::object()).value("tags",Json::object());
    static const std::set<std::string> technical={"encoder","encoding_tool","major_brand","minor_version","compatible_brands","duration","bps","number_of_frames","number_of_bytes"};
    for(auto it=before.begin();it!=before.end();++it){
        auto key=lower(it.key());if(technical.count(key) || key.rfind("_statistics_",0)==0)continue;
        auto expected=nativeFormatValue(sourceTags,key),embedded=nativeFormatValue(targetTags,key);
        if(expected.is_null())expected=it.value();
        if(!embedded.is_null()){
            if(!sameTag(key,expected,embedded))errors.push_back("Container:"+it.key());
            continue;
        }
        bool found=false;
        for(auto actual=after.begin();actual!=after.end();++actual)if(lower(actual.key())==key && sameTag(key,expected,actual.value())){found=true;break;}
        if(!found)errors.push_back("Container:"+it.key());
    }
    return errors.empty();
}
inline void verifyStreamTags(const Json& source,const Json& target,const Json& targetTags,bool audioOnly,std::vector<std::string>& errors) {
    static const std::set<std::string> technical={"encoder","handler_name","vendor_id","duration","bps","number_of_frames","number_of_bytes"};
    std::map<std::string,size_t> ordinals;
    for(const auto& stream:source.value("streams",Json::array())){
        auto type=stream.value("codec_type","");auto ordinal=ordinals[type]++;
        if(audioOnly && (type!="audio" || ordinal>0))continue;
        Json actualTags=Json::object();size_t index=0;bool found=false;
        for(const auto& actual:target.value("streams",Json::array()))if(actual.value("codec_type","")==type && index++==ordinal){actualTags=actual.value("tags",Json::object());found=true;break;}
        if(!found){errors.push_back("Stream:"+type+":"+std::to_string(ordinal));continue;}
        auto tags=stream.value("tags",Json::object());
        for(auto it=tags.begin();it!=tags.end();++it){
            auto key=lower(it.key());if(technical.count(key) || key.rfind("_statistics_",0)==0)continue;
            if(key=="language" && (it.value()=="und" || it.value()==""))continue;
            auto embedded=audioOnly?nativeFormatValue(targetTags,"audio_track_"+key):nativeFormatValue(actualTags,key);
            if(embedded.is_null() && !audioOnly){for(auto tag=actualTags.begin();tag!=actualTags.end();++tag)if(lower(tag.key())==key){embedded=tag.value();break;}}
            if(!sameTag(key,it.value(),embedded))errors.push_back("Stream:"+type+":"+std::to_string(ordinal)+":"+it.key());
        }
    }
}
// Copy directly into the media, then refuse output if required tags did not survive.
inline bool preserve(const Snapshot& source,const fs::path& target,std::vector<std::string>& errors) {
    errors.clear();auto args=exifTool();if(!source.valid || args.empty())return false;
    auto ext=lower(target.extension().u8string());bool photo=image(target);
    if(photo || ext==".mp4" || ext==".mov" || ext==".m4a") {
        args.insert(args.end(),{"-charset","filename=UTF8","-api","QuickTimeUTC=1","-overwrite_original","-TagsFromFile",source.source});
        if(photo){
            auto profile=value(source.metadata,"ColorSpaceData");
            if(profile.is_string() && profile.get<std::string>().find("RGB")!=0){errors.push_back("ICC không phải RGB; chưa có chuyển đổi màu bảo toàn profile.");return false;}
            args.insert(args.end(),{"-all:all","-ICC_Profile","--ThumbnailImage","--PreviewImage","--JpgFromRaw","--OtherImage",
                "--Orientation","--ExifImageWidth","--ExifImageHeight","--ImageWidth","--ImageHeight"});
            auto orientation=value(source.metadata,"Orientation");if(orientation.is_number_integer())args.push_back("-Orientation#="+orientation.dump());
            args.insert(args.end(),{"-ExifImageWidth<ImageWidth","-ExifImageHeight<ImageHeight"});
        }else{
            args.insert(args.end(),{"-EXIF:all","-XMP:all","-IPTC:all","-Keys:all","-ItemList:all","-UserData:all",
                "-QuickTime:CreateDate","-QuickTime:TrackCreateDate","-QuickTime:MediaCreateDate",
                "-XMP-exif:DateTimeOriginal<DateTimeOriginal","-ItemList:Artist<Artist"});
        }
        args.push_back(fs::absolute(target).u8string());std::string output;
        if(!capture(args,output)){errors.push_back("ExifTool không ghi được metadata vào file.");return false;}
    }
    Json actual;std::string error;if(!tags(target,actual,error)){errors.push_back(error);return false;}
    for(auto it=source.metadata.begin();it!=source.metadata.end();++it){
        if(!memoryTag(it.key()) || emptyDate(tagName(it.key()),it.value()))continue;
        if(it.key()=="File:Comment" && it.value().is_string() && it.value().get<std::string>().rfind("Lavc",0)==0)continue;
        auto match=actual.find(it.key());auto after=match!=actual.end()?match.value():value(actual,tagName(it.key()));
        if(!sameTag(tagName(it.key()),it.value(),after))errors.push_back(it.key());
    }
    if(!photo && !source.probe.is_null()){
        Json probe;if(!probeFile(target,probe)){errors.push_back("Không kiểm tra được metadata container bằng ffprobe.");return false;}
        verifyFormatTags(source.probe,probe,source.metadata,actual,errors);
        verifyStreamTags(source.probe,probe,actual,ext==".mp3",errors);
    }
    return errors.empty();
}
// Prefer capture dates; use last-write time as a marked fallback.
inline std::string albumMonth(const Snapshot& snapshot,bool& fallback) {
    fallback=false;
    for(const auto& key:{"SubSecDateTimeOriginal","DateTimeOriginal","DateCreated","CreationDate","CreateDate","MediaCreateDate"}){
        auto v=value(snapshot.metadata,key);if(!v.is_string())continue;auto s=v.get<std::string>();
        if(s.size()<10 || s[4]!=':' || s[7]!=':')continue;
        try{int y=std::stoi(s.substr(0,4)),m=std::stoi(s.substr(5,2)),d=std::stoi(s.substr(8,2));
            if(y>=1601 && y<=9999 && m>=1 && m<=12 && d>=1 && d<=31)return s.substr(0,4)+"/"+s.substr(5,2);
        }catch(...){}
    }
    SYSTEMTIME date{};FILETIME local{};fallback=true;
    if(snapshot.times.valid && FileTimeToLocalFileTime(&snapshot.times.written,&local) && FileTimeToSystemTime(&local,&date)){
        char result[16];snprintf(result,sizeof(result),"%04u/%02u",date.wYear,date.wMonth);return result;
    }return "UnknownDate";
}

inline bool publishHandle(HANDLE file,const fs::path& target) {
    auto name=fs::absolute(target).wstring();size_t bytes=name.size()*sizeof(wchar_t);std::vector<unsigned char> buffer(sizeof(FILE_RENAME_INFO)+bytes);
    auto rename=reinterpret_cast<FILE_RENAME_INFO*>(buffer.data());rename->ReplaceIfExists=FALSE;rename->RootDirectory=nullptr;rename->FileNameLength=DWORD(bytes);memcpy(rename->FileName,name.data(),bytes);
    return SetFileInformationByHandle(file,FileRenameInfo,rename,DWORD(buffer.size()))!=FALSE;
}
}
