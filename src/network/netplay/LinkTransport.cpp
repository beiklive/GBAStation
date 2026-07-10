#include "network/netplay/LinkTransport.hpp"

#include <borealis.hpp>

#include <array>
#include <chrono>
#include <cstring>
#include <utility>

namespace beiklive::netplay
{
namespace
{
constexpr size_t HEADER_SIZE = 16;

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

bool shouldLogLinkCounter(uint32_t count)
{
    return count <= 16 || (count % 300) == 0;
}

void writeU16(std::vector<uint8_t>& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void writeU32(std::vector<uint8_t>& out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

uint16_t readU16(const uint8_t* data)
{
    return static_cast<uint16_t>(data[0]) |
           static_cast<uint16_t>(data[1] << 8);
}

uint32_t readU32(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

std::vector<uint8_t> encodePacket(PacketType type, uint32_t sequence, const std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> out;
    out.reserve(HEADER_SIZE + payload.size());
    writeU32(out, PACKET_MAGIC);
    writeU16(out, PACKET_VERSION);
    writeU16(out, static_cast<uint16_t>(type));
    writeU32(out, sequence);
    writeU32(out, static_cast<uint32_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

bool decodePacket(const uint8_t* data, size_t size, PacketType& type, std::vector<uint8_t>& payload)
{
    if (!data || size < HEADER_SIZE)
    {
        brls::Logger::warning("[Netplay][Transport] drop packet: too small size={}", size);
        return false;
    }

    const uint32_t magic = readU32(data);
    const uint16_t version = readU16(data + 4);
    const auto rawType = readU16(data + 6);
    const uint32_t payloadSize = readU32(data + 12);
    if (magic != PACKET_MAGIC || version != PACKET_VERSION || HEADER_SIZE + payloadSize > size)
    {
        brls::Logger::warning("[Netplay][Transport] drop packet: magic={:#x} version={} rawType={} payload={} size={}",
                              magic, version, rawType, payloadSize, size);
        return false;
    }

    type = static_cast<PacketType>(rawType);
    payload.assign(data + HEADER_SIZE, data + HEADER_SIZE + payloadSize);
    return true;
}
} // namespace

LinkTransport::~LinkTransport()
{
    stop();
}

bool LinkTransport::start(uint16_t localPort, PacketCallback callback)
{
    stop();
    m_networkReady = m_network.Initialize();
    if (!m_networkReady)
    {
        brls::Logger::error("[Netplay][Transport] NetworkManager initialize failed localPort={}", localPort);
        return false;
    }

    if (!m_socket.open(localPort, false))
    {
        brls::Logger::error("[Netplay][Transport] socket open failed localPort={} errno={}",
                            localPort, UdpSocket::lastSocketError());
        m_network.Shutdown();
        m_networkReady = false;
        return false;
    }

    m_callback = std::move(callback);
    m_running.store(true);
    m_thread = std::thread(&LinkTransport::receiveLoop, this);
    brls::Logger::info("[Netplay][Transport] started localPort={}", localPort);
    return true;
}

void LinkTransport::stop()
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
    m_callback = nullptr;
    if (wasRunning)
        brls::Logger::info("[Netplay][Transport] stopped");
}

bool LinkTransport::sendPacket(const std::string& host, uint16_t port, PacketType type, const std::vector<uint8_t>& payload)
{
    const uint32_t sequence = m_sequence.fetch_add(1);
    const auto packet = encodePacket(type, sequence, payload);
    const bool sent = m_socket.sendTo(host, port, packet.data(), packet.size());
    if (type == PacketType::LinkData)
    {
        if (shouldLogLinkCounter(sequence) || !sent)
        {
            brls::Logger::info("[Netplay][Transport] send {} seq={} to {}:{} payload={} ok={} errno={}",
                               packetTypeName(type), sequence, host, port, payload.size(), sent ? 1 : 0,
                               sent ? 0 : UdpSocket::lastSocketError());
        }
    }
    else
    {
        brls::Logger::info("[Netplay][Transport] send {} seq={} to {}:{} payload={} ok={} errno={}",
                           packetTypeName(type), sequence, host, port, payload.size(), sent ? 1 : 0,
                           sent ? 0 : UdpSocket::lastSocketError());
    }
    return sent;
}

void LinkTransport::receiveLoop()
{
    std::array<uint8_t, 2048> buffer{};
    while (m_running.load())
    {
        std::string host;
        uint16_t port = 0;
        const int received = m_socket.receiveFrom(buffer.data(), buffer.size(), host, port);
        if (received > 0)
        {
            PacketType type = PacketType::Discover;
            std::vector<uint8_t> payload;
            if (decodePacket(buffer.data(), static_cast<size_t>(received), type, payload) && m_callback)
            {
                if (type != PacketType::LinkData)
                {
                    brls::Logger::info("[Netplay][Transport] recv {} from {}:{} payload={} bytes={}",
                                       packetTypeName(type), host, port, payload.size(), received);
                }
                m_callback(type, std::move(payload), std::move(host), port);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

} // namespace beiklive::netplay
