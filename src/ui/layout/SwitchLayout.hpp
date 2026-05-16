#pragma once

#include <borealis.hpp>
#include <atomic>
#include "core/common.h"
#include "Layout.hpp"
#include "ui/utils/GameCard.hpp"
#include "ui/utils/RoundButton.hpp"

namespace beiklive
{
    class SwitchLayout : public beiklive::Layout
    {
    public:
        SwitchLayout();
        ~SwitchLayout() = default;

        void refreshGameList(beiklive::GameList gameList) override;
        brls::Box* getContentBox() { return m_cardRow; }
        void buildCardRow(beiklive::GameList gameList);
        void buildFunctionArea();
        void _buildEmptyCards();
    private:
        brls::HScrollingFrame* m_frame;
        brls::Box* m_cardRow;

        brls::Box* m_functionArea;

        std::atomic<int> m_loadGen{0};
    };
} // namespace beiklive
