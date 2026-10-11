#pragma once
#include <filesystem>
#include <string>

// Cấp độ tăng dần: cấp cao dùng được mọi tính năng của cấp thấp.
// Giá trị 1–4 được lưu trong JSON; không đổi số khi đổi tên hiển thị.
enum class ProfileLevel {
    Unconfigured = 0,
    Beginner = 1,
    Standard = 2,
    Developer = 3,
    Technician = 4
};

// THÊM TÍNH NĂNG — bước 1: thêm một tên ở nhóm phù hợp, trước Count.
// Bước 2: thêm dòng quyền tương ứng trong FEATURE_RULES (SecurityProfiles.cpp).
// Bước 3: gọi SystemCore::requireFeature(...) trước khi chạy backend,
//         rồi cập nhật tài liệu/bộ kiểm thử quyền.
enum class Feature {
    // Dọn dẹp và tối ưu hệ thống.
    BasicClean,
    DeepClean,
    Startup,
    BackgroundServices,
    Visuals,
    OptimizeAll,
    WindowsUpdate,
    AdvancedServices,

    // Mạng và bảo mật.
    NetworkRepair,
    Firewall,
    SecurityStatus,
    Wifi,
    LanScan,
    LocalDrop,

    // Công cụ và tiện ích.
    AutoClick,
    SpamText,
    AutoPaste,
    DownloadApps,
    Bloatware,
    Battery,

    // Xử lý media.
    Compress,
    ExtractAudio,
    Speed,
    Convert,
    Rename,
    Steganography,
    Album,

    Count // Số lượng tính năng; không phải tính năng có thể chạy.
};

// Independent configuration object also used by the isolated test harness.
class SecurityProfiles {
    std::filesystem::path file;
    int tier = static_cast<int>(ProfileLevel::Unconfigured);
    // Cấu hình mới mặc định ẩn để giảm nguy cơ xóa nhầm trong Explorer.
    bool hidden = true;
    bool applyVisibility() const;
public:
    explicit SecurityProfiles(std::filesystem::path path);
    void load();
    bool save(int newTier, bool hide, std::string& message);
    bool reset(std::string& message);
    bool allowed(Feature feature) const;
    int currentTier() const { return tier; }
    bool hideConfig() const { return hidden; }
    const std::filesystem::path& path() const { return file; }
    static int minimumTier(Feature feature);
    static const char* featureName(Feature feature);
    static const char* tierName(int tier);
};
