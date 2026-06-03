#include "RecyclingGrid.hpp"
#include "core/common.h"

#include <algorithm>
#include <cmath>

GameGridView::GameGridView()
{
    setFocusable(true);
    setHideHighlightBackground(true);
    setHideHighlightBorder(true);
    setHideClickAnimation(true);
    setBackground(brls::ViewBackground::NONE);
    setClipsToBounds(true);

    m_fontId = brls::Application::getDefaultFont();
    m_lastFrameTime = std::chrono::steady_clock::now();

    m_paddingLeft = 5.f;
    m_paddingRight = 5.f;
    m_paddingTop = 5.f;
}

GameGridView::~GameGridView()
{
    delete m_dataSource;
    NVGcontext* vg = brls::Application::getNVGContext();
    if (vg) {
        for (auto& kv : m_textureCache) {
            if (kv.second >= 0) nvgDeleteImage(vg, kv.second);
        }
    }
    m_textureCache.clear();
    for (auto& item : m_items) {
        item.textureHandle = -1;
        item.imageLayerHandle = -1;
    }
}

void GameGridView::setDataSource(GameGridDataSource* source)
{
    if (m_dataSource) delete m_dataSource;
    m_requestNextPage = false;
    m_dataSource = source;
    if (m_isLayouted) reloadData();
}

void GameGridView::setPadding(float top, float right, float /*bottom*/, float left)
{
    m_paddingTop = top;
    m_paddingRight = right;
    m_paddingLeft = left;
}

void GameGridView::setMultiSelectMode(bool on)
{
    m_multiSelectMode = on;
    if (!on) m_selectedForDelete.clear();
}

void GameGridView::toggleDeleteSelection(size_t index)
{
    if (m_selectedForDelete.count(static_cast<int>(index)))
        m_selectedForDelete.erase(static_cast<int>(index));
    else
        m_selectedForDelete.insert(static_cast<int>(index));
}

void GameGridView::clearDeleteSelection()
{
    m_selectedForDelete.clear();
    m_multiSelectMode = false;
}

void GameGridView::setItemFavourite(size_t index, bool fav)
{
    if (index < m_items.size())
        m_items[index].favorite = fav;
}

void GameGridView::setItemTitle(size_t index, const std::string& title)
{
    if (index < m_items.size())
        m_items[index].title = title;
}

void GameGridView::setItemImagePath(size_t index, const std::string& path)
{
    if (index < m_items.size()) {
        auto& item = m_items[index];
        item.imagePath = path;
        item.textureHandle = -1;
        item.textureLoading = false;
        item.textureReady = false;
    }
}

void GameGridView::setTitleFontSize(int opt)
{
    switch (opt) {
        case 0: m_titleFontSize = 16; break;
        case 1: m_titleFontSize = 19; break;
        case 2: m_titleFontSize = 22; break;
        default: m_titleFontSize = 16; break;
    }
    for (auto& item : m_items) item.marqueeMaxOffset = 0.f;
}

void GameGridView::setDefaultCellFocus(size_t index)
{
    m_defaultCellFocus = index;
    m_selectedIndex = static_cast<int>(index);
}

float GameGridView::_getItemWidth()
{
    float usable = getWidth() - m_paddingLeft - m_paddingRight;
    if (usable <= 0.f) return estimatedRowHeight;
    return (usable - estimatedRowSpace * (spanCount - 1)) / spanCount - 10.f;
}

float GameGridView::_getRowHeight()
{
    return estimatedRowHeight + estimatedRowSpace;
}

float GameGridView::_getItemX(int col)
{
    float itemW = _getItemWidth();
    return m_paddingLeft + col * (itemW + estimatedRowSpace);
}

float GameGridView::_getItemY(int row)
{
    return m_paddingTop + row * _getRowHeight() - m_scrollY;
}

int GameGridView::_getRowCount()
{
    if (m_items.empty()) return 0;
    return static_cast<int>((m_items.size() - 1) / spanCount + 1);
}

void GameGridView::reloadData()
{
    if (!m_dataSource) return;
    if (!m_isLayouted) m_isLayouted = true;

    size_t count = m_dataSource->getItemCount();
    m_items.clear();
    m_items.resize(count);
    for (size_t i = 0; i < count; i++) {
        m_items[i].reset();
        m_dataSource->populateItem(m_items[i], i);
    }

    m_scrollY = 0.f;
    m_targetScrollY = 0.f;
    float viewH = getHeight();
    if (viewH < 1.f) viewH = 720.f;
    m_maxScrollY = std::max(0.f, _getRowCount() * _getRowHeight()
                           + m_paddingTop - viewH);

    size_t focusIdx = m_defaultCellFocus;
    if (m_selectedGameId != 0) {
        for (size_t i = 0; i < m_items.size(); i++) {
            if (m_items[i].gameId == m_selectedGameId) {
                focusIdx = i;
                break;
            }
        }
    }
    if (focusIdx >= m_items.size()) focusIdx = 0;
    m_selectedIndex = static_cast<int>(focusIdx);
    m_selectedGameId = m_items[m_selectedIndex].gameId;
    if (m_focusChangeCallback) m_focusChangeCallback(m_selectedIndex);

    _updateVisibleRange();
    _ensureSelectedVisible();
    m_scrollY = m_targetScrollY;

    m_requestNextPage = false;
}

void GameGridView::notifyDataChanged()
{
    if (!m_dataSource) return;

    size_t oldCount = m_items.size();
    size_t newCount = m_dataSource->getItemCount();
    m_items.resize(newCount);
    for (size_t i = oldCount; i < newCount; i++) {
        m_items[i].reset();
        m_dataSource->populateItem(m_items[i], i);
    }

    float viewH = getHeight();
    if (viewH < 1.f) viewH = 720.f;
    m_maxScrollY = std::max(0.f, _getRowCount() * _getRowHeight()
                           + m_paddingTop - viewH);
    m_scrollY = std::min(m_scrollY, m_maxScrollY);
    m_targetScrollY = m_scrollY;
    _updateVisibleRange();

    m_requestNextPage = false;
}

void GameGridView::clearData()
{
    if (m_dataSource) {
        m_dataSource->clearData();
        reloadData();
    }
}

void GameGridView::onLayout()
{
    View::onLayout();
    if (!m_isLayouted && m_dataSource) {
        reloadData();
    }
}

bool GameGridView::_tryMoveUp()
{
    if (m_items.empty()) return false;
    int row = m_selectedIndex / spanCount;
    if (row > 0) {
        m_selectedIndex -= spanCount;
        m_selectedIndex = std::max(0, m_selectedIndex);
        return true;
    }
    return false;
}

bool GameGridView::_tryMoveDown()
{
    if (m_items.empty()) return false;
    int row = m_selectedIndex / spanCount;
    int maxRow = _getRowCount() - 1;
    if (row < maxRow) {
        m_selectedIndex += spanCount;
        if (m_selectedIndex >= static_cast<int>(m_items.size()))
            m_selectedIndex = static_cast<int>(m_items.size()) - 1;
        return true;
    }
    return false;
}

bool GameGridView::_tryMoveLeft()
{
    if (m_items.empty()) return false;
    int col = m_selectedIndex % spanCount;
    if (col > 0) {
        m_selectedIndex -= 1;
        return true;
    }
    return false;
}

bool GameGridView::_tryMoveRight()
{
    if (m_items.empty()) return false;
    int col = m_selectedIndex % spanCount;
    if (col < spanCount - 1) {
        m_selectedIndex += 1;
        if (m_selectedIndex >= static_cast<int>(m_items.size()))
            m_selectedIndex = static_cast<int>(m_items.size()) - 1;
        return true;
    }
    return false;
}

void GameGridView::_moveUp()
{
    if (_tryMoveUp()) {
        m_focusMoved = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.3f;
        m_shakeDir = -1.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

void GameGridView::_moveDown()
{
    if (_tryMoveDown()) {
        m_focusMoved = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.3f;
        m_shakeDir = 1.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

void GameGridView::_moveLeft()
{
    if (_tryMoveLeft()) {
        m_focusMoved = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.3f;
        m_shakeDir = -2.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

void GameGridView::_moveRight()
{
    if (_tryMoveRight()) {
        m_focusMoved = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
    } else {
        m_shakeTime = 0.3f;
        m_shakeDir = 2.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
    }
}

void GameGridView::_updateVisibleRange()
{
    if (m_items.empty()) return;

    float rowH = _getRowHeight();
    float viewH = getHeight();

    m_visibleStartRow = static_cast<int>(std::floor((m_scrollY - m_paddingTop) / rowH));
    m_visibleStartRow = std::max(0, m_visibleStartRow);

    int visibleRows = static_cast<int>(std::ceil(viewH / rowH)) + 1;
    m_visibleEndRow = m_visibleStartRow + visibleRows;
    m_visibleEndRow = std::min(m_visibleEndRow, _getRowCount());
}

void GameGridView::_ensureSelectedVisible()
{
    if (m_items.empty()) return;

    int selRow = m_selectedIndex / spanCount;
    float rowH = _getRowHeight();
    float itemY = m_paddingTop + selRow * rowH;
    float itemBottom = itemY + estimatedRowHeight;
    float viewH = getHeight();

    float target = m_scrollY;

    if (itemY < target + m_paddingTop) {
        target = itemY - m_paddingTop;
    } else if (itemBottom > target + viewH - m_paddingTop) {
        target = itemBottom - viewH + m_paddingTop;
    }

    target = std::max(0.f, std::min(target, m_maxScrollY));
    m_targetScrollY = target;
}

void GameGridView::_updateScrollPhysics(float delta)
{
    if (m_maxScrollY <= 0.f) {
        m_scrollY = 0.f;
        m_targetScrollY = 0.f;
        return;
    }

    float diff = m_targetScrollY - m_scrollY;
    if (std::abs(diff) > 0.5f)
        m_scrollY += diff * std::min(1.f, delta * 8.f);
    else
        m_scrollY = m_targetScrollY;

    m_scrollY = std::max(0.f, std::min(m_scrollY, m_maxScrollY));
}

void GameGridView::_updateFocusAnimation(float delta)
{
    for (size_t i = 0; i < m_items.size(); i++) {
        auto& item = m_items[i];
        bool focused = (static_cast<int>(i) == m_selectedIndex);
        float targetGlow = focused ? 1.f : 0.f;
        item.focusGlow += (targetGlow - item.focusGlow) * 0.15f;
        item.selected = focused;
    }
}

void GameGridView::_updateMarquee(float delta)
{
    for (size_t i = 0; i < m_items.size(); i++) {
        auto& item = m_items[i];
        bool focused = (static_cast<int>(i) == m_selectedIndex);
        if (!focused) {
            item.marqueeOffset = std::max(0.f, item.marqueeOffset - delta * 100.f);
            continue;
        }
        if (item.marqueeMaxOffset > 0.f) {
            item.marqueeOffset += delta * 30.f;
            if (item.marqueeOffset > item.marqueeMaxOffset + 20.f) {
                item.marqueeOffset = 0.f;
            }
        }
    }

    // Calculate marquee limit for focused item
    if (m_selectedIndex >= 0 && static_cast<size_t>(m_selectedIndex) < m_items.size()) {
        auto& item = m_items[m_selectedIndex];
        if (!item.title.empty()) {
            NVGcontext* vg = brls::Application::getNVGContext();
            if (vg) {
                nvgFontSize(vg, m_titleFontSize);
                nvgFontFaceId(vg, m_fontId);
                float bounds[4];
                nvgTextBounds(vg, 0, 0, item.title.c_str(), nullptr, bounds);
                float textW = bounds[2] - bounds[0];
                float textMaxW = (estimatedRowHeight - 10.f) * 1.8f;
                if (textMaxW < 0.f) textMaxW = 100.f;
                if (textW > textMaxW)
                    item.marqueeMaxOffset = textW - textMaxW;
                else
                    item.marqueeMaxOffset = 0.f;
            }
        }
    }
}

void GameGridView::_loadTextures(NVGcontext* vg)
{
    if (!vg) return;

    int loadedThisFrame = 0;
    static const int MAX_LOADS_PER_FRAME = 2;

    for (size_t i = 0; i < m_items.size() && loadedThisFrame < MAX_LOADS_PER_FRAME; i++) {
        auto& item = m_items[i];
        if (!item.imagePath.empty() && item.textureHandle < 0 && !item.textureLoading) {
            auto it = m_textureCache.find(item.imagePath);
            if (it != m_textureCache.end()) {
                item.textureHandle = it->second;
                item.textureReady = true;
            } else {
                item.textureLoading = true;
                int handle = nvgCreateImage(vg, item.imagePath.c_str(), 0);
                item.textureHandle = handle;
                item.textureLoading = false;
                loadedThisFrame++;
                if (handle >= 0) {
                    item.textureReady = true;
                    m_textureCache[item.imagePath] = handle;
                }
            }
        }
        if (!item.imageLayerPath.empty() && item.imageLayerHandle < 0 && !item.textureLoading
            && loadedThisFrame < MAX_LOADS_PER_FRAME) {
            auto it = m_textureCache.find(item.imageLayerPath);
            if (it != m_textureCache.end()) {
                item.imageLayerHandle = it->second;
            } else {
                item.textureLoading = true;
                int handle = nvgCreateImage(vg, item.imageLayerPath.c_str(), 0);
                item.imageLayerHandle = handle;
                item.textureLoading = false;
                loadedThisFrame++;
                if (handle >= 0) {
                    m_textureCache[item.imageLayerPath] = handle;
                }
            }
        }
    }
}

void GameGridView::_evictTextures()
{
    const size_t maxCache = 128;
    if (m_textureCache.size() <= maxCache) return;

    NVGcontext* vg = brls::Application::getNVGContext();
    if (!vg) return;

    size_t toRemove = m_textureCache.size() - maxCache;
    auto it = m_textureCache.begin();
    while (toRemove > 0 && it != m_textureCache.end()) {
        bool inUse = false;
        for (auto& item : m_items) {
            if (item.textureHandle == it->second || item.imageLayerHandle == it->second) {
                inUse = true;
                break;
            }
        }
        if (!inUse) {
            nvgDeleteImage(vg, it->second);
            it = m_textureCache.erase(it);
            toRemove--;
        } else {
            ++it;
        }
    }
}

void GameGridView::_captureInputState()
{
    auto& state = brls::Application::getControllerState();
    m_prevUp = state.buttons[static_cast<int>(brls::BUTTON_UP)];
    m_prevDown = state.buttons[static_cast<int>(brls::BUTTON_DOWN)];
    m_prevLeft = state.buttons[static_cast<int>(brls::BUTTON_LEFT)];
    m_prevRight = state.buttons[static_cast<int>(brls::BUTTON_RIGHT)];
    m_prevA = state.buttons[static_cast<int>(brls::BUTTON_A)];
    float ly = state.axes[static_cast<int>(brls::LEFT_Y)];
    float lx = state.axes[static_cast<int>(brls::LEFT_X)];
    m_prevStickUp = (ly < -0.3f);
    m_prevStickDown = (ly > 0.3f);
    m_prevStickLeft = (lx < -0.3f);
    m_prevStickRight = (lx > 0.3f);
}

void GameGridView::_handleInput(float dt)
{
    if (m_items.empty()) return;

    bool focused = isFocused();
    if (!focused) {
        m_wasFocused = false;
        return;
    }
    if (!m_wasFocused) {
        _captureInputState();
        m_wasFocused = true;
    }
    if (m_interactionDisabled) return;

    auto& state = brls::Application::getControllerState();

    bool upNow = state.buttons[static_cast<int>(brls::BUTTON_UP)];
    if (upNow && !m_prevUp) {
        m_holdUpTime = 0.f;
        m_holdUpRepeat = 0.f;
        _moveUp();
    }
    if (upNow) {
        m_holdUpTime += dt;
        if (m_holdUpTime > HOLD_INITIAL_DELAY) {
            m_holdUpRepeat += dt;
            float interval = m_holdUpTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdUpRepeat >= interval) {
                m_holdUpRepeat -= interval;
                if (_tryMoveUp()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevUp = upNow;

    bool downNow = state.buttons[static_cast<int>(brls::BUTTON_DOWN)];
    if (downNow && !m_prevDown) {
        m_holdDownTime = 0.f;
        m_holdDownRepeat = 0.f;
        _moveDown();
    }
    if (downNow) {
        m_holdDownTime += dt;
        if (m_holdDownTime > HOLD_INITIAL_DELAY) {
            m_holdDownRepeat += dt;
            float interval = m_holdDownTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdDownRepeat >= interval) {
                m_holdDownRepeat -= interval;
                if (_tryMoveDown()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevDown = downNow;

    bool leftNow = state.buttons[static_cast<int>(brls::BUTTON_LEFT)];
    if (leftNow && !m_prevLeft) {
        m_holdLeftTime = 0.f;
        m_holdLeftRepeat = 0.f;
        _moveLeft();
    }
    if (leftNow) {
        m_holdLeftTime += dt;
        if (m_holdLeftTime > HOLD_INITIAL_DELAY) {
            m_holdLeftRepeat += dt;
            float interval = m_holdLeftTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdLeftRepeat >= interval) {
                m_holdLeftRepeat -= interval;
                if (_tryMoveLeft()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevLeft = leftNow;

    bool rightNow = state.buttons[static_cast<int>(brls::BUTTON_RIGHT)];
    if (rightNow && !m_prevRight) {
        m_holdRightTime = 0.f;
        m_holdRightRepeat = 0.f;
        _moveRight();
    }
    if (rightNow) {
        m_holdRightTime += dt;
        if (m_holdRightTime > HOLD_INITIAL_DELAY) {
            m_holdRightRepeat += dt;
            float interval = m_holdRightTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdRightRepeat >= interval) {
                m_holdRightRepeat -= interval;
                if (_tryMoveRight()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevRight = rightNow;

    // ── Stick (Left & Right) ──
    float ly = state.axes[static_cast<int>(brls::LEFT_Y)];
    float lx = state.axes[static_cast<int>(brls::LEFT_X)];
    float ry = state.axes[static_cast<int>(brls::RIGHT_Y)];
    float rx = state.axes[static_cast<int>(brls::RIGHT_X)];

    constexpr float STICK_DEADZONE = 0.3f;
    constexpr float STICK_DOMINANCE = 1.5f;
    float absLX = std::abs(lx), absLY = std::abs(ly);
    float absRX = std::abs(rx), absRY = std::abs(ry);

    auto stickDir = [](float x, float y, float ax, float ay) -> uint8_t {
        if (ax < STICK_DEADZONE && ay < STICK_DEADZONE) return 0;
        if (ax > ay * STICK_DOMINANCE) return (x > 0) ? 2 : 1;
        if (ay > ax * STICK_DOMINANCE) return (y > 0) ? 4 : 3;
        return 0;
    };

    uint8_t dir = 0;
    uint8_t ld = stickDir(lx, ly, absLX, absLY);
    uint8_t rd = stickDir(rx, ry, absRX, absRY);
    if (ld) dir = ld;
    if (rd) dir = rd;

    bool stickUp = (dir == 3), stickDown = (dir == 4);
    bool stickLeft = (dir == 1), stickRight = (dir == 2);

    if (stickUp && !m_prevStickUp) {
        m_holdUpTime = 0.f;
        m_holdUpRepeat = 0.f;
        _moveUp();
    }
    if (stickUp) {
        m_holdUpTime += dt;
        if (m_holdUpTime > HOLD_INITIAL_DELAY) {
            m_holdUpRepeat += dt;
            float interval = m_holdUpTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdUpRepeat >= interval) {
                m_holdUpRepeat -= interval;
                if (_tryMoveUp()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevStickUp = stickUp;

    if (stickDown && !m_prevStickDown) {
        m_holdDownTime = 0.f;
        m_holdDownRepeat = 0.f;
        _moveDown();
    }
    if (stickDown) {
        m_holdDownTime += dt;
        if (m_holdDownTime > HOLD_INITIAL_DELAY) {
            m_holdDownRepeat += dt;
            float interval = m_holdDownTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdDownRepeat >= interval) {
                m_holdDownRepeat -= interval;
                if (_tryMoveDown()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevStickDown = stickDown;

    if (stickLeft && !m_prevStickLeft) {
        m_holdLeftTime = 0.f;
        m_holdLeftRepeat = 0.f;
        _moveLeft();
    }
    if (stickLeft) {
        m_holdLeftTime += dt;
        if (m_holdLeftTime > HOLD_INITIAL_DELAY) {
            m_holdLeftRepeat += dt;
            float interval = m_holdLeftTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdLeftRepeat >= interval) {
                m_holdLeftRepeat -= interval;
                if (_tryMoveLeft()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevStickLeft = stickLeft;

    if (stickRight && !m_prevStickRight) {
        m_holdRightTime = 0.f;
        m_holdRightRepeat = 0.f;
        _moveRight();
    }
    if (stickRight) {
        m_holdRightTime += dt;
        if (m_holdRightTime > HOLD_INITIAL_DELAY) {
            m_holdRightRepeat += dt;
            float interval = m_holdRightTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdRightRepeat >= interval) {
                m_holdRightRepeat -= interval;
                if (_tryMoveRight()) {
                    m_focusMoved = true;
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
            }
        }
    }
    m_prevStickRight = stickRight;

    if (m_focusMoved) {
        m_selectedGameId = m_items[m_selectedIndex].gameId;
        if (m_focusChangeCallback) m_focusChangeCallback(m_selectedIndex);
    }

    bool aNow = state.buttons[static_cast<int>(brls::BUTTON_A)];
    if (aNow && !m_prevA && m_selectedIndex >= 0 && static_cast<size_t>(m_selectedIndex) < m_items.size()) {
        if (m_multiSelectMode)
            toggleDeleteSelection(m_selectedIndex);
        else if (m_dataSource)
            m_dataSource->onItemSelected(m_selectedIndex);
    }
    m_prevA = aNow;
}

void GameGridView::frame(brls::FrameContext* ctx)
{
    View::frame(ctx);

    if (!m_dataSource || m_items.empty()) return;

    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
    m_lastFrameTime = now;
    if (dt <= 0.f || dt > 0.5f) dt = 0.016f;

    _handleInput(dt);

    if (m_shakeTime > 0.f)
        m_shakeTime -= dt;

    if (m_focusMoved) {
        _ensureSelectedVisible();
        m_focusMoved = false;
    }

    _updateScrollPhysics(dt);
    _updateVisibleRange();
    _updateFocusAnimation(dt);
    _updateMarquee(dt);

    NVGcontext* vg = brls::Application::getNVGContext();
    _loadTextures(vg);
    _evictTextures();

    if (!m_requestNextPage && m_selectedIndex >= 0 &&
        static_cast<size_t>(m_selectedIndex) >= m_items.size() - static_cast<size_t>(spanCount) * 2 &&
        m_items.size() > 0) {
        if (m_nextPageCallback) {
            m_requestNextPage = true;
            m_nextPageCallback();
        }
    }
}

NVGcolor GameGridView::_getBadgeColor(PlatformBadgeColor color) const
{
    switch (color) {
        case PlatformBadgeColor::GBA: return nvgRGBA(108, 77,  191, 220);
        case PlatformBadgeColor::GBC: return nvgRGBA(0,   112, 221, 220);
        case PlatformBadgeColor::GB:  return nvgRGBA(0,   168, 107, 220);
        default:                      return nvgRGBA(100, 100, 100, 200);
    }
}

void GameGridView::draw(NVGcontext* vg, float x, float y, float w, float h,
                         brls::Style style, brls::FrameContext* ctx)
{
    if (!vg) return;
    if (!m_dataSource || m_items.empty()) return;

    nvgSave(vg);
    nvgIntersectScissor(vg, x, y, w, h);

    float itemW = _getItemWidth();
    float itemH = estimatedRowHeight;
    int startCol = 0;
    int startRow = m_visibleStartRow;
    int endRow = m_visibleEndRow;

    for (int row = startRow; row < endRow; row++) {
        for (int col = startCol; col < spanCount; col++) {
            int idx = row * spanCount + col;
            if (idx >= static_cast<int>(m_items.size())) break;

            const auto& item = m_items[idx];
            float ix = x + _getItemX(col);
            float iy = y + _getItemY(row);

            if (iy + itemH < y - 20.f || iy > y + h + 20.f) continue;

            bool focused = (idx == m_selectedIndex);
            _drawItem(vg, item, ix, iy, itemW, itemH, focused, idx);
        }
    }

    _drawScrollbar(vg, x+1, y, w, h);

    nvgResetScissor(vg);
    nvgRestore(vg);
}

void GameGridView::_drawItem(NVGcontext* vg, const GridDrawItem& item, float x, float y, float w, float h, bool focused, int idx)
{
    nvgSave(vg);

    float shakeX = 0.f;
    float shakeY = 0.f;
    if (focused && m_shakeTime > 0.f && m_shakeDir != 0.f) {
        float t = m_shakeTime / 0.3f;
        float decay = t * t;
        float freq = 80.f;
        float amount = std::sin(m_shakeTime * freq) * 6.f * decay;
        float adir = std::abs(m_shakeDir);
        if (adir < 1.5f)
            shakeY = amount * m_shakeDir;
        else
            shakeX = amount * (m_shakeDir > 0.f ? 1.f : -1.f);
    }

    if (focused && item.focusGlow > 0.01f) {
        float glowAlpha = item.focusGlow * 0.6f;
        float glowPad = 3.f;
        float gx = x + shakeX;
        float gy = y + shakeY;

        // 主高亮边框
        nvgBeginPath(vg);
        nvgRoundedRect(
            vg,
            gx,
            gy,
            w,
            h,
            4.f
        );

        nvgStrokeColor(
            vg,
            nvgRGBA(
                0,
                122,
                255,
                255
            )
        );

        nvgStrokeWidth(vg, 5.0f);
        nvgStroke(vg);
    }

    nvgBeginPath(vg);
    nvgRoundedRect(vg, x + shakeX, y + shakeY, w, h, 3.f);
    nvgFillColor(vg, nvgRGBA(42, 42, 42, 30));
    nvgFill(vg);
    
    nvgStrokeColor(vg, item.favorite ? nvgRGBA(224, 166, 87, 255) : nvgRGBA(110, 110, 110, 255));
    nvgStrokeWidth(vg, 1.0f);

    if (m_multiSelectMode) {
        bool sel = m_selectedForDelete.count(idx);
        nvgStrokeColor(vg, sel ? nvgRGBA(255, 60, 60, 255) : nvgRGBA(255, 255, 255, 220));
    } else {
        nvgStrokeColor(vg, item.favorite ? nvgRGBA(224, 166, 87, 255) : nvgRGBA(110, 110, 110, 255));
    }
    nvgStroke(vg);
    

    if (item.favorite)
        _drawFavourite(vg, item, x, y, w, h, shakeX, shakeY);
    


    if (item.empty) {
        _drawEmptyItem(vg, x, y, w, h);
    } else {
        float imageSize = h - 10.f;
        float imageX = x + 5.f;
        float imageY = y + 5.f;

        _drawImage(vg, item, imageX, imageY, imageSize);

        float textX = imageX + imageSize + 10.f;
        float textMaxWidth = imageSize * 1.8f;

        float titleY = y + 22.f;
        _drawBadge(vg, item, textX, titleY-3);
        _drawTitle(vg, item, textX + 40, titleY, textMaxWidth, focused);

        float playY = y + 50.f;
        _drawPlayTime(vg, item.playTime, textX, playY, textMaxWidth);

        float subY = playY + 25.f;
        _drawSubText(vg, item.subText, textX, subY, textMaxWidth);
    }

    nvgRestore(vg);
}

void GameGridView::_drawImage(NVGcontext* vg, const GridDrawItem& item, float x, float y, float imageSize)
{
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, imageSize, imageSize, 3.f);

    if (item.textureHandle >= 0 && item.textureReady) {
        NVGpaint paint = nvgImagePattern(vg, x, y, imageSize, imageSize, 0.f, item.textureHandle, 1.f);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
    } else {
        nvgFillColor(vg, nvgRGBA(60, 60, 60, 200));
        nvgFill(vg);
    }

    nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 150));
    nvgStrokeWidth(vg, 0.5f);
    nvgStroke(vg);
}

void GameGridView::_drawBadge(NVGcontext* vg, const GridDrawItem& item, float x, float y)
{
    if (item.badgeText.empty() || item.badgeColor == PlatformBadgeColor::NONE) return;

    float badgeW = 36.f;
    float badgeH = 20.f;

    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, badgeW, badgeH, 4.f);
    nvgFillColor(vg, _getBadgeColor(item.badgeColor));
    nvgFill(vg);

    nvgFontSize(vg, 12.f);
    nvgFontFaceId(vg, m_fontId);
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
    nvgText(vg, x + badgeW * 0.5f, y + badgeH * 0.5f, item.badgeText.c_str(), nullptr);
}

void GameGridView::_drawTitle(NVGcontext* vg, const GridDrawItem& item, float x, float y, float maxWidth, bool focused)
{
    nvgFontSize(vg, m_titleFontSize);
    nvgFontFaceId(vg, m_fontId);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

    nvgSave(vg);
    nvgIntersectScissor(vg, x, y - 2.f, maxWidth, 20.f);

    if (focused && item.marqueeMaxOffset > 0.f) {
        nvgText(vg, x - item.marqueeOffset, y, item.title.c_str(), nullptr);
    } else {
        nvgText(vg, x, y, item.title.c_str(), nullptr);
    }

    nvgRestore(vg);
}

void GameGridView::_drawSubText(NVGcontext* vg, const std::string& text, float x, float y, float maxWidth)
{
    if (text.empty()) return;

    nvgFontSize(vg, 14.f);
    nvgFontFaceId(vg, m_fontId);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGBA(130, 130, 130, 255));

    nvgText(vg, x, y, text.c_str(), nullptr);
}

void GameGridView::_drawPlayTime(NVGcontext* vg, const std::string& text, float x, float y, float maxWidth)
{
    if (text.empty()) return;

    nvgFontSize(vg, 14.f);
    nvgFontFaceId(vg, m_fontId);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGBA(121, 201, 249, 255));

    nvgText(vg, x, y, text.c_str(), nullptr);
}

void GameGridView::_drawEmptyItem(NVGcontext* vg, float x, float y, float w, float h)
{
    nvgFontSize(vg, 16.f);
    nvgFontFaceId(vg, m_fontId);
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGBA(150, 150, 150, 200));
    nvgText(vg, x + w * 0.5f, y + h * 0.5f, "空", nullptr);
}

void GameGridView::_drawFavourite(NVGcontext* vg, const GridDrawItem& item, float x, float y, float w, float h, float sx, float sy)
{
    if (m_favIconHandle < 0)
    {
        std::string path = BK_RES("img/ui/light/fa.png");
        m_favIconHandle = nvgCreateImage(
            vg,
            path.c_str(),
            NVG_IMAGE_PREMULTIPLIED
        );
        if (m_favIconHandle < 0)
            return;
    }
    float iconSize = 100.f;
    float pad = 1.f;

    float ix = x + w - iconSize - pad + sx;
    float iy = y + h - iconSize - pad + sy;
    nvgSave(vg);
    nvgIntersectScissor(
        vg,
        x + sx,
        y + sy,
        w,
        h
    );
    // 图片纹理
    NVGpaint paint = nvgImagePattern(
        vg,
        ix,
        iy,
        iconSize,
        iconSize,
        0.f,
        m_favIconHandle,
        0.2f
    );
    // 绘制透明PNG
    nvgBeginPath(vg);
    nvgRoundedRect(vg, ix, iy, iconSize, iconSize, 3.f);

    nvgFillPaint(vg, paint);
    nvgFill(vg);

    nvgRestore(vg);
}

void GameGridView::_drawScrollbar(NVGcontext* vg, float x, float y, float w, float h)
{
    if (m_maxScrollY <= 0.f) return;

    float totalH = _getRowCount() * _getRowHeight() + m_paddingTop;
    if (totalH <= 0.f) return;

    float barH = h * (h / totalH);
    barH = std::max(barH, 20.f);
    float barY = y + (m_scrollY / m_maxScrollY) * (h - barH);
    float barX = x + w - 5.f;
    float barW = 3.f;

    nvgBeginPath(vg);
    nvgRoundedRect(vg, barX, barY, barW, barH, 1.5f);
    nvgFillColor(vg, nvgRGBA(180, 180, 180, 120));
    nvgFill(vg);
}
