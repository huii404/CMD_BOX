param (
    [string]$gxxPath
)

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$esc = [char]27

$cReset  = "$esc[0m"
$cPink   = "$esc[95m"
$cCyan   = "$esc[96m"
$cYellow = "$esc[93m"
$bgGreen = "$esc[42;30m"
$bgRed   = "$esc[41;97m"

$buildArgs = "-std=c++17 -O3 -fopenmp -mavx2 -mfma -Iinclude src\*.cpp -o bin\main.exe -lws2_32 -liphlpapi -lole32 -lwindowscodecs -loleaut32 -luuid -static-libgcc -static-libstdc++ -static -s"

$errLog = "$env:TEMP\cmd_build_err.log"
if (Test-Path $errLog) { Remove-Item -Force $errLog }

$proc = Start-Process -FilePath $gxxPath -ArgumentList $buildArgs -NoNewWindow -PassThru -RedirectStandardError $errLog
$sw = [System.Diagnostics.Stopwatch]::StartNew()

$frames = @('|','/','-','\')
$i = 0

# Chữ cố định: In 1 lần duy nhất trước vòng lặp, đứng yên hoàn toàn
Write-Host -NoNewline "  $cCyan[☕] Đang nấu code:$cReset "

$lastLen = 0

while (-not $proc.HasExited) {
    $s = $sw.Elapsed.TotalSeconds.ToString("0.0")
    $f = $frames[$i % 4]
    
    # Chỉ in spinner và số thời gian thay đổi tại chỗ
    $back = "`b" * $lastLen
    $disp = "$cPink[$f]$cReset $cYellow${s}s$cReset"
    Write-Host -NoNewline "$back$disp"
    
    # Độ dài hiển thị mắt người (không tính mã màu ANSI): "[X] X.Xs" = 8 ký tự
    $lastLen = 3 + 1 + $s.Length + 1
    
    Start-Sleep -Milliseconds 80
    $i++
}

$proc.WaitForExit()
$tot = $sw.Elapsed.TotalSeconds.ToString("0.0")
$back = "`b" * $lastLen

if ($proc.ExitCode -eq 0) {
    Write-Host "$back$bgGreen [OK] ${tot}s! SIÊU MƯỢT $cReset  "
} else {
    Write-Host "$back$bgRed [X] TOANG! (${tot}s) $cReset  "
}

exit $proc.ExitCode
