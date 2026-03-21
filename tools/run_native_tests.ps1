param()

Write-Host "Building native tests..."

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
$buildDir = Join-Path $projectRoot "build_native_tests"
if (!(Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }

function Build-And-Run-Test($testPath) {
    $exeName = [System.IO.Path]::GetFileNameWithoutExtension($testPath)
    $outPath = Join-Path $buildDir ($exeName + ".exe")
    Write-Host "Compiling $testPath -> $outPath"
    $compileArgs = @(
        '-std=c++17', '-O0', '-g',
        '-I', "$projectRoot/src",
        '-I', "$projectRoot/test/support/native_arduino",
        '-I', "$projectRoot/test/support/native_unity",
        $testPath,
        '-o', $outPath
    )
    $result = & g++ @compileArgs 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Compile failed for ${testPath}`n$result" -ForegroundColor Red
        return 1
    }
    Write-Host "Running $outPath`n"
    & $outPath
    return $LASTEXITCODE
}

$tests = Get-ChildItem -Path "$projectRoot/test" -Recurse -Filter test_main.cpp |
    Where-Object { $_.FullName -notlike "*support*" } | ForEach-Object { $_.FullName }

$overallStatus = 0
foreach ($t in $tests) {
    $status = Build-And-Run-Test $t
    if ($status -ne 0) { $overallStatus = $status }
}

if ($overallStatus -eq 0) {
    Write-Host "All native tests passed." -ForegroundColor Green
    exit 0
} else {
    Write-Host "Some native tests failed (exit code $overallStatus)." -ForegroundColor Red
    exit $overallStatus
}
