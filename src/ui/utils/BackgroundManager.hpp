#pragma once

#include <borealis.hpp>
#include "ui/widget/DynamicBackgroundBox.hpp"

namespace beiklive
{

    class BackgroundManager
    {
    public:
        static BackgroundManager& instance();

        void init(brls::Box* hostBox);
        void showBackground(bool show);
        void showShader(bool show);
        void setGradientTheme(GradientTheme theme);

    private:
        BackgroundManager() = default;
        bool m_inited = false;
        brls::Image*                    m_bgImage = nullptr;
        beiklive::DynamicBackgroundBox* m_shader  = nullptr;
        brls::Box*                      m_host    = nullptr;

        void ensureLayersCreated();
    };

} // namespace beiklive
