# TỔNG QUAN ĐỐI CHIẾU & NHẬN ĐỊNH: IMAGE ENHANCER (BASE) VS. PRO EDITION

> **Tài liệu tổng quan, so sánh chi tiết và nhận định kiến trúc thuật toán giữa hai bộ công cụ:**
>
> 1. [README_IMAGE_ENHANCER.md](file:///g:/Code/C++/project/CMD/README_IMAGE_ENHANCER.md) — Bản Base Standard
> 2. [README_IMAGE_ENHANCER_PRO.md](file:///g:/Code/C++/project/CMD/README_IMAGE_ENHANCER_PRO.md) — Bản Pro Edition
>
> **Mục tiêu:** Cung cấp cái nhìn toàn diện về bước chuyển dịch từ xử lý ảnh kỹ thuật số cổ điển (Base) sang nhiếp ảnh điện toán cấp độ phòng thu / flagship (Pro Edition), phân tích đánh giá ưu/nhược điểm và định hướng triển khai thực tế.

---

## I. BẢNG SO SÁNH TỔNG HỢP KIẾN TRÚC & THUẬT TOÁN

| Tiêu chí so sánh                            | Bản Chuẩn (BASE)                                                                                                                                                                                                                                                                            | Bản Chuyên sâu (PRO EDITION)                                                                                                   | Đánh giá giá trị nâng cấp                                                                             |
| :--------------------------------------------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :-------------------------------------------------------------------------------------------------------------------------------- | :----------------------------------------------------------------------------------------------------------- |
| **Không gian màu xử lý**             | ITU-R BT.601 ($Y, Cb, Cr$) | **Oklab / OkLCh** ($L, C, h$)                                                                                                                                                                                                                          | ⭐⭐⭐⭐⭐ Triệt tiêu hiện tượng lệch Hue ở vùng màu bão hòa cao (đỏ, cam, da người).                              |                                                                                                              |
| **Phân rã đa tần số**               | **2-Scale Guided Filter**• Micro ($r=1, \epsilon=300$)• Macro ($r=3, \epsilon=1200$)• Trọng số tĩnh: $1.35 / 0.65$ | **3-Scale Guided Filter**• Nano ($r=0.5, \epsilon=80$)• Micro ($r=1$)• Macro ($r=3$)• Trọng số thích ứng động theo `localFreq` | ⭐⭐⭐⭐⭐ Tách biệt được chi tiết siêu mịn (lông tơ, vân da, gân lá mảnh) mà không làm thô khối lớn.         |                                                                                                              |
| **Khử nhiễu & Giới hạn Coring**      | **Cauchy Continuous Coring** tĩnh:$k = 12.0$ cố định toàn ảnh                                                                                                                                                                                                                   | **Noise-Floor Adaptive Coring**:Hệ số $k \in [6.0, 40.0]$ thích ứng theo MAD (Median Absolute Deviation) $7\times7$ | ⭐⭐⭐⭐⭐ Chống biến nhiễu hạt (ISO cao, JPEG artifact) thành "sạn giả chi tiết".                   |
| **Kiểm soát quầng sáng (Halo)**      | Không có module chuyên biệt (chỉ dựa vào CAS & Cauchy)                                                                                                                                                                                                                                 | **Local Clamp ($3\times3$)** với hệ số `haloTolerance` nới lỏng biên an toàn                                     | ⭐⭐⭐⭐ Khử triệt để viền trắng/đen viền tương phản cao (mái nhà nền trời, văn bản).       |
| **Tăng cường tương phản cục bộ** | **CLAHE** (lưới $8\times8$ ô, ngưỡng cắt clipLimit = 2.5, nội suy Bilinear)                                                                                                                                                                                                    | **Local Laplacian Tone Mapping** (Laplacian Pyramid đa mức tham chiếu)                                                   | ⭐⭐⭐⭐ Loại bỏ hiện tượng phẳng ảnh và ranh giới ô (blocking), tạo chiều sâu 3D tự nhiên.   |
| **Tách lớp chất liệu (Texture)**     | Không có (gộp chung cạnh biên và bề mặt)                                                                                                                                                                                                                                              | **Texture Layer Synthesis** (Bilateral Filter tách riêng Structure và Texture)                                           | ⭐⭐⭐⭐ Tương đương thanh trượt Clarity & Texture của Adobe Lightroom.                              |
| **Xử lý dải động (Dynamic Range)**  | Clamp giá trị sau khi xử lý (Soft Gamut Roll-off)                                                                                                                                                                                                                                         | **Highlight/Shadow Local Recovery** bằng Gaussian $\sigma=30$ chạy trước Sharpen                                      | ⭐⭐⭐⭐ Cứu chi tiết vùng cháy sáng/tối mịt trước khi làm nét, tránh khuếch đại noise đáy. |
| **Bảo vệ chân dung (Portrait)**       | Phân vùng nhị phân cứng dựa trên ngưỡng dải$Cb \in [77, 128], Cr \in [133, 175]$                                                                                                                                                                                                  | **Skin Probability Mask** xác suất Gaussian 2D chuyển tiếp mềm                                                         | ⭐⭐⭐⭐ Loại bỏ viền "cắt dán" mặt nạ giả tạo giữa da và tóc/mí mắt.                          |
| **Độ sâu bit nội bộ**               | 8-bit sRGB / BGRA 32bpp                                                                                                                                                                                                                                                                       | Hỗ trợ pipeline**16-bit/kênh** & Linear-Light RAW                                                                        | ⭐⭐⭐ Giảm thiểu hiện tượng đứt gãy màu (color banding) khi hậu kỳ nặng.                        |
| **Cơ chế tối ưu hiệu năng**        | Đa luồng CPU thuần OpenMP (`#pragma omp parallel for`)                                                                                                                                                                                                                                   | **SIMD AVX2 Intrinsics** (`__m256`) + **Tile Streaming** (512x512 overlap) cho ảnh > 24MP                          | ⭐⭐⭐⭐ Xử lý mượt mà ảnh kích thước cực lớn mà không tràn RAM hệ thống.                    |

---

## II. PHÂN TÍCH CHUYÊN SÂU CÁC ĐỘT PHÁ CỦA BẢN PRO

### 1. Bước nhảy vọt về nhận thức thị giác: YCbCr sang Oklab/OkLCh

* **Hạn chế của Base:** Mô hình $YCbCr$ của ITU-R BT.601 vốn được thiết kế cho truyền hình analog thế kỷ trước. Nó không phải là không gian màu đều tri giác (Perceptually Uniform). Khi tăng nét mạnh ở kênh Luma ($Y$) rồi scale Chroma theo công thức $lumaRatio^{1.25}$, góc màu (Hue) thực tế bị trôi lệch, biểu hiện rõ nhất ở sắc tố đỏ tươi bị ngả cam hoặc màu da người bị vàng vọt.
* **Đột phá của Pro:** Không gian Oklab (Ottosson, 2020) mang tính đồng nhất tri giác cao nhất hiện nay. Bản Pro chuyển sang dạng cực $OkLCh$ ($L$: Lightness, $C$: Chroma, $h$: Hue angle). Quá trình tăng nét chỉ tác động lên $L$ và điều biến độ no màu $C$, trong khi góc màu $h$ được **khóa bất biến tuyệt đối ($h_{new} = h_{orig}$)**. Nhờ đó, màu sắc giữ được 100% bản sắc gốc dù tăng tương phản cực hạn.

### 2. Xử lý đa tần số thích ứng thực sự: 3-Scale vs. 2-Scale

* **Ở bản Base:** 2 tầng Guided Filter (Micro $r=1$, Macro $r=3$) chia dải tần thành 2 phần và cộng gộp với tỷ lệ cố định $1.35 : 0.65$. Cách này hoạt động tốt với ảnh phong cảnh thông thường, nhưng gặp khó khăn khi gặp bề mặt da người (cần làm mịn) nằm cạnh sợi tóc siêu mảnh (cần làm nét).
* **Ở bản Pro:** Bổ sung tầng **Nano-scale ($r=0.5, \epsilon=80$)** có khả năng can thiệp dưới mức pixel. Đặc biệt, hệ thống sử dụng bản đồ tần số cục bộ `localFrequencyMap` tính từ phương sai Laplacian:
  * Vùng giàu chi tiết cao tần (gân lá, sợi vải, tóc): tự động đẩy trọng số $w_{nano}$ lên tối đa, hạ $w_{macro}$.
  * Vùng đồng nhất (bầu trời, má người, mảng tường): tự động hạ $w_{nano}$, ưu tiên $w_{macro}$ để giữ độ êm ái, phẳng mịn, triệt tiêu sạn nhiễu.

### 3. Kiểm soát nhiễu thích ứng: Adaptive MAD Cauchy Coring

* **Hàm Cauchy của Base:** Sử dụng mẫu số $grad^2 + 12.0$. Giá trị $12.0$ là hằng số kinh nghiệm, chỉ tối ưu cho ảnh sạch chuẩn ISO 100. Khi đưa vào ảnh chụp đêm, ảnh ISO cao (như ISO 1600 - 6400) hoặc ảnh nén nặng từ mạng xã hội, các đốm nhiễu hạt có gradient $\approx 4 - 8$ vẫn bị hàm Cauchy giữ lại và CAS khuếch đại lên thành hạt muối tiêu rất thô.
* **Cơ chế MAD của Pro:** Đo đạc độ phân tán tuyệt đối trung vị (Median Absolute Deviation) trong cửa sổ $7\times7$ tại các vùng đồng nhất. Từ đó tính ra độ lệch chuẩn nhiễu thực tế $\sigma_{noise}$. Ngưỡng lọc $k$ sẽ tự co giãn linh hoạt từ $6.0$ (ảnh cực nét, sạch) tới $40.0$ (ảnh rất nhiễu). Điều này đảm bảo: **ảnh nhiễu sẽ được làm sạch thông minh trước khi sắc cạnh**.

### 4. Xóa bỏ hiện tượng quầng sáng (Halo-Free Sharpening)

* Mọi thuật toán tăng cường độ nét cổ điển (Unsharp Mask, CAS, Laplacian) khi gặp ranh giới tương phản cực cao (ví dụ: viền nóc nhà màu đen tương phản với nền trời trắng) đều có xu hướng đẩy các pixel cạnh trời sáng hơn bình thường và cạnh tường tối hơn bình thường, tạo ra vệt viền trắng/đen (quầng Halo).
* Bản Pro giải quyết triệt để vấn đề này bằng **Local Clamp**: Giá trị độ sáng sau làm nét bắt buộc phải nằm trong giới hạn bao lân cận của cửa sổ $3\times3$ gốc có nới lỏng dung sai kiểm soát (`haloTolerance`). Nhờ đó, đường nét sắc bén tự nhiên, không lộ dấu vết can thiệp kỹ thuật số.

---

## III. ĐÁNH GIÁ TOÀN DIỆN: ƯU ĐIỂM & NHƯỢC ĐIỂM CỦA TỪNG PHIÊN BẢN

```text
               ┌──────────────────────────────────────────────────┐
               │           BẢN BASE (STANDARD ENHANCER)           │
               ├──────────────────────────────────────────────────┤
               │ Ưu điểm:                                         │
               │  • Tốc độ xử lý cực nhanh, ngốn rất ít CPU/RAM.   │
               │  • Thuật toán ổn định, gọn gàng, dễ bảo trì.     │
               │  • Rất hiệu quả cho ảnh thông thường, ảnh web.   │
               │ Nhược điểm:                                      │
               │  • Dễ sinh sạn nếu ảnh gốc có ISO cao.           │
               │  • Quầng sáng nhẹ (halo) ở ảnh tương phản gắt.   │
               │  • Lệch hue nhẹ ở vùng màu rực rỡ.               │
               └──────────────────────────────────────────────────┘
                                        │
                                        ▼  Nâng cấp toàn diện
               ┌──────────────────────────────────────────────────┐
               │              BẢN PRO (PRO EDITION)               │
               ├──────────────────────────────────────────────────┤
               │ Ưu điểm:                                         │
               │  • Chất lượng đầu ra đạt chuẩn Studio / Flagship.│
               │  • Giữ màu tuyệt đối nhờ Oklab/OkLCh.            │
               │  • Chống quầng sáng Halo và khử sạn thích ứng.   │
               │  • Tách riêng Texture và Tonal Depth 3D.         │
               │  • Mặt nạ da mềm mượt tự nhiên, hỗ trợ 16-bit.   │
               │ Nhược điểm:                                      │
               │  • Chi phí tính toán cao hơn khoảng 2.0x - 3.5x. │
               │  • Cần tập lệnh AVX2 để đạt hiệu suất tối ưu.    │
               │  • Độ phức tạp mã nguồn và cấu hình tham số cao. │
               └──────────────────────────────────────────────────┘
```

### 1. Khi nào nên dùng bản Chuẩn (BASE)?

* **Xử lý hàng loạt (Batch Processing):** Cần xử lý hàng trăm, hàng nghìn ảnh sản phẩm e-commerce hoặc ảnh chụp màn hình trong thời gian ngắn.
* **Môi trường phần cứng hạn chế:** Máy tính cấu hình văn phòng, CPU đời cũ không hỗ trợ tập lệnh SIMD mở rộng hoặc môi trường ảo hóa bị giới hạn tài nguyên tính toán.
* **Nguồn ảnh đầu vào ổn định:** Ảnh chụp ban ngày đủ sáng, ảnh đồ họa vector hoặc ảnh đã qua xử lý trước sạch sẽ.

### 2. Khi nào bắt buộc cần bản Chuyên sâu (PRO)?

* **Phục hồi ảnh cũ, ảnh hỏng, ảnh chụp đêm ISO cao:** Nơi mà nhiễu hạt nặng nề đòi hỏi cơ chế thích ứng MAD để không biến ảnh thành bức tranh chấm hạt.
* **Chân dung nghệ thuật & Chụp người (Studio Portrait):** Nơi ranh giới giữa da mặt, viền môi, mí mắt và sợi tóc cần sự chuyển tiếp mượt mà của Gaussian Soft Mask và Nano-Scale.
* **Ảnh phong cảnh nghệ thuật dải tương phản cao (HDR/RAW):** Cần Local Laplacian Tone Mapping để tái hiện độ sâu của mây trời, vách đá mà không bị hiệu ứng HDR giả tạo hay vỡ mảng tương phản.
* **In ấn ấn phẩm chất lượng cao:** Đòi hỏi đường biên sắc nét không viền quầng (Halo-free) và không bị đứt gãy dải màu (Color Banding).

---

## IV. MÔ HÌNH TRIỂN KHAI TRONG HỆ THỐNG CMD BOX (TỰ ĐỘNG 100%)

Nhằm tối ưu hóa trải nghiệm người dùng trong dự án **CMD BOX**, hệ thống **loại bỏ hoàn toàn bước chọn Level thủ công**, chuyển đổi toàn diện sang quy trình **Tự động chấm điểm, Phân tích đa yếu tố và Render trực tiếp**:

1. **Bản Nâng cao (PRO EDITION) — Chấm điểm 7 yếu tố & Bù thích ứng liên tục:**
   * Tự động quét và chấm điểm đa chiều: *Clarity Score (độ sắc nét), Noise Score (nhiễu nền MAD & SNR dB), Dynamic Range (dải động & bết tối/cháy sáng), Texture Score (độ phức tạp bề mặt), Thin Feature Ratio (tỷ lệ nét mảnh), Nhận diện đối tượng & Tỷ lệ da mặt*.
   * Tự động nội suy bù điểm ảnh Lanczos-3 kèm thuật toán chống quầng giả (Anti-Ringing Clamping).
   * Tự động tính toán bộ tham số bù liên tục (Continuous Compensation Functions) áp dụng chuỗi render cao cấp Oklab/OkLCh.
2. **Bản Cơ bản (BASE) — Tự động thích ứng thông minh (Auto-Adaptive):**
   * Tự động phân tích ma trận kích thước (Megapixels), mật độ nén (BPP), độ sắc nét gradient và tỷ lệ da mặt.
   * Tự động điều chỉnh tỷ lệ phóng đại Lanczos-3 (100% - 150%) và cân bằng tương phản thích ứng CLAHE kết hợp 2-Scale Guided Filter.
   * Ưu tiên tốc độ cực nhanh cho xử lý ảnh số lượng lớn hoặc môi trường máy tính văn phòng.

---

## V. KẾT LUẬN

Bản **ImageEnhancer Pro Edition** không đơn thuần là sự tinh chỉnh tham số của bản Base, mà là một **sự tái thiết kế toàn diện về mặt toán học và thị giác máy tính**. Nó giải quyết triệt để 5 "điểm nghẽn" cố hữu của xử lý ảnh truyền thống: *nhiễu hạt phóng đại, quầng viền tương phản (halo), lệch màu bão hòa, bệt khối tương phản và ranh giới mặt nạ cứng*.

Sự hiện diện song hành của cả hai phiên bản tạo nên một giải pháp xử lý hình ảnh toàn diện, linh hoạt từ tốc độ cho đến chất lượng chuyên nghiệp trong hệ sinh thái mã nguồn C++ của dự án CMD BOX.
