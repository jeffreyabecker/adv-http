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
    [string]$OutDir = "$PSScriptRoot\out",
    [switch]$StrictMode = $false
)

$SrcDir = (Resolve-Path $SrcDir).Path
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# Updated regex to capture bracket type: group 1 = bracket type (" or <), group 2 = include path
$includeRegex = '#\s*include\s*([<"])(.+?)[>"]'

function Resolve-Include {
    param(
        [string]$IncludingFileDir,
        [string]$IncludeText,
        [string]$SrcDir
    )
    # returns array of resolved paths (absolute) under $SrcDir or empty

    $results = @()

    # If include contains path separators (relative), resolve relative to including file dir
    if ($IncludeText -match '[/\\]') {
        # Normalize path separators to forward slashes for consistency
        $normalizedInclude = $IncludeText -replace '\\', '/'
        
        # Resolve relative to including file's directory
        $candidate = Join-Path $IncludingFileDir $normalizedInclude
        $candidate = (($candidate -replace '\\', '/') -replace '/+', '/') # normalize
        
        if (Test-Path $candidate) { 
            $results += (Resolve-Path $candidate).Path 
        }
        else {
            # Try from SrcDir root as fallback (for legacy includes)
            $candidate2 = Join-Path $SrcDir $normalizedInclude
            $candidate2 = (($candidate2 -replace '\\', '/') -replace '/+', '/')
            if (Test-Path $candidate2) { 
                $results += (Resolve-Path $candidate2).Path 
            }
        }
    }
    else {
        # No path separators; search by file name under src dir (flat includes like "FileName.h")
        # This handles legacy code or files not yet in subfolders
        $matches = Get-ChildItem -Path $SrcDir -Recurse -File -Include $IncludeText  -ErrorAction SilentlyContinue
        foreach ($m in $matches) { $results += $m.FullName }
    }

    return $results | Select-Object -Unique
}

$allFiles = Get-ChildItem -Path $SrcDir -Recurse -File -Include *.h,*.hpp,*.c,*.cpp,*.cc | Sort-Object FullName

$report = @()
$unresolvedIncludes = @()
$hierarchyIssues = @()

foreach ($file in $allFiles) {
    $text = Get-Content -Raw -Path $file.FullName -ErrorAction SilentlyContinue
    if (-not $text) { continue }

    $matches = [regex]::Matches($text, $includeRegex)
    $deps = @()
    foreach ($m in $matches) {
        $bracketType = $m.Groups[1].Value  # '<' for system, '"' for project
        $inc = $m.Groups[2].Value.Trim()   # The include path
        $resolved = Resolve-Include -IncludingFileDir $file.DirectoryName -IncludeText $inc -SrcDir $SrcDir
        $isProject = $false
        if ($resolved.Count -gt 0) { $isProject = $true }

        # Determine type: 
        # - Angle brackets (<>) = system/external header
        # - Quotes ("") = project include
        # - If quotes but not resolved = error
        # - If angle brackets = always external, even if has path separators
        if ($bracketType -eq '<') {
            $depType = 'external'
        }
        elseif ($isProject) {
            $depType = 'project'
        }
        else {
            $depType = 'external'
        }

        # Track unresolved project includes (only those in quotes that weren't found)
        if ($bracketType -eq '"' -and -not $isProject) {
            $unresolvedIncludes += [pscustomobject]@{
                file = $file.FullName
                relative = $file.FullName.Substring($SrcDir.Length).TrimStart('\','/')
                include = $inc
            }
        }

        # Track hierarchy issues: files in subfolders including root-level files without paths
        if ($file.DirectoryName -ne $SrcDir -and $inc -notmatch '[/\\]' -and $isProject) {
            # File is in a subfolder, includes a file by name only
            # Check if it's a cross-folder include (not in same folder)
            $resolvedDir = (Split-Path $resolved[0] -Parent)
            if ($resolvedDir -ne $file.DirectoryName) {
                $hierarchyIssues += [pscustomobject]@{
                    file = $file.FullName
                    relative = $file.FullName.Substring($SrcDir.Length).TrimStart('\','/')
                    include = $inc
                    resolvedTo = $resolved[0].Substring($SrcDir.Length).TrimStart('\','/')
                    message = "Cross-folder reference by name (should use relative path)"
                }
            }
        }

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
                $resolvedPaths = $d.resolved | ForEach-Object { $_.Substring($SrcDir.Length).TrimStart('\','/') }
                $line += " -> " + ($resolvedPaths -join ", ")
            }
            Write-Output $line
        }
    }
    Write-Output ""
}

# Report unresolved includes
if ($unresolvedIncludes.Count -gt 0) {
    Write-Output ""
    Write-Output "=== UNRESOLVED INCLUDES (Errors) ==="
    $unresolvedIncludes | ForEach-Object {
        Write-Output "ERROR in $($_.relative)"
        Write-Output "  Include '$($_.include)' not found"
    }
    Write-Output ""
}

# Report hierarchy issues
if ($hierarchyIssues.Count -gt 0) {
    Write-Output ""
    Write-Output "=== HIERARCHY ISSUES (Warnings) ==="
    Write-Output "Files in subfolders should use explicit relative paths for cross-folder includes:"
    Write-Output ""
    $hierarchyIssues | ForEach-Object {
        Write-Output "WARNING in $($_.relative)"
        Write-Output "  Include '$($_.include)' resolves to: $($_.resolvedTo)"
        Write-Output "  Issue: $($_.message)"
    }
    Write-Output ""
}

# Summary
Write-Output "=== SUMMARY ==="
Write-Output "Total files scanned: $($allFiles.Count)"
Write-Output "Total includes found: $($report | ForEach-Object { $_.dependencies.Count } | Measure-Object -Sum | Select-Object -ExpandProperty Sum)"
Write-Output "Project includes: $($report | ForEach-Object { $_.dependencies | Where-Object { $_.type -eq 'project' } } | Measure-Object | Select-Object -ExpandProperty Count)"
Write-Output "External includes: $($report | ForEach-Object { $_.dependencies | Where-Object { $_.type -eq 'external' } } | Measure-Object | Select-Object -ExpandProperty Count)"
Write-Output "Unresolved includes: $($unresolvedIncludes.Count)"
Write-Output "Hierarchy issues: $($hierarchyIssues.Count)"

if ($unresolvedIncludes.Count -gt 0 -and $StrictMode) {
    Write-Output ""
    Write-Output "STRICT MODE: Failures detected!"
    exit 1
}

Write-Output ""
Write-Output "--- End of Report ---"
