@echo off
REM A-Trader Qt Manager Windows Build Script

echo ========================================
echo A-Trader Qt Manager Windows Build
echo ========================================
echo.

REM Check if Z: drive is mapped
if not exist Z:\qt_manager (
    echo [ERROR] Z: drive not mapped!
    echo Please run: subst Z: \\wsl$\Ubuntu\home\zd\A-Trader
    pause
    exit /b 1
)

REM Enter project directory
Z:
cd qt_manager

REM Clean old build
if exist build-windows (
    echo [1/5] Cleaning old build directory...
    rmdir /s /q build-windows
)

REM Create build directory
echo [2/5] Creating build directory...
mkdir build-windows
cd build-windows

REM Configure CMake
echo [3/5] Configuring CMake...
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DCMAKE_PREFIX_PATH=D:\Qt\6.10.2\msvc2022_64

if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    echo Please check:
    echo 1. vcpkg has installed zeromq and cppzmq
    echo 2. Qt path is correct
    pause
    exit /b 1
)

REM Build
echo [4/5] Building project...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

REM Create config file
echo [5/5] Creating config file...
cd Release
if not exist config.json (
    echo {"connection":{"server_address":"localhost","pub_port":5555,"rep_port":5556}} > config.json
)

echo.
echo ========================================
echo Build Success!
echo ========================================
echo Executable: Z:\qt_manager\build-windows\Release\qt_manager.exe
echo.
echo Before running, make sure:
echo 1. CTP Core is running in WSL
echo 2. config.json is properly configured
echo.
pause
