# BÁO CÁO KỸ THUẬT: LOCAL WEB DROP & AUTO-DISCOVERY BEACON

**Trạm truyền file P2P nội bộ & cơ chế bắn sóng tự động ghép đôi**

> **Phân hệ:** `Internet` / `internet` | **Ngôn ngữ:** C++17 Native (Winsock2)
> **Nguyên lý cốt lõi:** Không qua Server trung gian Internet (Zero-Cloud) – YÊU CẦU KẾT NỐI MẠNG CỤC BỘ (LAN) LIÊN THÔNG – Không sinh file tạm – Tự phát hiện thiết bị qua UDP Broadcast – Truyền file tốc độ LAN qua TCP

---

## I. Cơ sở lý thuyết: Bản chất Mạng & Điều kiện Tiên quyết

### 1. Mạng là môi trường truyền dẫn (Transmission Medium)
Trong mô hình mạng máy tính (OSI / TCP-IP), **mạng là môi trường truyền dẫn bắt buộc** (vật lý qua cáp đồng/cáp quang hoặc vô tuyến qua sóng Wi-Fi).
- **Mất mạng = Mất gói tin:** Nếu một trong hai máy mất kết nối mạng (tắt Wi-Fi, đứt cáp LAN, mất tín hiệu sóng, ngắt card mạng), đường truyền vật lý/logic bị triệt tiêu hoàn toàn. Khi đó:
  - Gói tin TCP (`SYN`, `ACK`, `DATA`) không thể đến đích, socket sẽ rơi vào trạng thái `WSAETIMEDOUT` hoặc `WSAECONNRESET`.
  - Gói tin UDP Broadcast/Unicast bị thất lạc hoàn toàn trên tầng liên kết dữ liệu (Data Link Layer).
  - **Kết luận kỹ thuật:** Tính năng truyền file P2P hay Web Drop **KHÔNG THỂ hoạt động khi 1 trong 2 thiết bị mất mạng**. Cả 2 máy **bắt buộc phải duy trì liên kết mạng chung** trong suốt quá trình truyền file.

### 2. Phân định rõ ràng: "Không cần Internet (WAN)" ≠ "Không cần mạng"
- **Không cần Internet WAN (Zero-Cloud):** Không gửi dữ liệu lên máy chủ đám mây (Google Drive, Zalo, OneDrive...), không tốn băng thông gói cước nhà mạng ra quốc tế, không chịu rủi ro rò rỉ dữ liệu qua bên thứ ba.
- **BẮT BUỘC CÓ MẠNG NỘI BỘ (LAN Link):** Hai thiết bị phải liên thông với nhau qua một hạ tầng mạng cục bộ:
  - Cùng kết nối vào một Router Wi-Fi / Switch mạng LAN gia đình/văn phòng.
  - HOẶC một máy phát Mobile Hotspot (điểm truy cập di động) cho máy kia kết nối vào.
  - HOẶC cắm trực tiếp cáp mạng LAN / cáp USB Tethering giữa 2 máy.

### 3. Vì sao LAN nhanh hơn Internet
Khi tải qua Internet, gói tin phải đi qua nhiều chặng định tuyến ngoài (ISP, gateway quốc tế, server trung gian) nên bị giới hạn bởi băng thông gói cước (vd 100 Mbps ≈ 12 MB/s).
Khi truyền trực tiếp trong mạng LAN qua Wi-Fi 5GHz/Wi-Fi 6 hoặc cáp Gigabit Ethernet, gói tin đi thẳng giữa 2 máy qua Switch/Router nội bộ với băng thông phần cứng cao. Tuy nhiên, **tốc độ thực tế phụ thuộc chất lượng sóng, khoảng cách, nhiễu kênh và năng lực đọc/ghi của ổ cứng**, thông thường dao động từ **20–100 MB/s**, không nên phóng đại con số tối đa lý thuyết.

**Đặc tính kỹ thuật chuẩn xác:**
- **Zero-Cloud:** Hoạt động hoàn toàn trong mạng nội bộ, không cần đường truyền Internet ra thế giới.
- **Zero-Temp-File:** Đọc file tuần tự theo khối (256 KB) và ghi thẳng vào socket / ổ đĩa, không sinh file tạm ở `%TEMP%`.
- **Yêu cầu kết nối:** Cả 2 thiết bị phải liên tục online trong cùng một mạng LAN.

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

## IV. Vì sao thiết bị không kết nối được hoặc đứt truyền tải (Các rào cản thực tế)

| Rào cản | Hiện tượng | Nguyên nhân kỹ thuật & Cách khắc phục |
| :--- | :--- | :--- |
| **Một trong hai máy mất mạng** | Đang truyền thì dừng hẳn, console báo `Ngắt kết nối`, điện thoại báo `Download Failed` | **Nguyên lý:** Mạng là môi trường truyền dẫn, mất mạng = mất gói tin. Khi 1 máy ngắt kết nối (tắt Wi-Fi, đứt cáp), socket TCP gửi bị lỗi (`bytesSent <= 0` hoặc `WSAECONNRESET`).<br>**Khắc phục:** Bắt buộc cả 2 máy phải giữ kết nối mạng LAN liên tục và ổn định. |
| **Bind nhầm Loopback** | PC vào `localhost:8888` được, điện thoại vào IP thật thì bị từ chối. | Bind socket TCP vào `INADDR_ANY` (`0.0.0.0`), không dùng `127.0.0.1`. |
| **Windows Firewall chặn Inbound** | Điện thoại báo `Connection Timed Out` (Firewall drop gói SYN). | Tự động chạy `netsh advfirewall firewall add rule name="CMDBOX_DROP" dir=in action=allow protocol=TCP localport=8888` khi bật tính năng; xoá rule khi tắt. |
| **Lấy nhầm IP card mạng ảo** | Hiện IP của VMware/VirtualBox (`172.x.x.x`, `192.168.56.x`) thay vì IP Wi-Fi thật. | Dùng `GetAdaptersAddresses` (Win32), chỉ lấy adapter đang có Default Gateway hướng ra Wi-Fi/LAN thật. |
| **Mạng nội bộ không có Internet (Không vẽ được QR)** | Màn hình console chỉ hiện URL, không vẽ được hình mã QR ASCII. | Hàm `fetchQrCodeApi()` dùng `curl` gọi API ngoài `qrenco.de`. Khi mạng cô lập không có Internet WAN, lệnh sẽ timeout (1-2s). Điều này **không ảnh hưởng** đến truyền file: người dùng chỉ cần mở trình duyệt gõ thẳng địa chỉ IP:Port hiển thị trên màn hình. |

---

## V. Thuật toán truyền file (Chunked Stream chống tràn RAM)

Đọc và gửi theo khối 256KB (hoặc 64KB), không nạp toàn bộ file vào RAM. `send()` có thể gửi *một phần* dữ liệu mỗi lần gọi (partial send), nên bắt buộc phải lặp gửi cho tới khi hết khối, không giả định gửi hết trong 1 lần gọi:

```cpp
char buffer[CHUNK_SIZE];
std::ifstream file(filePath, std::ios::binary);

while (isRunning && (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)) {
    int toSend = (int)file.gcount();
    int sentSoFar = 0;
    while (sentSoFar < toSend && isRunning) {
        int bytesSent = send(clientSocket, buffer + sentSoFar, toSend - sentSoFar, 0);
        if (bytesSent <= 0) goto client_disconnected; // thiết bị nhận ngắt kết nối hoặc mất mạng
        sentSoFar += bytesSent;
        totalTransferred += bytesSent;
    }
}
client_disconnected:;
```

RAM tiêu thụ giữ ổn định ở mức thấp (vài chục MB, không phụ thuộc dung lượng file) do chỉ giữ 1 buffer cố định tại một thời điểm.

---

## VI. Giới hạn kỹ thuật

1. **Bắt buộc chung mạng LAN / Liên kết truyền dẫn liên thông:** Cả 2 máy phải kết nối vào cùng một hệ thống mạng cục bộ (chung Router Wi-Fi, chung switch, hoặc 1 máy phát Mobile Hotspot cho máy kia bắt). **Nếu 1 trong 2 máy mất mạng, việc truyền file hoàn toàn không thể thực hiện do mất gói tin.**
2. **Không định tuyến qua Internet:** UDP Broadcast và dải IP nội bộ (`192.168.x.x`, `10.x.x.x`, `172.16-31.x.x`) không đi qua Internet — điện thoại dùng 4G/5G độc lập ngoài luồng LAN sẽ không thấy được máy tính.
3. **AP Isolation trên Wi-Fi công cộng:** Một số quán café/khách sạn bật tính năng cô lập thiết bị (AP Isolation/Client Isolation). Khi đó các máy dù cùng bắt 1 Wi-Fi nhưng Router cấm giao tiếp P2P với nhau. Khắc phục: bật Mobile Hotspot trên một trong hai thiết bị rồi cho thiết bị kia kết nối vào.
4. **Mã QR Console phụ thuộc Internet WAN:** Thư viện in mã QR ASCII trên Console dựa vào lệnh `curl` gọi API `qrenco.de`. Nếu mạng nội bộ không có Internet ra ngoài, mã QR ASCII không thể render; người dùng nhập link URL IP trực tiếp trên trình duyệt.
5. **Khoá màn hình khi nhận trên điện thoại:** iOS/Android có cơ chế tiết kiệm pin tự động ngắt kết nối socket của trình duyệt chạy ngầm khi màn hình tắt giữa chừng tải file lớn — cần giữ sáng màn hình khi đang nhận file.
6. **Quyền Administrator:** Cần chạy với quyền Admin để tự động thêm/xoá rule Firewall Windows.
7. **Một kết nối tại một thời điểm:** Thiết kế hiện tại phục vụ 1 client TCP/lượt truyền; nếu cần nhiều thiết bị tải cùng lúc phải mở thread riêng cho mỗi `accept()`.

---

## VII. Kịch bản sử dụng thực tế

1. **PC → Điện thoại:** Gửi video 4K vừa dựng sang iPhone/Android qua mạng Wi-Fi nội bộ mà không bị nén chất lượng như qua Zalo/Messenger, không tốn dung lượng 4G.
2. **PC → PC:** Hai máy tính trong cùng văn phòng hoặc gia đình gửi file dữ liệu dung lượng lớn cho nhau qua LAN tốc độ cao, không cần tháo lắp USB.
