# ĐẶC TẢ KIẾN TRÚC & THUẬT TOÁN LÀM NÉT ẢNH CHUYÊN SÂU: IMAGE ENHANCER ENGINE — **PRO EDITION**

> **Tài liệu nâng cấp từ bản BASE** — bổ sung các module mới để đạt chất lượng đầu ra cấp độ studio ảnh chuyên nghiệp / máy ảnh flagship.
> **Vị trí mã nguồn:** `include/ImageEnhancerPro.h` & `src/ImageEnhancerPro.cpp`
> **Ngôn ngữ:** C++17 Native | **Đồ họa:** WIC | **Gia tốc:** OpenMP + AVX2 SIMD | **Không gian màu nội bộ:** Oklab / OkLCh

---

## I. VÌ SAO CẦN NÂNG CẤP LÊN PRO?

Bản BASE đã giải quyết tốt vấn đề bệt ảnh và tụt màu, nhưng vẫn còn 5 giới hạn khi thử với ảnh chất lượng cao / ảnh nhiễu / ảnh có dải sáng rộng:

| Hạn chế của BASE                                         | Hệ quả thực tế                                                                          | Giải pháp PRO                                                                               |
| :---------------------------------------------------------- | :------------------------------------------------------------------------------------------ | :-------------------------------------------------------------------------------------------- |
| Chỉ 2 tầng Guided Filter (micro/macro)                    | Vân tóc mảnh và khối 3D lớn bị gộp chung trọng số cố định 1.35/0.65            | **3-Scale Decomposition** (Nano/Micro/Macro) với trọng số thích ứng theo nội dung |
| Ngưỡng Cauchy`12.0` cố định toàn ảnh               | Ảnh nhiễu hạt cao (ISO cao, ảnh nén JPEG thấp) bị khuếch đại nhiễu thành "sạn" | **Noise-Floor Adaptive Threshold** dùng ước lượng MAD cục bộ                     |
| CAS + Cauchy có thể vượt biên độ lân cận           | Xuất hiện viền sáng/tối viền cạnh (halo) ở ảnh tương phản cao                   | **Halo Suppression bằng Local Clamp**                                                  |
| Chroma tracking trên YCbCr (không đều tri giác)        | Ở vùng bão hòa cao (đỏ, cam), màu vẫn lệch nhẹ khi tăng nét mạnh               | Chuyển sang**Oklab/OkLCh** — không gian màu đều tri giác                         |
| CLAHE lưới 8×8 tạo tương phản cục bộ nhưng phẳng | Ảnh phong cảnh thiếu "độ sâu" tự nhiên như xử lý RAW chuyên nghiệp             | **Local Laplacian Tone Mapping** thay thế, không gây đảo gradient                  |
| Không tách lớp texture riêng                            | Không thể tăng "chất liệu" (da, vải, gỗ) độc lập với cạnh biên                 | **Texture Layer Synthesis** (tương tự Clarity/Texture của Lightroom)                |
| Nhận diện da bằng ngưỡng nhị phân Cb/Cr              | Biên chuyển giữa da và tóc/mí mắt bị gãy cứng, dễ lộ ranh giới xử lý         | **Skin Probability Mask** dạng Gaussian mềm thay vì bật/tắt                        |

---

## II. SƠ ĐỒ PIPELINE PRO (10 BƯỚC)

```text
        [FILE ẢNH ĐẦU VÀO] (JPG, PNG, BMP, TIFF, HEIC, DNG, WebP, RAW*)
                                      │
                                      ▼
   B1: Giải mã WIC Native sang BGRA 32bpp / linear-light nếu là RAW
                                      │
                                      ▼
   B2: Phân tích ma trận ảnh MỞ RỘNG
       - clarityScore, skinPercent (như BASE)
       - NEW: noiseFloor (ước lượng MAD), dynamicRange (histogram percentile)
                                      │
                                      ▼
   B3: Auto-Adaptive Presets & Lanczos-3 Scale (giữ nguyên BASE)
                                      │
                                      ▼
   B4: Highlight/Shadow Local Recovery (NEW)
       - Local tone compression trước khi sharpen, tránh mất chi tiết vùng cháy/tối
                                      │
                                      ▼
   B5: Tách kênh sang Oklab (L, a, b) — thay ITU-R BT.601 YCbCr (NEW)
                                      │
                                      ▼
   B6: Local Laplacian Tone Mapping (thay CLAHE lưới 8×8) (NEW)
                                      │
                                      ▼
   B7: Phân rã 3-Scale Guided Filter: Nano (r=0.5) / Micro (r=1) / Macro (r=3) (NÂNG CẤP)
                                      │
                                      ▼
   B8: Pipeline Làm nét đa tần số thích ứng
       - Noise-Floor Adaptive Cauchy Coring (NÂNG CẤP)
       - Multi-Radius CAS + Halo Suppression Local Clamp (NÂNG CẤP)
       - Texture Layer Synthesis — Clarity/Texture tách lớp (NEW)
                                      │
                                      ▼
   B9: Dual-Zone Portrait Protection với Skin Probability Mask mềm (NÂNG CẤP)
                                      │
                                      ▼
   B10: Constant-Saturation Chroma Tracking trên OkLCh + Soft Gamut Roll-off (NÂNG CẤP)
                                      │
                                      ▼
        [FILE ẢNH ĐẦU RA] (WIC Encoder chất lượng cao, hỗ trợ 16-bit/kênh)
```

*RAW: nếu pipeline đầu vào có bộ giải mã RAW![1788939083691](image/README_IMAGE_ENHANCER_PRO/1788939083691.png) riêng, bước B1 nhận buffer linear-light 16-bit thay vì sRGB 8-bit.*

---

## III. CHI TIẾT CÁC MODULE NÂNG CẤP

### 1. Phân rã 3-Scale Guided Filter (thay 2-Scale)

* **Vị trí hàm:** `applyGuidedFilter3Scale(...)`
* Bổ sung tầng **Nano-scale** (`r = 0.5, ε = 80.0`) đứng trước tầng Micro, chuyên cô lập chi tiết ở cấp độ dưới 1 pixel (răng cưa lông tơ, hạt sạn phim mô phỏng, vân da mịn):

$$
nanoDetail = Y_{center} - baseLumaNano[idx]
$$

$$
microDetail = baseLumaNano[idx] - baseLumaMicro[idx]
$$

$$
macroDetail = baseLumaMicro[idx] - baseLumaMacro[idx]
$$

* **Trọng số thích ứng theo nội dung** (thay vì hằng số cố định 1.35/0.65 của BASE): trọng số mỗi tầng được điều biến theo `localFrequencyMap` — vùng tần số cao (tóc, gân lá) ưu tiên Nano/Micro, vùng tần số thấp (da, bầu trời) ưu tiên Macro để tránh lộ hạt:

$$
w_{nano} = 1.5 \times f(localFreq), \quad w_{micro} = 1.2, \quad w_{macro} = 0.55 \times (1 - f(localFreq))
$$

$$
diffGuided = (nanoDetail \times w_{nano} + microDetail \times w_{micro} + macroDetail \times w_{macro}) \times (opts.detailBoost - 1.0)
$$

trong đó $f(localFreq) \in [0,1]$ là tỷ lệ năng lượng tần số cao cục bộ, tính bằng phương sai Laplacian trong cửa sổ 5×5.

---

### 2. Ngưỡng Cauchy thích ứng theo nhiễu nền (Noise-Floor Adaptive Coring)

* **Vấn đề của BASE:** hằng số `12.0` trong mẫu số hàm Cauchy là cố định toàn ảnh — không phân biệt được ảnh sạch (ISO 100) với ảnh nhiễu (ISO 3200), dẫn đến khuếch đại hạt nhiễu thành sạn giả chi tiết.
* **Giải pháp PRO:** ước lượng nhiễu nền cục bộ bằng **Median Absolute Deviation (MAD)** trên vùng phẳng lân cận (loại trừ cạnh biên), sau đó dùng làm hệ số chuẩn hoá thay cho hằng số:

$$
\sigma_{noise} = 1.4826 \times \text{median}(|Y_i - \text{median}(Y_{\omega})|), \quad i \in \omega_{7\times7}
$$

$$
k = \max(6.0,\ \min(40.0,\ 4 \times \sigma_{noise}^2))
$$

$$
edgeWeight = \frac{grad^2}{grad^2 + k} \times edgeSensitivity
$$

* **Hiệu ứng:** ảnh sạch giữ nguyên độ nhạy cao (giống BASE, $k \approx 12$); ảnh nhiễu tự động nâng ngưỡng $k$ lên tới 40, khiến hàm Cauchy bỏ qua nhiễu hạt nhưng vẫn giữ cạnh thật.

---

### 3. Chống viền sáng/tối bằng kẹp cục bộ (Halo Suppression — Local Clamp)

* **Vị trí hàm:** `applyHaloClamp(...)`, chạy ngay sau bước cộng dồn `diffGuided` và CAS.
* **Nguyên lý:** giá trị $Y$ sau khi làm nét không được vượt quá min/max của vùng lân cận gốc $3\times3$ nhân với hệ số nới lỏng `haloTolerance` (mặc định 1.15):

$$
Y_{min} = \min(Y_{\omega_{3\times3}}), \quad Y_{max} = \max(Y_{\omega_{3\times3}})
$$

$$
Y_{sharp}' = \text{clamp}\big(Y_{sharp},\ Y_{min} - (Y_{max}-Y_{min}) \times 0.15\, \times haloTolerance,\ \ Y_{max} + (Y_{max}-Y_{min}) \times 0.15 \times haloTolerance\big)
$$

* **Hiệu ứng:** loại bỏ hoàn toàn viền trắng/đen dọc theo cạnh tương phản mạnh (mái nhà trên nền trời, chữ đen trên nền trắng) mà không làm giảm độ nét cảm nhận.

---

### 4. Chroma Tracking trên không gian màu Oklab/OkLCh (thay YCbCr)

* **Vấn đề của BASE:** công thức $lumaRatio^{1.25}$ hoạt động tốt trên YCbCr nhưng YCbCr không đều tri giác — cùng một mức mở rộng chroma gây lệch hue khác nhau tuỳ vùng màu (đỏ lệch nhiều hơn xanh lá).
* **Giải pháp PRO:** chuyển sang không gian **Oklab**, tách thành $L$ (độ sáng đều tri giác), $C$ (chroma), $h$ (hue) theo dạng cực OkLCh:

$$
lumaRatio = \frac{L_{sharp}}{L_{center}}, \quad chromaExpansion = \text{clamp}(lumaRatio^{1.2},\ 0.85,\ 1.75)
$$

$$
C_{new} = C_{orig} \times chromaExpansion, \quad h_{new} = h_{orig} \quad \text{(giữ nguyên hue tuyệt đối)}
$$

* **Hiệu ứng thực tế:** vì $h$ được giữ cố định tuyệt đối trong không gian đều tri giác, hiện tượng lệch hue ở vùng đỏ/cam/da biến mất hoàn toàn — điều mà YCbCr không đảm bảo được do quan hệ phi tuyến với hue thật.

---

### 5. Local Laplacian Tone Mapping (thay CLAHE lưới 8×8)

* **Vấn đề của BASE:** CLAHE dùng nội suy song tuyến giữa các tile rời rạc — vẫn có nguy cơ đảo gradient nhẹ (gradient reversal) ở biên tile khi ClipLimit cao.
* **Giải pháp PRO:** áp dụng **Local Laplacian Filter** (Paris et al.) — xây dựng Laplacian Pyramid tại nhiều mức sáng tham chiếu $g$, áp hàm remap từng điểm ảnh theo mức tham chiếu gần nhất:

$$
r(i, g) = g + \text{sign}(Y_i - g) \times \alpha \times |Y_i - g|^{\beta}, \quad \beta < 1 \text{ (tăng chi tiết)}
$$

trong đó $\alpha$ tương ứng `claheBlend` của BASE, $\beta \approx 0.6-0.8$ điều khiển độ "mềm" của tương phản cục bộ.

* **Hiệu ứng:** tương phản cục bộ mượt mà tuyệt đối, không còn ranh giới tile, giữ nguyên bảo toàn cạnh (edge-preserving) — cho cảm giác "độ sâu" giống ảnh RAW xử lý chuyên nghiệp thay vì cảm giác HDR giả.

---

### 6. Tách lớp chất liệu — Texture Layer Synthesis (NEW, không có ở BASE)

* **Vị trí hàm:** `synthesizeTextureLayer(...)`
* **Nguyên lý:** dùng Bilateral Filter tách ảnh thành **lớp cấu trúc** (structure, cạnh lớn) và **lớp chất liệu** (texture, vi chi tiết lặp lại như vải, gỗ, da):

$$
Y_{structure} = \text{BilateralFilter}(Y,\ \sigma_{space}=4,\ \sigma_{range}=25)
$$

$$
Y_{texture} = Y - Y_{structure}
$$

$$
Y_{final} = Y_{structure} + Y_{texture} \times (1 + textureBoost) + diffGuided \times (1 + clarityBoost)
$$

* **Khác biệt với làm nét cạnh thông thường:** texture boost tăng "chất liệu" đều khắp bề mặt (không chỉ tại cạnh), tương đương thanh trượt *Texture* của Lightroom — giúp vải, tóc, cỏ trông "có khối" hơn mà không tạo halo như tăng `amount` thô.

---

### 7. Phục hồi vùng sáng/tối cục bộ trước khi làm nét (Highlight/Shadow Local Recovery)

* **Vấn đề:** làm nét trực tiếp trên vùng cháy sáng (highlight clip) hoặc đen sâu (shadow crush) khuếch đại nhiễu lượng tử hoá thay vì chi tiết thật.
* **Giải pháp:** áp dụng nén tông cục bộ nhẹ **trước** bước B7, dựa trên bản đồ độ sáng làm mờ mạnh (Gaussian $\sigma=30$):

$$
Y_{local} = \text{GaussianBlur}(Y, \sigma=30)
$$

$$
Y_{recovered} = Y - shadowLift \times \max(0, 0.3 - Y_{local}) + highlightPull \times \max(0, Y_{local} - 0.85)
$$

Mặc định `shadowLift = 0.08`, `highlightPull = 0.06` — đủ để "mở" chi tiết ẩn trong vùng cực sáng/tối mà không làm ảnh bị xám (flat).

---

### 8. Mặt nạ da xác suất mềm (Skin Probability Mask)

* **Vấn đề của BASE:** điều kiện nhị phân `isSkin = Cb∈[77,128] & Cr∈[133,175]` tạo ranh giới cứng, dễ lộ viền xử lý giữa da và tóc/lông mày.
* **Giải pháp PRO:** thay bằng mô hình xác suất Gaussian 2D trên không gian Cb-Cr, tâm và ma trận hiệp phương sai học từ tập mẫu da chuẩn ITU-R:

$$
P(skin) = \exp\Big(-\tfrac{1}{2}(x-\mu)^T \Sigma^{-1} (x-\mu)\Big), \quad x = (Cb, Cr)
$$

$$
smoothFactor = skinSmooth \times P(skin) \times \Big(1 - \frac{grad}{14.0}\Big)^{+}
$$

* **Hiệu ứng:** chuyển tiếp mượt giữa vùng da – tóc – mí mắt, loại bỏ hoàn toàn viền "mặt nạ" thấy được ở ảnh chân dung xử lý mạnh tay.

---

### 9. Tối ưu hiệu năng cho ảnh lớn (SIMD AVX2 + Tile Streaming)

* Toàn bộ vòng lặp pixel (Lanczos, Guided Filter, Cauchy Coring) được vector hoá bằng **AVX2 intrinsics** (`__m256`) xử lý 8 pixel float/lượt, kết hợp `#pragma omp parallel for` theo dòng.
* Với ảnh > 24MP, engine chuyển sang **Tile Streaming**: chia ảnh thành các tile 512×512 có viền chồng lấn (overlap = bán kính lọc lớn nhất, thường 8px) để tránh giới hạn RAM và giữ tính liên tục giữa các tile khi ghép lại.

---

## IV. BẢNG THAM SỐ MỞ RỘNG (`EnhanceOptionsPro`)

| Tham số mới        | Kiểu     | Ý nghĩa kỹ thuật                                               | Dải khuyến nghị |
| :------------------- | :-------- | :----------------------------------------------------------------- | :----------------- |
| `nanoDetailBoost`  | `float` | Cường độ tầng Nano-scale (r=0.5)                              | `1.10 - 1.60`    |
| `haloTolerance`    | `float` | Hệ số nới lỏng kẹp Local Clamp chống halo                    | `1.00 - 1.30`    |
| `noiseAdaptive`    | `bool`  | Bật ước lượng MAD nhiễu nền để tự chỉnh ngưỡng Cauchy | `true / false`   |
| `textureBoost`     | `float` | Cường độ lớp chất liệu (Texture Layer Synthesis)            | `0.00 - 0.60`    |
| `clarityBoost`     | `float` | Cường độ tương phản cục bộ Local Laplacian                | `0.00 - 0.50`    |
| `shadowLift`       | `float` | Mức mở chi tiết vùng tối trước khi làm nét                | `0.00 - 0.15`    |
| `highlightPull`    | `float` | Mức kéo chi tiết vùng cháy sáng                              | `0.00 - 0.12`    |
| `skinProbSigma`    | `float` | Độ mềm chuyển tiếp mặt nạ da Gaussian                       | `0.60 - 1.20`    |
| `use16BitPipeline` | `bool`  | Xử lý nội bộ 16-bit/kênh thay 8-bit (giảm banding)           | `true / false`   |

### Bảng Preset PRO mới

| Tham số                           | Level 4: PRO Ultra HD | Level 5: PRO Studio Portrait |
| :--------------------------------- | :-------------------- | :--------------------------- |
| `scalePercent`                   | 140%                  | 120%                         |
| `amount`                         | 1.70                  | 1.20                         |
| `detailBoost` (3-scale)          | 1.75                  | 1.40                         |
| `textureBoost`                   | 0.35                  | 0.20                         |
| `clarityBoost`                   | 0.30                  | 0.10                         |
| `noiseAdaptive`                  | true                  | true                         |
| `haloTolerance`                  | 1.10                  | 1.20                         |
| `isPortrait`                     | false                 | true                         |
| `skinSmooth` / `skinProbSigma` | 0.00                  | 0.42 / 0.85                  |
| `shadowLift` / `highlightPull` | 0.10 / 0.08           | 0.06 / 0.05                  |
| `use16BitPipeline`               | true                  | true                         |

---

## V. AUTO-ADAPTIVE NÂNG CẤP (LEVEL 0 — PRO)

Bổ sung so với BASE, `analyzeImageBuffer` PRO tính thêm:

1. **`noiseFloor`** (qua MAD 7×7):
   * $noiseFloor > 6.0 \implies$ bật `noiseAdaptive=true`, giảm `amount` tự động 15%.
2. **`dynamicRange`** (percentile 1%–99% histogram):
   * $dynamicRange > 200 \implies$ kích hoạt `shadowLift`/`highlightPull` mức trung bình để tránh mất chi tiết hai đầu dải sáng.
3. **Giữ nguyên logic thích ứng scale/clarity/skin của BASE**, chỉ thay giá trị `amount`/`cas` đầu ra bằng cặp tương ứng ở preset PRO.

---

## VI. TÓM TẮT KHÁC BIỆT BASE vs PRO

| Tiêu chí                    | BASE                       | PRO                                                     |
| :---------------------------- | :------------------------- | :------------------------------------------------------ |
| Số tầng phân rã chi tiết | 2 (Micro/Macro)            | 3 (Nano/Micro/Macro) + trọng số thích ứng nội dung |
| Ngưỡng khử nhiễu Cauchy   | Hằng số cố định       | Thích ứng theo MAD nhiễu nền cục bộ               |
| Chống halo                   | Không có cơ chế riêng | Local Clamp chuyên biệt                               |
| Không gian màu chroma       | YCbCr                      | Oklab/OkLCh (đều tri giác)                           |
| Tương phản cục bộ        | CLAHE lưới 8×8          | Local Laplacian Filter (không tile boundary)           |
| Lớp chất liệu (texture)    | Không tách riêng        | Texture Layer Synthesis độc lập                      |
| Vùng sáng/tối cực trị    | Không xử lý riêng      | Local Recovery trước khi sharpen                      |
| Mặt nạ da                   | Ngưỡng nhị phân        | Xác suất Gaussian mềm                                |
| Độ sâu màu nội bộ       | 8-bit                      | Tuỳ chọn 16-bit/kênh                                 |
| Hiệu năng ảnh lớn         | OpenMP thô                | AVX2 SIMD + Tile Streaming overlap                      |
