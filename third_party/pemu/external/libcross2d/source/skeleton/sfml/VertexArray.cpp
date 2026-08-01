////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2017 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include "cross2d/c2d.h"
#include "cross2d/skeleton/sfml/VertexArray.hpp"

namespace c2d {
////////////////////////////////////////////////////////////
    VertexArray::VertexArray() :
            m_vertices(),
            m_primitiveType(Points) {
        update();
    }


////////////////////////////////////////////////////////////
    VertexArray::VertexArray(PrimitiveType type, std::size_t vertexCount) :
            m_vertices(vertexCount),
            m_primitiveType(type) {
        update();
    }


////////////////////////////////////////////////////////////
    std::size_t VertexArray::getVertexCount() const {
        return m_vertices.size();
    }


////////////////////////////////////////////////////////////
    Vertex &VertexArray::operator[](std::size_t index) {
        return m_vertices[index];
    }


////////////////////////////////////////////////////////////
    const Vertex &VertexArray::operator[](std::size_t index) const {
        return m_vertices[index];
    }


////////////////////////////////////////////////////////////
    void VertexArray::clear() {
        m_vertices.clear();
        update();
    }


////////////////////////////////////////////////////////////
    void VertexArray::resize(std::size_t vertexCount) {
        m_vertices.resize(vertexCount);
        update();
    }


////////////////////////////////////////////////////////////
    void VertexArray::append(const Vertex &vertex) {
        m_vertices.push_back(vertex);
        // do not call update here, "Text" will use append a lot (gl buffer overflow)
        //update();
    }


////////////////////////////////////////////////////////////
    void VertexArray::setPrimitiveType(PrimitiveType type) {
        m_primitiveType = type;
    }


////////////////////////////////////////////////////////////
    PrimitiveType VertexArray::getPrimitiveType() const {
        return m_primitiveType;
    }


////////////////////////////////////////////////////////////
    FloatRect VertexArray::getBounds() const {
        if (!m_vertices.empty()) {
            float left = m_vertices[0].position.x;
            float top = m_vertices[0].position.y;
            float right = m_vertices[0].position.x;
            float bottom = m_vertices[0].position.y;

            for (std::size_t i = 1; i < m_vertices.size(); ++i) {
                Vector2f position = m_vertices[i].position;

                // Update left and right
                if (position.x < left)
                    left = position.x;
                else if (position.x > right)
                    right = position.x;

                // Update top and bottom
                if (position.y < top)
                    top = position.y;
                else if (position.y > bottom)
                    bottom = position.y;
            }

            return {left, top, right - left, bottom - top};
        } else {
            // Array is empty
            return {};
        }
    }

    std::vector<Vertex> *VertexArray::getVertices() {
        return &m_vertices;
    }

    void VertexArray::update() {
#ifdef __GL2__
        if (c2d_renderer == nullptr || !c2d_renderer->available
            || m_vertices.empty()) {
            return;
        }

        if (vbo < 1) {
            GL_CHECK(glGenBuffers(1, &vbo));
        }

        //printf("VertexArray::update(%p): vbo: %i, vertices: %lu, size: %lu\n",
          //     this, vbo, m_vertices.size(), sizeof(Vertex) * m_vertices.size());
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertices.size(), m_vertices.data(), GL_STREAM_DRAW));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
#endif
    }

    void VertexArray::bind() const {
#ifdef __GL2__
        if (c2d_renderer == nullptr || !c2d_renderer->available) {
            return;
        }
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
#endif
    }

    void VertexArray::unbind() const {
#ifdef __GL2__
        if (c2d_renderer == nullptr || !c2d_renderer->available) {
            return;
        }
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
#endif
    }

    VertexArray::~VertexArray() {
#ifdef __GL2__
        if (c2d_renderer == nullptr || !c2d_renderer->available) {
            return;
        }
        if (vbo > 0) {
            GL_CHECK(glDeleteBuffers(1, &vbo));
        }
#endif
    }

} // namespace c2d
