@echo off
REM A-Trader Qt Manager Windows Deployment Script

echo ========================================
echo A-Trader Qt Manager Deployment
echo ========================================
echo.

REM Check if executable exists
if not exist Z:\qt_manager\build-windows\Release\qt_manager.exe (
    echo [ERROR] qt_manager.exe not found!
    echo Please build the project first using build_windows.bat
    echo.
    echo Expected location: Z:\qt_manager\build-windows\Release\qt_manager.exe
    pause
    exit /b 1
)

echo [1/3] Deploying Qt dependencies...

REM Run windeployqt
D:\Qt\6.10.2\msvc2022_64\bin\windeployqt.exe ^
    Z:\qt_manager\build-windows\Release\qt_manager.exe ^
    --qmldir Z:\qt_manager\qml

if %errorlevel% neq 0 (
    echo [ERROR] windeployqt failed!
    echo Please check Qt installation path: D:\Qt\6.10.2\msvc2022_64\bin
    pause
    exit /b 1
)

echo [2/3] Copying vcpkg DLLs...

REM Copy ZeroMQ DLL
copy C:\vcpkg\installed\x64-windows\bin\libzmq-*.dll Z:\qt_manager\build-windows\Release\ >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] Could not copy ZeroMQ DLL, it might already exist or not be needed
)

echo [3/3] Creating config file...

REM Create config file if not exists
if not exist Z:\qt_manager\build-windows\Release\config.json (
    echo {"connection":{"server_address":"localhost","pub_port":5555,"rep_port":5556}} > Z:\qt_manager\build-windows\Release\config.json
)

echo.
echo ========================================
echo Deployment Success!
echo ========================================
echo.
echo You can now run: Z:\qt_manager\build-windows\Release\qt_manager.exe
echo.
echo Before running, make sure:
echo 1. CTP Core is running in WSL
echo 2. config.json is properly configured
echo.
pause
