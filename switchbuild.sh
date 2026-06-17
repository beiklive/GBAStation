#!/bin/bash
# ============================================================
# Nintendo Switch 编译脚本（DevkitPro / libnx）
# 使用 CMAKE_DEPENDS_USE_COMPILER=FALSE 避免编译器输出的
# Windows 路径（含冒号 E:）导致 GNU Make 依赖解析失败。
# 编译后产物：
#   build_switch/GBAStation.nro           —— Switch 可执行文件（NRO 格式）
#
# 依赖：
#   - 已安装 DevkitPro，并设置环境变量 DEVKITPRO
#   - 已通过 dkp-pacman 安装 switch-dev 组：
#     sudo dkp-pacman -S switch-dev
#
# 使用方式：
#   export DEVKITPRO=/opt/devkitpro   # 若尚未设置
#   ./switchbuild.sh
# ============================================================
set -e

# ── 环境检查 ──────────────────────────────────────────────
if [ -z "${DEVKITPRO}" ]; then
    echo "[错误] 未设置环境变量 DEVKITPRO。"
    echo "       请先执行：export DEVKITPRO=/opt/devkitpro"
    exit 1
fi
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=/opt/devkitpro/devkitA64

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
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_DEPENDS_USE_COMPILER=FALSE

echo "[3/4] 编译主程序 ELF（并行线程：${JOBS}）..."
cmake --build . -j "${JOBS}"

echo "[4/4] 打包为 NRO 文件..."
cmake --build . --target GBAStation.nro

cd ..
echo ""
echo "[完成] 产物目录：${BUILD_DIR}/"
# ── 显示文件大小（MB） ──────────────────────────────────────
echo ""
echo "==================== 编译产物大小 ===================="
if [ -f "${BUILD_DIR}/GBAStation.nro" ]; then
    NRO_SIZE=$(du -b "${BUILD_DIR}/GBAStation.nro" | awk '{printf "%.2f", $1/1024/1024}')
    echo "✅ GBAStation.nro    : ${NRO_SIZE} MB"
else
    echo "❌ GBAStation.nro    : 文件不存在"
fi
echo "======================================================"
