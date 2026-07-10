#pragma once

#include "network/NetworkManager.h"
#include "network/netplay/Packet.hpp"
#include "network/netplay/UdpSocket.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace beiklive::netplay
{

class LinkTransport
{
public:
    using PacketCallback = std::function<void(PacketType, std::vector<uint8_t>, std::string, uint16_t)>;

    LinkTransport() = default;
    ~LinkTransport();

    LinkTransport(const LinkTransport&) = delete;
    LinkTransport& operator=(const LinkTransport&) = delete;

    bool start(uint16_t localPort, PacketCallback callback);
    void stop();
    bool isRunning() const { return m_running.load(); }

    bool sendPacket(const std::string& host, uint16_t port, PacketType type, const std::vector<uint8_t>& payload);

private:
    void receiveLoop();

    beiklive::network::NetworkManager m_network;
    UdpSocket m_socket;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<uint32_t> m_sequence{1};
    bool m_networkReady = false;
    PacketCallback m_callback;
};

} // namespace beiklive::netplay
