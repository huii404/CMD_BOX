param([string]$Compiler = 'C:\msys64\ucrt64\bin\g++.exe', [string]$BuiltSandbox = '')
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$project = Split-Path $PSScriptRoot -Parent
$sandbox = if ($BuiltSandbox) { (Resolve-Path -LiteralPath $BuiltSandbox).Path } else { Join-Path $PSScriptRoot ('sandbox\run-' + [guid]::NewGuid().ToString('N')) }
if (!$sandbox.StartsWith((Join-Path $PSScriptRoot 'sandbox') + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Sandbox must be inside tests/sandbox' }
New-Item -ItemType Directory -Path $sandbox -Force | Out-Null
Push-Location $project
try {
    if (!$BuiltSandbox) {
    & $Compiler -std=c++17 -O0 -Iinclude tests\security_profiles_test.cpp src\core\SecurityProfiles.cpp -o "$sandbox\security_profiles_test.exe" -static
    if ($LASTEXITCODE -ne 0) { throw 'Test build failed' }
    }
    & "$sandbox\security_profiles_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Configuration tests failed' }
    if (!$BuiltSandbox) {
    $sources = @('src/main.cpp') + @(Get-ChildItem src\core\*.cpp,src\optimizer\*.cpp,src\diskcleaner\*.cpp,src\network\*.cpp,src\tools\*.cpp,src\media\*.cpp | ForEach-Object FullName)
    & $Compiler -std=c++17 -O0 -fopenmp -Iinclude $sources -o "$sandbox\cmd_box.exe" -lbcrypt -lws2_32 -liphlpapi -lole32 -lwindowscodecs -loleaut32 -luuid -lversion -static
    if ($LASTEXITCODE -ne 0) { throw 'App build failed' }
    }
    $app = Join-Path $sandbox 'cmd_box.exe'
    $config = Join-Path $sandbox 'cmd_box.json'
    function Invoke-SandboxApp([string]$Arguments, [string]$InputText = '') {
        $start = [Diagnostics.ProcessStartInfo]::new()
        $start.FileName = $app
        $start.Arguments = $Arguments
        $start.UseShellExecute = $false
        $start.CreateNoWindow = $true
        $start.RedirectStandardInput = $true
        $start.RedirectStandardOutput = $true
        $start.StandardOutputEncoding = [Text.UTF8Encoding]::new($false)
        $process = [Diagnostics.Process]::Start($start)
        $process.StandardInput.Write($InputText)
        $process.StandardInput.Close()
        $output = $process.StandardOutput.ReadToEnd()
        $process.WaitForExit()
        $script:LASTEXITCODE = $process.ExitCode
        $process.Dispose()
        return $output
    }
    if (Test-Path -LiteralPath $config) { Remove-Item -LiteralPath $config -Force }
    function Test-SetupFrame([string]$OutputText) {
        $visible = $OutputText -replace '\x1b\[[0-9;]*[A-Za-z]', ''
        $rows = @($visible -split "`n" | Where-Object { $_ -match '^  [│╭╰]' })
        if (!$rows.Count) { throw 'Setup frame was not rendered' }
        foreach ($row in $rows) {
            if ($row.TrimEnd("`r").Length -ne 72) { throw "Setup frame overflow: $row" }
        }
    }
    $result = Invoke-SandboxApp "setup" "5`n`n6`n`n0`n"
    Test-SetupFrame $result
    if (Test-Path -LiteralPath $config) { throw 'Unconfigured matrix/hide wrote configuration' }
    if ($result -notmatch '27  Sắp album') { throw 'Feature matrix is incomplete' }
    Write-Output 'PASS: Setup and matrix layout, unconfigured hide denied'
    $result = Invoke-SandboxApp "setup" "2`ny`n`n0`n"
    $createdConfig = Get-Content -LiteralPath $config -Raw | ConvertFrom-Json
    if (!$createdConfig.hide_config -or !(Get-Item -LiteralPath $config -Force).Attributes.HasFlag([IO.FileAttributes]::Hidden)) { throw 'First Setup did not hide JSON by default' }
    $result = Invoke-SandboxApp "setup" "7`ny`n`n0`n"
    if (Test-Path -LiteralPath $config) { throw 'Hidden config reset failed' }
    Write-Output 'PASS: first Setup hides JSON, reset removes Hidden config'
    foreach ($command in @('clean','media','scan-network')) {
        $result = Invoke-SandboxApp $command
        if ($LASTEXITCODE -ne 3) { throw "Unconfigured CLI allowed $command" }
    }
    foreach ($tier in 1..4) {
        @{version=1;tier=$tier;hide_config=$false} | ConvertTo-Json | Set-Content -LiteralPath $config -Encoding utf8
        # Cancellation occurs at the confirmation before any system mutation.
        $result = Invoke-SandboxApp "optimize 4" "n`n"
        $expected = if ($tier -lt 3) { 3 } else { 0 }
        if ($LASTEXITCODE -ne $expected) { throw "Tier $tier CLI mismatch" }
        if ($tier -ge 3 -and ($result -join "`n") -notmatch 'Đã hủy') { throw 'Cancellation not verified' }
        Write-Output "PASS: CLI tier $tier"
    }
    # CLI Setup modifies only the sandbox JSON.
    $result = Invoke-SandboxApp "setup" "1`n`n0`n"
    if ((Get-Content -LiteralPath $config -Raw | ConvertFrom-Json).tier -ne 1) { throw 'Setup downgrade failed' }
    Test-SetupFrame $result
    $before = Get-Content -LiteralPath $config -Raw
    $beforeTime = (Get-Item -LiteralPath $config).LastWriteTimeUtc.Ticks
    $result = Invoke-SandboxApp "setup" "1`n`n0`n"
    if ((Get-Content -LiteralPath $config -Raw) -ne $before) { throw 'Same-tier selection rewrote config' }
    if ((Get-Item -LiteralPath $config).LastWriteTimeUtc.Ticks -ne $beforeTime) { throw 'Same-tier selection touched config' }
    if ($result -notmatch 'HỒ SƠ HIỆN TẠI') { throw 'Same-tier feedback missing' }
    Test-SetupFrame $result
    $result = Invoke-SandboxApp "setup" "4`nn`n0`n"
    if ((Get-Content -LiteralPath $config -Raw | ConvertFrom-Json).tier -ne 1) { throw 'Rejected upgrade changed tier' }
    $result = Invoke-SandboxApp "setup" "4`ny`n`n0`n"
    if ((Get-Content -LiteralPath $config -Raw | ConvertFrom-Json).tier -ne 4) { throw 'Confirmed upgrade failed' }
    $result = Invoke-SandboxApp "setup" "6`n`n0`n"
    if (!(Get-Item -LiteralPath $config -Force).Attributes.HasFlag([IO.FileAttributes]::Hidden)) { throw 'Setup hide failed' }
    $result = Invoke-SandboxApp "setup" "7`ny`n`n0`n"
    if (Test-Path -LiteralPath $config) { throw 'Setup reset failed' }
    Test-SetupFrame $result
    Write-Output 'PASS: Setup downgrade, rejected/confirmed upgrade, Hidden, reset'
    'invalid-json' | Set-Content -LiteralPath $config
    $result = Invoke-SandboxApp "clean"
    if ($LASTEXITCODE -ne 3) { throw 'Invalid JSON did not deny CLI' }
    Invoke-SandboxApp "--help"
    if ($LASTEXITCODE -ne 0) { throw 'Help failed' }
    Invoke-SandboxApp "--version"
    if ($LASTEXITCODE -ne 0) { throw 'Version failed' }
    Write-Output "PASS: isolated CLI tests. Sandbox: $sandbox"
} finally { Pop-Location }
