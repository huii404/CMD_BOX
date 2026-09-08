#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

// Kết quả phân tích ban đầu (Dùng cho Chặng 2 & Chặng 3)
struct ProImageAnalysis {
    std::string filePath;
    std::string filename;
    int origW = 0;
    int origH = 0;
    int targetW = 0;
    int targetH = 0;
    float megaPixels = 0.0f;
    float origLaplacianVar = 0.0f; // Điểm đo độ nét Laplace gốc
    float skinPercent = 0.0f;      // Tỷ lệ da người (%)
    float noiseSigma = 0.0f;       // Ước lượng mức nhiễu nền
    std::string detectedType;       // "Chân dung", "Phong cảnh", "Nén mờ/Cũ"

    // Tham số AI tự thích ứng (Kê đơn Chặng 3)
    int scalePercent = 140;        // Hệ số phóng đại (%)
    float neuralBoost = 1.65f;     // Hệ số khuếch đại vi cấu trúc
    float denoiseStrength = 0.15f; // Mức khử nhiễu
    float gamutRetain = 1.0f;      // Bảo toàn dải màu (Gamut Roll-off)
    uintmax_t oldSizeBytes = 0;
};

// Kết quả báo cáo sau khi hoàn thành render (Dùng cho Chặng 5)
struct ProQualityReport {
    int index = 0;
    std::string filename;
    std::string detectedType;
    uintmax_t oldSizeBytes = 0;
    uintmax_t newSizeBytes = 0;
    float origLaplacianVar = 0.0f;     // Phương sai Laplace gốc S_orig
    float procLaplacianVar = 0.0f;     // Phương sai Laplace sau phục chế S_sharp
    float sharpnessGainPercent = 0.0f; // Tỷ lệ vượt trội thực tế: ((proc - orig) / orig) * 100%
    float elapsedSec = 0.0f;           // Thời gian render (dùng hiển thị ở chặng 4)
    bool success = false;
};

class ImageEnhancerPro {
public:
    // Kiểm tra định dạng ảnh hỗ trợ
    static bool isSupportedImage(const std::string& filePath);

    // Chặng 2: Quét ma trận điểm ảnh, tính phương sai Laplace gốc và phân loại thích ứng
    static bool analyzeImagePro(const std::string& inputPath, ProImageAnalysis& outAnalysis);

    // Tính phương sai toán tử vi sai Laplace (Laplacian Variance Focus Measure)
    // S = Var(∇² I) - Cơ sở toán học minh chứng cho độ nét thật
    static float calculateLaplacianVariance(const std::vector<float>& luma, int width, int height);

    // Chặng 4: Pipeline render PRO - Phân rã cấu trúc & tăng cường đa tầng
    static bool enhanceImagePro(
        const ProImageAnalysis& analysis,
        const std::string& outputPath,
        ProQualityReport& outReport,
        std::function<void(float percent, float elapsedSec)> progressCallback = nullptr
    );

private:
    static std::vector<uint8_t> lanczos3Resample(
        const std::vector<uint8_t>& src, int srcW, int srcH, int srcStride,
        int dstW, int dstH, int dstStride);
    static float lanczos3Kernel(float x);
    static std::vector<float> fastBoxFilter(const std::vector<float>& src, int width, int height, int radius);
    static std::vector<float> applyGuidedFilter(
        const std::vector<float>& p, const std::vector<float>& I,
        int width, int height, int radius, float eps);
    static void applyCLAHE(
        std::vector<float>& luma, int width, int height,
        float clipLimit, float blendFactor);
};
