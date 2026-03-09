cmake_minimum_required(VERSION 3.10)

# =========================================================
# 路径变量（与根目录 CMakeLists.txt 保持一致）
# =========================================================
set(BOREALIS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/borealis/library)
set(MGBA_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/mgba)

# =========================================================
# borealis UI 框架
#
# 构建静态库目标 'borealis'。
# 平台选项（PLATFORM_DESKTOP / PLATFORM_SWITCH 等）
# 和渲染器选项（USE_SDL2 / USE_GLFW / USE_DEKO3D 等）
# 由根目录 CMakeLists.txt 通过 commonOption.cmake /
# toolchain.cmake 在包含本文件之前设置完毕。
# =========================================================
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/borealis/library)

# =========================================================
# mGBA 模拟器  –  libretro 核心构建
#
# 使用 EXCLUDE_FROM_ALL 后，mGBA 自身的 CMakeLists 会
# 自动检测到 LIBMGBA_ONLY=ON，从而：
#   • 跳过所有 GUI / SDL / Qt 前端
#   • 禁用外部依赖搜索（zlib、png 等）
#   • 强制 BUILD_STATIC=ON → 生成静态库目标 'mgba'
#     供主程序直接链接
#
# 在此基础上，通过 BUILD_LIBRETRO=ON 额外构建
# 独立的 'mgba_libretro' 共享库（即可被 RetroArch /
# Lakka 等加载的 libretro 核心二进制文件）。
#
# 支持平台：
#   桌面端（Windows / macOS / Linux） → mgba_libretro.dll/.dylib/.so
#   Nintendo Switch（DevkitPro / libnx）→ mgba_libretro.so
# =========================================================

# 强制启用 libretro 核心；通过 CACHE 方式写入以保证
# 值能跨越 add_subdirectory() 的作用域边界生效。

set(BUILD_LIBRETRO ON  CACHE BOOL "" FORCE)
# set(SKIP_LIBRARY   ON  CACHE BOOL "" FORCE)   # 只构建核心，跳过 libmgba
# set(BUILD_SDL      OFF CACHE BOOL "" FORCE)
# set(BUILD_QT       OFF CACHE BOOL "" FORCE)

# mGBA 内部通过 `DEFINED SWITCH`（CMake 变量）检测 Switch 平台；
# borealis 工具链设置的是 PLATFORM_SWITCH，不会触发 mGBA 的逻辑。
# 但该版本的 mGBA 没有 src/platform/switch 目录，直接设置 SWITCH=TRUE
# 会导致 add_subdirectory 找不到目录而报错。
# 因此手动复制 mGBA IS_EMBEDDED 分支中实际需要的 OFF 项：
# if (PLATFORM_SWITCH)
#     set(USE_SQLITE3     OFF CACHE BOOL "" FORCE)
#     set(USE_DEBUGGERS   OFF CACHE BOOL "" FORCE)
#     set(USE_DISCORD_RPC OFF CACHE BOOL "" FORCE)
#     set(USE_LIBZIP      OFF CACHE BOOL "" FORCE)
# endif ()

# 禁止 mGBA 构建文档、性能分析及各类测试相关目标
set(BUILD_PERF     OFF CACHE BOOL "" FORCE)
set(BUILD_TEST     OFF CACHE BOOL "" FORCE)
set(BUILD_SUITE    OFF CACHE BOOL "" FORCE)
set(BUILD_CINEMA   OFF CACHE BOOL "" FORCE)
set(BUILD_DOCGEN   OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mgba)
