#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <map>
#include <cwctype>
#include <cctype>
#include <chrono>

namespace DropV2 {
namespace fs = std::filesystem;
constexpr size_t MaxFiles = 10000, MaxManifest = 8 * 1024 * 1024;
struct Entry { fs::path source; std::string name, hash; uint64_t size = 0; };
inline bool number(const std::string& s, uint64_t& n) {
    if (s.empty() || s.size() > 20 || s.find_first_not_of("0123456789") != std::string::npos) return false;
    try { n = std::stoull(s); return true; } catch (...) { return false; }
}
inline std::string hex(const std::string& s) {
    const char* digits = "0123456789abcdef"; std::string out;
    for (unsigned char c : s) { out += digits[c >> 4]; out += digits[c & 15]; } return out;
}
inline bool unhex(const std::string& s, std::string& out) {
    if (s.size() % 2) return false;
    out.clear();
    auto digit = [](char c) { return c >= '0' && c <= '9' ? c-'0' : c >= 'a' && c <= 'f' ? c-'a'+10 : -1; };
    for (size_t i=0; i<s.size(); i+=2) { int a=digit(s[i]), b=digit(s[i+1]); if (a<0 || b<0) return false; out += char(a*16+b); }
    return true;
}
inline bool validName(const std::string& name) {
    if (name.empty() || name.size() > 4096 || name.front() == '/' || name.back() == '/') return false;
    std::istringstream in(name); std::string part;
    while (std::getline(in, part, '/')) {
        if (part.empty() || part == "." || part == ".." || part.back()=='.' || part.back()==' ') return false;
        for (unsigned char c : part) if (c<32 || c==127 || std::string("\\:*?\"<>|").find(char(c))!=std::string::npos) return false;
        std::string stem=part.substr(0,part.find('.'));
        std::transform(stem.begin(),stem.end(),stem.begin(),[](unsigned char c){return char(std::toupper(c));});
        if (stem=="CON" || stem=="PRN" || stem=="AUX" || stem=="NUL" ||
            (stem.size()==4 && (stem.substr(0,3)=="COM" || stem.substr(0,3)=="LPT") && stem[3]>='1' && stem[3]<='9')) return false;
    }
    return true;
}
inline std::string sha256(const fs::path& path) {
    BCRYPT_ALG_HANDLE alg=nullptr; BCRYPT_HASH_HANDLE hash=nullptr;
    if (BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0) return {};
    DWORD len=0, got=0; std::string result;
    if (BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&len),sizeof(len),&got,0)>=0) {
        std::vector<unsigned char> object(len), buffer(262144); unsigned char digest[32];
        if (BCryptCreateHash(alg,&hash,object.data(),len,nullptr,0,0)>=0) {
            std::ifstream in(path,std::ios::binary); bool ok=bool(in);
            while (ok && in) { in.read(reinterpret_cast<char*>(buffer.data()),buffer.size()); auto n=in.gcount();
                if (n && BCryptHashData(hash,buffer.data(),DWORD(n),0)<0) ok=false; }
            if (ok && !in.bad() && BCryptFinishHash(hash,digest,sizeof(digest),0)>=0)
                result=hex(std::string(reinterpret_cast<char*>(digest),sizeof(digest)));
            BCryptDestroyHash(hash);
        }
    }
    BCryptCloseAlgorithmProvider(alg,0); return result;
}
inline std::string manifest(const std::vector<Entry>& entries) {
    std::ostringstream out; out << "CMDBOX2\n";
    for (const auto& e:entries) out << hex(e.name) << '\t' << e.size << '\t' << e.hash << '\n';
    return out.str();
}
inline bool parseManifest(const std::string& body, std::vector<Entry>& entries) {
    entries.clear(); if (body.size()>MaxManifest) return false;
    std::istringstream in(body); std::string line; if (!std::getline(in,line) || line!="CMDBOX2") return false;
    std::map<std::wstring,bool> names; uint64_t total=0;
    while (std::getline(in,line)) {
        size_t a=line.find('\t'), b=line.find('\t',a==std::string::npos ? 0 : a+1); Entry e;
        if (a==std::string::npos || b==std::string::npos || !unhex(line.substr(0,a),e.name) || !validName(e.name) ||
            !number(line.substr(a+1,b-a-1),e.size) || e.size>uint64_t(INT64_MAX)) return false;
        auto top=e.name.substr(0,e.name.find('/'));
        std::transform(top.begin(),top.end(),top.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(top==".cmd-box-partials") return false;
        e.hash=line.substr(b+1); if (e.hash.size()!=64 || e.hash.find_first_not_of("0123456789abcdef")!=std::string::npos) return false;
        try {
            auto key=fs::u8path(e.name).wstring(); for (auto& c:key) c=std::towlower(c);
            if (!names.emplace(key,true).second) return false;
        } catch (...) { return false; }
        if (e.size>uint64_t(INT64_MAX)-total || entries.size()>=MaxFiles) return false;
        total+=e.size; entries.push_back(e);
    }
    // A file cannot also be an ancestor directory (including case-insensitive aliases).
    for (const auto& item:names) {
        for (size_t pos=item.first.find(L'/'); pos!=std::wstring::npos; pos=item.first.find(L'/',pos+1))
            if (names.count(item.first.substr(0,pos))) return false;
    }
    return !entries.empty();
}
struct Socket {
    SOCKET value=INVALID_SOCKET;
    Socket()=default; explicit Socket(SOCKET v):value(v){}
    Socket(const Socket&)=delete; Socket& operator=(const Socket&)=delete;
    ~Socket(){ if(value!=INVALID_SOCKET) closesocket(value); }
};
inline void timeouts(SOCKET s, DWORD ms=3000) {
    setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&ms),sizeof(ms));
    setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&ms),sizeof(ms));
}
inline bool sendBytes(SOCKET s,const char* data,size_t size) {
    while(size) { int n=send(s,data,int((std::min)(size,size_t(262144))),0); if(n<=0)return false; data+=n;size-=n; } return true;
}
inline bool readHeader(SOCKET s,std::string& header,std::string& pending) {
    header.clear(); pending.clear(); char buf[4096]; auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    while(header.size()<16384 && std::chrono::steady_clock::now()<deadline) {
        int n=recv(s,buf,sizeof(buf),0); if(n<=0)return false;header.append(buf,n);
        size_t end=header.find("\r\n\r\n");if(end!=std::string::npos) {pending=header.substr(end+4);header.resize(end+2);return true;}
    } return false;
}
inline std::string field(const std::string& header,const std::string& key) {
    std::istringstream in(header);std::string line;std::getline(in,line);
    while(std::getline(in,line)) { auto colon=line.find(':');if(colon==std::string::npos)continue;
        auto name=line.substr(0,colon);std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(name==key) {auto value=line.substr(colon+1);while(!value.empty() && (value.front()==' ' || value.front()=='\t'))value.erase(0,1);if(!value.empty() && value.back()=='\r')value.pop_back();return value;}
    }return {};
}
inline void response(SOCKET s,int status,const std::string& body="",const std::string& type="text/plain; charset=UTF-8",bool head=false) {
    std::string h="HTTP/1.1 "+std::to_string(status)+" Response\r\nContent-Type: "+type+"\r\nContent-Length: "+std::to_string(body.size())+"\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n";
    if(sendBytes(s,h.data(),h.size()) && !head)sendBytes(s,body.data(),body.size());
}
// Supported single HTTP byte range, with strict overflow/bounds checks.
inline bool range(const std::string& value,uint64_t size,uint64_t& start,uint64_t& end) {
    start=0;end=size ? size-1 : 0;if(value.empty())return true;
    if (!size || value.rfind("bytes=",0)!=0)return false;
    auto s=value.substr(6);auto dash=s.find('-');if(dash==std::string::npos)return false;
    auto a=s.substr(0,dash),b=s.substr(dash+1);uint64_t n=0;
    if(a.empty()) {if(!number(b,n) || !n)return false;start=size-(std::min)(n,size);return true;}
    if(!number(a,start) || start>=size)return false;
    if(!b.empty()) {if(!number(b,n) || n<start)return false;end=(std::min)(n,size-1);}return true;
}
}

// Hash an already exclusively opened partial file before publishing it.
namespace DropV2 {
inline std::string sha256Handle(HANDLE file) {
    LARGE_INTEGER zero{}; if (!SetFilePointerEx(file,zero,nullptr,FILE_BEGIN)) return {};
    BCRYPT_ALG_HANDLE alg=nullptr; BCRYPT_HASH_HANDLE hash=nullptr;
    if (BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0) return {};
    DWORD length=0,received=0;std::string result;
    if (BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&length),sizeof(length),&received,0)>=0) {
        std::vector<unsigned char> object(length),buffer(262144);unsigned char digest[32];
        if (BCryptCreateHash(alg,&hash,object.data(),length,nullptr,0,0)>=0) {
            bool ok=true;DWORD n=0;
            do {if(!ReadFile(file,buffer.data(),DWORD(buffer.size()),&n,nullptr)){ok=false;break;}
                if(n && BCryptHashData(hash,buffer.data(),n,0)<0){ok=false;break;}}while(n);
            if(ok && BCryptFinishHash(hash,digest,sizeof(digest),0)>=0)result=hex(std::string(reinterpret_cast<char*>(digest),sizeof(digest)));
            BCryptDestroyHash(hash);
        }
    }BCryptCloseAlgorithmProvider(alg,0);return result;
}
inline bool publish(HANDLE file,const fs::path& finalPath) {
    auto name=fs::absolute(finalPath).wstring();size_t bytes=name.size()*sizeof(wchar_t);
    std::vector<unsigned char> storage(sizeof(FILE_RENAME_INFO)+bytes);
    auto info=reinterpret_cast<FILE_RENAME_INFO*>(storage.data());info->ReplaceIfExists=FALSE;info->RootDirectory=nullptr;
    info->FileNameLength=DWORD(bytes);memcpy(info->FileName,name.data(),bytes);
    return SetFileInformationByHandle(file,FileRenameInfo,info,DWORD(storage.size()))!=FALSE;
}
}

namespace DropV2 {
inline std::string sha256Bytes(const std::string& bytes) {
    BCRYPT_ALG_HANDLE alg=nullptr; BCRYPT_HASH_HANDLE hash=nullptr; std::string result;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
    DWORD length=0,got=0;
    if(BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&length),sizeof(length),&got,0)>=0){
        std::vector<unsigned char> object(length);unsigned char digest[32];
        if(BCryptCreateHash(alg,&hash,object.data(),length,nullptr,0,0)>=0){
            if(BCryptHashData(hash,reinterpret_cast<PUCHAR>(const_cast<char*>(bytes.data())),ULONG(bytes.size()),0)>=0 && BCryptFinishHash(hash,digest,sizeof(digest),0)>=0)result=hex(std::string(reinterpret_cast<char*>(digest),sizeof(digest)));
            BCryptDestroyHash(hash);
        }
    }BCryptCloseAlgorithmProvider(alg,0);return result;
}
}
