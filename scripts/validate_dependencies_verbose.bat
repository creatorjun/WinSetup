@echo off
setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%validate_dependencies.ps1" -ProjectRoot "%PROJECT_ROOT%" -Verbose

if %ERRORLEVEL% neq 0 (
    exit /b 1
)

exit /b 0
