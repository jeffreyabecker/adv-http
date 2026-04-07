param()

Write-Host "Running consolidated native PlatformIO tests..."

$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Definition
$projectRoot = Split-Path -Parent $scriptDirectory
Push-Location $projectRoot

try {
    & (Join-Path $scriptDirectory 'assert-no-legacy-http-tokens.ps1')
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    if (Get-Command pio -ErrorAction SilentlyContinue) {
        & pio test -e native
    }
    elseif (Get-Command platformio -ErrorAction SilentlyContinue) {
        & platformio test -e native
    }
    else {
        Write-Host "PlatformIO CLI not found." -ForegroundColor Red
        exit 1
    }

    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
