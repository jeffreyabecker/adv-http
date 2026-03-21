param()

Write-Host "Running consolidated native PlatformIO tests..."

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
Push-Location $projectRoot

try {
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
