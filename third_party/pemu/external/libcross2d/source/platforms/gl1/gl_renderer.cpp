//
// Created by cpasjuste on 21/11/16.
//

#ifdef __GL1__

// GLdc doesn't like mixing blend and non blend textures
//#define BLEND_TEST 1
//#define __GL1_IMMEDIATE__ 1

#include "cross2d/c2d.h"

using namespace c2d;

GLRenderer::GLRenderer(const Vector2f &size) : Renderer(size) {
    printf("GL1Renderer\n");
    m_shaderList = (ShaderList *) new ShaderList();
}

void GLRenderer::initGL() {
    printf("GL vendor   : %s\n", glGetString(GL_VENDOR));
    printf("GL renderer : %s\n", glGetString(GL_RENDERER));
    printf("GL version  : %s\n", glGetString(GL_VERSION));

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

#ifndef BLEND_TEST
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, getSize().x, getSize().y, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void GLRenderer::draw(VertexArray *vertexArray, const Transform &transform, Texture *texture) {
    Vertex *vertices;
    size_t vertexCount;

    if (vertexArray == nullptr || vertexArray->getVertexCount() < 1) {
        //printf("GLRenderer::draw: no vertices\n");
        return;
    }

    vertices = vertexArray->getVertices()->data();
    vertexCount = vertexArray->getVertexCount();

    if (transform == Transform::Identity) {
        glLoadIdentity();
    } else {
        glLoadMatrixf(transform.getMatrix());
    }

    auto tex = (GLTexture *) texture;
    if (tex && tex->available) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex->m_texID);
#ifndef __GL1_IMMEDIATE__
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
#if BLEND_TEST
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    }
#if BLEND_TEST
    else if (vertices[0].color.a < 255) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
#endif

    GLenum mode = modes[vertexArray->getPrimitiveType()];

#ifdef __GL1_IMMEDIATE__
    glBegin(mode);
    for (unsigned int i = 0; i < vertexCount; i++) {
        if (tex && tex->available) {
            glTexCoord2f(vertices[i].texCoords.x, vertices[i].texCoords.y);
        }
        glColor4f((float) vertices[i].color.r / 255.0f,
                  (float) vertices[i].color.g / 255.0f,
                  (float) vertices[i].color.b / 255.0f,
                  (float) vertices[i].color.a / 255.0f);
        glVertex2f(vertices[i].position.x, vertices[i].position.y);
    }
    glEnd();
#else
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &vertices[0].position);
    if (tex && tex->available) {
        glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &vertices[0].texCoords);
    }
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &vertices[0].color);

    glDrawArrays(mode, 0, (GLsizei) vertexCount);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
#endif

    if (tex && tex->available) {
#ifndef __GL1_IMMEDIATE__
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
#if BLEND_TEST
        glDisable(GL_BLEND);
#endif
        glDisable(GL_TEXTURE_2D);
    }
#if BLEND_TEST
    else if (vertices[0].color.a < 255) {
        glDisable(GL_BLEND);
    }
#endif
}

void GLRenderer::clear() {
    glClearColor((float) m_clearColor.r / 255.0f,
                 (float) m_clearColor.g / 255.0f,
                 (float) m_clearColor.b / 255.0f,
                 (float) m_clearColor.a / 255.0f);
    glClearDepth(0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLRenderer::flip(bool draw, bool inputs) {
    // call base class (draw childs)
    Renderer::flip(draw, inputs);
}

GLRenderer::~GLRenderer() {
    printf("~GL1Renderer\n");
}

#endif // __GL1__
