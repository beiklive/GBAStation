#!/bin/bash
# ============================================================
# FlashNX Rust staticlib 构建脚本 (devkitPro / devkitA64)
# 产物: third_party/flashnx/target/aarch64-nintendo-switch-freestanding/release/libruffle_switch.a
# ============================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FLASHNX_DIR="${SCRIPT_DIR}/third_party/flashnx"

echo "=== 构建 FlashNX Ruffle 静态库 ==="
echo "目录: ${FLASHNX_DIR}"

if ! command -v rustup &>/dev/null; then
    echo ""
    echo "[错误] 未安装 Rust"
    echo "  安装方法: curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh"
    echo "  然后重启终端"
    exit 1
fi

if ! command -v aarch64-none-elf-gcc &>/dev/null; then
    echo ""
    echo "[错误] 未找到 aarch64-none-elf-gcc (devkitA64 链接器)"
    echo "  请确保 DEVKITPRO 已设置: export DEVKITPRO=/opt/devkitpro"
    echo "  并安装了 switch-dev: sudo dkp-pacman -S switch-dev"
    exit 1
fi

echo ""
echo "[1/3] 安装 nightly 工具链 + rust-src ..."
cd "${FLASHNX_DIR}"
rustup install nightly 2>/dev/null || true
rustup component add rust-src --toolchain nightly 2>/dev/null || true

echo "[2/3] 清除旧构建产物..."
cargo +nightly clean 2>/dev/null || true

echo "[3/3] 构建 libruffle_switch.a (release, ~3-5分钟)..."
cargo +nightly build --release

echo ""
OUT="${FLASHNX_DIR}/target/aarch64-nintendo-switch-freestanding/release/libruffle_switch.a"
if [ -f "$OUT" ]; then
    SIZE=$(ls -lh "$OUT" | awk '{print $5}')
    echo "[完成] ${OUT} (${SIZE})"
else
    echo "[错误] 产物未生成，请检查上方 cargo 输出"
    exit 1
fi
