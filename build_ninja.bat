@echo off
setlocal

set "ROOT_DIR=%~dp0"
set "PLUGIN_DIR=%ROOT_DIR%plugin"
set "BUILD_DIR=%PLUGIN_DIR%\build-ninja-win"
set "CONFIG=Release"
set "VSDEV_CMD=C:\BuildTools\Common7\Tools\VsDevCmd.bat"

if not exist "%PLUGIN_DIR%\CMakeLists.txt" (
    echo Expected plugin CMake project at: %PLUGIN_DIR%
    exit /b 1
)

if not exist "%VSDEV_CMD%" (
    echo VsDevCmd.bat was not found at %VSDEV_CMD%
    exit /b 1
)

call "%VSDEV_CMD%" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b 1

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake was not found on PATH.
    exit /b 1
)

where ninja >nul 2>nul
if errorlevel 1 (
    echo Ninja was not found on PATH.
    exit /b 1
)

where cl >nul 2>nul
if errorlevel 1 (
    echo cl.exe was not found on PATH after loading the MSVC environment.
    exit /b 1
)

if exist "%BUILD_DIR%" (
    echo Resetting Ninja build directory %BUILD_DIR%
    rmdir /s /q "%BUILD_DIR%"
    if exist "%BUILD_DIR%" (
        echo Failed to remove %BUILD_DIR%. Close any process using that folder and try again.
        exit /b 1
    )
)

echo Configuring TurboTuning in %BUILD_DIR%
cmake -S "%PLUGIN_DIR%" -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=%CONFIG% -DCMAKE_C_COMPILER=cl.exe -DCMAKE_CXX_COMPILER=cl.exe
if errorlevel 1 exit /b 1

echo Building project
cmake --build "%BUILD_DIR%" --config %CONFIG%
if errorlevel 1 exit /b 1

echo.
echo Build complete.
echo Standalone EXE output:
echo   %BUILD_DIR%\TurboTuning_artefacts\%CONFIG%\Standalone\Turbo Harp.exe
echo VST3 output:
echo   %BUILD_DIR%\TurboTuning_artefacts\%CONFIG%\VST3
