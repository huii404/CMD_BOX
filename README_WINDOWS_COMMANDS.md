# Bổ sung sổ tay lệnh Windows CMD (đợt 2)

Nối tiếp `README_WINDOWS_COMMANDS.md` và `..._BOSUNG.md`, chỉ có nội dung **chưa xuất hiện** ở hai file đó. Ký hiệu như cũ: `Xem` / `Ghi` / `Admin` / **`Nguy hiểm`**.

Mục lục: A. Lệnh mới · B. Nhóm chức năng mới (B19–B32) · C. Giải mã lỗi · D. Công cụ lọc output · E. Dữ liệu mẫu JSON cho app

---

## A. LỆNH MỚI

### A1. Tệp, tìm kiếm, quyền
| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `where /r "D:\" *.pdf` | Tìm tệp theo tên/đuôi trong cả cây thư mục. | Xem |
| `powershell -NoProfile -c "Get-ChildItem D:\ -Recurse -File -EA 0 \| Sort Length -Desc \| Select -First 20 FullName,Length"` | 20 tệp lớn nhất trên ổ D. | Xem |
| `find /c /v "" "D:\log.txt"` | Đếm số dòng của tệp. | Xem |
| `fc /b "a.bin" "b.bin"` | So sánh hai tệp nhị phân. | Xem |
| `dir /r "D:\file.exe"` | Hiện luồng ẩn `Zone.Identifier` (tệp tải từ Internet). | Xem |
| `powershell -NoProfile -c "Unblock-File 'D:\file.exe'"` | Bỏ cờ "tải từ Internet" (chỉ với tệp đã tin cậy). | Ghi |
| `copy /b "a.part1"+"a.part2" "a.full"` | Ghép tệp nhị phân. | Ghi |
| `robocopy "D:\A" "E:\A" /E /MT:16 /XD node_modules .git /XF *.tmp` | Chép đa luồng, bỏ qua thư mục/tệp chỉ định. | Ghi |
| `robocopy "D:\A" "E:\A" /E /MOVE` | Chép rồi **xóa nguồn**. | Ghi, **Nguy hiểm** |
| `icacls "D:\Data" /save "D:\acl.txt" /t /c` | Sao lưu quyền (ACL) trước khi sửa. | Ghi |
| `icacls "D:\" /restore "D:\acl.txt"` | Khôi phục ACL đã lưu (trỏ vào thư mục cha). | Ghi, Admin |
| `powershell -NoProfile -c "Clear-RecycleBin -Force"` | Dọn thùng rác mọi ổ. | Ghi, **Nguy hiểm** |

### A2. Mạng bổ sung
| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `netsh interface set interface name="Wi-Fi" admin=disabled` / `admin=enabled` | Tắt/bật card mạng (làm mới kết nối). | Ghi, Admin |
| `netsh interface ip show dnsservers` | Xem DNS đang đặt cho từng card. | Xem |
| `netsh wlan show drivers` | Xem driver Wi-Fi, băng tần hỗ trợ (2.4/5GHz, chuẩn 802.11). | Xem |
| `netsh wlan delete profile name="TenWifi"` | Quên một mạng Wi-Fi đã lưu. | Ghi |
| `netsh wlan export profile name="TenWifi" key=clear folder="D:\wifi"` | Xuất hồ sơ Wi-Fi để chuyển máy (**tệp chứa mật khẩu rõ**, cần bảo vệ). | Ghi |
| `netsh wlan add profile filename="D:\wifi\Wi-Fi-TenWifi.xml"` | Nạp lại hồ sơ Wi-Fi. | Ghi |
| `netsh int tcp show global` | Xem tham số TCP (autotuning, RSS...). | Xem |
| `ipconfig /registerdns` | Đăng ký lại tên máy lên DNS (mạng công ty). | Ghi |
| `netsh advfirewall reset` | Đưa tường lửa về mặc định, **mất mọi luật tự thêm**. | Ghi, Admin, **Nguy hiểm** |
| `curl.exe -L -o "file.zip" "https://..."` | Tải tệp. | Ghi |
| `curl.exe -s -o nul -w "%{http_code}" https://example.com` | Chỉ lấy mã HTTP (kiểm tra web sống/chết). | Xem |
| `curl.exe -L -s -o nul -w "%{speed_download}" "https://speed.cloudflare.com/__down?bytes=25000000"` | Đo tốc độ tải (byte/giây). | Xem |
| `powershell -NoProfile -c "Get-NetAdapter \| select Name,Status,LinkSpeed,MacAddress"` | Danh sách card mạng gọn. | Xem |
| `powershell -NoProfile -c "Get-NetTCPConnection -State Listen \| select LocalAddress,LocalPort,OwningProcess"` | Cổng đang nghe kèm PID. | Xem |
| `mstsc /v:TenMay` | Mở Remote Desktop tới máy khác. | Xem |

### A3. Thời gian, vùng, nguồn điện
| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `w32tm /query /status` | Trạng thái đồng bộ giờ, nguồn giờ. | Xem |
| `w32tm /resync` | Đồng bộ giờ ngay (giờ sai gây lỗi HTTPS/chứng chỉ). | Ghi, Admin |
| `w32tm /config /syncfromflags:manual /manualpeerlist:"time.windows.com" /update` | Đặt máy chủ giờ. | Ghi, Admin |
| `tzutil /g` / `tzutil /l` | Xem múi giờ hiện tại / danh sách múi giờ. | Xem |
| `tzutil /s "SE Asia Standard Time"` | Đặt múi giờ (Việt Nam). | Ghi, Admin |
| `powercfg /change monitor-timeout-ac 10` | Tắt màn hình sau 10 phút khi cắm điện (`-dc` = chạy pin). | Ghi |
| `powercfg /change standby-timeout-ac 0` | Không bao giờ ngủ khi cắm điện (`0` = tắt). | Ghi |
| `powershell -NoProfile -c "(Get-Date)-(gcim Win32_OperatingSystem).LastBootUpTime"` | Máy đã chạy liên tục bao lâu (uptime). | Xem |

### A4. Driver, thiết bị, ứng dụng
| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `pnputil /enum-devices /problem` | Liệt kê thiết bị đang báo lỗi (dấu chấm than). | Xem, Admin |
| `pnputil /enum-drivers` | Danh sách driver trong kho. | Xem, Admin |
| `pnputil /export-driver * "D:\Drivers"` | Sao lưu toàn bộ driver bên thứ ba (trước khi cài lại Windows). | Ghi, Admin |
| `driverquery /si` | Kiểm tra driver có chữ ký số hay không. | Xem |
| `powershell -NoProfile -c "Get-Volume"` | Các phân vùng, hệ tệp, dung lượng, tình trạng. | Xem |
| `powershell -NoProfile -c "Optimize-Volume -DriveLetter C -ReTrim -Verbose"` | TRIM ổ SSD. | Ghi, Admin |
| `powershell -NoProfile -c "Repair-Volume -DriveLetter C -Scan"` | Quét lỗi ổ đĩa khi đang dùng, không cần khởi động lại. | Xem, Admin |
| `powershell -NoProfile -c "Get-Printer \| select Name,PrinterStatus,PortName"` | Danh sách máy in. | Xem |
| `net stop spooler` + `del /q /f /s "%SystemRoot%\System32\spool\PRINTERS\*"` + `net start spooler` | Xóa hàng đợi in bị kẹt. | Ghi, Admin |
| `net stop audiosrv /y` + `net start audiosrv` | Khởi động lại dịch vụ âm thanh (mất tiếng). | Ghi, Admin |
| `net stop bthserv` + `net start bthserv` | Khởi động lại dịch vụ Bluetooth. | Ghi, Admin |
| `wsreset.exe` | Xóa cache Microsoft Store. | Ghi |
| `reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall" /s /v DisplayName` | Liệt kê phần mềm đã cài (không dùng `wmic product`, chậm và gây cấu hình lại MSI). | Xem |
| `powershell -NoProfile -c "Get-AppxPackage *xbox* \| Remove-AppxPackage"` | Gỡ ứng dụng Store (ví dụ Xbox). | Ghi |
| `winget export -o "D:\apps.json"` | Xuất danh sách ứng dụng đã cài. | Ghi |
| `winget import -i "D:\apps.json" --accept-package-agreements --accept-source-agreements` | Cài lại hàng loạt từ danh sách. | Ghi |
| `wsl --list --verbose` / `wsl --shutdown` / `wsl --update` | Xem/tắt/cập nhật WSL. | Xem / Ghi |
| `wsl --export Ubuntu "D:\ubuntu.tar"` | Sao lưu một bản WSL. | Ghi |

### A5. Nhật ký lỗi, treo máy, BSOD
| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `wevtutil qe System /q:"*[System[(EventID=41 or EventID=6008)]]" /c:5 /rd:true /f:text` | Lần tắt máy bất thường gần nhất (mất điện, treo, BSOD). | Xem, Admin |
| `wevtutil qe System /q:"*[System[(EventID=1001)]]" /c:3 /rd:true /f:text` | Sự kiện BugCheck (mã BSOD). | Xem, Admin |
| `wevtutil qe Application /q:"*[System[(EventID=1000)]]" /c:5 /rd:true /f:text` | Ứng dụng nào vừa crash, module lỗi. | Xem |
| `wevtutil qe System /q:"*[System[(EventID=7034 or EventID=7031)]]" /c:5 /rd:true /f:text` | Dịch vụ vừa sập bất ngờ. | Xem |
| `dir "C:\Windows\Minidump" /o-d` | Danh sách tệp dump BSOD mới nhất. | Xem, Admin |
| `wevtutil qe Security /q:"*[System[(EventID=4625)]]" /c:10 /rd:true /f:text` | 10 lần đăng nhập thất bại gần nhất (phát hiện dò mật khẩu). | Xem, Admin |
| `wevtutil epl System "D:\System.evtx"` | Xuất nhật ký ra tệp để gửi người hỗ trợ. | Ghi |

### A6. Tài khoản, truy cập từ xa
| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `net user TenMoi * /add` | Tạo tài khoản (hỏi mật khẩu, không để lộ trong lịch sử lệnh). | Ghi, Admin |
| `net user TenUser /active:no` | Vô hiệu hóa tài khoản (an toàn hơn xóa). | Ghi, Admin |
| `net user TenUser /delete` | Xóa tài khoản. | Ghi, Admin, **Nguy hiểm** |
| `net localgroup administrators TenUser /add` | Cấp quyền Admin cho tài khoản. | Ghi, Admin |
| `qwinsta` | Liệt kê phiên Remote Desktop/console. | Xem |
| `reg query "HKLM\SYSTEM\CurrentControlSet\Control\Terminal Server" /v fDenyTSConnections` | Remote Desktop đang bật không (`0` = bật, `1` = tắt). | Xem |
| `reg add "HKLM\SYSTEM\CurrentControlSet\Control\Terminal Server" /v fDenyTSConnections /t REG_DWORD /d 1 /f` | Tắt Remote Desktop (đổi `/d 0` để bật, kèm mở tường lửa và dùng NLA). | Ghi, Admin |

### A7. Dành cho lập trình viên (C++/C#/Web)
| Lệnh mẫu | Tác dụng | Mức |
| --- | --- | --- |
| `dotnet --list-sdks` / `dotnet --list-runtimes` | .NET SDK/runtime đã cài. | Xem |
| `reg query "HKLM\SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full" /v Release` | Bản .NET Framework 4.x (số Release, tra bảng của Microsoft). | Xem |
| `"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath` | Tìm thư mục Visual Studio mới nhất (cho script build). | Xem |
| `dumpbin /dependents "app.exe"` | DLL mà exe phụ thuộc (chạy trong *Developer Command Prompt*). | Xem |
| `tasklist /m "*.dll" /fi "imagename eq app.exe"` | DLL đang nạp trong tiến trình app. | Xem |
| `start /min "" "app.exe"` | Chạy app thu nhỏ. | Ghi |
| `where python node dotnet git cmake` | Kiểm tra công cụ nào có trong `PATH`. | Xem |

---

## B. NHÓM CHỨC NĂNG MỚI

### B19. Máy tự khởi động lại / màn hình xanh (BSOD)
`wevtutil ... EventID=41 or 6008` (tắt bất thường) → `EventID=1001` (mã BugCheck) → `dir C:\Windows\Minidump /o-d` (có dump không) → `pnputil /enum-devices /problem` (thiết bị lỗi) → `driverquery /si` (driver không ký) → `sfc /scannow` + `DISM /RestoreHealth` → `mdsched` (kiểm tra RAM).
Cách đọc: mã BugCheck + tên driver trong nhật ký thường chỉ ra thủ phạm; cập nhật hoặc gỡ driver đó trước khi cài lại Windows.

### B20. Máy in kẹt lệnh
`powershell Get-Printer` → `net stop spooler` → xóa `%SystemRoot%\System32\spool\PRINTERS\*` → `net start spooler`. Admin; nếu vẫn lỗi, xem driver máy in qua `pnputil /enum-drivers`.

### B21. Mất tiếng / Bluetooth không nhận
`net stop audiosrv /y` → `net start audiosrv`; Bluetooth: `net stop bthserv` → `net start bthserv` → `pnputil /enum-devices /problem` để xem driver có lỗi không. Nếu app mất âm riêng, kiểm tra `tasklist /fi "imagename eq tenapp.exe"`.

### B22. Giờ máy sai, web báo lỗi chứng chỉ
`w32tm /query /status` → `tzutil /g` (đúng múi giờ chưa) → `w32tm /resync` → nếu thất bại: `net stop w32time` + `net start w32time` + `w32tm /resync`. Giờ lệch hơn vài phút là nguyên nhân phổ biến của lỗi HTTPS.

### B23. Thiết bị / driver lỗi và sao lưu driver
`pnputil /enum-devices /problem` → `driverquery /si` → trước khi cài lại máy: `pnputil /export-driver * "D:\Drivers"` → cài lại: `pnputil /add-driver "D:\Drivers\*.inf" /subdirs /install` (Admin).

### B24. Microsoft Store / ứng dụng Store lỗi
`wsreset.exe` → `winget source reset --force` → `winget source update` → nếu app cụ thể lỗi: `winget uninstall` rồi `winget install` lại. Store không mở: kiểm tra `sfc /scannow` và `DISM /RestoreHealth` (B2).

### B25. Bật / kiểm tra Remote Desktop an toàn
Kiểm tra: `reg query ... fDenyTSConnections` + `netstat -ano | findstr :3389` + `qwinsta`. Bật: đặt `fDenyTSConnections=0` + bật nhóm luật tường lửa "Remote Desktop" + dùng mật khẩu mạnh/NLA + không mở cổng 3389 ra Internet. Tắt lại bằng `/d 1`. Tên nhóm luật phụ thuộc ngôn ngữ Windows, app nên liệt kê bằng `show rule name=all` rồi lọc.

### B26. Kiểm tra đăng nhập thất bại (phòng thủ)
`wevtutil qe Security ... EventID=4625` (Admin) → xem nguồn IP/tài khoản trong output → `net user` + `net localgroup administrators` (có tài khoản lạ không) → `quser` (ai đang đăng nhập) → nếu thấy bất thường: `net user TenUser /active:no` + đổi mật khẩu.

### B27. Chuyển sang máy mới (di cư)
`winget export` (danh sách app) → `pnputil /export-driver` (nếu cùng phần cứng) → `netsh wlan export profile key=clear` (Wi-Fi, bảo vệ tệp) → `robocopy /E /MT:16` dữ liệu → `reg export` các nhánh cấu hình cần → máy mới: `winget import` + `netsh wlan add profile`.

### B28. Tìm tệp lớn / dọn dữ liệu thừa
`powershell Get-ChildItem ... Sort Length -Desc | Select -First 20` → với từng tệp xem `dir /r` + ngày sửa → chuyển đi bằng `robocopy /MOVE` sau khi có bản sao → `Clear-RecycleBin`. Luôn cho người dùng duyệt danh sách trước khi xóa.

### B29. Đo chất lượng mạng
`ping -n 30 1.1.1.1` (mất gói, jitter) → `pathping -q 50 example.com` (hop nào rớt gói) → `curl ... %{speed_download}` (tốc độ) → `netsh wlan show interfaces` (tín hiệu Wi-Fi %) → `netsh wlan show wlanreport` (lịch sử rớt mạng).

### B30. Quét thiết bị trong mạng LAN của mình
`ipconfig` (xác định dải, ví dụ 192.168.1.x) → `for /l %i in (1,1,254) do @ping -n 1 -w 100 192.168.1.%i >nul` (trong `.bat` dùng `%%i`) → `arp -a` (IP ↔ MAC các máy phản hồi). Chỉ dùng trên mạng của chính mình.

### B31. Đổi DNS nhanh theo hồ sơ
Cloudflare: `netsh interface ip set dns name="Wi-Fi" static 1.1.1.1` + `add dns ... 1.0.0.1 index=2`; Google: `8.8.8.8` + `8.8.4.4`; tự động: `... dhcp`. Sau khi đổi: `ipconfig /flushdns` → `nslookup example.com`. Lưu tên card thực (`netsh interface show interface`).

### B32. Môi trường dev có đủ chưa
`where python node dotnet git cmake` → `dotnet --list-sdks` → `vswhere -latest` → `wsl --list --verbose` → `reg query ...NDP\v4\Full /v Release`. Xuất kết quả thành bảng "Có / Thiếu" để người dùng biết cần cài gì bằng `winget install`.

---

## C. GIẢI MÃ LỖI (rất hữu ích để app báo lỗi dễ hiểu)

| Lệnh | Tác dụng |
| --- | --- |
| `net helpmsg 5` | Giải thích mã lỗi Win32 (5 = Access denied). |
| `certutil -error 0x80070005` | Giải thích mã HRESULT. |

| Mã | Ý nghĩa | Gợi ý cho app |
| --- | --- | --- |
| 2 / 3 | Không thấy tệp / đường dẫn | Kiểm tra lại đường dẫn, dấu `"..."`. |
| 5 | Bị từ chối truy cập | Cần Admin hoặc tài nguyên đang bị khóa/bảo vệ. |
| 32 | Tệp đang được tiến trình khác dùng | Đóng ứng dụng đó hoặc thử lại. |
| 87 | Tham số không hợp lệ | Sai cú pháp lệnh. |
| 740 | Cần nâng quyền | Chạy lại với verb `runas` (UAC). |
| 1223 | Người dùng từ chối UAC | Không thử lại tự động. |
| 1053 / 1056 / 1058 / 1060 / 1062 | Dịch vụ: timeout / đã chạy / bị tắt / không tồn tại / chưa chạy | Kiểm tra `sc.exe qc` rồi mới quyết định. |
| `0x800f081f`, `0x800f0906` (DISM) | Không tìm thấy nguồn sửa | Cần Internet/Windows Update hoặc ISO, dùng `/Source`. |
| `taskkill` 128 | Không có tiến trình đó | Có thể đã tắt rồi. |
| `findstr`/`find` 1 | Không tìm thấy kết quả khớp | Không phải lỗi hệ thống. |

---

## D. CÔNG CỤ LỌC OUTPUT (dùng sau dấu `|`)

| Lệnh | Tác dụng |
| --- | --- |
| `\| findstr /i "chuoi"` / `findstr /v` | Giữ / loại dòng chứa chuỗi. |
| `\| findstr /r "^[0-9]"` | Lọc theo biểu thức chính quy cơ bản. |
| `\| find /c /v ""` | Đếm số dòng kết quả. |
| `\| sort /r` | Sắp xếp ngược. |
| `\| more` | Xem từng trang. |
| `for /f "skip=3 tokens=1,2" %%A in ('lệnh') do ...` | Bỏ 3 dòng đầu, lấy cột 1 và 2. |
| `\| powershell -NoProfile -c "$input \| Select -First 10"` | Lấy 10 dòng đầu (CMD không có `head`). |

---

## E. DỮ LIỆU MẪU JSON CHO APP

```json
{
  "commands": [
    {
      "id": "net.flushdns",
      "name_vi": "Xóa cache DNS",
      "cmd": "ipconfig /flushdns",
      "group": "mang",
      "level": "ghi",
      "admin": false,
      "undo": null,
      "timeout_s": 10,
      "output": "text"
    },
    {
      "id": "fw.block_app",
      "name_vi": "Chặn ứng dụng ra mạng",
      "cmd": "netsh advfirewall firewall add rule name=\"CMDBOX Block {name}\" dir=out action=block program=\"{path}\" enable=yes",
      "params": [{ "key": "name", "type": "safe_string" }, { "key": "path", "type": "existing_file" }],
      "group": "tuong_lua",
      "level": "ghi",
      "admin": true,
      "undo": "netsh advfirewall firewall delete rule name=\"CMDBOX Block {name}\"",
      "timeout_s": 15,
      "output": "text"
    },
    {
      "id": "proc.list",
      "name_vi": "Danh sách tiến trình",
      "cmd": "tasklist /fo csv /nh",
      "group": "tien_trinh",
      "level": "xem",
      "admin": false,
      "undo": null,
      "timeout_s": 10,
      "output": "csv"
    }
  ],
  "combos": [
    {
      "id": "combo.network_fix",
      "name_vi": "Sửa mạng chậm / mất mạng",
      "steps": ["net.ipconfig_all", "net.ping_ip", "net.ping_name", "net.flushdns", "net.renew", "net.winsock_reset"],
      "stop_on_error": false,
      "needs_restart": true,
      "restore_point": true
    }
  ]
}
```

Quy ước tham số: `safe_string` (chỉ chữ/số/dấu cách/gạch), `existing_file` (đường dẫn phải tồn tại), `int` (ép kiểu số, dùng cho PID/cổng). App thay `{name}`, `{path}` sau khi kiểm tra, không nối chuỗi thô.
