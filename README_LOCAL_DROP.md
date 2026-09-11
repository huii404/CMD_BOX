# BÁO CÁO KỸ THUẬT: LOCAL WEB DROP & AUTO-DISCOVERY BEACON

**Trạm truyền file P2P nội bộ & cơ chế bắn sóng tự động ghép đôi**

> **Phân hệ:** `Internet` / `internet` | **Ngôn ngữ:** C++17 Native (Winsock2)
> **Nguyên lý cốt lõi:** Không server trung gian – Không sinh file tạm – Tự phát hiện thiết bị qua UDP Broadcast – Truyền file tốc độ LAN qua TCP

---

## I. Cơ sở lý thuyết: Vì sao LAN nhanh hơn Internet

Khi tải qua Internet (Zalo, Drive...), gói tin phải đi qua nhiều chặng (nhà mạng, server trung gian, có thể ở nước ngoài) nên bị giới hạn bởi băng thông gói cước (vd 100 Mbps ≈ 12 MB/s).

Khi truyền trực tiếp trong LAN qua Wi-Fi 5GHz/Wi-Fi 6, gói tin chỉ đi qua Router nội bộ, nên **tốc độ lý thuyết cao hơn** (chuẩn Wi-Fi 5/6 hỗ trợ 500–1200 Mbps). Tuy nhiên **tốc độ thực tế luôn thấp hơn lý thuyết đáng kể** do overhead giao thức, nhiễu sóng, khoảng cách, và số thiết bị cùng chia băng thông — mức thực dụng phổ biến là **20–100 MB/s**, không nên quảng cáo mức 150 MB/s như một con số mặc định.

**Lưu ý sửa lỗi logic:** Với file dung lượng lớn (vd 1.85 GB), việc tải **vẫn cần thời gian và vẫn hiển thị thanh tiến trình** (như chính Dashboard ở mục II mô tả %). Không nên mô tả là "mở link file đã nằm sẵn trong máy" — điều này chỉ đúng với file rất nhỏ (vài MB). Nên diễn đạt lại là: *"tốc độ tải nhanh hơn nhiều lần so với qua Internet, và với file nhỏ có thể hoàn tất gần như tức thời."*

**Đặc tính không đổi:**

- **Zero-Cloud:** Không phụ thuộc Internet ngoài, chỉ cần Router còn hoạt động.
- **Zero-Temp-File:** Đọc file tuần tự theo khối và ghi thẳng vào socket, không sinh file tạm ở `%TEMP%`.

---

## II. Dashboard Console (giữ nguyên, tham khảo)

```text
========================================================================================
                  CMD BOX - LOCAL WEB DROP (TRẠM TRUYỀN FILE NỘI BỘ)
========================================================================================
 [*] File đang phát : D:\Videos\Demo_Project_4K.mp4 (1.85 GB)
 [*] Tình trạng     : Sẵn sàng kết nối [Lắng nghe cổng TCP 8888]
 [*] Địa chỉ tải về : http://192.168.10.103:8888/download
               [ QUÉT MÃ QR BẰNG CAMERA ĐIỆN THOẠI ĐỂ TẢI NGAY ]
 [ Phím 0: Dừng truyền file & Đóng cổng ]
----------------------------------------------------------------------------------------
 [*] Thiết bị kết nối : 192.168.10.45 (Apple iPhone 15 Pro)
 [*] Tốc độ truyền    : [████████████████████░░░░░] 78% (68.5 MB/s)
========================================================================================
```

---

## III. Cơ chế Auto-Discovery Beacon (2 tầng mạng)

### 1. Vì sao dùng UDP Broadcast thay vì TCP để dò tìm

TCP cần biết IP đích trước để bắt tay 3 bước (`SYN → SYN-ACK → ACK`). Khi chưa biết máy kia ở đâu, không thể mở kết nối TCP. UDP Broadcast (`255.255.255.255`) cho phép gửi 1 gói tin mà **mọi máy trong cùng mạng LAN đều nhận được cùng lúc** — đúng công nghệ nền tảng của Bonjour (Apple) và Spotify Connect.

**Lưu ý kỹ thuật bắt buộc:** Socket UDP gửi broadcast phải bật cờ `SO_BROADCAST` trước khi `sendto()`, nếu không hệ điều hành sẽ từ chối gửi.

### 2. Luồng hoạt động 2 chiều

```text
       [ MÁY PHÁT (SENDER) ]                               [ MÁY NHẬN (RECEIVER) ]
                 │  1. UDP Broadcast (Port 53318):                    │
                 │     "CMDBOX_BEACON|FILE|clip.mp4|1.8GB|URL"        │
                 ├────────────────────────────────────────────────────>│
                 │                                                     ▼
                 │                                     Popup xác nhận trên CMD:
                 │                                     "Máy PC-A muốn gửi 'clip.mp4'.
                 │                                      Chấp nhận? [y/N]"
                 │  2. UDP Unicast phản hồi:                          │ (bấm 'y')
                 │     "CMDBOX_ACCEPT|OK"                             │
                 │<────────────────────────────────────────────────────┤
                 ▼                                                     ▼
   [ Mở luồng TCP Stream ] ═══════════════════════════════> [ Nhận file trực tiếp vào ổ đĩa ]
```

**Lưu ý kiến trúc đa luồng (bổ sung — tài liệu gốc chưa nêu):**
Vì UDP Broadcast (lắng nghe/phát) và TCP Server (chờ kết nối) phải chạy **song song, không được chặn nhau**, cần tách ít nhất 2 thread riêng biệt:

- Thread 1: `recvfrom()` lắng nghe UDP Beacon trên port 53318.
- Thread 2: `accept()` chờ kết nối TCP trên port 8888.
  Console chính (UI) phải chạy trên thread riêng, không dùng chung với 2 thread mạng để tránh treo giao diện khi `recv()`/`accept()` đang block.

### 3. Nguyên tắc an toàn

- Ở Menu chính: không mở socket nào, không có thread ngầm.
- Bấm `[1] Bắn file`: mở socket UDP phát Beacon.
- Bấm `[2] Chờ nhận`: mở socket UDP lắng nghe Beacon.
- Bấm `[0]` hoặc truyền xong: đóng toàn bộ socket (`closesocket`), dừng thread ngay. Nên dùng RAII (destructor tự đóng socket) để đảm bảo dọn dẹp kể cả khi có exception hoặc thoát đột ngột — tài liệu gốc chỉ đề cập trường hợp thoát bình thường.

---

## IV. Vì sao điện thoại không mở được link (3 rào cản)

| Rào cản                                | Hiện tượng                                                                              | Khắc phục                                                                                                                                                          |
| :--------------------------------------- | :----------------------------------------------------------------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Bind nhầm Loopback**            | PC vào`localhost:8888` được, điện thoại vào IP thật thì bị từ chối.         | Bind socket TCP vào`INADDR_ANY` (`0.0.0.0`), không dùng `127.0.0.1`.                                                                                        |
| **Windows Firewall chặn Inbound** | Điện thoại báo`Connection Timed Out` (Firewall drop gói SYN).                       | Tự động chạy`netsh advfirewall firewall add rule name="CMDBOX_DROP" dir=in action=allow protocol=TCP localport=8888` khi bật tính năng; xoá rule khi tắt. |
| **Lấy nhầm IP card mạng ảo**   | Hiện IP của VMware/VirtualBox (`172.x.x.x`, `192.168.56.x`) thay vì IP Wi-Fi thật. | Dùng`GetAdaptersAddresses` (Win32), chỉ lấy adapter đang có Default Gateway hướng ra Wi-Fi.                                                                 |

---

## V. Thuật toán truyền file (Chunked Stream chống tràn RAM)

Đọc và gửi theo khối 64KB, không nạp toàn bộ file vào RAM. **Sửa lỗi so với bản gốc:** `send()` có thể gửi *một phần* dữ liệu mỗi lần gọi (partial send), nên bắt buộc phải lặp gửi cho tới khi hết khối, không giả định gửi hết trong 1 lần gọi:

```cpp
char buffer[65536];
std::ifstream file(filePath, std::ios::binary);

while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
    int toSend = (int)file.gcount();
    int sentSoFar = 0;
    while (sentSoFar < toSend) {
        int bytesSent = send(clientSocket, buffer + sentSoFar, toSend - sentSoFar, 0);
        if (bytesSent <= 0) goto disconnected; // thiết bị nhận ngắt kết nối
        sentSoFar += bytesSent;
        totalTransferred += bytesSent;
    }
}
disconnected:;
```

RAM tiêu thụ giữ ổn định ở mức thấp (cỡ vài chục MB, không phụ thuộc dung lượng file) do chỉ giữ 1 buffer 64KB tại một thời điểm — nên tránh nêu con số tuyệt đối cụ thể (vd "dưới 15MB") vì còn phụ thuộc overhead của OS/thread/stack.

---

## VI. Giới hạn kỹ thuật

1. **Cùng mạng LAN/Wi-Fi:** UDP Broadcast và IP dải `192.168.x.x` không định tuyến qua Internet — điện thoại dùng 4G/5G riêng sẽ không thấy được.
2. **AP Isolation trên Wi-Fi công cộng:** Một số quán café/khách sạn cô lập thiết bị với nhau. Khắc phục: bật Mobile Hotspot trên Windows rồi cho thiết bị kia kết nối vào.
3. **Khoá màn hình khi nhận trên điện thoại:** iOS/Android có thể tạm dừng trình duyệt khi khoá màn hình giữa lúc tải file lớn — nên giữ sáng màn hình.
4. **Quyền Administrator:** Cần chạy với quyền Admin để tự động thêm/xoá rule Firewall mà không hiện popup UAC giữa chừng.
5. **Chỉ hỗ trợ IPv4:** Cơ chế Broadcast và bind `INADDR_ANY` như mô tả chỉ áp dụng cho IPv4; nếu mạng chỉ có IPv6 sẽ cần cơ chế khác (không nằm trong phạm vi tài liệu này).
6. **Một kết nối tại một thời điểm:** Thiết kế hiện tại phục vụ 1 client TCP/lượt truyền; nếu cần nhiều thiết bị tải cùng lúc phải mở thread riêng cho mỗi `accept()`.

---

## VII. Kịch bản sử dụng thực tế

1. **PC → Điện thoại:** Gửi video 4K vừa dựng sang iPhone/Android để đăng mạng xã hội mà không bị nén như qua Zalo.
2. **PC → PC:** Hai máy trong văn phòng gửi file cài đặt dung lượng lớn cho nhau qua LAN, không cần USB.
