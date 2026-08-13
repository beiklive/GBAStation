cmake_minimum_required(VERSION 3.10)



set(BOREALIS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/borealis/library)
set(MGBA_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba)

# 构建选项
include(${BOREALIS_LIBRARY}/cmake/commonOption.cmake)

# 依赖项
option(USE_SHARED_LIB "Whether to use shared libs provided by system" OFF)
cmake_dependent_option(USE_SYSTEM_FMT "" OFF "NOT USE_SHARED_LIB" ON)
cmake_dependent_option(USE_SYSTEM_TINYXML2 "" OFF "NOT USE_SHARED_LIB" ON)
cmake_dependent_option(USE_SYSTEM_TWEENY "" OFF "NOT USE_SHARED_LIB" ON)

if (APPLE AND PLATFORM_DESKTOP)
    option(BUNDLE_MACOS_APP "Bundle a app for macOS" OFF)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "" FORCE)
    set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET[arch=arm64] "11.0" CACHE STRING "" FORCE)
    message(STATUS "CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")
endif ()

# 工具链配置
include(${BOREALIS_LIBRARY}/cmake/toolchain.cmake)

# 项目信息
project(GBAStation)
set(VERSION_MAJOR "0")
set(VERSION_MINOR "3")
set(VERSION_ALTER "10")
set(VERSION_BUILD "0")
set(PACKAGE_NAME "com.beiklive.GBAStation")
set(PROJECT_AUTHOR "beiklive")
set(PROJECT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/default.png)
set(PROJECT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources)

if (PLATFORM_SWITCH)
    set(BUILD_MELONDS_CORE ON CACHE BOOL "Build melonDS only for the external NDS Stub NRO" FORCE)
else()
    set(BUILD_MELONDS_CORE OFF CACHE BOOL "Build the embedded melonDS native core" FORCE)
endif()

if (USE_LIBROMFS)
    add_libromfs(${PROJECT_NAME} ${PROJECT_RESOURCES})
endif ()



# 为 mGBA 打开 POSIX/GNU 扩展，确保 strdup 有声明
if (TARGET mgba AND NOT WIN32)
    target_compile_definitions(mgba PRIVATE _POSIX_C_SOURCE=200809L _GNU_SOURCE)
endif ()

# Switch (devkitA64/newlib) 交叉编译时，CMake feature-detection 无法运行目标
# 平台二进制，导致 HAVE_LOCALE / HAVE_STRTOF_L 未被检测到。
# 但 devkitA64 的 newlib 实际提供了 locale_t / strtof_l / localtime_r，
# 若不补充这些宏，formatting.h 会重复定义 locale_t / strtof_l，产生类型冲突。
if (TARGET mgba AND PLATFORM_SWITCH)
    target_compile_definitions(mgba PRIVATE
        HAVE_LOCALE
        HAVE_STRTOF_L
        HAVE_LOCALTIME_R
    )
endif ()

add_subdirectory(third_party)
add_subdirectory(src/emulator/genesis)

# FAKE-08 is embedded as a runtime-only static library. Platform hosts,
# SDL frontends, libretro glue and the upstream main() are intentionally
# excluded; BeikLiveStation provides its host implementation in core/pico8.
set(FAKE08_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fake-08-master)
# The source archive did not contain submodule contents. libs/z8lua is the
# upstream-pinned commit e6928578d46b61fd5ea30cfcf547e855a30a0553.
set(FAKE08_RUNTIME_SOURCES
    ${FAKE08_DIR}/source/Audio.cpp
    ${FAKE08_DIR}/source/cart.cpp
    ${FAKE08_DIR}/source/emojiconversion.cpp
    ${FAKE08_DIR}/source/filehelpers.cpp
    ${FAKE08_DIR}/source/filter.cpp
    ${FAKE08_DIR}/source/fontdata.cpp
    ${FAKE08_DIR}/source/graphics.cpp
    ${FAKE08_DIR}/source/Input.cpp
    ${FAKE08_DIR}/source/logger.cpp
    ${FAKE08_DIR}/source/mathhelpers.cpp
    ${FAKE08_DIR}/source/nibblehelpers.cpp
    ${FAKE08_DIR}/source/picoluaapi.cpp
    ${FAKE08_DIR}/source/printHelper.cpp
    ${FAKE08_DIR}/source/stringToDataHelpers.cpp
    ${FAKE08_DIR}/source/synth.cpp
    ${FAKE08_DIR}/source/vm.cpp
    ${FAKE08_DIR}/libs/lodepng/lodepng.cpp
)
set(FAKE08_LUA_SOURCES
    ${FAKE08_DIR}/libs/z8lua/lapi.c
    ${FAKE08_DIR}/libs/z8lua/lauxlib.c
    ${FAKE08_DIR}/libs/z8lua/lbaselib.c
    ${FAKE08_DIR}/libs/z8lua/lcode.c
    ${FAKE08_DIR}/libs/z8lua/lcorolib.c
    ${FAKE08_DIR}/libs/z8lua/lctype.c
    ${FAKE08_DIR}/libs/z8lua/ldblib.c
    ${FAKE08_DIR}/libs/z8lua/ldebug.c
    ${FAKE08_DIR}/libs/z8lua/ldo.c
    ${FAKE08_DIR}/libs/z8lua/ldump.c
    ${FAKE08_DIR}/libs/z8lua/eris.c
    ${FAKE08_DIR}/libs/z8lua/lfunc.c
    ${FAKE08_DIR}/libs/z8lua/lgc.c
    ${FAKE08_DIR}/libs/z8lua/linit.c
    ${FAKE08_DIR}/libs/z8lua/llex.c
    ${FAKE08_DIR}/libs/z8lua/lmem.c
    ${FAKE08_DIR}/libs/z8lua/lobject.c
    ${FAKE08_DIR}/libs/z8lua/lopcodes.c
    ${FAKE08_DIR}/libs/z8lua/lparser.c
    ${FAKE08_DIR}/libs/z8lua/lpico8lib.c
    ${FAKE08_DIR}/libs/z8lua/lstate.c
    ${FAKE08_DIR}/libs/z8lua/lstring.c
    ${FAKE08_DIR}/libs/z8lua/lstrlib.c
    ${FAKE08_DIR}/libs/z8lua/ltable.c
    ${FAKE08_DIR}/libs/z8lua/ltablib.c
    ${FAKE08_DIR}/libs/z8lua/ltm.c
    ${FAKE08_DIR}/libs/z8lua/lundump.c
    ${FAKE08_DIR}/libs/z8lua/lvm.c
    ${FAKE08_DIR}/libs/z8lua/lzio.c
)
set_source_files_properties(${FAKE08_LUA_SOURCES} PROPERTIES LANGUAGE CXX)
add_library(fake08_runtime STATIC
    ${FAKE08_RUNTIME_SOURCES}
    ${FAKE08_LUA_SOURCES}
)
target_include_directories(fake08_runtime PUBLIC
    ${FAKE08_DIR}/source
    ${FAKE08_DIR}/libs/z8lua
    ${FAKE08_DIR}/libs/lodepng
    ${FAKE08_DIR}/libs/simpleini
    ${FAKE08_DIR}/libs/miniz
)
target_compile_definitions(fake08_runtime PUBLIC
    LOAD_PACK_INS=0
    VER_STR="v0.0.2.20"
)
set_target_properties(fake08_runtime PROPERTIES CXX_STANDARD 17)

# mGBA: 目标创建后再补充编译宏（Switch/newlib + POSIX/GNU 扩展）
if (TARGET mgba AND NOT WIN32)
    target_compile_definitions(mgba PRIVATE
        _POSIX_C_SOURCE=200809L
        _GNU_SOURCE
    )
endif ()

if (TARGET mgba AND PLATFORM_SWITCH)
    target_compile_definitions(mgba PRIVATE
        HAVE_LOCALE
        HAVE_STRTOF_L
        HAVE_LOCALTIME_R
    )
endif ()

find_package(Threads REQUIRED)
list(APPEND APP_PLATFORM_LIB ${CMAKE_THREAD_LIBS_INIT})

# expose the repository root so LibretroLoader.hpp can include the vendored
# libretro API header used by NES/SNES wrappers.
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "Building mGBA from source (third_party/mgba)")

# ============================================================
# Audio: use native platform audio APIs.
#   Linux   → ALSA  (libasound)
#   Windows → WinMM (mmsystem)
#   macOS   → CoreAudio (AudioUnit)
#   Switch  → libnx audout
# ============================================================
if (PLATFORM_DESKTOP)
    if (UNIX AND NOT APPLE)
        # Linux – ALSA
        find_library(ALSA_LIB asound)
        if (ALSA_LIB)
            list(APPEND APP_PLATFORM_LIB ${ALSA_LIB})
            add_compile_definitions(BK_AUDIO_ALSA)
            message(STATUS "Audio backend: ALSA (${ALSA_LIB})")
        else()
            message(STATUS "Audio backend: none (libasound not found, samples discarded)")
        endif()
    elseif (WIN32)
        list(APPEND APP_PLATFORM_LIB winmm)
        add_compile_definitions(BK_AUDIO_WINMM)
        message(STATUS "Audio backend: WinMM")
    elseif (APPLE)
        list(APPEND APP_PLATFORM_LIB
             "-framework AudioUnit"
             "-framework CoreAudio"
             "-framework CoreServices")
        add_compile_definitions(BK_AUDIO_COREAUDIO)
        message(STATUS "Audio backend: CoreAudio")
    endif()
elseif (PLATFORM_SWITCH)
    # audout is provided by libnx – already linked via borealis/toolchain
    message(STATUS "Audio backend: libnx audout")
elseif (PLATFORM_ANDROID)
    # OpenSL ES / AAudio via SDL2 (handled by SDL2 internally)
    message(STATUS "Audio backend: SDL2 (OpenSL ES / AAudio)")
else()
    message(STATUS "Audio backend: none (samples will be discarded)")
endif ()

include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz)
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mongoose)
file(GLOB_RECURSE MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(FILTER MAIN_SRC EXCLUDE REGEX ".*/src/platform/switch/nds_stub/.*")
list(FILTER MAIN_SRC EXCLUDE REGEX ".*/src/emulator/melonds/.*")
list(FILTER MAIN_SRC EXCLUDE REGEX ".*/src/emulator/genesis/.*")
list(FILTER MAIN_SRC EXCLUDE REGEX ".*/src/game/mgba/.*")
list(FILTER MAIN_SRC EXCLUDE REGEX ".*/src/tests/.*")
list(APPEND MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz/miniz.c)
list(APPEND MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mongoose/mongoose.c)
list(APPEND MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/qrcodegen/qrcodegen.cpp)
# list(APPEND MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)


set(PLATFORM_OPTION)
if (PLATFORM_DESKTOP)
    if (WIN32)
        # 如果项目本地存在 resource.rc.in 资源描述文件则使用
        if (EXISTS "${CMAKE_SOURCE_DIR}/resources/resource.rc.in")
            configure_file("${CMAKE_SOURCE_DIR}/resources/resource.rc.in" "${CMAKE_BINARY_DIR}/resource.rc")
            list(APPEND MAIN_SRC ${CMAKE_BINARY_DIR}/resource.rc)
        endif ()
        if (MINGW)
            # set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static" CACHE STRING "" FORCE)
        endif ()
    elseif (BUNDLE_MACOS_APP)
        # 如果项目本地存在 icns 图标文件则使用
        if (EXISTS "${CMAKE_SOURCE_DIR}/resources/borealis.icns")
            list(APPEND MAIN_SRC ${CMAKE_SOURCE_DIR}/resources/borealis.icns)
        endif ()
    endif ()
elseif (PLATFORM_SWITCH)
    list(APPEND MAIN_SRC ${CMAKE_SOURCE_DIR}/third_party/borealis/library/lib/platforms/switch/switch_wrapper.c)
endif ()




# 构建可执行目标
program_target(${PROJECT_NAME} "${MAIN_SRC}")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}")
target_compile_definitions(${PROJECT_NAME} PRIVATE COLOR_16_BIT COLOR_5_6_5)
target_link_libraries(${PROJECT_NAME} PRIVATE fake08_runtime)
target_link_libraries(${PROJECT_NAME} PRIVATE genesis_core)
if (PLATFORM_SWITCH)
    add_subdirectory(src/core/forwarder/hbl forwarder_hbl)
    add_dependencies(${PROJECT_NAME} gbastation_forwarder_hbl_exefs)
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/forwarder/ForwarderCore.cpp
        PROPERTIES COMPILE_OPTIONS "-std=gnu++2c;--embed-dir=${GBASTATION_FORWARDER_EMBED_DIR}"
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE MG_ARCH=MG_ARCH_UNIX)
endif()


# 生成发布文件
if (PLATFORM_DESKTOP)
    if (BUNDLE_MACOS_APP)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
            MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
            MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}"
            MACOSX_BUNDLE_ICON_FILE "borealis.icns"
            MACOSX_BUNDLE_COPYRIGHT "Copyright 2023 ${PROJECT_AUTHOR}"
        )
        if (EXISTS "${CMAKE_SOURCE_DIR}/resources/borealis.icns")
            set_target_properties(${PROJECT_NAME} PROPERTIES
                RESOURCE "${CMAKE_SOURCE_DIR}/resources/borealis.icns"
            )
        endif ()
        add_custom_target(${PROJECT_NAME}.data
                COMMAND "${CMAKE_COMMAND}" -E copy_directory ${PROJECT_RESOURCES} ${PROJECT_NAME}.app/Contents/Resources/resources
        )
    else ()
        add_custom_target(${PROJECT_NAME}.data
                COMMAND "${CMAKE_COMMAND}" -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources
        )
    endif ()
    if (NOT USE_LIBROMFS)
        add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}.data)
    endif ()
elseif (PLATFORM_SWITCH)
    if (USE_DEKO3D)
        gen_dksh("${PROJECT_RESOURCES}/shaders")
    endif ()
    add_custom_target(${PROJECT_NAME}.nro DEPENDS ${PROJECT_NAME}
        COMMAND ${NX_NACPTOOL_EXE} --create "${PROJECT_NAME}" "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}" ${PROJECT_NAME}.nacp --titleid=${PROJECT_TITLEID}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/resources
        COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/resources/core/GBAStationNDSStub.nro
        COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/resources/core/GBAStationFlycastStub.nro
        COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/resources/core/GBAStationFBNeoStub.nro
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources/font
        COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro --icon=${PROJECT_ICON} --nacp=${PROJECT_NAME}.nacp --romfsdir=${CMAKE_BINARY_DIR}/resources
    )

    add_subdirectory(nds_stub)

endif ()



target_include_directories(${PROJECT_NAME} PRIVATE ${APP_PLATFORM_INCLUDE})
target_compile_options(${PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections ${APP_PLATFORM_OPTION})
target_link_libraries(${PROJECT_NAME} PRIVATE borealis ${APP_PLATFORM_LIB})
target_link_libraries(${PROJECT_NAME} PRIVATE giflib)
target_link_libraries(${PROJECT_NAME} PRIVATE ffmpeg_avformat ffmpeg_avcodec ffmpeg_swscale ffmpeg_avutil)
add_dependencies(${PROJECT_NAME} ffmpeg_minimal)
if (WIN32)
    # Required by FFmpeg's minimal MinGW configuration (random seed and the
    # built-in H.264 decoder). Keep this explicit rather than relying on
    # whichever desktop frontend happens to link these system libraries.
    target_link_libraries(${PROJECT_NAME} PRIVATE bcrypt ole32 user32)
endif()
if(PLATFORM_SWITCH)
    target_link_libraries(${PROJECT_NAME} PRIVATE jpeg)
endif()
if(NOT PLATFORM_ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE curl)
endif()

# ============================================================
# 静态链接模拟器核心
#
# mGBA          – GBA / GBC / GB      (native source API)
# FCEUmm        – NES / Famicom       (libretro, fceumm_retro_*)
# Snes9x        – SNES / SFC          (libretro, snes9x_retro_*)
# Nestopia      – NES / Famicom       (libretro, nestopia_retro_*)
# Gambatte      – GB / GBC            (libretro source, gambatte_retro_*)
# Snes9x 2005   – SNES / SFC          (libretro, snes9x2005_retro_*)
# NDS runs in GBAStationNDSStub.nro, launched as a separate Switch NRO.
#
# mGBA 不再构建/加载 mgba_libretro，也不复制 mgba 动态库。
# ============================================================
if (TARGET fceumm_core)
    target_link_libraries(${PROJECT_NAME} PRIVATE fceumm_core)
    message(STATUS "Linking FCEUmm (NES) core statically")
endif()
if (TARGET snes9x_core)
    target_link_libraries(${PROJECT_NAME} PRIVATE snes9x_core)
    message(STATUS "Linking Snes9x (SNES) core statically")
endif()
if (TARGET nestopia_core)
    target_link_libraries(${PROJECT_NAME} PRIVATE nestopia_core)
    message(STATUS "Linking Nestopia (NES) core statically")
endif()
if (TARGET gambatte_core)
    target_link_libraries(${PROJECT_NAME} PRIVATE gambatte_core)
    message(STATUS "Linking Gambatte (GB/GBC) core statically")
endif()
if (TARGET snes9x2005_core)
    target_link_libraries(${PROJECT_NAME} PRIVATE snes9x2005_core)
    message(STATUS "Linking Snes9x 2005 (SNES) core statically")
endif()
if (TARGET mgba)
    target_link_libraries(${PROJECT_NAME} PRIVATE mgba)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba/include
        ${CMAKE_BINARY_DIR}/third_party/mgba/include
    )
    message(STATUS "Linking mGBA native core")
endif()
