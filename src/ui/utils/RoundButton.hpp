#pragma once
#include "core/common.h"

#include <borealis.hpp>

namespace beiklive
{
    
class RoundButton : public brls::Box
{
  public:
    RoundButton(const std::string& imagePath, const std::string& text, std::function<void()> onClick);

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;
    void onChildFocusGained(brls::View* directChild, brls::View* focusedView) override;
    void onChildFocusLost(brls::View* directChild, brls::View* focusedView) override;
    // brls::View* getDefaultFocus() override;

    void setImage(const std::string& imagePath);
    void setText(const std::string& text);

  private:
    brls::Box*            m_imageWrapper = nullptr; // 圆形圆角图片容器
    brls::Image*          m_image        = nullptr;
    brls::Label*          m_label        = nullptr;
    std::function<void()> m_onClick;

    // 焦点/悬停缩放动画
    bool  m_focused = false;
    float m_scale   = 1.0f;

    // 点击弹跳动画
    bool  m_clickAnimating = false;
    float m_clickT         = 0.0f;
    float m_clickScale     = 1.0f;
};



} // namespace beiklive
