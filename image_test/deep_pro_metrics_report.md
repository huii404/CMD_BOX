# BÁO CÁO TOÀN DIỆN CÁC CHỈ SỐ ĐÁNH GIÁ CHUYÊN SÂU CỦA PRO TRÊN 3 PHIÊN BẢN (ORIG vs BASE vs PRO)

> Sử dụng trực tiếp bộ phân tích đo lường `ImageEnhancerPro::analyzeImageBufferPro()` trên từng điểm ảnh thực tế.

### Tệp: `anh_buoi_toi.jpg` (640x480)

| Logic đánh giá chuyên sâu của PRO | Bản Gốc (ORIG) | Bản BASE (Đã render) | Bản PRO V2 (Đã render) | Ý nghĩa & Đánh giá chuyên sâu |
| :--- | :---: | :---: | :---: | :--- |
| **1. Ngữ cảnh phân loại (`detectedType`)** | Phong cảnh / Chi tiết cao (Landscape) | Phong cảnh / Chi tiết cao (Landscape) | **Phong cảnh / Chi tiết cao (Landscape)** | Ngữ cảnh nhận diện của bộ não PRO |
| **2. Xếp hạng chất lượng (`qualityGrade`)** | Tuyệt vời (Studio Grade) | Tuyệt vời (Studio Grade) | **Tuyệt vời (Studio Grade)** | Cấp độ chất lượng tổng thể |
| **3. Độ sắc nét biên đa hướng (`edgeSharpness`)** | 77.89 | 91.98 | **100.00** | Tenengrad 4 góc (0-100) |
| **4. Năng lượng vi mô tần số cao (`highFreqEnergy`)** | 100.00 | 100.00 | **100.00** | Tần số chi tiết siêu nhỏ |
| **5. Mức độ mờ / Nhòe (`blurDegree`)** | 0.00 | 0.00 | **0.00** | Càng thấp càng nét (0: cực nét) |
| **6. Tỷ lệ tín hiệu trên nhiễu (`snrDb`)** | 41.83 dB | 47.00 dB | **30.20 dB** | Độ trong trẻo của tín hiệu ảnh |
| **7. Mức nhiễu nền (`noiseFloor MAD`)** | 0.34 | 0.00 | **1.48** | Phương sai MAD trên vùng phẳng |
| **8. Độ phức tạp vân ảnh (`textureComplexity`)** | 71.21 | 64.87 | **60.56** | Độ giàu kết cấu (vải, gân lá, da) |
| **9. Tỷ lệ nét mảnh (`thinFeatureRatio`)** | 0.60 | 0.60 | **0.63** | Tỷ lệ chi tiết < 3 pixel |
| **10. Tỷ lệ da người Melanin (`skinPercent`)** | 5.81% | 5.47% | **4.17%** | Lọc chuẩn Melanin theo YCbCr |
| **11. Dải tương phản động (`dynamicRange`)** | 167.00 | 179.00 | **192.00** | Dải sắc độ 1% - 99% |
| **12. Vỡ hạt nén JPEG (`compressionBlockiness`)** | 100.00 | 100.00 | **100.00** | Mức độ khối artifact 8x8 |
| **13. Độ bão hòa màu (`colorSaturation`)** | 17.27 | 17.14 | **17.15** | Độ tươi màu trung bình |
| **14. Dung lượng tệp ảnh thực tế trên đĩa** | 40 KB | 182 KB | 309 KB | File nhị phân thật trên đĩa |

### Tệp: `buoi_toi_canh_quan.jpg` (780x975)

| Logic đánh giá chuyên sâu của PRO | Bản Gốc (ORIG) | Bản BASE (Đã render) | Bản PRO V2 (Đã render) | Ý nghĩa & Đánh giá chuyên sâu |
| :--- | :---: | :---: | :---: | :--- |
| **1. Ngữ cảnh phân loại (`detectedType`)** | Phong cảnh / Chi tiết cao (Landscape) | Phong cảnh / Chi tiết cao (Landscape) | **Phong cảnh / Chi tiết cao (Landscape)** | Ngữ cảnh nhận diện của bộ não PRO |
| **2. Xếp hạng chất lượng (`qualityGrade`)** | Tuyệt vời (Studio Grade) | Tuyệt vời (Studio Grade) | **Tuyệt vời (Studio Grade)** | Cấp độ chất lượng tổng thể |
| **3. Độ sắc nét biên đa hướng (`edgeSharpness`)** | 58.84 | 71.96 | **86.04** | Tenengrad 4 góc (0-100) |
| **4. Năng lượng vi mô tần số cao (`highFreqEnergy`)** | 88.64 | 100.00 | **100.00** | Tần số chi tiết siêu nhỏ |
| **5. Mức độ mờ / Nhòe (`blurDegree`)** | 0.00 | 0.00 | **0.00** | Càng thấp càng nét (0: cực nét) |
| **6. Tỷ lệ tín hiệu trên nhiễu (`snrDb`)** | 29.83 dB | 29.88 dB | **28.25 dB** | Độ trong trẻo của tín hiệu ảnh |
| **7. Mức nhiễu nền (`noiseFloor MAD`)** | 1.48 | 1.48 | **1.84** | Phương sai MAD trên vùng phẳng |
| **8. Độ phức tạp vân ảnh (`textureComplexity`)** | 73.70 | 73.31 | **81.62** | Độ giàu kết cấu (vải, gân lá, da) |
| **9. Tỷ lệ nét mảnh (`thinFeatureRatio`)** | 0.52 | 0.56 | **0.60** | Tỷ lệ chi tiết < 3 pixel |
| **10. Tỷ lệ da người Melanin (`skinPercent`)** | 0.00% | 0.00% | **0.00%** | Lọc chuẩn Melanin theo YCbCr |
| **11. Dải tương phản động (`dynamicRange`)** | 184.00 | 185.00 | **190.00** | Dải sắc độ 1% - 99% |
| **12. Vỡ hạt nén JPEG (`compressionBlockiness`)** | 100.00 | 100.00 | **100.00** | Mức độ khối artifact 8x8 |
| **13. Độ bão hòa màu (`colorSaturation`)** | 13.78 | 14.21 | **14.36** | Độ tươi màu trung bình |
| **14. Dung lượng tệp ảnh thực tế trên đĩa** | 123 KB | 339 KB | 572 KB | File nhị phân thật trên đĩa |

### Tệp: `chan_dung1.jpg` (1500x1000)

| Logic đánh giá chuyên sâu của PRO | Bản Gốc (ORIG) | Bản BASE (Đã render) | Bản PRO V2 (Đã render) | Ý nghĩa & Đánh giá chuyên sâu |
| :--- | :---: | :---: | :---: | :--- |
| **1. Ngữ cảnh phân loại (`detectedType`)** | Ảnh mờ / Cần phục hồi nét (Blur/Defocus) | Ảnh mờ / Cần phục hồi nét (Blur/Defocus) | **Ảnh mờ / Cần phục hồi nét (Blur/Defocus)** | Ngữ cảnh nhận diện của bộ não PRO |
| **2. Xếp hạng chất lượng (`qualityGrade`)** | Mờ nặng / Suy giảm (Heavy Blur / Degraded) | Mờ nặng / Suy giảm (Heavy Blur / Degraded) | **Mờ nặng / Suy giảm (Heavy Blur / Degraded)** | Cấp độ chất lượng tổng thể |
| **3. Độ sắc nét biên đa hướng (`edgeSharpness`)** | 34.99 | 28.03 | **30.23** | Tenengrad 4 góc (0-100) |
| **4. Năng lượng vi mô tần số cao (`highFreqEnergy`)** | 31.51 | 40.78 | **60.59** | Tần số chi tiết siêu nhỏ |
| **5. Mức độ mờ / Nhòe (`blurDegree`)** | 70.05 | 74.41 | **65.81** | Càng thấp càng nét (0: cực nét) |
| **6. Tỷ lệ tín hiệu trên nhiễu (`snrDb`)** | 47.92 dB | 36.79 dB | **30.94 dB** | Độ trong trẻo của tín hiệu ảnh |
| **7. Mức nhiễu nền (`noiseFloor MAD`)** | 0.00 | 0.74 | **1.48** | Phương sai MAD trên vùng phẳng |
| **8. Độ phức tạp vân ảnh (`textureComplexity`)** | 21.10 | 19.86 | **27.80** | Độ giàu kết cấu (vải, gân lá, da) |
| **9. Tỷ lệ nét mảnh (`thinFeatureRatio`)** | 0.25 | 0.25 | **0.29** | Tỷ lệ chi tiết < 3 pixel |
| **10. Tỷ lệ da người Melanin (`skinPercent`)** | 5.61% | 5.83% | **5.34%** | Lọc chuẩn Melanin theo YCbCr |
| **11. Dải tương phản động (`dynamicRange`)** | 199.00 | 205.00 | **209.00** | Dải sắc độ 1% - 99% |
| **12. Vỡ hạt nén JPEG (`compressionBlockiness`)** | 100.00 | 39.00 | **50.93** | Mức độ khối artifact 8x8 |
| **13. Độ bão hòa màu (`colorSaturation`)** | 3.98 | 4.23 | **4.17** | Độ tươi màu trung bình |
| **14. Dung lượng tệp ảnh thực tế trên đĩa** | 59 KB | 246 KB | 556 KB | File nhị phân thật trên đĩa |

### Tệp: `chan_dung2.jpg` (736x1308)

| Logic đánh giá chuyên sâu của PRO | Bản Gốc (ORIG) | Bản BASE (Đã render) | Bản PRO V2 (Đã render) | Ý nghĩa & Đánh giá chuyên sâu |
| :--- | :---: | :---: | :---: | :--- |
| **1. Ngữ cảnh phân loại (`detectedType`)** | Người + Phong cảnh (Environmental Portrait) | Người + Phong cảnh (Environmental Portrait) | **Người + Phong cảnh (Environmental Portrait)** | Ngữ cảnh nhận diện của bộ não PRO |
| **2. Xếp hạng chất lượng (`qualityGrade`)** | Tuyệt vời (Studio Grade) | Tuyệt vời (Studio Grade) | **Sắc nét tốt (Good Clarity)** | Cấp độ chất lượng tổng thể |
| **3. Độ sắc nét biên đa hướng (`edgeSharpness`)** | 53.78 | 69.73 | **77.48** | Tenengrad 4 góc (0-100) |
| **4. Năng lượng vi mô tần số cao (`highFreqEnergy`)** | 100.00 | 100.00 | **100.00** | Tần số chi tiết siêu nhỏ |
| **5. Mức độ mờ / Nhòe (`blurDegree`)** | 0.00 | 0.00 | **0.00** | Càng thấp càng nét (0: cực nét) |
| **6. Tỷ lệ tín hiệu trên nhiễu (`snrDb`)** | 27.72 dB | 28.14 dB | **24.54 dB** | Độ trong trẻo của tín hiệu ảnh |
| **7. Mức nhiễu nền (`noiseFloor MAD`)** | 1.94 | 1.90 | **2.97** | Phương sai MAD trên vùng phẳng |
| **8. Độ phức tạp vân ảnh (`textureComplexity`)** | 100.00 | 100.00 | **100.00** | Độ giàu kết cấu (vải, gân lá, da) |
| **9. Tỷ lệ nét mảnh (`thinFeatureRatio`)** | 0.49 | 0.55 | **0.61** | Tỷ lệ chi tiết < 3 pixel |
| **10. Tỷ lệ da người Melanin (`skinPercent`)** | 23.74% | 21.94% | **19.23%** | Lọc chuẩn Melanin theo YCbCr |
| **11. Dải tương phản động (`dynamicRange`)** | 189.00 | 194.00 | **200.00** | Dải sắc độ 1% - 99% |
| **12. Vỡ hạt nén JPEG (`compressionBlockiness`)** | 100.00 | 100.00 | **100.00** | Mức độ khối artifact 8x8 |
| **13. Độ bão hòa màu (`colorSaturation`)** | 25.99 | 26.66 | **26.54** | Độ tươi màu trung bình |
| **14. Dung lượng tệp ảnh thực tế trên đĩa** | 157 KB | 503 KB | 801 KB | File nhị phân thật trên đĩa |

### Tệp: `chu-in-bi-mo.png` (800x1067)

| Logic đánh giá chuyên sâu của PRO | Bản Gốc (ORIG) | Bản BASE (Đã render) | Bản PRO V2 (Đã render) | Ý nghĩa & Đánh giá chuyên sâu |
| :--- | :---: | :---: | :---: | :--- |
| **1. Ngữ cảnh phân loại (`detectedType`)** | Tài liệu / Văn bản (Document / Text) | Tài liệu / Văn bản (Document / Text) | **Tài liệu / Văn bản (Document / Text)** | Ngữ cảnh nhận diện của bộ não PRO |
| **2. Xếp hạng chất lượng (`qualityGrade`)** | Sắc nét tốt (Good Clarity) | Tuyệt vời (Studio Grade) | **Sắc nét tốt (Good Clarity)** | Cấp độ chất lượng tổng thể |
| **3. Độ sắc nét biên đa hướng (`edgeSharpness`)** | 60.72 | 85.85 | **95.81** | Tenengrad 4 góc (0-100) |
| **4. Năng lượng vi mô tần số cao (`highFreqEnergy`)** | 100.00 | 100.00 | **100.00** | Tần số chi tiết siêu nhỏ |
| **5. Mức độ mờ / Nhòe (`blurDegree`)** | 1.87 | 0.00 | **0.00** | Càng thấp càng nét (0: cực nét) |
| **6. Tỷ lệ tín hiệu trên nhiễu (`snrDb`)** | 20.35 dB | 22.48 dB | **21.27 dB** | Độ trong trẻo của tín hiệu ảnh |
| **7. Mức nhiễu nền (`noiseFloor MAD`)** | 2.35 | 2.39 | **2.98** | Phương sai MAD trên vùng phẳng |
| **8. Độ phức tạp vân ảnh (`textureComplexity`)** | 42.90 | 81.14 | **100.00** | Độ giàu kết cấu (vải, gân lá, da) |
| **9. Tỷ lệ nét mảnh (`thinFeatureRatio`)** | 0.48 | 0.52 | **0.60** | Tỷ lệ chi tiết < 3 pixel |
| **10. Tỷ lệ da người Melanin (`skinPercent`)** | 0.00% | 0.00% | **0.00%** | Lọc chuẩn Melanin theo YCbCr |
| **11. Dải tương phản động (`dynamicRange`)** | 98.00 | 127.00 | **138.00** | Dải sắc độ 1% - 99% |
| **12. Vỡ hạt nén JPEG (`compressionBlockiness`)** | 100.00 | 100.00 | **100.00** | Mức độ khối artifact 8x8 |
| **13. Độ bão hòa màu (`colorSaturation`)** | 2.15 | 2.28 | **2.30** | Độ tươi màu trung bình |
| **14. Dung lượng tệp ảnh thực tế trên đĩa** | 1011 KB | 2337 KB | 2727 KB | File nhị phân thật trên đĩa |

### Tệp: `may-in-bi-mo.jpg` (800x450)

| Logic đánh giá chuyên sâu của PRO | Bản Gốc (ORIG) | Bản BASE (Đã render) | Bản PRO V2 (Đã render) | Ý nghĩa & Đánh giá chuyên sâu |
| :--- | :---: | :---: | :---: | :--- |
| **1. Ngữ cảnh phân loại (`detectedType`)** | Tài liệu / Văn bản (Document / Text) | Tài liệu / Văn bản (Document / Text) | **Tài liệu / Văn bản (Document / Text)** | Ngữ cảnh nhận diện của bộ não PRO |
| **2. Xếp hạng chất lượng (`qualityGrade`)** | Tuyệt vời (Studio Grade) | Tuyệt vời (Studio Grade) | **Tuyệt vời (Studio Grade)** | Cấp độ chất lượng tổng thể |
| **3. Độ sắc nét biên đa hướng (`edgeSharpness`)** | 62.68 | 68.53 | **72.17** | Tenengrad 4 góc (0-100) |
| **4. Năng lượng vi mô tần số cao (`highFreqEnergy`)** | 67.56 | 76.50 | **95.42** | Tần số chi tiết siêu nhỏ |
| **5. Mức độ mờ / Nhòe (`blurDegree`)** | 0.00 | 0.00 | **0.00** | Càng thấp càng nét (0: cực nét) |
| **6. Tỷ lệ tín hiệu trên nhiễu (`snrDb`)** | 28.00 dB | 29.50 dB | **26.20 dB** | Độ trong trẻo của tín hiệu ảnh |
| **7. Mức nhiễu nền (`noiseFloor MAD`)** | 1.48 | 1.48 | **2.29** | Phương sai MAD trên vùng phẳng |
| **8. Độ phức tạp vân ảnh (`textureComplexity`)** | 46.10 | 51.81 | **63.30** | Độ giàu kết cấu (vải, gân lá, da) |
| **9. Tỷ lệ nét mảnh (`thinFeatureRatio`)** | 0.44 | 0.45 | **0.50** | Tỷ lệ chi tiết < 3 pixel |
| **10. Tỷ lệ da người Melanin (`skinPercent`)** | 0.00% | 0.00% | **0.00%** | Lọc chuẩn Melanin theo YCbCr |
| **11. Dải tương phản động (`dynamicRange`)** | 149.00 | 177.00 | **187.00** | Dải sắc độ 1% - 99% |
| **12. Vỡ hạt nén JPEG (`compressionBlockiness`)** | 100.00 | 100.00 | **100.00** | Mức độ khối artifact 8x8 |
| **13. Độ bão hòa màu (`colorSaturation`)** | 2.03 | 2.13 | **2.14** | Độ tươi màu trung bình |
| **14. Dung lượng tệp ảnh thực tế trên đĩa** | 42 KB | 147 KB | 258 KB | File nhị phân thật trên đĩa |

### Tệp: `phong_canh.jpg` (2048x1265)

| Logic đánh giá chuyên sâu của PRO | Bản Gốc (ORIG) | Bản BASE (Đã render) | Bản PRO V2 (Đã render) | Ý nghĩa & Đánh giá chuyên sâu |
| :--- | :---: | :---: | :---: | :--- |
| **1. Ngữ cảnh phân loại (`detectedType`)** | Phong cảnh / Chi tiết cao (Landscape) | Phong cảnh / Chi tiết cao (Landscape) | **Phong cảnh / Chi tiết cao (Landscape)** | Ngữ cảnh nhận diện của bộ não PRO |
| **2. Xếp hạng chất lượng (`qualityGrade`)** | Sắc nét tốt (Good Clarity) | Sắc nét tốt (Good Clarity) | **Sắc nét tốt (Good Clarity)** | Cấp độ chất lượng tổng thể |
| **3. Độ sắc nét biên đa hướng (`edgeSharpness`)** | 64.57 | 83.87 | **96.37** | Tenengrad 4 góc (0-100) |
| **4. Năng lượng vi mô tần số cao (`highFreqEnergy`)** | 100.00 | 100.00 | **100.00** | Tần số chi tiết siêu nhỏ |
| **5. Mức độ mờ / Nhòe (`blurDegree`)** | 0.00 | 0.00 | **0.00** | Càng thấp càng nét (0: cực nét) |
| **6. Tỷ lệ tín hiệu trên nhiễu (`snrDb`)** | 24.19 dB | 24.17 dB | **23.40 dB** | Độ trong trẻo của tín hiệu ảnh |
| **7. Mức nhiễu nền (`noiseFloor MAD`)** | 2.81 | 3.15 | **3.65** | Phương sai MAD trên vùng phẳng |
| **8. Độ phức tạp vân ảnh (`textureComplexity`)** | 100.00 | 100.00 | **100.00** | Độ giàu kết cấu (vải, gân lá, da) |
| **9. Tỷ lệ nét mảnh (`thinFeatureRatio`)** | 0.56 | 0.60 | **0.63** | Tỷ lệ chi tiết < 3 pixel |
| **10. Tỷ lệ da người Melanin (`skinPercent`)** | 0.42% | 0.20% | **0.14%** | Lọc chuẩn Melanin theo YCbCr |
| **11. Dải tương phản động (`dynamicRange`)** | 182.00 | 204.00 | **216.00** | Dải sắc độ 1% - 99% |
| **12. Vỡ hạt nén JPEG (`compressionBlockiness`)** | 100.00 | 100.00 | **100.00** | Mức độ khối artifact 8x8 |
| **13. Độ bão hòa màu (`colorSaturation`)** | 26.69 | 25.89 | **25.23** | Độ tươi màu trung bình |
| **14. Dung lượng tệp ảnh thực tế trên đĩa** | 599 KB | 1649 KB | 2499 KB | File nhị phân thật trên đĩa |

