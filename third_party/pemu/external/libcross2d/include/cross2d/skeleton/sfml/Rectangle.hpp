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

#ifndef C2D_RECTANGLE_HPP
#define C2D_RECTANGLE_HPP

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Transformable.hpp"

namespace c2d {
////////////////////////////////////////////////////////////
/// \brief Specialized shape representing a rectangle
///
////////////////////////////////////////////////////////////
    class Rectangle : public Transformable {
    public:

        ////////////////////////////////////////////////////////////
        /// \brief Default constructor
        ///
        /// \param size Size of the rectangle
        ///
        ////////////////////////////////////////////////////////////
        explicit Rectangle(const c2d::Vector2f &size = c2d::Vector2f(0, 0));

        explicit Rectangle(const c2d::FloatRect &rect = c2d::FloatRect(0, 0, 0, 0));

        ////////////////////////////////////////////////////////////
        /// \brief Set the size of the rectangle
        ///
        /// \param size New size of the rectangle
        ///
        /// \see getSize
        ///
        ////////////////////////////////////////////////////////////
        void setSize(const c2d::Vector2f &size) override;

        void setSize(float width, float height) override;

        ////////////////////////////////////////////////////////////
        /// \brief Get the size of the rectangle
        ///
        /// \return Size of the rectangle
        ///
        /// \see setSize
        ///
        ////////////////////////////////////////////////////////////
        c2d::Vector2f getSize() override;

        FloatRect getLocalBounds() const override;

        FloatRect getGlobalBounds() const;

        Origin getOrigin() const override;

        void setOrigin(const Origin &origin) override;

    protected:
        Vector2f m_size; ///< Size of the rectangle

    private:

        ////////////////////////////////////////////////////////////
        // Member data
        ////////////////////////////////////////////////////////////
        Origin m_rect_origin = Origin::TopLeft;
    };

} // namespace c2d

#endif // C2D_RECTANGLE_HPP
