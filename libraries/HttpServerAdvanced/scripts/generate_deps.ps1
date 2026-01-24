<#
Generate dependency list for each source file under the src/ folder.

Outputs:
 - dependencies.json : structured JSON with resolved includes
 - dependencies.txt  : human-readable dependency list

Usage:
 PS> .\generate_deps.ps1 -SrcDir ..\src -OutDir .\out
#>
param(
    [string]$SrcDir = "$PSScriptRoot\..\src",
    [string]$OutDir = "$PSScriptRoot\out"
)

$SrcDir = (Resolve-Path $SrcDir).Path
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$includeRegex = '#\s*include\s*["<](.+?)[">]'

function Resolve-Include {
    param(
        [string]$IncludingFileDir,
        [string]$IncludeText
    )
    # returns array of resolved paths (absolute) under $SrcDir or empty

    $results = @()

    # If include contains path separators (relative), resolve relative to including file dir
    if ($IncludeText -match '[/\\]') {
        $candidate = Join-Path $IncludingFileDir $IncludeText
        if (Test-Path $candidate) { $results += (Resolve-Path $candidate).Path }
        else {
            # Try from SrcDir root
            $candidate2 = Join-Path $SrcDir $IncludeText
            if (Test-Path $candidate2) { $results += (Resolve-Path $candidate2).Path }
        }
    }
    else {
        # No path separators; search by file name under src dir
        $matches = Get-ChildItem -Path $SrcDir -Recurse -File -Include $IncludeText  -ErrorAction SilentlyContinue
        foreach ($m in $matches) { $results += $m.FullName }
    }

    return $results | Select-Object -Unique
}

$allFiles = Get-ChildItem -Path $SrcDir -Recurse -File -Include *.h,*.hpp,*.c,*.cpp,*.cc | Sort-Object FullName

$report = @()

foreach ($file in $allFiles) {
    $text = Get-Content -Raw -Path $file.FullName -ErrorAction SilentlyContinue
    if (-not $text) { continue }

    $matches = [regex]::Matches($text, $includeRegex)
    $deps = @()
    foreach ($m in $matches) {
        $inc = $m.Groups[1].Value.Trim()
        $resolved = Resolve-Include -IncludingFileDir $file.DirectoryName -IncludeText $inc
        $isProject = $false
        if ($resolved.Count -gt 0) { $isProject = $true }

        if ($isProject) { $depType = 'project' } else { $depType = 'external' }

        $deps += [pscustomobject]@{
            include  = $inc
            resolved = $resolved
            type     = $depType
        }
    }

    $report += [pscustomobject]@{
        file = $file.FullName
        relative = $file.FullName.Substring($SrcDir.Length).TrimStart('\','/')
        dependencies = $deps
    }
}

# Print human-readable report to stdout
Write-Output "--- Dependency Report (Human-readable) ---"
foreach ($entry in $report) {
    Write-Output "File: $($entry.relative)"
    if ($entry.dependencies.Count -eq 0) {
        Write-Output "  (no includes found)"
    }
    else {
        foreach ($d in $entry.dependencies) {
            $line = "  - $($d.include) [type=$($d.type)]"
            if ($d.resolved.Count -gt 0) {
                $line += " -> " + ($d.resolved -join ", ")
            }
            Write-Output $line
        }
    }
    Write-Output ""
}

Write-Output "--- End of Report ---"
