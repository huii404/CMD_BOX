#include "SecurityProfiles.h"
#include "third_party/json.hpp"
#include <windows.h>
#include <fstream>
#include <iostream>

namespace {
using Level = ProfileLevel;

struct FeatureRule {
    Feature feature;
    const char* name;
    Level minimumLevel;
    bool allowBeforeSetup = false;
};

// THÊM / GIỚI HẠN TÍNH NĂNG — chỉnh tại bảng này.
// Mỗi dòng gồm: tên trong code, tên hiển thị, cấp thấp nhất được dùng.
// Ví dụ Standard: Standard / Developer / Technician được dùng; Beginner bị chặn.
// Đổi minimumLevel để đổi quyền. Không cần sửa một mảng số riêng nữa.
// true ở cột cuối: chẩn đoán chỉ đọc được phép khi chưa Setup.
constexpr FeatureRule FEATURE_RULES[] = {
    // Dọn dẹp và tối ưu hệ thống.
    {Feature::BasicClean,         "Dọn cơ bản / cache / Downloads", Level::Beginner},
    {Feature::DeepClean,          "Dọn sâu & DISM",                 Level::Standard},
    {Feature::Startup,            "Ứng dụng khởi động",             Level::Standard},
    {Feature::BackgroundServices, "Dịch vụ nền",                    Level::Developer},
    {Feature::Visuals,            "Giao diện & Taskbar",             Level::Standard},
    {Feature::OptimizeAll,        "Tối ưu toàn bộ",                  Level::Developer},
    {Feature::WindowsUpdate,      "Sửa Windows Update",             Level::Developer},
    {Feature::AdvancedServices,   "Quản lý 40 dịch vụ",              Level::Technician},

    // Mạng và bảo mật.
    {Feature::NetworkRepair,      "Sửa mạng",                       Level::Standard},
    {Feature::Firewall,           "Lá chắn Firewall",               Level::Technician},
    {Feature::SecurityStatus,     "Trạng thái an ninh",              Level::Standard, true},
    {Feature::Wifi,               "Mật khẩu Wi-Fi",                 Level::Beginner},
    {Feature::LanScan,            "Quét LAN",                       Level::Beginner},
    {Feature::LocalDrop,          "LocalDrop",                      Level::Beginner},

    // Công cụ và tiện ích.
    {Feature::AutoClick,          "Auto Click",                     Level::Beginner},
    {Feature::SpamText,           "Spam Text",                      Level::Beginner},
    {Feature::AutoPaste,          "Dán nhiều dòng",                 Level::Standard},
    {Feature::DownloadApps,       "Tải ứng dụng",                   Level::Beginner},
    {Feature::Bloatware,          "Gỡ Bloatware",                    Level::Developer},
    {Feature::Battery,            "Chẩn đoán pin",                   Level::Beginner, true},

    // Xử lý media.
    {Feature::Compress,           "Nén media",                      Level::Beginner},
    {Feature::ExtractAudio,       "Tách MP3",                       Level::Beginner},
    {Feature::Speed,              "Tốc độ video",                   Level::Beginner},
    {Feature::Convert,            "Đổi định dạng",                  Level::Beginner},
    {Feature::Rename,             "Chuẩn hóa tên",                  Level::Beginner},
    {Feature::Steganography,      "Steganography",                  Level::Standard},
    {Feature::Album,              "Sắp album",                      Level::Beginner}
};

// Bắt lỗi thiếu/trùng dòng quyền ngay khi biên dịch.
constexpr bool validFeatureRules() {
    constexpr int count = static_cast<int>(Feature::Count);
    if (sizeof(FEATURE_RULES) / sizeof(*FEATURE_RULES) != count) return false;
    bool seen[count]{};
    for (const auto& rule : FEATURE_RULES) {
        int feature = static_cast<int>(rule.feature);
        int minimum = static_cast<int>(rule.minimumLevel);
        if (feature < 0 || feature >= count || seen[feature] || minimum < 1 || minimum > 4) return false;
        seen[feature] = true;
    }
    return true;
}
static_assert(validFeatureRules(), "Every Feature needs one valid permission rule");

const FeatureRule* findRule(Feature feature) {
    for (const auto& rule : FEATURE_RULES) {
        if (rule.feature == feature) return &rule;
    }
    return nullptr;
}
}

SecurityProfiles::SecurityProfiles(std::filesystem::path path)
    : file(std::move(path)) {
    load();
}

const char* SecurityProfiles::tierName(int value) {
    switch (static_cast<ProfileLevel>(value)) {
    case ProfileLevel::Beginner:     return "Beginner";
    case ProfileLevel::Standard:     return "Standard";
    case ProfileLevel::Developer:    return "Developer";
    case ProfileLevel::Technician:   return "Technician";
    default:                        return "Chưa thiết lập";
    }
}

int SecurityProfiles::minimumTier(Feature feature) {
    const auto* rule = findRule(feature);
    // 5 cao hơn mọi hồ sơ hợp lệ: tên tính năng sai luôn bị chặn.
    return rule ? static_cast<int>(rule->minimumLevel) : 5;
}

const char* SecurityProfiles::featureName(Feature feature) {
    const auto* rule = findRule(feature);
    return rule ? rule->name : "Không hợp lệ";
}

bool SecurityProfiles::allowed(Feature feature) const {
    const auto* rule = findRule(feature);
    if (!rule) return false;
    if (tier == static_cast<int>(ProfileLevel::Unconfigured)) {
        return rule->allowBeforeSetup;
    }
    // Quyền hồ sơ độc lập với quyền Administrator của Windows.
    return tier >= static_cast<int>(rule->minimumLevel);
}

void SecurityProfiles::load() {
    tier = 0;
    hidden = true;
    try {
        std::ifstream input(file, std::ios::binary);
        auto data = nlohmann::json::parse(input);
        if (!data.is_object() || !data.contains("version") || !data["version"].is_number_integer() || data["version"] != 1 ||
            !data.contains("tier") || !data["tier"].is_number_integer() || data["tier"] < 1 || data["tier"] > 4 ||
            !data.contains("hide_config") || !data["hide_config"].is_boolean()) return;
        tier = data["tier"].get<int>();
        hidden = data["hide_config"].get<bool>();
        input.close();
        // Khôi phục thuộc tính hiển thị khi file được sao chép hoặc bị đổi bên ngoài.
        // Tôn trọng hide_config=false nếu người dùng chủ động chọn Hiện trong Setup.
        if (!applyVisibility()) {
            std::cerr << "[!] Đã nạp hồ sơ; không đổi được thuộc tính Hidden của cmd_box.json.\n";
        }
    } catch (...) { /* Missing or invalid configuration remains unconfigured. */ }
}
bool SecurityProfiles::save(int newTier, bool hide, std::string& message) {
    message.clear();
    if (newTier < 1 || newTier > 4) { message = "Hồ sơ không hợp lệ."; return false; }
    wchar_t temporary[MAX_PATH]{};
    if (!GetTempFileNameW(file.parent_path().c_str(), L"cbp", 0, temporary)) {
        message = "Không tạo được file tạm cạnh executable."; return false;
    }
    bool written = false;
    try {
        std::ofstream output(std::filesystem::path(temporary), std::ios::binary | std::ios::trunc);
        output << nlohmann::json({{"version",1},{"tier",newTier},{"hide_config",hide}}).dump(2) << '\n';
        output.close(); written = static_cast<bool>(output);
    } catch (...) {}
    if (!written || !MoveFileExW(temporary, file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temporary); message = "Lưu cấu hình thất bại; giữ hồ sơ cũ."; return false;
    }
    tier = newTier; hidden = hide;
    if (!applyVisibility()) {
        message = "Đã lưu hồ sơ; không đổi được thuộc tính Hidden.";
    }
    return true;
}
bool SecurityProfiles::applyVisibility() const {
    DWORD attributes = GetFileAttributesW(file.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) return false;
    // Chỉ đổi Hidden, không thêm System hoặc Read-only.
    DWORD desired = attributes & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NORMAL);
    if (hidden) desired |= FILE_ATTRIBUTE_HIDDEN;
    if (!desired) desired = FILE_ATTRIBUTE_NORMAL;
    if (desired == attributes) return true;
    return SetFileAttributesW(file.c_str(), desired) != 0;
}

bool SecurityProfiles::reset(std::string& message) {
    message.clear();
    if (!DeleteFileW(file.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND) {
        message = "Không xóa được cấu hình; giữ hồ sơ cũ."; return false;
    }
    tier = 0;
    hidden = true; return true;
}
