# 🎉 Windows WSL 编译成功总结

## ✅ 已完成的工作

### 1. **代码修改**
- ✅ 修改 `CMakeLists.txt` 支持 vcpkg 的 ZeroMQ 包
- ✅ 支持动态配置 ZMQ 连接地址（`zmq_topics.h`）
- ✅ Qt Manager 加载 `config.json` 配置文件

### 2. **编译脚本**
- ✅ `build_windows.bat` - 自动化编译脚本
- ✅ `deploy_windows.bat` - 自动化部署脚本

### 3. **文档**
- ✅ `docs/WINDOWS_WSL_BUILD.md` - 完整的 Windows 编译指南
- ✅ `docs/QUICK_START_REMOTE.md` - 快速开始指南
- ✅ `docs/WSL_BUILD_FIX.md` - 紧急修复指南
- ✅ `docs/REMOTE_CONNECTION.md` - 远程连接详细说明

---

## 🚀 快速开始（从零到运行）

### **步骤 1：安装依赖（一次性）**

```cmd
REM 1. 安装 vcpkg
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

REM 2. 安装依赖包
.\vcpkg install cppzmq:x64-windows
.\vcpkg install nlohmann-json:x64-windows
.\vcpkg integrate install

REM 3. 安装 Qt 6.10.2 for MSVC 2022
REM 下载：https://www.qt.io/download-qt-installer
REM 选择组件：MSVC 2022 64-bit

REM 4. 安装 Visual Studio 2022
REM 下载：https://visualstudio.microsoft.com/
REM 选择工作负载：使用 C++ 的桌面开发
```

---

### **步骤 2：映射 WSL 驱动器**

```cmd
REM 映射 Z: 驱动器
subst Z: \\wsl$\Ubuntu\home\zd\A-Trader

REM 验证
Z:
dir
```

---

### **步骤 3：编译**

```cmd
REM 打开 x64 Native Tools Command Prompt for VS 2022

REM 进入项目
Z:
cd qt_manager

REM 编译
build_windows.bat
```

---

### **步骤 4：部署 Qt 依赖**

```cmd
REM 复制 Qt DLL
Z:
cd qt_manager
deploy_windows.bat
```

---

### **步骤 5：运行**

```cmd
REM 1. 在 WSL 中启动 CTP Core
wsl
cd /home/zd/A-Trader/ctp_core/build
./ctp_core

REM 2. 在 Windows 中启动 Qt Manager
Z:\qt_manager\build-windows\Release\qt_manager.exe
```

---

## 📋 关键文件位置

| 文件 | 位置 | 说明 |
|------|------|------|
| 源代码 | `Z:\qt_manager\` | WSL 中的代码 |
| 编译输出 | `Z:\qt_manager\build-windows\` | Windows 编译目录 |
| 可执行文件 | `Z:\qt_manager\build-windows\Release\qt_manager.exe` | 最终程序 |
| 配置文件 | `Z:\qt_manager\build-windows\Release\config.json` | 连接配置 |
| 编译脚本 | `Z:\qt_manager\build_windows.bat` | 自动化编译 |
| 部署脚本 | `Z:\qt_manager\deploy_windows.bat` | 自动化部署 |

---

## 🔧 配置文件

`Z:\qt_manager\build-windows\Release\config.json`：

```json
{
    "connection": {
        "server_address": "localhost",
        "pub_port": 5555,
        "rep_port": 5556
    }
}
```

> 💡 **说明**：使用 `localhost` 连接 WSL 中的 CTP Core

---

## 📦 依赖清单

### **Windows 依赖**
- Visual Studio 2022 (MSVC)
- Qt 6.10.2 for MSVC 2022 64-bit
- vcpkg
  - `cppzmq:x64-windows` (ZeroMQ C++ 头文件)
  - `nlohmann-json:x64-windows` (JSON 库)

### **WSL 依赖**
- CTP Core（已编译）
- PostgreSQL（已配置）

---

## 🎯 工作流程

```
┌─────────────────────────────────────────────────────┐
│                   开发流程                           │
├─────────────────────────────────────────────────────┤
│                                                      │
│  📝 代码编辑                                         │
│     └─ VSCode (WSL 扩展) - 在 WSL 中编辑           │
│                                                      │
│  🔧 CTP Core                                        │
│     ├─ 编译: WSL Ubuntu 终端                        │
│     └─ 运行: WSL Ubuntu 终端                        │
│                                                      │
│  🖥️ Qt Manager                                      │
│     ├─ Linux 开发测试: WSL 中编译                   │
│     └─ Windows 生产发布: Windows 中编译             │
│                                                      │
│  🌐 网络连接                                         │
│     └─ Windows Qt Manager → localhost → WSL Core   │
│                                                      │
└─────────────────────────────────────────────────────┘
```

---

## ⚠️ 常见问题

### Q1: 找不到 `zmq.hpp`
**A:** 安装 `cppzmq`：
```cmd
C:\vcpkg\vcpkg install cppzmq:x64-windows
```

### Q2: 找不到 Qt DLL
**A:** 运行部署脚本：
```cmd
Z:\qt_manager\deploy_windows.bat
```

### Q3: CMD 不支持 UNC 路径
**A:** 使用 `subst` 映射驱动器：
```cmd
subst Z: \\wsl$\Ubuntu\home\zd\A-Trader
```

### Q4: 重启后 Z: 驱动器消失
**A:** `subst` 是临时映射，重启后需要重新执行。可以创建启动脚本自动化。

### Q5: 连接不上 WSL Core
**A:** 检查：
1. WSL Core 是否正在运行
2. `config.json` 中地址是否为 `localhost`
3. 端口 5555/5556 是否被占用

---

## 🎉 成功标志

编译成功后应该看到：
```
========================================
Build Success!
========================================
Executable: Z:\qt_manager\build-windows\Release\qt_manager.exe
```

部署成功后应该看到：
```
========================================
Deployment Success!
========================================
```

运行后应该看到 Qt Manager 窗口，并能连接到 WSL 中的 CTP Core。

---

## 📚 相关文档

- **完整编译指南**：`docs/WINDOWS_WSL_BUILD.md`
- **快速开始**：`docs/QUICK_START_REMOTE.md`
- **紧急修复**：`docs/WSL_BUILD_FIX.md`
- **远程连接**：`docs/REMOTE_CONNECTION.md`

---

**祝您使用愉快！** 🎉
