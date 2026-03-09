#!/bin/bash
# ============================================================
# Nintendo Switch 编译脚本（DevkitPro / libnx）
# 编译后产物：
#   build_switch/BKStation.nro           —— Switch 可执行文件（NRO 格式）
#   build_switch/mgba_libretro.so       —— libretro 核心
#
# 依赖：
#   - 已安装 DevkitPro，并设置环境变量 DEVKITPRO
#     官方安装说明：https://devkitpro.org/wiki/Getting_Started
#   - 已通过 dkp-pacman 安装 switch-dev 组：
#     sudo dkp-pacman -S switch-dev
#
# 使用方式：
#   export DEVKITPRO=/opt/devkitpro   # 若尚未设置
#   ./build_switch.sh
# ============================================================
set -e

# ── 环境检查 ──────────────────────────────────────────────
if [ -z "${DEVKITPRO}" ]; then
    echo "[错误] 未设置环境变量 DEVKITPRO。"
    echo "       请先执行：export DEVKITPRO=/opt/devkitpro"
    exit 1
fi

# 并行编译线程数
JOBS=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)

# 构建目录
BUILD_DIR="build_switch"

echo "[1/4] 创建构建目录 ${BUILD_DIR} ..."
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "[2/4] 运行 CMake 配置（Switch 平台 / Release）..."
cmake .. \
    -DPLATFORM_SWITCH=ON \
    -DCMAKE_BUILD_TYPE=Release

echo "[3/4] 编译主程序 ELF（并行线程：${JOBS}）..."
cmake --build . -j "${JOBS}"

echo "[4/4] 打包为 NRO 文件..."
cmake --build . --target BKStation.nro

cd ..
echo ""
echo "[完成] 产物目录：${BUILD_DIR}/"
echo "  主程序：${BUILD_DIR}/BKStation.nro"
echo "  核心库：${BUILD_DIR}/mgba_libretro.so"
