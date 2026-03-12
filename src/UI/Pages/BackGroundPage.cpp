#include "UI/Pages/BackGroundPage.hpp"

BackGroundPage::BackGroundPage()
{
    setBackground(brls::ViewBackground::NONE);
    setFocusable(false);
    setImagePath("");
}

void BackGroundPage::setImagePath(const std::string &path)
{
    m_bgImage = new brls::Image();
    m_bgImage->setFocusable(false);
    m_bgImage->setScalingType(brls::ImageScalingType::FIT);
    m_bgImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    if (!path.empty()) {
        // 有封面：直接显示图片
        m_bgImage->setImageFromFile(path);
        addView(m_bgImage);
    } else {
        // 无封面：显示默认图片
        m_bgImage->setImageFromFile(BK_APP_DEFAULT_BG);
        addView(m_bgImage);
    }
}
