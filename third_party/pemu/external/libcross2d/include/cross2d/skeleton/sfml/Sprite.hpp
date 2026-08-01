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

#ifndef C2D_SPRITE_HPP
#define C2D_SPRITE_HPP

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "cross2d/skeleton/sfml/RectangleShape.hpp"

namespace c2d {

////////////////////////////////////////////////////////////
/// \brief Drawable representation of a texture, with its
///        own transformations, color, etc.
///
////////////////////////////////////////////////////////////
    class Sprite : public RectangleShape {
    public:

        ////////////////////////////////////////////////////////////
        /// \brief Default constructor
        ///
        /// Creates an empty sprite with no source texture.
        ///
        ////////////////////////////////////////////////////////////
        Sprite();

        ////////////////////////////////////////////////////////////
        /// \brief Construct the sprite from a source texture
        ///
        /// \param texture Source texture
        ///
        /// \see setTexture
        ///
        ////////////////////////////////////////////////////////////
        explicit Sprite(Texture *texture);

        ////////////////////////////////////////////////////////////
        /// \brief Construct the sprite from a sub-rectangle of a source texture
        ///
        /// \param texture   Source texture
        /// \param rectangle Sub-rectangle of the texture to assign to the sprite
        ///
        /// \see setTexture, setTextureRect
        ///
        ////////////////////////////////////////////////////////////
        Sprite(Texture *texture, const IntRect &rectangle);

        void setTextureRect(const IntRect &rect) override;
    };

} // namespace c2d

#endif // C2D_SPRITE_HPP
