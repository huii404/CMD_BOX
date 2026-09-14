#ifndef TASKLOG_H
#define TASKLOG_H

#include <filesystem>
#include <string>

class TaskLog {
public:
    static std::filesystem::path path();
    static void write(const std::string& action, const std::string& result,
                      const std::string& detail = "", const std::string& outputPath = "") noexcept;
    static void showRecent();
};

#endif
