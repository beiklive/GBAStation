#!/bin/bash
# ============================================================
# Nintendo Switch 编译脚本（DevkitPro / libnx）
# 编译后产物：
#   build_switch/GBAStation.nro           —— Switch 可执行文件（NRO 格式）
#
# 前置步骤（Flash 支持）：
#   ./build_flashnx.sh                    —— 编译 Rust staticlib
#
# 依赖：
#   - 已安装 DevkitPro，并设置环境变量 DEVKITPRO
#     官方安装说明：https://devkitpro.org/wiki/Getting_Started
#   - 已通过 dkp-pacman 安装 switch-dev 组：
#     sudo dkp-pacman -S switch-dev
#   - 已安装 Rust nightly + rust-src
#     curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
#     rustup install nightly-x86_64-pc-windows-gnu
#     rustup component add rust-src --toolchain nightly-x86_64-pc-windows-gnu
#   - (Flash 可选) 已运行 ./build_flashnx.sh 预构建 libruffle_switch.a
#
# 使用方式：
#   export DEVKITPRO=/opt/devkitpro   # 若尚未设置
#   ./build_flashnx.sh                # 首次/更新后
#   ./switchbuild.sh
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
cmake --build . --target GBAStation.nro

cd ..
echo ""
echo "[完成] 产物目录：${BUILD_DIR}/"
# ── 新增：显示文件大小（MB） ──────────────────────────────────
echo ""
echo "==================== 编译产物大小 ===================="
# 计算文件大小，单位 MB（保留 2 位小数）
if [ -f "${BUILD_DIR}/GBAStation.nro" ]; then
    NRO_SIZE=$(du -b "${BUILD_DIR}/GBAStation.nro" | awk '{printf "%.2f", $1/1024/1024}')
    echo "✅ GBAStation.nro    : ${NRO_SIZE} MB"
else
    echo "❌ GBAStation.nro    : 文件不存在"
fi
echo "======================================================"