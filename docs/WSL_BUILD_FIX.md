# ⚠️ WSL 编译问题修复

## 🔴 问题

虽然 PowerShell 可以访问 UNC 路径（`\\wsl$\...`），但 **MSBuild 在编译时会调用 CMD**，导致以下错误：

```
CMD 不支持将 UNC 路径作为当前目录。
error MSB3073: 命令已退出，代码为 1。
```

## ✅ 解决方案：驱动器映射

### **方法 1：使用 subst 命令（推荐）**

```cmd
# 创建虚拟驱动器映射
subst Z: \\wsl$\Ubuntu\home\zd\A-Trader

# 验证映射成功
Z:
dir
```

> ⚠️ **注意**：`subst` 创建的是临时映射，**重启后会消失**，需要重新执行。

**永久化方案**：创建启动脚本 `setup_wsl_drive.bat`

```batch

@echo off
echo 正在映射 WSL 驱动器...
subst Z: \\wsl$\Ubuntu\home\zd\A-Trader
if %errorlevel% equ 0 (
    echo ✓ Z: 驱动器映射成功
) else (
    echo ✗ 映射失败，可能已经存在
)
```

将此脚本快捷方式放入启动文件夹（Win+R 运行 `shell:startup`）即可开机自动映射。

---

### **方法 2：使用 net use 命令（永久映射）**

```cmd
# 在新的 CMD 窗口中执行（不是 PowerShell）
net use Z: \\wsl$\Ubuntu\home\zd\A-Trader /persistent:yes

# 或者
net use Z: \\wsl.localhost\Ubuntu\home\zd\A-Trader /persistent:yes

# 验证映射成功
Z:
dir
```

> ⚠️ **注意**：如果提示"系统错误 64"，说明当前 PowerShell 正在使用 UNC 路径，请在新的 CMD 窗口中执行。

### **步骤 2：删除之前的编译目录**

```cmd
# 在 WSL 中删除
wsl
cd /home/zd/A-Trader/qt_manager
rm -rf build-windows
exit
```

### **步骤 3：使用映射驱动器编译**

```cmd
# 1. 打开 x64 Native Tools Command Prompt for VS 2022

# 2. 进入映射的驱动器
Z:
cd qt_manager

# 3. 创建编译目录
mkdir build-windows
cd build-windows

# 4. 配置 CMake
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DCMAKE_PREFIX_PATH=D:\Qt\6.10.2\msvc2022_64

# 5. 编译
cmake --build . --config Release
```

---

## 📝 完整流程（从头开始）

```cmd
REM === 1. 映射驱动器 ===
net use Z: \\wsl$\Ubuntu\home\zd\A-Trader /persistent:yes

REM === 2. 打开 VS 2022 Command Prompt ===
REM 在开始菜单搜索 "x64 Native Tools Command Prompt for VS 2022"

REM === 3. 进入项目 ===
Z:
cd qt_manager

REM === 4. 清理旧编译（如果存在）===
rmdir /s /q build-windows

REM === 5. 创建新编译目录 ===
mkdir build-windows
cd build-windows

REM === 6. 配置 CMake ===
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DCMAKE_PREFIX_PATH=D:\Qt\6.10.2\msvc2022_64

REM === 7. 编译 ===
cmake --build . --config Release

REM === 8. 创建配置文件 ===
cd Release
echo {"connection":{"server_address":"localhost","pub_port":5555,"rep_port":5556}} > config.json

REM === 9. 运行 ===
qt_manager.exe
```

---

## 🎯 关键点

1. ❌ **不要使用** `\\wsl$\...` 路径编译
2. ✅ **必须使用** 映射驱动器（Z:）
3. ✅ 使用 **VS Command Prompt**，不是普通 CMD
4. ✅ 配置文件使用 `localhost` 连接 WSL Core

---

## 🧪 验证

编译成功后应该看到：

```
[100%] Built target qt_manager
```

可执行文件位置：

```
Z:\qt_manager\build-windows\Release\qt_manager.exe
```

---

## 🚀 运行

```cmd
# 1. 在 WSL 中启动 Core
wsl
cd /home/zd/A-Trader/ctp_core/build
./ctp_core

# 2. 在 Windows 中启动 Qt Manager
Z:
cd qt_manager\build-windows\Release
qt_manager.exe
```

---

**现在应该可以正常编译了！** 🎉
