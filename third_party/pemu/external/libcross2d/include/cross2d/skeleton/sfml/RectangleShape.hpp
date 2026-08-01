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

#ifndef C2D_RECTANGLESHAPE_HPP
#define C2D_RECTANGLESHAPE_HPP

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Shape.hpp"

namespace c2d {
////////////////////////////////////////////////////////////
/// \brief Specialized shape representing a rectangle
///
////////////////////////////////////////////////////////////
    class RectangleShape : public Shape {
    public:

        ////////////////////////////////////////////////////////////
        /// \brief Default constructor
        ///
        /// \param size Size of the rectangle
        ///
        ////////////////////////////////////////////////////////////
        explicit RectangleShape(const c2d::Vector2f &size = c2d::Vector2f());

        explicit RectangleShape(const c2d::FloatRect &rect = c2d::FloatRect());

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

        ////////////////////////////////////////////////////////////
        /// \brief Set the radius of the rounded corners
        ///
        /// \param Radius of the rounded corners
        ///
        /// \see GetRadius
        ///
        ////////////////////////////////////////////////////////////
        void setCornersRadius(float radius);

        ////////////////////////////////////////////////////////////
        /// \brief Get the radius of the rounded corners
        ///
        /// \return Radius of the rounded corners
        ///
        /// \see SetRadius
        ///
        ////////////////////////////////////////////////////////////
        float getCornersRadius() const;

        ////////////////////////////////////////////////////////////
        /// \brief Set the number of points of each corner
        ///
        /// \param count New number of points of the rounded rectangle
        ///
        /// \see GetPointCount
        ///
        ////////////////////////////////////////////////////////////
        void setCornerPointCount(unsigned int count);

        ////////////////////////////////////////////////////////////
        /// \brief Get the number of points defining the shape
        ///
        /// \return Number of points of the shape. For rectangle
        ///         shapes, this number is always 4.
        ///
        ////////////////////////////////////////////////////////////
        std::size_t getPointCount() const override;

        ////////////////////////////////////////////////////////////
        /// \brief Get a point of the rectangle
        ///
        /// The returned point is in local coordinates, that is,
        /// the shape's transforms (position, rotation, scale) are
        /// not taken into account.
        /// The result is undefined if \a index is out of the valid range.
        ///
        /// \param index Index of the point to get, in range [0 .. 3]
        ///
        /// \return index-th point of the shape
        ///
        ////////////////////////////////////////////////////////////
        c2d::Vector2f getPoint(std::size_t index) const override;

    private:

        ////////////////////////////////////////////////////////////
        // Member data
        ////////////////////////////////////////////////////////////
        c2d::Vector2f m_size; ///< Size of the rectangle
        float m_radius = 0;
        unsigned int m_corner_point_count = 1;
    };

} // namespace c2d

#endif // C2D_RECTANGLESHAPE_HPP


////////////////////////////////////////////////////////////
/// \class sf::RectangleShape
/// \ingroup graphics
///
/// This class inherits all the functions of sf::Transformable
/// (position, rotation, scale, bounds, ...) as well as the
/// functions of sf::Shape (outline, color, texture, ...).
///
/// Usage example:
/// \code
/// sf::RectangleShape rectangle;
/// rectangle.setSize(sf::Vector2f(100, 50));
/// rectangle.setOutlineColor(sf::Color::Red);
/// rectangle.setOutlineThickness(5);
/// rectangle.setPosition(10, 20);
/// ...
/// window.draw(rectangle);
/// \endcode
///
/// \see sf::Shape, sf::CircleShape, sf::ConvexShape
///
////////////////////////////////////////////////////////////
