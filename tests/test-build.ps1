param([string]$Compiler = 'C:\msys64\ucrt64\bin\g++.exe')
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$fixture = Join-Path $PSScriptRoot ('sandbox\build test-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force "$fixture\scripts", "$fixture\src", "$fixture\include", "$fixture\assets" | Out-Null
Copy-Item -LiteralPath "$project\build.bat" -Destination "$fixture\build.bat"
Copy-Item -LiteralPath "$project\scripts\build.ps1" -Destination "$fixture\scripts\build.ps1"
Copy-Item -LiteralPath "$project\assets\app.ico" -Destination "$fixture\assets\app.ico"
'1 ICON "assets/app.ico"' | Set-Content "$fixture\assets\resource.rc" -Encoding ascii
'#define VALUE 7' | Set-Content "$fixture\include\build test.h" -Encoding ascii
"#include `"build test.h`"`n#include <iostream>`nint unit();`nint main() { std::cout << VALUE + unit(); }" | Set-Content "$fixture\src\main.cpp" -Encoding ascii
"#include `"build test.h`"`nint unit() { int sum=0;`n#pragma omp parallel for num_threads(2) reduction(+:sum)`nfor (int i=0; i<1; ++i) sum+=VALUE; return sum; }" | Set-Content "$fixture\src\unit.cpp" -Encoding ascii
function Run-Build([string]$Expected, [switch]$Fail, [switch]$Rebuild) {
    $arguments = @('-NoProfile','-ExecutionPolicy','Bypass','-File',"$fixture\scripts\build.ps1",'-Compiler',$Compiler,'-NoRun')
    if ($Rebuild) { $arguments += '-Rebuild' }
    $output = (& powershell @arguments) -join "`n"
    $code = $LASTEXITCODE
    if (($Fail -and $code -eq 0) -or (!$Fail -and $code -ne 0)) { throw "Unexpected result: $output" }
    if ($Expected -and $output -notmatch $Expected) { throw "Expected $Expected : $output" }
    Write-Output $output
}
Run-Build '2/2 C\+\+ files'
$exe = "$fixture\bin\main.exe"
$original = (Get-FileHash $exe).Hash
$unchanged = Run-Build '0/2 C\+\+ files'
if (($unchanged -join "`n") -notmatch 'Reuse cached resource.o' -or ($unchanged -join "`n") -notmatch 'Unchanged; reuse main.exe') { throw 'No-op rebuilt resource or executable' }
if ((Get-FileHash $exe).Hash -ne $original) { throw 'No-op changed executable' }
'#define VALUE 8' | Set-Content "$fixture\include\build test.h" -Encoding ascii
Run-Build '2/2 C\+\+ files'
if ((& $exe) -ne '16') { throw 'Header dependency did not update executable' }
'// one source changed' | Add-Content "$fixture\src\unit.cpp"
Run-Build '1/2 C\+\+ files'
'// resource changed' | Add-Content "$fixture\assets\resource.rc"
$iconBuild = Run-Build '0/2 C\+\+ files'
if (($iconBuild -join "`n") -notmatch 'Compile changed resource/icon') { throw 'RC change did not rebuild resource' }
'int unused() { return 3; }' | Set-Content "$fixture\src\added.cpp" -Encoding ascii
Run-Build '1/3 C\+\+ files'
Remove-Item -LiteralPath "$fixture\src\added.cpp"
$removed = Run-Build '0/2 C\+\+ files'
if (($removed -join "`n") -notmatch '\[LINK\] main.next.exe') { throw 'Removed source did not relink' }
$beforeFailure = (Get-FileHash $exe).Hash
'invalid C++ syntax !' | Add-Content "$fixture\src\unit.cpp"
Run-Build '' -Fail
if ((Get-FileHash $exe).Hash -ne $beforeFailure) { throw 'Failed compile replaced working exe' }
'int unit() { return 8; }' | Set-Content "$fixture\src\unit.cpp" -Encoding ascii
Run-Build '2/2 C\+\+ files' -Rebuild
Run-Build '0/2 C\+\+ files'
Push-Location $fixture
try {
    $batch = & cmd /d /c .\build.bat -NoRun
    if ($LASTEXITCODE -ne 0 -or ($batch -join "`n") -match '\[ERROR\]') { throw 'Batch wrapper failed' }
    if (($batch -join "`n") -notmatch '0/2 C\+\+ files') { throw 'Batch wrapper ignored cache' }
    $batch = & cmd /d /c .\build.bat -NoRun -Compiler missing.exe
    if ($LASTEXITCODE -ne 1) { throw 'Batch wrapper swallowed failure exit code' }
} finally { Pop-Location }
Write-Output "PASS: cache, paths with spaces, headers, source add/remove, resources, OpenMP, failure preservation, force rebuild, batch wrapper. Fixture: $fixture"
