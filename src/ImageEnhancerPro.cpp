#include "ImageEnhancerPro.h"
#include <windows.h>
#include <wincodec.h>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <numeric>

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
            opt.nanoDetailBoost = 1.35f;
            opt.textureBoost = 0.25f;
            opt.clarityBoost = 0.20f;
            opt.haloTolerance = 1.15f;
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
            opt.nanoDetailBoost = 1.45f;
            opt.textureBoost = 0.30f;
            opt.clarityBoost = 0.25f;
            opt.haloTolerance = 1.12f;
            break;

        case 4: // Level 4: PRO Ultra HD
            opt.scalePercent = 140;
            opt.amount = 1.70f;
            opt.detailBoost = 1.75f;
            opt.nanoDetailBoost = 1.45f;
            opt.textureBoost = 0.35f;
            opt.clarityBoost = 0.30f;
            opt.noiseAdaptive = true;
            opt.haloTolerance = 1.10f;
            opt.isPortrait = false;
            opt.skinSmooth = 0.00f;
            opt.shadowLift = 0.10f;
            opt.highlightPull = 0.08f;
            opt.use16BitPipeline = true;
            opt.contrast = 1.06f;
            opt.vibrance = 0.08f;
            break;

        case 5: // Level 5: PRO Studio Portrait
            opt.scalePercent = 120;
            opt.amount = 1.20f;
            opt.detailBoost = 1.40f;
            opt.nanoDetailBoost = 1.25f;
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
            break;

        case 0:
        default:
            // Auto Adaptive default
            opt.amount = 1.40f;
            opt.scalePercent = 130;
            opt.detailBoost = 1.50f;
            opt.nanoDetailBoost = 1.30f;
            opt.textureBoost = 0.25f;
            opt.clarityBoost = 0.18f;
            opt.noiseAdaptive = true;
            opt.haloTolerance = 1.15f;
            break;
    }
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
            for (int i = 0; i < 6; ++i) {
                const uint8_t* srcRow = src.data() + idxY[i] * srcStride;
                float wy = kY[i];
                for (int j = 0; j < 6; ++j) {
                    float w = wy * kX[j];
                    const uint8_t* pix = srcRow + idxX[j] * 4;
                    bAcc += pix[0] * w;
                    gAcc += pix[1] * w;
                    rAcc += pix[2] * w;
                    aAcc += pix[3] * w;
                }
            }

            uint8_t* outPix = dstRow + x * 4;
            outPix[0] = (uint8_t)std::clamp(rAcc, 0.0f, 255.0f); // B
            outPix[1] = (uint8_t)std::clamp(gAcc, 0.0f, 255.0f); // G
            outPix[2] = (uint8_t)std::clamp(bAcc, 0.0f, 255.0f); // R
            outPix[3] = (uint8_t)std::clamp(aAcc, 0.0f, 255.0f); // A
        }
    }
    return dst;
}

std::vector<float> ImageEnhancerPro::fastBoxFilter(const std::vector<float>& src, int width, int height, int radius) {
    std::vector<float> temp(width * height);
    std::vector<float> dst(width * height);
    float invW = 1.0f / (2 * radius + 1);

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        int rowIdx = y * width;
        float sum = src[rowIdx] * radius;
        for (int x = 0; x <= radius; ++x) {
            sum += src[rowIdx + std::min(x, width - 1)];
        }
        for (int x = 0; x < width; ++x) {
            int left = std::max(0, x - radius - 1);
            int right = std::min(width - 1, x + radius);
            sum += src[rowIdx + right] - src[rowIdx + left];
            temp[rowIdx + x] = sum * invW;
        }
    }

    #pragma omp parallel for schedule(static)
    for (int x = 0; x < width; ++x) {
        float sum = temp[x] * radius;
        for (int y = 0; y <= radius; ++y) {
            sum += temp[std::min(y, height - 1) * width + x];
        }
        for (int y = 0; y < height; ++y) {
            int top = std::max(0, y - radius - 1);
            int bottom = std::min(height - 1, y + radius);
            sum += temp[bottom * width + x] - temp[top * width + x];
            dst[y * width + x] = sum * invW;
        }
    }
    return dst;
}

std::vector<float> ImageEnhancerPro::fastBlur(const std::vector<float>& src, int width, int height, int radius) {
    if (radius <= 0) return src;
    std::vector<float> b1 = fastBoxFilter(src, width, height, radius);
    std::vector<float> b2 = fastBoxFilter(b1, width, height, radius);
    return fastBoxFilter(b2, width, height, radius);
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
// Highlight & Shadow Local Recovery (B4)
// -------------------------------------------------------------
void ImageEnhancerPro::applyHighlightShadowRecovery(
    std::vector<float>& luma, int width, int height,
    float shadowLift, float highlightPull)
{
    if (shadowLift <= 0.001f && highlightPull <= 0.001f) return;

    // Gaussian approximation via large-radius box filter
    int r = std::clamp(std::min(width, height) / 32, 10, 40);
    std::vector<float> localMean = fastBlur(luma, width, height, r);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; ++i) {
        float loc = localMean[i];
        float orig = luma[i];

        float lift = shadowLift * std::max(0.0f, 0.30f - loc);
        float pull = highlightPull * std::max(0.0f, loc - 0.82f);

        luma[i] = std::clamp(orig + lift - pull, 0.0f, 1.0f);
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

    // 2-level Laplacian decomposition
    int r = std::clamp(std::min(width, height) / 64, 4, 16);
    std::vector<float> base = fastBlur(luma, width, height, r);

    const float alpha = clarityBoost * 1.4f;
    const float beta = 0.70f; // Compression power for micro-contrast

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; ++i) {
        float y = luma[i];
        float g = base[i];
        float diff = y - g;
        float sign = (diff >= 0.0f) ? 1.0f : -1.0f;
        float absDiff = std::abs(diff);

        float remapped = g + sign * alpha * std::pow(absDiff, beta);
        luma[i] = std::clamp(remapped, 0.0f, 1.0f);
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

    // 1. Nano scale (r=1, eps=0.0005f in [0,1] normalized space)
    std::vector<float> nanoBase = applyGuidedFilterSingle(luma, luma, width, height, 1, 0.0005f);
    // 2. Micro scale (r=2, eps=0.0020f)
    std::vector<float> microBase = applyGuidedFilterSingle(nanoBase, nanoBase, width, height, 2, 0.0020f);
    // 3. Macro scale (r=4, eps=0.0080f)
    std::vector<float> macroBase = applyGuidedFilterSingle(microBase, microBase, width, height, 4, 0.0080f);

    // Compute local frequency map via 5x5 Laplacian variance
    std::vector<float> localFreq(nPixels, 0.0f);
    #pragma omp parallel for schedule(static)
    for (int y = 2; y < height - 2; ++y) {
        for (int x = 2; x < width - 2; ++x) {
            int idx = y * width + x;
            float center = luma[idx];
            float lap = std::abs(4.0f * center - luma[idx - 1] - luma[idx + 1] - luma[idx - width] - luma[idx + width]);
            localFreq[idx] = std::clamp(lap * 8.0f, 0.0f, 1.0f);
        }
    }

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < nPixels; ++i) {
        float f = localFreq[i];
        float nanoDetail = luma[i] - nanoBase[i];
        float microDetail = nanoBase[i] - microBase[i];
        float macroDetail = microBase[i] - macroBase[i];

        float wNano = 1.50f * (0.4f + 0.6f * f) * opts.nanoDetailBoost;
        float wMicro = 1.20f;
        float wMacro = 0.55f * (1.0f - 0.5f * f);

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

    // Edge-preserving decomposition
    std::vector<float> structure = applyGuidedFilterSingle(luma, luma, width, height, 3, 0.015f);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; ++i) {
        float texture = luma[i] - structure[i];
        luma[i] = std::clamp(luma[i] + texture * textureBoost, 0.0f, 1.0f);
    }
}

// -------------------------------------------------------------
// Halo Suppression Local Clamp
// -------------------------------------------------------------
void ImageEnhancerPro::applyHaloClamp(
    std::vector<float>& sharpLuma,
    const std::vector<float>& origLuma,
    int width, int height,
    float haloTolerance)
{
    std::vector<float> clamped = sharpLuma;

    #pragma omp parallel for schedule(static)
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int idx = y * width + x;
            float minVal = origLuma[idx];
            float maxVal = origLuma[idx];

            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    float v = origLuma[(y + dy) * width + (x + dx)];
                    minVal = std::min(minVal, v);
                    maxVal = std::max(maxVal, v);
                }
            }

            float range = (maxVal - minVal) * 0.15f * haloTolerance;
            clamped[idx] = std::clamp(sharpLuma[idx], minVal - range, maxVal + range);
        }
    }
    sharpLuma = std::move(clamped);
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
    int skinPixels = 0;
    int sampleCount = 0;
    std::vector<int> hist(256, 0);

    // Subsampling step for fast analysis
    int step = std::max(1, (int)std::sqrt((width * height) / 500000.0f));

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

            // Grayscale luma (ITU-R BT.601)
            float Y = 0.299f * r + 0.587f * g + 0.114f * b;
            int yInt = std::clamp((int)std::round(Y), 0, 255);
            hist[yInt]++;

            // Gradient
            const uint8_t* pixR = rowCur + (x + step) * 4;
            const uint8_t* pixL = rowCur + (x - step) * 4;
            const uint8_t* pixB = rowNext + x * 4;
            const uint8_t* pixT = rowPrev + x * 4;

            float yR = 0.299f * pixR[2] + 0.587f * pixR[1] + 0.114f * pixR[0];
            float yL = 0.299f * pixL[2] + 0.587f * pixL[1] + 0.114f * pixL[0];
            float yB = 0.299f * pixB[2] + 0.587f * pixB[1] + 0.114f * pixB[0];
            float yT = 0.299f * pixT[2] + 0.587f * pixT[1] + 0.114f * pixT[0];

            float grad = (std::abs(yR - yL) + std::abs(yB - yT)) / (2.0f * step);
            gradSum += grad;

            // Skin tone detection
            float Cb = 128.0f - 0.168736f * r - 0.331264f * g + 0.500000f * b;
            float Cr = 128.0f + 0.500000f * r - 0.418688f * g - 0.081312f * b;
            if (Cb >= 77.0f && Cb <= 128.0f && Cr >= 133.0f && Cr <= 175.0f) {
                skinPixels++;
            }

            // Estimate flat region variance for noise floor
            if (grad < 2.5f) {
                flatRegionVariances.push_back(grad);
            }

            sampleCount++;
        }
    }

    if (sampleCount > 0) {
        float avgGrad = (float)(gradSum / sampleCount);
        score.clarityScore = std::clamp(avgGrad * 7.5f, 0.0f, 100.0f);
        score.skinPercent = ((float)skinPixels / sampleCount) * 100.0f;
    }

    // Dynamic Range: 1% to 99% percentile
    int totalHist = sampleCount;
    int p1 = 0, p99 = 255;
    int acc = 0;
    for (int i = 0; i < 256; ++i) {
        acc += hist[i];
        if (p1 == 0 && acc >= totalHist * 0.01) p1 = i;
        if (acc >= totalHist * 0.99) { p99 = i; break; }
    }
    score.dynamicRange = (float)(p99 - p1);

    // Noise Floor estimation via MAD on flat regions
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

    if (score.skinPercent >= 8.0f) {
        score.detectedType = "Chân dung (Portrait)";
    } else if (score.clarityScore >= 55.0f) {
        score.detectedType = "Phong cảnh / Chi tiết cao";
    } else {
        score.detectedType = "Ảnh thường / Cần phục hồi nét";
    }

    return score;
}

// -------------------------------------------------------------
// Core Processing Pipeline in Oklab / OkLCh Space
// -------------------------------------------------------------
void ImageEnhancerPro::processSharpenOklab(
    const std::vector<uint8_t>& src, std::vector<uint8_t>& dst,
    int width, int height, int stride,
    const EnhanceOptionsPro& opts,
    float estimatedNoise)
{
    int nPixels = width * height;
    std::vector<float> origL(nPixels);
    std::vector<float> origC(nPixels);
    std::vector<float> origH(nPixels);
    std::vector<float> origCb(nPixels);
    std::vector<float> origCr(nPixels);

    // 1. Convert sRGB to Oklab / OkLCh
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        const uint8_t* row = src.data() + y * stride;
        int rowIdx = y * width;
        for (int x = 0; x < width; ++x) {
            const uint8_t* pix = row + x * 4;
            float b = pix[0];
            float g = pix[1];
            float r = pix[2];

            OklabPixel lab = sRGBToOklab(r, g, b);
            OkLChPixel lch = oklabToOkLCh(lab);

            int idx = rowIdx + x;
            origL[idx] = lch.L;
            origC[idx] = lch.C;
            origH[idx] = lch.h;

            // BT.601 Cb/Cr for skin model
            origCb[idx] = 128.0f - 0.168736f * r - 0.331264f * g + 0.500000f * b;
            origCr[idx] = 128.0f + 0.500000f * r - 0.418688f * g - 0.081312f * b;
        }
    }

    // 2. Highlight/Shadow Local Recovery on L
    std::vector<float> procL = origL;
    applyHighlightShadowRecovery(procL, width, height, opts.shadowLift, opts.highlightPull);

    // 3. Local Laplacian Tone Mapping on L
    applyLocalLaplacianToneMapping(procL, width, height, opts.clarityBoost);

    // 4. 3-Scale Guided Filter Decomposition
    std::vector<float> diffGuided(nPixels, 0.0f);
    applyGuidedFilter3Scale(procL, diffGuided, width, height, opts);

    // 5. Texture Layer Synthesis
    synthesizeTextureLayer(procL, width, height, opts.textureBoost);

    // 6. Blur L for CAS and contrast
    std::vector<float> blurL = fastBlur(procL, width, height, opts.radius);

    // 7. Calculate Noise-Adaptive Cauchy Coring Parameter
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
            float center = procL[idx];

            // 4-neighbor gradient
            int lIdx = rowIdx + std::max(0, x - 1);
            int rIdx = rowIdx + std::min(width - 1, x + 1);
            int tIdx = std::max(0, y - 1) * width + x;
            int bIdx = std::min(height - 1, y + 1) * width + x;

            float gradL = std::abs(procL[rIdx] - procL[lIdx]);
            float gradV = std::abs(procL[bIdx] - procL[tIdx]);
            float grad255 = (gradL + gradV) * 255.0f;

            // Adaptive Cauchy Coring
            float edgeWeight = (grad255 * grad255) / (grad255 * grad255 + cauchyK) * opts.edgeSensitivity;

            // Contrast Adaptive Sharpening (CAS)
            float minVal = std::min({ procL[lIdx], procL[rIdx], procL[tIdx], procL[bIdx], center });
            float maxVal = std::max({ procL[lIdx], procL[rIdx], procL[tIdx], procL[bIdx], center });
            float ampLimit = std::min(center - minVal, maxVal - center);
            float casW = (ampLimit > 1e-5f) ? (opts.casStrength * std::clamp(ampLimit * 3.0f, 0.0f, 1.0f)) : 0.0f;

            float diff = (center - blurL[idx]) * opts.amount;
            float res = center + diff * edgeWeight + diffGuided[idx] * (1.0f + casW);

            // 8. Soft Skin Probability Mask
            if (opts.isPortrait) {
                float cb = origCb[idx];
                float cr = origCr[idx];
                // 2D Gaussian Skin Model
                float dCb = (cb - 109.0f) / (18.0f * opts.skinProbSigma);
                float dCr = (cr - 152.0f) / (14.0f * opts.skinProbSigma);
                float pSkin = std::exp(-0.5f * (dCb * dCb + dCr * dCr));

                if (pSkin > 0.05f) {
                    float smoothWeight = opts.skinSmooth * pSkin * std::max(0.0f, 1.0f - grad255 / 14.0f);
                    res = res * (1.0f - smoothWeight) + (center * 0.70f + blurL[idx] * 0.30f) * smoothWeight;
                }
            }

            sharpL[idx] = std::clamp(res, 0.0f, 1.0f);
        }
    }

    // 9. Halo Suppression Local Clamp
    applyHaloClamp(sharpL, origL, width, height, opts.haloTolerance);

    // 10. Recompose in OkLCh with Constant-Saturation Chroma Tracking and Soft Gamut Roll-off
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        const uint8_t* srcRow = src.data() + y * stride;
        uint8_t* dstRow = dst.data() + y * stride;
        int rowIdx = y * width;

        for (int x = 0; x < width; ++x) {
            int idx = rowIdx + x;
            float L_sharp = sharpL[idx];
            float L_orig  = std::max(0.001f, origL[idx]);

            // Chroma expansion proportional to perceived brightness
            float lumaRatio = L_sharp / L_orig;
            float chromaExpansion = std::clamp(std::pow(lumaRatio, 1.20f), 0.85f, 1.75f);

            OkLChPixel lchNew;
            lchNew.L = L_sharp;
            lchNew.C = origC[idx] * chromaExpansion;
            lchNew.h = origH[idx]; // Absolute Hue locked!

            OklabPixel labNew = okLChToOklab(lchNew);

            float rOut, gOut, bOut;
            oklabTosRGB(labNew.L, labNew.a, labNew.b, rOut, gOut, bOut);

            // Soft Gamut Roll-off to avoid hard clipping distortion
            float maxComp = std::max({ rOut, gOut, bOut });
            if (maxComp > 255.0f) {
                float compFactor = 255.0f / maxComp;
                rOut *= compFactor;
                gOut *= compFactor;
                bOut *= compFactor;
            }

            dstRow[x * 4 + 0] = (uint8_t)std::clamp(bOut, 0.0f, 255.0f);
            dstRow[x * 4 + 1] = (uint8_t)std::clamp(gOut, 0.0f, 255.0f);
            dstRow[x * 4 + 2] = (uint8_t)std::clamp(rOut, 0.0f, 255.0f);
            dstRow[x * 4 + 3] = srcRow[x * 4 + 3]; // Preserve original Alpha
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
        // --- CHẾ ĐỘ AUTO-ADAPTIVE PRO ---
        opts = getPresetPro(0);

        // 1. Phóng đại thích ứng
        if (score.megaPixels < 0.6f) {
            opts.scalePercent = 150;
        } else if (score.megaPixels < 1.8f) {
            opts.scalePercent = 130;
        } else if (score.megaPixels < 4.0f) {
            opts.scalePercent = 115;
        } else {
            opts.scalePercent = 100;
        }

        // 2. Điều chỉnh nét theo clarityScore
        if (score.clarityScore < 40.0f) {
            opts.amount = 1.60f;
            opts.casStrength = 1.15f;
            opts.detailBoost = 1.65f;
        } else if (score.clarityScore < 70.0f) {
            opts.amount = 1.30f;
            opts.casStrength = 0.95f;
            opts.detailBoost = 1.50f;
        } else {
            opts.amount = 0.95f;
            opts.casStrength = 0.70f;
            opts.detailBoost = 1.30f;
        }

        // 3. Phân loại Chân dung vs Phong cảnh
        if (score.skinPercent >= 8.0f) {
            opts.isPortrait = true;
            opts.skinSmooth = 0.42f;
            opts.clarityBoost = 0.12f;
            opts.textureBoost = 0.20f;
        } else {
            opts.isPortrait = false;
            opts.skinSmooth = 0.00f;
            opts.clarityBoost = 0.25f;
            opts.textureBoost = 0.35f;
        }

        // 4. Thích ứng theo Noise Floor
        if (score.noiseFloor > 6.0f) {
            opts.noiseAdaptive = true;
            opts.amount *= 0.85f;
        }

        // 5. Thích ứng theo Dynamic Range
        if (score.dynamicRange > 200.0f) {
            opts.shadowLift = 0.08f;
            opts.highlightPull = 0.06f;
        }
    } else {
        opts = getPresetPro(level);
    }

    UINT procW = origW;
    UINT procH = origH;
    UINT procStride = origStride;
    std::vector<uint8_t> scaledPixels;

    // Lanczos-3 Super-Sampling
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
    if (outScore) *outScore = score;

    // Oklab / OkLCh Pro Pipeline Processing
    std::vector<uint8_t> dstPixels(procH * procStride);
    processSharpenOklab(scaledPixels, dstPixels, procW, procH, procStride, opts, score.noiseFloor);

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
        hr = pFrameEncode->WritePixels(procH, procStride, (UINT)dstPixels.size(), dstPixels.data());
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
