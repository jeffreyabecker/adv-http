param(
    [string[]]$Environments = @('rpipicow', 'esp32dev', 'esp8266')
)

Write-Host "Running embedded PlatformIO validation..."

$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Definition
$projectRoot = Split-Path -Parent $scriptDirectory
Push-Location $projectRoot

try {
    & (Join-Path $scriptDirectory 'assert-no-legacy-http-tokens.ps1')
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    foreach ($environment in $Environments) {
        Write-Host ("Compiling embedded environment '{0}'..." -f $environment)

        if (Get-Command pio -ErrorAction SilentlyContinue) {
            & pio run -e $environment
        }
        elseif (Get-Command platformio -ErrorAction SilentlyContinue) {
            & platformio run -e $environment
        }
        else {
            Write-Host "PlatformIO CLI not found." -ForegroundColor Red
            exit 1
        }

        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }

    exit 0
}
finally {
    Pop-Location
}