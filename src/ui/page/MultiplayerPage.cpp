#include "ui/page/MultiplayerPage.hpp"

#include "core/Tools.hpp"

#include <algorithm>
#include <cstdio>

namespace beiklive
{
namespace
{
NVGcolor colorBgPanel()
{
    return nvgRGBA(12, 14, 18, 178);
}

NVGcolor colorPanelSoft()
{
    return nvgRGBA(28, 34, 42, 150);
}

NVGcolor colorStroke()
{
    return nvgRGBA(255, 255, 255, 44);
}

NVGcolor colorAccent()
{
    return nvgRGBA(86, 200, 172, 235);
}

NVGcolor colorAccentSoft()
{
    return nvgRGBA(86, 200, 172, 54);
}

NVGcolor colorText()
{
    return nvgRGBA(245, 247, 250, 245);
}

NVGcolor colorMuted()
{
    return nvgRGBA(184, 194, 204, 205);
}

std::string crcText(uint32_t crc)
{
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%08X", crc);
    return buffer;
}

bool isGbaEntry(const beiklive::GameEntry& entry)
{
    return entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGBA);
}

} // namespace

MultiplayerPage::MultiplayerPage()
{
    showHeader(false);
    showFooter(false);
    hideFooterLine();
    setFocusable(true);
    setHideHighlight(true);

    m_actions = {
        {"创建房间", "使用当前选中的 GBA 游戏创建局域网房间"},
        {"扫描房间", "刷新局域网房间列表"},
        {"准备/开始", "加入后发送准备，房主则发送开始信号"},
        {"离开房间", "断开当前房间并回到待机状态"},
    };

    beiklive::netplay::NetplayManager::instance().loadProfile();
    refreshGames();
    registerPageActions();
}

void MultiplayerPage::willAppear(bool resetState)
{
    beiklive::Box::willAppear(resetState);
    refreshGames();
    m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
    brls::Application::giveFocus(this);
}

void MultiplayerPage::frame(brls::FrameContext* ctx)
{
    beiklive::Box::frame(ctx);
    beiklive::netplay::NetplayManager::instance().poll();
    m_snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
}

void MultiplayerPage::draw(NVGcontext* vg, float x, float y, float w, float h,
                           brls::Style style, brls::FrameContext* ctx)
{
    beiklive::Box::draw(vg, x, y, w, h, style, ctx);
    if (m_font < 0)
        m_font = brls::Application::getDefaultFont();

    // 绘制半透明整页遮罩：覆盖 Box 背景之上，位置为整个多人页面区域。
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillColor(vg, nvgRGBA(0, 0, 0, 84));
    nvgFill(vg);

    drawHeader(vg, x + 54.f, y + 34.f, w - 108.f);
    drawActions(vg, x + 54.f, y + 124.f, 320.f, h - 216.f);
    drawRooms(vg, x + 398.f, y + 124.f, 392.f, h - 216.f);
    drawPlayers(vg, x + 814.f, y + 124.f, w - 868.f, 238.f);
    drawGames(vg, x + 814.f, y + 386.f, w - 868.f, h - 478.f);
    drawFooter(vg, x + 54.f, y + h - 68.f, w - 108.f, 36.f);
}

void MultiplayerPage::refreshGames()
{
    m_gbaGames.clear();
    if (!beiklive::GameDB)
        return;

    auto games = beiklive::GameDB->getByPlatform(beiklive::enums::EmuPlatform::EmuGBA);
    for (auto& game : games)
    {
        if (isGbaEntry(game))
            m_gbaGames.push_back(std::move(game));
    }
    if (m_gameIndex >= static_cast<int>(m_gbaGames.size()))
        m_gameIndex = static_cast<int>(m_gbaGames.size()) - 1;
    if (m_gameIndex < 0)
        m_gameIndex = 0;
}

void MultiplayerPage::registerPageActions()
{
    registerAction("选择", brls::BUTTON_A, [this](brls::View*) {
        activateCurrent();
        return true;
    });
    registerAction("返回", brls::BUTTON_B, [this](brls::View*) {
        closePage();
        return true;
    });
    registerAction("创建房间", brls::BUTTON_X, [this](brls::View*) {
        switchPanel(Panel::Games);
        if (!m_gbaGames.empty())
            beiklive::netplay::NetplayManager::instance().startHosting(m_gbaGames[m_gameIndex]);
        return true;
    });
    registerAction("扫描房间", brls::BUTTON_Y, [this](brls::View*) {
        switchPanel(Panel::Rooms);
        beiklive::netplay::NetplayManager::instance().startScanning();
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
        if (m_panel == Panel::Rooms)
            switchPanel(Panel::Actions);
        else if (m_panel == Panel::Games)
            switchPanel(Panel::Rooms);
        return true;
    }, true, true, brls::SOUND_NONE);
    registerAction("", brls::BUTTON_RIGHT, [this](brls::View*) {
        if (m_panel == Panel::Actions)
            switchPanel(Panel::Rooms);
        else if (m_panel == Panel::Rooms)
            switchPanel(Panel::Games);
        return true;
    }, true, true, brls::SOUND_NONE);
}

void MultiplayerPage::activateCurrent()
{
    auto& manager = beiklive::netplay::NetplayManager::instance();
    if (m_panel == Panel::Actions)
    {
        switch (m_actionIndex)
        {
        case 0:
            switchPanel(Panel::Games);
            if (!m_gbaGames.empty())
                manager.startHosting(m_gbaGames[m_gameIndex]);
            else
                brls::Application::notify("没有可用于联机的 GBA 游戏");
            break;
        case 1:
            switchPanel(Panel::Rooms);
            manager.startScanning();
            break;
        case 2:
            if (m_snapshot.hosting)
                manager.startGame();
            else
                manager.markLocalReady();
            break;
        case 3:
            manager.leaveRoom();
            break;
        default:
            break;
        }
        return;
    }

    if (m_panel == Panel::Rooms)
    {
        if (m_roomIndex >= 0 && m_roomIndex < static_cast<int>(m_snapshot.rooms.size()))
            manager.joinRoom(m_snapshot.rooms[m_roomIndex].roomId);
        return;
    }

    if (m_panel == Panel::Games)
    {
        if (!m_gbaGames.empty())
            manager.startHosting(m_gbaGames[m_gameIndex]);
    }
}

void MultiplayerPage::moveSelection(int delta)
{
    if (m_panel == Panel::Actions)
    {
        const int count = static_cast<int>(m_actions.size());
        m_actionIndex = (m_actionIndex + delta + count) % count;
    }
    else if (m_panel == Panel::Rooms)
    {
        const int count = std::max(1, static_cast<int>(m_snapshot.rooms.size()));
        m_roomIndex = (m_roomIndex + delta + count) % count;
    }
    else
    {
        const int count = std::max(1, static_cast<int>(m_gbaGames.size()));
        m_gameIndex = (m_gameIndex + delta + count) % count;
    }
}

void MultiplayerPage::switchPanel(Panel panel)
{
    m_panel = panel;
}

void MultiplayerPage::closePage()
{
    brls::sync([this]() {
        beiklive::popActivity(this);
    });
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

void MultiplayerPage::drawActions(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制左侧操作区：四个直接操作按钮，位置为页面左列。
    drawRoundedRect(vg, x, y, w, h, 8.f, colorBgPanel(), colorStroke(), 1.f);
    drawText(vg, "操作", x + 20.f, y + 28.f, 20.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    float itemY = y + 58.f;
    for (int i = 0; i < static_cast<int>(m_actions.size()); ++i)
    {
        const bool focused = m_panel == Panel::Actions && i == m_actionIndex;
        drawRoundedRect(vg, x + 14.f, itemY, w - 28.f, 72.f, 7.f,
                        focused ? colorAccentSoft() : colorPanelSoft(),
                        focused ? colorAccent() : colorStroke(),
                        focused ? 2.f : 1.f);
        drawText(vg, m_actions[i].title, x + 32.f, itemY + 23.f, 18.f, colorText(),
                 NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        drawText(vg, m_actions[i].subtitle, x + 32.f, itemY + 49.f, 13.f, colorMuted(),
                 NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        itemY += 84.f;
    }
}

void MultiplayerPage::drawRooms(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制中间房间列表：每个房间展示房主、游戏、CRC 和人数。
    drawRoundedRect(vg, x, y, w, h, 8.f, colorBgPanel(), colorStroke(), 1.f);
    drawText(vg, "局域网房间", x + 20.f, y + 28.f, 20.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    drawText(vg, m_snapshot.scanning ? "扫描中" : "待机", x + w - 20.f, y + 28.f, 14.f,
             m_snapshot.scanning ? colorAccent() : colorMuted(), NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);

    if (m_snapshot.rooms.empty())
    {
        drawText(vg, "暂无房间，按 Y 扫描", x + w * 0.5f, y + h * 0.5f, 18.f, colorMuted(),
                 NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        return;
    }

    float itemY = y + 58.f;
    for (int i = 0; i < static_cast<int>(m_snapshot.rooms.size()) && itemY + 86.f < y + h - 16.f; ++i)
    {
        const auto& room = m_snapshot.rooms[i];
        const bool focused = m_panel == Panel::Rooms && i == m_roomIndex;
        drawRoundedRect(vg, x + 14.f, itemY, w - 28.f, 82.f, 7.f,
                        focused ? colorAccentSoft() : colorPanelSoft(),
                        focused ? colorAccent() : colorStroke(),
                        focused ? 2.f : 1.f);
        drawText(vg, room.title.empty() ? "未知 GBA 游戏" : room.title,
                 x + 32.f, itemY + 22.f, 17.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        drawText(vg, "Host: " + room.hostName + "   CRC " + crcText(room.crc32),
                 x + 32.f, itemY + 48.f, 13.f, colorMuted(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        drawText(vg, std::to_string(room.players) + "/" + std::to_string(room.maxPlayers),
                 x + w - 32.f, itemY + 41.f, 16.f, colorAccent(), NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        itemY += 94.f;
    }
}

void MultiplayerPage::drawPlayers(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制右上玩家槽位：最多 4 人，预留未来 Link Cable 扩展。
    drawRoundedRect(vg, x, y, w, h, 8.f, colorBgPanel(), colorStroke(), 1.f);
    drawText(vg, "玩家", x + 20.f, y + 28.f, 20.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    const float gap = 12.f;
    const float slotW = (w - 40.f - gap) * 0.5f;
    const float slotH = 76.f;
    for (int i = 0; i < 4; ++i)
    {
        const float sx = x + 20.f + (i % 2) * (slotW + gap);
        const float sy = y + 58.f + (i / 2) * (slotH + gap);
        const auto& player = m_snapshot.players[i];
        const bool occupied = !player.name.empty();
        drawRoundedRect(vg, sx, sy, slotW, slotH, 7.f,
                        occupied ? colorPanelSoft() : nvgRGBA(255, 255, 255, 18),
                        occupied ? colorStroke() : nvgRGBA(255, 255, 255, 28), 1.f);

        // 绘制玩家头像圆点：位于每个玩家槽左侧。
        nvgBeginPath(vg);
        nvgCircle(vg, sx + 28.f, sy + slotH * 0.5f, 15.f);
        nvgFillColor(vg, occupied ? colorAccent() : nvgRGBA(120, 128, 136, 120));
        nvgFill(vg);

        drawText(vg, occupied ? player.name : ("空位 " + std::to_string(i + 1)),
                 sx + 54.f, sy + 28.f, 16.f, occupied ? colorText() : colorMuted(),
                 NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        drawText(vg, occupied ? (player.ready ? "READY" : "WAIT") : "开放",
                 sx + 54.f, sy + 52.f, 12.f, occupied && player.ready ? colorAccent() : colorMuted(),
                 NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    }
}

void MultiplayerPage::drawGames(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制右下 GBA 游戏选择区：创建房间时用当前选中游戏。
    drawRoundedRect(vg, x, y, w, h, 8.f, colorBgPanel(), colorStroke(), 1.f);
    drawText(vg, "GBA 游戏", x + 20.f, y + 28.f, 20.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    if (m_gbaGames.empty())
    {
        drawText(vg, "游戏库里还没有 GBA 游戏", x + w * 0.5f, y + h * 0.5f, 17.f, colorMuted(),
                 NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        return;
    }

    float itemY = y + 58.f;
    for (int i = 0; i < static_cast<int>(m_gbaGames.size()) && itemY + 62.f < y + h - 14.f; ++i)
    {
        const auto& game = m_gbaGames[i];
        const bool focused = m_panel == Panel::Games && i == m_gameIndex;
        drawRoundedRect(vg, x + 14.f, itemY, w - 28.f, 58.f, 7.f,
                        focused ? colorAccentSoft() : colorPanelSoft(),
                        focused ? colorAccent() : colorStroke(),
                        focused ? 2.f : 1.f);
        drawText(vg, game.title.empty() ? beiklive::tools::getFileNameWithoutExtension(game.path) : game.title,
                 x + 30.f, itemY + 22.f, 16.f, colorText(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        drawText(vg, "CRC " + crcText(static_cast<uint32_t>(game.crc32)),
                 x + 30.f, itemY + 44.f, 12.f, colorMuted(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        itemY += 68.f;
    }
}

void MultiplayerPage::drawFooter(NVGcontext* vg, float x, float y, float w, float h) const
{
    // 绘制底部按键提示：位置为页面底边，说明当前可用手柄操作。
    drawRoundedRect(vg, x, y, w, h, 8.f, nvgRGBA(10, 12, 16, 142), colorStroke(), 1.f);
    drawText(vg, "A 选择     B 返回     X 创建房间     Y 扫描     方向键切换",
             x + w * 0.5f, y + h * 0.5f, 15.f, colorMuted(), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
}

} // namespace beiklive
