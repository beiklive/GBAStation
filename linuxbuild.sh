#!/bin/bash
# ============================================================
# Linux 编译脚本
# 编译后产物：
#   build_linux/New_mGBA                —— 主程序
#   build_linux/mgba_libretro.so        —— libretro 核心
#
# 依赖（Ubuntu / Debian 示例）：
#   sudo apt install cmake build-essential \
#       libgl1-mesa-dev libglu1-mesa-dev \
#       libx11-dev libxrandr-dev libxinerama-dev \
#       libxcursor-dev libxi-dev libdbus-1-dev
# ============================================================
set -e

# 并行编译线程数
JOBS=$(nproc)

# 构建目录
BUILD_DIR="build_linux"

echo "[1/3] 创建构建目录 ${BUILD_DIR} ..."
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "[2/3] 运行 CMake 配置（桌面平台 / Release）..."
cmake .. \
    -DPLATFORM_DESKTOP=ON \
    -DCMAKE_BUILD_TYPE=Release

echo "[3/3] 开始编译（并行线程：${JOBS}）..."
cmake --build . -j "${JOBS}"

cd ..
echo ""
echo "[完成] 产物目录：${BUILD_DIR}/"
