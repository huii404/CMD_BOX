#ifndef DEV_CLEANER_H
#define DEV_CLEANER_H

#include "CleanerCore.h"
#include <vector>

// Dọn dẹp cache công cụ lập trình (Python, Node, Go, Rust, Java, IDEs); bảo vệ .git và kho gói
class DevCleaner {
private:
    static std::vector<fs::path> detectDevScanRoots();
    static int cleanDirectoryArtifacts(const fs::path& rootPath,
                                       const std::vector<std::string>& targetDirNames,
                                       const std::vector<std::string>& targetExtensions,
                                       bool dryRun,
                                       CleanStats& stats);
public:
    static CleanStats clean(bool dryRun = false, bool scanProjects = true);
};

#endif // DEV_CLEANER_H
