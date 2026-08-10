#pragma once

#include <limits>
#include <vector>

#include "LayoutItem.hpp"

namespace beiklive
{
    /// 焦点管理器：方向键在 Tile 之间移动焦点（按最近目标寻找）
    class FocusManager
    {
    public:
        void setCurrent(LayoutItem* item) { m_focused = item; }
        LayoutItem* current() const { return m_focused; }

        LayoutItem* moveLeft(std::vector<LayoutItem>& items);
        LayoutItem* moveRight(std::vector<LayoutItem>& items);
        LayoutItem* moveUp(std::vector<LayoutItem>& items);
        LayoutItem* moveDown(std::vector<LayoutItem>& items);

        LayoutItem* resetToFirst(std::vector<LayoutItem>& items);

    private:
        LayoutItem* move(std::vector<LayoutItem>& items, int dx, int dy);
        static LayoutItem* findNearest(std::vector<LayoutItem>& items,
                                       const LayoutItem* current,
                                       int dx, int dy);
        static LayoutItem* firstFocusable(std::vector<LayoutItem>& items);

        LayoutItem* m_focused = nullptr;
    };
} // namespace beiklive
