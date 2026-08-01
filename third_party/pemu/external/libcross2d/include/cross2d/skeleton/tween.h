//
// Created by cpasjuste on 23/09/18.
//

#ifndef C2D_TWEEN_H
#define C2D_TWEEN_H

#include "cross2d/skeleton/tweeny/include/tweeny.h"
#include "cross2d/skeleton/sfml/Clock.hpp"
#include "cross2d/skeleton/sfml/Color.hpp"
#include "cross2d/skeleton/sfml/Vector2.hpp"

namespace c2d {

    class Transformable;

    enum class TweenType : int {
        Position = 1, Rotation, Scale, Size, Color, Alpha
    };

    enum class TweenLoop : int {
        None = 1, Loop = 2, PingPong = 3,
    };

    enum class TweenDirection : int {
        Current, Forward, Backward
    };

    enum class TweenState : int {
        Playing = 1, Stopped
    };

    ///
    /// main tweener class
    ///
    class Tween {

    public:

        Tween(float from, float to, float duration,
              TweenLoop loop = TweenLoop::None, TweenState state = TweenState::Stopped);

        Tween(const Vector2f &from, const Vector2f &to, float duration,
              TweenLoop loop = TweenLoop::None, TweenState state = TweenState::Stopped);

        Tween(const Color &from, const Color &to, float duration,
              TweenLoop loop = TweenLoop::None, TweenState state = TweenState::Stopped);

        virtual ~Tween();

        void play(TweenDirection direction = TweenDirection::Current, bool reset = false);

        void reset();

        TweenState getState();

        void setState(TweenState state);

        TweenDirection getDirection();

        void setDirection(TweenDirection direction);

        void setTransform(Transformable *transform);

        void step();

    protected:
        // transform to tween
        Transformable *transform = nullptr;
        // tweener info
        TweenType type;
        TweenLoop loop;
        TweenDirection direction = TweenDirection::Current;
        TweenState state = TweenState::Stopped;
        tweeny::tween<float, float, float, float> tween;
        float from[4];
        float to[4];
        // timer stuff
        Clock *deltaClock = nullptr;
        Time delta;
        Time time;
    };

    ///
    /// Position tweener
    ///
    class TweenPosition : public Tween {

    public:

        TweenPosition(
                const Vector2f &from, const Vector2f &to, float duration,
                TweenLoop loop = TweenLoop::None, TweenState state = TweenState::Stopped)
                : Tween(from, to, duration, loop, state) {
            this->type = TweenType::Position;
        };

        void setFromTo(const Vector2f &from, const Vector2f &to, float duration = 0);

        Vector2f getFrom() {
            return {from[0], from[1]};
        }

        Vector2f getTo() {
            return {to[0], to[1]};
        }
    };

    ///
    /// Rotation tweener
    ///
    class TweenRotation : public Tween {

    public:

        TweenRotation(
                float from, float to, float duration,
                TweenLoop loop = TweenLoop::None, TweenState state = TweenState::Stopped)
                : Tween(from, to, duration, loop, state) {
            this->type = TweenType::Rotation;
        }

        void setFromTo(float from, float to, float duration = 0);

        float getFrom() {
            return from[0];
        }

        float getTo() {
            return to[0];
        }
    };

    ///
    /// Scale tweener
    ///
    class TweenScale : public Tween {

    public:

        TweenScale(
                const Vector2f &from, const Vector2f &to, float duration,
                TweenLoop loop = TweenLoop::None, TweenState state = TweenState::Stopped)
                : Tween(from, to, duration, loop, state) {
            this->type = TweenType::Scale;
        }

        void setFromTo(const Vector2f &from, const Vector2f &to, float duration = 0);

        Vector2f getFrom() {
            return {from[0], from[1]};
        }

        Vector2f getTo() {
            return {to[0], to[1]};
        }
    };

    ///
    /// Scale tweener
    ///
    class TweenSize : public Tween {

    public:

        TweenSize(
                const Vector2f &from, const Vector2f &to, float duration,
                TweenLoop loop = TweenLoop::None, TweenState state = TweenState::Stopped)
                : Tween(from, to, duration, loop, state) {
            this->type = TweenType::Size;
        }

        void setFromTo(const Vector2f &from, const Vector2f &to, float duration = 0);

        Vector2f getFrom() {
            return {from[0], from[1]};
        }

        Vector2f getTo() {
            return {to[0], to[1]};
        }
    };

    ///
    /// Color tweener
    ///
    class TweenColor : public Tween {

    public:

        TweenColor(
                const Color &from, const Color &to, float duration,
                TweenLoop loop = TweenLoop::None, TweenState state = TweenState::Stopped)
                : Tween(from, to, duration, loop, state) {
            this->type = TweenType::Color;
        }

        void setFromTo(const Color &from, const Color &to, float duration = 0);

        Color getFrom() {
            return {(uint8_t) from[0], (uint8_t) from[1], (uint8_t) from[2], (uint8_t) from[3]};
        }

        Color getTo() {
            return {(uint8_t) to[0], (uint8_t) to[1], (uint8_t) to[2], (uint8_t) to[3]};
        }
    };

    ///
    /// Alpha tweener
    ///
    class TweenAlpha : public Tween {

    public:

        TweenAlpha(
                float from, float to, float duration,
                TweenLoop loop = TweenLoop::None, TweenState state = TweenState::Stopped)
                : Tween(from / 255, to / 255, duration, loop, state) {
            this->type = TweenType::Alpha;
        }

        void setFromTo(float from, float to, float duration = 0);

        float getFrom() {
            return from[0] * 255;
        }

        float getTo() {
            return to[0] * 255;
        }
    };
}

#endif //C2D_TWEEN_H
