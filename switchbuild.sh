#!/usr/bin/env bash
# ============================================================
# Nintendo Switch 构建脚本（MSYS2 + macOS + Linux）
#
# 支持：
#   - Windows + MSYS2
#   - macOS
#   - Linux
#
# 输出：
#   build_switch/GBAStation.nro
#
# 使用：
#   ./switchbuild.sh
#
# ============================================================

set -e

# ────────────────────────────────────────────────────────────
# Windows 非 MSYS 环境自动切换到 MSYS2
# ────────────────────────────────────────────────────────────

if [ -z "$MSYSTEM" ] && [ -n "$WINDIR" ]; then

    MSYS2_PATHS=(
        "C:/msys64/msys2.exe"
        "D:/msys64/msys2.exe"
        "E:/msys64/msys2.exe"
        "C:/tools/msys64/msys2.exe"
    )

    for p in "${MSYS2_PATHS[@]}"; do
        WIN_PATH=$(echo "$p" | sed 's#/#\\#g')

        if [ -f "$WIN_PATH" ]; then
            "$WIN_PATH" \
                -defterm \
                -no-start \
                -here \
                -ucrt64 \
                -shell bash \
                -lc "cd \"$(pwd)\" && ./switchbuild.sh"

            exit $?
        fi
    done

    echo "[错误] 未找到 MSYS2"
    exit 1
fi

# ────────────────────────────────────────────────────────────
# 平台检测
# ────────────────────────────────────────────────────────────

OS="$(uname -s)"

case "$OS" in
    Darwin*)
        PLATFORM="mac"
        ;;
    Linux*)
        PLATFORM="linux"
        ;;
    MINGW*|MSYS*)
        PLATFORM="windows"
        ;;
    *)
        PLATFORM="unknown"
        ;;
esac

echo "[平台] ${PLATFORM}"

# ────────────────────────────────────────────────────────────
# devkitPro 环境
# ────────────────────────────────────────────────────────────

export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export DEVKITA64="${DEVKITA64:-${DEVKITPRO}/devkitA64}"

export PATH="${DEVKITPRO}/tools/bin:${PATH}"
export PATH="${DEVKITA64}/bin:${PATH}"

if [ ! -f "${DEVKITPRO}/cmake/Switch.cmake" ]; then

    echo ""
    echo "[错误] 找不到:"
    echo "    ${DEVKITPRO}/cmake/Switch.cmake"
    echo ""
    echo "请确认："
    echo "1. 已安装 devkitPro"
    echo "2. DEVKITPRO 环境变量正确"
    echo ""

    exit 1
fi

# ────────────────────────────────────────────────────────────
# 获取CPU线程数
# ────────────────────────────────────────────────────────────

if command -v nproc >/dev/null 2>&1; then

    CPU_COUNT=$(nproc)

elif [ "$PLATFORM" = "mac" ]; then

    CPU_COUNT=$(sysctl -n hw.logicalcpu)

else

    CPU_COUNT=4

fi

# M1/M2/M3 MacBook Air 限制并行数量
if [ "$PLATFORM" = "mac" ]; then

    if [ "$CPU_COUNT" -ge 8 ]; then
        JOBS=4
    else
        JOBS=$CPU_COUNT
    fi

else

    JOBS=$CPU_COUNT

fi

echo "[线程] ${JOBS}"

# ────────────────────────────────────────────────────────────
# 路径
# ────────────────────────────────────────────────────────────

ROOT_DIR="$(pwd)"
BUILD_DIR="${ROOT_DIR}/build_switch"
THREEDS_STUB_SOURCE="${ROOT_DIR}/../GBAStation_3DS/GBAStation3DSStub.nro"
EXTERNAL_CORE_NROS=(
    "FBNeo.nro"
    "Flycast.nro"
)

mkdir -p "${BUILD_DIR}"

clean_external_core_nro_outputs() {
    for nro in "${EXTERNAL_CORE_NROS[@]}"; do
        rm -f "${BUILD_DIR}/${nro}"
        rm -f "${BUILD_DIR}/GBAStation/core/${nro}"
        rm -f "${BUILD_DIR}/resources/core/${nro}"
    done
}

clean_external_core_nro_outputs

# ────────────────────────────────────────────────────────────
# 临时目录
# ────────────────────────────────────────────────────────────

export TMPDIR="${BUILD_DIR}/tmp"

mkdir -p "${TMPDIR}"

if [ "$PLATFORM" = "windows" ]; then

    if command -v cygpath >/dev/null 2>&1; then

        TMP_WIN=$(cygpath -w "${TMPDIR}")

        export TMP="${TMP_WIN}"
        export TEMP="${TMP_WIN}"

    fi

else

    export TMP="${TMPDIR}"
    export TEMP="${TMPDIR}"

fi

# ────────────────────────────────────────────────────────────
# 开始构建
# ────────────────────────────────────────────────────────────

cd "${BUILD_DIR}"

echo ""
echo "[1/4] CMake配置..."

cmake .. \
    -DPLATFORM_SWITCH=ON \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_DEPENDS_USE_COMPILER=FALSE

echo ""
echo "[2/4] 编译..."

cmake --build . -j "${JOBS}"

echo ""
echo "[3/4] 打包 NRO..."

cmake --build . --target GBAStation.nro
cmake --build . --target GBAStationNDSStub.nro

cd ..

mkdir -p "${BUILD_DIR}/GBAStation/core"
if [ -f "${BUILD_DIR}/GBAStationNDSStub.nro" ]; then
    cp "${BUILD_DIR}/GBAStationNDSStub.nro" "${BUILD_DIR}/GBAStation/core/GBAStationNDSStub.nro"
fi
if [ -f "${THREEDS_STUB_SOURCE}" ]; then
    cp "${THREEDS_STUB_SOURCE}" "${BUILD_DIR}/GBAStation/core/GBAStation3DSStub.nro"
    echo "[3DS] 已复制最新 GBAStation3DSStub.nro"
else
    echo "[警告] 未找到 3DS Stub: ${THREEDS_STUB_SOURCE}"
fi

clean_external_core_nro_outputs
echo "[外置核心] FBNeo/Flycast 不写入仓库和发布包，请放到 SD 卡 /GBAStation/core/"

# ────────────────────────────────────────────────────────────
# 输出大小
# ────────────────────────────────────────────────────────────

echo ""
echo "==================== 编译结果 ===================="

if [ -f "${BUILD_DIR}/GBAStation.nro" ]; then

    if [ "$PLATFORM" = "mac" ]; then
        SIZE=$(stat -f%z "${BUILD_DIR}/GBAStation.nro")
    else
        SIZE=$(stat -c%s "${BUILD_DIR}/GBAStation.nro")
    fi

    SIZE_MB=$(awk "BEGIN {printf \"%.2f\", ${SIZE}/1024/1024}")

    echo "✅ GBAStation.nro : ${SIZE_MB} MB"

else

    echo "❌ GBAStation.nro 不存在"

fi

if [ -f "${BUILD_DIR}/GBAStationNDSStub.nro" ]; then

    if [ "$PLATFORM" = "mac" ]; then
        SIZE=$(stat -f%z "${BUILD_DIR}/GBAStationNDSStub.nro")
    else
        SIZE=$(stat -c%s "${BUILD_DIR}/GBAStationNDSStub.nro")
    fi

    SIZE_MB=$(awk "BEGIN {printf \"%.2f\", ${SIZE}/1024/1024}")

    echo "✅ GBAStationNDSStub.nro : ${SIZE_MB} MB"

else

    echo "❌ GBAStationNDSStub.nro 不存在"

fi

if [ -f "${BUILD_DIR}/GBAStation/core/GBAStation3DSStub.nro" ]; then
    if [ "$PLATFORM" = "mac" ]; then
        SIZE=$(stat -f%z "${BUILD_DIR}/GBAStation/core/GBAStation3DSStub.nro")
    else
        SIZE=$(stat -c%s "${BUILD_DIR}/GBAStation/core/GBAStation3DSStub.nro")
    fi
    SIZE_MB=$(awk "BEGIN {printf \"%.2f\", ${SIZE}/1024/1024}")
    echo "✅ GBAStation/core/GBAStation3DSStub.nro : ${SIZE_MB} MB"
else
    echo "❌ GBAStation/core/GBAStation3DSStub.nro 不存在"
fi

echo "=================================================="

echo ""
echo "[完成]"
echo "${BUILD_DIR}/GBAStation.nro"
echo "${BUILD_DIR}/GBAStationNDSStub.nro"
echo "${BUILD_DIR}/GBAStation/core/GBAStationNDSStub.nro"
echo "${BUILD_DIR}/GBAStation/core/GBAStation3DSStub.nro"
echo "SD:/GBAStation/core/FBNeo.nro"
echo "SD:/GBAStation/core/Flycast.nro"
