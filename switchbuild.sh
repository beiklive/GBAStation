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

export PATH="/usr/bin:/mingw64/bin:/ucrt64/bin:${PATH}"
export PATH="${DEVKITPRO}/tools/bin:${PATH}"
export PATH="${DEVKITA64}/bin:${PATH}"
export PKG_CONFIG_PATH="${DEVKITPRO}/portlibs/switch/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

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
PFBN_BUILD_DIR="${ROOT_DIR}/build_switch_pfbneo"
FLYCAST_BUILD_DIR="${ROOT_DIR}/build_switch_flycast"
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
# 外置核心构建
# ────────────────────────────────────────────────────────────

require_external_core_tools() {
    if ! command -v ninja >/dev/null 2>&1; then
        echo "[错误] 构建外置核心需要 Ninja，请先安装 ninja"
        exit 1
    fi
}

build_pfbneo_external_core() {
    echo ""
    echo "[4/6] 构建 FBNeo 外置核心..."

    if [ ! -d "${ROOT_DIR}/third_party/pemu" ]; then
        echo "[错误] 找不到 third_party/pemu，无法构建 FBNeo 外置核心"
        exit 1
    fi

    mkdir -p "${PFBN_BUILD_DIR}/tmp"

    TMPDIR="${PFBN_BUILD_DIR}/tmp" \
    TMP="${PFBN_BUILD_DIR}/tmp" \
    TEMP="${PFBN_BUILD_DIR}/tmp" \
    cmake \
        -S "${ROOT_DIR}/third_party/pemu" \
        -B "${PFBN_BUILD_DIR}" \
        -DPLATFORM_SWITCH=ON \
        -DOPTION_EMU=pfbneo \
        -DOPTION_MPV_PLAYER=OFF \
        -DOPTION_BUILTIN_LIBCONFIG=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="${DEVKITPRO}/cmake/Switch.cmake" \
        -DCMAKE_PREFIX_PATH="${DEVKITPRO}/portlibs/switch" \
        -G Ninja

    TMPDIR="${PFBN_BUILD_DIR}/tmp" \
    TMP="${PFBN_BUILD_DIR}/tmp" \
    TEMP="${PFBN_BUILD_DIR}/tmp" \
    cmake --build "${PFBN_BUILD_DIR}" --target pfbneo.nro -j "${JOBS}"

    local output="${PFBN_BUILD_DIR}/src/cores/pfbneo/pfbneo.nro"
    if [ ! -f "${output}" ]; then
        echo "[错误] FBNeo 外置核心构建完成但未找到输出: ${output}"
        exit 1
    fi

    mkdir -p "${BUILD_DIR}/GBAStation/core"
    cp "${output}" "${BUILD_DIR}/GBAStation/core/FBNeo.nro"
}

build_flycast_external_core() {
    echo ""
    echo "[5/6] 构建 Flycast 外置核心..."

    if [ ! -d "${ROOT_DIR}/third_party/flycast" ]; then
        echo "[错误] 找不到 third_party/flycast，无法构建 Flycast 外置核心"
        exit 1
    fi

    mkdir -p "${FLYCAST_BUILD_DIR}/tmp"
    rm -f "${FLYCAST_BUILD_DIR}/CMakeCache.txt"
    rm -rf "${FLYCAST_BUILD_DIR}/CMakeFiles"

    TMPDIR="${FLYCAST_BUILD_DIR}/tmp" \
    TMP="${FLYCAST_BUILD_DIR}/tmp" \
    TEMP="${FLYCAST_BUILD_DIR}/tmp" \
    cmake \
        -S "${ROOT_DIR}/third_party/flycast" \
        -B "${FLYCAST_BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DNINTENDO_SWITCH=ON \
        -DUSE_VULKAN=OFF \
        -DUSE_HOST_SDL=ON \
        -DCMAKE_TOOLCHAIN_FILE="${DEVKITPRO}/cmake/Switch.cmake" \
        -DCMAKE_PREFIX_PATH="${DEVKITPRO}/portlibs/switch" \
        -G Ninja

    TMPDIR="${FLYCAST_BUILD_DIR}/tmp" \
    TMP="${FLYCAST_BUILD_DIR}/tmp" \
    TEMP="${FLYCAST_BUILD_DIR}/tmp" \
    cmake --build "${FLYCAST_BUILD_DIR}" --target flycast.nro -j "${JOBS}"

    local output="${FLYCAST_BUILD_DIR}/flycast.nro"
    if [ ! -f "${output}" ]; then
        echo "[错误] Flycast 外置核心构建完成但未找到输出: ${output}"
        exit 1
    fi

    mkdir -p "${BUILD_DIR}/GBAStation/core"
    cp "${output}" "${BUILD_DIR}/GBAStation/core/Flycast.nro"
}

print_nro_size() {
    local label="$1"
    local file="$2"

    if [ -f "${file}" ]; then
        local size
        if [ "$PLATFORM" = "mac" ]; then
            size=$(stat -f%z "${file}")
        else
            size=$(stat -c%s "${file}")
        fi
        local size_mb
        size_mb=$(awk "BEGIN {printf \"%.2f\", ${size}/1024/1024}")
        echo "✅ ${label} : ${size_mb} MB"
    else
        echo "❌ ${label} 不存在"
    fi
}

# ────────────────────────────────────────────────────────────
# 开始构建
# ────────────────────────────────────────────────────────────

cd "${BUILD_DIR}"

echo ""
echo "[1/6] CMake配置..."

cmake .. \
    -DPLATFORM_SWITCH=ON \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_DEPENDS_USE_COMPILER=FALSE

echo ""
echo "[2/6] 编译..."

cmake --build . -j "${JOBS}"

echo ""
echo "[3/6] 打包 NRO..."

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

require_external_core_tools
build_pfbneo_external_core
build_flycast_external_core

# ────────────────────────────────────────────────────────────
# 输出大小
# ────────────────────────────────────────────────────────────

echo ""
echo "==================== 编译结果 ===================="

print_nro_size "GBAStation.nro" "${BUILD_DIR}/GBAStation.nro"
print_nro_size "GBAStationNDSStub.nro" "${BUILD_DIR}/GBAStationNDSStub.nro"
print_nro_size "GBAStation/core/GBAStationNDSStub.nro" "${BUILD_DIR}/GBAStation/core/GBAStationNDSStub.nro"
print_nro_size "GBAStation/core/GBAStation3DSStub.nro" "${BUILD_DIR}/GBAStation/core/GBAStation3DSStub.nro"
print_nro_size "GBAStation/core/FBNeo.nro" "${BUILD_DIR}/GBAStation/core/FBNeo.nro"
print_nro_size "GBAStation/core/Flycast.nro" "${BUILD_DIR}/GBAStation/core/Flycast.nro"

echo "=================================================="

echo ""
echo "[完成]"
echo "${BUILD_DIR}/GBAStation.nro"
echo "${BUILD_DIR}/GBAStationNDSStub.nro"
echo "${BUILD_DIR}/GBAStation/core/GBAStationNDSStub.nro"
echo "${BUILD_DIR}/GBAStation/core/GBAStation3DSStub.nro"
echo "${BUILD_DIR}/GBAStation/core/FBNeo.nro"
echo "${BUILD_DIR}/GBAStation/core/Flycast.nro"
