@echo off
REM ========================================================
REM A-Trader Fast Launch Script for Development
REM ========================================================
REM This script sets up temporary environments to run the app
REM without copying all DLLs via windeployqt.
REM ========================================================

echo [Dev] Setting up environment...

REM 1. Set Qt Environment Variables
set QT_BIN_DIR=D:\Qt\6.10.2\msvc2022_64\bin
set PATH=%QT_BIN_DIR%;%PATH%

REM 2. Set vcpkg Environment Variables (ZeroMQ)
set VCPKG_BIN_DIR=C:\vcpkg\installed\x64-windows\bin
set PATH=%VCPKG_BIN_DIR%;%PATH%

REM 3. Check Executable
set EXE_PATH=Z:\qt_manager\build-windows\Release\qt_manager.exe

if not exist "%EXE_PATH%" (
    echo [ERROR] Executable not found: %EXE_PATH%
    echo Please run build_windows.bat first.
    pause
    exit /b 1
)

REM 4. Check Config File
if not exist "Z:\qt_manager\build-windows\Release\config.json" (
    echo [Info] Creating default config.json...
    echo {"connection":{"server_address":"localhost","pub_port":5555,"rep_port":5556}} > "Z:\qt_manager\build-windows\Release\config.json"
)

echo [Dev] Starting application...
start "" "%EXE_PATH%"

echo [Dev] Done.
