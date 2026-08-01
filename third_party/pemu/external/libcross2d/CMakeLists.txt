cmake_minimum_required(VERSION 3.10)
#set(CMAKE_VERBOSE_MAKEFILE ON)

####################
# TARGET PLATFORM
####################
option(PLATFORM_LINUX "Linux target platform" OFF)
option(PLATFORM_WINDOWS "Windows target platform (msys/mingw64)" OFF)
option(PLATFORM_SWITCH "Nintendo Switch target platform" OFF)
option(PLATFORM_3DS "Nintendo 3DS target platform" OFF)
option(PLATFORM_VITA "Sony PS Vita target platform" OFF)
option(PLATFORM_PS5 "Sony PS5 target platform" OFF)
option(PLATFORM_PS4 "Sony PS4 target platform" OFF)
option(PLATFORM_PS3 "Sony PS3 target platform" OFF)
option(PLATFORM_DREAMCAST "Sega Dreamcast platform target" OFF)
option(PLATFORM_ANDROID "Android platform target" OFF)

# setup target toolchain
include(cmake/toolchain.cmake)

####################
# BUILD OPTIONS
####################
option(OPTION_SDL1 "SDL1 support" OFF)
option(OPTION_SDL2 "SDL2 support" OFF)
option(OPTION_SDL2_RGB565 "SDL2 with rgb565 support (retropico)" OFF)
option(OPTION_RENDER_GL1 "OpenGL 1.2 rendering" OFF)
option(OPTION_RENDER_GL2 "OpenGL 4.3 rendering" OFF)
option(OPTION_RENDER_GLES2 "OpenGLES 2.0 rendering" OFF)
option(OPTION_RENDER_GLES3 "OpenGLES 3.0 rendering" OFF)
option(OPTION_LOADER_GLAD "Glad OpenGL loader" ON)
option(OPTION_LOADER_GLEW "Glew OpenGL loader" OFF)
option(OPTION_GL_DUMP_SHADERS "Dump shaders binaries" OFF)
if (PLATFORM_PS4)
    option(OPTION_GL_SHADERS_BINARY "Use glsl shaders binaries" ON)
else ()
    option(OPTION_GL_SHADERS_BINARY "Use glsl shaders binaries" OFF)
endif ()
option(OPTION_BUILTIN_LIBCONFIG "Build libconfig from sources" OFF)
if (PLATFORM_DREAMCAST)
    option(OPTION_FREETYPE "Build with freetype support" OFF)
else ()
    option(OPTION_FREETYPE "Build with freetype support" ON)
endif ()
option(OPTION_BOX2D "Build with box2d support" OFF)
option(OPTION_TEST "Build test executable" OFF)
set(ANDROID_ASSETS_PATH "" CACHE STRING "Android assets path")

####################
# SANITY CHECKS
####################
if (PLATFORM_LINUX OR PLATFORM_WINDOWS)
    if (NOT (OPTION_SDL1 OR OPTION_RENDER_GL1 OR OPTION_RENDER_GLES2 OR OPTION_RENDER_GLES3))
        message(STATUS "C2D: SDL2 OpenGL 4.3 support enabled")
        set(OPTION_SDL2 ON CACHE BOOL "SDL2 support" FORCE)
        set(OPTION_RENDER_GL2 ON CACHE BOOL "OpenGL 4.3 rendering" FORCE)
    endif ()
endif ()

if (PLATFORM_ANDROID)
    message(STATUS "C2D: Android with OpenGLES 2.0 support enabled")
    set(OPTION_SDL2 ON CACHE BOOL "SDL2 support" FORCE)
    set(OPTION_RENDER_GLES2 ON CACHE BOOL "OpenGLES 2.0 rendering" FORCE)
    set(OPTION_LOADER_GLAD OFF CACHE BOOL "Glad OpenGL loader" FORCE)
endif ()

if (PLATFORM_SWITCH)
    message(STATUS "C2D: Nintendo Switch with OpenGL 4.3 support enabled")
    set(OPTION_SDL2 ON CACHE BOOL "SDL2 support" FORCE)
    set(OPTION_RENDER_GL2 ON CACHE BOOL "OpenGL 4.3 rendering" FORCE)
    set(OPTION_BOX2D ON CACHE BOOL "Build with box2d support" FORCE)
endif ()

if (PLATFORM_VITA)
    message(STATUS "C2D: Ps Vita with Gxm support enabled")
    set(OPTION_SDL2 ON CACHE BOOL "SDL2 support" FORCE)
    #set(OPTION_BOX2D ON CACHE BOOL "Build with box2d support" FORCE)
endif ()

if (PLATFORM_PS5)
    message(STATUS "C2D: SDL2 OpenGL 4.3 support enabled")
    set(OPTION_SDL2 ON CACHE BOOL "SDL2 support" FORCE)
    set(OPTION_RENDER_GL2 ON CACHE BOOL "OpenGL 4.3 rendering" FORCE)
endif ()

if (PLATFORM_PS4)
    message(STATUS "C2D: PS4 with SDL2 OpenGLES 2.0 support enabled")
    set(OPTION_SDL2 ON CACHE BOOL "OpenGLES 2.0 rendering" FORCE)
    set(OPTION_RENDER_GLES2 ON CACHE BOOL "OpenGLES 2.0 rendering" FORCE)
    set(OPTION_LOADER_GLAD OFF CACHE BOOL "Glad OpenGL loader" FORCE)
endif ()

if (PLATFORM_DREAMCAST)
    message(STATUS "C2D: DREAMCAST OpenGL 1.2 support enabled")
    set(OPTION_RENDER_GL1 ON CACHE BOOL "OpenGL 1.2 rendering" FORCE)
    set(OPTION_LOADER_GLAD OFF CACHE BOOL "Glad OpenGL loader" FORCE)
endif ()

if (PLATFORM_PS3)
    set(OPTION_BUILTIN_LIBCONFIG ON CACHE BOOL "Build libconfig from sources" FORCE)
endif ()

####################
# COMMON STUFF
####################

project(cross2d)

file(GLOB C2D_SOURCES
        source/widgets/*.c*
        source/skeleton/*.c*
        source/skeleton/sfml/*.c*)

# deps
include(FindPkgConfig)
if (OPTION_FREETYPE)
    pkg_search_module(FREETYPE REQUIRED freetype2)
endif ()
if (NOT PLATFORM_PS3)
    pkg_search_module(PNG REQUIRED libpng)
    pkg_search_module(ZLIB REQUIRED zlib)
endif ()

set(C2D_CFLAGS -Wall -Wno-narrowing -DLIBCONFIG_STATIC -D__CROSS2D__)
set(C2D_INCLUDES include ${FREETYPE_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
set(C2D_LDFLAGS ${FREETYPE_LIBRARIES} ${PNG_LIBRARIES} ${ZLIB_LIBRARIES})
set(CMAKE_CXX_STANDARD 17)

if (NOT OPTION_FREETYPE)
    list(APPEND C2D_CFLAGS -D__NO_FREETYPE__)
endif ()

if (OPTION_BUILTIN_LIBCONFIG)
    list(APPEND LIBCONFIG_INCLUDES source/skeleton/libconfig)
    file(GLOB LIBCONFIG_SOURCES source/skeleton/libconfig/*.c)
else ()
    pkg_search_module(LIBCONFIG REQUIRED libconfig)
    list(APPEND C2D_INCLUDES ${LIBCONFIG_INCLUDE_DIRS})
    list(APPEND C2D_LDFLAGS ${LIBCONFIG_LIBRARIES})
endif ()

if (OPTION_SDL1)
    pkg_search_module(SDL REQUIRED sdl)
    list(APPEND C2D_INCLUDES ${SDL_INCLUDE_DIRS})
    list(APPEND C2D_LDFLAGS ${SDL_LIBRARIES})
    list(APPEND C2D_CFLAGS -D__SDL1__)
    file(GLOB SDL_SRC source/platforms/sdl1/*.c*)
    list(APPEND C2D_SOURCES ${SDL_SRC})
    if (NOT (OPTION_RENDER_GL1 OR OPTION_RENDER_GLES2 OR OPTION_RENDER_GLES3))
        set(OPTION_RENDER_GL2 ON CACHE BOOL "OpenGL 4.3 rendering" FORCE)
    endif ()
endif ()

if (OPTION_SDL2)
    pkg_search_module(SDL2 REQUIRED sdl2)
    list(APPEND C2D_INCLUDES ${SDL2_INCLUDE_DIRS})
    if (NOT PLATFORM_VITA)
        list(APPEND C2D_CFLAGS -D__SDL2__)
    endif ()
    list(APPEND C2D_LDFLAGS ${SDL2_LIBRARIES})
    file(GLOB SDL2_SRC source/platforms/sdl2/*.c*)
    list(APPEND C2D_SOURCES ${SDL2_SRC})
    if (OPTION_SDL2_RGB565)
        list(APPEND C2D_CFLAGS -D__SDL2_RGB565__)
    endif ()
endif ()

# gl2 / gles2
if (OPTION_RENDER_GL2 OR OPTION_RENDER_GLES2 OR OPTION_RENDER_GLES3)
    file(GLOB GL_SRC source/platforms/gl2/*.c*)
    if (NOT OPTION_GL_SHADERS_BINARY)
        file(GLOB GL_SHADERS_SRC source/platforms/gl2/shaders/*.c*)
    endif ()
    list(APPEND C2D_SOURCES ${GL_SRC} ${GL_SHADERS_SRC})
    list(APPEND C2D_CFLAGS -D__GL2__)
    if (OPTION_RENDER_GLES2 OR OPTION_RENDER_GLES3)
        if (OPTION_RENDER_GLES2)
            list(APPEND C2D_CFLAGS -D__GLES2__)
        else ()
            list(APPEND C2D_CFLAGS -D__GLES3__)
        endif ()
        if (NOT PLATFORM_PS4)
            list(APPEND C2D_LDFLAGS GLESv2)
        endif ()
    elseif (NOT PLATFORM_SWITCH)
        set(OpenGL_GL_PREFERENCE GLVND)
        find_package(OpenGL REQUIRED)
        list(APPEND C2D_INCLUDES ${OPENGL_INCLUDE_DIRS})
        list(APPEND C2D_LDFLAGS ${OPENGL_LIBRARIES})
        find_package(glm REQUIRED)
        list(APPEND C2D_INCLUDES ${GLM_INCLUDE_DIRS})
        list(APPEND C2D_LDFLAGS ${GLM_LIBRARIES})
    endif ()
endif ()

# gl1
if (OPTION_RENDER_GL1)
    file(GLOB GL_SRC source/platforms/gl1/*.c*)
    list(APPEND GL_SRC source/platforms/gl2/gl_texture.cpp)
    list(APPEND C2D_SOURCES ${GL_SRC})
    list(APPEND C2D_CFLAGS -D__GL1__)
endif ()

# glad/glew
if (OPTION_RENDER_GL1 OR OPTION_RENDER_GL2 OR OPTION_RENDER_GLES2 OR OPTION_RENDER_GLES3)
    if (OPTION_LOADER_GLEW)
        set(GLEW_USE_STATIC_LIBS ON)
        find_package(GLEW REQUIRED)
        list(APPEND C2D_CFLAGS -D__GLEW__ -DGLEW_STATIC)
        list(APPEND C2D_LDFLAGS glew32)
    elseif (OPTION_LOADER_GLAD)
        if (OPTION_RENDER_GL1)
            list(APPEND C2D_SOURCES
                    source/platforms/glad/gl1/src/egl.c
                    source/platforms/glad/gl1/src/gl.c
            )
            list(APPEND C2D_INCLUDES source/platforms/glad/gl1/include)
        elseif (OPTION_RENDER_GL2)
            list(APPEND C2D_SOURCES
                    source/platforms/glad/gl2/src/egl.c
                    source/platforms/glad/gl2/src/gl.c
            )
            list(APPEND C2D_INCLUDES source/platforms/glad/gl2/include)
        elseif (OPTION_RENDER_GLES2)
            list(APPEND C2D_SOURCES
                    source/platforms/glad/gles2/src/egl.c
                    source/platforms/glad/gles2/src/gles2.c
            )
            list(APPEND C2D_INCLUDES source/platforms/glad/gles2/include)
        elseif (OPTION_RENDER_GLES3)
            list(APPEND C2D_SOURCES
                    source/platforms/glad/gles3/src/egl.c
                    source/platforms/glad/gles3/src/gles2.c
            )
            list(APPEND C2D_INCLUDES source/platforms/glad/gles3/include)
        endif ()
        list(APPEND C2D_CFLAGS -D__GLAD__)
    endif ()
endif ()

if (OPTION_GL_DUMP_SHADERS)
    list(APPEND C2D_CFLAGS -DGL_DUMP_SHADERS)
endif ()
if (OPTION_GL_SHADERS_BINARY)
    list(APPEND C2D_CFLAGS -DGL_SHADERS_BINARY)
endif ()

if (OPTION_BOX2D)
    find_package(box2d REQUIRED)
    list(APPEND C2D_LDFLAGS box2d)
    list(APPEND C2D_CFLAGS -D__BOX2D__)
endif ()

# export tools
set(ZIP zip CACHE STRING "zip executable path")

#####################
# PLATORM SPECIFIC
#####################
if (PLATFORM_VITA)
    #####################
    # VITA PLATORM
    #####################
    enable_language(ASM)
    set(TITLE_ID CROSS0001)
    set(PLATFORM_INCLUDES
            source/platforms/posix
            source/platforms/psp2
            source/platforms/psp2/vita-shader-collection/includes
    )
    file(GLOB PLATFORM_SOURCES
            source/platforms/posix/posix_io.cpp
            source/platforms/psp2/*.c*
    )
    list(REMOVE_ITEM C2D_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/source/platforms/sdl2/sdl2_renderer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/source/platforms/sdl2/sdl2_device.cpp
    )
    list(APPEND C2D_LDFLAGS
            ${CMAKE_CURRENT_SOURCE_DIR}/source/platforms/psp2/vita-shader-collection/lib/libvitashaders.a
            vita2d box2d SceGxm_stub ScePower_stub
    )
    list(APPEND C2D_CFLAGS -D__PSP2__ -DNO_KEYBOARD
            -mtune=cortex-a9 -march=armv7-a -mfpu=neon -mfloat-abi=hard
            #-ffast-math -fsingle-precision-constant
    )
elseif (PLATFORM_PS5)
    file(GLOB PLATFORM_SOURCES source/platforms/posix/*.c*)
elseif (PLATFORM_PS4)
    #####################
    # PS4 PLATORM
    #####################
    set(PLATFORM_INCLUDES
            include/cross2d/platforms/ps4
            include/cross2d/platforms/posix
    )
    file(GLOB PLATFORM_SOURCES
            source/platforms/ps4/*.c*
            source/platforms/posix/*.c*
    )
    if (OPTION_GL_SHADERS_BINARY)
        file(GLOB SHADERS_SOURCES source/platforms/ps4/shaders/*.c*)
        list(APPEND PLATFORM_SOURCES ${SHADERS_SOURCES})
    endif ()
    list(APPEND C2D_CFLAGS -D__PSP4__ -DNO_KEYBOARD -Wno-writable-strings)
elseif (PLATFORM_PS3)
    enable_language(ASM)
    set(PLATFORM_INCLUDES
            source/platforms/ps3
            source/platforms/ps3/tiny3d/source
            include/cross2d/platforms/ps3/tiny3d
            source/platforms/posix
            ${PSL1GHT}/portlibs/ppu/include/freetype2
    )
    file(GLOB PLATFORM_SOURCES
            source/platforms/ps3/*.c*
            source/platforms/ps3/tiny3d/source/*.c*
            source/platforms/ps3/tiny3d/source/*.S*
            source/platforms/posix/posix_io.cpp
    )
    list(APPEND C2D_LDFLAGS
            rsx gcm_sys sysutil sysmodule rt lv2
            io audio
            freetype tiff jpeg pngdec
            m z
    )
    list(APPEND C2D_CFLAGS -D__PS3__ -D__C2D_ARGB__ -DNO_KEYBOARD)
elseif (PLATFORM_3DS)
    #####################
    # 3DS PLATORM
    #####################
    enable_language(ASM)
    set(PLATFORM_INCLUDES
            source/platforms/3ds
            source/platforms/posix
    )
    file(GLOB PLATFORM_SOURCES
            source/platforms/3ds/*.c*
            source/platforms/3ds/*.s
            source/platforms/posix/*.c*
    )
    list(APPEND C2D_LDFLAGS citro3d ctru m)
    list(APPEND C2D_CFLAGS -DARM11 -D__3DS__)
    # TODO
    #ctr_add_shader_library(${PROJECT_NAME}.pica ${CMAKE_CURRENT_SOURCE_DIR}/source/platforms/3ds/vshader.pica)
    #dkp_add_embedded_binary_library(${PROJECT_NAME}.pica.h "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pica.shbin")
    #####################
    # SWITCH PLATORM
    #####################
elseif (PLATFORM_SWITCH)
    set(PLATFORM_INCLUDES
            source/platforms/posix
            source/platforms/switch)
    file(GLOB PLATFORM_SOURCES
            source/platforms/posix/posix_io.cpp
            source/platforms/posix/posix_clock.cpp
            source/platforms/switch/*.c*)
    list(APPEND C2D_CFLAGS -fPIC -D__SWITCH__ -DNO_KEYBOARD)
    if (OPTION_DEBUG_SVC)
        list(APPEND C2D_CFLAGS -D__DEBUG_SVC__)
    endif ()
    ########################
    # DREAMCAST PLATORM
    ########################
elseif (PLATFORM_DREAMCAST)
    #enable_language(ASM)
    file(GLOB_RECURSE PLATFORM_SOURCES source/platforms/dreamcast/*.c*)
    set(PLATFORM_INCLUDES source/platforms/dreamcast)
    list(APPEND C2D_CFLAGS -D__DREAMCAST__ -D__GL1_IMMEDIATE__ -DNO_KEYBOARD)
    list(APPEND C2D_LDFLAGS c kallisti kosutils kosext2fs GLdc c z m) # kosfat
    ########################
    # LINUX PLATFORM
    ########################
elseif (PLATFORM_LINUX)
    file(GLOB PLATFORM_SOURCES source/platforms/posix/*.c*)
    list(APPEND C2D_CFLAGS -D__LINUX__)
    ########################
    # WIN64 PLATFORM
    ########################
elseif (PLATFORM_WINDOWS)
    file(GLOB PLATFORM_SOURCES source/platforms/posix/*.c*)
    # get msys root path
    if (${MSYS})
        # build from "MSYS2 MinGW x64" shell
        set(MSYS_ROOT "/" CACHE STRING "msys root path" FORCE)
    else ()
        # built with ninja from clion
        string(REPLACE "mingw64/bin/cc.exe" "" MSYS_ROOT_TEMP ${CMAKE_C_COMPILER})
        set(MSYS_ROOT ${MSYS_ROOT_TEMP} CACHE STRING "msys root path" FORCE)
        set(ZIP "${MSYS_ROOT}/usr/bin/zip.exe" CACHE STRING "zip executable path" FORCE)
    endif ()
    list(APPEND C2D_LDFLAGS
            freetype brotlidec brotlicommon
            harfbuzz graphite2 rpcrt4 dwrite bz2 png z
            ole32 oleaut32 imm32 winmm version setupapi gdi32 opengl32)
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        list(APPEND C2D_LDFLAGS -mwindows)
    endif ()
elseif (PLATFORM_ANDROID)
    file(GLOB PLATFORM_SOURCES source/platforms/posix/*.c*)
    list(APPEND PLATFORM_SOURCES source/platforms/android/android_io.cpp)
    list(APPEND PLATFORM_SOURCES source/platforms/android/android_device.cpp)
endif ()

add_library(${PROJECT_NAME} STATIC ${C2D_SOURCES} ${LIBCONFIG_SOURCES} ${PLATFORM_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${C2D_INCLUDES} ${LIBCONFIG_INCLUDES} ${PLATFORM_INCLUDES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${C2D_LDFLAGS})
target_compile_options(${PROJECT_NAME} PUBLIC ${C2D_CFLAGS})

#####################
# test executable
#####################
if (OPTION_TEST)
    project(cross2d_test)
    set(PROJECT_AUTHOR "Cpasjuste")
    set(VERSION_MAJOR "1")
    set(VERSION_MINOR "0")
    if (PLATFORM_PS4)
        set(PS4_PKG_TITLE "libcross2d sample")
        set(PS4_PKG_TITLE_ID "CROS20001")
        set(PS4_PKG_VERSION "01.00")
    endif ()
    add_executable(${PROJECT_NAME} test/main.cpp)
    target_link_libraries(${PROJECT_NAME} cross2d)
    #############
    # targets
    #############
    include(cmake/targets.cmake)
endif ()
