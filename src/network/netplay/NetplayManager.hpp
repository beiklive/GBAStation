#pragma once

#include "core/common.h"
#include "network/netplay/Packet.hpp"
#include "network/netplay/RingQueue.hpp"

#include <mutex>
#include <optional>
#include <string>

namespace beiklive::netplay
{

class NetplayManager
{
public:
    static NetplayManager& instance();

    NetplaySnapshot snapshot() const;

    void loadProfile();
    void saveProfile(const std::string& nickname, uint8_t avatar);

    bool startHosting(const beiklive::GameEntry& game);
    void startScanning();
    void stopScanning();
    bool joinRoom(uint64_t roomId);
    void leaveRoom();
    void markLocalReady();
    void startGame();

    void poll();
    void pushIncomingLinkData(const LinkDataPacket& packet);
    bool popIncomingLinkData(LinkDataPacket& packet);
    void sendLinkData(const LinkDataPacket& packet);

private:
    NetplayManager() = default;

    uint64_t makeRoomId() const;
    std::optional<beiklive::GameEntry> firstGbaGame() const;
    void rebuildDemoRoomsLocked();
    void setStatusLocked(const std::string& text);

    mutable std::mutex m_mutex;
    NetplaySnapshot m_snapshot;
    RingQueue<LinkDataPacket, 256> m_incomingLinkData;
};

} // namespace beiklive::netplay
