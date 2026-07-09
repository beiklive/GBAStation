#pragma once

#include "core/enums.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace beiklive::netplay
{

constexpr uint32_t PACKET_MAGIC = 0x424B4C4Eu; // "BKLN"
constexpr uint16_t PACKET_VERSION = 1;
constexpr uint8_t MAX_LINK_PLAYERS = 4;

enum class NetplayState
{
    Idle,
    Hosting,
    WaitingPlayer,
    Connected,
    LoadingGame,
    WaitingReady,
    Running,
    Disconnected,
};

enum class PacketType : uint16_t
{
    Discover = 1,
    RoomInfo,
    JoinRequest,
    JoinAccept,
    JoinReject,
    Leave,
    StartGame,
    Ready,
    Go,
    Heartbeat,
    Disconnect,
    LinkData,
};

struct PacketHeader
{
    uint32_t magic = PACKET_MAGIC;
    uint16_t version = PACKET_VERSION;
    PacketType type = PacketType::Discover;
    uint32_t sequence = 0;
    uint32_t payloadSize = 0;
};

struct PlayerInfo
{
    uint8_t playerId = 0;
    uint8_t avatar = 0;
    bool ready = false;
    std::string name;
};

struct RoomInfo
{
    uint64_t roomId = 0;
    std::string hostName;
    uint8_t avatar = 0;
    std::string title;
    uint32_t crc32 = 0;
    uint8_t players = 0;
    uint8_t maxPlayers = 2;
    uint32_t ping = 0;
    std::string endpoint;
};

struct LinkDataPacket
{
    uint64_t cycle = 0;
    uint16_t data = 0;
    uint8_t flags = 0;
    uint8_t playerId = 0;
};

struct StartGamePacket
{
    uint32_t crc32 = 0;
    uint32_t randomSeed = 0;
    uint64_t rtcUnixTime = 0;
    std::string title;
    std::array<PlayerInfo, MAX_LINK_PLAYERS> players{};
};

struct NetplaySnapshot
{
    NetplayState state = NetplayState::Idle;
    bool hosting = false;
    bool scanning = false;
    std::string nickname;
    std::string statusText;
    RoomInfo currentRoom;
    std::vector<RoomInfo> rooms;
    std::array<PlayerInfo, MAX_LINK_PLAYERS> players{};
    uint8_t localPlayerId = 0;
    uint32_t txPackets = 0;
    uint32_t rxPackets = 0;
};

const char* toString(NetplayState state);

} // namespace beiklive::netplay
