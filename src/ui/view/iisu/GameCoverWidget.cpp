#include "GameCoverWidget.hpp"

#include <algorithm>

#include "core/Tools.hpp"
#include "core/Translation.hpp"
#include "GridSystem.hpp"
#include "TextureManager.hpp"

namespace
{
    NVGcolor platformBadgeColor(int platform)
    {
        using beiklive::enums::EmuPlatform;
        switch (static_cast<EmuPlatform>(platform)) {
            case EmuPlatform::EmuGBA:  return nvgRGBA(108, 77, 191, 220);
            case EmuPlatform::EmuGBC:  return nvgRGBA(0, 112, 221, 220);
            case EmuPlatform::EmuGB:   return nvgRGBA(0, 168, 107, 220);
            case EmuPlatform::EmuNES:  return nvgRGBA(218, 41, 28, 220);
            case EmuPlatform::EmuSNES: return nvgRGBA(160, 100, 180, 220);
            case EmuPlatform::EmuNDS:  return nvgRGBA(54, 150, 190, 220);
            case EmuPlatform::Emu3DS:  return nvgRGBA(230, 79, 91, 220);
            case EmuPlatform::EmuGenesis: return nvgRGBA(23, 55, 139, 220);
            case EmuPlatform::EmuArcade: return nvgRGBA(236, 134, 44, 220);
            case EmuPlatform::EmuDreamcast: return nvgRGBA(0, 142, 180, 220);
            case EmuPlatform::EmuPSP: return nvgRGBA(67, 118, 226, 220);
            default: return nvgRGBA(100, 100, 100, 200);
        }
    }

    // 单行截断：按 UTF-8 字符从尾部裁剪并追加省略号
    std::string truncateWithEllipsis(const std::string& text,
                                     NVGcontext* vg, int fontId,
                                     float size, float maxWidth)
    {
        if (maxWidth <= 0.f)
            return "";
        nvgFontFaceId(vg, fontId);
        nvgFontSize(vg, size);
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, text.c_str(), nullptr, bounds);
        if (bounds[2] - bounds[0] <= maxWidth)
            return text;

        const std::string ellipsis = "...";
        std::string cur = text;
        while (!cur.empty()) {
            size_t start = cur.size() - 1;
            while (start > 0 &&
                   (static_cast<unsigned char>(cur[start]) & 0xC0) == 0x80)
                --start;
            cur.erase(start);
            const std::string candidate = cur + ellipsis;
            nvgTextBounds(vg, 0.f, 0.f, candidate.c_str(), nullptr, bounds);
            if (bounds[2] - bounds[0] <= maxWidth)
                return candidate;
        }
        return ellipsis;
    }
} // namespace

namespace beiklive
{
    GameCoverWidget::GameCoverWidget(std::string gameId)
        : m_gameId(std::move(gameId))
    {
    }

    GameCoverWidget::~GameCoverWidget()
    {
        if (m_textures && m_coverTexture > 0)
            m_textures->releaseTexture(
                brls::Application::getNVGContext(), m_coverPath);
    }

    void GameCoverWidget::setGameId(const std::string& id)
    {
        if (m_gameId == id)
            return;
        m_gameId = id;
        m_info.reset();
        m_infoResolved = false;
        m_textureRequested = false;
        m_coverPath.clear();
        m_coverTexture = 0;
    }

    void GameCoverWidget::setGameDataProvider(GameDataProvider* provider)
    {
        if (m_provider == provider)
            return;
        m_provider = provider;
        m_info.reset();
        m_infoResolved = false;
        m_textureRequested = false;
        m_coverPath.clear();
        m_coverTexture = 0;
    }

    std::string GameCoverWidget::displayName()
    {
        if (!m_infoResolved) {
            m_infoResolved = true;
            if (m_provider)
                m_info = m_provider->getGame(m_gameId);
        }
        return m_info ? m_info->title : m_gameId;
    }

    void GameCoverWidget::onFocus()
    {
        m_focused = true;
    }

    void GameCoverWidget::onBlur()
    {
        m_focused = false;
    }

    bool GameCoverWidget::loadCover(NVGcontext* vg)
    {
        if (m_textureRequested)
            return m_coverTexture > 0;
        m_textureRequested = true;
        if (!m_info || !m_textures)
            return false;
        m_coverPath = m_info->coverPath;
        m_coverTexture = m_textures->loadTexture(vg, m_coverPath);
        return m_coverTexture > 0;
    }

    void GameCoverWidget::draw(NVGcontext* vg, const GridRect& rect)
    {
        if (!vg)
            return;

        // 首次绘制时解析游戏数据
        if (!m_infoResolved) {
            m_infoResolved = true;
            if (m_provider)
                m_info = m_provider->getGame(m_gameId);
        }
        const int fontId = brls::Application::getDefaultFont();

        nvgSave(vg);

        const bool hasCover =
            m_info && !m_info->coverPath.empty() &&
            loadCover(vg) && m_coverTexture > 0;

        if (hasCover) {
            int imageW = 0;
            int imageH = 0;
            nvgImageSize(vg, m_coverTexture, &imageW, &imageH);
            if (imageW > 0 && imageH > 0) {
                const float coverScale = std::max(
                    rect.width / static_cast<float>(imageW),
                    rect.height / static_cast<float>(imageH));
                const float drawW = static_cast<float>(imageW) * coverScale;
                const float drawH = static_cast<float>(imageH) * coverScale;
                const float drawX = rect.left + (rect.width - drawW) * 0.5f;
                const float drawY = rect.top + (rect.height - drawH) * 0.5f;

                nvgBeginPath(vg);
                nvgRoundedRect(vg, rect.left, rect.top,
                               rect.width, rect.height, m_radius);
                NVGpaint paint = nvgImagePattern(
                    vg, drawX, drawY, drawW, drawH, 0.f, m_coverTexture,
                    m_focused ? 1.f : 0.92f);
                nvgFillPaint(vg, paint);
                nvgFill(vg);
            }
        } else {
            // 无封面占位：灰底 + 标题
            nvgBeginPath(vg);
            nvgRoundedRect(vg, rect.left, rect.top,
                           rect.width, rect.height, m_radius);
            nvgFillColor(vg, nvgRGBA(80, 80, 90, 150));
            nvgFill(vg);
            const std::string title =
                m_info ? m_info->title : L("未知游戏");
            nvgFontFaceId(vg, fontId);
            nvgFontSize(vg, 18.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(230, 235, 245, 190));
            nvgText(vg, rect.left + rect.width * 0.5f,
                    rect.top + rect.height * 0.5f, title.c_str(), nullptr);
        }

        // 底部信息栏：标题 + 平台徽章 + 游玩时间
        if (m_info) {
            constexpr float barH = 58.f;
            const float barTop = rect.top + rect.height - barH;

            nvgBeginPath(vg);
            nvgRoundedRectVarying(vg, rect.left, barTop,
                                  rect.width, barH,
                                  0.f, 0.f, m_radius, m_radius);
            nvgFillColor(vg, nvgRGBA(12, 14, 20, 165));
            nvgFill(vg);

            nvgFontFaceId(vg, fontId);
            nvgFontSize(vg, 15.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            nvgFillColor(vg, nvgRGBA(250, 251, 255, 240));
            const std::string title = truncateWithEllipsis(
                m_info->title, vg, fontId, 15.f, rect.width - 20.f);
            nvgText(vg, rect.left + 10.f, barTop + 8.f,
                    title.c_str(), nullptr);

            const std::string badgeText =
                beiklive::tools::platformBadgeName(m_info->platform);
            const std::string playTimeText = m_info->playTime > 0
                ? beiklive::tools::formatPlayTime(m_info->playTime)
                : L("未游玩");

            nvgFontSize(vg, 12.f);
            float badgeBounds[4]{};
            nvgTextBounds(vg, 0.f, 0.f, badgeText.c_str(),
                          nullptr, badgeBounds);
            const float badgeW = std::max(
                30.f, (badgeBounds[2] - badgeBounds[0]) + 14.f);
            constexpr float badgeH = 17.f;
            const float badgeY = barTop + 31.f;

            nvgBeginPath(vg);
            nvgRoundedRect(vg, rect.left + 10.f, badgeY,
                           badgeW, badgeH, badgeH * 0.5f);
            nvgFillColor(vg, platformBadgeColor(m_info->platform));
            nvgFill(vg);

            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 235));
            nvgText(vg, rect.left + 10.f + badgeW * 0.5f,
                    badgeY + badgeH * 0.5f, badgeText.c_str(), nullptr);

            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(205, 212, 226, 200));
            nvgText(vg, rect.left + 10.f + badgeW + 8.f,
                    badgeY + badgeH * 0.5f, playTimeText.c_str(), nullptr);
        }

        nvgRestore(vg);
    }
} // namespace beiklive
