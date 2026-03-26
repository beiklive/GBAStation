#pragma once
#include <borealis.hpp>

#include "core/common.h"

namespace beiklive
{

    class ListItemCell : public brls::RecyclerCell
    {
    public:
        ListItemCell();
        ~ListItemCell() = default;

        // 数据绑定
        void setTitle(const std::string& text);
        void setSubTitle(const std::string& text);
        void setIcon(const std::string& path);

        // 焦点
        void onFocusGained() override;
        void onFocusLost() override;

        static ListItemCell* create();





    private:
        brls::Label*     m_text    = nullptr;
        brls::Label*     m_subtext = nullptr;
        brls::Image*     m_icon    = nullptr;
        brls::Rectangle* m_accent  = nullptr;
    };

} // namespace beiklive
