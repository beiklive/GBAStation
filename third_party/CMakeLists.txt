cmake_minimum_required(VERSION 3.10)

# =========================================================
# 路径变量（与根目录 CMakeLists.txt 保持一致）
# =========================================================
set(BOREALIS_LIBRARY   ${CMAKE_CURRENT_SOURCE_DIR}/borealis/library)
set(MGBA_PATH          ${CMAKE_CURRENT_SOURCE_DIR}/mgba)
set(FCEUMM_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/libretro-fceumm)
set(SNES9X_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/snes9x)
set(GENESIS_PATH       ${CMAKE_CURRENT_SOURCE_DIR}/Genesis-Plus-GX)

# =========================================================
# borealis UI 框架
# =========================================================
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/borealis/library)

# =========================================================
# mGBA 模拟器  –  libretro 核心构建
# =========================================================
set(BUILD_LIBRETRO ON  CACHE BOOL "" FORCE)
set(BUILD_SDL      OFF CACHE BOOL "" FORCE)
set(BUILD_QT       OFF CACHE BOOL "" FORCE)

if (PLATFORM_SWITCH)
    set(CMAKE_TOOLCHAIN_FILE   "mgba/src/platform/switch/CMakeToolchain.txt")
    set(USE_SQLITE3     OFF CACHE BOOL "" FORCE)
endif ()

set(BUILD_PERF     OFF CACHE BOOL "" FORCE)
set(BUILD_TEST     OFF CACHE BOOL "" FORCE)
set(BUILD_SUITE    OFF CACHE BOOL "" FORCE)
set(BUILD_CINEMA   OFF CACHE BOOL "" FORCE)
set(BUILD_DOCGEN   OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mgba)

if (PLATFORM_SWITCH AND TARGET mgba_libretro)
    target_sources(mgba_libretro PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/util/vfs/vfs-fd.c
        ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/util/vfs/vfs-dirent.c
    )
endif ()

# =========================================================
# 通用：libretro 符号重命名宏
#
# 将 retro_* API 函数名映射为 {prefix}_retro_*，
# 避免多个核心静态链接时发生符号冲突。
# =========================================================
function(apply_retro_symbol_rename TARGET PREFIX)
    target_compile_definitions(${TARGET} PRIVATE
        "retro_init=${PREFIX}_retro_init"
        "retro_deinit=${PREFIX}_retro_deinit"
        "retro_api_version=${PREFIX}_retro_api_version"
        "retro_get_system_info=${PREFIX}_retro_get_system_info"
        "retro_get_system_av_info=${PREFIX}_retro_get_system_av_info"
        "retro_set_environment=${PREFIX}_retro_set_environment"
        "retro_set_video_refresh=${PREFIX}_retro_set_video_refresh"
        "retro_set_audio_sample=${PREFIX}_retro_set_audio_sample"
        "retro_set_audio_sample_batch=${PREFIX}_retro_set_audio_sample_batch"
        "retro_set_input_poll=${PREFIX}_retro_set_input_poll"
        "retro_set_input_state=${PREFIX}_retro_set_input_state"
        "retro_set_controller_port_device=${PREFIX}_retro_set_controller_port_device"
        "retro_reset=${PREFIX}_retro_reset"
        "retro_run=${PREFIX}_retro_run"
        "retro_serialize_size=${PREFIX}_retro_serialize_size"
        "retro_serialize=${PREFIX}_retro_serialize"
        "retro_unserialize=${PREFIX}_retro_unserialize"
        "retro_load_game=${PREFIX}_retro_load_game"
        "retro_unload_game=${PREFIX}_retro_unload_game"
        "retro_get_memory_data=${PREFIX}_retro_get_memory_data"
        "retro_get_memory_size=${PREFIX}_retro_get_memory_size"
        "retro_cheat_reset=${PREFIX}_retro_cheat_reset"
        "retro_cheat_set=${PREFIX}_retro_cheat_set"
        "retro_get_region=${PREFIX}_retro_get_region"
    )
endfunction()

# =========================================================
# FCEUmm (NES) 核心 – 按 Makefile.libretro + Makefile.common
# 提取源文件，仅编译 libretro 版本使用的代码。
# =========================================================
set(FCEUMM_CORE_DIR ${FCEUMM_PATH}/src)

# 板级 MAP 实现（与 Makefile.common:25-26 一致）
file(GLOB FCEUMM_BOARDS_SRC   ${FCEUMM_CORE_DIR}/boards/*.c)
file(GLOB FCEUMM_INPUT_SRC    ${FCEUMM_CORE_DIR}/input/*.c)
file(GLOB FCEUMM_NTSC_SRC     ${FCEUMM_CORE_DIR}/ntsc/nes_ntsc.c)

# 主源文件（与 Makefile.common:38-63 一致）
set(FCEUMM_MAIN_SRC
    ${FCEUMM_CORE_DIR}/drivers/libretro/libretro.c
    ${FCEUMM_CORE_DIR}/drivers/libretro/libretro_dipswitch.c
    ${FCEUMM_CORE_DIR}/cart.c
    ${FCEUMM_CORE_DIR}/cheat.c
    ${FCEUMM_CORE_DIR}/crc32.c
    ${FCEUMM_CORE_DIR}/fceu-endian.c
    ${FCEUMM_CORE_DIR}/fceu-memory.c
    ${FCEUMM_CORE_DIR}/fceu.c
    ${FCEUMM_CORE_DIR}/fds.c
    ${FCEUMM_CORE_DIR}/fds_apu.c
    ${FCEUMM_CORE_DIR}/file.c
    ${FCEUMM_CORE_DIR}/filter.c
    ${FCEUMM_CORE_DIR}/general.c
    ${FCEUMM_CORE_DIR}/input.c
    ${FCEUMM_CORE_DIR}/md5.c
    ${FCEUMM_CORE_DIR}/nsf.c
    ${FCEUMM_CORE_DIR}/palette.c
    ${FCEUMM_CORE_DIR}/ppu.c
    ${FCEUMM_CORE_DIR}/sound.c
    ${FCEUMM_CORE_DIR}/state.c
    ${FCEUMM_CORE_DIR}/video.c
    ${FCEUMM_CORE_DIR}/vsuni.c
    ${FCEUMM_CORE_DIR}/ines.c
    ${FCEUMM_CORE_DIR}/unif.c
    ${FCEUMM_CORE_DIR}/x6502.c
)

# libretro-common 兼容层（Makefile.common:65-80; STATIC_LINKING!=1 时追加）
set(FCEUMM_LRC_DIR ${FCEUMM_CORE_DIR}/drivers/libretro/libretro-common)
set(FCEUMM_COMPAT_SRC
    ${FCEUMM_LRC_DIR}/compat/compat_posix_string.c
    ${FCEUMM_LRC_DIR}/compat/compat_snprintf.c
    ${FCEUMM_LRC_DIR}/compat/compat_strcasestr.c
    ${FCEUMM_LRC_DIR}/compat/compat_strl.c
    ${FCEUMM_LRC_DIR}/compat/fopen_utf8.c
    ${FCEUMM_LRC_DIR}/encodings/encoding_utf.c
    ${FCEUMM_LRC_DIR}/file/file_path.c
    ${FCEUMM_LRC_DIR}/file/file_path_io.c
    ${FCEUMM_LRC_DIR}/streams/file_stream.c
    ${FCEUMM_LRC_DIR}/streams/file_stream_transforms.c
    ${FCEUMM_LRC_DIR}/streams/memory_stream.c
    ${FCEUMM_LRC_DIR}/string/stdstring.c
    ${FCEUMM_LRC_DIR}/time/rtime.c
    ${FCEUMM_LRC_DIR}/vfs/vfs_implementation.c
)

add_library(fceumm_core STATIC
    ${FCEUMM_BOARDS_SRC}
    ${FCEUMM_INPUT_SRC}
    ${FCEUMM_NTSC_SRC}
    ${FCEUMM_MAIN_SRC}
    ${FCEUMM_COMPAT_SRC}
)

target_include_directories(fceumm_core PRIVATE
    ${FCEUMM_CORE_DIR}/drivers/libretro
    ${FCEUMM_LRC_DIR}/include
    ${FCEUMM_CORE_DIR}
    ${FCEUMM_CORE_DIR}/input
    ${FCEUMM_CORE_DIR}/boards
    ${FCEUMM_CORE_DIR}/ntsc
)

target_compile_definitions(fceumm_core PRIVATE
    __LIBRETRO__
    PATH_MAX=1024
    FCEU_VERSION_NUMERIC=9813
    FRONTEND_SUPPORTS_RGB888
    HAVE_NTSC_FILTER
)

apply_retro_symbol_rename(fceumm_core fceumm)

set_target_properties(fceumm_core PROPERTIES C_STANDARD 11)

# =========================================================
# Snes9x (SNES) 核心 – 按 libretro/Makefile + Makefile.common
# =========================================================
set(SNES9X_CORE_DIR ${SNES9X_PATH})

# C 源文件（Makefile.common:11,69-75）
set(SNES9X_C_SRC
    ${SNES9X_CORE_DIR}/filter/snes_ntsc.c
    ${SNES9X_CORE_DIR}/unzip/unzip.c
    ${SNES9X_CORE_DIR}/unzip/ioapi.c
)

# C++ 源文件（Makefile.common:12-65）
set(SNES9X_CXX_SRC
    ${SNES9X_CORE_DIR}/apu/apu.cpp
    ${SNES9X_CORE_DIR}/apu/bapu/dsp/sdsp.cpp
    ${SNES9X_CORE_DIR}/apu/bapu/smp/smp.cpp
    ${SNES9X_CORE_DIR}/apu/bapu/smp/smp_state.cpp
    ${SNES9X_CORE_DIR}/bsx.cpp
    ${SNES9X_CORE_DIR}/c4.cpp
    ${SNES9X_CORE_DIR}/c4emu.cpp
    ${SNES9X_CORE_DIR}/cheats.cpp
    ${SNES9X_CORE_DIR}/cheats2.cpp
    ${SNES9X_CORE_DIR}/clip.cpp
    ${SNES9X_CORE_DIR}/conffile.cpp
    ${SNES9X_CORE_DIR}/controls.cpp
    ${SNES9X_CORE_DIR}/cpu.cpp
    ${SNES9X_CORE_DIR}/cpuexec.cpp
    ${SNES9X_CORE_DIR}/cpuops.cpp
    ${SNES9X_CORE_DIR}/crosshairs.cpp
    ${SNES9X_CORE_DIR}/dma.cpp
    ${SNES9X_CORE_DIR}/dsp.cpp
    ${SNES9X_CORE_DIR}/dsp1.cpp
    ${SNES9X_CORE_DIR}/dsp2.cpp
    ${SNES9X_CORE_DIR}/dsp3.cpp
    ${SNES9X_CORE_DIR}/dsp4.cpp
    ${SNES9X_CORE_DIR}/fxinst.cpp
    ${SNES9X_CORE_DIR}/fxemu.cpp
    ${SNES9X_CORE_DIR}/gfx.cpp
    ${SNES9X_CORE_DIR}/globals.cpp
    ${SNES9X_CORE_DIR}/loadzip.cpp
    ${SNES9X_CORE_DIR}/memmap.cpp
    ${SNES9X_CORE_DIR}/obc1.cpp
    ${SNES9X_CORE_DIR}/msu1.cpp
    ${SNES9X_CORE_DIR}/ppu.cpp
    ${SNES9X_CORE_DIR}/stream.cpp
    ${SNES9X_CORE_DIR}/sa1.cpp
    ${SNES9X_CORE_DIR}/sa1cpu.cpp
    ${SNES9X_CORE_DIR}/screenshot.cpp
    ${SNES9X_CORE_DIR}/sdd1.cpp
    ${SNES9X_CORE_DIR}/sdd1emu.cpp
    ${SNES9X_CORE_DIR}/seta.cpp
    ${SNES9X_CORE_DIR}/seta010.cpp
    ${SNES9X_CORE_DIR}/seta011.cpp
    ${SNES9X_CORE_DIR}/seta018.cpp
    ${SNES9X_CORE_DIR}/snapshot.cpp
    ${SNES9X_CORE_DIR}/snes9x.cpp
    ${SNES9X_CORE_DIR}/spc7110.cpp
    ${SNES9X_CORE_DIR}/srtc.cpp
    ${SNES9X_CORE_DIR}/tile.cpp
    ${SNES9X_CORE_DIR}/tileimpl-n1x1.cpp
    ${SNES9X_CORE_DIR}/tileimpl-n2x1.cpp
    ${SNES9X_CORE_DIR}/tileimpl-h2x1.cpp
    ${SNES9X_CORE_DIR}/sha256.cpp
    ${SNES9X_CORE_DIR}/bml.cpp
    ${SNES9X_CORE_DIR}/movie.cpp
    ${SNES9X_CORE_DIR}/fscompat.cpp
    ${SNES9X_CORE_DIR}/libretro/libretro.cpp
)

add_library(snes9x_core STATIC
    ${SNES9X_C_SRC}
    ${SNES9X_CXX_SRC}
)

target_include_directories(snes9x_core PRIVATE
    ${SNES9X_CORE_DIR}/libretro
    ${SNES9X_CORE_DIR}/libretro/libretro-common/include
    ${SNES9X_CORE_DIR}
    ${SNES9X_CORE_DIR}/apu
    ${SNES9X_CORE_DIR}/apu/bapu
    ${SNES9X_CORE_DIR}/unzip
)

target_compile_definitions(snes9x_core PRIVATE
    __LIBRETRO__
    RIGHTSHIFT_IS_SAR
    ALLOW_CPU_OVERCLOCK
    HAVE_STDINT_H
    HAVE_STRINGS_H
)

# Snes9x 需要 zlib（unzip 功能）；优先使用 mGBA 内建的 zlibstatic
if (TARGET zlibstatic)
    target_link_libraries(snes9x_core PRIVATE zlibstatic)
    target_include_directories(snes9x_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/third-party/zlib
        ${CMAKE_BINARY_DIR}/third_party/zlib
    )
else()
    find_package(ZLIB)
    if (ZLIB_FOUND)
        target_link_libraries(snes9x_core PRIVATE ZLIB::ZLIB)
    endif()
endif()

apply_retro_symbol_rename(snes9x_core snes9x)

set_target_properties(snes9x_core PROPERTIES
    CXX_STANDARD 14
    C_STANDARD 11
)

# =========================================================
# Genesis Plus GX 核心 – 按 Makefile.libretro + libretro/Makefile.common
# =========================================================
set(GENESIS_CORE_DIR ${GENESIS_PATH})

file(GLOB GENESIS_CORE_SRC
    ${GENESIS_CORE_DIR}/core/*.c
)
file(GLOB GENESIS_Z80_SRC
    ${GENESIS_CORE_DIR}/core/z80/*.c
)
file(GLOB GENESIS_M68K_SRC
    ${GENESIS_CORE_DIR}/core/m68k/*.c
)
file(GLOB GENESIS_NTSC_SRC
    ${GENESIS_CORE_DIR}/core/ntsc/*.c
)
file(GLOB GENESIS_SOUND_SRC
    ${GENESIS_CORE_DIR}/core/sound/*.c
)
file(GLOB GENESIS_TREMOR_SRC
    ${GENESIS_CORE_DIR}/core/sound/tremor/*.c
)
file(GLOB GENESIS_INPUTHW_SRC
    ${GENESIS_CORE_DIR}/core/input_hw/*.c
)
file(GLOB GENESIS_CDHW_SRC
    ${GENESIS_CORE_DIR}/core/cd_hw/*.c
)
file(GLOB GENESIS_CARTHW_SRC
    ${GENESIS_CORE_DIR}/core/cart_hw/*.c
)
file(GLOB GENESIS_SVP_SRC
    ${GENESIS_CORE_DIR}/core/cart_hw/svp/*.c
)

# main libretro entry point
set(GENESIS_LIBRETRO_SRC
    ${GENESIS_CORE_DIR}/libretro/libretro.c
)

# libretro-common 兼容层（Makefile.common:63-83）
set(GENESIS_LRC_DIR ${GENESIS_CORE_DIR}/libretro/libretro-common)
set(GENESIS_LRC_SRC
    ${GENESIS_LRC_DIR}/streams/file_stream.c
    ${GENESIS_LRC_DIR}/streams/file_stream_transforms.c
    ${GENESIS_LRC_DIR}/compat/fopen_utf8.c
    ${GENESIS_LRC_DIR}/compat/compat_snprintf.c
    ${GENESIS_LRC_DIR}/compat/compat_strl.c
    ${GENESIS_LRC_DIR}/compat/compat_strcasestr.c
    ${GENESIS_LRC_DIR}/compat/compat_posix_string.c
    ${GENESIS_LRC_DIR}/encodings/encoding_utf.c
    ${GENESIS_LRC_DIR}/file/file_path.c
    ${GENESIS_LRC_DIR}/file/retro_dirent.c
    ${GENESIS_LRC_DIR}/lists/string_list.c
    ${GENESIS_LRC_DIR}/lists/dir_list.c
    ${GENESIS_LRC_DIR}/memmap/memalign.c
    ${GENESIS_LRC_DIR}/string/stdstring.c
    ${GENESIS_LRC_DIR}/vfs/vfs_implementation.c
    ${GENESIS_LRC_DIR}/cdrom/cdrom.c
    ${GENESIS_LRC_DIR}/vfs/vfs_implementation_cdrom.c
)

# zlib: 使用 mGBA 内建的 zlibstatic 而非重复嵌入（避免符号冲突）
if (TARGET zlibstatic)
    target_include_directories(genesis_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/third-party/zlib
        ${CMAKE_BINARY_DIR}/third_party/zlib
    )
    target_link_libraries(genesis_core PRIVATE zlibstatic)
else()
    # fallback: 嵌入 zlib 源文件
    set(GENESIS_ZLIB_DIR ${GENESIS_CORE_DIR}/libretro/deps/zlib-1.2.11)
    set(GENESIS_ZLIB_SRC
        ${GENESIS_ZLIB_DIR}/adler32.c
        ${GENESIS_ZLIB_DIR}/crc32.c
        ${GENESIS_ZLIB_DIR}/inffast.c
        ${GENESIS_ZLIB_DIR}/inflate.c
        ${GENESIS_ZLIB_DIR}/inftrees.c
        ${GENESIS_ZLIB_DIR}/zutil.c
    )
    target_sources(genesis_core PRIVATE ${GENESIS_ZLIB_SRC})
    target_include_directories(genesis_core PRIVATE ${GENESIS_DEPS_DIR}/zlib-1.2.11)
endif()

# CHD / LZMA / zstd 依赖（Makefile.common:25-57）
set(GENESIS_DEPS_DIR ${GENESIS_CORE_DIR}/libretro/deps)
set(GENESIS_LZMA_SRC
    ${GENESIS_DEPS_DIR}/lzma-19.00/src/Alloc.c
    ${GENESIS_DEPS_DIR}/lzma-19.00/src/Bra86.c
    ${GENESIS_DEPS_DIR}/lzma-19.00/src/BraIA64.c
    ${GENESIS_DEPS_DIR}/lzma-19.00/src/CpuArch.c
    ${GENESIS_DEPS_DIR}/lzma-19.00/src/Delta.c
    ${GENESIS_DEPS_DIR}/lzma-19.00/src/LzFind.c
    ${GENESIS_DEPS_DIR}/lzma-19.00/src/Lzma86Dec.c
    ${GENESIS_DEPS_DIR}/lzma-19.00/src/LzmaDec.c
    ${GENESIS_DEPS_DIR}/lzma-19.00/src/LzmaEnc.c
    ${GENESIS_DEPS_DIR}/lzma-19.00/src/Sort.c
)
set(GENESIS_LIBCHDR_SRC
    ${GENESIS_DEPS_DIR}/libchdr/src/libchdr_bitstream.c
    ${GENESIS_DEPS_DIR}/libchdr/src/libchdr_cdrom.c
    ${GENESIS_DEPS_DIR}/libchdr/src/libchdr_chd.c
    ${GENESIS_DEPS_DIR}/libchdr/src/libchdr_flac.c
    ${GENESIS_DEPS_DIR}/libchdr/src/libchdr_huffman.c
)
set(GENESIS_ZSTD_SRC
    ${GENESIS_DEPS_DIR}/zstd/lib/common/entropy_common.c
    ${GENESIS_DEPS_DIR}/zstd/lib/common/error_private.c
    ${GENESIS_DEPS_DIR}/zstd/lib/common/fse_decompress.c
    ${GENESIS_DEPS_DIR}/zstd/lib/common/zstd_common.c
    ${GENESIS_DEPS_DIR}/zstd/lib/common/xxhash.c
    ${GENESIS_DEPS_DIR}/zstd/lib/decompress/huf_decompress.c
    ${GENESIS_DEPS_DIR}/zstd/lib/decompress/zstd_ddict.c
    ${GENESIS_DEPS_DIR}/zstd/lib/decompress/zstd_decompress.c
    ${GENESIS_DEPS_DIR}/zstd/lib/decompress/zstd_decompress_block.c
)

add_library(genesis_core STATIC
    ${GENESIS_CORE_SRC}
    ${GENESIS_Z80_SRC}
    ${GENESIS_M68K_SRC}
    ${GENESIS_NTSC_SRC}
    ${GENESIS_SOUND_SRC}
    ${GENESIS_TREMOR_SRC}
    ${GENESIS_INPUTHW_SRC}
    ${GENESIS_CDHW_SRC}
    ${GENESIS_CARTHW_SRC}
    ${GENESIS_SVP_SRC}
    ${GENESIS_LIBRETRO_SRC}
    ${GENESIS_LRC_SRC}
    ${GENESIS_LZMA_SRC}
    ${GENESIS_LIBCHDR_SRC}
    ${GENESIS_ZSTD_SRC}
)

target_include_directories(genesis_core PRIVATE
    ${GENESIS_CORE_DIR}/libretro
    ${GENESIS_LRC_DIR}/include
    ${GENESIS_CORE_DIR}/core
    ${GENESIS_CORE_DIR}/core/z80
    ${GENESIS_CORE_DIR}/core/m68k
    ${GENESIS_CORE_DIR}/core/ntsc
    ${GENESIS_CORE_DIR}/core/sound
    ${GENESIS_CORE_DIR}/core/sound/minimp3
    ${GENESIS_CORE_DIR}/core/input_hw
    ${GENESIS_CORE_DIR}/core/cd_hw
    ${GENESIS_CORE_DIR}/core/cart_hw
    ${GENESIS_CORE_DIR}/core/cart_hw/svp
    ${GENESIS_CORE_DIR}/core/sound/tremor
    ${GENESIS_DEPS_DIR}/libchdr/include
    ${GENESIS_DEPS_DIR}/lzma-19.00/include
    ${GENESIS_DEPS_DIR}/zstd/lib
)

target_compile_definitions(genesis_core PRIVATE
    __LIBRETRO__
    USE_LIBRETRO_VFS
    USE_16BPP_RENDERING
    FRONTEND_SUPPORTS_RGB565
    LSB_FIRST
    BYTE_ORDER=LITTLE_ENDIAN
    ALIGN_LONG
    ALIGN_WORD
    M68K_OVERCLOCK_SHIFT=20
    Z80_OVERCLOCK_SHIFT=20
    HAVE_YM3438_CORE
    HAVE_OPLL_CORE
    HAVE_ZLIB
    USE_LIBTREMOR
    USE_LIBCHDR
    USE_PER_SOUND_CHANNELS_CONFIG
    _7ZIP_ST
    ZSTD_DISABLE_ASM
    MAXROMSIZE=33554432
    HAVE_CDROM
    ENABLE_SUB_68K_ADDRESS_ERROR_EXCEPTIONS
    INLINE="static inline"
)

apply_retro_symbol_rename(genesis_core genesis)

set_target_properties(genesis_core PROPERTIES
    C_STANDARD 11
    CXX_STANDARD 11
)

