#pragma once

#include <borealis.hpp>

#include "common.hpp"

class StartPageView : public brls::Box
{
  public:
    StartPageView();
    ~StartPageView();
    void Init();
    void onFocusGained() override;
    void onFocusLost() override;
    void onLayout() override;

    static brls::View* create();

  private:
    brls::Image* m_bgImage = nullptr;
};

