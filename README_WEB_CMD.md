# BÁO CÁO KIẾN TRÚC & PHÂN TÍCH KỸ THUẬT: WEB CMD REMOTE TERMINAL

**Hệ thống điều khiển máy tính từ xa qua Web Socket/HTTP Engine dành cho CMD BOX**

> **Phân hệ:** `Network` & `Core` | **Ngôn ngữ:** C++17 Native (Winsock2, Win32 API)  
> **Nguyên lý cốt lõi:** Zero-Cloud (Mạng Cục bộ LAN, Không cần Internet WAN) – YÊU CẦU LIÊN KẾT MẠNG THÔNG SUỐT – Zero-Dependency (Không cần Node.js/Python) – Bi-directional Real-time I/O (Pipes & ConPTY) – Embedded Single Page App (SPA)

---

## I. Bối cảnh Kỹ thuật & Bản chất Mạng

### 1. Nguyên lý Truyền dẫn: Mạng là nơi truyền tin (Mất mạng = Mất gói tin)
Trong kiến trúc mạng máy tính TCP/IP, **mạng là môi trường truyền dẫn bắt buộc** (Physical / Data Link / Network layers).
* **Mất mạng = Mất gói tin:** Nếu 1 trong 2 thiết bị mất kết nối mạng (tắt Wi-Fi, rớt mạng, mất sóng Hotspot, ngắt cáp USB Tethering), không có bất kỳ luồng dữ liệu nào có thể di chuyển giữa 2 thiết bị. Gói tin TCP HTTP/SSE sẽ bị drop hoàn toàn, socket lập tức timeout hoặc báo lỗi đứt kết nối (`WSAECONNRESET`).
* **Phân định rõ ràng: "Không cần Internet WAN" ≠ "Không cần mạng":**
  * *Không cần Internet WAN:* Không phụ thuộc hạ tầng đám mây công cộng (TeamViewer, AnyDesk, AWS...), không gửi tín hiệu ra ngoài Internet toàn cầu, không tốn dữ liệu di động 4G ra quốc tế.
  * *BẮT BUỘC PHẢI CÓ LIÊN KẾT MẠNG CỤC BỘ (LAN Link):* Điện thoại và máy tính bắt buộc phải kết nối chung một môi trường mạng nội bộ để trao đổi gói tin IP.

### 2. Kịch bản Vận hành Thực tế (Khi Mất điện / Mất Internet diện rộng)
Khi sự cố xảy ra (ví dụ mất điện, Router Wi-Fi sập nguồn, đứt cáp quang biển):
* Các công cụ remote qua Internet (AnyDesk, UltraViewer, SSH qua domain) bị vô hiệu hóa hoàn toàn do thiếu server đám mây.
* **Giải pháp mạng cục bộ của CMD BOX:** Người dùng thiết lập một liên kết mạng cục bộ thay thế giữa 2 thiết bị:
  * Điện thoại bật **Mobile Hotspot** (điểm phát sóng Wi-Fi cá nhân) cho laptop/PC bắt vào (kể cả khi điện thoại không bật 4G/không có Internet).
  * HOẶC kết nối trực tiếp qua dây cáp **USB Tethering** hoặc cáp mạng LAN Ad-hoc.
* **Lưu ý tiên quyết:** Chỉ cần duy trì liên kết mạng cục bộ này thì Web CMD mới có thể truyền nhận gói tin điều khiển. Nếu 1 trong 2 thiết bị bị ngắt mạng, kết nối điều khiển sẽ bị gián đoạn ngay lập tức.

---

## II. Kiến trúc Tổng thể Hệ thống (System Architecture)

```text
 ┌──────────────────────────────────────────────────────────────────────────────────┐
 │                     CLIENT: ĐIỆN THOẠI / MÁY TÍNH BẢNG                           │
 │  Trình duyệt di động (Safari, Chrome) truy cập: http://192.168.43.10:53317       │
 │                                                                                  │
 │  ┌─────────────────────────────────┐   ┌──────────────────────────────────────┐  │
 │  │  TAB 1: QUICK ACTION CARDS      │   │  TAB 2: INTERACTIVE WEB TERMINAL     │  │
 │  │  - Dọn rác hệ thống (1-Click)   │   │  - Xterm/Console Output (Neon Theme) │  │
 │  │  - Tối ưu hiệu năng             │   │  - Thanh phím ảo (Ctrl+C, Tab, Up/Dn)│  │
 │  │  - Soi pin & Tắt máy từ xa      │   │  - Input bar gửi lệnh shell          │  │
 │  └─────────────────────────────────┘   └──────────────────────────────────────┘  │
 └─────────────────────────▲───────────────────────────────────────▲────────────────┘
                           │ HTTP POST / WebSocket                 │ SSE / HTTP Stream
                           ▼                                       ▼
 ┌──────────────────────────────────────────────────────────────────────────────────┐
 │                      CMD BOX ENGINE (C++17 WINSOCK SERVER)                       │
 │                                                                                  │
 │  ┌────────────────────────────────────────────────────────────────────────────┐  │
 │  │ 1. HTTP Router & Token Auth (Xác thực mã PIN 4 số chống truy cập trái phép)│  │
 │  └──────────────────────────────────────┬─────────────────────────────────────┘  │
 │                                         │                                        │
 │          ┌──────────────────────────────┴──────────────────────────────┐         │
 │          ▼                                                             ▼         │
 │  ┌────────────────────────────────┐         ┌─────────────────────────────────┐  │
 │  │ A. DISPATCHER MODULE (ACTIONS) │         │ B. PSEUDO-TERMINAL ENGINE (CMD) │  │
 │  │ - Gọi hàm native C++ nội bộ:   │         │ - Tạo Named Pipe / Anonymous    │  │
 │  │   + SystemOptimizer            │         │ - CreateProcessA ("cmd.exe")    │  │
 │  │   + UtilityTools               │         │ - AssignProcessToJobObject      │  │
 │  │   + Power/Battery Diagnostics  │         │ - Đọc Stdout/Stderr thời thực   │  │
 │  └────────────────────────────────┘         └─────────────────────────────────┘  │
 └──────────────────────────────────────────────────────────────────────────────────┘
```

---

## III. Phân tích Kỹ thuật Các Thành phần Cốt lõi

### 1. WinSock HTTP/SSE Server Engine siêu nhẹ
Thay vì dùng thư viện cồng kềnh như Boost.Asio hay cpp-httplib, hệ thống sử dụng **Winsock2 Native Multi-threaded Socket**:
* **Lắng nghe kết nối:** Socket TCP bind vào `0.0.0.0` (cổng mặc định `53317`) để chấp nhận kết nối từ mọi interface Wi-Fi/Hotspot/Ethernet.
* **Cơ chế đa luồng:** Mỗi request từ Client được gán cho một luồng làm việc riêng (hoặc dùng `std::thread` detach / Thread Pool gọn nhẹ).
* **Server-Sent Events (SSE):** Để đẩy output dòng lệnh về điện thoại mà không cần thiết lập giao thức WebSocket phức tạp, SSE là giải pháp tối ưu nhất:
  * Header HTTP trả về:
    ```http
    HTTP/1.1 200 OK
    Content-Type: text/event-stream
    Cache-Control: no-cache
    Connection: keep-alive
    Access-Control-Allow-Origin: *
    ```
  * Máy tính chỉ cần ghi định dạng: `data: [nội dung dòng chữ]\n\n` vào socket mỗi khi tiến trình có output mới.

### 2. Kỹ thuật Điều hướng I/O Tiến trình (Process Redirection qua Anonymous Pipes)
Để điện thoại có thể chạy lệnh CMD và nhận lại kết quả nguyên bản:
* **Khởi tạo 2 Anonymous Pipes:**
  * `hChildStdIn_Rd` / `hChildStdIn_Wr` (Ghi lệnh từ Web vào shell).
  * `hChildStdOut_Rd` / `hChildStdOut_Wr` (Đọc kết quả từ shell đẩy ra Web).
* **Kế thừa Handle:** Cấu hình `SECURITY_ATTRIBUTES` với `bInheritHandle = TRUE` để tiến trình con kế thừa pipe.
* **Khởi chạy Process an toàn với JobObject:**
  ```cpp
  STARTUPINFOA si = { sizeof(si) };
  si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE; // Ẩn hoàn toàn cửa sổ đen trên PC
  si.hStdOutput = hChildStdOut_Wr;
  si.hStdError  = hChildStdOut_Wr;
  si.hStdInput  = hChildStdIn_Rd;
  ```
* **Chống rò rỉ tiến trình (Zombie Processes):** Bắt buộc gán tiến trình vào `JobObject` với cờ `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`. Khi client ngắt kết nối hoặc Web CMD tắt, toàn bộ tiến trình con (`cmd.exe`, `ping`, `curl`...) tự động bị tiêu diệt sạch sẽ.

### 3. Cơ chế Bảo mật Mạng Cục bộ (Zero-Trust Local PIN Auth)
Trong môi trường mạng mở (như dùng chung Wi-Fi hoặc Hotspot), bất kỳ ai cùng mạng cũng có thể gõ IP máy tính để truy cập. Do đó, hệ thống áp dụng cơ chế xác thực:
1. Khi khởi động Web CMD, máy tính sinh ngẫu nhiên mã **PIN 4 hoặc 6 chữ số** (ví dụ: `8294`) và in to trên console màn hình máy tính.
2. Trình duyệt trên điện thoại mở ra sẽ bị chặn bởi màn hình **Lockscreen**.
3. Người dùng nhập mã PIN -> Client gửi header `X-Auth-Token` (băm SHA256 hoặc HMAC token tạm thời với salt thời gian).
4. Server xác minh hợp lệ mới cấp quyền thực thi lệnh shell hoặc chạy tính năng hệ thống.

---

## IV. Thiết kế Giao diện Người dùng Nhúng (Embedded Single Page App)

Toàn bộ Web UI được gói gọn trong 1 file HTML duy nhất, viết bằng Vanilla HTML/CSS/JS thuần, không dùng thư viện hay CDN bên ngoài (Zero External CDN) để tải trọn vẹn giao diện ngay cả khi mạng nội bộ không có kết nối Internet WAN:

### Layout Mô phỏng trên Điện thoại
```text
 ┌──────────────────────────────────────────────┐
 │  CMD BOX REMOTE                 [●] PIN: OK  │
 │  Pin: 84% [Đang xả] │ CPU: 18% │ RAM: 4.2GB  │
 ├──────────────────────────────────────────────┤
 │  [ Tab: Điều khiển nhanh ] [ Tab: Terminal ] │
 ├──────────────────────────────────────────────┤
 │                                              │
 │  C:\Windows\system32> ipconfig               │
 │  IPv4 Address. . . . . : 192.168.43.10       │
 │  Subnet Mask . . . . . : 255.255.255.0       │
 │  Default Gateway . . . : 192.168.43.1        │
 │                                              │
 │  C:\Windows\system32> _                      │
 │                                              │
 ├──────────────────────────────────────────────┤
 │  [TAB]  [CTRL+C]  [▲]  [▼]  [CLEAR]  [EXIT]  │
 ├──────────────────────────────────────────────┤
 │  [ Nhập lệnh tại đây...          ] [ GỬI ]   │
 └──────────────────────────────────────────────┘
```

* **Thanh phím trợ năng di động (Virtual Mobile Toolbar):**
  * `[Ctrl + C]`: Gửi tín hiệu `GenerateConsoleCtrlEvent` ngắt lệnh đang chạy (như ping vô tận).
  * `[▲]` / `[▼]`: Lấy lại các câu lệnh trước đó từ mảng lịch sử (History Stack).
  * `[TAB]`: Gợi ý đường dẫn/lệnh.

---

## V. Các Vấn đề Kỹ thuật Cần Xử lý (Pitfalls & Solutions)

| Thử thách kỹ thuật | Nguyên nhân | Giải pháp kiến trúc tối ưu |
| :--- | :--- | :--- |
| **Một trong hai máy mất kết nối mạng** | Tắt Wi-Fi, mất sóng Hotspot, tuột cáp USB khiến đường truyền vật lý bị cắt đứt; gói tin không thể luân chuyển. | **Nguyên lý:** Mạng là môi trường truyền tin, mất mạng = mất gói tin. Socket báo `WSAECONNRESET` hoặc timeout.<br>**Khắc phục:** Bắt buộc duy trì liên kết mạng cục bộ thông suốt giữa 2 máy. Web client tích hợp chỉ báo trạng thái offline và tự động kết nối lại (`auto-reconnect`); Server tự hủy tiến trình shell mồ côi khi socket đứt. |
| **Treo luồng khi đọc Pipe (`ReadFile` Blocking)** | Lệnh chạy xong nhưng Pipe không đóng, `ReadFile` sẽ block vĩnh viễn luồng mạng. | Sử dụng `PeekNamedPipe` kiểm tra số byte có sẵn trong buffer trước khi gọi `ReadFile`. Nếu không có dữ liệu và tiến trình đã thoát (`WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0`), kết thúc stream. |
| **Mất Hotspot IP khi điện thoại phát sóng** | Khi mất điện, điện thoại phát Hotspot cho PC, card Wi-Fi PC nhận IP nhưng có thể không có Default Gateway ra Internet. | Tinh chỉnh hàm `detectBestLANIP()`: không bỏ qua adapter nếu thiếu Gateway trong kịch bản Hotspot/Ad-hoc; ưu tiên adapter Wi-Fi kết nối trực tiếp dải `192.168.43.x` (Android) hoặc `172.20.10.x` (iOS). |
| **Ký tự tiếng Việt & ANSI Color bị vỡ** | Lệnh Windows xuất UTF-8 hoặc CP437/CP850 khiến trình duyệt hiển thị lỗi ký tự `?` hoặc ``. | Luôn gọi `chcp 65001` trong sub-shell và chuyển đổi output sang UTF-8 hợp lệ trước khi gửi qua HTTP Stream. |
| **Xung đột Firewall Windows** | Windows Firewall tự động chặn kết nối Inbound tới cổng lạ từ mạng Private/Public. | Tự động tạo Firewall Rule bằng API `INetFwPolicy2` hoặc lệnh `netsh advfirewall` khi mở Web CMD và xóa rule khi đóng ứng dụng. |

---

## VI. Định hướng Tích hợp vào Codebase Hiện tại

1. **Tận dụng `LocalDrop`:**
   * Mở rộng lớp `LocalDrop` thành `WebRemoteService`.
   * Thêm Route `/terminal`, `/api/exec`, `/api/stream`, `/api/action`.
2. **Tích hợp `SystemCore`:**
   * Tái sử dụng `runRawCommand` và `SystemCore::isElevated()`.
   * Bổ sung API giám sát phần cứng: `% Pin`, `Nhiệt độ`, `RAM khả dụng` trả về định dạng JSON nhỏ gọn cho Dashboard.
3. **Phân quyền hành động:**
   * Cho phép đặt cờ **Read-Only / Safe Mode** (chỉ cho phép dọn rác, soi pin, không cho chạy lệnh xóa format ổ đĩa tự do).

---

> *Tài liệu được biên soạn làm cơ sở phân tích kiến trúc và định hướng triển khai module Web Remote cho CMD BOX.*
