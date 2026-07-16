// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <vector>
#include <glad/glad.h>
#ifdef __SWITCH__
extern "C" void (*eglGetProcAddress(const char* procname))();
#endif
#include "common/assert.h"
#include "common/logging/log.h"
#include "video_core/renderer_opengl/gl_shader_util.h"
#include "video_core/renderer_opengl/gl_vars.h"

namespace OpenGL {

namespace {

#ifndef GL_COMPLETION_STATUS_KHR
constexpr GLenum GL_COMPLETION_STATUS_KHR = 0x91B1;
#endif

using MaxShaderCompilerThreadsProc = void (*)(GLuint);
MaxShaderCompilerThreadsProc max_shader_compiler_threads{};
int parallel_compile_support = -1;

bool HasExtension(const char* wanted) {
    GLint count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &count);
    for (GLint index = 0; index < count; ++index) {
        const auto* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, index));
        if (extension && std::strcmp(extension, wanted) == 0) {
            return true;
        }
    }
    return false;
}

} // Anonymous namespace

bool SupportsParallelShaderCompile() {
    if (parallel_compile_support >= 0) {
        return parallel_compile_support != 0;
    }

#ifdef __SWITCH__
    const bool has_khr = HasExtension("GL_KHR_parallel_shader_compile");
    const bool has_arb = HasExtension("GL_ARB_parallel_shader_compile");
    if (has_khr) {
        max_shader_compiler_threads = reinterpret_cast<MaxShaderCompilerThreadsProc>(
            eglGetProcAddress("glMaxShaderCompilerThreadsKHR"));
    }
    if (!max_shader_compiler_threads && has_arb) {
        max_shader_compiler_threads = reinterpret_cast<MaxShaderCompilerThreadsProc>(
            eglGetProcAddress("glMaxShaderCompilerThreadsARB"));
    }
    parallel_compile_support = max_shader_compiler_threads ? 1 : 0;
    if (max_shader_compiler_threads) {
        // Two compiler workers occupy the otherwise idle Switch CPU cores while emulation and
        // presentation continue on their existing cores.
        max_shader_compiler_threads(2);
        LOG_INFO(Render_OpenGL, "Enabled parallel shader compilation with 2 workers");
    } else {
        LOG_WARNING(Render_OpenGL,
                    "Parallel shader compilation extension unavailable; using synchronous GLSL");
    }
#else
    parallel_compile_support = 0;
#endif
    return parallel_compile_support != 0;
}

bool FinishAsyncProgram(GLuint program) {
    GLint complete = GL_FALSE;
    glGetProgramiv(program, GL_COMPLETION_STATUS_KHR, &complete);
    if (complete != GL_TRUE) {
        return false;
    }

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint info_log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
        std::vector<char> error(std::max(info_log_length, 1));
        if (info_log_length > 1) {
            glGetProgramInfoLog(program, info_log_length, nullptr, error.data());
        }
        LOG_ERROR(Render_OpenGL, "Asynchronous shader link failed: {}", error.data());
    }
    return linked == GL_TRUE;
}

GLuint LoadShader(std::string_view source, GLenum type, const std::string& debug_name,
                  bool async_compile) {
    std::string preamble;
    if (GLES) {
        preamble = R"(#version 320 es

#if defined(GL_ANDROID_extension_pack_es31a)
#extension GL_ANDROID_extension_pack_es31a : enable
#endif // defined(GL_ANDROID_extension_pack_es31a)

#if defined(GL_EXT_clip_cull_distance)
#extension GL_EXT_clip_cull_distance : enable
#endif // defined(GL_EXT_clip_cull_distance)
)";
    } else {
        preamble = "#version 430 core\n";
    }

    std::string_view debug_type;
    switch (type) {
    case GL_VERTEX_SHADER:
        debug_type = "vertex";
        break;
    case GL_GEOMETRY_SHADER:
        debug_type = "geometry";
        break;
    case GL_FRAGMENT_SHADER:
        debug_type = "fragment";
        break;
    default:
        UNREACHABLE();
    }

    std::array<const GLchar*, 2> src_arr{preamble.data(), source.data()};
    std::array<GLint, 2> lengths{static_cast<GLint>(preamble.size()),
                                 static_cast<GLint>(source.size())};
    GLuint shader_id = glCreateShader(type);
    if (shader_id == 0) {
        GLenum err = glGetError();
        LOG_ERROR(Render_OpenGL, "glCreateShader failed for {} shader {} (err {})", debug_type,
                  debug_name, err);
        return shader_id;
    }

    glShaderSource(shader_id, static_cast<GLsizei>(src_arr.size()), src_arr.data(), lengths.data());
    LOG_DEBUG(Render_OpenGL, "Compiling {} shader {}...", debug_type, debug_name);
    glCompileShader(shader_id);

    if (async_compile && SupportsParallelShaderCompile()) {
        return shader_id;
    }

    GLint result = GL_FALSE;
    GLint info_log_length;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1) {
        std::vector<char> shader_error(info_log_length);
        glGetShaderInfoLog(shader_id, info_log_length, nullptr, &shader_error[0]);
        if (result == GL_TRUE) {
            LOG_DEBUG(Render_OpenGL, "Compile message for {} shader {}:\n{}", debug_type,
                      debug_name, &shader_error[0]);
        } else {
            LOG_ERROR(Render_OpenGL, "Error compiling {} shader {}:\n{}", debug_type, debug_name,
                      &shader_error[0]);
            LOG_ERROR(Render_OpenGL, "Shader source code:\n{}{}", src_arr[0], src_arr[1]);
        }
    } else if (result == GL_FALSE) {
        LOG_ERROR(Render_OpenGL, "Error compiling {} shader {}:\nNo log produced.", debug_type,
                  debug_name);
    }
    return shader_id;
}

GLuint LoadProgram(bool separable_program, std::span<const GLuint> shaders,
                   const std::string& debug_name, bool async_compile) {
    // Link the program
    LOG_DEBUG(Render_OpenGL, "Linking program...");

    GLuint program_id = glCreateProgram();

    for (GLuint shader : shaders) {
        if (shader != 0) {
            glAttachShader(program_id, shader);
        }
    }

    if (separable_program) {
        glProgramParameteri(program_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
    }

    glProgramParameteri(program_id, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
    glLinkProgram(program_id);

    if (async_compile && SupportsParallelShaderCompile()) {
        for (GLuint shader : shaders) {
            if (shader != 0) {
                glDetachShader(program_id, shader);
            }
        }
        return program_id;
    }

    // Check the program
    GLint result = GL_FALSE;
    GLint info_log_length;
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1) {
        std::vector<char> program_error(info_log_length);
        glGetProgramInfoLog(program_id, info_log_length, nullptr, &program_error[0]);
        if (result == GL_TRUE) {
            LOG_DEBUG(Render_OpenGL, "Link message for shader {}:\n{}", debug_name,
                      &program_error[0]);
        } else {
            LOG_ERROR(Render_OpenGL, "Error linking shader {}:\n{}", debug_name, &program_error[0]);
        }
    } else if (result == GL_FALSE) {
        LOG_ERROR(Render_OpenGL, "Error linking shader {}: No log produced.", debug_name);
    }

    for (GLuint shader : shaders) {
        if (shader != 0) {
            glDetachShader(program_id, shader);
        }
    }

    return program_id;
}

} // namespace OpenGL
