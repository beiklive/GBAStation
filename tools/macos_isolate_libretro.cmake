# 在 macOS 上把静态模拟器核心合并成单一 relocatable，并模拟 GNU objcopy 的
# --keep-global-symbol 行为，避免多核心静态链接时的符号冲突。
#
# 流程：
#   1) 用 Apple ld64 `-r -all_load` 把 RAWA 静态库成员合并到 tmp 对象；
#   2) 用 nm 列出全部全局符号，仅保留命中 KEEPFILE 规则的符号为全局；
#   3) 用 Apple ld64 `-r -exported_symbols_list` 把其余符号本地化，输出 OUT。
#
# 参数：
#   RAWA    - 原始静态库（raw archive）路径
#   OUT     - 输出对象
#   KEEPFILE- 每行一个需要保持为全局的符号名/模式；以 * 结尾表示前缀通配。
#             C 符号如 nestopia_retro_run 对应 Mach-O 符号 _nestopia_retro_run；
#             C++ 符号如 _ZN3Nes* 对应 Mach-O 符号 __ZN3Nes*（多一个前导下划线）。
#   ARCH    - 架构（arm64/x86_64），空则用 uname -m 推断
#   MINOS   - macOS 最低版本（例如 10.15 / 11.0）

if(NOT DEFINED RAWA OR NOT DEFINED OUT OR NOT DEFINED KEEPFILE)
    message(FATAL_ERROR "usage: cmake -DRAWA=... -DOUT=... -DKEEPFILE=... -DARCH=... -DMINOS=... -P macos_isolate_libretro.cmake")
endif()

# VERBATIM 会把自定义命令参数里的 " 作为字面字符，这里统一剥掉。
foreach(_v IN ITEMS RAWA OUT KEEPFILE ARCH MINOS)
    string(REPLACE "\"" "" "${_v}" "${${_v}}")
endforeach()

set(_arch "${ARCH}")
if(NOT _arch)
    execute_process(COMMAND uname -m OUTPUT_VARIABLE _uname OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_uname STREQUAL "x86_64")
        set(_arch "x86_64")
    else()
        set(_arch "arm64")
    endif()
endif()

if(NOT MINOS)
    set(MINOS "10.15")
endif()

set(_tmp "${OUT}.tmp.o")
file(REMOVE "${_tmp}" "${OUT}")

# 1) 合并静态库全部成员为一个 relocatable
execute_process(
    COMMAND ld -r -arch "${_arch}" -platform_version macos "${MINOS}" "${MINOS}"
            -all_load "${RAWA}" -o "${_tmp}"
    RESULT_VARIABLE _ld_rc
    ERROR_VARIABLE _ld_err
)
if(NOT _ld_rc EQUAL 0)
    message(FATAL_ERROR "ld -r -all_load failed: ${_ld_err}")
endif()
if(NOT EXISTS "${_tmp}")
    message(FATAL_ERROR "ld -r -all_load produced no output for ${RAWA}")
endif()

# 读取需要保持全局的符号模式
set(KEEP)
if(EXISTS "${KEEPFILE}")
    file(STRINGS "${KEEPFILE}" KEEP)
endif()

# 2) 解析 nm -g 输出，收集已定义的全局符号名
execute_process(
    COMMAND nm -g "${_tmp}"
    OUTPUT_VARIABLE _nm_out
    RESULT_VARIABLE _nm_rc
    ERROR_VARIABLE _nm_err
)
if(NOT _nm_rc EQUAL 0)
    message(FATAL_ERROR "nm failed: ${_nm_err}")
endif()

string(REPLACE "\n" ";" _nm_lines "${_nm_out}")

set(_matched)
foreach(_line IN LISTS _nm_lines)
    # 行形如: "0000... T _name" 或 " T _name"；跳过 (undefined)
    if(_line MATCHES "^[0-9a-fA-F]+[ \t]+([A-Z]) ([^ ]+)$")
        set(_type "${CMAKE_MATCH_1}")
        set(_name "${CMAKE_MATCH_2}")
    elseif(_line MATCHES "^[ \t]*([A-Z]) ([^ ]+)$")
        set(_type "${CMAKE_MATCH_1}")
        set(_name "${CMAKE_MATCH_2}")
    else()
        continue()
    endif()

    if(_type STREQUAL "U")
        continue()
    endif()

    set(_ok FALSE)
    foreach(_pat IN LISTS KEEP)
        if(_pat MATCHES "\\*$")
            string(LENGTH "${_pat}" _plen)
            math(EXPR _pplen "${_plen} - 1")
            string(SUBSTRING "${_pat}" 0 "${_pplen}" _prefix)
            string(FIND "${_name}" "_${_prefix}" _pos)
            if(_pos EQUAL 0)
                set(_ok TRUE)
            endif()
        else()
            if(_name STREQUAL "_${_pat}")
                set(_ok TRUE)
            endif()
        endif()
        if(_ok)
            break()
        endif()
    endforeach()

    if(_ok)
        list(APPEND _matched "${_name}")
    endif()
endforeach()

if(NOT _matched)
    message(FATAL_ERROR "no symbols matched keep list for ${_tmp}; output would be unusable")
endif()

set(_keep_file "${OUT}.keep.txt")
file(WRITE "${_keep_file}" "")
foreach(_name IN LISTS _matched)
    file(APPEND "${_keep_file}" "${_name}\n")
endforeach()

# 3) 本地化：仅保留 keep 列表中的符号为全局
execute_process(
    COMMAND ld -r -arch "${_arch}" -platform_version macos "${MINOS}" "${MINOS}"
            -exported_symbols_list "${_keep_file}" -o "${OUT}" "${_tmp}"
    RESULT_VARIABLE _localize_rc
    ERROR_VARIABLE _localize_err
)
file(REMOVE "${_keep_file}")
file(REMOVE "${_tmp}")
if(NOT _localize_rc EQUAL 0)
    message(FATAL_ERROR "ld -r -exported_symbols_list failed: ${_localize_err}")
endif()

list(LENGTH _matched _kept_count)
message(STATUS "isolated ${OUT} (kept ${_kept_count} global symbols)")
