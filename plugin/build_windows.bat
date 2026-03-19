@echo off
REM ── Turbo Tuning — Windows Build Script ──
REM Builds VST3 and Standalone using CMake + Visual Studio
setlocal

set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%build-vs2022"
set "JUCE_SOURCE_DIR=%BUILD_DIR%\_deps\juce-src"

echo.
echo ============================================
echo   TURBO TUNING — Windows Build
echo ============================================
echo.

REM Configure
echo [1/3] Configuring CMake...
if exist "%JUCE_SOURCE_DIR%\CMakeLists.txt" (
    cmake -S "%SCRIPT_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 -DJUCE_SOURCE_DIR:PATH="%JUCE_SOURCE_DIR%"
) else (
    cmake -S "%SCRIPT_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
)
if errorlevel 1 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

REM Build (Release)
echo.
echo [2/3] Building Release...
cmake --build %BUILD_DIR% --config Release --parallel
if errorlevel 1 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Build complete!
echo.
echo Output locations:
echo   VST3:       %BUILD_DIR%\TurboTuning_artefacts\Release\VST3\
echo   Standalone: %BUILD_DIR%\TurboTuning_artefacts\Release\Standalone\Turbo Harp.exe
echo.
pause
