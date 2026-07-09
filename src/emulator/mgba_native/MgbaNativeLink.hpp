#pragma once

#include "network/netplay/Packet.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

struct mCore;

namespace beiklive::mgba_native
{

class IMGbaLinkCallback
{
public:
    virtual ~IMGbaLinkCallback() = default;
    virtual void SendLinkPacket(const void* data, size_t size) = 0;
};

class MgbaNativeLink
{
public:
    MgbaNativeLink();
    ~MgbaNativeLink();

    MgbaNativeLink(const MgbaNativeLink&) = delete;
    MgbaNativeLink& operator=(const MgbaNativeLink&) = delete;

    bool AttachGbaCore(mCore* core);
    void DetachGbaCore(mCore* core);
    void Reset();

    void SetCallback(IMGbaLinkCallback* callback) { m_callback = callback; }
    void SendLinkData(const beiklive::netplay::LinkDataPacket& packet);
    bool ReceiveLinkData(beiklive::netplay::LinkDataPacket& packet);

private:
    struct State;
    std::unique_ptr<State> m_state;
    IMGbaLinkCallback* m_callback = nullptr;
};

} // namespace beiklive::mgba_native
