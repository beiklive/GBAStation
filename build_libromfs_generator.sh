#!/bin/bash
# ============================================================
# Build libromfs-generator for the host machine.
# This tool converts resource files into C++ source code
# that gets embedded into the native library.
# Required for cross-compilation (Android, iOS, Switch, etc.)
# ============================================================
set -e

PROJECT_PATH=$(dirname "$0")
LIBROMFS_PATH="${PROJECT_PATH}/third_party/borealis/library/lib/extern/libromfs/generator"
BUILD_DIR="${PROJECT_PATH}/build_libromfs_generator"

echo "Build libromfs-generator"

cmake -B "${BUILD_DIR}" "${LIBROMFS_PATH}" -G Ninja
cmake --build "${BUILD_DIR}"

cp "${BUILD_DIR}/libromfs-generator" "${PROJECT_PATH}/libromfs-generator"
echo "libromfs-generator → ${PROJECT_PATH}/libromfs-generator"

rm -rf "${BUILD_DIR}"
echo "Done."
