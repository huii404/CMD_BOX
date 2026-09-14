# Sổ tay lệnh Windows CMD

Tài liệu tham khảo nhanh cho **CMD BOX** và các lần bảo trì Windows sau này. Các ví dụ dành cho **Command Prompt (`cmd.exe`) trên Windows 10/11**; thay đường dẫn, tên dịch vụ, PID và tên tác vụ bằng giá trị trên máy của bạn. Đây là danh mục để tra cứu, **không phải script chạy hàng loạt**.

**Ký hiệu:** `Xem` = chỉ đọc; `Ghi` = tạo/sửa dữ liệu hoặc cấu hình; `Admin` = thường cần mở CMD bằng **Run as administrator**. Một số lệnh đọc cũng có thể bị từ chối nếu tài nguyên được bảo vệ. Đặt đường dẫn có dấu cách trong dấu `"..."`; dùng `lệnh /?` để xem đầy đủ cú pháp trước khi tự động hóa.

## 1. Tệp, thư mục và tìm kiếm

| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `cd /d D:\DuLieu` | Chuyển cả ổ đĩa và thư mục làm việc. | Xem |
| `dir /a /o:n "D:\DuLieu"` | Liệt kê cả tệp ẩn, sắp theo tên. | Xem |
| `tree "D:\DuLieu" /f` | Xem cây thư mục kèm tệp. | Xem |
| `where git` | Tìm vị trí chương trình trong `PATH`. | Xem |
| `type "D:\DuLieu\log.txt"` | In nội dung tệp văn bản. | Xem |
| `more "D:\DuLieu\log.txt"` | Đọc tệp dài từng màn hình. | Xem |
| `findstr /s /n /i "error" "D:\Logs\*.log"` | Tìm chữ trong nhiều tệp, hiện số dòng. | Xem |
| `fc "D:\a.txt" "D:\b.txt"` | So sánh hai tệp văn bản. | Xem |
| `attrib "D:\DuLieu\file.txt"` | Xem thuộc tính ẩn/chỉ đọc/hệ thống. | Xem |
| `copy "D:\a.txt" "E:\Backup\a.txt"` | Sao chép một tệp. | Ghi |
| `robocopy "D:\Data" "E:\Backup" /E /R:2 /W:2` | Sao chép cây thư mục, thử lại lỗi tối đa hai lần. | Ghi |
| `robocopy "D:\Data" "E:\Backup" /E /L` | Xem trước danh sách sao chép mà không chép. | Xem |
| `icacls "D:\Data"` | Xem quyền truy cập NTFS. | Xem |
| `cipher /c "D:\Data\file.txt"` | Xem trạng thái mã hóa EFS của tệp. | Xem |
| `compact /q "D:\Data"` | Xem trạng thái nén NTFS. | Xem |

**Lưu ý Robocopy:** Mã thoát `0–7` không nhất thiết là lỗi; từ `8` trở lên nghĩa là có ít nhất một lỗi sao chép. `/MIR` đồng bộ gương và **xóa tệp ở đích** nếu không còn ở nguồn; xem trước với `/L` và kiểm tra thật kỹ hai đường dẫn.

## 2. Mạng và kết nối

| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `ipconfig /all` | Xem IP, gateway, DNS, DHCP và thông tin card mạng. | Xem |
| `ipconfig /displaydns` | Xem bộ nhớ đệm DNS cục bộ. | Xem |
| `ipconfig /flushdns` | Xóa cache DNS để phân giải lại tên miền. | Ghi |
| `ping 1.1.1.1` | Kiểm tra kết nối IP và độ trễ ICMP. | Xem |
| `ping example.com` | Kiểm tra cả phân giải tên và kết nối ICMP. | Xem |
| `tracert example.com` | Xem các hop trên đường đến máy đích. | Xem |
| `pathping example.com` | Đo mất gói trên các hop; chạy lâu hơn `tracert`. | Xem |
| `nslookup example.com` | Hỏi DNS về bản ghi của tên miền. | Xem |
| `netstat -ano` | Xem kết nối/cổng đang nghe và PID. | Xem |
| `arp -a` | Xem bảng ánh xạ IPv4–MAC. | Xem |
| `route print` | Xem bảng định tuyến. | Xem |
| `getmac /v` | Xem địa chỉ MAC theo giao diện mạng. | Xem |
| `netsh interface show interface` | Xem tên và trạng thái giao diện mạng. | Xem |
| `netsh wlan show interfaces` | Xem trạng thái Wi-Fi hiện tại. | Xem |
| `netsh advfirewall show allprofiles` | Xem cấu hình các profile tường lửa. | Xem |
| `curl.exe -I https://example.com` | Xem HTTP response headers; máy phải có `curl.exe`. | Xem |
| `netsh winsock reset` | Đặt lại Winsock khi lỗi socket; thường cần khởi động lại. | Ghi, Admin |
| `netsh int ip reset` | Đặt lại cấu hình TCP/IP; thường cần khởi động lại. | Ghi, Admin |

Không nên tự động chạy `ipconfig /release` trên máy đang điều khiển từ xa: kết nối có thể mất trước khi chạy được `/renew`. Lệnh `ping` có thể thất bại vì máy đích chặn ICMP dù dịch vụ HTTP vẫn hoạt động.

## 3. Tiến trình, dịch vụ và tác vụ hẹn giờ

| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `tasklist` | Liệt kê tiến trình đang chạy. | Xem |
| `tasklist /svc` | Xem dịch vụ nằm trong mỗi tiến trình. | Xem |
| `taskkill /PID 1234` | Yêu cầu đóng tiến trình theo PID; kiểm tra PID trước. | Ghi; có thể cần Admin |
| `sc.exe query wuauserv` | Xem trạng thái dịch vụ Windows Update. | Xem |
| `sc.exe qc wuauserv` | Xem kiểu khởi động, đường dẫn thực thi và phụ thuộc của dịch vụ. | Xem |
| `sc.exe query state= all` | Liệt kê cả dịch vụ đang chạy và đã dừng. | Xem |
| `net start` | Liệt kê dịch vụ đang chạy. | Xem |
| `schtasks /query /fo LIST /v` | Xem chi tiết các tác vụ theo lịch. | Xem |
| `schtasks /query /tn "\Microsoft\Windows\Defrag\ScheduledDefrag"` | Xem một tác vụ cụ thể. | Xem |

`sc.exe` được viết đầy đủ để phân biệt với alias `sc` của PowerShell khi sao chép ví dụ sang cửa sổ PowerShell. Thao tác `sc.exe stop/config` hoặc `schtasks /run/change/delete` thay đổi trạng thái hệ thống; kiểm tra phụ thuộc, tên nội bộ và mục đích trước khi dùng.

## 4. Hệ thống, ổ đĩa và sửa lỗi

| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `ver` | Xem phiên bản Windows từ CMD. | Xem |
| `systeminfo` | Xem bản dựng, phần cứng, hotfix và thời gian khởi động. | Xem |
| `hostname` | Xem tên máy. | Xem |
| `whoami /all` | Xem tài khoản, nhóm và đặc quyền của phiên hiện tại. | Xem |
| `driverquery /v` | Xem driver đã cài và thông tin chi tiết. | Xem |
| `powercfg /a` | Xem các trạng thái ngủ máy hỗ trợ. | Xem |
| `powercfg /batteryreport /output "D:\battery.html"` | Tạo báo cáo pin HTML tại đường dẫn chỉ định. | Ghi |
| `fsutil volume diskfree C:` | Xem dung lượng trống của ổ. | Xem |
| `chkdsk C:` | Kiểm tra và báo lỗi hệ thống tệp, chưa sửa. | Xem |
| `sfc /verifyonly` | Kiểm tra tệp hệ thống, chưa sửa. | Xem, Admin |
| `sfc /scannow` | Quét và cố sửa tệp hệ thống được bảo vệ. | Ghi, Admin |
| `DISM /Online /Cleanup-Image /ScanHealth` | Quét hư hỏng kho thành phần của Windows đang chạy. | Xem, Admin |
| `DISM /Online /Cleanup-Image /RestoreHealth` | Sửa kho thành phần; có thể cần nguồn sửa chữa/Windows Update. | Ghi, Admin |
| `manage-bde -status` | Xem trạng thái mã hóa BitLocker của các ổ. | Xem; có thể cần Admin |
| `bcdedit /enum` | Xem cấu hình khởi động BCD. | Xem, Admin |
| `cleanmgr` | Mở công cụ dọn đĩa; chỉ xóa khi xác nhận trong giao diện. | Tương tác |

`chkdsk /f`, `bcdedit /set`, `bcdboot` và `diskpart` có thể thay đổi ổ/khả năng khởi động. Chỉ dùng sau khi xác định đúng ổ, sao lưu và có quy trình khôi phục.

## 5. Nhật ký, tài khoản, quyền và registry

| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `wevtutil el` | Liệt kê tên các kênh Event Log. | Xem |
| `wevtutil qe System /c:10 /rd:true /f:text` | Đọc 10 sự kiện System mới nhất. | Xem; có thể cần Admin |
| `reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run"` | Xem mục tự khởi động của người dùng hiện tại. | Xem |
| `reg export "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" "D:\Run-backup.reg"` | Sao lưu một nhánh registry ra tệp. | Ghi |
| `net user` | Liệt kê tài khoản cục bộ. | Xem |
| `net localgroup` | Liệt kê nhóm cục bộ. | Xem |
| `gpresult /r` | Xem chính sách nhóm áp dụng cho phiên hiện tại. | Xem |
| `icacls "D:\Data\file.txt"` | Xem ACL của tệp. | Xem |
| `certutil -hashfile "D:\setup.exe" SHA256` | Tính SHA-256 để đối chiếu tệp tải về. | Xem |
| `auditpol /get /category:*` | Xem chính sách ghi nhật ký bảo mật. | Xem; có thể cần Admin |

`takeown` và `icacls /grant` thay đổi quyền sở hữu/ACL; `reg add/delete` thay đổi cấu hình; `wevtutil cl` **xóa nhật ký sự kiện**. Khi tích hợp vào CMD BOX, nên hiển thị đích và hậu quả, sao lưu nếu phù hợp, rồi mới chạy.

## 6. Viết file `.bat` và kiểm tra lỗi

| Cú pháp mẫu | Tác dụng |
| --- | --- |
| `@echo off` | Ẩn việc in từng lệnh trong batch. |
| `set "OUT=D:\Logs"` | Gán biến, tránh vô tình chứa khoảng trắng cuối. |
| `setlocal` / `endlocal` | Giới hạn phạm vi thay đổi biến môi trường của batch. |
| `if exist "D:\Data\file.txt" echo Found` | Chỉ làm việc nếu tệp tồn tại. |
| `if errorlevel 1 echo Failed` | Kiểm tra mã thoát **từ ngưỡng 1 trở lên**, không chỉ bằng 1. |
| `call "D:\Scripts\step.bat"` | Gọi batch khác rồi quay lại batch hiện tại. |
| `timeout /t 5 /nobreak` | Chờ 5 giây. |
| `echo Done > "D:\Logs\run.log"` | Tạo/ghi đè tệp log. Dùng `>>` để ghi nối tiếp. |
| `lệnh 1>"D:\Logs\out.log" 2>&1` | Ghi cả stdout và stderr vào cùng tệp. |
| `lệnh_a && lệnh_b` | Chỉ chạy B khi A thành công. |
| `lệnh_a || echo Failed` | Xử lý khi A báo lỗi. |
| `exit /b 1` | Trả mã lỗi 1 từ batch/hàm `call`. |

Trong file `.bat`, biến vòng `for` dùng `%%A`; gõ trực tiếp trong CMD dùng `%A`. Các ký tự `>`, `|`, `&`, `&&`, `||`, biến `%VAR%` và `for` cần được **CMD diễn giải**; khi gọi từ C++ bằng `CreateProcess`, hãy chỉ định `cmd.exe /c` hoặc chạy tệp `.bat` phù hợp. Kiểm tra mã thoát của từng bước; đừng coi việc khởi tạo tiến trình thành công là lệnh đã làm xong và thành công.

Với nhiều thao tác **thực sự cần Admin** trong cùng một quy trình, có thể tạo một `.bat` tạm có các bước, kiểm tra lỗi từng bước và yêu cầu nâng quyền **một lần** để chạy batch. Chỉ gom các lệnh liên quan, hiển thị nội dung/đích trước khi chạy và tránh đặt dữ liệu đầu vào chưa kiểm soát trực tiếp vào chuỗi CMD.

## Nguồn tra cứu

- [Danh mục Windows commands A–Z của Microsoft Learn](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/windows-commands) — điểm bắt đầu để mở tài liệu từng lệnh và xem phiên bản Windows hỗ trợ.
- [Robocopy và mã thoát](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/robocopy), [ipconfig](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/ipconfig), [netsh](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/netsh).
- [sc.exe query](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/sc-query), [schtasks query](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/schtasks-query), [wevtutil](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/wevtutil).
- [SFC](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/sfc), [BCDEdit](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/bcdedit), [manage-bde](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/manage-bde), [icacls](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/icacls).

Tài liệu Microsoft là nguồn chuẩn khi cần cú pháp đầy đủ; một số tùy chọn thay đổi theo phiên bản Windows. README này chỉ tổng hợp những cách dùng phổ biến, chưa thay thế tài liệu của từng lệnh.
