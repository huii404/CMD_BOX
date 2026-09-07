#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <string>

struct ReleaseInfo {
    std::string releaseName;
    std::string version;
    std::string htmlUrl;
    std::string downloadUrl;
    std::string publishedAt;
    std::string body;
    bool valid = false;
};

class UpdateManager {
public:
    static const std::string CURRENT_VERSION;
    static const std::string GITHUB_REPO;
    static const std::string API_RELEASES_URL;

    // Kiểm tra ngầm trong nền (background thread) khi khởi động
    static void checkUpdateAsync();

    // Trạng thái kiểm tra
    static bool isCheckingFinished();
    static bool hasNewVersion();
    static std::string getRemoteVersion();
    static std::string getVersionStatusText();

    // Giao diện menu kiểm tra & cập nhật
    static void showUpdateMenu();

    // Truy vấn GitHub Releases API trực tiếp
    static ReleaseInfo fetchLatestRelease();

    // So sánh phiên bản Semantic Versioning (x.y.z)
    static bool isNewer(const std::string& currentVer, const std::string& remoteVer);
};

#endif
