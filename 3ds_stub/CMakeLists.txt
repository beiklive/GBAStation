cmake_minimum_required(VERSION 3.25)

set(THREEDS_STUB_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (PROJECT_NAME STREQUAL "citra" AND GBASTATION_3DS_STUB_IN_TREE)
    set(THREEDS_STUB_AZAHAR_DIR ${CMAKE_SOURCE_DIR})

    add_library(azahar_core INTERFACE)
    target_link_libraries(azahar_core INTERFACE
        citra_core
        video_core
        audio_core
        network
    )

    add_executable(GBAStation3DSStub
        ${THREEDS_STUB_DIR}/src/ThreeDSStubMain.cpp
        ${THREEDS_STUB_DIR}/src/ThreeDSRuntime.cpp
        ${THREEDS_STUB_DIR}/src/ThreeDSRenderer.cpp
        ${THREEDS_STUB_DIR}/src/ThreeDSInput.cpp
        ${THREEDS_STUB_DIR}/src/ThreeDSAudio.cpp
        ${THREEDS_STUB_DIR}/src/ThreeDSLog.cpp
        ${THREEDS_STUB_DIR}/src/ThreeDSPosix.cpp
    )
    set_target_properties(GBAStation3DSStub PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
    target_include_directories(GBAStation3DSStub PRIVATE
        ${THREEDS_STUB_DIR}/include
        ${THREEDS_STUB_AZAHAR_DIR}/src
        $ENV{DEVKITPRO}/portlibs/switch/include
    )
    target_link_directories(GBAStation3DSStub PRIVATE
        $ENV{DEVKITPRO}/portlibs/switch/lib
    )
    target_compile_definitions(GBAStation3DSStub PRIVATE
        __SWITCH__
        SWITCH
        HAVE_LIBNX
        ENABLE_OPENGL
    )
    target_compile_options(GBAStation3DSStub PRIVATE
        -O3
        -ffunction-sections
        -fdata-sections
        -fPIC
    )
    target_link_options(GBAStation3DSStub PRIVATE
        -pie
        -Wl,--gc-sections
    )
    target_link_libraries(GBAStation3DSStub PRIVATE
        azahar_core
        glad
        EGL
        glapi
        drm_nouveau
        nx
    )

    # The Switch build replaces Azahar's desktop sink registry with a libnx audout sink.
    set_source_files_properties(
        ${THREEDS_STUB_AZAHAR_DIR}/src/audio_core/sink_details.cpp
        PROPERTIES HEADER_FILE_ONLY TRUE
    )
    target_sources(audio_core PRIVATE ${THREEDS_STUB_DIR}/src/ThreeDSSinkDetails.cpp)
    target_include_directories(audio_core PRIVATE ${THREEDS_STUB_DIR}/include)
    target_compile_definitions(audio_core PRIVATE
        __SWITCH__
        SWITCH
        HAVE_LIBNX
    )

    if (TARGET crypto_obj)
        target_compile_definitions(crypto_obj PRIVATE SSIZE_MAX=__LONG_MAX__)
        target_compile_options(crypto_obj PRIVATE -include arpa/inet.h)
        target_sources(crypto_obj PRIVATE ${THREEDS_STUB_DIR}/src/libressl_ui_switch.c)
    endif()

    find_program(THREEDS_STUB_NACPTOOL_EXE NAMES nacptool HINTS "$ENV{DEVKITPRO}/tools/bin")
    find_program(THREEDS_STUB_ELF2NRO_EXE NAMES elf2nro HINTS "$ENV{DEVKITPRO}/tools/bin")
    if (NOT THREEDS_STUB_NACPTOOL_EXE OR NOT THREEDS_STUB_ELF2NRO_EXE)
        message(FATAL_ERROR "Could not find nacptool/elf2nro for GBAStation3DSStub")
    endif()

    if (NOT DEFINED GBASTATION_PROJECT_ICON)
        set(GBASTATION_PROJECT_ICON ${THREEDS_STUB_DIR}/../resources/icon/default.png)
    endif()

    add_custom_target(GBAStation3DSStub.nro ALL DEPENDS GBAStation3DSStub
        COMMAND ${THREEDS_STUB_NACPTOOL_EXE}
            --create "GBAStation 3DS Stub" "beiklive" "0.3.0"
            ${CMAKE_BINARY_DIR}/GBAStation3DSStub.nacp
        COMMAND ${THREEDS_STUB_ELF2NRO_EXE}
            $<TARGET_FILE:GBAStation3DSStub>
            ${CMAKE_BINARY_DIR}/GBAStation3DSStub.nro
            --icon=${GBASTATION_PROJECT_ICON}
            --nacp=${CMAKE_BINARY_DIR}/GBAStation3DSStub.nacp
    )
    return()
endif()

if (NOT PLATFORM_SWITCH)
    return()
endif()

include(ExternalProject)

set(THREEDS_STUB_STAGE_ROOT ${CMAKE_BINARY_DIR}/azahar_switch)
set(THREEDS_STUB_STAGE_SOURCE ${THREEDS_STUB_STAGE_ROOT}/source)
set(THREEDS_STUB_STAGE_BUILD ${THREEDS_STUB_STAGE_ROOT}/build)
set(THREEDS_STUB_OUTPUT ${THREEDS_STUB_STAGE_BUILD}/GBAStation3DSStub.nro)

ExternalProject_Add(GBAStation3DSStubExternal
    PREFIX ${THREEDS_STUB_STAGE_ROOT}/external
    SOURCE_DIR ${THREEDS_STUB_STAGE_SOURCE}
    BINARY_DIR ${THREEDS_STUB_STAGE_BUILD}
    DOWNLOAD_COMMAND ""
    UPDATE_COMMAND
        ${CMAKE_COMMAND}
        -DAZAHAR_SOURCE_DIR=${CMAKE_SOURCE_DIR}/third_party/azahar-2125.1.3
        -DSTAGE_ROOT=${THREEDS_STUB_STAGE_ROOT}
        -DTHREEDS_STUB_DIR=${THREEDS_STUB_DIR}
        -P ${THREEDS_STUB_DIR}/cmake/PrepareAzaharSource.cmake
    PATCH_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=$ENV{DEVKITPRO}/cmake/Switch.cmake
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DCMAKE_DEPENDS_USE_COMPILER=FALSE
        -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=${THREEDS_STUB_DIR}/cmake/AzaharSwitchOverrides.cmake
        -DGBASTATION_3DS_STUB_IN_TREE=ON
        -DGBASTATION_3DS_STUB_DIR=${THREEDS_STUB_DIR}
        -DGBASTATION_PROJECT_ICON=${PROJECT_ICON}
        -DARCHITECTURE=arm64
        -DENABLE_SDL2=OFF
        -DENABLE_QT=OFF
        -DENABLE_QT_TRANSLATION=OFF
        -DENABLE_TESTS=OFF
        -DENABLE_ROOM=OFF
        -DENABLE_ROOM_STANDALONE=OFF
        -DENABLE_WEB_SERVICE=OFF
        -DENABLE_SCRIPTING=OFF
        -DENABLE_CUBEB=OFF
        -DENABLE_OPENAL=OFF
        -DENABLE_LIBUSB=OFF
        -DENABLE_SOFTWARE_RENDERER=ON
        -DENABLE_OPENGL=ON
        -DENABLE_VULKAN=OFF
        -DUSE_DISCORD_PRESENCE=OFF
        -DENABLE_MICROPROFILE=OFF
        -DENABLE_LTO=OFF
        -DENABLE_NATIVE_OPTIMIZATION=OFF
        -DCITRA_USE_PRECOMPILED_HEADERS=OFF
        -DCITRA_WARNINGS_AS_ERRORS=OFF
        -DUSE_SYSTEM_OPENSSL=OFF
        -DUSE_SYSTEM_ZSTD=OFF
        -DHAVE_ARC4RANDOM_BUF=ON
        -DHAVE_ARC4RANDOM_UNIFORM=ON
        -DHAVE_GETENTROPY=ON
        -DHAVE_SYSLOG_R=ON
        -DDYNARMIC_TESTS=OFF
        -DDYNARMIC_FRONTENDS=A32
        -DDYNARMIC_ENABLE_NO_EXECUTE_SUPPORT=ON
        -DDYNARMIC_USE_PRECOMPILED_HEADERS=OFF
        -DDYNARMIC_USE_BUNDLED_EXTERNALS=ON
    BUILD_COMMAND
        ${CMAKE_COMMAND} --build <BINARY_DIR> --target GBAStation3DSStub.nro --parallel 2
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${THREEDS_STUB_OUTPUT}
)

add_custom_target(GBAStation3DSStub.nro ALL
    DEPENDS GBAStation3DSStubExternal
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${THREEDS_STUB_OUTPUT}
        ${CMAKE_BINARY_DIR}/GBAStation3DSStub.nro
)
