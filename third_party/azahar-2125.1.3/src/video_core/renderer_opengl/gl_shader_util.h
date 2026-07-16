// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <span>
#include <glad/glad.h>

namespace OpenGL {

/**
 * Utility function to create and compile an OpenGL GLSL shader
 * @param source String of the GLSL shader program
 * @param type Type of the shader (GL_VERTEX_SHADER, GL_GEOMETRY_SHADER or GL_FRAGMENT_SHADER)
 * @param debug_name debug name to show in logs
 */
GLuint LoadShader(std::string_view source, GLenum type, const std::string& debug_name,
                  bool async_compile = false);

/**
 * Utility function to create and link an OpenGL GLSL shader program
 * @param separable_program whether to create a separable program
 * @param shaders ID of shaders to attach to the program
 * @param debug_name debug name to show in logs
 * @returns Handle of the newly created OpenGL program object
 */
GLuint LoadProgram(bool separable_program, std::span<const GLuint> shaders,
                   const std::string& debug_name, bool async_compile = false);

/// Enables GL_KHR/ARB_parallel_shader_compile when the driver exposes it.
bool SupportsParallelShaderCompile();

/// Uses bounded batches so the old Switch nouveau driver cannot accumulate hundreds of pending
/// compiler jobs and exhaust memory during effect-heavy scenes.
bool ShouldUseParallelShaderCompile();

/// Non-blocking completion check followed by link validation for an asynchronously built program.
bool FinishAsyncProgram(GLuint program);

} // namespace OpenGL
