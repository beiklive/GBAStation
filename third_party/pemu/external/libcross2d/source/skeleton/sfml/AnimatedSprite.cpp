//
// Created by cpasjuste on 04/06/2021.
//

#include "cross2d/c2d.h"

using namespace c2d;

AnimatedSprite::AnimatedSprite(Texture *texture, const Vector2i &frameSize,
                               int rows, int frameCount, float speed) : Sprite(texture) {
    m_clock = new C2DClock;
    m_frameCount = frameCount;
    m_duration = speed;
    m_durationPerFrame = m_duration / (float) m_frameCount;

    int x_advance = 0;
    int y_advance = 0;

    for (int i = 0; i < frameCount; i++) {
        int x1 = frameSize.x * x_advance;
        int y1 = frameSize.y * y_advance;
        m_textureRects.emplace_back(x1, y1, frameSize.x, frameSize.y);
        x_advance++;
        if (x_advance == rows) {
            x_advance = 0;
            y_advance++;
        }
    }

    setTextureRect(m_textureRects[0]);
}

void AnimatedSprite::play() {
    m_currentFrame = 0;
    m_clock->restart();
    m_paused = false;
}

void AnimatedSprite::stop() {
    m_currentFrame = 0;
    m_paused = true;
}

void AnimatedSprite::pause() {
    m_paused = true;
}

void AnimatedSprite::resume() {
    m_paused = false;
}

void AnimatedSprite::setDuration(float duration) {
    m_duration = duration;
    m_durationPerFrame = m_duration / (float) m_frameCount;
}

float AnimatedSprite::getDuration() const {
    return m_duration;
}

void AnimatedSprite::onUpdate() {

    if (!m_paused) {
        if (m_clock->getElapsedTime().asSeconds() >= m_durationPerFrame) {
            m_currentFrame++;
            if (m_currentFrame >= m_frameCount) {
                m_currentFrame = 0;
            }
            setTextureRect(m_textureRects[m_currentFrame]);
            m_clock->restart();
        }
    }

    Sprite::onUpdate();
}

AnimatedSprite::~AnimatedSprite() {
    delete (m_clock);
}
