# 🛠️ CMD BOX - SYSTEM TOOLKIT
 
> **Ngôn ngữ:** C++17 | **Nền tảng:** Windows (x64)

---

## I. Giới thiệu Tổng quan

**CMD BOX** là bộ công cụ quản trị, bảo trì, tối ưu hóa và tiện ích hệ thống được xây dựng trên nền tảng C++ native. Phần mềm can thiệp an toàn vào hệ thống Windows thông qua Windows API trực tiếp, giúp tối ưu hóa hiệu năng, bảo mật và cung cấp các công cụ hữu ích mà không tiêu tốn tài nguyên nền.

### 🚀 Ưu điểm nổi bật
* **Quản lý tiến trình an toàn (Windows Job Objects):** Mọi tiến trình con khởi tạo từ tool đều được gắn vào Job Object của hệ điều hành. Khi người dùng đóng tool hoặc nhấn `Ctrl + C`, toàn bộ tiến trình con sẽ được dọn dẹp sạch sẽ, không gây chạy ngầm.
* **Tối ưu hóa tài nguyên:** Khởi tạo trễ (Lazy Loading) với `std::unique_ptr` và cơ chế Thread-safe, chiếm dưới 5MB RAM khi vận hành.
* **Giao diện Console tinh tế:** Giao diện dòng lệnh trực quan, điều hướng nhanh chóng bằng phím số.
* **Không phụ thuộc Cloud:** Các tính năng hoạt động trực tiếp qua Windows API native mà không gửi dữ liệu ra bên ngoài.

---

## II. Cấu trúc Tính năng

Chương trình được chia thành **4 phân hệ chính**:

### 1. 🛠️ Bảo trì & Tối ưu Hệ thống (`SystemOptimizer`)
* **Dọn rác chuyên sâu PRO:** Quét đa luồng, làm sạch toàn diện đa Profile của hơn 8 trình duyệt (Chrome, Edge, CocCoc, Brave, Vivaldi, Opera, Opera GX, Firefox), dọn rác Lập trình viên (Node/NPM, Yarn, Pip, NuGet, Gradle, Rust/Cargo, Go, VS Code Cache), thư mục Temp, Prefetch, Thumbcache, CBS Logs, Delivery Optimization, Windows Update Cache và dọn sạch Thùng rác.
* **Quản lý & Tắt ứng dụng khởi động:** Quét các Registry Run keys với Whitelist thông minh bảo vệ driver phần cứng, âm thanh (Realtek, Waves), card màn hình (NVIDIA, AMD, Intel) và OEM tools (ASUS, Dell, Lenovo, HP).
* **Quản lý Dịch vụ Windows (Services Control API):** Tinh chỉnh hoặc vô hiệu hóa các dịch vụ ngầm không cần thiết (Windows Update, Telemetry, Maps, Xbox Services, Error Reporting...) thông qua Win32 Service Manager.
* **Tinh chỉnh Taskbar Windows 11:** Ẩn các nút không dùng trên Taskbar (Search Box, Widgets, Chat/Teams, Task View, News & Interests, Copilot, Snap Assist).
* **Sửa lỗi Windows Update:** Dừng các dịch vụ liên quan, xóa sạch thư mục cache bị kẹt (`SoftwareDistribution`, `catroot2`) và khôi phục lại hoạt động.
* **Tối ưu hóa tổng thể (PRO 1-Click):** Tự động áp dụng toàn bộ tinh chỉnh Registry, giảm độ trễ tắt máy, tắt Hibernate giải phóng ổ C và dọn dẹp hệ thống chỉ với một thao tác.

### 2. 🌐 Mạng & Bảo mật (`Internet`)
* **Thông tin mạng chi tiết:** Xem địa chỉ IP nội bộ (LAN), truy vấn Public IP qua Internet, thông tin Adapter mạng, Subnet Mask và DNS Server.
* **Sửa lỗi & Khôi phục mạng toàn diện (Network Repair PRO):** Quy trình 8 bước tự động: Flush DNS, Reset Winsock Catalog, Reset TCP/IP Stack, xóa ARP Cache, Release & Renew IP, khởi động lại WinNAT & HNS (giải phóng dải cổng bị Hyper-V/Docker/WSL2 chiếm giữ và khắc phục triệt để lỗi Socket 10013), mở cổng Tường lửa Firewall cho HTTP LAN & LocalSend (TCP/UDP 53317), và chuyển trạng thái mạng sang Private Network.
* **Kích hoạt Bảo mật Toàn diện (Full Security Shield):** Bật Windows Defender & cập nhật Signature, bật Firewall, bật Controlled Folder Access, chặn các cổng nguy hiểm (445, 139, 135, 137, 138), cấu hình DNS over HTTPS (Cloudflare 1.1.1.1).
* **Kiểm tra trạng thái bảo mật:** Báo cáo chi tiết trạng thái của Defender, Firewall, dịch vụ Remote và DNS.
* **Xem mật khẩu Wi-Fi đã lưu:** Trích xuất tên, mật khẩu và chuẩn mã hóa của các mạng Wi-Fi từng kết nối.

### 3. ⚡ Công cụ Tự động & Tiện ích (`UtilityTools`)
* **Auto Click chuột:** Tự động click theo tọa độ với tùy chỉnh số lần, delay (ms), hỗ trợ phím ngắt khẩn cấp (`ESC` / `F6`).
* **Spam Text:** Tự động gửi tin nhắn hoặc văn bản lặp lại, hỗ trợ gõ chuẩn Tiếng Việt Unicode (`CF_UNICODETEXT`) và dừng khẩn cấp bằng `ESC` / `F6`.
* **Auto Paste dữ liệu:** Hỗ trợ nhập và dán tự động danh sách dữ liệu nhiều dòng chuẩn Unicode và ngắt khẩn cấp bằng phím tắt.
* **Trình tải & Cài đặt phần mềm:** Tải nhanh các phần mềm phổ biến (Chrome, CocCoc, Brave, EVKey, OpenKey, Zalo, Discord, Telegram, 7-Zip, WinRAR, WARP, VS Code, Notepad++, Git) kèm thanh tiến trình trực quan.
* **Gỡ bỏ ứng dụng rác (Bloatware Windows):** Quét và gỡ bỏ tận gốc các ứng dụng mặc định thừa thãi trên Windows bằng PowerShell script.

### 4. 🎬 Bộ xử lý Media (`MediaProcessor` - FFmpeg & GPU Acceleration)
* **Tăng tốc phần cứng GPU:** Tự động nhận diện GPU (NVIDIA NVENC, Intel QuickSync, AMD AMF) để tăng tốc độ render, nén và xử lý video siêu tốc.
* **Nén dung lượng Media:** Tự động tối ưu dung lượng Video / Ảnh mà vẫn giữ nguyên metadata gốc (EXIF, GPS).
* **Phục chế & Làm nét:** Sử dụng các bộ lọc chuyên sâu để khử nhiễu và tăng độ nét.
* **Chuyển đổi Mp4 -> Mp3:** Trích xuất âm thanh từ video nhanh chóng.
* **Thay đổi tốc độ Video:** Hỗ trợ từ 0.5x (Slow-motion) đến 2.0x (Tua nhanh) với thuật toán xử lý âm thanh chống méo tiếng.
* **Đổi đuôi định dạng:** Chuyển đổi qua lại giữa các định dạng media phổ biến.
* **Chuẩn hóa tên file:** Tự động định dạng lại tên file trong thư mục.
* **Ẩn & Dò tìm file bí mật:** Kỹ thuật Steganography giấu file dữ liệu vào trong Ảnh hoặc Video.

---

## III. Cấu trúc Thư mục

```text
CMD_BOX/
├── include/                 # Header files (.h)
│   ├── SystemCore.h         # Lớp cơ sở điều khiển Win32 API & Job Objects
│   ├── SystemOptimizer.h    # Module bảo trì & tối ưu hệ thống
│   ├── Internet.h           # Module mạng, socket P2P & bảo mật
│   ├── UtilityTools.h       # Module công cụ tự động & tiện ích
│   └── MediaProcessor.h     # Module xử lý media (FFmpeg)
├── src/                     # Source files (.cpp)
│   ├── SystemCore.cpp
│   ├── SystemOptimizer.cpp
│   ├── Internet.cpp
│   ├── UtilityTools.cpp
│   ├── MediaProcessor.cpp
│   └── main.cpp             # Entry point & Menu điều hướng
└── bin/                     # Thư mục chứa file sau khi build
    ├── cmd_box.exe          # File thực thi chính
    └── ffmpeg.exe           # Bộ xử lý media (tùy chọn)
```

---

## IV. Hướng dẫn Biên dịch & Chạy

### 1. Yêu cầu môi trường
* Hệ điều hành: Windows 10 / 11 (64-bit).
* Trình biên dịch: GCC/G++ (MinGW-w64 / MSYS2) hỗ trợ C++17 trở lên.

### 2. Lệnh biên dịch (Command Line)
```bash
g++ -std=c++17 -Iinclude src/*.cpp -o bin/cmd_box.exe -lws2_32 -liphlpapi -static-libgcc -static-libstdc++
```

### 3. Cài đặt FFmpeg (Dành cho Module Media)
1. Tải bản build Portable của FFmpeg từ [gyan.dev/ffmpeg/builds](https://www.gyan.dev/ffmpeg/builds/).
2. Giải nén và sao chép file `ffmpeg.exe` vào thư mục `bin/` cùng cấp với `cmd_box.exe`.

### 4. Khởi chạy
* **Khuyến nghị:** Nhấp chuột phải vào `cmd_box.exe` và chọn **Run as Administrator** để sử dụng đầy đủ các tính năng tối ưu hệ thống, chỉnh sửa Registry, Service API và cấu hình tường lửa.

---