#pragma once

#include <borealis.hpp>
#include <functional>
#include "common.hpp"

/// 关于页面：展示项目信息、libretro 说明及作者信息
class AboutPage : public beiklive::UI::BBox
{
  public:
    AboutPage();

  private:
    brls::Image*  m_topImage    = nullptr;  ///< 顶部 mgba 图片
    brls::Image*  m_authorImage = nullptr;  ///< 底部作者圆形头像
    brls::Label*  m_authorName  = nullptr;  ///< 底部作者名称
    brls::Label*  m_githubLabel = nullptr;  ///< Github 链接
    brls::Label*  m_biliLabel   = nullptr;  ///< BiliBili 链接

    void buildUI();
};
