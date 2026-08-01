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
#include <cmath>
#include "cross2d/c2d.h"

namespace {
    // Compute the normal of a segment
    c2d::Vector2f computeNormal(const c2d::Vector2f &p1, const c2d::Vector2f &p2) {
        c2d::Vector2f normal(p1.y - p2.y, p2.x - p1.x);
        float length = std::sqrt(normal.x * normal.x + normal.y * normal.y);
        if (length != 0.f)
            normal /= length;
        return normal;
    }

    // Compute the dot product of two vectors
    float dotProduct(const c2d::Vector2f &p1, const c2d::Vector2f &p2) {
        return p1.x * p2.x + p1.y * p2.y;
    }
}


namespace c2d {
////////////////////////////////////////////////////////////
    Shape::~Shape() {
#ifdef __BOX2D__
        if (m_world && m_world->getPhysics() && m_body) {
            m_world->getPhysics()->DestroyBody(m_body);
            m_body = nullptr;
        }
#endif
    }

    ////////////////////////////////////////////////////////////
    void Shape::setTexture(Texture *texture, bool resetRect) {
        if (texture) {
            m_texture = texture;
            // Recompute the texture area if requested, or if there was no texture & rect before
            if (resetRect || m_tex_rect == IntRect()) {
                setTextureRect(IntRect({0, 0}, Vector2i(m_texture->getSize())));
            }
            m_shape_dirty = true;
        }
    }


////////////////////////////////////////////////////////////
    Texture *Shape::getTexture() {
        return m_texture;
    }


////////////////////////////////////////////////////////////
    void Shape::setTextureRect(const IntRect &rect) {
        m_tex_rect = rect;
        if (m_texture) {
#if !defined(__VITA__) && !defined(__3DS__)
            m_texture->m_pitch = rect.width * m_texture->m_bpp;
#endif
        }
        m_shape_dirty = true;
    }


////////////////////////////////////////////////////////////
    const IntRect &Shape::getTextureRect() const {
        return m_tex_rect;
    }


////////////////////////////////////////////////////////////
    void Shape::setFillColor(const Color &color) {
        if (m_fillColor != color) {
            m_fillColor = color;
            m_shape_dirty = true;
        }
    }


////////////////////////////////////////////////////////////
    const Color &Shape::getFillColor() const {
        return m_fillColor;
    }

    void Shape::setAlpha(uint8_t alpha, bool recursive) {
        if (alpha != m_fillColor.a) {
            m_fillColor.a = alpha;
            m_outlineColor.a = alpha;
            m_shape_dirty = true;
            C2DObject::setAlpha(alpha, recursive);
        }
    }

    uint8_t Shape::getAlpha() {
        return m_fillColor.a;
    }

////////////////////////////////////////////////////////////
    void Shape::setOutlineColor(const Color &color) {
        if (m_outlineColor != color) {
            m_outlineColor = color;
            m_shape_dirty = true;
        }
    }


////////////////////////////////////////////////////////////
    const Color &Shape::getOutlineColor() const {
        return m_outlineColor;
    }


////////////////////////////////////////////////////////////
    void Shape::setOutlineThickness(float thickness) {
        if (m_outlineThickness != thickness) {
            m_outlineThickness = thickness;
            m_shape_dirty = true;
        }
    }


////////////////////////////////////////////////////////////
    float Shape::getOutlineThickness() const {
        return m_outlineThickness;
    }


////////////////////////////////////////////////////////////
    FloatRect Shape::getLocalBounds() const {
        return m_bounds;
    }


////////////////////////////////////////////////////////////
    FloatRect Shape::getGlobalBounds() const {
        return getTransform().transformRect(getLocalBounds());
    }

////////////////////////////////////////////////////////////
    Origin Shape::getOrigin() const {
        return m_shape_origin;
    }


////////////////////////////////////////////////////////////

    void Shape::setOrigin(const Origin &origin) {
#ifdef __BOX2D__ // TODO: fixme
        if (m_body) {
            return;
        }
#endif
        m_shape_origin = origin;

        switch (origin) {
            case Origin::Left:
                Transformable::setOriginVector(0, m_bounds.height / 2);
                break;
            case Origin::TopLeft:
                Transformable::setOriginVector(0, 0);
                break;
            case Origin::Top:
                Transformable::setOriginVector(m_bounds.width / 2, 0);
                break;
            case Origin::TopRight:
                Transformable::setOriginVector(m_bounds.width, 0);
                break;
            case Origin::Right:
                Transformable::setOriginVector(m_bounds.width, m_bounds.height / 2);
                break;
            case Origin::BottomRight:
                Transformable::setOriginVector(m_bounds.width, m_bounds.height);
                break;
            case Origin::Bottom:
                Transformable::setOriginVector(m_bounds.width / 2, m_bounds.height);
                break;
            case Origin::BottomLeft:
                Transformable::setOriginVector(0, m_bounds.height);
                break;
            case Origin::Center:
                Transformable::setOriginVector(m_bounds.width / 2 - getOutlineThickness(),
                                               m_bounds.height / 2 - getOutlineThickness());
                break;
            default:
                break;
        }
    }

    VertexArray *Shape::getVertexArray() {
        return &m_vertices;
    }

////////////////////////////////////////////////////////////
    Shape::Shape() :
            m_texture(nullptr),
            m_tex_rect(),
            m_fillColor(255, 255, 255),
            m_outlineColor(255, 255, 255),
            m_outlineThickness(0),
            m_vertices(TriangleFan),
            m_outlineVertices(TriangleStrip),
            m_insideBounds(),
            m_bounds() {
        type = Type::Shape;
    }

////////////////////////////////////////////////////////////
    void Shape::update() {
        // Get the total number of points of the shape
        std::size_t count = getPointCount();
        if (count < 3) {
            m_vertices.resize(0);
            m_outlineVertices.resize(0);
            m_shape_dirty = false;
            return;
        }

        m_vertices.resize(count + 2); // + 2 for center and repeated first point

        // Position
        for (std::size_t i = 0; i < count; ++i) {
            m_vertices[i + 1].position = getPoint(i);
        }
        m_vertices[count + 1].position = m_vertices[1].position;

        // Update the bounding rectangle
        m_vertices[0] = m_vertices[1]; // so that the result of getBounds() is correct
        m_insideBounds = m_vertices.getBounds();

        // Compute the center and make it the first vertex
        m_vertices[0].position.x = m_insideBounds.left + m_insideBounds.width / 2;
        m_vertices[0].position.y = m_insideBounds.top + m_insideBounds.height / 2;

        // Color
        updateFillColors();

        // Texture coordinates
        updateTexCoords();

        // Outline
        updateOutline();

        // origin
        setOrigin(m_shape_origin);

        m_shape_dirty = false;
    }

    void Shape::onDraw(Transform &transform, bool draw) {
        if (m_shape_dirty) {
            update();
        }

#ifdef __BOX2D__
        if (m_body) {
            b2Vec2 pos = m_body->GetPosition();
            setPosition(pos.x * m_world->getPixelsPerMeter(), pos.y * m_world->getPixelsPerMeter());
            float angle = m_body->GetAngle();
            setRotation(angle * 180 / M_PI);
        }
#endif

        if (draw) {
            Transform combined = transform * getTransform();
            if (getFillColor().a != 0) {
                c2d_renderer->draw(&m_vertices, combined, m_texture);
            }
            if (getOutlineColor().a != 0 && m_outlineThickness > 0) {
                c2d_renderer->draw(&m_outlineVertices, combined, nullptr);
            }
        }
        C2DObject::onDraw(transform, draw);
    }

////////////////////////////////////////////////////////////
    void Shape::updateFillColors() {
        for (std::size_t i = 0; i < m_vertices.getVertexCount(); ++i) {
            m_vertices[i].color = m_fillColor;
        }
        m_vertices.update();
    }

////////////////////////////////////////////////////////////
    void Shape::updateTexCoords() {
        if (m_texture == nullptr) {
            return;
        }

        for (std::size_t i = 0; i < m_vertices.getVertexCount(); ++i) {
            float xratio =
                    m_insideBounds.width > 0 ?
                    (m_vertices[i].position.x - m_insideBounds.left) / m_insideBounds.width : 0;
            float yratio =
                    m_insideBounds.height > 0 ?
                    (m_vertices[i].position.y - m_insideBounds.top) / m_insideBounds.height : 0;
            m_vertices[i].texCoords.x =
                    ((float) m_tex_rect.left + (float) m_tex_rect.width * xratio) /
                    (float) m_texture->getTextureSizePot().x;
            m_vertices[i].texCoords.y =
                    ((float) m_tex_rect.top + (float) m_tex_rect.height * yratio) /
                    (float) m_texture->getTextureSizePot().y;
#ifdef __BOX2D__
#ifndef __PSP2__ // TODO: fixme
            if (m_body) {
                m_vertices[i].texCoords.y *= -1;
            }
#endif
#endif
        }

        m_vertices.update();
    }

////////////////////////////////////////////////////////////
    void Shape::updateOutline() {
        // Return if there is no outline
        if (m_outlineThickness == 0.f) {
            m_outlineVertices.clear();
            m_bounds = m_insideBounds;
            return;
        }

        std::size_t count = m_vertices.getVertexCount() - 2;
        m_outlineVertices.resize((count + 1) * 2);

        for (std::size_t i = 0; i < count; ++i) {
            std::size_t index = i + 1;

            // Get the two segments shared by the current point
            Vector2f p0 = (i == 0) ? m_vertices[count].position : m_vertices[index - 1].position;
            Vector2f p1 = m_vertices[index].position;
            Vector2f p2 = m_vertices[index + 1].position;

            // Compute their normal
            Vector2f n1 = computeNormal(p0, p1);
            Vector2f n2 = computeNormal(p1, p2);

            // Make sure that the normals point towards the outside of the shape
            // (this depends on the order in which the points were defined)
            if (dotProduct(n1, m_vertices[0].position - p1) > 0)
                n1 = -n1;
            if (dotProduct(n2, m_vertices[0].position - p1) > 0)
                n2 = -n2;

            // Combine them to get the extrusion direction
            float factor = 1.f + (n1.x * n2.x + n1.y * n2.y);
            Vector2f normal = (n1 + n2) / factor;

            // Update the outline points
            m_outlineVertices[i * 2 + 0].position = p1;
            m_outlineVertices[i * 2 + 1].position = p1 + normal * m_outlineThickness;
        }

        // Duplicate the first point at the end, to close the outline
        m_outlineVertices[count * 2 + 0].position = m_outlineVertices[0].position;
        m_outlineVertices[count * 2 + 1].position = m_outlineVertices[1].position;

        // Update outline colors
        updateOutlineColors();

        // Update the shape's bounds
        m_bounds = m_outlineVertices.getBounds();

    }


////////////////////////////////////////////////////////////
    void Shape::updateOutlineColors() {
        for (std::size_t i = 0; i < m_outlineVertices.getVertexCount(); ++i)
            m_outlineVertices[i].color = m_outlineColor;
        m_outlineVertices.update();
    }

#ifdef __BOX2D__

    b2Body *Shape::addPhysicsBody(PhysicsWorld *world, b2BodyType type, float density, float friction) {
        if (!m_body) {

            m_world = world;
            m_bodyDef.type = type;
            m_bodyDef.position.Set(getPosition().x / m_world->getPixelsPerMeter(),
                                   getPosition().y / m_world->getPixelsPerMeter());
            m_body = m_world->getPhysics()->CreateBody(&m_bodyDef);

            Vector2f boxSize = {(getSize().x / world->getPixelsPerMeter()) / 2,
                                (getSize().y / world->getPixelsPerMeter()) / 2};

            m_polyShape.m_type = b2Shape::Type::e_polygon;
            m_polyShape.m_centroid = {boxSize.x, boxSize.y};

            size_t count = getPointCount();
            auto vs = (b2Vec2 *) malloc(sizeof(b2Vec2) * count);
            for (size_t i = 0; i < count; i++) {
                vs[i].Set(getPoint(i).x / world->getPixelsPerMeter(),
                          getPoint(i).y / world->getPixelsPerMeter());
            }
            m_polyShape.Set(vs, (int32) count);
            free(vs);

            m_fixtureDef.shape = &m_polyShape;
            m_fixtureDef.density = density;
            m_fixtureDef.friction = friction;
            m_fixture = m_body->CreateFixture(&m_fixtureDef);

            updateTexCoords();
        }

        return m_body;
    }

    void Shape::removePhysicsBody() {
        if (m_world && m_world->getPhysics() && m_body) {
            m_world->getPhysics()->DestroyBody(m_body);
            m_body = nullptr;
            m_fixture = nullptr;
        }
    }

    b2Body *Shape::getPhysicsBody() {
        return m_body;
    }

    b2Fixture *Shape::getPhysicsBodyFixture() {
        return m_fixture;
    }

#endif

} // namespace sf
