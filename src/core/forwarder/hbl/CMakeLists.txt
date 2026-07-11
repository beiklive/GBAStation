project(gbastation_forwarder_hbl LANGUAGES ASM C)

add_executable(gbastation_forwarder_hbl
    source/main.c
    source/trampoline.s
)

set_target_properties(gbastation_forwarder_hbl PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS ON
)

target_link_options(gbastation_forwarder_hbl PRIVATE -Wl,-wrap,exit)
target_compile_definitions(gbastation_forwarder_hbl PRIVATE VERSION="GBAStation")

find_program(NX_ELF2NSO_EXE NAMES elf2nso HINTS "${DEVKITPRO}/tools/bin" REQUIRED)
find_program(NX_NPDMTOOL_EXE NAMES npdmtool HINTS "${DEVKITPRO}/tools/bin" REQUIRED)

set(FORWARDER_EXEFS_DIR "${CMAKE_CURRENT_BINARY_DIR}/exefs")
file(MAKE_DIRECTORY "${FORWARDER_EXEFS_DIR}")

add_custom_command(
    OUTPUT "${FORWARDER_EXEFS_DIR}/main"
    COMMAND ${NX_ELF2NSO_EXE} "$<TARGET_FILE:gbastation_forwarder_hbl>" "${FORWARDER_EXEFS_DIR}/main"
    DEPENDS gbastation_forwarder_hbl
    VERBATIM
)

add_custom_command(
    OUTPUT "${FORWARDER_EXEFS_DIR}/main.npdm"
    COMMAND ${NX_NPDMTOOL_EXE} "${CMAKE_CURRENT_SOURCE_DIR}/hbl.json" "${FORWARDER_EXEFS_DIR}/main.npdm"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/hbl.json"
    VERBATIM
)

add_custom_target(gbastation_forwarder_hbl_exefs
    DEPENDS "${FORWARDER_EXEFS_DIR}/main" "${FORWARDER_EXEFS_DIR}/main.npdm"
)

set(GBASTATION_FORWARDER_EMBED_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
