#pragma once

#include "network/netplay/NetplayManager.hpp"
#include "ui/widget/Box.hpp"

#include <vector>

namespace beiklive
{

class MultiplayerPage : public beiklive::Box
{
public:
    MultiplayerPage();
    ~MultiplayerPage() override = default;

    void willAppear(bool resetState) override;
    void frame(brls::FrameContext* ctx) override;
    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

private:
    enum class Panel
    {
        Actions,
        Rooms,
        Games,
    };

    struct ActionItem
    {
        std::string title;
        std::string subtitle;
    };

    void refreshGames();
    void registerPageActions();
    void activateCurrent();
    void moveSelection(int delta);
    void switchPanel(Panel panel);
    void closePage();

    void drawText(NVGcontext* vg, const std::string& text, float x, float y,
                  float size, NVGcolor color, int align) const;
    void drawRoundedRect(NVGcontext* vg, float x, float y, float w, float h,
                         float radius, NVGcolor fill, NVGcolor stroke,
                         float strokeWidth) const;
    void drawHeader(NVGcontext* vg, float x, float y, float w) const;
    void drawActions(NVGcontext* vg, float x, float y, float w, float h) const;
    void drawRooms(NVGcontext* vg, float x, float y, float w, float h) const;
    void drawPlayers(NVGcontext* vg, float x, float y, float w, float h) const;
    void drawGames(NVGcontext* vg, float x, float y, float w, float h) const;
    void drawFooter(NVGcontext* vg, float x, float y, float w, float h) const;

    std::vector<beiklive::GameEntry> m_gbaGames;
    std::vector<ActionItem> m_actions;
    beiklive::netplay::NetplaySnapshot m_snapshot;
    Panel m_panel = Panel::Actions;
    int m_actionIndex = 0;
    int m_roomIndex = 0;
    int m_gameIndex = 0;
    int m_font = -1;
};

} // namespace beiklive
