param([switch]$ForceFFmpeg)
$ErrorActionPreference = 'Stop'

function Initialize-FFmpeg {
    param([switch]$Force)
    $ffmpegProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
    $ffmpegBin = Join-Path $ffmpegProjectRoot 'bin'
    $ffmpegNames = @('ffmpeg.exe')

    function Test-FFmpegBinary([string]$BinaryPath) {
        if (-not (Test-Path -LiteralPath $BinaryPath -PathType Leaf)) { return $false }
        try {
            & $BinaryPath -version *> $null
            return ($LASTEXITCODE -eq 0)
        } catch { return $false }
    }

    function Get-GyanText([string]$Uri) {
        $gyanContent = (Invoke-WebRequest -Uri $Uri -UseBasicParsing -TimeoutSec 30).Content
        if ($gyanContent -is [byte[]]) { return [Text.Encoding]::UTF8.GetString($gyanContent).Trim() }
        return ([string]$gyanContent).Trim()
    }

    $ffmpegReady = $true
    foreach ($ffmpegName in $ffmpegNames) {
        if (-not (Test-FFmpegBinary (Join-Path $ffmpegBin $ffmpegName))) { $ffmpegReady = $false }
    }
    if ($ffmpegReady -and -not $Force) {
        Write-Output 'Using bin/ffmpeg.exe. Use -ForceFFmpeg to download again.'
        return
    }
    if (-not [Environment]::Is64BitOperatingSystem) { throw 'Gyan FFmpeg builds require 64-bit Windows.' }

    # Resolve the latest release once, then use versioned URLs so an update cannot
    # change the archive between the checksum request and the download.
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
    $ffmpegBaseUrl = 'https://www.gyan.dev/ffmpeg/builds'
    $ffmpegVersion = Get-GyanText "$ffmpegBaseUrl/ffmpeg-release-essentials.zip.ver"
    if ($ffmpegVersion -notmatch '^\d+\.\d+(?:\.\d+)?$') { throw 'Invalid FFmpeg release version from Gyan.' }
    $ffmpegPackageUrl = "$ffmpegBaseUrl/packages/ffmpeg-$ffmpegVersion-essentials_build.zip"
    $ffmpegExpectedHash = Get-GyanText "$ffmpegPackageUrl.sha256"
    if ($ffmpegExpectedHash -notmatch '^[a-fA-F0-9]{64}$') { throw 'Invalid FFmpeg SHA-256 from Gyan.' }

    $ffmpegTempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $ffmpegStage = Join-Path $ffmpegTempRoot ('cmd-box-ffmpeg-' + [guid]::NewGuid())
    New-Item -ItemType Directory -Path $ffmpegStage | Out-Null
    try {
        $ffmpegArchive = Join-Path $ffmpegStage 'ffmpeg.zip'
        Write-Output "Downloading Gyan FFmpeg $ffmpegVersion (release essentials)..."
        Invoke-WebRequest -Uri $ffmpegPackageUrl -OutFile $ffmpegArchive -UseBasicParsing -TimeoutSec 600
        $ffmpegActualHash = (Get-FileHash -LiteralPath $ffmpegArchive -Algorithm SHA256).Hash
        if ($ffmpegActualHash -ne $ffmpegExpectedHash) { throw 'FFmpeg archive checksum mismatch; bin was not changed.' }
        $ffmpegExtract = Join-Path $ffmpegStage 'extracted'
        Expand-Archive -LiteralPath $ffmpegArchive -DestinationPath $ffmpegExtract
        $ffmpegPackageRoot = Join-Path $ffmpegExtract "ffmpeg-$ffmpegVersion-essentials_build"
        foreach ($ffmpegName in $ffmpegNames) {
            if (-not (Test-FFmpegBinary (Join-Path $ffmpegPackageRoot "bin/$ffmpegName"))) {
                throw "Downloaded $ffmpegName failed to start; bin was not changed."
            }
        }
        foreach ($ffmpegDocName in @('LICENSE', 'README.txt')) {
            if (-not (Test-Path -LiteralPath (Join-Path $ffmpegPackageRoot $ffmpegDocName) -PathType Leaf)) {
                throw "FFmpeg package is missing $ffmpegDocName; bin was not changed."
            }
        }
        New-Item -ItemType Directory -Force -Path $ffmpegBin | Out-Null
        $ffmpegDocs = Join-Path $ffmpegBin 'ffmpeg-docs'
        New-Item -ItemType Directory -Force -Path $ffmpegDocs | Out-Null
        foreach ($ffmpegDocName in @('LICENSE', 'README.txt')) {
            Copy-Item -LiteralPath (Join-Path $ffmpegPackageRoot $ffmpegDocName) -Destination $ffmpegDocs -Force
        }
        foreach ($ffmpegName in $ffmpegNames) {
            Copy-Item -LiteralPath (Join-Path $ffmpegPackageRoot "bin/$ffmpegName") -Destination $ffmpegBin -Force
        }
        Write-Output "FFmpeg $ffmpegVersion ready in bin. License and README: bin/ffmpeg-docs."
    } finally {
        $ffmpegResolvedStage = [IO.Path]::GetFullPath($ffmpegStage)
        if ((Split-Path -Parent $ffmpegResolvedStage) -eq $ffmpegTempRoot.TrimEnd('\', '/') -and
            (Split-Path -Leaf $ffmpegResolvedStage).StartsWith('cmd-box-ffmpeg-')) {
            Remove-Item -LiteralPath $ffmpegResolvedStage -Recurse -Force
        }
    }
}

Initialize-FFmpeg -Force:$ForceFFmpeg

$mediaProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$mediaBin = Join-Path $mediaProjectRoot 'bin'
$mediaSource = Join-Path $mediaBin 'exiftool-source/exiftool-13.59/exiftool'
New-Item -ItemType Directory -Force -Path $mediaBin | Out-Null
if (Test-Path -LiteralPath (Join-Path $mediaBin 'exiftool.exe')) {
    Write-Output 'Using bin/exiftool.exe (keep its exiftool_files folder beside it).'
    return
}
$mediaPerl = Get-Command perl.exe -ErrorAction SilentlyContinue
if (-not $mediaPerl -and (Test-Path -LiteralPath 'C:/msys64/usr/bin/perl.exe')) { $mediaPerl = Get-Item -LiteralPath 'C:/msys64/usr/bin/perl.exe' }
if (-not $mediaPerl) { throw 'Install the official ExifTool Windows portable bundle in bin, or provide Perl in PATH. See docs/MEDIA.md.' }
if (-not (Test-Path -LiteralPath $mediaSource)) {
    $mediaTempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $mediaStage = Join-Path $mediaTempRoot ('cmd-box-exiftool-' + [guid]::NewGuid())
    New-Item -ItemType Directory -Path $mediaStage | Out-Null
    try {
        $mediaArchive = Join-Path $mediaStage 'source.zip'
        Invoke-WebRequest -Uri 'https://codeload.github.com/exiftool/exiftool/zip/refs/tags/13.59' -OutFile $mediaArchive -UseBasicParsing
        $mediaHash = (Get-FileHash -LiteralPath $mediaArchive -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($mediaHash -ne '542315bbb4b302b5334b6defd608d9ce2c97299c5608ab83a9e6c232abd56f18') { throw 'ExifTool archive checksum mismatch.' }
        Expand-Archive -LiteralPath $mediaArchive -DestinationPath $mediaStage
        $mediaRuntime = Split-Path -Parent $mediaSource
        New-Item -ItemType Directory -Force -Path $mediaRuntime | Out-Null
        foreach ($mediaPart in @('lib', 'LICENSE', 'README', 'exiftool')) {
            Copy-Item -LiteralPath (Join-Path $mediaStage "exiftool-13.59/$mediaPart") -Destination $mediaRuntime -Recurse -Force
        }
    } finally {
        $mediaResolvedStage = [IO.Path]::GetFullPath($mediaStage)
        if ((Split-Path -Parent $mediaResolvedStage) -eq $mediaTempRoot.TrimEnd('\', '/') -and (Split-Path -Leaf $mediaResolvedStage).StartsWith('cmd-box-exiftool-')) {
            Remove-Item -LiteralPath $mediaResolvedStage -Recurse -Force
        }
    }
}
$mediaPerlPath = if ($mediaPerl.Source) { $mediaPerl.Source } else { $mediaPerl.FullName }
& $mediaPerlPath $mediaSource -ver
if ($LASTEXITCODE -ne 0) { throw 'ExifTool failed to start.' }
Write-Output 'FFmpeg and ExifTool ready.'
