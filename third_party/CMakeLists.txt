cmake_minimum_required(VERSION 3.10)

# =========================================================
# 路径变量（与根目录 CMakeLists.txt 保持一致）
# =========================================================
set(BOREALIS_LIBRARY   ${CMAKE_CURRENT_SOURCE_DIR}/borealis/library)
set(MGBA_PATH          ${CMAKE_CURRENT_SOURCE_DIR}/mgba)
set(FCEUMM_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/libretro-fceumm)
set(SNES9X_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/snes9x)

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
        "retro_load_game_special=${PREFIX}_retro_load_game_special"
    )
endfunction()

function(apply_data_symbol_rename TARGET PREFIX)
    target_compile_definitions(${TARGET} PRIVATE
        "log_cb=${PREFIX}_log_cb"
        "options_intl=${PREFIX}_options_intl"
    )
    foreach(LOCALE us tr vn val uk sv sr sk ru pt_pt pt_br pl no nl ko ja it id hu hr he gl fr fi fa es eo en el de da cy cs cht chs ca ast ar)
        target_compile_definitions(${TARGET} PRIVATE
            "options_${LOCALE}=${PREFIX}_options_${LOCALE}"
            "option_cats_${LOCALE}=${PREFIX}_option_cats_${LOCALE}"
            "option_defs_${LOCALE}=${PREFIX}_option_defs_${LOCALE}"
        )
    endforeach()
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
apply_data_symbol_rename(fceumm_core fceumm)
target_compile_definitions(fceumm_core PRIVATE "PPU=fceumm_PPU")

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

if (PLATFORM_SWITCH)
    target_compile_definitions(snes9x_core PRIVATE
        __SWITCH__
        SWITCH
        HAVE_LIBNX
        IOAPI_NO_64
    )
endif ()

apply_retro_symbol_rename(snes9x_core snes9x)
apply_data_symbol_rename(snes9x_core snes9x)
target_compile_definitions(snes9x_core PRIVATE "PPU=snes9x_PPU")

set_target_properties(snes9x_core PROPERTIES
    CXX_STANDARD 14
    C_STANDARD 11
)

# =========================================================
# PicoDrive 核心 – 替代 Genesis Plus GX（MD/Genesis/SMS/GG/CD/32X）
# 按 Makefile.libretro + platform/common/common.mak
# =========================================================
set(PICODRIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/picodrive)

set(PICODRIVE_CORE_SRC
    ${PICODRIVE_PATH}/pico/pico.c
    ${PICODRIVE_PATH}/pico/cart.c
    ${PICODRIVE_PATH}/pico/memory.c
    ${PICODRIVE_PATH}/pico/state.c
    ${PICODRIVE_PATH}/pico/sek.c
    ${PICODRIVE_PATH}/pico/z80if.c
    ${PICODRIVE_PATH}/pico/videoport.c
    ${PICODRIVE_PATH}/pico/draw2.c
    ${PICODRIVE_PATH}/pico/draw.c
    ${PICODRIVE_PATH}/pico/mode4.c
    ${PICODRIVE_PATH}/pico/misc.c
    ${PICODRIVE_PATH}/pico/eeprom.c
    ${PICODRIVE_PATH}/pico/patch.c
    ${PICODRIVE_PATH}/pico/debug.c
    ${PICODRIVE_PATH}/pico/media.c
    ${PICODRIVE_PATH}/pico/sms.c
    ${PICODRIVE_PATH}/pico/cd/mcd.c
    ${PICODRIVE_PATH}/pico/cd/memory.c
    ${PICODRIVE_PATH}/pico/cd/sek.c
    ${PICODRIVE_PATH}/pico/cd/cdc.c
    ${PICODRIVE_PATH}/pico/cd/cdd.c
    ${PICODRIVE_PATH}/pico/cd/cd_image.c
    ${PICODRIVE_PATH}/pico/cd/cd_parse.c
    ${PICODRIVE_PATH}/pico/cd/gfx.c
    ${PICODRIVE_PATH}/pico/cd/gfx_dma.c
    ${PICODRIVE_PATH}/pico/cd/misc.c
    ${PICODRIVE_PATH}/pico/cd/pcm.c
    ${PICODRIVE_PATH}/pico/cd/megasd.c
    ${PICODRIVE_PATH}/pico/32x/32x.c
    ${PICODRIVE_PATH}/pico/32x/memory.c
    ${PICODRIVE_PATH}/pico/32x/draw.c
    ${PICODRIVE_PATH}/pico/32x/sh2soc.c
    ${PICODRIVE_PATH}/pico/32x/pwm.c
    ${PICODRIVE_PATH}/pico/pico/pico.c
    ${PICODRIVE_PATH}/pico/pico/memory.c
    ${PICODRIVE_PATH}/pico/pico/xpcm.c
    ${PICODRIVE_PATH}/pico/carthw/carthw.c
    ${PICODRIVE_PATH}/pico/carthw/eeprom_spi.c
    ${PICODRIVE_PATH}/pico/carthw/svp/svp.c
    ${PICODRIVE_PATH}/pico/carthw/svp/memory.c
    ${PICODRIVE_PATH}/pico/carthw/svp/ssp16.c
    ${PICODRIVE_PATH}/pico/sound/sound.c
    ${PICODRIVE_PATH}/pico/sound/resampler.c
    ${PICODRIVE_PATH}/pico/sound/sn76496.c
    ${PICODRIVE_PATH}/pico/sound/ym2612.c
    ${PICODRIVE_PATH}/pico/sound/ym2413.c
    ${PICODRIVE_PATH}/pico/sound/vgm.c
    ${PICODRIVE_PATH}/pico/sound/mix.c
    ${PICODRIVE_PATH}/pico/sound/emu2413/emu2413.c
    ${PICODRIVE_PATH}/cpu/fame/famec.c
    ${PICODRIVE_PATH}/cpu/cz80/cz80.c
    ${PICODRIVE_PATH}/cpu/drc/cmn.c
    ${PICODRIVE_PATH}/cpu/sh2/sh2.c
    ${PICODRIVE_PATH}/cpu/sh2/compiler.c
    ${PICODRIVE_PATH}/cpu/sh2/mame/sh2pico.c
    ${PICODRIVE_PATH}/platform/libretro/libretro.c
)

set(PICODRIVE_LRC_DIR ${PICODRIVE_PATH}/platform/libretro/libretro-common)
set(PICODRIVE_LRC_SRC
    ${PICODRIVE_LRC_DIR}/streams/file_stream.c
    ${PICODRIVE_LRC_DIR}/streams/file_stream_transforms.c
    ${PICODRIVE_LRC_DIR}/compat/fopen_utf8.c
    ${PICODRIVE_LRC_DIR}/compat/compat_strl.c
    ${PICODRIVE_LRC_DIR}/compat/compat_strcasestr.c
    ${PICODRIVE_LRC_DIR}/compat/compat_posix_string.c
    ${PICODRIVE_LRC_DIR}/encodings/encoding_utf.c
    ${PICODRIVE_LRC_DIR}/file/file_path.c
    ${PICODRIVE_LRC_DIR}/file/file_path_io.c
    ${PICODRIVE_LRC_DIR}/string/stdstring.c
    ${PICODRIVE_LRC_DIR}/time/rtime.c
    ${PICODRIVE_LRC_DIR}/vfs/vfs_implementation.c
    ${PICODRIVE_LRC_DIR}/formats/png/rpng.c
    ${PICODRIVE_LRC_DIR}/memmap/memmap.c
    ${PICODRIVE_LRC_DIR}/streams/trans_stream_zlib.c
    ${PICODRIVE_LRC_DIR}/streams/trans_stream.c
    ${PICODRIVE_LRC_DIR}/streams/trans_stream_pipe.c
)

set(PICODRIVE_PLAT_SRC
    ${PICODRIVE_PATH}/platform/common/ogg.c
    ${PICODRIVE_PATH}/platform/common/mp3.c
    ${PICODRIVE_PATH}/platform/common/mp3_sync.c
    ${PICODRIVE_PATH}/platform/common/mp3_ogg_stubs.c
)

set(PICODRIVE_TREMOR_DIR ${PICODRIVE_PATH}/platform/common/tremor)
set(PICODRIVE_TREMOR_SRC
    ${PICODRIVE_TREMOR_DIR}/bitwise.c
    ${PICODRIVE_TREMOR_DIR}/block.c
    ${PICODRIVE_TREMOR_DIR}/codebook.c
    ${PICODRIVE_TREMOR_DIR}/floor0.c
    ${PICODRIVE_TREMOR_DIR}/floor1.c
    ${PICODRIVE_TREMOR_DIR}/framing.c
    ${PICODRIVE_TREMOR_DIR}/info.c
    ${PICODRIVE_TREMOR_DIR}/mapping0.c
    ${PICODRIVE_TREMOR_DIR}/mdct.c
    ${PICODRIVE_TREMOR_DIR}/registry.c
    ${PICODRIVE_TREMOR_DIR}/res012.c
    ${PICODRIVE_TREMOR_DIR}/sharedbook.c
    ${PICODRIVE_TREMOR_DIR}/synthesis.c
    ${PICODRIVE_TREMOR_DIR}/vorbisfile.c
    ${PICODRIVE_TREMOR_DIR}/window.c
)

set(PICODRIVE_ZLIB_DIR ${PICODRIVE_PATH}/zlib)
set(PICODRIVE_ZLIB_SRC
    ${PICODRIVE_ZLIB_DIR}/adler32.c
    ${PICODRIVE_ZLIB_DIR}/crc32.c
    ${PICODRIVE_ZLIB_DIR}/inffast.c
    ${PICODRIVE_ZLIB_DIR}/inflate.c
    ${PICODRIVE_ZLIB_DIR}/inftrees.c
    ${PICODRIVE_ZLIB_DIR}/trees.c
    ${PICODRIVE_ZLIB_DIR}/deflate.c
    ${PICODRIVE_ZLIB_DIR}/zutil.c
    ${PICODRIVE_ZLIB_DIR}/compress.c
    ${PICODRIVE_ZLIB_DIR}/uncompr.c
)

set(PICODRIVE_UNZIP_SRC
    ${PICODRIVE_PATH}/unzip/unzip.c
)

add_library(picodrive_core STATIC
    ${PICODRIVE_CORE_SRC}
    ${PICODRIVE_LRC_SRC}
    ${PICODRIVE_TREMOR_SRC}
    ${PICODRIVE_ZLIB_SRC}
    ${PICODRIVE_UNZIP_SRC}
    ${PICODRIVE_PLAT_SRC}
)

target_include_directories(picodrive_core PRIVATE
    ${PICODRIVE_PATH}
    ${PICODRIVE_PATH}/pico
    ${PICODRIVE_PATH}/pico/cd
    ${PICODRIVE_PATH}/pico/32x
    ${PICODRIVE_PATH}/pico/carthw
    ${PICODRIVE_PATH}/pico/pico
    ${PICODRIVE_PATH}/pico/sound
    ${PICODRIVE_PATH}/cpu
    ${PICODRIVE_PATH}/cpu/cz80
    ${PICODRIVE_PATH}/cpu/fame
    ${PICODRIVE_PATH}/cpu/drc
    ${PICODRIVE_PATH}/cpu/sh2
    ${PICODRIVE_PATH}/cpu/sh2/mame
    ${PICODRIVE_PATH}/platform/libretro
    ${PICODRIVE_LRC_DIR}/include
    ${PICODRIVE_LRC_DIR}/include/compat
    ${PICODRIVE_LRC_DIR}/include/encodings
    ${PICODRIVE_LRC_DIR}/include/formats
    ${PICODRIVE_LRC_DIR}/include/streams
    ${PICODRIVE_LRC_DIR}/include/string
    ${PICODRIVE_LRC_DIR}/include/vfs
    ${PICODRIVE_TREMOR_DIR}
    ${PICODRIVE_ZLIB_DIR}
    ${PICODRIVE_PATH}/unzip
    ${PICODRIVE_PATH}/platform/common
)

target_compile_definitions(picodrive_core PRIVATE
    __LIBRETRO__
    USE_LIBRETRO_VFS
    FRONTEND_SUPPORTS_RGB565
    EMU_F68K
    _USE_CZ80
    DRC_SH2
    USE_TREMOR
    HAVE_ZLIB
    LSB_FIRST
    FAMEC_NO_GOTOS
    "REVISION=\"\""
)

if (PLATFORM_SWITCH)
    target_compile_definitions(picodrive_core PRIVATE
        __SWITCH__
        SWITCH
        HAVE_LIBNX
    )
endif ()

apply_retro_symbol_rename(picodrive_core picodrive)
apply_data_symbol_rename(picodrive_core picodrive)

set_target_properties(picodrive_core PROPERTIES
    C_STANDARD 11
    CXX_STANDARD 11
)

# =========================================================
# Genesis Plus GX – MD / Genesis / SMS / GG / Sega CD
# 动态/静态链接 libretro 核心
# 替代 PicoDrive，更稳定、兼容性更好
# =========================================================
set(GENESIS_PLUS_GX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/genesis_plus_gx)

set(GENESIS_PLUS_GX_CORE_SRC_DIRS
    ${GENESIS_PLUS_GX_PATH}/core
    ${GENESIS_PLUS_GX_PATH}/core/z80
    ${GENESIS_PLUS_GX_PATH}/core/m68k
    ${GENESIS_PLUS_GX_PATH}/core/ntsc
    ${GENESIS_PLUS_GX_PATH}/core/sound
    ${GENESIS_PLUS_GX_PATH}/core/sound/minimp3
    ${GENESIS_PLUS_GX_PATH}/core/input_hw
    ${GENESIS_PLUS_GX_PATH}/core/cd_hw
    ${GENESIS_PLUS_GX_PATH}/core/cart_hw
    ${GENESIS_PLUS_GX_PATH}/core/cart_hw/svp
)

set(GENESIS_PLUS_GX_LRC_SRC
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/streams/file_stream.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/streams/file_stream_transforms.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/compat/fopen_utf8.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/compat/compat_snprintf.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/compat/compat_strl.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/compat/compat_strcasestr.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/compat/compat_posix_string.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/encodings/encoding_utf.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/file/file_path.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/file/retro_dirent.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/lists/string_list.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/lists/dir_list.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/memmap/memalign.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/string/stdstring.c
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/vfs/vfs_implementation.c
)

set(GENESIS_PLUS_GX_ZLIB_SRC
    ${GENESIS_PLUS_GX_PATH}/libretro/deps/zlib-1.2.11/adler32.c
    ${GENESIS_PLUS_GX_PATH}/libretro/deps/zlib-1.2.11/crc32.c
    ${GENESIS_PLUS_GX_PATH}/libretro/deps/zlib-1.2.11/inffast.c
    ${GENESIS_PLUS_GX_PATH}/libretro/deps/zlib-1.2.11/inflate.c
    ${GENESIS_PLUS_GX_PATH}/libretro/deps/zlib-1.2.11/inftrees.c
    ${GENESIS_PLUS_GX_PATH}/libretro/deps/zlib-1.2.11/zutil.c
)

file(GLOB_RECURSE GENESIS_PLUS_GX_CORE_SRC
    ${GENESIS_PLUS_GX_PATH}/core/*.c
)
list(FILTER GENESIS_PLUS_GX_CORE_SRC EXCLUDE REGEX ".*/libchdr/.*")

add_library(genesis_plus_gx_core STATIC
    ${GENESIS_PLUS_GX_CORE_SRC}
    ${GENESIS_PLUS_GX_LRC_SRC}
    ${GENESIS_PLUS_GX_ZLIB_SRC}
)

target_include_directories(genesis_plus_gx_core PRIVATE
    ${GENESIS_PLUS_GX_PATH}/libretro
    ${GENESIS_PLUS_GX_PATH}/core
    ${GENESIS_PLUS_GX_PATH}/core/z80
    ${GENESIS_PLUS_GX_PATH}/core/m68k
    ${GENESIS_PLUS_GX_PATH}/core/ntsc
    ${GENESIS_PLUS_GX_PATH}/core/sound
    ${GENESIS_PLUS_GX_PATH}/core/sound/minimp3
    ${GENESIS_PLUS_GX_PATH}/core/input_hw
    ${GENESIS_PLUS_GX_PATH}/core/cd_hw
    ${GENESIS_PLUS_GX_PATH}/core/cart_hw
    ${GENESIS_PLUS_GX_PATH}/core/cart_hw/svp
    ${GENESIS_PLUS_GX_PATH}/libretro/libretro-common/include
    ${GENESIS_PLUS_GX_PATH}/libretro/deps/zlib-1.2.11
)

target_compile_definitions(genesis_plus_gx_core PRIVATE
    USE_16BPP_RENDERING
    FRONTEND_SUPPORTS_RGB565
    HAVE_NO_LANGEXTRA
    HAVE_ZLIB
    LSB_FIRST
    __LIBRETRO__
    M68K_OVERCLOCK_SHIFT=20
    Z80_OVERCLOCK_SHIFT=20
    HAVE_YM3438_CORE
    HAVE_OPLL_CORE
    USE_LIBRETRO_VFS
)

target_compile_options(genesis_plus_gx_core PRIVATE
    -fgnu89-inline
)

# GENPLUS_STATIC_INLINE makes macros.h use static inline instead of static __inline__
# This matches the Makefile.libretro behavior for non-MSVC platforms

if (NOT PLATFORM_SWITCH)
    target_compile_definitions(genesis_plus_gx_core PRIVATE
        _7ZIP_ST
        ZSTD_DISABLE_ASM
        HAVE_CHD
    )
    set(GENESIS_PLUS_GX_CHD_DEPS
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/lzma-19.00/src/Alloc.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/lzma-19.00/src/Bra86.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/lzma-19.00/src/BraIA64.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/lzma-19.00/src/CpuArch.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/lzma-19.00/src/Delta.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/lzma-19.00/src/LzFind.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/lzma-19.00/src/Lzma86Dec.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/lzma-19.00/src/LzmaDec.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/lzma-19.00/src/LzmaEnc.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/lzma-19.00/src/Sort.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/libchdr/src/libchdr_bitstream.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/libchdr/src/libchdr_cdrom.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/libchdr/src/libchdr_chd.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/libchdr/src/libchdr_flac.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/libchdr/src/libchdr_huffman.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/zstd/lib/common/entropy_common.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/zstd/lib/common/error_private.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/zstd/lib/common/fse_decompress.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/zstd/lib/common/zstd_common.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/zstd/lib/common/xxhash.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/zstd/lib/decompress/huf_decompress.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/zstd/lib/decompress/zstd_ddict.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/zstd/lib/decompress/zstd_decompress.c
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/zstd/lib/decompress/zstd_decompress_block.c
    )
    target_sources(genesis_plus_gx_core PRIVATE ${GENESIS_PLUS_GX_CHD_DEPS})
    target_include_directories(genesis_plus_gx_core PRIVATE
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/libchdr/include
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/lzma-19.00/include
        ${GENESIS_PLUS_GX_PATH}/libretro/deps/zstd/lib
    )
endif ()

if (PLATFORM_SWITCH)
    target_compile_definitions(genesis_plus_gx_core PRIVATE
        __SWITCH__
        SWITCH
        HAVE_LIBNX
    )
endif ()

apply_retro_symbol_rename(genesis_plus_gx_core genesis_plus_gx)
apply_data_symbol_rename(genesis_plus_gx_core genesis_plus_gx)

set_target_properties(genesis_plus_gx_core PROPERTIES
    C_STANDARD 11
    CXX_STANDARD 11
)

