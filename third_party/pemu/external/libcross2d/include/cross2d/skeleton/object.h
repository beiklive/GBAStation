//
// Created by cpasjuste on 12/12/17.
//

#ifndef CROSS2D_OBJECT_H
#define CROSS2D_OBJECT_H

#include <cstdint>
#include <vector>
#include "cross2d/skeleton/sfml/Transform.hpp"
#include "tween.h"
#include "input.h"

namespace c2d {

    enum class Origin : int {
        Left, TopLeft, Top, TopRight,
        Right, BottomRight, Bottom, BottomLeft, Center
    };

    enum class Visibility : int {
        Visible, Hidden
    };

    enum class DeleteMode : int {
        Auto, Manual
    };

    enum class Type : int {
        Shape, Texture, Text, Other
    };

    class C2DObject {

    public:

        virtual ~C2DObject();

        // childs
        virtual void add(C2DObject *object);

        // remove a widget without calling it's destructor
        virtual void remove(C2DObject *object);

        // tweeners
        virtual void add(Tween *tween);

        // remove a tweener without calling it's destructor
        virtual void remove(Tween *tween);

        // visibility
        virtual bool isVisible();

        virtual void setVisibility(Visibility visibility, bool tweenPlay = false);

        // deletion mode
        virtual DeleteMode getDeleteMode();

        virtual void setDeleteMode(DeleteMode mode);

        virtual int getLayer();

        virtual void setLayer(int layer);

        virtual std::vector<C2DObject *> getChilds();

        virtual C2DObject *getChild(const std::string &name);

        Type getType() const;

        bool available = false;

        // common to all objects (Rectangle, Texture, Text..)
        virtual FloatRect getLocalBounds() const {
            return {v2_dummy.x, v2_dummy.x, v2_dummy.x, v2_dummy.x};
        };

        virtual c2d::Vector2f getSize() { return v2_dummy; };

        virtual void setSize(const c2d::Vector2f &size) {};

        virtual void setSize(float width, float height) {};

        virtual const Vector2f &getPosition() const { return v2_dummy; };

        virtual void setPosition(float x, float y) {};

        virtual void setPosition(const Vector2f &position) {};

        virtual float getRotation() const { return v2_dummy.x; };

        virtual void setRotation(float angle) {};

        virtual const Vector2f &getScale() const { return v2_dummy; };

        virtual void setScale(float factorX, float factorY) {};

        virtual void setScale(const Vector2f &factors) {};

        virtual void move(float offsetX, float offsetY) {};

        virtual void move(const Vector2f &offset) {};

        virtual void rotate(float angle) {};

        virtual void scale(float factorX, float factorY) {};

        virtual void scale(const Vector2f &factor) {};

        virtual void setOrigin(const Origin &origin) {};

        virtual Origin getOrigin() const { return Origin::TopLeft; };

        virtual const Color &getFillColor() const { return col_dummy; };

        virtual void setFillColor(const Color &color) {};

        virtual const Color &getOutlineColor() const { return col_dummy; };

        virtual void setOutlineColor(const Color &color) {};

        virtual float getOutlineThickness() const { return v2_dummy.x; };

        virtual void setOutlineThickness(float thickness) {};

        virtual uint8_t getAlpha() { return 0; };

        virtual void setAlpha(uint8_t alpha, bool recursive = false);

        virtual void setName(const std::string &name) { m_name = name; };

        virtual std::string getName() { return m_name; };

    protected:

        // always called, even if the object is hidden
        virtual void onUpdate();

        virtual bool onInput(Input::Player *players);

        virtual void onDraw(Transform &transform, bool draw = true);

        virtual void setDrawingState(bool isDrawn);

        Type type = Type::Other;
        Transform transformation;

    private:
        int layer = 0;
        std::string m_name;
        C2DObject *parent = nullptr;
        std::vector<Tween *> tweens;
        std::vector<C2DObject *> childs;
        DeleteMode deleteMode = DeleteMode::Auto;
        Visibility visibility_current = Visibility::Visible;
        Visibility visibility_wanted = Visibility::Visible;
        bool m_is_drawn = true;

        Vector2f v2_dummy;
        Color col_dummy;
    };
}

#endif //CROSS2D_OBJECT_H
