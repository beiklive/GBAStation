//! Minimal OpenGL FFI subset used by the project.
//!
//! Targets desktop GL core profile (the EGL context in cpp/gl_context.cpp is
//! created with EGL_OPENGL_API + EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT, GL 4.3,
//! and EGL_STENCIL_SIZE=8, so masking can use the stencil buffer).
//! All names match the standard libGL ABI exposed by switch-mesa.

#![allow(non_snake_case, non_camel_case_types, dead_code)]

use core::ffi::{c_char, c_float, c_int, c_uchar, c_uint, c_void};

pub type GLbitfield = c_uint;
pub type GLboolean = c_uchar;
pub type GLchar = c_char;
pub type GLenum = c_uint;
pub type GLfloat = c_float;
pub type GLint = c_int;
pub type GLsizei = c_int;
pub type GLsizeiptr = isize;
pub type GLintptr = isize;
pub type GLuint = c_uint;

pub const GL_FALSE: GLboolean = 0;
pub const GL_TRUE: GLboolean = 1;

pub const GL_COLOR_BUFFER_BIT: GLbitfield = 0x0000_4000;
pub const GL_DEPTH_BUFFER_BIT: GLbitfield = 0x0000_0100;
pub const GL_STENCIL_BUFFER_BIT: GLbitfield = 0x0000_0400;

pub const GL_POINTS: GLenum = 0x0000;
pub const GL_LINES: GLenum = 0x0001;
pub const GL_TRIANGLES: GLenum = 0x0004;
pub const GL_FLOAT: GLenum = 0x1406;
pub const GL_UNSIGNED_INT: GLenum = 0x1405;
pub const GL_UNSIGNED_BYTE: GLenum = 0x1401;

pub const GL_ARRAY_BUFFER: GLenum = 0x8892;
pub const GL_ELEMENT_ARRAY_BUFFER: GLenum = 0x8893;
pub const GL_STATIC_DRAW: GLenum = 0x88E4;
pub const GL_DYNAMIC_DRAW: GLenum = 0x88E8;

pub const GL_FRAGMENT_SHADER: GLenum = 0x8B30;
pub const GL_VERTEX_SHADER: GLenum = 0x8B31;
pub const GL_COMPILE_STATUS: GLenum = 0x8B81;
pub const GL_LINK_STATUS: GLenum = 0x8B82;
pub const GL_INFO_LOG_LENGTH: GLenum = 0x8B84;

pub const GL_BLEND: GLenum = 0x0BE2;
pub const GL_ZERO: GLenum = 0;
pub const GL_ONE: GLenum = 1;
pub const GL_SRC_COLOR: GLenum = 0x0300;
pub const GL_ONE_MINUS_SRC_COLOR: GLenum = 0x0301;
pub const GL_SRC_ALPHA: GLenum = 0x0302;
pub const GL_ONE_MINUS_SRC_ALPHA: GLenum = 0x0303;
// Blend equations (glBlendEquation / glBlendEquationSeparate)
pub const GL_FUNC_ADD: GLenum = 0x8006;
pub const GL_FUNC_SUBTRACT: GLenum = 0x800A;
pub const GL_FUNC_REVERSE_SUBTRACT: GLenum = 0x800B;

// Textures
pub const GL_TEXTURE_2D: GLenum = 0x0DE1;
pub const GL_RGBA: GLenum = 0x1908;
pub const GL_RGBA8: GLenum = 0x8058;
pub const GL_TEXTURE0: GLenum = 0x84C0;
pub const GL_TEXTURE_WRAP_S: GLenum = 0x2802;
pub const GL_TEXTURE_WRAP_T: GLenum = 0x2803;
pub const GL_TEXTURE_MIN_FILTER: GLenum = 0x2801;
pub const GL_TEXTURE_MAG_FILTER: GLenum = 0x2800;
pub const GL_LINEAR: GLenum = 0x2601;
pub const GL_NEAREST: GLenum = 0x2600;
pub const GL_CLAMP_TO_EDGE: GLenum = 0x812F;
pub const GL_REPEAT: GLenum = 0x2901;
pub const GL_MIRRORED_REPEAT: GLenum = 0x8370;
pub const GL_UNPACK_ALIGNMENT: GLenum = 0x0CF5;
pub const GL_PACK_ALIGNMENT: GLenum = 0x0D05;
pub const GL_TEXTURE1: GLenum = 0x84C1;

// Framebuffer objects (offscreen rendering for cacheAsBitmap / filters)
pub const GL_FRAMEBUFFER: GLenum = 0x8D40;
pub const GL_COLOR_ATTACHMENT0: GLenum = 0x8CE0;
pub const GL_FRAMEBUFFER_BINDING: GLenum = 0x8CA6;
pub const GL_FRAMEBUFFER_COMPLETE: GLenum = 0x8CD5;
pub const GL_VIEWPORT: GLenum = 0x0BA2;
// Renderbuffer (depth-stencil attachment so masks work inside an FBO)
pub const GL_RENDERBUFFER: GLenum = 0x8D41;
pub const GL_DEPTH24_STENCIL8: GLenum = 0x88F0;
pub const GL_DEPTH_STENCIL_ATTACHMENT: GLenum = 0x821A;

// Stencil
pub const GL_STENCIL_TEST: GLenum = 0x0B90;
pub const GL_NEVER: GLenum = 0x0200;
pub const GL_LESS: GLenum = 0x0201;
pub const GL_EQUAL: GLenum = 0x0202;
pub const GL_LEQUAL: GLenum = 0x0203;
pub const GL_GREATER: GLenum = 0x0204;
pub const GL_NOTEQUAL: GLenum = 0x0205;
pub const GL_GEQUAL: GLenum = 0x0206;
pub const GL_ALWAYS: GLenum = 0x0207;
pub const GL_KEEP: GLenum = 0x1E00;
pub const GL_REPLACE: GLenum = 0x1E01;
pub const GL_INCR: GLenum = 0x1E02;
pub const GL_DECR: GLenum = 0x1E03;
pub const GL_INVERT: GLenum = 0x150A;

extern "C" {
    pub fn glClearColor(r: GLfloat, g: GLfloat, b: GLfloat, a: GLfloat);
    pub fn glClearStencil(s: GLint);
    pub fn glClear(mask: GLbitfield);
    pub fn glViewport(x: GLint, y: GLint, w: GLsizei, h: GLsizei);
    pub fn glDrawArrays(mode: GLenum, first: GLint, count: GLsizei);
    pub fn glDrawElements(mode: GLenum, count: GLsizei, ty: GLenum, indices: *const c_void);

    pub fn glEnable(cap: GLenum);
    pub fn glDisable(cap: GLenum);
    pub fn glBlendFunc(sfactor: GLenum, dfactor: GLenum);
    pub fn glBlendFuncSeparate(
        src_rgb: GLenum,
        dst_rgb: GLenum,
        src_alpha: GLenum,
        dst_alpha: GLenum,
    );
    pub fn glBlendEquation(mode: GLenum);
    pub fn glBlendEquationSeparate(mode_rgb: GLenum, mode_alpha: GLenum);
    pub fn glLineWidth(width: GLfloat);
    pub fn glColorMask(r: GLboolean, g: GLboolean, b: GLboolean, a: GLboolean);
    pub fn glStencilMask(mask: GLuint);
    pub fn glStencilFunc(func: GLenum, refr: GLint, mask: GLuint);
    pub fn glStencilOp(sfail: GLenum, dpfail: GLenum, dppass: GLenum);

    pub fn glGenBuffers(n: GLsizei, buffers: *mut GLuint);
    pub fn glDeleteBuffers(n: GLsizei, buffers: *const GLuint);
    pub fn glBindBuffer(target: GLenum, buffer: GLuint);
    pub fn glBufferData(target: GLenum, size: GLsizeiptr, data: *const c_void, usage: GLenum);
    pub fn glBufferSubData(
        target: GLenum,
        offset: GLintptr,
        size: GLsizeiptr,
        data: *const c_void,
    );
    /// Like `glDrawElements`, but adds `base_vertex` to each fetched index
    /// before reading from the bound VBO. Lets us pack many shapes into a
    /// single VBO with each shape using local indices 0..N.
    pub fn glDrawElementsBaseVertex(
        mode: GLenum,
        count: GLsizei,
        ty: GLenum,
        indices: *const c_void,
        base_vertex: GLint,
    );

    pub fn glGenVertexArrays(n: GLsizei, arrays: *mut GLuint);
    pub fn glDeleteVertexArrays(n: GLsizei, arrays: *const GLuint);
    pub fn glBindVertexArray(array: GLuint);
    pub fn glEnableVertexAttribArray(index: GLuint);
    pub fn glVertexAttribPointer(
        index: GLuint,
        size: GLint,
        ty: GLenum,
        normalized: GLboolean,
        stride: GLsizei,
        pointer: *const c_void,
    );

    pub fn glCreateShader(ty: GLenum) -> GLuint;
    pub fn glShaderSource(
        shader: GLuint,
        count: GLsizei,
        strings: *const *const GLchar,
        lengths: *const GLint,
    );
    pub fn glCompileShader(shader: GLuint);
    pub fn glGetShaderiv(shader: GLuint, pname: GLenum, params: *mut GLint);
    pub fn glGetShaderInfoLog(
        shader: GLuint,
        max_len: GLsizei,
        length: *mut GLsizei,
        info_log: *mut GLchar,
    );
    pub fn glDeleteShader(shader: GLuint);

    pub fn glCreateProgram() -> GLuint;
    pub fn glAttachShader(program: GLuint, shader: GLuint);
    pub fn glLinkProgram(program: GLuint);
    pub fn glGetProgramiv(program: GLuint, pname: GLenum, params: *mut GLint);
    pub fn glGetProgramInfoLog(
        program: GLuint,
        max_len: GLsizei,
        length: *mut GLsizei,
        info_log: *mut GLchar,
    );
    pub fn glDeleteProgram(program: GLuint);
    pub fn glUseProgram(program: GLuint);

    pub fn glGetUniformLocation(program: GLuint, name: *const GLchar) -> GLint;
    pub fn glUniformMatrix3fv(
        location: GLint,
        count: GLsizei,
        transpose: GLboolean,
        value: *const GLfloat,
    );
    pub fn glUniform1i(location: GLint, v0: GLint);
    pub fn glUniform1f(location: GLint, v0: GLfloat);
    pub fn glUniform2f(location: GLint, v0: GLfloat, v1: GLfloat);
    pub fn glUniform4f(location: GLint, x: GLfloat, y: GLfloat, z: GLfloat, w: GLfloat);
    pub fn glUniformMatrix4fv(
        location: GLint,
        count: GLsizei,
        transpose: GLboolean,
        value: *const GLfloat,
    );

    // Textures
    pub fn glGenTextures(n: GLsizei, textures: *mut GLuint);
    pub fn glDeleteTextures(n: GLsizei, textures: *const GLuint);
    pub fn glBindTexture(target: GLenum, texture: GLuint);
    pub fn glActiveTexture(texture: GLenum);
    pub fn glTexImage2D(
        target: GLenum,
        level: GLint,
        internal_format: GLint,
        width: GLsizei,
        height: GLsizei,
        border: GLint,
        format: GLenum,
        ty: GLenum,
        pixels: *const c_void,
    );
    pub fn glTexSubImage2D(
        target: GLenum,
        level: GLint,
        xoffset: GLint,
        yoffset: GLint,
        width: GLsizei,
        height: GLsizei,
        format: GLenum,
        ty: GLenum,
        pixels: *const c_void,
    );
    pub fn glTexParameteri(target: GLenum, pname: GLenum, param: GLint);
    pub fn glPixelStorei(pname: GLenum, param: GLint);
    /// Copy a rectangle of the currently-bound read framebuffer into the
    /// currently-bound texture (must already have storage allocated). Used to
    /// snapshot the backdrop for complex blend modes (multiply/overlay/…).
    pub fn glCopyTexSubImage2D(
        target: GLenum,
        level: GLint,
        xoffset: GLint,
        yoffset: GLint,
        x: GLint,
        y: GLint,
        width: GLsizei,
        height: GLsizei,
    );

    // Framebuffer objects
    pub fn glGenFramebuffers(n: GLsizei, framebuffers: *mut GLuint);
    pub fn glDeleteFramebuffers(n: GLsizei, framebuffers: *const GLuint);
    pub fn glBindFramebuffer(target: GLenum, framebuffer: GLuint);
    pub fn glFramebufferTexture2D(
        target: GLenum,
        attachment: GLenum,
        textarget: GLenum,
        texture: GLuint,
        level: GLint,
    );
    pub fn glCheckFramebufferStatus(target: GLenum) -> GLenum;
    pub fn glGetIntegerv(pname: GLenum, params: *mut GLint);
    pub fn glIsEnabled(cap: GLenum) -> GLboolean;
    pub fn glReadPixels(
        x: GLint,
        y: GLint,
        width: GLsizei,
        height: GLsizei,
        format: GLenum,
        ty: GLenum,
        pixels: *mut c_void,
    );
    pub fn glGenRenderbuffers(n: GLsizei, renderbuffers: *mut GLuint);
    pub fn glDeleteRenderbuffers(n: GLsizei, renderbuffers: *const GLuint);
    pub fn glBindRenderbuffer(target: GLenum, renderbuffer: GLuint);
    pub fn glRenderbufferStorage(
        target: GLenum,
        internalformat: GLenum,
        width: GLsizei,
        height: GLsizei,
    );
    pub fn glFramebufferRenderbuffer(
        target: GLenum,
        attachment: GLenum,
        renderbuffertarget: GLenum,
        renderbuffer: GLuint,
    );

    pub fn glGetError() -> GLenum;
    pub fn glGetFramebufferAttachmentParameteriv(
        target: GLenum,
        attachment: GLenum,
        pname: GLenum,
        params: *mut GLint,
    );
}

// Default-framebuffer stencil introspection (mask debugging).
pub const GL_STENCIL: GLenum = 0x1802;
pub const GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE: GLenum = 0x8217;

pub const GL_NO_ERROR: GLenum = 0;
pub const GL_INVALID_ENUM: GLenum = 0x0500;
pub const GL_INVALID_VALUE: GLenum = 0x0501;
pub const GL_INVALID_OPERATION: GLenum = 0x0502;
pub const GL_OUT_OF_MEMORY: GLenum = 0x0505;
pub const GL_INVALID_FRAMEBUFFER_OPERATION: GLenum = 0x0506;
