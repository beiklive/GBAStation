set(NDS_STUB_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(NDS_STUB_REPO_ROOT ${CMAKE_SOURCE_DIR})

find_program(NDS_STUB_UAM_EXE NAMES uam HINTS "$ENV{DEVKITPRO}/tools/bin")
if (NOT NDS_STUB_UAM_EXE)
    message(FATAL_ERROR "Could not find uam for GBAStationNDSStub Deko shaders")
endif()

set(NDS_STUB_ROMFS_DIR ${CMAKE_BINARY_DIR}/nds_stub_romfs)
set(NDS_STUB_SHADER_DIR ${NDS_STUB_ROMFS_DIR}/shaders)
set(NDS_STUB_GENERATED_SHADER_DIR ${CMAKE_BINARY_DIR}/nds_stub_generated_shaders)
set(ARCDELTA_SWITCH_SHADER_DIR ${NDS_STUB_REPO_ROOT}/third_party/ArcDelta_melonDS/src/frontend/switch)
set(ARCDELTA_CORE_SHADER_DIR ${NDS_STUB_REPO_ROOT}/third_party/ArcDelta_melonDS/src/shaders)

file(MAKE_DIRECTORY ${NDS_STUB_SHADER_DIR})
file(MAKE_DIRECTORY ${NDS_STUB_GENERATED_SHADER_DIR})
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
    ${NDS_STUB_DIR}/src/NdsDekoStubMain.cpp
    ${NDS_STUB_DIR}/src/NdsDekoRuntime.cpp
    ${NDS_STUB_DIR}/src/NdsCheatDatabase.cpp
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
target_compile_options(GBAStationNDSStub PRIVATE -ffunction-sections -fdata-sections ${APP_PLATFORM_OPTION})
target_link_libraries(GBAStationNDSStub PRIVATE nx deko3d arcdelta_melonds_core arcdelta_switch_gfx)
add_dependencies(GBAStationNDSStub GBAStationNDSStubDekoShaders)

add_custom_target(GBAStationNDSStub.nro ALL DEPENDS GBAStationNDSStub
    COMMAND ${NX_NACPTOOL_EXE} --create "GBAStation NDS Stub" "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}" ${CMAKE_BINARY_DIR}/GBAStationNDSStub.nacp
    COMMAND ${NX_ELF2NRO_EXE} $<TARGET_FILE:GBAStationNDSStub> ${CMAKE_BINARY_DIR}/GBAStationNDSStub.nro --icon=${PROJECT_ICON} --nacp=${CMAKE_BINARY_DIR}/GBAStationNDSStub.nacp --romfsdir=${NDS_STUB_ROMFS_DIR}
)
