#include "network/netplay/LanDiscovery.hpp"

#include <borealis.hpp>

#include <array>
#include <cstring>
#include <sstream>
#include <vector>

namespace beiklive::netplay
{
namespace
{
constexpr uint16_t DISCOVERY_PORT = 45871;
constexpr const char* BROADCAST_ADDR = "255.255.255.255";
constexpr const char* DISCOVER_PREFIX = "BKLN_DISCOVER_V1";
constexpr const char* ROOM_PREFIX = "BKLN_ROOM_V1";
constexpr auto ROOM_TTL = std::chrono::seconds(3);

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
} // namespace

LanDiscovery::LanDiscovery() = default;

LanDiscovery::~LanDiscovery()
{
    stop();
}

bool LanDiscovery::startHosting(RoomInfo room)
{
    stop();
    m_networkReady = m_network.Initialize();
    if (!m_networkReady)
    {
        brls::Logger::error("[Netplay][Discovery] host NetworkManager initialize failed");
        return false;
    }
    if (!m_socket.open(DISCOVERY_PORT, true))
    {
        brls::Logger::error("[Netplay][Discovery] host socket open failed port={} errno={}",
                            DISCOVERY_PORT, UdpSocket::lastSocketError());
        m_network.Shutdown();
        m_networkReady = false;
        return false;
    }

    brls::Logger::info("[Netplay][Discovery] host room prepared id={} title={} crc={:#x} endpoint={}",
                       room.roomId, room.title, room.crc32, room.endpoint);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_hostRoom = std::move(room);
    }

    m_running.store(true);
    m_thread = std::thread(&LanDiscovery::hostLoop, this);
    return true;
}

bool LanDiscovery::startScanning(RoomsCallback callback)
{
    stop();
    m_networkReady = m_network.Initialize();
    if (!m_networkReady)
    {
        brls::Logger::error("[Netplay][Discovery] scan NetworkManager initialize failed");
        return false;
    }
    if (!m_socket.open(DISCOVERY_PORT, true))
    {
        brls::Logger::error("[Netplay][Discovery] scan socket open failed port={} errno={}",
                            DISCOVERY_PORT, UdpSocket::lastSocketError());
        m_network.Shutdown();
        m_networkReady = false;
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_seenRooms.clear();
        m_callback = std::move(callback);
    }

    m_running.store(true);
    m_thread = std::thread(&LanDiscovery::scanLoop, this);
    return true;
}

void LanDiscovery::stop()
{
    const bool wasRunning = m_running.load() || m_thread.joinable() || m_networkReady;
    if (m_running.exchange(false) && m_thread.joinable())
        m_thread.join();
    else if (m_thread.joinable())
        m_thread.join();

    m_socket.close();
    if (m_networkReady)
    {
        m_network.Shutdown();
        m_networkReady = false;
    }
    if (wasRunning)
        brls::Logger::info("[Netplay][Discovery] stopped");
}

void LanDiscovery::hostLoop()
{
    brls::Logger::info("LAN discovery host started");
    std::array<char, 1024> buffer{};
    while (m_running.load())
    {
        std::string host;
        uint16_t port = 0;
        const int received = m_socket.receiveFrom(buffer.data(), buffer.size() - 1, host, port);
        if (received > 0)
        {
            buffer[static_cast<size_t>(received)] = '\0';
            const std::string request(buffer.data(), static_cast<size_t>(received));
            if (request == DISCOVER_PREFIX)
            {
                brls::Logger::info("[Netplay][Discovery] host received discover from {}:{}", host, port);
                RoomInfo room;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    room = m_hostRoom;
                }

                const std::string payload = encodeRoom(room);
                if (!m_socket.sendTo(host, port, payload.data(), payload.size()))
                {
                    brls::Logger::error("[Netplay][Discovery] room reply failed to {}:{} errno={}",
                                        host, port, UdpSocket::lastSocketError());
                }
                else
                {
                    brls::Logger::info("[Netplay][Discovery] room reply sent to {}:{} id={} title={} crc={:#x}",
                                       host, port, room.roomId, room.title, room.crc32);
                }
            }
            else
            {
                brls::Logger::warning("[Netplay][Discovery] host ignored unknown discovery payload from {}:{} size={}",
                                      host, port, received);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    brls::Logger::info("LAN discovery host stopped");
}

void LanDiscovery::scanLoop()
{
    brls::Logger::info("LAN discovery scan started");
    std::array<char, 1024> buffer{};
    const std::string directedBroadcast = UdpSocket::detectDirectedBroadcastAddress();
    std::vector<std::string> broadcastTargets = {directedBroadcast};
    if (directedBroadcast != BROADCAST_ADDR)
        broadcastTargets.push_back(BROADCAST_ADDR);
    brls::Logger::info("[Netplay][Discovery] scan broadcast targets directed={} fallback={}",
                       directedBroadcast, directedBroadcast != BROADCAST_ADDR ? BROADCAST_ADDR : "none");
    auto nextPublish = std::chrono::steady_clock::now();
    auto nextDiscover = std::chrono::steady_clock::now();
    while (m_running.load())
    {
        const auto now = std::chrono::steady_clock::now();
        if (now >= nextDiscover)
        {
            for (const auto& target : broadcastTargets)
            {
                if (!m_socket.sendTo(target, DISCOVERY_PORT, DISCOVER_PREFIX, std::strlen(DISCOVER_PREFIX)))
                {
                    brls::Logger::error("[Netplay][Discovery] request failed to {}:{} errno={}",
                                        target, DISCOVERY_PORT, UdpSocket::lastSocketError());
                }
                else
                {
                    brls::Logger::info("[Netplay][Discovery] discover request sent to {}:{}", target, DISCOVERY_PORT);
                }
            }
            nextDiscover = now + std::chrono::seconds(1);
        }

        std::string host;
        uint16_t port = 0;
        const int received = m_socket.receiveFrom(buffer.data(), buffer.size() - 1, host, port);
        if (received > 0)
        {
            buffer[static_cast<size_t>(received)] = '\0';
            RoomInfo room;
            if (decodeRoom(std::string(buffer.data(), static_cast<size_t>(received)), room))
            {
                room.endpoint = host + ":45872";
                brls::Logger::info("[Netplay][Discovery] room received from {}:{} id={} title={} crc={:#x} players={}/{}",
                                   host, port, room.roomId, room.title, room.crc32,
                                   static_cast<int>(room.players), static_cast<int>(room.maxPlayers));
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_seenRooms[room.roomId] = {room, std::chrono::steady_clock::now()};
                }
            }
            else
            {
                brls::Logger::warning("[Netplay][Discovery] scan ignored payload from {}:{} size={}",
                                      host, port, received);
            }
        }

        if (now >= nextPublish)
        {
            publishRooms();
            nextPublish = now + std::chrono::milliseconds(300);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    brls::Logger::info("LAN discovery scan stopped");
}

void LanDiscovery::publishRooms()
{
    RoomsCallback callback;
    std::vector<RoomInfo> rooms;
    const auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_seenRooms.begin(); it != m_seenRooms.end();)
        {
            if (now - it->second.lastSeen > ROOM_TTL)
            {
                brls::Logger::info("[Netplay][Discovery] room expired id={} endpoint={}",
                                   it->second.room.roomId, it->second.room.endpoint);
                it = m_seenRooms.erase(it);
            }
            else
            {
                rooms.push_back(it->second.room);
                ++it;
            }
        }
        callback = m_callback;
    }
    if (callback)
        callback(std::move(rooms));
}

std::string LanDiscovery::encodeRoom(const RoomInfo& room)
{
    std::ostringstream out;
    out << ROOM_PREFIX << '|'
        << room.roomId << '|'
        << sanitizeField(room.hostName) << '|'
        << static_cast<int>(room.avatar) << '|'
        << sanitizeField(room.title) << '|'
        << room.crc32 << '|'
        << static_cast<int>(room.players) << '|'
        << static_cast<int>(room.maxPlayers);
    return out.str();
}

bool LanDiscovery::decodeRoom(const std::string& text, RoomInfo& room)
{
    auto parts = split(text, '|');
    if (parts.size() < 8 || parts[0] != ROOM_PREFIX)
        return false;

    try
    {
        room.roomId = static_cast<uint64_t>(std::stoull(parts[1]));
        room.hostName = parts[2];
        room.avatar = static_cast<uint8_t>(std::stoi(parts[3]));
        room.title = parts[4];
        room.crc32 = static_cast<uint32_t>(std::stoul(parts[5]));
        room.players = static_cast<uint8_t>(std::stoi(parts[6]));
        room.maxPlayers = static_cast<uint8_t>(std::stoi(parts[7]));
    }
    catch (...)
    {
        return false;
    }
    return room.roomId != 0;
}

} // namespace beiklive::netplay
