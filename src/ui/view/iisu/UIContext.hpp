#pragma once

#include "LayoutManager.hpp"
#include "TextureManager.hpp"
#include "Widget.hpp"

namespace beiklive
{
    /// 统一上下文：所有 Widget 从 Context 获取服务（布局 / 资源）
    class UIContext
    {
    public:
        TextureManager& textures() { return m_textures; }
        const TextureManager& textures() const { return m_textures; }

        LayoutManager& layout() { return m_layout; }
        const LayoutManager& layout() const { return m_layout; }

        /// 添加布局元素并自动为 Widget 注入资源管理器
        void addItem(const LayoutItem& item)
        {
            m_layout.addItem(item);
            auto& items = m_layout.items();
            if (!items.empty() && items.back().widget)
                items.back().widget->setTextureManager(&m_textures);
        }

    private:
        TextureManager m_textures;
        LayoutManager m_layout;
    };
} // namespace beiklive
