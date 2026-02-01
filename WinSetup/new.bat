@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion

echo ====================================
echo WinSetup Project Structure Generator
echo ====================================
echo.

set ROOT=%~dp0

echo [1/5] Creating base directories...
mkdir "%ROOT%src" 2>nul
mkdir "%ROOT%tests" 2>nul
mkdir "%ROOT%docs" 2>nul
mkdir "%ROOT%scripts" 2>nul

echo [2/5] Creating Layer 0: Abstractions...
mkdir "%ROOT%src\abstractions\platform" 2>nul
mkdir "%ROOT%src\abstractions\io" 2>nul
mkdir "%ROOT%src\abstractions\async" 2>nul
mkdir "%ROOT%src\abstractions\messaging" 2>nul
mkdir "%ROOT%src\abstractions\reactive" 2>nul

call :CreateBOMFile "%ROOT%src\abstractions\platform\ITextEncoder.h"
call :CreateBOMFile "%ROOT%src\abstractions\platform\IPlatformHandle.h"
call :CreateBOMFile "%ROOT%src\abstractions\platform\IWindowHandle.h"
call :CreateBOMFile "%ROOT%src\abstractions\platform\IThreadPool.h"
call :CreateBOMFile "%ROOT%src\abstractions\io\IFileSystem.h"
call :CreateBOMFile "%ROOT%src\abstractions\io\IStreamReader.h"
call :CreateBOMFile "%ROOT%src\abstractions\io\IStreamWriter.h"
call :CreateBOMFile "%ROOT%src\abstractions\io\IPath.h"
call :CreateBOMFile "%ROOT%src\abstractions\async\IExecutor.h"
call :CreateBOMFile "%ROOT%src\abstractions\async\IScheduler.h"
call :CreateBOMFile "%ROOT%src\abstractions\async\IAwaitable.h"
call :CreateBOMFile "%ROOT%src\abstractions\async\IAsyncContext.h"
call :CreateBOMFile "%ROOT%src\abstractions\messaging\IEventBus.h"
call :CreateBOMFile "%ROOT%src\abstractions\messaging\IDispatcher.h"
call :CreateBOMFile "%ROOT%src\abstractions\messaging\IMessageQueue.h"
call :CreateBOMFile "%ROOT%src\abstractions\reactive\IObservable.h"
call :CreateBOMFile "%ROOT%src\abstractions\reactive\IProperty.h"

echo [3/5] Creating Layer 1: Domain...
mkdir "%ROOT%src\domain\primitives" 2>nul
mkdir "%ROOT%src\domain\functional" 2>nul
mkdir "%ROOT%src\domain\memory" 2>nul
mkdir "%ROOT%src\domain\validation" 2>nul

call :CreateBOMFile "%ROOT%src\domain\primitives\Expected.h"
call :CreateBOMFile "%ROOT%src\domain\primitives\Result.h"
call :CreateBOMFile "%ROOT%src\domain\primitives\Error.h"
call :CreateBOMFile "%ROOT%src\domain\functional\Monads.h"
call :CreateBOMFile "%ROOT%src\domain\functional\Optional.h"
call :CreateBOMFile "%ROOT%src\domain\functional\Pipeline.h"
call :CreateBOMFile "%ROOT%src\domain\functional\Compose.h"
call :CreateBOMFile "%ROOT%src\domain\memory\SmartPtr.h"
call :CreateBOMFile "%ROOT%src\domain\memory\UniqueResource.h"
call :CreateBOMFile "%ROOT%src\domain\memory\SharedResource.h"
call :CreateBOMFile "%ROOT%src\domain\memory\PoolAllocator.h"
call :CreateBOMFile "%ROOT%src\domain\validation\PathValidator.h"
call :CreateBOMFile "%ROOT%src\domain\validation\PathValidator.cpp"
call :CreateBOMFile "%ROOT%src\domain\validation\ValidationRules.h"
call :CreateBOMFile "%ROOT%src\domain\validation\ValidationResult.h"

echo [4/5] Creating Layer 2: Application...
mkdir "%ROOT%src\application\async" 2>nul
mkdir "%ROOT%src\application\messaging" 2>nul
mkdir "%ROOT%src\application\reactive" 2>nul
mkdir "%ROOT%src\application\events" 2>nul

call :CreateBOMFile "%ROOT%src\application\async\Task.h"
call :CreateBOMFile "%ROOT%src\application\async\TaskScheduler.h"
call :CreateBOMFile "%ROOT%src\application\async\TaskScheduler.cpp"
call :CreateBOMFile "%ROOT%src\application\async\AsyncContext.h"
call :CreateBOMFile "%ROOT%src\application\async\AsyncContext.cpp"
call :CreateBOMFile "%ROOT%src\application\async\Awaitable.h"
call :CreateBOMFile "%ROOT%src\application\async\Promise.h"
call :CreateBOMFile "%ROOT%src\application\messaging\EventBus.h"
call :CreateBOMFile "%ROOT%src\application\messaging\EventBus.cpp"
call :CreateBOMFile "%ROOT%src\application\messaging\Dispatcher.h"
call :CreateBOMFile "%ROOT%src\application\messaging\Dispatcher.cpp"
call :CreateBOMFile "%ROOT%src\application\messaging\MessageQueue.h"
call :CreateBOMFile "%ROOT%src\application\reactive\Property.h"
call :CreateBOMFile "%ROOT%src\application\reactive\Observable.h"
call :CreateBOMFile "%ROOT%src\application\reactive\Subject.h"
call :CreateBOMFile "%ROOT%src\application\reactive\Operators.h"
call :CreateBOMFile "%ROOT%src\application\events\LogEvent.h"
call :CreateBOMFile "%ROOT%src\application\events\ProgressEvent.h"
call :CreateBOMFile "%ROOT%src\application\events\ErrorEvent.h"
call :CreateBOMFile "%ROOT%src\application\events\InstallEvent.h"

echo [5/5] Creating Layer 3: Adapters...
mkdir "%ROOT%src\adapters\platform\windows\encoding" 2>nul
mkdir "%ROOT%src\adapters\platform\windows\threading" 2>nul
mkdir "%ROOT%src\adapters\platform\windows\window" 2>nul
mkdir "%ROOT%src\adapters\platform\windows\filesystem" 2>nul

call :CreateBOMFile "%ROOT%src\adapters\platform\windows\encoding\Win32TextEncoder.h"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\encoding\Win32TextEncoder.cpp"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\threading\Win32ThreadPool.h"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\threading\Win32ThreadPool.cpp"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\threading\Win32Thread.h"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\threading\Win32Thread.cpp"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\window\Win32WindowHandle.h"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\window\Win32WindowHandle.cpp"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\window\Win32Handle.h"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\window\Win32Handle.cpp"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\filesystem\Win32FileSystem.h"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\filesystem\Win32FileSystem.cpp"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\filesystem\Win32File.h"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\filesystem\Win32File.cpp"
call :CreateBOMFile "%ROOT%src\adapters\platform\windows\Win32Factory.h"

echo [6/7] Creating Layer 4: Infrastructure...
mkdir "%ROOT%src\infrastructure\composition" 2>nul
mkdir "%ROOT%src\infrastructure\logging" 2>nul
mkdir "%ROOT%src\infrastructure\config" 2>nul
mkdir "%ROOT%src\infrastructure\diagnostics" 2>nul

call :CreateBOMFile "%ROOT%src\infrastructure\composition\DependencyContainer.h"
call :CreateBOMFile "%ROOT%src\infrastructure\composition\DependencyContainer.cpp"
call :CreateBOMFile "%ROOT%src\infrastructure\composition\ServiceLocator.h"
call :CreateBOMFile "%ROOT%src\infrastructure\logging\WindowsLogger.h"
call :CreateBOMFile "%ROOT%src\infrastructure\logging\WindowsLogger.cpp"
call :CreateBOMFile "%ROOT%src\infrastructure\config\ConfigLoader.h"
call :CreateBOMFile "%ROOT%src\infrastructure\config\ConfigLoader.cpp"
call :CreateBOMFile "%ROOT%src\infrastructure\diagnostics\PerformanceMonitor.h"
call :CreateBOMFile "%ROOT%src\infrastructure\diagnostics\PerformanceMonitor.cpp"
call :CreateBOMFile "%ROOT%src\infrastructure\main.cpp"

echo [7/7] Creating Test Structure...
mkdir "%ROOT%tests\domain" 2>nul
mkdir "%ROOT%tests\application" 2>nul
mkdir "%ROOT%tests\adapters" 2>nul
mkdir "%ROOT%tests\integration" 2>nul
mkdir "%ROOT%tests\mocks" 2>nul

call :CreateBOMFile "%ROOT%tests\domain\ExpectedTests.cpp"
call :CreateBOMFile "%ROOT%tests\domain\MonadsTests.cpp"
call :CreateBOMFile "%ROOT%tests\domain\PathValidatorTests.cpp"
call :CreateBOMFile "%ROOT%tests\domain\MemoryTests.cpp"
call :CreateBOMFile "%ROOT%tests\application\EventBusTests.cpp"
call :CreateBOMFile "%ROOT%tests\application\DispatcherTests.cpp"
call :CreateBOMFile "%ROOT%tests\application\TaskTests.cpp"
call :CreateBOMFile "%ROOT%tests\application\PropertyTests.cpp"
call :CreateBOMFile "%ROOT%tests\adapters\Win32TextEncoderTests.cpp"
call :CreateBOMFile "%ROOT%tests\adapters\Win32ThreadPoolTests.cpp"
call :CreateBOMFile "%ROOT%tests\adapters\Win32FileSystemTests.cpp"
call :CreateBOMFile "%ROOT%tests\integration\EndToEndTests.cpp"
call :CreateBOMFile "%ROOT%tests\integration\PerformanceTests.cpp"
call :CreateBOMFile "%ROOT%tests\mocks\MockTextEncoder.h"
call :CreateBOMFile "%ROOT%tests\mocks\MockWindowHandle.h"
call :CreateBOMFile "%ROOT%tests\mocks\MockThreadPool.h"
call :CreateBOMFile "%ROOT%tests\mocks\MockFileSystem.h"
call :CreateBOMFile "%ROOT%tests\mocks\MockEventBus.h"
call :CreateBOMFile "%ROOT%tests\TestMain.cpp"

echo [8/8] Creating Documentation and Scripts...
mkdir "%ROOT%docs\architecture" 2>nul
mkdir "%ROOT%docs\guides" 2>nul
mkdir "%ROOT%docs\api" 2>nul

call :CreateBOMFile "%ROOT%docs\architecture\layers.md"
call :CreateBOMFile "%ROOT%docs\architecture\dependencies.md"
call :CreateBOMFile "%ROOT%docs\architecture\patterns.md"
call :CreateBOMFile "%ROOT%docs\guides\testing.md"
call :CreateBOMFile "%ROOT%docs\guides\contributing.md"
call :CreateBOMFile "%ROOT%docs\guides\migration.md"
call :CreateBOMFile "%ROOT%docs\api\abstractions.md"
call :CreateBOMFile "%ROOT%docs\api\domain.md"
call :CreateBOMFile "%ROOT%docs\api\application.md"
call :CreateBOMFile "%ROOT%scripts\build.bat"
call :CreateBOMFile "%ROOT%scripts\clean.bat"
call :CreateBOMFile "%ROOT%scripts\test.bat"

echo [9/9] Creating Root Files and VS Solution Structure...
call :CreateBOMFile "%ROOT%WinSetup.sln"
call :CreateBOMFile "%ROOT%ARCHITECTURE.md"
call :CreateBOMFile "%ROOT%.gitignore"
call :CreateBOMFile "%ROOT%src\abstractions\Abstractions.vcxproj"
call :CreateBOMFile "%ROOT%src\abstractions\Abstractions.vcxproj.filters"
call :CreateBOMFile "%ROOT%src\domain\Domain.vcxproj"
call :CreateBOMFile "%ROOT%src\domain\Domain.vcxproj.filters"
call :CreateBOMFile "%ROOT%src\application\Application.vcxproj"
call :CreateBOMFile "%ROOT%src\application\Application.vcxproj.filters"
call :CreateBOMFile "%ROOT%src\adapters\Adapters.vcxproj"
call :CreateBOMFile "%ROOT%src\adapters\Adapters.vcxproj.filters"
call :CreateBOMFile "%ROOT%src\infrastructure\Infrastructure.vcxproj"
call :CreateBOMFile "%ROOT%src\infrastructure\Infrastructure.vcxproj.filters"
call :CreateBOMFile "%ROOT%tests\Tests.vcxproj"
call :CreateBOMFile "%ROOT%tests\Tests.vcxproj.filters"

echo.
echo ====================================
echo Project structure created successfully!
echo ====================================
echo.
echo Total files created: 120+
echo Encoding: UTF-8 with BOM
echo Build System: Visual Studio Solution
echo.
echo Project Structure:
echo   - WinSetup.sln (Main solution file)
echo   - Abstractions.vcxproj (Layer 0)
echo   - Domain.vcxproj (Layer 1)
echo   - Application.vcxproj (Layer 2)
echo   - Adapters.vcxproj (Layer 3)
echo   - Infrastructure.vcxproj (Layer 4 + Main Entry)
echo   - Tests.vcxproj (Google Test)
echo.
pause
exit /b 0

:CreateBOMFile
set "filepath=%~1"
echo.|set /p="ï»¿" > "%filepath%"
echo Created: %filepath%
exit /b 0
