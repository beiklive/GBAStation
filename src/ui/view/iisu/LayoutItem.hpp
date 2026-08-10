#pragma once

#include <cstdint>
#include <memory>

namespace beiklive
{
    class Widget; // 前向声明，LayoutManager 中获取完整类型

    /// 布局元素：唯一 id + 网格坐标 + 占用格子数 + 内容 Widget
    struct LayoutItem
    {
        uint32_t id = 0;
        int x = 0; // 网格列
        int y = 0; // 网格行
        int w = 1; // 占用列数
        int h = 1; // 占用行数

        std::shared_ptr<Widget> widget = nullptr; // 内容组件
        bool visible = true;
        bool focusable = true; // 背景图片等不可聚焦元素设为 false
    };
} // namespace beiklive
