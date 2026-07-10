#pragma once

#include "network/netplay/NetplayManager.hpp"
#include "ui/widget/Box.hpp"

#include <array>
#include <string>
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
    enum class PageMode
    {
        Landing,
        HostLibrary,
        HostWaiting,
        JoinRooms,
    };

    enum class SetupPurpose
    {
        None,
        Host,
        Join,
    };

    enum class FocusZone
    {
        Landing,
        Setup,
        Tabs,
        Grid,
        Rooms,
        Waiting,
    };

    enum class SortMode : int
    {
        LastPlayed = 0,
        PlayTime,
        FirstLetter,
    };

    struct PlatformTab
    {
        const char* label;
        beiklive::enums::EmuPlatform platform;
    };

    void refreshEntries();
    void rebuildVisibleEntries();
    void registerPageActions();

    void activateCurrent();
    void moveSelection(int delta);
    void moveHorizontal(int delta);
    void openProfileSetup(SetupPurpose purpose);
    void closeProfileSetup();
    void confirmProfileSetup();
    void openNameEditor();
    void openSearchEditor();
    void openManualJoinEditor();
    void cycleSortMode();
    void selectGameForHosting();
    void joinSelectedRoom();
    void joinManualEndpoint(const std::string& endpoint);
    void approvePendingJoin();
    void rejectPendingJoin();
    void launchNetplayGameIfReady();
    bool findNetplayGame(beiklive::GameEntry& out) const;
    void closePage();

    std::string currentPlatformName() const;
    std::string sortModeName() const;
    std::string displayTitle(const beiklive::GameEntry& entry) const;
    int currentPlatformInt() const;
    int gridColumnCount() const { return 2; }
    int visibleGridRows(float h) const;
    int visibleRoomRows(float h) const;
    void ensureGridVisible(float h);
    void ensureRoomVisible(float h);

    void drawText(NVGcontext* vg, const std::string& text, float x, float y,
                  float size, NVGcolor color, int align) const;
    void drawRoundedRect(NVGcontext* vg, float x, float y, float w, float h,
                         float radius, NVGcolor fill, NVGcolor stroke,
                         float strokeWidth) const;
    void drawAvatar(NVGcontext* vg, float cx, float cy, float radius,
                    uint8_t avatar, const std::string& name, bool large) const;
    void drawHeader(NVGcontext* vg, float x, float y, float w) const;
    void drawLanding(NVGcontext* vg, float x, float y, float w, float h) const;
    void drawProfileBlock(NVGcontext* vg, float x, float y, float w, float h) const;
    void drawHostLibrary(NVGcontext* vg, float x, float y, float w, float h);
    void drawTabs(NVGcontext* vg, float x, float y, float w, float h) const;
    void drawGameGrid(NVGcontext* vg, float x, float y, float w, float h);
    void drawRooms(NVGcontext* vg, float x, float y, float w, float h);
    void drawWaiting(NVGcontext* vg, float x, float y, float w, float h) const;
    void drawSetupModal(NVGcontext* vg, float x, float y, float w, float h) const;
    void drawConnectingModal(NVGcontext* vg, float x, float y, float w, float h) const;
    void drawFooter(NVGcontext* vg, float x, float y, float w, float h) const;

    static std::string titleToSortKey(const std::string& title);

    std::vector<beiklive::GameEntry> m_allEntries;
    std::vector<beiklive::GameEntry> m_visibleEntries;
    beiklive::netplay::NetplaySnapshot m_snapshot;

    PageMode m_mode = PageMode::Landing;
    FocusZone m_focus = FocusZone::Landing;
    SetupPurpose m_setupPurpose = SetupPurpose::None;
    bool m_profileSetupOpen = false;

    std::string m_pendingName;
    uint8_t m_pendingAvatar = 0;
    int m_setupIndex = 0;

    int m_landingIndex = 0;
    int m_tabIndex = 0;
    int m_gridIndex = 0;
    int m_gridScrollRow = 0;
    int m_roomIndex = 0;
    int m_roomScroll = 0;
    int m_waitingActionIndex = 0;

    SortMode m_sortMode = SortMode::LastPlayed;
    std::string m_searchTerm;
    bool m_isSearching = false;
    std::string m_manualEndpoint = "192.168.1.10:45872";
    bool m_manualConnecting = false;
    bool m_gameLaunchRequested = false;

    int m_font = -1;
    mutable float m_lastGridHeight = 360.f;
    mutable float m_lastRoomHeight = 360.f;

    static constexpr std::array<PlatformTab, 3> kTabs = {{
        {"GBA", beiklive::enums::EmuPlatform::EmuGBA},
        {"GBC", beiklive::enums::EmuPlatform::EmuGBC},
        {"GB",  beiklive::enums::EmuPlatform::EmuGB},
    }};
};

} // namespace beiklive
