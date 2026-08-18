cmake_minimum_required(VERSION 3.10)

# =========================================================
# 路径变量（与根目录 CMakeLists.txt 保持一致）
# =========================================================
set(BOREALIS_LIBRARY   ${CMAKE_CURRENT_SOURCE_DIR}/borealis/library)
set(MGBA_PATH          ${CMAKE_CURRENT_SOURCE_DIR}/mgba)
set(FCEUMM_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/libretro-fceumm)
set(SNES9X_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/snes9x)
set(NESTOPIA_PATH      ${CMAKE_CURRENT_SOURCE_DIR}/nestopia)
set(GAMBATTE_PATH      ${CMAKE_CURRENT_SOURCE_DIR}/gambatte-libretro)
set(SNES9X2005_PATH    ${CMAKE_CURRENT_SOURCE_DIR}/snes9x2005)
set(MELONDS_PATH       ${CMAKE_CURRENT_SOURCE_DIR}/melonDS)

option(BUILD_MELONDS_CORE "Build the embedded melonDS native core" OFF)

# =========================================================
# borealis UI 框架
# =========================================================
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/borealis/library)

# =========================================================
# giflib - GIF87a/GIF89a decoder used by UI image widgets
# =========================================================
add_library(giflib STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/giflib/dgif_lib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/giflib/gifalloc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/giflib/gif_err.c
    ${CMAKE_CURRENT_SOURCE_DIR}/giflib/gif_hash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/giflib/openbsd-reallocarray.c
)
target_include_directories(giflib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/giflib
)

# =========================================================
# FFmpeg - background MP4 video playback
#
# Do not use the system FFmpeg executable or shared libraries.  The custom
# target configures an isolated, static build with MOV/MP4 demuxing, common
# video decoders, file input and RGBA scaling. In particular it excludes
# programs, filters, devices, encoders, muxers and network support.
# =========================================================
set(FFMPEG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/FFmpeg)
# FFmpeg's configure generates headers and objects in its source directory.
# Keep a self-contained source copy under each CMake build directory so
# Windows and Switch never share architecture-specific static archives.
set(FFMPEG_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-background-audio)
set(FFMPEG_BUILD_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/configure_ffmpeg_minimal.sh)
# The configured copy is regenerated whenever the vendored source changes.
# Without this dependency CMake can retain archives from a prior platform or
# source revision because custom-command OUTPUT paths themselves still exist.
file(GLOB_RECURSE FFMPEG_SOURCE_FILES CONFIGURE_DEPENDS
    ${FFMPEG_SOURCE_DIR}/*
)

if (WIN32)
    # CMake's MinGW compiler lives in <msys-root>/mingw64/bin.  Use the
    # matching MSYS shell so configure and GNU make see a consistent PATH.
    get_filename_component(_ffmpeg_mingw_bin "${CMAKE_C_COMPILER}" DIRECTORY)
    get_filename_component(_ffmpeg_mingw_root "${_ffmpeg_mingw_bin}" DIRECTORY)
    get_filename_component(_ffmpeg_msys_root "${_ffmpeg_mingw_root}" DIRECTORY)
    set(FFMPEG_SHELL "${_ffmpeg_msys_root}/usr/bin/bash.exe")
    set(FFMPEG_CC "${_ffmpeg_mingw_bin}/gcc.exe")
    set(FFMPEG_AR "${_ffmpeg_mingw_bin}/ar.exe")
    set(FFMPEG_RANLIB "${_ffmpeg_mingw_bin}/ranlib.exe")
else()
    find_program(FFMPEG_SHELL NAMES bash sh REQUIRED)
    set(FFMPEG_CC ${CMAKE_C_COMPILER})
    set(FFMPEG_AR ${CMAKE_AR})
    set(FFMPEG_RANLIB ${CMAKE_RANLIB})
endif()
if (PLATFORM_SWITCH)
    set(FFMPEG_TARGET_KIND switch)
else()
    set(FFMPEG_TARGET_KIND native)
endif()

set(FFMPEG_STATIC_LIBRARIES
    ${FFMPEG_BUILD_DIR}/libavutil/libavutil.a
    ${FFMPEG_BUILD_DIR}/libavcodec/libavcodec.a
    ${FFMPEG_BUILD_DIR}/libavformat/libavformat.a
    ${FFMPEG_BUILD_DIR}/libswscale/libswscale.a
)

add_custom_command(
    OUTPUT ${FFMPEG_STATIC_LIBRARIES}
    COMMAND ${FFMPEG_SHELL} ${FFMPEG_BUILD_SCRIPT}
            ${FFMPEG_SOURCE_DIR} ${FFMPEG_BUILD_DIR}
            ${FFMPEG_CC} ${FFMPEG_AR} ${FFMPEG_RANLIB} ${FFMPEG_TARGET_KIND}
    DEPENDS ${FFMPEG_SOURCE_FILES} ${FFMPEG_BUILD_SCRIPT}
    COMMENT "Building minimal FFmpeg (MP4/H.264 background decoder)"
    VERBATIM
)
add_custom_target(ffmpeg_minimal DEPENDS ${FFMPEG_STATIC_LIBRARIES})

function(add_ffmpeg_static_library TARGET_NAME LIBRARY_PATH)
    add_library(${TARGET_NAME} STATIC IMPORTED GLOBAL)
    set_target_properties(${TARGET_NAME} PROPERTIES
        IMPORTED_LOCATION ${LIBRARY_PATH}
        # Public FFmpeg headers are source files.  Generated configuration
        # headers are consumed only while building the isolated static libs.
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_SOURCE_DIR}"
    )
    add_dependencies(${TARGET_NAME} ffmpeg_minimal)
endfunction()

add_ffmpeg_static_library(ffmpeg_avutil ${FFMPEG_BUILD_DIR}/libavutil/libavutil.a)
add_ffmpeg_static_library(ffmpeg_avcodec ${FFMPEG_BUILD_DIR}/libavcodec/libavcodec.a)
add_ffmpeg_static_library(ffmpeg_avformat ${FFMPEG_BUILD_DIR}/libavformat/libavformat.a)
add_ffmpeg_static_library(ffmpeg_swscale ${FFMPEG_BUILD_DIR}/libswscale/libswscale.a)
target_link_libraries(ffmpeg_avcodec INTERFACE ffmpeg_avutil)
target_link_libraries(ffmpeg_avformat INTERFACE ffmpeg_avcodec ffmpeg_avutil)
target_link_libraries(ffmpeg_swscale INTERFACE ffmpeg_avutil)

# =========================================================
# mGBA 模拟器  –  源码 native 静态库构建
# =========================================================
set(BUILD_LIBRETRO OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC   ON  CACHE BOOL "" FORCE)
set(BUILD_SHARED   OFF CACHE BOOL "" FORCE)
set(BUILD_SDL      OFF CACHE BOOL "" FORCE)
set(BUILD_QT       OFF CACHE BOOL "" FORCE)
set(BUILD_LTO      OFF CACHE BOOL "" FORCE)

if (PLATFORM_SWITCH)
    set(CMAKE_TOOLCHAIN_FILE   "mgba/src/platform/switch/CMakeToolchain.txt")
    set(LIBMGBA_ONLY    ON  CACHE BOOL "" FORCE)
    set(BUILD_GL        OFF CACHE BOOL "" FORCE)
    set(BUILD_GLES2     OFF CACHE BOOL "" FORCE)
    set(BUILD_GLES3     OFF CACHE BOOL "" FORCE)
    set(USE_SQLITE3     OFF CACHE BOOL "" FORCE)
endif ()

set(BUILD_PERF     OFF CACHE BOOL "" FORCE)
set(BUILD_TEST     OFF CACHE BOOL "" FORCE)
set(BUILD_SUITE    OFF CACHE BOOL "" FORCE)
set(BUILD_CINEMA   OFF CACHE BOOL "" FORCE)
set(BUILD_DOCGEN   OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mgba)

if (TARGET mgba)
    target_compile_definitions(mgba PRIVATE COLOR_16_BIT COLOR_5_6_5)
endif ()

if (PLATFORM_SWITCH AND TARGET mgba)
    target_sources(mgba PRIVATE
        ${MGBA_PATH}/src/util/vfs/vfs-fd.c
        ${MGBA_PATH}/src/util/vfs/vfs-dirent.c
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
    foreach(LOCALE us tr vn val uk sv sr sk si ru ro pt_pt pt_br pl oc no nl mt ko ja it id hu hr he gl fr fi fa es eo en el de da cy cs cht chs ca ast ar)
        target_compile_definitions(${TARGET} PRIVATE
            "options_${LOCALE}=${PREFIX}_options_${LOCALE}"
            "option_cats_${LOCALE}=${PREFIX}_option_cats_${LOCALE}"
            "option_defs_${LOCALE}=${PREFIX}_option_defs_${LOCALE}"
        )
    endforeach()
endfunction()

set(LIBRETRO_API_SYMBOLS
    retro_init
    retro_deinit
    retro_api_version
    retro_get_system_info
    retro_get_system_av_info
    retro_set_environment
    retro_set_video_refresh
    retro_set_audio_sample
    retro_set_audio_sample_batch
    retro_set_input_poll
    retro_set_input_state
    retro_set_controller_port_device
    retro_reset
    retro_run
    retro_serialize_size
    retro_serialize
    retro_unserialize
    retro_load_game
    retro_unload_game
    retro_get_memory_data
    retro_get_memory_size
    retro_cheat_reset
    retro_cheat_set
    retro_get_region
    retro_load_game_special
)

function(add_isolated_libretro_core TARGET OBJECT_TARGET PREFIX)
    set(_raw_target "${TARGET}_raw")
    set(_tmp_obj "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_combined.tmp.o")
    set(_out_obj "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_combined.o")
    set(_keep_args)
    foreach(_sym IN LISTS LIBRETRO_API_SYMBOLS)
        list(APPEND _keep_args "--keep-global-symbol=${PREFIX}_${_sym}")
    endforeach()
    if (PREFIX STREQUAL "nestopia")
        list(APPEND _keep_args
            "--wildcard"
            "--keep-global-symbol=_ZN3Nes*"
            "--keep-global-symbol=_ZNK3Nes*"
            "--keep-global-symbol=_ZTVN3Nes*"
            "--keep-global-symbol=_ZTIN3Nes*"
            "--keep-global-symbol=_ZTSN3Nes*"
            "--keep-global-symbol=_ZNSt*"
            "--keep-global-symbol=_ZNKSt*"
            "--keep-global-symbol=_ZSt*"
        )
    elseif(PREFIX STREQUAL "gambatte")
        list(APPEND _keep_args
            "--wildcard"
            "--keep-global-symbol=_ZN8gambatte*"
            "--keep-global-symbol=_ZNK8gambatte*"
            "--keep-global-symbol=_ZTVN8gambatte*"
            "--keep-global-symbol=_ZTIN8gambatte*"
            "--keep-global-symbol=_ZTSN8gambatte*"
            "--keep-global-symbol=_ZNSt*"
            "--keep-global-symbol=_ZNKSt*"
            "--keep-global-symbol=_ZSt*"
            "--keep-global-symbol=blip*"
        )
    endif()

    add_library(${_raw_target} STATIC $<TARGET_OBJECTS:${OBJECT_TARGET}>)
    add_dependencies(${_raw_target} ${OBJECT_TARGET})
    set_target_properties(${_raw_target} PROPERTIES LINKER_LANGUAGE C)

    add_custom_command(
        OUTPUT "${_out_obj}"
        COMMAND ${CMAKE_COMMAND} -E rm -f "${_tmp_obj}" "${_out_obj}"
        COMMAND ${CMAKE_CXX_COMPILER} -r -o "${_tmp_obj}"
                -Wl,--whole-archive "$<TARGET_FILE:${_raw_target}>" -Wl,--no-whole-archive
        COMMAND ${CMAKE_OBJCOPY} ${_keep_args} "${_tmp_obj}" "${_out_obj}"
        DEPENDS ${_raw_target}
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
    set_source_files_properties("${_out_obj}" PROPERTIES
        GENERATED TRUE
        EXTERNAL_OBJECT TRUE
    )
    add_library(${TARGET} STATIC "${_out_obj}")
    add_dependencies(${TARGET} ${OBJECT_TARGET})
    set_target_properties(${TARGET} PROPERTIES LINKER_LANGUAGE C)
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

add_library(fceumm_core_obj OBJECT
    ${FCEUMM_BOARDS_SRC}
    ${FCEUMM_INPUT_SRC}
    ${FCEUMM_NTSC_SRC}
    ${FCEUMM_MAIN_SRC}
    ${FCEUMM_COMPAT_SRC}
)

target_include_directories(fceumm_core_obj PRIVATE
    ${FCEUMM_CORE_DIR}/drivers/libretro
    ${FCEUMM_LRC_DIR}/include
    ${FCEUMM_CORE_DIR}
    ${FCEUMM_CORE_DIR}/input
    ${FCEUMM_CORE_DIR}/boards
    ${FCEUMM_CORE_DIR}/ntsc
)

target_compile_definitions(fceumm_core_obj PRIVATE
    __LIBRETRO__
    PATH_MAX=1024
    FCEU_VERSION_NUMERIC=9900
    # Match upstream Makefile.libretro's libnx target (WANT_32BPP=1).
    # The frontend converts XRGB8888 to its internal RGBA snapshot format.
    FRONTEND_SUPPORTS_RGB888
    HAVE_NTSC_FILTER
)
if (PLATFORM_SWITCH)
    # Match FCEUmm's upstream `platform=libnx` build. These flags select
    # Switch-safe VFS and static-linking paths used before the first emulated
    # frame, rather than the desktop fallback selected by a generic CMake build.
    target_compile_definitions(fceumm_core_obj PRIVATE
        SWITCH=1
        __SWITCH__
        HAVE_LIBNX
        RARCH_INTERNAL
        STATIC_LINKING
        ARM
    )
    # libnx builds explicitly remove host Linux defines before selecting
    # Switch-specific VFS and platform code.
    target_compile_options(fceumm_core_obj PRIVATE -U__linux__ -U__linux -mtp=soft)
endif()

apply_retro_symbol_rename(fceumm_core_obj fceumm)
apply_data_symbol_rename(fceumm_core_obj fceumm)
target_compile_definitions(fceumm_core_obj PRIVATE "PPU=fceumm_PPU")

set_target_properties(fceumm_core_obj PROPERTIES C_STANDARD 11)
add_isolated_libretro_core(fceumm_core fceumm_core_obj fceumm)

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
# Nestopia (NES) 核心
# =========================================================
file(GLOB NESTOPIA_CORE_SRC
    ${NESTOPIA_PATH}/source/core/*.cpp
    ${NESTOPIA_PATH}/source/core/api/*.cpp
    ${NESTOPIA_PATH}/source/core/board/*.cpp
    ${NESTOPIA_PATH}/source/core/input/*.cpp
    ${NESTOPIA_PATH}/source/core/vssystem/*.cpp
)
list(REMOVE_ITEM NESTOPIA_CORE_SRC
    ${NESTOPIA_PATH}/source/core/NstVideoFilter2xSaI.cpp
    ${NESTOPIA_PATH}/source/core/NstVideoFilterHqX.cpp
    ${NESTOPIA_PATH}/source/core/NstVideoFilterScaleX.cpp
    ${NESTOPIA_PATH}/source/core/NstVideoFilterxBR.cpp
)
list(APPEND NESTOPIA_CORE_SRC
    ${NESTOPIA_PATH}/libretro/libretro.cpp
)

set(NESTOPIA_LRC_DIR ${NESTOPIA_PATH}/libretro/libretro-common)
set(NESTOPIA_LRC_SRC
    ${NESTOPIA_LRC_DIR}/compat/compat_posix_string.c
    ${NESTOPIA_LRC_DIR}/compat/compat_strcasestr.c
    ${NESTOPIA_LRC_DIR}/compat/compat_snprintf.c
    ${NESTOPIA_LRC_DIR}/compat/compat_strl.c
    ${NESTOPIA_LRC_DIR}/compat/fopen_utf8.c
    ${NESTOPIA_LRC_DIR}/encodings/encoding_utf.c
    ${NESTOPIA_LRC_DIR}/file/file_path.c
    ${NESTOPIA_LRC_DIR}/file/file_path_io.c
    ${NESTOPIA_LRC_DIR}/streams/file_stream.c
    ${NESTOPIA_LRC_DIR}/time/rtime.c
    ${NESTOPIA_LRC_DIR}/vfs/vfs_implementation.c
)

add_library(nestopia_core_obj OBJECT ${NESTOPIA_CORE_SRC} ${NESTOPIA_LRC_SRC})
target_include_directories(nestopia_core_obj PRIVATE
    ${NESTOPIA_PATH}/libretro
    ${NESTOPIA_PATH}
    ${NESTOPIA_PATH}/libretro/libretro-common/include
)
target_compile_definitions(nestopia_core_obj PRIVATE __LIBRETRO__)
apply_retro_symbol_rename(nestopia_core_obj nestopia)
apply_data_symbol_rename(nestopia_core_obj nestopia)
set_target_properties(nestopia_core_obj PROPERTIES CXX_STANDARD 11 C_STANDARD 11)
add_isolated_libretro_core(nestopia_core nestopia_core_obj nestopia)

# =========================================================
# Gambatte (GB/GBC) 核心
# =========================================================
set(GAMBATTE_CORE_DIR ${GAMBATTE_PATH}/libgambatte/src)
set(GAMBATTE_LR_DIR ${GAMBATTE_PATH}/libgambatte/libretro)
set(GAMBATTE_LRC_DIR ${GAMBATTE_PATH}/libgambatte/libretro-common)
set(GAMBATTE_SRC
    ${GAMBATTE_LR_DIR}/gambatte_log.c
    ${GAMBATTE_LR_DIR}/blipper.c
    ${GAMBATTE_LR_DIR}/cc_resampler.c
    ${GAMBATTE_CORE_DIR}/bootloader.cpp
    ${GAMBATTE_CORE_DIR}/cpu.cpp
    ${GAMBATTE_CORE_DIR}/gambatte.cpp
    ${GAMBATTE_CORE_DIR}/initstate.cpp
    ${GAMBATTE_CORE_DIR}/interrupter.cpp
    ${GAMBATTE_CORE_DIR}/interruptrequester.cpp
    ${GAMBATTE_CORE_DIR}/gambatte-memory.cpp
    ${GAMBATTE_CORE_DIR}/sound.cpp
    ${GAMBATTE_CORE_DIR}/statesaver.cpp
    ${GAMBATTE_CORE_DIR}/tima.cpp
    ${GAMBATTE_CORE_DIR}/video.cpp
    ${GAMBATTE_CORE_DIR}/video_libretro.cpp
    ${GAMBATTE_CORE_DIR}/mem/cartridge.cpp
    ${GAMBATTE_CORE_DIR}/mem/cartridge_libretro.cpp
    ${GAMBATTE_CORE_DIR}/mem/huc3.cpp
    ${GAMBATTE_CORE_DIR}/mem/memptrs.cpp
    ${GAMBATTE_CORE_DIR}/mem/rtc.cpp
    ${GAMBATTE_CORE_DIR}/sound/channel1.cpp
    ${GAMBATTE_CORE_DIR}/sound/channel2.cpp
    ${GAMBATTE_CORE_DIR}/sound/channel3.cpp
    ${GAMBATTE_CORE_DIR}/sound/channel4.cpp
    ${GAMBATTE_CORE_DIR}/sound/duty_unit.cpp
    ${GAMBATTE_CORE_DIR}/sound/envelope_unit.cpp
    ${GAMBATTE_CORE_DIR}/sound/length_counter.cpp
    ${GAMBATTE_CORE_DIR}/video/ly_counter.cpp
    ${GAMBATTE_CORE_DIR}/video/lyc_irq.cpp
    ${GAMBATTE_CORE_DIR}/video/next_m0_time.cpp
    ${GAMBATTE_CORE_DIR}/video/ppu.cpp
    ${GAMBATTE_CORE_DIR}/video/sprite_mapper.cpp
    ${GAMBATTE_LR_DIR}/libretro.cpp
    ${GAMBATTE_LRC_DIR}/compat/compat_posix_string.c
    ${GAMBATTE_LRC_DIR}/compat/compat_snprintf.c
    ${GAMBATTE_LRC_DIR}/compat/compat_strcasestr.c
    ${GAMBATTE_LRC_DIR}/compat/compat_strl.c
    ${GAMBATTE_LRC_DIR}/compat/fopen_utf8.c
    ${GAMBATTE_LRC_DIR}/encodings/encoding_utf.c
    ${GAMBATTE_LRC_DIR}/file/file_path.c
    ${GAMBATTE_LRC_DIR}/file/file_path_io.c
    ${GAMBATTE_LRC_DIR}/streams/file_stream.c
    ${GAMBATTE_LRC_DIR}/streams/file_stream_transforms.c
    ${GAMBATTE_LRC_DIR}/string/stdstring.c
    ${GAMBATTE_LRC_DIR}/time/rtime.c
    ${GAMBATTE_LRC_DIR}/vfs/vfs_implementation.c
)

add_library(gambatte_core_obj OBJECT ${GAMBATTE_SRC})
target_include_directories(gambatte_core_obj PRIVATE
    ${GAMBATTE_CORE_DIR}
    ${GAMBATTE_PATH}/libgambatte/include
    ${GAMBATTE_PATH}/common
    ${GAMBATTE_LR_DIR}
    ${GAMBATTE_LRC_DIR}/include
)
# Use Gambatte's native RGB565 output. It retains the core's 256-pixel row
# stride while avoiding the XRGB8888 path shared with FCEUmm.
target_compile_definitions(gambatte_core_obj PRIVATE __LIBRETRO__ VIDEO_RGB565)
target_compile_options(gambatte_core_obj PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-include;cstdint>")
if (PLATFORM_SWITCH)
    target_compile_definitions(gambatte_core_obj PRIVATE __SWITCH__ SWITCH HAVE_LIBNX)
endif()
apply_retro_symbol_rename(gambatte_core_obj gambatte)
apply_data_symbol_rename(gambatte_core_obj gambatte)
set_target_properties(gambatte_core_obj PROPERTIES
    CXX_STANDARD 11
    C_STANDARD 11
)
add_isolated_libretro_core(gambatte_core gambatte_core_obj gambatte)

# =========================================================
# Snes9x 2005 (SNES) 核心
# =========================================================
set(SNES9X2005_SRC_DIR ${SNES9X2005_PATH}/source)
set(SNES9X2005_LRC_DIR ${SNES9X2005_PATH}/libretro-common)
set(SNES9X2005_SRC
    ${SNES9X2005_SRC_DIR}/c4.c
    ${SNES9X2005_SRC_DIR}/c4emu.c
    ${SNES9X2005_SRC_DIR}/cheats2.c
    ${SNES9X2005_SRC_DIR}/cheats.c
    ${SNES9X2005_SRC_DIR}/clip.c
    ${SNES9X2005_SRC_DIR}/cpu.c
    ${SNES9X2005_SRC_DIR}/cpuexec.c
    ${SNES9X2005_SRC_DIR}/cpuops.c
    ${SNES9X2005_SRC_DIR}/data.c
    ${SNES9X2005_SRC_DIR}/dma.c
    ${SNES9X2005_SRC_DIR}/dsp1.c
    ${SNES9X2005_SRC_DIR}/fxemu.c
    ${SNES9X2005_SRC_DIR}/fxinst.c
    ${SNES9X2005_SRC_DIR}/gfx.c
    ${SNES9X2005_SRC_DIR}/getset.c
    ${SNES9X2005_SRC_DIR}/globals.c
    ${SNES9X2005_SRC_DIR}/memmap.c
    ${SNES9X2005_SRC_DIR}/obc1.c
    ${SNES9X2005_SRC_DIR}/ppu.c
    ${SNES9X2005_SRC_DIR}/sa1.c
    ${SNES9X2005_SRC_DIR}/sa1cpu.c
    ${SNES9X2005_SRC_DIR}/sdd1.c
    ${SNES9X2005_SRC_DIR}/sdd1emu.c
    ${SNES9X2005_SRC_DIR}/seta010.c
    ${SNES9X2005_SRC_DIR}/seta011.c
    ${SNES9X2005_SRC_DIR}/seta018.c
    ${SNES9X2005_SRC_DIR}/seta.c
    ${SNES9X2005_SRC_DIR}/spc7110.c
    ${SNES9X2005_SRC_DIR}/spc7110dec.c
    ${SNES9X2005_SRC_DIR}/srtc.c
    ${SNES9X2005_SRC_DIR}/tile.c
    ${SNES9X2005_PATH}/libretro.c
    ${SNES9X2005_SRC_DIR}/apu.c
    ${SNES9X2005_SRC_DIR}/soundux.c
    ${SNES9X2005_SRC_DIR}/spc700.c
)

add_library(snes9x2005_core_obj OBJECT ${SNES9X2005_SRC})
target_include_directories(snes9x2005_core_obj PRIVATE
    ${SNES9X2005_PATH}
    ${SNES9X2005_SRC_DIR}
    ${SNES9X2005_LRC_DIR}/include
)
target_compile_definitions(snes9x2005_core_obj PRIVATE __LIBRETRO__ LOAD_FROM_MEMORY LAGFIX)
if (PLATFORM_SWITCH)
    target_compile_definitions(snes9x2005_core_obj PRIVATE __SWITCH__ SWITCH HAVE_LIBNX)
endif()
apply_retro_symbol_rename(snes9x2005_core_obj snes9x2005)
apply_data_symbol_rename(snes9x2005_core_obj snes9x2005)
target_compile_definitions(snes9x2005_core_obj PRIVATE "PPU=snes9x2005_PPU")
set_target_properties(snes9x2005_core_obj PROPERTIES C_STANDARD 11)
add_isolated_libretro_core(snes9x2005_core snes9x2005_core_obj snes9x2005)

# =========================================================
# melonDS (NDS) 原生核心
#
# 主程序不再内嵌 NDS 运行时；需要独立 NDS NRO 时再显式开启。
# =========================================================
if (BUILD_MELONDS_CORE)
set(MELONDS_SRC_DIR ${MELONDS_PATH}/src)

set(MELONDS_CORE_SRC
    ${MELONDS_SRC_DIR}/ARCodeFile.cpp
    ${MELONDS_SRC_DIR}/ARDatabaseDAT.cpp
    ${MELONDS_SRC_DIR}/AREngine.cpp
    ${MELONDS_SRC_DIR}/ARM.cpp
    ${MELONDS_SRC_DIR}/ARMInterpreter.cpp
    ${MELONDS_SRC_DIR}/ARMInterpreter_ALU.cpp
    ${MELONDS_SRC_DIR}/ARMInterpreter_Branch.cpp
    ${MELONDS_SRC_DIR}/ARMInterpreter_LoadStore.cpp
    ${MELONDS_SRC_DIR}/CP15.cpp
    ${MELONDS_SRC_DIR}/CRC32.cpp
    ${MELONDS_SRC_DIR}/DMA.cpp
    ${MELONDS_SRC_DIR}/DMA_Timings.cpp
    ${MELONDS_SRC_DIR}/DSi.cpp
    ${MELONDS_SRC_DIR}/DSi_AES.cpp
    ${MELONDS_SRC_DIR}/DSi_Camera.cpp
    ${MELONDS_SRC_DIR}/DSi_DSP.cpp
    ${MELONDS_SRC_DIR}/DSi_I2C.cpp
    ${MELONDS_SRC_DIR}/DSi_I2S.cpp
    ${MELONDS_SRC_DIR}/DSi_NAND.cpp
    ${MELONDS_SRC_DIR}/DSi_NDMA.cpp
    ${MELONDS_SRC_DIR}/DSi_NWifi.cpp
    ${MELONDS_SRC_DIR}/DSi_SD.cpp
    ${MELONDS_SRC_DIR}/DSi_SPI_TSC.cpp
    ${MELONDS_SRC_DIR}/FATIO.cpp
    ${MELONDS_SRC_DIR}/FATStorage.cpp
    ${MELONDS_SRC_DIR}/FreeBIOS.cpp
    ${MELONDS_SRC_DIR}/GBACart.cpp
    ${MELONDS_SRC_DIR}/GBACartMotionPak.cpp
    ${MELONDS_SRC_DIR}/GPU.cpp
    ${MELONDS_SRC_DIR}/GPU2D.cpp
    ${MELONDS_SRC_DIR}/GPU2D_Soft.cpp
    ${MELONDS_SRC_DIR}/GPU3D.cpp
    ${MELONDS_SRC_DIR}/GPU_OpenGL.cpp
    ${MELONDS_SRC_DIR}/GPU3D_Compute.cpp
    ${MELONDS_SRC_DIR}/GPU3D_OpenGL.cpp
    ${MELONDS_SRC_DIR}/GPU3D_Soft.cpp
    ${MELONDS_SRC_DIR}/GPU3D_Texcache.cpp
    ${MELONDS_SRC_DIR}/GPU3D_TexcacheOpenGL.cpp
    ${MELONDS_SRC_DIR}/Mic.cpp
    ${MELONDS_SRC_DIR}/NDS.cpp
    ${MELONDS_SRC_DIR}/NDSCart.cpp
    ${MELONDS_SRC_DIR}/NDSCartR4.cpp
    ${MELONDS_SRC_DIR}/ROMList.cpp
    ${MELONDS_SRC_DIR}/RTC.cpp
    ${MELONDS_SRC_DIR}/Savestate.cpp
    ${MELONDS_SRC_DIR}/SPI.cpp
    ${MELONDS_SRC_DIR}/SPI_Firmware.cpp
    ${MELONDS_SRC_DIR}/SPU.cpp
    ${MELONDS_SRC_DIR}/Utils.cpp
    ${MELONDS_SRC_DIR}/Wifi.cpp
    ${MELONDS_SRC_DIR}/WifiAP.cpp
    ${MELONDS_SRC_DIR}/OpenGLSupport.cpp
    ${MELONDS_SRC_DIR}/DSP_HLE/UcodeBase.cpp
    ${MELONDS_SRC_DIR}/DSP_HLE/AACUcode.cpp
    ${MELONDS_SRC_DIR}/DSP_HLE/G711Ucode.cpp
    ${MELONDS_SRC_DIR}/DSP_HLE/GraphicsUcode.cpp
    ${MELONDS_SRC_DIR}/fatfs/ff.c
    ${MELONDS_SRC_DIR}/fatfs/ffsystem.c
    ${MELONDS_SRC_DIR}/fatfs/ffunicode.c
    ${MELONDS_SRC_DIR}/sha1/sha1.c
    ${MELONDS_SRC_DIR}/tiny-AES-c/aes.c
    ${MELONDS_SRC_DIR}/xxhash/xxhash.c
    ${MELONDS_SRC_DIR}/blip-buf/blip_buf.c
)

if (PLATFORM_SWITCH)
    list(APPEND MELONDS_CORE_SRC
        ${MELONDS_SRC_DIR}/ARM_InstrInfo.cpp
        ${MELONDS_SRC_DIR}/ARMJIT.cpp
        ${MELONDS_SRC_DIR}/ARMJIT_Memory.cpp
        ${MELONDS_SRC_DIR}/ARMJIT_Global.cpp
        ${MELONDS_SRC_DIR}/dolphin/CommonFuncs.cpp
        ${MELONDS_SRC_DIR}/dolphin/Arm64Emitter.cpp
        ${MELONDS_SRC_DIR}/dolphin/MathUtil.cpp
        ${MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_Compiler.cpp
        ${MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_ALU.cpp
        ${MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_LoadStore.cpp
        ${MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_Branch.cpp
        ${MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_Linkage.S
    )
endif()

enable_language(ASM)

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${MELONDS_SRC_DIR}/teakra ${CMAKE_CURRENT_BINARY_DIR}/melonDS_teakra EXCLUDE_FROM_ALL)

set(MELONDS_VERSION_SUFFIX "" CACHE STRING "" FORCE)
set(MELONDS_EMBED_BUILD_INFO OFF CACHE BOOL "" FORCE)
set(MELONDS_GIT_BRANCH "" CACHE STRING "" FORCE)
set(MELONDS_GIT_HASH "" CACHE STRING "" FORCE)
set(MELONDS_BUILD_PROVIDER "GBAStation" CACHE STRING "" FORCE)
configure_file("${MELONDS_SRC_DIR}/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/melonDS_version/version.h")

add_library(melonds_core STATIC ${MELONDS_CORE_SRC})
target_include_directories(melonds_core PUBLIC
    ${MELONDS_SRC_DIR}
    ${BOREALIS_LIBRARY}/include/borealis/extern
    ${CMAKE_CURRENT_BINARY_DIR}/melonDS_version
)
target_compile_definitions(melonds_core PUBLIC
    OGLRENDERER_ENABLED
    MELONDS_GL_HEADER=<glad/glad.h>
)
target_compile_options(melonds_core PUBLIC -fwrapv)
target_compile_options(melonds_core PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wno-invalid-offsetof>")

if (PLATFORM_SWITCH)
    target_compile_definitions(melonds_core PUBLIC
        ENABLE_JIT=1
        JIT_ENABLED
    )
    target_compile_definitions(melonds_core PUBLIC
        __SWITCH__
        SWITCH
        HAVE_LIBNX
        ARCHITECTURE_ARM64=1
    )
    target_compile_options(melonds_core PRIVATE
        -O3
        -mcpu=cortex-a57
        -ffast-math
        -fno-lto
        -fomit-frame-pointer
    )
    target_link_options(melonds_core PRIVATE -flto)
endif()

target_link_libraries(melonds_core PRIVATE teakra)

if (PLATFORM_SWITCH)
set(ARCDELTA_MELONDS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ArcDelta_melonDS)
set(ARCDELTA_MELONDS_SRC_DIR ${ARCDELTA_MELONDS_PATH}/src)
set(ARCDELTA_MELONDS_CORE_SRC
    ${ARCDELTA_MELONDS_SRC_DIR}/ARCodeFile.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/AREngine.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARM.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARMInterpreter.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARMInterpreter_ALU.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARMInterpreter_Branch.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARMInterpreter_LoadStore.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/Config.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/CP15.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/CRC32.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/DMA.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/DSi.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/DSi_AES.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/DSi_Camera.cpp
    ${CMAKE_SOURCE_DIR}/nds_stub/src/NdsArcDeltaDsiDspStub.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/DSi_I2C.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/DSi_NDMA.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/DSi_NWifi.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/DSi_SD.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/DSi_SPI_TSC.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/GBACart.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/GPU.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/GPU2D.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/GPU2D_Soft.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/GPU2D_Deko.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/GPU3D.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/GPU3D_Transform.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/GPU3D_Soft.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/GPU3D_Deko.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/NDS.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/NDSCart.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/NDSCart_SRAMManager.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/RTC.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/Savestate.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/SPI.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/SPU.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/Wifi.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/WifiAP.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARM_InstrInfo.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARMJIT.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARMJIT_Memory.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/dolphin/CommonFuncs.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/dolphin/Arm64Emitter.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/dolphin/MathUtil.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_Compiler.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_ALU.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_LoadStore.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_Branch.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_Linkage.S
    ${ARCDELTA_MELONDS_SRC_DIR}/frontend/switch/PlatformConfig.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/frontend/switch/FaultHandler.cpp
    ${ARCDELTA_MELONDS_SRC_DIR}/tiny-AES-c/aes.c
    ${ARCDELTA_MELONDS_SRC_DIR}/xxhash/xxhash.c
)

add_library(arcdelta_melonds_core STATIC ${ARCDELTA_MELONDS_CORE_SRC})
target_include_directories(arcdelta_melonds_core PUBLIC
    ${ARCDELTA_MELONDS_SRC_DIR}
)
target_compile_definitions(arcdelta_melonds_core PUBLIC
    DEKOGPU_ENABLED
    JIT_ENABLED
    MELONDS_VERSION="AD"
    __SWITCH__
    SWITCH
    HAVE_LIBNX
    ARCHITECTURE_ARM64=1
)
target_compile_options(arcdelta_melonds_core PRIVATE
    -O3
    -flto
    -fPIC
    "$<$<COMPILE_LANGUAGE:CXX>:-Wno-invalid-offsetof>"
)
target_link_libraries(arcdelta_melonds_core PRIVATE teakra nx deko3d)

set(ARCDELTA_SWITCH_FRONTEND_DIR ${ARCDELTA_MELONDS_SRC_DIR}/frontend/switch)
add_library(arcdelta_switch_gfx STATIC
    ${ARCDELTA_SWITCH_FRONTEND_DIR}/Gfx.cpp
    ${ARCDELTA_SWITCH_FRONTEND_DIR}/GpuMemHeap.cpp
    ${ARCDELTA_SWITCH_FRONTEND_DIR}/UploadBuffer.cpp
    ${ARCDELTA_SWITCH_FRONTEND_DIR}/mm_vec/mm_vec.c
    ${ARCDELTA_SWITCH_FRONTEND_DIR}/stb_truetype/stb_truetype.c
)
target_include_directories(arcdelta_switch_gfx PUBLIC
    ${ARCDELTA_SWITCH_FRONTEND_DIR}
    ${ARCDELTA_MELONDS_SRC_DIR}
)
target_compile_definitions(arcdelta_switch_gfx PUBLIC
    __SWITCH__
    SWITCH
    HAVE_LIBNX
)
target_compile_options(arcdelta_switch_gfx PRIVATE
    -O3
    -flto
    -fPIC
)
target_link_libraries(arcdelta_switch_gfx PRIVATE nx deko3d)
endif()
endif()
