#include "MediaProcessor.h"
#include "MediaMetadata.h"
#include "SystemCore.h"
#include <set>
#include <algorithm>
#include <iostream>
namespace fs=std::filesystem;

namespace MediaAlbum {
struct Item {fs::path source,relative;MediaMetadata::Snapshot snapshot;bool fallback=false;};
inline bool collect(const fs::path& source,std::vector<Item>& items,std::string& error) {
    items.clear();
    try {
        auto root=fs::absolute(source).lexically_normal();
        if(!fs::is_directory(root) || (GetFileAttributesW(root.c_str())&FILE_ATTRIBUTE_REPARSE_POINT)){error="Nguồn phải là thư mục thường.";return false;}
        const std::set<std::string> media={".jpg",".jpeg",".png",".webp",".tif",".tiff",".bmp",".heic",".heif",".dng",".avif",".mp4",".mov",".mkv",".avi",".webm",".mp3",".flac",".wav",".m4a"};
        for(fs::recursive_directory_iterator it(root),end;it!=end;++it){
            auto path=it->path();DWORD attr=GetFileAttributesW(path.c_str());
            if(attr==INVALID_FILE_ATTRIBUTES || (attr&FILE_ATTRIBUTE_REPARSE_POINT)){if(it->is_directory())it.disable_recursion_pending();continue;}
            if(it->is_directory()){
                auto name=path.filename().u8string();if(name=="CMD_BOX_Album" || name=="CMD_BOX_Output" || name.rfind("cmd-box-",0)==0)it.disable_recursion_pending();continue;
            }
            if(!it->is_regular_file())continue;
            auto ext=path.extension().u8string();std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return char(std::tolower(c));});if(!media.count(ext))continue;
            if(items.size()>=10000){error="Quá 10000 file; chọn thư mục nhỏ hơn.";return false;}
            Item item;item.source=path;item.snapshot=MediaMetadata::snapshot(path,error);if(!item.snapshot.valid)return false;
            auto month=MediaMetadata::albumMonth(item.snapshot,item.fallback);
            // Keep source subfolders to avoid name collisions.
            item.relative=fs::u8path(month)/path.lexically_relative(root);items.push_back(std::move(item));
            std::cout<<"\r Đã kiểm tra "<<items.size()<<" file..."<<std::flush;
        }return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
inline bool copy(const Item& item,const fs::path& root,std::string& error) {
    auto output=root/item.relative;std::error_code ec;
    // Lock ancestors before creating directories.
    std::vector<fs::path> dirs;for(auto p=output.parent_path();!p.empty() && p!=p.root_path();p=p.parent_path())dirs.push_back(p);
    std::reverse(dirs.begin(),dirs.end());
    for(const auto& dir:dirs){FileSafety::AncestorLocks parents;if(!parents.acquire(dir)){error="Thư mục chứa junction/symlink.";return false;}
        if(!CreateDirectoryW(dir.c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS){error="Không tạo được thư mục album.";return false;}
        DWORD attr=GetFileAttributesW(dir.c_str());if(attr==INVALID_FILE_ATTRIBUTES || !(attr&FILE_ATTRIBUTE_DIRECTORY) || (attr&FILE_ATTRIBUTE_REPARSE_POINT)){error="Thư mục album không an toàn.";return false;}}
    if(fs::exists(output,ec)){error="Đã có file cùng tên; bỏ qua, không ghi đè.";return false;}
    FileSafety::AncestorLocks parents;if(!parents.acquire(output))return false;
    std::ifstream in(item.source,std::ios::binary);FileSafety::ExclusiveOutput out(output);auto size=fs::file_size(item.source,ec);
    if(ec || !in || !out || !FileSafety::copyExact(in,out,size,false) || !out.commit()){error="Không sao chép được file.";return false;}
    auto record=MediaMetadata::record(item.snapshot,output,item.snapshot.metadata,{});record["albumDateFallback"]=item.fallback;
    if(!MediaMetadata::writeJson(fs::path(output.wstring()+L".metadata.json"),record)){error="File đã sao chép nhưng không lưu được metadata JSON.";return false;}
    if(!MediaMetadata::setTimes(item.snapshot.times,output)){error="File đã sao chép nhưng không đồng bộ được timestamp.";return false;}return true;
}
}

void MediaProcessor::organizeAlbumFolder() {
    std::cout<<"\n== SẮP ALBUM THEO NĂM / THÁNG ==\nChọn THƯ MỤC nguồn (quét cả thư mục con; 0: Hủy): ";
    std::string raw;std::getline(std::cin,raw);raw=SystemCore::trim(raw);if(raw.empty() || raw=="0")return;
    if(raw.size()>1 && raw.front()=='\"' && raw.back()=='\"')raw=raw.substr(1,raw.size()-2);
    auto source=fs::absolute(fs::u8path(raw)).lexically_normal();std::vector<MediaAlbum::Item> items;std::string error;
    if(!MediaAlbum::collect(source,items,error)){std::cout<<"\nLỗi: "<<error<<'\n';SystemCore::waitEnter();return;}
    auto output=source/L"CMD_BOX_Album";size_t fallback=0;
    std::cout<<"\nBản sao sẽ lưu tại: "<<output.u8string()<<"\n";
    for(const auto& item:items){std::cout<<"  "<<item.source.lexically_relative(source).u8string()<<" -> "<<item.relative.u8string()<<(item.fallback?" [dùng ngày sửa file vì thiếu ngày chụp/quay]":"")<<'\n';fallback+=item.fallback;}
    if(items.empty()){std::cout<<"Không tìm thấy file media.\n";return;}
    std::cout<<"Ngày chụp/quay được ưu tiên; "<<fallback<<" file dùng ngày sửa. File gốc giữ nguyên.\n";
    if(!SystemCore::confirm("Sao chép theo danh sách trên? [y/N]: "))return;
    size_t count=0;for(const auto& item:items){if(MediaAlbum::copy(item,output,error))++count;else std::cout<<item.source.filename().u8string()<<": "<<error<<'\n';}
    std::cout<<"Đã sao chép "<<count<<'/'<<items.size()<<" file.\n";SystemCore::waitEnter();
}
