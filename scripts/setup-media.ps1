param()
$ErrorActionPreference = 'Stop'
$mediaProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$mediaBin = Join-Path $mediaProjectRoot 'bin'
$mediaSource = Join-Path $mediaBin 'exiftool-source/exiftool-13.59/exiftool'
New-Item -ItemType Directory -Force -Path $mediaBin | Out-Null
if (Test-Path -LiteralPath (Join-Path $mediaBin 'exiftool.exe')) {
    Write-Output 'Using bin/exiftool.exe (keep its exiftool_files folder beside it).'
    exit 0
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
Write-Output 'ExifTool source backend ready. Also supply ffprobe.exe beside ffmpeg.exe or in PATH.'
