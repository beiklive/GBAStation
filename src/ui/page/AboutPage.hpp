#pragma once

#include "core/common.h"
#include "ui/utils/Box.hpp"

namespace beiklive
{
    /// 关于页面：展示项目信息及作者信息
    class AboutPage : public beiklive::Box
    {
    public:
        AboutPage();

    private:
        brls::Image *m_authorImage = nullptr; ///< 作者圆形头像
        brls::Label *m_authorName  = nullptr; ///< 作者名称
        brls::Label *m_githubLabel = nullptr; ///< Github 链接
        brls::Label *m_biliLabel   = nullptr; ///< BiliBili 链接

        void buildUI();
    };
} // namespace beiklive
