#pragma once

#include <borealis.hpp>
#include "core/common.h"
namespace beiklive
{

    /// 简化版底部提示栏：只包含按键提示列表，无时间和系统图标
    class HintsBar : public brls::Box
    {
    public:
        HintsBar();
        ~HintsBar() = default;

    private:
        brls::Hints* m_hints = nullptr;
    };

} // namespace beiklive
