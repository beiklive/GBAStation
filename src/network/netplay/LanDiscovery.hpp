#pragma once

#include "network/NetworkManager.h"
#include "network/netplay/Packet.hpp"
#include "network/netplay/UdpSocket.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace beiklive::netplay
{

class LanDiscovery
{
public:
    using RoomsCallback = std::function<void(std::vector<RoomInfo>)>;

    LanDiscovery();
    ~LanDiscovery();

    bool startHosting(RoomInfo room);
    bool startScanning(RoomsCallback callback);
    void stop();
    bool isRunning() const { return m_running.load(); }

private:
    struct SeenRoom
    {
        RoomInfo room;
        std::chrono::steady_clock::time_point lastSeen;
    };

    void hostLoop();
    void scanLoop();
    void publishRooms();

    static std::string encodeRoom(const RoomInfo& room);
    static bool decodeRoom(const std::string& text, RoomInfo& room);

    beiklive::network::NetworkManager m_network;
    UdpSocket m_socket;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    bool m_networkReady = false;

    std::mutex m_mutex;
    RoomInfo m_hostRoom;
    std::unordered_map<uint64_t, SeenRoom> m_seenRooms;
    RoomsCallback m_callback;
};

} // namespace beiklive::netplay
