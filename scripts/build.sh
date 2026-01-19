#!/bin/bash

# A-Trader 一键编译脚本 (Linux)
# 确保系统已安装: g++, cmake, qt6-base-dev, qt6-declarative-dev, libzmq3-dev, nlohmann-json3-dev

set -e

PROJECT_ROOT=$(pwd)

echo "--- 开始编译 A-Trader ---"

# 1. 编译 ctp_core
echo ">>> 正在编译 ctp_core..."
mkdir -p ctp_core/build
cd ctp_core/build
cmake ..
make -j$(nproc)
cd $PROJECT_ROOT

# 2. 编译 qt_manager
echo ">>> 正在编译 qt_manager..."
mkdir -p qt_manager/build
cd qt_manager/build
cmake ..
make -j$(nproc)
cd $PROJECT_ROOT

echo "--- 编译完成 ---"
echo "ctp_core 可执行文件位于: ctp_core/build/ctp_core"
echo "qt_manager 可执行文件位于: qt_manager/build/qt_manager"
