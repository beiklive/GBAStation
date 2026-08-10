#include "FocusManager.hpp"

#include <cmath>

namespace beiklive
{
    LayoutItem* FocusManager::firstFocusable(
        std::vector<LayoutItem>& items)
    {
        for (auto& item : items)
            if (item.visible && item.focusable)
                return &item;
        return nullptr;
    }

    LayoutItem* FocusManager::findNearest(
        std::vector<LayoutItem>& items, const LayoutItem* current,
        int dx, int dy)
    {
        if (!current || items.empty())
            return nullptr;

        LayoutItem* best = nullptr;
        float bestScore = std::numeric_limits<float>::max();

        const float curCx =
            static_cast<float>(current->x) + current->w * 0.5f;
        const float curCy =
            static_cast<float>(current->y) + current->h * 0.5f;

        for (auto& item : items) {
            if (&item == current || !item.visible || !item.focusable)
                continue;

            const float cx = static_cast<float>(item.x) + item.w * 0.5f;
            const float cy = static_cast<float>(item.y) + item.h * 0.5f;
            const float vx = cx - curCx;
            const float vy = cy - curCy;

            // 必须位于目标方向上
            if (dx != 0 && vx * static_cast<float>(dx) <= 0.f)
                continue;
            if (dy != 0 && vy * static_cast<float>(dy) <= 0.f)
                continue;

            // 主方向距离优先，垂直方向偏移作为惩罚
            const float primary = dx != 0 ? std::abs(vx) : std::abs(vy);
            const float secondary = dx != 0 ? std::abs(vy) : std::abs(vx);
            const float score = primary + secondary * 2.f;
            if (score < bestScore) {
                best = &item;
                bestScore = score;
            }
        }
        return best;
    }

    LayoutItem* FocusManager::move(std::vector<LayoutItem>& items,
                                   int dx, int dy)
    {
        if (LayoutItem* target = findNearest(items, m_focused, dx, dy))
            m_focused = target;
        else if (!m_focused)
            m_focused = firstFocusable(items);
        return m_focused;
    }

    LayoutItem* FocusManager::moveLeft(std::vector<LayoutItem>& items)
    {
        return move(items, -1, 0);
    }

    LayoutItem* FocusManager::moveRight(std::vector<LayoutItem>& items)
    {
        return move(items, 1, 0);
    }

    LayoutItem* FocusManager::moveUp(std::vector<LayoutItem>& items)
    {
        return move(items, 0, -1);
    }

    LayoutItem* FocusManager::moveDown(std::vector<LayoutItem>& items)
    {
        return move(items, 0, 1);
    }

    LayoutItem* FocusManager::resetToFirst(
        std::vector<LayoutItem>& items)
    {
        m_focused = firstFocusable(items);
        return m_focused;
    }
} // namespace beiklive
