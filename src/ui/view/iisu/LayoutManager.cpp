#include "LayoutManager.hpp"

#include <algorithm>
#include <cmath>

#include "AnimationManager.hpp"
#include "Easing.hpp"
#include "ui/utils/GradientFocus.hpp"
#include "Widget.hpp"

namespace beiklive
{
    void LayoutManager::addItem(const LayoutItem& item)
    {
        // 注意：vector 扩容会使 LayoutManager 持有的 LayoutItem* 失效，
        // 添加元素后应通过 resetFocusToFirst() 重建焦点
        m_items.push_back(item);
    }

    void LayoutManager::removeItem(size_t index)
    {
        if (index >= m_items.size())
            return;
        m_items.erase(m_items.begin() + static_cast<std::ptrdiff_t>(index));
    }

    void LayoutManager::clear()
    {
        m_items.clear();
        // 指针失效，重置焦点单元格并终止引用旧指针的动画
        m_focus.resetToFirst();
        if (m_animations)
            m_animations->clear();
    }

    void LayoutManager::applyFocusChange(LayoutItem* oldFocus,
                                         LayoutItem* newFocus)
    {
        if (oldFocus == newFocus)
            return;
        if (oldFocus) {
            if (oldFocus->widget)
                oldFocus->widget->onBlur();
            animateItemScale(oldFocus, 1.f);
        }
        if (newFocus) {
            if (newFocus->widget)
                newFocus->widget->onFocus();
            animateItemScale(newFocus, 1.04f);
        }
    }

    void LayoutManager::animateItemScale(LayoutItem* item,
                                         float targetScale)
    {
        if (!m_animations || !item)
            return;
        const float from = item->transform.scale;
        if (std::abs(from - targetScale) < 0.001f)
            return;
        m_animations->add(Animation{
            0.18f, 0.f,
            [item, from, targetScale](float t) {
                item->transform.scale =
                    from + (targetScale - from) * easing::easeOutCubic(t);
            },
            nullptr, false});
    }

    void LayoutManager::setFocusShake(float dx, float dy)
    {
        m_focusShakeActive = true;
        m_focusShakeTime = 0.f;
        m_focusShakeDirX = dx;
        m_focusShakeDirY = dy;
    }

    void LayoutManager::moveFocus(UIAction action)
    {
        LayoutItem* oldItem = currentItem();
        const GridConfig& cfg = m_grid.config();
        if (cfg.columns <= 0 || cfg.rows <= 0)
            return;

        int targetX = m_focus.cellX();
        int targetY = m_focus.cellY();

        switch (action) {
            case UIAction::Left: {
                // 焦点在跨格 Item 内时整体跳出，否则单格移动
                if (oldItem && targetX > oldItem->x)
                    targetX = oldItem->x - 1;
                else
                    targetX -= 1;
                if (m_grid.scrollable()) {
                    if (targetX < 0)
                        targetX = 0; // 滚动网格不换行
                } else {
                    targetX = (targetX % cfg.columns + cfg.columns) % cfg.columns;
                }
                break;
            }
            case UIAction::Right: {
                if (oldItem && targetX < oldItem->x + oldItem->w - 1)
                    targetX = oldItem->x + oldItem->w;
                else
                    targetX += 1;
                if (m_grid.scrollable()) {
                    if (targetX >= cfg.columns)
                        targetX = cfg.columns - 1; // 滚动网格不换行
                } else {
                    targetX = (targetX % cfg.columns + cfg.columns) % cfg.columns;
                }
                break;
            }
            case UIAction::Up: {
                if (oldItem && targetY > oldItem->y)
                    targetY = oldItem->y - 1;
                else
                    targetY -= 1;
                if (targetY < 0)
                    targetY = 0;
                break;
            }
            case UIAction::Down: {
                if (oldItem && targetY < oldItem->y + oldItem->h - 1)
                    targetY = oldItem->y + oldItem->h;
                else
                    targetY += 1;
                if (targetY >= cfg.rows)
                    targetY = cfg.rows - 1;
                break;
            }
            default:
                break;
        }

        m_focus.setCell(targetX, targetY, cfg.columns, cfg.rows);
        scrollToFocus();
        applyFocusChange(oldItem, currentItem());
    }

    void LayoutManager::resetFocusToFirst()
    {
        LayoutItem* oldItem = currentItem();
        m_focus.resetToFirst();
        scrollToFocus();
        applyFocusChange(oldItem, currentItem());
    }

    void LayoutManager::scrollToFocus()
    {
        if (!m_grid.scrollable())
            return;
        const float viewW = m_grid.viewWidth();
        if (viewW <= 0.f)
            return;
        const GridRect rect = m_grid.getItemRect(
            m_focus.cellX(), m_focus.cellY(), 1, 1);
        float target = m_grid.scrollX();
        if (rect.left < 0.f)
            target += rect.left; // 焦点露出左边界
        else if (rect.left + rect.width > viewW)
            target += rect.left + rect.width - viewW; // 焦点露出右边界
        target = std::max(0.f, std::min(target, m_grid.maxScrollX()));
        m_grid.setScrollX(target);
    }

    LayoutItem* LayoutManager::currentItem()
    {
        const int fx = m_focus.cellX();
        const int fy = m_focus.cellY();
        for (auto& item : m_items) {
            if (!item.visible)
                continue;
            if (fx >= item.x && fx < item.x + item.w &&
                fy >= item.y && fy < item.y + item.h)
                return &item;
        }
        return nullptr;
    }

    void LayoutManager::update(float delta)
    {
        if (m_focusShakeActive) {
            m_focusShakeTime += delta;
            if (m_focusShakeTime >= 0.35f)
                m_focusShakeActive = false;
        }
        for (auto& item : m_items) {
            if (item.widget)
                item.widget->update(delta);
        }
    }

    void LayoutManager::drawCard(NVGcontext* vg, const GridRect& rect,
                                 float radius)
    {
        // 阴影（参考 switch 布局游戏卡片）
        NVGpaint cardShadow = nvgBoxGradient(
            vg, rect.left + 4.f, rect.top + 5.f,
            rect.width, rect.height, radius, 5.f,
            nvgRGBA(0, 0, 0, 82), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, rect.left - 1.f, rect.top,
                rect.width + 10.f, rect.height + 11.f);
        nvgRoundedRect(vg, rect.left, rect.top,
                       rect.width, rect.height, radius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, cardShadow);
        nvgFill(vg);

        // 卡片边缘
        nvgBeginPath(vg);
        nvgRoundedRect(vg, rect.left, rect.top,
                       rect.width, rect.height, radius);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 10));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 70));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
    }

    void LayoutManager::draw(NVGcontext* vg, float time)
    {
        if (!vg)
            return;

        const float radius = m_grid.config().radius;
        LayoutItem* focused = currentItem();

        for (const auto& item : m_items) {
            if (!item.visible)
                continue;

            // Grid → Pixel
            const GridRect rect = m_grid.getItemRect(item);

            if (item.widget) {
                nvgSave(vg);
                applyItemTransform(vg, item, rect);

                // 设置过的格子：卡片背景（阴影 + 边缘）
                drawCard(vg, rect, radius);

                // 内容到格子边缘 5px 边距
                constexpr float inset = 5.f;
                GridRect content = rect;
                content.left += inset;
                content.top += inset;
                content.width -= inset * 2.f;
                content.height -= inset * 2.f;
                item.widget->draw(vg, content);

                nvgRestore(vg);
            }
        }

        // 焦点框：流光渐变描边（框在格子与内容之间的边距缝内）
        if (m_focusVisible) {
            constexpr float inset = 5.f; // 与内容边距一致
            const GridRect focusRect = focused
                ? m_grid.getItemRect(*focused)
                : m_grid.getItemRect(m_focus.cellX(), m_focus.cellY(), 1, 1);

            // 移动被阻挡时向移动方向抖动
            float shakeX = 0.f;
            float shakeY = 0.f;
            if (m_focusShakeActive) {
                const float k = 1.f - m_focusShakeTime / 0.35f;
                const float s = std::sin(m_focusShakeTime * 60.f) * k * 6.f;
                shakeX = m_focusShakeDirX * s;
                shakeY = m_focusShakeDirY * s;
            }

            const float fx = focusRect.left + inset + shakeX;
            const float fy = focusRect.top + inset + shakeY;
            const float fw = focusRect.width - inset * 2.f;
            const float fh = focusRect.height - inset * 2.f;
            if (fw > 0.f && fh > 0.f) {
                nvgSave(vg);
                if (focused)
                    applyItemTransform(vg, *focused, focusRect);
                // 焦点框圆角比卡片/内容小
                beiklive::ui::drawGradientFocusBorder(
                    vg, fx, fy, fw, fh,
                    12.f, 5.f, 1.f,
                    beiklive::ui::gradientFocusAnimationOffset(time));
                nvgRestore(vg);
            }
        }
    }

    void LayoutManager::applyItemTransform(NVGcontext* vg,
                                           const LayoutItem& item,
                                           const GridRect& rect) const
    {
        const Transform& tf = item.transform;
        if (tf.scale != 1.f) {
            const float cx = rect.left + rect.width * 0.5f;
            const float cy = rect.top + rect.height * 0.5f;
            nvgTranslate(vg, cx, cy);
            nvgScale(vg, tf.scale, tf.scale);
            nvgTranslate(vg, -cx, -cy);
        }
        if (tf.offsetX != 0.f || tf.offsetY != 0.f)
            nvgTranslate(vg, tf.offsetX, tf.offsetY);
        if (tf.alpha < 1.f)
            nvgGlobalAlpha(vg, tf.alpha);
    }
} // namespace beiklive
