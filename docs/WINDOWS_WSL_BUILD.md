# 🪟 WSL 环境下 Windows 编译指南

## 🎯 优势

由于您使用 **WSL (Windows Subsystem for Linux)**，Windows 可以直接访问 WSL 文件系统：

- ✅ **无需复制代码**：Windows 直接访问 WSL 文件
- ✅ **代码同步**：修改一处，两边都生效
- ✅ **节省空间**：不需要维护两份代码

---

## 📂 WSL 文件系统访问

### **从 Windows 访问 WSL 文件**

在 Windows 文件资源管理器中输入：

```
\\wsl$\Ubuntu\home\zd\A-Trader
```

或者在命令行中：

```cmd
cd \\wsl$\Ubuntu\home\zd\A-Trader\qt_manager
```

> 💡 **提示**：`Ubuntu` 是您的 WSL 发行版名称，如果使用其他发行版请替换

---

## 🛠️ Windows 编译步骤

### **前置准备**

#### 1. 安装 Visual Studio 2022

- 下载：https://visualstudio.microsoft.com/
- 安装时选择：**使用 C++ 的桌面开发**

#### 2. 安装 Qt 6.x for Windows

- 下载：https://www.qt.io/download-qt-installer
- 选择：**MSVC 2022 64-bit** 组件

#### 3. 安装 vcpkg（包管理器）

```cmd
# 在 C:\ 下安装 vcpkg
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

#### 4. 安装依赖库

```cmd
# 安装 ZeroMQ 和 nlohmann-json
C:\vcpkg\vcpkg install zeromq:x64-windows
C:\vcpkg\vcpkg install nlohmann-json:x64-windows

# 集成到 Visual Studio
C:\vcpkg\vcpkg integrate install
```

---

## 🔨 编译 Qt Manager

### **方法 1：使用 Visual Studio（推荐）**

```cmd
# 1. 打开 x64 Native Tools Command Prompt for VS 2022

# 2. 进入 WSL 项目目录
cd \\wsl$\Ubuntu\home\zd\A-Trader\qt_manager

# 3. 创建 Windows 编译目录
mkdir build-windows
cd build-windows

# 4. 配置 CMake（使用 vcpkg）
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DCMAKE_PREFIX_PATH=C:\Qt\6.8.0\msvc2022_64

# 5. 编译
cmake --build . --config Release

# 6. 可执行文件位置
# \\wsl$\Ubuntu\home\zd\A-Trader\qt_manager\build-windows\Release\qt_manager.exe
```

### **方法 2：使用 Qt Creator（更简单）**

```cmd
# 1. 启动 Qt Creator

# 2. 打开项目
File -> Open File or Project
选择：\\wsl$\Ubuntu\home\zd\A-Trader\qt_manager\CMakeLists.txt

# 3. 配置 Kit
选择：Desktop Qt 6.x MSVC2022 64bit

# 4. 配置 CMake 参数
在 Projects -> Build Settings -> CMake 中添加：
CMAKE_TOOLCHAIN_FILE = C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# 5. 点击 Build -> Build Project
```

---

## 📝 配置文件

### **创建 Windows 配置**

在 `\\wsl$\Ubuntu\home\zd\A-Trader\qt_manager\build-windows\` 目录下创建 `config.json`：

```json
{
    "connection": {
        "server_address": "localhost",
        "pub_port": 5555,
        "rep_port": 5556,
        "comment": "Windows 通过 WSL 访问 Core，使用 localhost"
    }
}
```

> 💡 **重要**：由于 Core 运行在 WSL 中，Windows 访问 WSL 服务使用 `localhost` 即可！

---

## 🚀 运行

### **启动 CTP Core（在 WSL 中）**

```bash
# 在 WSL Ubuntu 终端
cd /home/zd/A-Trader/ctp_core/build
./ctp_core
```

### **启动 Qt Manager（在 Windows 中）**

```cmd
# 方法 1：命令行
cd \\wsl$\Ubuntu\home\zd\A-Trader\qt_manager\build-windows\Release
qt_manager.exe

# 方法 2：双击运行
# 在文件资源管理器中双击 qt_manager.exe
```

---

## 🔍 WSL 网络说明

### **WSL2 网络模式**

WSL2 使用虚拟网络，但 Windows 可以通过 `localhost` 访问 WSL 服务：

```
┌─────────────────────────────────────────┐
│           Windows 主机                   │
│  ┌────────────────────────────────────┐ │
│  │  Qt Manager (qt_manager.exe)      │ │
│  │  连接到: localhost:5555/5556       │ │
│  └──────────────┬─────────────────────┘ │
│                 │ localhost              │
│  ┌──────────────▼─────────────────────┐ │
│  │  WSL2 (Ubuntu)                     │ │
│  │  ┌──────────────────────────────┐  │ │
│  │  │  CTP Core                    │  │ │
│  │  │  监听: 0.0.0.0:5555/5556     │  │ │
│  │  └──────────────────────────────┘  │ │
│  └────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

---

## ✅ 验证连接

### **1. 检查 WSL 端口监听**

```bash
# 在 WSL 中
netstat -tuln | grep 555
# 应输出：
# tcp  0.0.0.0:5555  LISTEN
# tcp  0.0.0.0:5556  LISTEN
```

### **2. 从 Windows 测试连接**

```cmd
# 使用 PowerShell
Test-NetConnection -ComputerName localhost -Port 5555
Test-NetConnection -ComputerName localhost -Port 5556

# 或使用 telnet
telnet localhost 5555
```

---

## 🎨 开发工作流

### **推荐工作流**

```
1. 代码编辑
   ├─ Linux: VSCode (WSL 扩展)
   └─ Windows: VSCode / Qt Creator

2. CTP Core 编译和运行
   └─ WSL Ubuntu 终端

3. Qt Manager 编译
   ├─ Linux 测试: WSL 中编译
   └─ Windows 发布: Windows 中编译

4. 调试
   ├─ Linux: GDB
   └─ Windows: Visual Studio Debugger
```

---

## 📦 部署

### **打包 Windows 版本**

```cmd
# 1. 复制可执行文件
copy \\wsl$\Ubuntu\home\zd\A-Trader\qt_manager\build-windows\Release\qt_manager.exe C:\A-Trader\

# 2. 复制 Qt 依赖（使用 windeployqt）
cd C:\Qt\6.8.0\msvc2022_64\bin
windeployqt.exe C:\A-Trader\qt_manager.exe

# 3. 复制配置文件
copy \\wsl$\Ubuntu\home\zd\A-Trader\qt_manager\config.example.json C:\A-Trader\config.json

# 4. 现在可以分发 C:\A-Trader\ 目录
```

---

## ❓ 常见问题

### Q: WSL 文件访问慢怎么办？

**A:** 编译时确实会比原生 Windows 慢一些，但可以接受。如果需要更快速度：
- 将编译输出目录 `build-windows` 放在 Windows 文件系统（如 `C:\Temp\build-windows`）
- 源代码仍在 WSL，只有编译产物在 Windows

### Q: 找不到 `\\wsl$\Ubuntu` 路径？

**A:** 
1. 确保 WSL 正在运行：`wsl`
2. 检查发行版名称：`wsl -l -v`
3. 使用正确的发行版名称替换 `Ubuntu`

### Q: Windows 编译后在 Linux 能用吗？

**A:** 不能！Windows 和 Linux 二进制不兼容：
- Windows 编译 → 只能在 Windows 运行
- Linux 编译 → 只能在 Linux 运行

### Q: 需要在 Windows 安装 PostgreSQL 吗？

**A:** 不需要！Qt Manager 不直接访问数据库，只通过 Core 访问。

---

## 🎯 总结

使用 WSL 的优势：

| 特性 | WSL 方案 | 传统方案 |
|------|----------|----------|
| 代码同步 | ✅ 自动 | ❌ 手动复制 |
| 磁盘占用 | ✅ 单份代码 | ❌ 双份代码 |
| 开发效率 | ✅ 高 | ❌ 低 |
| 网络配置 | ✅ localhost | ❌ 需配置 IP |

---

## 📚 参考资料

- WSL 文档：https://docs.microsoft.com/zh-cn/windows/wsl/
- Qt for Windows：https://doc.qt.io/qt-6/windows.html
- vcpkg：https://vcpkg.io/

---

**现在您可以在 Windows 下直接编译，无需复制代码！** 🎉
