# CƠ CHẾ DỌN RÁC & LÀM SẠCH HỆ THỐNG WINDOWS (WINDOWS DISK CLEANER)

> **Tài liệu Kỹ thuật Phân hệ Dọn đĩa & Tối ưu Ổ cứng**  
> **Module Chuyên trách:** `DiskCleaner` (`include/DiskCleaner.h` & `src/optimizer/DiskCleaner.cpp`)  
> **Nền tảng:** Windows 10 / 11 (x64) | C++17 Native | Win32 API

---

## I. Tổng quan Kiến trúc

Trong quá trình vận hành hệ điều hành Windows, dung lượng ổ đĩa (đặc biệt là phân vùng hệ thống `C:\`) liên tục bị hao hụt bởi nhiều nguồn khác nhau:
1. Tệp tin tạm thời thời gian thực sinh ra từ các tiến trình đang chạy.
2. Dữ liệu đệm đa tầng của các trình duyệt web và phần mềm chat.
3. Gói cập nhật Windows tích tụ sau mỗi bản vá (`WinSxS`, `$WINDOWS.~BT`, `Windows.old`).
4. Thư viện và gói bộ đệm trong môi trường phát triển phần mềm (Node.js, Python, Gradle, Rust, Go).
5. **Thói quen người dùng:** Tải về các bộ cài đặt ứng dụng (`.exe`, `.msi`) trong thư mục `Downloads`, cài đặt xong nhưng không bao giờ xóa tệp gốc, hoặc bấm tải lặp đi lặp lại tạo ra nhiều bản sao `app (1).exe`, `app (2).exe`.

Phân hệ **`DiskCleaner`** được thiết kế độc lập, tách biệt hoàn toàn khỏi logic điều phối Registry/Service của `SystemOptimizer` nhằm tuân thủ nguyên lý *Single Responsibility Principle (SRP)*, giúp mã nguồn tường minh và dễ bảo trì.

---

## II. Các Tầng Dọn Rác Chi Tiết (5 Nhiệm Vụ)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       BỘ ĐIỀU PHỐI DỌN RÁC (DISKCLEANER)                     │
├─────────────┬─────────────┬─────────────┬─────────────┬─────────────────────┤
│  Nhiệm vụ 1 │  Nhiệm vụ 2 │  Nhiệm vụ 3 │  Nhiệm vụ 4 │      Nhiệm vụ 5     │
│  Rác Bề Mặt │  Trình Duyệt│  Chuyên Sâu │   Môi Trường│   Downloads Thông   │
│   & Temp    │ & Ứng Dụng  │  Hệ Thống   │     Dev     │   Minh (Exe / Msi)  │
└─────────────┴─────────────┴─────────────┴─────────────┴─────────────────────┘
```

### 1. Nhiệm vụ 1: Dọn rác bề mặt & Cache người dùng
- **Mục tiêu:** Quét sạch các thư mục tạm tức thời do Windows và người dùng kích hoạt.
- **Các vị trí xử lý:**
  - `%TEMP%` và `%SystemRoot%\Temp`: Tệp tạm của người dùng và dịch vụ hệ thống.
  - `%AppData%\Microsoft\Windows\Recent`: Danh sách tệp mở gần đây.
  - `%LocalAppData%\D3DSCache`: Bộ đệm Direct3D Shader của DirectX.
  - `%LocalAppData%\Low\Microsoft\CryptnetUrlCache`: Bộ đệm xác thực chứng chỉ số SSL/TLS.
  - `%LocalAppData%\CrashDumps`: Bản ghi bộ nhớ khi ứng dụng của bên thứ ba bị sập.
  - `%LocalAppData%\Microsoft\Windows\WER`: Báo cáo lỗi cá nhân (`ReportArchive`, `ReportQueue`, `Temp`).
  - `%ProgramData%\Microsoft\Windows\WER\Temp`: Hàng đợi báo cáo lỗi toàn hệ thống.
  - `%LocalAppData%\Microsoft\Windows\INetCache`: Tệp đệm web ngầm và WebView2.
  - Xóa sạch Thùng rác (`Clear-RecycleBin`) & làm mới DNS (`ipconfig /flushdns`).
- **Thực thi song song:** Sử dụng `std::thread` chạy đồng thời nhiều tác vụ dọn dẹp để giảm thời gian chờ của người dùng xuống mức tối thiểu.

---

### 2. Nhiệm vụ 2: Dọn rác Trình duyệt & Ứng dụng
- **Mục tiêu:** Thu hồi dung lượng khổng lồ tích tụ từ bộ đệm mạng và kết xuất đồ họa.
- **Hỗ trợ đa cấu hình (Multi-Profile Support):**
  - Quét duyệt đệ quy toàn bộ thư mục User Data của các trình duyệt Chromium: Google Chrome, Microsoft Edge, Cốc Cốc, Brave Browser, Vivaldi, Opera, Opera GX.
  - Nhận diện chính xác từng profile: `Default`, `Profile 1`, `Profile 2`, `Guest Profile`, `System Profile`.
  - Làm sạch các thư mục đệm con: `Cache`, `Code Cache`, `GPUCache`, `DawnCache`, `ShaderCache`, `GrShaderCache`, `GraphiteDawnCache`, `Service Worker/CacheStorage`.
  - Hỗ trợ Mozilla Firefox: Duyệt toàn bộ cấu hình `*.default-release` trong cả `%AppData%` và `%LocalAppData%` (`cache2`, `startupCache`, `jumpListCache`).
- **Ứng dụng giao tiếp & Đồ họa:**
  - Discord Cache & Code Cache (`%AppData%\discord`).
  - Telegram Desktop Cache (`%AppData%\Telegram Desktop\tdata\user_data\cache`).
  - NVIDIA Shader Cache (`%LocalAppData%\NVIDIA\GLCache`).
  - File cache ảnh thu nhỏ Windows Explorer (`thumbcache_*.db`).
- **Cam kết an toàn:** Tuyệt đối không can thiệp vào tệp Cookies, Mật khẩu đã lưu hay Lịch sử duyệt web của người dùng.

---

### 3. Nhiệm vụ 3: Dọn dẹp Chuyên sâu Hệ thống & Tồn dư Cập nhật
- **Mục tiêu:** Giải phóng hàng chục Gigabyte tồn dư sau khi nâng cấp Windows hoặc cập nhật bản vá.
- **Quyền hạn:** Chạy dưới quyền Administrator thông qua Batch Script bảo mật.
- **Các thành phần xử lý:**
  - Tồn dư phiên bản Windows cũ: Lấy quyền sở hữu (`takeown`), gán quyền ghi (`icacls`) và xóa sạch `$WINDOWS.~BT`, `$WINDOWS.~WS`, `Windows.old`.
  - Kỹ thuật dọn siêu tốc Robocopy: Dùng `robocopy /mir` đồng bộ thư mục rỗng để xóa nhanh chóng hàng trăm nghìn file nhỏ trong `%SystemRoot%\Temp` và `Prefetch`.
  - Dọn dẹp kho lưu trữ gói cài đặt chuẩn Windows DISM:
    ```cmd
    dism /online /cleanup-image /startcomponentcleanup /resetbase
    ```
    Loại bỏ hoàn toàn các phiên bản cũ của các bản cập nhật đã được thay thế trong thư mục `WinSxS`.
  - Giải phóng bộ đệm phân phối bản vá `Delivery Optimization`:
    ```powershell
    Get-DeliveryOptimizationStatus | Remove-DeliveryOptimizationCache -Confirm:$false
    ```
  - Xóa kho tải bản cập nhật dở dang trong `%SystemRoot%\SoftwareDistribution\Download`.
  - Làm sạch nhật ký bảo trì CBS Logs (`%SystemRoot%\Logs\CBS`), `Panther` (nhật ký cài đặt hệ điều hành), `LiveKernelReports`, và xóa sạch toàn bộ các bản ghi sự kiện Windows cũ (`wevtutil cl`).
  - Tắt tệp ngủ đông `hiberfil.sys` (`powercfg -h off`) nếu không sử dụng chế độ Hibernate, ngay lập tức thu hồi dung lượng tương đương 40% - 100% dung lượng RAM thực tế của máy.

---

### 4. Nhiệm vụ 4: Dọn rác Môi trường Lập trình (Dev Caches & Artifacts)
- **Mục tiêu:** Dành riêng cho nhà phát triển, giải phóng các vùng đệm mã nguồn và thư viện trung gian.
- **Quét đĩa tự động:** Tự động phát hiện toàn bộ các ổ đĩa cố định trên hệ thống (`C:`, `D:`, `E:`, `G:...`) và quét các thư mục làm việc phổ biến (`Code`, `Projects`, `Source`, `Repos`, `Workspace`).
- **Bảo vệ tuyệt đối kho lưu trữ Git:** Luôn luôn bỏ qua và bảo vệ 100% các thư mục `.git`, `.github`, file cấu hình `.gitignore`.
- **Thư mục và thư viện được làm sạch:**
  - **Node.js / Web:** `npm-cache`, `Yarn\Cache`, `pnpm\store`, `deno\deps`, `.turbo`, `.next`, `.nuxt`, `.parcel-cache`, và các thư mục `node_modules` sinh ra trong các dự án.
  - **Python:** Pip cache, `__pycache__`, `.pytest_cache`, `.mypy_cache`, `.ruff_cache`, tệp nhị phân `.pyc`, `.pyo`.
  - **Java / Android:** `.gradle\caches`, `.gradle\daemon`, `.android\cache`, `.m2\repository`.
  - **C# / .NET:** NuGet v3-cache, `.nuget\packages`.
  - **Rust / Go:** Cargo registry cache/src, `.rustup\downloads`, `go-build`.
  - **IDE & Trình soạn thảo:** Bộ nhớ đệm làm việc nặng nề của VS Code (`Code\Cache`, `CachedExtensionVSIXs`, `workspaceStorage`) và Cursor IDE.

---

### 5. Nhiệm vụ 5: Quản lý Thông minh Thư mục Tải về (Smart Downloads Cleanup)

Đây là phân hệ dọn dẹp hoàn toàn mới, được xây dựng dựa trên hành vi tải file thực tế của người dùng Windows.

```
                   THƯ MỤC DOWNLOADS
                           │
         ┌─────────────────┴─────────────────┐
         ▼                                   ▼
  Tệp .EXE / .MSI                   Tệp tải dở dang
         │                        (.crdownload, .part, .tmp)
         ├────────────────────────┐          │
         ▼                        ▼          ▼
  Kiểm tra Trùng lặp       Kiểm tra Cài đặt  Kiểm tra Thời gian
   app.exe vs app (1).exe    trong Registry   (Tồn tại > 24 giờ)
         │                        │          │
         ▼                        ▼          ▼
  Giữ bản gốc,             Đã cài đặt: Xóa   Xóa vĩnh viễn
  Xóa bản trùng (1),(2)    Chưa cài: Giữ lại
```

#### A. Quyết định Thiết kế: KHÔNG sử dụng Thùng Rác (Recycle Bin)
- **Lý do:** Mục tiêu cốt lõi của công cụ dọn rác hệ thống là **thu hồi dung lượng lưu trữ thực tế trên ổ đĩa**.
- Nếu xóa file chuyển vào Thùng rác (thông qua Win32 `SHFileOperation` với cờ `FOF_ALLOWUNDO`), dữ liệu vật lý vẫn nằm nguyên trên phân vùng đĩa cứng, chiếm nguyên dung lượng cung cấp. Người dùng sẽ thấy dung lượng ổ đĩa khả dụng không thay đổi và lầm tưởng chức năng bị lỗi.
- Do đó, `DiskCleaner` áp dụng cơ chế xóa vật lý dứt điểm (`std::filesystem::remove`) kết hợp với các rào chắn điều kiện lọc an toàn tuyệt đối.

#### B. Phạm vi Áp dụng: CHỈ áp dụng cho tệp Bộ Cài Đặt (`.exe` và `.msi`)
- **Tại sao không áp dụng cho tài liệu và ảnh?**
  - Người dùng thường tải về các tài liệu, ảnh, tệp PDF từ email, Facebook, Zalo với các tên gọi như `bao_cao (1).docx`, `IMG_0001 (1).jpg`. Các tệp này thường chứa các phiên bản chỉnh sửa khác nhau hoặc các bức ảnh chụp riêng biệt. Tự động xóa các tệp này dựa trên tên gọi có nguy cơ rất cao làm mất tài liệu cá nhân của người dùng.
  - Ngược lại, các file `.exe` và `.msi` trong Downloads hầu như 100% chỉ là các gói cài đặt phần mềm dùng một lần. Sau khi đã cài xong, chúng trở thành rác chiếm dung lượng vô ích (thường từ hàng chục MB đến vài GB mỗi file).

#### C. Cơ chế 1: Khử Trùng lặp Bộ Cài Đặt (Duplicate Installer Pruning)
- Windows có cơ chế tự động đánh số khi tải file trùng tên: `setup.exe` -> `setup (1).exe` -> `setup (2).exe`.
- Thuật toán `DiskCleaner` sử dụng biểu thức chính quy (Regex):
  ```regex
  ^(.*?)\s*\(\d+\)$
  ```
  để bóc tách tên gốc. Khi phát hiện bộ tệp trùng lặp:
  - Giữ lại bản gốc không có số đuôi (`setup.exe`).
  - Xóa bỏ các bản tải thừa (`setup (1).exe`, `setup (2).exe`...).

#### D. Cơ chế 2: Quét Registry Đối Chiếu Phần Mềm Đã Cài Đặt
Để biết một file `.exe` đã được cài đặt vào máy hay chưa, `DiskCleaner` thực hiện quy trình 3 bước:
1. **Lập bản đồ toàn bộ phần mềm đã cài đặt trên Windows:**
   Truy vấn đệ quy các khóa Registry quản lý gỡ cài đặt của hệ điều hành:
   - `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall` (Ứng dụng 64-bit toàn hệ thống).
   - `HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall` (Ứng dụng 32-bit trên nền Windows 64-bit).
   - `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall` (Ứng dụng cài đặt cho từng người dùng, ví dụ: VS Code User, Telegram, Discord).
   Trích xuất tất cả các giá trị chuỗi `DisplayName` và chuẩn hóa về dạng chữ thường không dấu cách (lowercase normalized tokens).
2. **Trích xuất Siêu dữ liệu Tệp Cài Đặt:**
   - Đọc tên file gốc (ví dụ: `Git-2.43.0-64-bit.exe` -> tách từ khóa `git`).
   - Nạp động thư viện hệ thống `version.dll` thông qua `LoadLibraryA("version.dll")` và các hàm API `GetFileVersionInfoA`, `VerQueryValueA` để đọc trực tiếp bảng tài nguyên Version Resource (`FileDescription`, `ProductName`) nằm trong PE Header của file `.exe`.
3. **Đối chiếu & Quyết định:**
   - Nếu tên hoặc mô tả của file `.exe` khớp với bất kỳ phần mềm nào đang có mặt trong danh sách cài đặt của Registry: Xác nhận **Đã cài đặt** -> Tiến hành xóa bộ cài đặt trong Downloads để thu hồi bộ nhớ.
   - Nếu không tìm thấy dấu vết trong Registry: Xác nhận **Chưa cài đặt** (người dùng vừa tải về chuẩn bị dùng hoặc phần mềm dạng portable) -> **Giữ nguyên 100%, tuyệt đối không xóa**.

#### E. Cơ chế 3: Làm sạch tệp tải dở dang (Broken/Incomplete Downloads)
- Tự động phát hiện các file tải dang dở bị kẹt lại do rớt mạng hoặc đóng trình duyệt đột ngột: đuôi `.crdownload` (Chrome, Edge, Cốc Cốc), `.part` (Firefox), và `.tmp`.
- Điều kiện an toàn: Chỉ xóa nếu thời gian chỉnh sửa cuối cùng (`last_write_time`) của tệp đã **trên 24 giờ**, bảo đảm không can thiệp vào các tệp mà người dùng đang thực sự tải ở phiên làm việc hiện tại.

---

## III. Cấu trúc Tệp & Tổ chức Mã nguồn

```
CMD/
├── include/
│   ├── DiskCleaner.h           # Khai báo lớp chuyên trách dọn đĩa DiskCleaner
│   ├── SystemOptimizer.h       # Khai báo SystemOptimizer (ủy quyền gọi DiskCleaner)
│   └── SystemCore.h            # Tiện ích Win32, JobObject, màu sắc console
├── src/
│   ├── optimizer/
│   │   ├── DiskCleaner.cpp     # Triển khai 5 nhiệm vụ dọn rác, Win32 Registry, Version API
│   │   └── SystemOptimizer.cpp # Tối ưu hóa hiệu năng, khởi động, dịch vụ ngầm (đã tinh gọn)
│   └── main.cpp                # Giao diện menu điều khiển phân cấp trực quan
└── README_WINDOWS_CLEANER.md   # Tài liệu kỹ thuật chi tiết
```

---

## IV. Bảng Tổng Hợp Tác Vụ Dọn Rác Trên Menu

Khi người dùng khởi chạy ứng dụng và chọn menu **[1] Dọn rác & Tối ưu Hệ thống**:

| Lựa chọn | Tác vụ | Nội dung thực hiện |
| :--- | :--- | :--- |
| **[1]** | **Dọn rác bề mặt** | `%TEMP%`, CrashDumps, WER, D3DSCache, INetCache, Flush DNS, Recycle Bin |
| **[2]** | **Dọn rác Trình duyệt & App** | Chrome, Edge, Brave, Cốc Cốc, Firefox, Discord, Telegram, NVIDIA Cache |
| **[3]** | **Dọn chuyên sâu & Cập nhật** | DISM WinSxS ResetBase, $WINDOWS.~BT, Windows.old, Logs CBS, EventLogs |
| **[4]** | **Dọn rác Dev** | node_modules, pip cache, gradle, cargo, nuget, go-build, VS Code cache |
| **[5]** | **Dọn tệp cài đặt Downloads** | Xóa file `.exe`/`.msi` đã cài đặt, xóa bản trùng lặp `(1)`, xóa tệp dở dang >24h |
| **[6]** | **[⚡] Dọn toàn diện Hệ thống** | Chạy liên hoàn **Mục 1 + 2 + 3 + 5** (Tất cả ngoại trừ rác Dev của lập trình viên) |
| **[7]** | **[🚀] Dọn tất cả** | Chạy toàn bộ cả **5 mục** (Bao gồm cả rác Dev) |

---

## V. Đảm bảo An Toàn & Tương Thích Ngược
1. **Khả năng tương thích ngược 100%:** Lớp `SystemOptimizer` chứa sẵn các hàm ủy quyền `inline` trỏ sang `cleaner.*`, giúp các module khác nếu có gọi đến phương thức cũ đều biên dịch và hoạt động chính xác mà không cần sửa đổi.
2. **Không phụ thuộc thư viện liên kết ngoài:** `version.dll` được nạp động trong runtime bằng `LoadLibraryA` / `GetProcAddress`, không phát sinh thêm cờ liên kết (`-lversion`) trong tệp kịch bản biên dịch `build.bat`.
3. **Bỏ qua file đang bận (Non-blocking In-use Skips):** Tất cả các thao tác xóa đều được bao bọc trong cấu trúc `try / catch` và kiểm tra mã lỗi `std::error_code`, tệp tin đang bị khóa bởi tiến trình khác sẽ được bỏ qua nhẹ nhàng mà không gây lỗi hoặc treo chương trình.
