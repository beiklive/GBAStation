//
// Created by cpasjuste on 09/12/18.
//

#ifndef C2D_GRADIENT_RECTANGLE_H
#define C2D_GRADIENT_RECTANGLE_H

namespace c2d {

    class GradientRectangle : public Transformable {
    public:

        enum Direction {
            Up, Down, Left, Right, Default
        };

        explicit GradientRectangle(const Vector2f &size = Vector2f());

        explicit GradientRectangle(const FloatRect &rect = FloatRect());

        void setSize(const Vector2f &size) override;

        void setSize(float width, float height) override;

        Vector2f getSize() override;

        FloatRect getLocalBounds() const override;

        FloatRect getGlobalBounds() const;

        void setOrigin(const Origin &origin) override;

        Origin getOrigin() const override;

        void setColor(const Color &color1, const Color &color2, const Direction &direction = Default);

        void setFillColor(const Color &color) override;

        const Color &getFillColor() const override;

        void setAlpha(uint8_t alpha, bool recursive = false) override;

        uint8_t getAlpha() override;

    protected:

        void onDraw(Transform &transform, bool draw = true) override;

    private:

        void updatePositions();

        Direction m_direction = Direction::Right;
        VertexArray m_vertices;
        Vector2f m_size;
    };
}

#endif //C2D_GRADIENT_RECTANGLE_H
