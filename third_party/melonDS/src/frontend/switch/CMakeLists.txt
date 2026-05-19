project(switch)

SET(SOURCES_SWITCH
    main.cpp
    Platform.cpp
    PlatformConfig.cpp
    Gfx.cpp
    BoxGui.cpp
    BackButton.cpp
    Filebrowser.cpp
    KeyExplanations.cpp
    ROMMetaDatabase.cpp
    SettingsDialog.cpp
    StartMenu.cpp
    FaultHandler.cpp
    ErrorDialog.cpp
    GpuMemHeap.cpp
    UploadBuffer.cpp
    RetroAchievements.cpp
    NotificationSystem.cpp
    InputConfig.cpp
    TriggerNotification.cpp
    RATracker.cpp
    ../Util_ROM.cpp
    ../Util_Audio.cpp
    ../Util_Video.cpp

    mm_vec/mm_vec.c
    stb_truetype/stb_truetype.c
    stb_image/stb_image.c

    ../../../external/rcheevos/src/rc_util.c
    ../../../external/rcheevos/src/rc_client_raintegration.c
    ../../../external/rcheevos/src/rc_client.c
    ../../../external/rcheevos/src/rc_compat.c
    ../../../external/rcheevos/src/rc_compat.h
    ../../../external/rcheevos/src/rc_version.c
    ../../../external/rcheevos/src/rapi/rc_api_common.c
    ../../../external/rcheevos/src/rapi/rc_api_editor.c
    ../../../external/rcheevos/src/rapi/rc_api_info.c
    ../../../external/rcheevos/src/rapi/rc_api_runtime.c
    ../../../external/rcheevos/src/rapi/rc_api_user.c
    ../../../external/rcheevos/src/rcheevos/alloc.c
    ../../../external/rcheevos/src/rcheevos/condition.c
    ../../../external/rcheevos/src/rcheevos/condset.c
    ../../../external/rcheevos/src/rcheevos/consoleinfo.c
    ../../../external/rcheevos/src/rcheevos/format.c
    ../../../external/rcheevos/src/rcheevos/lboard.c
    ../../../external/rcheevos/src/rcheevos/memref.c
    ../../../external/rcheevos/src/rcheevos/operand.c
    ../../../external/rcheevos/src/rcheevos/rc_validate.c
    ../../../external/rcheevos/src/rcheevos/richpresence.c
    ../../../external/rcheevos/src/rcheevos/runtime_progress.c
    ../../../external/rcheevos/src/rcheevos/runtime.c
    ../../../external/rcheevos/src/rcheevos/trigger.c
    ../../../external/rcheevos/src/rcheevos/value.c
    ../../../external/rcheevos/src/rhash/cdreader.c
    ../../../external/rcheevos/src/rhash/hash_encrypted.c
    ../../../external/rcheevos/src/rhash/hash_rom.c
    ../../../external/rcheevos/src/rhash/hash_zip.c
    ../../../external/rcheevos/src/rhash/hash.c
    ../../../external/rcheevos/src/rhash/md5.c
    ../../../external/rcheevos/src/rurl/url.c
)

include(${CMAKE_SOURCE_DIR}/cmake/Toolchain-cross-Switch.cmake)

add_executable(melonDS.elf ${SOURCES_SWITCH})
target_compile_definitions(melonDS.elf PRIVATE RC_CLIENT_SUPPORTS_HASH)
if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    if (ENABLE_LTO)
        target_link_libraries(melonDS.elf nxd deko3dd core nx)
    else()
        target_link_libraries(melonDS.elf core deko3dd nxd)
    endif()
else()
    if (ENABLE_LTO)
        target_link_libraries(melonDS.elf core nx deko3d)
    else()
        target_link_libraries(melonDS.elf core deko3d nx)
    endif()
endif()

target_link_libraries(melonDS.elf curl z)

target_include_directories(melonDS.elf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(melonDS.elf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_include_directories(melonDS.elf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_include_directories(melonDS.elf PUBLIC "${CMAKE_SOURCE_DIR}/external/rcheevos/include")

file(READ "version.h" VERSION_HEADER)
string(REGEX MATCH "PROJECT_VERSION \"([0-9.]+)\"" _ ${VERSION_HEADER})
set(PROJECT_VERSION ${CMAKE_MATCH_1})

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/control.nacp
                   COMMAND ${NACPTOOL} --create "melonDS Switch" "Arisotura, RSDuck, Hydr8gon, Jpe230, Gheovgos" "release ${PROJECT_VERSION}" ${CMAKE_BINARY_DIR}/control.nacp)

make_directory(${CMAKE_BINARY_DIR}/romfs/shaders)
configure_file(${CMAKE_SOURCE_DIR}/icon/melon_128x128.png ${CMAKE_BINARY_DIR}/romfs/melon_128x128.png COPYONLY)

compile_shader(vert Default_vsh)
compile_shader(frag Default_fsh)

compile_shader(vert ../../shaders/FullscreenQuad_vsh)
compile_shader_instance(frag ../../shaders/ComposeBGOBJ_fsh ComposeBGOBJ_fsh "ComposeBGOBJ")
compile_shader_instance(frag ../../shaders/ComposeBGOBJ_fsh ComposeBGOBJDirectBitmapOnly_fsh "ShowBitmap")
compile_shader_instance(frag ../../shaders/ComposeBGOBJ_fsh ComposeBGOBJShowBitmap_fsh "ShowBitmap;ComposeBGOBJ")
compile_shader_instance(frag ../../shaders/BGText_fsh BGText4bpp_fsh "Text4bpp")
compile_shader_instance(frag ../../shaders/BGText_fsh BGText4bppMosaic_fsh "Text4bpp;Mosaic")
compile_shader_instance(frag ../../shaders/BGText_fsh BGText8bpp_fsh "Text8bpp")
compile_shader_instance(frag ../../shaders/BGText_fsh BGText8bppMosaic_fsh "Text8bpp;Mosaic")
compile_shader_instance(frag ../../shaders/BGAffine_fsh BGAffine_fsh "Affine")
compile_shader_instance(frag ../../shaders/BGAffine_fsh BGAffineMosaic_fsh "Affine;Mosaic")
compile_shader_instance(frag ../../shaders/BGAffine_fsh BGExtendedBitmap8bpp_fsh "ExtendedBitmap8bpp")
compile_shader_instance(frag ../../shaders/BGAffine_fsh BGExtendedBitmap8bppMosaic_fsh "ExtendedBitmap8bpp;Mosaic")
compile_shader_instance(frag ../../shaders/BGAffine_fsh BGExtendedBitmapDirect_fsh "ExtendedBitmapDirectColor")
compile_shader_instance(frag ../../shaders/BGAffine_fsh BGExtendedBitmapDirectMosaic_fsh "ExtendedBitmapDirectColor;Mosaic")
compile_shader_instance(frag ../../shaders/BGAffine_fsh BGExtendedMixed_fsh "ExtendedMixed")
compile_shader_instance(frag ../../shaders/BGAffine_fsh BGExtendedMixedMosaic_fsh "ExtendedMixed;Mosaic")
compile_shader_instance(vert ../../shaders/OBJ_vsh OBJRegular_vsh "OBJRegular")
compile_shader_instance(vert ../../shaders/OBJ_vsh OBJAffine_vsh "OBJAffine")
compile_shader_instance(frag ../../shaders/OBJ_fsh OBJ4bpp_fsh "OBJ4bpp")
compile_shader_instance(frag ../../shaders/OBJ_fsh OBJ8bpp_fsh "OBJ8bpp")
compile_shader_instance(frag ../../shaders/OBJ_fsh OBJBitmap_fsh "OBJBitmap")
compile_shader_instance(frag ../../shaders/OBJ_fsh OBJWindow4bpp_fsh "OBJ4bpp;OBJWindow")
compile_shader_instance(frag ../../shaders/OBJ_fsh OBJWindow8bpp_fsh "OBJ8bpp;OBJWindow")
compile_shader_instance(comp ../../shaders/GPU3D_Comp InterpXSpansZBuffer "InterpSpans;ZBuffer")
compile_shader_instance(comp ../../shaders/GPU3D_Comp InterpXSpansWBuffer "InterpSpans;WBuffer")
compile_shader_instance(comp ../../shaders/GPU3D_Comp BinCombined "BinCombined")
compile_shader_instance(comp ../../shaders/GPU3D_Comp DepthBlendWBuffer "DepthBlend;WBuffer")
compile_shader_instance(comp ../../shaders/GPU3D_Comp DepthBlendZBuffer "DepthBlend;ZBuffer")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseNoTextureWBuffer "Rasterise;WBuffer;NoTexture")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseNoTextureWBufferToon "Rasterise;WBuffer;NoTexture;Toon")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseNoTextureWBufferHighlight "Rasterise;WBuffer;NoTexture;Highlight")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseUseTextureDecalWBuffer "Rasterise;WBuffer;UseTexture;Decal")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseUseTextureModulateWBuffer "Rasterise;WBuffer;UseTexture;Modulate")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseUseTextureToonWBuffer "Rasterise;WBuffer;UseTexture;Toon")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseUseTextureHighlightWBuffer "Rasterise;WBuffer;UseTexture;Highlight")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseShadowMaskWBuffer "Rasterise;WBuffer;ShadowMask")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseNoTextureZBuffer "Rasterise;ZBuffer;NoTexture")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseNoTextureZBufferToon "Rasterise;ZBuffer;NoTexture;Toon")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseNoTextureZBufferHighlight "Rasterise;ZBuffer;NoTexture;Highlight")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseUseTextureDecalZBuffer "Rasterise;ZBuffer;UseTexture;Decal")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseUseTextureModulateZBuffer "Rasterise;ZBuffer;UseTexture;Modulate")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseUseTextureToonZBuffer "Rasterise;ZBuffer;UseTexture;Toon")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseUseTextureHighlightZBuffer "Rasterise;ZBuffer;UseTexture;Highlight")
compile_shader_instance(comp ../../shaders/GPU3D_Comp RasteriseShadowMaskZBuffer "Rasterise;ZBuffer;ShadowMask")
compile_shader_instance(comp ../../shaders/GPU3D_Comp FinalPass "FinalPass")
compile_shader_instance(comp ../../shaders/GPU3D_Comp FinalPassEdge "FinalPass;EdgeMarking")
compile_shader_instance(comp ../../shaders/GPU3D_Comp FinalPassFog "FinalPass;Fog")
compile_shader_instance(comp ../../shaders/GPU3D_Comp FinalPassEdgeFog "FinalPass;EdgeMarking;Fog")
compile_shader_instance(comp ../../shaders/GPU3D_Comp FinalPassAA "FinalPass;AntiAliasing")
compile_shader_instance(comp ../../shaders/GPU3D_Comp FinalPassEdgeAA "FinalPass;EdgeMarking;AntiAliasing")
compile_shader_instance(comp ../../shaders/GPU3D_Comp FinalPassFogAA "FinalPass;Fog;AntiAliasing")
compile_shader_instance(comp ../../shaders/GPU3D_Comp FinalPassEdgeFogAA "FinalPass;EdgeMarking;Fog;AntiAliasing")
compile_shader_instance(comp ../../shaders/GPU3D_Comp ClearCoarseBinMask "ClearCoarseBinMask")
compile_shader_instance(comp ../../shaders/GPU3D_Comp ClearIndirectWorkCount "ClearIndirectWorkCount")
compile_shader_instance(comp ../../shaders/GPU3D_Comp CalculateWorkOffsets "CalculateWorkOffsets")
compile_shader_instance(comp ../../shaders/GPU3D_Comp SortWork "SortWork")

message("${shader_paths}")
add_custom_target(melonDS.nro ALL
    ${ELF2NRO} ${CMAKE_BINARY_DIR}/melonDS.elf ${CMAKE_BINARY_DIR}/melonDS.nro --icon=${CMAKE_SOURCE_DIR}/icon/melon_switch.jpg --nacp=${CMAKE_BINARY_DIR}/control.nacp --romfsdir=${CMAKE_BINARY_DIR}/romfs
    DEPENDS melonDS.elf
    ${CMAKE_BINARY_DIR}/control.nacp
    "${shader_paths}")