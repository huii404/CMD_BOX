<# : chooser
@echo off
chcp 65001 >nul
powershell -NoProfile -ExecutionPolicy Bypass -Command "$code = [System.IO.File]::ReadAllText('%~f0', [System.Text.Encoding]::UTF8); & ([ScriptBlock]::Create($code))"
exit /b %errorlevel%
#>

[Console]::OutputEncoding = [Text.Encoding]::UTF8
$OutputEncoding = [Text.Encoding]::UTF8

$ESC = [char]27
$C_RESET  = "$ESC[0m"
$C_BOLD   = "$ESC[1m"
$C_CYAN   = "$ESC[96m"
$C_GREEN  = "$ESC[92m"
$C_YELLOW = "$ESC[93m"
$C_ROSE   = "$ESC[38;2;255;151;177m"
$C_MUTED  = "$ESC[38;2;151;157;184m"
$C_TEXT   = "$ESC[38;2;232;235;247m"
$C_ORCHID = "$ESC[38;2;195;165;255m"

# ==============================================================================
# CẤU TRÚC DANH MỤC ỨNG DỤNG PHÂN THEO NHÓM
# ==============================================================================
$Groups = @(
    [PSCustomObject]@{ Key = "W"; Name = "TRÌNH DUYỆT WEB"; Column = 1; Items = @(
        [PSCustomObject]@{ Id = 1;  Name = "Google Chrome"; Url = "https://dl.google.com/tag/s/appname%3DGoogle%2520Chrome/update2/installers/ChromeSetup.exe"; FileName = "ChromeSetup.exe" },
        [PSCustomObject]@{ Id = 2;  Name = "Cốc Cốc"; Url = "https://files.coccoc.com/browser/coccoc_vi.exe"; FileName = "CocCocSetup.exe" },
        [PSCustomObject]@{ Id = 3;  Name = "Brave Browser"; Url = "https://laptop-updates.brave.com/latest/winx64"; FileName = "BraveSetup.exe" },
        [PSCustomObject]@{ Id = 4;  Name = "Mozilla Firefox"; Url = "https://download.mozilla.org/?product=firefox-latest-ssl&os=win64&lang=vi"; FileName = "FirefoxSetup.exe" },
        [PSCustomObject]@{ Id = 5;  Name = "Opera Browser"; Url = "https://net.geo.opera.com/opera/stable/windows?utm_tryeverything=true"; FileName = "OperaSetup.exe" }
    )},
    [PSCustomObject]@{ Key = "V"; Name = "GÕ TIẾNG VIỆT"; Column = 2; Items = @(
        [PSCustomObject]@{ Id = 6;  Name = "EVKey"; Url = "https://github.com/lamquangminh/EVKey/releases/download/v5.0.4/EVKey.zip"; FileName = "EVKey.zip" },
        [PSCustomObject]@{ Id = 7;  Name = "OpenKey"; Url = "https://github.com/tphan/openkey/releases/latest/download/OpenKey-Windows-x64.zip"; FileName = "OpenKey.zip" },
        [PSCustomObject]@{ Id = 8;  Name = "UniKey"; Url = "https://www.unikey.org/assets/release/unikey43RC5-200929-win64.zip"; FileName = "UniKey.zip" }
    )},
    [PSCustomObject]@{ Key = "C"; Name = "CHAT & LIÊN LẠC"; Column = 2; Items = @(
        [PSCustomObject]@{ Id = 9;  Name = "Zalo PC"; Url = "https://zalo.me/download/zalo-pc"; FileName = "ZaloSetup.exe" },
        [PSCustomObject]@{ Id = 10; Name = "Discord"; Url = "https://discord.com/api/downloads/distributions/app/installers/latest?channel=stable&platform=win&arch=x64"; FileName = "DiscordSetup.exe" },
        [PSCustomObject]@{ Id = 11; Name = "Telegram"; Url = "https://telegram.org/dl/desktop/win64"; FileName = "TelegramSetup.exe" },
        [PSCustomObject]@{ Id = 12; Name = "Zoom"; Url = "https://zoom.us/client/latest/ZoomInstaller.exe"; FileName = "ZoomInstaller.exe" },
        [PSCustomObject]@{ Id = 13; Name = "Skype"; Url = "https://go.skype.com/windows.desktop.download"; FileName = "SkypeSetup.exe" }
    )},
    [PSCustomObject]@{ Key = "F"; Name = "NÉN & QUẢN LÝ TỆP"; Column = 1; Items = @(
        [PSCustomObject]@{ Id = 14; Name = "7-Zip"; Url = "https://www.7-zip.org/a/7z2408-x64.exe"; FileName = "7zipSetup.exe" },
        [PSCustomObject]@{ Id = 15; Name = "WinRAR"; Url = "https://www.rarlab.com/rar/winrar-x64-701.exe"; FileName = "WinRARSetup.exe" },
        [PSCustomObject]@{ Id = 16; Name = "Bandizip"; Url = "https://dl.bandisoft.com/bandizip.std/BANDIZIP-SETUP-STD-ALL.EXE"; FileName = "BandizipSetup.exe" },
        [PSCustomObject]@{ Id = 17; Name = "Everything Search"; Url = "https://www.voidtools.com/Everything-1.4.1.1026.x64-Setup.exe"; FileName = "EverythingSetup.exe" }
    )},
    [PSCustomObject]@{ Key = "M"; Name = "ĐA PHƯƠNG TIỆN & ĐỒ HỌA"; Column = 1; Items = @(
        [PSCustomObject]@{ Id = 18; Name = "VLC Media Player"; Url = "https://get.videolan.org/vlc/last/win64/vlc-3.0.21-win64.exe"; FileName = "VLCSetup.exe" },
        [PSCustomObject]@{ Id = 19; Name = "PotPlayer"; Url = "https://t1.daumcdn.net/potplayer/PotPlayer/Version/Latest/PotPlayerSetup64.exe"; FileName = "PotPlayerSetup64.exe" },
        [PSCustomObject]@{ Id = 20; Name = "Spotify"; Url = "https://download.scdn.co/SpotifySetup.exe"; FileName = "SpotifySetup.exe" },
        [PSCustomObject]@{ Id = 21; Name = "OBS Studio"; Url = "https://cdn-fastly.obsproject.com/downloads/OBS-Studio-30.2.2-Windows-Installer.exe"; FileName = "OBSStudioSetup.exe" },
        [PSCustomObject]@{ Id = 22; Name = "Audacity"; Url = "https://github.com/audacity/audacity/releases/download/Audacity-3.6.4/audacity-win-3.6.4-64bit.exe"; FileName = "AudacitySetup.exe" },
        [PSCustomObject]@{ Id = 23; Name = "HandBrake"; Url = "https://github.com/HandBrake/HandBrake/releases/download/1.8.2/HandBrake-1.8.2-x86_64-Win_GUI.exe"; FileName = "HandBrakeSetup.exe" },
        [PSCustomObject]@{ Id = 24; Name = "ShareX"; Url = "https://github.com/ShareX/ShareX/releases/download/v16.1.0/ShareX-16.1.0-setup.exe"; FileName = "ShareXSetup.exe" }
    )},
    [PSCustomObject]@{ Key = "N"; Name = "MẠNG & ĐIỀU KHIỂN"; Column = 2; Items = @(
        [PSCustomObject]@{ Id = 25; Name = "WARP 1.1.1.1"; Url = "https://1111-releases.cloudflareclient.com/windows/Cloudflare_WARP_Release-x64.msi"; FileName = "CloudflareWARP.msi" },
        [PSCustomObject]@{ Id = 26; Name = "LocalSend"; Url = "https://github.com/localsend/localsend/releases/latest/download/LocalSend-1.16.1-windows-x86-64.exe"; FileName = "LocalSendSetup.exe" },
        [PSCustomObject]@{ Id = 27; Name = "UltraViewer"; Url = "https://ultraviewer.net/vi/UltraViewer_setup_6.6_vi.exe"; FileName = "UltraViewerSetup.exe" },
        [PSCustomObject]@{ Id = 28; Name = "AnyDesk"; Url = "https://download.anydesk.com/AnyDesk.exe"; FileName = "AnyDesk.exe" }
    )},
    [PSCustomObject]@{ Key = "S"; Name = "TIỆN ÍCH HỆ THỐNG"; Column = 2; Items = @(
        [PSCustomObject]@{ Id = 29; Name = "CPU-Z"; Url = "https://download.cpuid.com/cpu-z/cpu-z_2.11-en.exe"; FileName = "CPUZSetup.exe" },
        [PSCustomObject]@{ Id = 30; Name = "CrystalDiskInfo"; Url = "https://github.com/hiyohiyo/CrystalDiskInfo/releases/download/9.3.2/CrystalDiskInfo9_3_2.zip"; FileName = "CrystalDiskInfo.zip" },
        [PSCustomObject]@{ Id = 31; Name = "Geek Uninstaller"; Url = "https://geekuninstaller.com/geek.zip"; FileName = "GeekUninstaller.zip" },
        [PSCustomObject]@{ Id = 32; Name = "Rufus"; Url = "https://github.com/pbatard/rufus/releases/download/v4.5/rufus-4.5.exe"; FileName = "Rufus.exe" },
        [PSCustomObject]@{ Id = 33; Name = "Bitwarden"; Url = "https://vault.bitwarden.com/download/?app=desktop&platform=windows"; FileName = "BitwardenSetup.exe" }
    )},
    [PSCustomObject]@{ Key = "D"; Name = "LẬP TRÌNH & SOẠN THẢO"; Column = 1; Items = @(
        [PSCustomObject]@{ Id = 34; Name = "VS Code"; Url = "https://code.visualstudio.com/sha/download?build=stable&os=win32-x64-user"; FileName = "VSCodeSetup.exe" },
        [PSCustomObject]@{ Id = 35; Name = "Notepad++"; Url = "https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v8.6.7/npp.8.6.7.Installer.x64.exe"; FileName = "NotepadPlusPlusSetup.exe" },
        [PSCustomObject]@{ Id = 36; Name = "Sublime Text"; Url = "https://download.sublimetext.com/sublime_text_build_4180_x64_setup.exe"; FileName = "SublimeTextSetup.exe" },
        [PSCustomObject]@{ Id = 37; Name = "Git for Windows"; Url = "https://github.com/git-for-windows/git/releases/download/v2.45.1.windows.1/Git-2.45.1-64-bit.exe"; FileName = "GitSetup.exe" },
        [PSCustomObject]@{ Id = 38; Name = "Node.js LTS"; Url = "https://nodejs.org/dist/v20.16.0/node-v20.16.0-x64.msi"; FileName = "NodejsSetup.msi" },
        [PSCustomObject]@{ Id = 39; Name = "Python 3"; Url = "https://www.python.org/ftp/python/3.12.5/python-3.12.5-amd64.exe"; FileName = "PythonSetup.exe" },
        [PSCustomObject]@{ Id = 40; Name = "PuTTY"; Url = "https://the.earth.li/~sgtatham/putty/latest/w64/putty-64bit-installer.msi"; FileName = "PuTTYSetup.msi" },
        [PSCustomObject]@{ Id = 41; Name = "WinSCP"; Url = "https://winscp.net/download/WinSCP-6.3.5-Setup.exe"; FileName = "WinSCPSetup.exe" },
        [PSCustomObject]@{ Id = 42; Name = "Postman"; Url = "https://dl.pstmn.io/download/latest/win64"; FileName = "PostmanSetup.exe" }
    )}
)

# Tra cứu nhanh
$AppsById = @{}
$GroupsByKey = @{}
$AllApps = [System.Collections.Generic.List[PSCustomObject]]::new()

foreach ($g in $Groups) {
    $GroupsByKey[$g.Key.ToUpper()] = $g
    foreach ($item in $g.Items) {
        $AppsById[$item.Id] = $item
        $AllApps.Add($item)
    }
}

# Thư mục Downloads
$DownloadsDir = [IO.Path]::Combine([Environment]::GetFolderPath('UserProfile'), 'Downloads')
if (-not (Test-Path -LiteralPath $DownloadsDir)) {
    $null = New-Item -ItemType Directory -Path $DownloadsDir -Force
}

# ==============================================================================
# HÀM BỔ TRỢ ĐỊNH DẠNG & TẢI
# ==============================================================================
function Format-FileSize([long]$Bytes) {
    if ($Bytes -ge 1GB) { return "{0:N1} GB" -f ($Bytes / 1GB) }
    if ($Bytes -ge 1MB) { return "{0:N1} MB" -f ($Bytes / 1MB) }
    if ($Bytes -ge 1KB) { return "{0:N1} KB" -f ($Bytes / 1KB) }
    return "$Bytes B"
}

function Strip-Ansi([string]$text) {
    return ($text -replace '\x1b\[[0-9;]*m', '')
}

function Get-DisplayLength([string]$text) {
    $plain = Strip-Ansi $text
    $len = 0
    foreach ($c in $plain.ToCharArray()) {
        # Đếm ký tự dấu tiếng Việt hoặc ký tự rộng nếu có
        $code = [int]$c
        if ($code -ge 0x1100 -and ($code -le 0x115f -or $code -eq 0x2329 -or $code -eq 0x232a -or ($code -ge 0x2e80 -and $code -le 0xa4cf) -or ($code -ge 0xac00 -and $code -le 0xd7a3))) {
            $len += 2
        } else {
            $len += 1
        }
    }
    return $len
}

function Pad-Ansi([string]$text, [int]$targetWidth) {
    $currentLen = Get-DisplayLength $text
    $pad = $targetWidth - $currentLen
    if ($pad -gt 0) {
        return $text + (' ' * $pad)
    }
    return $text
}

function Download-AppItem($app) {
    $targetPath = Join-Path $DownloadsDir $app.FileName
    Write-Host "  ${C_CYAN}[>] Đang tải:${C_RESET} $($app.Name)..."
    
    $success = $false
    $curlCmd = Get-Command curl.exe -ErrorAction SilentlyContinue
    if ($curlCmd) {
        $curlPath = $curlCmd.Source
        & $curlPath --fail --location --connect-timeout 15 --max-time 1800 --output $targetPath -- $app.Url
        if ($LASTEXITCODE -eq 0 -and (Test-Path -LiteralPath $targetPath)) {
            $success = $true
        }
    } else {
        try {
            [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -Uri $app.Url -OutFile $targetPath -UseBasicParsing -TimeoutSec 1800
            $success = $true
        } catch {
            $success = $false
        }
    }

    if ($success -and (Test-Path -LiteralPath $targetPath)) {
        $size = (Get-Item -LiteralPath $targetPath).Length
        $sizeStr = Format-FileSize $size
        Write-Host "      ${C_GREEN}[✓] Thành công:${C_RESET} $($app.FileName) ($sizeStr)"
        return $targetPath
    } else {
        Write-Host "      ${C_ROSE}[!] Thất bại:${C_RESET} $($app.Name)"
        return $null
    }
}

# ==============================================================================
# HÀM PHÂN TÍCH LỰA CHỌN (HỖ TRỢ NHÓM [W, V, C...], SỐ LẺ, DẢI SỐ)
# ==============================================================================
function Parse-UserSelection([string]$inputStr) {
    $selectedIds = [System.Collections.Generic.List[int]]::new()
    $tokens = $inputStr -replace ',', ' ' -split '\s+' | Where-Object { $_ -ne '' }
    foreach ($token in $tokens) {
        $upper = $token.ToUpper()
        if ($GroupsByKey.ContainsKey($upper)) {
            # Chọn cả nhóm theo phím tắt (W, V, C, F, M, N, S, D)
            $grp = $GroupsByKey[$upper]
            foreach ($item in $grp.Items) {
                if (-not $selectedIds.Contains($item.Id)) { $selectedIds.Add($item.Id) }
            }
        } elseif ($token -match '^(\d+)-(\d+)$') {
            # Chọn theo dải số (vd: 1-5, 14-17)
            $start = [int]$matches[1]
            $end = [int]$matches[2]
            if ($start -gt $end) { $tmp = $start; $start = $end; $end = $tmp }
            $start = [Math]::Max(1, $start)
            $end = [Math]::Min(42, $end)
            for ($k = $start; $k -le $end; $k++) {
                if ($AppsById.ContainsKey($k) -and -not $selectedIds.Contains($k)) {
                    $selectedIds.Add($k)
                }
            }
        } elseif ($token -match '^\d+$') {
            # Chọn số lẻ (vd: 1, 9, 34)
            $val = [int]$token
            if ($AppsById.ContainsKey($val) -and -not $selectedIds.Contains($val)) {
                $selectedIds.Add($val)
            }
        }
    }
    return $selectedIds
}

# ==============================================================================
# GIAO DIỆN CHÍNH & VÒNG LẶP ĐIỀU KHIỂN
# ==============================================================================
while ($true) {
    Clear-Host
    Write-Host "${C_YELLOW}${C_BOLD}╭── TẢI & CÀI ĐẶT PHẦN MỀM TỰ ĐỘNG ──────────────────────────────────────────────────────────╮${C_RESET}"
    Write-Host "${C_MUTED}│ Thư mục: $DownloadsDir | Tổng cộng: $($AllApps.Count) ứng dụng (8 nhóm chuyên trách)      │${C_RESET}"
    Write-Host "${C_YELLOW}╰────────────────────────────────────────────────────────────────────────────────────────────╯${C_RESET}"

    # Chuẩn bị dữ liệu hiển thị theo 2 cột
    $col1Groups = $Groups | Where-Object { $_.Column -eq 1 }
    $col2Groups = $Groups | Where-Object { $_.Column -eq 2 }

    $col1Lines = [System.Collections.Generic.List[string]]::new()
    $col2Lines = [System.Collections.Generic.List[string]]::new()

    $boxWidth = 41
    function Build-GroupLines($groupList, $linesList) {
        foreach ($g in $groupList) {
            $hdrPrefix = " ╭─ [$($g.Key)] $($g.Name) "
            $hdrLen = Get-DisplayLength $hdrPrefix
            $dashCount = [Math]::Max(2, ($boxWidth - $hdrLen))
            $dashes = "─" * $dashCount
            $linesList.Add("${C_YELLOW}${C_BOLD}$hdrPrefix$dashes${C_RESET}")
            foreach ($it in $g.Items) {
                $num = ("[{0,2}]" -f $it.Id)
                $linesList.Add(" │ ${C_CYAN}$num${C_TEXT} $($it.Name)${C_RESET}")
            }
            $botDashes = "─" * ($boxWidth - 2)
            $linesList.Add("${C_MUTED} ╰$botDashes${C_RESET}")
        }
    }

    Build-GroupLines $col1Groups $col1Lines
    Build-GroupLines $col2Groups $col2Lines

    $maxRows = [Math]::Max($col1Lines.Count, $col2Lines.Count)
    while ($col1Lines.Count -lt $maxRows) { $col1Lines.Add("") }
    while ($col2Lines.Count -lt $maxRows) { $col2Lines.Add("") }

    for ($r = 0; $r -lt $maxRows; $r++) {
        $left = Pad-Ansi $col1Lines[$r] 45
        $right = $col2Lines[$r]
        Write-Host "$left   $right"
    }

    Write-Host ""
    Write-Host "${C_YELLOW}╭── ĐIỀU KHIỂN & LỰA CHỌN TẢI ───────────────────────────────────────────────────────────────╮${C_RESET}"
    Write-Host "${C_TEXT}│ ${C_ORCHID}Tải cả nhóm:${C_RESET} [W] Web  [V] Việt  [C] Chat  [F] Nén  [M] Media  [N] Mạng  [S] Sys  [D] Dev    │"
    Write-Host "${C_TEXT}│ ${C_GREEN}Tải tất cả:${C_RESET}  [A] Tất cả 42 app   |  ${C_CYAN}Tải lẻ:${C_RESET} Gõ số lẻ (1, 3, 5) hoặc dải số (1-5)         │"
    Write-Host "${C_TEXT}│ ${C_ROSE}Quay lại:${C_RESET}    [0] Quay lại menu chính                                                      │"
    Write-Host "${C_YELLOW}╰────────────────────────────────────────────────────────────────────────────────────────────╯${C_RESET}"
    Write-Host -NoNewline "${C_BOLD}  Lựa chọn của bạn (ví dụ: W hoặc 1,3,5 hoặc 1-5): ${C_RESET}"
    
    $choice = [Console]::ReadLine()
    if ($null -eq $choice) { break }
    $choice = $choice.Trim()

    if ($choice -eq "") { continue }
    if ($choice -eq "0" -or $choice.ToUpper() -eq "Q") { break }

    if ($choice.ToUpper() -eq "A") {
        Clear-Host
        Write-Host "${C_YELLOW}╭── TẢI TẤT CẢ $($AllApps.Count) ỨNG DỤNG ───────────────────────────────────────────────────╮${C_RESET}"
        Write-Host "${C_YELLOW}╰────────────────────────────────────────────────────────────────────────────╯${C_RESET}`n"
        $okCount = 0
        for ($k = 0; $k -lt $AllApps.Count; $k++) {
            $res = Download-AppItem $AllApps[$k]
            if ($res) { $okCount++ }
            Write-Host ""
        }
        Write-Host "${C_GREEN}[✓] Đã hoàn tất $okCount/$($AllApps.Count) ứng dụng.${C_RESET}"
        Write-Host -NoNewline "  Mở thư mục Downloads? (Y/n): "
        $openAns = [Console]::ReadLine()
        if ($openAns -eq "" -or $openAns.ToUpper() -eq "Y") {
            Start-Process explorer.exe $DownloadsDir
        }
        continue
    }

    $selectedIds = Parse-UserSelection $choice
    if ($selectedIds.Count -eq 0) {
        Write-Host "`n  ${C_ROSE}[!] Lựa chọn không hợp lệ! Vui lòng nhập phím tắt nhóm (W,V,C...) hoặc số (1-42).${C_RESET}"
        Start-Sleep -Milliseconds 1200
        continue
    }

    if ($selectedIds.Count -eq 1) {
        $app = $AppsById[$selectedIds[0]]
        Clear-Host
        $hdrText = "TẢI ỨNG DỤNG [#{0}]: {1}" -f $app.Id, $app.Name
        Write-Host "${C_YELLOW}╭── $hdrText ─────────────────────────────────────────────────╮${C_RESET}"
        Write-Host "${C_YELLOW}╰────────────────────────────────────────────────────────────────────────────╯${C_RESET}`n"
        $fileSaved = Download-AppItem $app
        if ($fileSaved) {
            Write-Host "`n  ${C_GREEN}[✓] Vị trí:${C_RESET} $fileSaved"
            Write-Host -NoNewline "  Mở file cài đặt ngay? (y/N): "
            $runAns = [Console]::ReadLine()
            if ($runAns.ToUpper() -eq "Y") {
                Start-Process $fileSaved
            }
        } else {
            Write-Host "`n  Nhấn Enter để tiếp tục..."
            $null = [Console]::ReadLine()
        }
    } else {
        Clear-Host
        Write-Host "${C_YELLOW}╭── TẢI $($selectedIds.Count) ỨNG DỤNG ĐÃ CHỌN ──────────────────────────────────────────────╮${C_RESET}"
        Write-Host "${C_YELLOW}╰────────────────────────────────────────────────────────────────────────────╯${C_RESET}`n"
        $okCount = 0
        for ($i = 0; $i -lt $selectedIds.Count; $i++) {
            $app = $AppsById[$selectedIds[$i]]
            Write-Host "  [{$($i + 1)}/{$($selectedIds.Count)}] ..."
            $res = Download-AppItem $app
            if ($res) { $okCount++ }
            Write-Host ""
        }
        Write-Host "${C_GREEN}[✓] Đã tải $okCount/$($selectedIds.Count) ứng dụng thành công.${C_RESET}"
        Write-Host -NoNewline "  Mở thư mục Downloads? (Y/n): "
        $openAns = [Console]::ReadLine()
        if ($openAns -eq "" -or $openAns.ToUpper() -eq "Y") {
            Start-Process explorer.exe $DownloadsDir
        }
    }
}
