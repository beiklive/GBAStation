#pragma once

#include "core/common.h"
#include "ui/utils/Box.hpp"

namespace beiklive
{
    class GameImportView : public beiklive::Box
    {
    public:
        GameImportView();
        ~GameImportView();

    private:
        void setupButtonLayout();
        void setupProgressLayout();
        void showButtonLayout();
        void showProgressLayout();

        void onSelectLpl(int platform);
        void startImport(const std::string& lplPath, int platform);

        static std::string platformName(int platform);
        static std::string platformDirName(int platform);

        brls::Box* m_layoutBox = nullptr;
        brls::Box* m_progressBox = nullptr;
        brls::Label* m_progressTitleLabel = nullptr;
        brls::Label* m_progressCountLabel = nullptr;
        brls::Rectangle* m_progressBar = nullptr;

        bool m_importing = false;
        int m_totalItems = 0;
        int m_importedCount = 0;
    };
}
