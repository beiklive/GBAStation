#include "network/netplay/NetplayManager.hpp"

#include "core/Tools.hpp"

#include <algorithm>
#include <chrono>

namespace beiklive::netplay
{
namespace
{
bool isGbaPlatform(int platform)
{
    return platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGBA);
}
} // namespace

const char* toString(NetplayState state)
{
    switch (state)
    {
    case NetplayState::Idle: return "Idle";
    case NetplayState::Hosting: return "Hosting";
    case NetplayState::WaitingPlayer: return "WaitingPlayer";
    case NetplayState::Connected: return "Connected";
    case NetplayState::LoadingGame: return "LoadingGame";
    case NetplayState::WaitingReady: return "WaitingReady";
    case NetplayState::Running: return "Running";
    case NetplayState::Disconnected: return "Disconnected";
    }
    return "Unknown";
}

NetplayManager& NetplayManager::instance()
{
    static NetplayManager manager;
    return manager;
}

NetplaySnapshot NetplayManager::snapshot() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_snapshot;
}

void NetplayManager::loadProfile()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshot.nickname = GET_SETTING_KEY_STR("netplay.nickname", "Player");
    m_snapshot.players[0].playerId = 0;
    m_snapshot.players[0].avatar = static_cast<uint8_t>(GET_SETTING_KEY_INT("netplay.avatar", 0));
    m_snapshot.players[0].name = m_snapshot.nickname;
    setStatusLocked("多人联机待机中");
}

void NetplayManager::saveProfile(const std::string& nickname, uint8_t avatar)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshot.nickname = nickname.empty() ? "Player" : nickname.substr(0, 31);
    m_snapshot.players[0].name = m_snapshot.nickname;
    m_snapshot.players[0].avatar = avatar;
    SET_SETTING_KEY_STR("netplay.nickname", m_snapshot.nickname);
    SET_SETTING_KEY_INT("netplay.avatar", avatar);
}

bool NetplayManager::startHosting(const beiklive::GameEntry& game)
{
    if (!isGbaPlatform(game.platform) || game.path.empty())
        return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshot.state = NetplayState::WaitingPlayer;
    m_snapshot.hosting = true;
    m_snapshot.scanning = false;
    m_snapshot.localPlayerId = 0;
    m_snapshot.currentRoom = {};
    m_snapshot.currentRoom.roomId = makeRoomId();
    m_snapshot.currentRoom.hostName = m_snapshot.nickname.empty() ? "Player" : m_snapshot.nickname;
    m_snapshot.currentRoom.avatar = m_snapshot.players[0].avatar;
    m_snapshot.currentRoom.title = game.title.empty() ? beiklive::tools::getFileNameWithoutExtension(game.path) : game.title;
    m_snapshot.currentRoom.crc32 = static_cast<uint32_t>(game.crc32);
    m_snapshot.currentRoom.players = 1;
    m_snapshot.currentRoom.maxPlayers = 2;
    m_snapshot.players[0].ready = true;
    m_snapshot.players[1] = {};
    setStatusLocked("已创建房间，等待玩家加入");
    return true;
}

void NetplayManager::startScanning()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshot.state = NetplayState::Idle;
    m_snapshot.hosting = false;
    m_snapshot.scanning = true;
    rebuildDemoRoomsLocked();
    setStatusLocked("正在扫描局域网房间");
}

void NetplayManager::stopScanning()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshot.scanning = false;
    setStatusLocked("已停止扫描");
}

bool NetplayManager::joinRoom(uint64_t roomId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_snapshot.rooms.begin(), m_snapshot.rooms.end(), [roomId](const RoomInfo& room) {
        return room.roomId == roomId;
    });
    if (it == m_snapshot.rooms.end())
        return false;

    m_snapshot.currentRoom = *it;
    m_snapshot.currentRoom.players = std::max<uint8_t>(2, m_snapshot.currentRoom.players);
    m_snapshot.state = NetplayState::Connected;
    m_snapshot.hosting = false;
    m_snapshot.scanning = false;
    m_snapshot.localPlayerId = 1;
    const uint8_t localAvatar = m_snapshot.players[0].avatar;
    m_snapshot.players[0] = {0, it->avatar, true, it->hostName};
    m_snapshot.players[1] = {1, localAvatar, false, m_snapshot.nickname};
    setStatusLocked("已加入房间，等待房主开始");
    return true;
}

void NetplayManager::leaveRoom()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshot.state = NetplayState::Idle;
    m_snapshot.hosting = false;
    m_snapshot.scanning = false;
    m_snapshot.currentRoom = {};
    for (auto& player : m_snapshot.players)
        player = {};
    m_snapshot.players[0].playerId = 0;
    m_snapshot.players[0].name = m_snapshot.nickname.empty() ? "Player" : m_snapshot.nickname;
    m_incomingLinkData.clear();
    setStatusLocked("已离开房间");
}

void NetplayManager::markLocalReady()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshot.players[m_snapshot.localPlayerId].ready = true;
    m_snapshot.state = NetplayState::WaitingReady;
    setStatusLocked("本机已准备，等待同步信号");
}

void NetplayManager::startGame()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_snapshot.hosting || m_snapshot.currentRoom.roomId == 0)
        return;

    m_snapshot.state = NetplayState::LoadingGame;
    m_snapshot.players[0].ready = true;
    if (m_snapshot.currentRoom.players < 2)
    {
        m_snapshot.currentRoom.players = 2;
        m_snapshot.players[1] = {1, 1, true, "Guest"};
    }
    setStatusLocked("开始游戏信号已发出");
}

void NetplayManager::poll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_snapshot.scanning && m_snapshot.rooms.empty())
        rebuildDemoRoomsLocked();
}

void NetplayManager::pushIncomingLinkData(const LinkDataPacket& packet)
{
    if (m_incomingLinkData.push(packet))
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_snapshot.rxPackets;
    }
}

bool NetplayManager::popIncomingLinkData(LinkDataPacket& packet)
{
    auto value = m_incomingLinkData.pop();
    if (!value)
        return false;
    packet = *value;
    return true;
}

void NetplayManager::sendLinkData(const LinkDataPacket& packet)
{
    (void)packet;
    std::lock_guard<std::mutex> lock(m_mutex);
    ++m_snapshot.txPackets;
}

uint64_t NetplayManager::makeRoomId() const
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return static_cast<uint64_t>(now) ^ 0xB3114C00123ULL;
}

std::optional<beiklive::GameEntry> NetplayManager::firstGbaGame() const
{
    if (!beiklive::GameDB)
        return std::nullopt;

    const auto games = beiklive::GameDB->getByPlatform(beiklive::enums::EmuPlatform::EmuGBA);
    if (games.empty())
        return std::nullopt;
    return games.front();
}

void NetplayManager::rebuildDemoRoomsLocked()
{
    m_snapshot.rooms.clear();
    auto game = firstGbaGame();
    if (!game)
        return;

    RoomInfo room;
    room.roomId = makeRoomId() ^ 0x55AA55AAu;
    room.hostName = "LAN Host";
    room.avatar = 1;
    room.title = game->title.empty() ? beiklive::tools::getFileNameWithoutExtension(game->path) : game->title;
    room.crc32 = static_cast<uint32_t>(game->crc32);
    room.players = 1;
    room.maxPlayers = 2;
    room.ping = 0;
    room.endpoint = "discovery-pending";
    m_snapshot.rooms.push_back(room);
}

void NetplayManager::setStatusLocked(const std::string& text)
{
    m_snapshot.statusText = text;
}

} // namespace beiklive::netplay
