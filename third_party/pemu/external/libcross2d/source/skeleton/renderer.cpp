//
// Created by cpasjuste on 09/12/16.
//

#include "cross2d/c2d.h"

using namespace c2d;

Renderer *c2d_renderer = nullptr;

#ifdef __SWITCH__

void initNxLink();

#endif

Renderer::Renderer(const Vector2f &size) : Rectangle(size) {
#ifdef __SWITCH__
    initNxLink();
#endif
    printf("Renderer(%p)\n", this);

    // set static access to renderer (TODO: handle this in a smarter way)
    c2d_renderer = this;

    m_input = new C2DInput();
    m_io = new C2DIo();
    m_font = new C2DFont();
    m_font->loadDefault();
    m_elapsedClock = new C2DClock();
    m_deltaClock = new C2DClock();
    m_fpsClock = new C2DClock();
}

void Renderer::onUpdate() {
    // time
    m_deltaTime = m_deltaClock->restart();

    // fps
    if (m_fpsClock->getElapsedTime().asSeconds() >= 1) {
        m_fps = m_frames / m_fpsClock->getElapsedTime().asSeconds();
        m_fpsClock->restart();
        m_frames = 0;
        if (m_stats_print) {
            printf("fps: %f\n", m_fps);
        }
    }
    m_frames++;

    // input
    if (m_process_inputs) {
        auto players = m_input->update();
        for (int i = 0; i < PLAYER_MAX; i++) {
            unsigned int buttons = players[i].buttons;
            if (buttons > 0 && buttons != Input::Button::Delay) {
                onInput(players);
                break;
            }
        }
    }

    Rectangle::onUpdate();
}

void Renderer::flip(bool draw, bool inputs) {
    m_process_inputs = inputs;
    onUpdate();

    // call base class (draw childs)
    if (draw) {
        clear();
        Transform trans = Transform::Identity;
        Rectangle::onDraw(trans, draw);
    }
}

void Renderer::setClearColor(const Color &color) {
    m_clearColor = color;
}

Color Renderer::getClearColor() const {
    return m_clearColor;
}

Time Renderer::getElapsedTime() const {
    return m_elapsedClock->getElapsedTime();
}

Time Renderer::getDeltaTime() const {
    return m_deltaTime;
}

float Renderer::getFps() const {
    return m_fps;
}

void Renderer::setShaderList(ShaderList *list) {
    m_shaderList = list;
}

ShaderList *Renderer::getShaderList() {
    return m_shaderList;
}

Renderer::~Renderer() {
    printf("~Renderer(%p)\n", this);

    delete (m_font);
    delete (m_io);
    delete (m_input);

    delete (m_elapsedClock);
    delete (m_deltaClock);
    delete (m_fpsClock);

    if (m_shaderList) {
        delete (m_shaderList);
    }
}
