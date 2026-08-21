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

struct GpuCodecInfo {
    std::string encoder;
    std::string compressParams;
    std::string speedParams;
    std::string enhanceParamsLevel1;
    std::string enhanceParamsLevel2;
    std::string enhanceParamsLevel3;
    std::string displayName;
};

class MediaProcessor {
private:
    std::string getFFmpegPath();
    GpuCodecInfo getGpuEncoder();
    bool runCommand(const std::string& command);
    void compressImage(const std::string& inputPath, const std::string& outputPath, int quality);
    void extractAudioCore(const std::string& inputPath, const std::string& outputPath);
    void changeSpeedCore(const std::string& inputPath, const std::string& outputPath, float speedMultiplier);

    // Core chức năng Ẩn & Trích xuất file trong Media
    bool embedFileIntoContainerCore(const std::string& containerPath, const std::string& hiddenFilePath, const std::string& outputPath, uintmax_t maxContainerSize, std::string& errorMsg);
    bool hideFileInImageCore(const std::string& imagePath, const std::string& hiddenFilePath, const std::string& outputPath, std::string& errorMsg);
    bool hideFileInVideoCore(const std::string& videoPath, const std::string& hiddenFilePath, const std::string& outputPath, std::string& errorMsg);
    bool extractHiddenFromMediaCore(const std::string& containerPath, const std::string& outputPath, std::string& errorMsg);

public:
    MediaProcessor();
    ~MediaProcessor();

    void processMediaAuto(); 
    void processExtractAudioBatch();
    void processChangeSpeedBatch();
    void processMediaEnhancementAuto();
    void processConvertFormatBatch();
    void normalizeMediaFilenames();
    
    // Hàm mẹ: Ẩn file trong file (Menu lồng)
    void processAnFileTrongFile();

    // Các chức năng con
    void hideFileInImage();
    void hideFileInVideo();
    void extractHiddenFromMedia();
};

#endif