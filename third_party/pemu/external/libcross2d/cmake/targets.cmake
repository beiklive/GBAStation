cmake_minimum_required(VERSION 3.10)
#set(CMAKE_VERBOSE_MAKEFILE ON)

###########################################################
# Copy data to binary directory (common to all platforms)
###########################################################
add_custom_target(${PROJECT_NAME}.data
        # cleanup data in binary dir
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/data_romfs
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/data_datadir
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/data_romfs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/data_datadir
        # copy data to binary directory, for program execution when invoked from cmake build directory
        COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/data/common/datadir -D DST=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
        COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/data/${TARGET_PLATFORM}/datadir -D DST=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
        # cache data to binary directory
        # this allow parent projects to add files here before packaging (make project-name_target_release)
        COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/data/common/romfs -D DST=${CMAKE_CURRENT_BINARY_DIR}/data_romfs -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
        COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/data/common/datadir -D DST=${CMAKE_CURRENT_BINARY_DIR}/data_datadir -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
        COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/data/${TARGET_PLATFORM}/romfs -D DST=${CMAKE_CURRENT_BINARY_DIR}/data_romfs -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
        COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/data/${TARGET_PLATFORM}/datadir -D DST=${CMAKE_CURRENT_BINARY_DIR}/data_datadir -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}.data)

########################
# Linux/Windows targets
########################
if (PLATFORM_LINUX OR PLATFORM_WINDOWS)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
    if (PLATFORM_WINDOWS)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${MSYS_ROOT}usr/bin/bash -l ${cross2d_SOURCE_DIR}/cmake/mingw_copy_libs.sh "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}"
                COMMENT "Copying required mingw dependencies...")
    endif ()
    add_custom_target(${PROJECT_NAME}_${TARGET_PLATFORM}_release
            DEPENDS ${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/data_romfs
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}" ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/*.dll -D DST=${CMAKE_BINARY_DIR}/release/${PROJECT_NAME} -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/data_datadir -D DST=${CMAKE_BINARY_DIR}/release/${PROJECT_NAME} -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/data_romfs -D DST=${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/data_romfs -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND cd ${CMAKE_BINARY_DIR}/release && ${ZIP} -r ../${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip ${PROJECT_NAME}
    )
endif ()

########################
# Android targets
########################
if (PLATFORM_ANDROID)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            # copy romfs to android assets
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${ANDROID_ASSETS_PATH}/romfs
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_ASSETS_PATH}/romfs
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/data_romfs -D DST=${ANDROID_ASSETS_PATH}/romfs -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND echo "${VERSION_MAJOR}.${VERSION_MINOR}" > ${ANDROID_ASSETS_PATH}/romfs/version
            # copy datadir to android assets
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${ANDROID_ASSETS_PATH}/data
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_ASSETS_PATH}/data
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/data_datadir -D DST=${ANDROID_ASSETS_PATH}/data -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND echo "${VERSION_MAJOR}.${VERSION_MINOR}" > ${ANDROID_ASSETS_PATH}/data/version
            COMMENT "Copying android assets...")
endif ()

###########################
# Nintendo Switch target
###########################
if (PLATFORM_SWITCH)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
    add_custom_target(${PROJECT_NAME}.nro
            DEPENDS ${PROJECT_NAME}
            DEPENDS ${PROJECT_NAME}.data
            COMMAND ${DEVKITPRO}/tools/bin/nacptool --create "${PROJECT_NAME}" "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}" ${PROJECT_NAME}.nacp
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/data_romfs"
            COMMAND ${DEVKITPRO}/tools/bin/elf2nro "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.nro" "--icon=${CMAKE_CURRENT_SOURCE_DIR}/data/${TARGET_PLATFORM}/icon.jpg" "--nacp=${PROJECT_NAME}.nacp" "--romfsdir=${CMAKE_CURRENT_BINARY_DIR}/data_romfs")
    add_custom_target(${PROJECT_NAME}_${TARGET_PLATFORM}_release
            DEPENDS ${PROJECT_NAME}.nro
            COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.nro ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/data_datadir -D DST=${CMAKE_BINARY_DIR}/release/${PROJECT_NAME} -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND cd ${CMAKE_BINARY_DIR}/release && ${ZIP} -r ../${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip ${PROJECT_NAME}
    )
endif (PLATFORM_SWITCH)

#####################
# 3DS target
#####################
if (PLATFORM_3DS)
    add_custom_target(${PROJECT_NAME}.3dsx
            DEPENDS ${PROJECT_NAME}
            DEPENDS ${PROJECT_NAME}.data
            COMMAND ${DEVKITPRO}/tools/bin/smdhtool --create "${PROJECT_NAME}" "${PROJECT_NAME}" "${PROJECT_AUTHOR}" "${CMAKE_CURRENT_SOURCE_DIR}/data/${TARGET_PLATFORM}/icon.png" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.smdh"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/data_romfs
            COMMAND ${DEVKITPRO}/tools/bin/3dsxtool "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.3dsx" "--smdh=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.smdh" "--romfs=${CMAKE_CURRENT_BINARY_DIR}/data_romfs")
    add_custom_target(${PROJECT_NAME}_${TARGET_PLATFORM}_release
            DEPENDS ${PROJECT_NAME}.3dsx
            COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.3dsx ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/data_datadir -D DST=${CMAKE_BINARY_DIR}/release/${PROJECT_NAME} -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND cd ${CMAKE_BINARY_DIR}/release && ${ZIP} -r ../${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip ${PROJECT_NAME}
    )
endif (PLATFORM_3DS)

#####################
# VITA target
#####################
if (PLATFORM_VITA)
    add_custom_target(${PROJECT_NAME}.vpk
            DEPENDS ${PROJECT_NAME}
            DEPENDS ${PROJECT_NAME}.data
            # create eboot
            COMMAND ${VITASDK}/bin/vita-elf-create ${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.velf
            COMMAND ${VITASDK}/bin/vita-make-fself -c ${PROJECT_NAME}.velf eboot.bin
            # create vpk
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/vpk
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/vpk
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/data_romfs -D DST=${CMAKE_CURRENT_BINARY_DIR}/vpk -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND ${CMAKE_COMMAND} -E copy eboot.bin ${CMAKE_CURRENT_BINARY_DIR}/vpk/eboot.bin
            COMMAND ${VITASDK}/bin/vita-mksfoex -s "TITLE_ID=${TITLE_ID}" -s "APP_VER=0${VERSION_MAJOR}.${VERSION_MINOR}0" -d "ATTRIBUTE2=12" "${PROJECT_NAME}" ${CMAKE_CURRENT_BINARY_DIR}/vpk/sce_sys/param.sfo
            COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/vpk && ${ZIP} -r ../${PROJECT_NAME}.vpk .
    )
    add_custom_target(${PROJECT_NAME}_${TARGET_PLATFORM}_release
            DEPENDS ${PROJECT_NAME}.vpk
            COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vpk ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/data_datadir -D DST=${CMAKE_BINARY_DIR}/release/${PROJECT_NAME} -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND cd ${CMAKE_BINARY_DIR}/release && ${ZIP} -r ../${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip ${PROJECT_NAME}
    )
endif (PLATFORM_VITA)

###########################
# PS4 target
###########################
if (PLATFORM_PS4)
    string(REPLACE "." "" PS4_PKG_VERSION_CLEAN ${PS4_PKG_VERSION})
    add_self(${PROJECT_NAME})
    add_pkg(${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/data_romfs ${PS4_PKG_TITLE_ID} ${PS4_PKG_TITLE} ${PS4_PKG_VERSION})
    add_dependencies(${PROJECT_NAME}_pkg ${PROJECT_NAME}.data)
    add_custom_target(${PROJECT_NAME}_${TARGET_PLATFORM}_release
            DEPENDS ${PROJECT_NAME}_pkg
            COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${PKG_OUT_NAME} ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/data_datadir -D DST=${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/data/${PROJECT_NAME} -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND cd ${CMAKE_BINARY_DIR}/release && ${ZIP} -r ../${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip ${PROJECT_NAME}
    )
endif (PLATFORM_PS4)

if (PLATFORM_PS5)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
    add_custom_target(${PROJECT_NAME}_${TARGET_PLATFORM}_release
            DEPENDS ${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E make_directory   ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}" ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/
            COMMAND ${CMAKE_COMMAND} -E copy "${PS5_PAYLOAD_SDK}/target/user/homebrew/lib/libOSMesa.so.8" ${CMAKE_BINARY_DIR}/release/${PROJECT_NAME}/
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/data_datadir -D DST=${CMAKE_BINARY_DIR}/release/${PROJECT_NAME} -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/data_romfs   -D DST=${CMAKE_BINARY_DIR}/release/${PROJECT_NAME} -P ${CMAKE_CURRENT_LIST_DIR}/copy_custom.cmake
            COMMAND cd ${CMAKE_BINARY_DIR}/release && ${ZIP} -r ../${PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}_${TARGET_PLATFORM}.zip ${PROJECT_NAME}
    )
endif (PLATFORM_PS5)

###########################
# Sony ps3 target
###########################
if (PLATFORM_PS3)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
    add_custom_target(${PROJECT_NAME}.self
            DEPENDS ${PROJECT_NAME}
            DEPENDS ${PROJECT_NAME}.data
            COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME} ${PROJECT_NAME}.sprx
            COMMAND ${PSL1GHT}/bin/sprxlinker ${PROJECT_NAME}.sprx
            COMMAND ${PSL1GHT}/bin/make_self ${PROJECT_NAME}.sprx ${PROJECT_NAME}.self
    )
endif (PLATFORM_PS3)

###########################
# Dreamcast target
###########################
if (PLATFORM_DREAMCAST)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
    # romdisk handling (tricky..)
    add_custom_target(
            ${PROJECT_NAME}.romdisk ALL
            DEPENDS dummy_romdisk
    )
    add_custom_command(OUTPUT
            dummy_romdisk ${CMAKE_BINARY_DIR}/romdisk.o
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/data_romfs
            COMMAND ${KOS_BASE}/utils/genromfs/genromfs -f ${CMAKE_BINARY_DIR}/romdisk.img -d ${CMAKE_CURRENT_BINARY_DIR}/data_romfs -v
            COMMAND KOS_ARCH=${KOS_ARCH} KOS_AS=${KOS_AS} KOS_AFLAGS=${KOS_AFLAGS} KOS_LD=${KOS_LD} KOS_OBJCOPY=${KOS_OBJCOPY} /bin/bash ${KOS_BASE}/utils/bin2o/bin2o ${CMAKE_BINARY_DIR}/romdisk.img romdisk ${CMAKE_BINARY_DIR}/romdisk.o
    )
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/romdisk.o)
    add_custom_target(${PROJECT_NAME}.bin
            DEPENDS ${PROJECT_NAME}
            DEPENDS ${PROJECT_NAME}.data
            COMMAND ${CMAKE_OBJCOPY} -R .stack -O binary "${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}" ${PROJECT_NAME}.bin
    )
    add_custom_target(${PROJECT_NAME}.cdi
            DEPENDS ${PROJECT_NAME}.bin
            COMMAND ${KOS_BASE}/utils/scramble/scramble ${PROJECT_NAME}.bin 1DS_CORE.BIN
            COMMAND genisoimage -V ${PROJECT_NAME} -G ${CMAKE_SOURCE_DIR}/data/dreamcast/IP.BIN -joliet -rock -l -x .svn -o ${PROJECT_NAME}.iso 1DS_CORE.BIN ${CMAKE_CURRENT_BINARY_DIR}/data_datadir
            COMMAND cdi4dc ${PROJECT_NAME}.iso ${PROJECT_NAME}.cdi -d >> cdi4dc.log
    )
endif (PLATFORM_DREAMCAST)
