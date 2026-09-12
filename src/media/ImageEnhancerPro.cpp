#include "ImageEnhancerPro.h"
#include <windows.h>
#include <wincodec.h>
#include <wincodecsdk.h>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace fs = std::filesystem;

static std::wstring toWideString(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), (int)str.size(), NULL, 0);
    if (sizeNeeded <= 0) {
        sizeNeeded = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), NULL, 0);
        if (sizeNeeded <= 0) return L"";
        std::wstring wstr(sizeNeeded, 0);
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);
        return wstr;
    }
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);
    return wstr;
}

void EnhanceOptionsPro::sanitize() {
    amount = std::clamp(amount, 0.0f, 3.5f);
    radius = std::clamp(radius, 1, 10);
    edgeSensitivity = std::clamp(edgeSensitivity, 0.1f, 3.0f);
    contrast = std::clamp(contrast, 0.5f, 2.0f);
    vibrance = std::clamp(vibrance, -1.0f, 1.0f);
    scalePercent = std::clamp(scalePercent, 50, 400);
    casStrength = std::clamp(casStrength, 0.0f, 2.0f);
    skinSmooth = std::clamp(skinSmooth, 0.0f, 1.0f);
    skinPorePreserve = std::clamp(skinPorePreserve, 0.0f, 1.0f);
    compressionBlockiness = std::clamp(compressionBlockiness, 0.0f, 100.0f);
    claheBlend = std::clamp(claheBlend, 0.0f, 1.0f);
    detailBoost = std::clamp(detailBoost, 0.0f, 3.5f);
    nanoDetailBoost = std::clamp(nanoDetailBoost, 0.0f, 3.5f);
    haloTolerance = std::clamp(haloTolerance, 0.5f, 3.0f);
    textureBoost = std::clamp(textureBoost, 0.0f, 2.0f);
    clarityBoost = std::clamp(clarityBoost, 0.0f, 2.0f);
    shadowLift = std::clamp(shadowLift, 0.0f, 0.5f);
    highlightPull = std::clamp(highlightPull, 0.0f, 0.5f);
    strokeAnisotropy = std::clamp(strokeAnisotropy, 0.0f, 1.0f);
}

EnhanceOptionsPro ImageEnhancerPro::getPresetPro(int level) {
    EnhanceOptionsPro opt;
    switch (level) {
        case 1: // Base Portrait
            opt.amount = 1.10f;
            opt.radius = 2;
            opt.edgeSensitivity = 1.15f;
            opt.contrast = 1.03f;
            opt.vibrance = 0.05f;
            opt.scalePercent = 125;
            opt.casStrength = 0.75f;
            opt.isPortrait = true;
            opt.skinSmooth = 0.45f;
            opt.skinPorePreserve = 0.85f;
            opt.crestLimiter = true;
            opt.claheBlend = 0.15f;
            opt.detailBoost = 1.35f;
            opt.nanoDetailBoost = 1.20f;
            opt.textureBoost = 0.15f;
            opt.clarityBoost = 0.10f;
            opt.haloTolerance = 1.20f;
            break;

        case 2: // Base Landscape
            opt.amount = 1.50f;
            opt.radius = 2;
            opt.edgeSensitivity = 1.35f;
            opt.contrast = 1.06f;
            opt.vibrance = 0.08f;
            opt.scalePercent = 135;
            opt.casStrength = 1.15f;
            opt.isPortrait = false;
            opt.claheBlend = 0.25f;
            opt.detailBoost = 1.55f;
            opt.nanoDetailBoost = 1.80f;
            opt.textureBoost = 0.25f;
            opt.clarityBoost = 0.20f;
            opt.haloTolerance = 1.25f;
            opt.antiBloat = true;
            break;

        case 3: // Base Ultra
            opt.amount = 1.95f;
            opt.radius = 2;
            opt.edgeSensitivity = 1.55f;
            opt.contrast = 1.08f;
            opt.vibrance = 0.10f;
            opt.scalePercent = 150;
            opt.casStrength = 1.35f;
            opt.isPortrait = false;
            opt.claheBlend = 0.35f;
            opt.detailBoost = 1.70f;
            opt.nanoDetailBoost = 1.85f;
            opt.textureBoost = 0.30f;
            opt.clarityBoost = 0.25f;
            opt.haloTolerance = 1.25f;
            opt.antiBloat = true;
            break;

        case 4: // Level 4: PRO Ultra HD
            opt.scalePercent = 140;
            opt.amount = 1.70f;
            opt.detailBoost = 1.75f;
            opt.nanoDetailBoost = 1.80f;
            opt.textureBoost = 0.35f;
            opt.clarityBoost = 0.30f;
            opt.noiseAdaptive = true;
            opt.haloTolerance = 1.25f;
            opt.isPortrait = false;
            opt.skinSmooth = 0.00f;
            opt.shadowLift = 0.10f;
            opt.highlightPull = 0.08f;
            opt.enableDither = true;
            opt.contrast = 1.06f;
            opt.vibrance = 0.08f;
            opt.antiBloat = true;
            break;

        case 5: // Level 5: PRO Studio Portrait
            opt.scalePercent = 120;
            opt.amount = 1.20f;
            opt.casStrength = 0.80f;     // CAS êm dịu, triệt tiêu răng cưa
            opt.detailBoost = 1.40f;
            opt.nanoDetailBoost = 1.40f;
            opt.textureBoost = 0.20f;
            opt.clarityBoost = 0.10f;
            opt.noiseAdaptive = true;
            opt.haloTolerance = 1.20f;
            opt.isPortrait = true;
            opt.skinSmooth = 0.42f;
            opt.skinPorePreserve = 0.88f;
            opt.crestLimiter = true;
            opt.shadowLift = 0.06f;
            opt.highlightPull = 0.05f;
            opt.enableDither = true;
            opt.contrast = 1.03f;
            opt.vibrance = 0.05f;
            opt.antiBloat = true;
            break;

        case 0:
        default:
            // Auto Adaptive default (PRO V2)
            opt.amount = 1.40f;
            opt.scalePercent = 130;
            opt.detailBoost = 1.50f;
            opt.nanoDetailBoost = 1.80f;
            opt.textureBoost = 0.25f;
            opt.clarityBoost = 0.18f;
            opt.noiseAdaptive = true;
            opt.haloTolerance = 1.25f;
            opt.antiBloat = true;
            break;
    }
    return opt;
}

EnhanceOptionsPro ImageEnhancerPro::computeAdaptiveOptions(const ImageScorePro& score) {
    EnhanceOptionsPro opt;

    // Chuẩn hoá các chỉ số chất lượng độc lập về dải 0.0 - 1.0 (Mục V.1)
    float clarityScoreNorm = std::clamp(score.clarityScore / 100.0f, 0.0f, 1.0f);
    float noiseScore = std::clamp(1.0f - score.noiseFloor / 25.0f, 0.0f, 1.0f);
    float dynamicRangeScore = std::clamp(score.dynamicRange / 220.0f, 0.0f, 1.0f);
    float textureEnergyScore = std::clamp(score.textureComplexity / 100.0f, 0.0f, 1.0f);
    float thinFeatureRatio = std::clamp(score.thinFeatureRatio, 0.0f, 1.0f);
    float shadowClipRatio = std::clamp(score.shadowClipPercent / 100.0f, 0.0f, 1.0f);
    float highlightClipRatio = std::clamp(score.highlightClipPercent / 100.0f, 0.0f, 1.0f);
    float skinPercent = std::clamp(score.skinPercent / 100.0f, 0.0f, 1.0f);

    // Mục V.2: Hàm bù điểm liên tục (Compensation Function)
    // 1. Hệ số suy giảm do nhiễu (noiseAtt)
    float noiseAtt = std::clamp(0.65f + 0.35f * noiseScore, 0.65f, 1.00f);

    // 2. Cường độ làm nét (amount): ảnh càng mờ càng bù mạnh, ảnh nét vẫn giữ lực nét tối thiểu 1.20f
    opt.amount = std::clamp(1.20f + 0.65f * std::pow(1.0f - clarityScoreNorm, 1.10f), 1.20f, 1.85f) * noiseAtt;

    // 3. Multi-Scale Detail Boost (detailBoost)
    opt.detailBoost = std::clamp(1.30f + 0.45f * (1.0f - clarityScoreNorm), 1.30f, 1.75f);
    opt.nanoDetailBoost = std::clamp(1.20f + 0.40f * (1.0f - clarityScoreNorm), 1.20f, 1.65f);

    // 4. Local Laplacian Tone Mapping (clarityBoost)
    opt.clarityBoost = std::clamp(0.10f + 0.35f * (1.0f - dynamicRangeScore), 0.10f, 0.45f);

    // 5. Texture Layer Synthesis (textureBoost)
    opt.textureBoost = std::clamp(0.08f + 0.35f * (1.0f - textureEnergyScore), 0.08f, 0.45f) * noiseAtt;

    // 6. Highlight/Shadow Local Recovery
    opt.shadowLift = std::clamp(0.04f + 0.12f * shadowClipRatio, 0.04f, 0.16f);
    opt.highlightPull = std::clamp(0.03f + 0.10f * highlightClipRatio, 0.03f, 0.14f);

    // 7. Cường độ chống phình nét mảnh (strokeAnisotropy) & Thin-Stroke Gating
    opt.thinStrokeGate = true;
    opt.strokeAnisotropy = std::clamp(0.70f + 0.30f * thinFeatureRatio, 0.70f, 1.00f);
    opt.antiBloat = true;

    // 8. Chống quầng sáng Halo Suppression (haloTolerance)
    opt.haloTolerance = std::clamp(1.10f + 0.20f * clarityScoreNorm, 1.10f, 1.30f);

    // 9. Hòa trộn bảo vệ chân dung liên tục (portraitBlend)
    // Kích hoạt chế độ Portrait khi mật độ da đáng kể (>= 12%)
    float portraitBlend = std::clamp((skinPercent - 0.10f) / 0.18f, 0.0f, 1.0f);
    opt.isPortrait = (portraitBlend > 0.05f);
    opt.skinSmooth = 0.50f * portraitBlend;
    opt.skinPorePreserve = 0.85f;
    opt.crestLimiter = true;
    opt.compressionBlockiness = score.compressionBlockiness;

    // 10. Phân loại ngữ cảnh Document / Text vs Portrait Studio vs Environmental Portrait vs Defocus vs Landscape
    bool isDoc = (score.detectedType == "Tài liệu / Văn bản (Document / Text)");
    bool isDefocus = (score.detectedType == "Ảnh mờ / Cần phục hồi nét (Blur/Defocus)");
    bool isStudioPortrait = (score.detectedType == "Chân dung cận cảnh (Portrait Studio)");
    bool isEnvPortrait = (score.detectedType == "Người + Phong cảnh (Environmental Portrait)");

    if (isDoc) {
        opt.isDocument = true;
        opt.isPortrait = false;
        opt.skinSmooth = 0.0f;
        opt.textureBoost = 0.0f; // Triệt tiêu bơm hạt vào nền giấy / biểu đồ
        opt.claheBlend = 0.16f;  // Tách nét chữ vừa vặn, không sinh hạt nền
        opt.contrast = 1.03f;
        opt.amount = 1.60f;      // Nét chữ sắc sảo
        opt.detailBoost = 1.55f;
        opt.nanoDetailBoost = 1.30f;
        opt.haloTolerance = 1.05f; // Khóa chặt quầng sáng quanh chữ
        opt.casStrength = 1.20f;
        opt.edgeSensitivity = 1.30f;
        opt.vibrance = 0.02f;
        opt.antiBloat = true;
        opt.thinStrokeGate = true;
        opt.strokeAnisotropy = 0.90f;
    } else if (isDefocus) {
        opt.isPortrait = false;
        opt.amount = 1.75f;
        opt.detailBoost = 1.65f;
        opt.nanoDetailBoost = 1.60f;
        opt.textureBoost = 0.18f;
        opt.clarityBoost = 0.22f;
        opt.casStrength = 1.20f;
        opt.edgeSensitivity = 1.35f;
        opt.haloTolerance = 1.20f;
        opt.contrast = 1.04f;
        opt.shadowLift = 0.08f;
        opt.highlightPull = 0.06f;
        opt.antiBloat = true;
    } else if (isStudioPortrait) {
        // Chân dung Studio cận cảnh: làm nét êm ái, bảo vệ da mịn màng, triệt tiêu 100% noi và răng cưa
        opt.isPortrait = true;
        opt.claheBlend = 0.06f;
        opt.textureBoost = 0.02f;    // Tuyệt đối không bơm hạt lên mặt
        opt.clarityBoost = 0.06f;    // Khống chế micro-contrast không làm lộ lỗ chân lông
        opt.nanoDetailBoost = 1.05f; // Hạ nano boost trên chân dung để tránh noi
        opt.amount = 1.28f;          // Nét tự nhiên, tinh tế, vừa vặn
        opt.casStrength = 0.85f;     // CAS êm dịu, triệt tiêu răng cưa
        opt.haloTolerance = 1.15f;
        opt.edgeSensitivity = 1.20f;
        opt.contrast = 1.02f;        // Tương phản dịu dàng tôn vinh làn da
        opt.vibrance = 0.035f;       // Màu da hồng hào tự nhiên, không rực cam
        opt.antiBloat = true;
        opt.thinStrokeGate = true;
        opt.strokeAnisotropy = 0.70f;
        opt.skinSmooth = 0.45f;
        opt.skinPorePreserve = 0.88f; // Giữ 88% vi cấu trúc lỗ chân lông tự nhiên
        opt.crestLimiter = true;
    } else if (isEnvPortrait) {
        // Người + Phong cảnh: Mặt/da được bảo vệ mịn tự nhiên, hậu cảnh phong cảnh vẫn giữ lực nét cao
        opt.isPortrait = true;
        opt.claheBlend = 0.10f;
        opt.textureBoost = 0.10f;
        opt.clarityBoost = 0.12f;
        opt.nanoDetailBoost = 1.40f;
        opt.amount = 1.45f;
        opt.detailBoost = 1.50f;
        opt.casStrength = 1.00f;
        opt.haloTolerance = 1.20f;
        opt.edgeSensitivity = 1.25f;
        opt.contrast = 1.035f;
        opt.vibrance = 0.045f;
        opt.antiBloat = true;
        opt.thinStrokeGate = true;
        opt.strokeAnisotropy = 0.75f;
        opt.skinSmooth = 0.35f;
        opt.skinPorePreserve = 0.82f; // Giữ 82% vi cấu trúc lỗ chân lông môi trường
        opt.crestLimiter = true;
    } else {
        // Phong cảnh / Chi tiết cao
        opt.isPortrait = false;
        opt.claheBlend = std::clamp(0.10f + 0.15f * (1.0f - dynamicRangeScore), 0.08f, 0.25f);
        opt.nanoDetailBoost = 1.65f; // Xung kích tầng Nano Acutance cho phong cảnh
        opt.haloTolerance = 1.20f;
        opt.casStrength = 1.05f;
        opt.edgeSensitivity = 1.25f;
        opt.contrast = 1.04f;
        opt.vibrance = 0.05f;
        opt.antiBloat = true;
        opt.thinStrokeGate = false;
        opt.strokeAnisotropy = 0.50f;
    }

    // 11. Tự động tính toán tỷ lệ nội suy Lanczos-3 (scalePercent)
    if (score.megaPixels < 0.60f) {
        opt.scalePercent = 150; // Ảnh nhỏ: bù tối đa 150%
    } else if (score.megaPixels < 1.80f) {
        opt.scalePercent = 130; // Ảnh vừa: bù 130%
    } else if (score.megaPixels < 4.00f) {
        opt.scalePercent = 115; // Ảnh lớn: bù nhẹ 115%
    } else {
        opt.scalePercent = 100; // Ảnh độ phân giải cao giữ nguyên tỷ lệ 100%
    }

    // Giảm nội suy nếu nhiễu nền quá cao để tránh phóng đại noise grain
    if (noiseScore < 0.45f) {
        opt.scalePercent = std::max(100, opt.scalePercent - 15);
    }

    opt.noiseAdaptive = true;
    opt.enableDither = (score.dynamicRange > 180.0f);

    return opt;
}

bool ImageEnhancerPro::isSupportedImage(const std::string& filePath) {
    std::string ext = fs::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" ||
            ext == ".tif" || ext == ".tiff" || ext == ".webp" || ext == ".heic" || ext == ".dng");
}

float ImageEnhancerPro::lanczos3Kernel(float x) {
    x = std::abs(x);
    if (x < 1e-6f) return 1.0f;
    if (x >= 3.0f) return 0.0f;
    const float PI = 3.14159265358979323846f;
    float pix = PI * x;
    return (std::sin(pix) / pix) * (std::sin(pix / 3.0f) / (pix / 3.0f));
}

std::vector<uint8_t> ImageEnhancerPro::lanczos3Resample(
    const std::vector<uint8_t>& src, int srcW, int srcH, int srcStride,
    int dstW, int dstH, int dstStride)
{
    std::vector<uint8_t> dst(dstH * dstStride);
    float scaleX = (float)srcW / dstW;
    float scaleY = (float)srcH / dstH;

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < dstH; ++y) {
        float srcY = (y + 0.5f) * scaleY - 0.5f;
        int y0 = (int)std::floor(srcY);
        float dy = srcY - y0;

        float kY[6];
        int idxY[6];
        float sumKY = 0.0f;
        for (int i = -2; i <= 3; ++i) {
            int curY = y0 + i;
            idxY[i + 2] = std::clamp(curY, 0, srcH - 1);
            float w = lanczos3Kernel(dy - i);
            kY[i + 2] = w;
            sumKY += w;
        }
        float invSumKY = (std::abs(sumKY) > 1e-6f) ? (1.0f / sumKY) : 1.0f;
        for (int i = 0; i < 6; ++i) kY[i] *= invSumKY;

        uint8_t* dstRow = dst.data() + y * dstStride;

        for (int x = 0; x < dstW; ++x) {
            float srcX = (x + 0.5f) * scaleX - 0.5f;
            int x0 = (int)std::floor(srcX);
            float dx = srcX - x0;

            float kX[6];
            int idxX[6];
            float sumKX = 0.0f;
            for (int j = -2; j <= 3; ++j) {
                int curX = x0 + j;
                idxX[j + 2] = std::clamp(curX, 0, srcW - 1);
                float w = lanczos3Kernel(dx - j);
                kX[j + 2] = w;
                sumKX += w;
            }
            float invSumKX = (std::abs(sumKX) > 1e-6f) ? (1.0f / sumKX) : 1.0f;
            for (int j = 0; j < 6; ++j) kX[j] *= invSumKX;

            float rAcc = 0.0f, gAcc = 0.0f, bAcc = 0.0f, aAcc = 0.0f;
            float minB = 255.0f, maxB = 0.0f;
            float minG = 255.0f, maxG = 0.0f;
            float minR = 255.0f, maxR = 0.0f;

            for (int i = 0; i < 6; ++i) {
                const uint8_t* srcRow = src.data() + idxY[i] * srcStride;
                float wy = kY[i];
                for (int j = 0; j < 6; ++j) {
                    float w = wy * kX[j];
                    const uint8_t* pix = srcRow + idxX[j] * 4;
                    float b = pix[0];
                    float g = pix[1];
                    float r = pix[2];
                    float a = pix[3];

                    bAcc += b * w;
                    gAcc += g * w;
                    rAcc += r * w;
                    aAcc += a * w;

                    // Hộp giới hạn lân cận trung tâm 4x4 để kẹp chống quầng sóng (Anti-ringing)
                    if (i >= 1 && i <= 4 && j >= 1 && j <= 4) {
                        minB = std::min(minB, b); maxB = std::max(maxB, b);
                        minG = std::min(minG, g); maxG = std::max(maxG, g);
                        minR = std::min(minR, r); maxR = std::max(maxR, r);
                    }
                }
            }

            // Anti-Ringing Clamping: Kẹp giá trị không vượt quá min-max cục bộ
            bAcc = std::clamp(bAcc, minB, maxB);
            gAcc = std::clamp(gAcc, minG, maxG);
            rAcc = std::clamp(rAcc, minR, maxR);

            uint8_t* outPix = dstRow + x * 4;
            outPix[0] = (uint8_t)std::clamp(bAcc, 0.0f, 255.0f); // B (Blue)
            outPix[1] = (uint8_t)std::clamp(gAcc, 0.0f, 255.0f); // G (Green)
            outPix[2] = (uint8_t)std::clamp(rAcc, 0.0f, 255.0f); // R (Red)
            outPix[3] = (uint8_t)std::clamp(aAcc, 0.0f, 255.0f); // A (Alpha)
        }
    }
    return dst;
}

std::vector<float> ImageEnhancerPro::fastBoxFilter(const std::vector<float>& src, int width, int height, int radius) {
    if (radius <= 0) return src;
    int total = width * height;
    std::vector<float> temp(total);
    std::vector<float> dst(total);

    // Lượt quét ngang O(N) với sliding window
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        int rowOffset = y * width;
        float sum = 0.0f;
        for (int i = -radius; i <= radius; ++i) {
            int cx = std::clamp(i, 0, width - 1);
            sum += src[rowOffset + cx];
        }
        for (int x = 0; x < width; ++x) {
            temp[rowOffset + x] = sum;
            int leftX = std::clamp(x - radius, 0, width - 1);
            int rightX = std::clamp(x + radius + 1, 0, width - 1);
            sum += src[rowOffset + rightX] - src[rowOffset + leftX];
        }
    }

    // Lượt quét dọc O(N) với sliding window và chuẩn hóa
    float invArea = 1.0f / ((2 * radius + 1) * (2 * radius + 1));

    #pragma omp parallel for schedule(static)
    for (int x = 0; x < width; ++x) {
        float sum = 0.0f;
        for (int i = -radius; i <= radius; ++i) {
            int cy = std::clamp(i, 0, height - 1);
            sum += temp[cy * width + x];
        }
        for (int y = 0; y < height; ++y) {
            dst[y * width + x] = sum * invArea;
            int topY = std::clamp(y - radius, 0, height - 1);
            int botY = std::clamp(y + radius + 1, 0, height - 1);
            sum += temp[botY * width + x] - temp[topY * width + x];
        }
    }

    return dst;
}

std::vector<float> ImageEnhancerPro::fastBlur(const std::vector<float>& src, int width, int height, int radius) {
    if (radius < 1) radius = 1;
    std::vector<float> buffer1 = src;
    std::vector<float> buffer2(src.size());
    int div = radius * 2 + 1;

    for (int pass = 0; pass < 3; ++pass) {
        // Lượt quét ngang (Horizontal)
        #pragma omp parallel for schedule(static)
        for (int y = 0; y < height; ++y) {
            int rowOffset = y * width;
            float sum = 0.0f;
            for (int i = -radius; i <= radius; ++i) {
                int cx = std::clamp(i, 0, width - 1);
                sum += buffer1[rowOffset + cx];
            }
            for (int x = 0; x < width; ++x) {
                buffer2[rowOffset + x] = sum / div;
                int leftX = std::clamp(x - radius, 0, width - 1);
                int rightX = std::clamp(x + radius + 1, 0, width - 1);
                sum += buffer1[rowOffset + rightX] - buffer1[rowOffset + leftX];
            }
        }

        // Lượt quét dọc (Vertical)
        #pragma omp parallel for schedule(static)
        for (int x = 0; x < width; ++x) {
            float sum = 0.0f;
            for (int i = -radius; i <= radius; ++i) {
                int cy = std::clamp(i, 0, height - 1);
                sum += buffer2[(cy * width) + x];
            }
            for (int y = 0; y < height; ++y) {
                buffer1[(y * width) + x] = sum / div;
                int topY = std::clamp(y - radius, 0, height - 1);
                int botY = std::clamp(y + radius + 1, 0, height - 1);
                sum += buffer2[(botY * width) + x] - buffer2[(topY * width) + x];
            }
        }
    }

    return buffer1;
}

// -------------------------------------------------------------
// Adaptive In-Loop Deblocking Filter (ITU-T / H.264 Inspired)
// -------------------------------------------------------------
void ImageEnhancerPro::applyAdaptiveDeblocking(
    std::vector<float>& luma, int width, int height, float blockiness)
{
    if (blockiness < 10.0f) return;

    // Ngưỡng thích ứng suy biến từ chỉ số blockiness [10, 100]
    float alpha = std::clamp(blockiness * 0.16f, 2.5f, 9.0f);
    float beta = alpha * 0.55f;
    float maxDelta = alpha * 0.35f;

    // 1. Quét lọc biên đứng (Vertical 8x8 block boundaries: x = 8, 16, 24...)
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        int rowIdx = y * width;
        for (int x = 8; x < width; x += 8) {
            int p1_idx = rowIdx + x - 2;
            int p0_idx = rowIdx + x - 1;
            int q0_idx = rowIdx + x;
            int q1_idx = rowIdx + x + 1;

            float p1 = luma[p1_idx];
            float p0 = luma[p0_idx];
            float q0 = luma[q0_idx];
            float q1 = (q1_idx < rowIdx + width) ? luma[q1_idx] : q0;

            float dStep = std::abs(q0 - p0);
            float dP = std::abs(p0 - p1);
            float dQ = std::abs(q1 - q0);

            // Chỉ lọc khi đây là bước nhảy viền nén khối (không phải đường biên vật thể thực)
            if (dStep < alpha && dP < beta && dQ < beta) {
                float delta = std::clamp((q0 - p0) * 0.40f, -maxDelta, maxDelta);
                luma[p0_idx] += delta;
                luma[q0_idx] -= delta;
            }
        }
    }

    // 2. Quét lọc biên ngang (Horizontal 8x8 block boundaries: y = 8, 16, 24...)
    #pragma omp parallel for schedule(static)
    for (int y = 8; y < height; y += 8) {
        int p1_row = (y - 2) * width;
        int p0_row = (y - 1) * width;
        int q0_row = y * width;
        int q1_row = std::min(y + 1, height - 1) * width;

        for (int x = 0; x < width; ++x) {
            float p1 = luma[p1_row + x];
            float p0 = luma[p0_row + x];
            float q0 = luma[q0_row + x];
            float q1 = luma[q1_row + x];

            float dStep = std::abs(q0 - p0);
            float dP = std::abs(p0 - p1);
            float dQ = std::abs(q1 - q0);

            if (dStep < alpha && dP < beta && dQ < beta) {
                float delta = std::clamp((q0 - p0) * 0.40f, -maxDelta, maxDelta);
                luma[p0_row + x] += delta;
                luma[q0_row + x] -= delta;
            }
        }
    }
}

// -------------------------------------------------------------
// Pre-Sharpening Outlier Despeckler (Khử chấm nhiễu phân tán có khoảng cách)
// -------------------------------------------------------------
void ImageEnhancerPro::applyOutlierDespeckle(
    std::vector<float>& luma, int width, int height,
    float estimatedNoise, const std::vector<float>* pSkinMask)
{
    float noiseSigma = std::max(1.2f, estimatedNoise);
    float noiseThresh = noiseSigma * 2.2f;

    std::vector<float> cleanLuma = luma;

    #pragma omp parallel for schedule(static)
    for (int y = 1; y < height - 1; ++y) {
        int rowIdx = y * width;
        int prevRow = (y - 1) * width;
        int nextRow = (y + 1) * width;

        for (int x = 1; x < width - 1; ++x) {
            int idx = rowIdx + x;
            float center = luma[idx];

            float n0 = luma[prevRow + x - 1];
            float n1 = luma[prevRow + x];
            float n2 = luma[prevRow + x + 1];
            float n3 = luma[rowIdx + x - 1];
            float n4 = luma[rowIdx + x + 1];
            float n5 = luma[nextRow + x - 1];
            float n6 = luma[nextRow + x];
            float n7 = luma[nextRow + x + 1];

            // Tìm min và max trong 8 điểm lân cận
            float min1 = std::min({n0, n1, n2, n3, n4, n5, n6, n7});
            float max1 = std::max({n0, n1, n2, n3, n4, n5, n6, n7});

            // Gradient nội bộ của 8 lân cận (không bao gồm center)
            float gradN = (std::abs(n4 - n3) + std::abs(n6 - n1) + std::abs(n7 - n0) + std::abs(n5 - n2)) * 0.25f;

            // Nếu center đột biến vượt trần hoặc thủng sàn so với toàn bộ 8 điểm lân cận
            bool isOutlierPeak = (center > max1 + 0.5f);
            bool isOutlierPit  = (center < min1 - 0.5f);

            if ((isOutlierPeak || isOutlierPit) && gradN < 14.0f) {
                float pSkin = (pSkinMask && !pSkinMask->empty()) ? (*pSkinMask)[idx] : 0.0f;
                float dev = isOutlierPeak ? (center - max1) : (min1 - center);
                if (dev < noiseThresh * 2.5f || pSkin > 0.05f) {
                    float target = isOutlierPeak ? max1 : min1;
                    float blend = (pSkin > 0.05f) ? 0.85f : 0.70f;
                    cleanLuma[idx] = center * (1.0f - blend) + target * blend;
                }
            }
        }
    }

    luma = std::move(cleanLuma);
}

// -------------------------------------------------------------
// Luma-Guided Chroma Denoising (Khử đốm màu loang lổ trên Cb, Cr)
// -------------------------------------------------------------
void ImageEnhancerPro::applyChromaDenoise(
    std::vector<float>& cb, std::vector<float>& cr,
    const std::vector<float>& luma, int width, int height)
{
    std::vector<float> cleanCb = cb;
    std::vector<float> cleanCr = cr;

    #pragma omp parallel for schedule(static)
    for (int y = 1; y < height - 1; ++y) {
        int rowIdx = y * width;
        int prevRow = (y - 1) * width;
        int nextRow = (y + 1) * width;

        for (int x = 1; x < width - 1; ++x) {
            int idx = rowIdx + x;
            float lumC = luma[idx];

            float sumW = 0.0f;
            float sumCb = 0.0f;
            float sumCr = 0.0f;

            int coords[9] = {
                prevRow + x - 1, prevRow + x, prevRow + x + 1,
                rowIdx + x - 1,  idx,         rowIdx + x + 1,
                nextRow + x - 1, nextRow + x, nextRow + x + 1
            };
            float spatialWeights[9] = {
                0.075f, 0.125f, 0.075f,
                0.125f, 0.200f, 0.125f,
                0.075f, 0.125f, 0.075f
            };

            for (int k = 0; k < 9; ++k) {
                int cIdx = coords[k];
                float dLum = std::abs(luma[cIdx] - lumC);
                float rangeW = 1.0f / (1.0f + dLum * 0.15f);
                float w = spatialWeights[k] * rangeW;
                sumW += w;
                sumCb += cb[cIdx] * w;
                sumCr += cr[cIdx] * w;
            }

            if (sumW > 1e-4f) {
                cleanCb[idx] = sumCb / sumW;
                cleanCr[idx] = sumCr / sumW;
            }
        }
    }

    cb = std::move(cleanCb);
    cr = std::move(cleanCr);
}

// -------------------------------------------------------------
// Contrast Limited Adaptive Histogram Equalization (CLAHE)
// -------------------------------------------------------------
void ImageEnhancerPro::applyCLAHE(
    std::vector<float>& luma, int width, int height,
    float clipLimit, float blendFactor)
{
    if (blendFactor <= 0.001f) return;

    const int GRID_X = 8;
    const int GRID_Y = 8;
    int tileW = (width + GRID_X - 1) / GRID_X;
    int tileH = (height + GRID_Y - 1) / GRID_Y;

    std::vector<std::vector<std::vector<float>>> mappings(
        GRID_Y, std::vector<std::vector<float>>(GRID_X, std::vector<float>(256, 0.0f))
    );

    // 1. Tính toán Histogram và CDF phân phối tích lũy cho từng tile 8x8 (Song song hóa đa luồng OpenMP)
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int ty = 0; ty < GRID_Y; ++ty) {
        for (int tx = 0; tx < GRID_X; ++tx) {
            int yStart = ty * tileH;
            int yEnd = std::min(yStart + tileH, height);
            int currentTileH = yEnd - yStart;
            int xStart = tx * tileW;
            int xEnd = std::min(xStart + tileW, width);
            int currentTileW = xEnd - xStart;
            int tileArea = currentTileW * currentTileH;
            if (tileArea <= 0) continue;

            std::vector<int> hist(256, 0);
            for (int y = yStart; y < yEnd; ++y) {
                int row = y * width;
                for (int x = xStart; x < xEnd; ++x) {
                    int bin = std::clamp((int)std::round(luma[row + x]), 0, 255);
                    hist[bin]++;
                }
            }

            int clipVal = (int)std::max(1.0f, (clipLimit * tileArea) / 256.0f);
            int excess = 0;
            for (int i = 0; i < 256; ++i) {
                if (hist[i] > clipVal) {
                    excess += hist[i] - clipVal;
                    hist[i] = clipVal;
                }
            }

            int bonus = excess / 256;
            int remainder = excess % 256;
            for (int i = 0; i < 256; ++i) {
                hist[i] += bonus;
                if (i < remainder) hist[i]++;
            }

            int cdf = 0;
            for (int i = 0; i < 256; ++i) {
                cdf += hist[i];
                mappings[ty][tx][i] = ((float)cdf / (float)tileArea) * 255.0f;
            }
        }
    }

    // 2. Nội suy song tuyến tính (Bilinear Interpolation) giữa các tile lân cận
    std::vector<float> claheOut = luma;

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        float gy = (y - tileH * 0.5f) / (float)tileH;
        int ty0 = (int)std::floor(gy);
        int ty1 = ty0 + 1;
        float dy = gy - ty0;

        int ty0_c = std::clamp(ty0, 0, GRID_Y - 1);
        int ty1_c = std::clamp(ty1, 0, GRID_Y - 1);

        int row = y * width;
        for (int x = 0; x < width; ++x) {
            float gx = (x - tileW * 0.5f) / (float)tileW;
            int tx0 = (int)std::floor(gx);
            int tx1 = tx0 + 1;
            float dx = gx - tx0;

            int tx0_c = std::clamp(tx0, 0, GRID_X - 1);
            int tx1_c = std::clamp(tx1, 0, GRID_X - 1);

            int val = std::clamp((int)std::round(luma[row + x]), 0, 255);

            float v00 = mappings[ty0_c][tx0_c][val];
            float v10 = mappings[ty0_c][tx1_c][val];
            float v01 = mappings[ty1_c][tx0_c][val];
            float v11 = mappings[ty1_c][tx1_c][val];

            float top = v00 * (1.0f - dx) + v10 * dx;
            float bot = v01 * (1.0f - dx) + v11 * dx;
            float eqVal = top * (1.0f - dy) + bot * dy;

            // Shadow floor protection: CLAHE không được dìm vùng tối Y < 25 xuống sâu hơn ban đầu
            float curLuma = luma[row + x];
            if (curLuma < 25.0f && eqVal < curLuma) {
                float protect = (25.0f - curLuma) / 25.0f;
                eqVal = eqVal * (1.0f - protect) + curLuma * protect;
            }

            claheOut[row + x] = (1.0f - blendFactor) * curLuma + blendFactor * eqVal;
        }
    }

    luma = std::move(claheOut);
}

// -------------------------------------------------------------
// Highlight & Shadow Local Recovery (B4)
// -------------------------------------------------------------
void ImageEnhancerPro::applyHighlightShadowRecovery(
    std::vector<float>& luma, int width, int height,
    float shadowLift, float highlightPull)
{
    if (shadowLift <= 0.001f && highlightPull <= 0.001f) return;

    int r = std::clamp(std::min(width, height) / 32, 8, 32);
    std::vector<float> localMean = fastBlur(luma, width, height, r);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; ++i) {
        float loc = localMean[i];
        float orig = luma[i];

        // Mở rộng dải cứu tối lên Y < 55.0f với hàm nâng bậc 2 êm dịu (Quadratic Shadow Lift)
        if (loc < 60.0f && orig < 55.0f && shadowLift > 0.01f) {
            float darkFactor = (55.0f - orig) / 55.0f; // 1.0 tại orig=0, giảm mượt về 0 tại orig=55
            float lift = shadowLift * 28.0f * (darkFactor * darkFactor);
            luma[i] = std::clamp(orig + lift, 0.0f, 255.0f);
        }

        // Mở rộng dải cứu cháy sáng xuống Y > 215.0f với hàm nén bậc 2
        if (loc > 210.0f && orig > 215.0f && highlightPull > 0.01f) {
            float brightFactor = (orig - 215.0f) / 40.0f; // 0.0 tại orig=215, tăng dần lên 1.0 tại 255
            float pull = highlightPull * 22.0f * (brightFactor * brightFactor);
            luma[i] = std::clamp(orig - pull, 0.0f, 255.0f);
        }
    }
}

// -------------------------------------------------------------
// Local Laplacian Tone Mapping (B6)
// -------------------------------------------------------------
void ImageEnhancerPro::applyLocalLaplacianToneMapping(
    std::vector<float>& luma, int width, int height,
    float clarityBoost,
    const std::vector<float>* pSkinMask)
{
    if (clarityBoost <= 0.001f) return;

    int r = std::clamp(std::min(width, height) / 48, 3, 12);
    std::vector<float> base = fastBlur(luma, width, height, r);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; ++i) {
        float diff = luma[i] - base[i];
        float damp = 12.0f / (std::abs(diff) + 12.0f);
        float maskVal = (pSkinMask && !pSkinMask->empty()) ? (*pSkinMask)[i] : 0.0f;
        if (std::isnan(maskVal)) maskVal = 0.0f;
        float skinDamp = 1.0f - std::clamp(maskVal, 0.0f, 1.0f) * 0.85f;
        float updated = luma[i] + diff * clarityBoost * damp * skinDamp;
        if (!std::isnan(updated)) {
            luma[i] = std::clamp(updated, 0.0f, 255.0f);
        }
    }
}

// -------------------------------------------------------------
// Guided Filter Implementation
// -------------------------------------------------------------
std::vector<float> ImageEnhancerPro::applyGuidedFilterSingle(
    const std::vector<float>& p, const std::vector<float>& I,
    int width, int height, int radius, float eps)
{
    int nPixels = width * height;
    std::vector<float> mean_I = fastBoxFilter(I, width, height, radius);
    std::vector<float> mean_p = fastBoxFilter(p, width, height, radius);

    std::vector<float> Ip(nPixels);
    std::vector<float> II(nPixels);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < nPixels; ++i) {
        Ip[i] = I[i] * p[i];
        II[i] = I[i] * I[i];
    }

    std::vector<float> mean_Ip = fastBoxFilter(Ip, width, height, radius);
    std::vector<float> mean_II = fastBoxFilter(II, width, height, radius);

    std::vector<float> a(nPixels);
    std::vector<float> b(nPixels);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < nPixels; ++i) {
        float var_I = mean_II[i] - mean_I[i] * mean_I[i];
        float cov_Ip = mean_Ip[i] - mean_I[i] * mean_p[i];
        float a_val = cov_Ip / (var_I + eps);
        a[i] = a_val;
        b[i] = mean_p[i] - a_val * mean_I[i];
    }

    std::vector<float> mean_a = fastBoxFilter(a, width, height, radius);
    std::vector<float> mean_b = fastBoxFilter(b, width, height, radius);

    std::vector<float> q(nPixels);
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < nPixels; ++i) {
        q[i] = mean_a[i] * I[i] + mean_b[i];
    }
    return q;
}

// -------------------------------------------------------------
// 3-Scale Guided Filter Decomposition (B7)
// -------------------------------------------------------------
void ImageEnhancerPro::applyGuidedFilter3Scale(
    const std::vector<float>& luma,
    std::vector<float>& diffGuided,
    int width, int height,
    const EnhanceOptionsPro& opts,
    const std::vector<float>* pSkinMask,
    std::vector<float>* pMicroBaseOut,
    float estimatedNoise)
{
    int nPixels = width * height;
    diffGuided.assign(nPixels, 0.0f);

    std::vector<float> nanoBase = applyGuidedFilterSingle(luma, luma, width, height, 1, 100.0f);
    std::vector<float> microBase = applyGuidedFilterSingle(luma, luma, width, height, 2, 350.0f);
    std::vector<float> macroBase = applyGuidedFilterSingle(luma, luma, width, height, 4, 1400.0f);

    if (pMicroBaseOut) {
        *pMicroBaseOut = microBase; // Tái sử dụng tầng structure r=2 cho texture layer
    }

    float noiseThresh = std::max(1.8f, 1.8f * estimatedNoise);

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        int rowIdx = y * width;
        int yPrev = std::max(0, y - 1) * width;
        int yNext = std::min(height - 1, y + 1) * width;

        for (int x = 0; x < width; ++x) {
            int i = rowIdx + x;
            float nanoDetail = luma[i] - nanoBase[i];
            float microDetail = nanoBase[i] - microBase[i];
            float macroDetail = microBase[i] - macroBase[i];

            // Gradient Gating: ức chế nano-noise ở các vùng phẳng (bầu trời, mặt nước, tường trơn)
            int xPrev = rowIdx + std::max(0, x - 1);
            int xNext = rowIdx + std::min(width - 1, x + 1);
            float grad = std::abs(luma[xNext] - luma[xPrev]) + std::abs(luma[yNext + x] - luma[yPrev + x]);
            float flatGate = std::clamp((grad - noiseThresh) / (2.5f * noiseThresh), 0.0f, 1.0f);

            // Skin Damping & Natural Pore Preservation (Anti-bệt vs Anti-gai)
            float maskVal = (pSkinMask && !pSkinMask->empty()) ? (*pSkinMask)[i] : 0.0f;
            if (std::isnan(maskVal)) maskVal = 0.0f;
            float skinDamp = 1.0f - std::clamp(maskVal, 0.0f, 1.0f) * 0.90f;

            float poreGate = (skinDamp > 0.5f ? skinDamp : 0.5f);
            if (maskVal > 0.05f) {
                // Tách tần số bảo tồn lỗ chân lông tự nhiên:
                // microDetail mang vi cấu trúc lỗ chân lông thực tế.
                // Nhiễu hạt < 1.5 bị triệt tiêu (anti-gai hạt), gai mụn > 6.0 bị nén mềm (anti-gai mụn),
                // biên độ trung bình [1.5, 6.0] được bảo tồn với hệ số skinPorePreserve (anti-bệt da).
                float poreAmp = std::abs(microDetail);
                float pFactor = 0.0f;
                if (poreAmp >= 1.5f && poreAmp <= 6.0f) {
                    pFactor = opts.skinPorePreserve;
                } else if (poreAmp > 6.0f) {
                    pFactor = opts.skinPorePreserve * (6.0f / poreAmp);
                } else {
                    pFactor = opts.skinPorePreserve * (poreAmp / 1.5f) * 0.35f;
                }
                poreGate = (1.0f - maskVal) * poreGate + maskVal * pFactor;
            }

            // Gradient Gating trên cả 3 tầng tần số: triệt tiêu hạt nhiễu nổi trên mảng phẳng
            float wNano = 1.45f * opts.nanoDetailBoost * flatGate * skinDamp;
            float wMicro = 1.15f * flatGate * poreGate;
            float wMacro = 0.65f * (0.15f + 0.85f * flatGate);

            float guided = (nanoDetail * wNano + microDetail * wMicro + macroDetail * wMacro);
            diffGuided[i] = guided * (opts.detailBoost - 1.0f);
        }
    }
}

// -------------------------------------------------------------
// Texture Layer Synthesis (B8)
// -------------------------------------------------------------
void ImageEnhancerPro::synthesizeTextureLayer(
    std::vector<float>& luma,
    int width, int height,
    float textureBoost,
    const std::vector<float>* pSkinMask,
    const std::vector<float>* pPrecomputedStructure)
{
    if (textureBoost <= 0.001f) return;

    std::vector<float> fallbackStructure;
    const std::vector<float>* pStructure = pPrecomputedStructure;
    if (!pStructure) {
        fallbackStructure = applyGuidedFilterSingle(luma, luma, width, height, 2, 250.0f);
        pStructure = &fallbackStructure;
    }
    const std::vector<float>& structure = *pStructure;

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; ++i) {
        float texture = luma[i] - structure[i];
        float damp = 8.0f / (std::abs(texture) + 8.0f);
        // Triệt tiêu bơm hạt texture vào vùng da người (giữ da mịn màng, tự nhiên)
        float maskVal = (pSkinMask && !pSkinMask->empty()) ? (*pSkinMask)[i] : 0.0f;
        if (std::isnan(maskVal)) maskVal = 0.0f;
        float skinDamp = 1.0f - std::clamp(maskVal, 0.0f, 1.0f);
        float updated = luma[i] + texture * textureBoost * damp * skinDamp;
        if (!std::isnan(updated)) {
            luma[i] = std::clamp(updated, 0.0f, 255.0f);
        }
    }
}

// -------------------------------------------------------------
// Image Score & Extended Buffer Analysis
// -------------------------------------------------------------
ImageScorePro ImageEnhancerPro::analyzeImageBufferPro(
    const std::vector<uint8_t>& src,
    int width, int height, int stride,
    uintmax_t fileSize)
{
    ImageScorePro score;
    score.origW = width;
    score.origH = height;
    score.megaPixels = (width * height) / 1000000.0f;
    score.bpp = (fileSize > 0) ? ((float)fileSize / (width * height)) : 0.0f;

    double gradSum = 0.0;
    double tenengradSum = 0.0;
    double lapSum = 0.0;
    double saturationSum = 0.0;
    int skinPixels = 0;
    int texturePixels = 0;
    int shadowClipCount = 0;
    int highlightClipCount = 0;
    int sampleCount = 0;
    std::vector<int> hist(256, 0);

    // Phát hiện vỡ ô vuông nén JPEG (8x8 block boundary vs inner block)
    double blockBoundaryGrad = 0.0;
    int blockBoundaryCount = 0;
    double blockInnerGrad = 0.0;
    int blockInnerCount = 0;

    // Tách biệt hoàn toàn bộ đếm nét mảnh (Thin Feature Ratio)
    int edgeCountForThin = 0;
    int thinFeatureCount = 0;

    int lightPixelCount = 0;
    int darkPixelCount = 0;
    int midPixelCount = 0;

    // Bước nhảy lấy mẫu cân bằng độ chính xác cao và tốc độ: kẹp tối đa step <= 3 để không bỏ sót chi tiết
    int step = std::clamp((int)std::sqrt((width * height) / 1200000.0f), 1, 3);

    std::vector<float> flatRegionVariances;

    // Lưới không gian 4x4 (16 ô) để đo phân bố độ nét tiêu điểm cục bộ (Focal Region vs Background Bokeh)
    std::vector<double> tileGradSum(16, 0.0);
    std::vector<int> tileSampleCount(16, 0);

    for (int y = step; y < height - step; y += step) {
        const uint8_t* rowPrev = src.data() + (y - step) * stride;
        const uint8_t* rowCur  = src.data() + y * stride;
        const uint8_t* rowNext = src.data() + (y + step) * stride;

        int tileY = std::clamp(y * 4 / height, 0, 3);

        for (int x = step; x < width - step; x += step) {
            const uint8_t* pix = rowCur + x * 4;
            float b = pix[0];
            float g = pix[1];
            float r = pix[2];

            // Phân vùng tiêu điểm cục bộ
            int tileX = std::clamp(x * 4 / width, 0, 3);
            int tileIdx = tileY * 4 + tileX;

            // Độ sáng đơn sắc (ITU-R BT.601 luma)
            float Y = 0.299f * r + 0.587f * g + 0.114f * b;
            int yInt = std::clamp((int)std::round(Y), 0, 255);
            hist[yInt]++;

            // Kiểm tra phân bố lưỡng cực Bimodal (giấy trắng vs mực đen)
            if (yInt >= 170) lightPixelCount++;
            else if (yInt <= 90) darkPixelCount++;
            else midPixelCount++;

            // Kiểm tra clipping sáng/tối
            if (yInt <= 6) shadowClipCount++;
            if (yInt >= 248) highlightClipCount++;

            // Độ bão hòa màu
            float maxC = std::max(r, std::max(g, b));
            float minC = std::min(r, std::min(g, b));
            saturationSum += (maxC - minC);

            // Lấy mẫu gradient 4 hướng: Ngang, Dọc, Chéo 45, Chéo 135
            const uint8_t* pixR = rowCur + (x + step) * 4;
            const uint8_t* pixL = rowCur + (x - step) * 4;
            const uint8_t* pixB = rowNext + x * 4;
            const uint8_t* pixT = rowPrev + x * 4;

            float yR = 0.299f * pixR[2] + 0.587f * pixR[1] + 0.114f * pixR[0];
            float yL = 0.299f * pixL[2] + 0.587f * pixL[1] + 0.114f * pixL[0];
            float yB = 0.299f * pixB[2] + 0.587f * pixB[1] + 0.114f * pixB[0];
            float yT = 0.299f * pixT[2] + 0.587f * pixT[1] + 0.114f * pixT[0];

            float dx = (yR - yL) / (2.0f * step);
            float dy = (yB - yT) / (2.0f * step);

            // Đường chéo
            const uint8_t* pixBR = rowNext + (x + step) * 4;
            const uint8_t* pixTL = rowPrev + (x - step) * 4;
            const uint8_t* pixTR = rowPrev + (x + step) * 4;
            const uint8_t* pixBL = rowNext + (x - step) * 4;

            float yBR = 0.299f * pixBR[2] + 0.587f * pixBR[1] + 0.114f * pixBR[0];
            float yTL = 0.299f * pixTL[2] + 0.587f * pixTL[1] + 0.114f * pixTL[0];
            float yTR = 0.299f * pixTR[2] + 0.587f * pixTR[1] + 0.114f * pixTR[0];
            float yBL = 0.299f * pixBL[2] + 0.587f * pixBL[1] + 0.114f * pixBL[0];

            float dd1 = (yBR - yTL) / (2.828f * step);
            float dd2 = (yTR - yBL) / (2.828f * step);

            float grad = std::sqrt(dx * dx + dy * dy + dd1 * dd1 + dd2 * dd2);
            gradSum += grad;
            tenengradSum += (dx * dx + dy * dy);

            tileGradSum[tileIdx] += grad;
            tileSampleCount[tileIdx]++;

            // Tần số vi mô Micro-Laplacian
            float lap = std::abs(4.0f * Y - yR - yL - yB - yT) / (float)(step * step);
            lapSum += lap;

            // Kiểm tra ranh giới khối JPEG 8x8
            if ((x % 8 == 0) || (y % 8 == 0)) {
                blockBoundaryGrad += grad;
                blockBoundaryCount++;
            } else if ((x % 8 == 4) && (y % 8 == 4)) {
                blockInnerGrad += grad;
                blockInnerCount++;
            }

            // Đếm mật độ vân ảnh hữu cơ (Texture)
            if (grad >= 3.0f && grad <= 30.0f) {
                texturePixels++;
            }

            // Đếm tỷ lệ nét mảnh (Thin Feature Ratio: pixel cạnh có laplacian lớn so với bề rộng < 3px)
            if (grad >= 6.0f) {
                edgeCountForThin++;
                if (lap > 5.0f) {
                    thinFeatureCount++;
                }
            }

            // Nhận diện sắc diện da người chuẩn Melanin ROI trong YCbCr
            // Đảm bảo không nhận nhầm vùng phẳng đồ họa nhân tạo (synthetic flat fill: grad < 1.2f)
            float Cb = 128.0f - 0.168736f * r - 0.331264f * g + 0.500000f * b;
            float Cr = 128.0f + 0.500000f * r - 0.418688f * g - 0.081312f * b;
            float diffRB = r - b;
            float diffRG = r - g;
            if (grad >= 1.2f && Cb >= 77.0f && Cb <= 128.0f && Cr >= 133.0f && Cr <= 175.0f && 
                Y >= 35.0f && Y <= 215.0f && 
                diffRB >= 8.0f && diffRB <= 75.0f && 
                diffRG >= 3.0f && diffRG <= 50.0f && 
                r > g && r > b && (r < 1.34f * g)) 
            {
                skinPixels++;
            }

            // Thu thập mẫu vùng phẳng để ước tính MAD nhiễu nền
            if (grad < 2.5f) {
                flatRegionVariances.push_back(lap);
            }

            sampleCount++;
        }
    }

    if (sampleCount > 0) {
        float avgGrad = (float)(gradSum / sampleCount);

        // Đánh giá sắc nét vùng tiêu điểm cục bộ (Focal Region vs Background Bokeh)
        float maxTileGrad = 0.0f;
        for (int t = 0; t < 16; ++t) {
            if (tileSampleCount[t] > 0) {
                float tAvg = (float)(tileGradSum[t] / tileSampleCount[t]);
                if (tAvg > maxTileGrad) maxTileGrad = tAvg;
            }
        }
        // Nếu có sự phân hóa mạnh giữa vùng cực nét và vùng mờ mịt (ảnh chân dung xóa phông / DOF nông),
        // kết hợp độ nét vùng tiêu điểm để tránh việc hậu cảnh bokeh kéo sập điểm độ nét của toàn ảnh
        float effectiveClarityGrad = avgGrad;
        if (maxTileGrad > 2.0f * avgGrad && maxTileGrad > 3.5f) {
            effectiveClarityGrad = 0.55f * maxTileGrad + 0.45f * avgGrad;
        }

        // Đường cong độ nét liên tục chuẩn Tenengrad Acutance + Local Gradient
        float tenengradAcutance = (float)std::sqrt(tenengradSum / sampleCount);
        float effectiveAcutance = 0.50f * effectiveClarityGrad + 0.50f * tenengradAcutance;
        score.clarityScore = std::clamp(100.0f * (1.0f - std::exp(-effectiveAcutance / 16.0f)), 0.0f, 100.0f);
        score.skinPercent = ((float)skinPixels / sampleCount) * 100.0f;
        score.edgeSharpness = std::clamp(tenengradAcutance * 4.2f, 0.0f, 100.0f);
        score.highFreqEnergy = std::clamp((float)(lapSum / sampleCount) * 10.0f, 0.0f, 100.0f);
        score.blurDegree = std::clamp(100.0f * std::exp(-effectiveAcutance / 9.0f), 0.0f, 100.0f);
        score.shadowClipPercent = ((float)shadowClipCount / sampleCount) * 100.0f;
        score.highlightClipPercent = ((float)highlightClipCount / sampleCount) * 100.0f;
        score.colorSaturation = std::clamp(((float)(saturationSum / sampleCount) / 255.0f) * 100.0f, 0.0f, 100.0f);
        score.textureComplexity = std::clamp(((float)texturePixels / sampleCount) * 180.0f, 0.0f, 100.0f);
        score.thinFeatureRatio = (edgeCountForThin > 0) ? std::clamp((float)thinFeatureCount / edgeCountForThin, 0.0f, 1.0f) : 0.25f;
    }

    // Dynamic Range: 1% đến 99% percentile
    int totalHist = sampleCount;
    int p1 = 0, p99 = 255;
    bool foundP1 = false;
    int acc = 0;
    for (int i = 0; i < 256; ++i) {
        acc += hist[i];
        if (!foundP1 && acc >= totalHist * 0.01) { p1 = i; foundP1 = true; }
        if (acc >= totalHist * 0.99) { p99 = i; break; }
    }
    score.dynamicRange = (float)(p99 - p1);

    // Ước lượng mức nhiễu nền MAD (Median Absolute Deviation)
    if (!flatRegionVariances.empty()) {
        size_t mid = flatRegionVariances.size() / 2;
        std::nth_element(flatRegionVariances.begin(), flatRegionVariances.begin() + mid, flatRegionVariances.end());
        float median = flatRegionVariances[mid];

        std::vector<float> devs(flatRegionVariances.size());
        for (size_t i = 0; i < devs.size(); ++i) {
            devs[i] = std::abs(flatRegionVariances[i] - median);
        }
        std::nth_element(devs.begin(), devs.begin() + mid, devs.end());
        score.noiseFloor = 1.4826f * devs[mid];
    } else {
        score.noiseFloor = 2.0f;
    }

    // Ước lượng tỷ lệ SNR (Signal-to-Noise Ratio) in dB
    float signalStd = std::max(5.0f, score.dynamicRange / 4.0f);
    float noiseSigma = std::max(0.2f, score.noiseFloor);
    score.snrDb = std::clamp(20.0f * std::log10(signalStd / noiseSigma), 10.0f, 55.0f);

    // Đánh giá mức độ vỡ ô vuông nén JPEG
    if (blockBoundaryCount > 0 && blockInnerCount > 0) {
        float avgB = (float)(blockBoundaryGrad / blockBoundaryCount);
        float avgI = (float)(blockInnerGrad / blockInnerCount);
        if (avgI > 0.01f && avgB > avgI) {
            float ratio = (avgB - avgI) / avgI;
            score.compressionBlockiness = std::clamp(ratio * 50.0f, 0.0f, 100.0f);
        }
    }

    // Đặc trưng phân tách lưỡng cực Bimodal (đỉnh giấy trắng + đỉnh mực đen)
    float lightRatio = (sampleCount > 0) ? (float)lightPixelCount / sampleCount : 0.0f;
    float darkRatio  = (sampleCount > 0) ? (float)darkPixelCount / sampleCount : 0.0f;
    float midRatio   = (sampleCount > 0) ? (float)midPixelCount / sampleCount : 0.0f;
    bool isBimodal   = (lightRatio >= 0.40f && darkRatio >= 0.05f && midRatio <= 0.35f);

    // Phân loại ngữ cảnh ảnh PRO V4 chuẩn xác
    // 1. Nhận diện Tài liệu / Biểu đồ / Sơ đồ / Chụp màn hình (Document / Text / Diagram)
    bool isDocument = false;
    if (isBimodal && (score.colorSaturation < 20.0f || lightRatio > 0.50f)) {
        isDocument = true;
    } else if (score.colorSaturation < 14.0f && lightRatio > 0.55f) {
        isDocument = true;
    }

    if (isDocument) {
        score.detectedType = "Tài liệu / Văn bản (Document / Text)";
    } else if (score.skinPercent >= 21.0f) {
        score.detectedType = "Chân dung cận cảnh (Portrait Studio)";
    } else if (score.skinPercent >= 7.0f) {
        score.detectedType = "Người + Phong cảnh (Environmental Portrait)";
    } else if (score.blurDegree >= 28.0f || score.clarityScore < 40.0f) {
        score.detectedType = "Ảnh mờ / Cần phục hồi nét (Blur/Defocus)";
    } else if (score.textureComplexity >= 25.0f || score.clarityScore >= 45.0f) {
        score.detectedType = "Phong cảnh / Chi tiết cao (Landscape)";
    } else if (score.compressionBlockiness >= 30.0f || (score.bpp > 0.0f && score.bpp < 0.18f)) {
        score.detectedType = "Ảnh nén suy hao (Compressed/Web)";
    } else {
        score.detectedType = "Phong cảnh / Chi tiết cao (Landscape)";
    }

    // Xếp hạng chất lượng ảnh (qualityGrade)
    if (score.clarityScore >= 60.0f && score.noiseFloor <= 2.5f) {
        score.qualityGrade = "Tuyệt vời (Studio Grade)";
    } else if (score.clarityScore >= 42.0f) {
        score.qualityGrade = "Sắc nét tốt (Good Clarity)";
    } else if (score.clarityScore >= 25.0f) {
        score.qualityGrade = "Hơi mờ / Cần bù nét (Soft / Needs Sharp)";
    } else {
        score.qualityGrade = "Mờ nặng / Suy giảm (Heavy Blur / Degraded)";
    }

    return score;
}

// -------------------------------------------------------------
// -------------------------------------------------------------
// Core Processing Pipeline in Studio YCbCr Space (with Chroma Tracking & Anti-Halo)
// -------------------------------------------------------------
void ImageEnhancerPro::processSharpenPro(
    const std::vector<uint8_t>& src, std::vector<uint8_t>& dst,
    int width, int height, int stride,
    const EnhanceOptionsPro& opts,
    float estimatedNoise)
{
    int nPixels = width * height;
    std::vector<float> origR(nPixels);
    std::vector<float> origG(nPixels);
    std::vector<float> origB(nPixels);
    std::vector<float> luma(nPixels);
    std::vector<float> chromaCb(nPixels);
    std::vector<float> chromaCr(nPixels);
    std::vector<uint8_t> alpha(nPixels);

    // 1. Chuyển đổi sang YCbCr (ITU-R BT.601) và lưu giữ RGB gốc để bảo toàn sắc độ
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        const uint8_t* row = src.data() + y * stride;
        int rowIdx = y * width;
        for (int x = 0; x < width; ++x) {
            const uint8_t* pix = row + x * 4;
            float b = pix[0];
            float g = pix[1];
            float r = pix[2];
            int idx = rowIdx + x;

            alpha[idx] = pix[3];
            origR[idx] = r;
            origG[idx] = g;
            origB[idx] = b;

            luma[idx]     = 0.299f * r + 0.587f * g + 0.114f * b;
            chromaCb[idx] = -0.168736f * r - 0.331264f * g + 0.500000f * b + 128.0f;
            chromaCr[idx] = 0.500000f * r - 0.418688f * g - 0.081312f * b + 128.0f;
        }
    }

    // 1.5. Mặt nạ xác suất da người tự nhiên (Continuous Soft Skin Probability Map - Không còn ngưỡng cứng)
    std::vector<float> skinMask(nPixels, 0.0f);
    auto smoothstepVal = [](float edge0, float edge1, float x) -> float {
        float denom = edge1 - edge0;
        if (std::abs(denom) < 1e-5f) {
            return (x >= edge1) ? 1.0f : 0.0f;
        }
        float t = std::clamp((x - edge0) / denom, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    };

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < nPixels; ++i) {
        float r = origR[i], g = origG[i], b = origB[i];
        float cb = chromaCb[i], cr = chromaCr[i];
        float Y = luma[i];

        // 1. Khoảng cách Mahalanobis elip trong không gian sắc độ Cb-Cr
        float dCb = (cb - 105.0f) / 16.0f;
        float dCr = (cr - 150.0f) / 14.0f;
        float chromaDistSq = dCb * dCb + dCr * dCr;
        if (chromaDistSq > 10.0f) {
            skinMask[i] = 0.0f;
            continue;
        }
        float chromaProb = std::exp(-0.5f * chromaDistSq);

        // 2. Chuyển tiếp mềm mại dải độ chói Y [20, 235]
        float lumaWeight = smoothstepVal(20.0f, 45.0f, Y) * (1.0f - smoothstepVal(210.0f, 235.0f, Y));

        // 3. Tỷ lệ quang phổ Melanin RGB chuyển tiếp trơn tru
        float diffRB = r - b;
        float diffRG = r - g;
        float rgbWeight = smoothstepVal(4.0f, 12.0f, diffRB) * 
                          (1.0f - smoothstepVal(72.0f, 90.0f, diffRB)) *
                          smoothstepVal(1.0f, 5.0f, diffRG) *
                          (1.0f - smoothstepVal(48.0f, 65.0f, diffRG));

        // 4. Triệt tiêu hoàng hôn đỏ / lửa cam rực (loại bỏ màu đỏ cam gắt không phải da: r < 1.34*g)
        float sunsetWeight = (g > 1.0f) ? (1.0f - smoothstepVal(1.22f * g, 1.38f * g, r)) : 1.0f;

        float p = chromaProb * lumaWeight * rgbWeight * sunsetWeight;
        if (std::isnan(p) || p <= 0.0f) {
            skinMask[i] = 0.0f;
        } else {
            skinMask[i] = std::min(p, 1.0f);
        }
    }

    // 1.6. Khử nhiễu ô vuông nén 8x8 (Adaptive In-Loop Deblocking) ngay từ gốc trước khi phân rã
    applyAdaptiveDeblocking(luma, width, height, opts.compressionBlockiness);

    // 1.7. Khử chấm nhiễu phân tán có khoảng cách (Pre-Sharpening Outlier Despeckler)
    applyOutlierDespeckle(luma, width, height, estimatedNoise, &skinMask);

    // 1.8. Khử nhiễu màu sắc độ loang lổ (Luma-Guided Chroma Denoising)
    applyChromaDenoise(chromaCb, chromaCr, luma, width, height);

    // 2. Cân bằng tương phản cục bộ thích ứng CLAHE
    if (opts.claheBlend > 0.001f) {
        applyCLAHE(luma, width, height, 2.5f, opts.claheBlend);
    }

    // 3. Highlight/Shadow Local Recovery
    applyHighlightShadowRecovery(luma, width, height, opts.shadowLift, opts.highlightPull);

    // 4. Local Laplacian Tone Mapping (với bảo vệ da không bị sần mụn)
    applyLocalLaplacianToneMapping(luma, width, height, opts.clarityBoost, &skinMask);

    // 5. Phân rã đa tầng 3-Scale Guided Filter (Nano, Micro, Macro với bảo vệ da)
    std::vector<float> diffGuided(nPixels, 0.0f);
    std::vector<float> microBase(nPixels, 0.0f);
    applyGuidedFilter3Scale(luma, diffGuided, width, height, opts, &skinMask, &microBase, estimatedNoise);

    // 6. Texture Layer Synthesis (Tái sử dụng microBase đã lọc ở bước 5, không lọc lại!)
    synthesizeTextureLayer(luma, width, height, opts.textureBoost, &skinMask, &microBase);

    // 7. Mặt nạ làm mờ Gaussian cho CAS
    std::vector<float> blurL = fastBlur(luma, width, height, opts.radius);

    // 8. Ngưỡng Cauchy thích ứng phương sai nhiễu nền MAD
    float cauchyK = 12.0f;
    if (opts.noiseAdaptive) {
        cauchyK = std::clamp(4.0f * estimatedNoise * estimatedNoise, 6.0f, 40.0f);
    }

    std::vector<float> sharpL(nPixels);

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        int rowIdx = y * width;
        for (int x = 0; x < width; ++x) {
            int idx = rowIdx + x;
            float yCenter = luma[idx];
            float yBlur = blurL[idx];

            // 8 điểm lân cận (4 trực giao + 4 đường chéo) để chống răng cưa đường viền nghiêng
            float yLeft   = luma[rowIdx + std::max(0, x - 1)];
            float yRight  = luma[rowIdx + std::min(width - 1, x + 1)];
            float yTop    = luma[std::max(0, y - 1) * width + x];
            float yBottom = luma[std::min(height - 1, y + 1) * width + x];

            float yTL = luma[std::max(0, y - 1) * width + std::max(0, x - 1)];
            float yTR = luma[std::max(0, y - 1) * width + std::min(width - 1, x + 1)];
            float yBL = luma[std::min(height - 1, y + 1) * width + std::max(0, x - 1)];
            float yBR = luma[std::min(height - 1, y + 1) * width + std::min(width - 1, x + 1)];

            float minY = std::min({ yCenter, yLeft, yRight, yTop, yBottom, yTL, yTR, yBL, yBR });
            float maxY = std::max({ yCenter, yLeft, yRight, yTop, yBottom, yTL, yTR, yBL, yBR });
            float grad = std::abs(yRight - yLeft) + std::abs(yBottom - yTop);

            // Đo tính dị hướng đường chéo (Diagonal Anisotropy) chuẩn xác về toán học:
            // Cạnh thẳng (dọc/ngang): D1 ≈ D2 => |D1 - D2| ≈ 0 => isDiagonalEdge = 0 (KHÔNG nhầm lẫn cạnh thẳng!)
            // Cạnh nghiêng (45/135 độ): D1 >> D2 hoặc D2 >> D1 => isDiagonalEdge tiến tới 1.0
            float D1 = std::abs(yBR - yTL); // Đường chéo chính
            float D2 = std::abs(yTR - yBL); // Đường chéo phụ
            float diagAnisotropy = std::abs(D1 - D2) / (D1 + D2 + 1e-4f);
            float isDiagonalEdge = diagAnisotropy * std::clamp((D1 + D2 - 3.0f) / 8.0f, 0.0f, 1.0f);

            // Directional Coherence Gating (Phân biệt nét thực sự vs chấm nhiễu phân tán có khoảng cách)
            float orthoDiff = std::abs(std::abs(yRight - yLeft) - std::abs(yBottom - yTop));
            float orthoCoherence = orthoDiff / (grad + 1e-4f);
            float maxCoherence = std::max(orthoCoherence, diagAnisotropy);

            // Cauchy Continuous Coring thích ứng độ chói cục bộ (Luminance-Aware Cauchy Coring):
            // Vùng tối (Y < 75) có SNR thấp và cảm biến nhiều nhiễu hạt => tăng ngưỡng coring để chặn noise
            float lumFactor = 1.0f + 1.25f * std::max(0.0f, (75.0f - yCenter) / 75.0f);
            float localCauchyK = cauchyK * lumFactor;
            float edgeWeight = (grad * grad) / (grad * grad + localCauchyK) * opts.edgeSensitivity;

            // Chấm nhiễu phân tán có khoảng cách: grad trung bình nhưng không có tính định hướng tiếp tuyến
            if (grad < 20.0f) {
                float coherenceDamp = 0.50f + 0.50f * maxCoherence;
                edgeWeight *= coherenceDamp;
            }

            // Contrast Adaptive Sharpening (CAS) với Anti-Aliasing Damping (Triệt tiêu 100% răng cưa)
            float range = std::max(maxY - minY, 0.01f);
            float peak = std::min(yCenter - minY, maxY - yCenter) / range;
            float effectiveCasStrength = opts.casStrength * (1.0f - 0.40f * isDiagonalEdge);

            // High-Contrast Crest Limiter (Anti-gai cành cây, viền tán lá, sợi tóc ngược sáng)
            if (opts.crestLimiter && range > 45.0f) {
                float crestDamp = 1.0f - 0.35f * smoothstepVal(45.0f, 110.0f, range);
                effectiveCasStrength *= crestDamp;
            }

            float casFactor = 0.5f + 0.5f * peak * effectiveCasStrength;

            float diffY = (yCenter - yBlur) * opts.amount;

            // Topology Sống Nét / Rãnh Nét 8 hướng đồng bộ (Directional Extremum):
            // Một điểm là Sống Nét (Ridge) nếu là đỉnh theo trục ngang, trục dọc HOẶC một trong hai trục chéo
            bool isOrthogonalRidge = (yCenter >= yLeft && yCenter >= yRight) || (yCenter >= yTop && yCenter >= yBottom);
            bool isDiagonalRidge   = (yCenter >= yTL && yCenter >= yBR) || (yCenter >= yTR && yCenter >= yBL);
            bool isRidge = isOrthogonalRidge || isDiagonalRidge;

            bool isOrthogonalValley = (yCenter <= yLeft && yCenter <= yRight) || (yCenter <= yTop && yCenter <= yBottom);
            bool isDiagonalValley   = (yCenter <= yTL && yCenter <= yBR) || (yCenter <= yTR && yCenter <= yBL);
            bool isValley = isOrthogonalValley || isDiagonalValley;

            // Anti-Bloat Lateral Inhibition: Chỉ ghìm sườn dốc khi thực sự là sườn dốc đơn điệu 8 hướng
            if (!isValley && !isRidge && grad > 8.0f && opts.antiBloat) {
                diffY *= 0.85f; // Ghìm 15% ở sườn dốc, giữ chân nét cố định, bảo tồn khoảng dãn pha sub-pixel
            }

            float guidedTerm = diffGuided[idx];

            // JPEG Grid Anti-Spiking: Giảm kích nét trên đường phân chia ô 8x8 của ảnh nén
            if (opts.compressionBlockiness > 25.0f) {
                if ((x % 8 == 0 || (x + 1) % 8 == 0) || (y % 8 == 0 || (y + 1) % 8 == 0)) {
                    float gridDamp = 1.0f - std::min(opts.compressionBlockiness / 100.0f, 0.40f) * 0.70f;
                    diffY *= gridDamp;
                    guidedTerm *= gridDamp;
                }
            }

            // Thin-Stroke Protection (Chống phình nét mảnh, bảo toàn nét chữ & nét vẽ)
            if (opts.thinStrokeGate && (isRidge || isValley) && grad > 5.0f) {
                float strokeDamp = 1.0f - 0.35f * opts.strokeAnisotropy;
                diffY *= strokeDamp;
                guidedTerm *= strokeDamp;
            }

            // Triệt tiêu nano/micro noise trên da người trước khi đưa vào hàm nét với chuyển tiếp Hermite trơn tru (Anti-gai viền)
            float pSkin = skinMask[idx];
            float skinEdgeFactor = 0.0f;
            if (pSkin > 0.05f) {
                skinEdgeFactor = smoothstepVal(7.0f, 22.0f, grad);
                guidedTerm *= (1.0f - pSkin * (1.0f - 0.50f * skinEdgeFactor) * 0.85f);
                diffY *= (1.0f - pSkin * (1.0f - skinEdgeFactor) * 0.55f);
            }

            // Context-Aware Anti-Halo & Headroom Clamping
            bool isDocMode = opts.isDocument;
            float haloMargin;
            if (isDocMode) {
                // Tài liệu: kẹp chặt 4% + posDamp để triệt tiêu 100% sọc trắng quanh chữ
                float posMargin = std::max(0.0f, maxY - yCenter);
                float posDamp = std::clamp(posMargin / (range * 0.35f + 0.1f), 0.0f, 1.0f);
                if (diffY > 0.0f) diffY *= posDamp;
                if (guidedTerm > 0.0f) guidedTerm *= posDamp;
                haloMargin = range * 0.04f * opts.haloTolerance + 0.5f;
            } else {
                // Phong cảnh & Chân dung: Siết chặt hơn ở cạnh nghiêng để triệt tiêu hoàn toàn răng cưa bậc thang
                float diagClamp = 1.0f - 0.40f * isDiagonalEdge;
                haloMargin = range * 0.14f * opts.haloTolerance * diagClamp + 1.0f;
            }

            float res = yCenter + (diffY * casFactor + guidedTerm) * edgeWeight;
            res = std::clamp(res, minY - haloMargin, maxY + haloMargin);

            // Bảo vệ vùng bóng tối: không để CAS dìm pixel sâu hơn 85% luma gốc khi Y < 20
            if (yCenter < 20.0f && res < yCenter * 0.85f) {
                res = yCenter * 0.85f;
            }

            // Studio Beauty Dual-Zone: Mịn đều tone da nhưng bảo tồn vân lỗ chân lông tự nhiên (Anti-bệt da)
            // và giữ 100% độ nét cho mắt, mi, chân mày, tóc và viền môi (Anti-gai viền nhờ chuyển tiếp Hermite)
            if (pSkin > 0.05f) {
                float flatSkinWeight = (1.0f - skinEdgeFactor) * pSkin;
                if (flatSkinWeight > 0.01f) {
                    float baseSmooth = opts.isPortrait ? opts.skinSmooth : 0.45f;
                    float smoothStrength = baseSmooth * flatSkinWeight;

                    // Tone smoothing: làm mịn mảng sắc tố màu không đồng đều
                    // trong khi vi cấu trúc micro-relief (lỗ chân lông tự nhiên) được giữ lại nguyên bản
                    float toneSmoothed = yBlur;
                    float naturalPore = yCenter - microBase[idx];
                    float poreAmp = std::abs(naturalPore);
                    if (poreAmp > 6.0f) {
                        naturalPore *= (6.0f / poreAmp); // Nén mềm gai mụn / khuyết điểm lớn
                    }
                    float skinReconstructed = toneSmoothed + naturalPore * opts.skinPorePreserve;
                    res = res * (1.0f - smoothStrength) + skinReconstructed * smoothStrength;
                }
            }

            // Asymmetric S-Curve Micro-Contrast: Kéo tương phản midtones nhưng bảo vệ vùng tối Y < 35 chống dìm đen
            if (std::abs(opts.contrast - 1.0f) > 0.001f) {
                float norm = std::clamp(res / 255.0f, 0.0f, 1.0f);
                float shadowProtection = std::clamp((res - 35.0f) / 35.0f, 0.0f, 1.0f);
                float s = norm + (opts.contrast - 1.0f) * 1.5f * norm * (1.0f - norm) * (norm - 0.5f) * shadowProtection;
                float sVal = s * 255.0f;
                if (res < 35.0f && sVal < res) {
                    sVal = res; // Tuyệt đối không dìm tối khi đã ở vùng bóng râm
                }
                res = std::clamp(sVal, 0.0f, 255.0f);
            }

            sharpL[idx] = res;
        }
    }

    // 9. Recompose với YCbCr BT.601 Studio Gamut: Khóa góc Hue bất biến 100% & Soft Gamut Roll-off
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        uint8_t* dstRow = dst.data() + y * stride;
        int rowIdx = y * width;

        for (int x = 0; x < width; ++x) {
            int idx = rowIdx + x;
            float sharpY = sharpL[idx];
            float yCenter = luma[idx];

            float cb = chromaCb[idx] - 128.0f;
            float cr = chromaCr[idx] - 128.0f;

            if (yCenter > 0.5f) {
                float lumaRatio = sharpY / yCenter;
                // Tuyến tính hoá bù sắc độ (không dùng std::pow transcendent) để tăng tốc 3X và chống bão hòa màu quá đà
                float chromaExpansion = std::clamp(1.0f + (lumaRatio - 1.0f) * 0.25f, 0.88f, 1.15f);

                // Gamut Soft Roll-off: tối ưu không gọi std::pow(..., 4.0f)
                float chromaDist = std::sqrt(cb * cb + cr * cr);
                float cdNorm = chromaDist * (1.0f / 112.0f);
                float cd2 = cdNorm * cdNorm;
                float cd4 = cd2 * cd2;
                float rollOff = 1.0f - std::clamp(cd4, 0.0f, 0.5f);
                chromaExpansion = 1.0f + (chromaExpansion - 1.0f) * rollOff;

                cb *= chromaExpansion;
                cr *= chromaExpansion;
            }

            float r = sharpY + 1.402f * cr;
            float g = sharpY - 0.344136f * cb - 0.714136f * cr;
            float b = sharpY + 1.772f * cb;

            // Smart Vibrance
            if (opts.vibrance > 0.001f) {
                float maxVal = std::max({r, g, b});
                float minVal = std::min({r, g, b});
                float sat = (maxVal - minVal) / (maxVal + 0.001f);
                float boost = (1.0f - sat * 0.5f) * opts.vibrance;

                r += (r - sharpY) * boost;
                g += (g - sharpY) * boost;
                b += (b - sharpY) * boost;
            }

            // High Precision 16-bit Dither to prevent color quantization banding
            if (opts.enableDither) {
                // TPDF (Triangular Probability Density Function) spatial micro-dither
                float dither = (((x ^ (y * 17)) & 7) - 3.5f) / 28.0f;
                r += dither;
                g += dither;
                b += dither;
            }

            // Gamut Underflow Clamp & NaN Shield: triệt tiêu giá trị âm hoặc NaN
            if (std::isnan(r) || r < 0.0f) r = 0.0f;
            if (std::isnan(g) || g < 0.0f) g = 0.0f;
            if (std::isnan(b) || b < 0.0f) b = 0.0f;

            // Soft Gamut Roll-off: co tỉ lệ đồng đều cả 3 kênh nếu vượt 255
            float maxComponent = std::max({r, g, b});
            if (maxComponent > 255.0f) {
                float compression = 255.0f / maxComponent;
                r *= compression;
                g *= compression;
                b *= compression;
            }

            dstRow[x * 4 + 0] = (uint8_t)std::clamp((int)std::round(b), 0, 255);
            dstRow[x * 4 + 1] = (uint8_t)std::clamp((int)std::round(g), 0, 255);
            dstRow[x * 4 + 2] = (uint8_t)std::clamp((int)std::round(r), 0, 255);
            dstRow[x * 4 + 3] = alpha[idx];
        }
    }
}

// -------------------------------------------------------------
// Main enhanceImage API Implementation
// -------------------------------------------------------------
bool ImageEnhancerPro::enhanceImage(
    const std::string& inputPath,
    const std::string& outputPath,
    int level,
    ImageScorePro* outScore,
    std::string* outErrorMessage,
    EnhanceErrorPro* outErrorCode)
{
    auto setError = [&](EnhanceErrorPro code, const std::string& msg) {
        if (outErrorCode) *outErrorCode = code;
        if (outErrorMessage) *outErrorMessage = msg;
    };

    if (!fs::exists(inputPath)) {
        setError(EnhanceErrorPro::FileNotFound, "Không tìm thấy tệp ảnh nguồn: " + inputPath);
        return false;
    }

    CoInitialize(NULL);

    IWICImagingFactory* pFactory = NULL;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
    if (FAILED(hr) || !pFactory) {
        setError(EnhanceErrorPro::DecoderInitFailed, "Không thể khởi tạo WIC Imaging Factory của Windows.");
        CoUninitialize();
        return false;
    }

    std::wstring wInputPath = toWideString(inputPath);
    IWICBitmapDecoder* pDecoder = NULL;
    hr = pFactory->CreateDecoderFromFilename(wInputPath.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (FAILED(hr) || !pDecoder) {
        if (hr == 0x88982F50 /* WINCODEC_ERR_COMPONENTNOTFOUND */ || hr == REGDB_E_CLASSNOTREG) {
            setError(EnhanceErrorPro::CodecNotFoundOrUnsupported,
                "Hệ thống Windows thiếu Codec WIC để giải mã định dạng ảnh này (cần cài đặt HEIF Image Extensions hoặc Raw Image Extension từ Microsoft Store).");
        } else {
            char hexBuf[32];
            sprintf_s(hexBuf, "0x%08lX", (unsigned long)hr);
            setError(EnhanceErrorPro::DecoderInitFailed,
                std::string("Không thể khởi tạo bộ giải mã WIC cho tệp ảnh (Mã lỗi HRESULT: ") + hexBuf + ").");
        }
        pFactory->Release();
        CoUninitialize();
        return false;
    }

    IWICBitmapFrameDecode* pFrame = NULL;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr) || !pFrame) {
        setError(EnhanceErrorPro::FrameDecodeFailed, "Không thể giải mã khung hình ảnh thứ nhất (Frame 0).");
        pDecoder->Release();
        pFactory->Release();
        CoUninitialize();
        return false;
    }

    IWICFormatConverter* pConverter = NULL;
    pFactory->CreateFormatConverter(&pConverter);
    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        setError(EnhanceErrorPro::FormatConversionFailed, "Không thể chuyển đổi định dạng pixel sang 32bpp BGRA.");
        pConverter->Release();
        pFrame->Release();
        pDecoder->Release();
        pFactory->Release();
        CoUninitialize();
        return false;
    }

    UINT origW = 0, origH = 0;
    pConverter->GetSize(&origW, &origH);
    if (origW == 0 || origH == 0) {
        pConverter->Release();
        pFrame->Release();
        pDecoder->Release();
        pFactory->Release();
        CoUninitialize();
        return false;
    }

    double dpiX = 96.0, dpiY = 96.0;
    pFrame->GetResolution(&dpiX, &dpiY);

    UINT origStride = origW * 4;
    std::vector<uint8_t> srcPixels(origH * origStride);
    hr = pConverter->CopyPixels(NULL, origStride, (UINT)srcPixels.size(), srcPixels.data());
    pConverter->Release();

    if (FAILED(hr)) {
        pFrame->Release();
        pDecoder->Release();
        pFactory->Release();
        CoUninitialize();
        return false;
    }

    uintmax_t inFileSize = fs::exists(inputPath) ? fs::file_size(inputPath) : 0;
    ImageScorePro score = analyzeImageBufferPro(srcPixels, origW, origH, origStride, inFileSize);
    EnhanceOptionsPro opts;

    if (level <= 0) {
        // --- CHẾ ĐỘ NỘI SUY, TỰ ĐÁNH GIÁ ĐA GÓC ĐỘ & RENDER THÍCH ỨNG ---
        opts = computeAdaptiveOptions(score);
    } else {
        opts = getPresetPro(level);
    }
    opts.sanitize();

    UINT procW = origW;
    UINT procH = origH;
    UINT procStride = origStride;
    std::vector<uint8_t> scaledPixels;

    // Lanczos-3 Super-Sampling (Nội suy thích ứng)
    if (opts.scalePercent > 100) {
        procW = (UINT)std::round(origW * (opts.scalePercent / 100.0));
        procH = (UINT)std::round(origH * (opts.scalePercent / 100.0));
        procStride = procW * 4;
        scaledPixels = lanczos3Resample(srcPixels, origW, origH, origStride, procW, procH, procStride);
    } else {
        scaledPixels = std::move(srcPixels);
    }

    score.scalePercent = opts.scalePercent;
    score.procW = procW;
    score.procH = procH;

    bool isDoc = (score.detectedType == "Tài liệu / Văn bản (Document / Text)");
    if (isDoc) {
        score.renderStrategy = "Tài liệu / Văn bản Pro (CLAHE tương phản sâu + Chữ sắc lẹm + Asymmetric Anti-Halo)";
    } else if (opts.isPortrait) {
        score.renderStrategy = "Chân dung Studio Pro (Mịn da tự nhiên + Nano Layer + Asymmetric Anti-Halo)";
    } else if (score.detectedType == "Ảnh mờ / Cần phục hồi nét (Blur/Defocus)") {
        score.renderStrategy = "Phục hồi nét mờ Pro (Multi-Scale De-blur + High Acutance CAS + Contrast Lift)";
    } else {
        score.renderStrategy = "Phong cảnh / Đa dụng Pro (3-Scale Guided Filter + Adaptive CLAHE + Texture Boost)";
    }
    if (outScore) *outScore = score;

    // Pro Pipeline Processing
    std::vector<uint8_t> dstPixels(procH * procStride);
    processSharpenPro(scaledPixels, dstPixels, procW, procH, procStride, opts, score.noiseFloor);

    // Save output via WIC Encoder
    std::string ext = fs::path(outputPath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    GUID containerFormat = GUID_ContainerFormatJpeg;
    if (ext == ".png") {
        containerFormat = GUID_ContainerFormatPng;
    } else if (ext == ".bmp") {
        containerFormat = GUID_ContainerFormatBmp;
    } else if (ext == ".tif" || ext == ".tiff") {
        containerFormat = GUID_ContainerFormatTiff;
    }

    IWICStream* pStream = NULL;
    hr = pFactory->CreateStream(&pStream);
    std::wstring wOutputPath = toWideString(outputPath);
    if (SUCCEEDED(hr)) {
        hr = pStream->InitializeFromFilename(wOutputPath.c_str(), GENERIC_WRITE);
    }

    IWICBitmapEncoder* pEncoder = NULL;
    if (SUCCEEDED(hr)) {
        hr = pFactory->CreateEncoder(containerFormat, NULL, &pEncoder);
    }
    if (SUCCEEDED(hr)) {
        hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
    }

    IWICBitmapFrameEncode* pFrameEncode = NULL;
    IPropertyBag2* pPropertyBag = NULL;
    if (SUCCEEDED(hr)) {
        hr = pEncoder->CreateNewFrame(&pFrameEncode, &pPropertyBag);
    }

    if (SUCCEEDED(hr)) {
        if (containerFormat == GUID_ContainerFormatJpeg) {
            PROPBAG2 optProp = { 0 };
            optProp.pstrName = (LPOLESTR)L"ImageQuality";
            VARIANT varVal;
            VariantInit(&varVal);
            varVal.vt = VT_R4;
            varVal.fltVal = 0.96f; // Studio Grade Quality
            pPropertyBag->Write(1, &optProp, &varVal);
        }
        hr = pFrameEncode->Initialize(pPropertyBag);
    }

    // Sao chép nguyên vẹn khối Metadata (EXIF, GPS, Giờ chụp, Model máy ảnh, XMP) từ ảnh gốc
    if (SUCCEEDED(hr)) {
        IWICMetadataBlockReader* pBlockReader = NULL;
        IWICMetadataBlockWriter* pBlockWriter = NULL;
        if (SUCCEEDED(pFrame->QueryInterface(IID_PPV_ARGS(&pBlockReader))) &&
            SUCCEEDED(pFrameEncode->QueryInterface(IID_PPV_ARGS(&pBlockWriter)))) {
            pBlockWriter->InitializeFromBlockReader(pBlockReader);
        }
        if (pBlockReader) pBlockReader->Release();
        if (pBlockWriter) pBlockWriter->Release();
    }

    WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
    if (SUCCEEDED(hr)) {
        hr = pFrameEncode->SetSize(procW, procH);
        float scaleFactor = opts.scalePercent / 100.0f;
        pFrameEncode->SetResolution(dpiX * scaleFactor, dpiY * scaleFactor);

        hr = pFrameEncode->SetPixelFormat(&pixelFormat);
    }

    if (SUCCEEDED(hr)) {
        if (pixelFormat == GUID_WICPixelFormat32bppBGRA) {
            hr = pFrameEncode->WritePixels(procH, procStride, (UINT)dstPixels.size(), dstPixels.data());
        } else {
            // Fallback sang BGR 24bpp khi encoder JPEG chỉ nhận 24bpp
            UINT bgrStride = procW * 3;
            std::vector<uint8_t> bgrPixels(procH * bgrStride);
            #pragma omp parallel for schedule(static)
            for (UINT y = 0; y < procH; ++y) {
                UINT srcRow = y * procStride;
                UINT dstRow = y * bgrStride;
                for (UINT x = 0; x < procW; ++x) {
                    UINT sp = srcRow + x * 4;
                    UINT dp = dstRow + x * 3;
                    bgrPixels[dp]     = dstPixels[sp];     // B
                    bgrPixels[dp + 1] = dstPixels[sp + 1]; // G
                    bgrPixels[dp + 2] = dstPixels[sp + 2]; // R
                }
            }
            WICPixelFormatGUID bgrFormat = GUID_WICPixelFormat24bppBGR;
            pFrameEncode->SetPixelFormat(&bgrFormat);
            hr = pFrameEncode->WritePixels(procH, bgrStride, (UINT)bgrPixels.size(), bgrPixels.data());
        }
    }

    if (SUCCEEDED(hr)) {
        hr = pFrameEncode->Commit();
    }
    if (SUCCEEDED(hr)) {
        hr = pEncoder->Commit();
    }

    // Cleanup COM pointers
    if (pPropertyBag) pPropertyBag->Release();
    if (pFrameEncode) pFrameEncode->Release();
    if (pEncoder) pEncoder->Release();
    if (pStream) pStream->Release();
    pFrame->Release();
    pDecoder->Release();
    pFactory->Release();
    CoUninitialize();

    // Đồng bộ ngày giờ tạo/sửa đổi tệp tin trên hệ điều hành trùng khớp ảnh gốc
    if (SUCCEEDED(hr)) {
        setError(EnhanceErrorPro::Success, "Xử lý nâng cao chất lượng ảnh thành công.");
        try {
            if (fs::exists(inputPath) && fs::exists(outputPath)) {
                fs::last_write_time(outputPath, fs::last_write_time(inputPath));
            }
        } catch (...) {}
    } else {
        setError(EnhanceErrorPro::EncodingFailed, "Lỗi khi ghi và mã hóa tệp ảnh kết quả qua bộ mã hóa WIC.");
    }

    return SUCCEEDED(hr);
}

ImageScorePro ImageEnhancerPro::analyzeImageFile(const std::string& filePath) {
    if (!fs::exists(filePath)) return ImageScorePro();

    CoInitialize(NULL);
    IWICImagingFactory* pFactory = NULL;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
    if (FAILED(hr) || !pFactory) {
        CoUninitialize();
        return ImageScorePro();
    }

    std::wstring wPath = toWideString(filePath);
    IWICBitmapDecoder* pDecoder = NULL;
    hr = pFactory->CreateDecoderFromFilename(wPath.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (FAILED(hr) || !pDecoder) {
        pFactory->Release();
        CoUninitialize();
        return ImageScorePro();
    }

    IWICBitmapFrameDecode* pFrame = NULL;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr) || !pFrame) {
        pDecoder->Release();
        pFactory->Release();
        CoUninitialize();
        return ImageScorePro();
    }

    IWICFormatConverter* pConverter = NULL;
    pFactory->CreateFormatConverter(&pConverter);
    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        pConverter->Release();
        pFrame->Release();
        pDecoder->Release();
        pFactory->Release();
        CoUninitialize();
        return ImageScorePro();
    }

    UINT w = 0, h = 0;
    pConverter->GetSize(&w, &h);
    if (w == 0 || h == 0) {
        pConverter->Release();
        pFrame->Release();
        pDecoder->Release();
        pFactory->Release();
        CoUninitialize();
        return ImageScorePro();
    }

    UINT stride = w * 4;
    std::vector<uint8_t> pixels(h * stride);
    hr = pConverter->CopyPixels(NULL, stride, (UINT)pixels.size(), pixels.data());
    pConverter->Release();
    pFrame->Release();
    pDecoder->Release();
    pFactory->Release();
    CoUninitialize();

    if (FAILED(hr)) return ImageScorePro();
    uintmax_t sz = fs::exists(filePath) ? fs::file_size(filePath) : 0;
    return analyzeImageBufferPro(pixels, w, h, stride, sz);
}
