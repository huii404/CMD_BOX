#ifndef MEDIA_PROCESSOR_H
#define MEDIA_PROCESSOR_H

#include <cstdint>
#include <string>

struct GpuCodecInfo {
    std::string encoder;
    std::string compressParams;
    std::string speedParams;
    std::string displayName;
};

class MediaProcessor {
private:
    std::string getFFmpegPath();
    GpuCodecInfo getGpuEncoder();
    bool extractAudioCore(const std::string& inputPath, const std::string& outputPath);
    bool changeSpeedCore(const std::string& inputPath, const std::string& outputPath, float speedMultiplier);
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
    void processConvertFormatBatch();
    void normalizeMediaFilenames();
    void organizeAlbumFolder();
    void processAnFileTrongFile();
    void hideFileInImage();
    void hideFileInVideo();
    void extractHiddenFromMedia();
};

#endif
