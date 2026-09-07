# CMD BOX - SYSTEM TOOLKIT

> **Nền tảng:** Windows (x64) | **Ngôn ngữ:** C++17 | **Biên dịch:** MinGW-w64 (GCC / UCRT64)

---

## I. Giới thiệu Tổng quan

**CMD BOX** là bộ công cụ dòng lệnh (CLI) hiệu năng cao dành cho quản trị, bảo trì, tối ưu hóa hệ điều hành, bảo mật mạng và xử lý đa phương tiện trên nền tảng Windows. 

Dự án được xây dựng hoàn toàn bằng **C++ native**, can thiệp trực tiếp qua hệ thống Win32 API, Windows Imaging Component (WIC) và tối ưu hóa tính toán song song với OpenMP. Phần mềm không sử dụng các dịch vụ đám mây bên thứ ba, không thu thập dữ liệu người dùng và vận hành độc lập không cần cài đặt (portable).

### Đặc tính Kỹ thuật
- **Quản lý tiến trình an toàn (Windows Job Objects):** Tất cả tiến trình con do công cụ khởi tạo đều được kiểm soát trong Job Object của hệ điều hành. Khi đóng ứng dụng hoặc nhận tín hiệu ngắt (`Ctrl + C`), toàn bộ cây tiến trình con được giải phóng triệt để, ngăn ngừa rò rỉ bộ nhớ hoặc tiến trình chạy ngầm.
- **Khởi tạo trễ (Lazy Loading):** Áp dụng mô hình con trỏ thông minh `std::unique_ptr` kết hợp cơ chế kiểm tra đa luồng (Double-Checked Locking / Thread-safe). Ứng dụng chỉ cấp phát bộ nhớ khi người dùng truy cập phân hệ tương ứng, duy trì mức chiếm dụng RAM dưới 5 MB ở trạng thái chờ.
- **Xử lý đồ họa & thuật toán song song:** Module xử lý hình ảnh được tối ưu hóa ở mức số học điểm động (float), hỗ trợ tăng tốc đa luồng CPU (OpenMP) và giải mã định dạng ảnh trực tiếp từ bộ giải mã hệ thống WIC.
- **Tương thích toàn diện:** Hoạt động ổn định trên Windows 10 và Windows 11 (64-bit).

---

## II. Cấu trúc Phân hệ Chức năng

Chương trình được phân tách thành 4 phân hệ chính theo từng lĩnh vực chuyên biệt:

### 1. Bảo trì & Tối ưu Hệ thống (`SystemOptimizer`)
- **Dọn rác nhanh (Quick Clean):** Tự động dọn dẹp các vùng lưu trữ tạm thời của hệ thống bao gồm `%TEMP%`, `Windows\Temp`, `Prefetch`, bộ nhớ đệm hình thu nhỏ (`Thumbcache`) và làm rỗng Thùng rác (Recycle Bin).
- **Dọn rác chuyên sâu (Deep Clean PRO):** Quét đa luồng và làm sạch dữ liệu đệm, cookies, lịch sử của hơn 8 trình duyệt phổ biến (Google Chrome, Microsoft Edge, Cốc Cốc, Brave, Vivaldi, Opera, Opera GX, Mozilla Firefox); đồng thời dọn sạch log bảo trì hệ thống (`CBS Logs`), `Delivery Optimization` và thư mục tải về của Windows Update.
- **Quản lý ứng dụng khởi động:** Phân tích các khóa Registry `Run` và `RunOnce` trên cả `HKCU` và `HKLM`. Tích hợp danh sách trắng (Whitelist) thông minh để bảo vệ các driver phần cứng quan trọng (âm thanh Realtek, Waves; card đồ họa NVIDIA, AMD, Intel) cùng phần mềm điều khiển OEM (ASUS, Dell, HP, Lenovo).
- **Quản lý Dịch vụ Windows (Win32 Service Control API):** Tinh chỉnh trạng thái khởi động hoặc vô hiệu hóa các dịch vụ ngầm không thiết yếu nhằm giải phóng tài nguyên (Windows Telemetry, Maps Broker, Xbox Services, Error Reporting Service, DiagTrack).
- **Tinh chỉnh giao diện Taskbar Windows 11:** Tùy biến nhanh thanh tác vụ: ẩn Hộp tìm kiếm (Search Box), Widget thời tiết/tin tức, Chat/Teams, Task View, Copilot và hỗ trợ tự động khởi động lại tiến trình `explorer.exe` để áp dụng ngay lập tức.
- **Sửa lỗi Windows Update:** Tạm dừng các dịch vụ điều phối cập nhật (`wuauserv`, `bits`, `cryptsvc`), giải phóng các gói dữ liệu cập nhật bị hỏng trong thư mục `SoftwareDistribution` và `catroot2`, sau đó tái kích hoạt các dịch vụ về trạng thái chuẩn.
- **Tối ưu hóa tổng thể (PRO 1-Click):** Tinh chỉnh cấu hình Registry nhằm tối ưu hóa phản hồi I/O, giảm thời gian chờ tắt ứng dụng bị treo (`WaitToKillServiceTimeout`), vô hiệu hóa SysMain/SuperFetch đối với ổ cứng thể rắn (SSD) và quản lý trạng thái Hibernate để giải phóng dung lượng phân vùng hệ thống.
- **Dọn dẹp môi trường Lập trình viên (Dev Cache Clean):** Tự động truy quét và dọn sạch bộ nhớ đệm của các môi trường phát triển: Python (`__pycache__`, `.pytest_cache`), Node.js/NPM, Yarn, Pip, Java/Gradle, Rust/Cargo, Go build cache và Visual Studio Code workspace storage.

### 2. Mạng & An toàn Hệ thống (`Internet`)
- **Thông tin mạng chi tiết:** Truy xuất và hiển thị trạng thái card mạng vật lý/ảo (Network Adapters), địa chỉ IPv4 nội bộ, IPv4 công cộng (Public IP WAN), Subnet Mask, Default Gateway và hệ thống DNS Server đang phân giải.
- **Khôi phục mạng toàn diện (Network Repair PRO):** Quy trình tự động 8 bước chuẩn hóa:
  1. Xóa sạch bộ nhớ đệm DNS (`ipconfig /flushdns`).
  2. Khôi phục danh mục kết nối mạng (`netsh winsock reset`).
  3. Thiết lập lại ngăn xếp giao thức mạng (`netsh int ip reset`).
  4. Xóa bảng ánh xạ địa chỉ MAC (`arp -d *`).
  5. Giải phóng và cấp phát lại cấu hình IP (`release` & `renew`).
  6. Khởi động lại dịch vụ WinNAT & HNS nhằm giải phóng dải cổng mạng bị Hyper-V / Docker / WSL2 chiếm dụng (khắc phục triệt để lỗi `Socket Error 10013`).
  7. Cấu hình tường lửa cho phép kết nối chia sẻ tệp nội bộ (HTTP LAN & cổng LocalSend 53317).
  8. Thiết lập cấu hình mạng về trạng thái Private Network.
- **Lá chắn bảo mật toàn diện (Full Security Shield):** Kích hoạt Windows Defender và cập nhật cơ sở dữ liệu nhận diện mẫu mã độc mới nhất, bật tường lửa Windows Firewall trên toàn bộ Profile, kích hoạt chế độ chống tống tiền Controlled Folder Access (CFA), đóng các cổng dịch vụ mạng có nguy cơ bị khai thác từ xa (445, 139, 135, 137, 138) và thiết lập DNS over HTTPS (Cloudflare DoH `1.1.1.1`).
- **Kiểm tra trạng thái bảo mật:** Rà soát và đánh giá mức độ an toàn của hệ thống thông qua trạng thái hoạt động của Defender, Firewall, dịch vụ Remote Desktop Protocol (RDP) và tính hợp lệ của DNS.
- **Trích xuất mật khẩu Wi-Fi:** Liệt kê toàn bộ hồ sơ mạng Wi-Fi đã từng kết nối trên thiết bị, hiển thị tên mạng (SSID), chuẩn mã hóa bảo mật và mật khẩu ở dạng văn bản rõ.
- **Bảo vệ tập tin cấu hình Hosts:** Kiểm tra tính toàn vẹn của tệp `C:\Windows\System32\drivers\etc\hosts`, tự động cảnh báo các quy tắc điều hướng đáng ngờ nhắm vào cổng thanh toán, ngân hàng điện tử, mạng xã hội, dịch vụ cập nhật hoặc trang web diệt virus.

### 3. Công cụ Tự động & Tiện ích (`UtilityTools`)
- **Tự động nhấp chuột (Auto Click):** Mô phỏng thao tác nhấp chuột theo tọa độ cố định hoặc vị trí con trỏ hiện tại với tần suất tùy chỉnh (tính bằng mili-giây), hỗ trợ phím ngắt khẩn cấp (`ESC` / `F6`).
- **Gửi văn bản tự động (Spam Text):** Hỗ trợ phát chuỗi ký tự liên tục với định dạng Unicode tiếng Việt đầy đủ (`CF_UNICODETEXT`), tối ưu độ trễ giữa các lần gửi và trang bị cơ chế dừng an toàn qua phím tắt.
- **Tự động dán dữ liệu nhiều dòng (Auto Paste):** Đọc tuần tự từng dòng văn bản từ bộ đệm và tự động điền vào các trường nhập liệu tương ứng trên màn hình.
- **Trình quản lý & Cài đặt phần mềm tự động:** Đọc danh mục phần mềm từ tệp cấu hình `apps.txt`, tự động tải về các ứng dụng thông dụng (Chrome, Brave, Cốc Cốc, Zalo, Telegram, Discord, VS Code, Git, 7-Zip, EVKey, OpenKey...) kèm thanh hiển thị tiến trình tải trực quan.
- **Gỡ bỏ ứng dụng mặc định (Bloatware Removal):** Quét và gỡ bỏ tận gốc các gói ứng dụng UWP dư thừa được cài sẵn trên Windows thông qua lệnh PowerShell an toàn.
- **Chẩn đoán tình trạng Pin Laptop (Battery Health Diagnostic):** Giao tiếp trực tiếp với hệ thống quản lý năng lượng ACPI của Windows API và báo cáo năng lượng (`powercfg`), trích xuất chính xác công suất thiết kế (Design Capacity), công suất sạc đầy hiện tại (Full Charge Capacity), chu kỳ sạc và tỷ lệ hao mòn chai pin thực tế.

### 4. Xử lý Đa phương tiện & Nâng cao Chất lượng Ảnh (`MediaProcessor` & `ImageEnhancer`)
- **Phục chế & Làm nét ảnh chuyên sâu (`ImageEnhancer`):**
  - Xây dựng hoàn toàn bằng C++ native, không phụ thuộc công cụ ngoài, tận dụng bộ giải mã Windows Imaging Component (WIC) để hỗ trợ đầy đủ các định dạng: JPG, PNG, BMP, TIFF, WebP, HEIC, DNG (RAW).
  - **Nội suy tái tạo mẫu Lanczos-3 (Lanczos-3 Resampling):** Sử dụng cửa sổ hàm Sinc $6 \times 6$ nhằm phóng đại kích thước ảnh mà vẫn bảo toàn dải tần số cao Nyquist, loại bỏ hiện tượng nhòe khối mờ vốn có ở thuật toán Bicubic truyền thống.
  - **Bộ lọc dẫn đường 2 tầng (2-Scale Guided Filter):** Tách bạch cấu trúc hình ảnh thành hai tầng chi tiết: tầng vi mô ($r=1$) cô lập chính xác từng sợi tóc, sợi gân lá ở mức 1 điểm ảnh; tầng cấu trúc ($r=3$) bảo toàn khối nổi và độ sâu tổng thể.
  - **Khử bệt màu liên tục Cauchy (Cauchy Continuous Coring):** Thay thế việc cắt ngưỡng nhị phân cứng bằng hàm mật độ liên tục $edgeWeight = \frac{grad^2}{grad^2 + 12.0}$, giữ trọn vẹn các vi chi tiết và triệt tiêu hoàn toàn hiện tượng dính chùm điểm ảnh (pixel clumping).
  - **Bù màu bảo toàn độ bão hòa (Constant-Saturation Chroma Tracking):** Tự động điều chỉnh biên độ màu sắc dựa trên tỷ lệ gia tăng độ sáng luma ($Y$), giúp màu xanh của cây cối hoặc các mảng màu rực rỡ không bị bạc màu hoặc xuất hiện viền trắng sáng.
  - **Bảo vệ vùng da chân dung hai lớp (Dual-Zone Portrait Protection):** Phân tích không gian màu chuẩn ITU-R BT.601 để nhận diện chính xác vùng da người; tự động làm mịn vi hạt đối với bề mặt da phẳng (trán, má, cằm) trong khi vẫn tăng cường độ nét sắc sảo cho ngũ quan (mắt, con ngươi, lông mi, bờ môi và sợi tóc).
  - **Nén dải màu mềm mại (Soft Gamut Roll-off):** Tự động co tỷ lệ 3 kênh RGB khi độ bão hòa chạm ngưỡng giới hạn 255, chống hiện tượng cháy sáng cục bộ.
- **Xử lý Video & Âm thanh (Tăng tốc phần cứng FFmpeg):**
  - Tự động nhận diện và tận dụng bộ mã hóa phần cứng GPU (NVIDIA NVENC, Intel QuickSync, AMD AMF).
  - Nén tối ưu dung lượng Video và Ảnh mà vẫn lưu giữ nguyên vẹn siêu dữ liệu gốc (EXIF, GPS).
  - Trích xuất âm thanh từ Video sang định dạng MP3 hàng loạt.
  - Thay đổi tốc độ Video (từ 0.5x đến 2.0x) kết hợp bộ lọc âm thanh thích ứng chống méo cao độ.
  - Chuyển đổi định dạng tệp đa phương tiện linh hoạt.
  - Chuẩn hóa tên tập tin theo quy chuẩn đồng nhất trong thư mục.
  - Kỹ thuật giấu dữ liệu trong tập tin ảnh/video (Steganography).

---

## III. Cấu trúc Mã nguồn

```text
CMD_BOX/
├── bin/                     # Thư mục chứa tệp nhị phân sau biên dịch
│   ├── apps.txt             # Cấu hình danh mục tải phần mềm tự động
│   ├── ffmpeg.exe           # Bộ công cụ xử lý media hỗ trợ tăng tốc phần cứng
│   └── main.exe             # Tệp thực thi chính của chương trình
├── include/                 # Danh mục tệp tiêu đề (Header files)
│   ├── ImageEnhancer.h      # Khai báo thuật toán xử lý & làm nét ảnh
│   ├── Internet.h           # Khai báo module mạng, bảo mật & tường lửa
│   ├── MediaProcessor.h     # Khai báo bộ xử lý video, âm thanh & Steganography
│   ├── SystemCore.h         # Khung điều khiển Win32 API, Job Object & Console I/O
│   ├── SystemOptimizer.h    # Khai báo module bảo trì, dọn dẹp & chỉnh sửa Registry
│   └── UtilityTools.h       # Khai báo tiện ích tự động, clicker & chẩn đoán pin
├── src/                     # Danh mục mã nguồn (Source files)
│   ├── apps.txt             # Tệp nguồn cấu hình danh mục phần mềm
│   ├── ImageEnhancer.cpp    # Cài đặt Lanczos-3, 2-Scale Guided Filter & WIC
│   ├── Internet.cpp         # Cài đặt xử lý socket, tường lửa & kiểm tra bảo mật
│   ├── main.cpp             # Điểm khởi chạy (Entry point) & hệ thống menu
│   ├── MediaProcessor.cpp   # Cài đặt giao tiếp FFmpeg & GPU acceleration
│   ├── SystemCore.cpp       # Cài đặt quản lý tiến trình con & tương tác hệ thống
│   ├── SystemOptimizer.cpp  # Cài đặt dọn dẹp rác, Service API & Taskbar
│   └── UtilityTools.cpp     # Cài đặt tự động hóa chuột/bàn phím & đọc ACPI pin
├── build.bat                # Kịch bản tự động dò tìm trình biên dịch và build mã nguồn
└── README.md                # Tài liệu hướng dẫn kỹ thuật của dự án
```

---

## IV. Hướng dẫn Biên dịch & Vận hành

### 1. Yêu cầu Hệ thống
- **Hệ điều hành:** Windows 10 hoặc Windows 11 (phiên bản 64-bit).
- **Bộ biên dịch C++:** GCC/G++ từ bộ công cụ MinGW-w64 (MSYS2 UCRT64 hoặc MINGW64) hỗ trợ đầy đủ tiêu chuẩn **C++17** và OpenMP (`-fopenmp`).

### 2. Biên dịch Tự động
Khởi chạy tệp script đi kèm trong thư mục gốc của dự án:
```cmd
build.bat
```
Script sẽ tự động tìm kiếm trình biên dịch `g++` trong hệ thống (hoặc đường dẫn mặc định `C:\msys64\ucrt64\bin`), tiến hành biên dịch với mức độ tối ưu hóa `-O3`, liên kết tĩnh toàn bộ thư viện cần thiết và xuất tệp nhị phân tại `bin\main.exe`.

### 3. Biên dịch Thủ công qua Dòng lệnh
Nếu muốn tự biên dịch thủ công qua Command Prompt hoặc PowerShell:
```cmd
g++ -std=c++17 -O3 -fopenmp -Iinclude src\*.cpp -o bin\main.exe -lws2_32 -liphlpapi -lole32 -lwindowscodecs -loleaut32 -luuid -static-libgcc -static-libstdc++ -static -s
copy /y "src\apps.txt" "bin\apps.txt"
```

*Giải thích các cờ liên kết (Linker Flags):*
- `-lws2_32 -liphlpapi`: Giao diện mạng Windows Sockets và IP Helper API.
- `-lole32 -lwindowscodecs -loleaut32 -luuid`: Hỗ trợ giao tiếp COM và bộ giải mã hình ảnh Windows Imaging Component (WIC).
- `-fopenmp`: Kích hoạt xử lý đa luồng CPU cho các thuật toán nội suy và lọc ảnh.
- `-static-libgcc -static-libstdc++ -static -s`: Đóng gói tĩnh toàn bộ thư viện C++ runtime và lược bỏ ký hiệu gỡ lỗi, giúp file `.exe` chạy độc lập mà không yêu cầu DLL ngoài.

### 4. Tích hợp FFmpeg (Tùy chọn cho Phân hệ Media)
Để sử dụng các tính năng nén video, trích xuất âm thanh và thay đổi tốc độ khung hình:
1. Tải bản build Portable của FFmpeg từ trang chính thức: [gyan.dev/ffmpeg/builds](https://www.gyan.dev/ffmpeg/builds/).
2. Đặt tệp thực thi `ffmpeg.exe` vào thư mục `bin\` (cùng cấp với `main.exe`).

### 5. Khởi chạy Ứng dụng
- Để tất cả các tính năng can thiệp Registry, Win32 Service Manager, Tường lửa hệ thống và xóa tập tin hệ thống hoạt động chính xác, hãy nhấp chuột phải vào `bin\main.exe` và chọn **Run as Administrator** (hoặc mở Command Prompt với quyền quản trị viên).