# Media: metadata và album theo thời gian

## Phụ thuộc

Media sử dụng FFmpeg, ffprobe và ExifTool. Công cụ được tìm cạnh `main.exe`, trong `bin` của checkout, rồi trong PATH. Thiếu backend metadata thì dừng trước render; không báo thành công khi chưa đọc được thông tin nguồn.

Bản Windows portable: đặt `exiftool.exe` (đổi tên từ `exiftool(-k).exe`) và thư mục `exiftool_files` cùng cấp trong `bin`. Lấy bản chính thức tại [ExifTool](https://exiftool.org/). Khi phân phối ứng dụng, cung cấp cả FFmpeg, ffprobe và runtime ExifTool, kèm giấy phép của chúng. Không chỉ gửi `main.exe`.

Checkout này cũng hỗ trợ source ExifTool 13.59 với Perl đã có trong PATH hoặc MSYS2. Source nằm trong `bin/exiftool-source/exiftool-13.59`; không cài vào hệ thống. Có thể chuẩn bị lại bằng:

```powershell
./scripts/setup-media.ps1
```

Script dùng tag 13.59 của [repo tác giả](https://github.com/exiftool/exiftool), kiểm tra SHA-256 archive trước giải nén. SHA-256 của archive đã kiểm chứng trong phiên này: `542315bbb4b302b5334b6defd608d9ce2c97299c5608ab83a9e6c232abd56f18`.

## Bảo tồn metadata khi xuất

`MediaMetadata.h` điều phối backend ExifTool, snapshot timestamp, kiểm tra metadata và ghi JSON. Đã bỏ parser/chèn JPEG APP1/PNG chunk tự viết gây lặp block, tràn độ dài và ghi trực tiếp vào file đầu ra.

Các luồng nén, đổi định dạng, đổi tốc độ và trích MP3:

1. Đọc timestamp **trước khi** đọc metadata và render.
2. Snapshot metadata nguồn, kể cả raw EXIF, ICC/MakerNotes được ExifTool đọc và thông tin ffprobe khi có.
3. Render trong thư mục tạm riêng, giữ nguồn.
4. Với ảnh: `-noautorotate` giữ hướng pixel nguồn; ExifTool chép EXIF/XMP/ICC phù hợp, giữ Orientation và cập nhật kích thước EXIF theo ảnh đích. Thumbnail/preview nguồn không chép vào bản render.
5. Đọc lại metadata đầu ra, đối chiếu các trường kỷ niệm và profile màu; mọi trường không chuyển được được ghi cảnh báo.
6. Ghi `filename.ext.metadata.json` chứa metadata nguồn, metadata kết quả, timestamp nguồn và thông tin bị thiếu. Bản ghi metadata thuộc cặp bản xuất; nên sao lưu cả hai cùng nguồn gốc.
7. Khóa và đổi tên file cùng metadata đi kèm bằng handle, không ghi đè. Nếu lưu cặp file hoặc đồng bộ timestamp thất bại, hủy các file do lần chạy đó tạo.

Không khẳng định mọi tag nhúng giữ nguyên khi chuyển giữa các định dạng. JSON lưu bản export mà backend đọc được; file nguồn giữ nguyên vẫn là bản lưu trữ đầy đủ nhất. Tag kỹ thuật như kích thước, thời lượng hoặc encoder phải phản ánh bản xuất.

### Video và chất lượng

- Bỏ mapping nhầm metadata cấp file vào track. Metadata stream đi theo stream được map.
- Chuyển container thử `-map 0 -c copy` trước, không đoán tương thích từ đuôi MOV/MKV. Các stream và chương được giữ nếu muxer hỗ trợ.
- Khi buộc render, giữ tất cả audio và subtitle được hỗ trợ. Không âm thầm bỏ nhiều video, telemetry hoặc attachment: từ chối render nguồn phức tạp và giữ nguồn để remux.
- Đổi tốc độ dùng độ chính xác cao cho video/audio và chia mốc chapter cho hệ số tốc độ.
- PNG có thể giữ 16-bit khi chuyển TIFF 16-bit; JPEG/WebP 8-bit có thông báo trước render. Nén tự động nguồn ảnh trên 8-bit chọn PNG, không ép xuống JPEG. Ảnh float không bị tự giảm độ chính xác.
- HDR hoặc video trên 8-bit chỉ remux. Nén/đổi tốc độ chưa render HDR/10-bit; từ chối để tránh giảm bit-depth/dải màu mà không có sự lựa chọn.
- ICC không phải RGB (ví dụ CMYK/Gray) không gắn vào kết quả RGB; profile nguồn lưu trong JSON, có cảnh báo. Chưa triển khai chuyển đổi ICC CMYK bằng engine quản lý màu.
- Trích MP3 lấy track audio đầu tiên; nguồn còn nguyên, thông tin các track khác nằm trong snapshot/ffprobe. Đây không phải tính năng gộp mọi track audio.
- Báo dung lượng tiết kiệm dùng kích thước file cuối sau ghi metadata, không dùng số đo trước ghi.

### File ẩn

Footer mới `CBOXHID2` lưu kích thước cover/payload/JSON, tên và timestamp gốc của payload. Cover và payload được sao chép nguyên byte (payload XOR như trước), nên metadata nhúng vẫn còn. Trích xuất ghi JSON và khôi phục timestamp của payload. Footer `HIDE` cũ vẫn đọc được; không thể khôi phục tên/timestamp không được lưu trong định dạng cũ. XOR không phải mã hóa bảo mật.

## Sắp album từ THƯ MỤC

Menu **Media → [7] Sắp album từ thư mục**:

- Chọn thư mục, quét media trong các thư mục con; bỏ junction/symlink và `CMD_BOX_Output`/`CMD_BOX_Album` để không quét lặp.
- Ưu tiên `SubSecDateTimeOriginal`, `DateTimeOriginal`, `DateCreated`, `CreationDate`, `CreateDate`, `MediaCreateDate`.
- Nếu không có ngày chụp/quay hợp lệ, dùng ngày sửa file và ghi rõ trong preview/JSON; không coi đó là ngày chụp đã được xác minh.
- Xem trước rồi xác nhận; chỉ sao chép, không di chuyển hoặc đổi tên nguồn.
- Đầu ra: `nguồn/CMD_BOX_Album/YYYY/MM/<đường dẫn tương đối gốc>`. Giữ các thư mục tương đối để hai nguồn cùng tên không đè nhau.
- File cùng tên đã có được bỏ qua và thông báo. Giữ nguyên metadata nhúng bằng sao chép nguyên byte, cùng timestamp và JSON.
- Giới hạn mỗi lần quét 10.000 file. Album hiện không tự nhóm theo sự kiện hoặc vị trí.

## Kiểm thử

`tests/media_integrity.cpp` tạo fixture riêng trong thư mục tạm hệ thống và tự xóa sau khi kiểm thử thành công; không sửa ảnh cá nhân. Kiểm tra JPEG→PNG/WebP giữ EXIF/XMP/ICC/GPS/Orientation, tên tiếng Việt, timestamp/JSON, album theo ngày chụp và loại trừ đầu ra, không ghi đè, remux giữ hai audio/ngôn ngữ, TIFF→PNG 16-bit, chapter khi đổi tốc độ thật, trích MP3, footer file ẩn mới/cũ.

```powershell
g++ -std=c++17 -O0 -Iinclude tests/media_integrity.cpp -o bin/media-test.exe -lole32 -lwindowscodecs -luuid -static-libgcc -static-libstdc++ -static
./bin/media-test.exe
```

Một số môi trường sandbox chặn signal pipe của Perl/MSYS; chạy test trong terminal Windows bình thường. Kiểm thử không chứng minh mọi model máy ảnh, tag riêng hãng, HDR hoặc codec đều tương thích.
