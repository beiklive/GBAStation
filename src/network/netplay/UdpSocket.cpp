#include "network/netplay/UdpSocket.hpp"

#include <borealis.hpp>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <cerrno>
#include <cstring>
#include <sstream>

namespace beiklive::netplay
{
namespace
{
#ifdef _WIN32
using NativeSocket = SOCKET;
using SocketLen = int;
constexpr intptr_t INVALID_SOCKET_VALUE = static_cast<intptr_t>(INVALID_SOCKET);

bool ensureWinsock()
{
    static bool initialized = false;
    if (initialized)
        return true;
    WSADATA data{};
    initialized = WSAStartup(MAKEWORD(2, 2), &data) == 0;
    return initialized;
}

void closeSocket(intptr_t handle)
{
    closesocket(static_cast<SOCKET>(handle));
}
#else
using NativeSocket = int;
using SocketLen = socklen_t;
constexpr intptr_t INVALID_SOCKET_VALUE = -1;

bool ensureWinsock()
{
    return true;
}

void closeSocket(intptr_t handle)
{
    ::close(static_cast<int>(handle));
}
#endif

int platformSocketError()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

bool isUsableLocalAddress(const char* address)
{
    if (!address)
        return false;
    return std::strcmp(address, "0.0.0.0") != 0 &&
           std::strcmp(address, "127.0.0.1") != 0;
}

std::string detectAddressFromHostname()
{
    char hostname[256]{};
    if (gethostname(hostname, sizeof(hostname)) != 0)
        return {};

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    addrinfo* result = nullptr;
    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0)
        return {};

    std::string address;
    for (addrinfo* it = result; it; it = it->ai_next)
    {
        auto* addr = reinterpret_cast<sockaddr_in*>(it->ai_addr);
        char ip[64]{};
        const char* text = inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
        if (isUsableLocalAddress(text))
        {
            address = text;
            break;
        }
    }
    freeaddrinfo(result);
    return address;
}
} // namespace

UdpSocket::~UdpSocket()
{
    close();
}

bool UdpSocket::open(uint16_t port, bool broadcast)
{
    close();
    if (!ensureWinsock())
    {
        brls::Logger::error("[Netplay][Socket] winsock/socket service init failed");
        return false;
    }

    m_handle = static_cast<intptr_t>(::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    if (m_handle == INVALID_SOCKET_VALUE)
    {
        brls::Logger::error("[Netplay][Socket] socket create failed errno={}", platformSocketError());
        m_handle = -1;
        return false;
    }

    int enabled = 1;
    setsockopt(static_cast<NativeSocket>(m_handle), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&enabled), sizeof(enabled));
    if (broadcast)
        setsockopt(static_cast<NativeSocket>(m_handle), SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&enabled), sizeof(enabled));

#ifdef _WIN32
    u_long nonBlocking = 1;
    ioctlsocket(static_cast<SOCKET>(m_handle), FIONBIO, &nonBlocking);
#else
    int flags = fcntl(static_cast<int>(m_handle), F_GETFL, 0);
    if (flags >= 0)
        fcntl(static_cast<int>(m_handle), F_SETFL, flags | O_NONBLOCK);
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (::bind(static_cast<NativeSocket>(m_handle), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        brls::Logger::error("[Netplay][Socket] bind failed port={} broadcast={} errno={}",
                            port, broadcast ? 1 : 0, platformSocketError());
        close();
        return false;
    }
    brls::Logger::info("[Netplay][Socket] open port={} broadcast={} handle={}",
                       port, broadcast ? 1 : 0, static_cast<long long>(m_handle));
    return true;
}

void UdpSocket::close()
{
    if (m_handle >= 0)
    {
        brls::Logger::info("[Netplay][Socket] close handle={}", static_cast<long long>(m_handle));
        closeSocket(m_handle);
        m_handle = -1;
    }
}

bool UdpSocket::sendTo(const std::string& host, uint16_t port, const void* data, size_t size)
{
    if (m_handle < 0 || !data || size == 0)
    {
        brls::Logger::warning("[Netplay][Socket] send skipped invalid handle={} size={}",
                              static_cast<long long>(m_handle), size);
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
    {
        brls::Logger::warning("[Netplay][Socket] send invalid address host={}", host);
        return false;
    }

    const int sent = ::sendto(static_cast<NativeSocket>(m_handle), static_cast<const char*>(data), static_cast<int>(size), 0,
                              reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (sent != static_cast<int>(size))
    {
        brls::Logger::error("[Netplay][Socket] sendto failed host={} port={} size={} sent={} errno={}",
                            host, port, size, sent, platformSocketError());
    }
    return sent == static_cast<int>(size);
}

int UdpSocket::receiveFrom(void* buffer, size_t capacity, std::string& fromHost, uint16_t& fromPort)
{
    if (m_handle < 0 || !buffer || capacity == 0)
        return -1;

    sockaddr_in from{};
    SocketLen fromLen = sizeof(from);
    const int received = ::recvfrom(static_cast<NativeSocket>(m_handle), static_cast<char*>(buffer), static_cast<int>(capacity), 0,
                                    reinterpret_cast<sockaddr*>(&from), &fromLen);
    if (received <= 0)
        return received;

    char ip[64]{};
    const char* text = inet_ntop(AF_INET, &from.sin_addr, ip, sizeof(ip));
    fromHost = text ? text : "";
    fromPort = ntohs(from.sin_port);
    return received;
}

std::string UdpSocket::detectLocalAddress()
{
    if (!ensureWinsock())
    {
        brls::Logger::error("[Netplay][Socket] detectLocalAddress socket service init failed");
        return "0.0.0.0";
    }

    const auto handle = static_cast<intptr_t>(::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    if (handle == INVALID_SOCKET_VALUE)
    {
        brls::Logger::error("[Netplay][Socket] detectLocalAddress socket create failed errno={}", platformSocketError());
        return "0.0.0.0";
    }

    sockaddr_in remote{};
    remote.sin_family = AF_INET;
    remote.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);

    std::string address = "0.0.0.0";
    if (::connect(static_cast<NativeSocket>(handle), reinterpret_cast<sockaddr*>(&remote), sizeof(remote)) == 0)
    {
        sockaddr_in local{};
        SocketLen localLen = sizeof(local);
        if (::getsockname(static_cast<NativeSocket>(handle), reinterpret_cast<sockaddr*>(&local), &localLen) == 0)
        {
            char ip[64]{};
            const char* text = inet_ntop(AF_INET, &local.sin_addr, ip, sizeof(ip));
            if (isUsableLocalAddress(text))
                address = text;
        }
    }

    closeSocket(handle);
    if (address == "0.0.0.0")
    {
        const auto fallback = detectAddressFromHostname();
        if (!fallback.empty())
            address = fallback;
    }
    brls::Logger::info("[Netplay][Socket] local address detected {}", address);
    return address;
}

std::string UdpSocket::detectDirectedBroadcastAddress()
{
    const std::string local = detectLocalAddress();
    unsigned int a = 0;
    unsigned int b = 0;
    unsigned int c = 0;
    unsigned int d = 0;
    char tail = '\0';
    if (std::sscanf(local.c_str(), "%u.%u.%u.%u%c", &a, &b, &c, &d, &tail) != 4 ||
        a > 255 || b > 255 || c > 255 || d > 255)
    {
        brls::Logger::warning("[Netplay][Socket] directed broadcast fallback for local={}", local);
        return "255.255.255.255";
    }

    std::ostringstream out;
    out << a << '.' << b << '.' << c << ".255";
    brls::Logger::info("[Netplay][Socket] directed broadcast {} from local={}", out.str(), local);
    return out.str();
}

int UdpSocket::lastSocketError()
{
    return platformSocketError();
}

} // namespace beiklive::netplay
