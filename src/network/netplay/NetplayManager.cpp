#include "network/netplay/NetplayManager.hpp"

#include "core/Tools.hpp"

#include <borealis.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>

namespace beiklive::netplay
{
namespace
{
constexpr uint16_t CONTROL_PORT = 45872;
constexpr auto HANDSHAKE_DELAY = std::chrono::milliseconds(1000);
constexpr auto HANDSHAKE_RETRY = std::chrono::milliseconds(1000);
constexpr auto RUN_RELEASE_DELAY = std::chrono::milliseconds(250);

const char* packetTypeName(PacketType type)
{
    switch (type)
    {
    case PacketType::Discover: return "Discover";
    case PacketType::RoomInfo: return "RoomInfo";
    case PacketType::JoinRequest: return "JoinRequest";
    case PacketType::JoinAccept: return "JoinAccept";
    case PacketType::JoinReject: return "JoinReject";
    case PacketType::Leave: return "Leave";
    case PacketType::StartGame: return "StartGame";
    case PacketType::Ready: return "Ready";
    case PacketType::Go: return "Go";
    case PacketType::Heartbeat: return "Heartbeat";
    case PacketType::Disconnect: return "Disconnect";
    case PacketType::LinkData: return "LinkData";
    case PacketType::CoreReady: return "CoreReady";
    case PacketType::RunGo: return "RunGo";
    }
    return "Unknown";
}

const char* linkModeName(uint8_t mode)
{
    switch (mode)
    {
    case 0: return "SIO_NORMAL_8";
    case 1: return "SIO_NORMAL_32";
    case 2: return "SIO_MULTI";
    default: return "SIO_OTHER";
    }
}

bool shouldLogLinkCounter(uint32_t count)
{
    return count <= 24 || (count % 300) == 0;
}

bool isLinkPlatform(int platform)
{
    return platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGBA) ||
           platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGBC) ||
           platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGB);
}

std::vector<std::string> split(const std::string& text, char sep)
{
    std::vector<std::string> out;
    std::string item;
    std::istringstream stream(text);
    while (std::getline(stream, item, sep))
        out.push_back(item);
    return out;
}

std::string sanitizeField(std::string text)
{
    for (char& ch : text)
        if (ch == '|')
            ch = '/';
    return text;
}

std::vector<uint8_t> toPayload(const std::string& text)
{
    return {text.begin(), text.end()};
}

std::string fromPayload(const std::vector<uint8_t>& payload)
{
    return {payload.begin(), payload.end()};
}

std::vector<uint8_t> makeJoinAcceptPayload(const RoomInfo& room)
{
    std::ostringstream out;
    out << room.roomId << '|'
        << sanitizeField(room.hostName) << '|'
        << static_cast<int>(room.avatar) << '|'
        << sanitizeField(room.title) << '|'
        << room.crc32 << "|1";
    return toPayload(out.str());
}

std::string endpointHost(const std::string& endpoint)
{
    const auto sep = endpoint.find(':');
    return sep == std::string::npos ? endpoint : endpoint.substr(0, sep);
}

uint16_t endpointPort(const std::string& endpoint, uint16_t fallback)
{
    const auto sep = endpoint.rfind(':');
    if (sep == std::string::npos)
        return fallback;
    try
    {
        const int port = std::stoi(endpoint.substr(sep + 1));
        if (port > 0 && port <= 65535)
            return static_cast<uint16_t>(port);
    }
    catch (...)
    {
    }
    return fallback;
}

uint64_t parseU64(const std::string& text)
{
    try { return static_cast<uint64_t>(std::stoull(text)); }
    catch (...) { return 0; }
}

uint32_t parseU32(const std::string& text)
{
    try { return static_cast<uint32_t>(std::stoul(text)); }
    catch (...) { return 0; }
}

int parseInt(const std::string& text)
{
    try { return std::stoi(text); }
    catch (...) { return 0; }
}

void writeLe16(std::vector<uint8_t>& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void writeLe32(std::vector<uint8_t>& out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void writeLe64(std::vector<uint8_t>& out, uint64_t value)
{
    writeLe32(out, static_cast<uint32_t>(value & 0xFFFFFFFFULL));
    writeLe32(out, static_cast<uint32_t>(value >> 32));
}

uint16_t readLe16(const std::vector<uint8_t>& in, size_t offset)
{
    return static_cast<uint16_t>(in[offset]) |
           static_cast<uint16_t>(in[offset + 1] << 8);
}

uint32_t readLe32(const std::vector<uint8_t>& in, size_t offset)
{
    return static_cast<uint32_t>(in[offset]) |
           (static_cast<uint32_t>(in[offset + 1]) << 8) |
           (static_cast<uint32_t>(in[offset + 2]) << 16) |
           (static_cast<uint32_t>(in[offset + 3]) << 24);
}

uint64_t readLe64(const std::vector<uint8_t>& in, size_t offset)
{
    return static_cast<uint64_t>(readLe32(in, offset)) |
           (static_cast<uint64_t>(readLe32(in, offset + 4)) << 32);
}

std::vector<uint8_t> encodeLinkData(const LinkDataPacket& packet)
{
    std::vector<uint8_t> out;
    out.reserve(19);
    writeLe64(out, packet.cycle);
    writeLe32(out, packet.data);
    writeLe16(out, packet.siocnt);
    writeLe16(out, packet.rcnt);
    out.push_back(packet.flags);
    out.push_back(packet.playerId);
    out.push_back(packet.mode);
    return out;
}

bool decodeLinkData(const std::vector<uint8_t>& payload, LinkDataPacket& packet)
{
    if (payload.size() < 19)
        return false;

    packet.cycle = readLe64(payload, 0);
    packet.data = readLe32(payload, 8);
    packet.siocnt = readLe16(payload, 12);
    packet.rcnt = readLe16(payload, 14);
    packet.flags = payload[16];
    packet.playerId = payload[17];
    packet.mode = payload[18];
    return true;
}

uint32_t crc32Update(uint32_t crc, const uint8_t* data, size_t size)
{
    static std::array<uint32_t, 256> table{};
    static bool initialized = false;
    if (!initialized)
    {
        for (uint32_t i = 0; i < table.size(); ++i)
        {
            uint32_t value = i;
            for (int bit = 0; bit < 8; ++bit)
                value = (value >> 1) ^ (0xEDB88320u & (0u - (value & 1u)));
            table[i] = value;
        }
        initialized = true;
    }

    crc = ~crc;
    for (size_t i = 0; i < size; ++i)
        crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    return ~crc;
}

uint32_t computeFileCrc32(const std::string& path)
{
    if (path.empty())
        return 0;

    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        brls::Logger::warning("[Netplay][Manager] CRC32 open failed path={}", path);
        return 0;
    }

    uint32_t crc = 0;
    std::array<uint8_t, 64 * 1024> buffer{};
    while (file)
    {
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const auto count = file.gcount();
        if (count > 0)
            crc = crc32Update(crc, buffer.data(), static_cast<size_t>(count));
    }
    return crc;
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

NetplayManager::~NetplayManager()
{
    brls::Logger::info("[Netplay][Manager] shutdown");
    m_transport.stop();
    m_discovery.stop();
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
    brls::Logger::info("[Netplay][Manager] profile loaded nickname={} avatar={}",
                       m_snapshot.nickname, static_cast<int>(m_snapshot.players[0].avatar));
}

void NetplayManager::saveProfile(const std::string& nickname, uint8_t avatar)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshot.nickname = nickname.empty() ? "Player" : nickname.substr(0, 31);
    m_snapshot.players[0].name = m_snapshot.nickname;
    m_snapshot.players[0].avatar = avatar;
    SET_SETTING_KEY_STR("netplay.nickname", m_snapshot.nickname);
    SET_SETTING_KEY_INT("netplay.avatar", avatar);
    brls::Logger::info("[Netplay][Manager] profile saved nickname={} avatar={}",
                       m_snapshot.nickname, static_cast<int>(avatar));
}

bool NetplayManager::startHosting(const beiklive::GameEntry& game)
{
    if (!isLinkPlatform(game.platform) || game.path.empty())
    {
        brls::Logger::warning("[Netplay][Manager] startHosting rejected platform={} pathEmpty={}",
                              game.platform, game.path.empty() ? 1 : 0);
        return false;
    }

    beiklive::GameEntry roomGame = game;
    if (roomGame.crc32 == 0)
    {
        const uint32_t computedCrc = computeFileCrc32(roomGame.path);
        if (computedCrc != 0)
        {
            roomGame.crc32 = static_cast<int>(computedCrc);
            if (beiklive::GameDB)
            {
                beiklive::GameDB->upsertByPath(roomGame);
                beiklive::GameDB->flush();
            }
            brls::Logger::info("[Netplay][Manager] computed room CRC32 path={} crc={:#x}",
                               roomGame.path, computedCrc);
        }
        else
        {
            brls::Logger::warning("[Netplay][Manager] startHosting CRC32 remains zero path={}",
                                  roomGame.path);
        }
    }

    RoomInfo room;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        room.roomId = makeRoomId();
        room.hostName = m_snapshot.nickname.empty() ? "Player" : m_snapshot.nickname;
        room.avatar = m_snapshot.players[0].avatar;
        room.title = roomGame.title.empty() ? beiklive::tools::getFileNameWithoutExtension(roomGame.path) : roomGame.title;
        room.crc32 = static_cast<uint32_t>(roomGame.crc32);
        room.players = 1;
        room.maxPlayers = 2;
        room.ping = 0;
        room.endpoint = "local";
        m_snapshot.scanning = false;
        m_snapshot.rooms.clear();
        m_clientJoinPhase = ClientJoinPhase::None;
        m_clientJoinHost.clear();
        m_clientJoinPort = 0;
        m_clientDiscoverPayload.clear();
        m_clientJoinPayload.clear();
        m_delayedPackets.clear();
        m_runGoQueued = false;
        m_runReleasePending = false;
        m_runReleaseRoomId = 0;
        m_runReleaseDue = {};
        setStatusLocked("正在创建局域网房间");
        brls::Logger::info("[Netplay][Manager] startHosting prepare room id={} title={} crc={:#x} platform={} path={}",
                           room.roomId, room.title, room.crc32, roomGame.platform, roomGame.path);
    }

    const bool discoveryOk = m_discovery.startHosting(room);
    if (discoveryOk)
        room.endpoint = UdpSocket::detectLocalAddress() + ":" + std::to_string(CONTROL_PORT);
    const bool transportOk = discoveryOk && m_transport.start(CONTROL_PORT, [this](PacketType type, std::vector<uint8_t> payload, std::string host, uint16_t port) {
        handleControlPacket(type, std::move(payload), std::move(host), port);
    });

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!discoveryOk || !transportOk)
    {
        m_discovery.stop();
        m_transport.stop();
        m_snapshot.state = NetplayState::Idle;
        m_snapshot.hosting = false;
        m_snapshot.scanning = false;
        m_snapshot.currentRoom = {};
        setStatusLocked(discoveryOk ? "创建房间失败：无法启动控制端口" : "创建房间失败：无法启动局域网广播");
        brls::Logger::error("[Netplay][Manager] startHosting failed discoveryOk={} transportOk={} endpoint={}",
                            discoveryOk ? 1 : 0, transportOk ? 1 : 0, room.endpoint);
        return false;
    }

    m_snapshot.state = NetplayState::WaitingPlayer;
    m_snapshot.hosting = true;
    m_snapshot.scanning = false;
    m_snapshot.localPlayerId = 0;
    m_snapshot.currentRoom = std::move(room);
    m_snapshot.players[0].ready = true;
    m_snapshot.players[1] = {};
    setStatusLocked("已创建房间，等待玩家加入：" + m_snapshot.currentRoom.endpoint);
    brls::Logger::info("[Netplay][Manager] host ready room={} endpoint={} localPlayerId={} state={}",
                       m_snapshot.currentRoom.roomId, m_snapshot.currentRoom.endpoint,
                       static_cast<int>(m_snapshot.localPlayerId), toString(m_snapshot.state));
    return true;
}

void NetplayManager::startScanning()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_snapshot.state = NetplayState::Idle;
        m_snapshot.hosting = false;
        m_snapshot.scanning = true;
        m_snapshot.rooms.clear();
        m_clientJoinPhase = ClientJoinPhase::None;
        m_clientJoinHost.clear();
        m_clientJoinPort = 0;
        m_clientDiscoverPayload.clear();
        m_clientJoinPayload.clear();
        m_delayedPackets.clear();
        m_runGoQueued = false;
        m_runReleasePending = false;
        m_runReleaseRoomId = 0;
        m_runReleaseDue = {};
        setStatusLocked("正在扫描局域网房间");
        brls::Logger::info("[Netplay][Manager] startScanning");
    }

    const bool discoveryOk = m_discovery.startScanning([this](std::vector<RoomInfo> rooms) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_snapshot.scanning)
            return;

        m_snapshot.rooms = std::move(rooms);
        if (m_snapshot.rooms.empty())
            setStatusLocked("正在扫描局域网房间");
        else
            setStatusLocked("已发现 " + std::to_string(m_snapshot.rooms.size()) + " 个房间");
        brls::Logger::info("[Netplay][Manager] scan update rooms={}", m_snapshot.rooms.size());
    });

    if (!discoveryOk)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_snapshot.scanning = false;
        m_snapshot.rooms.clear();
        setStatusLocked("扫描失败：无法启动局域网监听");
        brls::Logger::error("[Netplay][Manager] startScanning failed");
    }
}

void NetplayManager::stopScanning()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_snapshot.scanning = false;
        setStatusLocked("已停止扫描");
        brls::Logger::info("[Netplay][Manager] stopScanning");
    }
    m_discovery.stop();
}

bool NetplayManager::joinRoom(uint64_t roomId)
{
    RoomInfo room;
    std::string host;
    std::string nickname;
    uint8_t avatar = 0;
    std::vector<uint8_t> joinPayload;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find_if(m_snapshot.rooms.begin(), m_snapshot.rooms.end(), [roomId](const RoomInfo& room) {
            return room.roomId == roomId;
        });
        if (it == m_snapshot.rooms.end())
        {
            brls::Logger::warning("[Netplay][Manager] joinRoom failed roomId={} not found", roomId);
            return false;
        }

        room = *it;
        host = endpointHost(room.endpoint);
        nickname = m_snapshot.nickname.empty() ? "Player" : m_snapshot.nickname;
        avatar = m_snapshot.players[0].avatar;

        m_snapshot.currentRoom = room;
        m_snapshot.state = NetplayState::Idle;
        m_snapshot.hosting = false;
        m_snapshot.scanning = false;
        m_snapshot.localPlayerId = 1;
        m_peerHost = host;
        m_peerPort = CONTROL_PORT;
        m_runGoQueued = false;
        m_runReleasePending = false;
        m_runReleaseRoomId = 0;
        m_runReleaseDue = {};
        setStatusLocked("正在发送加入请求");
        brls::Logger::info("[Netplay][Manager] joinRoom room={} host={} title={} crc={:#x}",
                           room.roomId, host, room.title, room.crc32);
    }
    m_discovery.stop();

    if (host.empty() || !m_transport.start(0, [this](PacketType type, std::vector<uint8_t> payload, std::string packetHost, uint16_t packetPort) {
            handleControlPacket(type, std::move(payload), std::move(packetHost), packetPort);
        }))
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_snapshot.state = NetplayState::Idle;
        setStatusLocked("加入失败：无法启动控制端口");
        brls::Logger::error("[Netplay][Manager] joinRoom transport start failed host={}", host);
        return false;
    }

    std::ostringstream payload;
    payload << room.roomId << '|'
            << sanitizeField(nickname) << '|'
            << static_cast<int>(avatar) << '|'
            << room.crc32 << '|'
            << sanitizeField(room.title);
    joinPayload = toPayload(payload.str());
    if (!m_transport.sendPacket(host, CONTROL_PORT, PacketType::JoinRequest, joinPayload))
    {
        m_transport.stop();
        std::lock_guard<std::mutex> lock(m_mutex);
        m_snapshot.state = NetplayState::Idle;
        setStatusLocked("加入失败：发送请求失败");
        brls::Logger::error("[Netplay][Manager] joinRoom send JoinRequest failed host={} port={}", host, CONTROL_PORT);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_clientJoinPhase = ClientJoinPhase::WaitingApproval;
        m_clientJoinHost = host;
        m_clientJoinPort = CONTROL_PORT;
        m_clientDiscoverPayload.clear();
        m_clientJoinPayload = joinPayload;
        m_nextClientRetry = std::chrono::steady_clock::now() + HANDSHAKE_RETRY;
        setStatusLocked("已发送正式加入请求，等待房主同意");
        brls::Logger::info("[Netplay][Manager] JoinRequest sent room={} host={} port={}", room.roomId, host, CONTROL_PORT);
    }
    return true;
}

bool NetplayManager::joinManual(const std::string& host, uint16_t port)
{
    if (host.empty() || port == 0)
    {
        brls::Logger::warning("[Netplay][Manager] joinManual rejected host={} port={}", host, port);
        return false;
    }

    std::string nickname;
    uint8_t avatar = 0;
    std::vector<uint8_t> discoverPayload;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        nickname = m_snapshot.nickname.empty() ? "Player" : m_snapshot.nickname;
        avatar = m_snapshot.players[0].avatar;
        m_snapshot.state = NetplayState::Idle;
        m_snapshot.hosting = false;
        m_snapshot.scanning = false;
        m_snapshot.localPlayerId = 1;
        m_snapshot.currentRoom = {};
        m_snapshot.currentRoom.endpoint = host + ":" + std::to_string(port);
        m_peerHost = host;
        m_peerPort = port;
        m_runGoQueued = false;
        m_runReleasePending = false;
        m_runReleaseRoomId = 0;
        m_runReleaseDue = {};
        setStatusLocked("正在请求手动房间信息");
        brls::Logger::info("[Netplay][Manager] joinManual host={} port={} nickname={} avatar={}",
                           host, port, nickname, static_cast<int>(avatar));
    }
    m_discovery.stop();

    if (!m_transport.start(0, [this](PacketType type, std::vector<uint8_t> payload, std::string packetHost, uint16_t packetPort) {
            handleControlPacket(type, std::move(payload), std::move(packetHost), packetPort);
        }))
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        setStatusLocked("手动加入失败：无法启动控制端口");
        brls::Logger::error("[Netplay][Manager] joinManual transport start failed host={} port={}", host, port);
        return false;
    }

    std::ostringstream payload;
    payload << sanitizeField(nickname) << '|' << static_cast<int>(avatar);
    discoverPayload = toPayload(payload.str());
    if (!m_transport.sendPacket(host, port, PacketType::Discover, discoverPayload))
    {
        m_transport.stop();
        std::lock_guard<std::mutex> lock(m_mutex);
        setStatusLocked("手动加入失败：发送请求失败");
        brls::Logger::error("[Netplay][Manager] joinManual Discover send failed host={} port={}", host, port);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_clientJoinPhase = ClientJoinPhase::RequestingRoomInfo;
        m_clientJoinHost = host;
        m_clientJoinPort = port;
        m_clientDiscoverPayload = discoverPayload;
        m_clientJoinPayload.clear();
        m_nextClientRetry = std::chrono::steady_clock::now() + HANDSHAKE_RETRY;
        setStatusLocked("已发送房间信息请求，等待房主回复房间信息");
        brls::Logger::info("[Netplay][Manager] manual Discover sent host={} port={}", host, port);
    }
    return true;
}

void NetplayManager::leaveRoom()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_snapshot.state = NetplayState::Idle;
        m_snapshot.hosting = false;
        m_snapshot.scanning = false;
        m_snapshot.currentRoom = {};
        m_snapshot.pendingJoin = {};
        m_snapshot.rooms.clear();
        for (auto& player : m_snapshot.players)
            player = {};
        m_snapshot.players[0].playerId = 0;
        m_snapshot.players[0].name = m_snapshot.nickname.empty() ? "Player" : m_snapshot.nickname;
        m_peerHost.clear();
        m_peerPort = 0;
        m_clientJoinPhase = ClientJoinPhase::None;
        m_clientJoinHost.clear();
        m_clientJoinPort = 0;
        m_clientDiscoverPayload.clear();
        m_clientJoinPayload.clear();
        m_delayedPackets.clear();
        m_incomingLinkData.clear();
        m_runGoQueued = false;
        m_runReleasePending = false;
        m_runReleaseRoomId = 0;
        m_runReleaseDue = {};
        setStatusLocked("已离开房间");
        brls::Logger::info("[Netplay][Manager] leaveRoom");
    }
    m_discovery.stop();
    m_transport.stop();
}

void NetplayManager::approvePendingJoin()
{
    std::string host;
    uint16_t port = 0;
    std::vector<uint8_t> replyPayload;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_snapshot.hosting || !m_snapshot.pendingJoin.active ||
            !m_snapshot.pendingJoin.approvalReady ||
            m_snapshot.pendingJoin.roomId != m_snapshot.currentRoom.roomId)
            return;

        host = endpointHost(m_snapshot.pendingJoin.endpoint);
        port = endpointPort(m_snapshot.pendingJoin.endpoint, CONTROL_PORT);
        m_peerHost = host;
        m_peerPort = port;
        m_snapshot.state = NetplayState::Connected;
        m_snapshot.currentRoom.players = 2;
        m_snapshot.players[1] = {1, m_snapshot.pendingJoin.avatar, false, m_snapshot.pendingJoin.name};

        replyPayload = makeJoinAcceptPayload(m_snapshot.currentRoom);
        setStatusLocked("已同意 " + m_snapshot.pendingJoin.name + " 加入，1秒后发送确认");
        brls::Logger::info("[Netplay][Manager] approve join name={} endpoint={} room={} peer={}:{}",
                           m_snapshot.pendingJoin.name, m_snapshot.pendingJoin.endpoint,
                           m_snapshot.currentRoom.roomId, host, port);
        m_snapshot.pendingJoin = {};
    }

    if (!host.empty())
    {
        queuePacket(host, port, PacketType::JoinAccept, replyPayload, HANDSHAKE_DELAY, "已发送同意信息，等待客户端确认");
        m_discovery.stop();
    }
}

void NetplayManager::rejectPendingJoin()
{
    std::string host;
    uint16_t port = 0;
    uint64_t roomId = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_snapshot.hosting || !m_snapshot.pendingJoin.active)
            return;

        host = endpointHost(m_snapshot.pendingJoin.endpoint);
        port = endpointPort(m_snapshot.pendingJoin.endpoint, CONTROL_PORT);
        roomId = m_snapshot.pendingJoin.roomId;
        setStatusLocked("已拒绝 " + m_snapshot.pendingJoin.name + " 加入，1秒后发送拒绝信息");
        brls::Logger::info("[Netplay][Manager] reject join name={} endpoint={} room={}",
                           m_snapshot.pendingJoin.name, m_snapshot.pendingJoin.endpoint, roomId);
        m_snapshot.pendingJoin = {};
    }

    if (!host.empty())
        queuePacket(host, port, PacketType::JoinReject, toPayload(std::to_string(roomId) + "|房主已拒绝"),
                    HANDSHAKE_DELAY, "已发送拒绝信息");
}

void NetplayManager::markLocalReady()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshot.players[m_snapshot.localPlayerId].ready = true;
    m_snapshot.state = NetplayState::WaitingReady;
    setStatusLocked("本机已准备，等待同步信号");
    brls::Logger::info("[Netplay][Manager] local ready playerId={} room={} state={}",
                       static_cast<int>(m_snapshot.localPlayerId), m_snapshot.currentRoom.roomId,
                       toString(m_snapshot.state));
}

void NetplayManager::markCoreReady()
{
    std::string peerHost;
    uint16_t peerPort = 0;
    uint64_t roomId = 0;
    uint8_t localPlayerId = 0;
    bool sendCoreReady = false;
    bool queueRunGo = false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_snapshot.currentRoom.roomId == 0 ||
            m_snapshot.state == NetplayState::Idle ||
            m_snapshot.state == NetplayState::Disconnected ||
            m_snapshot.state == NetplayState::Running)
            return;

        roomId = m_snapshot.currentRoom.roomId;
        localPlayerId = m_snapshot.localPlayerId;
        peerHost = m_peerHost;
        peerPort = m_peerPort;

        const bool wasReady = m_snapshot.players[localPlayerId].ready;
        m_snapshot.players[localPlayerId].ready = true;
        m_snapshot.state = NetplayState::WaitingReady;
        m_incomingLinkData.clear();

        if (!wasReady)
        {
            setStatusLocked(m_snapshot.hosting
                ? "房主核心与 Link Adapter 已就绪，等待客户端 CoreReady"
                : "客户端核心与 Link Adapter 已就绪，1秒后发送 CoreReady");
            brls::Logger::info("[Netplay][Manager] CoreReady local playerId={} hosting={} room={} peer={}:{}",
                               static_cast<int>(localPlayerId), m_snapshot.hosting ? 1 : 0,
                               roomId, peerHost, peerPort);
        }

        if (m_snapshot.hosting)
        {
            queueRunGo = m_snapshot.players[0].ready &&
                         m_snapshot.players[1].ready &&
                         !m_runGoQueued &&
                         !peerHost.empty() &&
                         peerPort != 0;
            if (queueRunGo)
            {
                m_runGoQueued = true;
                setStatusLocked("双方核心已就绪，1秒后发送 RunGo");
                brls::Logger::info("[Netplay][Manager] queue RunGo after local CoreReady room={} peer={}:{}",
                                   roomId, peerHost, peerPort);
            }
        }
        else
        {
            sendCoreReady = !peerHost.empty() && peerPort != 0;
        }
    }

    if (sendCoreReady)
    {
        queuePacket(peerHost, peerPort, PacketType::CoreReady,
                    toPayload(std::to_string(roomId) + "|" + std::to_string(localPlayerId)),
                    HANDSHAKE_DELAY, "已发送 CoreReady，等待房主 RunGo");
    }

    if (queueRunGo)
    {
        queuePacket(peerHost, peerPort, PacketType::RunGo, toPayload(std::to_string(roomId)),
                    HANDSHAKE_DELAY, "RunGo 已发送，等待本机同步放行");
    }
}

void NetplayManager::startGame()
{
    std::string peerHost;
    uint16_t peerPort = 0;
    std::vector<uint8_t> payload;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_snapshot.hosting || m_snapshot.currentRoom.roomId == 0)
            return;
        if (m_snapshot.state == NetplayState::LoadingGame ||
            m_snapshot.state == NetplayState::WaitingReady ||
            m_snapshot.state == NetplayState::Running)
        {
            brls::Logger::info("[Netplay][Manager] startGame ignored state={} room={}",
                               toString(m_snapshot.state), m_snapshot.currentRoom.roomId);
            return;
        }

        if (m_snapshot.currentRoom.players < 2 || m_peerHost.empty() || m_peerPort == 0)
        {
            setStatusLocked("等待玩家加入后才能开始");
            brls::Logger::warning("[Netplay][Manager] startGame blocked players={} peer={}:{} room={}",
                                  static_cast<int>(m_snapshot.currentRoom.players),
                                  m_peerHost, m_peerPort, m_snapshot.currentRoom.roomId);
            return;
        }

        m_snapshot.state = NetplayState::LoadingGame;
        m_snapshot.players[0].ready = false;
        m_snapshot.players[1].ready = false;
        m_incomingLinkData.clear();
        m_runGoQueued = false;
        m_runReleasePending = false;
        m_runReleaseRoomId = 0;
        m_runReleaseDue = {};
        peerHost = m_peerHost;
        peerPort = m_peerPort;

        const uint32_t seed = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
        const uint64_t rtc = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
        std::ostringstream out;
        out << m_snapshot.currentRoom.roomId << '|'
            << m_snapshot.currentRoom.crc32 << '|'
            << seed << '|'
            << rtc << '|'
            << sanitizeField(m_snapshot.currentRoom.title);
        payload = toPayload(out.str());
        setStatusLocked("开始游戏信号已发出，等待客户端准备");
        brls::Logger::info("[Netplay][Manager] startGame room={} crc={:#x} seed={} rtc={} peer={}:{} title={}",
                           m_snapshot.currentRoom.roomId, m_snapshot.currentRoom.crc32,
                           seed, rtc, peerHost, peerPort, m_snapshot.currentRoom.title);
    }

    if (!m_transport.sendPacket(peerHost, peerPort, PacketType::StartGame, payload))
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_snapshot.state = NetplayState::Connected;
        setStatusLocked("开始游戏失败：发送信号失败");
        brls::Logger::error("[Netplay][Manager] StartGame send failed peer={}:{}", peerHost, peerPort);
    }
}

void NetplayManager::poll()
{
    std::vector<DelayedPacket> duePackets;
    std::vector<DelayedPacket> retryPackets;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto now = std::chrono::steady_clock::now();

        if (m_runReleasePending && now >= m_runReleaseDue)
        {
            if (m_snapshot.currentRoom.roomId == m_runReleaseRoomId &&
                m_snapshot.state != NetplayState::Idle &&
                m_snapshot.state != NetplayState::Disconnected)
            {
                m_snapshot.state = NetplayState::Running;
                m_runReleasePending = false;
                m_runReleaseRoomId = 0;
                m_runReleaseDue = {};
                setStatusLocked("RunGo 生效，开始同步运行");
                brls::Logger::info("[Netplay][Manager] local RunGo release room={} entering Running",
                                   m_snapshot.currentRoom.roomId);
            }
            else
            {
                brls::Logger::warning("[Netplay][Manager] local RunGo release ignored currentRoom={} expected={} state={}",
                                      m_snapshot.currentRoom.roomId, m_runReleaseRoomId,
                                      toString(m_snapshot.state));
                m_runReleasePending = false;
                m_runReleaseRoomId = 0;
                m_runReleaseDue = {};
            }
        }

        auto it = m_delayedPackets.begin();
        while (it != m_delayedPackets.end())
        {
            if (it->due <= now)
            {
                brls::Logger::info("[Netplay][Manager] delayed packet due type={} target={}:{} payload={}",
                                   packetTypeName(it->type), it->host, it->port, it->payload.size());
                duePackets.push_back(std::move(*it));
                it = m_delayedPackets.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (m_clientJoinPhase == ClientJoinPhase::RequestingRoomInfo &&
            !m_clientJoinHost.empty() && m_clientJoinPort != 0 &&
            !m_clientDiscoverPayload.empty() && m_nextClientRetry <= now)
        {
            retryPackets.push_back({now, m_clientJoinHost, m_clientJoinPort,
                                    PacketType::Discover, m_clientDiscoverPayload,
                                    "已重发房间信息请求，等待房主回复房间信息"});
            brls::Logger::info("[Netplay][Manager] retry Discover target={}:{}", m_clientJoinHost, m_clientJoinPort);
            m_nextClientRetry = now + HANDSHAKE_RETRY;
        }
        else if (m_clientJoinPhase == ClientJoinPhase::WaitingApproval &&
                 !m_clientJoinHost.empty() && m_clientJoinPort != 0 &&
                 !m_clientJoinPayload.empty() && m_nextClientRetry <= now)
        {
            retryPackets.push_back({now, m_clientJoinHost, m_clientJoinPort,
                                    PacketType::JoinRequest, m_clientJoinPayload,
                                    "已重发正式加入请求，等待房主同意"});
            brls::Logger::info("[Netplay][Manager] retry JoinRequest target={}:{}", m_clientJoinHost, m_clientJoinPort);
            m_nextClientRetry = now + HANDSHAKE_RETRY;
        }
    }

    duePackets.insert(duePackets.end(),
                      std::make_move_iterator(retryPackets.begin()),
                      std::make_move_iterator(retryPackets.end()));

    for (const auto& packet : duePackets)
    {
        const bool sent = m_transport.sendPacket(packet.host, packet.port, packet.type, packet.payload);
        if (!packet.sentStatus.empty() || !sent)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            setStatusLocked(sent ? packet.sentStatus : "连接握手发送失败，等待重试");
            brls::Logger::info("[Netplay][Manager] delayed packet sent type={} target={}:{} ok={} status={}",
                               packetTypeName(packet.type), packet.host, packet.port, sent ? 1 : 0,
                               sent ? packet.sentStatus : "send failed");
        }

        if (sent && packet.type == PacketType::RunGo)
        {
            const uint64_t roomId = parseU64(fromPayload(packet.payload));
            std::lock_guard<std::mutex> lock(m_mutex);
            if (roomId != 0 && m_snapshot.currentRoom.roomId == roomId)
            {
                m_runReleasePending = true;
                m_runReleaseRoomId = roomId;
                m_runReleaseDue = std::chrono::steady_clock::now() + RUN_RELEASE_DELAY;
                setStatusLocked("RunGo 已发出，等待本机同步放行");
                brls::Logger::info("[Netplay][Manager] scheduled local RunGo release room={} delayMs={}",
                                   roomId, static_cast<long long>(RUN_RELEASE_DELAY.count()));
            }
        }
    }
}

void NetplayManager::pushIncomingLinkData(const LinkDataPacket& packet)
{
    bool pushed = m_incomingLinkData.push(packet);
    if (!pushed)
    {
        LinkDataPacket dropped;
        auto droppedValue = m_incomingLinkData.pop();
        if (droppedValue)
            dropped = *droppedValue;
        pushed = m_incomingLinkData.push(packet);
        if (droppedValue)
        {
            uint32_t dropCount = 0;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                dropCount = ++m_droppedIncomingLinkData;
            }
            if (shouldLogLinkCounter(dropCount))
            {
                brls::Logger::warning("[Netplay][LinkData] incoming queue full drops={}, dropped oldest player={} mode={} cycle={} data={:#x}; keep latest player={} mode={} cycle={} data={:#x}",
                                      dropCount, static_cast<int>(dropped.playerId), linkModeName(dropped.mode),
                                      dropped.cycle, dropped.data, static_cast<int>(packet.playerId),
                                      linkModeName(packet.mode), packet.cycle, packet.data);
            }
        }
    }

    if (pushed)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_snapshot.rxPackets;
        if (shouldLogLinkCounter(m_snapshot.rxPackets))
        {
            brls::Logger::info("[Netplay][LinkData] queued rx={} player={} mode={} cycle={} data={:#x} siocnt={:#x} rcnt={:#x} flags={:#x}",
                               m_snapshot.rxPackets, static_cast<int>(packet.playerId), linkModeName(packet.mode),
                               packet.cycle, packet.data, packet.siocnt, packet.rcnt, static_cast<int>(packet.flags));
        }
    }
    else
    {
        brls::Logger::warning("[Netplay][LinkData] incoming queue push failed even after dropping oldest player={} mode={} cycle={} data={:#x}",
                              static_cast<int>(packet.playerId), linkModeName(packet.mode), packet.cycle, packet.data);
    }
}

bool NetplayManager::popIncomingLinkData(LinkDataPacket& packet)
{
    auto value = m_incomingLinkData.pop();
    if (!value)
        return false;
    packet = *value;
    uint32_t popCount = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        popCount = ++m_poppedIncomingLinkData;
    }
    if (shouldLogLinkCounter(popCount))
    {
        brls::Logger::info("[Netplay][LinkData] pop #{} player={} mode={} cycle={} data={:#x} flags={:#x}",
                           popCount, static_cast<int>(packet.playerId), linkModeName(packet.mode),
                           packet.cycle, packet.data, static_cast<int>(packet.flags));
    }
    return true;
}

void NetplayManager::sendLinkData(const LinkDataPacket& packet)
{
    std::string peerHost;
    uint16_t peerPort = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_snapshot.state != NetplayState::Running || m_peerHost.empty() || m_peerPort == 0)
        {
            brls::Logger::warning("[Netplay][LinkData] send skipped state={} peer={}:{} player={} mode={} data={:#x}",
                                  toString(m_snapshot.state), m_peerHost, m_peerPort,
                                  static_cast<int>(packet.playerId), linkModeName(packet.mode), packet.data);
            return;
        }

        peerHost = m_peerHost;
        peerPort = m_peerPort;
    }

    if (m_transport.sendPacket(peerHost, peerPort, PacketType::LinkData, encodeLinkData(packet)))
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_snapshot.txPackets;
        if (shouldLogLinkCounter(m_snapshot.txPackets))
        {
            brls::Logger::info("[Netplay][LinkData] sent tx={} peer={}:{} player={} mode={} cycle={} data={:#x} siocnt={:#x} rcnt={:#x} flags={:#x}",
                               m_snapshot.txPackets, peerHost, peerPort, static_cast<int>(packet.playerId),
                               linkModeName(packet.mode), packet.cycle, packet.data, packet.siocnt, packet.rcnt,
                               static_cast<int>(packet.flags));
        }
    }
    else
    {
        brls::Logger::error("[Netplay][LinkData] send failed peer={}:{} player={} mode={} cycle={} data={:#x}",
                            peerHost, peerPort, static_cast<int>(packet.playerId),
                            linkModeName(packet.mode), packet.cycle, packet.data);
    }
}

void NetplayManager::handleControlPacket(PacketType type, std::vector<uint8_t> payload, std::string host, uint16_t port)
{
    if (type == PacketType::LinkData)
    {
        LinkDataPacket packet;
        if (!decodeLinkData(payload, packet))
        {
            brls::Logger::warning("[Netplay][Manager] malformed LinkData from {}:{} payload={}",
                                  host, port, payload.size());
            return;
        }

        bool accept = false;
        std::string peerHost;
        uint16_t peerPort = 0;
        NetplayState state = NetplayState::Idle;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            state = m_snapshot.state;
            peerHost = m_peerHost;
            peerPort = m_peerPort;
            accept = m_snapshot.state == NetplayState::Running &&
                     (m_peerHost.empty() || m_peerHost == host);
            if (accept && m_peerPort == 0)
                m_peerPort = port;
        }

        if (accept)
            pushIncomingLinkData(packet);
        else
            brls::Logger::warning("[Netplay][Manager] drop LinkData from {}:{} state={} peer={}:{} packetPlayer={} mode={} data={:#x}",
                                  host, port, toString(state), peerHost, peerPort,
                                  static_cast<int>(packet.playerId), linkModeName(packet.mode), packet.data);
        return;
    }

    const auto text = fromPayload(payload);
    const auto parts = split(text, '|');
    brls::Logger::info("[Netplay][Manager] handle packet type={} from {}:{} payload={} text={}",
                       packetTypeName(type), host, port, payload.size(), text.substr(0, 160));

    if (type == PacketType::Discover)
    {
        std::vector<uint8_t> replyPayload;
        PacketType replyType = PacketType::JoinReject;
        std::string sentStatus;
        const std::string requesterName = parts.empty() || parts[0].empty() ? "玩家" : parts[0].substr(0, 31);
        const auto requesterAvatar = static_cast<uint8_t>(parts.size() >= 2 ? std::clamp(parseInt(parts[1]), 0, 255) : 0);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_snapshot.hosting || m_snapshot.currentRoom.roomId == 0)
            {
                replyPayload = toPayload("0|房间不可用");
                sentStatus = "已回复房间不可用";
                brls::Logger::warning("[Netplay][Manager] Discover ignored from {}:{} hosting={} room={}",
                                      host, port, m_snapshot.hosting ? 1 : 0, m_snapshot.currentRoom.roomId);
            }
            else
            {
                std::ostringstream out;
                out << m_snapshot.currentRoom.roomId << '|'
                    << sanitizeField(m_snapshot.currentRoom.hostName) << '|'
                    << static_cast<int>(m_snapshot.currentRoom.avatar) << '|'
                    << sanitizeField(m_snapshot.currentRoom.title) << '|'
                    << m_snapshot.currentRoom.crc32 << '|'
                    << static_cast<int>(m_snapshot.currentRoom.players) << '|'
                    << static_cast<int>(m_snapshot.currentRoom.maxPlayers);
                replyType = PacketType::RoomInfo;
                replyPayload = toPayload(out.str());
                m_snapshot.pendingJoin.active = true;
                m_snapshot.pendingJoin.approvalReady = false;
                m_snapshot.pendingJoin.roomId = m_snapshot.currentRoom.roomId;
                m_snapshot.pendingJoin.name = requesterName;
                m_snapshot.pendingJoin.avatar = requesterAvatar;
                m_snapshot.pendingJoin.crc32 = 0;
                m_snapshot.pendingJoin.title = "等待正式加入请求";
                m_snapshot.pendingJoin.endpoint = host + ":" + std::to_string(port);
                setStatusLocked("收到 " + requesterName + " 的房间信息请求，1秒后回复房间信息");
                brls::Logger::info("[Netplay][Manager] Discover accepted requester={} avatar={} endpoint={} room={} crc={:#x}",
                                   requesterName, static_cast<int>(requesterAvatar),
                                   m_snapshot.pendingJoin.endpoint, m_snapshot.currentRoom.roomId,
                                   m_snapshot.currentRoom.crc32);
                sentStatus = "已回复房间信息，等待客户端发送正式加入请求";
            }
        }

        queuePacket(host, port, replyType, replyPayload, HANDSHAKE_DELAY, sentStatus);
        return;
    }

    if (type == PacketType::RoomInfo)
    {
        if (parts.size() < 7)
        {
            brls::Logger::warning("[Netplay][Manager] RoomInfo malformed parts={} from {}:{}", parts.size(), host, port);
            return;
        }

        RoomInfo room;
        room.roomId = parseU64(parts[0]);
        room.hostName = parts[1].empty() ? "Host" : parts[1].substr(0, 31);
        room.avatar = static_cast<uint8_t>(std::clamp(parseInt(parts[2]), 0, 255));
        room.title = parts[3];
        room.crc32 = parseU32(parts[4]);
        room.players = static_cast<uint8_t>(std::clamp(parseInt(parts[5]), 0, 255));
        room.maxPlayers = static_cast<uint8_t>(std::clamp(parseInt(parts[6]), 0, 255));
        room.endpoint = host + ":" + std::to_string(port);
        if (room.roomId == 0 || room.crc32 == 0)
        {
            brls::Logger::warning("[Netplay][Manager] RoomInfo invalid room={} crc={:#x} from {}:{}",
                                  room.roomId, room.crc32, host, port);
            return;
        }

        std::string nickname;
        uint8_t avatar = 0;
        std::vector<uint8_t> joinPayload;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_snapshot.hosting)
            {
                brls::Logger::warning("[Netplay][Manager] RoomInfo ignored while hosting from {}:{}", host, port);
                return;
            }
            if (m_snapshot.currentRoom.roomId == room.roomId &&
                (m_clientJoinPhase == ClientJoinPhase::WaitingApproval ||
                 m_clientJoinPhase == ClientJoinPhase::Connected))
            {
                brls::Logger::info("[Netplay][Manager] duplicate RoomInfo ignored room={} from {}:{}",
                                   room.roomId, host, port);
                return;
            }

            nickname = m_snapshot.nickname.empty() ? "Player" : m_snapshot.nickname;
            avatar = m_snapshot.players[0].avatar;
            m_snapshot.currentRoom = room;
            m_snapshot.localPlayerId = 1;
            m_peerHost = host;
            m_peerPort = port;
            setStatusLocked("已收到房间信息，1秒后发送正式加入请求");
            brls::Logger::info("[Netplay][Manager] RoomInfo accepted room={} hostName={} endpoint={} crc={:#x} players={}/{}",
                               room.roomId, room.hostName, room.endpoint, room.crc32,
                               static_cast<int>(room.players), static_cast<int>(room.maxPlayers));
        }

        std::ostringstream out;
        out << room.roomId << '|'
            << sanitizeField(nickname) << '|'
            << static_cast<int>(avatar) << '|'
            << room.crc32 << '|'
            << sanitizeField(room.title);
        joinPayload = toPayload(out.str());
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_clientJoinPhase = ClientJoinPhase::WaitingApproval;
            m_clientJoinHost = host;
            m_clientJoinPort = port;
            m_clientJoinPayload = joinPayload;
            m_nextClientRetry = std::chrono::steady_clock::now() + HANDSHAKE_DELAY + HANDSHAKE_RETRY;
        }
        queuePacket(host, port, PacketType::JoinRequest, joinPayload, HANDSHAKE_DELAY,
                    "已发送正式加入请求，等待房主同意");
        return;
    }

    if (type == PacketType::JoinRequest)
    {
        if (parts.size() < 5)
        {
            brls::Logger::warning("[Netplay][Manager] JoinRequest malformed parts={} from {}:{}", parts.size(), host, port);
            return;
        }

        const uint64_t roomId = parseU64(parts[0]);
        const std::string guestName = parts[1].empty() ? "Guest" : parts[1].substr(0, 31);
        const auto guestAvatar = static_cast<uint8_t>(std::clamp(parseInt(parts[2]), 0, 255));
        const uint32_t guestCrc = parseU32(parts[3]);

        std::vector<uint8_t> replyPayload;
        PacketType replyType = PacketType::JoinReject;
        std::string replyStatus;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_snapshot.hosting || m_snapshot.currentRoom.roomId != roomId)
            {
                replyPayload = toPayload(std::to_string(roomId) + "|房间不可用");
                replyStatus = "已回复房间不可用";
                brls::Logger::warning("[Netplay][Manager] JoinRequest room unavailable requested={} current={} hosting={} from {}:{}",
                                      roomId, m_snapshot.currentRoom.roomId, m_snapshot.hosting ? 1 : 0, host, port);
            }
            else if (m_snapshot.currentRoom.crc32 != guestCrc)
            {
                replyPayload = toPayload(std::to_string(roomId) + "|ROM CRC32 不一致");
                replyStatus = "已回复 CRC32 不一致";
                brls::Logger::warning("[Netplay][Manager] JoinRequest crc mismatch guest={:#x} host={:#x} name={} from {}:{}",
                                      guestCrc, m_snapshot.currentRoom.crc32, guestName, host, port);
            }
            else if (m_snapshot.currentRoom.players >= m_snapshot.currentRoom.maxPlayers)
            {
                if (m_snapshot.state == NetplayState::Connected && m_peerHost == host && m_peerPort == port)
                {
                    replyType = PacketType::JoinAccept;
                    replyPayload = makeJoinAcceptPayload(m_snapshot.currentRoom);
                    setStatusLocked("收到已连接客户端的重复正式加入请求，1秒后重发同意信息");
                    replyStatus = "已重发同意信息，等待客户端确认";
                    brls::Logger::info("[Netplay][Manager] duplicate JoinRequest from connected peer {}:{} room={}",
                                       host, port, roomId);
                }
                else
                {
                    replyPayload = toPayload(std::to_string(roomId) + "|房间已满");
                    replyStatus = "已回复房间已满";
                    brls::Logger::warning("[Netplay][Manager] JoinRequest room full from {}:{} room={}",
                                          host, port, roomId);
                }
            }
            else
            {
                m_snapshot.pendingJoin.active = true;
                m_snapshot.pendingJoin.approvalReady = true;
                m_snapshot.pendingJoin.roomId = roomId;
                m_snapshot.pendingJoin.name = guestName;
                m_snapshot.pendingJoin.avatar = guestAvatar;
                m_snapshot.pendingJoin.crc32 = guestCrc;
                m_snapshot.pendingJoin.title = parts[4];
                m_snapshot.pendingJoin.endpoint = host + ":" + std::to_string(port);
                setStatusLocked("收到 " + guestName + " 的正式加入请求，请选择同意或拒绝");
                brls::Logger::info("[Netplay][Manager] JoinRequest pending name={} avatar={} crc={:#x} title={} endpoint={}",
                                   guestName, static_cast<int>(guestAvatar), guestCrc,
                                   m_snapshot.pendingJoin.title, m_snapshot.pendingJoin.endpoint);
            }
        }

        if (!replyPayload.empty())
            queuePacket(host, port, replyType, replyPayload, HANDSHAKE_DELAY, replyStatus);
        return;
    }

    if (type == PacketType::JoinAccept)
    {
        if (parts.size() < 6)
        {
            brls::Logger::warning("[Netplay][Manager] JoinAccept malformed parts={} from {}:{}", parts.size(), host, port);
            return;
        }

        const uint64_t roomId = parseU64(parts[0]);
        const std::string hostName = parts[1].empty() ? "Host" : parts[1].substr(0, 31);
        const auto hostAvatar = static_cast<uint8_t>(std::clamp(parseInt(parts[2]), 0, 255));
        const uint32_t crc32 = parseU32(parts[4]);
        const auto playerId = static_cast<uint8_t>(std::clamp(parseInt(parts[5]), 0, static_cast<int>(MAX_LINK_PLAYERS - 1)));

        std::vector<uint8_t> ackPayload;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_snapshot.hosting || m_snapshot.currentRoom.roomId != roomId || m_snapshot.currentRoom.crc32 != crc32)
            {
                brls::Logger::warning("[Netplay][Manager] JoinAccept ignored hosting={} room={} gotRoom={} crc={:#x} gotCrc={:#x}",
                                      m_snapshot.hosting ? 1 : 0, m_snapshot.currentRoom.roomId,
                                      roomId, m_snapshot.currentRoom.crc32, crc32);
                return;
            }

            const uint8_t localAvatar = m_snapshot.players[0].avatar;
            const std::string localName = m_snapshot.nickname.empty() ? "Player" : m_snapshot.nickname;
            m_peerHost = host;
            m_peerPort = port;
            m_snapshot.currentRoom.hostName = hostName;
            m_snapshot.currentRoom.avatar = hostAvatar;
            m_snapshot.currentRoom.players = 2;
            m_snapshot.state = NetplayState::Connected;
            m_snapshot.localPlayerId = playerId;
            m_snapshot.players[0] = {0, hostAvatar, true, hostName};
            m_snapshot.players[playerId] = {playerId, localAvatar, false, localName};
            m_clientJoinPhase = ClientJoinPhase::Connected;
            m_clientJoinHost.clear();
            m_clientJoinPort = 0;
            m_clientDiscoverPayload.clear();
            m_clientJoinPayload.clear();
            setStatusLocked("已收到房主确认，1秒后发送连接确认");
            ackPayload = toPayload(std::to_string(roomId) + "|" + std::to_string(playerId));
            brls::Logger::info("[Netplay][Manager] JoinAccept accepted room={} host={} playerId={} peer={}:{}",
                               roomId, hostName, static_cast<int>(playerId), host, port);
        }

        queuePacket(host, port, PacketType::Heartbeat, ackPayload, HANDSHAKE_DELAY,
                    "已确认连接，等待房主开始");
        return;
    }

    if (type == PacketType::JoinReject)
    {
        const std::string reason = parts.size() >= 2 ? parts[1] : "房主拒绝加入";
        std::lock_guard<std::mutex> lock(m_mutex);
        m_snapshot.state = NetplayState::Idle;
        m_snapshot.currentRoom = {};
        m_clientJoinPhase = ClientJoinPhase::None;
        m_clientJoinHost.clear();
        m_clientJoinPort = 0;
        m_clientDiscoverPayload.clear();
        m_clientJoinPayload.clear();
        m_delayedPackets.clear();
        setStatusLocked("加入失败：" + reason);
        brls::Logger::warning("[Netplay][Manager] JoinReject reason={} from {}:{}", reason, host, port);
        return;
    }

    if (type == PacketType::Heartbeat)
    {
        if (parts.size() < 2)
        {
            brls::Logger::warning("[Netplay][Manager] Heartbeat malformed parts={} from {}:{}", parts.size(), host, port);
            return;
        }

        const uint64_t roomId = parseU64(parts[0]);
        const auto playerId = static_cast<uint8_t>(std::clamp(parseInt(parts[1]), 0, static_cast<int>(MAX_LINK_PLAYERS - 1)));
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_snapshot.hosting || m_snapshot.currentRoom.roomId != roomId)
        {
            brls::Logger::warning("[Netplay][Manager] Heartbeat ignored hosting={} currentRoom={} gotRoom={} from {}:{}",
                                  m_snapshot.hosting ? 1 : 0, m_snapshot.currentRoom.roomId, roomId, host, port);
            return;
        }

        m_peerHost = host;
        m_peerPort = port;
        m_snapshot.state = NetplayState::Connected;
        if (playerId < m_snapshot.players.size() && !m_snapshot.players[playerId].name.empty())
            setStatusLocked("连接已建立，玩家 " + m_snapshot.players[playerId].name + " 已确认，按 A 开始游戏");
        else
            setStatusLocked("连接已建立，按 A 开始游戏");
        brls::Logger::info("[Netplay][Manager] Heartbeat accepted playerId={} peer={}:{} room={}",
                           static_cast<int>(playerId), host, port, roomId);
        return;
    }

    if (type == PacketType::StartGame)
    {
        if (parts.size() < 5)
        {
            brls::Logger::warning("[Netplay][Manager] StartGame malformed parts={} from {}:{}", parts.size(), host, port);
            return;
        }

        const uint64_t roomId = parseU64(parts[0]);
        const uint32_t crc32 = parseU32(parts[1]);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_snapshot.hosting || m_snapshot.currentRoom.roomId != roomId || m_snapshot.currentRoom.crc32 != crc32)
            {
                brls::Logger::warning("[Netplay][Manager] StartGame ignored hosting={} currentRoom={} gotRoom={} crc={:#x} gotCrc={:#x}",
                                      m_snapshot.hosting ? 1 : 0, m_snapshot.currentRoom.roomId,
                                      roomId, m_snapshot.currentRoom.crc32, crc32);
                return;
            }
            if (m_snapshot.state == NetplayState::LoadingGame ||
                m_snapshot.state == NetplayState::WaitingReady ||
                m_snapshot.state == NetplayState::Running)
            {
                m_peerHost = host;
                m_peerPort = port;
                brls::Logger::info("[Netplay][Manager] duplicate StartGame ignored state={} room={} crc={:#x}",
                                   toString(m_snapshot.state), roomId, crc32);
                return;
            }

            m_peerHost = host;
            m_peerPort = port;
            m_snapshot.state = NetplayState::LoadingGame;
            m_snapshot.players[0].ready = false;
            m_snapshot.players[m_snapshot.localPlayerId].ready = false;
            m_incomingLinkData.clear();
            m_runGoQueued = false;
            m_runReleasePending = false;
            m_runReleaseRoomId = 0;
            m_runReleaseDue = {};
            setStatusLocked("收到开始游戏信号，正在创建核心");
            brls::Logger::info("[Netplay][Manager] StartGame accepted room={} crc={:#x} localPlayerId={} peer={}:{}",
                               roomId, crc32, static_cast<int>(m_snapshot.localPlayerId), host, port);
        }
        return;
    }

    if (type == PacketType::Ready)
    {
        if (parts.size() < 2)
        {
            brls::Logger::warning("[Netplay][Manager] Ready malformed parts={} from {}:{}", parts.size(), host, port);
            return;
        }

        const uint64_t roomId = parseU64(parts[0]);
        const auto playerId = static_cast<uint8_t>(std::clamp(parseInt(parts[1]), 0, static_cast<int>(MAX_LINK_PLAYERS - 1)));
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_snapshot.hosting || m_snapshot.currentRoom.roomId != roomId)
            {
                brls::Logger::warning("[Netplay][Manager] Ready ignored hosting={} currentRoom={} gotRoom={} from {}:{}",
                                  m_snapshot.hosting ? 1 : 0, m_snapshot.currentRoom.roomId, roomId, host, port);
                return;
            }

            setStatusLocked("收到旧版 Ready，继续等待 CoreReady");
            brls::Logger::info("[Netplay][Manager] legacy Ready accepted playerId={} room={} waiting CoreReady",
                               static_cast<int>(playerId), roomId);
        }
        return;
    }

    if (type == PacketType::CoreReady)
    {
        if (parts.size() < 2)
        {
            brls::Logger::warning("[Netplay][Manager] CoreReady malformed parts={} from {}:{}", parts.size(), host, port);
            return;
        }

        const uint64_t roomId = parseU64(parts[0]);
        const auto playerId = static_cast<uint8_t>(std::clamp(parseInt(parts[1]), 0, static_cast<int>(MAX_LINK_PLAYERS - 1)));
        bool queueRunGo = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_snapshot.hosting || m_snapshot.currentRoom.roomId != roomId)
            {
                brls::Logger::warning("[Netplay][Manager] CoreReady ignored hosting={} currentRoom={} gotRoom={} from {}:{}",
                                      m_snapshot.hosting ? 1 : 0, m_snapshot.currentRoom.roomId, roomId, host, port);
                return;
            }

            m_peerHost = host;
            m_peerPort = port;
            m_snapshot.players[playerId].ready = true;
            setStatusLocked("收到客户端 CoreReady，等待本机核心就绪");
            brls::Logger::info("[Netplay][Manager] CoreReady accepted playerId={} room={} peer={}:{} localReady={} peerReady={}",
                               static_cast<int>(playerId), roomId, host, port,
                               m_snapshot.players[0].ready ? 1 : 0,
                               m_snapshot.players[1].ready ? 1 : 0);

            queueRunGo = m_snapshot.players[0].ready &&
                         m_snapshot.players[1].ready &&
                         !m_runGoQueued &&
                         !m_peerHost.empty() &&
                         m_peerPort != 0;
            if (queueRunGo)
            {
                m_runGoQueued = true;
                setStatusLocked("双方核心已就绪，1秒后发送 RunGo");
                brls::Logger::info("[Netplay][Manager] queue RunGo after peer CoreReady room={} peer={}:{}",
                                   roomId, m_peerHost, m_peerPort);
            }
        }

        if (queueRunGo)
        {
            queuePacket(host, port, PacketType::RunGo, toPayload(std::to_string(roomId)),
                        HANDSHAKE_DELAY, "RunGo 已发送，等待本机同步放行");
        }
        return;
    }

    if (type == PacketType::RunGo)
    {
        if (parts.empty())
        {
            brls::Logger::warning("[Netplay][Manager] RunGo malformed from {}:{}", host, port);
            return;
        }

        const uint64_t roomId = parseU64(parts[0]);
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_snapshot.currentRoom.roomId != roomId)
        {
            brls::Logger::warning("[Netplay][Manager] RunGo ignored currentRoom={} gotRoom={} from {}:{}",
                                  m_snapshot.currentRoom.roomId, roomId, host, port);
            return;
        }

        m_runReleasePending = true;
        m_runReleaseRoomId = roomId;
        m_runReleaseDue = std::chrono::steady_clock::now() + RUN_RELEASE_DELAY;
        setStatusLocked("收到 RunGo，等待同步放行");
        brls::Logger::info("[Netplay][Manager] RunGo accepted room={} peer={}:{} releaseDelayMs={}",
                           roomId, host, port, static_cast<long long>(RUN_RELEASE_DELAY.count()));
        return;
    }

    if (type == PacketType::Go)
    {
        if (parts.empty())
        {
            brls::Logger::warning("[Netplay][Manager] Go malformed from {}:{}", host, port);
            return;
        }

        const uint64_t roomId = parseU64(parts[0]);
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_snapshot.currentRoom.roomId != roomId)
        {
            brls::Logger::warning("[Netplay][Manager] Go ignored currentRoom={} gotRoom={} from {}:{}",
                                  m_snapshot.currentRoom.roomId, roomId, host, port);
            return;
        }

        m_runReleasePending = true;
        m_runReleaseRoomId = roomId;
        m_runReleaseDue = std::chrono::steady_clock::now() + RUN_RELEASE_DELAY;
        setStatusLocked("收到旧版 Go，等待同步放行");
        brls::Logger::info("[Netplay][Manager] legacy Go accepted room={} peer={}:{} releaseDelayMs={}",
                           roomId, host, port, static_cast<long long>(RUN_RELEASE_DELAY.count()));
        return;
    }
}

uint64_t NetplayManager::makeRoomId() const
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return static_cast<uint64_t>(now) ^ 0xB3114C00123ULL;
}

void NetplayManager::setStatusLocked(const std::string& text)
{
    m_snapshot.statusText = text;
    brls::Logger::info("[Netplay][Status] {}", text);
}

void NetplayManager::queuePacket(std::string host, uint16_t port, PacketType type, std::vector<uint8_t> payload,
                                 std::chrono::milliseconds delay, std::string sentStatus)
{
    if (host.empty() || port == 0)
    {
        brls::Logger::warning("[Netplay][Manager] queuePacket ignored type={} host={} port={}",
                              packetTypeName(type), host, port);
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    brls::Logger::info("[Netplay][Manager] queuePacket type={} target={}:{} delayMs={} payload={} status={}",
                       packetTypeName(type), host, port,
                       static_cast<long long>(delay.count()), payload.size(), sentStatus);
    m_delayedPackets.push_back({
        std::chrono::steady_clock::now() + delay,
        std::move(host),
        port,
        type,
        std::move(payload),
        std::move(sentStatus),
    });
}

} // namespace beiklive::netplay
