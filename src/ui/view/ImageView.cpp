#include "ImageView.hpp"
#include "core/Translation.hpp"

#include <algorithm>

namespace beiklive
{
    ImageView::ImageView(const std::string& imagePath)
    {
        this->setHideHighlightBackground(true); 
        this->setHideHighlightBorder(true);   
        this->setHideClickAnimation(true);  
        setFocusable(true);
        setGrow(1.f);
        setWidthPercentage(100.f);
        setHeightPercentage(100.f);
        setAlignItems(brls::AlignItems::CENTER);
        setJustifyContent(brls::JustifyContent::CENTER);
        setClipsToBounds(true);
        setBackgroundColor(nvgRGBA(0, 0, 0, 255));

        m_image = new brls::Image();
        m_image->setScalingType(brls::ImageScalingType::FIT);
        m_image->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_image->setFocusable(false);
        addView(m_image);
        bool mode1 = true;
        registerAction(L("切换渲染模式"), brls::BUTTON_LT, [this, &mode1](brls::View*) -> bool {
            mode1 = !mode1;
            m_image->setInterpolation(mode1 ? brls::ImageInterpolation::LINEAR: brls::ImageInterpolation::NEAREST);
            return true;
        }, false, true);

        registerAction(L("缩小"), brls::BUTTON_LB, [this](brls::View*) -> bool {
            m_zoom = std::max(0.1f, m_zoom / 1.1f);
            _updateImageLayout();
            return true;
        }, false, true);
        registerAction(L("放大"), brls::BUTTON_RB, [this](brls::View*) -> bool {
            m_zoom = std::min(20.0f, m_zoom * 1.1f);
            _updateImageLayout();
            return true;
        }, false, true);
        registerAction(L("复位"), brls::BUTTON_Y, [this](brls::View*) -> bool {
            _resetView();
            return true;
        });

        if (!imagePath.empty())
            setImagePath(imagePath);
    }

    void ImageView::setImagePath(const std::string& imagePath)
    {
        m_imagePath = imagePath;
        if (m_image)
            m_image->setImageFromFile(imagePath);
        _resetView();
    }

    void ImageView::_resetView()
    {
        m_zoom = 1.0f;
        m_offsetX = 0.0f;
        m_offsetY = 0.0f;
        _updateImageLayout();
    }

    void ImageView::_updateImageLayout()
    {
        if (!m_image)
            return;

        const float viewW = std::max(1.0f, getWidth());
        const float viewH = std::max(1.0f, getHeight());
        float imgW = m_image->getOriginalImageWidth();
        float imgH = m_image->getOriginalImageHeight();
        if (imgW <= 0.0f || imgH <= 0.0f) {
            m_image->setWidthPercentage(96.f);
            m_image->setHeightPercentage(96.f);
            m_image->setTranslationX(m_offsetX);
            m_image->setTranslationY(m_offsetY);
            return;
        }

        const float scale = std::min(viewW / imgW, viewH / imgH) * m_zoom;
        const float drawW = std::max(1.0f, imgW * scale);
        const float drawH = std::max(1.0f, imgH * scale);
        m_image->setDimensions(drawW, drawH);

        const float maxX = std::max(0.0f, (drawW - viewW) * 0.5f + 80.0f);
        const float maxY = std::max(0.0f, (drawH - viewH) * 0.5f + 80.0f);
        m_offsetX = std::clamp(m_offsetX, -maxX, maxX);
        m_offsetY = std::clamp(m_offsetY, -maxY, maxY);
        m_image->setTranslationX(m_offsetX);
        m_image->setTranslationY(m_offsetY);
    }

    void ImageView::draw(NVGcontext* vg, float x, float y, float w, float h,
                         brls::Style style, brls::FrameContext* ctx)
    {
        _updateImageLayout();
        brls::Box::draw(vg, x, y, w, h, style, ctx);
    }

    void ImageView::frame(brls::FrameContext* ctx)
    {
        brls::Box::frame(ctx);

        const auto& state = brls::Application::getControllerState();
        const float step = 18.0f * std::max(1.0f, m_zoom);
        bool moved = false;

        if (state.buttons[brls::BUTTON_LEFT]) {
            m_offsetX -= step;
            moved = true;
        }
        if (state.buttons[brls::BUTTON_RIGHT]) {
            m_offsetX += step;
            moved = true;
        }
        if (state.buttons[brls::BUTTON_UP]) {
            m_offsetY -= step;
            moved = true;
        }
        if (state.buttons[brls::BUTTON_DOWN]) {
            m_offsetY += step;
            moved = true;
        }

        if (moved)
            _updateImageLayout();

    }
}
