#pragma once
#include <borealis.hpp>
#include "common.hpp"


class SettingPage : public beiklive::UI::BBox
{
  public:
    SettingPage();
    ~SettingPage();
    void Init();

  private:
    brls::TabFrame* m_tabframe = nullptr;
};