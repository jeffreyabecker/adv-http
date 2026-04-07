param()

$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Definition
$projectRoot = Split-Path -Parent $scriptDirectory

$includePatterns = @('*.h', '*.hpp', '*.cpp', '*.cxx', '*.cc', '*.ino')
$scanRoots = @(
    (Join-Path $projectRoot 'src')
    (Join-Path $projectRoot 'test')
)

$legacyPatterns = @(
    @{
        Name = 'legacy namespace token'
        Pattern = 'httpadv::'
    },
    @{
        Name = 'legacy include root'
        Pattern = 'httpadv/v1'
    },
    @{
        Name = 'legacy source-tree include root'
        Pattern = 'src/httpadv/v1'
    }
)

$violations = New-Object System.Collections.Generic.List[object]

foreach ($scanRoot in $scanRoots) {
    if (-not (Test-Path $scanRoot)) {
        continue
    }

    $files = Get-ChildItem -Path $scanRoot -Recurse -File -Include $includePatterns
    foreach ($file in $files) {
        $lines = Get-Content $file.FullName
        for ($index = 0; $index -lt $lines.Count; ++$index) {
            $line = $lines[$index]
            foreach ($legacyPattern in $legacyPatterns) {
                if ($line -match [regex]::Escape($legacyPattern.Pattern)) {
                    $relativePath = [System.IO.Path]::GetRelativePath($projectRoot, $file.FullName)
                    $violations.Add([pscustomobject]@{
                        Path = $relativePath
                        Line = $index + 1
                        Type = $legacyPattern.Name
                        Text = $line.Trim()
                    })
                }
            }
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Host 'Legacy HTTP rename tokens found in active source/test files:' -ForegroundColor Red
    foreach ($violation in $violations) {
        Write-Host ("  {0}:{1} [{2}] {3}" -f $violation.Path, $violation.Line, $violation.Type, $violation.Text) -ForegroundColor Red
    }

    exit 1
}

Write-Host 'No legacy HTTP rename tokens found in active source/test files.' -ForegroundColor Green