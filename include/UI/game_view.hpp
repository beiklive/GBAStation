#pragma once

#include <borealis.hpp>

#ifdef __SWITCH__


#include "common.hpp"


class GameView : public brls::Box
{
  public:
    GameView(std::string gameName);
    GameView();
    ~GameView();

    void draw(NVGcontext* vg, float x, float y, float width, float height,
              brls::Style style, brls::FrameContext* ctx) override;
    void onFocusGained() override;
    void onFocusLost() override;
    void onLayout() override;

  private:
    std::string m_gameName;
    bool m_exitRequested = false;

    void initialize();
};

#endif