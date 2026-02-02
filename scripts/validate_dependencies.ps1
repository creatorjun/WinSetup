param(
    [Parameter(Mandatory=$false)]
    [string]$ProjectRoot = ".."
)

$ErrorActionPreference = "Stop"

try {
    Write-Host "==================================================" -ForegroundColor Cyan
    Write-Host "  WinSetup Dependency Check" -ForegroundColor Cyan
    Write-Host "==================================================" -ForegroundColor Cyan
    Write-Host ""

    $scriptDir = $PSScriptRoot
    
    if ([System.IO.Path]::IsPathRooted($ProjectRoot)) {
        $projectRootPath = $ProjectRoot
    } else {
        $projectRootPath = Join-Path $scriptDir $ProjectRoot
        $projectRootPath = [System.IO.Path]::GetFullPath($projectRootPath)
    }

    Write-Host "[INFO] Script Location: $scriptDir" -ForegroundColor Gray
    Write-Host "[INFO] Project Root: $projectRootPath" -ForegroundColor Gray
    Write-Host ""

    $srcPath = Join-Path $projectRootPath "WinSetup\src"
    
    if (-not (Test-Path $srcPath)) {
        Write-Host "[WARN] Source directory not found: $srcPath" -ForegroundColor Yellow
        Write-Host "[SKIP] Validation skipped" -ForegroundColor Yellow
        Write-Host ""
        exit 0
    }

    Write-Host "[INFO] Source directory: $srcPath" -ForegroundColor Green
    Write-Host ""

    $layers = @{
        "abstractions" = @{ Level = 0; Allowed = @() }
        "domain"       = @{ Level = 1; Allowed = @("abstractions") }
        "application"  = @{ Level = 2; Allowed = @("abstractions", "domain") }
        "adapters"     = @{ Level = 3; Allowed = @("abstractions", "domain") }
        "infrastructure" = @{ Level = 4; Allowed = @("abstractions", "domain", "application", "adapters") }
    }

    Write-Host "Layer Rules:" -ForegroundColor Yellow
    foreach ($layer in $layers.Keys | Sort-Object) {
        $config = $layers[$layer]
        $deps = if ($config.Allowed.Count -gt 0) { $config.Allowed -join ", " } else { "NONE" }
        Write-Host ("  L{0}: {1,-15} -> [{2}]" -f $config.Level, $layer, $deps) -ForegroundColor Gray
    }
    Write-Host ""

    $files = @(Get-ChildItem -Path $srcPath -Include *.h,*.cpp -Recurse -ErrorAction SilentlyContinue)
    
    if ($files.Count -eq 0) {
        Write-Host "[SKIP] No source files found" -ForegroundColor Yellow
        exit 0
    }

    Write-Host "[SCAN] Checking $($files.Count) files..." -ForegroundColor Gray
    Write-Host ""

    $violations = 0
    $stdLibs = @('Windows\.h', 'windows\.h', 'iostream', 'memory', 'string', 'vector', 'algorithm', 
                 'functional', 'stdexcept', 'optional', 'variant', 'format', 'chrono', 'atomic',
                 'utility', 'type_traits', 'new', 'cstddef', 'cstdint', 'cstring')

    foreach ($file in $files) {
        $relativePath = $file.FullName.Replace("$projectRootPath\", "")
        
        $currentLayer = $null
        foreach ($layer in $layers.Keys) {
            if ($relativePath -match "src[\\/]$layer[\\/]") {
                $currentLayer = $layer
                break
            }
        }

        if (-not $currentLayer) { continue }

        $content = Get-Content $file.FullName -Raw -ErrorAction SilentlyContinue
        if (-not $content) { continue }

        $includes = [regex]::Matches($content, '#include\s+["<]([^">]+)[">]')

        foreach ($match in $includes) {
            $include = $match.Groups[1].Value
            
            $isStdLib = $false
            foreach ($std in $stdLibs) {
                if ($include -match "^$std") {
                    $isStdLib = $true
                    break
                }
            }
            if ($isStdLib) { continue }

            foreach ($targetLayer in $layers.Keys) {
                if ($include -match "src[\\/]$targetLayer[\\/]") {
                    $allowedDeps = $layers[$currentLayer].Allowed
                    
                    if ($allowedDeps -notcontains $targetLayer) {
                        $violations++
                        Write-Host ""
                        Write-Host "  [X] VIOLATION #$violations" -ForegroundColor Red
                        Write-Host "      File: $relativePath" -ForegroundColor Red
                        Write-Host "      Layer: $currentLayer (L$($layers[$currentLayer].Level))" -ForegroundColor Yellow
                        Write-Host "      Include: $include" -ForegroundColor Yellow
                        Write-Host "      Target: $targetLayer (L$($layers[$targetLayer].Level))" -ForegroundColor Yellow
                        Write-Host "      Allowed: [$($allowedDeps -join ', ')]" -ForegroundColor Cyan
                    }
                    break
                }
            }
        }
    }

    Write-Host ""
    Write-Host "==================================================" -ForegroundColor Cyan
    Write-Host "  Result: $violations violations in $($files.Count) files" -ForegroundColor $(if($violations -eq 0){'Green'}else{'Red'})
    Write-Host "==================================================" -ForegroundColor Cyan
    Write-Host ""

    if ($violations -gt 0) {
        Write-Host "[FAIL] Architecture violations detected!" -ForegroundColor Red
        exit 1
    }

    Write-Host "[PASS] Clean architecture verified!" -ForegroundColor Green
    exit 0

} catch {
    Write-Host ""
    Write-Host "[ERROR] $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "[ERROR] Stack: $($_.ScriptStackTrace)" -ForegroundColor DarkGray
    exit 1
}
