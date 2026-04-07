param()

$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Definition
$projectRoot = Split-Path -Parent $scriptDirectory

$includePatterns = @('*.h', '*.hpp', '*.cpp', '*.cxx', '*.cc', '*.ino', '*.md', '*.json', '*.properties', '*.ini', '*.ps1', '*.txt')
$scanRoots = @($projectRoot)

$excludedDirectories = @(
    '.git',
    '.pio',
    'build',
    'tools/build_native_tests',
    'llhttp'
)

$approvedReferencePaths = @(
    'docs/backlog/task-lists/httpserveradvanced/001-lumalink-http-rename-and-platform-extraction-backlog.md',
    'docs/backlog/task-lists/re-arduino/000-re-arduino-summary-backlog.md',
    'docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md',
    'docs/httpserveradvanced/LUMALINK_HTTP_RENAME_RELEASE_NOTES.md',
    'test/README.md'
)

$excludedFiles = @(
    'tools/assert-no-legacy-http-tokens.ps1'
)

$legacyPatterns = @(
    @{
        Name = 'legacy public umbrella token'
        Pattern = '\bHttpServerAdvanced(?:\.h)?\b'
    },
    @{
        Name = 'legacy namespace token'
        Pattern = '\bhttpadv::'
    },
    @{
        Name = 'legacy include root'
        Pattern = '\bhttpadv/v1/'
    },
    @{
        Name = 'legacy source-tree include root'
        Pattern = '\bsrc/httpadv/v1/'
    }
)

$violations = New-Object System.Collections.Generic.List[object]

foreach ($scanRoot in $scanRoots) {
    if (-not (Test-Path $scanRoot)) {
        continue
    }

    $files = Get-ChildItem -Path $scanRoot -Recurse -File -Include $includePatterns
    foreach ($file in $files) {
        $relativePath = [System.IO.Path]::GetRelativePath($projectRoot, $file.FullName).Replace('\', '/')

        if ($approvedReferencePaths -contains $relativePath) {
            continue
        }

        if ($excludedFiles -contains $relativePath) {
            continue
        }

        $isInExcludedDirectory = $false
        foreach ($excludedDirectory in $excludedDirectories) {
            if ($relativePath.StartsWith($excludedDirectory + '/', [System.StringComparison]::OrdinalIgnoreCase)) {
                $isInExcludedDirectory = $true
                break
            }
        }

        if ($isInExcludedDirectory) {
            continue
        }

        $lines = Get-Content $file.FullName
        for ($index = 0; $index -lt $lines.Count; ++$index) {
            $line = $lines[$index]
            foreach ($legacyPattern in $legacyPatterns) {
                if ($line -cmatch $legacyPattern.Pattern) {
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
    Write-Host 'Legacy HTTP rename tokens found outside approved historical paths:' -ForegroundColor Red
    foreach ($violation in $violations) {
        Write-Host ("  {0}:{1} [{2}] {3}" -f $violation.Path, $violation.Line, $violation.Type, $violation.Text) -ForegroundColor Red
    }

    exit 1
}

Write-Host 'No legacy HTTP rename tokens found outside approved historical paths.' -ForegroundColor Green