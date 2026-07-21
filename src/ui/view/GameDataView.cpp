#include "GameDataView.hpp"

#include "core/Tools.hpp"
#include "core/common.h"
#include "ui/utils/GradientFocus.hpp"
#include "ui/utils/MaterialIcons.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    std::string encodeUtf8(char32_t codepoint)
    {
        std::string output;
        if (codepoint <= 0x7F) {
            output.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            output.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0xFFFF) {
            output.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            output.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        return output;
    }

    float easeOutBack(float value)
    {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.f;
        const float t = value - 1.f;
        return 1.f + c3 * t * t * t + c1 * t * t;
    }

    uint8_t stickDirection(float x, float y)
    {
        constexpr float deadzone = 0.35f;
        constexpr float dominance = 1.35f;
        const float ax = std::abs(x);
        const float ay = std::abs(y);
        if (ax < deadzone && ay < deadzone) return 0;
        if (ax > ay * dominance) return x < 0.f ? 1 : 2;
        if (ay > ax * dominance) return y < 0.f ? 3 : 4;
        return 0;
    }

    void strokeRoundedRectInside(NVGcontext* vg, float x, float y,
                                 float w, float h, float radius,
                                 float width, NVGcolor color)
    {
        const float inset = width * 0.5f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + inset, y + inset,
                       std::max(0.f, w - width), std::max(0.f, h - width),
                       std::max(0.f, radius - inset));
        nvgStrokeColor(vg, color);
        nvgStrokeWidth(vg, width);
        nvgStroke(vg);
    }
}

namespace beiklive
{
    GameDataView::GameDataView(beiklive::GameEntry entry)
        : m_entry(std::move(entry))
    {
        setFocusable(true);
        setHideHighlightBackground(true);
        setHideHighlightBorder(true);
        setHideClickAnimation(true);
        setBackground(brls::ViewBackground::NONE);
        setClipsToBounds(true);

        m_fontId = brls::Application::getDefaultFont();
        m_materialFontId = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
        m_switchIconFontId = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
        m_lastFrameTime = std::chrono::steady_clock::now();
        m_states.resize(10);

        registerClickAction([this](brls::View*) -> bool {
            _activate();
            return true;
        });
        registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
            if (m_previewActive)
                _closeImagePreview();
            else if (onBack)
                onBack();
            return true;
        });
        registerAction("上一分类", brls::BUTTON_LB, [this](brls::View*) -> bool {
            _switchSection(-1);
            return true;
        });
        registerAction("下一分类", brls::BUTTON_RB, [this](brls::View*) -> bool {
            _switchSection(1);
            return true;
        });
        registerAction("次要操作", brls::BUTTON_X, [this](brls::View*) -> bool {
            _secondaryAction();
            return true;
        });
        registerAction("设置封面", brls::BUTTON_Y, [this](brls::View*) -> bool {
            _tertiaryAction();
            return true;
        });
        registerAction("切换渲染模式", brls::BUTTON_LT, [this](brls::View*) -> bool {
            if (!m_previewActive || m_previewClosing)
                return false;
            m_previewNearest = !m_previewNearest;
            invalidate();
            return true;
        }, false, true);
    }

    GameDataView::~GameDataView()
    {
        NVGcontext* vg = brls::Application::getNVGContext();
        if (vg) {
            for (const auto& image : m_imageCache) {
                if (image.second > 0)
                    nvgDeleteImage(vg, image.second);
            }
            for (const auto& image : m_nearestImageCache) {
                if (image.second > 0)
                    nvgDeleteImage(vg, image.second);
            }
        }
        m_imageCache.clear();
        m_nearestImageCache.clear();
    }

    void GameDataView::setStateSlots(std::vector<StateSlot> slots)
    {
        m_states = std::move(slots);
        if (m_states.empty()) m_stateIndex = 0;
        else m_stateIndex = std::clamp(m_stateIndex, 0, static_cast<int>(m_states.size()) - 1);
        invalidate();
    }

    void GameDataView::setScreenshots(std::vector<MediaItem> screenshots)
    {
        m_screenshots = std::move(screenshots);
        if (m_screenshots.empty()) m_screenshotIndex = 0;
        else m_screenshotIndex = std::clamp(
            m_screenshotIndex, 0, static_cast<int>(m_screenshots.size()) - 1);
        invalidate();
    }

    void GameDataView::setBackups(std::vector<MediaItem> backups,
                                  bool batterySaveExists)
    {
        m_backups = std::move(backups);
        m_batterySaveExists = batterySaveExists;
        if (m_backups.empty()) {
            m_backupIndex = 0;
            m_batteryPane = 0;
        } else {
            m_backupIndex = std::clamp(
                m_backupIndex, 0, static_cast<int>(m_backups.size()) - 1);
        }
        invalidate();
    }

    void GameDataView::setCoverPath(const std::string& path)
    {
        m_entry.logoPath = path;
        invalidate();
    }

    void GameDataView::openImagePreview(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_screenshots.size()))
            return;
        m_previewIndex = index;
        m_screenshotIndex = index;
        m_previewActive = true;
        m_previewClosing = false;
        m_previewNearest = false;
        m_previewTransition = 0.f;
        _resetImagePreview();
        invalidate();
    }

    void GameDataView::_closeImagePreview()
    {
        if (!m_previewActive || m_previewClosing)
            return;
        m_previewClosing = true;
        invalidate();
    }

    void GameDataView::_resetImagePreview()
    {
        m_previewZoom = 1.f;
        m_previewOffsetX = 0.f;
        m_previewOffsetY = 0.f;
    }

    void GameDataView::restoreFocus()
    {
        brls::sync([this]() {
            brls::Application::giveFocus(this);
            _captureDirections();
        });
    }

    void GameDataView::playExitAnimation(std::function<void()> completion)
    {
        if (m_exitAnimationRunning)
            return;
        m_exitAnimationRunning = true;
        m_exitCompletionArmed = false;
        m_exitCompletion = std::move(completion);
        invalidate();
    }

    void GameDataView::_switchSection(int direction)
    {
        if (m_exitAnimationRunning)
            return;
        if (m_previewActive) {
            if (!m_previewClosing) {
                if (direction < 0)
                    m_previewZoom = std::max(0.1f, m_previewZoom / 1.1f);
                else
                    m_previewZoom = std::min(20.f, m_previewZoom * 1.1f);
            }
            return;
        }
        int index = static_cast<int>(m_section);
        index = (index + (direction < 0 ? -1 : 1) + 3) % 3;
        m_section = static_cast<Section>(index);
        m_sectionDirection = direction < 0 ? -1 : 1;
        m_contentTransition = 0.f;
        m_scrollY = 0.f;
        m_targetScrollY = 0.f;
        if (onSectionChanged) onSectionChanged(m_section);
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        invalidate();
    }

    bool GameDataView::_moveUp()
    {
        if (m_previewActive) {
            if (!m_previewClosing)
                m_previewOffsetY -= 18.f * std::max(1.f, m_previewZoom);
            return true;
        }
        switch (m_section) {
            case Section::STATES:
                if (m_stateIndex >= 5) { m_stateIndex -= 5; return true; }
                break;
            case Section::SCREENSHOTS: {
                const int cols = _currentScreenshotColumns();
                if (m_screenshotIndex >= cols) { m_screenshotIndex -= cols; return true; }
                break;
            }
            case Section::BATTERY:
                if (m_batteryPane == 0 && m_actionIndex > 0) { --m_actionIndex; return true; }
                if (m_batteryPane == 1 && m_backupIndex > 0) { --m_backupIndex; return true; }
                break;
        }
        return false;
    }

    bool GameDataView::_moveDown()
    {
        if (m_previewActive) {
            if (!m_previewClosing)
                m_previewOffsetY += 18.f * std::max(1.f, m_previewZoom);
            return true;
        }
        switch (m_section) {
            case Section::STATES:
                if (m_stateIndex + 5 < static_cast<int>(m_states.size())) {
                    m_stateIndex += 5;
                    return true;
                }
                break;
            case Section::SCREENSHOTS: {
                const int next = m_screenshotIndex + _currentScreenshotColumns();
                if (next < static_cast<int>(m_screenshots.size())) {
                    m_screenshotIndex = next;
                    return true;
                }
                break;
            }
            case Section::BATTERY:
                if (m_batteryPane == 0 && m_actionIndex < 2) { ++m_actionIndex; return true; }
                if (m_batteryPane == 1 && m_backupIndex + 1 < static_cast<int>(m_backups.size())) {
                    ++m_backupIndex;
                    return true;
                }
                break;
        }
        return false;
    }

    bool GameDataView::_moveLeft()
    {
        if (m_previewActive) {
            if (!m_previewClosing)
                m_previewOffsetX -= 18.f * std::max(1.f, m_previewZoom);
            return true;
        }
        switch (m_section) {
            case Section::STATES:
                if (m_stateIndex % 5 > 0) { --m_stateIndex; return true; }
                break;
            case Section::SCREENSHOTS:
                if (!m_screenshots.empty() && m_screenshotIndex % _currentScreenshotColumns() > 0) {
                    --m_screenshotIndex;
                    return true;
                }
                break;
            case Section::BATTERY:
                if (m_batteryPane == 1) { m_batteryPane = 0; return true; }
                break;
        }
        return false;
    }

    bool GameDataView::_moveRight()
    {
        if (m_previewActive) {
            if (!m_previewClosing)
                m_previewOffsetX += 18.f * std::max(1.f, m_previewZoom);
            return true;
        }
        switch (m_section) {
            case Section::STATES:
                if (m_stateIndex % 5 < 4 && m_stateIndex + 1 < static_cast<int>(m_states.size())) {
                    ++m_stateIndex;
                    return true;
                }
                break;
            case Section::SCREENSHOTS:
                if (!m_screenshots.empty() &&
                    m_screenshotIndex % _currentScreenshotColumns() < _currentScreenshotColumns() - 1 &&
                    m_screenshotIndex + 1 < static_cast<int>(m_screenshots.size())) {
                    ++m_screenshotIndex;
                    return true;
                }
                break;
            case Section::BATTERY:
                if (m_batteryPane == 0 && !m_backups.empty()) {
                    m_batteryPane = 1;
                    return true;
                }
                break;
        }
        return false;
    }

    void GameDataView::_activate()
    {
        if (m_exitAnimationRunning)
            return;
        if (m_previewActive) {
            _closeImagePreview();
            return;
        }
        if (m_section == Section::STATES) {
            if (m_stateIndex >= 0 && m_stateIndex < static_cast<int>(m_states.size()) &&
                m_states[static_cast<size_t>(m_stateIndex)].exists && onDeleteState)
                onDeleteState(m_stateIndex);
            return;
        }
        if (m_section == Section::SCREENSHOTS) {
            if (m_screenshotIndex >= 0 &&
                m_screenshotIndex < static_cast<int>(m_screenshots.size()))
                openImagePreview(m_screenshotIndex);
            return;
        }
        if (m_batteryPane == 0) {
            if (m_actionIndex == 0 && onExportSave) onExportSave();
            else if (m_actionIndex == 1 && onImportSave) onImportSave();
            else if (m_actionIndex == 2 && onBackupSave) onBackupSave();
        } else if (m_backupIndex >= 0 &&
                   m_backupIndex < static_cast<int>(m_backups.size()) &&
                   onRestoreBackup) {
            onRestoreBackup(m_backupIndex);
        }
    }

    void GameDataView::_secondaryAction()
    {
        if (m_exitAnimationRunning)
            return;
        if (m_previewActive)
            return;
        if (m_section == Section::STATES &&
            m_stateIndex >= 0 && m_stateIndex < static_cast<int>(m_states.size()) &&
            m_states[static_cast<size_t>(m_stateIndex)].exists && onDeleteState) {
            onDeleteState(m_stateIndex);
        } else if (m_section == Section::SCREENSHOTS &&
                   m_screenshotIndex >= 0 &&
                   m_screenshotIndex < static_cast<int>(m_screenshots.size()) &&
                   onDeleteScreenshot) {
            onDeleteScreenshot(m_screenshotIndex);
        } else if (m_section == Section::BATTERY && m_batteryPane == 1 &&
                   m_backupIndex >= 0 && m_backupIndex < static_cast<int>(m_backups.size()) &&
                   onDeleteBackup) {
            onDeleteBackup(m_backupIndex);
        }
    }

    void GameDataView::_tertiaryAction()
    {
        if (m_previewActive) {
            if (!m_previewClosing)
                _resetImagePreview();
            return;
        }
        if (m_section == Section::SCREENSHOTS &&
            m_screenshotIndex >= 0 &&
            m_screenshotIndex < static_cast<int>(m_screenshots.size()) &&
            onSetScreenshotCover) {
            onSetScreenshotCover(m_screenshotIndex);
        }
    }

    void GameDataView::_captureDirections()
    {
        auto& state = brls::Application::getControllerState();
        const float lx = state.axes[static_cast<int>(brls::LEFT_X)];
        const float ly = state.axes[static_cast<int>(brls::LEFT_Y)];
        const float rx = state.axes[static_cast<int>(brls::RIGHT_X)];
        const float ry = state.axes[static_cast<int>(brls::RIGHT_Y)];
        uint8_t direction = stickDirection(lx, ly);
        const uint8_t rightDirection = stickDirection(rx, ry);
        if (rightDirection) direction = rightDirection;
        m_prevUp = state.buttons[static_cast<int>(brls::BUTTON_UP)] || direction == 3;
        m_prevDown = state.buttons[static_cast<int>(brls::BUTTON_DOWN)] || direction == 4;
        m_prevLeft = state.buttons[static_cast<int>(brls::BUTTON_LEFT)] || direction == 1;
        m_prevRight = state.buttons[static_cast<int>(brls::BUTTON_RIGHT)] || direction == 2;
    }

    void GameDataView::_handleDirectionInput(float dt)
    {
        if (!isFocused()) {
            m_wasFocused = false;
            return;
        }
        if (!m_wasFocused) {
            _captureDirections();
            m_wasFocused = true;
            return;
        }

        auto& state = brls::Application::getControllerState();
        const float lx = state.axes[static_cast<int>(brls::LEFT_X)];
        const float ly = state.axes[static_cast<int>(brls::LEFT_Y)];
        const float rx = state.axes[static_cast<int>(brls::RIGHT_X)];
        const float ry = state.axes[static_cast<int>(brls::RIGHT_Y)];
        uint8_t direction = stickDirection(lx, ly);
        const uint8_t rightDirection = stickDirection(rx, ry);
        if (rightDirection) direction = rightDirection;
        const bool up = state.buttons[static_cast<int>(brls::BUTTON_UP)] || direction == 3;
        const bool down = state.buttons[static_cast<int>(brls::BUTTON_DOWN)] || direction == 4;
        const bool left = state.buttons[static_cast<int>(brls::BUTTON_LEFT)] || direction == 1;
        const bool right = state.buttons[static_cast<int>(brls::BUTTON_RIGHT)] || direction == 2;

        if (m_previewActive && !m_previewClosing) {
            const float step = 420.f * dt * std::max(1.f, m_previewZoom);
            if (up) m_previewOffsetY -= step;
            if (down) m_previewOffsetY += step;
            if (left) m_previewOffsetX -= step;
            if (right) m_previewOffsetX += step;
            m_prevUp = up;
            m_prevDown = down;
            m_prevLeft = left;
            m_prevRight = right;
            return;
        }

        auto update = [&](bool now, bool& previous, float& held, float& repeat,
                          const std::function<bool()>& move) {
            bool moved = false;
            if (now && !previous) {
                held = 0.f;
                repeat = 0.f;
                moved = move();
            } else if (now) {
                held += dt;
                if (held > 0.32f) {
                    repeat += dt;
                    const float interval = held > 1.2f ? 0.045f : 0.10f;
                    if (repeat >= interval) {
                        repeat = 0.f;
                        moved = move();
                    }
                }
            } else {
                held = 0.f;
                repeat = 0.f;
            }
            if (moved)
                brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
            previous = now;
        };

        update(up, m_prevUp, m_holdUp, m_repeatUp, [this]() { return _moveUp(); });
        update(down, m_prevDown, m_holdDown, m_repeatDown, [this]() { return _moveDown(); });
        update(left, m_prevLeft, m_holdLeft, m_repeatLeft, [this]() { return _moveLeft(); });
        update(right, m_prevRight, m_holdRight, m_repeatRight, [this]() { return _moveRight(); });
    }

    void GameDataView::_updateScrollTarget(float viewportHeight)
    {
        if (m_section == Section::SCREENSHOTS && !m_screenshots.empty()) {
            const float rowHeight = 218.f;
            const int row = m_screenshotIndex / _currentScreenshotColumns();
            const int rows = static_cast<int>((m_screenshots.size() + 3) / 4);
            const float total = rows * rowHeight;
            m_targetScrollY = std::clamp(
                row * rowHeight + rowHeight * 0.5f - viewportHeight * 0.5f,
                0.f, std::max(0.f, total - viewportHeight));
        } else if (m_section == Section::BATTERY && m_batteryPane == 1 && !m_backups.empty()) {
            const float rowHeight = 76.f;
            const float total = m_backups.size() * rowHeight;
            m_targetScrollY = std::clamp(
                m_backupIndex * rowHeight + rowHeight * 0.5f - viewportHeight * 0.5f,
                0.f, std::max(0.f, total - viewportHeight));
        } else {
            m_targetScrollY = 0.f;
        }
    }

    void GameDataView::frame(brls::FrameContext* ctx)
    {
        brls::View::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.1f) dt = 0.016f;
        dt = std::min(dt, 0.033f);

        m_focusTime += dt;
        if (m_exitAnimationRunning) {
            m_pageEntrance = std::max(0.f, m_pageEntrance - dt * 5.f);
            if (m_pageEntrance <= 0.f && m_exitCompletion) {
                if (!m_exitCompletionArmed) {
                    m_exitCompletionArmed = true;
                } else {
                    auto completion = std::move(m_exitCompletion);
                    m_exitCompletion = nullptr;
                    brls::sync(std::move(completion));
                }
            }
        } else {
            m_pageEntrance = std::min(1.f, m_pageEntrance + dt * 4.8f);
            m_contentTransition = std::min(1.f, m_contentTransition + dt * 5.5f);
            _handleDirectionInput(dt);
        }
        if (m_previewActive) {
            if (m_previewClosing) {
                m_previewTransition = std::max(0.f, m_previewTransition - dt * 7.5f);
                if (m_previewTransition <= 0.f) {
                    if (m_previewIndex >= 0 &&
                        m_previewIndex < static_cast<int>(m_screenshots.size())) {
                        const std::string& path =
                            m_screenshots[static_cast<size_t>(m_previewIndex)].path;
                        auto nearest = m_nearestImageCache.find(path);
                        if (nearest != m_nearestImageCache.end()) {
                            NVGcontext* vg = brls::Application::getNVGContext();
                            if (vg && nearest->second > 0)
                                nvgDeleteImage(vg, nearest->second);
                            m_nearestImageCache.erase(nearest);
                        }
                    }
                    m_previewActive = false;
                    m_previewClosing = false;
                    m_previewIndex = -1;
                    _captureDirections();
                }
            } else {
                m_previewTransition = std::min(1.f, m_previewTransition + dt * 7.f);
            }
        }
        _updateScrollTarget(492.f);
        m_scrollY += (m_targetScrollY - m_scrollY) * std::min(1.f, dt * 10.f);
        invalidate();
    }

    void GameDataView::_drawPanel(NVGcontext* vg, float x, float y, float w, float h,
                                  float radius, bool filled, float alpha)
    {
        NVGpaint shadow = nvgBoxGradient(
            vg, x + 3.f, y + 3.f, w, h, radius, 5.f,
            nvgRGBA(0, 0, 0, static_cast<unsigned char>(68.f * alpha)),
            nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, x - 2.f, y - 2.f, w + 10.f, h + 10.f);
        nvgRoundedRect(vg, x, y, w, h, radius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);
        if (filled) {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, w, h, radius);
            nvgFillColor(vg, nvgRGBA(255, 255, 255,
                static_cast<unsigned char>(13.f * alpha)));
            nvgFill(vg);
        }
        strokeRoundedRectInside(vg, x, y, w, h, radius, 1.f,
            nvgRGBA(255, 255, 255, static_cast<unsigned char>(72.f * alpha)));
    }

    void GameDataView::_drawFocus(NVGcontext* vg, float x, float y, float w, float h,
                                  float radius, float alpha)
    {
        beiklive::ui::drawGradientFocusBorder(
            vg, x - 2.f, y - 2.f, w + 4.f, h + 4.f, radius + 2.f,
            5.f, alpha,
            beiklive::ui::gradientFocusAnimationOffset(m_focusTime));
    }

    int GameDataView::_getImage(NVGcontext* vg, const std::string& path, bool nearest)
    {
        if (!vg || path.empty()) return -1;
        auto& cache = nearest ? m_nearestImageCache : m_imageCache;
        auto found = cache.find(path);
        if (found != cache.end()) return found->second;
#ifdef __SWITCH__
        constexpr int maxLoadsPerFrame = 2;
#else
        constexpr int maxLoadsPerFrame = 4;
#endif
        if (m_imageLoadsThisFrame >= maxLoadsPerFrame)
            return -1;
        ++m_imageLoadsThisFrame;
        const int image = nvgCreateImage(
            vg, path.c_str(), nearest ? NVG_IMAGE_NEAREST : 0);
        cache[path] = image;
        return image;
    }

    void GameDataView::_drawImageCover(NVGcontext* vg, const std::string& path,
                                       float x, float y, float w, float h, float radius)
    {
        const int image = _getImage(vg, path);
        if (image <= 0) {
            _drawMaterialIcon(vg, beiklive::material::IMAGE_PLACEHOLDER,
                              x + w * 0.5f, y + h * 0.5f, 58.f,
                              nvgRGBA(255, 255, 255, 82));
            return;
        }
        int iw = 0;
        int ih = 0;
        nvgImageSize(vg, image, &iw, &ih);
        if (iw <= 0 || ih <= 0) return;
        const float scale = std::max(w / static_cast<float>(iw),
                                     h / static_cast<float>(ih));
        const float dw = iw * scale;
        const float dh = ih * scale;
        const float ix = x + (w - dw) * 0.5f;
        const float iy = y + (h - dh) * 0.5f;
        nvgSave(vg);
        nvgIntersectScissor(vg, x, y, w, h);
        NVGpaint paint = nvgImagePattern(vg, ix, iy, dw, dh, 0.f, image, 1.f);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, radius);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
        nvgRestore(vg);
    }

    void GameDataView::_drawMaterialIcon(NVGcontext* vg, char32_t icon,
                                         float x, float y, float size,
                                         NVGcolor color)
    {
        if (m_materialFontId < 0) return;
        const std::string text = encodeUtf8(icon);
        nvgFontFaceId(vg, m_materialFontId);
        nvgFontSize(vg, size);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, color);
        nvgText(vg, x, y, text.c_str(), nullptr);
    }

    void GameDataView::_drawSwitchButton(NVGcontext* vg, brls::ControllerButton button,
                                         float x, float y, float size, NVGcolor color)
    {
        if (m_switchIconFontId < 0) return;
        const std::string glyph = brls::Hint::getKeyIcon(button);
        nvgFontFaceId(vg, m_switchIconFontId);
        nvgFontSize(vg, size);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, color);
        nvgText(vg, x, y, glyph.c_str(), nullptr);
    }

    void GameDataView::_drawHint(NVGcontext* vg, float x, float y,
                                 brls::ControllerButton button,
                                 const std::string& label)
    {
        _drawSwitchButton(vg, button, x + 16.f, y, 29.f,
                          nvgRGBA(255, 255, 255, 245));
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 21.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 225));
        nvgText(vg, x + 37.f, y, label.c_str(), nullptr);
    }

    void GameDataView::_drawHeader(NVGcontext* vg, float x, float y, float w)
    {
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 27.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(247, 248, 252, 255));
        nvgText(vg, x + 30.f, y + 35.f, "游戏数据", nullptr);

        const char* labels[] = {"即时存档", "游戏图片", "电池存档"};
        const float centerX = x + w * 0.5f;
        const float centerY = y + 35.f;
        const int selected = static_cast<int>(m_section);
        _drawSwitchButton(vg, brls::BUTTON_LB, centerX - 260.f, centerY,
                          22.f, nvgRGBA(255, 255, 255, 218));
        _drawSwitchButton(vg, brls::BUTTON_RB, centerX + 260.f, centerY,
                          22.f, nvgRGBA(255, 255, 255, 218));
        for (int offset = -1; offset <= 1; ++offset) {
            const int index = (selected + offset + 3) % 3;
            const float itemX = centerX + offset * 158.f;
            const float prominence = offset == 0 ? 1.f : 0.42f;
            if (offset == 0) {
                _drawPanel(vg, itemX - 69.f, centerY - 23.f, 138.f, 46.f,
                           8.f, true, 1.f);
            }
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, offset == 0 ? 22.f : 18.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255,
                static_cast<unsigned char>(255.f * prominence)));
            nvgText(vg, itemX, centerY, labels[index], nullptr);
        }

        const float lineY = y + 73.f;
        NVGpaint shadow = nvgBoxGradient(
            vg, x + 32.f, lineY + 2.f, w - 64.f, 1.f, 0.5f, 5.f,
            nvgRGBA(0, 0, 0, 62), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, x + 27.f, lineY - 3.f, w - 54.f, 10.f);
        nvgRect(vg, x + 30.f, lineY - 0.5f, w - 60.f, 1.f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + 30.f, lineY);
        nvgLineTo(vg, x + w - 30.f, lineY);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 48));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
    }

    void GameDataView::_drawSummary(NVGcontext* vg, float x, float y, float w, float h)
    {
        _drawPanel(vg, x, y, w, h, 8.f, true, 1.f);
        const float coverX = x + 28.f;
        const float coverY = y + 28.f;
        const float coverW = w - 56.f;
        const float coverH = 238.f;
        _drawPanel(vg, coverX, coverY, coverW, coverH, 8.f, false, 0.8f);
        _drawImageCover(vg, m_entry.logoPath, coverX, coverY, coverW, coverH, 8.f);

        const std::string title = m_entry.title.empty() ? m_entry.path : m_entry.title;
        nvgSave(vg);
        nvgIntersectScissor(vg, x + 22.f, coverY + coverH + 24.f, w - 44.f, 38.f);
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 24.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 245));
        nvgText(vg, x + w * 0.5f, coverY + coverH + 43.f, title.c_str(), nullptr);
        nvgRestore(vg);

        const std::string badge = beiklive::tools::platformBadgeName(m_entry.platform);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + w * 0.5f - 31.f, coverY + coverH + 70.f,
                       62.f, 26.f, 5.f);
        nvgFillColor(vg, nvgRGBA(79, 153, 222, 205));
        nvgFill(vg);
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 14.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 245));
        nvgText(vg, x + w * 0.5f, coverY + coverH + 83.f, badge.c_str(), nullptr);

        int stateCount = 0;
        for (const auto& state : m_states) if (state.exists) ++stateCount;
        const std::string values[] = {
            std::to_string(stateCount) + " / 10",
            std::to_string(m_screenshots.size()),
            m_batterySaveExists ? "存在" : "不存在",
            std::to_string(m_backups.size())
        };
        const char* names[] = {"即时存档", "游戏图片", "电池存档", "备份数量"};
        const float startY = coverY + coverH + 126.f;
        for (int i = 0; i < 4; ++i) {
            const float rowY = startY + i * 35.f;
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 16.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 145));
            nvgText(vg, x + 24.f, rowY, names[i], nullptr);
            nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 232));
            nvgText(vg, x + w - 24.f, rowY, values[i].c_str(), nullptr);
        }
    }

    void GameDataView::_drawStates(NVGcontext* vg, float x, float y, float w, float h)
    {
        const int cols = 5;
        const float gap = 12.f;
        const float cardW = (w - gap * (cols - 1)) / cols;
        const float cardH = (h - gap) * 0.5f;
        for (int i = 0; i < static_cast<int>(m_states.size()); ++i) {
            const int row = i / cols;
            const int col = i % cols;
            const float cardX = x + col * (cardW + gap);
            const float cardY = y + row * (cardH + gap);
            const auto& state = m_states[static_cast<size_t>(i)];
            _drawPanel(vg, cardX, cardY, cardW, cardH, 8.f, state.exists, 1.f);
            if (state.exists && !state.thumbnail.empty())
                _drawImageCover(vg, state.thumbnail, cardX + 8.f, cardY + 8.f,
                                cardW - 16.f, cardH - 72.f, 6.f);
            else
                _drawMaterialIcon(vg, beiklive::material::SAVE,
                                  cardX + cardW * 0.5f, cardY + cardH * 0.43f,
                                  48.f, nvgRGBA(255, 255, 255, state.exists ? 180 : 65));
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 18.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, state.exists ? 240 : 120));
            nvgText(vg, cardX + cardW * 0.5f, cardY + cardH - 42.f,
                    state.title.c_str(), nullptr);
            if (state.exists) {
                nvgFontSize(vg, 13.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, 145));
                nvgText(vg, cardX + cardW * 0.5f, cardY + cardH - 18.f,
                        state.time.c_str(), nullptr);
            }
            if (i == m_stateIndex)
                _drawFocus(vg, cardX, cardY, cardW, cardH, 8.f);
        }
    }

    void GameDataView::_drawScreenshots(NVGcontext* vg, float x, float y,
                                        float w, float h)
    {
        if (m_screenshots.empty()) {
            _drawMaterialIcon(vg, beiklive::material::PHOTO_LIBRARY,
                              x + w * 0.5f, y + h * 0.44f, 70.f,
                              nvgRGBA(255, 255, 255, 70));
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 22.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 135));
            nvgText(vg, x + w * 0.5f, y + h * 0.60f, "暂无游戏图片", nullptr);
            return;
        }

        const int cols = _currentScreenshotColumns();
        const float gap = 12.f;
        const float cardW = (w - gap * (cols - 1)) / cols;
        const float cardH = 204.f;
        const float rowH = 218.f;
        nvgSave(vg);
        nvgIntersectScissor(vg, x - 4.f, y - 4.f, w + 8.f, h + 8.f);
        for (int i = 0; i < static_cast<int>(m_screenshots.size()); ++i) {
            const int row = i / cols;
            const int col = i % cols;
            const float cardY = y + row * rowH - m_scrollY;
            if (cardY + cardH < y - 10.f || cardY > y + h + 10.f) continue;
            const float cardX = x + col * (cardW + gap);
            const auto& shot = m_screenshots[static_cast<size_t>(i)];
            _drawPanel(vg, cardX, cardY, cardW, cardH, 8.f, true, 1.f);
            _drawImageCover(vg, shot.path, cardX + 8.f, cardY + 8.f,
                            cardW - 16.f, 142.f, 6.f);
            nvgSave(vg);
            nvgIntersectScissor(vg, cardX + 10.f, cardY + 155.f, cardW - 20.f, 24.f);
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 15.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 235));
            nvgText(vg, cardX + 11.f, cardY + 167.f, shot.title.c_str(), nullptr);
            nvgRestore(vg);
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 12.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 125));
            nvgText(vg, cardX + 11.f, cardY + 188.f, shot.time.c_str(), nullptr);
            if (i == m_screenshotIndex)
                _drawFocus(vg, cardX, cardY, cardW, cardH, 8.f);
        }
        nvgRestore(vg);
    }

    void GameDataView::_drawBattery(NVGcontext* vg, float x, float y,
                                    float w, float h)
    {
        const float actionW = 300.f;
        const float gap = 24.f;
        const float listX = x + actionW + gap;
        const float listW = w - actionW - gap;
        const char* labels[] = {"导出存档", "导入存档", "创建备份"};
        const char32_t icons[] = {
            beiklive::material::CLOUD_UPLOAD,
            beiklive::material::CLOUD_DOWNLOAD,
            beiklive::material::BACKUP
        };
        const float buttonH = 90.f;
        const float startY = y + (h - buttonH * 3.f - 28.f) * 0.5f;
        for (int i = 0; i < 3; ++i) {
            const float buttonY = startY + i * (buttonH + 14.f);
            _drawPanel(vg, x, buttonY, actionW, buttonH, 8.f, true, 1.f);
            _drawMaterialIcon(vg, icons[i], x + 46.f, buttonY + buttonH * 0.5f,
                              36.f, nvgRGBA(255, 255, 255, 225));
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 23.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 235));
            nvgText(vg, x + 82.f, buttonY + buttonH * 0.5f, labels[i], nullptr);
            if (m_batteryPane == 0 && i == m_actionIndex)
                _drawFocus(vg, x, buttonY, actionW, buttonH, 8.f);
        }

        _drawPanel(vg, listX, y, listW, h, 8.f, true, 1.f);
        const float listPadding = 28.f;
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 22.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 238));
        nvgText(vg, listX + listPadding, y + 30.f, "备份记录", nullptr);
        nvgFontSize(vg, 15.f);
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 135));
        const std::string count = std::to_string(m_backups.size()) + " 个";
        nvgText(vg, listX + listW - listPadding, y + 30.f, count.c_str(), nullptr);

        nvgBeginPath(vg);
        nvgMoveTo(vg, listX + listPadding, y + 57.f);
        nvgLineTo(vg, listX + listW - listPadding, y + 57.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 42));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        if (m_backups.empty()) {
            _drawMaterialIcon(vg, beiklive::material::BACKUP,
                              listX + listW * 0.5f, y + h * 0.46f, 62.f,
                              nvgRGBA(255, 255, 255, 65));
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 19.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 120));
            nvgText(vg, listX + listW * 0.5f, y + h * 0.60f,
                    "暂无电池存档备份", nullptr);
            return;
        }

        const float contentY = y + 70.f;
        const float contentH = h - 84.f;
        const float rowH = 76.f;
        nvgSave(vg);
        nvgIntersectScissor(
            vg, listX + listPadding - 5.f, contentY,
            listW - listPadding * 2.f + 10.f, contentH);
        for (int i = 0; i < static_cast<int>(m_backups.size()); ++i) {
            const float rowY = contentY + i * rowH - m_scrollY;
            if (rowY + 64.f < contentY || rowY > contentY + contentH) continue;
            const auto& backup = m_backups[static_cast<size_t>(i)];
            const float rowX = listX + listPadding;
            const float rowW = listW - listPadding * 2.f;
            _drawPanel(vg, rowX, rowY, rowW, 64.f, 7.f, false, 0.82f);
            _drawMaterialIcon(vg, beiklive::material::RESTORE,
                              rowX + 30.f, rowY + 32.f, 29.f,
                              nvgRGBA(255, 255, 255, 190));
            nvgSave(vg);
            nvgIntersectScissor(vg, rowX + 58.f, rowY + 7.f, rowW - 76.f, 26.f);
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 17.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 230));
            nvgText(vg, rowX + 58.f, rowY + 20.f, backup.title.c_str(), nullptr);
            nvgRestore(vg);
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 13.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 125));
            nvgText(vg, rowX + 58.f, rowY + 45.f, backup.time.c_str(), nullptr);
            if (m_batteryPane == 1 && i == m_backupIndex)
                _drawFocus(vg, rowX, rowY, rowW, 64.f, 7.f);
        }
        nvgRestore(vg);
    }

    void GameDataView::_drawFooter(NVGcontext* vg, float x, float y, float w, float h)
    {
        const float lineY = y + 1.f;
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + 30.f, lineY);
        nvgLineTo(vg, x + w - 30.f, lineY);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 48));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        const float cy = y + h * 0.5f;
        const float pathLeft = x + 30.f;
        const float pathRight = x + w * 0.5f - 24.f;
        const float iconX = pathLeft + 14.f;
        const float textX = pathLeft + 38.f;
        const float textW = std::max(40.f, pathRight - textX);
        _drawMaterialIcon(vg, beiklive::material::STORAGE, iconX, cy,
                          24.f, nvgRGBA(255, 255, 255, 175));
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 16.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 155));
        float pathBounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, m_entry.path.c_str(), nullptr, pathBounds);
        const float pathWidth = pathBounds[2] - pathBounds[0];
        nvgSave(vg);
        nvgIntersectScissor(vg, textX, y + 8.f, textW, h - 16.f);
        if (pathWidth <= textW) {
            nvgText(vg, textX, cy, m_entry.path.c_str(), nullptr);
        } else {
            const float gap = 58.f;
            const float travel = pathWidth + gap;
            const float offset = std::fmod(std::max(0.f, m_focusTime - 0.8f) * 34.f, travel);
            nvgText(vg, textX - offset, cy, m_entry.path.c_str(), nullptr);
            nvgText(vg, textX - offset + travel, cy, m_entry.path.c_str(), nullptr);
        }
        nvgRestore(vg);

        struct HintItem { brls::ControllerButton button; const char* label; };
        std::vector<HintItem> hints;
        hints.push_back({brls::BUTTON_B, "返回"});
        if (m_section == Section::STATES) {
            hints.push_back({brls::BUTTON_A, "删除"});
        } else if (m_section == Section::SCREENSHOTS) {
            hints.push_back({brls::BUTTON_Y, "设为封面"});
            hints.push_back({brls::BUTTON_X, "删除"});
            hints.push_back({brls::BUTTON_A, "查看"});
        } else if (m_batteryPane == 1) {
            hints.push_back({brls::BUTTON_X, "删除"});
            hints.push_back({brls::BUTTON_A, "还原"});
        } else {
            hints.push_back({brls::BUTTON_A, "执行"});
        }

        float cursor = x + w - 28.f;
        for (const auto& hint : hints) {
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 21.f);
            float bounds[4]{};
            nvgTextBounds(vg, 0.f, 0.f, hint.label, nullptr, bounds);
            const float width = 38.f + bounds[2] - bounds[0];
            cursor -= width;
            _drawHint(vg, cursor, cy, hint.button, hint.label);
            cursor -= 34.f;
        }
    }

    void GameDataView::_drawImagePreview(NVGcontext* vg, float x, float y,
                                         float w, float h)
    {
        if (!m_previewActive || m_previewIndex < 0 ||
            m_previewIndex >= static_cast<int>(m_screenshots.size()))
            return;

        const float progress = 1.f - std::pow(
            1.f - std::clamp(m_previewTransition, 0.f, 1.f), 3.f);
        const auto& screenshot = m_screenshots[static_cast<size_t>(m_previewIndex)];
        nvgSave(vg);
        nvgGlobalAlpha(vg, progress);
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 226));
        nvgFill(vg);

        const float viewX = x + 46.f;
        const float viewY = y + 54.f;
        const float viewW = w - 92.f;
        const float viewH = h - 142.f;
        const int image = _getImage(vg, screenshot.path, m_previewNearest);
        if (image > 0) {
            int imageW = 0;
            int imageH = 0;
            nvgImageSize(vg, image, &imageW, &imageH);
            if (imageW > 0 && imageH > 0) {
                const float baseScale = std::min(
                    viewW / static_cast<float>(imageW),
                    viewH / static_cast<float>(imageH));
                const float scale = baseScale * m_previewZoom;
                const float drawW = imageW * scale;
                const float drawH = imageH * scale;
                const float maxOffsetX = std::max(0.f, (drawW - viewW) * 0.5f + 80.f);
                const float maxOffsetY = std::max(0.f, (drawH - viewH) * 0.5f + 80.f);
                m_previewOffsetX = std::clamp(m_previewOffsetX, -maxOffsetX, maxOffsetX);
                m_previewOffsetY = std::clamp(m_previewOffsetY, -maxOffsetY, maxOffsetY);
                const float scaleAnimation = 0.94f + progress * 0.06f;
                const float finalW = drawW * scaleAnimation;
                const float finalH = drawH * scaleAnimation;
                const float imageX = viewX + viewW * 0.5f - finalW * 0.5f + m_previewOffsetX;
                const float imageY = viewY + viewH * 0.5f - finalH * 0.5f + m_previewOffsetY;
                nvgSave(vg);
                nvgIntersectScissor(vg, viewX, viewY, viewW, viewH);
                NVGpaint paint = nvgImagePattern(
                    vg, imageX, imageY, finalW, finalH, 0.f, image, 1.f);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, imageX, imageY, finalW, finalH, 5.f);
                nvgFillPaint(vg, paint);
                nvgFill(vg);
                nvgRestore(vg);
            }
        } else {
            _drawMaterialIcon(vg, beiklive::material::IMAGE_PLACEHOLDER,
                              x + w * 0.5f, y + h * 0.46f, 72.f,
                              nvgRGBA(255, 255, 255, 100));
        }

        nvgSave(vg);
        nvgIntersectScissor(vg, x + 180.f, y + 14.f, w - 360.f, 32.f);
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 20.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 225));
        nvgText(vg, x + w * 0.5f, y + 30.f, screenshot.title.c_str(), nullptr);
        nvgRestore(vg);

        const float hintY = y + h - 34.f;
        struct PreviewHint { brls::ControllerButton button; const char* label; };
        const PreviewHint hints[] = {
            {brls::BUTTON_B, "关闭"},
            {brls::BUTTON_Y, "复位"},
            {brls::BUTTON_LT, m_previewNearest ? "最近邻" : "线性"},
            {brls::BUTTON_RB, "放大"},
            {brls::BUTTON_LB, "缩小"},
        };
        float cursor = x + w - 32.f;
        for (const auto& hint : hints) {
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 21.f);
            float bounds[4]{};
            nvgTextBounds(vg, 0.f, 0.f, hint.label, nullptr, bounds);
            const float hintWidth = 37.f + bounds[2] - bounds[0];
            cursor -= hintWidth;
            _drawHint(vg, cursor, hintY, hint.button, hint.label);
            cursor -= 24.f;
        }
        nvgRestore(vg);
    }

    void GameDataView::draw(NVGcontext* vg, float x, float y, float w, float h,
                            brls::Style style, brls::FrameContext* ctx)
    {
        (void)style;
        (void)ctx;
        if (!vg) return;
        const float page = std::clamp(m_pageEntrance, 0.f, 1.f);
        const float eased = 1.f - std::pow(1.f - page, 3.f);
        const float bounce = easeOutBack(page);
        const float content = 1.f - std::pow(1.f - m_contentTransition, 3.f);
        m_imageLoadsThisFrame = 0;

        nvgSave(vg);
        nvgIntersectScissor(vg, x, y, w, h);
        nvgGlobalAlpha(vg, 0.08f + eased * 0.92f);
        _drawHeader(vg, x, y - (1.f - bounce) * 78.f, w);
        _drawFooter(vg, x, y + h - 72.f + (1.f - bounce) * 74.f, w, 72.f);

        const float contentY = y + 92.f;
        const float contentH = h - 164.f;
        const float summaryX = x + 30.f;
        const float summaryW = 250.f;
        _drawSummary(vg, summaryX - (1.f - content) * 28.f,
                     contentY, summaryW, contentH);

        const float mainX = x + 306.f;
        const float mainW = w - 336.f;
        const float slide = static_cast<float>(m_sectionDirection) *
            (1.f - content) * 90.f;
        nvgSave(vg);
        nvgGlobalAlpha(vg, content);
        if (m_section == Section::STATES)
            _drawStates(vg, mainX + slide, contentY, mainW, contentH);
        else if (m_section == Section::SCREENSHOTS)
            _drawScreenshots(vg, mainX + slide, contentY, mainW, contentH);
        else
            _drawBattery(vg, mainX + slide, contentY, mainW, contentH);
        nvgRestore(vg);
        _drawImagePreview(vg, x, y, w, h);
        nvgRestore(vg);
    }
}
