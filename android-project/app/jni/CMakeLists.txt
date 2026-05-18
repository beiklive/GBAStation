cmake_minimum_required(VERSION 3.10)

project(GBASTATION_ANDROID)

set(BUILD_SHARED_LIBS ON CACHE BOOL "")

add_subdirectory(SDL)

if(TARGET SDL3::SDL3-shared)
    add_library(SDL2 ALIAS SDL3::SDL3-shared)
elseif(TARGET SDL3::SDL3)
    add_library(SDL2 ALIAS SDL3::SDL3)
elseif(TARGET SDL3-shared)
    add_library(SDL2 ALIAS SDL3-shared)
else()
    message(FATAL_ERROR "Could not find SDL3 target. Available targets:")
    message(FATAL_ERROR "  Expected: SDL3::SDL3-shared, SDL3::SDL3, or SDL3-shared")
endif()

if(NOT TARGET SDL2main)
    add_library(SDL2main INTERFACE)
endif()

SET(PLATFORM_ANDROID ON CACHE BOOL "")
SET(LIBROMFS_PREBUILT_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/borealis/libromfs-generator" CACHE STRING "")

if(NOT EXISTS "${LIBROMFS_PREBUILT_GENERATOR}")
    message(FATAL_ERROR "libromfs-generator not found. Run build_libromfs_generator.sh first.")
endif()

add_subdirectory(borealis)
