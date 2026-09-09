# ĐẶC TẢ KIẾN TRÚC & THUẬT TOÁN LÀM NÉT ẢNH CHUYÊN SÂU: IMAGE ENHANCER ENGINE

> **Tài liệu nghiên cứu kỹ thuật, công thức toán học và thiết kế kiến trúc module `ImageEnhancer`**  
> **Vị trí mã nguồn:** [`include/ImageEnhancer.h`](include/ImageEnhancer.h) & [`src/ImageEnhancer.cpp`](src/ImageEnhancer.cpp)  
> **Ngôn ngữ:** C++17 Native | **Thư viện đồ họa:** Windows Imaging Component (WIC) | **Gia tốc:** OpenMP CPU Multi-threading

---

## I. TỔNG QUAN & MỤC TIÊU THIẾT KẾ (BASE)

Module `ImageEnhancer` được xây dựng thuần C++ trên nền Win32 / WIC nhằm cung cấp bộ công cụ phục chế và nâng cao độ nét hình ảnh đạt chuẩn đồ họa chuyên nghiệp mà **hoàn toàn không phụ thuộc vào FFmpeg hay các phần mềm bên ngoài**.

### 2 Mục tiêu cốt lõi giải quyết triệt để các hạn chế truyền thống:
1. **Triệt tiêu hiện tượng "Bệt ảnh" & "Dính cụm điểm ảnh" (Anti-Posterization & Anti-Clustering):**
   * *Vấn đề cũ:* Các thuật toán Unsharp Mask hoặc Bicubic truyền thống làm nhòe các pixel lân cận vào nhau hoặc cắt cụt biên độ biến thiên vi mô, biến các điểm ảnh $1\ 1\ 1$ thành các mảng bệt to $111\ 111\ 111$.
   * *Giải pháp:* Thay thế bằng nội suy bảo toàn tần số cao **Lanczos-3**, bộ lọc dẫn đường đa tầng **2-Scale Guided Filter** và hàm làm trơn liên tục **Cauchy Continuous Coring**.
2. **Bảo toàn độ bão hòa màu sắc (Anti-Desaturation / Constant-Saturation Chroma Tracking):**
   * *Vấn đề cũ:* Khi độ sáng Luma ($Y$) ở gân lá cây hoặc đường viền tăng cao, tỷ lệ độ bão hòa màu sắc bị loãng, dẫn đến hiện tượng "viền trắng sáng" hoặc lá cây bị biến từ xanh lục thẫm thành màu xanh trắng bạc nhợt nhạt.
   * *Giải pháp:* Tự động mở rộng không gian màu Chroma theo tỷ lệ đồng dạng độ sáng ($lumaRatio^{1.25}$) kết hợp nén dải màu mềm mại (**Soft Gamut Roll-off**).

---

## II. SƠ ĐỒ PIPELINE XỬ LÝ TOÀN DIỆN (7 BƯỚC)

```text
               [FILE ẢNH ĐẦU VÀO] (JPG, PNG, BMP, TIFF, HEIC, DNG, WebP)
                                      │
                                      ▼
             ┌──────────────────────────────────────────────────┐
             │ BƯỚC 1: Giải mã WIC Native sang BGRA 32bpp       │
             └──────────────────────────────────────────────────┘
                                      │
                                      ▼
             ┌──────────────────────────────────────────────────┐
             │ BƯỚC 2: Phân tích ma trận ảnh (analyzeImageBuffer)│
             │         - Tính Megapixels & BPP (Bytes Per Pixel)│
             │         - Đo độ sắc nét gradient (clarityScore)  │
             │         - Nhận diện vùng da ITU-R (skinPercent)  │
             └──────────────────────────────────────────────────┘
                                      │
                                      ▼
             ┌──────────────────────────────────────────────────┐
             │ BƯỚC 3: Auto-Adaptive Presets & Lanczos-3 Scale  │
             │         - Phóng đại thích ứng: 100% -> 150%      │
             │         - Bảo toàn dải tần số cao Nyquist 6x6    │
             └──────────────────────────────────────────────────┘
                                      │
                                      ▼
             ┌──────────────────────────────────────────────────┐
             │ BƯỚC 4: Tách kênh Luma/Chroma (ITU-R BT.601)     │
             │         - Tách kênh Y (Luma) & Cb, Cr            │
             │         - Lưu độc lập RGB gốc (origR, origG, origB)│
             └──────────────────────────────────────────────────┘
                                      │
                                      ▼
             ┌──────────────────────────────────────────────────┐
             │ BƯỚC 5: Tương phản thích ứng cục bộ CLAHE        │
             │         - Chia lưới 8x8 tiles, ClipLimit = 2.5   │
             │         - Nội suy Bilinear giữa 4 tile lân cận   │
             └──────────────────────────────────────────────────┘
                                      │
                                      ▼
             ┌──────────────────────────────────────────────────┐
             │ BƯỚC 6: Phân rã 2-Scale Guided Filter (He et al.)│
             │         - Micro-scale (r=1, eps=300): Gân lá, tóc│
             │         - Macro-scale (r=3, eps=1200): Khối nổi  │
             └──────────────────────────────────────────────────┘
                                      │
                                      ▼
             ┌──────────────────────────────────────────────────┐
             │ BƯỚC 7: Pipeline Làm nét & Tái tạo màu sắc       │
             │         - Cauchy Continuous Coring (Khử bệt)     │
             │         - Contrast Adaptive Sharpening (CAS)     │
             │         - Dual-Zone Portrait: Bảo vệ & làm mịn da│
             │         - Constant-Saturation Chroma Tracking    │
             │         - Soft Gamut Roll-off (Chống cháy màu)   │
             └──────────────────────────────────────────────────┘
                                      │
                                      ▼
             [FILE ẢNH ĐẦU RA] (Ghi bằng WIC Encoder chất lượng cao)
```

---

## III. CHI TIẾT CÁC THUẬT TOÁN CỐT LÕI & CÔNG THỨC TOÁN HỌC (BASE)

### 1. Hạt nhân nội suy Lanczos-3 (Lanczos-3 Resampling)
* **Vị trí hàm:** `lanczos3Kernel(float x)` & `lanczos3Resample(...)`
* **Công thức toán học:**
  $$L(x) = \begin{cases} 
  1 & \text{khi } x = 0 \\
  \text{sinc}(x) \cdot \text{sinc}(x / 3) = \dfrac{3 \sin(\pi x) \sin(\pi x / 3)}{\pi^2 x^2} & \text{khi } 0 < |x| < 3 \\
  0 & \text{khi } |x| \ge 3
  \end{cases}$$
* **Đặc tính kỹ thuật:**
  * Bán kính cửa sổ $r = 3$, quét vùng lân cận $6 \times 6 = 36$ điểm ảnh xung quanh.
  * Tái tạo hàm Sinc lý thuyết với cửa sổ triệt tiêu biên, hạn chế tối đa hiện tượng răng cưa (aliasing) và làm nhòe khối (blurring) của Bicubic.
  * Phân rã đa luồng OpenMP tĩnh theo từng dòng quét (`#pragma omp parallel for schedule(static)`).

---

### 2. Cân bằng tương phản thích ứng giới hạn ngưỡng CLAHE
* **Vị trí hàm:** `applyCLAHE(std::vector<float>& luma, int width, int height, float clipLimit, float blendFactor)`
* **Thuật toán:**
  1. Chia ma trận kênh sáng $Y$ thành lưới $8 \times 8 = 64$ ô (tiles).
  2. Tính lược đồ mức xám (Histogram 256 mức) cho từng ô độc lập.
  3. Cắt ngưỡng tại `clipLimit = 2.5` để chống khuếch đại nhiễu hạt ở vùng tối, tái phân bổ lượng pixel dư thừa đều vào 256 bins.
  4. Tính hàm phân phối tích lũy (CDF) để sinh bảng ánh xạ mức xám mới cho từng ô.
  5. Áp dụng nội suy song tuyến tính (Bilinear Interpolation) giữa 4 ô bao quanh mỗi pixel để loại bỏ hoàn toàn hiện tượng đường ranh giới giữa các ô (blocking artifacts).

---

### 3. Bộ lọc dẫn đường 2 tầng (2-Scale Guided Filter)
* **Vị trí hàm:** `applyGuidedFilter(...)` & khối thực thi trong `processSharpenYCbCr`
* **Công thức toán học (Kaiming He):**
  Bộ lọc giả định mối quan hệ tuyến tính cục bộ giữa ảnh lọc $q$ và ảnh dẫn đường $I$ trong cửa sổ $\omega_k$:
  $$q_i = a_k I_i + b_k \quad (\forall i \in \omega_k)$$
  $$a_k = \frac{\frac{1}{|\omega|}\sum_{i \in \omega_k} I_i p_i - \mu_k \bar{p}_k}{\sigma_k^2 + \epsilon}, \quad b_k = \bar{p}_k - a_k \mu_k$$
* **Cơ chế phân rã 2 tầng trong ImageEnhancer:**
  * **Tầng vi mô (Micro-scale $r=1, \epsilon=300.0$):**
    $$microDetail = Y_{center} - baseLumaMicro[idx]$$
    Cô lập chính xác các chi tiết tần số cực cao ở cấp độ **1 pixel riêng lẻ** (sợi gân lá siêu mảnh, sợi lông mi, đường vân tóc).
  * **Tầng cấu trúc (Macro-scale $r=3, \epsilon=1200.0$):**
    $$macroDetail = baseLumaMicro[idx] - baseLumaMacro[idx]$$
    Bảo toàn độ khối nổi, độ sâu 3D của tán cây và cấu trúc xương mặt.
  * **Độ bù tổng hợp:**
    $$diffGuided = (microDetail \times 1.35 + macroDetail \times 0.65) \times (opts.detailBoost - 1.0)$$

---

### 4. Khử bệt ảnh bằng hàm Cauchy liên tục (Cauchy Continuous Coring)
* **Vấn đề của thuật toán cũ:**
  ```cpp
  // LỖI CŨ: Cắt ngưỡng nhị phân làm bệt hạt
  if (grad < 3.2f) edgeWeight = 0.0f; // Toàn bộ gân lá mịn bên trong tán cây bị san phẳng!
  ```
* **Công thức Cauchy mới:**
  $$grad = |Y_{right} - Y_{left}| + |Y_{bottom} - Y_{top}|$$
  $$edgeWeight = \frac{grad^2}{grad^2 + 12.0} \times edgeSensitivity$$
* **Nguyên lý:** 
  * Khi gradient vi mô rất nhỏ (nhiễu hạt nền), hàm tiệm cận 0 một cách mượt mà.
  * Khi gradient trung bình (vân gân lá, nếp vải), hàm giữ lại từ 40% - 80% chi tiết thay vì chặt cụt về 0.
  * Khi gradient lớn (đường viền nét), hàm tiệm cận 1.0.

---

### 5. Thuật toán bù màu Constant-Saturation Chroma Tracking
* **Vấn đề:** Khi độ sáng tăng ở đường viền gân lá ($Y_{sharp} > Y_{center}$), nếu giữ nguyên hiệu màu $(R - Y)$, màu sẽ bị rửa trôi thành trắng xanh.
* **Công thức bù màu đồng dạng:**
  $$lumaRatio = \frac{Y_{sharp}}{Y_{center}}$$
  $$chromaExpansion = \text{clamp}(lumaRatio^{1.25},\ 0.85,\ 1.75)$$
  $$R_{new} = Y_{sharp} + (R_{orig} - Y_{center}) \times chromaExpansion$$
  $$G_{new} = Y_{sharp} + (G_{orig} - Y_{center}) \times chromaExpansion$$
  $$B_{new} = Y_{sharp} + (B_{orig} - Y_{center}) \times chromaExpansion$$
* **Hiệu ứng thực tế:** Sắc xanh thẫm tự nhiên của lá cây được giữ nguyên 100%, không bị xuất hiện quầng viền trắng nhợt.

---

### 6. Chống cháy sáng & Lệch màu (Soft Gamut Roll-off)
* **Vấn đề:** Khi kênh Green được tăng nét mạnh có thể vượt quá trần 255. Nếu dùng `clamp(G, 0, 255)` đơn lẻ, tỷ lệ $R:G:B$ bị bóp méo, làm biến dạng màu tại đỉnh sáng.
* **Công thức co tỷ lệ mềm:**
  $$maxComponent = \max(R_{new}, G_{new}, B_{new})$$
  $$\text{Nếu } maxComponent > 255.0 \implies compression = \frac{255.0}{maxComponent}$$
  $$R = R \times compression, \quad G = G \times compression, \quad B = B \times compression$$

---

### 7. Bảo vệ chân dung thông minh (Dual-Zone Portrait Protection)
* **Nhận diện màu da chuẩn ITU-R BT.601:**
  $$isSkin = (Cb \in [77.0, 128.0]) \ \& \ (Cr \in [133.0, 175.0])$$
* **Phân vùng 2 lớp (Dual-Zone):**
  * *Vùng da phẳng ($grad < 14.0$):* Áp dụng làm mịn vi hạt theo tỷ lệ nghịch với gradient:
    $$smoothFactor = skinSmooth \times (1.0 - \frac{grad}{14.0})$$
    $$Y_{sharp} = Y_{sharp} \times (1.0 - smoothFactor) + (Y_{center} \times 0.75 + Y_{blur} \times 0.25) \times smoothFactor$$
  * *Vùng ngũ quan & tóc ($grad \ge 14.0$ hoặc ngoài vùng da):* Mắt, lông mi, lông mày, bờ môi và sợi tóc nằm ngoài điều kiện lọc làm mịn, được hưởng trọn bộ tăng nét vi mô $r=1$ từ Guided Filter.

---

## IV. BẢNG THAM SỐ CẤU HÌNH HỆ THỐNG (BASE)

### 1. Ý nghĩa các tham số trong `EnhanceOptions`
| Tham số | Kiểu | Ý nghĩa kỹ thuật | Dải thích ứng tự động |
| :--- | :--- | :--- | :--- |
| `amount` | `float` | Hệ số khuếch đại nét tổng thể | `0.90 - 1.60` (Tự động theo `clarityScore`) |
| `radius` | `int` | Bán kính mặt nạ mờ Gaussian 3-pass | `2` (Tối ưu cho vi mô) |
| `threshold` | `float` | Ngưỡng phân tách nhiễu và chi tiết thực | `1.4 - 2.5` |
| `edgeSensitivity` | `float` | Độ nhạy biên độ của hàm Cauchy | `1.15 - 1.35` |
| `contrast` | `float` | Hệ số tương phản cục bộ S-Curve | `1.04 - 1.06` |
| `vibrance` | `float` | Độ tươi màu thông minh (tăng vùng màu nhạt) | `0.05 - 0.08` |
| `scalePercent` | `int` | Tỷ lệ phóng đại bù điểm ảnh Lanczos-3 | `100% - 150%` (Tự động theo Megapixels) |
| `casStrength` | `float` | Trọng số bộ lọc tương phản thích ứng CAS | `0.70 - 1.15` |
| `isPortrait` | `bool` | Tự động kích hoạt chế độ bảo vệ da chân dung | `true` khi `skinPercent >= 8.0%` |
| `skinSmooth` | `float` | Cường độ làm mịn da mặt | `0.40 - 0.45` |
| `claheBlend` | `float` | Tỷ lệ hòa trộn tương phản thích ứng CLAHE | `0.15 - 0.30` |
| `detailBoost` | `float` | Hệ số tăng cường vi chi tiết 2-Scale Guided Filter | `1.30 - 1.60` |

---

## V. HỆ THỐNG TỰ ĐỘNG CHẤM ĐIỂM & PHÂN TÍCH YẾU TỐ 100% (AUTO-ADAPTIVE PIPELINE)

> **Loại bỏ hoàn toàn cơ chế chọn Level thủ công.** Hệ thống tự động phân tích ma trận điểm ảnh thông qua hàm `analyzeImageBuffer`, tự lượng hóa chất lượng ảnh đầu vào, tự quyết định tỷ lệ nội suy Lanczos-3 và bộ tham số tối ưu, sau đó render xuất file hoàn toàn tự động.

### 1. Phân tích ma trận đầu vào (`analyzeImageBuffer`)
1. **Định lượng kích thước & mật độ dữ liệu:**
   * $\text{MegaPixels} = (\text{Width} \times \text{Height}) / 10^6$
   * $\text{BPP (Bytes Per Pixel)} = \text{FileSize} / (\text{Width} \times \text{Height})$ — nhận biết mức độ nén của ảnh nguồn.
2. **Chấm điểm độ sắc nét vi mô (`clarityScore` 0 - 100):**
   * Tính toán tổng biến thiên gradient cục bộ vi mô: $\text{grad} = |Y(x+1, y) - Y(x-1, y)| + |Y(x, y+1) - Y(x, y-1)|$.
   * Chuẩn hóa về thang điểm 100: Điểm càng thấp biểu thị ảnh càng nhòe mờ, out-focus hoặc bị nén bệt.
3. **Phân tích đối tượng & tỷ lệ da người (`skinPercent`):**
   * Quét phân bố sắc độ không gian ITU-R BT.601 ($Cb \in [77, 128], Cr \in [133, 175]$).
   * Phân loại tự động: Ảnh Chân dung (`isPortrait = true` khi $\ge 8.0\%$) hoặc Ảnh Phong cảnh / Kiến trúc / Văn bản.

### 2. Tự động ra quyết định nội suy & Bù thông số (Adaptive Decision & Render)
* **Quyết định tỷ lệ nội suy bù điểm ảnh (Lanczos-3 Resampling):**
  * $\text{MegaPixels} < 0.6 \implies \text{Scale } 150\%$ (Bù điểm ảnh mạnh mẽ cho ảnh kích thước nhỏ, ảnh icon, avatar).
  * $0.6 \le \text{MegaPixels} < 1.8 \implies \text{Scale } 130\%$.
  * $1.8 \le \text{MegaPixels} < 4.0 \implies \text{Scale } 115\%$.
  * $\text{MegaPixels} \ge 4.0 \implies \text{Scale } 100\%$ (Ảnh 4K+ giữ nguyên phân giải gốc, tránh lãng phí tài nguyên).
* **Quyết định mức độ làm nét thích ứng theo điểm `clarityScore`:**
  * $\text{Clarity} < 40/100 \implies amount = 1.60, cas = 1.15, clahe = 0.30$ (Bù nét sâu cho ảnh mờ).
  * $40 \le \text{Clarity} < 70 \implies amount = 1.25, cas = 0.90, clahe = 0.22$ (Cân bằng tự nhiên).
  * $\text{Clarity} \ge 70/100 \implies amount = 0.90, cas = 0.70, clahe = 0.15$ (Ảnh đã nét sẵn, chỉ bảo toàn và đẩy chi tiết vi mô).
* **Kết xuất (Render):** Chạy luồng phân rã 2-Scale Guided Filter, Cauchy Coring, Chroma Tracking và xuất file ra đĩa bằng WIC Encoder chất lượng cao.


