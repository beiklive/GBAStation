//
// Created by cpasjuste on 25/05/2021.
//

#ifdef __BOX2D__

#include "cross2d/c2d.h"
#include "cross2d/skeleton/physicsworld.h"

using namespace c2d;

PhysicsWorld::PhysicsWorld(const c2d::Vector2f &gravity, const c2d::Vector2i &iterations, float pixelsPerMeter)
        : Rectangle({10000, 10000}) {

    Rectangle::setOrigin(Origin::BottomLeft);
    Rectangle::setPosition(0, c2d_renderer->getSize().y - 10000);
    Rectangle::setScale(1.0f, -1.0f);

    m_world = new b2World({gravity.x, gravity.y});
    m_iterations = iterations;
    m_pixelsPerMeter = pixelsPerMeter;
}

void PhysicsWorld::onUpdate() {

    if (m_world && !physicsPaused) {
        m_world->Step(c2d_renderer->getDeltaTime().asSeconds(), m_iterations.x, m_iterations.y);
    }

    Rectangle::onUpdate();
}

PhysicsWorld::~PhysicsWorld() {

    if (m_world) {
        b2Body *b = m_world->GetBodyList();
        while (b) {
            b2Body *bNext = b->GetNext();
            m_world->DestroyBody(b);
            b = bNext;
        }
        delete (m_world);
        m_world = nullptr;
    }
}

#endif