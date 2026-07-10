#include "ui/page/MultiplayerPage.hpp"

#include "core/Tools.hpp"
#include "ui/page/GamePage.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>

namespace beiklive
{
namespace
{
constexpr uint16_t DEFAULT_CONTROL_PORT = 45872;

NVGcolor colorBgPanel() { return nvgRGBA(12, 14, 18, 178); }
NVGcolor colorPanelSoft() { return nvgRGBA(28, 34, 42, 150); }
NVGcolor colorStroke() { return nvgRGBA(255, 255, 255, 44); }
NVGcolor colorAccent() { return nvgRGBA(86, 200, 172, 235); }
NVGcolor colorAccentSoft() { return nvgRGBA(86, 200, 172, 54); }
NVGcolor colorText() { return nvgRGBA(245, 247, 250, 245); }
NVGcolor colorMuted() { return nvgRGBA(184, 194, 204, 205); }

std::string crcText(uint32_t crc)
{
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%08X", crc);
    return buffer;
}

bool isLinkPlatform(int platform)
{
    return platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGBA) ||
           platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGBC) ||
           platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGB);
}

std::string lowerCopy(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::string trimCopy(const std::string& text)
{
    const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (first >= last)
        return {};
    return {first, last};
}

bool parseEndpoint(const std::string& endpoint, std::string& host, uint16_t& port)
{
    std::string text = trimCopy(endpoint);
    if (text.empty())
        return false;

    port = DEFAULT_CONTROL_PORT;
    const auto sep = text.rfind(':');
    if (sep != std::string::npos)
    {
        const std::string portText = trimCopy(text.substr(sep + 1));
        text = trimCopy(text.substr(0, sep));
        if (portText.empty())
            return false;
        try
        {
            const int parsed = std::stoi(portText);
            if (parsed <= 0 || parsed > 65535)
                return false;
            port = static_cast<uint16_t>(parsed);
        }
        catch (...)
        {
            return false;
        }
    }

    host = text;
    return !host.empty();
}

} // namespace

MultiplayerPage::MultiplayerPage()
{
    showHeader(false);
    showFooter(false);
    hideFooterLine();
    setFocusable(true);
    setHideHighlight(true);

    beiklive::netplay::NetplayManager::instance().loadProfile();
    m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
    m_pendingName = m_snapshot.nickname.empty() ? "Player" : m_snapshot.nickname;
    m_pendingAvatar = m_snapshot.players[0].avatar;

    refreshEntries();
    registerPageActions();
}

void MultiplayerPage::willAppear(bool resetState)
{
    beiklive::Box::willAppear(resetState);
    refreshEntries();
    m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
    brls::Application::giveFocus(this);
}

void MultiplayerPage::frame(brls::FrameContext* ctx)
{
    beiklive::Box::frame(ctx);
    beiklive::netplay::NetplayManager::instance().poll();
    m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
    if (m_manualConnecting &&
        (m_snapshot.currentRoom.roomId != 0 ||
         m_snapshot.statusText.find("加入失败") != std::string::npos ||
         m_snapshot.statusText.find("手动加入失败") != std::string::npos))
    {
        m_manualConnecting = false;
    }
    if (m_mode == PageMode::JoinRooms && m_snapshot.currentRoom.roomId != 0)
    {
        m_mode = PageMode::HostWaiting;
        m_focus = FocusZone::Waiting;
    }
    if (m_mode == PageMode::HostWaiting && !m_snapshot.hosting && m_snapshot.currentRoom.roomId == 0)
    {
        m_mode = PageMode::JoinRooms;
        m_focus = FocusZone::Rooms;
    }
    launchNetplayGameIfReady();
}

void MultiplayerPage::draw(NVGcontext* vg, float x, float y, float w, float h,
                           brls::Style style, brls::FrameContext* ctx)
{
    beiklive::Box::draw(vg, x, y, w, h, style, ctx);
    if (m_font < 0)
        m_font = brls::Application::getDefaultFont();

    // 绘制整页暗色遮罩：覆盖多人页面全屏背景，降低动态背景干扰。
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillColor(vg, nvgRGBA(0, 0, 0, 84));
    nvgFill(vg);

    drawHeader(vg, x + 54.f, y + 32.f, w - 108.f);

    if (m_mode == PageMode::Landing)
    {
        drawLanding(vg, x + 54.f, y + 124.f, w - 108.f, h - 212.f);
    }
    else
    {
        drawProfileBlock(vg, x + 54.f, y + 124.f, 170.f, h - 212.f);
        if (m_mode == PageMode::HostLibrary)
            drawHostLibrary(vg, x + 246.f, y + 124.f, w - 300.f, h - 212.f);
        else if (m_mode == PageMode::HostWaiting)
            drawWaiting(vg, x + 246.f, y + 124.f, w - 300.f, h - 212.f);
        else
            drawRooms(vg, x + 246.f, y + 124.f, w - 300.f, h - 212.f);
    }

    if (m_profileSetupOpen)
        drawSetupModal(vg, x, y, w, h);
    if (m_manualConnecting)
        drawConnectingModal(vg, x, y, w, h);

    drawFooter(vg, x + 54.f, y + h - 66.f, w - 108.f, 34.f);
}

void MultiplayerPage::refreshEntries()
{
    m_allEntries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
    m_allEntries.erase(std::remove_if(m_allEntries.begin(), m_allEntries.end(),
        [](const beiklive::GameEntry& entry) { return !isLinkPlatform(entry.platform); }),
        m_allEntries.end());
    rebuildVisibleEntries();
}

void MultiplayerPage::rebuildVisibleEntries()
{
    const int targetPlatform = currentPlatformInt();
    m_visibleEntries = m_allEntries;
    m_visibleEntries.erase(std::remove_if(m_visibleEntries.begin(), m_visibleEntries.end(),
        [targetPlatform](const beiklive::GameEntry& entry) { return entry.platform != targetPlatform; }),
        m_visibleEntries.end());

    if (m_isSearching && !m_searchTerm.empty())
    {
        const std::string needle = lowerCopy(m_searchTerm);
        m_visibleEntries.erase(std::remove_if(m_visibleEntries.begin(), m_visibleEntries.end(),
            [this, &needle](const beiklive::GameEntry& entry) {
                return lowerCopy(displayTitle(entry)).find(needle) == std::string::npos;
            }), m_visibleEntries.end());
    }

    switch (m_sortMode)
    {
    case SortMode::PlayTime:
        std::sort(m_visibleEntries.begin(), m_visibleEntries.end(),
            [](const GameEntry& a, const GameEntry& b) { return a.playTime > b.playTime; });
        break;
    case SortMode::FirstLetter:
        std::sort(m_visibleEntries.begin(), m_visibleEntries.end(),
            [this](const GameEntry& a, const GameEntry& b) {
                return titleToSortKey(displayTitle(a)) < titleToSortKey(displayTitle(b));
            });
        break;
    case SortMode::LastPlayed:
    default:
        std::sort(m_visibleEntries.begin(), m_visibleEntries.end(),
            [](const GameEntry& a, const GameEntry& b) { return a.lastPlayed > b.lastPlayed; });
        break;
    }

    if (m_gridIndex >= static_cast<int>(m_visibleEntries.size()))
        m_gridIndex = static_cast<int>(m_visibleEntries.size()) - 1;
    if (m_gridIndex < 0)
        m_gridIndex = 0;
    ensureGridVisible(m_lastGridHeight);
}

void MultiplayerPage::registerPageActions()
{
    registerAction("选择", brls::BUTTON_A, [this](brls::View*) {
        activateCurrent();
        return true;
    });
    registerAction("返回", brls::BUTTON_B, [this](brls::View*) {
        if (m_profileSetupOpen)
        {
            closeProfileSetup();
            return true;
        }
        if (m_manualConnecting)
        {
            m_manualConnecting = false;
            beiklive::netplay::NetplayManager::instance().leaveRoom();
            m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
            return true;
        }
        if (m_mode != PageMode::Landing)
        {
            beiklive::netplay::NetplayManager::instance().leaveRoom();
            m_mode = PageMode::Landing;
            m_focus = FocusZone::Landing;
            return true;
        }
        closePage();
        return true;
    });
    registerAction("搜索", brls::BUTTON_X, [this](brls::View*) {
        if (m_profileSetupOpen)
            openNameEditor();
        else if (m_mode == PageMode::HostLibrary)
            openSearchEditor();
        else if (m_mode == PageMode::JoinRooms)
            openManualJoinEditor();
        return true;
    });
    registerAction("排序", brls::BUTTON_Y, [this](brls::View*) {
        if (m_mode == PageMode::HostLibrary && !m_profileSetupOpen)
            cycleSortMode();
        return true;
    });
    registerAction("", brls::BUTTON_UP, [this](brls::View*) {
        moveSelection(-1);
        return true;
    }, true, true, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_DOWN, [this](brls::View*) {
        moveSelection(1);
        return true;
    }, true, true, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_LEFT, [this](brls::View*) {
        moveHorizontal(-1);
        return true;
    }, true, true, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_RIGHT, [this](brls::View*) {
        moveHorizontal(1);
        return true;
    }, true, true, brls::SOUND_NONE);
}

void MultiplayerPage::activateCurrent()
{
    if (m_profileSetupOpen)
    {
        if (m_setupIndex == 0)
            openNameEditor();
        else if (m_setupIndex == 2)
            confirmProfileSetup();
        return;
    }

    if (m_mode == PageMode::Landing)
    {
        openProfileSetup(m_landingIndex == 0 ? SetupPurpose::Host : SetupPurpose::Join);
        return;
    }

    if (m_mode == PageMode::HostLibrary)
    {
        if (m_focus == FocusZone::Tabs)
        {
            m_focus = FocusZone::Grid;
            return;
        }
        selectGameForHosting();
        return;
    }

    if (m_mode == PageMode::HostWaiting)
    {
        if (m_snapshot.hosting && m_snapshot.pendingJoin.active && m_snapshot.pendingJoin.approvalReady)
        {
            if (m_waitingActionIndex == 0)
                approvePendingJoin();
            else
                rejectPendingJoin();
        }
        else if (m_snapshot.hosting && (!m_snapshot.pendingJoin.active || !m_snapshot.pendingJoin.approvalReady))
        {
            beiklive::netplay::NetplayManager::instance().startGame();
            m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
            launchNetplayGameIfReady();
        }
        return;
    }

    if (m_mode == PageMode::JoinRooms)
        joinSelectedRoom();
}

void MultiplayerPage::moveSelection(int delta)
{
    if (m_profileSetupOpen)
    {
        m_setupIndex = (m_setupIndex + delta + 3) % 3;
        return;
    }

    if (m_mode == PageMode::Landing)
    {
        m_landingIndex = (m_landingIndex + delta + 2) % 2;
        return;
    }

    if (m_mode == PageMode::HostLibrary)
    {
        if (m_focus == FocusZone::Tabs)
        {
            m_tabIndex = (m_tabIndex + delta + static_cast<int>(kTabs.size())) % static_cast<int>(kTabs.size());
            m_gridIndex = 0;
            m_gridScrollRow = 0;
            rebuildVisibleEntries();
        }
        else
        {
            const int count = static_cast<int>(m_visibleEntries.size());
            if (count <= 0)
                return;
            m_gridIndex = std::clamp(m_gridIndex + delta * gridColumnCount(), 0, count - 1);
            ensureGridVisible(m_lastGridHeight);
        }
        return;
    }

    if (m_mode == PageMode::JoinRooms)
    {
        const int count = static_cast<int>(m_snapshot.rooms.size());
        if (count <= 0)
            return;
        m_roomIndex = (m_roomIndex + delta + count) % count;
        ensureRoomVisible(m_lastRoomHeight);
    }

    if (m_mode == PageMode::HostWaiting && m_snapshot.hosting &&
        m_snapshot.pendingJoin.active && m_snapshot.pendingJoin.approvalReady)
        m_waitingActionIndex = (m_waitingActionIndex + delta + 2) % 2;
}

void MultiplayerPage::moveHorizontal(int delta)
{
    if (m_profileSetupOpen)
    {
        if (m_setupIndex == 1)
            m_pendingAvatar = static_cast<uint8_t>((static_cast<int>(m_pendingAvatar) + delta + 6) % 6);
        return;
    }

    if (m_mode == PageMode::Landing)
    {
        m_landingIndex = (m_landingIndex + delta + 2) % 2;
        return;
    }

    if (m_mode == PageMode::HostLibrary)
    {
        if (m_focus == FocusZone::Tabs && delta > 0)
        {
            m_focus = FocusZone::Grid;
            return;
        }
        if (m_focus == FocusZone::Grid && delta < 0 && m_gridIndex % gridColumnCount() == 0)
        {
            m_focus = FocusZone::Tabs;
            return;
        }
        const int count = static_cast<int>(m_visibleEntries.size());
        if (count <= 0)
            return;
        m_gridIndex = std::clamp(m_gridIndex + delta, 0, count - 1);
        ensureGridVisible(m_lastGridHeight);
    }

    if (m_mode == PageMode::HostWaiting && m_snapshot.hosting &&
        m_snapshot.pendingJoin.active && m_snapshot.pendingJoin.approvalReady)
        m_waitingActionIndex = (m_waitingActionIndex + delta + 2) % 2;
}

void MultiplayerPage::openProfileSetup(SetupPurpose purpose)
{
    m_setupPurpose = purpose;
    m_profileSetupOpen = true;
    m_focus = FocusZone::Setup;
    m_setupIndex = 2;
    m_pendingName = m_snapshot.nickname.empty() ? "Player" : m_snapshot.nickname;
    m_pendingAvatar = m_snapshot.players[0].avatar;
}

void MultiplayerPage::closeProfileSetup()
{
    m_profileSetupOpen = false;
    m_setupPurpose = SetupPurpose::None;
    m_focus = (m_mode == PageMode::Landing) ? FocusZone::Landing : m_focus;
}

void MultiplayerPage::confirmProfileSetup()
{
    beiklive::netplay::NetplayManager::instance().saveProfile(m_pendingName, m_pendingAvatar);
    m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
    const SetupPurpose purpose = m_setupPurpose;
    closeProfileSetup();

    if (purpose == SetupPurpose::Host)
    {
        m_mode = PageMode::HostLibrary;
        m_focus = FocusZone::Tabs;
        refreshEntries();
    }
    else if (purpose == SetupPurpose::Join)
    {
        m_mode = PageMode::JoinRooms;
        m_focus = FocusZone::Rooms;
        m_roomIndex = 0;
        m_roomScroll = 0;
        beiklive::netplay::NetplayManager::instance().startScanning();
    }
}

void MultiplayerPage::openNameEditor()
{
    auto* ime = brls::Application::getPlatform()->getImeManager();
    if (!ime)
        return;

    ime->openForText(
        [this](std::string text) {
            if (!text.empty())
                m_pendingName = text.substr(0, 31);
        },
        "设置昵称", "", 32, m_pendingName,
        brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
}

void MultiplayerPage::openSearchEditor()
{
    auto* ime = brls::Application::getPlatform()->getImeManager();
    if (!ime)
        return;

    ime->openForText(
        [this](std::string text) {
            m_searchTerm = text;
            m_isSearching = !m_searchTerm.empty();
            m_gridIndex = 0;
            m_gridScrollRow = 0;
            rebuildVisibleEntries();
        },
        "搜索游戏", "", 128, m_searchTerm,
        brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
}

void MultiplayerPage::openManualJoinEditor()
{
    auto* ime = brls::Application::getPlatform()->getImeManager();
    if (!ime)
        return;

    ime->openForText(
        [this](std::string text) {
            if (!text.empty())
            {
                m_manualEndpoint = trimCopy(text);
                joinManualEndpoint(m_manualEndpoint);
            }
        },
        "手动加入房间", "输入房主IP:端口，例如 192.168.1.10:45872", 64, m_manualEndpoint,
        brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
}

void MultiplayerPage::cycleSortMode()
{
    const int next = (static_cast<int>(m_sortMode) + 1) % 3;
    m_sortMode = static_cast<SortMode>(next);
    m_gridIndex = 0;
    m_gridScrollRow = 0;
    rebuildVisibleEntries();
}

void MultiplayerPage::selectGameForHosting()
{
    if (m_visibleEntries.empty() || m_gridIndex < 0 || m_gridIndex >= static_cast<int>(m_visibleEntries.size()))
    {
        brls::Application::notify("当前分类没有可用游戏");
        return;
    }

    auto entry = m_visibleEntries[m_gridIndex];
    const bool ok = beiklive::netplay::NetplayManager::instance().startHosting(entry);
    if (!ok)
    {
        brls::Application::notify("创建房间失败");
        return;
    }

    m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
    m_mode = PageMode::HostWaiting;
    m_focus = FocusZone::Waiting;
}

void MultiplayerPage::joinSelectedRoom()
{
    if (m_snapshot.rooms.empty() || m_roomIndex < 0 || m_roomIndex >= static_cast<int>(m_snapshot.rooms.size()))
        return;

    beiklive::netplay::NetplayManager::instance().joinRoom(m_snapshot.rooms[m_roomIndex].roomId);
    m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
}

void MultiplayerPage::joinManualEndpoint(const std::string& endpoint)
{
    std::string host;
    uint16_t port = DEFAULT_CONTROL_PORT;
    if (!parseEndpoint(endpoint, host, port))
    {
        brls::Application::notify("请输入正确的 IP:端口");
        return;
    }

    const bool ok = beiklive::netplay::NetplayManager::instance().joinManual(host, port);
    m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
    if (!ok)
        brls::Application::notify("手动加入失败");
    else
        m_manualConnecting = true;
}

void MultiplayerPage::approvePendingJoin()
{
    beiklive::netplay::NetplayManager::instance().approvePendingJoin();
    m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
    m_waitingActionIndex = 0;
}

void MultiplayerPage::rejectPendingJoin()
{
    beiklive::netplay::NetplayManager::instance().rejectPendingJoin();
    m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
    m_waitingActionIndex = 0;
}

void MultiplayerPage::launchNetplayGameIfReady()
{
    if (m_gameLaunchRequested ||
        (m_snapshot.state != beiklive::netplay::NetplayState::LoadingGame &&
         m_snapshot.state != beiklive::netplay::NetplayState::WaitingReady &&
         m_snapshot.state != beiklive::netplay::NetplayState::Running) ||
        m_snapshot.currentRoom.roomId == 0)
        return;

    m_gameLaunchRequested = true;
    beiklive::GameEntry entry;
    if (!findNetplayGame(entry))
    {
        m_gameLaunchRequested = false;
        brls::Application::notify("本机游戏库未找到匹配 ROM");
        return;
    }

    auto* gamePage = new beiklive::GamePage(entry);
    gamePage->setNetplayMode(true);
    auto* frame = new brls::AppletFrame(gamePage);
    HIDE_BRLS_BAR(frame);
    brls::Logger::info("Pushing netplay GamePage activity for: " + entry.title);
    brls::sync([frame, this, gamePage]() {
        beiklive::pushActivity(frame, this, gamePage, [gamePage]() { gamePage->startGame(); });
    });
}

bool MultiplayerPage::findNetplayGame(beiklive::GameEntry& out) const
{
    const auto& room = m_snapshot.currentRoom;
    if (room.crc32 != 0)
    {
        auto it = std::find_if(m_allEntries.begin(), m_allEntries.end(),
            [&room](const beiklive::GameEntry& entry) {
                return static_cast<uint32_t>(entry.crc32) == room.crc32 && !entry.path.empty();
            });
        if (it != m_allEntries.end())
        {
            out = *it;
            return true;
        }
    }

    const std::string roomTitle = lowerCopy(room.title);
    if (!roomTitle.empty())
    {
        auto it = std::find_if(m_allEntries.begin(), m_allEntries.end(),
            [this, &roomTitle](const beiklive::GameEntry& entry) {
                return lowerCopy(displayTitle(entry)) == roomTitle && !entry.path.empty();
            });
        if (it != m_allEntries.end())
        {
            out = *it;
            return true;
        }
    }

    return false;
}

void MultiplayerPage::closePage()
{
    brls::sync([this]() {
        beiklive::popActivity(this);
    });
}

std::string MultiplayerPage::currentPlatformName() const
{
    return kTabs[m_tabIndex].label;
}

std::string MultiplayerPage::sortModeName() const
{
    switch (m_sortMode)
    {
    case SortMode::PlayTime: return "游玩时长";
    case SortMode::FirstLetter: return "首字母";
    case SortMode::LastPlayed:
    default: return "最近游玩";
    }
}

std::string MultiplayerPage::displayTitle(const beiklive::GameEntry& entry) const
{
    if (!entry.title.empty())
        return entry.title;
    return beiklive::tools::getFileNameWithoutExtension(entry.path);
}

int MultiplayerPage::currentPlatformInt() const
{
    return static_cast<int>(kTabs[m_tabIndex].platform);
}

int MultiplayerPage::visibleGridRows(float h) const
{
    return std::max(1, static_cast<int>((h - 72.f) / 112.f));
}

int MultiplayerPage::visibleRoomRows(float h) const
{
    return std::max(1, static_cast<int>((h - 72.f) / 94.f));
}

void MultiplayerPage::ensureGridVisible(float h)
{
    const int rows = visibleGridRows(h);
    const int row = m_gridIndex / gridColumnCount();
    if (row < m_gridScrollRow)
        m_gridScrollRow = row;
    else if (row >= m_gridScrollRow + rows)
        m_gridScrollRow = row - rows + 1;
    if (m_gridScrollRow < 0)
        m_gridScrollRow = 0;
}

void MultiplayerPage::ensureRoomVisible(float h)
{
    const int rows = visibleRoomRows(h);
    if (m_roomIndex < m_roomScroll)
        m_roomScroll = m_roomIndex;
    else if (m_roomIndex >= m_roomScroll + rows)
        m_roomScroll = m_roomIndex - rows + 1;
    if (m_roomScroll < 0)
        m_roomScroll = 0;
}

void MultiplayerPage::drawText(NVGcontext* vg, const std::string& text, float x, float y,
                               float size, NVGcolor color, int align) const
{
    nvgFontFaceId(vg, m_font);
    nvgFontSize(vg, size);
    nvgFillColor(vg, color);
    nvgTextAlign(vg, align);
    nvgText(vg, x, y, text.c_str(), nullptr);
}

void MultiplayerPage::drawRoundedRect(NVGcontext* vg, float x, float y, float w, float h,
                                      float radius, NVGcolor fill, NVGcolor stroke,
                                      float strokeWidth) const
{
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, w, h, radius);
    nvgFillColor(vg, fill);
    nvgFill(vg);
    if (strokeWidth > 0.f)
    {
        nvgStrokeColor(vg, stroke);
        nvgStrokeWidth(vg, strokeWidth);
        nvgStroke(vg);
    }
}

void MultiplayerPage::drawAvatar(NVGcontext* vg, float cx, float cy, float radius,
                                 uint8_t avatar, const std::string& name, bool large) const
{
    static const std::array<NVGcolor, 6> fills = {{
        nvgRGBA(86, 200, 172, 255),
        nvgRGBA(121, 174, 249, 255),
        nvgRGBA(236, 153, 92, 255),
        nvgRGBA(216, 117, 161, 255),
        nvgRGBA(173, 139, 245, 255),
        nvgRGBA(232, 196, 88, 255),
    }};

    // 绘制圆形头像图片占位：左上个人资料区域和设置弹窗中使用。
    nvgBeginPath(vg);
    nvgCircle(vg, cx, cy, radius);
    nvgFillColor(vg, fills[avatar % fills.size()]);
    nvgFill(vg);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 210));
    nvgStrokeWidth(vg, large ? 2.6f : 1.6f);
    nvgStroke(vg);

    std::string initial = name.empty() ? "P" : name.substr(0, 1);
    drawText(vg, initial, cx, cy + (large ? 2.f : 1.f), large ? radius * 0.78f : radius * 0.72f,
             nvgRGBA(8, 10, 14, 235), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
}

void MultiplayerPage::drawHeader(NVGcontext* vg, float x, float y, float w) const
{
    // 绘制顶部标题栏：左侧标题，右侧显示联机状态和收发包统计。
    drawRoundedRect(vg, x, y, w, 66.f, 8.f, colorBgPanel(), colorStroke(), 1.f);
    drawText(vg, "多人游戏", x + 24.f, y + 24.f, 28.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    drawText(vg, m_snapshot.statusText.empty() ? toString(m_snapshot.state) : m_snapshot.statusText,
             x + 24.f, y + 50.f, 15.f, colorMuted(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    const std::string counters = "TX " + std::to_string(m_snapshot.txPackets) +
                                 "   RX " + std::to_string(m_snapshot.rxPackets);
    drawText(vg, counters, x + w - 24.f, y + 34.f, 16.f, colorMuted(), NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
}

void MultiplayerPage::drawLanding(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制初始页主体：仅显示“创建房间”和“加入房间”两个大按钮。
    drawRoundedRect(vg, x, y, w, h, 8.f, colorBgPanel(), colorStroke(), 1.f);
    const float buttonW = 320.f;
    const float buttonH = 142.f;
    const float gap = 34.f;
    const float startX = x + (w - buttonW * 2.f - gap) * 0.5f;
    const float by = y + (h - buttonH) * 0.5f;
    const std::array<std::string, 2> titles = { "创建房间", "加入房间" };
    const std::array<std::string, 2> subs = { "选择本机游戏并等待玩家", "扫描局域网房间并加入" };
    for (int i = 0; i < 2; ++i)
    {
        const bool focused = i == m_landingIndex;
        const float bx = startX + i * (buttonW + gap);
        drawRoundedRect(vg, bx, by, buttonW, buttonH, 8.f,
                        focused ? colorAccentSoft() : colorPanelSoft(),
                        focused ? colorAccent() : colorStroke(), focused ? 2.5f : 1.f);
        drawText(vg, titles[i], bx + buttonW * 0.5f, by + 54.f, 27.f, colorText(),
                 NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        drawText(vg, subs[i], bx + buttonW * 0.5f, by + 92.f, 16.f, colorMuted(),
                 NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    }
}

void MultiplayerPage::drawProfileBlock(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制左上个人资料栏：圆形头像显示在上方，昵称显示在头像下方。
    drawRoundedRect(vg, x, y, w, h, 8.f, colorBgPanel(), colorStroke(), 1.f);
    drawAvatar(vg, x + w * 0.5f, y + 74.f, 42.f, m_snapshot.players[0].avatar,
               m_snapshot.nickname, true);
    drawText(vg, m_snapshot.nickname.empty() ? "Player" : m_snapshot.nickname,
             x + w * 0.5f, y + 138.f, 18.f, colorText(), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    drawText(vg, m_snapshot.hosting ? "房主" : "玩家",
             x + w * 0.5f, y + 166.f, 13.f, colorMuted(), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

    const auto& room = m_snapshot.currentRoom;
    if (room.roomId != 0)
    {
        drawText(vg, "当前房间", x + 20.f, y + 232.f, 15.f, colorMuted(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        drawText(vg, room.title.empty() ? "未选择游戏" : room.title,
                 x + 20.f, y + 260.f, 15.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        drawText(vg, "CRC " + crcText(room.crc32), x + 20.f, y + 286.f, 13.f, colorMuted(),
                 NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    }
}

void MultiplayerPage::drawHostLibrary(NVGcontext* vg, float x, float y, float w, float h)
{
    // 绘制创建房间页面主体：左边平台 TabFrame，右边两列游戏库网格。
    drawRoundedRect(vg, x, y, w, h, 8.f, colorBgPanel(), colorStroke(), 1.f);
    drawTabs(vg, x + 18.f, y + 58.f, 120.f, h - 78.f);
    drawText(vg, "选择游戏", x + 158.f, y + 28.f, 21.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    drawText(vg, currentPlatformName() + " / " + sortModeName() +
                     (m_isSearching ? (" / 搜索: " + m_searchTerm) : ""),
             x + w - 24.f, y + 28.f, 14.f, colorMuted(), NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    drawGameGrid(vg, x + 158.f, y + 58.f, w - 178.f, h - 78.f);
}

void MultiplayerPage::drawTabs(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制左侧 TabFrame 标签：GBA/GBC/GB 三个 Tab 垂直排列。
    const float tabH = 62.f;
    for (int i = 0; i < static_cast<int>(kTabs.size()); ++i)
    {
        const bool selected = i == m_tabIndex;
        const bool focused = selected && m_focus == FocusZone::Tabs;
        const float ty = y + i * (tabH + 10.f);
        drawRoundedRect(vg, x, ty, w, tabH, 7.f,
                        selected ? colorAccentSoft() : colorPanelSoft(),
                        focused ? colorAccent() : (selected ? nvgRGBA(86, 200, 172, 150) : colorStroke()),
                        focused ? 2.5f : 1.f);
        drawText(vg, kTabs[i].label, x + w * 0.5f, ty + tabH * 0.5f, 19.f,
                 selected ? colorText() : colorMuted(), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    }
}

void MultiplayerPage::drawGameGrid(NVGcontext* vg, float x, float y, float w, float h)
{
    m_lastGridHeight = h;
    ensureGridVisible(h);

    // 绘制右侧两列游戏网格：策略参考 GameLibraryPage/RecyclingGrid 的卡片式列表。
    drawRoundedRect(vg, x, y, w, h, 8.f, nvgRGBA(10, 12, 16, 98), colorStroke(), 1.f);
    if (m_visibleEntries.empty())
    {
        drawText(vg, m_isSearching ? "没有匹配的游戏" : "当前平台没有游戏",
                 x + w * 0.5f, y + h * 0.5f, 18.f, colorMuted(), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        return;
    }

    const int cols = gridColumnCount();
    const int rows = visibleGridRows(h);
    const float gap = 10.f;
    const float itemW = (w - 28.f - gap) / cols;
    const float itemH = 104.f;
    const int first = m_gridScrollRow * cols;
    const int maxItems = rows * cols;

    for (int i = 0; i < maxItems; ++i)
    {
        const int index = first + i;
        if (index >= static_cast<int>(m_visibleEntries.size()))
            break;

        const int col = i % cols;
        const int row = i / cols;
        const float ix = x + 14.f + col * (itemW + gap);
        const float iy = y + 14.f + row * (itemH + gap);
        const auto& entry = m_visibleEntries[index];
        const bool focused = m_focus == FocusZone::Grid && index == m_gridIndex;

        drawRoundedRect(vg, ix, iy, itemW, itemH, 5.f,
                        focused ? colorAccentSoft() : nvgRGBA(42, 42, 42, 35),
                        focused ? colorAccent() : (entry.favourite ? nvgRGBA(224, 166, 87, 220) : nvgRGBA(110, 110, 110, 220)),
                        focused ? 2.4f : 1.f);

        // 绘制游戏封面占位块：位于网格卡片左侧，和 GameLibraryPage 的 imageSize 布局一致。
        const float imageSize = itemH - 12.f;
        drawRoundedRect(vg, ix + 6.f, iy + 6.f, imageSize, imageSize, 4.f,
                        nvgRGBA(60, 60, 60, 190), nvgRGBA(100, 100, 100, 150), 0.5f);
        drawText(vg, currentPlatformName(), ix + 6.f + imageSize * 0.5f, iy + 6.f + imageSize * 0.5f,
                 16.f, colorMuted(), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        const float tx = ix + imageSize + 18.f;
        drawRoundedRect(vg, tx, iy + 16.f, 36.f, 20.f, 4.f, colorAccent(), colorAccent(), 0.f);
        drawText(vg, currentPlatformName(), tx + 18.f, iy + 26.f, 11.f, nvgRGBA(255, 255, 255, 255),
                 NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        drawText(vg, displayTitle(entry), tx + 44.f, iy + 15.f, 16.f, colorText(),
                 NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        drawText(vg, beiklive::tools::formatPlayTime(entry.playTime), tx, iy + 48.f, 13.f,
                 nvgRGBA(121, 201, 249, 255), NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        drawText(vg, "CRC " + crcText(static_cast<uint32_t>(entry.crc32)), tx, iy + 72.f, 13.f,
                 colorMuted(), NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    }
}

void MultiplayerPage::drawRooms(NVGcontext* vg, float x, float y, float w, float h)
{
    m_lastRoomHeight = h;
    ensureRoomVisible(h);

    // 绘制加入房间页面主体：单列表展示扫描到的房间。
    drawRoundedRect(vg, x, y, w, h, 8.f, colorBgPanel(), colorStroke(), 1.f);
    drawText(vg, "加入房间", x + 24.f, y + 28.f, 21.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    drawText(vg, m_snapshot.scanning ? "扫描中" : "已停止", x + w - 196.f, y + 28.f, 14.f,
             m_snapshot.scanning ? colorAccent() : colorMuted(), NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);

    // 绘制右上手动加入按钮：按 X 打开输入框，用户输入房主 IP:端口。
    drawRoundedRect(vg, x + w - 176.f, y + 12.f, 152.f, 32.f, 7.f,
                    colorPanelSoft(), colorStroke(), 1.f);
    drawText(vg, "X 手动加入", x + w - 100.f, y + 28.f, 14.f, colorText(),
             NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

    if (m_snapshot.rooms.empty())
    {
        drawText(vg, "暂无房间，按 X 手动输入 IP:端口", x + w * 0.5f, y + h * 0.5f, 18.f, colorMuted(),
                 NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        return;
    }

    const int rows = visibleRoomRows(h);
    const float itemH = 84.f;
    const float gap = 10.f;
    for (int i = 0; i < rows; ++i)
    {
        const int index = m_roomScroll + i;
        if (index >= static_cast<int>(m_snapshot.rooms.size()))
            break;

        const auto& room = m_snapshot.rooms[index];
        const bool focused = index == m_roomIndex;
        const float iy = y + 62.f + i * (itemH + gap);
        drawRoundedRect(vg, x + 18.f, iy, w - 36.f, itemH, 7.f,
                        focused ? colorAccentSoft() : colorPanelSoft(),
                        focused ? colorAccent() : colorStroke(), focused ? 2.4f : 1.f);
        drawAvatar(vg, x + 52.f, iy + itemH * 0.5f, 22.f, room.avatar, room.hostName, false);
        drawText(vg, room.title.empty() ? "未知游戏" : room.title, x + 88.f, iy + 24.f, 17.f,
                 colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        drawText(vg, "Host: " + room.hostName + "   CRC " + crcText(room.crc32),
                 x + 88.f, iy + 52.f, 13.f, colorMuted(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        drawText(vg, std::to_string(room.players) + "/" + std::to_string(room.maxPlayers),
                 x + w - 44.f, iy + itemH * 0.5f, 16.f, colorAccent(), NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    }
}

void MultiplayerPage::drawWaiting(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制房间等待页面：展示 title/CRC/连接地址和双方玩家状态。
    drawRoundedRect(vg, x, y, w, h, 8.f, colorBgPanel(), colorStroke(), 1.f);
    const auto& room = m_snapshot.currentRoom;
    const std::string endpoint = room.endpoint.empty() || room.endpoint == "local" ? "0.0.0.0:45872" : room.endpoint;
    std::string title = m_snapshot.hosting ? "等待玩家加入  " + endpoint : "等待房主同意";
    if (!m_snapshot.hosting && m_snapshot.state == beiklive::netplay::NetplayState::Connected)
        title = "已加入房间，等待房主开始";
    drawText(vg, title, x + 28.f, y + 34.f, 24.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    drawText(vg, room.title.empty() ? "未选择游戏" : room.title, x + 28.f, y + 86.f, 22.f,
             colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    drawText(vg, "CRC " + crcText(room.crc32), x + 28.f, y + 122.f, 15.f, colorMuted(),
             NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    // 绘制房主手动加入地址：显示本机局域网 IP、广播端口和控制端口，供客户端手动输入。
    drawText(vg, "地址 " + endpoint, x + 28.f, y + 150.f, 15.f, colorAccent(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    drawText(vg, "广播端口 45871   控制端口 45872", x + 28.f, y + 174.f, 13.f, colorMuted(),
             NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    // 绘制等待流程状态：显示当前握手阶段，例如 1 秒后回复、等待正式请求、等待房主同意。
    drawText(vg, m_snapshot.statusText.empty() ? "等待连接流程继续" : m_snapshot.statusText,
             x + 28.f, y + 204.f, 14.f, colorMuted(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    if (m_snapshot.hosting)
    {
        const float panelW = 278.f;
        const float panelX = x + w - panelW - 24.f;
        const float panelY = y + 70.f;
        const float panelH = 246.f;
        // 绘制右侧客户端请求栏：展示当前正在请求加入的客户端信息和审批按钮。
        drawRoundedRect(vg, panelX, panelY, panelW, panelH, 8.f,
                        nvgRGBA(10, 12, 16, 118), colorStroke(), 1.f);
        drawText(vg, "客户端请求", panelX + 18.f, panelY + 28.f, 18.f,
                 colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

        if (m_snapshot.pendingJoin.active)
        {
            const auto& pending = m_snapshot.pendingJoin;
            drawAvatar(vg, panelX + 48.f, panelY + 76.f, 24.f, pending.avatar, pending.name, false);
            drawText(vg, pending.name.empty() ? "玩家" : pending.name, panelX + 84.f, panelY + 66.f, 16.f,
                     colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            drawText(vg, pending.endpoint, panelX + 84.f, panelY + 90.f, 12.f,
                     colorMuted(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            drawText(vg, pending.approvalReady ? "正式加入请求" : "房间信息请求",
                     panelX + 18.f, panelY + 118.f, 13.f, colorAccent(),
                     NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            drawText(vg, pending.title.empty() ? "未知游戏" : pending.title, panelX + 18.f, panelY + 142.f, 14.f,
                     colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            const std::string pendingDetail =
                pending.approvalReady ? ("CRC " + crcText(pending.crc32))
                                      : (m_snapshot.statusText.find("1秒后") != std::string::npos
                                             ? "1秒后回复房间信息"
                                             : "已回复房间信息，等待客户端发送正式加入请求");
            drawText(vg, pendingDetail,
                     panelX + 18.f, panelY + 166.f, 13.f,
                     colorMuted(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

            if (pending.approvalReady)
            {
                const float buttonY = panelY + 198.f;
                const float buttonW = 108.f;
                const std::array<std::string, 2> labels = {"同意", "拒绝"};
                for (int i = 0; i < 2; ++i)
                {
                    const bool focused = i == m_waitingActionIndex;
                    const float buttonX = panelX + 18.f + i * (buttonW + 18.f);
                    // 绘制审批按钮：左侧同意，右侧拒绝，方向键切换，A 确认。
                    drawRoundedRect(vg, buttonX, buttonY, buttonW, 38.f, 7.f,
                                    focused ? colorAccentSoft() : colorPanelSoft(),
                                    focused ? colorAccent() : colorStroke(), focused ? 2.2f : 1.f);
                    drawText(vg, labels[i], buttonX + buttonW * 0.5f, buttonY + 19.f, 15.f,
                             focused ? colorText() : colorMuted(), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                }
            }
        }
        else
        {
            drawText(vg, "暂无客户端请求", panelX + panelW * 0.5f, panelY + panelH * 0.5f, 15.f,
                     colorMuted(), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        }
    }

    const float rowY = y + 230.f;
    for (int i = 0; i < 4; ++i)
    {
        const auto& player = m_snapshot.players[i];
        const bool occupied = !player.name.empty();
        const float px = x + 28.f + i * 150.f;
        drawRoundedRect(vg, px, rowY, 126.f, 116.f, 8.f,
                        occupied ? colorPanelSoft() : nvgRGBA(255, 255, 255, 18),
                        occupied ? colorStroke() : nvgRGBA(255, 255, 255, 28), 1.f);
        drawAvatar(vg, px + 63.f, rowY + 42.f, 26.f, player.avatar, player.name, false);
        drawText(vg, occupied ? player.name : "空位", px + 63.f, rowY + 84.f, 14.f,
                 occupied ? colorText() : colorMuted(), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    }
}

void MultiplayerPage::drawSetupModal(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制资料设置弹窗遮罩：进入创建/加入流程前设置昵称和头像。
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillColor(vg, nvgRGBA(0, 0, 0, 150));
    nvgFill(vg);

    const float mw = 520.f;
    const float mh = 360.f;
    const float mx = x + (w - mw) * 0.5f;
    const float my = y + (h - mh) * 0.5f;
    drawRoundedRect(vg, mx, my, mw, mh, 8.f, nvgRGBA(18, 22, 28, 236), colorStroke(), 1.f);
    drawText(vg, m_setupPurpose == SetupPurpose::Host ? "创建房间资料" : "加入房间资料",
             mx + 28.f, my + 34.f, 23.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    drawAvatar(vg, mx + mw * 0.5f, my + 104.f, 42.f, m_pendingAvatar, m_pendingName, true);

    const std::array<std::string, 3> labels = {
        "昵称  " + (m_pendingName.empty() ? "Player" : m_pendingName),
        "头像  " + std::to_string(static_cast<int>(m_pendingAvatar) + 1) + " / 6",
        "完成",
    };
    for (int i = 0; i < 3; ++i)
    {
        const bool focused = i == m_setupIndex;
        const float iy = my + 166.f + i * 56.f;
        drawRoundedRect(vg, mx + 48.f, iy, mw - 96.f, 44.f, 7.f,
                        focused ? colorAccentSoft() : colorPanelSoft(),
                        focused ? colorAccent() : colorStroke(), focused ? 2.f : 1.f);
        drawText(vg, labels[i], mx + mw * 0.5f, iy + 22.f, 16.f,
                 focused ? colorText() : colorMuted(), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    }
}

void MultiplayerPage::drawConnectingModal(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制手动连接弹窗遮罩：输入 IP 后等待房主 RoomInfo/JoinAccept 回包。
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillColor(vg, nvgRGBA(0, 0, 0, 126));
    nvgFill(vg);

    const float mw = 560.f;
    const float mh = 190.f;
    const float mx = x + (w - mw) * 0.5f;
    const float my = y + (h - mh) * 0.5f;
    drawRoundedRect(vg, mx, my, mw, mh, 8.f, nvgRGBA(18, 22, 28, 238), colorStroke(), 1.f);

    drawText(vg, "正在连接房主", mx + 32.f, my + 42.f, 24.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    drawText(vg, "目标 " + m_manualEndpoint, mx + 32.f, my + 84.f, 16.f, colorAccent(),
             NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    drawText(vg, m_snapshot.statusText.empty() ? "等待房主响应..." : m_snapshot.statusText,
             mx + 32.f, my + 122.f, 15.f, colorMuted(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    drawText(vg, "B 取消", mx + mw - 32.f, my + mh - 30.f, 14.f, colorMuted(),
             NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
}

void MultiplayerPage::drawFooter(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制底部按键提示：位置为页面底边，根据当前模式展示可用操作。
    drawRoundedRect(vg, x, y, w, h, 8.f, nvgRGBA(10, 12, 16, 142), colorStroke(), 1.f);
    std::string hint = "A 选择     B 返回     方向键移动";
    if (m_profileSetupOpen)
        hint = "A 选择/完成     X 编辑昵称     B 取消     左右切换头像";
    else if (m_mode == PageMode::HostLibrary)
        hint = "A 选择游戏     X 搜索     Y 排序     B 返回     方向键移动";
    else if (m_mode == PageMode::HostWaiting)
        hint = (m_snapshot.hosting && m_snapshot.pendingJoin.active && m_snapshot.pendingJoin.approvalReady)
                   ? "A 确认     左右切换同意/拒绝     B 返回"
                   : (m_snapshot.hosting && m_snapshot.pendingJoin.active)
                         ? "A 开始游戏     等待客户端发送正式加入请求     B 返回"
                   : (m_snapshot.hosting ? "A 开始游戏     B 返回" : "等待房主开始     B 返回");
    else if (m_mode == PageMode::JoinRooms)
        hint = "A 加入房间     X 手动加入     B 返回     方向键选择";
    drawText(vg, hint, x + w * 0.5f, y + h * 0.5f, 15.f, colorMuted(),
             NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
}

std::string MultiplayerPage::titleToSortKey(const std::string& title)
{
    static nlohmann::json pinyinMap;
    static bool loaded = false;
    if (!loaded)
    {
        std::ifstream f(BK_RES("pinyin/pingyin.json"));
        if (f.is_open())
            f >> pinyinMap;
        loaded = true;
    }

    std::string key;
    for (size_t i = 0; i < title.size(); ++i)
    {
        std::string ch(1, title[i]);
        unsigned char c = static_cast<unsigned char>(title[i]);
        if (c >= 0x80 && i + 2 < title.size())
        {
            ch = title.substr(i, 3);
            i += 2;
        }
        if (pinyinMap.contains(ch))
            key += pinyinMap[ch].get<std::string>();
        else if (c >= 0x80)
            key += "\xFF";
        else if (std::isdigit(c))
            key += std::string(1, '\x00') + ch;
        else if (std::isalpha(c))
            key += std::string(1, '\x01') + std::string(1, static_cast<char>(std::tolower(c)));
        else
            key += std::string(1, '\x02') + ch;
    }
    return key;
}

} // namespace beiklive
