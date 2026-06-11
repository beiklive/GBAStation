#pragma once

#include <borealis.hpp>
#include <string>
#include <vector>

namespace beiklive
{

    /// 自定义详情单元格：左边可选 Label 或 Image，右边可动态添加多个 Label/Image
    class DetailCell : public brls::Box
    {
    public:
        DetailCell();
        ~DetailCell() = default;

        // ── 左侧 ──

        /// 设置左侧文本（与 setLeftImage 互斥，后者优先）
        void setLeftText(const std::string& text);
        void setLeftTextSize(float size);
        void setLeftTextColor(NVGcolor color);

        /// 设置左侧图标（调用后隐藏左侧文本）
        void setLeftImage(const std::string& path);
        void setLeftImageSize(float w, float h);

        // ── 右侧（动态添加）──

        /// 添加右侧文本标签，返回 Label 指针供外部进一步设置样式
        brls::Label* addRightLabel(const std::string& text = "");

        /// 添加右侧图标，返回 Image 指针供外部进一步设置样式
        brls::Image* addRightImage(const std::string& path = "");

        /// 清空右侧所有子视图
        void clearRightViews();

        /// 便捷：设置右侧单个文本（清空原有内容后添加一个 Label）
        void setRightText(const std::string& text);

    private:
        brls::Label* m_leftLabel = nullptr;
        brls::Image* m_leftImage = nullptr;
        brls::Box*   m_rightBox  = nullptr;

        void _ensureRightBox();
    };

} // namespace beiklive
