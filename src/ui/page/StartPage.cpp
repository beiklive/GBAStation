#include "StartPage.hpp"
#include "core/Translation.hpp"
#include "SteamGridDbPage.hpp"
#include "CoverEditorPage.hpp"
#include "core/SteamGridDb.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "core/Tools.hpp"
#include "core/rom/PspMeta.hpp"
#include "core/ThreadPool.hpp"
#include "core/ThreeDsTitlePaths.hpp"
#include "core/ExternalCoreSession.hpp"
#include "core/forwarder/ForwarderInstaller.hpp"
#include "ui/utils/MaterialIcons.hpp"
#include "ui/utils/NdsEnvironment.hpp"
#include "ui/utils/CheatMatcher.hpp"
#include <borealis/core/logger.hpp>
#include <borealis/views/dropdown.hpp>
#include <borealis/views/hint.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <vector>

#ifdef __SWITCH__
#include "platform/switch/NroLauncher.hpp"
#endif

namespace
{

bool deleteGameFileIfExists(const std::string& path)
{
    if (path.empty()) {
        brls::Logger::info("[Game Delete] entry path empty, nothing to remove");
        return true;
    }

    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if (ec) {
        brls::Logger::warning(
            "[Game Delete] entry exists failed: path={} code={} error={}",
            path, ec.value(), ec.message());
        return false;
    }
    if (!exists) {
        brls::Logger::info("[Game Delete] entry already missing: {}", path);
        return true;
    }

    ec.clear();
    const bool removed = std::filesystem::remove(path, ec);
    if (ec || !removed) {
        brls::Logger::warning(
            "[Game Delete] entry remove failed: path={} removed={} code={} error={}",
            path, removed, ec.value(), ec ? ec.message() : "remove returned false");
        return false;
    }
    brls::Logger::info("[Game Delete] entry removed: {}", path);
    return true;
}

bool deleteGameFilesForEntry(const beiklive::GameEntry& entry)
{
    brls::Logger::info(
        "[Game Delete] file removal begin: platform={} path={} stored_title_id={}",
        entry.platform, entry.path, entry.threeDsTitleId);
    if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS))
    {
        const std::string titleId = beiklive::three_ds::resolveTitleId(
            entry.threeDsTitleId, entry.path);
        if (titleId.empty())
            brls::Logger::warning(
                "[Game Delete] unable to resolve 3DS title id: path={}", entry.path);
        const bool titleFilesRemoved = titleId.empty() ||
            beiklive::three_ds::deleteInstalledContentAndShaderCache(titleId);
        const bool entryFileRemoved = deleteGameFileIfExists(entry.path);
        brls::Logger::info(
            "[Game Delete] 3DS file removal result: path={} title_id={} "
            "title_files_removed={} entry_file_removed={} success={}",
            entry.path, titleId, titleFilesRemoved, entryFileRemoved,
            titleFilesRemoved && entryFileRemoved);
        return entryFileRemoved && titleFilesRemoved;
    }
    const bool removed = deleteGameFileIfExists(entry.path);
    brls::Logger::info(
        "[Game Delete] file removal result: path={} success={}", entry.path, removed);
    return removed;
}

void preserveThreeDsMenuSettings(json& root, const std::filesystem::path& file)
{
    std::ifstream in(file);
    if (!in.is_open())
        return;

    json existing = json::parse(in, nullptr, false);
    if (!existing.is_object())
        return;

    constexpr const char* keys[] = {
        "fastforward.multiplier",
        "ndsScreenLayout",
        "ndsScreenOrientation",
        "ndsInternalResolution",
        "ndsIntegerScale",
        "ndsScreenGap",
        "ndsTopScale",
        "ndsTopOffsetX",
        "ndsTopOffsetY",
        "ndsBottomScale",
        "ndsBottomOffsetX",
        "ndsBottomOffsetY",
        "ndsBottomOpacity",
        "overlayEnabled",
        "overlayPath",
    };
    for (const char* key : keys)
    {
        if (!root.contains(key) && existing.contains(key))
            root[key] = existing[key];
    }
}

} // namespace

namespace beiklive
{
// 平台 → 文件类型（机种选择后构造启动条目）。
beiklive::enums::FileType platformToFileType(int platform)
{
    switch (static_cast<beiklive::enums::EmuPlatform>(platform))
    {
        case beiklive::enums::EmuPlatform::EmuGBA:       return beiklive::enums::FileType::GBA_ROM;
        case beiklive::enums::EmuPlatform::EmuGBC:       return beiklive::enums::FileType::GBC_ROM;
        case beiklive::enums::EmuPlatform::EmuGB:        return beiklive::enums::FileType::GB_ROM;
        case beiklive::enums::EmuPlatform::EmuNES:       return beiklive::enums::FileType::NES_ROM;
        case beiklive::enums::EmuPlatform::EmuSNES:      return beiklive::enums::FileType::SNES_ROM;
        case beiklive::enums::EmuPlatform::EmuNDS:       return beiklive::enums::FileType::NDS_ROM;
        case beiklive::enums::EmuPlatform::Emu3DS:       return beiklive::enums::FileType::THREEDS_ROM;
        case beiklive::enums::EmuPlatform::EmuGenesis:   return beiklive::enums::FileType::GENESIS_ROM;
        case beiklive::enums::EmuPlatform::EmuArcade:    return beiklive::enums::FileType::ARCADE_ROM;
        case beiklive::enums::EmuPlatform::EmuDreamcast: return beiklive::enums::FileType::DREAMCAST_ROM;
        case beiklive::enums::EmuPlatform::EmuPSP:       return beiklive::enums::FileType::PSP_ROM;
        default: return beiklive::enums::FileType::NORMAL_FILE;
    }
}

    // 机种选择弹窗：歧义后缀（iso/bin/cue/zip/7z 等）从文件列表启动时，
    // 列出候选机种供选择（深色卡片 + 图标行 + 焦点框，与文件列表风格一致）。
#ifdef ABSOLUTE
#undef ABSOLUTE
#endif
    class PlatformPickerOverlay final : public brls::View
    {
    public:
        std::function<void(int platform)> onPicked;

        PlatformPickerOverlay()
        {
            setFocusable(true);
            setVisibility(brls::Visibility::GONE);
            setPositionType(brls::PositionType::ABSOLUTE);
            setPositionTop(0.f);
            setPositionLeft(0.f);
            setWidth(1280.f);
            setHeight(720.f);
            HIDE_BRLS_HIGHLIGHT(this);
            setCustomNavigationRoute(brls::FocusDirection::UP, this);
            setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
            setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
            setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

            auto moveUp = [this](brls::View*) -> bool {
                if (!m_open || m_closing)
                    return false;
                return true;
            };
            auto moveDown = [this](brls::View*) -> bool {
                if (!m_open || m_closing)
                    return false;
                return true;
            };
            auto confirm = [this](brls::View*) -> bool {
                if (!m_open || m_closing)
                    return false;
                _finish(m_selected);
                return true;
            };
            auto cancel = [this](brls::View*) -> bool {
                if (!m_open || m_closing)
                    return false;
                _finish(-1);
                return true;
            };
            registerAction("", brls::BUTTON_UP, moveUp, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_DOWN, moveDown, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_UP, moveUp, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_DOWN, moveDown, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_LEFT, moveUp, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_RIGHT, moveDown, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_LEFT, moveUp, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_RIGHT, moveDown, true, true, brls::SOUND_NONE);
            registerAction(L("选择"), brls::BUTTON_A, confirm, false, false, brls::SOUND_NONE);
            registerAction(L("取消"), brls::BUTTON_B, cancel, false, false, brls::SOUND_NONE);
            registerAction(L("取消"), brls::BUTTON_START, cancel, false, false, brls::SOUND_NONE);
        }

        void open(std::vector<int> candidates, int defaultIndex,
                  const std::string& fileName)
        {
            m_candidates = std::move(candidates);
            m_selected = std::max(0, std::min(
                defaultIndex, static_cast<int>(m_candidates.size()) - 1));
            m_fileName = fileName;
            m_open = true;
            m_closing = false;
            m_progress = 0.f;
            m_lastFrame = std::chrono::steady_clock::now();
            m_prevUp = m_prevDown = m_prevLeft = m_prevRight = false;
            m_prevA = m_prevB = m_prevStart = false;
            setVisibility(brls::Visibility::VISIBLE);
            brls::Application::giveFocus(this);
        }

        void close()
        {
            m_open = false;
            m_closing = false;
            m_progress = 0.f;
            setVisibility(brls::Visibility::GONE);
        }

        bool isOpen() const { return m_open; }

        void frame(brls::FrameContext* ctx) override
        {
            brls::View::frame(ctx);
            const auto now = std::chrono::steady_clock::now();
            const float dt = std::chrono::duration<float>(now - m_lastFrame).count();
            m_lastFrame = now;
            if (m_open && !m_closing && m_progress < 1.f)
                m_progress = std::min(1.f, m_progress + dt * 6.f);
            if (m_closing)
            {
                m_progress = std::max(0.f, m_progress - dt * 8.f);
                if (m_progress <= 0.f)
                    close();
            }
            else if (m_open)
            {
                _pollInput();
            }
            invalidate();
        }

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style, brls::FrameContext*) override
        {
            if (!m_open || !vg)
                return;
            if (m_defaultFont < 0)
                m_defaultFont = brls::Application::getDefaultFont();
            if (m_materialFont < 0)
                m_materialFont = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
            if (m_switchFont < 0)
                m_switchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);

            const float alpha = std::max(0.f, std::min(1.f, m_progress));
            const float eased = 1.f - std::pow(1.f - m_progress, 3.f);

            nvgSave(vg);
            nvgGlobalAlpha(vg, alpha);

            // 遮罩（底部留出 hint 栏）
            nvgBeginPath(vg);
            nvgRect(vg, x, y, w, h - 56.f);
            nvgFillColor(vg, nvgRGBA(0, 0, 0,
                static_cast<unsigned char>(205.f * alpha)));
            nvgFill(vg);

            const float rowH = 56.f;
            const float panelW = 640.f;
            const float panelH = 96.f + static_cast<float>(m_candidates.size()) * rowH + 34.f;
            const float panelX = x + (w - panelW) * 0.5f;
            const float panelY = y + (h - panelH) * 0.5f + (1.f - eased) * 42.f;

            // 面板阴影
            const NVGpaint shadow = nvgBoxGradient(
                vg, panelX + 5.f, panelY + 7.f, panelW, panelH, 18.f, 8.f,
                nvgRGBA(0, 0, 0, 130), nvgRGBA(0, 0, 0, 0));
            nvgBeginPath(vg);
            nvgRect(vg, panelX - 8.f, panelY - 8.f, panelW + 20.f, panelH + 20.f);
            nvgRoundedRect(vg, panelX, panelY, panelW, panelH, 18.f);
            nvgPathWinding(vg, NVG_HOLE);
            nvgFillPaint(vg, shadow);
            nvgFill(vg);

            // 面板背景
            nvgBeginPath(vg);
            nvgRoundedRect(vg, panelX, panelY, panelW, panelH, 18.f);
            nvgFillColor(vg, nvgRGBA(30, 32, 38, 242));
            nvgFill(vg);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, panelX + 1.f, panelY + 1.f,
                           panelW - 2.f, panelH - 2.f, 17.f);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 34));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);

            // 标题
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 17.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(215, 222, 233, 235));
            nvgText(vg, panelX + 28.f, panelY + 34.f, L("选择机种").c_str(), nullptr);
            nvgFontSize(vg, 15.f);
            nvgFillColor(vg, nvgRGBA(170, 178, 190, 190));
            std::string fileTitle = m_fileName;
            if (fileTitle.size() > 34)
                fileTitle = fileTitle.substr(0, 34) + "...";
            nvgText(vg, panelX + panelW - 28.f, panelY + 34.f, fileTitle.c_str(), nullptr);

            // 分隔线
            nvgBeginPath(vg);
            nvgMoveTo(vg, panelX + 26.f, panelY + 60.f);
            nvgLineTo(vg, panelX + panelW - 26.f, panelY + 60.f);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 26));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);

            // 机种行
            const std::string pathPrefix = "img/ui/" + std::string(
                brls::Application::getPlatform()->getThemeVariant() ==
                        brls::ThemeVariant::DARK
                    ? "light/"
                    : "dark/");
            for (size_t i = 0; i < m_candidates.size(); ++i)
            {
                const float rowX = panelX + 18.f;
                const float rowY = panelY + 72.f + static_cast<float>(i) * rowH;
                const float rowW = panelW - 36.f;
                const bool focused = static_cast<int>(i) == m_selected;

                nvgBeginPath(vg);
                nvgRoundedRect(vg, rowX, rowY, rowW, rowH - 8.f, 8.f);
                nvgFillColor(vg, focused
                    ? nvgRGBA(79, 193, 255, 32)
                    : nvgRGBA(255, 255, 255, 5));
                nvgFill(vg);
                if (focused)
                {
                    nvgBeginPath(vg);
                    nvgRoundedRect(vg, rowX + 0.5f, rowY + 0.5f,
                                   rowW - 1.f, rowH - 9.f, 7.5f);
                    nvgStrokeColor(vg, nvgRGBA(79, 193, 255, 150));
                    nvgStrokeWidth(vg, 1.5f);
                    nvgStroke(vg);
                }

                // 平台图标
                const beiklive::enums::FileType fileType = platformToFileType(m_candidates[i]);
                const std::string iconPath =
                    beiklive::tools::getIconPathWithPrefix(fileType, pathPrefix);
                int imageHandle = _iconHandle(fileType, iconPath, vg);
                if (imageHandle > 0)
                {
                    int iw = 0, ih = 0;
                    nvgImageSize(vg, imageHandle, &iw, &ih);
                    if (iw > 0 && ih > 0)
                    {
                        float drawW = 34.f;
                        float drawH = drawW * static_cast<float>(ih) / iw;
                        const NVGpaint ip = nvgImagePattern(
                            vg, rowX + 14.f, rowY + (rowH - 8.f) * 0.5f - drawH * 0.5f,
                            drawW, drawH, 0.f, imageHandle, alpha);
                        nvgBeginPath(vg);
                        nvgRoundedRect(vg, rowX + 14.f,
                                       rowY + (rowH - 8.f) * 0.5f - drawH * 0.5f,
                                       drawW, drawH, 6.f);
                        nvgFillPaint(vg, ip);
                        nvgFill(vg);
                    }
                }

                // 平台名
                nvgFontFaceId(vg, m_defaultFont);
                nvgFontSize(vg, 20.f);
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, focused
                    ? nvgRGBA(255, 255, 255, 255)
                    : nvgRGBA(226, 232, 240, 220));
                const std::string name =
                    beiklive::tools::platformName(m_candidates[i]);
                nvgText(vg, rowX + 62.f, rowY + (rowH - 8.f) * 0.5f,
                        name.c_str(), nullptr);

                // 右侧提示
                nvgFontSize(vg, 14.f);
                nvgFillColor(vg, focused
                    ? nvgRGBA(79, 193, 255, 235)
                    : nvgRGBA(150, 158, 172, 150));
                nvgText(vg, rowX + rowW - 14.f, rowY + (rowH - 8.f) * 0.5f,
                        focused ? L("按 A 启动").c_str() : L("选择").c_str(), nullptr);
            }

            nvgRestore(vg);
        }

    private:
        int _iconHandle(beiklive::enums::FileType fileType, const std::string& path, NVGcontext* vg)
        {
            const auto it = m_iconCache.find(path);
            if (it != m_iconCache.end())
                return it->second;
            int handle = nvgCreateImage(vg, path.c_str(), 0);
            if (handle == 0)
                handle = -1;
            m_iconCache[path] = handle;
            return handle;
        }

        void _pollInput()
        {
            auto& st = brls::Application::getControllerState();
            const bool up = st.buttons[brls::BUTTON_UP] ||
                            st.buttons[brls::BUTTON_NAV_UP];
            const bool down = st.buttons[brls::BUTTON_DOWN] ||
                              st.buttons[brls::BUTTON_NAV_DOWN];
            const bool left = st.buttons[brls::BUTTON_LEFT] ||
                              st.buttons[brls::BUTTON_NAV_LEFT];
            const bool right = st.buttons[brls::BUTTON_RIGHT] ||
                               st.buttons[brls::BUTTON_NAV_RIGHT];
            const bool a = st.buttons[brls::BUTTON_A];
            const bool b = st.buttons[brls::BUTTON_B];
            const bool start = st.buttons[brls::BUTTON_START];

            if (up && !m_prevUp)
                _move(-1);
            if (down && !m_prevDown)
                _move(1);
            if (left && !m_prevLeft)
                _move(-1);
            if (right && !m_prevRight)
                _move(1);
            if (a && !m_prevA)
                _finish(m_selected);
            if ((b && !m_prevB) || (start && !m_prevStart))
                _finish(-1);

            m_prevUp = up;
            m_prevDown = down;
            m_prevLeft = left;
            m_prevRight = right;
            m_prevA = a;
            m_prevB = b;
            m_prevStart = start;
        }

        void _move(int dir)
        {
            if (m_candidates.empty())
                return;
            m_selected = (m_selected + dir + static_cast<int>(m_candidates.size())) %
                         static_cast<int>(m_candidates.size());
            brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
            invalidate();
        }

        void _finish(int index)
        {
            if (m_closing)
                return;
            m_closing = true;
            brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
            const int platform =
                (index >= 0 && index < static_cast<int>(m_candidates.size()))
                    ? m_candidates[static_cast<size_t>(index)] : -1;
            brls::sync([this, platform]() {
                if (onPicked)
                    onPicked(platform);
            });
        }

        std::vector<int> m_candidates;
        int m_selected = 0;
        std::string m_fileName;
        bool m_open = false;
        bool m_closing = false;
        float m_progress = 0.f;
        std::chrono::steady_clock::time_point m_lastFrame;
        int m_defaultFont = -1;
        int m_materialFont = -1;
        int m_switchFont = -1;
        bool m_prevUp = false;
        bool m_prevDown = false;
        bool m_prevLeft = false;
        bool m_prevRight = false;
        bool m_prevA = false;
        bool m_prevB = false;
        bool m_prevStart = false;
        std::unordered_map<std::string, int> m_iconCache;
    };


    class HomeShortcutSettingsOverlay final : public brls::View
    {
    public:
        HomeShortcutSettingsOverlay()
        {
            setFocusable(true);
            setVisibility(brls::Visibility::GONE);
#ifdef ABSOLUTE
#undef ABSOLUTE
#endif
            setPositionType(brls::PositionType::ABSOLUTE);
            setPositionTop(0.f);
            setPositionLeft(0.f);
            setWidth(1280.f);
            setHeight(720.f);
            HIDE_BRLS_HIGHLIGHT(this);
            setCustomNavigationRoute(brls::FocusDirection::UP, this);
            setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
            setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
            setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

            auto toggle = [this](brls::View*) -> bool {
                if (!m_open || m_closing)
                    return m_open;
                // iisu 模式：按钮列表（布局调整 / 卡片设置）
                if (m_showLayoutButtons) {
                    brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
                    if (m_selectedRow == 0) {
                        if (onLayoutEditRequested)
                            onLayoutEditRequested();
                    } else {
                        if (onCardSettingsRequested)
                            onCardSettingsRequested();
                    }
                    close();
                    return true;
                }
                // switch 模式：PICO-8 入口显示切换
                m_pico8Visible = !m_pico8Visible;
                m_press = 1.f;
                brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
                if (onPico8VisibleChanged)
                    onPico8VisibleChanged(m_pico8Visible);
                invalidate();
                return true;
            };
            auto consume = [this](brls::View*) -> bool { return m_open; };
            auto closeAction = [this](brls::View*) -> bool {
                if (!m_open)
                    return false;
                close();
                return true;
            };
            auto rowMove = [this](int dir) -> bool {
                if (!m_open || m_closing || !m_showLayoutButtons)
                    return m_open;
                m_selectedRow = (m_selectedRow + dir + 2) % 2;
                brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
                invalidate();
                return true;
            };

            registerAction(L("选择"), brls::BUTTON_A, toggle, false, false, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_LEFT, consume, true, false, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_RIGHT, consume, true, false, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_LEFT, consume, true, false, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_RIGHT, consume, true, false, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_UP, [rowMove](brls::View*) { return rowMove(-1); }, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_DOWN, [rowMove](brls::View*) { return rowMove(1); }, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_UP, consume, true, false, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_DOWN, consume, true, false, brls::SOUND_NONE);
            registerAction(L("返回"), brls::BUTTON_B, closeAction, false, false, brls::SOUND_NONE);
            registerAction(L("关闭"), brls::BUTTON_START, closeAction, false, false, brls::SOUND_NONE);
        }

        void open(bool pico8Visible)
        {
            m_pico8Visible = pico8Visible;
            m_open = true;
            m_closing = false;
            m_progress = 0.f;
            m_press = 0.f;
            m_lastFrame = std::chrono::steady_clock::now();
            setVisibility(brls::Visibility::VISIBLE);
            brls::Application::giveFocus(this);
        }

        void close()
        {
            if (!m_open || m_closing)
                return;
            m_closing = true;
            brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
        }

        bool isOpen() const { return m_open; }
        brls::View* getDefaultFocus() override { return this; }
        brls::View* getNextFocus(brls::FocusDirection, brls::View*) override { return this; }

        void frame(brls::FrameContext* ctx) override
        {
            brls::View::frame(ctx);
            if (!m_open)
                return;
            const auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - m_lastFrame).count();
            m_lastFrame = now;
            if (dt <= 0.f || dt > 0.25f)
                dt = 0.016f;
            m_press = std::max(0.f, m_press - dt * 7.f);
            m_progress += (m_closing ? -1.f : 1.f) * dt *
                (m_closing ? 5.4f : 4.6f);
            m_progress = _clamp01(m_progress);
            if (m_closing && m_progress <= 0.f)
                _finishClose();
            invalidate();
        }

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style, brls::FrameContext*) override
        {
            if (!m_open || !vg)
                return;
            if (m_defaultFont < 0)
                m_defaultFont = brls::Application::getDefaultFont();
            if (m_materialFont < 0)
                m_materialFont = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
            if (m_switchFont < 0)
                m_switchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);

            const float alpha = _smooth(m_progress);
            const float eased = _back(m_progress);
            nvgBeginPath(vg);
            nvgRect(vg, x, y, w, h);
            nvgFillColor(vg, nvgRGBA(0, 0, 0,
                static_cast<unsigned char>(205.f * alpha)));
            nvgFill(vg);

            const float panelW = 720.f;
            const float panelH = 326.f;
            const float panelX = x + (w - panelW) * 0.5f;
            const float panelY = y + (h - panelH) * 0.5f + (1.f - eased) * 42.f;
            const NVGpaint shadow = nvgBoxGradient(
                vg, panelX + 5.f, panelY + 7.f, panelW, panelH, 18.f, 8.f,
                nvgRGBA(0, 0, 0, 115), nvgRGBA(0, 0, 0, 0));
            nvgBeginPath(vg);
            nvgRect(vg, panelX - 4.f, panelY - 4.f, panelW + 18.f, panelH + 20.f);
            nvgRoundedRect(vg, panelX, panelY, panelW, panelH, 18.f);
            nvgPathWinding(vg, NVG_HOLE);
            nvgFillPaint(vg, shadow);
            nvgFill(vg);

            nvgBeginPath(vg);
            nvgRoundedRect(vg, panelX, panelY, panelW, panelH, 18.f);
            nvgFillColor(vg, nvgRGBA(25, 29, 39, 248));
            nvgFill(vg);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 74));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);

            const std::string settingsIcon = _utf8(beiklive::material::SETTINGS);
            nvgFontFaceId(vg, m_materialFont);
            nvgFontSize(vg, 37.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(91, 193, 255, 245));
            nvgText(vg, panelX + 34.f, panelY + 50.f, settingsIcon.c_str(), nullptr);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 28.f);
            nvgFillColor(vg, nvgRGBA(246, 248, 252, 248));
            nvgText(vg, panelX + 84.f, panelY + 50.f, L("首页功能").c_str(), nullptr);
            nvgFontSize(vg, 15.f);
            nvgFillColor(vg, nvgRGBA(192, 201, 215, 190));
            const std::string subtitle =
                m_showLayoutButtons ? L("布局与卡片设置")
                                    : L("首页快捷按键显示设置");
            nvgText(vg, panelX + 35.f, panelY + 83.f,
                    subtitle.c_str(), nullptr);

            if (m_showLayoutButtons) {
                // iisu 模式：按钮列表（布局调整 / 卡片设置）
                const float rowX = panelX + 28.f;
                const float rowW = panelW - 56.f;
                constexpr float rowH = 64.f;
                constexpr float rowGap = 12.f;
                const float rowsY = panelY + 108.f;
                const std::string icons[2] = {
                    _utf8(beiklive::material::EDIT),
                    _utf8(beiklive::material::SETTINGS),
                };
                const char* labels[2] = {
                    L("布局调整").c_str(),
                    L("卡片设置").c_str(),
                };
                for (int i = 0; i < 2; ++i) {
                    const bool selected = m_selectedRow == i;
                    const float ry = rowsY + static_cast<float>(i) * (rowH + rowGap);
                    const float rowScale = 1.f - (selected ? m_press : 0.f) * 0.018f;
                    nvgSave(vg);
                    nvgTranslate(vg, rowX + rowW * 0.5f, ry + rowH * 0.5f);
                    nvgScale(vg, rowScale, rowScale);
                    nvgTranslate(vg, -(rowX + rowW * 0.5f), -(ry + rowH * 0.5f));
                    nvgBeginPath(vg);
                    nvgRoundedRect(vg, rowX, ry, rowW, rowH, 12.f);
                    nvgFillColor(vg, selected
                        ? nvgRGBA(91, 193, 255, 220)
                        : nvgRGBA(255, 255, 255, 14));
                    nvgFill(vg);
                    nvgStrokeColor(vg, selected
                        ? nvgRGBA(168, 224, 255, 230)
                        : nvgRGBA(255, 255, 255, 45));
                    nvgStrokeWidth(vg, 1.f);
                    nvgStroke(vg);
                    nvgFontFaceId(vg, m_materialFont);
                    nvgFontSize(vg, 30.f);
                    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                    nvgFillColor(vg, selected
                        ? nvgRGBA(18, 24, 34, 245)
                        : nvgRGBA(96, 195, 255, 235));
                    nvgText(vg, rowX + 22.f, ry + rowH * 0.5f, icons[i].c_str(), nullptr);
                    nvgFontFaceId(vg, m_defaultFont);
                    nvgFontSize(vg, 22.f);
                    nvgFillColor(vg, selected
                        ? nvgRGBA(18, 24, 34, 245)
                        : nvgRGBA(242, 245, 250, 242));
                    nvgText(vg, rowX + 68.f, ry + rowH * 0.5f, labels[i], nullptr);
                    nvgRestore(vg);
                }
            } else if (m_showPico8Option) {
                const float rowX = panelX + 28.f;
                const float rowY = panelY + 124.f;
                const float rowW = panelW - 56.f;
                const float rowH = 78.f;
                const float rowScale = 1.f - m_press * 0.018f;
                nvgSave(vg);
                nvgTranslate(vg, rowX + rowW * 0.5f, rowY + rowH * 0.5f);
                nvgScale(vg, rowScale, rowScale);
                nvgTranslate(vg, -(rowX + rowW * 0.5f), -(rowY + rowH * 0.5f));
                nvgBeginPath(vg);
                nvgRoundedRect(vg, rowX, rowY, rowW, rowH, 12.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, 32));
                nvgFill(vg);
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 112));
                nvgStrokeWidth(vg, 1.f);
                nvgStroke(vg);
                nvgFontFaceId(vg, m_materialFont);
                nvgFontSize(vg, 30.f);
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(96, 195, 255, 245));
                nvgText(vg, rowX + 24.f, rowY + rowH * 0.5f, settingsIcon.c_str(), nullptr);
                nvgFontFaceId(vg, m_defaultFont);
                nvgFontSize(vg, 22.f);
                nvgFillColor(vg, nvgRGBA(242, 245, 250, 242));
                nvgText(vg, rowX + 72.f, rowY + rowH * 0.5f,
                        L("PICO-8入口显示").c_str(), nullptr);
                _drawChoice(vg, rowX + rowW - 220.f, rowY + 17.f, "是", m_pico8Visible);
                _drawChoice(vg, rowX + rowW - 112.f, rowY + 17.f, "否", !m_pico8Visible);
                nvgRestore(vg);
            }

            float cursor = panelX + panelW - 30.f;
            const std::string hintA = m_showLayoutButtons ? L("选择") : L("切换");
            _drawHint(vg, brls::BUTTON_START, L("关闭").c_str(), cursor, panelY + panelH - 27.f, alpha);
            _drawHint(vg, brls::BUTTON_B, L("返回").c_str(), cursor, panelY + panelH - 27.f, alpha);
            _drawHint(vg, brls::BUTTON_A, hintA.c_str(),
                      cursor, panelY + panelH - 27.f, alpha);
        }

        std::function<void(bool)> onPico8VisibleChanged;
        std::function<void()> onClosed;
        std::function<void()> onLayoutEditRequested;
        std::function<void()> onCardSettingsRequested;

        void setShowPico8Option(bool show) { m_showPico8Option = show; }
        void setShowLayoutButtons(bool show) { m_showLayoutButtons = show; }

    private:
        static float _clamp01(float value)
        {
            return std::max(0.f, std::min(1.f, value));
        }

        static float _smooth(float value)
        {
            value = _clamp01(value);
            return value * value * (3.f - 2.f * value);
        }

        static float _back(float value)
        {
            value = _clamp01(value);
            constexpr float c1 = 1.35f;
            constexpr float c3 = c1 + 1.f;
            const float shifted = value - 1.f;
            return 1.f + c3 * shifted * shifted * shifted + c1 * shifted * shifted;
        }

        static std::string _utf8(char32_t codepoint)
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

        void _finishClose()
        {
            m_open = false;
            m_closing = false;
            setVisibility(brls::Visibility::GONE);
            if (onClosed)
                onClosed();
        }

        void _drawChoice(NVGcontext* vg, float x, float y,
                         const char* text, bool selected)
        {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, 86.f, 44.f, 10.f);
            nvgFillColor(vg, selected
                ? nvgRGBA(91, 193, 255, 220)
                : nvgRGBA(255, 255, 255, 12));
            nvgFill(vg);
            nvgStrokeColor(vg, selected
                ? nvgRGBA(168, 224, 255, 230)
                : nvgRGBA(255, 255, 255, 45));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 20.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, selected
                ? nvgRGBA(18, 24, 34, 245)
                : nvgRGBA(220, 228, 240, 225));
            nvgText(vg, x + 43.f, y + 22.f, text, nullptr);
        }

        void _drawHint(NVGcontext* vg, brls::ControllerButton button,
                       const char* text, float& cursor, float y, float alpha)
        {
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 16.f);
            float bounds[4]{};
            nvgTextBounds(vg, 0, 0, text, nullptr, bounds);
            cursor -= bounds[2] - bounds[0] + 41.f;
            nvgFontFaceId(vg, m_switchFont);
            nvgFontSize(vg, 26.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255,
                static_cast<unsigned char>(245.f * alpha)));
            const std::string glyph = brls::Hint::getKeyIcon(button);
            nvgText(vg, cursor + 12.f, y, glyph.c_str(), nullptr);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 16.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgText(vg, cursor + 28.f, y, text, nullptr);
            cursor -= 12.f;
        }

        bool m_open = false;
        bool m_closing = false;
        bool m_pico8Visible = true;
        bool m_showPico8Option = true;
        bool m_showLayoutButtons = false;
        int m_selectedRow = 0;
        float m_progress = 0.f;
        float m_press = 0.f;
        int m_defaultFont = -1;
        int m_materialFont = -1;
        int m_switchFont = -1;
        std::chrono::steady_clock::time_point m_lastFrame;
    };

    namespace
    {
        // Leave the first frame free, then prepare the complete library snapshot.
        constexpr long START_PAGE_REFRESH_DEFER_MS = 16;

        bool shouldUseNdsExternalNro(const beiklive::GameEntry& entry)
        {
#ifdef __SWITCH__
            return entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS);
#else
            (void)entry;
            return false;
#endif
        }

        bool shouldUseNdsExternalNro(const beiklive::DirListData& dirItem)
        {
#ifdef __SWITCH__
            return dirItem.itemType == beiklive::enums::FileType::NDS_ROM;
#else
            (void)dirItem;
            return false;
#endif
        }

        bool shouldUseThreeDsExternalNro(const beiklive::GameEntry& entry)
        {
            return entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS);
        }

        bool shouldUseThreeDsExternalNro(const beiklive::DirListData& dirItem)
        {
            return dirItem.itemType == beiklive::enums::FileType::THREEDS_ROM;
        }

        bool shouldUseArcadeExternalNro(const beiklive::GameEntry& entry)
        {
            return entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuArcade);
        }

        bool shouldUseArcadeExternalNro(const beiklive::DirListData& dirItem)
        {
            return dirItem.itemType == beiklive::enums::FileType::ARCADE_ROM;
        }

        bool shouldUseDreamcastExternalNro(const beiklive::GameEntry& entry)
        {
            return entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuDreamcast);
        }

        bool shouldUseDreamcastExternalNro(const beiklive::DirListData& dirItem)
        {
            return dirItem.itemType == beiklive::enums::FileType::DREAMCAST_ROM;
        }

        bool shouldUsePspExternalNro(const beiklive::GameEntry& entry)
        {
            return entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP);
        }

        bool shouldUsePspExternalNro(const beiklive::DirListData& dirItem)
        {
            return dirItem.itemType == beiklive::enums::FileType::PSP_ROM;
        }

        [[maybe_unused]] bool exportThreeDsCoreConfig()
        {
            if (!beiklive::SettingManager)
                return false;

            json root = json::object();
            const auto putInt = [&root](const char* outKey, const char* settingKey, int def) {
                root[outKey] = GET_SETTING_KEY_INT(settingKey, def);
            };
            const auto putFloat = [&root](const char* outKey, const char* settingKey, float def) {
                root[outKey] = GET_SETTING_KEY_FLOAT(settingKey, def);
            };
            const auto putStr = [&root](const char* outKey, const char* settingKey, const char* def) {
                root[outKey] = GET_SETTING_KEY_STR(settingKey, def);
            };

            putInt("upscale", "core.azahar.upscale", 1);
            putInt("use_cpu_jit", "core.azahar.use_cpu_jit", 1);
            putInt("new_3ds", "core.azahar.new_3ds", 1);
            putInt("cpu_clock", "core.azahar.cpu_clock", 100);
            putStr("region", "core.azahar.region", "auto");
            putStr("language", "core.azahar.language", "");
            putStr("username", "core.azahar.username", "");
            putStr("input_type", "core.azahar.input_type", "null");
            putInt("use_hw_shader", "core.azahar.use_hw_shader", 1);
            putInt("use_shader_jit", "core.azahar.use_shader_jit", 1);
            putInt("accurate_mul", "core.azahar.accurate_mul", 1);
            putInt("disk_shader_cache", "core.azahar.disk_shader_cache", 1);
            putInt("async_gpu", "core.azahar.async_gpu", 0);
            putInt("strict_gpu_sync", "core.azahar.strict_gpu_sync", 0);
            putInt("async_shaders", "core.azahar.async_shaders", 1);
            putInt("show_shader_compile_notice", "core.azahar.show_shader_compile_notice", 1);
            putInt("async_presentation", "core.azahar.async_presentation", 1);
            putInt("spirv_shader_gen", "core.azahar.spirv_shader_gen", 1);
            putInt("disable_spirv_optimizer", "core.azahar.disable_spirv_optimizer", 1);
            putInt("vsync", "core.azahar.vsync", 1);
            putFloat("frame_limit", "core.azahar.frame_limit", 100.0f);
            putInt("simulate_3ds_gpu_timings", "core.azahar.simulate_3ds_gpu_timings", 0);
            putInt("renderer_debug", "core.azahar.renderer_debug", 0);
            putInt("dump_command_buffers", "core.azahar.dump_command_buffers", 0);
            putInt("disable_right_eye", "core.azahar.disable_right_eye", 1);
            putStr("texture_filter", "core.azahar.texture_filter", "none");
            putStr("texture_sampling", "core.azahar.texture_sampling", "game");
            putInt("custom_textures", "core.azahar.custom_textures", 0);
            putInt("dump_textures", "core.azahar.dump_textures", 0);
            putInt("use_virtual_sd", "core.azahar.use_virtual_sd", 1);
            putStr("layout", "core.azahar.layout", "default");
            putStr("small_screen_position", "core.azahar.small_screen_position", "bottom_right");
            putStr("display_orientation", "core.azahar.display_orientation", "horizontal");
            putStr("display_rotation", "core.azahar.display_rotation", "0");
            putStr("display_size", "core.azahar.display_size", "default");
            putFloat("large_screen_proportion", "core.azahar.large_screen_proportion", 4.0f);
            putStr("audio_emulation", "core.azahar.audio_emulation", "hle");
            putInt("audio_stretching", "core.azahar.audio_stretching", 0);
            putInt("realtime_audio", "core.azahar.realtime_audio", 1);
            root["fastforward.multiplier"] = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);

#ifdef __SWITCH__
            const std::filesystem::path dir("sdmc:/GBAStation/3ds/config/cores");
            const std::filesystem::path file("sdmc:/GBAStation/3ds/config/cores/azahar.jsonc");
#else
            const std::filesystem::path dir = std::filesystem::path(beiklive::path::ROOT) /
                "GBAStation" / "3ds" / "config" / "cores";
            const std::filesystem::path file = dir / "azahar.jsonc";
#endif
            preserveThreeDsMenuSettings(root, file);
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            if (ec) {
                brls::Logger::warning("3DS config directory create failed: {}", ec.message());
                return false;
            }
            std::ofstream out(file, std::ios::trunc);
            if (!out.is_open()) {
                brls::Logger::warning("3DS config export failed: {}", file.string());
                return false;
            }
            out << root.dump(2) << "\n";
            brls::Logger::info("3DS config exported: {}", file.string());
            return true;
        }

        void ensureGameDbEntryForFileLaunch(const beiklive::DirListData& dirItem)
        {
            if (!beiklive::GameDB || dirItem.fullPath.empty())
                return;

            auto entryOpt = beiklive::GameDB->findByPath(dirItem.fullPath);
            beiklive::GameEntry entry = entryOpt.value_or(beiklive::GameEntry{});
            bool changed = !entryOpt.has_value();

            const int platform = static_cast<int>(dirItem.itemType);
            const std::string stem = beiklive::tools::getFileNameWithoutExtension(dirItem.fileName);

            if (entry.path.empty()) {
                entry.path = dirItem.fullPath;
                changed = true;
            }
            if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::NONE)) {
                entry.platform = platform;
                changed = true;
            }
            if (entry.core.empty()) {
                entry.core = beiklive::GetDefaultCoreId(platform);
                changed = true;
            }
            entry.core = beiklive::NormalizeCoreId(entry.platform, entry.core);
            if (entry.title.empty()) {
                entry.title = GET_MAPPING_KEY_STR(stem, stem);
                changed = true;
            }
            if (entry.savePath.empty()) {
                entry.savePath = beiklive::tools::defaultGameSavePath(entry.platform, entry.path);
                changed = true;
            }
            if (entry.logoPath.empty()) {
                entry.logoPath = beiklive::tools::getDefaultLogoPath(
                    static_cast<beiklive::enums::EmuPlatform>(entry.platform),
                    entry.path);
                changed = true;
            }
            if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP)) {
                // PSP ROM：文件浏览打开时提取真实游戏标题与 ICON0 封面
                // （保存到该 ROM 的存档目录）。TITLE 仅在仍是默认文件名/映射名
                // 时覆盖；封面仅当仍是默认资源图时替换。
                const std::string realTitle = beiklive::psp_meta::ExtractTitle(entry.path);
                if (!realTitle.empty() && entry.title == GET_MAPPING_KEY_STR(stem, stem)) {
                    entry.title = realTitle;
                    changed = true;
                }
                if (entry.logoPath.empty() ||
                    entry.logoPath == beiklive::tools::getDefaultLogoPath(
                        static_cast<beiklive::enums::EmuPlatform>(entry.platform), entry.path))
                {
                    const std::string icon = beiklive::psp_meta::ExtractIcon0(entry.path, entry.savePath);
                    if (!icon.empty()) {
                        entry.logoPath = icon;
                        changed = true;
                    }
                }
            }

            if (beiklive::tools::tryUseNdsInternalIconCover(entry))
                changed = true;
            if (entry.screenShotPath.empty()) {
                entry.screenShotPath = beiklive::path::screenshotPath();
                changed = true;
            }

            if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS)) {
                const std::string titleId = beiklive::three_ds::resolveTitleId(
                    entry.threeDsTitleId, entry.path);
                if (!titleId.empty() && titleId != entry.threeDsTitleId) {
                    entry.threeDsTitleId = titleId;
                    changed = true;
                }
            }

            if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS)) {
                if (entry.ndsScreenLayout.empty()) {
                    entry.ndsScreenLayout = "priority_top";
                    changed = true;
                }
                if (entry.ndsScreenOrientation.empty()) {
                    entry.ndsScreenOrientation = "0";
                    changed = true;
                }
            }

            std::error_code ec;
            std::filesystem::create_directories(entry.savePath, ec);
            beiklive::GameDB->upsertByPath(entry);
            if (changed)
                brls::Logger::info("StartPage: added file launch entry to GameDB: {}", entry.path);
            beiklive::GameDB->flush();
        }

#ifdef __SWITCH__
        bool launchNdsExternalNro(const std::string& romPath, const std::string& title)
        {
            const std::string nroPath = GET_SETTING_KEY_STR("nds.externalNro.path", "/GBAStation/core/GBAStationNDSStub.nro");
            const std::string returnPath = GET_SETTING_KEY_STR("nds.externalNro.returnPath", "sdmc:/switch/GBAStation.nro");

            auto result = beiklive::switch_platform::launchNroOnExit({nroPath, romPath, returnPath});
            if (!result.success)
            {
                brls::Logger::error("NDS external NRO launch failed for {}: {}", title, result.message);
                brls::Application::notify(L("NDS独立NRO启动失败：") + result.message);
                return false;
            }

            brls::Logger::info("NDS external NRO configured for {}: {}", title, result.message);
            brls::Application::notify(L("正在启动NDS独立NRO..."));
            brls::sync([]() { brls::Application::quit(); });
            return true;
        }

        bool launchThreeDsExternalNro(const std::string& romPath, const std::string& title)
        {
            exportThreeDsCoreConfig();
            const std::string nroPath = GET_SETTING_KEY_STR(
                "3ds.externalNro.path", "/GBAStation/core/GBAStation3DSStub.nro");
            const std::string returnPath = GET_SETTING_KEY_STR(
                "3ds.externalNro.returnPath", "sdmc:/switch/GBAStation.nro");

            auto result = beiklive::switch_platform::launchNroOnExit(
                {nroPath, romPath, returnPath});
            if (!result.success)
            {
                brls::Logger::error("3DS external NRO launch failed for {}: {}", title, result.message);
                brls::Application::notify(L("3DS独立NRO启动失败：") + result.message);
                return false;
            }

            brls::Logger::info("3DS external NRO configured for {}: {}", title, result.message);
            brls::Application::notify(L("正在启动3DS独立NRO..."));
            brls::sync([]() { brls::Application::quit(); });
            return true;
        }

        bool launchExternalCoreNro(const std::string& romPath,
                                   const std::string& title,
                                   const std::string& label,
                                   int platform,
                                   const char* pathKey,
                                   const char* defaultPath,
                                   const char* returnKey)
        {
            if (platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuDreamcast))
            {
                std::string extension = std::filesystem::path(romPath).extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(),
                               [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                if (extension == ".iso")
                {
                    brls::Application::notify(
                        L("DC外部核心不支持缺少轨道信息的原始ISO，请转换为CHD，或使用GDI/CDI/CUE"));
                    return false;
                }
                if (extension == ".zip" || extension == ".7z")
                {
                    brls::Application::notify(
                        L("DC外部核心不能直接运行压缩包，请先解压为CHD/GDI/CDI/CUE"));
                    return false;
                }
            }

            const std::string nroPath = GET_SETTING_KEY_STR(pathKey, defaultPath);
            const std::string returnPath = GET_SETTING_KEY_STR(returnKey, "sdmc:/switch/GBAStation.nro");
			const std::string sessionToken = beiklive::makeExternalCoreSessionToken(romPath);

			beiklive::switch_platform::NroLaunchRequest request;
			request.nroPath = nroPath;
			request.romPath = romPath;
			request.returnNroPath = returnPath;
			request.extraArgs = {"--gbastation-session", sessionToken};
			auto result = beiklive::switch_platform::launchNroOnExit(request);
            if (!result.success)
            {
                brls::Logger::error("{} external NRO launch failed for {}: {}", label, title, result.message);
                brls::Application::notify(label + std::string(L("独立NRO启动失败：")) + result.message);
                return false;
            }

			if (!beiklive::beginExternalCoreSession(romPath, platform, sessionToken))
				brls::Logger::error("{} external session tracking could not start for {}", label, romPath);

            brls::Logger::info("{} external NRO configured for {}: {}", label, title, result.message);
            brls::Application::notify(L("正在启动") + label + L("独立NRO..."));
            brls::sync([]() { brls::Application::quit(); });
            return true;
        }

#endif
    }

    StartPage::StartPage()
    {
        brls::Logger::debug("StartPage initialized");
        brls::sync([this]()
                   {
        this->showHeader(false);
        this->hideFooterLine();
        this->showFooter(false);
        // this->showBackground(true);
        // 动态背景由 Box::setupShaderLayer 根据配置初始化
        Init();
        brls::Application::giveFocus(this); });
    }

    StartPage::~StartPage()
    {
        m_alive.store(false);
        m_aliveToken->store(false);
    }

    void StartPage::Init()
    {
        // 读取主题配置
        if (!CHECK_KEY("theme"))
        {
            SET_SETTING_KEY_INT("theme", (int)beiklive::enums::ThemeLayout::SWITCH_THEME);
        }
        int theme = GET_SETTING_KEY_INT("theme", (int)beiklive::enums::ThemeLayout::SWITCH_THEME);
        brls::Logger::debug("Current theme: " + std::to_string(theme));

        if (theme == (int)beiklive::enums::ThemeLayout::IISU_THEME)
        {
            _useIisuLayout();
        }
        else
        {
            _useSwitchLayout();
        }
    }

    void StartPage::onResume()
    {
        brls::Logger::debug("StartPage onResume called");
        _applyRuntimeUiSettings();
        _requestRecentGamesRefresh(true);
    }

    void StartPage::onActivityResume()
    {
        if (switchLayout)
            switchLayout->playEntranceAnimation();
        else if (iisuLayout)
            iisuLayout->playEntranceAnimation();
        onResume();
    }

    void StartPage::_applyRuntimeUiSettings()
    {
        // 重新读取动态背景配置（设置页面可能已修改）
        bool enableShader = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_SHADER, 1) != 0;
        this->showShader(enableShader);
        if (enableShader) {
            std::string themeStr = GET_SETTING_KEY_STR(beiklive::SettingKey::KEY_UI_GRADIENT_THEME, "VscodeBlack");
            if (themeStr == "Midnight")           this->setGradientTheme(GradientTheme::Midnight);
            else if (themeStr == "LemonYellow")   this->setGradientTheme(GradientTheme::LemonYellow);
            else if (themeStr == "AvocadoGreen")  this->setGradientTheme(GradientTheme::AvocadoGreen);
            else if (themeStr == "StrawberryRed") this->setGradientTheme(GradientTheme::StrawberryRed);
            else if (themeStr == "OceanBlue")     this->setGradientTheme(GradientTheme::OceanBlue);
            else if (themeStr == "SakuraPink")    this->setGradientTheme(GradientTheme::SakuraPink);
            else                                   this->setGradientTheme(GradientTheme::VscodeBlack);
        }
        // 重新读取背景图片配置
        bool enableBg = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, 0) != 0;
        this->showBackground(enableBg);
        if (enableBg) {
            std::string bgPath = GET_SETTING_KEY_STR(beiklive::SettingKey::KEY_UI_BG_IMAGE_PATH, "");
            if (!bgPath.empty())
                this->setBackgroundImage(bgPath);
        }
        if (switchLayout) {
            switchLayout->setPico8ShortcutVisible(
                GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_PICO8_SHORTCUT_VISIBLE, 1) != 0);
        }
        if (iisuLayout) {
            iisuLayout->setPico8ShortcutVisible(
                GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_PICO8_SHORTCUT_VISIBLE, 1) != 0);
        }
    }

    void StartPage::_requestRecentGamesRefresh(bool defer)
    {
        // 每次回到起始页时刷新游戏列表，获取最新的最近玩过的10款游戏
        if ((!switchLayout && !iisuLayout) || m_homeDeletePending)
            return;

        int gen = ++m_recentRefreshGen;
        auto dispatchRefresh = [this, gen]() {
            if (!m_alive.load() || gen != m_recentRefreshGen.load())
                return;

            ThreadPool::instance().enqueuePriority([this, gen]() {
                if (!m_alive.load() || gen != m_recentRefreshGen.load()) return;

                auto prepared = beiklive::GameLibraryPage::prepareInitialData();
                beiklive::GameList recent;
                const size_t recentCount = std::min<size_t>(
                    10, prepared.entries.size());
                recent.reserve(recentCount);
                recent.insert(recent.end(), prepared.entries.begin(),
                              prepared.entries.begin() + recentCount);

                brls::sync([this, gen, recent = std::move(recent),
                            prepared = std::move(prepared)]() mutable {
                    if (!m_alive.load() || gen != m_recentRefreshGen.load() || !switchLayout) return;

                    m_libraryPreparedData = std::move(prepared);
                    brls::View* currentFocus = brls::Application::getCurrentFocus();
                    bool needInitialCardFocus = !currentFocus || currentFocus == this || currentFocus->isHidden();
                    if (switchLayout)
                        switchLayout->refreshGameList(recent);
                    if (iisuLayout)
                        iisuLayout->refreshGameList(recent);
                    if (m_resetCardFocusOnNextRefresh)
                    {
                        m_resetCardFocusOnNextRefresh = false;
                        if (switchLayout)
                            switchLayout->resetCardFocusToFirst();
                        if (iisuLayout)
                            iisuLayout->resetCardFocusToFirst();
                    }
                    else if (needInitialCardFocus)
                    {
                        if (switchLayout)
                            switchLayout->restoreCardFocus(false);
                        if (iisuLayout)
                            iisuLayout->restoreCardFocus(false);
                    }
                });
            });
        };

        if (defer)
            brls::delay(START_PAGE_REFRESH_DEFER_MS, dispatchRefresh);
        else
            dispatchRefresh();
    }

    void StartPage::willAppear(bool resetState)
    {
        brls::Box::willAppear(resetState);
        onActivityResume();
    }

    bool StartPage::_pushGameActivity(const beiklive::GameEntry& entry,
                                      beiklive::Box* previousPage)
    {
        if (!beiklive::tools::isFileExists(entry.path)) {
            brls::Application::notify(L("文件不存在: ") + entry.title);
            return false;
        }
        if (shouldUseNdsExternalNro(entry))
        {
#ifdef __SWITCH__
            if (!beiklive::ensureNdsEnvironmentReady())
                return false;
            return launchNdsExternalNro(entry.path, entry.title);
#endif
        }
        if (shouldUseThreeDsExternalNro(entry))
        {
#ifdef __SWITCH__
            return launchThreeDsExternalNro(entry.path, entry.title);
#else
            brls::Application::notify(L("3DS 独立运行时仅支持 Switch"));
            return false;
#endif
        }
        if (shouldUseArcadeExternalNro(entry))
        {
#ifdef __SWITCH__
            return launchExternalCoreNro(entry.path, entry.title, "Arcade",
                static_cast<int>(beiklive::enums::EmuPlatform::EmuArcade),
                "arcade.externalNro.path", "/GBAStation/core/GBAStationFBNeoStub.nro",
                "arcade.externalNro.returnPath");
#else
            brls::Application::notify(L("Arcade 独立运行时仅支持 Switch"));
            return false;
#endif
        }
        if (shouldUseDreamcastExternalNro(entry))
        {
#ifdef __SWITCH__
            return launchExternalCoreNro(entry.path, entry.title, "DC",
                static_cast<int>(beiklive::enums::EmuPlatform::EmuDreamcast),
                "dc.externalNro.path", "/GBAStation/core/GBAStationFlycastStub.nro",
                "dc.externalNro.returnPath");
#else
            brls::Application::notify(L("DC 独立运行时仅支持 Switch"));
            return false;
#endif
        }
        if (shouldUsePspExternalNro(entry))
        {
#ifdef __SWITCH__
            return launchExternalCoreNro(entry.path, entry.title, "PSP",
				static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP),
                "psp.externalNro.path", "/GBAStation/core/GBAStationPPSSPPStub.nro",
                "psp.externalNro.returnPath");
#else
            brls::Application::notify(L("PSP 独立运行时仅支持 Switch"));
            return false;
#endif
        }

        auto* gamePage = new beiklive::GamePage(entry);
        m_gamePage = gamePage;
        auto* frame = new brls::AppletFrame(gamePage);
        HIDE_BRLS_BAR(frame);
        brls::Logger::info("Pushing GamePage activity for: " + entry.title);
        // GamePage 退出仍使用 beiklive::popActivity，因此保留其返回页记录，
        // 但推入过程直接交给 Borealis，避免再次播放页面级隐藏动画。
        beiklive::g_beiklive_boxes.push_back(previousPage);
        brls::Application::pushActivity(
            new brls::Activity(frame), brls::TransitionAnimation::NONE);
        if (auto* library = dynamic_cast<beiklive::GameLibraryPage*>(previousPage))
            library->resetLaunchOverlay();
        gamePage->startGame();
        return true;
    }

void StartPage::_launchDirItem(const beiklive::DirListData& dirItem, beiklive::Box* previousPage)
    {
        if (!beiklive::tools::isFileExists(dirItem.fullPath)) {
            brls::Application::notify(L("文件不存在: ") + dirItem.fileName);
            return;
        }
        if (shouldUseThreeDsExternalNro(dirItem))
        {
            ensureGameDbEntryForFileLaunch(dirItem);
#ifdef __SWITCH__
            launchThreeDsExternalNro(dirItem.fullPath, dirItem.fileName);
            return;
#else
            brls::Application::notify(L("3DS 独立运行时仅支持 Switch"));
            return;
#endif
        }
        if (shouldUseNdsExternalNro(dirItem))
        {
            if (!beiklive::ensureNdsEnvironmentReady())
                return;
            ensureGameDbEntryForFileLaunch(dirItem);
#ifdef __SWITCH__
            launchNdsExternalNro(dirItem.fullPath, dirItem.fileName);
            return;
#endif
        }
        if (shouldUseArcadeExternalNro(dirItem))
        {
            ensureGameDbEntryForFileLaunch(dirItem);
#ifdef __SWITCH__
            launchExternalCoreNro(dirItem.fullPath, dirItem.fileName, "Arcade",
                static_cast<int>(beiklive::enums::EmuPlatform::EmuArcade),
                "arcade.externalNro.path", "/GBAStation/core/GBAStationFBNeoStub.nro",
                "arcade.externalNro.returnPath");
            return;
#else
            brls::Application::notify(L("Arcade 独立运行时仅支持 Switch"));
            return;
#endif
        }
        if (shouldUseDreamcastExternalNro(dirItem))
        {
            ensureGameDbEntryForFileLaunch(dirItem);
#ifdef __SWITCH__
            launchExternalCoreNro(dirItem.fullPath, dirItem.fileName, "DC",
                static_cast<int>(beiklive::enums::EmuPlatform::EmuDreamcast),
                "dc.externalNro.path", "/GBAStation/core/GBAStationFlycastStub.nro",
                "dc.externalNro.returnPath");
            return;
#else
            brls::Application::notify(L("DC 独立运行时仅支持 Switch"));
            return;
#endif
        }
        if (shouldUsePspExternalNro(dirItem))
        {
            ensureGameDbEntryForFileLaunch(dirItem);
#ifdef __SWITCH__
            launchExternalCoreNro(dirItem.fullPath, dirItem.fileName, "PSP",
				static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP),
                "psp.externalNro.path", "/GBAStation/core/GBAStationPPSSPPStub.nro",
                "psp.externalNro.returnPath");
            return;
#else
            brls::Application::notify(L("PSP 独立运行时仅支持 Switch"));
            return;
#endif
        }

        auto* gamePage = new beiklive::GamePage(dirItem);
        m_gamePage = gamePage;
        auto* frame = new brls::AppletFrame(gamePage);
        HIDE_BRLS_BAR(frame);
        brls::Logger::info("Pushing GamePage activity for: " + dirItem.fileName);
        beiklive::g_beiklive_boxes.push_back(previousPage);
        brls::Application::pushActivity(
            new brls::Activity(frame), brls::TransitionAnimation::NONE);
        gamePage->startGame();
    }

void StartPage::_pushGameActivity(const beiklive::DirListData& dirItem, beiklive::Box* previousPage)
{
    if (!beiklive::tools::isFileExists(dirItem.fullPath))
    {
        brls::Application::notify(L("文件不存在: ") + dirItem.fileName);
        return;
    }

    // 歧义后缀（iso/bin/cue/zip/7z 等可属于多个机种）：弹窗让用户选择机种。
    const std::string ext = beiklive::tools::getFileExtension(
        std::filesystem::path(dirItem.fullPath));
    const auto candidates = beiklive::tools::candidatePlatformsForExtension(ext);
    if (candidates.size() > 1)
    {
        const int currentPlatform = beiklive::tools::platformFromFileType(dirItem.itemType);
        int defaultIndex = 0;
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            if (candidates[i] == currentPlatform)
            {
                defaultIndex = static_cast<int>(i);
                break;
            }
        }
        _showPlatformPicker(dirItem, previousPage, candidates, defaultIndex);
        return;
    }
    _launchDirItem(dirItem, previousPage);
}

void StartPage::_showPlatformPicker(const beiklive::DirListData& dirItem,
                                    beiklive::Box* previousPage,
                                    const std::vector<int>& candidates,
                                    int defaultIndex)
{
    if (!m_platformPicker || m_platformPicker->isOpen())
        return;
    m_platformPicker->onPicked = [this, dirItem, previousPage](int platform) {
        if (m_fileListPage)
        {
            m_fileListPage->setInteractionDisabled(false);
            m_fileListPage->setPickerActive(false);
        }
        if (platform < 0)
            return; // 取消
        beiklive::DirListData forced = dirItem;
        forced.itemType = platformToFileType(platform);
        _launchDirItem(forced, previousPage);
    };
    if (m_fileListPage)
    {
        m_fileListPage->setInteractionDisabled(true);
        m_fileListPage->setPickerActive(true);
    }
    m_platformPicker->open(candidates, defaultIndex, dirItem.fileName);
}

    void StartPage::_useSwitchLayout()
    {
        brls::Logger::debug("Using SWITCH theme layout");
        switchLayout = new beiklive::SwitchLayout();
        switchLayout->setGrow(1.f);
        switchLayout->setPico8ShortcutVisible(
            GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_PICO8_SHORTCUT_VISIBLE, 1) != 0);
        switchLayout->onGameActivated = [this](const beiklive::GameEntry &entry)
        {
            m_resetCardFocusOnNextRefresh = true;
            auto fresh = beiklive::GameDB
                ? beiklive::GameDB->findByPath(entry.path)
                : std::optional<beiklive::GameEntry>{};
            const auto& e = fresh.has_value() ? *fresh : entry;
            brls::Logger::info("Game activated: " + e.title);
            if (!_pushGameActivity(e, this)) {
                m_gameLaunchPending = false;
                if (switchLayout) {
                    switchLayout->playEntranceAnimation();
                    switchLayout->restoreCardFocus(false);
                }
            }
        };
        switchLayout->onGameOptions = [this](const beiklive::GameEntry &entry)
        {
            brls::Logger::info("Game options opened: " + entry.title);
            _showGameOptionsPanel(entry);
        };

        switchLayout->onGameLibraryOpened = [this]()
        {
            brls::Logger::info("Game Library opened");
            _openGameLibrary();
        };

        switchLayout->onFileBrowserOpened = [this]()
        {
            brls::Logger::info("File Browser opened");
            _openFileList();
        };
        switchLayout->onDataManagementOpened = [this]()
        {
            brls::Logger::info("Data Management opened");
            _openDataManagement();
        };
        switchLayout->onSettingsOpened = [this]()
        {
            brls::Logger::info("Settings opened");
            _openSettings();
        };
        switchLayout->onAboutOpened = [this]()
        {
            brls::Logger::info("About opened");
            _openAbout();
        };
        switchLayout->onPico8Opened = [this]()
        {
            brls::Logger::info("PICO-8 shortcut opened");
            _openPico8Page();
        };
        switchLayout->onExitRequested = [this]()
        {
            brls::Logger::info("Exit requested");
            if (switchLayout) {
                switchLayout->playExitAnimation([]() {
                    brls::Application::quit();
                });
            } else {
                brls::Application::quit();
            }
        };
        switchLayout->registerAction(L("设置主页"), brls::BUTTON_START,
            [this](brls::View*) -> bool {
                _showShortcutSettings();
                return true;
            },
            false, false, brls::SOUND_CLICK);
        this->getContentBox()->addView(switchLayout);
        m_shortcutSettingsOverlay = new HomeShortcutSettingsOverlay();
        m_shortcutSettingsOverlay->setShowPico8Option(true);
        m_shortcutSettingsOverlay->onPico8VisibleChanged = [this](bool visible) {
            SET_SETTING_KEY_INT(
                beiklive::SettingKey::KEY_UI_PICO8_SHORTCUT_VISIBLE,
                visible ? 1 : 0);
            if (switchLayout)
                switchLayout->setPico8ShortcutVisible(visible);
            brls::Application::notify(
                visible ? L("已显示 PICO-8 入口") : L("已隐藏 PICO-8 入口"));
        };
        m_shortcutSettingsOverlay->onClosed = [this]() {
            if (switchLayout)
                brls::Application::giveFocus(switchLayout);
        };
        this->getContentBox()->addView(m_shortcutSettingsOverlay);
        _requestRecentGamesRefresh(true);
    }

    void StartPage::_useIisuLayout()
    {
        brls::Logger::debug("Using IISU theme layout");
        iisuLayout = new beiklive::IisuLayout();
        iisuLayout->setGrow(1.f);
        iisuLayout->setPico8ShortcutVisible(
            GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_PICO8_SHORTCUT_VISIBLE, 1) != 0);
        iisuLayout->onGameActivated = [this](const beiklive::GameEntry &entry)
        {
            m_resetCardFocusOnNextRefresh = true;
            auto fresh = beiklive::GameDB
                ? beiklive::GameDB->findByPath(entry.path)
                : std::optional<beiklive::GameEntry>{};
            const auto& e = fresh.has_value() ? *fresh : entry;
            brls::Logger::info("Game activated: " + e.title);
            if (!_pushGameActivity(e, this)) {
                m_gameLaunchPending = false;
                if (iisuLayout) {
                    iisuLayout->playEntranceAnimation();
                    iisuLayout->restoreCardFocus(false);
                }
            }
        };
        iisuLayout->onGameOptions = [this](const beiklive::GameEntry &entry)
        {
            brls::Logger::info("Game options opened: " + entry.title);
            _showGameOptionsPanel(entry);
        };

        iisuLayout->onGameLibraryOpened = [this]()
        {
            brls::Logger::info("Game Library opened");
            _openGameLibrary();
        };

        iisuLayout->onFileBrowserOpened = [this]()
        {
            brls::Logger::info("File Browser opened");
            _openFileList();
        };

        iisuLayout->onDataManagementOpened = [this]()
        {
            brls::Logger::info("Data Management opened");
            _openDataManagement();
        };

        iisuLayout->onSettingsOpened = [this]()
        {
            brls::Logger::info("Settings opened");
            _openSettings();
        };

        iisuLayout->onAboutOpened = [this]()
        {
            brls::Logger::info("About opened");
            _openAbout();
        };

        iisuLayout->onPico8Opened = [this]()
        {
            brls::Logger::info("PICO-8 shortcut opened (iisu layout)");
            brls::Application::notify(L("iisu 布局暂不支持 PICO-8 入口"));
        };

        iisuLayout->onExitRequested = [this]()
        {
            brls::Logger::info("Exit requested");
            if (iisuLayout) {
                iisuLayout->playExitAnimation([]() {
                    brls::Application::quit();
                });
            } else {
                brls::Application::quit();
            }
        };
        iisuLayout->registerAction(L("设置主页"), brls::BUTTON_START,
            [this](brls::View*) -> bool {
                _showShortcutSettings();
                return true;
            },
            false, false, brls::SOUND_CLICK);
        this->getContentBox()->addView(iisuLayout);
        m_shortcutSettingsOverlay = new HomeShortcutSettingsOverlay();
        m_shortcutSettingsOverlay->setShowPico8Option(false);
        m_shortcutSettingsOverlay->setShowLayoutButtons(true);
        m_shortcutSettingsOverlay->onLayoutEditRequested = [this]() {
            if (iisuLayout)
                iisuLayout->enterEditMode();
        };
        m_shortcutSettingsOverlay->onCardSettingsRequested = [this]() {
            if (iisuLayout)
                iisuLayout->requestCardSettings();
        };
        m_shortcutSettingsOverlay->onPico8VisibleChanged = [this](bool visible) {
            SET_SETTING_KEY_INT(
                beiklive::SettingKey::KEY_UI_PICO8_SHORTCUT_VISIBLE,
                visible ? 1 : 0);
            if (iisuLayout)
                iisuLayout->setPico8ShortcutVisible(visible);
            brls::Application::notify(
                visible ? L("已显示 PICO-8 入口") : L("已隐藏 PICO-8 入口"));
        };
        m_shortcutSettingsOverlay->onClosed = [this]() {
            if (iisuLayout)
                brls::Application::giveFocus(iisuLayout);
        };
        this->getContentBox()->addView(m_shortcutSettingsOverlay);
        _requestRecentGamesRefresh(true);
    }

    void StartPage::_showShortcutSettings()
    {
        if (!m_shortcutSettingsOverlay || m_shortcutSettingsOverlay->isOpen())
            return;
        const bool visible = GET_SETTING_KEY_INT(
            beiklive::SettingKey::KEY_UI_PICO8_SHORTCUT_VISIBLE, 1) != 0;
        m_shortcutSettingsOverlay->open(visible);
    }

    void StartPage::_openPico8Page()
    {
        if (!switchLayout) {
            brls::Application::unblockInputs();
            return;
        }
        auto* pico8Page = new beiklive::Pico8Page(switchLayout);
        brls::Application::pushActivity(
            new brls::Activity(pico8Page),
            brls::TransitionAnimation::NONE);
        brls::Application::unblockInputs();
    }

    void StartPage::_openGameLibrary()
    {
        if (!beiklive::GameDB ||
            (m_libraryPreparedData.ready &&
             m_libraryPreparedData.entries.empty())) {
            auto* dialog = new brls::Dialog(L("游戏库为空，请从文件列表选择游戏或者从设置中进行游戏导入"));
            dialog->addButton(L("确定"), [](){});
            dialog->open();
            return;
        }

        brls::Logger::debug("Opening Game Library Page");
        auto prepared = std::move(m_libraryPreparedData);
        m_libraryPreparedData = {};
        auto pushLibrary = [this, prepared = std::move(prepared)]() mutable {
            auto* gameLibraryPage =
                new beiklive::GameLibraryPage(std::move(prepared));
            auto* frame = new brls::AppletFrame(gameLibraryPage);
            gameLibraryPage->onGameSelected =
                [this, gameLibraryPage](const beiklive::GameEntry& entry) {
                    brls::Logger::info(
                        "Game selected from library: " + entry.title);
                    _pushGameActivity(entry, gameLibraryPage);
                };
            HIDE_BRLS_BAR(frame);
            beiklive::g_beiklive_boxes.push_back(this);
            brls::Application::pushActivity(
                new brls::Activity(frame), brls::TransitionAnimation::NONE);
        };
        if (switchLayout)
            switchLayout->playExitAnimation(std::move(pushLibrary));
        else
            pushLibrary();
    }

    void StartPage::_openFileList()
    {
        brls::Logger::debug("Opening File List Page");
        m_fileListPage = new beiklive::FileListPage();
        m_platformPicker = new PlatformPickerOverlay();
        m_fileListPage->addView(m_platformPicker);
        m_fileListPage->onRequestClose = [this]() {
            beiklive::popActivity(m_fileListPage);
        };

        m_fileListPage->registerAction(
            L("关闭列表"),
            brls::BUTTON_START,
            [this](brls::View *)
            {
                brls::sync([this]()
                           { beiklive::popActivity(m_fileListPage); });

                return true;
            });
        m_fileListPage->setFliter(beiklive::enums::FilterMode::Whitelist, {"gba", "gbc", "gb", "nes", "fds", "sfc", "smc", "nds", "cia", "cci", "3ds", "md", "gen", "bin", "smd", "sms", "gg", "sg", "cue", "cdi", "gdi", "chd", "iso", "cso", "pbp", "zip", "7z", "png"});

        m_fileListPage->onFileSelected = [this](beiklive::DirListData dirItem)
        {
            // 歧义后缀（iso/bin/cue/zip/7z 等）：多机种可运行，弹出机种选择
            const std::vector<int> candidates =
                beiklive::tools::candidatePlatformsForExtension(
                    beiklive::tools::getFileExtension(dirItem.fullPath));
            if (candidates.size() > 1)
            {
                _showPlatformPicker(dirItem, this, candidates, 0);
                return;
            }

            switch (dirItem.itemType)
            {
            case beiklive::enums::FileType::IMAGE_FILE:
                brls::Application::notify(L("查看图片：") + dirItem.fileName);
                break;
            case beiklive::enums::FileType::GBA_ROM:
            case beiklive::enums::FileType::GBC_ROM:
            case beiklive::enums::FileType::GB_ROM:
            case beiklive::enums::FileType::NES_ROM:
            case beiklive::enums::FileType::SNES_ROM:
            case beiklive::enums::FileType::NDS_ROM:
            case beiklive::enums::FileType::THREEDS_ROM:
            case beiklive::enums::FileType::GENESIS_ROM:
            case beiklive::enums::FileType::ARCADE_ROM:
            case beiklive::enums::FileType::DREAMCAST_ROM:
            case beiklive::enums::FileType::PSP_ROM:
                brls::Application::notify(L("启动游戏：") + dirItem.fileName);
                _pushGameActivity(dirItem, this);
                break;
            default:
                brls::Logger::debug("Selected item: " + dirItem.fileName + ", type: " + std::to_string((int)dirItem.itemType));
                break;
            }
        };

        auto *frame = new brls::AppletFrame(m_fileListPage);
        HIDE_BRLS_BAR(frame);
        auto pushPage = [this, frame]() {
            brls::Logger::info("Pushing FileListPage activity");
            beiklive::g_beiklive_boxes.push_back(this);
            brls::Application::pushActivity(
                new brls::Activity(frame), brls::TransitionAnimation::NONE);
            m_fileListPage->showDriveList();
        };
        if (switchLayout)
            switchLayout->playExitAnimation(std::move(pushPage));
        else
            pushPage();
    }

    void StartPage::_openSettings()
    {
        brls::Logger::debug("Opening Settings Page");
        auto *settingPage = new beiklive::SettingPage();
        auto *frame       = new brls::AppletFrame(settingPage);
        HIDE_BRLS_BAR(frame);
        auto pushPage = [this, frame]() {
            beiklive::g_beiklive_boxes.push_back(this);
            brls::Application::pushActivity(
                new brls::Activity(frame), brls::TransitionAnimation::NONE);
        };
        if (switchLayout)
            switchLayout->playExitAnimation(std::move(pushPage));
        else
            pushPage();
    }

    void StartPage::_openAbout()
    {
        brls::Logger::debug("Opening About Page");
        auto *aboutPage = new beiklive::AboutPage();
        auto *frame     = new brls::AppletFrame(aboutPage);
        HIDE_BRLS_BAR(frame);
        auto pushPage = [this, frame]() {
            beiklive::g_beiklive_boxes.push_back(this);
            brls::Application::pushActivity(
                new brls::Activity(frame), brls::TransitionAnimation::NONE);
        };
        if (switchLayout)
            switchLayout->playExitAnimation(std::move(pushPage));
        else
            pushPage();
    }

    void StartPage::_openDataManagement()
    {
        brls::Logger::debug("Opening Data Management Page");
        auto *dataPage = new beiklive::DataManagementPage();
        auto *frame    = new brls::AppletFrame(dataPage);
        HIDE_BRLS_BAR(frame);
        auto pushPage = [this, frame]() {
            beiklive::g_beiklive_boxes.push_back(this);
            brls::Application::pushActivity(
                new brls::Activity(frame), brls::TransitionAnimation::NONE);
        };
        if (switchLayout)
            switchLayout->playExitAnimation(std::move(pushPage));
        else
            pushPage();
    }

    void StartPage::_showGameOptionsPanel(const beiklive::GameEntry& entry)
    {
        _hideGameOptionsPanel();
        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        m_gameOptionsSidebar->setNanoVgMenu(true);
        m_gameOptionsSidebar->setLaunchFadeToBlack(true);
        if (switchLayout) {
            m_gameOptionsSidebar->setNanoVgPreviewImageHandle(
                switchLayout->acquireSelectedCoverTexture());
        }
        this->getBottomBar()->setVisibility(brls::Visibility::GONE);

        const std::string path = entry.path;
        const std::string filename =
            beiklive::tools::getFileNameWithoutExtension(entry.path);

        m_gameOptionsSidebar->addButton(
            L("启动游戏"), beiklive::material::PLAY_ARROW,
            [this, entry](const beiklive::GameEntry&) {
                if (entry.platform == static_cast<int>(
                        beiklive::enums::EmuPlatform::EmuNDS) &&
                    !beiklive::ensureNdsEnvironmentReady())
                    return;
                m_gameLaunchPending = true;
                if (switchLayout)
                    switchLayout->playExitAnimation();
                _closeGameOptionsPanelAnimated([this, entry]() {
                    if (switchLayout && switchLayout->onGameActivated)
                        switchLayout->onGameActivated(entry);
                    m_gameLaunchPending = false;
                }, true);
            });

        m_gameOptionsSidebar->addButton(
            entry.favourite ? L("取消收藏") : L("加入收藏"),
            entry.favourite ? beiklive::material::FAVORITE : beiklive::material::FAVORITE_BORDER,
            [this, path, fav = entry.favourite](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated([this, path, fav]() {
                    const std::string msg = fav
                        ? L("确定要取消收藏吗？")
                        : L("确定要加入收藏吗？");
                    auto* dlg = new brls::Dialog(msg);
                    dlg->addButton(L("确认"), [this, path, fav]() {
                        if (beiklive::GameDB) {
                            beiklive::GameDB->set(
                                path, "favourite", nlohmann::json(!fav));
                            beiklive::GameDB->flush();
                            _requestRecentGamesRefresh(false);
                        }
                    });
                    dlg->addButton(L("取消"), []() {});
                    dlg->open();
                });
            });

        const int operationsMenu = m_gameOptionsSidebar->addSubmenu(
            L("游戏操作"), beiklive::material::SETTINGS);

        m_gameOptionsSidebar->addSubmenuButton(
            operationsMenu, L("修改映射名称"), beiklive::material::EDIT,
            [this, path, title = entry.title, filename](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated(
                    [this, path, title, filename]() {
                        auto* ime = brls::Application::getPlatform()->getImeManager();
                        if (!ime)
                            return;
                        ime->openForText(
                            [this, path, filename](std::string text) {
                                if (text.empty() || !beiklive::GameDB)
                                    return;
                                beiklive::GameDB->set(
                                    path, "title", nlohmann::json(text));
                                beiklive::NameMappingManager->Set(
                                    filename, text, true);
                                beiklive::GameDB->flush();
                                beiklive::NameMappingManager->Save();
                                _requestRecentGamesRefresh(false);
                            },
                            L("编辑游戏名称"), "", 128, title,
                            brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
                    });
            });

        const int coverMenu = m_gameOptionsSidebar->addNestedSubmenu(
            operationsMenu, L("修改封面"), beiklive::material::IMAGE);

        m_gameOptionsSidebar->addNestedSubmenuButton(
            operationsMenu, coverMenu, L("从 SteamGridDB 获取"),
            beiklive::material::CLOUD_DOWNLOAD,
            [this, entry](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated([this, entry]() {
                    if (!beiklive::steamgriddb::hasApiKey()) {
                        brls::Application::notify(
                            L("请去设置-模拟器页面输入 SteamGridDB Api Key"));
                        return;
                    }
                    beiklive::openSteamGridDbPage(entry,
                        [this](const std::string&) {
                            _requestRecentGamesRefresh(false);
                        });
                });
            });

        m_gameOptionsSidebar->addNestedSubmenuButton(
            operationsMenu, coverMenu, L("从本地选择"), 0xE2C8,
            [this](const beiklive::GameEntry& game) {
                const auto pickerLocation = beiklive::getGameCoverPickerLocation(game);
                _closeGameOptionsPanelAnimated(
                    [this, game, pickerLocation]() {
                        beiklive::openFilePicker(
                            {"png", "jpg"},
                            [this, game](const std::string& selectedPath) {
                                if (selectedPath.empty())
                                    return;
                                beiklive::openCoverEditorPage(game, selectedPath,
                                    [this](const std::string&) {
                                        _requestRecentGamesRefresh(false);
                                    });
                            },
                            pickerLocation.startPath,
                            pickerLocation.filename);
                    });
            });

        m_gameOptionsSidebar->addSubmenuButton(
            operationsMenu, L("安装到 Switch 桌面"),
            beiklive::material::INSTALL_APP,
            [this](const beiklive::GameEntry& game) {
                _closeGameOptionsPanelAnimated([game]() {
                    beiklive::forwarder::showInstallDialog(game);
                });
            });

        if (beiklive::GetCoreOptions(entry.platform).size() > 1) {
            m_gameOptionsSidebar->addSubmenuButton(
                operationsMenu, L("核心切换"), beiklive::material::MEMORY,
                [this, path, platform = entry.platform,
                 core = entry.core](const beiklive::GameEntry&) {
                    _closeGameOptionsPanelAnimated(
                        [this, path, platform, core]() {
                            const auto options =
                                beiklive::GetCoreOptions(platform);
                            std::vector<std::string> names;
                            names.reserve(options.size());
                            for (const auto& option : options)
                                names.push_back(option.name);
                            auto* dropdown = new brls::Dropdown(
                                L("核心切换"), names,
                                [this, path, options](int selected) {
                                    if (selected < 0 ||
                                        selected >= static_cast<int>(options.size()))
                                        return;
                                    if (beiklive::GameDB) {
                                        beiklive::GameDB->set(
                                            path, "core",
                                            nlohmann::json(options[selected].id));
                                        beiklive::GameDB->flush();
                                        brls::Application::notify(
                                            L("已切换核心：") + options[selected].name);
                                        _requestRecentGamesRefresh(false);
                                    }
                                },
                                beiklive::GetCoreSelectionIndex(platform, core));
                            brls::Application::pushActivity(
                                new brls::Activity(dropdown));
                        });
                });
        }

        m_gameOptionsSidebar->addSubmenuButton(
            operationsMenu, L("删除游戏"), beiklive::material::DELETE_ICON,
            [this, path, entry](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated([this, path, entry]() {
                    auto deleteGame = [this, path, entry](bool deleteRomFile) {
                            if (!beiklive::GameDB) {
                                brls::Application::notify(L("删除失败"));
                                return;
                            }
                            m_homeDeletePending = true;
                            ++m_recentRefreshGen;
                            if (switchLayout)
                                switchLayout->removeGameByPath(path);

                            auto alive = m_aliveToken;
                            ThreadPool::instance().enqueue([
                                this, alive, path, entry, deleteRomFile]() {
                                bool removedFile = true;
                                if (deleteRomFile)
                                    removedFile = deleteGameFilesForEntry(entry);

                                bool removedRecord = false;
                                if (removedFile && alive->load() && beiklive::GameDB) {
                                    if (beiklive::GameDB->getAll().size() <= 1) {
                                        beiklive::GameDB->clearAll();
                                        removedRecord = true;
                                    } else {
                                        removedRecord =
                                            beiklive::GameDB->removeByPath(path);
                                        if (removedRecord)
                                            beiklive::GameDB->flush();
                                    }
                                }
                                brls::Logger::info(
                                    "[Game Delete] database remove result: path={} removed={} "
                                    "delete_files={} files_removed={}",
                                    path, removedRecord, deleteRomFile, removedFile);
                                if (!alive->load())
                                    return;

                                brls::sync([
                                    this, alive, removedRecord, removedFile,
                                    deleteRomFile]() {
                                    if (!alive->load())
                                        return;
                                    if (!removedRecord) {
                                        m_homeDeletePending = false;
                                        if (switchLayout)
                                            switchLayout->cancelGameRemoval();
                                        brls::Application::notify(
                                            deleteRomFile && !removedFile
                                                ? L("游戏文件删除失败，记录已保留")
                                                : L("删除失败"));
                                        _requestRecentGamesRefresh(false);
                                        return;
                                    }

                                    auto finish = [this, alive, removedFile,
                                                   deleteRomFile]() {
                                        if (!alive->load())
                                            return;
                                        m_homeDeletePending = false;
                                        if (deleteRomFile)
                                            brls::Application::notify(removedFile
                                                ? L("已删除游戏")
                                                : L("已移除记录，游戏文件删除失败"));
                                        else
                                            brls::Application::notify(
                                                L("已从游戏库移除该游戏"));
                                        _requestRecentGamesRefresh(false);
                                    };
                                    if (switchLayout)
                                        switchLayout->completeGameRemoval(
                                            std::move(finish));
                                    else
                                        finish();
                                });
                            });
                    };
                    auto* dialog = new brls::Dialog(
                        L("请选择游戏的删除方式"));
                    dialog->addButton(L("仅从库中移除"),
                        [deleteGame]() { deleteGame(false); });
                    dialog->addButton(L("移除并删除文件"),
                        [deleteGame]() { deleteGame(true); });
                    dialog->addButton(L("取消"), []() {});
                    dialog->setCancelable(false);
                    dialog->open();
                });
            });

        m_gameOptionsSidebar->onClosed = [this]() {
            if (switchLayout)
                switchLayout->releaseSelectedCoverTexture();
            if (switchLayout)
                switchLayout->restoreCardFocus(false);
            this->getBottomBar()->setVisibility(brls::Visibility::GONE);
        };
        m_gameOptionsSidebar->onCloseRequested = [this]() {
            _closeGameOptionsPanelAnimated({});
        };

        this->addView(m_gameOptionsSidebar);
        m_gameOptionsSidebar->open(entry);
    }

    void StartPage::_hideGameOptionsPanel()
    {
        if (!m_gameOptionsSidebar)
            return;
        auto* stale = m_gameOptionsSidebar;
        m_gameOptionsSidebar = nullptr;
        if (switchLayout)
            switchLayout->releaseSelectedCoverTexture();
        stale->removeFromSuperView(true);
        this->getBottomBar()->setVisibility(brls::Visibility::GONE);
    }

    void StartPage::_closeGameOptionsPanelAnimated(
        std::function<void()> completion, bool launchTransition)
    {
        if (!m_gameOptionsSidebar) {
            if (completion)
                completion();
            return;
        }
        auto* sidebar = m_gameOptionsSidebar;
        auto alive = m_aliveToken;
        auto finishClose = [this, alive, sidebar,
                            completion = std::move(completion)]() mutable {
            brls::sync([this, alive, sidebar,
                        completion = std::move(completion)]() mutable {
                if (!alive->load())
                    return;
                if (m_gameOptionsSidebar == sidebar)
                    m_gameOptionsSidebar = nullptr;
                sidebar->removeFromSuperView(true);
                this->getBottomBar()->setVisibility(brls::Visibility::GONE);
                if (completion)
                    completion();
            });
        };
        if (launchTransition)
            sidebar->closeForLaunch(std::move(finishClose));
        else
            sidebar->close(std::move(finishClose));
    }
}
