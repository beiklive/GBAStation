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
#   ./switchbuild.sh [-j JOBS] [--build-dir DIR] [--core-dir DIR]
#
# ============================================================

set -euo pipefail

# ────────────────────────────────────────────────────────────
# Windows 非 MSYS 环境自动切换到 MSYS2
# ────────────────────────────────────────────────────────────

if [ -z "${MSYSTEM:-}" ] && [ -n "${WINDIR:-}" ]; then

    FORWARDED_ARGS=""
    for arg in "$@"; do
        printf -v escaped_arg ' %q' "$arg"
        FORWARDED_ARGS+="$escaped_arg"
    done

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
                -lc "cd \"$(pwd)\" && ./switchbuild.sh${FORWARDED_ARGS}"

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

usage() {
    cat <<'EOF'
Usage: ./switchbuild.sh [-j JOBS] [--build-dir DIR] [--core-dir DIR]

  --core-dir DIR  Directory containing the four downloaded core Release NROs:
                  GBAStation3DSStub.nro, GBAStationFBNeoStub.nro,
                  GBAStationFlycastStub.nro and GBAStationPPSSPPStub.nro.
  --build-dir DIR CMake build directory (default: build_switch).
  -j, --jobs N   Parallel build jobs.
EOF
}

BUILD_DIR_OVERRIDE=""
CORE_DIR="${CORE_DIR:-}"
JOBS_OVERRIDE=""
while [ "$#" -gt 0 ]; do
    case "$1" in
        -j|--jobs)
            [ "$#" -ge 2 ] || { echo "[错误] $1 需要参数"; exit 2; }
            JOBS_OVERRIDE="$2"; shift 2 ;;
        --build-dir)
            [ "$#" -ge 2 ] || { echo "[错误] --build-dir 需要参数"; exit 2; }
            BUILD_DIR_OVERRIDE="$2"; shift 2 ;;
        --core-dir)
            [ "$#" -ge 2 ] || { echo "[错误] --core-dir 需要参数"; exit 2; }
            CORE_DIR="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "[错误] 未知参数: $1"; usage; exit 2 ;;
    esac
done

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

if [ -n "${JOBS_OVERRIDE}" ]; then
    JOBS="${JOBS_OVERRIDE}"
fi

echo "[线程] ${JOBS}"

# ────────────────────────────────────────────────────────────
# 路径
# ────────────────────────────────────────────────────────────

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR_OVERRIDE:-${BUILD_DIR:-${ROOT_DIR}/build_switch}}"

if [ -n "${CORE_DIR}" ]; then
    CORE_DIR="$(cd "${CORE_DIR}" && pwd)"
    THREEDS_STUB_SOURCE="${CORE_DIR}/GBAStation3DSStub.nro"
    FBNEO_STUB_SOURCE="${CORE_DIR}/GBAStationFBNeoStub.nro"
    FLYCAST_STUB_SOURCE="${CORE_DIR}/GBAStationFlycastStub.nro"
    PPSSPP_STUB_SOURCE="${CORE_DIR}/GBAStationPPSSPPStub.nro"
    echo "[核心] 使用预置 Release 目录: ${CORE_DIR}"
else
    THREEDS_STUB_SOURCE="${ROOT_DIR}/../.example/dekopon/build/switch-codex/src/citra_switch/dekopon.nro"
    FBNEO_STUB_SOURCE="${ROOT_DIR}/../GBAStation_fbneo/GBAStationFBNeoStub.nro"
    FLYCAST_STUB_SOURCE="${ROOT_DIR}/../GBAStation_flycast/GBAStationFlycastStub.nro"
    PPSSPP_STUB_SOURCE="${ROOT_DIR}/../GBAStation_ppsspp/GBAStationPPSSPPStub.nro"
fi

if [ -n "${CORE_DIR}" ]; then
    for core in GBAStation3DSStub.nro GBAStationFBNeoStub.nro GBAStationFlycastStub.nro GBAStationPPSSPPStub.nro; do
        if [ ! -s "${CORE_DIR}/${core}" ]; then
            echo "[错误] --core-dir 缺少 Release 核心: ${CORE_DIR}/${core}"
            exit 1
        fi
    done
fi
EXTERNAL_CORE_NROS=(
    "FBNeo.nro"
    "Flycast.nro"
    "GBAStation3DSStub.nro"
    "GBAStationFBNeoStub.nro"
    "GBAStationFlycastStub.nro"
    "GBAStationPPSSPPStub.nro"
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

# Compiler subprocesses run from nested CMake directories.  Keep the temp
# directory absolute so make never interprets it relative to a subproject.
export TMPDIR="$(cd "${BUILD_DIR}" && pwd)/tmp"

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
# 外置核心复制
# ────────────────────────────────────────────────────────────

copy_external_core_stub() {
    local label="$1"
    local source="$2"
    local output_name="$3"

    echo ""
    echo "[4/4] 复制 ${label} 外置核心..."

    if [ ! -f "${source}" ]; then
        echo "[错误] 找不到 ${label} 外置核心: ${source}"
        echo "请先在对应独立仓库构建 ${output_name}"
        exit 1
    fi

    mkdir -p "${BUILD_DIR}/GBAStation/core"
    cp "${source}" "${BUILD_DIR}/GBAStation/core/${output_name}"
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
    echo "[错误] 未找到 3DS Stub: ${THREEDS_STUB_SOURCE}"
    exit 1
fi

copy_external_core_stub "FBNeo" "${FBNEO_STUB_SOURCE}" "GBAStationFBNeoStub.nro"
copy_external_core_stub "Flycast" "${FLYCAST_STUB_SOURCE}" "GBAStationFlycastStub.nro"
copy_external_core_stub "PPSSPP" "${PPSSPP_STUB_SOURCE}" "GBAStationPPSSPPStub.nro"

# ────────────────────────────────────────────────────────────
# 输出大小
# ────────────────────────────────────────────────────────────

echo ""
echo "==================== 编译结果 ===================="

print_nro_size "GBAStation.nro" "${BUILD_DIR}/GBAStation.nro"
print_nro_size "GBAStationNDSStub.nro" "${BUILD_DIR}/GBAStationNDSStub.nro"
print_nro_size "GBAStation/core/GBAStationNDSStub.nro" "${BUILD_DIR}/GBAStation/core/GBAStationNDSStub.nro"
print_nro_size "GBAStation/core/GBAStation3DSStub.nro" "${BUILD_DIR}/GBAStation/core/GBAStation3DSStub.nro"
print_nro_size "GBAStation/core/GBAStationFBNeoStub.nro" "${BUILD_DIR}/GBAStation/core/GBAStationFBNeoStub.nro"
print_nro_size "GBAStation/core/GBAStationFlycastStub.nro" "${BUILD_DIR}/GBAStation/core/GBAStationFlycastStub.nro"
print_nro_size "GBAStation/core/GBAStationPPSSPPStub.nro" "${BUILD_DIR}/GBAStation/core/GBAStationPPSSPPStub.nro"

echo ""
echo "==================== SHA-256 ===================="
for nro in "${BUILD_DIR}/GBAStation.nro" "${BUILD_DIR}/GBAStationNDSStub.nro" "${BUILD_DIR}/GBAStation/core/"*.nro; do
    [ -f "${nro}" ] && sha256sum "${nro}"
done

echo "=================================================="

echo ""
echo "[完成]"
echo "${BUILD_DIR}/GBAStation.nro"
echo "${BUILD_DIR}/GBAStationNDSStub.nro"
echo "${BUILD_DIR}/GBAStation/core/GBAStationNDSStub.nro"
echo "${BUILD_DIR}/GBAStation/core/GBAStation3DSStub.nro"
echo "${BUILD_DIR}/GBAStation/core/GBAStationFBNeoStub.nro"
echo "${BUILD_DIR}/GBAStation/core/GBAStationFlycastStub.nro"
echo "${BUILD_DIR}/GBAStation/core/GBAStationPPSSPPStub.nro"
