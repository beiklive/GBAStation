#pragma once

#include <borealis.hpp>
#include <string>

namespace beiklive
{
    class ImageView : public brls::Box
    {
    public:
        explicit ImageView(const std::string& imagePath = "");
        ~ImageView() override = default;

        void setImagePath(const std::string& imagePath);
        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;
        void frame(brls::FrameContext* ctx) override;

    private:
        brls::Image* m_image = nullptr;
        std::string m_imagePath;
        float m_zoom = 1.0f;
        float m_offsetX = 0.0f;
        float m_offsetY = 0.0f;

        void _updateImageLayout();
        void _resetView();
    };
}
