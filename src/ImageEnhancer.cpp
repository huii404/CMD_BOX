#include "ImageEnhancer.h"
#include <windows.h>
#include <wincodec.h>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <memory>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace fs = std::filesystem;

EnhanceOptions ImageEnhancer::getPreset(int level) {
    EnhanceOptions opt;
    switch (level) {
        case 1: // Nét Chân dung / Người & Da (Portrait - Mịn da, chống gai ảnh)
            opt.amount = 1.05f;
            opt.radius = 2;
            opt.threshold = 3.0f;
            opt.edgeSensitivity = 1.10f;
            opt.contrast = 1.03f;
            opt.vibrance = 0.04f;
            opt.scalePercent = 135;
            opt.casStrength = 0.75f;
            opt.isPortrait = true;
            opt.skinSmooth = 0.45f;
            break;
        case 3: // Siêu phục hồi cực đại (Ultra Max 2x Detail)
            opt.amount = 2.10f;
            opt.radius = 2;
            opt.threshold = 1.4f;
            opt.edgeSensitivity = 1.65f;
            opt.contrast = 1.09f;
            opt.vibrance = 0.10f;
            opt.scalePercent = 200;
            opt.casStrength = 1.45f;
            opt.isPortrait = false;
            break;
        case 2: // Nét Phong cảnh & Chi tiết cao (Landscape - Tăng nét vi mô, nổi khối)
        default:
            opt.amount = 1.55f;
            opt.radius = 2;
            opt.threshold = 1.8f;
            opt.edgeSensitivity = 1.35f;
            opt.contrast = 1.06f;
            opt.vibrance = 0.08f;
            opt.scalePercent = 150;
            opt.casStrength = 1.15f;
            opt.isPortrait = false;
            break;
    }
    return opt;
}

bool ImageEnhancer::isSupportedImage(const std::string& filePath) {
    std::string ext = fs::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".tif" || ext == ".tiff" || ext == ".webp");
}

bool ImageEnhancer::isWebP(const std::string& filePath) {
    std::string ext = fs::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".webp");
}

float ImageEnhancer::cubicKernel(float x) {
    x = std::abs(x);
    if (x <= 1.0f)
        return 1.5f * x * x * x - 2.5f * x * x + 1.0f;
    if (x < 2.0f)
        return -0.5f * x * x * x + 2.5f * x * x - 4.0f * x + 2.0f;
    return 0.0f;
}

float ImageEnhancer::applySmoothSCurve(float val, float contrast) {
    if (std::abs(contrast - 1.0f) < 0.001f) return val;
    float norm = std::clamp(val / 255.0f, 0.0f, 1.0f);
    float s = norm + (contrast - 1.0f) * 1.6f * norm * (1.0f - norm) * (norm - 0.5f);
    return std::clamp(s * 255.0f, 0.0f, 255.0f);
}

std::vector<uint8_t> ImageEnhancer::bicubicResample(
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
        int dstRowOffset = y * dstStride;

        for (int x = 0; x < dstW; ++x) {
            float srcX = (x + 0.5f) * scaleX - 0.5f;
            int x0 = (int)std::floor(srcX);
            float dx = srcX - x0;

            float sumB = 0.0f, sumG = 0.0f, sumR = 0.0f, sumA = 0.0f;

            for (int m = -1; m <= 2; ++m) {
                int py = std::clamp(y0 + m, 0, srcH - 1);
                int rowOffset = py * srcStride;
                float wy = cubicKernel(m - dy);

                for (int n = -1; n <= 2; ++n) {
                    int px = std::clamp(x0 + n, 0, srcW - 1) * 4;
                    float w = wy * cubicKernel(n - dx);

                    sumB += src[rowOffset + px] * w;
                    sumG += src[rowOffset + px + 1] * w;
                    sumR += src[rowOffset + px + 2] * w;
                    sumA += src[rowOffset + px + 3] * w;
                }
            }

            int outPx = dstRowOffset + (x * 4);
            dst[outPx]     = (uint8_t)std::clamp((int)std::round(sumB), 0, 255);
            dst[outPx + 1] = (uint8_t)std::clamp((int)std::round(sumG), 0, 255);
            dst[outPx + 2] = (uint8_t)std::clamp((int)std::round(sumR), 0, 255);
            dst[outPx + 3] = (uint8_t)std::clamp((int)std::round(sumA), 0, 255);
        }
    }
    return dst;
}

std::vector<float> ImageEnhancer::fastBlurLuma(const std::vector<float>& src, int width, int height, int radius) {
    if (radius < 1) radius = 1;
    std::vector<float> buffer1 = src;
    std::vector<float> buffer2(src.size());
    int div = radius * 2 + 1;

    // Lặp 3 lượt Box Blur hội tụ thành bộ lọc chuẩn Gauss (Gaussian Approximation)
    // Triệt tiêu 100% hình khối vuông (box shape) của Box Blur 1 lượt
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

void ImageEnhancer::processSharpenYCbCr(
    const std::vector<uint8_t>& src, std::vector<uint8_t>& dst,
    int width, int height, int stride, const EnhanceOptions& opts)
{
    int totalPixels = width * height;
    std::vector<float> luma(totalPixels);
    std::vector<float> chromaCb(totalPixels);
    std::vector<float> chromaCr(totalPixels);
    std::vector<uint8_t> alpha(totalPixels);

    // 1. Chuyển BGRA sang YCbCr (Chuẩn ITU-R BT.601)
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        int rowOffset = y * stride;
        int pixelRow = y * width;
        for (int x = 0; x < width; ++x) {
            int px = rowOffset + (x * 4);
            int idx = pixelRow + x;

            float b = src[px];
            float g = src[px + 1];
            float r = src[px + 2];
            alpha[idx] = src[px + 3];

            luma[idx]     = 0.299f * r + 0.587f * g + 0.114f * b;
            chromaCb[idx] = -0.168736f * r - 0.331264f * g + 0.5f * b + 128.0f;
            chromaCr[idx] = 0.5f * r - 0.418688f * g - 0.081312f * b + 128.0f;
        }
    }

    // 2. Làm mờ kênh Luminance Y để lấy mặt nạ viền
    std::vector<float> blurredLuma = fastBlurLuma(luma, width, height, opts.radius);

    float amount = opts.amount;
    float thresholdSq = opts.threshold * opts.threshold;
    float contrast = opts.contrast;
    float vibrance = opts.vibrance;
    float edgeSens = opts.edgeSensitivity;
    float casWeight = opts.casStrength;

    // 3. Contrast Adaptive Sharpening (CAS) + Anti-Halo trên kênh Y
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        int rowOffset = y * stride;
        int pixelRow = y * width;
        int prevRow = std::max(0, y - 1) * width;
        int nextRow = std::min(height - 1, y + 1) * width;

        for (int x = 0; x < width; ++x) {
            int idx = pixelRow + x;
            int prevX = std::max(0, x - 1);
            int nextX = std::min(width - 1, x + 1);

            float yCenter = luma[idx];
            float yBlur = blurredLuma[idx];

            // 4 điểm lân cận chữ thập (Cross 3x3)
            float yLeft = luma[pixelRow + prevX];
            float yRight = luma[pixelRow + nextX];
            float yTop = luma[prevRow + x];
            float yBottom = luma[nextRow + x];

            float minY = std::min(yCenter, std::min(std::min(yLeft, yRight), std::min(yTop, yBottom)));
            float maxY = std::max(yCenter, std::max(std::max(yLeft, yRight), std::max(yTop, yBottom)));

            // Gradient biên độ xác định viền thật vs vùng mịn
            float grad = std::abs(yRight - yLeft) + std::abs(yBottom - yTop);

            // Khử ranh giới khối vuông JPEG (De-blocking deadband):
            // Vùng nền mờ/phẳng có ranh giới khối 8x8 (grad <= 3.2f) -> triệt tiêu về 0 để không bị nổi ô vuông
            float edgeWeight = 0.0f;
            if (grad > 3.2f) {
                edgeWeight = std::clamp(((grad - 3.2f) / 14.0f) * edgeSens, 0.0f, 1.40f);
            }

            float diffY = yCenter - yBlur;
            if (std::abs(diffY) < 1.6f) {
                diffY = 0.0f; // Triệt tiêu vi sai siêu nhỏ của vết ghép khối vuông nén
            }

            // Soft-coring: Bỏ qua hạt nhiễu nhỏ, chỉ làm nét cấu trúc
            float wY = (diffY * diffY) / (diffY * diffY + thresholdSq);

            // CAS Dynamic Peak: Hạn chế biến dạng cục bộ
            float range = std::max(maxY - minY, 0.001f);
            float peak = std::min(yCenter - minY, maxY - yCenter) / range;
            float casFactor = 0.5f + 0.5f * peak * casWeight;

            float sharpY = yCenter + diffY * amount * wY * edgeWeight * casFactor;

            // Anti-Halo: Chống quầng sáng/tối giả tạo quanh viền
            float overshoot = (maxY - minY) * 0.15f + 1.5f;
            sharpY = std::clamp(sharpY, minY - overshoot, maxY + overshoot);

            // Đường cong tương phản vi mô Micro-Contrast S-Curve
            sharpY = applySmoothSCurve(sharpY, contrast);

            // Bảo vệ và làm mịn vùng da người (Chống gai ảnh, khử hạt sạn trên da mặt)
            if (opts.isPortrait) {
                float cbRaw = chromaCb[idx];
                float crRaw = chromaCr[idx];
                // Vùng nhận diện màu da người chuẩn ITU-R BT.601 YCbCr
                if (cbRaw >= 77.0f && cbRaw <= 128.0f && crRaw >= 133.0f && crRaw <= 175.0f) {
                    if (grad < 14.0f) {
                        float smoothFactor = opts.skinSmooth * (1.0f - grad / 14.0f);
                        sharpY = sharpY * (1.0f - smoothFactor) + (yCenter * 0.75f + yBlur * 0.25f) * smoothFactor;
                    }
                }
            }

            // 4. Tái tạo màu RGB từ (sharpY, Cb, Cr)
            float cb = chromaCb[idx] - 128.0f;
            float cr = chromaCr[idx] - 128.0f;

            if (yCenter > 1.0f) {
                float lumaRatio = std::clamp(sharpY / yCenter, 0.90f, 1.20f);
                float chromaScale = 1.05f + (lumaRatio - 1.0f) * 0.45f;
                cb *= chromaScale;
                cr *= chromaScale;
            } else {
                cb *= 1.05f;
                cr *= 1.05f;
            }

            float r = sharpY + 1.402f * cr;
            float g = sharpY - 0.344136f * cb - 0.714136f * cr;
            float b = sharpY + 1.772f * cb;

            // Tăng độ tươi thông minh (Smart Vibrance)
            if (vibrance > 0.001f) {
                float maxVal = std::max(r, std::max(g, b));
                float minVal = std::min(r, std::min(g, b));
                float sat = (maxVal - minVal) / (maxVal + 0.001f);
                float boost = (1.0f - sat) * vibrance;

                float gray = sharpY;
                r += (r - gray) * boost;
                g += (g - gray) * boost;
                b += (b - gray) * boost;
            }

            int px = rowOffset + (x * 4);
            dst[px]     = (uint8_t)std::clamp((int)std::round(b), 0, 255);
            dst[px + 1] = (uint8_t)std::clamp((int)std::round(g), 0, 255);
            dst[px + 2] = (uint8_t)std::clamp((int)std::round(r), 0, 255);
            dst[px + 3] = alpha[idx];
        }
    }
}

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

ImageScore ImageEnhancer::analyzeImageBuffer(
    const std::vector<uint8_t>& src, 
    int width, int height, int stride, 
    uintmax_t fileSize) 
{
    ImageScore score;
    score.origW = width;
    score.origH = height;
    score.megaPixels = (width * height) / 1000000.0f;
    score.bpp = (width * height > 0 && fileSize > 0) ? ((float)fileSize / (width * height)) : 0.25f;

    float totalGrad = 0.0f;
    int skinCount = 0;
    int sampleStep = (width * height > 1500000) ? 2 : 1;
    int samplesChecked = 0;

    for (int y = 1; y < height - 1; y += sampleStep) {
        int row = y * stride;
        for (int x = 1; x < width - 1; x += sampleStep) {
            int p = row + x * 4;
            float b = src[p];
            float g = src[p + 1];
            float r = src[p + 2];

            float yCenter = 0.299f * r + 0.587f * g + 0.114f * b;
            float yRight  = 0.299f * src[p + 6] + 0.587f * src[p + 5] + 0.114f * src[p + 4];
            float yDown   = 0.299f * src[p + stride + 2] + 0.587f * src[p + stride + 1] + 0.114f * src[p + stride];

            totalGrad += std::abs(yRight - yCenter) + std::abs(yDown - yCenter);

            float cb = -0.168736f * r - 0.331264f * g + 0.5f * b + 128.0f;
            float cr = 0.5f * r - 0.418688f * g - 0.081312f * b + 128.0f;
            if (cb >= 77.0f && cb <= 128.0f && cr >= 133.0f && cr <= 175.0f) {
                skinCount++;
            }
            samplesChecked++;
        }
    }

    float meanGrad = (samplesChecked > 0) ? (totalGrad / samplesChecked) : 8.0f;
    score.skinPercent = (samplesChecked > 0) ? ((float)skinCount / samplesChecked * 100.0f) : 0.0f;
    score.clarityScore = std::clamp(meanGrad * 5.0f, 10.0f, 98.0f);

    if (score.skinPercent >= 8.0f) {
        score.detectedType = "Chân dung (" + std::to_string((int)std::round(score.skinPercent)) + "% da)";
    } else if (score.clarityScore < 40.0f || score.bpp < 0.18f) {
        score.detectedType = "Ảnh mờ / Nén thấp";
    } else {
        score.detectedType = "Phong cảnh / Chi tiết";
    }

    return score;
}

bool ImageEnhancer::enhanceImage(
    const std::string& inputPath, 
    const std::string& outputPath, 
    int level, 
    ImageScore* outScore) 
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
    ImageScore score = analyzeImageBuffer(srcPixels, origW, origH, origStride, inFileSize);
    EnhanceOptions opts;

    if (level <= 0) {
        // --- CHẾ ĐỘ TỰ ĐỘNG THÔNG MINH (AUTO ADAPTIVE) ---
        // 1. Tự động bù điểm ảnh để cân bằng mật độ XY
        if (score.megaPixels < 0.6f) {
            opts.scalePercent = 180; // Ảnh nhỏ: bù mạnh điểm ảnh
        } else if (score.megaPixels < 1.8f) {
            opts.scalePercent = 145; // Ảnh vừa (720p-1080p): bù chuẩn 145%
        } else if (score.megaPixels < 4.0f) {
            opts.scalePercent = 120; // Ảnh lớn (2K-3K): bù nhẹ 120%
        } else {
            opts.scalePercent = 100; // Ảnh 4K+: giữ nguyên độ phân giải
        }

        // 2. Tự động điều chỉnh độ nét thích ứng theo điểm số nét
        if (score.clarityScore < 40.0f) {
            opts.amount = 1.65f;
            opts.threshold = 1.8f;
            opts.edgeSensitivity = 1.35f;
            opts.casStrength = 1.20f;
            opts.contrast = 1.07f;
            opts.vibrance = 0.07f;
        } else if (score.clarityScore < 70.0f) {
            opts.amount = 1.25f;
            opts.threshold = 2.4f;
            opts.edgeSensitivity = 1.20f;
            opts.casStrength = 0.90f;
            opts.contrast = 1.05f;
            opts.vibrance = 0.05f;
        } else {
            opts.amount = 0.85f;
            opts.threshold = 3.0f;
            opts.edgeSensitivity = 1.00f;
            opts.casStrength = 0.65f;
            opts.contrast = 1.02f;
            opts.vibrance = 0.03f;
        }

        // 3. Tự động nhận diện Chân dung / Da người
        if (score.skinPercent >= 8.0f) {
            opts.isPortrait = true;
            opts.skinSmooth = 0.45f;
        } else {
            opts.isPortrait = false;
        }
    } else {
        opts = getPreset(level);
    }

    UINT procW = origW;
    UINT procH = origH;
    UINT procStride = origStride;
    std::vector<uint8_t> scaledPixels;

    // 1. Phóng to nội suy Super-Sampling Catmull-Rom Bicubic
    if (opts.scalePercent > 100) {
        procW = (UINT)std::round(origW * (opts.scalePercent / 100.0));
        procH = (UINT)std::round(origH * (opts.scalePercent / 100.0));
        procStride = procW * 4;
        scaledPixels = bicubicResample(srcPixels, origW, origH, origStride, procW, procH, procStride);
    } else {
        scaledPixels = std::move(srcPixels);
    }

    score.scalePercent = opts.scalePercent;
    score.procW = procW;
    score.procH = procH;
    if (outScore) *outScore = score;

    // 2. Làm nét thích ứng YCbCr (CAS + Anti-Halo + S-Curve + Vibrance)
    std::vector<uint8_t> dstPixels(procH * procStride);
    processSharpenYCbCr(scaledPixels, dstPixels, procW, procH, procStride, opts);

    // 3. Ghi file ảnh bằng WIC Encoder
    std::string ext = fs::path(outputPath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    GUID containerFormat = GUID_ContainerFormatJpeg;
    if (ext == ".jpg" || ext == ".jpeg") {
        containerFormat = GUID_ContainerFormatJpeg;
    } else if (ext == ".png") {
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
        // Tối ưu chất lượng JPEG ở mức chuẩn 92% (Chất lượng mắt người nhìn không khác lossless)
        if (containerFormat == GUID_ContainerFormatJpeg && pPropertyBag) {
            PROPBAG2 opt = { 0 };
            opt.pstrName = (LPOLESTR)L"ImageQuality";
            VARIANT var;
            VariantInit(&var);
            var.vt = VT_R4;
            var.fltVal = 0.92f;
            pPropertyBag->Write(1, &opt, &var);
            VariantClear(&var);
        }
        hr = pFrameEncode->Initialize(pPropertyBag);
    }

    if (SUCCEEDED(hr)) {
        hr = pFrameEncode->SetSize(procW, procH);
        float scaleFactor = opts.scalePercent / 100.0f;
        pFrameEncode->SetResolution(dpiX * scaleFactor, dpiY * scaleFactor);

        WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
        hr = pFrameEncode->SetPixelFormat(&pixelFormat);

        if (SUCCEEDED(hr)) {
            if (pixelFormat == GUID_WICPixelFormat32bppBGRA) {
                hr = pFrameEncode->WritePixels(procH, procStride, (UINT)dstPixels.size(), dstPixels.data());
            } else {
                // Fallback sang BGR 24bpp (nếu encoder JPEG yêu cầu 24bpp)
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

        if (SUCCEEDED(hr)) hr = pFrameEncode->Commit();
        if (SUCCEEDED(hr)) hr = pEncoder->Commit();
    }

    if (pFrameEncode) pFrameEncode->Release();
    if (pPropertyBag) pPropertyBag->Release();
    if (pEncoder) pEncoder->Release();
    if (pStream) pStream->Release();
    pFrame->Release();
    pDecoder->Release();
    pFactory->Release();
    CoUninitialize();

    return SUCCEEDED(hr) && fs::exists(outputPath) && fs::file_size(outputPath) > 0;
}
