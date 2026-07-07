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
set(VERSION_MINOR "2")
set(VERSION_ALTER "3")
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
list(FILTER MAIN_SRC EXCLUDE REGEX ".*/src/emulator/mgba_native/.*")
list(FILTER MAIN_SRC EXCLUDE REGEX ".*/src/ui/page/NdsDekoGamePage\\.cpp$")
list(FILTER MAIN_SRC EXCLUDE REGEX ".*/src/ui/view/NdsDekoGameView\\.cpp$")
list(FILTER MAIN_SRC EXCLUDE REGEX ".*/src/ui/view/NdsDekoGameMenuView\\.cpp$")
list(FILTER MAIN_SRC EXCLUDE REGEX ".*/src/game/mgba/.*")
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
if (PLATFORM_SWITCH)
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
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources/font
        COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro --icon=${PROJECT_ICON} --nacp=${PROJECT_NAME}.nacp --romfsdir=${CMAKE_BINARY_DIR}/resources
    )

    add_subdirectory(nds_stub)
    add_subdirectory(mgba_stub)

    if (FALSE)
    find_program(NDS_STUB_UAM_EXE NAMES uam HINTS "$ENV{DEVKITPRO}/tools/bin")
    if (NOT NDS_STUB_UAM_EXE)
        message(FATAL_ERROR "Could not find uam for GBAStationNDSStub Deko shaders")
    endif()

    set(NDS_STUB_ROMFS_DIR ${CMAKE_BINARY_DIR}/nds_stub_romfs)
    set(NDS_STUB_SHADER_DIR ${NDS_STUB_ROMFS_DIR}/shaders)
    set(NDS_STUB_GENERATED_SHADER_DIR ${CMAKE_BINARY_DIR}/nds_stub_generated_shaders)
    set(ARCDELTA_SWITCH_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ArcDelta_melonDS/src/frontend/switch)
    set(ARCDELTA_CORE_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ArcDelta_melonDS/src/shaders)
    file(MAKE_DIRECTORY ${NDS_STUB_SHADER_DIR})
    file(MAKE_DIRECTORY ${NDS_STUB_GENERATED_SHADER_DIR})

    set(NDS_STUB_DEKO_SHADERS)
    function(nds_stub_compile_shader stage source_no_ext output_name)
        set(out_file ${NDS_STUB_SHADER_DIR}/${output_name}.dksh)
        add_custom_command(OUTPUT ${out_file}
            COMMAND ${NDS_STUB_UAM_EXE} -s ${stage} -o ${out_file} ${source_no_ext}.glsl
            DEPENDS ${source_no_ext}.glsl
        )
        list(APPEND NDS_STUB_DEKO_SHADERS ${out_file})
        set(NDS_STUB_DEKO_SHADERS ${NDS_STUB_DEKO_SHADERS} PARENT_SCOPE)
    endfunction()

    function(nds_stub_compile_shader_instance stage source_no_ext output_name defines)
        foreach(var IN LISTS defines)
            set(${var} 1)
        endforeach()
        set(generated_file ${NDS_STUB_GENERATED_SHADER_DIR}/${output_name}.glsl)
        configure_file(${source_no_ext}.glsl ${generated_file})
        set(out_file ${NDS_STUB_SHADER_DIR}/${output_name}.dksh)
        add_custom_command(OUTPUT ${out_file}
            COMMAND ${NDS_STUB_UAM_EXE} -s ${stage} -o ${out_file} ${generated_file}
            DEPENDS ${generated_file}
        )
        list(APPEND NDS_STUB_DEKO_SHADERS ${out_file})
        set(NDS_STUB_DEKO_SHADERS ${NDS_STUB_DEKO_SHADERS} PARENT_SCOPE)
    endfunction()

    nds_stub_compile_shader(vert ${ARCDELTA_SWITCH_SHADER_DIR}/Default_vsh Default_vsh)
    nds_stub_compile_shader(frag ${ARCDELTA_SWITCH_SHADER_DIR}/Default_fsh Default_fsh)
    nds_stub_compile_shader(vert ${ARCDELTA_CORE_SHADER_DIR}/FullscreenQuad_vsh FullscreenQuad_vsh)
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/ComposeBGOBJ_fsh ComposeBGOBJ_fsh "ComposeBGOBJ")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/ComposeBGOBJ_fsh ComposeBGOBJDirectBitmapOnly_fsh "ShowBitmap")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/ComposeBGOBJ_fsh ComposeBGOBJShowBitmap_fsh "ShowBitmap;ComposeBGOBJ")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGText_fsh BGText4bpp_fsh "Text4bpp")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGText_fsh BGText4bppMosaic_fsh "Text4bpp;Mosaic")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGText_fsh BGText8bpp_fsh "Text8bpp")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGText_fsh BGText8bppMosaic_fsh "Text8bpp;Mosaic")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGAffine_fsh BGAffine_fsh "Affine")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGAffine_fsh BGAffineMosaic_fsh "Affine;Mosaic")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGAffine_fsh BGExtendedBitmap8bpp_fsh "ExtendedBitmap8bpp")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGAffine_fsh BGExtendedBitmap8bppMosaic_fsh "ExtendedBitmap8bpp;Mosaic")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGAffine_fsh BGExtendedBitmapDirect_fsh "ExtendedBitmapDirectColor")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGAffine_fsh BGExtendedBitmapDirectMosaic_fsh "ExtendedBitmapDirectColor;Mosaic")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGAffine_fsh BGExtendedMixed_fsh "ExtendedMixed")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/BGAffine_fsh BGExtendedMixedMosaic_fsh "ExtendedMixed;Mosaic")
    nds_stub_compile_shader_instance(vert ${ARCDELTA_CORE_SHADER_DIR}/OBJ_vsh OBJRegular_vsh "OBJRegular")
    nds_stub_compile_shader_instance(vert ${ARCDELTA_CORE_SHADER_DIR}/OBJ_vsh OBJAffine_vsh "OBJAffine")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/OBJ_fsh OBJ4bpp_fsh "OBJ4bpp")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/OBJ_fsh OBJ8bpp_fsh "OBJ8bpp")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/OBJ_fsh OBJBitmap_fsh "OBJBitmap")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/OBJ_fsh OBJWindow4bpp_fsh "OBJ4bpp;OBJWindow")
    nds_stub_compile_shader_instance(frag ${ARCDELTA_CORE_SHADER_DIR}/OBJ_fsh OBJWindow8bpp_fsh "OBJ8bpp;OBJWindow")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp InterpXSpansZBuffer "InterpSpans;ZBuffer")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp InterpXSpansWBuffer "InterpSpans;WBuffer")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp BinCombined "BinCombined")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp DepthBlendWBuffer "DepthBlend;WBuffer")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp DepthBlendZBuffer "DepthBlend;ZBuffer")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseNoTextureWBuffer "Rasterise;WBuffer;NoTexture")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseNoTextureWBufferToon "Rasterise;WBuffer;NoTexture;Toon")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseNoTextureWBufferHighlight "Rasterise;WBuffer;NoTexture;Highlight")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseUseTextureDecalWBuffer "Rasterise;WBuffer;UseTexture;Decal")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseUseTextureModulateWBuffer "Rasterise;WBuffer;UseTexture;Modulate")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseUseTextureToonWBuffer "Rasterise;WBuffer;UseTexture;Toon")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseUseTextureHighlightWBuffer "Rasterise;WBuffer;UseTexture;Highlight")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseShadowMaskWBuffer "Rasterise;WBuffer;ShadowMask")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseNoTextureZBuffer "Rasterise;ZBuffer;NoTexture")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseNoTextureZBufferToon "Rasterise;ZBuffer;NoTexture;Toon")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseNoTextureZBufferHighlight "Rasterise;ZBuffer;NoTexture;Highlight")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseUseTextureDecalZBuffer "Rasterise;ZBuffer;UseTexture;Decal")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseUseTextureModulateZBuffer "Rasterise;ZBuffer;UseTexture;Modulate")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseUseTextureToonZBuffer "Rasterise;ZBuffer;UseTexture;Toon")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseUseTextureHighlightZBuffer "Rasterise;ZBuffer;UseTexture;Highlight")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp RasteriseShadowMaskZBuffer "Rasterise;ZBuffer;ShadowMask")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp FinalPass "FinalPass")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp FinalPassEdge "FinalPass;EdgeMarking")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp FinalPassFog "FinalPass;Fog")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp FinalPassEdgeFog "FinalPass;EdgeMarking;Fog")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp FinalPassAA "FinalPass;AntiAliasing")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp FinalPassEdgeAA "FinalPass;EdgeMarking;AntiAliasing")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp FinalPassFogAA "FinalPass;Fog;AntiAliasing")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp FinalPassEdgeFogAA "FinalPass;EdgeMarking;Fog;AntiAliasing")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp ClearCoarseBinMask "ClearCoarseBinMask")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp ClearIndirectWorkCount "ClearIndirectWorkCount")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp CalculateWorkOffsets "CalculateWorkOffsets")
    nds_stub_compile_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp SortWork "SortWork")

    add_custom_target(GBAStationNDSStubDekoShaders DEPENDS ${NDS_STUB_DEKO_SHADERS})

    add_executable(GBAStationNDSStub
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/switch/nds_stub/NdsDekoStubMain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/switch/nds_stub/NdsDekoRuntime.cpp
    )
    set_target_properties(GBAStationNDSStub PROPERTIES CXX_STANDARD 17)
    target_include_directories(GBAStationNDSStub PRIVATE
        ${BOREALIS_LIBRARY}/include/borealis/extern
    )
    target_compile_options(GBAStationNDSStub PRIVATE -ffunction-sections -fdata-sections ${APP_PLATFORM_OPTION})
    target_link_libraries(GBAStationNDSStub PRIVATE nx deko3d arcdelta_melonds_core arcdelta_switch_gfx)
    add_dependencies(GBAStationNDSStub GBAStationNDSStubDekoShaders)
    add_custom_target(GBAStationNDSStub.nro ALL DEPENDS GBAStationNDSStub
        COMMAND ${NX_NACPTOOL_EXE} --create "GBAStation NDS Stub" "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}" GBAStationNDSStub.nacp
        COMMAND ${NX_ELF2NRO_EXE} $<TARGET_FILE:GBAStationNDSStub> GBAStationNDSStub.nro --icon=${PROJECT_ICON} --nacp=GBAStationNDSStub.nacp --romfsdir=${NDS_STUB_ROMFS_DIR}
    )
    endif()
endif ()



target_include_directories(${PROJECT_NAME} PRIVATE ${APP_PLATFORM_INCLUDE})
target_compile_options(${PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections ${APP_PLATFORM_OPTION})
target_link_libraries(${PROJECT_NAME} PRIVATE borealis ${APP_PLATFORM_LIB})
if(NOT PLATFORM_ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE curl)
endif()

# ============================================================
# 静态链接模拟器核心
#
# FCEUmm        – NES / Famicom       (libretro, fceumm_retro_*)
# Snes9x        – SNES / SFC          (libretro, snes9x_retro_*)
# Nestopia      – NES / Famicom       (libretro, nestopia_retro_*)
# Snes9x 2005   – SNES / SFC          (libretro, snes9x2005_retro_*)
# NDS runs in GBAStationNDSStub.nro, launched as a separate Switch NRO.
# mGBA runs in GBAStationMgbaStub.nro, launched as a separate Switch NRO.
#
# 主程序 GameView 不再运行 mGBA native/libretro。
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
if (TARGET snes9x2005_core)
    target_link_libraries(${PROJECT_NAME} PRIVATE snes9x2005_core)
    message(STATUS "Linking Snes9x 2005 (SNES) core statically")
endif()
