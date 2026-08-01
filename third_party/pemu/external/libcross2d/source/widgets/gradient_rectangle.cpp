//
// Created by cpasjuste on 09/12/18.
//

#include "cross2d/c2d.h"

using namespace c2d;

GradientRectangle::GradientRectangle(const Vector2f &size) :
        m_vertices(TriangleStrip, 4) {
    GradientRectangle::setSize(size);
}

GradientRectangle::GradientRectangle(const FloatRect &rect) :
        m_vertices(TriangleStrip, 4) {
    GradientRectangle::setSize(rect.width, rect.height);
    GradientRectangle::setPosition(rect.left, rect.top);
}

void GradientRectangle::setSize(const Vector2f &size) {
    m_size = size;
    updatePositions();
}

void GradientRectangle::setSize(float width, float height) {
    m_size.x = width;
    m_size.y = height;
    updatePositions();
}

Vector2f GradientRectangle::getSize() {
    return m_size;
}

FloatRect GradientRectangle::getLocalBounds() const {
    return {0.f, 0.f, m_size.x, m_size.y};
}

FloatRect GradientRectangle::getGlobalBounds() const {
    Transform t = transformation * getTransform();
    return t.transformRect(getLocalBounds());
}

void GradientRectangle::setOrigin(const Origin &origin) {
    FloatRect m_bounds = getLocalBounds();
    Transformable::setOrigin(origin);

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
            Transformable::setOriginVector(m_bounds.width / 2, m_bounds.height / 2);
            break;
        default:
            break;
    }
}

Origin GradientRectangle::getOrigin() const {
    return Transformable::getOrigin();
}

void GradientRectangle::setColor(const Color &color1, const Color &color2, const Direction &direction) {
    if (direction != Default) {
        m_direction = direction;
    }
    if (m_direction == Right) {
        m_vertices[0].color = color1;
        m_vertices[1].color = color1;
        m_vertices[2].color = color2;
        m_vertices[3].color = color2;
    } else if (m_direction == Left) {
        m_vertices[0].color = color2;
        m_vertices[1].color = color2;
        m_vertices[2].color = color1;
        m_vertices[3].color = color1;
    } else if (m_direction == Up) {
        m_vertices[0].color = color2;
        m_vertices[1].color = color1;
        m_vertices[2].color = color2;
        m_vertices[3].color = color1;
    } else if (m_direction == Down) {
        m_vertices[0].color = color1;
        m_vertices[1].color = color2;
        m_vertices[2].color = color1;
        m_vertices[3].color = color2;
    }
    m_vertices.update();
}

void GradientRectangle::setFillColor(const Color &color) {
    m_vertices[0].color = color;
    m_vertices[1].color = color;
    m_vertices[2].color = color;
    m_vertices[3].color = color;
    m_vertices.update();
}

const Color &GradientRectangle::getFillColor() const {
    return m_vertices[0].color;
}

void GradientRectangle::setAlpha(uint8_t alpha, bool recursive) {
    if (alpha != m_vertices[0].color.a) {
        m_vertices[0].color.a = alpha;
        m_vertices[1].color.a = alpha;
        m_vertices[2].color.a = alpha;
        m_vertices[3].color.a = alpha;
        m_vertices.update();
    }

    if (recursive) {
        C2DObject::setAlpha(alpha, recursive);
    }
}

uint8_t GradientRectangle::getAlpha() {
    return m_vertices[0].color.a;
}

void GradientRectangle::updatePositions() {
    setOrigin(getOrigin());
    FloatRect bounds = getLocalBounds();
    m_vertices[0].position = Vector2f(0, 0);
    m_vertices[1].position = Vector2f(0, bounds.height);
    m_vertices[2].position = Vector2f(bounds.width, 0);
    m_vertices[3].position = Vector2f(bounds.width, bounds.height);
    m_vertices.update();
}

void GradientRectangle::onDraw(Transform &transform, bool draw) {
    if (draw) {
        Transform combined = transform * getTransform();
        c2d_renderer->draw(&m_vertices, combined, nullptr);
    }
    C2DObject::onDraw(transform, draw);
}
