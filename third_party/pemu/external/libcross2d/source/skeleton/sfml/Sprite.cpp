////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2018 Laurent Gomila (laurent@sfml-dev.org)
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

namespace c2d {

////////////////////////////////////////////////////////////
    Sprite::Sprite() : RectangleShape({0, 0}) {}

////////////////////////////////////////////////////////////
    Sprite::Sprite(Texture *texture) : RectangleShape(texture->getSize()) {
        setTexture(texture, true);
    }

////////////////////////////////////////////////////////////
    Sprite::Sprite(Texture *texture, const IntRect &rectangle)
            : RectangleShape({(float) rectangle.left, (float) rectangle.top,
                              (float) rectangle.width, (float) rectangle.height}) {
        m_tex_rect = rectangle;
        Shape::setTextureRect(rectangle);
        setTexture(texture);
    }

    void Sprite::setTextureRect(const IntRect &rect) {
        RectangleShape::setSize((float) rect.width, (float) rect.height);
        Shape::setTextureRect(rect);
    }

} // namespace c2d
