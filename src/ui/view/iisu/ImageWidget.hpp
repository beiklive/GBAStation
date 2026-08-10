#pragma once

#include <string>

#include "Widget.hpp"

namespace beiklive
{
    /// 图片 Widget：显示一张图片（圆角裁剪 + 等比覆盖居中）
    class ImageWidget : public Widget
    {
    public:
        explicit ImageWidget(std::string path);
        ~ImageWidget() override;

        void setPath(const std::string& path);

        void draw(NVGcontext* vg, const GridRect& rect) override;

        std::string typeName() const override { return "image"; }
        std::string dataId() const override { return m_path; }

    private:
        std::string m_path;
        int m_textureId = 0;
        bool m_textureRequested = false;
    };
} // namespace beiklive
