#!/usr/bin/env bash
# Build FFmpeg in a separate directory with only the MP4 background decoder.
set -euo pipefail

SOURCE_DIR="$1"
BUILD_DIR="$2"
CC="$3"
AR="$4"
RANLIB="$5"
TARGET_KIND="$6"

# The parent process can be launched from cmd.exe with a stripped PATH.
# Bootstrap MSYS's core tools before the first mkdir/cd invocation. FFmpeg
# itself resolves the compiler by its absolute path, so no dirname utility is
# needed before PATH is repaired.
export PATH="/usr/bin:$PATH"
# FFmpeg refuses to configure a native-MSYS binary. CMake invokes bash from
# cmd.exe, therefore establish the same target environment as an MSYS2
# MinGW64 shell explicitly.
if [ "${OS:-}" = "Windows_NT" ]; then
    export MSYSTEM=MINGW64
fi
mkdir -p "$BUILD_DIR/.tmp"
cd "$BUILD_DIR"
export TMPDIR="$BUILD_DIR/.tmp"
export TMP="$TMPDIR"
export TEMP="$TMPDIR"

OPTIONS=(
    "--cc=$CC" "--ar=$AR" "--ranlib=$RANLIB"
    --disable-asm --enable-small
    --disable-programs --disable-doc
    --disable-avdevice --disable-avfilter --disable-swresample
    --disable-everything
    --enable-avcodec --enable-avformat --enable-avutil --enable-swscale
    --enable-demuxer=mov --enable-decoder=h264 --enable-parser=h264
    --enable-protocol=file
    --enable-static --disable-shared --disable-network --disable-pthreads
    --disable-autodetect --disable-iconv --disable-zlib --disable-bzlib --disable-lzma
)

if [ "$TARGET_KIND" = "switch" ]; then
    OPTIONS+=(--arch=aarch64 --target-os=none --enable-cross-compile)
fi

"$SOURCE_DIR/configure" "${OPTIONS[@]}"
make -j1 libavutil/libavutil.a libavcodec/libavcodec.a \
    libavformat/libavformat.a libswscale/libswscale.a
