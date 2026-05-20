# =========================================================
# melonds_core — 官方 melonDS 模拟器核心静态库
#
# 与根 CMakeLists.txt 保持一致的平台设置：
#   - PLATFORM_DESKTOP / PLATFORM_SWITCH (borealis)
#   - Win32 / Apple / Linux / Switch 平台检测
#
# 构建选项：
#   ENABLE_JIT        → 启用 JIT 重编译器 (x86_64/ARM64)
#   ENABLE_OGLRENDERER → 启用 OpenGL 3D 渲染器
#   ENABLE_DEKOGPU    → 启用 deko3D GPU (Switch)
#
# 目标：melonds_core (STATIC)
# =========================================================
cmake_minimum_required(VERSION 3.16)

include(sources.cmake)

# =========================================================
# Include paths
# =========================================================
set(MELONDS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../melonDS/src)

# =========================================================
# 创建静态库目标
# =========================================================
add_library(melonds_core STATIC
    ${MELONDS_CORE_SOURCES}
)

target_include_directories(melonds_core PUBLIC
    ${MELONDS_SRC_DIR}
)

target_compile_definitions(melonds_core PRIVATE
    MELONDS_EMBEDDED
    ${APP_PLATFORM_OPTION}
)

target_compile_options(melonds_core PRIVATE
    -ffunction-sections -fdata-sections
    ${APP_PLATFORM_OPTION}
)

set_target_properties(melonds_core PROPERTIES CXX_STANDARD 17)

# =========================================================
# melonDS 版本信息
# =========================================================
set(MELONDS_VERSION "0.9.2")
target_compile_definitions(melonds_core PRIVATE MELONDS_VERSION="${MELONDS_VERSION}")

# =========================================================
# 平台特定定义
# =========================================================
if (PLATFORM_SWITCH)
    target_compile_definitions(melonds_core PUBLIC __SWITCH__)
endif()

# =========================================================
# JIT 支持 (x86_64 / ARM64)
# =========================================================
if ((CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64") AND NOT PLATFORM_SWITCH)
    set(ENABLE_JIT ON)
    target_compile_definitions(melonds_core PRIVATE JIT_ENABLED ARCHITECTURE_x86_64=1)
    enable_language(ASM)
    target_sources(melonds_core PRIVATE ${MELONDS_CORE_JIT_X64_SOURCES})
elseif ((CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64") OR PLATFORM_SWITCH)
    if (NOT APPLE)
        set(ENABLE_JIT ON)
    endif()
    if (ENABLE_JIT)
        target_compile_definitions(melonds_core PRIVATE JIT_ENABLED ARCHITECTURE_ARM64=1)
        enable_language(ASM)
        target_sources(melonds_core PRIVATE ${MELONDS_CORE_JIT_A64_SOURCES})
    endif()
endif()

# =========================================================
# OpenGL Renderer (disabled for Phase 1 - software rendering first)
#
# Enable later after software rendering is stable.
# =========================================================
# if (PLATFORM_DESKTOP)
#     set(ENABLE_OGLRENDERER ON)
#     target_compile_definitions(melonds_core PRIVATE OGLRENDERER_ENABLED)
#     target_sources(melonds_core PRIVATE ${MELONDS_CORE_OGL_SOURCES})
# endif()

# =========================================================
# Deko3D GPU (Switch only)
# =========================================================
if (PLATFORM_SWITCH)
    target_sources(melonds_core PRIVATE ${MELONDS_CORE_DEKO_SOURCES})
    if (USE_DEKO3D)
        target_compile_definitions(melonds_core PRIVATE DEKOGPU_ENABLED)
    endif()
endif()

# =========================================================
# teakra (DSi DSP 模拟)
# =========================================================
add_subdirectory(${MELONDS_SRC_DIR}/teakra ${CMAKE_CURRENT_BINARY_DIR}/teakra EXCLUDE_FROM_ALL)
target_link_libraries(melonds_core PRIVATE teakra)

# =========================================================
# 平台链接库
# =========================================================
if (WIN32)
    target_link_libraries(melonds_core PRIVATE ole32 comctl32 ws2_32)
elseif (PLATFORM_SWITCH)
    target_link_libraries(melonds_core PRIVATE nx)
elseif (UNIX AND NOT APPLE)
    target_link_libraries(melonds_core PRIVATE rt)
endif()

# =========================================================
# 确保 Borealis 的 OpenGL (GLAD) 头文件可用
# =========================================================
if (ENABLE_OGLRENDERER)
    target_link_libraries(melonds_core PRIVATE borealis)
endif()
