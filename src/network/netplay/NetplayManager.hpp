#pragma once

#include "core/common.h"
#include "network/netplay/LanDiscovery.hpp"
#include "network/netplay/LinkTransport.hpp"
#include "network/netplay/Packet.hpp"
#include "network/netplay/RingQueue.hpp"

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

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
    bool joinManual(const std::string& host, uint16_t port);
    void leaveRoom();
    void approvePendingJoin();
    void rejectPendingJoin();
    void markLocalReady();
    void markCoreReady();
    void startGame();

    void poll();
    void pushIncomingLinkData(const LinkDataPacket& packet);
    bool popIncomingLinkData(LinkDataPacket& packet);
    void sendLinkData(const LinkDataPacket& packet);

private:
    NetplayManager() = default;
    ~NetplayManager();

    void handleControlPacket(PacketType type, std::vector<uint8_t> payload, std::string host, uint16_t port);
    uint64_t makeRoomId() const;
    void setStatusLocked(const std::string& text);
    void queuePacket(std::string host, uint16_t port, PacketType type, std::vector<uint8_t> payload,
                     std::chrono::milliseconds delay, std::string sentStatus = {});

    enum class ClientJoinPhase
    {
        None,
        RequestingRoomInfo,
        WaitingApproval,
        Connected,
    };

    struct DelayedPacket
    {
        std::chrono::steady_clock::time_point due;
        std::string host;
        uint16_t port = 0;
        PacketType type = PacketType::Discover;
        std::vector<uint8_t> payload;
        std::string sentStatus;
    };

    mutable std::mutex m_mutex;
    NetplaySnapshot m_snapshot;
    LanDiscovery m_discovery;
    LinkTransport m_transport;
    std::string m_peerHost;
    uint16_t m_peerPort = 0;
    ClientJoinPhase m_clientJoinPhase = ClientJoinPhase::None;
    std::string m_clientJoinHost;
    uint16_t m_clientJoinPort = 0;
    std::vector<uint8_t> m_clientDiscoverPayload;
    std::vector<uint8_t> m_clientJoinPayload;
    std::chrono::steady_clock::time_point m_nextClientRetry{};
    std::vector<DelayedPacket> m_delayedPackets;
    RingQueue<LinkDataPacket, 256> m_incomingLinkData;
    uint32_t m_droppedIncomingLinkData = 0;
    uint32_t m_poppedIncomingLinkData = 0;
    bool m_runGoQueued = false;
    bool m_runReleasePending = false;
    uint64_t m_runReleaseRoomId = 0;
    std::chrono::steady_clock::time_point m_runReleaseDue{};
};

} // namespace beiklive::netplay
