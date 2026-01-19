---
name: build_and_run
description: 指导如何编译和运行 A-Trader 项目的 CTP Core 和 QT Manager 组件。
---

# 编译和运行指南

本项目分为两个主要部分：
1. **CTP Core**: C++ 后端，负责连接柜台、处理交易和行情。
2. **QT Manager**: C++ Qt/QML 前端，负责界面展示和用户交互。

## 1. 编译脚本

项目提供了便捷的 shell 脚本位于 `scripts/` 目录下。

### 编译 CTP Core
```bash
./scripts/ctp_build.sh
```
该脚本会：
- 清理旧的构建文件（可选）。
- 运行 CMake 配置。
- 编译生成 `ctp_core` 可执行文件。
- 输出位置: `ctp_core/build/ctp_core`

### 编译 QT Manager
```bash
./scripts/qt_build.sh
```
该脚本会：
- 运行 CMake 配置（检测 Qt6 环境）。
- 编译生成 `qt_manager` 可执行文件。
- 输出位置: `qt_manager/build/qt_manager`

### 一键编译所有 (build.sh)
```bash
./scripts/build.sh
```
同时编译上述两个组件。

## 2. 运行程序

建议在两个不同的终端中分别运行后端和前端。

### 运行 CTP Core (后端)
**必须先运行 Core**，因为它充当行情和交易数据的 Publisher，以及指令的 Server。

```bash
cd /home/zd/A-Trader/ctp_core/build
./ctp_core
```
*注意：Core 启动后会连接期货公司前置机，请关注终端输出的 "Connected", "Login Success" 等日志。*

### 运行 QT Manager (前端)
```bash
cd /home/zd/A-Trader/qt_manager/build
./qt_manager
```
*注意：前端启动后会尝试连接 Core 的 ZeroMQ 端口 (默认 5555 和 5556)。*

## 3. 常见问题
- **端口冲突**：确保 5555 (PUB) 和 5556 (REP) 端口未被占用。
- **动态库缺失**：CTP 的 `.so` 文件通常位于 `ctp_core/lib` 或系统路径。如果报错找不到库，请检查 `LD_LIBRARY_PATH`。
