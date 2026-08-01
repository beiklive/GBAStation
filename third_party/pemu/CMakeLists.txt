cmake_minimum_required(VERSION 3.10)
#set(CMAKE_VERBOSE_MAKEFILE ON)

SET(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)
SET(CMAKE_CXX_USE_RESPONSE_FILE_FOR_LIBRARIES 1)
SET(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")

# add libcross2d library. If you want to port pfba to another platform,
# you should (may) only need to port libcross2d library.
add_subdirectory(external/libcross2d)

####################
# BUILD OPTIONS
####################
set(OPTION_EMU pfbneo CACHE STRING "Emulator to build (pfbneo)")
set(OPTION_BUILTIN_LIBCONFIG OFF CACHE BOOL "Build libconfig from sources")
set(OPTION_MPV_PLAYER ON CACHE BOOL "Add support for video snaps (mpv)")
set(OPTION_UPDATE_CONFIG OFF CACHE BOOL "Force config file updating")
####################
# BUILD OPTIONS
####################

# linux/windows screen handling options
if (PLATFORM_LINUX OR PLATFORM_WINDOWS)
    set(OPTION_RESOLUTION ON CACHE BOOL "Add screen resolution option")
    set(OPTION_FULLSCREEN ON CACHE BOOL "Add switch to fullscreen option")
endif ()

# add sscrap (screenscrapper scrapper!) library
add_subdirectory(external/sscrap)

# required for cmake toolchain setup
project(pemu)
set(PROJECT_AUTHOR cpasjuste)
set(VERSION_MAJOR 7)
set(VERSION_MINOR 1)

# pemu (ui)
add_subdirectory(src/skeleton)

# cores
if (OPTION_EMU)
    if (OPTION_EMU STREQUAL "pfbneo")
        add_subdirectory(src/cores/pfbneo)
    elseif (OPTION_EMU STREQUAL "pgen")
        add_subdirectory(src/cores/pgen)
    elseif (OPTION_EMU STREQUAL "pnes")
        add_subdirectory(src/cores/pnes)
    elseif (OPTION_EMU STREQUAL "psnes")
        add_subdirectory(src/cores/psnes)
    elseif (OPTION_EMU STREQUAL "pgba")
        add_subdirectory(src/cores/pgba)
    else ()
        message(FATAL_ERROR "Unsupported OPTION_EMU: ${OPTION_EMU}")
    endif ()
else ()
    add_subdirectory(src/cores/pfbneo)
endif ()
