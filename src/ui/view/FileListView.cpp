#include "FileListView.hpp"
#include "core/Translation.hpp"
#include "core/PinyinTools.hpp"
#include "ui/utils/GradientFocus.hpp"
#include "ui/utils/MaterialIcons.hpp"

#include <cctype>
#include <unordered_map>

namespace beiklive {

namespace {
std::string encodeMaterialIcon(char32_t codepoint)
{
    std::string out;
    if (codepoint <= 0x7F) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return out;
}
}

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
    m_contentEntrance = 0.f;
    ensureFocusedVisible();
    if (m_focusedIndex >= 0 && m_focusedIndex < static_cast<int>(m_items.size())
        && onItemFocused)
        onItemFocused(m_items[static_cast<size_t>(m_focusedIndex)]);
}

void FileListView::clearItems() {
    m_items.clear();
    m_unfilteredItems.clear();
    m_filterActive = false;
    m_focusedIndex = -1;
    m_scrollY = 0.f;
    m_targetScrollY = 0.f;
    m_contentEntrance = 0.f;
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
        if (lower.find(lowerKw) != std::string::npos) {
            m_items.push_back(item);
            continue;
        }
        // 文件名含中文且关键词为 ASCII 时，支持拼音全拼/首字母匹配
        if (!lowerKw.empty() && beiklive::pinyin::containsCjk(item.text)) {
            bool pureAscii = true;
            for (unsigned char c : lowerKw) {
                if (c >= 0x80 || !std::isalnum(c)) { pureAscii = false; break; }
            }
            if (pureAscii) {
                const std::string pyFull = beiklive::pinyin::full(item.text);
                const std::string pyInit = beiklive::pinyin::initials(item.text);
                if ((!pyFull.empty() && pyFull.find(lowerKw) != std::string::npos) ||
                    (!pyInit.empty() && pyInit.find(lowerKw) != std::string::npos)) {
                    m_items.push_back(item);
                    continue;
                }
            }
        }
    }

    m_filterActive = true;
    m_focusedIndex = m_items.empty() ? -1 : 0;
    m_scrollY = 0.f;
    m_targetScrollY = 0.f;
    m_contentEntrance = 0.f;
    ensureFocusedVisible();
    if (m_focusedIndex >= 0 && m_focusedIndex < static_cast<int>(m_items.size())
        && onItemFocused)
        onItemFocused(m_items[static_cast<size_t>(m_focusedIndex)]);
    this->invalidate();
}

void FileListView::removeFilter() {
    if (!m_filterActive) return;
    m_items = m_unfilteredItems;
    m_filterActive = false;
    if (m_focusedIndex >= (int)m_items.size())
        m_focusedIndex = std::max(0, (int)m_items.size() - 1);
    ensureFocusedVisible();
    m_contentEntrance = 0.f;
    if (m_focusedIndex >= 0 && m_focusedIndex < static_cast<int>(m_items.size())
        && onItemFocused)
        onItemFocused(m_items[static_cast<size_t>(m_focusedIndex)]);
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

    if (m_loading) {
        const float cx = x + w * 0.5f;
        const float cy = y + h * 0.5f - 18.f;
        nvgBeginPath(vg);
        nvgArc(vg, cx, cy, 22.f, m_animTime * 4.8f,
               m_animTime * 4.8f + 4.4f, NVG_CW);
        nvgStrokeColor(vg, nvgRGBA(79, 193, 255, 230));
        nvgStrokeWidth(vg, 4.f);
        nvgLineCap(vg, NVG_ROUND);
        nvgStroke(vg);
        nvgFontFaceId(vg, m_font);
        nvgFontSize(vg, 20.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(225, 230, 238, 220));
        nvgText(vg, cx, cy + 48.f, L("正在加载目录...").c_str(), nullptr);
        nvgFontSize(vg, 14.f);
        nvgFillColor(vg, nvgRGBA(195, 203, 215, 150));
        nvgText(vg, cx, cy + 74.f, L("文件较多时可能需要一些时间").c_str(), nullptr);
        nvgRestore(vg);
        return;
    }

    if (m_items.empty()) {
        nvgFontFaceId(vg, m_font);
        nvgFontSize(vg, 22.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(205, 212, 223, 175));
        nvgText(vg, x + w * 0.5f, y + h * 0.5f - 8.f,
                m_filterActive ? L("没有匹配的文件").c_str() : L("此目录为空").c_str(), nullptr);
        nvgFontSize(vg, 15.f);
        nvgFillColor(vg, nvgRGBA(190, 198, 211, 135));
        nvgText(vg, x + w * 0.5f, y + h * 0.5f + 24.f,
                m_filterActive ? L("按 B 关闭搜索").c_str() : L("返回上一级或选择其他目录").c_str(), nullptr);
        nvgRestore(vg);
        return;
    }

    int first = (int)(m_scrollY / m_itemHeight);
    if (first < 0) first = 0;
    int last = first + (int)(h / m_itemHeight) + 2;
    if (last > (int)m_items.size()) last = (int)m_items.size();

    loadVisibleIcons(vg, first, last);

    NVGcolor textColor = uiTextPrimary();

    for (int i = first; i < last; i++) {
        float itemY = y + i * m_itemHeight - m_scrollY;
        bool focused = (i == m_focusedIndex);
        const float stagger = std::max(0.f, std::min(1.f,
            m_contentEntrance * 1.35f - static_cast<float>(i - first) * 0.055f));
        const float eased = 1.f - std::pow(1.f - stagger, 3.f);
        nvgSave(vg);
        nvgGlobalAlpha(vg, stagger);
        nvgTranslate(vg, (1.f - eased) * 34.f, 0.f);
        drawItem(vg, i, x, itemY, w,
                 focused ? nvgRGB(255, 255, 255) : textColor);
        nvgRestore(vg);
    }

    int vr = visibleRows();
    if (vr > 0 && static_cast<int>(m_items.size()) > vr)
        drawScrollbar(vg, x + w, y, w, h);

    nvgRestore(vg);
}

void FileListView::drawItem(NVGcontext* vg, int index, float itemX, float itemY,
                            float w, NVGcolor textColor) {
    const auto& item = m_items[index];
    const float rowX = itemX + 12.f;
    const float rowW = w - 24.f;
    const float baseY = itemY + 6.f;
    const float baseH = m_itemHeight - 12.f;
    float padX = rowX + 18.f;
    float padY = (m_itemHeight - m_iconSize) * 0.5f;
    float textX = padX + m_iconSize + 12.f;

    const NVGpaint baseShadow = nvgBoxGradient(
        vg, rowX + 4.f, baseY + 5.f, rowW, baseH, 8.f, 5.f,
        nvgRGBA(0, 0, 0, 48), nvgRGBA(0, 0, 0, 0));
    nvgBeginPath(vg);
    nvgRect(vg, rowX - 3.f, baseY - 3.f, rowW + 14.f, baseH + 15.f);
    nvgRoundedRect(vg, rowX, baseY, rowW, baseH, 8.f);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, baseShadow);
    nvgFill(vg);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, rowX, baseY, rowW, baseH, 8.f);
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 6));
    nvgFill(vg);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, rowX + 1.f, baseY + 1.f,
                   rowW - 2.f, baseH - 2.f, 7.f);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 34));
    nvgStrokeWidth(vg, 1.f);
    nvgStroke(vg);

    // Focus highlight - flowing gradient rounded border
    if (index == m_focusedIndex && m_focusedIndex >= 0) {
        float shakeY = 0.f;
        if (m_shakeTime > 0.f && m_shakeDir != 0) {
            float t = m_shakeTime / 0.35f;
            float decay = t * t;
            float freq = 80.f;
            shakeY = std::sin(m_shakeTime * freq) * 6.f * decay * m_shakeDir;
        }

        float rx = rowX;
        float ry = itemY + 6.f + shakeY;
        float rw = rowW;
        float rh = m_itemHeight - 12.f;

        const NVGpaint shadow = nvgBoxGradient(
            vg, rx + 4.f, ry + 5.f, rw, rh, 8.f, 5.f,
            nvgRGBA(0, 0, 0, 65), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, rx - 3.f, ry - 3.f, rw + 14.f, rh + 15.f);
        nvgRoundedRect(vg, rx, ry, rw, rh, 8.f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, rx, ry, rw, rh, 8.f);
        nvgFillColor(vg, nvgRGBA(79, 193, 255, 30));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, rx + 1.f, ry + 1.f, rw - 2.f, rh - 2.f, 7.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 125));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);

        beiklive::ui::drawGradientFocusBorder(
            vg,
            rx,
            ry,
            rw,
            rh,
            8.0f,
            3.0f,
            1.0f,
            beiklive::ui::gradientFocusAnimationOffset(m_animTime));

    }

    // Icon: archive/game entries use the bundled Material Icons font so they
    // remain crisp at every scale. Custom artwork still takes precedence when
    // no glyph was supplied.
    if (item.materialIcon != 0) {
        const std::string glyph = encodeMaterialIcon(item.materialIcon);
        nvgFontFaceId(vg, brls::Application::getFont(brls::FONT_MATERIAL_ICONS));
        nvgFontSize(vg, m_iconSize * 0.78f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, textColor);
        nvgText(vg, padX + m_iconSize * 0.5f, itemY + m_itemHeight * 0.5f,
                glyph.c_str(), nullptr);
    } else if (!item.iconPath.empty()) {
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
    float textMarginR = rowX + rowW - 18.f;

    nvgFontSize(vg, 22.f);
    nvgFillColor(vg, textColor);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgSave(vg);
    nvgIntersectScissor(vg, textX, itemY,
                       std::max(1.0f, rowX + rowW - textX - 150.0f), m_itemHeight);
    nvgText(vg, textX, centerY, item.text.c_str(), nullptr);
    nvgRestore(vg);

    nvgFontSize(vg, 15.f);
    nvgFillColor(vg, textColor);
    nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    nvgSave(vg);
    nvgIntersectScissor(vg, std::max(textX, rowX + rowW - 135.0f), itemY,
                       120.0f, m_itemHeight);
    nvgText(vg, textMarginR, centerY, item.subText.c_str(), nullptr);
    nvgRestore(vg);

    // Separator line
    nvgBeginPath(vg);
    nvgMoveTo(vg, textX, itemY + m_itemHeight - 1.f);
    nvgLineTo(vg, rowX + rowW - 18.f, itemY + m_itemHeight - 1.f);
    nvgStrokeColor(vg, uiDivider());
    nvgStrokeWidth(vg, 1.f);
    nvgStroke(vg);
}

void FileListView::drawScrollbar(NVGcontext* vg, float x, float y, float w, float h) {
    float maxScroll = m_items.size() * m_itemHeight - m_viewHeight;
    if (maxScroll <= 0.f) return;

    float barH = std::max(20.f, (m_viewHeight / (m_items.size() * m_itemHeight)) * m_viewHeight);
    float barY = y + (m_scrollY / maxScroll) * (m_viewHeight - barH);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, x - 6.f, barY, 3.f, barH, 1.5f);
    nvgFillColor(vg, uiTextMuted(0.70f));
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
    m_contentEntrance = std::min(1.f, m_contentEntrance + dt * 4.2f);
    if (m_loading || m_contentEntrance < 1.f)
        invalidate();

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
