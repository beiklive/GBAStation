#pragma once

#include "core/common.h"
#include <borealis.hpp>

namespace beiklive
{
    class HeaderBar : public brls::Box
    {
    public:
        HeaderBar();
        void setTitle(const std::string &title);
        void setTitleWidth(float width);
        void setPath(const std::string &path);
        void setInfo(const std::string &info);
        std::string getPath() const;

    private:
        brls::Label *m_titleLabel = nullptr;
        brls::Label *m_pathLabel = nullptr;
        brls::Label *m_infoLabel = nullptr;

        brls::Box *m_titleBox = nullptr;
        brls::Box *m_subtitleBox = nullptr;
    };

} // namespace beiklive
