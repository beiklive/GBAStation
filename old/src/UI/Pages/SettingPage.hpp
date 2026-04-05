#pragma once
#include <borealis.hpp>
#include "common.hpp"
#include "Control/InputMapping.hpp"


class SettingPage : public beiklive::UI::BBox
{
  public:
    SettingPage();
    ~SettingPage();
    void Init();

  private:
    brls::TabFrame* m_tabframe = nullptr;

    brls::ScrollingFrame* buildUITab();
    brls::ScrollingFrame* buildGameTab();
    brls::ScrollingFrame* buildDisplayTab();
    brls::ScrollingFrame* buildAudioTab();
    brls::ScrollingFrame* buildKeyBindTab();
    brls::ScrollingFrame* buildDebugTab();
};