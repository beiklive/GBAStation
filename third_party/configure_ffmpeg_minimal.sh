#!/usr/bin/env bash
# Build FFmpeg in a separate directory with common MP4 video decoders.
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
# An MSYS child can still have this directory as its current working
# directory briefly after a cancelled build.  Windows then refuses to remove
# the directory itself (EBUSY), even though its contents are disposable.
# Keep the root and clear its children so a retry never depends on that race.
mkdir -p "$BUILD_DIR"
find "$BUILD_DIR" -mindepth 1 -maxdepth 1 -exec rm -rf {} +
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

# The source tree can originate from a Windows archive/worktree where Git's
# executable bit is unavailable. FFmpeg invokes version.sh while generating
# its version headers, so both scripts must be executable inside the Linux CI
# container regardless of their source-file mode.
chmod +x "$BUILD_DIR/configure" "$BUILD_DIR/ffbuild/version.sh"

OPTIONS=(
    "--cc=$CC" "--ar=$AR" "--ranlib=$RANLIB"
    --enable-small --enable-pic
    --disable-programs --disable-doc
    --disable-avdevice --disable-avfilter
    --disable-everything
    --enable-avcodec --enable-avformat --enable-avutil --enable-swscale --enable-swresample
    # The player has one RGBA output path and supports the common codecs that
    # are routinely stored in an MP4 container. Keep common MP4 audio
    # decoders too; libswresample converts them to 48 kHz stereo PCM. Filters,
    # encoders and network/file protocols remain excluded: custom memory AVIO
    # is used.
    --enable-demuxer=mov
    --enable-decoder=h264,hevc,mpeg4,mpeg2video,mjpeg,vp8,vp9,av1,aac,alac,ac3,eac3,flac,mp3,opus,vorbis
    --enable-parser=h264,hevc,mpeg4video,mpegvideo,mjpeg,vc1,aac,ac3,mpegaudio,opus,vorbis
    --enable-static --disable-shared --disable-network --disable-pthreads
    --disable-autodetect --disable-iconv --disable-zlib --disable-bzlib --disable-lzma
)

if [ "$TARGET_KIND" = "switch" ]; then
    OPTIONS+=(--arch=aarch64 --cpu=cortex-a57 --target-os=none --enable-cross-compile)
else
    OPTIONS+=(--disable-asm)
fi

./configure "${OPTIONS[@]}"
make -j1 libavutil/libavutil.a libavcodec/libavcodec.a \
    libavformat/libavformat.a libswscale/libswscale.a libswresample/libswresample.a
