#include "LayoutManager.hpp"

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
        // 指针失效，重置焦点单元格
        m_focus.resetToFirst();
    }

    void LayoutManager::applyFocusChange(LayoutItem* oldFocus,
                                         LayoutItem* newFocus)
    {
        if (oldFocus == newFocus)
            return;
        if (oldFocus && oldFocus->widget)
            oldFocus->widget->onBlur();
        if (newFocus && newFocus->widget)
            newFocus->widget->onFocus();
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
                targetX = (targetX % cfg.columns + cfg.columns) % cfg.columns;
                break;
            }
            case UIAction::Right: {
                if (oldItem && targetX < oldItem->x + oldItem->w - 1)
                    targetX = oldItem->x + oldItem->w;
                else
                    targetX += 1;
                targetX = (targetX % cfg.columns + cfg.columns) % cfg.columns;
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
        applyFocusChange(oldItem, currentItem());
    }

    void LayoutManager::resetFocusToFirst()
    {
        LayoutItem* oldItem = currentItem();
        m_focus.resetToFirst();
        applyFocusChange(oldItem, currentItem());
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
            }
        }

        // 焦点框：流光渐变描边（Item 覆盖时框住整体，空白格框住单元格）
        if (m_focusVisible) {
            const GridRect focusRect = focused
                ? m_grid.getItemRect(*focused)
                : m_grid.getItemRect(m_focus.cellX(), m_focus.cellY(), 1, 1);
            beiklive::ui::drawGradientFocusBorder(
                vg, focusRect.left - 1.f, focusRect.top - 1.f,
                focusRect.width + 2.f, focusRect.height + 2.f,
                radius, 6.f, 1.f,
                beiklive::ui::gradientFocusAnimationOffset(time));
        }
    }
} // namespace beiklive
