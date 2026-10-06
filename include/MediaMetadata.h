#pragma once
#include "FileSafety.h"
#include "third_party/json.hpp"
#include <chrono>
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
    args.insert(args.end(),{"-j","-G1","-struct","-b","-n","-all:all","-ICC_Profile","-EXIF","-MakerNotes","-charset","filename=UTF8","-api","QuickTimeUTC=1",fs::absolute(path).u8string()});std::string output;
    if(!capture(args,output)){error="Không đọc được metadata bằng ExifTool.";return false;}
    try{auto data=Json::parse(output);if(!data.is_array() || data.size()!=1 || !data[0].is_object())throw std::runtime_error("tags");result=data[0];
        for(auto it=result.begin();it!=result.end();++it)if(it.key()=="ExifTool:Error"){error=it.value().dump();return false;}
        return true;
    }catch(...){error="ExifTool trả dữ liệu không hợp lệ.";return false;}
}
inline Json value(const Json& tags,const std::string& name) {
    for(auto it=tags.begin();it!=tags.end();++it){auto p=it.key().find_last_of(':');if(it.key().substr(p==std::string::npos?0:p+1)==name)return it.value();}return nullptr;
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
inline bool writeJson(const fs::path& path,const Json& data) {
    auto text=data.dump(2);FileSafety::ExclusiveOutput out(path);return out && out.write(text.data(),text.size()) && out.commit();
}
// Copy descriptive tags; refresh dimensions from the rendered file.
inline bool preserve(const Snapshot& source,const fs::path& target,std::vector<std::string>& warnings) {
    auto args=exifTool();if(!source.valid || args.empty())return false;
    if(image(target)) {
        args.insert(args.end(),{"-charset","filename=UTF8","-overwrite_original","-TagsFromFile",source.source,"-all:all",
            "--ThumbnailImage","--PreviewImage","--JpgFromRaw","--OtherImage","--Orientation","--ExifImageWidth","--ExifImageHeight","--ImageWidth","--ImageHeight"});
        // Keep orientation paired with -noautorotate.
        auto orientation=value(source.metadata,"Orientation");if(orientation.is_number_integer())args.push_back("-Orientation#="+orientation.dump());
        auto profile=value(source.metadata,"ColorSpaceData");if(profile.is_string() && profile.get<std::string>().find("RGB")!=0){args.push_back("--ICC_Profile");args.push_back("-ICC_Profile=");warnings.push_back("ICC gốc không phải RGB: profile lưu trong JSON, không gắn sai lên ảnh RGB.");}
        else args.push_back("-ICC_Profile");
        args.insert(args.end(),{"-ExifImageWidth<ImageWidth","-ExifImageHeight<ImageHeight",fs::absolute(target).u8string()});std::string output;
        if(!capture(args,output)){warnings.push_back("Không nhúng được đầy đủ metadata; thông tin nguồn được lưu trong file JSON đi kèm.");}
    }
    Json actual;std::string error;if(!tags(target,actual,error))return false;
    for(const auto& key:{"DateTimeOriginal","CreateDate","OffsetTimeOriginal","GPSLatitude","GPSLongitude","Make","Model","LensModel","Artist","Copyright","Description","Title","UserComment","Keywords","Subject","Rating","ImageDescription","OffsetTimeDigitized","SubSecTimeOriginal","Orientation","ICC_Profile"}){
        auto before=value(source.metadata,key);if(!before.is_null() && before!=value(actual,key))warnings.push_back(std::string(key)+": bản gốc được lưu trong JSON đi kèm.");
    }
    return true;
}
inline Json record(const Snapshot& source,const fs::path& output,const Json& actual,const std::vector<std::string>& warnings) {
    auto ticks=[](FILETIME t){return (uint64_t(t.dwHighDateTime)<<32)|t.dwLowDateTime;};
    return {{"schema","cmd-box-media/1"},{"source",source.source},{"output",output.u8string()},{"sourceMetadata",source.metadata},{"sourceProbe",source.probe},{"outputMetadata",actual},
        {"sourceFileTimes",{{"created",ticks(source.times.created)},{"accessed",ticks(source.times.accessed)},{"written",ticks(source.times.written)}}},{"warnings",warnings}};
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
