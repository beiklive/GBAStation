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
        m_isGif = false;
        m_gifFrame = 0;
        m_gifTimeMs = 0.f;
    }

    std::string ImageWidget::displayName()
    {
        return beiklive::tools::getFileName(m_path);
    }

    void ImageWidget::update(float delta)
    {
        if (!m_isGif || !m_textures)
            return;
        const size_t count = m_textures->gifFrameCount(m_path);
        if (count < 2)
            return;

        m_gifTimeMs += delta * 1000.f;
        const bool loop = m_textures->gifLooping(m_path);
        const float speed = m_speedMul > 0.01f ? m_speedMul : 1.f;
        size_t guard = 0;
        while (true) {
            // 有效延迟 = 文件延迟 / 速度倍率
            const uint32_t baseDelay =
                m_textures->gifDelayMs(m_path, m_gifFrame);
            const uint32_t effDelay = std::max<uint32_t>(
                1u, static_cast<uint32_t>(baseDelay / speed));
            if (m_gifTimeMs < effDelay)
                break;
            m_gifTimeMs -= static_cast<float>(effDelay);
            if (m_gifFrame + 1 < count) {
                ++m_gifFrame;
            } else if (loop) {
                m_gifFrame = 0;
            } else {
                // 不循环：停在末帧
                m_gifTimeMs = 0.f;
                break;
            }
            if (++guard > 64)
                break;
        }
    }

    void ImageWidget::draw(NVGcontext* vg, const GridRect& rect)
    {
        if (!vg)
            return;

        // 首次绘制时延迟加载纹理
        if (!m_textureRequested) {
            m_textureRequested = true;
            if (m_textures) {
                m_textureId = m_textures->loadTexture(vg, m_path);
                if (m_textures->isGifTexture(m_path)) {
                    m_isGif = true;
                    m_gifFrame = 0;
                }
            }
        }

        // GIF：直接使用当前帧的独立纹理（避免运行时更新纹理导致的闪烁）
        const int drawTexture = m_isGif
            ? m_textures->gifFrameTexture(m_path, m_gifFrame)
            : m_textureId;
        if (drawTexture <= 0) {
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
        nvgImageSize(vg, drawTexture, &imageW, &imageH);
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
            vg, drawX, drawY, drawW, drawH, 0.f, drawTexture, 1.f);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
    }
} // namespace beiklive
