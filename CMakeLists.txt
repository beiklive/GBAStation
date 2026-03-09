cmake_minimum_required(VERSION 3.10)



set(BOREALIS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/borealis/library)
set(MGBA_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mgba)

# 构建选项
include(${BOREALIS_LIBRARY}/cmake/commonOption.cmake)

# 依赖项
option(USE_SHARED_LIB "Whether to use shared libs provided by system" OFF)
cmake_dependent_option(USE_SYSTEM_FMT "" OFF "NOT USE_SHARED_LIB" ON)
cmake_dependent_option(USE_SYSTEM_TINYXML2 "" OFF "NOT USE_SHARED_LIB" ON)
cmake_dependent_option(USE_SYSTEM_TWEENY "" OFF "NOT USE_SHARED_LIB" ON)

if (APPLE AND PLATFORM_DESKTOP)
    option(BUNDLE_MACOS_APP "Bundle a app for macOS" OFF)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "" FORCE)
    set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET[arch=arm64] "11.0" CACHE STRING "" FORCE)
    message(STATUS "CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")
endif ()

# 工具链配置
include(${BOREALIS_LIBRARY}/cmake/toolchain.cmake)

# 项目信息
project(BKStation)
set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_ALTER "0")
set(VERSION_BUILD "0")
set(PACKAGE_NAME "com.beiklive.bkstation")
set(PROJECT_AUTHOR "beiklive")
set(PROJECT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/logo128.png)
set(PROJECT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources)

if (USE_LIBROMFS)
    add_libromfs(${PROJECT_NAME} ${PROJECT_RESOURCES})
endif ()

add_subdirectory(third_party)

# 为 mGBA 打开 POSIX/GNU 扩展，确保 strdup 有声明
if (TARGET mgba AND NOT WIN32)
    target_compile_definitions(mgba PRIVATE _POSIX_C_SOURCE=200809L _GNU_SOURCE)
endif ()

# Switch (devkitA64/newlib) 交叉编译时，CMake feature-detection 无法运行目标
# 平台二进制，导致 HAVE_LOCALE / HAVE_STRTOF_L 未被检测到。
# 但 devkitA64 的 newlib 实际提供了 locale_t / strtof_l / localtime_r，
# 若不补充这些宏，formatting.h 会重复定义 locale_t / strtof_l，产生类型冲突。
if (TARGET mgba AND PLATFORM_SWITCH)
    target_compile_definitions(mgba PRIVATE
        HAVE_LOCALE
        HAVE_STRTOF_L
        HAVE_LOCALTIME_R
    )
endif ()

find_package(Threads REQUIRED)
list(APPEND APP_PLATFORM_LIB ${CMAKE_THREAD_LIBS_INIT})

# 设置源文件与头文件路径
# list(APPEND APP_PLATFORM_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/demo/include)
# file(GLOB_RECURSE MAIN_SRC demo/*.cpp)

# mGBA 相关=============
# 链接 third_party/ 目录中构建的静态库目标 'mgba'。
# mGBA 以 PUBLIC 方式导出头文件路径，链接后会自动传播
# 至所有依赖它的目标，无需手动添加 include 路径。
message(STATUS "Building mGBA from source (third_party/mgba)")
list(APPEND APP_PLATFORM_LIB mgba)
# mGBA 相关=============

include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(APPEND MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)


set(PLATFORM_OPTION)
if (PLATFORM_DESKTOP)
    if (WIN32)
        # 如果项目本地存在 resource.rc.in 资源描述文件则使用
        if (EXISTS "${CMAKE_SOURCE_DIR}/resources/resource.rc.in")
            configure_file("${CMAKE_SOURCE_DIR}/resources/resource.rc.in" "${CMAKE_BINARY_DIR}/resource.rc")
            list(APPEND MAIN_SRC ${CMAKE_BINARY_DIR}/resource.rc)
        endif ()
        if (MINGW)
            # set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static" CACHE STRING "" FORCE)
        endif ()
    elseif (BUNDLE_MACOS_APP)
        # 如果项目本地存在 icns 图标文件则使用
        if (EXISTS "${CMAKE_SOURCE_DIR}/resources/borealis.icns")
            list(APPEND MAIN_SRC ${CMAKE_SOURCE_DIR}/resources/borealis.icns)
        endif ()
    endif ()
elseif (PLATFORM_SWITCH)
    list(APPEND MAIN_SRC ${CMAKE_SOURCE_DIR}/third_party/borealis/library/lib/platforms/switch/switch_wrapper.c)
endif ()




# 构建可执行目标
program_target(${PROJECT_NAME} "${MAIN_SRC}")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)


# 生成发布文件
if (PLATFORM_DESKTOP)
    if (BUNDLE_MACOS_APP)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
            MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
            MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}"
            MACOSX_BUNDLE_ICON_FILE "borealis.icns"
            MACOSX_BUNDLE_COPYRIGHT "Copyright 2023 ${PROJECT_AUTHOR}"
        )
        if (EXISTS "${CMAKE_SOURCE_DIR}/resources/borealis.icns")
            set_target_properties(${PROJECT_NAME} PROPERTIES
                RESOURCE "${CMAKE_SOURCE_DIR}/resources/borealis.icns"
            )
        endif ()
        add_custom_target(${PROJECT_NAME}.data
                COMMAND "${CMAKE_COMMAND}" -E copy_directory ${PROJECT_RESOURCES} ${PROJECT_NAME}.app/Contents/Resources/resources
        )
    else ()
        add_custom_target(${PROJECT_NAME}.data
                COMMAND "${CMAKE_COMMAND}" -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources
        )
    endif ()
    if (NOT USE_LIBROMFS)
        add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}.data)
    endif ()
elseif (PLATFORM_SWITCH)
    if (USE_DEKO3D)
        gen_dksh("${PROJECT_RESOURCES}/shaders")
    endif ()
    add_custom_target(${PROJECT_NAME}.nro DEPENDS ${PROJECT_NAME}
        COMMAND ${NX_NACPTOOL_EXE} --create "${PROJECT_NAME}" "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}" ${PROJECT_NAME}.nacp --titleid=${PROJECT_TITLEID}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/resources
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources/font
        COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro --icon=${PROJECT_ICON} --nacp=${PROJECT_NAME}.nacp --romfsdir=${CMAKE_BINARY_DIR}/resources
    )
endif ()



target_include_directories(${PROJECT_NAME} PRIVATE ${APP_PLATFORM_INCLUDE})
target_compile_options(${PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections ${APP_PLATFORM_OPTION})
target_link_libraries(${PROJECT_NAME} PRIVATE borealis ${APP_PLATFORM_LIB})
