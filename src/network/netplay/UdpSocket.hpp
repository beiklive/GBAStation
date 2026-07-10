#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace beiklive::netplay
{

class UdpSocket
{
public:
    UdpSocket() = default;
    ~UdpSocket();

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    bool open(uint16_t port, bool broadcast);
    void close();
    bool isOpen() const { return m_handle >= 0; }

    bool sendTo(const std::string& host, uint16_t port, const void* data, size_t size);
    int receiveFrom(void* buffer, size_t capacity, std::string& fromHost, uint16_t& fromPort);

    static std::string detectLocalAddress();
    static std::string detectDirectedBroadcastAddress();
    static int lastSocketError();

private:
    intptr_t m_handle = -1;
};

} // namespace beiklive::netplay
