# LocalDrop v2 — truyền file và thư mục trong LAN

## Cách sử dụng

Trong menu **Mạng & Bảo mật → Chia sẻ file cục bộ**:

- **[1] Public (Web QR):** chọn một hoặc nhiều file/thư mục. Điện thoại quét QR để mở danh sách và tải từng file. Chế độ này không cần mã ghép đôi và không phát beacon.
- **[2] Gửi file / thư mục:** chọn dữ liệu, đưa mã ghép đôi 6 số trên máy gửi cho người nhận. Điện thoại cũng có thể quét QR và nhập mã trên trang web.
- **[3] Nhận file / thư mục:** tìm máy gửi trong LAN, nhập mã ghép đôi, kiểm tra danh sách và xác nhận nhận vào Downloads.
- **0 / Esc:** dừng trạm hoặc dừng truyền; máy nhận giữ lại phần đã tải để tiếp tục.

Ví dụ chọn nhiều nguồn:

```text
"D:\Ảnh gia đình" "D:\Video\clip.mp4" "D:\Tài liệu\ghi chú.txt"
```

Đường dẫn có khoảng trắng cần đặt trong dấu nháy kép. Một đường dẫn đơn có thể nhập không có nháy, kể cả có khoảng trắng. Hai thiết bị phải liên thông trong cùng LAN; không cần Internet để truyền dữ liệu.

## Những điểm mới

- Gửi nhiều file và quét file trong thư mục con; giữ tên thư mục gốc và cấu trúc các file khi nhận bằng CMD BOX. Thư mục rỗng không có trong danh sách truyền.
- Đọc theo khối 256 KB, không nạp toàn bộ file vào RAM.
- Tính SHA-256 trước khi phát và khóa file nguồn để tránh sửa nội dung trong phiên truyền.
- Mã ghép đôi 6 số tạo bằng Windows BCrypt; beacon chỉ phát cổng, mã phiên, số file và tổng dung lượng. Không phát mã ghép đôi, tên file hay link có quyền truy cập.
- Giới hạn thử mã: sau 5 lần sai, tạm chặn các yêu cầu có mã trong 30 giây. Trang nhập mã vẫn mở được.
- HTTP `Range`, `206`, `416`, `HEAD`, `Content-Range`, `Accept-Ranges`, ETag và hash của file.
- Máy nhận tự thử tiếp tục tối đa 8 lần khi mất kết nối. Có thể vào mục Nhận lại sau khi hết lượt thử hoặc chủ động dừng.
- File tải dở nằm trong `.cmd-box-partials` bên dưới thư mục nhận. Chỉ đổi sang tên chính thức khi SHA-256 khớp. Hash sai thì đặt lại file tải dở về 0 byte để lượt sau tải lại.
- Thư mục nhận là `Downloads\CMD_BOX_Receive_<mã danh sách>`. Mã dựa trên danh sách file, tên, kích thước và hash, nên có thể tiếp tục khi mở lại máy gửi với cùng dữ liệu. File hoàn tất đúng hash được bỏ qua; file đích khác nội dung không bị ghi đè.
- Kiểm tra đường dẫn tương đối, tên thiết bị Windows, trùng tên, file/thư mục xung đột, junction/symlink và hard link của file tải dở.

## QR vẫn sử dụng API

QR lấy từ **qrenco.de** qua `curl.exe`, không triển khai thuật toán tạo QR trong ứng dụng. Yêu cầu API chỉ chứa địa chỉ trang chủ LAN, không chứa mã ghép đôi hay tên file. API có timeout; nếu không phản hồi, ứng dụng vẫn hiển thị địa chỉ để mở thủ công và tiếp tục phục vụ file.

Internet chỉ cần cho bước lấy QR. API bên ngoài nhận được địa chỉ trang chủ LAN.

## Phạm vi và tương thích

- HTTP chưa mã hóa; mã ghép đôi kiểm soát truy cập, không thay thế TLS. SHA-256 kiểm tra nội dung nhận khớp với danh sách của máy gửi, không xác thực danh tính máy gửi bằng mật mã.
- Tiếp tục tự động, nhận cả thư mục và kiểm tra SHA-256 tự động có trong **máy nhận CMD BOX**. Trang web tải từng file; khả năng tải tiếp phụ thuộc trình duyệt.
- Máy gửi xử lý một kết nối tải tại một thời điểm. Dò beacon có thể chậm khi máy gửi đang phục vụ một lượt tải dài.
- Giới hạn 10.000 file và danh sách 8 MiB; từ chối danh sách rỗng và các nguồn có cùng tên đích.
- Tên file và nội dung được giữ lại; chưa đồng bộ ACL, timestamp hoặc thư mục rỗng.
- Native P2P dùng giao thức `CMDBOX2`, nên cần cập nhật CMD BOX trên cả hai máy. Máy nhận phiên bản cũ không tương thích beacon mới.
- TCP thử cổng 8888 rồi 8889; UDP discovery dùng cổng 53318. Rule TCP tự tạo chỉ áp dụng mạng Private và local subnet, được xóa khi thoát bình thường. Discovery UDP phụ thuộc cấu hình firewall của máy nhận.

## Kiểm chứng

Build toàn ứng dụng cần thư viện Windows `bcrypt`; `build.bat` đã bổ sung `-lbcrypt`.

Bài kiểm thử tích hợp nằm tại `validation/localdrop_integration.cpp`. Nó chạy luồng gửi/nhận thật qua loopback, thay thao tác firewall và QR bằng stub, chuyển Downloads sang thư mục fixture trong workspace. Kiểm tra hash chuẩn, đường dẫn độc hại, tên tiếng Việt, cây thư mục, file rỗng, mã ghép đôi, portal, HEAD, byte range, ngắt kết nối và tải tiếp, đổi tên sau xác minh, bỏ qua file đã hoàn tất và giới hạn thử mã.

```powershell
g++ -std=c++17 -O0 -Iinclude validation/localdrop_integration.cpp -o bin/localdrop-test.exe -lbcrypt -lws2_32 -liphlpapi -lole32 -luuid -static-libgcc -static-libstdc++ -static
./bin/localdrop-test.exe
```

Kiểm thử loopback không thay thế kiểm thử hai thiết bị thật, firewall Windows, tốc độ Wi-Fi hoặc khả năng đáp ứng của API QR bên ngoài.
