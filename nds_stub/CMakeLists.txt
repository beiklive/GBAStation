set(NDS_STUB_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(NDS_STUB_REPO_ROOT ${CMAKE_SOURCE_DIR})

find_program(NDS_STUB_UAM_EXE NAMES uam HINTS "$ENV{DEVKITPRO}/tools/bin")
if (NOT NDS_STUB_UAM_EXE)
    message(FATAL_ERROR "Could not find uam for GBAStationNDSStub Deko shaders")
endif()

set(NDS_STUB_ROMFS_DIR ${CMAKE_BINARY_DIR}/nds_stub_romfs)
set(NDS_STUB_SHADER_DIR ${NDS_STUB_ROMFS_DIR}/shaders)
set(NDS_STUB_CONFIG_DIR ${NDS_STUB_ROMFS_DIR}/config)
set(NDS_STUB_MATERIAL_DIR ${NDS_STUB_ROMFS_DIR}/material)
set(NDS_STUB_IMAGE_UI_DIR ${NDS_STUB_ROMFS_DIR}/img/ui)
set(NDS_STUB_SOUND_DIR ${NDS_STUB_ROMFS_DIR}/sounds/switch)
set(NDS_STUB_LANG_DIR ${NDS_STUB_ROMFS_DIR}/lang)
set(NDS_STUB_GENERATED_SHADER_DIR ${CMAKE_BINARY_DIR}/nds_stub_generated_shaders)
set(NDS_STUB_SHADER_SOURCE_DIR ${NDS_STUB_DIR}/src/shaders)
set(ARCDELTA_SWITCH_SHADER_DIR ${NDS_STUB_REPO_ROOT}/third_party/ArcDelta_melonDS/src/frontend/switch)
set(ARCDELTA_CORE_SHADER_DIR ${NDS_STUB_REPO_ROOT}/third_party/ArcDelta_melonDS/src/shaders)

file(MAKE_DIRECTORY ${NDS_STUB_SHADER_DIR})
file(MAKE_DIRECTORY ${NDS_STUB_CONFIG_DIR})
file(MAKE_DIRECTORY ${NDS_STUB_MATERIAL_DIR})
file(MAKE_DIRECTORY ${NDS_STUB_IMAGE_UI_DIR})
file(MAKE_DIRECTORY ${NDS_STUB_SOUND_DIR})
file(MAKE_DIRECTORY ${NDS_STUB_LANG_DIR})
file(MAKE_DIRECTORY ${NDS_STUB_GENERATED_SHADER_DIR})
file(GLOB NDS_STUB_LANG_FILES ${NDS_STUB_REPO_ROOT}/nds_stub/resources/lang/*.json)
foreach(lang_file IN LISTS NDS_STUB_LANG_FILES)
    get_filename_component(lang_name ${lang_file} NAME)
    configure_file(${lang_file} ${NDS_STUB_LANG_DIR}/${lang_name} COPYONLY)
endforeach()
configure_file(${NDS_STUB_REPO_ROOT}/resources/config/nds_shaders.json
               ${NDS_STUB_CONFIG_DIR}/nds_shaders.json COPYONLY)
configure_file(${NDS_STUB_REPO_ROOT}/resources/material/MaterialIcons-Regular.ttf
               ${NDS_STUB_MATERIAL_DIR}/MaterialIcons-Regular.ttf COPYONLY)
configure_file(${NDS_STUB_REPO_ROOT}/resources/img/ui/border_gradient.png
               ${NDS_STUB_IMAGE_UI_DIR}/border_gradient.png COPYONLY)
file(GLOB NDS_STUB_SOUND_FILES ${NDS_STUB_REPO_ROOT}/resources/sounds/switch/*.wav)
foreach(sound_file IN LISTS NDS_STUB_SOUND_FILES)
    get_filename_component(sound_name ${sound_file} NAME)
    configure_file(${sound_file} ${NDS_STUB_SOUND_DIR}/${sound_name} COPYONLY)
endforeach()
file(REMOVE_RECURSE ${NDS_STUB_ROMFS_DIR}/img/ui/menu)
file(GLOB NDS_STUB_STALE_SCALED_SHADER_BINARIES
    ${NDS_STUB_SHADER_DIR}/*_x1.dksh
    ${NDS_STUB_SHADER_DIR}/*_x2.dksh
    ${NDS_STUB_SHADER_DIR}/*_x3.dksh
    ${NDS_STUB_SHADER_DIR}/*_x4.dksh
)
file(GLOB NDS_STUB_STALE_SCALED_SHADER_SOURCES
    ${NDS_STUB_GENERATED_SHADER_DIR}/*_x1.glsl
    ${NDS_STUB_GENERATED_SHADER_DIR}/*_x2.glsl
    ${NDS_STUB_GENERATED_SHADER_DIR}/*_x3.glsl
    ${NDS_STUB_GENERATED_SHADER_DIR}/*_x4.glsl
)
if (NDS_STUB_STALE_SCALED_SHADER_BINARIES OR NDS_STUB_STALE_SCALED_SHADER_SOURCES)
    file(REMOVE ${NDS_STUB_STALE_SCALED_SHADER_BINARIES} ${NDS_STUB_STALE_SCALED_SHADER_SOURCES})
endif()

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

function(nds_stub_compile_3d_shader_instance stage source_no_ext output_name defines scale)
    foreach(var IN LISTS defines)
        set(${var} 1)
    endforeach()
    set(NDS_3D_SCALE ${scale})
    set(generated_file ${NDS_STUB_GENERATED_SHADER_DIR}/${output_name}_x${scale}.glsl)
    configure_file(${source_no_ext}.glsl ${generated_file})
    set(out_file ${NDS_STUB_SHADER_DIR}/${output_name}_x${scale}.dksh)
    add_custom_command(OUTPUT ${out_file}
        COMMAND ${NDS_STUB_UAM_EXE} -s ${stage} -o ${out_file} ${generated_file}
        DEPENDS ${generated_file}
    )
    list(APPEND NDS_STUB_DEKO_SHADERS ${out_file})
    set(NDS_STUB_DEKO_SHADERS ${NDS_STUB_DEKO_SHADERS} PARENT_SCOPE)
endfunction()

nds_stub_compile_shader(vert ${ARCDELTA_SWITCH_SHADER_DIR}/Default_vsh Default_vsh)
nds_stub_compile_shader(frag ${ARCDELTA_SWITCH_SHADER_DIR}/Default_fsh Default_fsh)
nds_stub_compile_shader(frag ${NDS_STUB_SHADER_SOURCE_DIR}/NdsDot_fsh NdsDot_fsh)
nds_stub_compile_shader(frag ${NDS_STUB_SHADER_SOURCE_DIR}/NdsDotClear_fsh NdsDotClear_fsh)
nds_stub_compile_shader(frag ${NDS_STUB_SHADER_SOURCE_DIR}/NdsXbrzFreescale_fsh NdsXbrzFreescale_fsh)
nds_stub_compile_shader(frag ${NDS_STUB_SHADER_SOURCE_DIR}/NdsLcdGridNdsColor_fsh NdsLcdGridNdsColor_fsh)
nds_stub_compile_shader(frag ${NDS_STUB_SHADER_SOURCE_DIR}/NdsDrasticSimple_fsh NdsDrasticSimple_fsh)
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
set(NDS_STUB_GPU3D_SHADER_INSTANCES
    "InterpXSpansZBuffer|InterpSpans,ZBuffer"
    "InterpXSpansWBuffer|InterpSpans,WBuffer"
    "BinCombined|BinCombined"
    "DepthBlendWBuffer|DepthBlend,WBuffer"
    "DepthBlendZBuffer|DepthBlend,ZBuffer"
    "RasteriseNoTextureWBuffer|Rasterise,WBuffer,NoTexture"
    "RasteriseNoTextureWBufferToon|Rasterise,WBuffer,NoTexture,Toon"
    "RasteriseNoTextureWBufferHighlight|Rasterise,WBuffer,NoTexture,Highlight"
    "RasteriseUseTextureDecalWBuffer|Rasterise,WBuffer,UseTexture,Decal"
    "RasteriseUseTextureModulateWBuffer|Rasterise,WBuffer,UseTexture,Modulate"
    "RasteriseUseTextureToonWBuffer|Rasterise,WBuffer,UseTexture,Toon"
    "RasteriseUseTextureHighlightWBuffer|Rasterise,WBuffer,UseTexture,Highlight"
    "RasteriseShadowMaskWBuffer|Rasterise,WBuffer,ShadowMask"
    "RasteriseNoTextureZBuffer|Rasterise,ZBuffer,NoTexture"
    "RasteriseNoTextureZBufferToon|Rasterise,ZBuffer,NoTexture,Toon"
    "RasteriseNoTextureZBufferHighlight|Rasterise,ZBuffer,NoTexture,Highlight"
    "RasteriseUseTextureDecalZBuffer|Rasterise,ZBuffer,UseTexture,Decal"
    "RasteriseUseTextureModulateZBuffer|Rasterise,ZBuffer,UseTexture,Modulate"
    "RasteriseUseTextureToonZBuffer|Rasterise,ZBuffer,UseTexture,Toon"
    "RasteriseUseTextureHighlightZBuffer|Rasterise,ZBuffer,UseTexture,Highlight"
    "RasteriseShadowMaskZBuffer|Rasterise,ZBuffer,ShadowMask"
    "FinalPass|FinalPass"
    "FinalPassEdge|FinalPass,EdgeMarking"
    "FinalPassFog|FinalPass,Fog"
    "FinalPassEdgeFog|FinalPass,EdgeMarking,Fog"
    "FinalPassAA|FinalPass,AntiAliasing"
    "FinalPassEdgeAA|FinalPass,EdgeMarking,AntiAliasing"
    "FinalPassFogAA|FinalPass,Fog,AntiAliasing"
    "FinalPassEdgeFogAA|FinalPass,EdgeMarking,Fog,AntiAliasing"
    "ClearCoarseBinMask|ClearCoarseBinMask"
    "ClearIndirectWorkCount|ClearIndirectWorkCount"
    "CalculateWorkOffsets|CalculateWorkOffsets"
    "SortWork|SortWork"
)
foreach(scale RANGE 1 4)
    foreach(instance IN LISTS NDS_STUB_GPU3D_SHADER_INSTANCES)
        string(REPLACE "|" ";" instance_parts "${instance}")
        list(GET instance_parts 0 output_name)
        list(REMOVE_AT instance_parts 0)
        string(REPLACE "," ";" instance_defines "${instance_parts}")
        nds_stub_compile_3d_shader_instance(comp ${ARCDELTA_CORE_SHADER_DIR}/GPU3D_Comp ${output_name} "${instance_defines}" ${scale})
    endforeach()
endforeach()

add_custom_target(GBAStationNDSStubDekoShaders DEPENDS ${NDS_STUB_DEKO_SHADERS})

add_executable(GBAStationNDSStub
    ${NDS_STUB_DIR}/src/NdsDekoStubMain.cpp
    ${NDS_STUB_DIR}/src/NdsDekoRuntime.cpp
    ${NDS_STUB_DIR}/src/NdsStb.cpp
    ${NDS_STUB_DIR}/src/NdsCheatDatabase.cpp
    ${NDS_STUB_DIR}/src/NdsShaderCatalog.cpp
    ${NDS_STUB_DIR}/src/NdsUiAudio.cpp
    ${NDS_STUB_DIR}/src/NdsGameLayer.cpp
    ${NDS_STUB_DIR}/src/NdsMenuLayer.cpp
    ${NDS_STUB_DIR}/src/ui/UiPrimitives.cpp
    ${NDS_STUB_DIR}/src/ui/UiComponents.cpp
)
set_target_properties(GBAStationNDSStub PROPERTIES CXX_STANDARD 17)
target_include_directories(GBAStationNDSStub PRIVATE
    ${NDS_STUB_DIR}/include
    ${NDS_STUB_REPO_ROOT}/third_party/borealis/library/include/borealis/extern
)
target_compile_options(GBAStationNDSStub PRIVATE
    -ffunction-sections
    -fdata-sections
    -flto
    -fPIC
    ${APP_PLATFORM_OPTION}
)
target_link_options(GBAStationNDSStub PRIVATE -flto -fuse-linker-plugin -pie)
target_link_libraries(GBAStationNDSStub PRIVATE nx deko3d arcdelta_melonds_core arcdelta_switch_gfx)
add_dependencies(GBAStationNDSStub GBAStationNDSStubDekoShaders)

add_custom_target(GBAStationNDSStub.nro ALL DEPENDS GBAStationNDSStub
    COMMAND ${NX_NACPTOOL_EXE} --create "GBAStation NDS Stub" "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}" ${CMAKE_BINARY_DIR}/GBAStationNDSStub.nacp
    COMMAND ${NX_ELF2NRO_EXE} $<TARGET_FILE:GBAStationNDSStub> ${CMAKE_BINARY_DIR}/GBAStationNDSStub.nro --icon=${PROJECT_ICON} --nacp=${CMAKE_BINARY_DIR}/GBAStationNDSStub.nacp --romfsdir=${NDS_STUB_ROMFS_DIR}
)
