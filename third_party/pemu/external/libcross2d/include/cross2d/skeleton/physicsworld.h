//
// Created by cpasjuste on 25/05/2021.
//

#ifndef C2D_PHYSICSWORLD_H
#define C2D_PHYSICSWORLD_H

#ifdef __BOX2D__

#include <cstdio>
#include <box2d/box2d.h>
#include "cross2d/skeleton/sfml/Rectangle.hpp"

class PhysicsWorld : public c2d::Rectangle {

public:
    explicit PhysicsWorld(const c2d::Vector2f &gravity = {0, -10},
                          const c2d::Vector2i &iterations = {6, 2}, float m_pixelsPerMeter = 32);

    ~PhysicsWorld() override;

    b2World *getPhysics() {
        return m_world;
    }

    void setPaused(bool paused) {
        physicsPaused = paused;
    }

    bool isPaused() const {
        return physicsPaused;
    }

    void setPixelsPerMeter(float ppm) {
        m_pixelsPerMeter = ppm;
    }

    float getPixelsPerMeter() {
        return m_pixelsPerMeter;
    }

    void setScale(float factorX, float factorY) override {
        printf("PhysicsWorld::setScale: not implemented\n");
    }

    void setScale(const c2d::Vector2f &factors) override {
        printf("PhysicsWorld::setScale: not implemented\n");
    }

    void setSize(const c2d::Vector2f &size) override {
        printf("PhysicsWorld::setSize: not implemented\n");
    }

    void setSize(float width, float height) override {
        printf("PhysicsWorld::setSize: not implemented\n");
    }

    void setOrigin(const c2d::Origin &origin) override {
        printf("PhysicsWorld::setOrigin: not implemented\n");
    }

    void setOriginVector(float x, float y) override {
        printf("PhysicsWorld::setOriginVector: not implemented\n");
    }

    void setOriginVector(const c2d::Vector2f &origin) override {
        printf("PhysicsWorld::setOriginVector: not implemented\n");
    }

protected:
    void onUpdate() override;

private:
    b2World *m_world = nullptr;
    c2d::Vector2i m_iterations = {6, 2};
    float m_pixelsPerMeter = 32;
    bool physicsPaused = false;
};

#endif //__BOX2D__

#endif //C2D_PHYSICSWORLD_H
