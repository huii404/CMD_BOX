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
| `attrib -h -r -s "D:\DuLieu\*.*" /s /d` | Gỡ bỏ thuộc tính Ẩn, Chỉ đọc, Hệ thống hàng loạt (sửa lỗi virus ẩn file). | Ghi |
| `copy "D:\a.txt" "E:\Backup\a.txt"` | Sao chép một tệp. | Ghi |
| `robocopy "D:\Data" "E:\Backup" /E /R:2 /W:2` | Sao chép cây thư mục, thử lại lỗi tối đa hai lần. | Ghi |
| `robocopy "D:\Data" "E:\Backup" /E /L` | Xem trước danh sách sao chép mà không chép. | Xem |
| `takeown /f "D:\Khoa" /r /d y` | Chiếm quyền sở hữu (Ownership) thư mục/tệp bị khóa quyền. | Ghi, Admin |
| `icacls "D:\Khoa" /grant administrators:F /t` | Cấp toàn quyền (Full Control) cho Admin với thư mục và file con. | Ghi, Admin |
| `cipher /c "D:\Data\file.txt"` | Xem trạng thái mã hóa EFS của tệp. | Xem |
| `cipher /w:C:\Data` | Ghi đè dọn sạch vùng nhớ trống (Wipe free space) để chống khôi phục file đã xóa. | Ghi |
| `compact /q "D:\Data"` | Xem trạng thái nén NTFS. | Xem |
| `compact /c /s:"D:\Data" /i /exe:lzx` | Nén thư mục bằng thuật toán LZX (tiết kiệm dung lượng cao cho game/app). | Ghi |

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
| `netstat -b -ano` | Xem kết nối kèm tên tệp thực thi tạo ra kết nối đó. | Xem, Admin |
| `arp -a` | Xem bảng ánh xạ IPv4–MAC trên mạng LAN. | Xem |
| `route print` | Xem bảng định tuyến mạng. | Xem |
| `getmac /v` | Xem địa chỉ MAC theo giao diện mạng. | Xem |
| `netsh interface show interface` | Xem tên và trạng thái giao diện mạng. | Xem |
| `netsh wlan show interfaces` | Xem trạng thái Wi-Fi hiện tại (tốc độ kết nối, tín hiệu %). | Xem |
| `netsh wlan show profiles` | Liệt kê tất cả mạng Wi-Fi đã từng kết nối. | Xem |
| `netsh wlan show profile name="TenWifi" key=clear` | Xem mật khẩu Wi-Fi của mạng đã lưu (mục Key Content). | Xem |
| `netsh wlan show networks mode=bssid` | Quét danh sách sóng Wi-Fi xung quanh, chuẩn mã hóa và kênh (channel). | Xem |
| `netsh advfirewall show allprofiles` | Xem cấu hình các profile tường lửa (Domain, Private, Public). | Xem |
| `netsh advfirewall set allprofiles state on` | Bật lại toàn bộ profile tường lửa Windows Firewall. | Ghi, Admin |
| `curl.exe -I https://example.com` | Xem HTTP response headers; máy phải có `curl.exe`. | Xem |
| `netsh winsock reset` | Đặt lại Winsock khi lỗi socket; thường cần khởi động lại. | Ghi, Admin |
| `netsh int ip reset` | Đặt lại cấu hình TCP/IP; thường cần khởi động lại. | Ghi, Admin |

Không nên tự động chạy `ipconfig /release` trên máy đang điều khiển từ xa: kết nối có thể mất trước khi chạy được `/renew`. Lệnh `ping` có thể thất bại vì máy đích chặn ICMP dù dịch vụ HTTP vẫn hoạt động.

## 3. Tiến trình, dịch vụ và tác vụ hẹn giờ

| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `tasklist` | Liệt kê tiến trình đang chạy. | Xem |
| `tasklist /svc` | Xem dịch vụ nằm trong mỗi tiến trình `svchost.exe`. | Xem |
| `tasklist /m ole32.dll` | Tìm các tiến trình đang nạp DLL chỉ định. | Xem |
| `taskkill /PID 1234` | Yêu cầu đóng tiến trình theo PID; kiểm tra PID trước. | Ghi; có thể cần Admin |
| `taskkill /F /T /IM badapp.exe` | Ép buộc tắt tiến trình và toàn bộ cây tiến trình con (`/T`). | Ghi, Admin |
| `sc.exe query wuauserv` | Xem trạng thái dịch vụ Windows Update. | Xem |
| `sc.exe qc wuauserv` | Xem kiểu khởi động, đường dẫn thực thi và phụ thuộc của dịch vụ. | Xem |
| `sc.exe query state= all` | Liệt kê cả dịch vụ đang chạy và đã dừng. | Xem |
| `sc.exe config "TênDịchVụ" start= disabled` | Vô hiệu hóa dịch vụ khởi động cùng Windows (chú ý có dấu cách sau `start=`). | Ghi, Admin |
| `sc.exe config "TênDịchVụ" start= demand` | Đặt dịch vụ khởi động thủ công (Manual). | Ghi, Admin |
| `net start` | Liệt kê dịch vụ đang chạy. | Xem |
| `schtasks /query /fo LIST /v` | Xem chi tiết các tác vụ theo lịch. | Xem |
| `schtasks /query /tn "\Microsoft\Windows\Defrag\ScheduledDefrag"` | Xem một tác vụ cụ thể. | Xem |
| `schtasks /run /tn "TênTask"` | Kích hoạt chạy ngay một task đã lên lịch. | Ghi |
| `schtasks /change /tn "TênTask" /disable` | Tắt một tác vụ hẹn giờ trong Task Scheduler. | Ghi, Admin |

`sc.exe` được viết đầy đủ để phân biệt với alias `sc` của PowerShell khi sao chép ví dụ sang cửa sổ PowerShell. Thao tác `sc.exe stop/config` hoặc `schtasks /run/change/delete` thay đổi trạng thái hệ thống; kiểm tra phụ thuộc, tên nội bộ và mục đích trước khi dùng.

## 4. Hệ thống, ổ đĩa, tối ưu và sửa lỗi

| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `ver` | Xem phiên bản Windows từ CMD. | Xem |
| `systeminfo` | Xem bản dựng, phần cứng, hotfix và thời gian khởi động (Boot Time). | Xem |
| `hostname` | Xem tên máy. | Xem |
| `whoami /all` | Xem tài khoản, nhóm và đặc quyền của phiên hiện tại. | Xem |
| `driverquery /v` | Xem driver đã cài và thông tin chi tiết. | Xem |
| `powercfg /a` | Xem các trạng thái ngủ máy hỗ trợ (Sleep S3, Modern Standby S0). | Xem |
| `powercfg /requests` | Xem ứng dụng/driver nào đang ngăn máy ngủ (Display/System/Execution locks). | Xem |
| `powercfg /batteryreport /output "D:\battery.html"` | Tạo báo cáo pin HTML (đo độ chai pin, số chu kỳ sạc). | Ghi |
| `powercfg /energy /output "D:\energy.html"` | Phân tích 60 giây mức tiêu thụ năng lượng và cảnh báo lỗi phần cứng. | Ghi, Admin |
| `powercfg /h off` | Tắt chế độ ngủ đông và giải phóng tệp `hiberfil.sys` (tiết kiệm vài chục GB ổ C). | Ghi, Admin |
| `powercfg /duplicatescheme e9a42b02-d5df-448d-aa00-03f14749eb61` | Mở khóa gói nguồn ẩn Ultimate Performance (Hiệu năng tối đa). | Ghi, Admin |
| `fsutil volume diskfree C:` | Xem dung lượng trống chính xác của ổ. | Xem |
| `fsutil dirty query C:` | Kiểm tra ổ đĩa có cờ "dirty" (bị lỗi cần kiểm tra khi boot) không. | Xem, Admin |
| `fsutil behavior query DisableDeleteNotify` | Kiểm tra lệnh TRIM trên SSD (`0` = đã bật TRIM, `1` = đang tắt). | Xem, Admin |
| `defrag C: /O` | Tối ưu hóa ổ đĩa (chạy TRIM cho SSD hoặc chống phân mảnh cho HDD). | Ghi, Admin |
| `chkdsk C:` | Kiểm tra và báo lỗi hệ thống tệp ở chế độ chỉ đọc. | Xem |
| `chkdsk C: /f /r` | Lên lịch sửa lỗi hệ thống tệp và quét bad sector khi khởi động lại. | Ghi, Admin |
| `sfc /verifyonly` | Kiểm tra tệp hệ thống, chưa sửa. | Xem, Admin |
| `sfc /scannow` | Quét và tự động phục hồi tệp hệ thống Windows bị hỏng. | Ghi, Admin |
| `DISM /Online /Cleanup-Image /ScanHealth` | Quét hư hỏng kho thành phần (Component Store) của Windows. | Xem, Admin |
| `DISM /Online /Cleanup-Image /RestoreHealth` | Sửa kho thành phần qua Windows Update hoặc bộ cài ISO. | Ghi, Admin |
| `DISM /Online /Cleanup-Image /StartComponentCleanup /ResetBase` | Xóa bỏ các phiên bản cập nhật cũ trong WinSxS (tiết kiệm nhiều GB dung lượng). | Ghi, Admin |
| `vssadmin list shadows` | Xem danh sách bản sao lưu bóng (Volume Shadow Copies / Restore Points). | Xem, Admin |
| `vssadmin resize shadowstorage /for=C: /on=C: /maxsize=5GB` | Giới hạn dung lượng tối đa cho System Restore để tránh đầy ổ đĩa. | Ghi, Admin |
| `manage-bde -status` | Xem trạng thái mã hóa BitLocker và cơ chế bảo vệ của các ổ. | Xem; có thể cần Admin |
| `bcdedit /enum` | Xem cấu hình bộ nạp khởi động BCD. | Xem, Admin |
| `shutdown /r /fw /t 0` | Khởi động lại máy và vào thẳng màn hình cài đặt BIOS/UEFI. | Ghi, Admin |
| `shutdown /r /o /t 0` | Khởi động lại vào menu Khôi phục Nâng cao (Advanced Startup / Safe Mode). | Ghi |
| `cleanmgr /sageset:1` & `cleanmgr /sagerun:1` | Cấu hình và chạy dọn đĩa Disk Cleanup tự động theo kịch bản ngầm. | Ghi |

`chkdsk /f`, `bcdedit /set`, `bcdboot` và `diskpart` có thể thay đổi ổ/khả năng khởi động. Chỉ dùng sau khi xác định đúng ổ, sao lưu và có quy trình khôi phục.

## 5. Truy vấn phần cứng & Hệ thống qua WMIC / PowerShell CIM

Công cụ truy vấn nhanh thông số máy tính qua dòng lệnh (hữu ích để tạo báo cáo chẩn đoán hệ thống tự động trong CMD BOX):

| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `wmic diskdrive get model,serialnumber,status,size` | Kiểm tra tên ổ đĩa, số Serial và trạng thái S.M.A.R.T (`OK` hay lỗi). | Xem |
| `wmic memorychip get capacity,speed,devicelocator,manufacturer` | Xem dung lượng, bus RAM (MHz) và hãng sản xuất của từng thanh RAM. | Xem |
| `wmic cpu get name,numberofcores,numberoflogicalprocessors,maxclockspeed` | Xem tên CPU, số nhân, số luồng và xung nhịp tối đa. | Xem |
| `wmic bios get serialnumber,smbiosbiosversion,manufacturer` | Xem số Serial máy (rất hữu ích với laptop Dell/HP/Lenovo) và phiên bản BIOS. | Xem |
| `wmic baseboard get product,manufacturer,version` | Xem mã bo mạch chủ (Mainboard). | Xem |
| `wmic os get lastbootuptime,installdate` | Xem thời điểm khởi động máy gần nhất và ngày cài đặt Windows. | Xem |
| `wmic logicaldisk where DriveType=3 get deviceid,volumename,freespace,size` | Xem dung lượng trống và tổng dung lượng tất cả các ổ đĩa cố định. | Xem |

*Lưu ý:* Trên Windows 11 mới nhất, Microsoft đang dần thay thế WMIC bằng PowerShell (`Get-CimInstance`). Trong C++, có thể gọi trực tiếp Windows WMI API (`IWbemServices`) để đạt hiệu năng cao nhất mà không cần spawn tiến trình con.

## 6. Nhật ký, tài khoản, bảo mật, chứng chỉ và registry

| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `wevtutil el` | Liệt kê tên các kênh Event Log. | Xem |
| `wevtutil qe System /c:10 /rd:true /f:text` | Đọc 10 sự kiện System mới nhất. | Xem; có thể cần Admin |
| `wevtutil qe System /q:"*[System[(Level=2)]]" /c:5 /rd:true /f:text` | Lọc ra 5 sự kiện Lỗi nghiêm trọng (Level 2 - Error) gần nhất. | Xem, Admin |
| `wevtutil cl "Application"` | Xóa sạch nhật ký sự kiện của một kênh (giải phóng log). | Ghi, Admin |
| `reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run"` | Xem mục tự khởi động của người dùng hiện tại. | Xem |
| `reg query "HKLM\Software\Microsoft\Windows\CurrentVersion\Run"` | Xem mục tự khởi động của toàn hệ thống (All Users). | Xem |
| `reg export "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" "D:\Run-backup.reg"` | Sao lưu một nhánh registry ra tệp `.reg`. | Ghi |
| `reg add "HKCU\Software\TestApp" /v "Enabled" /t REG_DWORD /d 1 /f` | Thêm hoặc ghi đè giá trị registry mà không hỏi lại (`/f`). | Ghi |
| `reg delete "HKCU\Software\TestApp" /v "Enabled" /f` | Xóa giá trị registry chỉ định. | Ghi |
| `net user` | Liệt kê tài khoản người dùng cục bộ. | Xem |
| `net user %username%` | Xem thông tin chi tiết của tài khoản hiện tại (hạn mật khẩu, quyền nhóm). | Xem |
| `net localgroup administrators` | Liệt kê các tài khoản có quyền Quản trị viên (Admin) trên máy. | Xem |
| `gpresult /r` | Xem chính sách nhóm (Group Policy) áp dụng cho phiên hiện tại. | Xem |
| `gpupdate /force` | Làm mới và ép áp dụng lại Group Policy ngay lập tức. | Ghi |
| `certutil -hashfile "D:\setup.exe" SHA256` | Tính mã băm SHA-256 (hoặc MD5, SHA1) để kiểm tra tính toàn vẹn của tệp. | Xem |
| `certutil -urlcache * delete` | Xóa bộ nhớ đệm chứng chỉ số và CRL cache. | Ghi |
| `slmgr.vbs /xpr` | Kiểm tra bản quyền Windows (kích hoạt vĩnh viễn hay có ngày hết hạn KMS). | Xem |
| `slmgr.vbs /dli` | Xem thông tin chi tiết giấy phép bản quyền và phần đuôi Product Key. | Xem |
| `auditpol /get /category:*` | Xem chính sách ghi nhật ký bảo mật. | Xem; có thể cần Admin |

`takeown` và `icacls /grant` thay đổi quyền sở hữu/ACL; `reg add/delete` thay đổi cấu hình; `wevtutil cl` **xóa nhật ký sự kiện**. Khi tích hợp vào CMD BOX, nên hiển thị đích và hậu quả, sao lưu nếu phù hợp, rồi mới chạy.

## 7. Quản lý ứng dụng với WinGet

Trên Windows 10 (bản 1809 trở lên) và Windows 11, `winget` là trình quản lý gói chính thức của Microsoft, rất thích hợp để đưa vào tính năng tự động cập nhật phần mềm trong CMD BOX:

| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `winget list` | Liệt kê toàn bộ phần mềm đã cài đặt trên máy. | Xem |
| `winget search "Google Chrome"` | Tìm kiếm gói cài đặt phần mềm trên kho Microsoft Community. | Xem |
| `winget upgrade` | Liệt kê các phần mềm đã cài đặt có phiên bản mới cần cập nhật. | Xem |
| `winget upgrade --all --include-unknown` | Nâng cấp tất cả phần mềm trên máy lên bản mới nhất. | Ghi |
| `winget install --id Git.Git -e --silent` | Cài đặt ứng dụng ở chế độ im lặng (Silent Install, không hiện wizard). | Ghi |
| `winget uninstall --id ỨngDụng` | Gỡ cài đặt ứng dụng qua ID. | Ghi |

## 8. Viết file `.bat` và kiểm tra lỗi

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
- [WinGet Client Documentation](https://learn.microsoft.com/en-us/windows/package-manager/winget/) — Hướng dẫn cài đặt và tự động hóa cập nhật ứng dụng bằng Windows Package Manager.

Tài liệu Microsoft là nguồn chuẩn khi cần cú pháp đầy đủ; một số tùy chọn thay đổi theo phiên bản Windows. README này chỉ tổng hợp những cách dùng phổ biến, chưa thay thế tài liệu của từng lệnh.
