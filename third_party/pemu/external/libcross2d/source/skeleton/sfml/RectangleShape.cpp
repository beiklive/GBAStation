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

#include "cross2d/skeleton/sfml/RectangleShape.hpp"

namespace c2d {

////////////////////////////////////////////////////////////
    RectangleShape::RectangleShape(const Vector2f &size) {
        RectangleShape::setSize(size);
    }

    RectangleShape::RectangleShape(const FloatRect &rect) {
        RectangleShape::setSize(rect.width, rect.height);
        RectangleShape::setPosition(rect.left, rect.top);
    }

////////////////////////////////////////////////////////////
    void RectangleShape::setSize(const Vector2f &size) {
        if (m_size != size) {
            m_size = size;
            update();
        }
    }

    void RectangleShape::setSize(float width, float height) {
        if (width != m_size.x || height != m_size.y) {
            m_size.x = width;
            m_size.y = height;
            update();
        }
    }

////////////////////////////////////////////////////////////
    Vector2f RectangleShape::getSize() {
        return m_size;
    }

    ////////////////////////////////////////////////////////////
    void RectangleShape::setCornersRadius(float radius) {
        if (m_radius != radius) {
            m_radius = radius;
            update();
        }
    }

////////////////////////////////////////////////////////////
    float RectangleShape::getCornersRadius() const {
        return m_radius;
    }

////////////////////////////////////////////////////////////
    void RectangleShape::setCornerPointCount(unsigned int count) {
        if (m_corner_point_count != count) {
            m_corner_point_count = count;
            update();
        }
    }

////////////////////////////////////////////////////////////
    std::size_t RectangleShape::getPointCount() const {
        if (m_radius <= 0) {
            return 4;
        }
        return m_corner_point_count * 4;
    }

////////////////////////////////////////////////////////////
    Vector2f RectangleShape::getPoint(std::size_t index) const {
        if (m_radius <= 0) {
            switch (index) {
                default:
                case 0:
                    return {0, 0};
                case 1:
                    return {m_size.x, 0};
                case 2:
                    return {m_size.x, m_size.y};
                case 3:
                    return {0, m_size.y};
            }
        }

        if (index >= m_corner_point_count * 4) {
            return {};
        }

        Vector2f center;
        float deltaAngle = 90.0f / (float) (m_corner_point_count - 1);
        unsigned int centerIndex = index / m_corner_point_count;
        static const float pi = 3.141592654f;

        switch (centerIndex) {
            case 0:
                center.x = m_size.x - m_radius;
                center.y = m_radius;
                break;
            case 1:
                center.x = m_radius;
                center.y = m_radius;
                break;
            case 2:
                center.x = m_radius;
                center.y = m_size.y - m_radius;
                break;
            case 3:
                center.x = m_size.x - m_radius;
                center.y = m_size.y - m_radius;
                break;
            default:
                return {0, 0};
        }

        return {m_radius * std::cos(deltaAngle * (float) (index - centerIndex) * pi / 180) + center.x,
                -m_radius * std::sin(deltaAngle * (float) (index - centerIndex) * pi / 180) + center.y};
    }

} // namespace sf
