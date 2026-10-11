#include "SecurityProfiles.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs = std::filesystem;
void check(bool value, const char* description) { if (!value) throw std::runtime_error(description); }
int main() {
 try {
  wchar_t executable[32768]{}; GetModuleFileNameW(nullptr, executable, 32768);
  fs::path root = fs::path(executable).parent_path();
  fs::path config = root / "test-profile.json";
  DeleteFileW(config.c_str());
  SecurityProfiles p(config); std::string message;
  check(p.currentTier()==0 && p.hideConfig(), "missing config defaults to hidden");
  check(!p.allowed(Feature::BasicClean) && p.allowed(Feature::SecurityStatus) && p.allowed(Feature::Battery), "unconfigured diagnostics");
  // Giá trị mong đợi độc lập với bảng quyền trong production code.
  struct ExpectedPermission { Feature feature; int minimumTier; };
  const ExpectedPermission expected[] = {
    {Feature::BasicClean, 1},
    {Feature::DeepClean, 2},
    {Feature::Startup, 2},
    {Feature::BackgroundServices, 3},
    {Feature::Visuals, 2},
    {Feature::OptimizeAll, 3},
    {Feature::WindowsUpdate, 3},
    {Feature::AdvancedServices, 4},
    {Feature::NetworkRepair, 2},
    {Feature::Firewall, 4},
    {Feature::SecurityStatus, 2},
    {Feature::Wifi, 1},
    {Feature::LanScan, 1},
    {Feature::LocalDrop, 1},
    {Feature::AutoClick, 1},
    {Feature::SpamText, 1},
    {Feature::AutoPaste, 2},
    {Feature::DownloadApps, 1},
    {Feature::Bloatware, 3},
    {Feature::Battery, 1},
    {Feature::Compress, 1},
    {Feature::ExtractAudio, 1},
    {Feature::Speed, 1},
    {Feature::Convert, 1},
    {Feature::Rename, 1},
    {Feature::Steganography, 2},
    {Feature::Album, 1},
  };
  const char* levelNames[] = {"Beginner", "Standard", "Developer", "Technician"};
  for (int tier = 1; tier <= 4; ++tier)
    check(std::string(SecurityProfiles::tierName(tier)) == levelNames[tier-1], "English profile names");
  for (int tier=1; tier<=4; ++tier) {
   check(p.save(tier,false,message), "save tier");
   SecurityProfiles loaded(config); check(loaded.currentTier()==tier, "reload tier");
   for (const auto& permission : expected)
    check(loaded.allowed(permission.feature) == (tier >= permission.minimumTier), "permission matrix");
  }
  check(!p.allowed(static_cast<Feature>(999)), "invalid feature");
  check(p.save(2,true,message), "hide config");
  check((GetFileAttributesW(config.c_str()) & FILE_ATTRIBUTE_HIDDEN)!=0, "Hidden attribute");
  DWORD beforeAttributes = GetFileAttributesW(config.c_str());
  check(SetFileAttributesW(config.c_str(), beforeAttributes & ~FILE_ATTRIBUTE_HIDDEN), "simulate unhidden copy");
  SecurityProfiles copied(config);
  check(copied.currentTier()==2 && copied.hideConfig(), "hidden preference reload");
  check((GetFileAttributesW(config.c_str()) & FILE_ATTRIBUTE_HIDDEN)!=0, "load restores Hidden");
  check(p.save(1,false,message), "downgrade and unhide");
  check((GetFileAttributesW(config.c_str()) & FILE_ATTRIBUTE_HIDDEN)==0, "unhide attribute");
  SecurityProfiles visible(config);
  check(!visible.hideConfig() && (GetFileAttributesW(config.c_str()) & FILE_ATTRIBUTE_HIDDEN)==0, "explicit visible preference survives reload");
  check(!p.save(5,false,message) && p.currentTier()==1, "invalid save preserves tier");
  HANDLE lock=CreateFileW(config.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  check(lock!=INVALID_HANDLE_VALUE,"lock config");
  bool saved=p.save(4,false,message); bool deleted=p.reset(message); CloseHandle(lock);
  check(!saved && !deleted && p.currentTier()==1,"failed write/reset preserve tier");
  check(SecurityProfiles(config).currentTier()==1,"failed write preserves disk");
  check(p.reset(message) && p.currentTier()==0 && p.hideConfig(),"reset restores hidden default");
  const char* invalid[] = {"garbage", "{}", "{\"version\":2,\"tier\":4,\"hide_config\":false}", "{\"version\":1,\"tier\":0,\"hide_config\":false}", "{\"version\":1,\"tier\":5,\"hide_config\":false}", "{\"version\":1,\"tier\":2.0,\"hide_config\":false}", "{\"version\":1,\"tier\":true,\"hide_config\":false}", "{\"version\":1,\"tier\":4,\"hide_config\":0}", "{\"version\":1,\"tier\":18446744073709551615,\"hide_config\":false}"};
  for(auto value:invalid) { std::ofstream(config)<<value; p.load(); check(p.currentTier()==0 && !p.allowed(Feature::Firewall), "invalid JSON fails closed"); }
  check(p.reset(message),"cleanup config");
  SecurityProfiles missing(root/"nonexistent"/"config.json"); check(!missing.save(4,false,message) && missing.currentTier()==0,"unwritable directory");
  for(auto& entry:fs::directory_iterator(root)) check(entry.path().extension()!=L".tmp","temporary file cleanup");
  std::cout << "PASS: 108 role permissions, configuration validation, Hidden, reset, failed writes. No system tasks executed.\n";
  return 0;
 } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
