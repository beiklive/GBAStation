#pragma once

#include <borealis.hpp>

namespace beiklive
{
    struct GridRect;
    class TextureManager;

    /// Widget 类型（后续 JSON 用字符串映射到 WidgetFactory::create）
    enum class WidgetType
    {
        Empty,
        Image,
        GameCover,
        Folder,
        Live,
        Gif,
    };

    /// Widget 基类：布局元素的内容载体，由 LayoutItem 持有
    class Widget
    {
    public:
        virtual ~Widget() = default;

        virtual void update(float delta) {}
        virtual void draw(NVGcontext* vg, const GridRect& rect) {}
        virtual void onFocus() {}
        virtual void onBlur() {}

        /// 注入资源管理器（由 UIContext 统一提供）
        void setTextureManager(TextureManager* manager) { m_textures = manager; }

    protected:
        TextureManager* m_textures = nullptr;
    };
} // namespace beiklive
