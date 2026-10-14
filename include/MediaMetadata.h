#pragma once
#include "FileSafety.h"
#include "Json.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <set>

namespace MediaMetadata {
namespace fs = std::filesystem;
using Json = SimpleJson::Json;
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
    return {};
}
inline bool tags(const fs::path& path,Json& result,std::string& error) {
    auto args=exifTool();
    if(args.empty()){
        result=Json::object();
        return true;
    }
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
using Bytes = std::vector<unsigned char>;
// ---- Native EXIF (no external tool): read from JPEG/PNG/WebP, write into JPEG. ----
// FFmpeg's mjpeg/webp encoders drop EXIF, so the raw EXIF blob is carried over byte-for-byte.
inline bool readExifBlob(const fs::path& path,Bytes& blob) {
    blob.clear();
    std::ifstream in(path,std::ios::binary);if(!in)return false;
    unsigned char head[12]{};in.read(reinterpret_cast<char*>(head),12);auto got=in.gcount();in.clear();
    static const unsigned char tag[6]={'E','x','i','f',0,0};
    auto take=[&](size_t n)->bool{
        if(n==0||n>16u*1024*1024)return false;
        Bytes b(n);if(!in.read(reinterpret_cast<char*>(b.data()),std::streamsize(n)))return false;
        if(n>=6&&memcmp(b.data(),tag,6)==0)b.erase(b.begin(),b.begin()+6);
        blob=std::move(b);return !blob.empty();
    };
    if(got>=2&&head[0]==0xFF&&head[1]==0xD8){
        in.seekg(2);
        for(;;){
            unsigned char m[4];if(!in.read(reinterpret_cast<char*>(m),4)||m[0]!=0xFF||m[1]==0xD9||m[1]==0xDA)return false;
            size_t len=(size_t(m[2])<<8)|m[3];if(len<2)return false;size_t body=len-2;
            if(m[1]==0xE1&&body>=6){
                unsigned char id[6]{};if(!in.read(reinterpret_cast<char*>(id),6))return false;
                if(memcmp(id,tag,6)==0)return take(body-6);
                in.seekg(std::streamoff(body-6),std::ios::cur);
            }else in.seekg(std::streamoff(body),std::ios::cur);
        }
    }
    if(got>=8&&memcmp(head,"\x89PNG\r\n\x1a\n",8)==0){
        in.seekg(8);
        for(;;){
            unsigned char h[8];if(!in.read(reinterpret_cast<char*>(h),8))return false;
            size_t len=(size_t(h[0])<<24)|(size_t(h[1])<<16)|(size_t(h[2])<<8)|h[3];
            if(memcmp(h+4,"eXIf",4)==0)return take(len);
            if(memcmp(h+4,"IEND",4)==0)return false;
            in.seekg(std::streamoff(len+4),std::ios::cur);
        }
    }
    if(got>=12&&memcmp(head,"RIFF",4)==0&&memcmp(head+8,"WEBP",4)==0){
        in.seekg(12);
        for(;;){
            unsigned char h[8];if(!in.read(reinterpret_cast<char*>(h),8))return false;
            size_t len=size_t(h[4])|(size_t(h[5])<<8)|(size_t(h[6])<<16)|(size_t(h[7])<<24);
            if(memcmp(h,"EXIF",4)==0)return take(len);
            in.seekg(std::streamoff(len+(len&1)),std::ios::cur);
        }
    }
    if(got>=4&&((head[0]=='I'&&head[1]=='I'&&head[2]==0x2a&&head[3]==0)||(head[0]=='M'&&head[1]=='M'&&head[2]==0&&head[3]==0x2a))){
        in.seekg(0,std::ios::end);auto sz=size_t(in.tellg());in.seekg(0,std::ios::beg);
        return take((std::min)(sz,size_t(16u*1024*1024)));
    }
    return false;
}
// DateTimeOriginal / DateTimeDigitized / ModifyDate ("YYYY:MM:DD hh:mm:ss") from a raw EXIF TIFF blob.
inline std::string exifDate(const Bytes& t) {
    if(t.size()<8)return {};
    bool le=t[0]=='I'&&t[1]=='I';if(!le&&!(t[0]=='M'&&t[1]=='M'))return {};
    auto u16=[&](size_t o)->uint32_t{if(o+2>t.size())return 0;return le?uint32_t(t[o])|(uint32_t(t[o+1])<<8):(uint32_t(t[o])<<8)|t[o+1];};
    auto u32=[&](size_t o)->uint32_t{return le?u16(o)|(u16(o+2)<<16):(u16(o)<<16)|u16(o+2);};
    auto find=[&](size_t ifd,uint32_t tag,size_t& off,uint32_t& count)->bool{
        if(ifd+2>t.size())return false;
        uint32_t n=u16(ifd);
        for(uint32_t i=0;i<n;++i){size_t e=ifd+2+size_t(i)*12;if(e+12>t.size())return false;
            if(u16(e)==tag){count=u32(e+4);off=count<=4?e+8:u32(e+8);return true;}}
        return false;
    };
    auto text=[&](size_t off,uint32_t count)->std::string{
        if(off>=t.size()||!count)return {};
        std::string s(reinterpret_cast<const char*>(&t[off]),(std::min)(size_t(count),t.size()-off));
        auto z=s.find('\0');if(z!=std::string::npos)s.resize(z);return s;
    };
    size_t ifd0=u32(4),off=0;uint32_t count=0;
    if(find(ifd0,0x8769,off,count)){size_t exifIfd=u32(off);
        for(uint32_t tag:{0x9003u,0x9004u})if(find(exifIfd,tag,off,count)){auto s=text(off,count);if(s.size()>=19)return s;}}
    if(find(ifd0,0x0132,off,count)){auto s=text(off,count);if(s.size()>=19)return s;}
    return {};
}
// Rewrite a JPEG with the given EXIF blob placed after JFIF; any older Exif APP1 is dropped.
inline bool writeExifJpeg(const fs::path& target,const Bytes& blob,std::string& why) {
    if(blob.empty()||blob.size()+8>65535){why="EXIF vượt giới hạn 64KB của JPEG.";return false;}
    Bytes data;{std::ifstream in(target,std::ios::binary);if(!in){why="Không mở được JPEG đích.";return false;}data.assign(std::istreambuf_iterator<char>(in),{});}
    if(data.size()<4||data[0]!=0xFF||data[1]!=0xD8){why="Đích không phải JPEG hợp lệ.";return false;}
    static const unsigned char tag[6]={'E','x','i','f',0,0};
    Bytes app1={0xFF,0xE1,uint8_t((blob.size()+8)>>8),uint8_t((blob.size()+8)&0xFF)};
    app1.insert(app1.end(),tag,tag+6);app1.insert(app1.end(),blob.begin(),blob.end());
    Bytes out={0xFF,0xD8};size_t pos=2;bool placed=false;
    while(pos+4<=data.size()&&data[pos]==0xFF){
        unsigned m=data[pos+1];if(m==0xDA||m==0xD9)break;
        size_t len=(size_t(data[pos+2])<<8)|data[pos+3];if(len<2||pos+2+len>data.size()){why="JPEG đích bị hỏng.";return false;}
        bool oldExif=m==0xE1&&len>=8&&memcmp(&data[pos+4],tag,6)==0;
        if(!placed&&m!=0xE0){out.insert(out.end(),app1.begin(),app1.end());placed=true;}
        if(!oldExif)out.insert(out.end(),data.begin()+std::ptrdiff_t(pos),data.begin()+std::ptrdiff_t(pos+2+len));
        pos+=2+len;
    }
    if(!placed)out.insert(out.end(),app1.begin(),app1.end());
    out.insert(out.end(),data.begin()+std::ptrdiff_t(pos),data.end());
    fs::path temp=target;temp+=L".exif.tmp";
    {std::ofstream o(temp,std::ios::binary|std::ios::trunc);o.write(reinterpret_cast<const char*>(out.data()),std::streamsize(out.size()));if(!o){why="Không ghi được file tạm.";return false;}}
    if(!MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(temp.c_str());why="Không thay được JPEG đích.";return false;}
    return true;
}

inline uint32_t pngCrc32(const unsigned char* buf, size_t len) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320L ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        init = true;
    }
    uint32_t c = 0xFFFFFFFF;
    for (size_t i = 0; i < len; ++i) c = table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFF;
}

inline bool writeExifPng(const fs::path& target, const Bytes& blob, std::string& why) {
    if (blob.empty()) return true;
    Bytes data;
    {
        std::ifstream in(target, std::ios::binary);
        if (!in) { why = "Không mở được PNG đích."; return false; }
        data.assign(std::istreambuf_iterator<char>(in), {});
    }
    static const unsigned char pngHead[8] = {0x89, 'P', 'N', 'G', '\r', '\n', '\x1a', '\n'};
    if (data.size() < 33 || memcmp(data.data(), pngHead, 8) != 0) {
        why = "Đích không phải PNG hợp lệ.";
        return false;
    }
    Bytes filtered = {data.begin(), data.begin() + 8};
    size_t pos = 8;
    while (pos + 12 <= data.size()) {
        uint32_t len = (uint32_t(data[pos]) << 24) | (uint32_t(data[pos+1]) << 16) | (uint32_t(data[pos+2]) << 8) | uint32_t(data[pos+3]);
        size_t total = 12 + len;
        if (pos + total > data.size()) break;
        if (memcmp(data.data() + pos + 4, "eXIf", 4) != 0) {
            filtered.insert(filtered.end(), data.begin() + pos, data.begin() + pos + total);
        }
        pos += total;
    }
    data = std::move(filtered);

    size_t sz = blob.size();
    Bytes chunk;
    chunk.push_back(uint8_t(sz >> 24));
    chunk.push_back(uint8_t(sz >> 16));
    chunk.push_back(uint8_t(sz >> 8));
    chunk.push_back(uint8_t(sz & 0xFF));
    chunk.insert(chunk.end(), {'e', 'X', 'I', 'f'});
    chunk.insert(chunk.end(), blob.begin(), blob.end());
    uint32_t crc = pngCrc32(chunk.data() + 4, chunk.size() - 4);
    chunk.push_back(uint8_t(crc >> 24));
    chunk.push_back(uint8_t(crc >> 16));
    chunk.push_back(uint8_t(crc >> 8));
    chunk.push_back(uint8_t(crc & 0xFF));

    data.insert(data.begin() + 33, chunk.begin(), chunk.end());

    fs::path temp = target; temp += L".exif.tmp";
    {
        std::ofstream o(temp, std::ios::binary | std::ios::trunc);
        o.write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
        if (!o) { why = "Không ghi được file tạm."; return false; }
    }
    if (!MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp.c_str()); why = "Không thay được PNG đích."; return false;
    }
    return true;
}

inline bool writeExifWebp(const fs::path& target, const Bytes& blob, std::string& why) {
    if (blob.empty()) return true;
    Bytes data;
    {
        std::ifstream in(target, std::ios::binary);
        if (!in) { why = "Không mở được WebP đích."; return false; }
        data.assign(std::istreambuf_iterator<char>(in), {});
    }
    if (data.size() < 20 || memcmp(data.data(), "RIFF", 4) != 0 || memcmp(data.data() + 8, "WEBP", 4) != 0) {
        why = "Đích không phải WebP hợp lệ.";
        return false;
    }

    std::string firstChunk(reinterpret_cast<char*>(data.data() + 12), 4);
    if (firstChunk == "VP8X") {
        data[20] |= 0x08; // Set EXIF flag
        size_t pos = 12;
        Bytes filtered = {data.begin(), data.begin() + 12};
        while (pos + 8 <= data.size()) {
            uint32_t len = uint32_t(data[pos+4]) | (uint32_t(data[pos+5])<<8) | (uint32_t(data[pos+6])<<16) | (uint32_t(data[pos+7])<<24);
            size_t total = 8 + len + (len & 1);
            if (pos + total > data.size()) break;
            if (memcmp(data.data() + pos, "EXIF", 4) != 0) {
                filtered.insert(filtered.end(), data.begin() + pos, data.begin() + pos + total);
            }
            pos += total;
        }
        data = std::move(filtered);

        Bytes exif = {'E', 'X', 'I', 'F'};
        uint32_t sz = uint32_t(blob.size());
        exif.push_back(uint8_t(sz & 0xFF));
        exif.push_back(uint8_t((sz >> 8) & 0xFF));
        exif.push_back(uint8_t((sz >> 16) & 0xFF));
        exif.push_back(uint8_t((sz >> 24) & 0xFF));
        exif.insert(exif.end(), blob.begin(), blob.end());
        if (blob.size() % 2 != 0) exif.push_back(0);
        data.insert(data.end(), exif.begin(), exif.end());
    } else {
        uint32_t width = 0, height = 0;
        if (firstChunk == "VP8 " && data.size() >= 30) {
            width = ((uint32_t(data[26]) | (uint32_t(data[27]) << 8)) & 0x3FFF);
            height = ((uint32_t(data[28]) | (uint32_t(data[29]) << 8)) & 0x3FFF);
        } else if (firstChunk == "VP8L" && data.size() >= 25) {
            uint32_t bits = uint32_t(data[21]) | (uint32_t(data[22]) << 8) | (uint32_t(data[23]) << 16) | (uint32_t(data[24]) << 24);
            width = (bits & 0x3FFF) + 1;
            height = ((bits >> 14) & 0x3FFF) + 1;
        }
        if (width == 0 || height == 0) { width = 800; height = 600; }

        Bytes vp8x = {'V', 'P', '8', 'X', 10, 0, 0, 0};
        Bytes vp8x_data(10, 0);
        vp8x_data[0] = 0x08; // EXIF flag
        uint32_t w_minus_1 = width > 0 ? width - 1 : 0;
        uint32_t h_minus_1 = height > 0 ? height - 1 : 0;
        vp8x_data[4] = uint8_t(w_minus_1 & 0xFF);
        vp8x_data[5] = uint8_t((w_minus_1 >> 8) & 0xFF);
        vp8x_data[6] = uint8_t((w_minus_1 >> 16) & 0xFF);
        vp8x_data[7] = uint8_t(h_minus_1 & 0xFF);
        vp8x_data[8] = uint8_t((h_minus_1 >> 8) & 0xFF);
        vp8x_data[9] = uint8_t((h_minus_1 >> 16) & 0xFF);
        vp8x.insert(vp8x.end(), vp8x_data.begin(), vp8x_data.end());

        Bytes exif = {'E', 'X', 'I', 'F'};
        uint32_t sz = uint32_t(blob.size());
        exif.push_back(uint8_t(sz & 0xFF));
        exif.push_back(uint8_t((sz >> 8) & 0xFF));
        exif.push_back(uint8_t((sz >> 16) & 0xFF));
        exif.push_back(uint8_t((sz >> 24) & 0xFF));
        exif.insert(exif.end(), blob.begin(), blob.end());
        if (blob.size() % 2 != 0) exif.push_back(0);

        Bytes body;
        body.insert(body.end(), vp8x.begin(), vp8x.end());
        body.insert(body.end(), data.begin() + 12, data.end());
        body.insert(body.end(), exif.begin(), exif.end());

        data = {'R', 'I', 'F', 'F'};
        uint32_t totalRiff = uint32_t(body.size() + 4);
        data.push_back(uint8_t(totalRiff & 0xFF));
        data.push_back(uint8_t((totalRiff >> 8) & 0xFF));
        data.push_back(uint8_t((totalRiff >> 16) & 0xFF));
        data.push_back(uint8_t((totalRiff >> 24) & 0xFF));
        data.insert(data.end(), {'W', 'E', 'B', 'P'});
        data.insert(data.end(), body.begin(), body.end());
    }

    uint32_t finalRiffSize = uint32_t(data.size() - 8);
    data[4] = uint8_t(finalRiffSize & 0xFF);
    data[5] = uint8_t((finalRiffSize >> 8) & 0xFF);
    data[6] = uint8_t((finalRiffSize >> 16) & 0xFF);
    data[7] = uint8_t((finalRiffSize >> 24) & 0xFF);

    fs::path temp = target; temp += L".exif.tmp";
    {
        std::ofstream o(temp, std::ios::binary | std::ios::trunc);
        o.write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
        if (!o) { why = "Không ghi được file tạm."; return false; }
    }
    if (!MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp.c_str()); why = "Không thay được WebP đích."; return false;
    }
    return true;
}

struct Snapshot {Json metadata, probe;Times times;std::string source,captured;bool valid=false;};
inline Snapshot snapshot(const fs::path& source,std::string& error) {
    Snapshot s;s.times=readTimes(source);s.source=fs::absolute(source).u8string();s.valid=s.times.valid && tags(source,s.metadata,error);if(!s.times.valid)error="Không đọc được timestamp nguồn.";
    {Bytes blob;if(s.valid&&image(source)&&readExifBlob(source,blob))s.captured=exifDate(blob);}
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
    (void)targetTags;
    static const std::set<std::string> technical={"encoder","handler_name","vendor_id","duration","bps","number_of_frames","number_of_bytes","creation_time"};
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
            auto embedded=audioOnly?nativeFormatValue(target.value("format",Json::object()).value("tags",Json::object()),"audio_track_"+key):nativeFormatValue(actualTags,key);
            if(embedded.is_null() && !audioOnly){for(auto tag=actualTags.begin();tag!=actualTags.end();++tag)if(lower(tag.key())==key){embedded=tag.value();break;}}
            if(!sameTag(key,it.value(),embedded))errors.push_back("Stream:"+type+":"+std::to_string(ordinal)+":"+it.key());
        }
    }
}
// Carry metadata into the rendered file. Returns false only when a write was attempted and failed;
// `notes` lists anything that could not be preserved (caller shows it as a warning).
//  - Photos: raw EXIF blob is transplanted (FFmpeg alone drops it for JPEG/WebP).
//  - Audio/video: container/stream tags travel via FFmpeg -map_metadata; verified with ffprobe.
inline bool preserve(const Snapshot& source,const fs::path& target,std::vector<std::string>& notes) {
    notes.clear();
    auto ext=lower(target.extension().u8string());
    if(image(target)){
        Bytes blob,already;
        if(!readExifBlob(fs::u8path(source.source),blob))return true;
        readExifBlob(target,already);
        if(!already.empty())return true;
        if(ext==".jpg"||ext==".jpeg"){
            std::string why;Bytes check;
            if(!writeExifJpeg(target,blob,why)){notes.push_back("EXIF: "+why);return false;}
            if(!readExifBlob(target,check)||check!=blob){notes.push_back("EXIF ghi vào JPEG nhưng đọc lại không khớp.");return false;}
            return true;
        }
        if(ext==".png"){
            std::string why;Bytes check;
            if(!writeExifPng(target,blob,why)){notes.push_back("EXIF: "+why);return false;}
            if(!readExifBlob(target,check)){notes.push_back("EXIF ghi vào PNG nhưng đọc lại không khớp.");return false;}
            return true;
        }
        if(ext==".webp"){
            std::string why;Bytes check;
            if(!writeExifWebp(target,blob,why)){notes.push_back("EXIF: "+why);return false;}
            if(!readExifBlob(target,check)){notes.push_back("EXIF ghi vào WebP nhưng đọc lại không khớp.");return false;}
            return true;
        }
        notes.push_back("Định dạng "+ext+" không hỗ trợ mang EXIF theo tiêu chuẩn.");
        return true;
    }
    if(!source.probe.is_null()){
        Json probe;if(!probeFile(target,probe)){notes.push_back("Không kiểm tra được metadata container (cần ffprobe).");return true;}
        Json none=Json::object();
        verifyFormatTags(source.probe,probe,none,none,notes);
        verifyStreamTags(source.probe,probe,none,ext==".mp3",notes);
    }
    return true;
}

/* Ghi đè metadata của exiftool-source (chờ nâng cấp)
    auto errors=std::vector<std::string>();
    auto args=exifTool();if(!source.valid || args.empty())return false;
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
*/
// Prefer capture dates; use last-write time as a marked fallback.
inline std::string albumMonth(const Snapshot& snapshot,bool& fallback) {
    fallback=false;
    std::vector<std::string> dates;
    for(const auto& key:{"SubSecDateTimeOriginal","DateTimeOriginal","DateCreated","CreationDate","CreateDate","MediaCreateDate"}){
        auto v=value(snapshot.metadata,key);if(v.is_string())dates.push_back(v.get<std::string>());
    }
    if(!snapshot.captured.empty())dates.push_back(snapshot.captured);
    auto container=snapshot.probe.value("format",Json::object()).value("tags",Json::object());
    for(const auto& key:{"creation_time","com.apple.quicktime.creationdate"})
        if(container.contains(key)&&container[key].is_string())dates.push_back(normalizedDate(container[key].get<std::string>()));
    for(const auto& s:dates){
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
