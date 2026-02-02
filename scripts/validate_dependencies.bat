@echo off
setlocal

echo ================================================
echo   WinSetup Dependency Validation
echo ================================================
echo.
echo Current Directory: %CD%
echo Script Directory: %~dp0
echo.

set SCRIPT_DIR=%~dp0
set PS_SCRIPT=%SCRIPT_DIR%validate_dependencies.ps1
set PROJECT_ROOT=%SCRIPT_DIR%..

echo PS Script Path: %PS_SCRIPT%
echo Project Root: %PROJECT_ROOT%
echo.

if not exist "%PS_SCRIPT%" (
    echo ERROR: PowerShell script not found!
    echo Expected at: %PS_SCRIPT%
    echo.
    echo Directory contents:
    dir "%SCRIPT_DIR%"
    exit /b 1
)

echo Running PowerShell validation...
echo.

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "& { Set-Location '%SCRIPT_DIR%'; & '%PS_SCRIPT%' -ProjectRoot '%PROJECT_ROOT%' }"

set RESULT=%ERRORLEVEL%

if %RESULT% neq 0 (
    echo.
    echo Validation FAILED with code: %RESULT%
    exit /b %RESULT%
)

echo.
echo Validation PASSED
exit /b 0
