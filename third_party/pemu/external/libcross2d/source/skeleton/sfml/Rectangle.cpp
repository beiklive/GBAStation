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

#include "cross2d/skeleton/sfml/Rectangle.hpp"

using namespace c2d;

namespace c2d {

////////////////////////////////////////////////////////////
    Rectangle::Rectangle(const Vector2f &size) {
        setSize(size);
    }

    Rectangle::Rectangle(const FloatRect &rect) {
        setSize(rect.width, rect.height);
        setPosition(rect.left, rect.top);
    }

////////////////////////////////////////////////////////////
    void Rectangle::setSize(const Vector2f &size) {
        m_size = size;
    }

    void Rectangle::setSize(float width, float height) {
        m_size.x = width;
        m_size.y = height;
    }

////////////////////////////////////////////////////////////
    Vector2f Rectangle::getSize() {
        return m_size;
    }

////////////////////////////////////////////////////////////
    FloatRect Rectangle::getLocalBounds() const {
        return {0.f, 0.f, m_size.x, m_size.y};
    }


////////////////////////////////////////////////////////////
    FloatRect Rectangle::getGlobalBounds() const {
        Transform t = transformation * getTransform();
        return t.transformRect(getLocalBounds());
    }

    void Rectangle::setOrigin(const Origin &origin) {

        FloatRect m_bounds = getLocalBounds();
        m_rect_origin = origin;

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

    Origin Rectangle::getOrigin() const {
        return m_rect_origin;
    }

} // namespace c2d
