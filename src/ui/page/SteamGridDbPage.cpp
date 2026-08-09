#include "ui/page/SteamGridDbPage.hpp"
#include "ui/page/CoverEditorPage.hpp"
#include "core/Translation.hpp"

#include "core/SteamGridDb.hpp"
#include "core/ThreadPool.hpp"
#include "core/Tools.hpp"
#include "core/common.h"
#include "ui/utils/GradientFocus.hpp"
#include "ui/utils/MaterialIcons.hpp"
#include "ui/widget/Box.hpp"

#include <borealis.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace beiklive
{
namespace
{
    constexpr float kPi = 3.14159265358979323846f;

    float clamp01(float value)
    {
        return std::max(0.f, std::min(1.f, value));
    }

    float easeOutBack(float value)
    {
        value = clamp01(value);
        constexpr float c1 = 1.35f;
        constexpr float c3 = c1 + 1.f;
        const float shifted = value - 1.f;
        return 1.f + c3 * shifted * shifted * shifted + c1 * shifted * shifted;
    }

    float smooth(float value)
    {
        value = clamp01(value);
        return value * value * (3.f - 2.f * value);
    }

    std::string encodeUtf8(char32_t codepoint)
    {
        std::string out;
        if (codepoint <= 0x7F) out.push_back(static_cast<char>(codepoint));
        else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        return out;
    }

    class SteamGridDbPreviewCanvas final : public brls::View
    {
    public:
        SteamGridDbPreviewCanvas(
            steamgriddb::Asset asset, GameEntry entry,
            std::function<void(const std::string&)> onCoverChanged,
            brls::View* overlayRoot, brls::View* returnFocus)
            : m_asset(std::move(asset))
            , m_entry(std::move(entry))
            , m_onCoverChanged(std::move(onCoverChanged))
            , m_overlayRoot(overlayRoot)
            , m_returnFocus(returnFocus)
        {
            setFocusable(true);
            setGrow(1.f);
            setWidthPercentage(100.f);
            HIDE_BRLS_HIGHLIGHT(this);
            setCustomNavigationRoute(brls::FocusDirection::UP, this);
            setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
            setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
            setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

            auto previous = [this](brls::View*) {
                if (!m_saving) _move(-1);
                return true;
            };
            auto next = [this](brls::View*) {
                if (!m_saving) _move(1);
                return true;
            };
            registerAction("", brls::BUTTON_NAV_UP, previous, true, true,
                           brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_LEFT, previous, true, true,
                           brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_DOWN, next, true, true,
                           brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_RIGHT, next, true, true,
                           brls::SOUND_NONE);
            registerAction(L("选择"), brls::BUTTON_A,
                [this](brls::View*) {
                    if (m_saving || m_closing) return true;
                    if (m_selected == 0) _save();
                    else _beginClose();
                    return true;
                }, false, false, brls::SOUND_NONE);
            registerAction(L("返回"), brls::BUTTON_B,
                [this](brls::View*) {
                    if (!m_saving) _beginClose();
                    return true;
                }, false, false, brls::SOUND_NONE);
            m_lastFrame = std::chrono::steady_clock::now();
        }

        ~SteamGridDbPreviewCanvas() override
        {
            m_alive->store(false);
            if (m_image > 0) {
                if (auto* vg = brls::Application::getNVGContext())
                    nvgDeleteImage(vg, m_image);
            }
        }

        void frame(brls::FrameContext* ctx) override
        {
            brls::View::frame(ctx);
            const auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - m_lastFrame).count();
            m_lastFrame = now;
            if (dt <= 0.f || dt > 0.25f) dt = 0.016f;
            m_time += dt;
            if (m_closing) {
                m_entrance = std::max(0.f, m_entrance - dt * 5.4f);
                if (m_entrance <= 0.f && !m_popQueued) {
                    m_popQueued = true;
                    auto* overlayRoot = m_overlayRoot;
                    auto* returnFocus = m_returnFocus;
                    brls::sync([overlayRoot, returnFocus]() {
                        if (returnFocus)
                            brls::Application::giveFocus(returnFocus);
                        if (overlayRoot)
                            overlayRoot->removeFromSuperView();
                    });
                }
            } else {
                m_entrance = std::min(1.f, m_entrance + dt * 4.8f);
            }
            m_press = std::max(0.f, m_press - dt * 7.f);
            invalidate();
        }

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style, brls::FrameContext*) override
        {
            if (!vg) return;
            _ensureFonts();
            const float eased = easeOutBack(m_entrance);
            const float alpha = smooth(m_entrance);
            nvgSave(vg);
            nvgGlobalAlpha(vg, alpha);

            // Keep the result grid visible below the preview while lowering
            // its visual priority with a genuinely translucent mask.
            nvgBeginPath(vg);
            nvgRect(vg, x, y, w, h);
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 158));
            nvgFill(vg);

            const float imageW = 470.f;
            const float imageH = 520.f;
            const float floatY = std::sin(m_time * 2.5f) * 7.f;
            const float imageX = x + w * 0.28f - imageW * 0.5f -
                (1.f - eased) * 80.f;
            const float imageY = y + (h - imageH) * 0.5f + floatY;
            _drawShadow(vg, imageX, imageY, imageW, imageH, 18.f);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, imageX, imageY, imageW, imageH, 18.f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 12));
            nvgFill(vg);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 105));
            nvgStrokeWidth(vg, 1.2f);
            nvgStroke(vg);
            if (m_image < 0 && !m_asset.localPath.empty())
                m_image = nvgCreateImage(vg, m_asset.localPath.c_str(), 0);
            if (m_image > 0) {
                int iw = 0, ih = 0;
                nvgImageSize(vg, m_image, &iw, &ih);
                const float fit = iw > 0 && ih > 0
                    ? std::min((imageW - 24.f) / iw, (imageH - 24.f) / ih)
                    : 1.f;
                const float drawW = iw * fit;
                const float drawH = ih * fit;
                const float drawX = imageX + (imageW - drawW) * 0.5f;
                const float drawY = imageY + (imageH - drawH) * 0.5f;
                nvgBeginPath(vg);
                nvgRoundedRect(vg, drawX, drawY, drawW, drawH, 11.f);
                nvgFillPaint(vg, nvgImagePattern(
                    vg, drawX, drawY, drawW, drawH, 0.f, m_image, 1.f));
                nvgFill(vg);
            }

            const float menuX = x + w * 0.56f + (1.f - eased) * 100.f;
            const float menuY = y + h * 0.5f - 84.f;
            static const std::string labels[] = {L("设为封面"), L("返回")};
            static const char32_t icons[] = {
                beiklive::material::IMAGE, 0xE5C4};
            for (int i = 0; i < 2; ++i) {
                const float buttonY = menuY + i * 92.f;
                const bool focused = i == m_selected;
                const float scale = focused ? 1.f - m_press * 0.035f : 1.f;
                nvgSave(vg);
                nvgTranslate(vg, menuX + 180.f, buttonY + 34.f);
                nvgScale(vg, scale, scale);
                nvgTranslate(vg, -(menuX + 180.f), -(buttonY + 34.f));
                _drawShadow(vg, menuX, buttonY, 360.f, 68.f, 12.f);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, menuX, buttonY, 360.f, 68.f, 12.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, focused ? 40 : 10));
                nvgFill(vg);
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255,
                    focused ? 120 : 48));
                nvgStrokeWidth(vg, 1.f);
                nvgStroke(vg);
                if (focused) beiklive::ui::drawGradientFocusBorder(
                    vg, menuX, buttonY, 360.f, 68.f, 12.f, 3.f, 1.f,
                    beiklive::ui::gradientFocusAnimationOffset(m_time));
                nvgFontFaceId(vg, m_materialFont);
                nvgFontSize(vg, 30.f);
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(105, 198, 255, 245));
                const std::string icon = encodeUtf8(icons[i]);
                nvgText(vg, menuX + 28.f, buttonY + 34.f,
                        icon.c_str(), nullptr);
                nvgFontFaceId(vg, m_defaultFont);
                nvgFontSize(vg, 24.f);
                nvgFillColor(vg, nvgRGBA(245, 247, 252, 245));
                nvgText(vg, menuX + 78.f, buttonY + 34.f,
                        labels[i].c_str(), nullptr);
                nvgRestore(vg);
            }

            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 18.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(232, 237, 246, 225));
            nvgText(vg, menuX, menuY - 34.f,
                    m_status.c_str(), nullptr);
            _drawHints(vg, x, y, w, h, alpha);
            nvgRestore(vg);
        }

    private:
        steamgriddb::Asset m_asset;
        GameEntry m_entry;
        std::function<void(const std::string&)> m_onCoverChanged;
        brls::View* m_overlayRoot = nullptr;
        brls::View* m_returnFocus = nullptr;
        std::shared_ptr<std::atomic<bool>> m_alive =
            std::make_shared<std::atomic<bool>>(true);
        int m_selected = 0;
        int m_image = -1;
        int m_defaultFont = -1;
        int m_materialFont = -1;
        int m_switchFont = -1;
        float m_entrance = 0.f;
        float m_time = 0.f;
        float m_press = 0.f;
        bool m_saving = false;
        bool m_closing = false;
        bool m_popQueued = false;
        std::string m_status = L("选择图片操作");
        std::chrono::steady_clock::time_point m_lastFrame;

        void _ensureFonts()
        {
            if (m_defaultFont < 0)
                m_defaultFont = brls::Application::getDefaultFont();
            if (m_materialFont < 0)
                m_materialFont = brls::Application::getFont(
                    brls::FONT_MATERIAL_ICONS);
            if (m_switchFont < 0)
                m_switchFont = brls::Application::getFont(
                    brls::FONT_SWITCH_ICONS);
        }

        void _drawShadow(NVGcontext* vg, float x, float y, float w, float h,
                         float radius)
        {
            const NVGpaint shadow = nvgBoxGradient(
                vg, x + 4.f, y + 5.f, w, h, radius, 5.f,
                nvgRGBA(0, 0, 0, 82), nvgRGBA(0, 0, 0, 0));
            nvgBeginPath(vg);
            nvgRect(vg, x - 2.f, y - 2.f, w + 11.f, h + 12.f);
            nvgRoundedRect(vg, x, y, w, h, radius);
            nvgPathWinding(vg, NVG_HOLE);
            nvgFillPaint(vg, shadow);
            nvgFill(vg);
        }

        void _move(int direction)
        {
            m_selected = (m_selected + (direction < 0 ? -1 : 1) + 2) % 2;
            m_press = 0.f;
            brls::Application::getAudioPlayer()->play(
                brls::SOUND_FOCUS_CHANGE);
        }

        void _beginClose()
        {
            if (m_closing) return;
            m_closing = true;
            brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
        }

        void _save()
        {
            m_press = 1.f;
            m_saving = true;
            m_status = L("正在准备封面编辑...");
            auto alive = m_alive;
            auto asset = m_asset;
            ThreadPool::instance().enqueuePriority(
                [this, alive, asset = std::move(asset)]() mutable {
                    std::string error;
                    const bool ok = steamgriddb::ensureAssetCached(
                        asset, false, &error);
                    const std::string sourcePath = asset.localPath;
                    brls::sync([this, alive, ok, sourcePath,
                                error = std::move(error)]() {
                        if (!alive->load()) return;
                        m_saving = false;
                        if (!ok) {
                            m_status = error.empty()
                                ? L("封面保存失败") : error;
                            return;
                        }
                        openCoverEditorPage(m_entry, sourcePath, m_onCoverChanged);
                        _beginClose();
                    });
                });
        }

        void _drawSwitchGlyph(NVGcontext* vg,
                              brls::ControllerButton button,
                              float x, float y)
        {
            const std::string glyph = brls::Hint::getKeyIcon(button);
            nvgFontFaceId(vg, m_switchFont);
            nvgFontSize(vg, 27.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 242));
            nvgText(vg, x, y, glyph.c_str(), nullptr);
        }

        void _drawHints(NVGcontext* vg, float x, float y, float w, float h,
                        float alpha)
        {
            nvgGlobalAlpha(vg, alpha);
            const float hintY = y + h - 28.f;
            float cursor = x + w - 28.f;
            auto hint = [&](brls::ControllerButton button, const char* label) {
                nvgFontFaceId(vg, m_defaultFont);
                nvgFontSize(vg, 17.f);
                float bounds[4]{};
                nvgTextBounds(vg, 0.f, 0.f, label, nullptr, bounds);
                cursor -= bounds[2] - bounds[0] + 43.f;
                _drawSwitchGlyph(vg, button, cursor + 12.f, hintY);
                nvgFontFaceId(vg, m_defaultFont);
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(236, 240, 247, 230));
                nvgText(vg, cursor + 29.f, hintY, label, nullptr);
                cursor -= 13.f;
            };
            hint(brls::BUTTON_B, L("返回").c_str());
            hint(brls::BUTTON_A, L("选择").c_str());
        }
    };

    void openSteamGridDbPreviewPage(
        const steamgriddb::Asset& asset, const GameEntry& entry,
        std::function<void(const std::string&)> onCoverChanged,
        brls::Box* overlayHost, brls::View* returnFocus)
    {
        if (!overlayHost) return;
        auto* page = new beiklive::Box(brls::Axis::COLUMN);
        page->showHeader(false);
        page->showFooter(false);
        page->showBackground(false);
        page->showShader(false);
        page->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
        page->setPositionType(brls::PositionType::ABSOLUTE);
        page->setPositionTop(0.f);
        page->setPositionLeft(0.f);
        page->setWidthPercentage(100.f);
        page->setHeightPercentage(100.f);
        auto* canvas = new SteamGridDbPreviewCanvas(
            asset, entry, std::move(onCoverChanged), page, returnFocus);
        page->getContentBox()->addView(canvas);
        overlayHost->addView(page);
        brls::Application::giveFocus(canvas);
    }

    class SteamGridDbCanvas final : public brls::View
    {
    public:
        SteamGridDbCanvas(GameEntry entry,
                          std::function<void(const std::string&)> onCoverChanged,
                          brls::Box* overlayHost)
            : m_entry(std::move(entry))
            , m_onCoverChanged(std::move(onCoverChanged))
            , m_overlayHost(overlayHost)
        {
            setFocusable(true);
            setGrow(1.f);
            setWidthPercentage(100.f);
            HIDE_BRLS_HIGHLIGHT(this);
            setCustomNavigationRoute(brls::FocusDirection::UP, this);
            setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
            setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
            setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

            auto up = [this](brls::View*) { _moveVertical(-1); return true; };
            auto down = [this](brls::View*) { _moveVertical(1); return true; };
            auto left = [this](brls::View*) { _moveHorizontal(-1); return true; };
            auto right = [this](brls::View*) { _moveHorizontal(1); return true; };
            registerAction("", brls::BUTTON_NAV_UP, up, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_DOWN, down, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_LEFT, left, true, true, brls::SOUND_NONE);
            registerAction("", brls::BUTTON_NAV_RIGHT, right, true, true, brls::SOUND_NONE);
            registerAction(L("选择"), brls::BUTTON_A,
                [this](brls::View*) { _activate(); return true; },
                false, false, brls::SOUND_NONE);
            registerAction(L("返回"), brls::BUTTON_B,
                [this](brls::View*) { _back(); return true; },
                false, false, brls::SOUND_NONE);
            registerAction(L("上一类"), brls::BUTTON_LB,
                [this](brls::View*) { _switchType(-1); return true; },
                false, false, brls::SOUND_NONE);
            registerAction(L("下一类"), brls::BUTTON_RB,
                [this](brls::View*) { _switchType(1); return true; },
                false, false, brls::SOUND_NONE);
            registerAction(L("游戏名称搜索"), brls::BUTTON_RT,
                [this](brls::View*) { _searchCurrentGame(); return true; },
                false, false, brls::SOUND_NONE);
            registerAction(L("手动输入"), brls::BUTTON_LT,
                [this](brls::View*) { _manualSearch(); return true; },
                false, false, brls::SOUND_NONE);
            registerAction(L("清空搜索"), brls::BUTTON_Y,
                [this](brls::View*) { _clearSearch(); return true; },
                false, false, brls::SOUND_NONE);
            registerAction(L("过滤"), brls::BUTTON_X,
                [this](brls::View*) { _toggleFilter(); return true; },
                false, false, brls::SOUND_NONE);
            m_lastFrame = std::chrono::steady_clock::now();
            brls::sync([this]() { brls::Application::giveFocus(this); });
        }

        ~SteamGridDbCanvas() override
        {
            m_alive->store(false);
            NVGcontext* vg = brls::Application::getNVGContext();
            if (vg) {
                for (const auto& image : m_images)
                    if (image.second > 0) nvgDeleteImage(vg, image.second);
            }
        }

        void frame(brls::FrameContext* ctx) override
        {
            brls::View::frame(ctx);
            const auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - m_lastFrame).count();
            m_lastFrame = now;
            if (dt <= 0.f || dt > 0.25f) dt = 0.016f;
            m_time += dt;
            if (m_closing) {
                m_entrance = std::max(0.f, m_entrance - dt * 4.8f);
                if (m_entrance <= 0.f && !m_popQueued) {
                    m_popQueued = true;
                    brls::sync([]() {
                        brls::Application::popActivity(brls::TransitionAnimation::NONE);
                    });
                }
            } else {
                m_entrance = std::min(1.f, m_entrance + dt * 4.2f);
            }
            const float overlayTarget = m_mode == Mode::Results ? 0.f : 1.f;
            m_overlayProgress += (overlayTarget - m_overlayProgress) *
                std::min(1.f, dt * 13.f);
            m_tabMotion = std::min(1.f, m_tabMotion + dt * 8.f);
            m_pressMotion = std::max(0.f, m_pressMotion - dt * 6.f);
            m_scrollY += (m_targetScrollY - m_scrollY) *
                std::min(1.f, dt * 12.f);
            invalidate();
        }

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style, brls::FrameContext*) override
        {
            if (!vg) return;
            _ensureFonts();
            const float page = easeOutBack(m_entrance);
            const float alpha = smooth(m_entrance);
            nvgSave(vg);
            nvgGlobalAlpha(vg, alpha);
            _drawHeader(vg, x, y - (1.f - page) * 72.f, w);
            _drawResults(vg, x, y, w, h, page);
            _drawFooter(vg, x, y, w, h, alpha);
            nvgRestore(vg);
            if (m_overlayProgress > 0.002f) {
                if (m_mode == Mode::Filter) _drawFilter(vg, x, y, w, h);
            }
        }

    private:
        enum class Mode { Results, Filter };
        enum class LoadState { Idle, Searching, Ready, Error, Saving };

        GameEntry m_entry;
        std::function<void(const std::string&)> m_onCoverChanged;
        brls::Box* m_overlayHost = nullptr;
        steamgriddb::AssetGroups m_rawGroups;
        std::array<std::vector<steamgriddb::Asset>,
            static_cast<size_t>(steamgriddb::AssetType::Count)> m_filteredGroups;
        std::unordered_map<std::string, std::string> m_cachedPaths;
        std::unordered_map<std::string, int> m_images;
        std::unordered_set<std::string> m_pendingImages;
        steamgriddb::Filters m_filters;
        std::shared_ptr<std::atomic<bool>> m_alive =
            std::make_shared<std::atomic<bool>>(true);
        std::atomic<std::uint64_t> m_generation{0};
        steamgriddb::AssetType m_type = steamgriddb::AssetType::Grids;
        Mode m_mode = Mode::Results;
        LoadState m_loadState = LoadState::Idle;
        int m_selected = 0;
        std::array<int, static_cast<size_t>(steamgriddb::AssetType::Count)>
            m_visibleCounts{{10, 10, 10, 10}};
        int m_filterRow = 0;
        float m_entrance = 0.f;
        float m_overlayProgress = 0.f;
        float m_tabMotion = 1.f;
        float m_time = 0.f;
        float m_pressMotion = 0.f;
        float m_scrollY = 0.f;
        float m_targetScrollY = 0.f;
        bool m_closing = false;
        bool m_popQueued = false;
        std::string m_queryLabel;
        std::string m_status = L("按 ZR 使用当前游戏名搜索，或按 ZL 手动输入");
        int m_defaultFont = -1;
        int m_materialFont = -1;
        int m_switchFont = -1;
        std::chrono::steady_clock::time_point m_lastFrame;

        std::vector<steamgriddb::Asset>& _current()
        {
            return m_filteredGroups[static_cast<size_t>(m_type)];
        }

        int _visibleCount() const
        {
            const auto index = static_cast<size_t>(m_type);
            return std::min(m_visibleCounts[index],
                static_cast<int>(m_filteredGroups[index].size()));
        }

        void _resetGridState(bool resetAllTypes)
        {
            if (resetAllTypes) m_visibleCounts.fill(10);
            m_selected = 0;
            m_scrollY = 0.f;
            m_targetScrollY = 0.f;
        }

        void _updateTargetScroll()
        {
            constexpr int columns = 5;
            constexpr float rowPitch = 240.f;
            const int shown = _visibleCount();
            const bool hasMore = shown < static_cast<int>(
                m_filteredGroups[static_cast<size_t>(m_type)].size());
            const int total = shown + (hasMore ? 1 : 0);
            if (total <= 0) {
                m_targetScrollY = 0.f;
                return;
            }
            const int selectedRow = std::clamp(m_selected, 0, total - 1) /
                columns;
            const int rows = (total + columns - 1) / columns;
            const float maximum = std::max(0, rows - 2) * rowPitch;
            const float desired = selectedRow <= 1
                ? 0.f : (selectedRow - 1) * rowPitch;
            m_targetScrollY = std::clamp(desired, 0.f, maximum);
        }

        std::string _resultSummary() const
        {
            return "GRIDS " + std::to_string(m_filteredGroups[0].size()) +
                "  ·  HEROS " + std::to_string(m_filteredGroups[1].size()) +
                "  ·  LOGOS " + std::to_string(m_filteredGroups[2].size()) +
                "  ·  ICONS " + std::to_string(m_filteredGroups[3].size());
        }

        void _ensureFonts()
        {
            if (m_defaultFont < 0) m_defaultFont = brls::Application::getDefaultFont();
            if (m_materialFont < 0) m_materialFont =
                brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
            if (m_switchFont < 0) m_switchFont =
                brls::Application::getFont(brls::FONT_SWITCH_ICONS);
        }

        void _searchCurrentGame()
        {
            if (m_mode != Mode::Results || m_loadState == LoadState::Searching ||
                m_loadState == LoadState::Saving)
                return;
            std::vector<std::string> terms;
            if (!m_entry.title.empty()) terms.push_back(m_entry.title);
            const std::string filename =
                beiklive::tools::getFileNameWithoutExtension(m_entry.path);
            if (!filename.empty() && filename != m_entry.title)
                terms.push_back(filename);
            _beginSearch(std::move(terms), m_entry.title.empty() ? filename : m_entry.title);
        }

        void _manualSearch()
        {
            if (m_mode != Mode::Results || m_loadState == LoadState::Searching ||
                m_loadState == LoadState::Saving)
                return;
            auto* ime = brls::Application::getPlatform()->getImeManager();
            if (!ime) return;
            ime->openForText([this, alive = m_alive](std::string text) {
                if (!alive->load() || text.empty()) return;
                _beginSearch({text}, text);
            }, L("搜索 SteamGridDB"), L("输入游戏名称"), 128, m_queryLabel,
                brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
        }

        void _beginSearch(std::vector<std::string> terms, std::string label)
        {
            if (terms.empty()) return;
            const std::uint64_t generation = ++m_generation;
            m_queryLabel = std::move(label);
            m_loadState = LoadState::Searching;
            m_status = L("正在搜索游戏并读取素材列表...");
            _resetGridState(true);
            m_rawGroups = {};
            m_filteredGroups = {};
            m_pendingImages.clear();
            auto alive = m_alive;
            ThreadPool::instance().enqueuePriority([
                this, alive, generation, terms = std::move(terms)]() mutable {
                auto games = steamgriddb::searchGames(terms, alive.get());
                if (!games.ok) {
                    brls::sync([this, alive, generation, error = games.error]() {
                        if (!alive->load() || generation != m_generation.load()) return;
                        m_loadState = LoadState::Error;
                        m_status = error;
                    });
                    return;
                }
                auto assets = steamgriddb::fetchAllAssets(
                    games.value, 0, alive.get());
                brls::sync([this, alive, generation, assets = std::move(assets)]() mutable {
                    if (!alive->load() || generation != m_generation.load()) return;
                    if (!assets.ok) {
                        m_loadState = LoadState::Error;
                        m_status = assets.error;
                        return;
                    }
                    m_rawGroups = std::move(assets.value);
                    _applyFilters();
                    if (_current().empty()) {
                        for (int index = 0;
                             index < static_cast<int>(steamgriddb::AssetType::Count);
                             ++index) {
                            if (!m_filteredGroups[static_cast<size_t>(index)].empty()) {
                                m_type = static_cast<steamgriddb::AssetType>(index);
                                m_tabMotion = 0.f;
                                break;
                            }
                        }
                    }
                    m_loadState = LoadState::Ready;
                    m_status = L("搜索完成 · ") + _resultSummary();
                    brls::Logger::info("[SteamGridDB UI] {}", m_status);
                    _requestVisibleImages();
                });
            });
        }

        void _applyFilters()
        {
            for (size_t i = 0; i < m_rawGroups.size(); ++i) {
                for (auto& asset : m_rawGroups[i]) {
                    auto cached = m_cachedPaths.find(asset.url);
                    if (cached != m_cachedPaths.end()) asset.localPath = cached->second;
                }
                m_filteredGroups[i] = steamgriddb::applyFilters(
                    m_rawGroups[i], m_filters);
            }
            _resetGridState(true);
        }

        void _requestVisibleImages()
        {
            auto& items = _current();
            const int count = _visibleCount();
            const std::uint64_t generation = m_generation.load();
            for (int i = 0; i < count; ++i) {
                const int index = i;
                if (!items[static_cast<size_t>(index)].localPath.empty()) continue;
                steamgriddb::Asset asset = items[static_cast<size_t>(index)];
                if (!m_pendingImages.insert(asset.url).second) continue;
                auto alive = m_alive;
                ThreadPool::instance().enqueue([this, alive, generation,
                                                asset = std::move(asset)]() mutable {
                    std::string error;
                    const bool loaded =
                        steamgriddb::ensureAssetCached(
                            asset, true, &error, alive.get());
                    brls::sync([this, alive, generation, loaded,
                                error = std::move(error),
                                asset = std::move(asset)]() mutable {
                        if (!alive->load() || generation != m_generation.load()) return;
                        m_pendingImages.erase(asset.url);
                        if (!loaded) {
                            m_status = L("图片加载失败 · ") +
                                (error.empty() ? std::string(L("请检查网络")) : error);
                            brls::Logger::warning(
                                "[SteamGridDB UI] asset image failed: {} ({})",
                                asset.url, error);
                            invalidate();
                            return;
                        }
                        m_cachedPaths[asset.url] = asset.localPath;
                        for (auto& group : m_rawGroups)
                            for (auto& item : group)
                                if (item.url == asset.url) item.localPath = asset.localPath;
                        for (auto& group : m_filteredGroups)
                            for (auto& item : group)
                                if (item.url == asset.url) item.localPath = asset.localPath;
                        invalidate();
                    });
                });
            }
        }

        void _clearSearch()
        {
            if (m_mode != Mode::Results || m_loadState == LoadState::Saving) return;
            ++m_generation;
            m_rawGroups = {};
            m_filteredGroups = {};
            m_pendingImages.clear();
            m_queryLabel.clear();
            _resetGridState(true);
            m_loadState = LoadState::Idle;
            m_status = L("搜索已清空");
            brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
        }

        void _switchType(int direction)
        {
            if (m_mode != Mode::Results || m_loadState == LoadState::Saving) return;
            const int count = static_cast<int>(steamgriddb::AssetType::Count);
            int index = static_cast<int>(m_type);
            index = (index + (direction < 0 ? -1 : 1) + count) % count;
            m_type = static_cast<steamgriddb::AssetType>(index);
            _resetGridState(false);
            m_tabMotion = 0.f;
            _requestVisibleImages();
            brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        }

        void _toggleFilter()
        {
            if (m_loadState == LoadState::Searching || m_loadState == LoadState::Saving)
                return;
            m_mode = m_mode == Mode::Filter ? Mode::Results : Mode::Filter;
            m_filterRow = 0;
            brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
        }

        void _moveVertical(int direction)
        {
            if (m_closing || m_loadState == LoadState::Searching ||
                m_loadState == LoadState::Saving)
                return;
            if (m_mode == Mode::Filter) {
                m_filterRow = (m_filterRow + (direction < 0 ? -1 : 1) + 6) % 6;
            } else {
                constexpr int columns = 5;
                const int shown = _visibleCount();
                const bool hasMore = shown < static_cast<int>(_current().size());
                const int total = shown + (hasMore ? 1 : 0);
                if (total == 0) return;
                const int rows = (total + columns - 1) / columns;
                const int column = m_selected % columns;
                const int row = m_selected / columns;
                const int nextRow = (row + (direction < 0 ? -1 : 1) + rows) % rows;
                m_selected = std::min(total - 1, nextRow * columns + column);
                _updateTargetScroll();
            }
            brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        }

        void _moveHorizontal(int direction)
        {
            if (m_closing || m_loadState == LoadState::Searching ||
                m_loadState == LoadState::Saving)
                return;
            if (m_mode == Mode::Filter) {
                _adjustFilter(direction);
            } else {
                const int shown = _visibleCount();
                const bool hasMore = shown < static_cast<int>(_current().size());
                const int total = shown + (hasMore ? 1 : 0);
                if (total == 0) return;
                m_selected = (m_selected + (direction < 0 ? -1 : 1) + total) % total;
                _updateTargetScroll();
            }
            brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        }

        template <typename T>
        void _cycleValue(T& value, const std::vector<T>& values, int direction)
        {
            auto found = std::find(values.begin(), values.end(), value);
            int index = found == values.end() ? 0 :
                static_cast<int>(std::distance(values.begin(), found));
            index = (index + (direction < 0 ? -1 : 1) +
                     static_cast<int>(values.size())) % static_cast<int>(values.size());
            value = values[static_cast<size_t>(index)];
        }

        void _adjustFilter(int direction)
        {
            static const std::vector<int> widths = {0, 300, 400, 600, 920, 1920};
            static const std::vector<int> heights = {0, 450, 600, 900, 430, 620};
            static const std::vector<std::string> styles = {
                "", "official", "alternate", "blurred", "white_logo"};
            static const std::vector<std::string> mimes = {
                "", "image/png", "image/jpeg"};
            static const std::vector<std::string> languages = {
                "", "zh", "ja", "en"};
            switch (m_filterRow) {
                case 0: _cycleValue(m_filters.width, widths, direction); break;
                case 1: _cycleValue(m_filters.height, heights, direction); break;
                case 2: _cycleValue(m_filters.style, styles, direction); break;
                case 3: _cycleValue(m_filters.mime, mimes, direction); break;
                case 4: _cycleValue(m_filters.language, languages, direction); break;
                case 5: m_filters.allowHumor = !m_filters.allowHumor; break;
            }
            _applyFilters();
            _requestVisibleImages();
        }

        void _activate()
        {
            if (m_closing || m_loadState == LoadState::Searching ||
                m_loadState == LoadState::Saving)
                return;
            m_pressMotion = 1.f;
            if (m_mode == Mode::Filter) {
                m_mode = Mode::Results;
                _applyFilters();
                _requestVisibleImages();
            } else {
                auto& items = _current();
                const int shown = _visibleCount();
                if (m_selected == shown && shown < static_cast<int>(items.size())) {
                    auto& visible = m_visibleCounts[static_cast<size_t>(m_type)];
                    visible = std::min(shown + 10, static_cast<int>(items.size()));
                    // The former "show more" cell becomes the first appended
                    // image, keeping navigation continuous in both directions.
                    m_selected = shown;
                    _updateTargetScroll();
                    _requestVisibleImages();
                } else if (m_selected >= 0 && m_selected < shown) {
                    const int index = m_selected;
                    if (items[static_cast<size_t>(index)].localPath.empty()) {
                        m_status = L("图片仍在加载，请稍候");
                        _requestVisibleImages();
                        return;
                    }
                    const auto asset = items[static_cast<size_t>(index)];
                    auto alive = m_alive;
                    openSteamGridDbPreviewPage(
                        asset, m_entry,
                        [this, alive](const std::string& output) {
                            if (!alive->load()) return;
                            m_entry.logoPath = output;
                            m_status = L("封面已保存并压缩至最大 512 像素");
                            if (m_onCoverChanged) m_onCoverChanged(output);
                        }, m_overlayHost, this);
                }
            }
            brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
        }

        void _back()
        {
            if (m_loadState == LoadState::Saving) return;
            if (m_mode != Mode::Results) {
                m_mode = Mode::Results;
                brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
                return;
            }
            if (!m_closing) {
                m_closing = true;
                m_alive->store(false, std::memory_order_relaxed);
                ++m_generation;
                brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
            }
        }

        int _imageHandle(NVGcontext* vg, const std::string& path)
        {
            if (path.empty()) return -1;
            auto found = m_images.find(path);
            if (found != m_images.end()) return found->second;
            const int handle = nvgCreateImage(vg, path.c_str(), 0);
            m_images[path] = handle;
            return handle;
        }

        void _drawHeader(NVGcontext* vg, float x, float y, float w)
        {
            nvgFontFaceId(vg, m_materialFont);
            nvgFontSize(vg, 34.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(90, 190, 255, 245));
            const std::string icon = encodeUtf8(beiklive::material::IMAGE);
            nvgText(vg, x + 38.f, y + 49.f, icon.c_str(), nullptr);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 27.f);
            nvgFillColor(vg, nvgRGBA(245, 247, 252, 250));
            nvgText(vg, x + 82.f, y + 49.f, L("SteamGridDB 封面数据库").c_str(), nullptr);

            constexpr float spacing = 112.f;
            const float centerX = x + w - 300.f;
            const float firstX = centerX - spacing * 1.5f;
            const int current = static_cast<int>(m_type);
            const float pop = std::sin((1.f - smooth(m_tabMotion)) * kPi) * 7.f;
            for (int index = 0; index < 4; ++index) {
                const bool selected = index == current;
                const float px = firstX + index * spacing;
                const float prominence = selected ? 1.f : 0.42f;
                if (selected) {
                    nvgBeginPath(vg);
                    nvgRoundedRect(vg, px - 53.f, y + 28.f - pop,
                                   106.f, 43.f, 21.f);
                    nvgFillColor(vg, nvgRGBA(255, 255, 255, 25));
                    nvgFill(vg);
                    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 92));
                    nvgStrokeWidth(vg, 1.f);
                    nvgStroke(vg);
                }
                nvgFontFaceId(vg, m_defaultFont);
                nvgFontSize(vg, selected ? 19.f : 15.f);
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(245, 247, 252,
                    static_cast<unsigned char>(245.f * prominence)));
                nvgText(vg, px, y + 49.f - (selected ? pop : 0.f),
                    steamgriddb::assetTypeName(static_cast<steamgriddb::AssetType>(index)),
                    nullptr);
            }
            _drawSwitchGlyph(vg, brls::BUTTON_LB, firstX - 68.f, y + 49.f, 0.9f);
            _drawSwitchGlyph(vg, brls::BUTTON_RB,
                             firstX + spacing * 3.f + 68.f, y + 49.f, 0.9f);
            nvgBeginPath(vg);
            nvgMoveTo(vg, x + 36.f, y + 92.f);
            nvgLineTo(vg, x + w - 36.f, y + 92.f);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 42));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);
        }

        void _drawResults(NVGcontext* vg, float x, float y, float w, float h,
                          float page)
        {
            const float contentY = y + 113.f + (1.f - page) * 24.f;
            const float statusY = y + h - 76.f;
            if (m_loadState == LoadState::Idle || m_loadState == LoadState::Searching ||
                m_loadState == LoadState::Error || _current().empty()) {
                const char32_t statusIcon = m_loadState == LoadState::Searching
                    ? 0xE863 : (m_loadState == LoadState::Error ? 0xE000 : 0xE8B6);
                const float pulse = 0.88f + std::sin(m_time * 4.f) * 0.10f;
                nvgFontFaceId(vg, m_materialFont);
                nvgFontSize(vg, 72.f);
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(100, 193, 255,
                    static_cast<unsigned char>(235.f * pulse)));
                const std::string glyph = encodeUtf8(statusIcon);
                const float iconX = x + w * 0.5f;
                const float iconY = y + h * 0.43f;
                nvgSave(vg);
                nvgTranslate(vg, iconX, iconY);
                if (m_loadState == LoadState::Searching) {
                    // Match the image-card loading indicator: one clean,
                    // continuously rotating Material loading glyph.
                    nvgRotate(vg, m_time * 4.4f);
                    nvgText(vg, 0.f, 0.f, glyph.c_str(), nullptr);
                } else {
                    nvgText(vg, 0.f, 0.f, glyph.c_str(), nullptr);
                }
                nvgRestore(vg);
                nvgFontFaceId(vg, m_defaultFont);
                nvgFontSize(vg, 22.f);
                nvgFillColor(vg, nvgRGBA(225, 230, 240, 225));
                nvgText(vg, x + w * 0.5f, y + h * 0.56f, m_status.c_str(), nullptr);
            } else {
                const auto& items = _current();
                const int itemCount = _visibleCount();
                const bool hasMore = itemCount < static_cast<int>(items.size());
                constexpr int columns = 5;
                const float gapX = 18.f;
                const float gapY = 16.f;
                const float gridX = x + 40.f;
                const float gridW = w - 80.f;
                const float cellW = (gridW - gapX * (columns - 1)) / columns;
                const float cellH = 224.f;
                const float viewportBottom = statusY - 22.f;
                nvgSave(vg);
                nvgIntersectScissor(vg, x, contentY - 4.f, w,
                                    viewportBottom - contentY + 8.f);
                for (int i = 0; i < itemCount; ++i) {
                    const int column = i % columns;
                    const int row = i / columns;
                    const float itemX = gridX + column * (cellW + gapX);
                    const float itemY = contentY + row * (cellH + gapY) -
                        m_scrollY;
                    _drawAsset(vg, items[static_cast<size_t>(i)], i,
                               itemX, itemY, cellW, cellH);
                }
                if (hasMore) {
                    const int i = itemCount;
                    const int column = i % columns;
                    const int row = i / columns;
                    const float itemX = gridX + column * (cellW + gapX);
                    const float itemY = contentY + row * (cellH + gapY) -
                        m_scrollY;
                    const bool focused = m_selected == i;
                    const NVGpaint shadow = nvgBoxGradient(
                        vg, itemX + 4.f, itemY + 5.f, cellW, cellH,
                        13.f, 5.f, nvgRGBA(0, 0, 0, 82),
                        nvgRGBA(0, 0, 0, 0));
                    nvgBeginPath(vg);
                    nvgRect(vg, itemX - 2.f, itemY - 2.f,
                            cellW + 11.f, cellH + 12.f);
                    nvgRoundedRect(vg, itemX, itemY, cellW, cellH, 13.f);
                    nvgPathWinding(vg, NVG_HOLE);
                    nvgFillPaint(vg, shadow);
                    nvgFill(vg);
                    nvgBeginPath(vg);
                    nvgRoundedRect(vg, itemX, itemY, cellW, cellH, 13.f);
                    nvgFillColor(vg, nvgRGBA(255, 255, 255,
                        focused ? 34 : 10));
                    nvgFill(vg);
                    nvgStrokeColor(vg, nvgRGBA(255, 255, 255,
                        focused ? 116 : 44));
                    nvgStrokeWidth(vg, 1.f);
                    nvgStroke(vg);
                    if (focused) beiklive::ui::drawGradientFocusBorder(
                        vg, itemX, itemY, cellW, cellH, 13.f, 3.f, 1.f,
                        beiklive::ui::gradientFocusAnimationOffset(m_time));
                    nvgFontFaceId(vg, m_materialFont);
                    nvgFontSize(vg, 48.f);
                    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                    nvgFillColor(vg, nvgRGBA(105, 197, 255, 240));
                    const std::string more = encodeUtf8(0xE145);
                    nvgText(vg, itemX + cellW * 0.5f,
                            itemY + cellH * 0.40f,
                            more.c_str(), nullptr);
                    nvgFontFaceId(vg, m_defaultFont);
                    nvgFontSize(vg, 20.f);
                    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                    nvgFillColor(vg, nvgRGBA(240, 243, 248, 235));
                    nvgText(vg, itemX + cellW * 0.5f,
                            itemY + cellH * 0.64f,
                            L("显示更多").c_str(), nullptr);
                    nvgFontSize(vg, 14.f);
                    nvgFillColor(vg, nvgRGBA(190, 198, 212, 190));
                    const int remaining = static_cast<int>(items.size()) -
                        itemCount;
                    const std::string moreLabel = L("继续追加 ") +
                        std::to_string(std::min(10, remaining)) + L(" 张");
                    nvgText(vg, itemX + cellW * 0.5f,
                            itemY + cellH * 0.76f,
                            moreLabel.c_str(), nullptr);
                }
                nvgRestore(vg);
            }
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 16.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(192, 201, 216, 190));
            const std::string query = m_queryLabel.empty()
                ? m_status : L("搜索：") + m_queryLabel + "  ·  " + m_status;
            nvgText(vg, x + 40.f, statusY, query.c_str(), nullptr);
        }

        void _drawAsset(NVGcontext* vg, const steamgriddb::Asset& asset,
                        int index, float x, float y, float w, float h)
        {
            const bool focused = m_selected == index;
            const float pressed = focused ? m_pressMotion : 0.f;
            const float scale = 1.f - pressed * 0.035f;
            nvgSave(vg);
            nvgTranslate(vg, x + w * 0.5f, y + h * 0.5f);
            nvgScale(vg, scale, scale);
            nvgTranslate(vg, -(x + w * 0.5f), -(y + h * 0.5f));
            const NVGpaint shadow = nvgBoxGradient(
                vg, x + 4.f, y + 5.f, w, h, 13.f, 5.f,
                nvgRGBA(0, 0, 0, 85), nvgRGBA(0, 0, 0, 0));
            nvgBeginPath(vg);
            nvgRect(vg, x - 2.f, y - 2.f, w + 11.f, h + 12.f);
            nvgRoundedRect(vg, x, y, w, h, 13.f);
            nvgPathWinding(vg, NVG_HOLE);
            nvgFillPaint(vg, shadow);
            nvgFill(vg);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, w, h, 13.f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, focused ? 26 : 9));
            nvgFill(vg);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, focused ? 116 : 45));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);
            const float imageX = x + 8.f;
            const float imageY = y + 8.f;
            const float imageW = w - 16.f;
            const float imageH = h - 55.f;
            nvgSave(vg);
            nvgIntersectScissor(vg, imageX, imageY, imageW, imageH);
            const int handle = _imageHandle(vg, asset.localPath);
            if (handle > 0) {
                int iw = 0, ih = 0;
                nvgImageSize(vg, handle, &iw, &ih);
                const float scaleFit = iw > 0 && ih > 0
                    ? std::min(imageW / iw, imageH / ih) : 1.f;
                const float drawW = iw * scaleFit;
                const float drawH = ih * scaleFit;
                const float drawX = imageX + (imageW - drawW) * 0.5f;
                const float drawY = imageY + (imageH - drawH) * 0.5f;
                nvgBeginPath(vg);
                nvgRoundedRect(vg, drawX, drawY, drawW, drawH, 8.f);
                nvgFillPaint(vg, nvgImagePattern(vg, drawX, drawY, drawW,
                    drawH, 0.f, handle, 1.f));
                nvgFill(vg);
            } else {
                nvgBeginPath(vg);
                nvgRoundedRect(vg, imageX, imageY, imageW, imageH, 8.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, 12));
                nvgFill(vg);
                nvgFontFaceId(vg, m_materialFont);
                nvgFontSize(vg, 38.f);
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(155, 169, 191, 190));
                const std::string loading = encodeUtf8(0xE863);
                nvgSave(vg);
                nvgTranslate(vg, imageX + imageW * 0.5f,
                             imageY + imageH * 0.5f);
                nvgRotate(vg, m_time * 4.4f + index * 0.35f);
                nvgText(vg, 0.f, 0.f, loading.c_str(), nullptr);
                nvgRestore(vg);
            }
            nvgRestore(vg);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 13.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(205, 213, 226, 205));
            const std::string meta = std::to_string(asset.width) + "×" +
                std::to_string(asset.height) +
                (asset.language.empty() ? "" : "  " + asset.language);
            nvgText(vg, x + 10.f, y + h - 23.f, meta.c_str(), nullptr);
            if (focused) beiklive::ui::drawGradientFocusBorder(
                vg, x, y, w, h, 13.f, 3.f, 1.f,
                beiklive::ui::gradientFocusAnimationOffset(m_time));
            nvgRestore(vg);
        }

        std::string _filterValue(int row) const
        {
            switch (row) {
                case 0: return m_filters.width == 0 ? L("不限") : std::to_string(m_filters.width);
                case 1: return m_filters.height == 0 ? L("不限") : std::to_string(m_filters.height);
                case 2: return m_filters.style.empty() ? L("不限") : m_filters.style;
                case 3: return m_filters.mime.empty() ? L("不限") : m_filters.mime;
                case 4: return m_filters.language.empty() ? L("不限") : m_filters.language;
                case 5: return m_filters.allowHumor ? L("允许") : L("过滤");
                default: return L("不限");
            }
        }

        void _drawFilter(NVGcontext* vg, float x, float y, float w, float h)
        {
            const float eased = easeOutBack(m_overlayProgress);
            nvgBeginPath(vg);
            nvgRect(vg, x, y, w, h);
            nvgFillColor(vg, nvgRGBA(0, 0, 0,
                static_cast<unsigned char>(205.f * smooth(m_overlayProgress))));
            nvgFill(vg);
            const float panelW = 640.f;
            const float panelH = 500.f;
            const float panelX = x + (w - panelW) * 0.5f;
            const float panelY = y + (h - panelH) * 0.5f + (1.f - eased) * 35.f;
            nvgBeginPath(vg);
            nvgRoundedRect(vg, panelX, panelY, panelW, panelH, 18.f);
            nvgFillColor(vg, nvgRGBA(24, 28, 38, 246));
            nvgFill(vg);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 74));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 27.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(248, 249, 252, 248));
            nvgText(vg, panelX + 32.f, panelY + 43.f, L("素材过滤").c_str(), nullptr);
            nvgFontSize(vg, 14.f);
            nvgFillColor(vg, nvgRGBA(190, 199, 214, 190));
            nvgText(vg, panelX + 32.f, panelY + 72.f,
                    L("NSFW 成人内容始终过滤，不会显示").c_str(), nullptr);
        static const std::string labels[] = {
            L("宽度"), L("高度"), L("风格"), L("格式"), L("语言"), L("幽默内容")};
            for (int i = 0; i < 6; ++i) {
                const float rowY = panelY + 98.f + i * 58.f;
                const bool focused = i == m_filterRow;
                nvgBeginPath(vg);
                nvgRoundedRect(vg, panelX + 24.f, rowY, panelW - 48.f, 48.f, 10.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, focused ? 32 : 8));
                nvgFill(vg);
                if (focused) beiklive::ui::drawGradientFocusBorder(
                    vg, panelX + 24.f, rowY, panelW - 48.f, 48.f, 10.f, 3.f,
                    1.f, beiklive::ui::gradientFocusAnimationOffset(m_time));
                nvgFontFaceId(vg, m_defaultFont);
                nvgFontSize(vg, 19.f);
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(232, 236, 244, 235));
                nvgText(vg, panelX + 44.f, rowY + 24.f, labels[i].c_str(), nullptr);
                nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(105, 198, 255, 245));
                const std::string value = "‹  " + _filterValue(i) + "  ›";
                nvgText(vg, panelX + panelW - 44.f, rowY + 24.f,
                        value.c_str(), nullptr);
            }
        }

        void _drawSwitchGlyph(NVGcontext* vg, brls::ControllerButton button,
                              float x, float y, float alpha)
        {
            const std::string glyph = brls::Hint::getKeyIcon(button);
            nvgFontFaceId(vg, m_switchFont);
            nvgFontSize(vg, 27.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255,
                static_cast<unsigned char>(245.f * alpha)));
            nvgText(vg, x, y, glyph.c_str(), nullptr);
        }

        void _drawHint(NVGcontext* vg, brls::ControllerButton button,
                       const char* label, float& cursor, float y, float alpha)
        {
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 16.f);
            float bounds[4]{};
            nvgTextBounds(vg, 0.f, 0.f, label, nullptr, bounds);
            cursor -= bounds[2] - bounds[0] + 41.f;
            _drawSwitchGlyph(vg, button, cursor + 12.f, y, alpha);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 16.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(232, 236, 244,
                static_cast<unsigned char>(225.f * alpha)));
            nvgText(vg, cursor + 28.f, y, label, nullptr);
            cursor -= 12.f;
        }

        void _drawFooter(NVGcontext* vg, float x, float y, float w, float h,
                         float alpha)
        {
            float cursor = x + w - 28.f;
            const float hintY = y + h - 28.f;
            if (m_mode == Mode::Filter) {
                _drawHint(vg, brls::BUTTON_B, L("返回").c_str(), cursor, hintY, alpha);
                _drawHint(vg, brls::BUTTON_A, L("确认").c_str(), cursor, hintY, alpha);
                return;
            }
            _drawHint(vg, brls::BUTTON_B, L("返回").c_str(), cursor, hintY, alpha);
            _drawHint(vg, brls::BUTTON_A, L("选择").c_str(), cursor, hintY, alpha);
            _drawHint(vg, brls::BUTTON_X, L("过滤").c_str(), cursor, hintY, alpha);
            _drawHint(vg, brls::BUTTON_Y, L("清空").c_str(), cursor, hintY, alpha);
            _drawHint(vg, brls::BUTTON_LT, L("手动输入").c_str(), cursor, hintY, alpha);
            _drawHint(vg, brls::BUTTON_RT, L("游戏名搜索").c_str(), cursor, hintY, alpha);
        }
    };
}

void openSteamGridDbPage(
    const GameEntry& entry,
    std::function<void(const std::string&)> onCoverChanged)
{
    auto* page = new beiklive::Box(brls::Axis::COLUMN);
    page->showHeader(false);
    page->showFooter(false);
    page->setGrow(1.f);
    page->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
    auto* canvas = new SteamGridDbCanvas(
        entry, std::move(onCoverChanged), page->getContentBox());
    page->getContentBox()->addView(canvas);
    auto* frame = new brls::AppletFrame(page);
    HIDE_BRLS_BAR(frame);
    frame->setBackground(brls::ViewBackground::NONE);
    brls::Application::pushActivity(new brls::Activity(frame),
                                    brls::TransitionAnimation::NONE);
    brls::Application::giveFocus(canvas);
}
}
