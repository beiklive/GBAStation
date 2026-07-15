cmake_minimum_required(VERSION 3.25)

if (NOT DEFINED AZAHAR_SOURCE_DIR OR NOT DEFINED STAGE_ROOT OR NOT DEFINED THREEDS_STUB_DIR)
    message(FATAL_ERROR "PrepareAzaharSource.cmake requires AZAHAR_SOURCE_DIR, STAGE_ROOT and THREEDS_STUB_DIR")
endif()

set(STAGE_SOURCE ${STAGE_ROOT}/source)
set(DOWNLOAD_DIR ${STAGE_ROOT}/downloads)
file(MAKE_DIRECTORY ${STAGE_SOURCE} ${DOWNLOAD_DIR})
set(BASE_SOURCE_MARKER ${STAGE_SOURCE}/.gbastation-azahar-2125.1.3-v2)
if (NOT EXISTS ${BASE_SOURCE_MARKER})
    file(COPY ${AZAHAR_SOURCE_DIR}/ DESTINATION ${STAGE_SOURCE})
    file(WRITE ${BASE_SOURCE_MARKER} "2125.1.3\n")
endif()

function(write_if_different path contents)
    if (EXISTS ${path})
        file(READ ${path} current_contents)
        if (current_contents STREQUAL contents)
            return()
        endif()
    endif()
    file(WRITE ${path} "${contents}")
endfunction()

function(fetch_github_archive owner repository commit destination)
    set(marker ${destination}/.gbastation-commit)
    if (EXISTS ${marker})
        file(READ ${marker} installed_commit)
        string(STRIP "${installed_commit}" installed_commit)
        if (installed_commit STREQUAL commit)
            return()
        endif()
    endif()

    if (owner STREQUAL "azahar-emu" AND repository STREQUAL "soundtouch")
        set(archive ${DOWNLOAD_DIR}/${owner}-${repository}-${commit}.zip)
        set(url https://codeload.github.com/${owner}/${repository}/zip/${commit})
    else()
        set(archive ${DOWNLOAD_DIR}/${owner}-${repository}-${commit}.tar.gz)
        set(url https://codeload.github.com/${owner}/${repository}/tar.gz/${commit})
    endif()
    if (NOT EXISTS ${archive})
        message(STATUS "Downloading ${owner}/${repository}@${commit}")
        file(DOWNLOAD ${url} ${archive}
            STATUS download_status
            SHOW_PROGRESS
            TLS_VERIFY ON
        )
        list(GET download_status 0 download_code)
        list(GET download_status 1 download_message)
        if (NOT download_code EQUAL 0)
            file(REMOVE ${archive})
            message(FATAL_ERROR "Failed to download ${url}: ${download_message}")
        endif()
    endif()

    string(MD5 extract_id "${owner}/${repository}/${commit}")
    set(extract_dir ${DOWNLOAD_DIR}/extract-${extract_id})
    file(REMOVE_RECURSE ${extract_dir})
    file(MAKE_DIRECTORY ${extract_dir})
    if (owner STREQUAL "azahar-emu" AND repository STREQUAL "soundtouch")
        find_program(archive_tar NAMES bsdtar REQUIRED)
        execute_process(
            COMMAND ${archive_tar} -xf ${archive}
                    --exclude=*/source/SoundTouchDLL/LazarusTest/libSoundTouchDll.so
                    -C ${extract_dir}
            RESULT_VARIABLE extract_result
        )
    elseif (owner STREQUAL "facebook" AND repository STREQUAL "zstd")
        find_program(archive_tar NAMES bsdtar REQUIRED)
        execute_process(
            COMMAND ${archive_tar} -xf ${archive}
                    --exclude=*/tests/cli-tests/bin/unzstd
                    --exclude=*/tests/cli-tests/bin/zstdcat
                    -C ${extract_dir}
            RESULT_VARIABLE extract_result
        )
        if (NOT extract_result EQUAL 0)
            message(FATAL_ERROR "Failed to extract ${archive}")
        endif()
    else()
        file(ARCHIVE_EXTRACT INPUT ${archive} DESTINATION ${extract_dir})
    endif()
    file(GLOB extracted_roots LIST_DIRECTORIES TRUE ${extract_dir}/*)
    list(LENGTH extracted_roots extracted_count)
    if (NOT extracted_count EQUAL 1)
        message(FATAL_ERROR "Unexpected archive layout for ${owner}/${repository}@${commit}")
    endif()
    list(GET extracted_roots 0 extracted_root)

    file(REMOVE_RECURSE ${destination})
    file(MAKE_DIRECTORY ${destination})
    file(COPY ${extracted_root}/ DESTINATION ${destination})
    file(WRITE ${marker} "${commit}\n")
    file(REMOVE_RECURSE ${extract_dir})
endfunction()

# Azahar 2125.1.3 submodule revisions, taken from the official tag tree.
fetch_github_archive(azahar-emu ext-boost 6a85c3100499e886e11c87a5c2109eedacea0a61 ${STAGE_SOURCE}/externals/boost)
fetch_github_archive(weidai11 cryptopp 60f81a77e0c9a0e7ffc1ca1bc438ddfa2e43b78e ${STAGE_SOURCE}/externals/cryptopp)
fetch_github_archive(abdes cryptopp-cmake 00a151f8489daaa32434ab1f340e6750793ddf0c ${STAGE_SOURCE}/externals/cryptopp-cmake)
fetch_github_archive(septag dds-ktx c3ca8febc2457ab5c581604f3236a8a511fc2e45 ${STAGE_SOURCE}/externals/dds-ktx)
fetch_github_archive(azahar-emu dynarmic e77b1ba0b7da7cbe93021b01a663acfe7c4dd516 ${STAGE_SOURCE}/externals/dynarmic)
fetch_github_archive(lsalzman enet 2662c0de09e36f2a2030ccc2c528a3e4c9e8138a ${STAGE_SOURCE}/externals/enet)
fetch_github_archive(fmtlib fmt e424e3f2e607da02742f73db84873b8084fc714c ${STAGE_SOURCE}/externals/fmt)
fetch_github_archive(azahar-emu ext-library-headers 3b3e28dbe6d033395ce2967fa8030825e7b89de7 ${STAGE_SOURCE}/externals/library-headers)
fetch_github_archive(azahar-emu ext-libressl-portable 88b8e41b71099fabc57813bc06d8bc1aba050a19 ${STAGE_SOURCE}/externals/libressl)
fetch_github_archive(neobrain nihstro f4d8659decbfe5d234f04134b5002b82dc515a44 ${STAGE_SOURCE}/externals/nihstro)
fetch_github_archive(merryhime oaknut 6b1d57ea7ed4882d32a91eeaa6557b0ecb4da152 ${STAGE_SOURCE}/externals/oaknut)
fetch_github_archive(azahar-emu soundtouch 9ef8458d8561d9471dd20e9619e3be4cfe564796 ${STAGE_SOURCE}/externals/soundtouch)
fetch_github_archive(wwylele teakra 3d697a18df504f4677b65129d9ab14c7c597e3eb ${STAGE_SOURCE}/externals/teakra)
fetch_github_archive(Cyan4973 xxHash e626a72bc2321cd320e953a0ccf1584cad60f363 ${STAGE_SOURCE}/externals/xxHash)
fetch_github_archive(facebook zstd f8745da6ff1ad1e7bab384bd1f9d742439278e99 ${STAGE_SOURCE}/externals/zstd)

# Nested Azahar submodules.
fetch_github_archive(knik0 faad2 216f00e8ddba6f2c64caf481a04f1ddd78b93e78 ${STAGE_SOURCE}/externals/faad2/faad2)
fetch_github_archive(benhoyt inih 5cc5e2c24642513aaa5b19126aad42d0e4e0923e ${STAGE_SOURCE}/externals/inih/inih)
fetch_github_archive(lvandeve lodepng 0b1d9ccfc2093e5d6620cd9a11d03ee6ff6705f5 ${STAGE_SOURCE}/externals/lodepng/lodepng)

# Dynarmic ARM64 backend dependencies not supplied by the parent Azahar targets.
fetch_github_archive(azahar-emu mcl 5fc4beaf331037649b10625736b41365defb4f50 ${STAGE_SOURCE}/externals/dynarmic/externals/mcl)
fetch_github_archive(Tessil robin-map 054ec5ad67440fcd65e0497e5a27ef31f53fcc7f ${STAGE_SOURCE}/externals/dynarmic/externals/robin-map)

configure_file(
    ${THREEDS_STUB_DIR}/patches/oaknut/code_block.hpp
    ${STAGE_SOURCE}/externals/oaknut/include/oaknut/code_block.hpp
    COPYONLY
)

foreach(dynarmic_source IN ITEMS
        ${STAGE_SOURCE}/externals/dynarmic/src/dynarmic/backend/arm64/address_space.cpp
        ${STAGE_SOURCE}/externals/dynarmic/src/dynarmic/common/spin_lock_arm64.cpp)
    file(READ ${dynarmic_source} dynarmic_contents)
    string(REPLACE "mem.ptr(), mem.ptr()" "mem.ptr(), mem.xptr()"
        dynarmic_contents "${dynarmic_contents}")
    write_if_different(${dynarmic_source} "${dynarmic_contents}")
endforeach()

# The ARM64 backend normally invalidates its full 128 MiB code cache after emitting a small
# prelude. On Switch, invalidate only the bytes that were actually generated.
foreach(dynarmic_address_space IN ITEMS
        ${STAGE_SOURCE}/externals/dynarmic/src/dynarmic/backend/arm64/a32_address_space.cpp
        ${STAGE_SOURCE}/externals/dynarmic/src/dynarmic/backend/arm64/a64_address_space.cpp)
    file(READ ${dynarmic_address_space} dynarmic_address_space_contents)
    string(REPLACE
        "mem.invalidate_all();"
        "mem.invalidate(mem.ptr(), static_cast<std::size_t>(code.offset()));"
        dynarmic_address_space_contents
        "${dynarmic_address_space_contents}"
    )
    write_if_different(${dynarmic_address_space} "${dynarmic_address_space_contents}")
endforeach()

# A 128 MiB cache is excessive per emulated ARM core and per page table on Switch. 32 MiB is
# large enough for 3DS application code while substantially reducing committed JIT memory.
set(arm_dynarmic_source ${STAGE_SOURCE}/src/core/arm/dynarmic/arm_dynarmic.cpp)
file(READ ${arm_dynarmic_source} arm_dynarmic_contents)
if (NOT arm_dynarmic_contents MATCHES "config.code_cache_size = 32")
    string(REPLACE
        "    config.callbacks = cb.get();"
        "    config.callbacks = cb.get();\n#ifdef __SWITCH__\n    config.code_cache_size = 32 * 1024 * 1024;\n#endif"
        arm_dynarmic_contents
        "${arm_dynarmic_contents}"
    )
endif()
write_if_different(${arm_dynarmic_source} "${arm_dynarmic_contents}")

# Oaknut uses separate writable and executable aliases on Switch. The ARM64 Pica shader JIT must
# write through ptr() but call and branch through xptr(). Upstream assumes both aliases are equal.
foreach(shader_jit_source IN ITEMS
        ${STAGE_SOURCE}/src/video_core/shader/shader_jit_a64_compiler.cpp
        ${STAGE_SOURCE}/src/video_core/shader/shader_jit_a64_compiler.h)
    file(READ ${shader_jit_source} shader_jit_contents)
    string(REPLACE
        "code_mem->ptr()) +"
        "code_mem->xptr()) +"
        shader_jit_contents
        "${shader_jit_contents}"
    )
    write_if_different(${shader_jit_source} "${shader_jit_contents}")
endforeach()

set(common_error_source ${STAGE_SOURCE}/src/common/error.cpp)
file(READ ${common_error_source} common_error_contents)
string(REPLACE
    "#if defined(__GLIBC__) &&"
    "#if defined(__SWITCH__) || defined(__GLIBC__) &&"
    common_error_contents
    "${common_error_contents}"
)
write_if_different(${common_error_source} "${common_error_contents}")

# newlib does not expose the POSIX scheduler APIs used by Azahar. Use libnx's
# native thread-priority syscall and keep the values within Horizon's normal
# application priority range.
set(common_thread_source ${STAGE_SOURCE}/src/common/thread.cpp)
file(READ ${common_thread_source} common_thread_contents)
if (NOT common_thread_contents MATCHES "svcSetThreadPriority")
    string(REPLACE
        "#include \"common/thread.h\"\n"
        "#include \"common/thread.h\"\n#ifdef __SWITCH__\nextern \"C\" u32 svcSetThreadPriority(u32 handle, u32 priority);\n#endif\n"
        common_thread_contents
        "${common_thread_contents}"
    )
endif()
string(FIND "${common_thread_contents}" "constexpr u32 current_thread_handle" switch_priority_pos)
if (switch_priority_pos EQUAL -1)
    string(REPLACE
        "#else\n\nvoid SetCurrentThreadPriority(ThreadPriority new_priority) {\n    pthread_t this_thread = pthread_self();"
        "#elif defined(__SWITCH__)\n\nvoid SetCurrentThreadPriority(ThreadPriority new_priority) {\n    constexpr u32 current_thread_handle = 0xFFFF8000;\n    u32 priority = 0x2C;\n    switch (new_priority) {\n    case ThreadPriority::Low:\n        priority = 0x30;\n        break;\n    case ThreadPriority::High:\n        priority = 0x28;\n        break;\n    case ThreadPriority::VeryHigh:\n        priority = 0x24;\n        break;\n    case ThreadPriority::Critical:\n        priority = 0x20;\n        break;\n    default:\n        break;\n    }\n    svcSetThreadPriority(current_thread_handle, priority);\n}\n\n#else\n\nvoid SetCurrentThreadPriority(ThreadPriority new_priority) {\n    pthread_t this_thread = pthread_self();"
        common_thread_contents
        "${common_thread_contents}"
    )
endif()
string(REPLACE
    "void SetCurrentThreadName(const char* name) {\n#ifdef __APPLE__"
    "void SetCurrentThreadName(const char* name) {\n#ifdef __SWITCH__\n    (void)name;\n#elif defined(__APPLE__)"
    common_thread_contents
    "${common_thread_contents}"
)
write_if_different(${common_thread_source} "${common_thread_contents}")

set(gdbstub_source ${STAGE_SOURCE}/src/core/gdbstub/gdbstub.cpp)
file(READ ${gdbstub_source} gdbstub_contents)
if (NOT gdbstub_contents MATCHES "#include <arpa/inet.h>")
    string(REPLACE
        "#include <netinet/in.h>\n"
        "#include <arpa/inet.h>\n#include <netinet/in.h>\n"
        gdbstub_contents
        "${gdbstub_contents}"
    )
endif()
write_if_different(${gdbstub_source} "${gdbstub_contents}")

set(file_util_source ${STAGE_SOURCE}/src/common/file_util.cpp)
file(READ ${file_util_source} file_util_contents)
string(REPLACE
    "const std::string GetHomeDirectory() {\n    std::string home_path;"
    "const std::string GetHomeDirectory() {\n#ifdef __SWITCH__\n    return \"sdmc:/\";\n#else\n    std::string home_path;"
    file_util_contents
    "${file_util_contents}"
)
string(REPLACE
    "    return home_path;\n}\n\n/**\n * Follows the XDG Base Directory Specification"
    "    return home_path;\n#endif\n}\n\n/**\n * Follows the XDG Base Directory Specification"
    file_util_contents
    "${file_util_contents}"
)
string(FIND "${file_util_contents}"
    "#elif defined(__SWITCH__)\n    std::scoped_lock lock(m_file_pos_mutex)"
    switch_read_pos)
if (switch_read_pos EQUAL -1)
    string(REPLACE
        "#else\n    return pread(fileno(m_file), data, byte_count, offset);\n#endif"
        "#elif defined(__SWITCH__)\n    std::scoped_lock lock(m_file_pos_mutex);\n    const long position = std::ftell(m_file);\n    if (position < 0 || std::fseek(m_file, static_cast<long>(offset), SEEK_SET) != 0) {\n        return std::numeric_limits<std::size_t>::max();\n    }\n    const std::size_t read = std::fread(data, 1, byte_count, m_file);\n    std::fseek(m_file, position, SEEK_SET);\n    return read;\n#else\n    return pread(fileno(m_file), data, byte_count, offset);\n#endif"
        file_util_contents
        "${file_util_contents}"
    )
endif()
write_if_different(${file_util_source} "${file_util_contents}")

set(file_util_header ${STAGE_SOURCE}/src/common/file_util.h)
file(READ ${file_util_header} file_util_header_contents)
string(REPLACE
    "#ifdef HAVE_LIBRETRO\n#include <mutex>\n#endif"
    "#if defined(HAVE_LIBRETRO) || defined(__SWITCH__)\n#include <mutex>\n#endif"
    file_util_header_contents
    "${file_util_header_contents}"
)
string(REPLACE
    "#ifdef HAVE_LIBRETRO_VFS\n    // pread() doesn't touch the file position"
    "#if defined(HAVE_LIBRETRO_VFS) || defined(__SWITCH__)\n    // pread() doesn't touch the file position"
    file_util_header_contents
    "${file_util_header_contents}"
)
write_if_different(${file_util_header} "${file_util_header_contents}")

foreach(random_source IN ITEMS
        src/core/movie.cpp
        src/core/hw/rsa/rsa.cpp
        src/core/hw/ecc.cpp
        src/core/hle/service/cfg/cfg.cpp
        src/core/hle/service/nfc/nfc_device.cpp)
    file(READ ${STAGE_SOURCE}/${random_source} random_contents)
    string(REPLACE "#include <cryptopp/osrng.h>"
                   "#include \"three_ds_stub/ThreeDSRandom.hpp\""
                   random_contents "${random_contents}")
    string(REPLACE "#include \"cryptopp/osrng.h\""
                   "#include \"three_ds_stub/ThreeDSRandom.hpp\""
                   random_contents "${random_contents}")
    write_if_different(${STAGE_SOURCE}/${random_source} "${random_contents}")
endforeach()

set(libressl_crypto_cmake ${STAGE_SOURCE}/externals/libressl/crypto/CMakeLists.txt)
file(READ ${libressl_crypto_cmake} libressl_crypto_contents)
string(REPLACE
    "if(UNIX)\n\tset(CRYPTO_SRC \${CRYPTO_SRC} crypto_lock.c)"
    "if(UNIX OR CMAKE_SYSTEM_NAME STREQUAL \"NintendoSwitch\")\n\tset(CRYPTO_SRC \${CRYPTO_SRC} crypto_lock.c)"
    libressl_crypto_contents
    "${libressl_crypto_contents}"
)
string(REPLACE
    "\tset(CRYPTO_SRC \${CRYPTO_SRC} ui/ui_openssl.c)\n"
    ""
    libressl_crypto_contents
    "${libressl_crypto_contents}"
)
write_if_different(${libressl_crypto_cmake} "${libressl_crypto_contents}")

# Boost mapped_file requires mmap(), which newlib on Switch does not provide. Azahar's
# software-renderer build only uses file_descriptor, so leave that implementation enabled.
set(azahar_externals_cmake ${STAGE_SOURCE}/externals/CMakeLists.txt)
file(READ ${azahar_externals_cmake} azahar_externals_contents)
string(REPLACE
    "        \${CMAKE_SOURCE_DIR}/externals/boost/libs/iostreams/src/mapped_file.cpp\n"
    ""
    azahar_externals_contents
    "${azahar_externals_contents}"
)
write_if_different(${azahar_externals_cmake} "${azahar_externals_contents}")

# The compatibility list is a UI-only submodule, but upstream configures its qrc unconditionally.
file(MAKE_DIRECTORY ${STAGE_SOURCE}/dist/compatibility_list)
if (NOT EXISTS ${STAGE_SOURCE}/dist/compatibility_list/compatibility_list.qrc)
    file(WRITE ${STAGE_SOURCE}/dist/compatibility_list/compatibility_list.qrc "<RCC/>\n")
endif()
if (NOT EXISTS ${STAGE_SOURCE}/dist/compatibility_list/compatibility_list.json)
    file(WRITE ${STAGE_SOURCE}/dist/compatibility_list/compatibility_list.json "[]\n")
endif()

# Inject the standalone frontend after the upstream core targets have been declared.
file(READ ${STAGE_SOURCE}/CMakeLists.txt azahar_root_cmake)
if (NOT azahar_root_cmake MATCHES "GBASTATION_3DS_STUB_IN_TREE")
    file(APPEND ${STAGE_SOURCE}/CMakeLists.txt
        "\nif(GBASTATION_3DS_STUB_IN_TREE)\n"
        "  add_subdirectory(\"\${GBASTATION_3DS_STUB_DIR}\" \"\${CMAKE_BINARY_DIR}/../stub\")\n"
        "endif()\n"
    )
endif()
