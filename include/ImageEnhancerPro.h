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
    float bpp = 0.0f;                  // Bytes per pixel (Mức độ nén tệp)
    float clarityScore = 0.0f;         // Điểm độ nét tổng hợp (0 - 100)
    float skinPercent = 0.0f;          // Tỷ lệ vùng da người (%)
    int scalePercent = 100;            // Tỷ lệ nội suy tự động được chọn (%)
    std::string detectedType = "";     // Phân loại ngữ cảnh ảnh
    
    // --- 7 GÓC ĐỘ ĐÁNH GIÁ CHẤT LƯỢNG ẢNH PRO ---
    // Góc độ 1: Độ sắc nét biên đa hướng & Tần số chi tiết
    float edgeSharpness = 0.0f;        // Năng lượng biên đa hướng Tenengrad 4 góc (0 - 100)
    float highFreqEnergy = 0.0f;       // Tỷ lệ năng lượng vi mô tần số cao (0 - 100)
    
    // Góc độ 2: Mức độ mờ / Nhòe
    float blurDegree = 0.0f;           // Mức độ nhòe/mờ (0: cực nét, 100: mờ nặng)
    
    // Góc độ 3: Nhiễu hạt & Tỷ lệ tín hiệu trên nhiễu (SNR)
    float noiseFloor = 0.0f;           // Ước lượng mức nhiễu nền qua MAD trên vùng phẳng
    float snrDb = 0.0f;                // Tỷ lệ tín hiệu trên nhiễu ước tính (dB)
    
    // Góc độ 4: Suy hao nén & Vỡ ô vuông (JPEG Blockiness)
    float compressionBlockiness = 0.0f;// Mức độ vỡ khối JPEG artifact 8x8 (0 - 100)
    
    // Góc độ 5: Dải tương phản động & Cháy sáng/Cháy tối
    float dynamicRange = 0.0f;         // Dải động sáng (Percentile 1% - 99%: 0 - 255)
    float shadowClipPercent = 0.0f;    // Tỷ lệ diện tích bị bết tối/cháy đen (%)
    float highlightClipPercent = 0.0f; // Tỷ lệ diện tích bị cháy sáng/mất chi tiết (%)
    
    // Góc độ 6: Nhận diện da & Màu sắc
    float colorSaturation = 0.0f;      // Mức độ bão hòa màu trung bình (0 - 100)
    
    // Góc độ 7: Độ phong phú kết cấu bề mặt (Texture Complexity) & Nét mảnh
    float textureComplexity = 0.0f;    // Độ phức tạp vân ảnh / Entropy kết cấu (0 - 100)
    float thinFeatureRatio = 0.0f;     // Tỷ lệ chi tiết nét mảnh < 3px (0.0 - 1.0)
    
    // Tóm tắt đánh giá & Chiến lược render
    std::string qualityGrade = "";     // "Tuyệt vời", "Sắc nét tốt", "Hơi mờ / Cần bù nét", "Mờ nặng / Suy giảm"
    std::string renderStrategy = "";   // Mô tả chuỗi render tự động được kích hoạt
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

    // Tham số nâng cấp PRO theo đặc tả tài liệu (PRO V2)
    float nanoDetailBoost = 1.80f; // Cường độ tầng Nano-scale (xung kích r=1, eps=100)
    float haloTolerance = 1.25f;   // Hệ số nới lỏng kẹp Local Clamp chống halo
    bool noiseAdaptive = true;     // Ước lượng MAD nhiễu nền để tự chỉnh ngưỡng Cauchy
    float textureBoost = 0.25f;    // Cường độ lớp chất liệu (Texture Layer Synthesis)
    float clarityBoost = 0.20f;    // Cường độ tương phản cục bộ Local Laplacian
    float shadowLift = 0.08f;      // Mức mở chi tiết vùng tối trước khi làm nét
    float highlightPull = 0.06f;   // Mức kéo chi tiết vùng cháy sáng
    float skinProbSigma = 0.85f;   // Độ mềm chuyển tiếp mặt nạ da Gaussian
    bool use16BitPipeline = false; // Xử lý nội bộ 16-bit/kênh nếu có
    bool thinStrokeGate = true;    // Bật co bán kính + lọc định hướng chống phình nét mảnh
    float strokeAnisotropy = 0.85f;// Mức độ chỉ khuếch đại theo hướng gradient
    bool antiBloat = true;         // Cơ chế ức chế bên Lateral Inhibition chống dính điểm ảnh & bệt viền
};

class ImageEnhancerPro {
public:
    static EnhanceOptionsPro getPresetPro(int level);
    static EnhanceOptionsPro computeAdaptiveOptions(const ImageScorePro& score);
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

public:
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

    static void applyCLAHE(
        std::vector<float>& luma,
        int width, int height,
        float clipLimit, float blendFactor);

    static void processSharpenPro(
        const std::vector<uint8_t>& src, std::vector<uint8_t>& dst,
        int width, int height, int stride,
        const EnhanceOptionsPro& opts,
        float estimatedNoise);
};
