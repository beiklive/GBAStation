cmake_minimum_required(VERSION 3.10)
#set(CMAKE_VERBOSE_MAKEFILE ON)

####################
# TARGET PLATFORM
####################
option(PLATFORM_LINUX "Linux platform target" OFF)
option(PLATFORM_WINDOWS "Windows platform target (msys/mingw64)" OFF)
option(PLATFORM_RPI3 "RIP3 platform target" OFF)
option(PLATFORM_SWITCH "Nintendo Switch platform target" OFF)
option(PLATFORM_3DS "Nintendo 3DS platform target" OFF)
option(PLATFORM_VITA "Sony PS Vita platform target" OFF)
option(PLATFORM_PS4 "Sony PS4 platform target" OFF)
option(PLATFORM_PS5 "Sony PS5 platform target" OFF)
option(PLATFORM_ANDROID "Android platform target (aarch64)" OFF)
####################
# TARGET PLATFORM
####################

project(sscrap)

##############
# SCREENSCRAP
##############
option(BUILD_SSCRAP "Build sscrap binary" OFF)
set(SS_DEV_ID "none" CACHE STRING "screenscraper dev id")
set(SS_DEV_PWD "none" CACHE STRING "screenscraper dev password")

# handle deps
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(TinyXML2 REQUIRED)

# sources
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.c*)

# includes
set(INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CURL_INCLUDE_DIRS}
        ${TinyXML2_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIRS})

# ldflags
set(LDFLAGS
        ${CURL_LIBRARIES}
        ${TinyXML2_LIBRARIES}
        ${ZLIB_LIBRARIES})

set(CMAKE_CXX_STANDARD 14)

#####################
# PLATORM SPECIFIC
#####################
if (PLATFORM_SWITCH)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    list(APPEND CFLAGS -D__SWITCH__)
    list(APPEND LDFLAGS mbedtls mbedcrypto mbedx509 nx)
elseif (PLATFORM_VITA)
    list(APPEND CFLAGS -D__VITA__)
    list(APPEND LDFLAGS ssl crypto)
elseif (PLATFORM_PS4)
    list(APPEND CFLAGS)
    list(APPEND LDFLAGS tinyxml2 mbedtls mbedcrypto mbedx509 SceNet)
elseif (PLATFORM_PS5)
    #####################
    # PS5 PLATFORM
    #####################
    list(APPEND CFLAGS)
    list(APPEND LDFLAGS tinyxml2 z zstd ssl crypto psl)
elseif (PLATFORM_3DS)
    list(APPEND CFLAGS -D__3DS__)
    list(APPEND LDFLAGS mbedtls mbedcrypto mbedx509 ctru)
elseif (PLATFORM_LINUX)
    # ubuntu does not come with a static tinyxml2 build...
    list(REMOVE_ITEM LDFLAGS ${TinyXML2_LIBRARIES})
    list(APPEND SOURCES external/tinyxml2/tinyxml2.cpp)
elseif (PLATFORM_WINDOWS)
    list(APPEND CFLAGS -D__WINDOWS__)
    #list(APPEND LDFLAGS)
elseif (PLATFORM_ANDROID)
    list(APPEND LDFLAGS -ltinyxml2 -lssl -lcrypto)
endif ()

#####################
# SCREENSCRAP LIBRARY
#####################
add_library(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDES})
target_compile_options(${PROJECT_NAME} PUBLIC ${CFLAGS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LDFLAGS})

# get screenscraper api key
if(NOT ${SS_DEV_ID} MATCHES "none")
    message("-- Screensraper dev id: ${SS_DEV_ID}")
elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ss_dev_id.key")
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/ss_dev_id.key" SS_DEV_ID)
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/ss_dev_pwd.key" SS_DEV_PWD)
    message("-- Screensraper dev id: ${SS_DEV_ID}")
else ()
    message(WARNING "ss_dev_id.key and ss_dev_pwd.key files not found (or not set), \
        using dummy ss keys. Please fix if you want to use live scrapping...")
endif ()
target_compile_options(
        ${PROJECT_NAME} PUBLIC
        -DSS_DEV_ID=\"${SS_DEV_ID}\" -DSS_DEV_PWD=\"${SS_DEV_PWD}\")

#####################
# SCREENSCRAP TEST
#####################
if (BUILD_SSCRAP)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    find_package(Threads REQUIRED)
    find_package(MiniZip REQUIRED)
    file(GLOB HASH_SRC sscrap-utility/hashlibpp/*.cpp)
    add_executable(${PROJECT_NAME}-utility sscrap-utility/main.cpp sscrap-utility/utility.cpp ${HASH_SRC})
    target_link_libraries(${PROJECT_NAME}-utility
            ${PROJECT_NAME}
            ${CMAKE_THREAD_LIBS_INIT}
            ${MINIZIP_LIBRARIES}
            ${ZLIB_LIBRARIES})
    add_custom_command(TARGET ${PROJECT_NAME}-utility POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/databases ${CMAKE_BINARY_DIR}/databases
            COMMENT "Copying databases to binary directory..."
    )

    # releases
    if (PLATFORM_WINDOWS)
        add_custom_target(${PROJECT_NAME}-utility-release
                DEPENDS ${PROJECT_NAME}-utility
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/release
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/release
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/databases ${CMAKE_CURRENT_BINARY_DIR}/release/databases
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-utility${CMAKE_EXECUTABLE_SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/release/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}
                # msys libs...
                COMMAND /usr/bin/bash -l ${CMAKE_CURRENT_SOURCE_DIR}/cmake/mingw_copy_libs.sh "${CMAKE_CURRENT_BINARY_DIR}/release/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}"
                # zip
                COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/release && zip -r ../${PROJECT_NAME}-windows.zip .
                COMMENT "Building windows release..."
        )
        # retrodream stuff
        add_custom_target(${PROJECT_NAME}-retrodream
                DEPENDS ${PROJECT_NAME}-utility
                # utils (dc)
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/sscrap-utility/dreamcast/ffmpeg.exe ${CMAKE_CURRENT_BINARY_DIR}/release/
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/sscrap-utility/dreamcast/magick.exe ${CMAKE_CURRENT_BINARY_DIR}/release/
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/sscrap-utility/dreamcast/pngquant.exe ${CMAKE_CURRENT_BINARY_DIR}/release/
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/sscrap-utility/dreamcast/sscrap-retrodream.ps1 ${CMAKE_CURRENT_BINARY_DIR}/release/
        )
    elseif (PLATFORM_LINUX)
        add_custom_target(${PROJECT_NAME}-utility-release
                DEPENDS ${PROJECT_NAME}-utility
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/release
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/release
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/databases ${CMAKE_CURRENT_BINARY_DIR}/release/databases
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-utility${CMAKE_EXECUTABLE_SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/release/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}
                # zip
                COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/release && zip -r ../${PROJECT_NAME}-linux.zip .
                COMMENT "Building linux release..."
        )
    endif ()
endif ()
