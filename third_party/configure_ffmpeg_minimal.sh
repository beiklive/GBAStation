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
# Keep the generated tree private to this CMake build.  Performing both the
# cleanup and copy in Bash avoids MSYS/Windows path conversion differences in
# CMake -E and guarantees no host objects survive the copy.
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
# cp's default no-clobber protection is not useful in this disposable build
# tree when an interrupted prior run left an entry behind.
cp -a -f "$SOURCE_DIR/." "$BUILD_DIR/"
mkdir -p "$BUILD_DIR/.tmp"
cd "$BUILD_DIR"
# The vendored tree may contain archives and object files from a native build.
# This directory is a disposable copy, so remove every compiler byproduct
# before configure: otherwise Make can treat a MinGW object as up to date and
# package it into the Switch archive.
find . -type f \( -name '*.a' -o -name '*.o' -o -name '*.d' \) -delete
# ffbuild also contains source-controlled Makefile helpers such as common.mak.
# Remove only configure output, not that directory as a whole.
rm -f config.h config_components.h config.fate config.mak config.log
rm -f ffbuild/.config ffbuild/config.fate \
      ffbuild/config.log ffbuild/config.mak ffbuild/config.sh
export TMPDIR="$BUILD_DIR/.tmp"
export TMP="$TMPDIR"
export TEMP="$TMPDIR"

OPTIONS=(
    "--cc=$CC" "--ar=$AR" "--ranlib=$RANLIB"
    --disable-asm --enable-small --enable-pic
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

./configure "${OPTIONS[@]}"
make -j1 libavutil/libavutil.a libavcodec/libavcodec.a \
    libavformat/libavformat.a libswscale/libswscale.a
