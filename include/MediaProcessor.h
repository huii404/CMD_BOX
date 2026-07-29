#ifndef MEDIA_PROCESSOR_H
#define MEDIA_PROCESSOR_H

#include <string>
#include <windows.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <limits>
#include <filesystem>
#include <cstdio>
#include <mutex>

class MediaProcessor {
private:
    std::string getFFmpegPath();
    bool runCommand(const std::string& command);
    std::vector<std::string> getPathInput(const std::string& promptText); 
    
    void compressImage(const std::string& inputPath, const std::string& outputPath, int quality);
    void extractAudioCore(const std::string& inputPath, const std::string& outputPath);
    void changeSpeedCore(const std::string& inputPath, const std::string& outputPath, float speedMultiplier);
    int readIntLocal(const std::string& prompt);

public:
    MediaProcessor();
    ~MediaProcessor();

    void processMediaAuto(); 
    void processExtractAudioBatch();
    void processChangeSpeedBatch();
    void processMediaEnhancementAuto();
    void processConvertFormatBatch();  
};

#endif