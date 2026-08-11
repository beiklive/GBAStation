#pragma once

#include <cstddef>
#include <string>

#include "Widget.hpp"

namespace beiklive
{
    /// 图片 Widget：显示 PNG/JPG/GIF（圆角裁剪 + 等比覆盖居中，GIF 逐帧动画）
    class ImageWidget : public Widget
    {
    public:
        explicit ImageWidget(std::string path);
        ~ImageWidget() override;

        void setPath(const std::string& path);

        /// 播放速度倍率（1.0 = 文件原速，2.0 = 两倍速，0.5 = 半速）
        void setSpeed(float speed) { m_speedMul = speed; }
        float speed() const { return m_speedMul; }

        void update(float delta) override;
        void draw(NVGcontext* vg, const GridRect& rect) override;

        std::string typeName() const override { return "image"; }
        std::string dataId() const override { return m_path; }
        std::string displayName() override;

    private:
        std::string m_path;
        int m_textureId = 0;
        bool m_textureRequested = false;

        // GIF 动画状态
        float m_speedMul = 1.f;
        bool m_isGif = false;
        size_t m_gifFrame = 0;
        float m_gifTimeMs = 0.f;
    };
} // namespace beiklive
