#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct ImageScorePro {
    int origW = 0;
    int origH = 0;
    int procW = 0;
    int procH = 0;
    float megaPixels = 0.0f;
    float bpp = 0.0f;              // Bytes per pixel
    float clarityScore = 0.0f;     // Điểm độ nét (0 - 100)
    float skinPercent = 0.0f;      // Tỷ lệ da người (%)
    int scalePercent = 100;
    std::string detectedType = ""; // "Chân dung", "Phong cảnh", "Ảnh nén mờ", "Ảnh studio"
    
    // Thuộc tính mở rộng cho PRO
    float noiseFloor = 0.0f;       // Ước lượng mức nhiễu nền qua MAD 7x7
    float dynamicRange = 0.0f;     // Dải động sáng (Percentile 1% - 99%)
};

struct EnhanceOptionsPro {
    float amount = 1.50f;
    int radius = 2;
    float threshold = 2.0f;
    float edgeSensitivity = 1.25f;
    float contrast = 1.05f;
    float vibrance = 0.06f;
    int scalePercent = 135;
    float casStrength = 1.00f;
    bool isPortrait = false;
    float skinSmooth = 0.40f;
    float claheBlend = 0.25f;
    float detailBoost = 1.50f;     // Hệ số 3-Scale Guided Filter

    // Tham số nâng cấp PRO
    float nanoDetailBoost = 1.30f; // Cường độ tầng Nano-scale (r=0.5)
    float haloTolerance = 1.15f;   // Hệ số nới lỏng kẹp Local Clamp chống halo
    bool noiseAdaptive = true;     // Ước lượng MAD nhiễu nền để tự chỉnh ngưỡng Cauchy
    float textureBoost = 0.25f;    // Cường độ lớp chất liệu (Texture Layer Synthesis)
    float clarityBoost = 0.20f;    // Cường độ tương phản cục bộ Local Laplacian
    float shadowLift = 0.08f;      // Mức mở chi tiết vùng tối trước khi làm nét
    float highlightPull = 0.06f;   // Mức kéo chi tiết vùng cháy sáng
    float skinProbSigma = 0.85f;   // Độ mềm chuyển tiếp mặt nạ da Gaussian
    bool use16BitPipeline = false; // Xử lý nội bộ 16-bit/kênh nếu có
};

class ImageEnhancerPro {
public:
    static EnhanceOptionsPro getPresetPro(int level);
    static bool isSupportedImage(const std::string& filePath);
    
    static bool enhanceImage(
        const std::string& inputPath,
        const std::string& outputPath,
        int level = 0,
        ImageScorePro* outScore = nullptr);

    static ImageScorePro analyzeImageBufferPro(
        const std::vector<uint8_t>& src,
        int width, int height, int stride,
        uintmax_t fileSize);

private:
    static float lanczos3Kernel(float x);
    static std::vector<uint8_t> lanczos3Resample(
        const std::vector<uint8_t>& src, int srcW, int srcH, int srcStride,
        int dstW, int dstH, int dstStride);

    static std::vector<float> fastBoxFilter(const std::vector<float>& src, int width, int height, int radius);
    static std::vector<float> fastBlur(const std::vector<float>& src, int width, int height, int radius);

    static std::vector<float> applyGuidedFilterSingle(
        const std::vector<float>& p, const std::vector<float>& I,
        int width, int height, int radius, float eps);

    static void applyGuidedFilter3Scale(
        const std::vector<float>& luma,
        std::vector<float>& diffGuided,
        int width, int height,
        const EnhanceOptionsPro& opts);

    static void applyLocalLaplacianToneMapping(
        std::vector<float>& luma, int width, int height,
        float clarityBoost);

    static void applyHighlightShadowRecovery(
        std::vector<float>& luma, int width, int height,
        float shadowLift, float highlightPull);

    static void synthesizeTextureLayer(
        std::vector<float>& luma,
        int width, int height,
        float textureBoost);

    struct OklabPixel {
        float L, a, b;
    };
    struct OkLChPixel {
        float L, C, h;
    };
    static OklabPixel sRGBToOklab(float r, float g, float b);
    static void oklabTosRGB(float L, float a, float b, float& r, float& g, float& bOut);
    static OkLChPixel oklabToOkLCh(const OklabPixel& lab);
    static OklabPixel okLChToOklab(const OkLChPixel& lch);

    static void applyHaloClamp(
        std::vector<float>& sharpLuma,
        const std::vector<float>& origLuma,
        int width, int height,
        float haloTolerance);

    static void processSharpenOklab(
        const std::vector<uint8_t>& src, std::vector<uint8_t>& dst,
        int width, int height, int stride,
        const EnhanceOptionsPro& opts,
        float estimatedNoise);
};
