# CMD BOX - SYSTEM TOOLKIT

> **Nền tảng:** Windows (x64) | **Ngôn ngữ:** C++17 | **Biên dịch:** MinGW-w64 (GCC / UCRT64)
>
> **Phiên bản:** 2.1.1

Các tác vụ media giữ nguyên file nguồn và ghi kết quả vào thư mục `CMD_BOX_Output` nằm cạnh file nguồn. Dọn tự động yêu cầu xác nhận, giữ Thùng rác và thư mục rollback Windows.

Lần đầu dùng, chọn hồ sơ trong **Setup** hoặc `main.exe setup`. Menu và CLI kiểm tra cùng bảng quyền; chạy Admin vẫn tuân theo hồ sơ. Cấu hình `cmd_box.json` đặt cạnh executable. Xem [hồ sơ sử dụng và sandbox test](README_SECURITY_PROFILES.md).

Có thể mở menu như cũ hoặc gọi nhanh từ terminal:

Trong menu tương tác, dùng **↑/↓** để chọn và **Enter** để mở. Có thể gõ số rồi nhấn **Enter** để chọn nhanh; **Esc** hoặc **0 + Enter** để quay lại. Menu giữ nguyên khung khi di chuyển lựa chọn. Các màn hình tác vụ vẫn nhận đường dẫn và nội dung văn bản như trước.

```cmd
main.exe --help
main.exe clean
main.exe optimize 1
main.exe scan-network
main.exe security-status
main.exe media
```

**Tra cứu & Quy chuẩn:** [Sổ tay lệnh Windows CMD](README_WINDOWS_COMMANDS.md) — lệnh hữu ích, tác dụng, ví dụ | [Quy chuẩn Phát triển &amp; Kiến trúc](README_RULES.md) — 7 nguyên tắc thiết kế theo quy tắc.txt.

**Nghiên cứu tính năng sửa lỗi:** [Tên lỗi, 24 kịch bản, logic fix và lựa chọn C++/batch](README_FIX_RESEARCH.md) — nguồn Microsoft, ưu tiên triển khai và nhật ký nghiên cứu.

---

## I. Giới thiệu Tổng quan

**CMD BOX** là bộ công cụ dòng lệnh (CLI) hiệu năng cao dành cho quản trị, bảo trì, tối ưu hóa hệ điều hành, bảo mật mạng và xử lý đa phương tiện trên nền tảng Windows.

Dự án dùng **C++ native**, Win32 API và OpenMP. Bản build mặc định không yêu cầu AVX2/FMA. Media cần FFmpeg, ffprobe và ExifTool (xem [metadata và album](docs/MEDIA.md)); tải ứng dụng và kiểm tra release cần Internet. Web Drop dùng API QR bên thứ ba để tạo mã QR, còn nội dung file được truyền trực tiếp trong LAN. LocalDrop v2 hỗ trợ nhiều file/thư mục, mã ghép đôi, tải tiếp và kiểm tra SHA-256 (xem chi tiết tại phân hệ Mạng & An toàn Hệ thống).

### Đặc tính Kỹ thuật

- **Quản lý tiến trình:** Runner trực tiếp dùng Job Object và timeout để dừng cây child. Lệnh UAC qua ShellExecute và một số đường gọi system/_popen không có cùng bảo đảm; xem giới hạn trong báo cáo sửa.
- **Khởi tạo trễ (Lazy Loading):** Áp dụng mô hình con trỏ thông minh `std::unique_ptr` kết hợp cơ chế kiểm tra đa luồng (Double-Checked Locking / Thread-safe). Ứng dụng chỉ cấp phát bộ nhớ khi người dùng truy cập phân hệ tương ứng, duy trì mức chiếm dụng RAM cực thấp ở trạng thái chờ.
- **Bảo tồn metadata có kiểm chứng:** FFmpeg/ExifTool ghi metadata trực tiếp trong file và đọc lại để kiểm tra; không xuất JSON đi kèm. Nếu không giữ được trường kỷ niệm cần thiết, từ chối bản xuất và giữ nguồn. Thông tin kỹ thuật được cập nhật theo bản render. Xem [hướng dẫn](docs/MEDIA.md).
- **Tương thích toàn diện:** Hoạt động ổn định trên Windows 10 và Windows 11 (64-bit).

---

## II. Cấu trúc Phân hệ Chức năng

Chương trình được phân tách thành 4 phân hệ chính theo từng lĩnh vực chuyên biệt:

### 1. Bảo trì & Tối ưu Hệ thống (`SystemOptimizer` & `DiskCleaner`)

> *Xem mã nguồn chi tiết tại:* [`include/DiskCleaner.h`](include/DiskCleaner.h) và thư mục [`src/diskcleaner/`](src/diskcleaner/)

- **Dọn rác Đa Tầng Chuyên Trách (`DiskCleaner`):**
  - *Tầng 1 (Rác bề mặt & User Cache):* Dọn Temp, `Windows\Temp`, `CrashDumps`, WER, D3DSCache, INetCache và Flush DNS. Giữ Thùng rác.
  - *Tầng 2 (Trình duyệt & Ứng dụng):* Quét đa profile tất cả trình duyệt Chromium (Chrome, Edge, Cốc Cốc, Brave, Vivaldi, Opera, Opera GX) và Firefox; dọn cache Discord, Telegram, NVIDIA Shader Cache.
  - *Tầng 3 (Chuyên sâu Hệ thống):* Dọn cache Windows Update sau kiểm tra trạng thái dịch vụ, Delivery Optimization và log/dump cho phép. DISM dùng `/StartComponentCleanup`; giữ Windows.old và thư mục rollback.
  - *Tầng 4 (Môi trường lập trình):* Dọn cache công cụ. Dọn tự động không quét project; chốt engine chặn `.git`, marker bảo vệ và reparse point.
  - *Tầng 5 (Downloads):* File trùng và bộ cài được nhận diện đưa vào Thùng rác; file tải dở đủ điều kiện được xóa trực tiếp.
  - *Dọn liên hoàn:* Yêu cầu xác nhận trước khi thực hiện các nhóm trên.
- **Tăng tốc & Tối ưu Đa Tầng:**
  - *Tầng 1 (Tối ưu Khởi động):* Phân tích các khóa Registry `Run` và `RunOnce` (`HKCU` & `HKLM`). Tích hợp Whitelist thông minh bảo vệ driver phần cứng (Realtek, Waves, NVIDIA, AMD, Intel) và phần mềm điều khiển OEM (ASUS, Dell, HP, Lenovo).
  - *Tầng 2 (Tối ưu Dịch vụ ngầm):* Giữ nhóm 8 dịch vụ cũ: MapsBroker, WalletService, RetailDemo, DiagTrack, dmwappushservice, WerSvc, RemoteRegistry và wisvc.
  - *Tầng 3 (Tối ưu Giao diện & Độ nhạy Windows):* Tùy biến nhanh thanh tác vụ Taskbar Windows 11 (Search Box, Widget, Chat/Teams, Task View, Copilot) và giảm độ trễ phản hồi UI.
  - *Quản lý dịch vụ Windows nâng cao:* Danh sách 40 dịch vụ, chọn từng mục để chuyển Manual hoặc Disabled; mô tả ảnh hưởng được hiển thị trước khi áp dụng. CMD thường xin quyền Admin cho từng dịch vụ; mở chương trình bằng quyền Admin để chạy liên tục.
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
- **Trạm truyền file P2P nội bộ (LocalDrop v2):** Gửi nhiều file/thư mục qua LAN, ghép đôi bằng mã 6 số, trang tải bằng QR API. Máy nhận CMD BOX tự tiếp tục khi mất kết nối và kiểm tra SHA-256 trước khi công nhận hoàn tất; file tải dở lưu trong thư mục nhận (HTTP truyền trực tiếp trong LAN, xem giao thức tại [`include/LocalDropProtocol.h`](include/LocalDropProtocol.h)).

### 3. Công cụ Tự động & Tiện ích (`UtilityTools`)

- **Tự động nhấp chuột (Auto Click):** Mô phỏng thao tác nhấp chuột theo tọa độ cố định hoặc vị trí con trỏ hiện tại với tần suất mili-giây tùy chỉnh, phím ngắt khẩn cấp (`ESC` / `F6`).
- **Gửi văn bản tự động (Spam Text):** Phát chuỗi ký tự liên tục với định dạng Unicode tiếng Việt (`CF_UNICODETEXT`), tối ưu độ trễ giữa các lần gửi.
- **Tự động dán dữ liệu nhiều dòng (Auto Paste):** Đọc tuần tự từng dòng văn bản từ bộ đệm và tự động điền vào các trường nhập liệu tương ứng.
- **Cài đặt phần mềm tự động:** Tự động tải về và cài đặt các ứng dụng thông dụng (Chrome, Brave, Cốc Cốc, Zalo, Telegram, Discord, VS Code, Git, 7-Zip, EVKey, OpenKey...) theo nhóm hoặc tải lẻ thông qua kịch bản [`scripts/download-apps.bat`](scripts/download-apps.bat).
- **Gỡ bỏ ứng dụng rác (Bloatware Removal):** Quét và gỡ bỏ tận gốc các gói ứng dụng UWP dư thừa được cài sẵn trên Windows.
- **Kiểm tra Pin Laptop chuyên sâu (Battery Diagnostic):** Đọc trực tiếp từ ACPI và báo cáo pin Windows (`powercfg`), trích xuất công suất thiết kế, công suất sạc đầy hiện tại, chu kỳ sạc, tỷ lệ hao mòn chai pin thực tế và tình trạng sạc.

### 4. Xử lý Đa phương tiện (`MediaProcessor`)

#### Xử lý Video, Ảnh & Âm thanh (Tăng tốc phần cứng FFmpeg):

- Tự động nhận diện và tận dụng bộ mã hóa phần cứng GPU (NVIDIA NVENC, Intel QuickSync, AMD AMF).
- Nén tối ưu dung lượng Video MP4 và Ảnh (PNG/JPG) bảo toàn độ nét và Metadata.
- Trích xuất âm thanh từ Video sang định dạng MP3 hàng loạt.
- Thay đổi tốc độ Video (0.5x đến 2.0x) kèm bộ lọc âm thanh thích ứng chống méo cao độ.
- Đổi định dạng tệp đa phương tiện linh hoạt.
- Chuẩn hóa tên tập tin theo quy chuẩn đồng nhất trong thư mục.
- Sắp album từ thư mục theo năm/tháng chụp/quay, có preview; sao chép và giữ file gốc.
- Ẩn file vào file (Steganography).

## III. Cấu trúc Mã nguồn

```text
CMD_BOX/
├── bin/                     # Thư mục chứa tệp nhị phân sau biên dịch (được gitignore)
│   ├── ffmpeg.exe           # Bộ công cụ media tăng tốc GPU (tải qua setup-media.ps1)
│   └── main.exe             # Tệp thực thi chính của chương trình
├── docs/                    # Tài liệu kỹ thuật chuyên sâu & bản quyền bên thứ ba
│   ├── ExifTool_LICENSE     # Bản quyền công cụ ExifTool
│   ├── MEDIA.md             # Đặc tả xử lý Media, Metadata & Album
│   └── THIRD_PARTY.md       # Danh mục thành phần thư viện bên thứ ba
├── include/                 # Danh mục tệp tiêu đề (Header files)
│   ├── SystemCore.h         # Khung điều khiển Win32 API, Job Object, Console I/O dùng chung
│   ├── MenuStyle.h          # Chuẩn viền liền khối & màu ANSI Neon đồng nhất cho UI
│   ├── FileSafety.h         # Cơ chế chốt chặn an toàn, bảo vệ thư mục & file hệ thống
│   ├── CleanerCore.h        # Cốt lõi engine dọn dẹp hệ thống an toàn
│   ├── DiskCleaner.h        # Module dọn dẹp ổ đĩa đa tầng
│   ├── BrowserCleaner.h     # Dọn dẹp cache trình duyệt (Chromium, Firefox)
│   ├── DevCleaner.h         # Dọn dẹp môi trường dev (chặn xóa .git và project)
│   ├── DownloadsCleaner.h   # Phân loại và dọn dẹp thư mục Downloads
│   ├── SystemDeepCleaner.h  # Dọn dẹp chuyên sâu hệ thống (DISM, Windows Update cache)
│   ├── TempCleaner.h        # Dọn dẹp thư mục Temp, Prefetch an toàn
│   ├── SystemOptimizer.h    # Module tinh chỉnh dịch vụ Windows, Taskbar & Registry
│   ├── Internet.h           # Module mạng, bảo mật, tường lửa & sửa lỗi kết nối
│   ├── LocalDrop.h          # Truyền tệp P2P qua mạng nội bộ LAN (LocalDrop v2)
│   ├── LocalDropProtocol.h  # Giao thức truyền nhận dữ liệu LocalDrop v2
│   ├── NetworkScanner.h     # Quét thiết bị và phân tích mạng LAN
│   ├── OuiDatabase.h        # Tra cứu thông tin nhà sản xuất card mạng (MAC OUI)
│   ├── MediaProcessor.h     # Xử lý video, audio, nén đa phương tiện & Steganography
│   ├── MediaMetadata.h      # Trích xuất và bảo toàn metadata ảnh/video
│   ├── UtilityTools.h       # Tiện ích tự động hóa chuột, bàn phím, kiểm tra pin
│   ├── ProcessRunner.h      # Quản lý tiến trình qua Job Object & timeout
│   └── UpdateManager.h      # Kiểm tra và quản lý cập nhật phiên bản
├── src/                     # Danh mục mã nguồn phân tách theo module logic
│   ├── core/                # Module hệ thống cốt lõi (SystemCore, UpdateManager)
│   ├── diskcleaner/         # Module dọn rác chuyên trách (Browser, Dev, Downloads, Deep...)
│   ├── media/               # Module xử lý media & album (MediaProcessor, MediaAlbum)
│   ├── network/             # Module mạng & P2P (Internet, LocalDrop, NetworkScanner, OuiDatabase)
│   ├── optimizer/           # Module tối ưu hệ thống & Registry (SystemOptimizer)
│   ├── tools/               # Module tiện ích tự động hóa & chẩn đoán (UtilityTools)
│   └── main.cpp             # Điểm khởi chạy (Entry point) & hệ thống Menu điều khiển
├── scripts/                 # Kịch bản hỗ trợ tải công cụ & cài đặt môi trường
│   ├── download-apps.bat    # Bộ danh mục & trình tải ứng dụng tự động (hybrid 1 file duy nhất)
│   └── setup-media.ps1      # Kịch bản PowerShell tải tự động FFmpeg & ExifTool
├── build.bat                # Kịch bản biên dịch tối ưu -O3, OpenMP, giao diện ANSI Neon
├── quy tắc.txt              # Bộ quy ước kiến trúc & tiêu chuẩn thiết kế dự án
├── README_WINDOWS_COMMANDS.md # Sổ tay tra cứu lệnh Windows CMD
└── README.md                # Tài liệu hướng dẫn kỹ thuật trung tâm của dự án
```

---

## IV. Hướng dẫn Biên dịch & Vận hành

### 1. Yêu cầu Hệ thống

- **Hệ điều hành:** Windows 10 hoặc Windows 11 (phiên bản 64-bit).
- **Bộ biên dịch C++:** GCC/G++ từ MinGW-w64 (MSYS2 UCRT64 hoặc MINGW64), hỗ trợ C++17 và OpenMP.

### 2. Biên dịch Tự động

Chỉ cần nhấp đúp chuột vào tệp script:

```cmd
build.bat
```

Đóng CMD BOX trước khi build. `build.bat` gọi `scripts/build.ps1`, giữ `-O3 -fopenmp` và liên kết tĩnh. Cache object và dependency header nằm trong `bin/.build`; chỉ biên dịch file C++ hoặc header phụ thuộc đã đổi, mặc định tối đa 2 compiler chạy song song để hạn chế RAM. Khi không có thay đổi, bỏ qua cả compile và link.

Icon dùng `assets/resource.rc` và `assets/app.ico` có sẵn, không render lại ảnh. Nếu thiếu `.rc` nhưng có `app.ico`, script tạo `.rc` một lần. Object resource được giữ trong cache; thay resource/icon mới cần chạy windres. Thiếu windres hoặc resource lỗi sẽ báo lỗi thay vì âm thầm bỏ logo.

```cmd
build.bat -NoRun             REM Build nhưng không mở ứng dụng
build.bat -Rebuild -NoRun    REM Bắt buộc build lại toàn bộ
build.bat -Jobs 4 -NoRun     REM Tối đa 4 compiler nếu máy đủ RAM
```

Đo trong phiên kiểm thử trên máy hiện tại: build cũ toàn bộ 19 file khoảng **101 giây**; luồng mới biên dịch lại toàn bộ C++ với 2 compiler khoảng **64 giây**; gọi `build.bat -NoRun` khi không có thay đổi khoảng **1,7 giây** (bao gồm khởi động PowerShell và banner). Lần build đầu vẫn cần biên dịch đầy đủ, kết quả thay đổi theo máy và file sửa.

Compiler/flags thay đổi sẽ vô hiệu cache. Source được thêm/xóa sẽ cập nhật bước link. Build ghi `bin/main.next.exe` rồi thay `bin/main.exe` khi thành công; nếu EXE đang được dùng, giữ bản mới và báo đóng CMD BOX để thử lại. Không cưỡng ép kết thúc các tiến trình main.exe. Kiểm thử cache, header, icon và build thất bại: `powershell -NoProfile -ExecutionPolicy Bypass -File tests/test-build.ps1`.

### 3. Biên dịch Thủ công qua Dòng lệnh

Nếu muốn tự biên dịch thủ công qua Command Prompt hoặc PowerShell:

```cmd
g++ -std=c++17 -O3 -fopenmp -Iinclude src\*.cpp src\core\*.cpp src\optimizer\*.cpp src\diskcleaner\*.cpp src\network\*.cpp src\tools\*.cpp src\media\*.cpp -o bin\main.exe -lbcrypt -lws2_32 -liphlpapi -lole32 -lwindowscodecs -loleaut32 -luuid -lversion -static-libgcc -static-libstdc++ -static -s
```

*Giải thích các cờ liên kết (Linker Flags):*

- Bản mặc định không bật `-mavx2 -mfma` để chạy trên CPU không có các tập lệnh này.
- `-fopenmp`: Kích hoạt xử lý đa luồng CPU song song cho tất cả các thuật toán đồ họa.
- `-lws2_32 -liphlpapi`: Giao diện mạng Windows Sockets và IP Helper API.
- `-lole32 -lwindowscodecs -loleaut32 -luuid`: Hỗ trợ giao tiếp COM và Windows Imaging Component (WIC) để giải mã ảnh và sao chép metadata.
- `-static-libgcc -static-libstdc++ -static -s`: Đóng gói tĩnh toàn bộ thư viện C++ runtime, giúp file `.exe` chạy độc lập mà không yêu cầu cài thêm bất kỳ DLL nào bên ngoài.

### 4. Tích hợp FFmpeg (Tùy chọn cho Phân hệ Media)

Để sử dụng các tính năng nén video, trích xuất âm thanh và thay đổi tốc độ:

Chạy từ thư mục dự án:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\setup-media.ps1
```

Script tải bản **release essentials ZIP** mới nhất từ [Gyan](https://www.gyan.dev/ffmpeg/builds/), kiểm tra SHA-256 trước giải nén và kiểm tra chạy `ffmpeg.exe` trước khi chép vào `bin\`. Chỉ cài `ffmpeg.exe` từ gói, không cài `ffprobe.exe`. Giấy phép và README của gói nằm trong `bin\ffmpeg-docs`. Nếu FFmpeg đã chạy được, script bỏ qua tải; dùng `-ForceFFmpeg` để tải lại/cập nhật. Không cần quyền Administrator và không thay đổi PATH.

Script cũng chuẩn bị ExifTool; xem [hướng dẫn media](docs/MEDIA.md). Code media hiện vẫn dùng ffprobe để kiểm tra stream/chapter và metadata video; các tác vụ đó cần ffprobe có sẵn cạnh `main.exe` hoặc trong PATH.

### 5. Khởi chạy Ứng dụng

- Để tất cả các tính năng can thiệp Registry, Win32 Service Manager, Tường lửa hệ thống và xóa tập tin hệ thống hoạt động chính xác, hãy nhấp chuột phải vào `bin\main.exe` và chọn **Run as Administrator** (hoặc mở Command Prompt với quyền Administrator).
