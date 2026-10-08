# Media: metadata và album theo thời gian

## Phụ thuộc

Media sử dụng FFmpeg, ffprobe và ExifTool. Công cụ được tìm cạnh `main.exe`, trong `bin` của checkout, rồi trong PATH. Thiếu backend metadata thì dừng trước render; không báo thành công khi chưa đọc được thông tin nguồn.

Bản Windows portable: đặt `exiftool.exe` (đổi tên từ `exiftool(-k).exe`) và thư mục `exiftool_files` cùng cấp trong `bin`. Lấy bản chính thức tại [ExifTool](https://exiftool.org/). Khi phân phối ứng dụng, cung cấp cả FFmpeg, ffprobe và runtime ExifTool, kèm giấy phép của chúng. Không chỉ gửi `main.exe`.

Checkout này cũng hỗ trợ source ExifTool 13.59 với Perl đã có trong PATH hoặc MSYS2. Source nằm trong `bin/exiftool-source/exiftool-13.59`; không cài vào hệ thống. Có thể chuẩn bị lại bằng:

```powershell
./scripts/setup-media.ps1
```

Một script duy nhất `scripts/setup-media.ps1` chuẩn bị FFmpeg trước, kể cả khi đã có `exiftool.exe`, rồi chuẩn bị ExifTool. Script tải bản release essentials ZIP mới nhất từ [Gyan](https://www.gyan.dev/ffmpeg/builds/), lấy version rồi dùng URL có version cho archive và SHA-256, kiểm tra checksum và chạy thử FFmpeg trước khi cài vào `bin`. Chỉ chép `ffmpeg.exe`, không cài `ffprobe.exe`. Giữ `LICENSE`/`README.txt` tại `bin/ffmpeg-docs`. Bỏ qua tải khi FFmpeg sẵn có chạy được; dùng `setup-media.ps1 -ForceFFmpeg` để tải lại/cập nhật. Gyan cung cấp binary Windows x64; script không cài vào hệ thống hoặc sửa PATH.

Code media vẫn dùng ffprobe để đọc stream/chapter và kiểm tra metadata container video. Những tác vụ này cần ffprobe có sẵn cạnh `main.exe` hoặc trong PATH; script không chuẩn bị phụ thuộc này.

Phần ExifTool dùng tag 13.59 của [repo tác giả](https://github.com/exiftool/exiftool), kiểm tra SHA-256 archive trước giải nén: `542315bbb4b302b5334b6defd608d9ce2c97299c5608ab83a9e6c232abd56f18`.

## Bảo tồn metadata khi xuất

`MediaMetadata.h` điều phối ExifTool, đọc timestamp và kiểm tra metadata nhúng. Không xuất `.metadata.json`, không lưu metadata trong file dự phòng.

Các luồng nén, đổi định dạng, đổi tốc độ và trích MP3:

1. Đọc timestamp và metadata nguồn vào RAM trước render.
2. FFmpeg ghi metadata container/track trong lúc render hoặc remux. Với MP3, ngày quay, camera, GPS và mô tả được chuyển thành trường ID3 nếu biểu diễn được.
3. Với ảnh, ExifTool ghi EXIF/XMP/ICC trực tiếp vào file vừa render; giữ Orientation tương ứng với `-noautorotate`, cập nhật kích thước EXIF và bỏ thumbnail/preview cũ.
4. Với MP4/MOV, ExifTool bổ sung metadata native QuickTime/XMP; không lấy duration/codec cũ để ghi đè thông tin kỹ thuật mới.
5. Đọc lại file và đối chiếu các trường kỷ niệm: EXIF/IFD0/GPS/IPTC/XMP, ngày quay, camera, tác giả, mô tả, ICC/MakerNotes và metadata container. Tag khác tên/kiểu biểu diễn được đối chiếu theo alias; timezone và phần thập phân bằng 0 không bị coi là khác ngày.
6. Nếu không ghi hoặc không kiểm tra được trường cần giữ, **không công nhận bản xuất**; hiển thị tên trường và giữ nguồn. Không dùng JSON để bù thông tin thiếu.
7. Chỉ công bố một file media, không ghi đè và khôi phục timestamp. Nếu công bố hoặc timestamp thất bại thì hủy bản xuất do lần chạy đó tạo.

Thư mục làm việc và file chapter chỉ phục vụ chạy FFmpeg an toàn, được dọn sau thao tác; metadata của kết quả nằm trong chính file media và không phụ thuộc vào chúng. Nguồn không có metadata thì ứng dụng không tự bịa ngày chụp, GPS hay máy ảnh.

Giữa các định dạng, không phải mọi metadata đều có trường tương đương. Tag kỹ thuật như kích thước, thời lượng, codec, encoder và thumbnail phải phù hợp bản xuất; không bắt giữ nguyên chúng. Các định dạng/tag chưa được kiểm chứng có thể bị từ chối. File nguồn luôn được giữ.

### Video và chất lượng

- Bỏ mapping nhầm metadata cấp file vào track. Metadata stream đi theo stream được map.
- Chuyển container thử `-map 0 -c copy` trước, không đoán tương thích từ đuôi MOV/MKV. Các stream và chương được giữ nếu muxer hỗ trợ.
- Khi buộc render, giữ tất cả audio và subtitle được hỗ trợ. Không âm thầm bỏ nhiều video, telemetry hoặc attachment: từ chối render nguồn phức tạp và giữ nguồn để remux.
- Đổi tốc độ dùng độ chính xác cao cho video/audio và chia mốc chapter cho hệ số tốc độ.
- PNG có thể giữ 16-bit khi chuyển TIFF 16-bit; JPEG/WebP 8-bit có thông báo trước render. Nén tự động nguồn ảnh trên 8-bit chọn PNG, không ép xuống JPEG. Ảnh float không bị tự giảm độ chính xác.
- HDR hoặc video trên 8-bit chỉ remux. Nén/đổi tốc độ chưa render HDR/10-bit; từ chối để tránh giảm bit-depth/dải màu mà không có sự lựa chọn.
- ICC không phải RGB (ví dụ CMYK/Gray): từ chối render ảnh vì chưa có engine chuyển màu giữ profile đúng; không gắn profile sai và không lưu thay bằng JSON.
- Trích MP3 lấy track audio đầu tiên; nguồn còn nguyên; bản MP3 chỉ chứa track được trích. Đây không phải tính năng gộp mọi track audio.
- Báo dung lượng tiết kiệm dùng kích thước file cuối sau ghi metadata, không dùng số đo trước ghi.

### File ẩn

Footer mới `CBOXHID2` lưu kích thước cover/payload/JSON, tên và timestamp gốc của payload. Cover và payload được sao chép nguyên byte (payload XOR như trước), nên metadata nhúng vẫn còn. Trích xuất giữ nguyên byte payload và khôi phục timestamp, không xuất JSON đi kèm. JSON trong footer là cấu trúc nội bộ nằm ngay trong container, không phải file bên ngoài. Footer `HIDE` cũ vẫn đọc được; không thể khôi phục tên/timestamp không được lưu trong định dạng cũ. XOR không phải mã hóa bảo mật.

## Sắp album từ THƯ MỤC

Menu **Media → [7] Sắp album từ thư mục**:

- Chọn thư mục, quét media trong các thư mục con; bỏ junction/symlink và `CMD_BOX_Output`/`CMD_BOX_Album` để không quét lặp.
- Ưu tiên `SubSecDateTimeOriginal`, `DateTimeOriginal`, `DateCreated`, `CreationDate`, `CreateDate`, `MediaCreateDate`.
- Nếu không có ngày chụp/quay hợp lệ, dùng ngày sửa file và ghi rõ trong preview; không coi đó là ngày chụp đã được xác minh.
- Xem trước rồi xác nhận; chỉ sao chép, không di chuyển hoặc đổi tên nguồn.
- Đầu ra: `nguồn/CMD_BOX_Album/YYYY/MM/<đường dẫn tương đối gốc>`. Giữ các thư mục tương đối để hai nguồn cùng tên không đè nhau.
- File cùng tên đã có được bỏ qua và thông báo. Sao chép nguyên byte để giữ metadata nhúng và khôi phục timestamp; không render, không tạo JSON đi kèm.
- Giới hạn mỗi lần quét 10.000 file. Album hiện không tự nhóm theo sự kiện hoặc vị trí.

## Kiểm thử

`tests/media_integrity.cpp` tạo fixture riêng trong thư mục tạm hệ thống và tự xóa sau khi kiểm thử thành công; không sửa ảnh cá nhân. Kiểm tra JPEG→PNG/WebP giữ EXIF/XMP/ICC/GPS/Orientation, tên tiếng Việt, timestamp, không tạo JSON, MOV giữ camera/GPS/mô tả, MP3 giữ metadata ID3, từ chối bản xuất thiếu metadata, album theo ngày chụp và loại trừ đầu ra, không ghi đè, remux giữ hai audio/ngôn ngữ, TIFF→PNG 16-bit, chapter khi đổi tốc độ thật, trích MP3, footer file ẩn mới/cũ.

```powershell
g++ -std=c++17 -O0 -Iinclude tests/media_integrity.cpp -o bin/media-test.exe -lole32 -lwindowscodecs -luuid -static-libgcc -static-libstdc++ -static
./bin/media-test.exe
```

Một số môi trường sandbox chặn signal pipe của Perl/MSYS; chạy test trong terminal Windows bình thường. Kiểm thử không chứng minh mọi model máy ảnh, tag riêng hãng, HDR hoặc codec đều tương thích.
