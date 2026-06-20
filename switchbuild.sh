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
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export DEVKITA64="${DEVKITA64:-${DEVKITPRO}/devkitA64}"
if [ ! -f "${DEVKITPRO}/cmake/Switch.cmake" ]; then
    echo "[错误] 未找到 ${DEVKITPRO}/cmake/Switch.cmake。"
    echo "       请在 MSYS2/devkitPro 环境中运行，或设置 DEVKITPRO。"
    exit 1
fi

# 并行编译线程数
JOBS=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)

# 构建目录
BUILD_DIR="build_switch"
ROOT_DIR="$(pwd)"

# 部分 Windows/MSYS2 环境下 /tmp 会映射到 MSYS 安装目录，devkitA64 编译器
# 创建临时文件时可能因权限失败。固定到项目构建目录内，保证脚本可重复运行。
export TMPDIR="${ROOT_DIR}/${BUILD_DIR}/tmp"
TMPDIR_WIN="$(cygpath -w "${TMPDIR}" 2>/dev/null || echo "${TMPDIR}")"
export TMP="${TMPDIR_WIN}"
export TEMP="${TMPDIR_WIN}"

echo "[1/4] 创建构建目录 ${BUILD_DIR} ..."
mkdir -p "${BUILD_DIR}"
mkdir -p "${TMPDIR}"
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
