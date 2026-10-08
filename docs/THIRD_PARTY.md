# Thư viện bên thứ ba của media

- `include/third_party/json.hpp`: nlohmann/json v3.12.0, header từ repo chính thức, MIT. Dùng để đọc JSON của ExifTool/ffprobe trong RAM và cấu trúc footer nội bộ; không ghi sidecar JSON, không có DLL JSON đi kèm. Giấy phép tại `include/third_party/json.LICENSE`.
- ExifTool 13.59: chương trình riêng dùng để đọc/ghi metadata. Source runtime được tải từ tag chính thức vào `bin` (không nằm trong Git); license GPL-3.0 của source bundle được giữ tại `docs/ExifTool_LICENSE`. Xem README và điều khoản trong gói khi phân phối.
- Perl/MSYS, FFmpeg và ffprobe là runtime bên ngoài, không liên kết với header JSON. Khi phân phối phải giữ các thông báo và giấy phép tương ứng với build của từng công cụ.
- `scripts/setup-media.ps1` tải FFmpeg bản release essentials từ Gyan, chỉ cài `ffmpeg.exe`, kiểm tra SHA-256 và giữ giấy phép GPLv3 cùng README của build tại `bin/ffmpeg-docs`. Khi đóng gói, giữ cả thư mục này và tuân thủ điều khoản cung cấp source của bản build. Nguồn và thông tin build: https://www.gyan.dev/ffmpeg/builds/.

Nguồn: https://github.com/nlohmann/json/tree/v3.12.0 và https://github.com/exiftool/exiftool/tree/13.59.
