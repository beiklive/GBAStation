//
// Created by cpasjuste on 21/11/16.
//

#ifdef __GL2__

#define GLM_FORCE_PURE

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "cross2d/c2d.h"

using namespace c2d;

GLRenderer::GLRenderer(const Vector2f &size) : Renderer(size) {
    printf("GL2Renderer\n");
}

void GLRenderer::initGL() {
    printf("GL vendor   : %s\n", glGetString(GL_VENDOR));
    printf("GL renderer : %s\n", glGetString(GL_RENDERER));
    printf("GL version  : %s\n", glGetString(GL_VERSION));
    printf("GL glsl     : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

#ifdef glGenVertexArrays
    // vao
    GL_CHECK(glGenVertexArrays(1, &vao));
#endif
    GL_CHECK(glDisable(GL_DEPTH_TEST));
    GL_CHECK(glDepthMask(GL_FALSE));

    // init shaders
    m_shaderList = (ShaderList *) new GLShaderList();
}

void GLRenderer::draw(VertexArray *vertexArray, const Transform &transform, Texture *texture) {
    Vertex *vertices;
    size_t vertexCount;
    GLTexture *tex;
    GLShader *shader;
    Vector2f inputSize, textureSize, outputSize;

    if (vertexArray == nullptr || vertexArray->getVertexCount() < 1) {
        //printf("gl_render::draw: no vertices\n");
        return;
    }

    vertices = vertexArray->getVertices()->data();
    vertexCount = vertexArray->getVertexCount();
    tex = (GLTexture *) texture;
    shader = tex && tex->available ? (GLShader *) m_shaderList->get(0) :
             (GLShader *) ((GLShaderList *) m_shaderList)->color;
    if (tex && tex->m_shader && tex->m_shader->available) {
        shader = (GLShader *) tex->m_shader;
    }

    // set shader
    GL_CHECK(glUseProgram(shader->GetProgram()));

    // set projection
#if defined(__SDL2__)
    int w, h;
    SDL_Window *window = ((SDL2Renderer *) this)->getWindow();
    SDL_GL_GetDrawableSize(window, &w, &h);
#else
    int w = (int) getSize().x, h = (int) getSize().y;
#endif

    // update viewport
    glViewport(0, 0, w, h);

    // projection
    auto projectionMatrix = glm::orthoLH(0.0f, (float) w, (float) h, 0.0f, 0.0f, 1.0f);
    // view
    auto viewMatrix = glm::make_mat4(transform.getMatrix());
    // mpv
    auto mpvMatrix = projectionMatrix * viewMatrix;
    // set mpv matrix uniform
    shader->SetUniformMatrix("MVPMatrix", glm::value_ptr(mpvMatrix));

#ifdef glBindVertexArray
    // bind vao
    GL_CHECK(glBindVertexArray(vao));
#endif
    // bind vbo
    vertexArray->bind();

    // set vertex position
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                   (void *) offsetof(Vertex, position)));

    // set vertex colors
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex),
                                   (void *) offsetof(Vertex, color)));

    if (tex && tex->available) {
        // bind texture
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, tex->m_texID));
        // set tex coords
        GL_CHECK(glEnableVertexAttribArray(2));
        GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                       (void *) offsetof(Vertex, texCoords)));
        // set retroarch shader params
        textureSize = {(float) texture->getTextureSize().x, (float) texture->getTextureSize().y};
        inputSize = {(float) texture->getTextureRect().width,
                     (float) texture->getTextureRect().height};
        outputSize = {texture->getSize().x * texture->getScale().x,
                      texture->getSize().y * texture->getScale().y};
        shader->SetUniform("InputSize", inputSize);
        shader->SetUniform("TextureSize", textureSize);
        shader->SetUniform("OutputSize", outputSize);
#if 0
        printf("inputSize: %ix%i, textureSize: %ix%i, outputSize: %ix%i\n",
               (int) inputSize.x, (int) inputSize.y,
               (int) textureSize.x, (int) textureSize.y,
               (int) outputSize.x, (int) outputSize.y);
#endif
    }

    // enable blending if needed
    if (tex || vertices[0].color.a < 255) {
        GL_CHECK(glEnable(GL_BLEND));
        GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    }

    // draw
    GLenum mode = modes[vertexArray->getPrimitiveType()];
    GL_CHECK(glDrawArrays(mode, 0, (GLsizei) vertexCount));

    GL_CHECK(glDisableVertexAttribArray(0));
    GL_CHECK(glDisableVertexAttribArray(1));

    if (tex || vertices[0].color.a < 255) {
        GL_CHECK(glDisable(GL_BLEND));
        if (tex && tex->available) {
            GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
            GL_CHECK(glDisableVertexAttribArray(2));
        }
    }

#ifdef glBindVertexArray
    // unbind object vao
    glBindVertexArray(0);
#endif
    // unbind object vbo
    vertexArray->unbind();

    GL_CHECK(glUseProgram(0));
}

void GLRenderer::clear() {
    // clear screen
    GL_CHECK(glClearColor(m_clearColor.r / 255.0f,
                          m_clearColor.g / 255.0f,
                          m_clearColor.b / 255.0f,
                          m_clearColor.a / 255.0f));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
}

void GLRenderer::flip(bool draw, bool inputs) {
    // call base class (draw childs)
    Renderer::flip(draw, inputs);
}

GLRenderer::~GLRenderer() {
    printf("~GL2Renderer\n");
#ifdef glIsVertexArray
    if (glIsVertexArray(vao)) {
        GL_CHECK(glDeleteVertexArrays(1, &vao));
    }
#endif
}

#ifndef NDEBUG

namespace c2d {
    void CheckOpenGLError(const char *stmt, const char *fname, int line) {
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            printf("OpenGL error 0x%04x at %s:%i - for %s\n", err, fname, line, stmt);
            abort();
        }
    }
}
#endif

#endif // __GL2__
