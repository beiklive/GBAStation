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
        LayoutItem* oldFocus = m_focus.current();
        switch (action) {
            case UIAction::Up:    m_focus.moveUp(m_items); break;
            case UIAction::Down:  m_focus.moveDown(m_items); break;
            case UIAction::Left:  m_focus.moveLeft(m_items); break;
            case UIAction::Right: m_focus.moveRight(m_items); break;
            default: break;
        }
        applyFocusChange(oldFocus, m_focus.current());
    }

    void LayoutManager::resetFocusToFirst()
    {
        LayoutItem* oldFocus = m_focus.current();
        m_focus.resetToFirst(m_items);
        applyFocusChange(oldFocus, m_focus.current());
    }

    void LayoutManager::update(float delta)
    {
        for (auto& item : m_items) {
            if (item.widget)
                item.widget->update(delta);
        }
    }

    void LayoutManager::draw(NVGcontext* vg, float time)
    {
        if (!vg)
            return;

        const float radius = m_grid.config().radius;
        LayoutItem* focused = m_focus.current();

        for (const auto& item : m_items) {
            if (!item.visible)
                continue;

            // Grid → Pixel
            const GridRect rect = m_grid.getItemRect(item);

            // Widget 负责内容绘制
            if (item.widget)
                item.widget->draw(vg, rect);

            // 焦点框：流光渐变描边
            if (&item == focused) {
                beiklive::ui::drawGradientFocusBorder(
                    vg, rect.left - 1.f, rect.top - 1.f,
                    rect.width + 2.f, rect.height + 2.f,
                    radius, 6.f, 1.f,
                    beiklive::ui::gradientFocusAnimationOffset(time));
            }
        }
    }
} // namespace beiklive
