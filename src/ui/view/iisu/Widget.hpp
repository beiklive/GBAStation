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
        /// 确认键激活（FolderWidget 展开子布局等）
        virtual void onActivate() {}

        /// 注入资源管理器（由 UIContext 统一提供）
        void setTextureManager(TextureManager* manager) { m_textures = manager; }
        /// 内部元素圆角（默认比格子圆角小一点，由 UIContext 设置）
        void setCornerRadius(float radius) { m_radius = radius; }

        /// 组件类型标识（布局 JSON 用）
        virtual std::string typeName() const { return "empty"; }
        /// 组件数据标识（gameId / folderId / 图片路径 / live widgetId）
        virtual std::string dataId() const { return {}; }
        /// 显示名称（封面为游戏标题，图片为文件名等）
        virtual std::string displayName() { return dataId(); }

    protected:
        TextureManager* m_textures = nullptr;
        float m_radius = 14.f;
    };
} // namespace beiklive
