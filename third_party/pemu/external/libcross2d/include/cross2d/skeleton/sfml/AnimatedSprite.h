//
// Created by cpasjuste on 04/06/2021.
//

#ifndef C2D_ANIMATEDSPRITE_H
#define C2D_ANIMATEDSPRITE_H

namespace c2d {

    class AnimatedSprite : public Sprite {
    public:
        AnimatedSprite(Texture *texture, const Vector2i &frameSize, int rows, int frameCount, float duration);

        ~AnimatedSprite() override;

        void play();

        void stop();

        void pause();

        void resume();

        void setDuration(float duration);

        float getDuration() const;

    protected:
        void onUpdate() override;

    private:
        int m_frameCount;
        int m_currentFrame = 0;
        std::vector<IntRect> m_textureRects;
        float m_duration = 1.0f;
        float m_durationPerFrame;
        bool m_paused = false;
        Clock *m_clock;
    };
}

#endif //C2D_ANIMATEDSPRITE_H
