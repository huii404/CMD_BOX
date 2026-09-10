# KẾT QUẢ ĐỐI CHIẾU THỰC NGHIỆM ĐỘC LẬP: BASE VS PRO (IMAGE_TEST DATASET)

Thời gian thực thi: Sep 10 2026 10:06:37

### Tệp: `anh_buoi_toi.jpg` (640x480 - 0.31 MP)

| Tiêu chí so sánh | Bản Gốc (ORIG) | Bản BASE | Bản PRO (V2) | Nhận xét chi tiết |
| :--- | :---: | :---: | :---: | :--- |
| **Ngữ cảnh phân loại** | — | Chân dung (36% da) | **Phong cảnh / Chi tiết cao (Landscape)** | PRO phân loại chuyên sâu hơn |
| **Thời gian xử lý** | — | **124 ms** | 134 ms | BASE nhanh hơn 1.1x |
| **Độ dốc trung bình (`AvgGrad`)** | 8.36 | 9.84 | **12.57** | PRO sắc nét hơn |
| **Tần số vi mô (`AvgLap`)** | 21.41 | 17.53 | **24.24** | Chi tiết vi mô gân lá/sợi tóc |
| **Năng lượng biên Tenengrad** | 15.58 | 18.40 | **22.66** | Độ sắc cạnh biên |
| **Tỷ lệ tần số cao (`HighFreq%`)** | 42.95% | 42.33% | **48.15%** | Mật độ chi tiết đanh nét |
| **Dải động tương phản (`DynRange`)** | 167.00 | 179.00 | **192.00** | Khối sáng tối tương phản |
| **Nhiễu nền (`NoiseFloor MAD`)** | 0.72 | 0.03 | **1.48** | PRO nhạy noise hơn |
| **Tỷ lệ màu da Melanin** | 7.83% | 35.90% (Thô) | **5.81% (Melanin)** | Độ chính xác mặt nạ da |

### Tệp: `buoi_toi_canh_quan.jpg` (780x975 - 0.76 MP)

| Tiêu chí so sánh | Bản Gốc (ORIG) | Bản BASE | Bản PRO (V2) | Nhận xét chi tiết |
| :--- | :---: | :---: | :---: | :--- |
| **Ngữ cảnh phân loại** | — | Ảnh mờ / Nén thấp | **Phong cảnh / Chi tiết cao (Landscape)** | PRO phân loại chuyên sâu hơn |
| **Thời gian xử lý** | — | **186 ms** | 242 ms | BASE nhanh hơn 1.3x |
| **Độ dốc trung bình (`AvgGrad`)** | 6.80 | 7.98 | **9.86** | PRO sắc nét hơn |
| **Tần số vi mô (`AvgLap`)** | 8.86 | 11.98 | **15.68** | Chi tiết vi mô gân lá/sợi tóc |
| **Năng lượng biên Tenengrad** | 11.77 | 14.39 | **17.21** | Độ sắc cạnh biên |
| **Tỷ lệ tần số cao (`HighFreq%`)** | 36.73% | 40.76% | **50.40%** | Mật độ chi tiết đanh nét |
| **Dải động tương phản (`DynRange`)** | 184.00 | 185.00 | **190.00** | Khối sáng tối tương phản |
| **Nhiễu nền (`NoiseFloor MAD`)** | 1.48 | 1.47 | **2.46** | PRO nhạy noise hơn |
| **Tỷ lệ màu da Melanin** | 0.00% | 0.01% (Thô) | **0.00% (Melanin)** | Độ chính xác mặt nạ da |

### Tệp: `chan_dung1.jpg` (1500x1000 - 1.50 MP)

| Tiêu chí so sánh | Bản Gốc (ORIG) | Bản BASE | Bản PRO (V2) | Nhận xét chi tiết |
| :--- | :---: | :---: | :---: | :--- |
| **Ngữ cảnh phân loại** | — | Chân dung (9% da) | **Ảnh mờ / Cần phục hồi nét (Blur/Defocus)** | PRO phân loại chuyên sâu hơn |
| **Thời gian xử lý** | — | **384 ms** | 484 ms | BASE nhanh hơn 1.3x |
| **Độ dốc trung bình (`AvgGrad`)** | 1.78 | 1.52 | **2.04** | PRO sắc nét hơn |
| **Tần số vi mô (`AvgLap`)** | 3.15 | 4.08 | **6.06** | Chi tiết vi mô gân lá/sợi tóc |
| **Năng lượng biên Tenengrad** | 7.00 | 5.61 | **6.05** | Độ sắc cạnh biên |
| **Tỷ lệ tần số cao (`HighFreq%`)** | 8.59% | 10.88% | **17.77%** | Mật độ chi tiết đanh nét |
| **Dải động tương phản (`DynRange`)** | 199.00 | 205.00 | **209.00** | Khối sáng tối tương phản |
| **Nhiễu nền (`NoiseFloor MAD`)** | 0.00 | 0.74 | **1.56** | PRO nhạy noise hơn |
| **Tỷ lệ màu da Melanin** | 6.18% | 8.62% (Thô) | **5.61% (Melanin)** | Độ chính xác mặt nạ da |

### Tệp: `chan_dung2.jpg` (736x1308 - 0.96 MP)

| Tiêu chí so sánh | Bản Gốc (ORIG) | Bản BASE | Bản PRO (V2) | Nhận xét chi tiết |
| :--- | :---: | :---: | :---: | :--- |
| **Ngữ cảnh phân loại** | — | Chân dung (48% da) | **Người + Phong cảnh (Environmental Portrait)** | PRO phân loại chuyên sâu hơn |
| **Thời gian xử lý** | — | **248 ms** | 316 ms | BASE nhanh hơn 1.3x |
| **Độ dốc trung bình (`AvgGrad`)** | 6.60 | 8.33 | **9.64** | PRO sắc nét hơn |
| **Tần số vi mô (`AvgLap`)** | 11.12 | 14.18 | **17.08** | Chi tiết vi mô gân lá/sợi tóc |
| **Năng lượng biên Tenengrad** | 10.76 | 13.95 | **15.50** | Độ sắc cạnh biên |
| **Tỷ lệ tần số cao (`HighFreq%`)** | 44.51% | 51.73% | **63.11%** | Mật độ chi tiết đanh nét |
| **Dải động tương phản (`DynRange`)** | 189.00 | 194.00 | **200.00** | Khối sáng tối tương phản |
| **Nhiễu nền (`NoiseFloor MAD`)** | 2.54 | 2.86 | **4.00** | PRO nhạy noise hơn |
| **Tỷ lệ màu da Melanin** | 26.06% | 48.12% (Thô) | **23.74% (Melanin)** | Độ chính xác mặt nạ da |

### Tệp: `chu-in-bi-mo.png` (800x1067 - 0.85 MP)

| Tiêu chí so sánh | Bản Gốc (ORIG) | Bản BASE | Bản PRO (V2) | Nhận xét chi tiết |
| :--- | :---: | :---: | :---: | :--- |
| **Ngữ cảnh phân loại** | — | Phong cảnh / Chi tiết | **Tài liệu / Văn bản (Document / Text)** | PRO phân loại chuyên sâu hơn |
| **Thời gian xử lý** | — | **369 ms** | 434 ms | BASE nhanh hơn 1.2x |
| **Độ dốc trung bình (`AvgGrad`)** | 5.83 | 8.50 | **10.14** | PRO sắc nét hơn |
| **Tần số vi mô (`AvgLap`)** | 12.25 | 16.74 | **20.99** | Chi tiết vi mô gân lá/sợi tóc |
| **Năng lượng biên Tenengrad** | 12.14 | 17.17 | **19.16** | Độ sắc cạnh biên |
| **Tỷ lệ tần số cao (`HighFreq%`)** | 34.79% | 48.05% | **64.01%** | Mật độ chi tiết đanh nét |
| **Dải động tương phản (`DynRange`)** | 98.00 | 127.00 | **138.00** | Khối sáng tối tương phản |
| **Nhiễu nền (`NoiseFloor MAD`)** | 2.54 | 2.95 | **3.48** | PRO nhạy noise hơn |
| **Tỷ lệ màu da Melanin** | 0.00% | 0.09% (Thô) | **0.00% (Melanin)** | Độ chính xác mặt nạ da |

### Tệp: `may-in-bi-mo.jpg` (800x450 - 0.36 MP)

| Tiêu chí so sánh | Bản Gốc (ORIG) | Bản BASE | Bản PRO (V2) | Nhận xét chi tiết |
| :--- | :---: | :---: | :---: | :--- |
| **Ngữ cảnh phân loại** | — | Ảnh mờ / Nén thấp | **Tài liệu / Văn bản (Document / Text)** | PRO phân loại chuyên sâu hơn |
| **Thời gian xử lý** | — | **135 ms** | 129 ms | BASE nhanh hơn 1.0x |
| **Độ dốc trung bình (`AvgGrad`)** | 5.82 | 6.04 | **6.87** | PRO sắc nét hơn |
| **Tần số vi mô (`AvgLap`)** | 6.76 | 7.65 | **9.54** | Chi tiết vi mô gân lá/sợi tóc |
| **Năng lượng biên Tenengrad** | 12.54 | 13.71 | **14.43** | Độ sắc cạnh biên |
| **Tỷ lệ tần số cao (`HighFreq%`)** | 24.39% | 27.23% | **35.16%** | Mật độ chi tiết đanh nét |
| **Dải động tương phản (`DynRange`)** | 149.00 | 177.00 | **187.00** | Khối sáng tối tương phản |
| **Nhiễu nền (`NoiseFloor MAD`)** | 1.48 | 1.48 | **2.80** | PRO nhạy noise hơn |
| **Tỷ lệ màu da Melanin** | 0.00% | 0.38% (Thô) | **0.00% (Melanin)** | Độ chính xác mặt nạ da |

### Tệp: `phong_canh.jpg` (2048x1265 - 2.59 MP)

| Tiêu chí so sánh | Bản Gốc (ORIG) | Bản BASE | Bản PRO (V2) | Nhận xét chi tiết |
| :--- | :---: | :---: | :---: | :--- |
| **Ngữ cảnh phân loại** | — | Chân dung (10% da) | **Phong cảnh / Chi tiết cao (Landscape)** | PRO phân loại chuyên sâu hơn |
| **Thời gian xử lý** | — | **532 ms** | 688 ms | BASE nhanh hơn 1.3x |
| **Độ dốc trung bình (`AvgGrad`)** | 8.69 | 11.58 | **13.79** | PRO sắc nét hơn |
| **Tần số vi mô (`AvgLap`)** | 31.60 | 43.06 | **51.43** | Chi tiết vi mô gân lá/sợi tóc |
| **Năng lượng biên Tenengrad** | 12.91 | 16.77 | **19.27** | Độ sắc cạnh biên |
| **Tỷ lệ tần số cao (`HighFreq%`)** | 64.09% | 72.85% | **80.68%** | Mật độ chi tiết đanh nét |
| **Dải động tương phản (`DynRange`)** | 182.00 | 204.00 | **216.00** | Khối sáng tối tương phản |
| **Nhiễu nền (`NoiseFloor MAD`)** | 3.57 | 4.29 | **5.54** | PRO nhạy noise hơn |
| **Tỷ lệ màu da Melanin** | 1.21% | 9.97% (Thô) | **0.42% (Melanin)** | Độ chính xác mặt nạ da |

