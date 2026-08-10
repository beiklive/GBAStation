#include "FolderWidget.hpp"

#include <algorithm>

#include "core/Translation.hpp"
#include "GridSystem.hpp"
#include "ui/utils/MaterialIcons.hpp"

namespace
{
    std::string encodeUtf8(char32_t codepoint)
    {
        std::string out;
        if (codepoint <= 0x7F) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        return out;
    }
} // namespace

namespace beiklive
{
    FolderWidget::FolderWidget(std::string folderId)
        : m_folderId(std::move(folderId))
    {
    }

    void FolderWidget::setFolderId(const std::string& id)
    {
        if (m_folderId == id)
            return;
        m_folderId = id;
        m_info.reset();
        m_infoResolved = false;
    }

    void FolderWidget::setFolderDataProvider(FolderDataProvider* provider)
    {
        if (m_provider == provider)
            return;
        m_provider = provider;
        m_info.reset();
        m_infoResolved = false;
    }

    void FolderWidget::onActivate()
    {
        if (onActivated)
            onActivated();
    }

    void FolderWidget::onFocus()
    {
        m_focused = true;
    }

    void FolderWidget::onBlur()
    {
        m_focused = false;
    }

    void FolderWidget::draw(NVGcontext* vg, const GridRect& rect)
    {
        if (!vg)
            return;

        if (!m_infoResolved) {
            m_infoResolved = true;
            if (m_provider)
                m_info = m_provider->getFolder(m_folderId);
        }

                const int fontId = brls::Application::getDefaultFont();
        const int materialFontId =
            brls::Application::getFont(brls::FONT_MATERIAL_ICONS);

        nvgSave(vg);

        // 底
        nvgBeginPath(vg);
        nvgRoundedRect(vg, rect.left, rect.top,
                       rect.width, rect.height, m_radius);
        nvgFillColor(vg, nvgRGBA(64, 104, 156, m_focused ? 165 : 120));
        nvgFill(vg);

        // 文件夹图标
        if (materialFontId >= 0) {
            const std::string glyph = encodeUtf8(beiklive::material::FOLDER);
            const float iconSize =
                std::min(rect.width, rect.height) * 0.42f;
            const float iconY = rect.top + rect.height * 0.44f;
            nvgFontFaceId(vg, materialFontId);
            nvgFontSize(vg, iconSize);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(235, 242, 255,
                m_focused ? 235 : 190));
            nvgText(vg, rect.left + rect.width * 0.5f, iconY,
                    glyph.c_str(), nullptr);
        }

        // 底部信息栏：标题 + 子项数量
        constexpr float barH = 52.f;
        const float barTop = rect.top + rect.height - barH;
        nvgBeginPath(vg);
        nvgRoundedRectVarying(vg, rect.left, barTop,
                              rect.width, barH,
                              0.f, 0.f, m_radius, m_radius);
        nvgFillColor(vg, nvgRGBA(12, 14, 20, 165));
        nvgFill(vg);

        const std::string title = m_info ? m_info->title : L("文件夹");
        nvgFontFaceId(vg, fontId);
        nvgFontSize(vg, 16.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGBA(250, 251, 255, 240));
        nvgText(vg, rect.left + rect.width * 0.5f, barTop + 7.f,
                title.c_str(), nullptr);

        if (m_info) {
            const std::string countText =
                std::to_string(m_info->childCount) + L(" 款游戏");
            nvgFontSize(vg, 12.f);
            nvgFillColor(vg, nvgRGBA(205, 212, 226, 190));
            nvgText(vg, rect.left + rect.width * 0.5f, barTop + 28.f,
                    countText.c_str(), nullptr);
        }

        nvgRestore(vg);
    }
} // namespace beiklive
