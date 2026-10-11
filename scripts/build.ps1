param(
    [string]$Compiler = '',
    [string]$OutputDirectory = '',
    [ValidateRange(1,16)][int]$Jobs = 2,
    [switch]$Rebuild,
    [switch]$NoRun
)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
if (!$OutputDirectory) { $OutputDirectory = Join-Path $project 'bin' }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$cache = Join-Path $OutputDirectory '.build'
New-Item -ItemType Directory -Force $cache | Out-Null
$timer = [Diagnostics.Stopwatch]::StartNew()
$lock = $null
$running = [Collections.ArrayList]::new()
Push-Location $project
try {
    # One writer per cache. Never terminate an unrelated main.exe or compiler.
    $lock = [IO.File]::Open((Join-Path $cache 'build.lock'), 'OpenOrCreate', 'ReadWrite', 'None')
    if (!$Compiler) {
        $command = Get-Command g++.exe -ErrorAction SilentlyContinue
        if ($command) { $Compiler = $command.Source }
        else {
            $Compiler = @('C:\msys64\ucrt64\bin\g++.exe','C:\msys64\mingw64\bin\g++.exe') | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
        }
    }
    if (!$Compiler -or !(Test-Path -LiteralPath $Compiler)) { throw 'g++ not found. Install MinGW/MSYS2.' }
    $Compiler = (Resolve-Path -LiteralPath $Compiler).Path
    $flags = @('-std=c++17','-O3','-fopenmp','-Iinclude')
    $libraries = @('-fopenmp','-lbcrypt','-lws2_32','-liphlpapi','-lole32','-lwindowscodecs','-loleaut32','-luuid','-lversion','-static-libgcc','-static-libstdc++','-static','-s')
    # Use .NET directly: Get-FileHash may be unavailable when launched through cmd.
    function FileHash([string]$Path) {
        $stream = [IO.File]::OpenRead($Path)
        $algorithm = [Security.Cryptography.SHA256]::Create()
        try { return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '') }
        finally { $algorithm.Dispose(); $stream.Dispose() }
    }
    function ToolStamp([string]$Path) {
        $file = Get-Item -LiteralPath $Path
        return "$Path|$($file.Length)|$($file.LastWriteTimeUtc.Ticks)"
    }
    function Start-Tool([string]$Tool, [string[]]$Arguments) {
        $start = [Diagnostics.ProcessStartInfo]::new()
        $start.FileName = $Tool
        $start.WorkingDirectory = $project
        $start.UseShellExecute = $false
        $start.CreateNoWindow = $true
        $start.RedirectStandardOutput = $true
        $start.RedirectStandardError = $true
        # Invoke executable directly; no shell expansion of paths/arguments.
        $start.Arguments = ($Arguments | ForEach-Object { '"' + $_.Replace('"','\"') + '"' }) -join ' '
        $process = [Diagnostics.Process]::Start($start)
        return [pscustomobject]@{ Process=$process; Out=$process.StandardOutput.ReadToEndAsync(); Err=$process.StandardError.ReadToEndAsync() }
    }
    function Finish-Tool($Task) {
        $Task.Process.WaitForExit()
        $code = $Task.Process.ExitCode
        $stdout = $Task.Out.GetAwaiter().GetResult()
        $stderr = $Task.Err.GetAwaiter().GetResult()
        $Task.Process.Dispose()
        if ($stdout) { Write-Host $stdout.TrimEnd() }
        if ($stderr) { Write-Host $stderr.TrimEnd() }
        if ($code -ne 0) { throw "Tool failed (exit $code). Old executable preserved." }
    }
    function Needs-Compile([string]$Object, [string]$DependencyFile) {
        if (!(Test-Path -LiteralPath $Object) -or !(Test-Path -LiteralPath $DependencyFile)) { return $true }
        $objectTime = (Get-Item -LiteralPath $Object).LastWriteTimeUtc
        # GCC -MMD tracks project headers recursively, including json.hpp.
        $dependencies = (Get-Content -LiteralPath $DependencyFile -Raw) -replace '\\\r?\n',' '
        $colon = $dependencies.IndexOf(': ')
        if ($colon -lt 0) { return $true }
        $dependencies = $dependencies.Substring($colon + 2)
        foreach ($match in [regex]::Matches($dependencies, '(?:\\.|[^\s])+')) {
            $path = $match.Value -replace '\\([ #\\])','$1'
            if (!(Test-Path -LiteralPath $path)) { return $true }
            if ((Get-Item -LiteralPath $path).LastWriteTimeUtc -gt $objectTime) { return $true }
        }
        return $false
    }
    $stampPath = Join-Path $cache 'compiler.txt'
    $stamp = (ToolStamp $Compiler) + '|' + ($flags -join ' ')
    $invalidate = $Rebuild -or !(Test-Path -LiteralPath $stampPath) -or (Get-Content -LiteralPath $stampPath -Raw).TrimEnd() -ne $stamp
    $sources = @(Get-ChildItem src -Recurse -Filter *.cpp | Sort-Object FullName)
    if (!$sources.Count) { throw 'No C++ source files found.' }
    $objects = @()
    $queue = [Collections.Queue]::new()
    foreach ($source in $sources) {
        $relative = $source.FullName.Substring($project.Length + 1)
        $object = Join-Path $cache ([IO.Path]::ChangeExtension($relative, '.o'))
        $dependency = [IO.Path]::ChangeExtension($object, '.d')
        $objects += $object
        if ($invalidate -or (Needs-Compile $object $dependency)) {
            New-Item -ItemType Directory -Force (Split-Path $object -Parent) | Out-Null
            $queue.Enqueue([pscustomobject]@{Source=$relative; Object=$object; Dependency=$dependency})
        }
    }
    $compileCount = $queue.Count
    Write-Host "[BUILD] $compileCount/$($sources.Count) C++ files; workers=$Jobs; -O3 unchanged."
    while ($queue.Count -or $running.Count) {
        while ($queue.Count -and $running.Count -lt $Jobs) {
            $unit = $queue.Dequeue()
            Write-Host "[C++] $($unit.Source)"
            $task = Start-Tool $Compiler ($flags + @('-MMD','-MF',($unit.Dependency + '.next'),'-MT','object','-c',$unit.Source,'-o',($unit.Object + '.next')))
            [void]$running.Add([pscustomobject]@{Unit=$unit; Task=$task})
        }
        foreach ($item in @($running.ToArray())) {
            if ($item.Task.Process.HasExited) {
                Finish-Tool $item.Task
                Move-Item -LiteralPath ($item.Unit.Object + '.next') -Destination $item.Unit.Object -Force
                Move-Item -LiteralPath ($item.Unit.Dependency + '.next') -Destination $item.Unit.Dependency -Force
                [void]$running.Remove($item)
            }
        }
        if ($running.Count) { Start-Sleep -Milliseconds 100 }
    }
    # Persist compile signature independently: a linker retry need not compile again.
    $stamp | Set-Content $stampPath -Encoding utf8
    # The .rc is source text, not an image-render cache. Cache the compiled .o.
    $rc = if (Test-Path assets\resource.rc) { 'assets\resource.rc' } elseif (Test-Path resource.rc) { 'resource.rc' } else { '' }
    if (!$rc -and (Test-Path assets\app.ico)) {
        $rc = 'assets\resource.rc'
        '1 ICON "assets/app.ico"' | Set-Content -LiteralPath $rc -Encoding ascii
        Write-Host '[ICON] Created missing resource.rc from existing app.ico; no image rendering.'
    }
    if ($rc) {
        $windres = Join-Path (Split-Path $Compiler -Parent) 'windres.exe'
        if (!(Test-Path -LiteralPath $windres)) { throw 'windres not found beside g++; icon cannot be embedded.' }
        $resourceInputs = @((Resolve-Path $rc).Path)
        $resourceInputs += @(Get-ChildItem assets -Recurse -File | Where-Object Extension -in '.ico','.rc','.manifest' | ForEach-Object FullName)
        # Include files/resources referred to by quoted paths in the .rc.
        foreach ($match in [regex]::Matches((Get-Content $rc -Raw), '"([^"\r\n]+)"')) {
            foreach ($candidate in @($match.Groups[1].Value, (Join-Path (Split-Path (Resolve-Path $rc).Path -Parent) $match.Groups[1].Value))) {
                if (Test-Path -LiteralPath $candidate -PathType Leaf) { $resourceInputs += (Resolve-Path -LiteralPath $candidate).Path }
            }
        }
        $resourceStamp = (ToolStamp $windres) + '|' + $rc + '|' + ((@($resourceInputs | Sort-Object -Unique | ForEach-Object { "$($_):$(FileHash $_)" })) -join '|')
        $resource = Join-Path $cache 'resource.o'
        $resourceStampPath = Join-Path $cache 'resource.txt'
        if ($Rebuild -or !(Test-Path -LiteralPath $resource) -or !(Test-Path -LiteralPath $resourceStampPath) -or (Get-Content $resourceStampPath -Raw).TrimEnd() -ne $resourceStamp) {
            Write-Host '[ICON] Compile changed resource/icon.'
            Finish-Tool (Start-Tool $windres @($rc,'-O','coff','-o',($resource + '.next')))
            Move-Item -LiteralPath ($resource + '.next') -Destination $resource -Force
            $resourceStamp | Set-Content $resourceStampPath -Encoding utf8
        } else { Write-Host '[ICON] Reuse cached resource.o.' }
        $objects += $resource
    } else { Write-Host '[ICON] No resource.rc or app.ico; building without icon.' }
    $target = Join-Path $OutputDirectory 'main.exe'
    $next = Join-Path $OutputDirectory 'main.next.exe'
    $linkStampPath = Join-Path $cache 'link.txt'
    $linkStamp = $stamp + '|' + ($libraries -join ' ') + '|' + ($objects -join '|')
    $needsLink = $invalidate -or !(Test-Path -LiteralPath $target) -or !(Test-Path $linkStampPath) -or (Get-Content $linkStampPath -Raw).TrimEnd() -ne $linkStamp
    if (!$needsLink) {
        $targetTime = (Get-Item $target).LastWriteTimeUtc
        $needsLink = @($objects | Where-Object { (Get-Item $_).LastWriteTimeUtc -gt $targetTime }).Count -gt 0
    }
    if ($needsLink) {
        Write-Host '[LINK] main.next.exe'
        # Response file avoids Windows command-line length limits.
        $response = Join-Path $cache 'link.rsp'
        $responseLines = @($objects + @('-o',$next) + $libraries | ForEach-Object { '"' + $_.Replace('\','/').Replace('"','\"') + '"' })
        [IO.File]::WriteAllLines($response, [string[]]$responseLines, [Text.UTF8Encoding]::new($false))
        Finish-Tool (Start-Tool $Compiler @("@$response"))
        try { Move-Item -LiteralPath $next -Destination $target -Force }
        catch { throw "Close CMD BOX and retry. New executable kept at $next. $($_.Exception.Message)" }
        $linkStamp | Set-Content $linkStampPath -Encoding utf8
    } else { Write-Host '[LINK] Unchanged; reuse main.exe.' }
    $stamp | Set-Content $stampPath -Encoding utf8
    $timer.Stop()
    Write-Host ("[OK] {0} | {1:N2}s" -f $target,$timer.Elapsed.TotalSeconds)
    if (!$NoRun) { Start-Process -FilePath $target -WorkingDirectory $OutputDirectory -WindowStyle Normal }
} catch {
    Write-Host "[ERROR] $($_.Exception.Message)"
    exit 1
} finally {
    # Let only the compiler processes started here finish before releasing the lock.
    foreach ($item in @($running.ToArray())) {
        try { if (!$item.Task.Process.HasExited) { $item.Task.Process.WaitForExit() }; $item.Task.Process.Dispose() } catch {}
    }
    if ($lock) { $lock.Dispose() }
    Pop-Location
}
