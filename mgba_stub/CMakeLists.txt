set(MGBA_STUB_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MGBA_STUB_REPO_ROOT ${CMAKE_SOURCE_DIR})
if (NOT DEFINED NDS_STUB_ROMFS_DIR)
    set(NDS_STUB_ROMFS_DIR ${CMAKE_BINARY_DIR}/nds_stub_romfs)
endif()
set(MGBA_STUB_ROMFS_DIR ${NDS_STUB_ROMFS_DIR})

add_executable(GBAStationMgbaStub
    ${MGBA_STUB_DIR}/src/MgbaStubMain.cpp
    ${MGBA_STUB_DIR}/src/MgbaStubRuntime.cpp
    ${MGBA_STUB_DIR}/src/MgbaStb.cpp
    ${MGBA_STUB_DIR}/src/MgbaShaderCatalog.cpp
    ${MGBA_STUB_DIR}/src/MgbaUiAudio.cpp
    ${MGBA_STUB_DIR}/src/MgbaMenuLayer.cpp
    ${MGBA_STUB_DIR}/src/ui/UiPrimitives.cpp
    ${MGBA_STUB_DIR}/src/ui/UiComponents.cpp
)

set_target_properties(GBAStationMgbaStub PROPERTIES CXX_STANDARD 17)
target_include_directories(GBAStationMgbaStub PRIVATE
    ${MGBA_STUB_DIR}/include
    ${MGBA_STUB_REPO_ROOT}/third_party/borealis/library/include/borealis/extern
    ${MGBA_STUB_REPO_ROOT}/src
    ${MGBA_STUB_REPO_ROOT}/third_party/mgba/include
    ${CMAKE_BINARY_DIR}/third_party/mgba/include
)
target_compile_definitions(GBAStationMgbaStub PRIVATE COLOR_16_BIT COLOR_5_6_5)
target_compile_options(GBAStationMgbaStub PRIVATE -ffunction-sections -fdata-sections ${APP_PLATFORM_OPTION})
target_link_libraries(GBAStationMgbaStub PRIVATE nx deko3d mgba arcdelta_switch_gfx)
# Reuse only the generated Deko/Gfx RomFS assets. The mGBA menu code is local
# to mgba_stub and does not link NdsMenuLayer/UiComponents.
add_dependencies(GBAStationMgbaStub GBAStationNDSStubDekoShaders)

add_custom_target(GBAStationMgbaStub.nro ALL DEPENDS GBAStationMgbaStub
    COMMAND ${NX_NACPTOOL_EXE} --create "GBAStation mGBA Stub" "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}" ${CMAKE_BINARY_DIR}/GBAStationMgbaStub.nacp
    COMMAND ${NX_ELF2NRO_EXE} $<TARGET_FILE:GBAStationMgbaStub> ${CMAKE_BINARY_DIR}/GBAStationMgbaStub.nro --icon=${PROJECT_ICON} --nacp=${CMAKE_BINARY_DIR}/GBAStationMgbaStub.nacp --romfsdir=${MGBA_STUB_ROMFS_DIR}
)
