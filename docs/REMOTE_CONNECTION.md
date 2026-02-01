# A-Trader 远程连接配置指南

## 📋 概述

A-Trader 已支持**远程连接**功能，允许您在 Windows 上运行 Qt Manager 客户端，连接到 Linux 服务器上的 CTP Core。

---

## 🏗️ 架构说明

```
┌─────────────────────────────────────────────────────────────┐
│                    开发模式 (Linux)                          │
│  ┌──────────────┐         ┌──────────────┐                 │
│  │  CTP Core    │ ◄─────► │ Qt Manager   │                 │
│  │ (localhost)  │  TCP    │ (localhost)  │                 │
│  └──────────────┘  5555   └──────────────┘                 │
│                    5556                                      │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    生产模式 (跨平台)                         │
│  Linux 服务器                Windows 客户端                  │
│  ┌──────────────┐         ┌──────────────┐                 │
│  │  CTP Core    │ ◄─────► │ Qt Manager   │                 │
│  │ (0.0.0.0)    │  TCP    │ (远程连接)    │                 │
│  └──────────────┘  LAN    └──────────────┘                 │
│   端口: 5555/5556                                            │
└─────────────────────────────────────────────────────────────┘
```

---

## ⚙️ 配置步骤

### 1️⃣ **Linux 服务器端 (CTP Core)**

CTP Core 已经默认监听所有网卡 (`tcp://*:5555` 和 `tcp://*:5556`)，无需修改代码。

#### 检查防火墙设置

```bash
# 允许 5555 和 5556 端口
sudo ufw allow 5555/tcp
sudo ufw allow 5556/tcp

# 或者只允许特定 IP 访问（推荐）
sudo ufw allow from 192.168.1.100 to any port 5555
sudo ufw allow from 192.168.1.100 to any port 5556
```

#### 启动 CTP Core

```bash
cd /home/zd/A-Trader/ctp_core/build
./ctp_core
```

---

### 2️⃣ **Qt Manager 配置**

#### **Linux 开发模式**（本地连接）

编辑 `qt_manager/config.json`：

```json
{
    "connection": {
        "server_address": "127.0.0.1",
        "pub_port": 5555,
        "rep_port": 5556,
        "comment": "本地开发使用 localhost"
    }
}
```

#### **Windows 生产模式**（远程连接）

将 `config.json` 复制到 Windows Qt Manager 可执行文件同目录，并修改：

```json
{
    "connection": {
        "server_address": "192.168.1.10",
        "pub_port": 5555,
        "rep_port": 5556,
        "comment": "替换为 Linux 服务器的 IP 地址"
    }
}
```

---

## 🪟 Windows 编译指南

### **方案 A：使用 MSVC（推荐）**

```cmd
# 1. 安装 Visual Studio 2022 (包含 C++ 工具)
# 2. 安装 Qt 6.x for MSVC
# 3. 安装 vcpkg 并安装依赖

vcpkg install zeromq:x64-windows
vcpkg install nlohmann-json:x64-windows

# 4. 编译
cd A-Trader\qt_manager
mkdir build-windows
cd build-windows
cmake .. -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

### **方案 B：使用 MinGW**

```cmd
# 1. 安装 Qt 6.x for MinGW
# 2. 安装 MSYS2 并安装依赖

pacman -S mingw-w64-x86_64-zeromq
pacman -S mingw-w64-x86_64-nlohmann-json

# 3. 编译
cd A-Trader\qt_manager
mkdir build-windows
cd build-windows
cmake .. -G "MinGW Makefiles"
cmake --build .
```

---

## 🔒 安全建议

### **1. 使用 SSH 隧道（最简单）**

在 Windows 上建立 SSH 隧道，流量自动加密：

```cmd
ssh -L 5555:localhost:5555 -L 5556:localhost:5556 user@linux-server
```

然后 `config.json` 使用 `127.0.0.1`，流量通过加密隧道传输。

### **2. 配置防火墙**

```bash
# 只允许特定 IP 访问
sudo ufw allow from 192.168.1.100 to any port 5555
sudo ufw allow from 192.168.1.100 to any port 5556
```

### **3. 使用 VPN**

在公网环境下，建议使用 VPN 或 Wireguard 建立安全隧道。

---

## 🧪 测试连接

### **Linux 端测试**

```bash
# 检查端口是否监听
netstat -tuln | grep 555

# 应该看到：
# tcp  0.0.0.0:5555  LISTEN
# tcp  0.0.0.0:5556  LISTEN
```

### **Windows 端测试**

```cmd
# 测试连通性
telnet 192.168.1.10 5555
telnet 192.168.1.10 5556

# 或使用 PowerShell
Test-NetConnection -ComputerName 192.168.1.10 -Port 5555
```

---

## 📝 使用流程

### **开发阶段（Linux）**

1. 启动 CTP Core：`cd ctp_core/build && ./ctp_core`
2. 启动 Qt Manager：`cd qt_manager/build && ./qt_manager`
3. `config.json` 使用 `127.0.0.1`

### **生产阶段（Windows）**

1. Linux 服务器启动 CTP Core
2. Windows 修改 `config.json` 为服务器 IP
3. 运行 Windows 编译的 `qt_manager.exe`

---

## ❓ 常见问题

### **Q: 连接失败，显示 "Connection refused"**

**A:** 检查：
1. Linux 防火墙是否开放端口
2. CTP Core 是否正在运行
3. IP 地址是否正确

### **Q: 能连接但没有数据**

**A:** 检查：
1. CTP Core 是否已登录 CTP 服务器
2. 查看 Core 日志输出

### **Q: Windows 编译失败**

**A:** 确保：
1. Qt 版本 >= 6.2
2. ZeroMQ 和 nlohmann-json 已正确安装
3. CMake 能找到依赖库

---

## 📊 性能说明

- **局域网延迟**：< 5ms
- **广域网延迟**：取决于网络质量
- **带宽占用**：行情推送约 10-50 KB/s

---

## 🎯 下一步

- [ ] 在设置界面添加服务器地址配置
- [ ] 添加连接状态指示器
- [ ] 支持多服务器切换
- [ ] 添加连接重试机制

---

**祝您使用愉快！** 🚀
