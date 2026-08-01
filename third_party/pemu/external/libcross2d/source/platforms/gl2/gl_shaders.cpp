//
// Created by cpasjuste on 27/01/17.
//

#ifdef __GL2__

#include "cross2d/c2d.h"

#ifdef GL_SHADERS_BINARY
#ifdef __PS4__
#include "../ps4/shaders/ps4_shaders.h"
#endif
#else

#include "shaders/shaders.h"

#endif

#ifdef GL_DUMP_SHADERS
#include "gl_shader_dumper.h"
static c2d::GLShaderDumper *shaderDumper;
#endif

#define GL_PROGRAM_BINARY_FMT 0

using namespace c2d;

static GLuint compile(GLenum type, const char *source, int size = 0) {
    GLuint handle;
    GLint compile_result;
    GLchar msg[512];

    GL_CHECK(handle = glCreateShader(type));
    if (!handle) {
        printf("GLShader::compile: glCreateShader failed (%s)\n",
               type == GL_VERTEX_SHADER ? "vertex" : "fragment");
        return 0;
    }

    if (size > 0) {
        //printf("compile: glShaderBinary: type: %s, handle = %i, size = %i\n",
        //       type == GL_VERTEX_SHADER ? "vertex" : "fragment", handle, size);
        glShaderBinary(1, &handle, 0, source, size);
    } else {
        GL_CHECK(glShaderSource(handle, 1, &source, nullptr));
        GL_CHECK(glCompileShader(handle));
    }

    GL_CHECK(glGetShaderiv(handle, GL_COMPILE_STATUS, &compile_result));
    if (!compile_result) {
        glGetShaderInfoLog(handle, sizeof(msg), nullptr, msg);
        printf("GLShader::compile: %u: %s\n", type, msg);
        glDeleteShader(handle);
        return 0;
    }

    return handle;
}

GLShader::GLShader(const std::string &n, const char *vertex, const char *fragment,
                   int vertexSize, int fragmentSize, const std::string &version)
    : ShaderList::Shader(n) {
    GLuint vsh, fsh;
    std::string v, f;
    GLint link_result;

    // use precompiled if size > 0
    if (vertexSize > 0) {
        //printf("GLShader::GLShader: %s (from binary)\n", name.c_str());
        vsh = compile(GL_VERTEX_SHADER, vertex, vertexSize);
        fsh = compile(GL_FRAGMENT_SHADER, fragment, fragmentSize);
    } else {
        //printf("GLShader::GLShader: %s, version: %s\n", name.c_str(), version.c_str());
        v = std::string("#version " + version + "\n") + vertex;
        f = std::string("#version " + version + "\n") + fragment;
        vsh = compile(GL_VERTEX_SHADER, v.c_str(), vertexSize);
        if (!vsh) {
            printf("GLShader: vsh compilation failed\n");
            return;
        }
        fsh = compile(GL_FRAGMENT_SHADER, f.c_str(), fragmentSize);
        if (!fsh) {
            glDeleteShader(vsh);
            printf("GLShader: fsh compilation failed\n");
            return;
        }
    }

    GL_CHECK(program = glCreateProgram());
    GL_CHECK(glAttachShader(program, vsh));
    GL_CHECK(glAttachShader(program, fsh));
    GL_CHECK(glBindAttribLocation(program, 0, "VertexCoord"));
    GL_CHECK(glBindAttribLocation(program, 1, "COLOR"));
    GL_CHECK(glBindAttribLocation(program, 2, "TexCoord"));
    GL_CHECK(glLinkProgram(program));
    GL_CHECK(glGetProgramiv(program, GL_LINK_STATUS, &link_result));
    if (!link_result) {
        char buf[512];
        glGetProgramInfoLog(program, sizeof(buf), nullptr, buf);
        printf("GLShader: Link error: %s\n", buf);
        glDeleteShader(vsh);
        glDeleteShader(fsh);
        return;
    }

#if defined(__PS4__) && defined(GL_DUMP_SHADERS)
        shaderDumper->dump((int) vsh, name, "v");
        shaderDumper->dump((int) fsh, name, "f");
#endif

    GL_CHECK(glDeleteShader(vsh));
    GL_CHECK(glDeleteShader(fsh));

    available = true;
}

GLShader::GLShader(const std::string &n, const char *source, int size,
                   const std::string &version) : ShaderList::Shader(n) {
    GLuint vsh, fsh;
    GLint link_result;
    std::string vertex, fragment;

    //printf("GLShader::GLShader: %s, version: %s, size: %i\n",
    //     name.c_str(), version.c_str(), size);

    GL_CHECK(program = glCreateProgram());

    // use shader from source is size <= 0, else binary program
    if (size <= 0) {
        vertex = std::string("#version " + version + "\n#define VERTEX\n") + source;
        fragment = std::string("#version " + version + "\n#define FRAGMENT\n") + source;
        vsh = compile(GL_VERTEX_SHADER, vertex.c_str(), 0);
        if (!vsh) {
            printf("GLShader::GLShader: %s (version: %s): vsh compilation failed: %i (glGetError: %i)\n",
                   name.c_str(), version.c_str(), vsh, glGetError());
            return;
        }
        fsh = compile(GL_FRAGMENT_SHADER, fragment.c_str(), 0);
        if (!fsh) {
            glDeleteShader(vsh);
            printf("GLShader::GLShader: %s (version: %s): fsh compilation failed: %i (glGetError: %i)\n",
                   name.c_str(), version.c_str(), fsh, glGetError());
            return;
        }
        GL_CHECK(glAttachShader(program, vsh));
        GL_CHECK(glAttachShader(program, fsh));
        GL_CHECK(glBindAttribLocation(program, 0, "VertexCoord"));
        GL_CHECK(glBindAttribLocation(program, 1, "COLOR"));
        GL_CHECK(glBindAttribLocation(program, 2, "TexCoord"));
        GL_CHECK(glLinkProgram(program));
#ifdef GL_DUMP_SHADERS
#ifdef __PS4__
        shaderDumper->dump((int) vsh, name, "v");
        shaderDumper->dump((int) fsh, name, "f");
#endif
#endif
        GL_CHECK(glDeleteShader(vsh));
        GL_CHECK(glDeleteShader(fsh));
    } else {
#if !defined(__PS4__) && defined(glProgramBinary)
        // use binary program (we should not reach this on ps4)
        GL_CHECK(glProgramBinary(program, GL_PROGRAM_BINARY_FMT, source, size));
#endif
    }

    glGetProgramiv(program, GL_LINK_STATUS, &link_result);
    if (!link_result) {
        char buf[512];
        glGetProgramInfoLog(program, sizeof(buf), nullptr, buf);
        printf("GLShader: %s, link error: %s\n", n.c_str(), buf);
        return;
    }

    //printf("GLShader::GLShader: %s (version: %s): success\n", name.c_str(), version.c_str());
    available = true;
}

void GLShader::SetUniformMatrix(const GLchar *n, const GLfloat *v) {
    GLint loc;
    GL_CHECK(loc = glGetUniformLocation(program, n));
    if (loc > -1) {
        GL_CHECK(glUniformMatrix4fv(loc, 1, GL_FALSE, v));
    }
}

void GLShader::SetUniform(const GLchar *n, const Vector2f &v) {
    GLint loc;
    GL_CHECK(loc = glGetUniformLocation(program, n));
    if (loc > -1) {
        GL_CHECK(glUniform2f(loc, v.x, v.y));
    }
}

void GLShader::SetUniform(const GLchar *n, const float v0, const float v1) {
    GLint loc;
    GL_CHECK(loc = glGetUniformLocation(program, n));
    if (loc > -1) {
        GL_CHECK(glUniform2f(loc, v0, v1));
    }
}

void GLShader::SetUniform(const GLchar *n, int v) {
    GLint loc;
    GL_CHECK(loc = glGetUniformLocation(program, n));
    if (loc > -1) {
        GL_CHECK(glUniform1i(loc, v));
    }
}

GLuint GLShader::GetProgram() {
    return program;
}

GLShader::~GLShader() {
    if (program) {
        GL_CHECK(glDeleteProgram(program));
    }
}

GLShaderList::GLShaderList() : ShaderList() {
    std::string version = "100";
#ifdef GL_DUMP_SHADERS
#ifdef __PS4__
    shaderDumper = new GLShaderDumper("/data/c2d_shaders_dump");
#endif
#endif

#if !defined(__GLES2__) && !defined(__GLES3__)
    const char *glsl = (const char *) glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (glsl && strlen(glsl) > 0) {
        char v[16];
        int major, minor;
        sscanf(glsl, "%d.%d", &major, &minor);
        if (major > 0) {
            snprintf(v, 16, "%i", major * 100 + minor);
            version = v;
        }
    }
#endif

#if defined(__PS4__) && defined(GL_SHADERS_BINARY)
    // color shader
    color = new GLShader("c2d-color", c2d_c2d_color_v, c2d_c2d_color_f,
                         c2d_c2d_color_v_length, c2d_c2d_color_f_length);
    // texture shader
    add(new GLShader("c2d-texture", c2d_c2d_texture_v, c2d_c2d_texture_f,
                     c2d_c2d_texture_v_length, c2d_c2d_texture_f_length));

    // crt shaders
    add(new GLShader("crt-aperture", c2d_crt_aperture_v, c2d_crt_aperture_f,
                     c2d_crt_aperture_v_length, c2d_crt_aperture_f_length));

    add(new GLShader("crt-cgwg-fast", c2d_crt_cgwg_fast_v, c2d_crt_cgwg_fast_f,
                     c2d_crt_cgwg_fast_v_length, c2d_crt_cgwg_fast_f_length));
    add(new GLShader("crt-easymode", c2d_crt_easymode_v, c2d_crt_easymode_f,
                     c2d_crt_easymode_v_length, c2d_crt_easymode_f_length));
    add(new GLShader("crt-geom", c2d_crt_geom_v, c2d_crt_geom_f,
                     c2d_crt_geom_v_length, c2d_crt_geom_f_length));
    add(new GLShader("crt-hyllian", c2d_crt_hyllian_v, c2d_crt_hyllian_f,
                     c2d_crt_hyllian_v_length, c2d_crt_hyllian_f_length));
    add(new GLShader("crt-nes-mini", c2d_crt_nes_mini_v, c2d_crt_nes_mini_f,
                     c2d_crt_nes_mini_v_length, c2d_crt_nes_mini_f_length));
    add(new GLShader("crt-pi", c2d_crt_pi_v, c2d_crt_pi_f,
                     c2d_crt_pi_v_length, c2d_crt_pi_f_length));
    add(new GLShader("crt-pi-flat", c2d_crt_pi_flat_v, c2d_crt_pi_flat_f,
                     c2d_crt_pi_flat_v_length, c2d_crt_pi_flat_f_length));
    add(new GLShader("crt-zfast", c2d_crt_zfast_v, c2d_crt_zfast_f,
                     c2d_crt_zfast_v_length, c2d_crt_zfast_f_length));

    // handheld shaders
    add(new GLShader("handheld-bevel", c2d_handheld_bevel_v, c2d_handheld_bevel_f,
                     c2d_handheld_bevel_v_length, c2d_handheld_bevel_f_length));
    add(new GLShader("handheld-dot", c2d_handheld_dot_v, c2d_handheld_dot_f,
                     c2d_handheld_dot_v_length, c2d_handheld_dot_f_length));
    add(new GLShader("handheld-lcd1x", c2d_handheld_lcd1x_v, c2d_handheld_lcd1x_f,
                     c2d_handheld_lcd1x_v_length, c2d_handheld_lcd1x_f_length));
    add(new GLShader("handheld-lcd3x", c2d_handheld_lcd3x_v, c2d_handheld_lcd3x_f,
                     c2d_handheld_lcd3x_v_length, c2d_handheld_lcd3x_f_length));
    add(new GLShader("handheld-retro-v2", c2d_handheld_retro_v2_v, c2d_handheld_retro_v2_f,
                     c2d_handheld_retro_v2_v_length, c2d_handheld_retro_v2_f_length));
    add(new GLShader("handheld-zfast-lcd", c2d_handheld_zfast_lcd_v, c2d_handheld_zfast_lcd_f,
                     c2d_handheld_zfast_lcd_v_length, c2d_handheld_zfast_lcd_f_length));

    // interpolation shaders
    add(new GLShader("interpolation-aann", c2d_interpolation_aann_v, c2d_interpolation_aann_f,
                     c2d_interpolation_aann_v_length, c2d_interpolation_aann_f_length));
    add(new GLShader("interpolation-pixellate", c2d_interpolation_pixellate_v, c2d_interpolation_pixellate_f,
                     c2d_interpolation_pixellate_v_length, c2d_interpolation_pixellate_f_length));
    add(new GLShader("interpolation-quilez", c2d_interpolation_quilez_v, c2d_interpolation_quilez_f,
                     c2d_interpolation_quilez_v_length, c2d_interpolation_quilez_f_length));
    add(new GLShader("interpolation-sharp-bilinear", c2d_interpolation_sharp_bilinear_v,
                     c2d_interpolation_sharp_bilinear_f,
                     c2d_interpolation_sharp_bilinear_v_length, c2d_interpolation_sharp_bilinear_f_length));
    add(new GLShader("interpolation-sharp-bilinear-scanlines",
                     c2d_interpolation_sharp_bilinear_scanlines_v, c2d_interpolation_sharp_bilinear_scanlines_f,
                     c2d_interpolation_sharp_bilinear_scanlines_v_length,
                     c2d_interpolation_sharp_bilinear_scanlines_f_length));

    // scanlines
    add(new GLShader("scanline-simple", c2d_scanline_simple_v, c2d_scanline_simple_f,
                     c2d_scanline_simple_v_length, c2d_scanline_simple_f_length));

    // sharp
    add(new GLShader("sharp-sabr-v3.0", c2d_sharp_sabr_v30_v, c2d_sharp_sabr_v30_f,
                     c2d_sharp_sabr_v30_v_length, c2d_sharp_sabr_v30_f_length));
#else
    // default shader
    color = new GLShader("c2d-color", c2d_color_shader, c2d_color_shader_length, version);
    add(new GLShader("c2d-texture", c2d_texture_shader, c2d_texture_shader_length, version));

    // that's all for the "retroid pocket 2"
    std::string device = C2DDevice::getName();
    if (device == "pocket2") {
        return;
    }

    // crt shaders
    add(new GLShader("crt-aperture", c2d_crt_aperture_shader, c2d_crt_aperture_shader_length, version));
#ifndef __PS4__
#ifndef ANDROID // TODO: disable for armv7 devices only, gles2 devices?
    add(new GLShader("crt-caligari", c2d_crt_caligari_shader, c2d_crt_caligari_shader_length, version));
#endif
#endif //__PS4__
    add(new GLShader("crt-cgwg-fast", c2d_crt_cgwg_fast_shader, c2d_crt_cgwg_fast_shader_length, version));
    add(new GLShader("crt-easymode", c2d_crt_easymode_shader, c2d_crt_easymode_shader_length, version));
#ifndef __PS4__
    add(new GLShader("crt-fakelottes", c2d_crt_fakelottes_shader, c2d_crt_fakelottes_shader_length, version));
#endif //__PS4__
    add(new GLShader("crt-geom", c2d_crt_geom_shader, c2d_crt_geom_shader_length, version));
#ifndef __PS4__
    add(new GLShader("crt-geom-flat", c2d_crt_geom_flat_shader, c2d_crt_geom_flat_shader_length, version));
#endif //__PS4__
    add(new GLShader("crt-hyllian", c2d_crt_hyllian_shader, c2d_crt_hyllian_shader_length, version));
#ifndef __PS4__
    add(new GLShader("crt-lottes-fast", c2d_crt_lottes_fast_shader, c2d_crt_lottes_fast_shader_length, version));
    add(new GLShader("crt-lottes", c2d_crt_lottes_shader, c2d_crt_lottes_shader_length, version));
    add(new GLShader("crt-mattias", c2d_crt_mattias_shader, c2d_crt_mattias_shader_length, version));
#endif //__PS4__
    add(new GLShader("crt-nes-mini", c2d_crt_nes_mini_shader, c2d_crt_nes_mini_shader_length, version));
#ifndef __SWITCH__
    add(new GLShader("crt-pi", c2d_crt_pi_shader, c2d_crt_pi_shader_length, version));
    add(new GLShader("crt-pi-flat", c2d_crt_pi_flat_shader, c2d_crt_pi_flat_shader_length, version));
#endif
    add(new GLShader("crt-zfast", c2d_crt_zfast_shader, c2d_crt_zfast_shader_length, version));

    // handheld shaders
    add(new GLShader("handheld-bevel", c2d_handheld_bevel_shader, c2d_handheld_bevel_shader_length, version));
    add(new GLShader("handheld-dot", c2d_handheld_dot_shader, c2d_handheld_dot_shader_length, version));
    add(new GLShader("handheld-lcd1x", c2d_handheld_lcd1x_shader, c2d_handheld_lcd1x_shader_length, version));
    add(new GLShader("handheld-lcd3x", c2d_handheld_lcd3x_shader, c2d_handheld_lcd3x_shader_length, version));
    add(new GLShader("handheld-retro-v2", c2d_handheld_retro_v2_shader, c2d_handheld_retro_v2_shader_length, version));
    add(new GLShader("handheld-zfast-lcd", c2d_handheld_zfast_lcd_shader, c2d_handheld_zfast_lcd_shader_length,
                     version));

    // interpolation shaders
    add(new GLShader("interpolation-aann", c2d_interpolation_aann_shader,
                     c2d_interpolation_aann_shader_length, version));
    add(new GLShader("interpolation-pixellate", c2d_interpolation_pixellate_shader,
                     c2d_interpolation_pixellate_shader_length, version));
    add(new GLShader("interpolation-quilez", c2d_interpolation_quilez_shader,
                     c2d_interpolation_quilez_shader_length, version));
    add(new GLShader("interpolation-sharp-bilinear", c2d_interpolation_sharp_bilinear_shader,
                     c2d_interpolation_sharp_bilinear_shader_length, version));
    add(new GLShader("interpolation-sharp-bilinear-scanlines", c2d_interpolation_sharp_bilinear_scanlines_shader,
                     c2d_interpolation_sharp_bilinear_scanlines_shader_length, version));

    // scanlines
    add(new GLShader("scanline-simple", c2d_scanline_simple_shader, c2d_scanline_simple_shader_length, version));

    // sharp
    add(new GLShader("sharp-2xsal", c2d_sharp_2xsal_shader, c2d_sharp_2xsal_shader_length, version));
    add(new GLShader("sharp-sabr-v3.0", c2d_sharp_sabr_v30_shader, c2d_sharp_sabr_v30_shader_length, version));
    add(new GLShader("sharp-supereagle", c2d_sharp_supereagle_shader, c2d_sharp_supereagle_shader_length, version));
#ifndef __PS4__
    add(new GLShader("sharp-xbrz-freescale", c2d_sharp_xbrz_freescale_shader, c2d_sharp_xbrz_freescale_shader_length,
                     version));
#endif //__PS4__
#endif //__PS4__ / GL_SHADERS_BINARY
}

GLShaderList::~GLShaderList() {
#ifdef GL_DUMP_SHADERS
    delete (shaderDumper);
#endif
}

#endif // __GL2__
