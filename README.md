# CMD BOX - SYSTEM TOOLKIT

> **Nền tảng:** Windows (x64) | **Ngôn ngữ:** C++17 | **Biên dịch:** MinGW-w64 (GCC / UCRT64)

---

## I. Giới thiệu Tổng quan

**CMD BOX** là bộ công cụ dòng lệnh (CLI) hiệu năng cao dành cho quản trị, bảo trì, tối ưu hóa hệ điều hành, bảo mật mạng và xử lý đa phương tiện trên nền tảng Windows.

Dự án được xây dựng hoàn toàn bằng **C++ native**, can thiệp trực tiếp qua hệ thống Win32 API, Windows Imaging Component (WIC), SIMD (AVX2, FMA) và tối ưu hóa tính toán song song với OpenMP. Phần mềm không sử dụng các dịch vụ đám mây bên thứ ba, không thu thập dữ liệu người dùng và vận hành độc lập không cần cài đặt (portable).

### Đặc tính Kỹ thuật
- **Quản lý tiến trình an toàn (Windows Job Objects):** Tất cả tiến trình con do công cụ khởi tạo đều được kiểm soát trong Job Object của hệ điều hành. Khi đóng ứng dụng hoặc nhận tín hiệu ngắt (`Ctrl + C`), toàn bộ cây tiến trình con được giải phóng triệt để, ngăn ngừa rò rỉ bộ nhớ hoặc tiến trình chạy ngầm.
- **Khởi tạo trễ (Lazy Loading):** Áp dụng mô hình con trỏ thông minh `std::unique_ptr` kết hợp cơ chế kiểm tra đa luồng (Double-Checked Locking / Thread-safe). Ứng dụng chỉ cấp phát bộ nhớ khi người dùng truy cập phân hệ tương ứng, duy trì mức chiếm dụng RAM cực thấp ở trạng thái chờ.
- **Xử lý đồ họa & thuật toán song song:** Module xử lý hình ảnh được tối ưu hóa ở mức số học điểm động (float), hỗ trợ vector hóa phần cứng AVX2 + FMA và tăng tốc đa luồng CPU (OpenMP). Giải mã và mã hóa định dạng ảnh trực tiếp từ bộ giải mã hệ thống WIC.
- **Bảo toàn 100% Siêu dữ liệu (Metadata Preservation):** Sao chép nguyên vẹn khối siêu dữ liệu cấp thấp (EXIF, GPS tọa độ, Model máy ảnh, Ống kính, Giờ chụp) và đồng bộ ngày giờ tạo/sửa đổi tệp tin (`last_write_time`) trên hệ điều hành trùng khớp ảnh gốc.
- **Tương thích toàn diện:** Hoạt động ổn định trên Windows 10 và Windows 11 (64-bit).

---

## II. Cấu trúc Phân hệ Chức năng

Chương trình được phân tách thành 4 phân hệ chính theo từng lĩnh vực chuyên biệt:

### 1. Bảo trì & Tối ưu Hệ thống (`SystemOptimizer`)
- **Dọn rác Đa Tầng:**
  - *Tầng 1 (Dọn rác bề mặt):* Tự động dọn dẹp các vùng lưu trữ tạm thời `%TEMP%`, `Windows\Temp`, `Prefetch`, bộ nhớ đệm hình thu nhỏ (`Thumbcache`) và Thùng rác (Recycle Bin).
  - *Tầng 2 (Dọn rác Trình duyệt & Ứng dụng):* Quét và làm sạch dữ liệu đệm, cookies, lịch sử của hơn 8 trình duyệt phổ biến (Chrome, Edge, Cốc Cốc, Brave, Vivaldi, Opera, Opera GX, Firefox).
  - *Tầng 3 (Dọn dẹp Chuyên sâu Hệ thống):* Làm sạch log bảo trì (`CBS Logs`), `Delivery Optimization` và thư mục tải về của Windows Update.
  - *Tầng 4 (Dọn rác Môi trường lập trình):* Truy quét và dọn sạch bộ nhớ đệm lập trình: Python (`__pycache__`, `.pytest_cache`), Node.js/NPM, Yarn, Pip, Java/Gradle, Rust/Cargo, Go build cache và VS Code workspace storage.
  - *Dọn liên hoàn:* Tùy chọn dọn liên hoàn Tầng 1+2+3 hoặc toàn bộ cả 4 tầng chỉ với 1 click.
- **Tăng tốc & Tối ưu Đa Tầng:**
  - *Tầng 1 (Tối ưu Khởi động):* Phân tích các khóa Registry `Run` và `RunOnce` (`HKCU` & `HKLM`). Tích hợp Whitelist thông minh bảo vệ driver phần cứng (Realtek, Waves, NVIDIA, AMD, Intel) và phần mềm điều khiển OEM (ASUS, Dell, HP, Lenovo).
  - *Tầng 2 (Tối ưu Dịch vụ ngầm):* Vô hiệu hóa các dịch vụ ngầm không thiết yếu (Windows Telemetry, Maps Broker, Xbox Services, Error Reporting Service, DiagTrack).
  - *Tầng 3 (Tối ưu Giao diện & Độ nhạy Windows):* Tùy biến nhanh thanh tác vụ Taskbar Windows 11 (Search Box, Widget, Chat/Teams, Task View, Copilot) và giảm độ trễ phản hồi UI.
  - *Quản lý dịch vụ Windows nâng cao:* Bật/tắt trạng thái dịch vụ hệ thống linh hoạt qua Win32 Service Control API.
- **Sửa lỗi Windows Update:** Tạm dừng các dịch vụ điều phối cập nhật (`wuauserv`, `bits`, `cryptsvc`), giải phóng các gói dữ liệu cập nhật bị hỏng trong `SoftwareDistribution` và `catroot2`, tái kích hoạt dịch vụ về trạng thái chuẩn.

### 2. Mạng & An toàn Hệ thống (`Internet`)
- **Thông tin mạng chi tiết:** Truy xuất và hiển thị trạng thái card mạng vật lý/ảo, địa chỉ IPv4 nội bộ, Public IP WAN, Subnet Mask, Default Gateway và hệ thống DNS Server đang phân giải.
- **Khôi phục mạng toàn diện (Network Repair PRO):** Quy trình tự động 8 bước chuẩn hóa:
  1. Xóa sạch bộ nhớ đệm DNS (`ipconfig /flushdns`).
  2. Khôi phục danh mục kết nối mạng (`netsh winsock reset`).
  3. Thiết lập lại ngăn xếp giao thức mạng (`netsh int ip reset`).
  4. Xóa bảng ánh xạ địa chỉ MAC (`arp -d *`).
  5. Giải phóng và cấp phát lại cấu hình IP (`release` & `renew`).
  6. Khởi động lại dịch vụ WinNAT & HNS nhằm giải phóng dải cổng mạng bị Hyper-V / Docker / WSL2 chiếm dụng (khắc phục triệt để lỗi `Socket Error 10013`).
  7. Cấu hình tường lửa cho phép kết nối chia sẻ tệp nội bộ (HTTP LAN & cổng LocalSend 53317).
  8. Thiết lập cấu hình mạng về trạng thái Private Network.
- **Lá chắn bảo mật toàn diện (Full Security Shield):** Kích hoạt Windows Defender, cập nhật mẫu mã độc, bật tường lửa toàn diện, kích hoạt bảo vệ chống Ransomware (Controlled Folder Access), đóng các cổng dịch vụ mạng nguy hiểm (445, 139, 135, 137, 138) và cấu hình Cloudflare DoH `1.1.1.1`.
- **Kiểm tra trạng thái bảo mật:** Rà soát đánh giá trạng thái Defender, Firewall, dịch vụ RDP và tính hợp lệ của DNS.
- **Trích xuất mật khẩu Wi-Fi:** Liệt kê toàn bộ hồ sơ Wi-Fi đã lưu trên máy, hiển thị tên mạng (SSID), chuẩn bảo mật và mật khẩu rõ ràng.

### 3. Công cụ Tự động & Tiện ích (`UtilityTools`)
- **Tự động nhấp chuột (Auto Click):** Mô phỏng thao tác nhấp chuột theo tọa độ cố định hoặc vị trí con trỏ hiện tại với tần suất mili-giây tùy chỉnh, phím ngắt khẩn cấp (`ESC` / `F6`).
- **Gửi văn bản tự động (Spam Text):** Phát chuỗi ký tự liên tục với định dạng Unicode tiếng Việt (`CF_UNICODETEXT`), tối ưu độ trễ giữa các lần gửi.
- **Tự động dán dữ liệu nhiều dòng (Auto Paste):** Đọc tuần tự từng dòng văn bản từ bộ đệm và tự động điền vào các trường nhập liệu tương ứng.
- **Cài đặt phần mềm tự động:** Đọc danh mục phần mềm từ `apps.txt`, tự động tải về và cài đặt các ứng dụng thông dụng (Chrome, Brave, Cốc Cốc, Zalo, Telegram, Discord, VS Code, Git, 7-Zip, EVKey, OpenKey...) theo nhóm hoặc tải lẻ.
- **Gỡ bỏ ứng dụng rác (Bloatware Removal):** Quét và gỡ bỏ tận gốc các gói ứng dụng UWP dư thừa được cài sẵn trên Windows.
- **Kiểm tra Pin Laptop chuyên sâu (Battery Diagnostic):** Đọc trực tiếp từ ACPI và báo cáo pin Windows (`powercfg`), trích xuất công suất thiết kế, công suất sạc đầy hiện tại, chu kỳ sạc, tỷ lệ hao mòn chai pin thực tế và tình trạng sạc.

### 4. Xử lý Đa phương tiện & Làm nét Ảnh (`MediaProcessor`, `ImageEnhancer` & `ImageEnhancerPro`)

#### A. Thuật toán Làm nét Ảnh (2 Phiên bản):
- **Bản Base (`ImageEnhancer`):**
  - Hỗ trợ đầy đủ các định dạng: JPG, PNG, BMP, TIFF, WebP, HEIC, DNG (RAW).
  - Phóng đại siêu mẫu Lanczos-3 bảo toàn dải tần số cao.
  - Bộ lọc dẫn đường 2 tầng (2-Scale Guided Filter) tách bạch chi tiết vi mô ($r=1$) và cấu trúc ($r=3$).
  - Khử bệt màu liên tục Cauchy (Cauchy Continuous Coring) chống dính hạt pixel.
  - Bù màu Constant-Saturation Chroma Tracking giữ sắc tươi xanh tự nhiên cho cỏ cây hoa lá.
  - Bảo vệ vùng da chân dung chuẩn ITU-R BT.601 (làm mịn da phẳng, sắc nét ngũ quan).
- **Bản Pro v2 (`ImageEnhancerPro`) - Nâng cấp Vượt trội:**
  - **7 Góc độ phân tích chuyên sâu:** Tự động đo đạc năng lượng biên đa hướng Tenengrad, tỷ lệ tần số cao, độ nhòe blur, mức nhiễu nền MAD, vỡ khối JPEG blockiness 8x8, dải tương phản động và độ phức tạp kết cấu/nét mảnh.
  - **Phân loại ngữ cảnh tự động:** Nhận diện thông minh Chân dung (Portrait), Phong cảnh (Landscape), Tài liệu/Văn bản (Document), Ban đêm/Thiếu sáng (Low-light) hoặc Ảnh mờ nặng (Heavy Blur).
  - **Khử sương mù & Chói sáng:** Tự động nâng sáng vùng tối bị dìm (`shadowLift`) và thu hồi chi tiết vùng chói gắt (`highlightPull`).
  - **Nổi chủ thể & Nét đúng đối tượng:** Tăng cường chi tiết thân cây, cành lá, vách đá, đường sá mà không bị bệt viền, không xuất hiện quầng sáng quầng tối giả tạo (Anti-Halo).
  - **Ức chế bên (Lateral Inhibition):** Chống phình nét mảnh, giữ các đường kẻ/chữ viết thanh mảnh sắc lẹm.
  - **Bảo toàn 100% WIC Metadata:** Giữ nguyên vẹn toàn bộ EXIF, GPS, camera model, lens info và đồng bộ ngày giờ tạo tệp tin gốc.

#### B. Xử lý Video & Âm thanh (Tăng tốc phần cứng FFmpeg):
- Tự động nhận diện và tận dụng bộ mã hóa phần cứng GPU (NVIDIA NVENC, Intel QuickSync, AMD AMF).
- Nén tối ưu dung lượng Video MP4 và Ảnh (PNG/JPG) bảo toàn độ nét và Metadata.
- Trích xuất âm thanh từ Video sang định dạng MP3 hàng loạt.
- Thay đổi tốc độ Video (0.5x đến 2.0x) kèm bộ lọc âm thanh thích ứng chống méo cao độ.
- Đổi định dạng tệp đa phương tiện linh hoạt.
- Chuẩn hóa tên tập tin theo quy chuẩn đồng nhất trong thư mục.
- Ẩn file vào file (Steganography).

---

## III. Cấu trúc Mã nguồn

```text
CMD_BOX/
├── bin/                     # Thư mục chứa tệp nhị phân sau biên dịch
│   ├── apps.txt             # Cấu hình danh mục tải phần mềm tự động
│   ├── ffmpeg.exe           # Bộ công cụ xử lý media hỗ trợ tăng tốc GPU
│   └── main.exe             # Tệp thực thi chính của chương trình
├── include/                 # Danh mục tệp tiêu đề (Header files)
│   ├── ImageEnhancer.h      # Khai báo thuật toán làm nét ảnh Base
│   ├── ImageEnhancerPro.h   # Khai báo thuật toán làm nét ảnh Pro v2 & phân tích 7 góc độ
│   ├── Internet.h           # Khai báo module mạng, bảo mật & tường lửa
│   ├── MediaProcessor.h     # Khai báo bộ xử lý video, âm thanh & Steganography
│   ├── SystemCore.h         # Khung điều khiển Win32 API, Job Object & Console I/O
│   ├── SystemOptimizer.h    # Khai báo module bảo trì, dọn dẹp & chỉnh sửa Registry
│   └── UtilityTools.h       # Khai báo tiện ích tự động, clicker & chẩn đoán pin
├── src/                     # Danh mục mã nguồn (Source files)
│   ├── apps.txt             # Tệp nguồn cấu hình danh mục phần mềm
│   ├── ImageEnhancer.cpp    # Cài đặt Lanczos-3, 2-Scale Guided Filter & WIC
│   ├── ImageEnhancerPro.cpp # Cài đặt bộ làm nét PRO v2, 7 góc độ phân tích & WIC Metadata
│   ├── Internet.cpp         # Cài đặt xử lý socket, tường lửa & kiểm tra bảo mật
│   ├── main.cpp             # Điểm khởi chạy (Entry point) & hệ thống menu
│   ├── MediaProcessor.cpp   # Cài đặt giao tiếp FFmpeg & GPU acceleration
│   ├── SystemCore.cpp       # Cài đặt quản lý tiến trình con & tương tác hệ thống
│   ├── SystemOptimizer.cpp  # Cài đặt dọn dẹp rác đa tầng & tối ưu hệ thống
│   └── UtilityTools.cpp     # Cài đặt tự động hóa chuột/bàn phím & đọc ACPI pin
├── build.bat                # Kịch bản biên dịch 1 file duy nhất với loading thời gian thực
├── README.md                # Tài liệu hướng dẫn kỹ thuật của dự án
└── README_IMAGE_ENHANCER.md # Tài liệu đặc tả toán học & kiến trúc làm nét ảnh
```

---

## IV. Hướng dẫn Biên dịch & Vận hành

### 1. Yêu cầu Hệ thống
- **Hệ điều hành:** Windows 10 hoặc Windows 11 (phiên bản 64-bit).
- **Bộ biên dịch C++:** GCC/G++ từ bộ công cụ MinGW-w64 (MSYS2 UCRT64 hoặc MINGW64) hỗ trợ đầy đủ tiêu chuẩn **C++17**, OpenMP (`-fopenmp`) và tập lệnh vector **AVX2 + FMA**.

### 2. Biên dịch Tự động
Chỉ cần nhấp đúp chuột vào tệp script:
```cmd
build.bat
```
Script sẽ tự động dò tìm trình biên dịch `g++`, kích hoạt chế độ tối ưu hóa phần cứng cao nhất (`-O3 -fopenmp -mavx2 -mfma`), hiển thị hiệu ứng loading đếm giây thời gian thực và hỏi bạn có muốn mở `bin\main.exe` ngay sau khi hoàn tất hay không.

### 3. Biên dịch Thủ công qua Dòng lệnh
Nếu muốn tự biên dịch thủ công qua Command Prompt hoặc PowerShell:
```cmd
g++ -std=c++17 -O3 -fopenmp -mavx2 -mfma -Iinclude src\*.cpp -o bin\main.exe -lws2_32 -liphlpapi -lole32 -lwindowscodecs -loleaut32 -luuid -static-libgcc -static-libstdc++ -static -s
copy /y "src\apps.txt" "bin\apps.txt"
```

*Giải thích các cờ liên kết (Linker Flags):*
- `-mavx2 -mfma`: Tận dụng tập lệnh phần cứng AVX2 và FMA giúp tăng tốc độ xử lý điểm ảnh gấp 2-4 lần.
- `-fopenmp`: Kích hoạt xử lý đa luồng CPU song song cho tất cả các thuật toán đồ họa.
- `-lws2_32 -liphlpapi`: Giao diện mạng Windows Sockets và IP Helper API.
- `-lole32 -lwindowscodecs -loleaut32 -luuid`: Hỗ trợ giao tiếp COM và Windows Imaging Component (WIC) để giải mã ảnh và sao chép metadata.
- `-static-libgcc -static-libstdc++ -static -s`: Đóng gói tĩnh toàn bộ thư viện C++ runtime, giúp file `.exe` chạy độc lập mà không yêu cầu cài thêm bất kỳ DLL nào bên ngoài.

### 4. Tích hợp FFmpeg (Tùy chọn cho Phân hệ Media)
Để sử dụng các tính năng nén video, trích xuất âm thanh và thay đổi tốc độ:
1. Tải bản build Portable của FFmpeg từ trang chính thức: [gyan.dev/ffmpeg/builds](https://www.gyan.dev/ffmpeg/builds/).
2. Đặt tệp thực thi `ffmpeg.exe` vào thư mục `bin\` (cùng cấp với `main.exe`).

### 5. Khởi chạy Ứng dụng
- Để tất cả các tính năng can thiệp Registry, Win32 Service Manager, Tường lửa hệ thống và xóa tập tin hệ thống hoạt động chính xác, hãy nhấp chuột phải vào `bin\main.exe` và chọn **Run as Administrator** (hoặc mở Command Prompt với quyền Administrator).