#include "FileListView.hpp"

namespace beiklive {

FileListView::FileListView() {
    this->setFocusable(true);
    HIDE_BRLS_HIGHLIGHT(this);
    m_lastFrameTime = std::chrono::steady_clock::now();
}

// ── Data ──

void FileListView::setItems(const std::vector<beiklive::ListItem>& items) {
    m_items = items;
    if (m_focusedIndex < 0 && !m_items.empty())
        m_focusedIndex = 0;
    if (m_focusedIndex >= (int)m_items.size())
        m_focusedIndex = std::max(0, (int)m_items.size() - 1);
    m_scrollY = 0.f;
    ensureFocusedVisible();
}

void FileListView::clearItems() {
    m_items.clear();
    m_focusedIndex = -1;
    m_scrollY = 0.f;
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

// ── Drawing ──

void FileListView::draw(NVGcontext* vg, float x, float y, float w, float h,
                         brls::Style style, brls::FrameContext* ctx) {
    m_viewHeight = h;

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

    // Focus highlight – rotating dual-gradient rounded border + left accent bar
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

        float cx = rx + rw * 0.5f;
        float cy = ry + rh * 0.5f;
        float radius = std::max(rw, rh) * 0.6f;
        float angle = m_animTime * 1.8f;
        float sx = cx + std::cos(angle) * radius;
        float sy = cy + std::sin(angle) * radius;
        float ex = cx + std::cos(angle + 3.14159265f) * radius;
        float ey = cy + std::sin(angle + 3.14159265f) * radius;

        NVGcolor c1 = nvgRGBA(79, 193, 255, 200);
        NVGcolor c2 = nvgRGBA(30, 80, 140, 200);
        NVGpaint grad = nvgLinearGradient(vg, sx, sy, ex, ey, c1, c2);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, rx, ry, rw, rh, 8.f);
        nvgStrokeWidth(vg, 4.f);
        nvgStrokePaint(vg, grad);
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgRect(vg, rx, itemY + (m_itemHeight - 40.f) * 0.5f, 5.f, 40.f);
        nvgFillColor(vg, nvgRGBA(79, 193, 255, 255));
        nvgFill(vg);
    }

    // Icon
    if (!item.iconPath.empty()) {
        int img = getOrLoadIcon(vg, item.iconPath);
        if (img > 0) {
            NVGpaint paint = nvgImagePattern(vg, padX, itemY + padY,
                                              m_iconSize, m_iconSize, 0.f, img, 1.f);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, padX, itemY + padY, m_iconSize, m_iconSize, 6.f);
            nvgFillPaint(vg, paint);
            nvgFill(vg);
        }
    }

    // Title + Subtitle (horizontal: title left, subtitle right)
    float centerY = itemY + m_itemHeight * 0.5f + 2.f;
    float textMarginR = 4.f;

    nvgFontSize(vg, 22.f);
    nvgFillColor(vg, textColor);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgText(vg, textX, centerY, item.text.c_str(), nullptr);

    nvgFontSize(vg, 15.f);
    nvgFillColor(vg, textColor);
    nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    nvgText(vg, w - textMarginR, centerY, item.subText.c_str(), nullptr);

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

int FileListView::getOrLoadIcon(NVGcontext* vg, const std::string& path) {
    auto it = m_iconCache.find(path);
    if (it != m_iconCache.end()) return it->second;
    int handle = nvgCreateImage(vg, path.c_str(), 0);
    if (handle > 0) m_iconCache[path] = handle;
    return handle;
}

// ── Frame update ──

void FileListView::frame(brls::FrameContext* ctx) {
    brls::View::frame(ctx);

    if (m_interactionDisabled || m_items.empty()) return;

    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
    m_lastFrameTime = now;
    if (dt <= 0.f || dt > 0.5f) dt = 0.016f;

    m_animTime += dt;

    if (m_shakeTime > 0.f)
        m_shakeTime -= dt;

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
    } else {
        m_shakeTime = 0.35f;
        m_shakeDir = -1;
    }
}

void FileListView::moveDown() {
    if (m_focusedIndex < (int)m_items.size() - 1) {
        int old = m_focusedIndex;
        m_focusedIndex++;
        ensureFocusedVisible();
        fireFocusCallbacks(old);
    } else {
        m_shakeTime = 0.35f;
        m_shakeDir = 1;
    }
}

void FileListView::movePageUp() {
    if (m_items.empty()) return;
    int step = std::max(1, visibleRows() - 1);
    int old = m_focusedIndex;
    m_focusedIndex = std::max(0, m_focusedIndex - step);
    ensureFocusedVisible();
    fireFocusCallbacks(old);
}

void FileListView::movePageDown() {
    if (m_items.empty()) return;
    int step = std::max(1, visibleRows() - 1);
    int old = m_focusedIndex;
    m_focusedIndex = std::min((int)m_items.size() - 1, m_focusedIndex + step);
    ensureFocusedVisible();
    fireFocusCallbacks(old);
}

void FileListView::ensureFocusedVisible() {
    if (m_focusedIndex < 0 || m_items.empty()) return;

    float itemTop = m_focusedIndex * m_itemHeight;
    float itemBottom = itemTop + m_itemHeight;
    float viewTop = m_scrollY;
    float viewBottom = m_scrollY + m_viewHeight;

    if (itemTop < viewTop)
        m_scrollY = itemTop;
    else if (itemBottom > viewBottom)
        m_scrollY = itemBottom - m_viewHeight;

    float maxScroll = m_items.size() * m_itemHeight - m_viewHeight;
    if (m_scrollY < 0.f) m_scrollY = 0.f;
    if (m_scrollY > maxScroll && maxScroll > 0.f) m_scrollY = maxScroll;
    else if (maxScroll <= 0.f) m_scrollY = 0.f;
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
