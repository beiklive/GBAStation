#!/bin/bash
# ============================================================
# macOS 编译脚本
# 编译后产物：
#   build_macos/BKStation                —— 主程序（普通可执行文件）
#   build_macos/BKStation.app            —— macOS Bundle（需 -DBUNDLE_MACOS_APP=ON）
#   build_macos/mgba_libretro.dylib     —— libretro 核心
#
# 依赖：Xcode Command Line Tools / CMake（brew install cmake）
# 使用方式：
#   ./build_macos.sh              # 普通桌面模式
#   ./build_macos.sh --bundle     # 打包为 .app Bundle
# ============================================================
set -e

# 并行编译线程数
JOBS=$(sysctl -n hw.logicalcpu)

# 构建目录
BUILD_DIR="build_macos"

# 解析参数
BUNDLE_OPT="-DBUNDLE_MACOS_APP=OFF"
for arg in "$@"; do
    case $arg in
        --bundle) BUNDLE_OPT="-DBUNDLE_MACOS_APP=ON" ;;
    esac
done

echo "[1/3] 创建构建目录 ${BUILD_DIR} ..."
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "[2/3] 运行 CMake 配置（桌面平台 / Release）..."
cmake .. \
    -DPLATFORM_DESKTOP=ON \
    -DCMAKE_BUILD_TYPE=Release \
    ${BUNDLE_OPT}

echo "[3/3] 开始编译（并行线程：${JOBS}）..."
cmake --build . -j "${JOBS}"

cd ..
echo ""
echo "[完成] 产物目录：${BUILD_DIR}/"
