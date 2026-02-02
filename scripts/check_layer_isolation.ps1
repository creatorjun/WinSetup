param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

function Test-AbstractionPurity {
    param([string]$Path)
    
    $violations = @()
    $files = Get-ChildItem -Path $Path -Filter *.h -Recurse
    
    foreach ($file in $files) {
        $content = Get-Content $file.FullName -Raw
        
        if ($content -match '(?<!virtual\s)(?<!= 0;\s*)\{[^}]*\}(?!\s*;)' -and 
            $content -notmatch 'namespace|struct.*\{\s*\}|class.*\{\s*\};') {
            $violations += "Implementation found in $($file.Name)"
        }
        
        if ($content -match '#include\s+<windows\.h>|#include\s+".*\.cpp"') {
            $violations += "Platform-specific include in $($file.Name)"
        }
    }
    
    return $violations
}

function Test-DomainPurity {
    param([string]$Path)
    
    $violations = @()
    $files = Get-ChildItem -Path $Path -Include *.h,*.cpp -Recurse
    
    foreach ($file in $files) {
        $content = Get-Content $file.FullName -Raw
        
        if ($content -match '#include\s+<windows\.h>|#include\s+<WinBase\.h>') {
            $violations += "Windows API dependency in $($file.Name)"
        }
        
        if ($content -match 'HWND|HANDLE|DWORD|HINSTANCE') {
            $violations += "Win32 types in $($file.Name)"
        }
    }
    
    return $violations
}

function Test-ApplicationPurity {
    param([string]$Path)
    
    $violations = @()
    $files = Get-ChildItem -Path $Path -Include *.h,*.cpp -Recurse
    
    foreach ($file in $files) {
        $content = Get-Content $file.FullName -Raw
        
        if ($content -match '#include\s+<windows\.h>') {
            $violations += "Direct Windows API dependency in $($file.Name)"
        }
        
        if ($content -match 'new\s+Win32|Win32\w+\s+\w+;') {
            $violations += "Direct instantiation of adapter in $($file.Name)"
        }
    }
    
    return $violations
}

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "  Layer Isolation Check" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host ""

$totalViolations = 0

Write-Host "Checking Abstractions layer..." -ForegroundColor Yellow
$abstractionViolations = Test-AbstractionPurity "$ProjectRoot/src/abstractions"
if ($abstractionViolations.Count -gt 0) {
    foreach ($v in $abstractionViolations) {
        Write-Host "  ❌ $v" -ForegroundColor Red
        $totalViolations++
    }
} else {
    Write-Host "  ✅ Clean" -ForegroundColor Green
}
Write-Host ""

Write-Host "Checking Domain layer..." -ForegroundColor Yellow
$domainViolations = Test-DomainPurity "$ProjectRoot/src/domain"
if ($domainViolations.Count -gt 0) {
    foreach ($v in $domainViolations) {
        Write-Host "  ❌ $v" -ForegroundColor Red
        $totalViolations++
    }
} else {
    Write-Host "  ✅ Clean" -ForegroundColor Green
}
Write-Host ""

Write-Host "Checking Application layer..." -ForegroundColor Yellow
$appViolations = Test-ApplicationPurity "$ProjectRoot/src/application"
if ($appViolations.Count -gt 0) {
    foreach ($v in $appViolations) {
        Write-Host "  ❌ $v" -ForegroundColor Red
        $totalViolations++
    }
} else {
    Write-Host "  ✅ Clean" -ForegroundColor Green
}
Write-Host ""

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "  Total Violations: $totalViolations" -ForegroundColor $(if($totalViolations -eq 0){'Green'}else{'Red'})
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host ""

if ($totalViolations -gt 0) {
    exit 1
}

exit 0
