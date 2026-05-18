#!/bin/bash
# ============================================================
# Android 编译脚本（Gradle + CMake）
# 编译后产物：
#   android-project/app/build/outputs/apk/debug/app-debug.apk
#
# 依赖：
#   - Android SDK（环境变量 ANDROID_SDK_ROOT 或 ANDROID_HOME）
#   - Android NDK r22+（由 Gradle 自动下载或手动安装到 SDK 中）
#   - JDK 17+
#   - ninja-build, pkg-config
#
# 使用方式：
#   ./androidbuild.sh
# ============================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ANDROID_PROJECT="${SCRIPT_DIR}/android-project"
JNI_DIR="${ANDROID_PROJECT}/app/jni"

# ── 1. 构建 libromfs-generator（宿主机器工具） ─────────────────
echo "[1/4] Building libromfs-generator..."
LIBROMFS_PATH="${SCRIPT_DIR}/third_party/borealis/library/lib/extern/libromfs/generator"
BUILD_DIR="${SCRIPT_DIR}/build_libromfs_generator"

cmake -B "${BUILD_DIR}" "${LIBROMFS_PATH}" -G Ninja
cmake --build "${BUILD_DIR}"

cp "${BUILD_DIR}/libromfs-generator" "${SCRIPT_DIR}/libromfs-generator"
rm -rf "${BUILD_DIR}"
echo "  libromfs-generator ready."

# ── 2. 创建原生代码符号链接 ──────────────────────────────────
echo "[2/4] Creating JNI symlinks..."
mkdir -p "${JNI_DIR}"

cd "${JNI_DIR}"
if [ ! -L "SDL" ]; then
    ln -sf ../../../third_party/borealis/library/lib/extern/SDL SDL
fi
if [ ! -L "borealis" ]; then
    ln -sf ../../.. borealis
fi
cd "${SCRIPT_DIR}"

# ── 3. 复制 libromfs-generator ──────────────────────────────
cp "${SCRIPT_DIR}/libromfs-generator" "${JNI_DIR}/borealis/libromfs-generator"

# ── 4. 编译 APK ─────────────────────────────────────────────
echo "[3/4] Building APK (assembleDebug)..."
cd "${ANDROID_PROJECT}"
chmod +x gradlew
./gradlew assembleDebug

cd "${SCRIPT_DIR}"

echo ""
echo "[完成] 产物目录：${ANDROID_PROJECT}/app/build/outputs/apk/debug/"
ls -la "${ANDROID_PROJECT}/app/build/outputs/apk/debug/"*.apk 2>/dev/null || echo "  (no APK found)"
