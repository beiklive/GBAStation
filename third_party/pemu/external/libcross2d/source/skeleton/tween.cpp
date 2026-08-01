//
// Created by cpasjuste on 23/09/18.
//

#include "cross2d/c2d.h"
#include "cross2d/skeleton/tweeny/include/tweeny.h"

using namespace c2d;
using namespace tweeny;

Tween::Tween(float from, float to, float duration, TweenLoop loop, TweenState state) {

    this->tween = tweeny::from(from, 0.0f, 0.0f, 0.0f).to(to, 0.0f, 0.0f, 0.0f)
            .during(duration * 1000);
    this->from[0] = from;
    this->to[0] = to;
    this->loop = loop;
    this->state = state;
    this->deltaClock = new C2DClock();
}

Tween::Tween(const Vector2f &from, const Vector2f &to, float duration, TweenLoop loop, TweenState state) {

    this->tween = tweeny::from(from.x, from.y, 0.0f, 0.0f).to(to.x, to.y, 0.0f, 0.0f)
            .during(duration * 1000);
    this->from[0] = from.x;
    this->from[1] = from.y;
    this->to[0] = to.x;
    this->to[1] = to.y;
    this->loop = loop;
    this->state = state;
    this->deltaClock = new C2DClock();
}

Tween::Tween(const Color &from, const Color &to, float duration, TweenLoop loop, TweenState state) {

    this->tween = tweeny::from(
            (float) from.r / 255.0f, (float) from.g / 255.0f, (float) from.b / 255.0f, (float) from.a / 255.0f)
            .to((float) to.r / 255.0f, (float) to.g / 255.0f, (float) to.b / 255.0f, (float) to.a / 255.0f)
            .during(duration * 1000);

    this->from[0] = from.r;
    this->from[1] = from.g;
    this->from[2] = from.b;
    this->from[3] = from.a;
    this->to[0] = to.r;
    this->to[1] = to.g;
    this->to[2] = to.b;
    this->to[3] = to.a;
    this->loop = loop;
    this->state = state;
    this->deltaClock = new C2DClock();
}

void TweenPosition::setFromTo(const Vector2f &from, const Vector2f &to, float duration) {
    tween = tweeny::from(from.x, from.y, 0.0f, 0.0f).to(to.x, to.y, 0.0f, 0.0f)
            .during(duration > 0 ? duration * 1000 : tween.duration());
    this->from[0] = from.x;
    this->from[1] = from.y;
    this->to[0] = to.x;
    this->to[1] = to.y;
}

void TweenRotation::setFromTo(float from, float to, float duration) {
    tween = tweeny::from(from, 0.0f, 0.0f, 0.0f).to(to, 0.0f, 0.0f, 0.0f)
            .during(duration > 0 ? duration * 1000 : tween.duration());
    this->from[0] = from;
    this->to[0] = to;
}

void TweenScale::setFromTo(const Vector2f &from, const Vector2f &to, float duration) {
    tween = tweeny::from(from.x, from.y, 0.0f, 0.0f).to(to.x, to.y, 0.0f, 0.0f)
            .during(duration > 0 ? duration * 1000 : tween.duration());
    this->from[0] = from.x;
    this->from[1] = from.y;
    this->to[0] = to.x;
    this->to[1] = to.y;
}

void TweenSize::setFromTo(const Vector2f &from, const Vector2f &to, float duration) {
    tween = tweeny::from(from.x, from.y, 0.0f, 0.0f).to(to.x, to.y, 0.0f, 0.0f)
            .during(duration > 0 ? duration * 1000 : tween.duration());
    this->from[0] = from.x;
    this->from[1] = from.y;
    this->to[0] = to.x;
    this->to[1] = to.y;
}

void TweenColor::setFromTo(const Color &from, const Color &to, float duration) {
    tween = tweeny::from(
            (float) from.r / 255.0f, (float) from.g / 255.0f, (float) from.b / 255.0f, (float) from.a / 255.0f)
            .to((float) to.r / 255.0f, (float) to.g / 255.0f, (float) to.b / 255.0f, (float) to.a / 255.0f)
            .during(duration > 0 ? duration * 1000 : tween.duration());
    this->from[0] = from.r;
    this->from[1] = from.g;
    this->from[2] = from.b;
    this->from[3] = from.a;
    this->to[0] = to.r;
    this->to[1] = to.g;
    this->to[2] = to.b;
    this->to[3] = to.a;
}

void TweenAlpha::setFromTo(float from, float to, float duration) {
    tween = tweeny::from(from / 255, 0.0f, 0.0f, 0.0f).to(to / 255, 0.0f, 0.0f, 0.0f)
            .during(duration > 0 ? duration * 1000 : tween.duration());
    this->from[0] = from;
    this->to[0] = to;
}

Tween::~Tween() {

    printf("~Tween(%p)\n", this);
    if (deltaClock != nullptr) {
        delete (deltaClock);
        deltaClock = nullptr;
    }
}

void Tween::setTransform(c2d::Transformable *t) {

    transform = t;
}

void Tween::play(TweenDirection dir, bool _reset) {

    if (dir != TweenDirection::Current) {
        setDirection(dir);
    }

    if (_reset) {
        reset();
    }

    deltaClock->restart();
    state = TweenState::Playing;
}

void Tween::reset() {

    if (tween.duration() > 0) {
        tween.seek((float) 0);
    }
}

TweenState Tween::getState() {
    return state;
}

void Tween::setState(TweenState st) {
    state = st;
}

TweenDirection Tween::getDirection() {
    return direction;
}

void Tween::setDirection(TweenDirection dir) {

    direction = dir;

    if (direction == TweenDirection::Backward) {
        tween.backward();
    } else if (direction == TweenDirection::Forward) {
        tween.forward();
    }
};

void Tween::step() {

    float progress = 0, duration = 0;

    delta = deltaClock->restart();

    if (transform == nullptr || state == TweenState::Stopped) {
        state = TweenState::Stopped;
        return;
    }

    progress = tween.progress();
    duration = tween.duration();

    if ((duration > 0 && (progress <= 0.0f || progress >= 1.0f))) {
        if (loop == TweenLoop::None) {
            if ((progress >= 1.0f && direction == TweenDirection::Forward)
                || (progress <= 0.0f && direction == TweenDirection::Backward)) {
                state = TweenState::Stopped;
                return;
            }
        } else if (loop == TweenLoop::Loop) {
            reset();
        } else if (loop == TweenLoop::PingPong) {
            if (direction == TweenDirection::Forward) {
                setDirection(TweenDirection::Backward);
            } else {
                setDirection(TweenDirection::Forward);
            }
        }
    }

    auto *object = (C2DObject *) transform;

    auto float4 = tween.step((int32_t) delta.asMilliseconds(), true);
    if (type == TweenType::Position) {
        transform->setPosition(float4[0], float4[1]);
    } else if (type == TweenType::Rotation) {
        transform->setRotation(float4[0]);
    } else if (type == TweenType::Scale) {
        transform->setScale(float4[0], float4[1]);
    } else if (type == TweenType::Size) {
        if (object->getType() == Type::Text) {
            ((Text *) transform)->setCharacterSize((unsigned int) float4[0]);
        } else {
            ((RectangleShape *) transform)->setSize(float4[0], float4[1]);
        }
    } else if (type == TweenType::Color) {
        Color color = {(uint8_t) (float4[0] * 255.0f), (uint8_t) (float4[1] * 255.0f),
                       (uint8_t) (float4[2] * 255.0f), (uint8_t) (float4[3] * 255.0f)};
        if (object->getType() == Type::Text) {
            ((Text *) transform)->setFillColor(color);
        } else {
            ((Shape *) transform)->setFillColor(color);
        }
    } else if (type == TweenType::Alpha) {
        auto alpha = (uint8_t) (float4[0] * 255);
        if (object->getType() == Type::Text) {
            ((Text *) transform)->setAlpha(alpha, true);
        } else {
            ((Shape *) transform)->setAlpha(alpha, true);
        }
    }
}
