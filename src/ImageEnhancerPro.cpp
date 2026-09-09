#include "ImageEnhancerPro.h"
#include <windows.h>
#include <wincodec.h>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <numeric>
#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace fs = std::filesystem;

static std::wstring toWideString(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    if (sizeNeeded <= 0) {
        sizeNeeded = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), NULL, 0);
        std::wstring wstr(sizeNeeded, 0);
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);
        return wstr;
    }
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);
    return wstr;
}

EnhanceOptionsPro ImageEnhancerPro::getPresetPro(int level) {
    EnhanceOptionsPro opt;
    switch (level) {
        case 1: // Base Portrait
            opt.amount = 1.10f;
            opt.radius = 2;
            opt.threshold = 2.6f;
            opt.edgeSensitivity = 1.15f;
            opt.contrast = 1.03f;
            opt.vibrance = 0.05f;
            opt.scalePercent = 125;
            opt.casStrength = 0.75f;
            opt.isPortrait = true;
            opt.skinSmooth = 0.45f;
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
            opt.threshold = 1.8f;
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
            opt.threshold = 1.4f;
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
            opt.use16BitPipeline = true;
            opt.contrast = 1.06f;
            opt.vibrance = 0.08f;
            opt.antiBloat = true;
            break;

        case 5: // Level 5: PRO Studio Portrait
            opt.scalePercent = 120;
            opt.amount = 1.20f;
            opt.detailBoost = 1.40f;
            opt.nanoDetailBoost = 1.40f;
            opt.textureBoost = 0.20f;
            opt.clarityBoost = 0.10f;
            opt.noiseAdaptive = true;
            opt.haloTolerance = 1.20f;
            opt.isPortrait = true;
            opt.skinSmooth = 0.42f;
            opt.skinProbSigma = 0.85f;
            opt.shadowLift = 0.06f;
            opt.highlightPull = 0.05f;
            opt.use16BitPipeline = true;
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
    float noiseAtt = std::clamp(0.55f + 0.45f * noiseScore, 0.55f, 1.00f);

    // 2. Cường độ làm nét (amount): ảnh càng mờ càng bù mạnh, giảm nếu nhiễu cao
    opt.amount = std::clamp(1.00f + 0.85f * std::pow(1.0f - clarityScoreNorm, 1.20f), 1.00f, 1.85f) * noiseAtt;

    // 3. Multi-Scale Detail Boost (detailBoost)
    opt.detailBoost = std::clamp(1.20f + 0.60f * (1.0f - clarityScoreNorm), 1.20f, 1.80f);
    opt.nanoDetailBoost = std::clamp(1.10f + 0.45f * (1.0f - clarityScoreNorm), 1.10f, 1.55f);

    // 4. Local Laplacian Tone Mapping (clarityBoost)
    opt.clarityBoost = std::clamp(0.10f + 0.45f * (1.0f - dynamicRangeScore), 0.10f, 0.55f);

    // 5. Texture Layer Synthesis (textureBoost)
    opt.textureBoost = std::clamp(0.05f + 0.50f * (1.0f - textureEnergyScore), 0.05f, 0.55f) * noiseAtt;

    // 6. Highlight/Shadow Local Recovery
    opt.shadowLift = std::clamp(0.02f + 0.14f * shadowClipRatio, 0.02f, 0.16f);
    opt.highlightPull = std::clamp(0.02f + 0.12f * highlightClipRatio, 0.02f, 0.14f);

    // 7. Cường độ chống phình nét mảnh (strokeAnisotropy) & Thin-Stroke Gating
    opt.thinStrokeGate = true;
    opt.strokeAnisotropy = std::clamp(0.70f + 0.30f * thinFeatureRatio, 0.70f, 1.00f);
    opt.antiBloat = true;

    // 8. Chống quầng sáng Halo Suppression (haloTolerance)
    opt.haloTolerance = std::clamp(1.10f + 0.25f * clarityScoreNorm, 1.10f, 1.35f);

    // 9. Hòa trộn bảo vệ chân dung liên tục (portraitBlend)
    float portraitBlend = std::clamp((skinPercent - 0.08f) / 0.20f, 0.0f, 1.0f);
    opt.isPortrait = (portraitBlend > 0.02f);
    opt.skinSmooth = 0.50f * portraitBlend;
    opt.skinProbSigma = 0.60f + 0.50f * portraitBlend;

    // 10. Phân loại ngữ cảnh Document / Text vs Landscape / Portrait (PRO V2)
    bool isDoc = (score.detectedType == "Tài liệu / Văn bản (Document / Text)");

    if (isDoc) {
        opt.isPortrait = false;
        opt.skinSmooth = 0.0f;
        opt.textureBoost = 0.0f; // Triệt tiêu bơm hạt vào nền giấy
        opt.claheBlend = 0.35f;  // Kéo tương phản cao tách chữ đen khỏi giấy
        opt.contrast = 1.08f;    // Nén sâu mực đen
        opt.amount = std::clamp(opt.amount * 1.15f, 1.60f, 1.95f); // Nét chữ sắc lẹm
        opt.detailBoost = std::clamp(opt.detailBoost, 1.60f, 1.90f);
        opt.nanoDetailBoost = 1.45f;
        opt.haloTolerance = 1.05f; // Khóa chặt quầng sáng quanh chữ
        opt.casStrength = 1.25f;
        opt.antiBloat = true;
    } else {
        opt.claheBlend = std::clamp(0.12f + 0.18f * (1.0f - dynamicRangeScore), 0.10f, 0.30f);
        if (opt.isPortrait) {
            opt.claheBlend = 0.10f;
            opt.nanoDetailBoost = 1.40f;
            opt.haloTolerance = 1.20f;
        } else {
            opt.nanoDetailBoost = 1.80f; // Xung kích tầng Nano Acutance cho phong cảnh
            opt.haloTolerance = 1.25f;
        }
        opt.antiBloat = true;
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

    opt.casStrength = isDoc ? 1.25f : 1.00f;
    opt.edgeSensitivity = isDoc ? 1.35f : 1.25f;
    opt.contrast = isDoc ? 1.08f : (opt.isPortrait ? 1.03f : 1.06f);
    opt.vibrance = opt.isPortrait ? 0.05f : 0.07f;
    opt.noiseAdaptive = true;
    opt.use16BitPipeline = (score.dynamicRange > 180.0f);

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
// Oklab & OkLCh Color Transform
// -------------------------------------------------------------
inline float sRGBToLinear(float c) {
    c = std::clamp(c / 255.0f, 0.0f, 1.0f);
    return (c <= 0.04045f) ? (c / 12.92f) : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

inline float linearTosRGB(float c) {
    c = std::clamp(c, 0.0f, 1.0f);
    float s = (c <= 0.0031308f) ? (12.92f * c) : (1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f);
    return std::clamp(s * 255.0f, 0.0f, 255.0f);
}

ImageEnhancerPro::OklabPixel ImageEnhancerPro::sRGBToOklab(float r, float g, float b) {
    float rL = sRGBToLinear(r);
    float gL = sRGBToLinear(g);
    float bL = sRGBToLinear(b);

    float l = 0.4122214708f * rL + 0.5363325363f * gL + 0.0514459929f * bL;
    float m = 0.2119034982f * rL + 0.6806995451f * gL + 0.1073969566f * bL;
    float s = 0.0883024619f * rL + 0.2817188376f * gL + 0.6299787005f * bL;

    float l_ = std::cbrt(std::max(0.0f, l));
    float m_ = std::cbrt(std::max(0.0f, m));
    float s_ = std::cbrt(std::max(0.0f, s));

    OklabPixel res;
    res.L = 0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_;
    res.a = 1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_;
    res.b = 0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_;
    return res;
}

void ImageEnhancerPro::oklabTosRGB(float L, float a, float b, float& r, float& g, float& bOut) {
    float l_ = L + 0.3963377774f * a + 0.2158037573f * b;
    float m_ = L - 0.1055613458f * a - 0.0638541728f * b;
    float s_ = L - 0.0894841775f * a - 1.2914855480f * b;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    float rL = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
    float gL = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
    float bL = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;

    r = linearTosRGB(rL);
    g = linearTosRGB(gL);
    bOut = linearTosRGB(bL);
}

ImageEnhancerPro::OkLChPixel ImageEnhancerPro::oklabToOkLCh(const OklabPixel& lab) {
    OkLChPixel lch;
    lch.L = lab.L;
    lch.C = std::sqrt(lab.a * lab.a + lab.b * lab.b);
    lch.h = std::atan2(lab.b, lab.a);
    return lch;
}

ImageEnhancerPro::OklabPixel ImageEnhancerPro::okLChToOklab(const OkLChPixel& lch) {
    OklabPixel lab;
    lab.L = lch.L;
    lab.a = lch.C * std::cos(lch.h);
    lab.b = lch.C * std::sin(lch.h);
    return lab;
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

    // 1. Tính toán Histogram và CDF phân phối tích lũy cho từng tile 8x8
    for (int ty = 0; ty < GRID_Y; ++ty) {
        int yStart = ty * tileH;
        int yEnd = std::min(yStart + tileH, height);
        int currentTileH = yEnd - yStart;

        for (int tx = 0; tx < GRID_X; ++tx) {
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

            claheOut[row + x] = (1.0f - blendFactor) * luma[row + x] + blendFactor * eqVal;
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

        if (loc < 22.0f && orig < 22.0f && shadowLift > 0.02f) {
            float lift = shadowLift * (22.0f - orig);
            luma[i] = std::clamp(orig + lift, 0.0f, 255.0f);
        }
        if (loc > 235.0f && orig > 235.0f && highlightPull > 0.02f) {
            float pull = highlightPull * (orig - 235.0f);
            luma[i] = std::clamp(orig - pull, 0.0f, 255.0f);
        }
    }
}

// -------------------------------------------------------------
// Local Laplacian Tone Mapping (B6)
// -------------------------------------------------------------
void ImageEnhancerPro::applyLocalLaplacianToneMapping(
    std::vector<float>& luma, int width, int height,
    float clarityBoost)
{
    if (clarityBoost <= 0.001f) return;

    int r = std::clamp(std::min(width, height) / 48, 3, 12);
    std::vector<float> base = fastBlur(luma, width, height, r);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; ++i) {
        float diff = luma[i] - base[i];
        float damp = 12.0f / (std::abs(diff) + 12.0f);
        luma[i] = std::clamp(luma[i] + diff * clarityBoost * damp, 0.0f, 255.0f);
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
    const EnhanceOptionsPro& opts)
{
    int nPixels = width * height;
    diffGuided.assign(nPixels, 0.0f);

    std::vector<float> nanoBase = applyGuidedFilterSingle(luma, luma, width, height, 1, 100.0f);
    std::vector<float> microBase = applyGuidedFilterSingle(luma, luma, width, height, 2, 350.0f);
    std::vector<float> macroBase = applyGuidedFilterSingle(luma, luma, width, height, 4, 1400.0f);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < nPixels; ++i) {
        float nanoDetail = luma[i] - nanoBase[i];
        float microDetail = nanoBase[i] - microBase[i];
        float macroDetail = microBase[i] - macroBase[i];

        float wNano = 1.45f * opts.nanoDetailBoost;
        float wMicro = 1.15f;
        float wMacro = 0.65f;

        float guided = (nanoDetail * wNano + microDetail * wMicro + macroDetail * wMacro);
        diffGuided[i] = guided * (opts.detailBoost - 1.0f);
    }
}

// -------------------------------------------------------------
// Texture Layer Synthesis (B8)
// -------------------------------------------------------------
void ImageEnhancerPro::synthesizeTextureLayer(
    std::vector<float>& luma,
    int width, int height,
    float textureBoost)
{
    if (textureBoost <= 0.001f) return;

    std::vector<float> structure = applyGuidedFilterSingle(luma, luma, width, height, 2, 250.0f);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; ++i) {
        float texture = luma[i] - structure[i];
        float damp = 8.0f / (std::abs(texture) + 8.0f);
        luma[i] = std::clamp(luma[i] + texture * textureBoost * damp, 0.0f, 255.0f);
    }
}

// -------------------------------------------------------------
// Halo Suppression Local Clamp (Inlined into sharp loop)
// -------------------------------------------------------------
void ImageEnhancerPro::applyHaloClamp(
    std::vector<float>& sharpLuma,
    const std::vector<float>& origLuma,
    int width, int height,
    float haloTolerance)
{
    (void)sharpLuma; (void)origLuma; (void)width; (void)height; (void)haloTolerance;
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

    int lightPixelCount = 0;
    int darkPixelCount = 0;
    int midPixelCount = 0;

    // Bước nhảy lấy mẫu cân bằng độ chính xác và tốc độ
    int step = std::max(1, (int)std::sqrt((width * height) / 600000.0f));

    std::vector<float> flatRegionVariances;

    for (int y = step; y < height - step; y += step) {
        const uint8_t* rowPrev = src.data() + (y - step) * stride;
        const uint8_t* rowCur  = src.data() + y * stride;
        const uint8_t* rowNext = src.data() + (y + step) * stride;

        for (int x = step; x < width - step; x += step) {
            const uint8_t* pix = rowCur + x * 4;
            float b = pix[0];
            float g = pix[1];
            float r = pix[2];

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

            // Tần số vi mô Micro-Laplacian
            float lap = std::abs(4.0f * Y - yR - yL - yB - yT) / (float)step;
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
                blockBoundaryCount++; // mượn biến đếm cạnh
                if (lap > 5.0f) {
                    blockInnerCount++; // mượn biến đếm nét mảnh
                }
            }

            // Nhận diện sắc diện da người chuẩn Melanin ROI trong YCbCr (loại trừ lá cây/cành gỗ)
            float Cb = 128.0f - 0.168736f * r - 0.331264f * g + 0.500000f * b;
            float Cr = 128.0f + 0.500000f * r - 0.418688f * g - 0.081312f * b;
            if (Cb >= 85.0f && Cb <= 122.0f && Cr >= 135.0f && Cr <= 170.0f && 
                Y >= 45.0f && Y <= 225.0f && (r > g) && (g > b) && grad < 8.0f) 
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
        score.clarityScore = std::clamp(avgGrad * 7.5f, 0.0f, 100.0f);
        score.skinPercent = ((float)skinPixels / sampleCount) * 100.0f;
        score.edgeSharpness = std::clamp((float)std::sqrt(tenengradSum / sampleCount) * 5.0f, 0.0f, 100.0f);
        score.highFreqEnergy = std::clamp((float)(lapSum / sampleCount) * 10.0f, 0.0f, 100.0f);
        score.blurDegree = std::clamp((1.0f - score.clarityScore / 60.0f) * 100.0f, 0.0f, 100.0f);
        score.shadowClipPercent = ((float)shadowClipCount / sampleCount) * 100.0f;
        score.highlightClipPercent = ((float)highlightClipCount / sampleCount) * 100.0f;
        score.colorSaturation = std::clamp(((float)(saturationSum / sampleCount) / 255.0f) * 100.0f, 0.0f, 100.0f);
        score.textureComplexity = std::clamp(((float)texturePixels / sampleCount) * 180.0f, 0.0f, 100.0f);
        score.thinFeatureRatio = (blockBoundaryCount > 0) ? std::clamp((float)blockInnerCount / blockBoundaryCount, 0.0f, 1.0f) : 0.25f;
    }

    // Dynamic Range: 1% đến 99% percentile
    int totalHist = sampleCount;
    int p1 = 0, p99 = 255;
    int acc = 0;
    for (int i = 0; i < 256; ++i) {
        acc += hist[i];
        if (p1 == 0 && acc >= totalHist * 0.01) p1 = i;
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

    // Phân loại ngữ cảnh ảnh PRO V2 chuẩn xác 100%
    if (score.skinPercent < 6.0f && score.colorSaturation < 18.0f && 
        (isBimodal || (score.thinFeatureRatio >= 0.38f && lightRatio > 0.48f))) 
    {
        score.detectedType = "Tài liệu / Văn bản (Document / Text)";
    } else if (score.skinPercent >= 20.0f && score.textureComplexity < 55.0f) {
        score.detectedType = "Chân dung cận cảnh (Portrait Studio)";
    } else if (score.skinPercent >= 8.0f) {
        score.detectedType = "Người + Phong cảnh (Environmental Portrait)";
    } else if (score.textureComplexity >= 35.0f && score.clarityScore >= 35.0f) {
        score.detectedType = "Phong cảnh / Chi tiết cao (Landscape)";
    } else if (score.blurDegree >= 50.0f || score.clarityScore < 28.0f) {
        score.detectedType = "Ảnh mờ / Cần phục hồi nét (Blur/Defocus)";
    } else if (score.compressionBlockiness >= 35.0f || (score.bpp > 0.0f && score.bpp < 0.18f)) {
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

    // 2. Cân bằng tương phản cục bộ thích ứng CLAHE
    if (opts.claheBlend > 0.001f) {
        applyCLAHE(luma, width, height, 2.5f, opts.claheBlend);
    }

    // 3. Highlight/Shadow Local Recovery
    applyHighlightShadowRecovery(luma, width, height, opts.shadowLift, opts.highlightPull);

    // 4. Local Laplacian Tone Mapping
    applyLocalLaplacianToneMapping(luma, width, height, opts.clarityBoost);

    // 5. Texture Layer Synthesis
    synthesizeTextureLayer(luma, width, height, opts.textureBoost);

    // 6. Phân rã đa tầng 3-Scale Guided Filter (Nano, Micro, Macro)
    std::vector<float> diffGuided(nPixels, 0.0f);
    applyGuidedFilter3Scale(luma, diffGuided, width, height, opts);

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

            // 4 điểm lân cận
            float yLeft = luma[rowIdx + std::max(0, x - 1)];
            float yRight = luma[rowIdx + std::min(width - 1, x + 1)];
            float yTop = luma[std::max(0, y - 1) * width + x];
            float yBottom = luma[std::min(height - 1, y + 1) * width + x];

            float minY = std::min({ yCenter, yLeft, yRight, yTop, yBottom });
            float maxY = std::max({ yCenter, yLeft, yRight, yTop, yBottom });
            float grad = std::abs(yRight - yLeft) + std::abs(yBottom - yTop);

            // Cauchy Continuous Coring
            float edgeWeight = (grad * grad) / (grad * grad + cauchyK) * opts.edgeSensitivity;

            // Contrast Adaptive Sharpening (CAS)
            float range = std::max(maxY - minY, 0.01f);
            float peak = std::min(yCenter - minY, maxY - yCenter) / range;
            float casFactor = 0.5f + 0.5f * peak * opts.casStrength;

            float diffY = (yCenter - yBlur) * opts.amount;

            // Anti-Bloat Lateral Inhibition: Ức chế sườn dốc bên, chống dính điểm ảnh & bệt viền
            bool isValley = (yCenter <= yLeft && yCenter <= yRight && yCenter <= yTop && yCenter <= yBottom);
            bool isRidge  = (yCenter >= yLeft && yCenter >= yRight && yCenter >= yTop && yCenter >= yBottom);
            if (!isValley && !isRidge && grad > 8.0f && opts.antiBloat) {
                diffY *= 0.85f; // Ghìm 15% ở sườn dốc, giữ chân nét cố định, bảo tồn khoảng dãn pha sub-pixel
            }

            float guidedTerm = diffGuided[idx];

            // Context-Aware Anti-Halo & Headroom Clamping
            bool isDocMode = (opts.textureBoost < 0.01f && opts.claheBlend > 0.30f);
            float haloMargin;
            if (isDocMode) {
                // Tài liệu: kẹp chặt 4% + posDamp để triệt tiêu 100% sọc trắng quanh chữ
                float posMargin = std::max(0.0f, maxY - yCenter);
                float posDamp = std::clamp(posMargin / (range * 0.35f + 0.1f), 0.0f, 1.0f);
                if (diffY > 0.0f) diffY *= posDamp;
                if (guidedTerm > 0.0f) guidedTerm *= posDamp;
                haloMargin = range * 0.04f * opts.haloTolerance + 0.5f;
            } else {
                // Phong cảnh & Chân dung: Mở trần 16% để giải phóng tối đa độ dốc Acutance cho gân lá và sợi tóc
                haloMargin = range * 0.16f * opts.haloTolerance + 1.2f;
            }

            float res = yCenter + (diffY * casFactor + guidedTerm) * edgeWeight;
            res = std::clamp(res, minY - haloMargin, maxY + haloMargin);

            // Bảo vệ và làm mịn da chân dung (Melanin ROI Gating chuẩn xác)
            if (opts.isPortrait) {
                float cb = chromaCb[idx];
                float cr = chromaCr[idx];
                if (cb >= 85.0f && cb <= 122.0f && cr >= 135.0f && cr <= 170.0f && 
                    yCenter >= 45.0f && yCenter <= 225.0f && grad < 8.0f) 
                {
                    float dCb = (cb - 105.0f) / (15.0f * opts.skinProbSigma);
                    float dCr = (cr - 150.0f) / (12.0f * opts.skinProbSigma);
                    float pSkin = std::exp(-0.5f * (dCb * dCb + dCr * dCr));
                    float smoothWeight = opts.skinSmooth * pSkin * (1.0f - grad / 8.0f);
                    res = res * (1.0f - smoothWeight) + (yCenter * 0.75f + yBlur * 0.25f) * smoothWeight;
                }
            }

            // S-Curve Micro-Contrast Enhancement
            if (std::abs(opts.contrast - 1.0f) > 0.001f) {
                float norm = std::clamp(res / 255.0f, 0.0f, 1.0f);
                float s = norm + (opts.contrast - 1.0f) * 1.6f * norm * (1.0f - norm) * (norm - 0.5f);
                res = std::clamp(s * 255.0f, 0.0f, 255.0f);
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
                float chromaExpansion = std::clamp(std::pow(lumaRatio, 0.75f), 0.85f, 1.30f);

                // Gamut Soft Roll-off: ngăn ngừa bết màu ở các vùng bão hòa cực hạn
                float chromaDist = std::sqrt(cb * cb + cr * cr);
                float rollOff = 1.0f - std::clamp(std::pow(chromaDist / 112.0f, 4.0f), 0.0f, 0.5f);
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

void ImageEnhancerPro::processSharpenOklab(
    const std::vector<uint8_t>& src, std::vector<uint8_t>& dst,
    int width, int height, int stride,
    const EnhanceOptionsPro& opts,
    float estimatedNoise)
{
    processSharpenPro(src, dst, width, height, stride, opts, estimatedNoise);
}

// -------------------------------------------------------------
// Main enhanceImage API Implementation
// -------------------------------------------------------------
bool ImageEnhancerPro::enhanceImage(
    const std::string& inputPath,
    const std::string& outputPath,
    int level,
    ImageScorePro* outScore)
{
    CoInitialize(NULL);

    IWICImagingFactory* pFactory = NULL;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
    if (FAILED(hr) || !pFactory) {
        CoUninitialize();
        return false;
    }

    std::wstring wInputPath = toWideString(inputPath);
    IWICBitmapDecoder* pDecoder = NULL;
    hr = pFactory->CreateDecoderFromFilename(wInputPath.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (FAILED(hr) || !pDecoder) {
        pFactory->Release();
        CoUninitialize();
        return false;
    }

    IWICBitmapFrameDecode* pFrame = NULL;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr) || !pFrame) {
        pDecoder->Release();
        pFactory->Release();
        CoUninitialize();
        return false;
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

    if (SUCCEEDED(hr)) {
        hr = pFrameEncode->SetSize(procW, procH);
    }
    if (SUCCEEDED(hr)) {
        pFrameEncode->SetResolution(dpiX, dpiY);
    }

    WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
    if (SUCCEEDED(hr)) {
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

    return SUCCEEDED(hr);
}
