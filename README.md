# 🛠️ CMD-TOOL v0.4.0 

> **Tác giả:** huii404

---

## I. Giới thiệu Tổng quan & Ưu điểm Vượt trội

**CMD-TOOL** là phần mềm quản trị hệ thống chuyên sâu được xây dựng trên nền tảng C++ native, giúp can thiệp an toàn và tối ưu tài nguyên máy tính triệt để.

### 🚀 Ưu điểm độc quyền:
* **Quản lý Tiến trình An toàn (Windows Job Objects):** Các tiến trình con (DISM, SFC, msedgewebview2, curl...) được gán chặt vào `hJob`. Khi tắt đột ngột tool (`Ctrl + C` hoặc đóng cửa sổ), toàn bộ tiến trình con lập tức bị khai tử, không gây treo máy hay kẹt nền.
* **Siêu nhẹ & Tối ưu (Zero-Overhead):** Loại bỏ hoàn toàn `using namespace std;`, tối ưu vùng nhớ qua truyền tham chiếu hằng (`const std::string&`). Chiếm dưới 5MB RAM khi vận hành.
* **Gỡ sạch Bloatware:** Dùng PowerShell và lọc Regex bóc tách tận gốc ứng dụng rác cài sẵn (Copilot, Teams, Candy Crush...) mà không ảnh hưởng tới Microsoft Office.
* **Chia sẻ & Giao tiếp Nội bộ (LAN HTTP Server):** Tự khởi tạo mạng HTTP Server và Socket (Winsock2) trực tiếp trên RAM để truyền file qua Wi-Fi/LAN, không phụ thuộc Cloud, bảo mật tuyệt đối.

---

## II. Cấu trúc Chức năng Cốt lõi

### 1. 📊 Thông tin Hệ thống (Module: `Information`)
* **Phần cứng sâu:** Đọc CPU Model bằng hợp ngữ (`__cpuid`), tính Uptime, check dung lượng ổ C, phân biệt Win 10/11 và kiểm tra bản quyền qua `slmgr.vbs`.
* **Cấu hình mạng:** Kiểm tra toàn bộ cổng mạng đang kết nối/lắng nghe (ESTABLISHED/LISTENING) kèm PID quản lý qua `netstat/ipconfig`.

### 2. ⚡ Bảo trì & Tối ưu (Module: `SystemOptimizer`)
* **Dọn rác chuyên sâu:** Xóa sạch Cache của hơn 8 trình duyệt, thư mục Temp, Prefetch, bộ nhớ đệm Delivery Optimization và ép dọn thùng rác ngầm.
* **Tinh chỉnh Cấu trúc Registry:** Giảm thời gian chờ tắt máy (`WaitToKillAppTimeout`), tắt ngủ đông (giải phóng file `hiberfil.sys` vài chục GB), chặn Windows Telemetry thu thập dữ liệu ngầm.
* **Sửa lỗi Windows Update:** Dừng đồng bộ 4 dịch vụ cốt lõi (`wuauserv`, `cryptSvc`, `bits`, `msiserver`) để làm sạch thư mục kẹt `SoftwareDistribution`.
* **🎯 Tối ưu Taskbar (Mới v0.4.0):** Tự động tắt các nút thừa trên Taskbar Windows 11 bao gồm: Search, Widgets, Chat (Teams), Task View, News & Interests, Copilot và Snap Assist. Giúp giao diện gọn gàng, sạch sẽ, tăng trải nghiệm người dùng.

### 3. 🌐 Mạng & Chia sẻ (Module: `Internet`)
* **QuickShare PRO:** Kéo thả tệp tin để tạo máy chủ HTTP tải file qua LAN. Tự động kiểm soát file (< 1.5GB), mở Firewall Rule và hiển thị % tiến độ trực quan.
* **Chat LAN Nội bộ:** Khởi tạo Chat Server tại Port 9000. Hỗ trợ bộ giải mã URL (Decode URL) để xử lý ký tự đặc biệt, dấu cách từ điện thoại gửi lên.

### 4. 🤖 Công cụ Tự động (Module: `UtilityTools`)
* **Thao tác tự động:** Auto Click theo tọa độ (`GetCursorPos`), Spam Text, Auto Paste nhiều dòng liên tiếp bằng cách chiếm quyền Clipboard (`SetClipboardData`) và giả lập tổ hợp `Ctrl + V`, `Enter`.
* **Vẽ Mã QR:** Gọi `curl` liên kết API `qrenco.de` để render trực tiếp mã QR độ phân giải văn bản ngay trên màn hình Console.

### 5. 🎬 Xử lý Media (Module: `MediaProcessor`) - *Nâng cấp v0.4.0*
* **Nén dung lượng thông minh:** Tự động phát hiện và nén ảnh (JPG, PNG, WebP, HEIC) và video (MP4, MKV, AVI, MOV) với thuật toán tối ưu. Giữ nguyên metadata gốc bao gồm EXIF, GPS, thông số máy ảnh.
* **Phục chế & Làm nét bằng AI:** Tích hợp bộ lọc AI (hqdn3d, nlmeans, bm3d, unsharp) giúp khử nhiễu, tăng độ sắc nét, cân bằng độ tương phản cho ảnh và video.
* **Trích xuất âm thanh:** Chuyển đổi video sang MP3 nhanh chóng, giữ nguyên chất lượng âm thanh.
* **Thay đổi tốc độ video:** Hỗ trợ Slow-motion (0.5x) đến Tua nhanh (2.0x) với xử lý âm thanh không bị méo.
* **Chuyển đổi định dạng:** Đổi đuôi file ảnh/video mà vẫn giữ nguyên chất lượng và metadata gốc.
* **🎯 Tạo GIF từ ảnh (Mới v0.4.0):** Tạo ảnh động GIF từ 1 hoặc nhiều ảnh tĩnh. Hỗ trợ hiệu ứng Zoom, Pan, Fade, Rotate. Tạo hàng loạt GIF từ nhiều ảnh cùng lúc.

### 6. 📦 Ứng dụng & Tiện ích (Module: `UtilityTools` & `Manager`)
* **Trình cài đặt tự động:** Sử dụng `curl -L` tự động tải và kích hoạt cài đặt các app thiết yếu (Chrome, Zalo, Discord, VS Code...) vào thư mục Temp.
* **Quản lý Dịch vụ (Turn Off Services):** Giao diện căn lề bằng `std::setw`. Hỗ trợ tắt hàng loạt hoặc tắt riêng lẻ các dịch vụ ngầm vô dụng (Print Spooler, Bluetooth, Windows Insider, Xbox...) về `MANUAL` hoặc `DISABLED`.

---

## III. Hướng dẫn Sử dụng Nhanh

1. **Quyền khởi chạy:** Bắt buộc chạy bằng quyền Quản trị (**Run as Administrator**) để các API hệ thống (`OpenSCManager`, cấu hình Registry, Firewall) thực thi thành công.
2. **Điều hướng:** Nhập số từ `[1]` đến `[5]` tại Menu chính -> Nhấn `Enter`. Nhập `[0]` để quay lại menu cấp trước.
3. **Mẹo QuickShare:** Vào mục chia sẻ file -> Kéo thả trực tiếp file vào CMD (tool tự khử dấu ngoặc kép `"`) -> Lấy link IP hiển thị gửi cho thiết bị khác cùng mạng để tải xuống.
4. **Mẹo Quản lý Dịch vụ:** Vào menu tắt dịch vụ -> Gõ số thứ tự của dịch vụ (Ví dụ: `15` - Print Spooler) -> Nhập `[2]` để `DISABLED` (Tắt hẳn).
5. **Mẹo Tạo GIF:** Vào menu Media -> Chọn `[6] Tạo GIF từ ẢNH` -> Kéo thả ảnh -> Chọn hiệu ứng (Zoom/Pan/Fade/Rotate) hoặc tạo GIF động từ nhiều ảnh.

---

## IV. Lưu ý Quan trọng & Bảo mật

* **Windows Widgets:** Tool gỡ gói "Windows Web Experience Pack" để tắt hoàn toàn Widgets ngầm, giải phóng ~200MB RAM của `msedgewebview2.exe`. *Khuyến cáo: Không tự ý xóa file msedgewebview2.exe vật lý vì sẽ gây lỗi Zalo PC và Discord.*
* **Metadata ảnh:** Khi xử lý ảnh với FFmpeg, tool tự động thêm tham số `-map_metadata 0` để giữ nguyên metadata gốc (EXIF, GPS, thông số máy ảnh). Tuy nhiên, khi chuyển từ JPG sang PNG/WEBP, một số metadata có thể bị mất do định dạng không hỗ trợ.
* **Dọn dẹp trình duyệt:** Hãy tắt hẳn các trình duyệt (Chrome, Edge...) trước khi dọn dẹp để tránh lỗi tệp tin đang bận (`[!] Đang bận`).
* **Điểm khôi phục:** Nên tạo Điểm khôi phục hệ thống (**System Restore Point**) trước khi tắt dịch vụ hàng loạt. Muốn bật lại thủ công, sử dụng lệnh `services.msc`.

---

## V. Tài nguyên Bổ sung & Cấu hình Build

### 1. Cấu hình `.vscode/tasks.json` (Trình biên dịch g++ UCRT64)
```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "type": "cppbuild",
            "label": "C/C++: Build(Multi-file + Winsock)",
            "command": "C:\\msys64\\ucrt64\\bin\\g++.exe",
            "args": [
                "-fdiagnostics-color=always", "-g", "-std=c++17",
                "${workspaceFolder}\\src\\*.cpp",
                "-o", "${workspaceFolder}\\bin\\main.exe",
                "-lws2_32", "-static-libgcc", "-static-libstdc++", "-static"
            ],
            "options": { "cwd": "${workspaceFolder}" },
            "group": { "kind": "build", "isDefault": true }
        }
    ]
}
```

### 2. Thư viện bên thứ ba (Cấu hình Thư mục `/bin`)
Tải bản Portable của **FFmpeg** (lấy `ffmpeg.exe`) và **Waifu2x-NCNN-Vulkan** (lấy `waifu2x-ncnn-vulkan.exe` và thư mục `models`). Sắp xếp cấu trúc file trong thư mục đầu ra `/bin` như sau để tool nhận diện bằng đường dẫn tương đối:

```text
CMD_TOOL_v0.4.0/
├── include/                 # Thư mục chứa các file tiêu đề (.h)
├── src/                     # Thư mục chứa các file mã nguồn (.cpp)
└── bin/                     # THƯ MỤC CHỨA SẢN PHẨM SAU KHI BIÊN DỊCH
    ├── main.exe             # File thực thi chính sau khi build
    ├── ffmpeg.exe           # [Bỏ vào đây] Bộ xử lý media (FFmpeg)
    ├── waifu2x-ncnn-vulkan.exe # [Bỏ vào đây] Bộ xử lý AI Upscale
    ├── models/              # [Bỏ vào đây] Thư mục chứa model AI của Waifu2x
    │   ├── models-cunet/
    │   └── models-upconv_7_photo/
    └── log_memory/          # Thư mục tự sinh chứa nhật ký RAM Clipboard
        └── Clipboard_History.txt
```

### 3. Hướng dẫn cài đặt FFmpeg & Waifu2x

**FFmpeg:**
- Tải bản zip tại [gyan.dev/ffmpeg/builds](https://www.gyan.dev/ffmpeg/builds/) (Mục git master/release builds -> gói `ffmpeg-git-essentials.7z`)
- Giải nén và copy duy nhất file `ffmpeg.exe` từ thư mục `bin` vào thư mục `/bin` của tool

**Waifu2x-NCNN-Vulkan:**
- Tải gói Windows tại [GitHub nihui/waifu2x-ncnn-vulkan/releases](https://github.com/nihui/waifu2x-ncnn-vulkan/releases)
- Giải nén lấy file `waifu2x-ncnn-vulkan.exe` và nguyên thư mục `models` vào thư mục `/bin` của tool

---

## VI. Changelog v0.4.0

### ✨ Tính năng mới:
- **Tối ưu Taskbar:** Tự động tắt Search, Widgets, Chat/Teams, Task View, News & Interests, Copilot, Snap Assist
- **Tạo GIF từ ảnh:** Hỗ trợ tạo GIF từ 1 hoặc nhiều ảnh, hiệu ứng Zoom/Pan/Fade/Rotate
- **Xử lý GIF hàng loạt:** Tạo nhiều GIF cùng lúc từ nhiều ảnh
- **Metadata Preservation:** Giữ nguyên metadata ảnh (EXIF, GPS) khi xử lý với FFmpeg

### 🔧 Cải thiện:
- **Tối ưu `optimizeSystemPRO()`:** Gộp tối ưu Taskbar vào chức năng tối ưu hệ thống PRO
- **Xử lý HEIC:** Hỗ trợ chuyển đổi và nén ảnh HEIC sang JPG
- **Kiểm tra codec:** Tự động phát hiện và xử lý codec không tương thích khi đổi đuôi video

---
