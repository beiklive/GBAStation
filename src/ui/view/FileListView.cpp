#include "FileListView.hpp"
#include "ui/utils/GradientFocus.hpp"

namespace beiklive {

FileListView::FileListView() {
    this->setFocusable(true);
    HIDE_BRLS_HIGHLIGHT(this);
    m_lastFrameTime = std::chrono::steady_clock::now();
    m_font = brls::Application::getDefaultFont();
}

FileListView::~FileListView() {
    NVGcontext* vg = brls::Application::getNVGContext();
    if (vg) {
        for (const auto& kv : m_iconCache) {
            if (kv.second > 0)
                nvgDeleteImage(vg, kv.second);
        }
    }
    m_iconCache.clear();
}

// ── Data ──

void FileListView::setItems(const std::vector<beiklive::ListItem>& items) {
    m_unfilteredItems = items;
    m_filterActive = false;
    m_items = m_unfilteredItems;
    if (m_focusedIndex < 0 && !m_items.empty())
        m_focusedIndex = 0;
    if (m_focusedIndex >= (int)m_items.size())
        m_focusedIndex = std::max(0, (int)m_items.size() - 1);
    m_scrollY = 0.f;
    m_targetScrollY = 0.f;
    ensureFocusedVisible();
}

void FileListView::clearItems() {
    m_items.clear();
    m_unfilteredItems.clear();
    m_filterActive = false;
    m_focusedIndex = -1;
    m_scrollY = 0.f;
    m_targetScrollY = 0.f;
}

bool FileListView::focusItemByFilename(const std::string& filename) {
    if (filename.empty() || m_items.empty())
        return false;

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].text == filename || std::filesystem::path(m_items[i].data).filename().string() == filename) {
            int old = m_focusedIndex;
            m_focusedIndex = i;
            ensureFocusedVisible();
            m_scrollY = m_targetScrollY;
            fireFocusCallbacks(old);
            invalidate();
            return true;
        }
    }

    return false;
}

// ── Focus state ──

void FileListView::saveFocusState(const std::string& path) {
    if (m_focusedIndex >= 0)
        m_dirFocusIndex[path] = m_focusedIndex;
}

void FileListView::restoreFocusState(const std::string& path) {
    auto it = m_dirFocusIndex.find(path);
    if (it != m_dirFocusIndex.end() && it->second >= 0) {
        m_focusedIndex = it->second;
        m_dirFocusIndex.erase(it);
    }
}

void FileListView::applyFilter(const std::string& keyword) {
    if (!m_filterActive)
        m_unfilteredItems = m_items;

    if (keyword.empty()) {
        removeFilter();
        return;
    }

    m_items.clear();
    for (const auto& item : m_unfilteredItems) {
        if (item.text.empty()) continue;
        std::string lower = item.text;
        std::string lowerKw = keyword;
        for (auto& c : lower) c = static_cast<char>(std::tolower((unsigned char)c));
        for (auto& c : lowerKw) c = static_cast<char>(std::tolower((unsigned char)c));
        if (lower.find(lowerKw) != std::string::npos)
            m_items.push_back(item);
    }

    m_filterActive = true;
    m_focusedIndex = m_items.empty() ? -1 : 0;
    m_scrollY = 0.f;
    m_targetScrollY = 0.f;
    ensureFocusedVisible();
    this->invalidate();
}

void FileListView::removeFilter() {
    if (!m_filterActive) return;
    m_items = m_unfilteredItems;
    m_filterActive = false;
    if (m_focusedIndex >= (int)m_items.size())
        m_focusedIndex = std::max(0, (int)m_items.size() - 1);
    ensureFocusedVisible();
    this->invalidate();
}

// ── Drawing ──

void FileListView::draw(NVGcontext* vg, float x, float y, float w, float h,
                         brls::Style style, brls::FrameContext* ctx) {
    bool heightChanged = std::abs(m_lastLayoutHeight - h) > 0.5f;
    m_viewHeight = h;
    m_lastLayoutHeight = h;
    if (heightChanged) {
        ensureFocusedVisible();
        clampScroll();
        m_scrollY = m_targetScrollY;
    }

    nvgSave(vg);
    nvgScissor(vg, x, y, w, h);

    // Background
    // nvgBeginPath(vg);
    // nvgRect(vg, x, y, w, h);
    // nvgFillColor(vg, nvgRGBA(0, 0, 0, 40));
    // nvgFill(vg);

    if (m_items.empty()) {
        nvgRestore(vg);
        return;
    }

    int first = (int)(m_scrollY / m_itemHeight);
    if (first < 0) first = 0;
    int last = first + (int)(h / m_itemHeight) + 2;
    if (last > (int)m_items.size()) last = (int)m_items.size();

    loadVisibleIcons(vg, first, last);

    NVGcolor textColor = GET_THEME_COLOR("brls/text");

    for (int i = first; i < last; i++) {
        float itemY = y + i * m_itemHeight - m_scrollY;
        bool focused = (i == m_focusedIndex);
        drawItem(vg, i, itemY, w, focused ? nvgRGB(255, 255, 255) : textColor);
    }

    // Scrollbar
    // int vr = visibleRows();
    // if (vr > 0 && (int)m_items.size() > vr)
    //     drawScrollbar(vg, x + w, y, w, h);

    nvgRestore(vg);
}

void FileListView::drawItem(NVGcontext* vg, int index, float itemY, float w, NVGcolor textColor) {
    const auto& item = m_items[index];
    float padX = 48.f;
    float padY = (m_itemHeight - m_iconSize) * 0.5f;
    float textX = padX + m_iconSize + 12.f;

    // Focus highlight - flowing gradient rounded border + left accent bar
    if (index == m_focusedIndex && m_focusedIndex >= 0) {
        float shakeY = 0.f;
        if (m_shakeTime > 0.f && m_shakeDir != 0) {
            float t = m_shakeTime / 0.35f;
            float decay = t * t;
            float freq = 80.f;
            shakeY = std::sin(m_shakeTime * freq) * 6.f * decay * m_shakeDir;
        }

        float rx = 36.f;
        float ry = itemY + 6.f + shakeY;
        float rw = w - 16.f;
        float rh = m_itemHeight - 12.f;

        beiklive::ui::drawGradientFocusBorder(
            vg,
            rx,
            ry,
            rw,
            rh,
            8.0f,
            4.0f,
            1.0f,
            beiklive::ui::gradientFocusAnimationOffset(m_animTime));

        nvgBeginPath(vg);
        nvgRect(vg, rx, itemY + (m_itemHeight - 40.f) * 0.5f, 5.f, 40.f);
        nvgFillColor(vg, nvgRGBA(79, 193, 255, 255));
        nvgFill(vg);
    }

    // Icon
    if (!item.iconPath.empty()) {
        int img = getCachedIcon(item.iconPath);
        if (img > 0) {
            NVGpaint paint = nvgImagePattern(vg, padX, itemY + padY,
                                              m_iconSize, m_iconSize, 0.f, img, 1.f);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, padX, itemY + padY, m_iconSize, m_iconSize, 6.f);
            nvgFillPaint(vg, paint);
            nvgFill(vg);
        }
    }
    nvgFontFaceId(vg, m_font);
    // Title + Subtitle (horizontal: title left, subtitle right)
    float centerY = itemY + m_itemHeight * 0.5f + 2.f;
    float textMarginR = 4.f;

    nvgFontSize(vg, 22.f);
    nvgFillColor(vg, textColor);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgSave(vg);
    nvgIntersectScissor(vg, textX, itemY, std::max(1.0f, w - textX - 120.0f), m_itemHeight);
    nvgText(vg, textX, centerY, item.text.c_str(), nullptr);
    nvgRestore(vg);

    nvgFontSize(vg, 15.f);
    nvgFillColor(vg, textColor);
    nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    nvgSave(vg);
    nvgIntersectScissor(vg, std::max(textX, w - 120.0f), itemY, 120.0f, m_itemHeight);
    nvgText(vg, w - textMarginR, centerY, item.subText.c_str(), nullptr);
    nvgRestore(vg);

    // Separator line
    nvgBeginPath(vg);
    nvgMoveTo(vg, textX, itemY + m_itemHeight - 1.f);
    nvgLineTo(vg, w - padX - 4.f, itemY + m_itemHeight - 1.f);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 10));
    nvgStrokeWidth(vg, 1.f);
    nvgStroke(vg);
}

void FileListView::drawScrollbar(NVGcontext* vg, float x, float y, float w, float h) {
    float maxScroll = m_items.size() * m_itemHeight - m_viewHeight;
    if (maxScroll <= 0.f) return;

    float barH = std::max(20.f, (m_viewHeight / (m_items.size() * m_itemHeight)) * m_viewHeight);
    float barY = (m_scrollY / maxScroll) * (m_viewHeight - barH);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, x - 6.f, barY, 3.f, barH, 1.5f);
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 60));
    nvgFill(vg);
}

void FileListView::loadVisibleIcons(NVGcontext* vg, int first, int last) {
    if (!vg || first >= last) return;

    int loadedThisFrame = 0;
#ifdef __SWITCH__
    static constexpr int MAX_ICON_LOADS_PER_FRAME = 1;
#else
    static constexpr int MAX_ICON_LOADS_PER_FRAME = 2;
#endif

    auto loadIconAt = [this, vg, &loadedThisFrame](int index) {
        if (index < 0 || index >= static_cast<int>(m_items.size()) || loadedThisFrame >= MAX_ICON_LOADS_PER_FRAME)
            return;

        const std::string& path = m_items[index].iconPath;
        if (path.empty() || m_iconCache.count(path) || m_failedIconPaths.count(path))
            return;

        int handle = nvgCreateImage(vg, path.c_str(), 0);
        loadedThisFrame++;
        if (handle > 0)
            m_iconCache[path] = handle;
        else
            m_failedIconPaths.insert(path);
    };

    loadIconAt(m_focusedIndex);

    for (int i = first; i < last && loadedThisFrame < MAX_ICON_LOADS_PER_FRAME; i++)
        loadIconAt(i);
}

int FileListView::getCachedIcon(const std::string& path) const {
    auto it = m_iconCache.find(path);
    if (it != m_iconCache.end()) return it->second;
    return -1;
}

// ── Frame update ──

void FileListView::frame(brls::FrameContext* ctx) {
    brls::View::frame(ctx);

    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
    m_lastFrameTime = now;
    if (dt <= 0.f || dt > 0.5f) dt = 0.016f;

    m_animTime += dt;

    if (m_shakeTime > 0.f)
        m_shakeTime -= dt;

    // Smooth scroll lerp (configurable)
    if (GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_FILE_LIST_SCROLL_ANIM, 1)) {
        float diff = m_targetScrollY - m_scrollY;
        if (std::abs(diff) > 0.5f)
            m_scrollY += diff * std::min(1.f, dt * 8.f);
        else
            m_scrollY = m_targetScrollY;
    } else {
        m_scrollY = m_targetScrollY;
    }
    clampScroll();

    if (m_interactionDisabled || m_items.empty()) return;

    auto& state = brls::Application::getControllerState();

    // ── UP ──
    bool upNow = state.buttons[brls::BUTTON_UP];
    if (upNow && !m_prevUp) {
        m_holdUpTime = 0.f;
        m_holdUpRepeat = 0.f;
        moveUp();
    }
    if (upNow) {
        m_holdUpTime += dt;
        if (m_holdUpTime > HOLD_INITIAL_DELAY) {
            m_holdUpRepeat += dt;
            float interval = m_holdUpTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdUpRepeat >= interval) {
                m_holdUpRepeat -= interval;
                moveUp();
            }
        }
    }
    m_prevUp = upNow;

    // ── DOWN ──
    bool downNow = state.buttons[brls::BUTTON_DOWN];
    if (downNow && !m_prevDown) {
        m_holdDownTime = 0.f;
        m_holdDownRepeat = 0.f;
        moveDown();
    }
    if (downNow) {
        m_holdDownTime += dt;
        if (m_holdDownTime > HOLD_INITIAL_DELAY) {
            m_holdDownRepeat += dt;
            float interval = m_holdDownTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdDownRepeat >= interval) {
                m_holdDownRepeat -= interval;
                moveDown();
            }
        }
    }
    m_prevDown = downNow;

    // ── LEFT = Page Up (long press) ──
    bool leftNow = state.buttons[brls::BUTTON_LEFT];
    if (leftNow && !m_prevLeft) {
        m_holdLeftTime = 0.f;
        m_holdLeftRepeat = 0.f;
        movePageUp();
    }
    if (leftNow) {
        m_holdLeftTime += dt;
        if (m_holdLeftTime > HOLD_INITIAL_DELAY) {
            m_holdLeftRepeat += dt;
            float interval = m_holdLeftTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdLeftRepeat >= interval) {
                m_holdLeftRepeat -= interval;
                movePageUp();
            }
        }
    }
    m_prevLeft = leftNow;

    // ── RIGHT = Page Down (long press) ──
    bool rightNow = state.buttons[brls::BUTTON_RIGHT];
    if (rightNow && !m_prevRight) {
        m_holdRightTime = 0.f;
        m_holdRightRepeat = 0.f;
        movePageDown();
    }
    if (rightNow) {
        m_holdRightTime += dt;
        if (m_holdRightTime > HOLD_INITIAL_DELAY) {
            m_holdRightRepeat += dt;
            float interval = m_holdRightTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdRightRepeat >= interval) {
                m_holdRightRepeat -= interval;
                movePageDown();
            }
        }
    }
    m_prevRight = rightNow;

    // ── Stick ──
    float ly = state.axes[brls::LEFT_Y];
    float lx = state.axes[brls::LEFT_X];
    float ry_ = state.axes[brls::RIGHT_Y];
    float rx = state.axes[brls::RIGHT_X];

    constexpr float STICK_DEADZONE = 0.3f;
    constexpr float STICK_DOMINANCE = 1.5f;
    float absLX = std::abs(lx), absLY = std::abs(ly);
    float absRX = std::abs(rx), absRY = std::abs(ry_);

    auto stickDir = [](float x, float y, float ax, float ay) -> uint8_t {
        if (ax < STICK_DEADZONE && ay < STICK_DEADZONE) return 0;
        if (ax > ay * STICK_DOMINANCE) return (x > 0) ? 2 : 1;
        if (ay > ax * STICK_DOMINANCE) return (y > 0) ? 4 : 3;
        return 0;
    };

    uint8_t dir = 0;
    uint8_t ld = stickDir(lx, ly, absLX, absLY);
    uint8_t rd = stickDir(rx, ry_, absRX, absRY);
    if (ld) dir = ld;
    if (rd) dir = rd;

    bool stickUp = (dir == 3), stickDown = (dir == 4);
    bool stickLeft = (dir == 1), stickRight = (dir == 2);

    if (stickUp && !m_prevStickUp) { m_holdUpTime = 0.f; m_holdUpRepeat = 0.f; moveUp(); }
    if (stickUp) {
        m_holdUpTime += dt;
        if (m_holdUpTime > HOLD_INITIAL_DELAY) {
            m_holdUpRepeat += dt;
            float interval = m_holdUpTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdUpRepeat >= interval) { m_holdUpRepeat -= interval; moveUp(); }
        }
    }
    m_prevStickUp = stickUp;

    if (stickDown && !m_prevStickDown) { m_holdDownTime = 0.f; m_holdDownRepeat = 0.f; moveDown(); }
    if (stickDown) {
        m_holdDownTime += dt;
        if (m_holdDownTime > HOLD_INITIAL_DELAY) {
            m_holdDownRepeat += dt;
            float interval = m_holdDownTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdDownRepeat >= interval) { m_holdDownRepeat -= interval; moveDown(); }
        }
    }
    m_prevStickDown = stickDown;

    if (stickLeft && !m_prevStickLeft) { m_holdLeftTime = 0.f; m_holdLeftRepeat = 0.f; movePageUp(); }
    if (stickLeft) {
        m_holdLeftTime += dt;
        if (m_holdLeftTime > HOLD_INITIAL_DELAY) {
            m_holdLeftRepeat += dt;
            float interval = m_holdLeftTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdLeftRepeat >= interval) { m_holdLeftRepeat -= interval; movePageUp(); }
        }
    }
    m_prevStickLeft = stickLeft;

    if (stickRight && !m_prevStickRight) { m_holdRightTime = 0.f; m_holdRightRepeat = 0.f; movePageDown(); }
    if (stickRight) {
        m_holdRightTime += dt;
        if (m_holdRightTime > HOLD_INITIAL_DELAY) {
            m_holdRightRepeat += dt;
            float interval = m_holdRightTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdRightRepeat >= interval) { m_holdRightRepeat -= interval; movePageDown(); }
        }
    }
    m_prevStickRight = stickRight;

    // ── A = Select ──
    bool aNow = state.buttons[brls::BUTTON_A];
    if (aNow && !m_prevA && m_focusedIndex >= 0 && m_focusedIndex < (int)m_items.size()) {
        if (onItemClicked)
            onItemClicked(m_items[m_focusedIndex]);
    }
    m_prevA = aNow;
}

// ── Focus movement ──

void FileListView::moveUp() {
    if (m_focusedIndex > 0) {
        int old = m_focusedIndex;
        m_focusedIndex--;
        ensureFocusedVisible();
        fireFocusCallbacks(old);
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.35f;
        m_shakeDir = -1;
        // brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

void FileListView::moveDown() {
    if (m_focusedIndex < (int)m_items.size() - 1) {
        int old = m_focusedIndex;
        m_focusedIndex++;
        ensureFocusedVisible();
        fireFocusCallbacks(old);
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.35f;
        m_shakeDir = 1;
        // brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

void FileListView::movePageUp() {
    if (m_items.empty()) return;
    int step = std::max(1, visibleRows() - 1);
    int old = m_focusedIndex;
    m_focusedIndex = std::max(0, m_focusedIndex - step);
    ensureFocusedVisible();
    fireFocusCallbacks(old);
    if (old != m_focusedIndex)
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
}

void FileListView::movePageDown() {
    if (m_items.empty()) return;
    int step = std::max(1, visibleRows() - 1);
    int old = m_focusedIndex;
    m_focusedIndex = std::min((int)m_items.size() - 1, m_focusedIndex + step);
    ensureFocusedVisible();
    fireFocusCallbacks(old);
    if (old != m_focusedIndex)
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
}

void FileListView::_captureInputState()
{
    auto& state = brls::Application::getControllerState();
    m_prevUp = state.buttons[brls::BUTTON_UP];
    m_prevDown = state.buttons[brls::BUTTON_DOWN];
    m_prevLeft = state.buttons[brls::BUTTON_LEFT];
    m_prevRight = state.buttons[brls::BUTTON_RIGHT];
    m_prevA = state.buttons[brls::BUTTON_A];
    float ly = state.axes[brls::LEFT_Y];
    float lx = state.axes[brls::LEFT_X];
    m_prevStickUp = (ly < -0.3f);
    m_prevStickDown = (ly > 0.3f);
    m_prevStickLeft = (lx < -0.3f);
    m_prevStickRight = (lx > 0.3f);
}

void FileListView::ensureFocusedVisible() {
    if (m_focusedIndex < 0 || m_items.empty()) return;

    float itemTop = m_focusedIndex * m_itemHeight;
    float itemBottom = itemTop + m_itemHeight;
    float viewTop = m_targetScrollY;
    float viewBottom = m_targetScrollY + m_viewHeight;

    if (itemTop < viewTop)
        m_targetScrollY = itemTop;
    else if (itemBottom > viewBottom)
        m_targetScrollY = itemBottom - m_viewHeight;

    float maxScroll = m_items.size() * m_itemHeight - m_viewHeight;
    if (m_targetScrollY < 0.f) m_targetScrollY = 0.f;
    if (m_targetScrollY > maxScroll && maxScroll > 0.f) m_targetScrollY = maxScroll;
    else if (maxScroll <= 0.f) m_targetScrollY = 0.f;
}

void FileListView::clampScroll() {
    float maxScroll = m_items.size() * m_itemHeight - m_viewHeight;
    if (maxScroll <= 0.f) {
        m_scrollY = 0.f;
        m_targetScrollY = 0.f;
        return;
    }

    m_targetScrollY = std::max(0.f, std::min(m_targetScrollY, maxScroll));
    m_scrollY = std::max(0.f, std::min(m_scrollY, maxScroll));
}

int FileListView::visibleRows() const {
    if (m_viewHeight <= 0.f || m_itemHeight <= 0.f) return 1;
    return std::max(1, (int)(m_viewHeight / m_itemHeight));
}

void FileListView::fireFocusCallbacks(int oldIndex) {
    if (oldIndex >= 0 && oldIndex < (int)m_items.size()) {
        if (onItemFocusLost)
            onItemFocusLost(m_items[oldIndex]);
    }
    if (m_focusedIndex >= 0 && m_focusedIndex < (int)m_items.size()) {
        if (onItemFocused)
            onItemFocused(m_items[m_focusedIndex]);
    }
}

} // namespace beiklive
