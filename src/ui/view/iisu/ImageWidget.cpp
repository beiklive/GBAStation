#include "ImageWidget.hpp"

#include <algorithm>

#include "core/Tools.hpp"
#include "GridSystem.hpp"
#include "TextureManager.hpp"

namespace beiklive
{
    ImageWidget::ImageWidget(std::string path) : m_path(std::move(path))
    {
    }

    ImageWidget::~ImageWidget()
    {
        if (m_textures && m_textureId > 0)
            m_textures->releaseTexture(
                brls::Application::getNVGContext(), m_path);
    }

    void ImageWidget::setPath(const std::string& path)
    {
        if (m_path == path)
            return;
        if (m_textures && m_textureId > 0)
            m_textures->releaseTexture(
                brls::Application::getNVGContext(), m_path);
        m_path = path;
        m_textureId = 0;
        m_textureRequested = false;
    }

    std::string ImageWidget::displayName()
    {
        return beiklive::tools::getFileName(m_path);
    }

    void ImageWidget::draw(NVGcontext* vg, const GridRect& rect)
    {
        if (!vg)
            return;

        // 首次绘制时延迟加载纹理
        if (!m_textureRequested) {
            m_textureRequested = true;
            if (m_textures)
                m_textureId = m_textures->loadTexture(vg, m_path);
        }

        
        if (m_textureId <= 0) {
            // 加载失败占位：灰色底
            nvgBeginPath(vg);
            nvgRoundedRect(vg, rect.left, rect.top,
                           rect.width, rect.height, m_radius);
            nvgFillColor(vg, nvgRGBA(80, 80, 80, 130));
            nvgFill(vg);
            return;
        }

        int imageW = 0;
        int imageH = 0;
        nvgImageSize(vg, m_textureId, &imageW, &imageH);
        if (imageW <= 0 || imageH <= 0)
            return;

        // 等比覆盖居中：圆角路径 + 图片填充 = 圆角裁剪
        const float scale = std::max(
            rect.width / static_cast<float>(imageW),
            rect.height / static_cast<float>(imageH));
        const float drawW = static_cast<float>(imageW) * scale;
        const float drawH = static_cast<float>(imageH) * scale;
        const float drawX = rect.left + (rect.width - drawW) * 0.5f;
        const float drawY = rect.top + (rect.height - drawH) * 0.5f;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, rect.left, rect.top,
                       rect.width, rect.height, m_radius);
        NVGpaint paint = nvgImagePattern(
            vg, drawX, drawY, drawW, drawH, 0.f, m_textureId, 1.f);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
    }
} // namespace beiklive
