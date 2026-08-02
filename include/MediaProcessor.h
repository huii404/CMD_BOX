#ifndef MEDIA_PROCESSOR_H
#define MEDIA_PROCESSOR_H

#include "../include/SystemCore.h"
#include <string>
#include <windows.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <limits>
#include <filesystem>
#include <cstdio>
#include <mutex>
#include <iostream> 

class MediaProcessor {
private:
    std::string getFFmpegPath();
    bool runCommand(const std::string& command);
    void compressImage(const std::string& inputPath, const std::string& outputPath, int quality);
    void extractAudioCore(const std::string& inputPath, const std::string& outputPath);
    void changeSpeedCore(const std::string& inputPath, const std::string& outputPath, float speedMultiplier);

public:
    MediaProcessor();
    ~MediaProcessor();

    void processMediaAuto(); 
    void processExtractAudioBatch();
    void processChangeSpeedBatch();
    void processMediaEnhancementAuto();
    void processConvertFormatBatch();
    void normalizeMediaFilenames();
    void hideFileInImage();
    void hideFileInVideo();
    void extractHiddenFromMedia();
};

#endif