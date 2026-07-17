cmake_minimum_required(VERSION 3.25)

project(gbastation_uam_runtime LANGUAGES C CXX)

find_program(UAM_RUNTIME_BISON NAMES bison REQUIRED)
find_program(UAM_RUNTIME_FLEX NAMES flex REQUIRED)

set(UAM_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(UAM_GENERATED_GLSL_DIR ${UAM_GENERATED_DIR}/glsl)
set(UAM_GENERATED_GLCPP_DIR ${UAM_GENERATED_GLSL_DIR}/glcpp)
file(MAKE_DIRECTORY ${UAM_GENERATED_GLSL_DIR} ${UAM_GENERATED_GLCPP_DIR})

execute_process(
    COMMAND ${UAM_RUNTIME_BISON}
        -o ${UAM_GENERATED_GLSL_DIR}/glsl_parser.cpp
        -p _mesa_glsl_
        --defines=${UAM_GENERATED_GLSL_DIR}/glsl_parser.h
        ${CMAKE_CURRENT_SOURCE_DIR}/mesa-imported/glsl/glsl_parser.yy
    RESULT_VARIABLE UAM_BISON_GLSL_RESULT
)
execute_process(
    COMMAND ${UAM_RUNTIME_FLEX}
        -o ${UAM_GENERATED_GLSL_DIR}/glsl_lexer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mesa-imported/glsl/glsl_lexer.ll
    RESULT_VARIABLE UAM_FLEX_GLSL_RESULT
)
execute_process(
    COMMAND ${UAM_RUNTIME_BISON}
        -o ${UAM_GENERATED_GLCPP_DIR}/glcpp-parse.c
        -p glcpp_parser_
        --defines=${UAM_GENERATED_GLCPP_DIR}/glcpp-parse.h
        ${CMAKE_CURRENT_SOURCE_DIR}/mesa-imported/glsl/glcpp/glcpp-parse.y
    RESULT_VARIABLE UAM_BISON_GLCPP_RESULT
)
execute_process(
    COMMAND ${UAM_RUNTIME_FLEX}
        -o ${UAM_GENERATED_GLCPP_DIR}/glcpp-lex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/mesa-imported/glsl/glcpp/glcpp-lex.l
    RESULT_VARIABLE UAM_FLEX_GLCPP_RESULT
)

foreach(UAM_GENERATOR_RESULT IN ITEMS
        UAM_BISON_GLSL_RESULT UAM_FLEX_GLSL_RESULT UAM_BISON_GLCPP_RESULT UAM_FLEX_GLCPP_RESULT)
    if (NOT ${${UAM_GENERATOR_RESULT}} EQUAL 0)
        message(FATAL_ERROR "uam runtime parser generation failed: ${UAM_GENERATOR_RESULT}")
    endif()
endforeach()

file(GLOB_RECURSE UAM_RUNTIME_MESA_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/mesa-imported/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mesa-imported/*.cpp
)

add_library(gbastation_uam_runtime STATIC
    ${UAM_RUNTIME_MESA_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/source/compiler_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/glsl_frontend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/mini-os.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/runtime_compiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/tgsi_support.cpp
    ${UAM_GENERATED_GLSL_DIR}/glsl_parser.cpp
    ${UAM_GENERATED_GLSL_DIR}/glsl_lexer.cpp
    ${UAM_GENERATED_GLCPP_DIR}/glcpp-parse.c
    ${UAM_GENERATED_GLCPP_DIR}/glcpp-lex.c
)

target_include_directories(gbastation_uam_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_SOURCE_DIR}/mesa-imported
    ${UAM_GENERATED_DIR}
    ${UAM_GENERATED_GLSL_DIR}
    ${UAM_GENERATED_GLCPP_DIR}
)
target_compile_definitions(gbastation_uam_runtime PRIVATE
    DESKTOP
    _GNU_SOURCE
    _USE_MATH_DEFINES
    HAVE_POSIX_MEMALIGN
    __STDC_FORMAT_MACROS
    PACKAGE_STRING="uam-runtime-1.1.0"
)
target_compile_options(gbastation_uam_runtime PRIVATE
    -O2
    -ffunction-sections
    -fdata-sections
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-class-memaccess;-Wno-non-virtual-dtor>
)
set_target_properties(gbastation_uam_runtime PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
