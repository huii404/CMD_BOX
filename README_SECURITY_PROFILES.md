# Setup và hồ sơ sử dụng CMD BOX

> Đã triển khai Setup và kiểm tra hồ sơ trong C++/SystemCore. Setup giúp người dùng chọn nhóm tính năng phù hợp và tránh bấm nhầm thao tác hệ thống.

## 1. Nguyên tắc

- Người dùng tự chọn một trong 4 hồ sơ: **Beginner**, **Standard**, **Developer**, **Technician**.
- Chưa Setup: vẫn mở Setup, trợ giúp, phiên bản và chẩn đoán chỉ đọc; yêu cầu Setup trước khi dùng tính năng thay đổi dữ liệu/hệ thống.
- Chạy bằng Admin vẫn tuân theo hồ sơ đang chọn. Technician được mở toàn bộ tính năng, nhưng tác vụ cần quyền Windows vẫn phải qua UAC nếu chưa đủ quyền.
- Được đổi hồ sơ ngay. Khi nâng cấp, hiện cảnh báo trách nhiệm và yêu cầu xác nhận; không chờ 24 giờ.
- Đây là giới hạn tính năng trong tool; người dùng được phép sửa hoặc xóa file cấu hình của mình. Không cần đăng nhập riêng, mã hóa cấu hình hay cơ chế khóa vai trò dành cho sản phẩm thương mại.

## 2. Ma trận tính năng

Giữ ma trận 27 tính năng đang nghiên cứu. Dấu ✓ cho phép dùng tính năng theo hồ sơ; các xác nhận và điều kiện kỹ thuật của tác vụ vẫn áp dụng.

| STT | Tính năng trong CMD_BOX | Beginner | Standard | Developer | Technician |
| :---: | :--- | :---: | :---: | :---: | :---: |
| **01** | Dọn cơ bản (Temp, Cache, cache Dev, Downloads; giữ Thùng rác) | **✓** | **✓** | **✓** | **✓** |
| **02** | Dọn dẹp chuyên sâu hệ thống & DISM | **-** | **✓** | **✓** | **✓** |
| **03** | Tối ưu ứng dụng khởi động chậm máy | **-** | **✓** | **✓** | **✓** |
| **04** | Tắt dịch vụ nền vô ích (Maps, Telemetry...) | **-** | **-** | **✓** | **✓** |
| **05** | Tinh chỉnh Giao diện & Taskbar Windows | **-** | **✓** | **✓** | **✓** |
| **06** | Tối ưu liên hoàn toàn bộ | **-** | **-** | **✓** | **✓** |
| **07** | Sửa lỗi kẹt Windows Update | **-** | **-** | **✓** | **✓** |
| **08** | **Quản lý & Tắt 40 dịch vụ Windows sâu** | **-** | **-** | **-** | **✓** |
| **09** | Sửa lỗi mạng & Reset socket Winsock / IP | **-** | **✓** | **✓** | **✓** |
| **10** | **Lá chắn an ninh & Khóa port Firewall** | **-** | **-** | **-** | **✓** |
| **11** | Kiểm tra trạng thái an ninh & Spyware Proxy | **-** | **✓** | **✓** | **✓** |
| **12** | Xem mật khẩu Wi-Fi đã lưu | **✓** | **✓** | **✓** | **✓** |
| **13** | Quét thiết bị trong mạng LAN | **✓** | **✓** | **✓** | **✓** |
| **14** | Chia sẻ file cục bộ LAN (LocalDrop P2P) | **✓** | **✓** | **✓** | **✓** |
| **15** | Tự động click chuột (Auto Click) | **✓** | **✓** | **✓** | **✓** |
| **16** | Gửi văn bản tự động (Spam Text) | **✓** | **✓** | **✓** | **✓** |
| **17** | Dán dữ liệu tự động nhiều dòng | **-** | **✓** | **✓** | **✓** |
| **18** | Tải 42 ứng dụng tự động (.bat) | **✓** | **✓** | **✓** | **✓** |
| **19** | Gỡ bỏ ứng dụng rác (Bloatware, OneDrive...) | **-** | **-** | **✓** | **✓** |
| **20** | Soi pin Laptop & Báo cáo sức khỏe ACPI | **✓** | **✓** | **✓** | **✓** |
| **21** | Nén video / hình ảnh tự động (GPU) | **✓** | **✓** | **✓** | **✓** |
| **22** | Tách âm thanh MP3 từ video | **✓** | **✓** | **✓** | **✓** |
| **23** | Đổi tốc độ phát video | **✓** | **✓** | **✓** | **✓** |
| **24** | Chuyển đổi định dạng tệp media | **✓** | **✓** | **✓** | **✓** |
| **25** | Chuẩn hóa tên file theo ngày EXIF | **✓** | **✓** | **✓** | **✓** |
| **26** | Giấu & trích xuất file trong Media (Steganography)| **-** | **✓** | **✓** | **✓** |
| **27** | Sắp xếp album ảnh/video theo Năm / Tháng | **✓** | **✓** | **✓** | **✓** |

## 3. Menu Setup

```text
Setup — Hồ sơ hiện tại: Standard

[1] Beginner      — Tiện ích ít can thiệp hệ thống
[2] Standard      — Dọn rác, mạng, media, tối ưu cơ bản
[3] Developer     — Thêm công cụ dev, gỡ ứng dụng, sửa update
[4] Technician    — Toàn bộ tính năng
[5] Xem ma trận tính năng
[6] Hiện/ẩn file cấu hình trong Explorer
[7] Xóa cấu hình và thiết lập lại
[0] Quay lại
```

Khi nâng hồ sơ, hiện thông báo ngắn:

> Hồ sơ mới mở thêm các thao tác thay đổi hệ thống. Hãy đọc xác nhận trước khi chạy. Bạn tự chọn hồ sơ và chịu trách nhiệm với thao tác của mình. Tiếp tục? (y/n)

Xác nhận rồi lưu thành công thì áp dụng ngay; hạ hồ sơ không cần cảnh báo nâng cấp. Nếu tính năng bị giới hạn, báo tên hồ sơ hiện tại và hướng dẫn vào Setup. Không chuyển hồ sơ trong lúc một tác vụ sửa hệ thống đang chạy.

Setup dùng cùng khung màu với menu chính, có mô tả phạm vi, đánh dấu hồ sơ đang dùng và trạng thái hiện/ẩn cấu hình. Bảng 27 tính năng căn cột và tô sáng cột hồ sơ hiện tại. Chọn lại hồ sơ đang dùng chỉ hiển thị thông tin, không ghi lại file.

## 4. File cấu hình đơn giản

**Chọn JSON**, tên `cmd_box.json`, đặt cạnh `main.exe` để dùng portable. Lấy đường dẫn theo thư mục executable, không theo thư mục hiện tại của terminal. Mỗi thư mục tool dùng một cấu hình chung; các tài khoản Windows mở cùng bản portable sẽ dùng cùng hồ sơ.

```json
{
  "version": 1,
  "tier": 2,
  "hide_config": true
}
```

| Trường | Ý nghĩa |
| --- | --- |
| `version` | Phiên bản định dạng, hiện là 1 |
| `tier` | 1: Beginner; 2: Standard; 3: Developer; 4: Technician |
| `hide_config` | `false`: hiện file; `true`: đặt thuộc tính Hidden |

- JSON là văn bản UTF-8, có thể mở/sửa bằng trình soạn thảo. Không lưu mật khẩu hoặc dữ liệu nhạy cảm trong file này.
- Chỉ hỗ trợ một định dạng JSON để giữ code gọn; không cần thêm parser TXT/INI song song.
- Cấu hình mới mặc định ẩn trong Explorer để giảm nguy cơ xóa nhầm. Khi mở ứng dụng, áp dụng lại thuộc tính theo `hide_config`; lựa chọn Hiện (`false`) vẫn được giữ. Xóa cấu hình rồi Setup lại cũng dùng mặc định ẩn. Tùy chọn ẩn chỉ đặt/gỡ thuộc tính **Hidden**, không đặt System hay Read-only. Explorer vẫn có thể hiện file khi bật **Hidden items**; đây là tùy chọn hiển thị, không phải chống sửa.
- Nếu lưu JSON thành công nhưng đổi thuộc tính Hidden thất bại, giữ hồ sơ đã lưu và báo lỗi hiển thị; không báo mất cấu hình.

## 5. Logic gọn trong SystemCore

Các chức năng đã triển khai: nạp cấu hình, lưu hồ sơ, kiểm tra tính năng được phép và đổi thuộc tính Hidden. Dùng bảng quyền cố định trong chương trình; cấu hình chỉ chứa hồ sơ và tùy chọn hiển thị.

1. Nạp JSON: kiểm tra `version = 1`, `tier` là số nguyên từ 1–4 và `hide_config` là boolean. File thiếu/hỏng/giá trị sai đưa về **Chưa thiết lập**, cho người dùng chọn lại.
2. Menu và CLI kiểm tra cùng bảng quyền trước khi chạy tính năng. Không chỉ ẩn mục menu.
3. Lưu file tạm cùng thư mục rồi thay thế; chỉ đổi hồ sơ trong RAM sau khi lưu thành công. Nếu thư mục không ghi được, báo lỗi và giữ hồ sơ cũ.
4. Xóa cấu hình đưa về Chưa thiết lập. Không tự mở toàn bộ tính năng khi file mất/hỏng.
5. Combo nhiều bước kiểm tra quyền từng bước. Hồ sơ cao vẫn giữ xác nhận và cơ chế bảo vệ dữ liệu của từng tác vụ.

**Chọn C++ trong SystemCore** cho phần Setup. Các `.bat` sửa lỗi vẫn là backend tác vụ hiện có, không cần tự đọc hồ sơ. Không bổ sung cooldown, chữ ký cấu hình, hệ thống policy hoặc nhật ký đổi vai trò riêng cho thiết kế nhỏ này.

Quyền Windows và hồ sơ là hai kiểm tra riêng; xem [Microsoft: UAC](https://learn.microsoft.com/en-us/windows/security/application-security/application-control/user-account-control/how-it-works). Chi tiết logic sửa lỗi vẫn nằm trong [README_FIX_RESEARCH.md](README_FIX_RESEARCH.md).

### Cách thêm tính năng hoặc đổi giới hạn cấp độ

Bốn cấp độ được đặt tên trong `ProfileLevel` (`include/SecurityProfiles.h`): **Beginner = 1**, **Standard = 2**, **Developer = 3**, **Technician = 4**. Cấp cao dùng được các tính năng của cấp thấp. Tên cột ngắn trong ma trận là BEGIN / STD / DEV / TECH.

Bảng `FEATURE_RULES` trong `src/core/SecurityProfiles.cpp` là nơi khai báo quyền. Mỗi dòng chứa tên tính năng trong code, tên hiển thị, cấp tối thiểu và tùy chọn dùng trước Setup. Ví dụ:

```cpp
{Feature::DeepClean, "Dọn sâu & DISM", Level::Standard},
```

Dòng này cho phép Standard, Developer và Technician; Beginner bị chặn. Để đổi giới hạn, sửa `Level::Standard` ngay trên dòng này. Cột cuối `true` chỉ dành cho chẩn đoán được phép trước Setup; khi đã chọn hồ sơ vẫn áp dụng cấp tối thiểu.

Khi thêm tính năng:

1. Thêm một tên vào `enum class Feature`, trước `Count`.
2. Thêm một dòng tương ứng vào `FEATURE_RULES`. Compiler sẽ báo lỗi nếu thiếu/trùng tính năng hoặc cấp tối thiểu sai.
3. Gọi `SystemCore::requireFeature(Feature::TênMới)` trước backend, cho cả đường menu/CLI và từng bước combo.
4. Cập nhật ma trận tài liệu và các dòng `ExpectedPermission` trong sandbox test.

UI lấy tên hồ sơ qua `tierName()`; đổi tên tiếng Anh không đổi số `tier` trong JSON và không cần chuyển đổi cấu hình cũ.

## 6. Kiểm tra tối thiểu khi triển khai

| Trường hợp | Kết quả mong đợi |
| --- | --- |
| Thiếu/hỏng JSON hoặc tier sai | Yêu cầu Setup; không tự mở quyền |
| Admin chọn Beginner | Vẫn giới hạn tính năng theo Beginner |
| CLI gọi tính năng bị giới hạn | Bị chặn như menu |
| Nâng/hạ hồ sơ, lưu thành công | Áp dụng ngay theo luồng xác nhận |
| Lưu thất bại | Giữ hồ sơ cũ và báo lỗi |
| Chọn hiện/ẩn | Cập nhật tùy chọn và thuộc tính Hidden; báo nếu đặt thuộc tính thất bại |
| Xóa cấu hình | Trở về Chưa thiết lập |

Kiểm thử bằng `tests/security_profiles_test.cpp` và `tests/test-security-profiles.ps1`. Chạy từ thư mục dự án:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/test-security-profiles.ps1
```
Script biên dịch và chạy executable riêng trong `tests/sandbox`, cấu hình cũng chỉ nằm trong sandbox, không dùng `bin/cmd_box.json`. Bài kiểm thử C++ chỉ gọi cấu hình/bảng quyền, không liên kết các backend sửa hệ thống. Bài kiểm thử CLI gọi lệnh bị chặn, help/version, hủy tối ưu trước khi thực thi và kiểm tra Setup (hạ hồ sơ, từ chối/xác nhận nâng, ẩn và xóa cấu hình). Sandbox này cô lập executable/cấu hình kiểm thử, không phải máy ảo Windows.

Kết quả ngày 09/10/2026: biên dịch ứng dụng thành công; 108 ô quyền và các trường hợp cấu hình, CLI, Setup nêu trên đều PASS. Không chạy backend thay đổi hệ thống.

Quy ước bổ sung:

- Cache Dev được giới hạn ở cache công cụ, không quét project; Downloads dùng logic an toàn hiện có. Hai nhóm này thuộc tính năng 01 và được phép với Beginner.
- Thùng rác được giữ lại ở mọi hồ sơ, không bổ sung tác vụ xóa Thùng rác.
- Chưa thiết lập được xem trạng thái an ninh và chẩn đoán pin. Khi đã thiết lập, tính năng an ninh áp dụng ma trận (từ Standard).
- Biên dịch lại ứng dụng yêu cầu đã Setup, cùng mức tối thiểu với tải ứng dụng. Help/version và xem Setup luôn được phép.
- `main.exe setup` mở Setup; CLI trả mã 3 khi hồ sơ từ chối tính năng, mã 2 khi lệnh không hợp lệ.
- Các tác vụ chạy đồng bộ trong menu nên không đổi hồ sơ khi tác vụ còn chạy. Bảng quyền được dùng tại entry point backend và CLI; combo tối ưu và dọn dẹp kiểm tra từng nhóm.
- Cấu hình được nạp lúc khởi động; sửa file bên ngoài có hiệu lực ở lần khởi động tiếp theo.

## 7. Nhật ký thiết kế

| Lần | Ngày | Nội dung | Kết quả |
| --- | --- | --- | --- |
| PROFILE-R001 | 08/10/2026 | Nghiên cứu hồ sơ, UAC, cấu hình và luồng sửa lỗi | Bản phân tích ban đầu; phần thiết kế phức tạp được thay bằng bản gọn ở R002 |
| PROFILE-R002 | 08/10/2026 | Đơn giản hóa theo yêu cầu người dùng | JSON portable, đổi ngay sau cảnh báo, hiện/ẩn tùy chọn; chỉ sửa README, chưa code |
| PROFILE-R003 | 09/10/2026 | Triển khai Setup, JSON, chốt quyền menu/CLI/backend; bổ sung phạm vi dọn và sandbox test | Xem kiểm thử và giới hạn ở mục 6 |
| PROFILE-R004 | 09/10/2026 | Làm đẹp Setup, bảng quyền và màn hình xác nhận/kết quả; tránh ghi lại hồ sơ đang dùng; kiểm tra độ rộng khung trong sandbox | Chạy lại bộ kiểm thử quyền, cấu hình và UI |
| PROFILE-R005 | 11/10/2026 | Tách mỗi tính năng thành một dòng, hợp nhất tên và quyền vào FEATURE_RULES, thêm hướng dẫn mở rộng; đổi 4 cấp sang tiếng Anh | Giữ nguyên tier 1–4 và quyền hiện có |
| PROFILE-R006 | 11/10/2026 | Mặc định ẩn cấu hình mới, khôi phục thuộc tính Hidden khi nạp JSON, giữ tùy chọn Hiện; kiểm thử trong sandbox | Hidden chỉ giảm xóa nhầm, không khóa file |
