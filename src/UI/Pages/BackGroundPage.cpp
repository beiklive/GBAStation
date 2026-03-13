#include "UI/Pages/BackGroundPage.hpp"
#include "UI/Utils/ProImage.hpp"

BackGroundPage::BackGroundPage()
{
    setBackground(brls::ViewBackground::NONE);
    setFocusable(false);
    setImagePath("");
}

void BackGroundPage::setImagePath(const std::string &path)
{
    m_bgImage = new beiklive::UI::ProImage();
    m_bgImage->setFocusable(false);
    m_bgImage->setScalingType(brls::ImageScalingType::FIT);
    m_bgImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    if (!path.empty()) {
        // GIF is not supported as background; always load as static image.
        m_bgImage->setImageFromFile(path);
        addView(m_bgImage);
    } else {
        m_bgImage->setImageFromFile(BK_APP_DEFAULT_BG);
        addView(m_bgImage);
    }
}
