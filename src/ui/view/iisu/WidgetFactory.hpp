#pragma once

#include <memory>
#include <string>

#include "ColorWidget.hpp"
#include "ImageWidget.hpp"
#include "Widget.hpp"

namespace beiklive
{
    /// Widget 工厂：按类型创建组件（替代 LayoutManager 里的 if(type==...) 分支）
    class WidgetFactory
    {
    public:
        static std::shared_ptr<Widget> create(WidgetType type);

        /// 调试用：纯色块组件（临时验证用）
        static std::shared_ptr<Widget> createColor(NVGcolor color)
        {
            return std::make_shared<ColorWidget>(color);
        }

        /// 图片组件
        static std::shared_ptr<Widget> createImage(std::string path)
        {
            return std::make_shared<ImageWidget>(std::move(path));
        }
    };
} // namespace beiklive
