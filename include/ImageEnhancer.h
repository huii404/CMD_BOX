#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct ImageScore {
    int origW = 0;
    int origH = 0;
    int procW = 0;
    int procH = 0;
    float megaPixels = 0.0f;
    float bpp = 0.0f;              // Bytes per pixel
    float clarityScore = 0.0f;     // Điểm độ nét (0 - 100)
    float skinPercent = 0.0f;      // Tỷ lệ da người (%)
    int scalePercent = 100;
    std::string detectedType = ""; // "Chân dung", "Phong cảnh", "Ảnh nén mờ"
};

struct EnhanceOptions {
    float amount = 1.00f;
    int radius = 2;
    float threshold = 3.0f;
    float edgeSensitivity = 1.1f;
    float contrast = 1.04f;
    float vibrance = 0.05f;
    int scalePercent = 140;
    float casStrength = 0.70f;
    bool isPortrait = false;
    float skinSmooth = 0.40f;
};

class ImageEnhancer {
public:
    static EnhanceOptions getPreset(int level);
    static bool isSupportedImage(const std::string& filePath);
    static bool isWebP(const std::string& filePath);
    static bool enhanceImage(
        const std::string& inputPath, 
        const std::string& outputPath, 
        int level = 0, 
        ImageScore* outScore = nullptr);

    static ImageScore analyzeImageBuffer(
        const std::vector<uint8_t>& src, 
        int width, int height, int stride, 
        uintmax_t fileSize);

private:
    static std::vector<uint8_t> bicubicResample(
        const std::vector<uint8_t>& src, int srcW, int srcH, int srcStride,
        int dstW, int dstH, int dstStride);
    
    static float cubicKernel(float x);
    static float applySmoothSCurve(float val, float contrast);
    static std::vector<float> fastBlurLuma(const std::vector<float>& src, int width, int height, int radius);
    static void processSharpenYCbCr(
        const std::vector<uint8_t>& src, std::vector<uint8_t>& dst,
        int width, int height, int stride, const EnhanceOptions& opts);
};
