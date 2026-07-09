#include "emulator/mgba_native/MgbaNativeLink.hpp"

#include "network/netplay/NetplayManager.hpp"

#include <mgba/core/core.h>
#include <mgba/core/lockstep.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/sio/lockstep.h>

#include <memory>
#include <vector>

namespace beiklive::mgba_native
{
namespace
{
bool noopSignal(mLockstep*, unsigned)
{
    return true;
}

bool noopWait(mLockstep*, unsigned)
{
    return false;
}

void noopAddCycles(mLockstep*, int, int32_t)
{
}

int32_t noopUseCycles(mLockstep*, int, int32_t cycles)
{
    return cycles;
}

int32_t noopUnusedCycles(mLockstep*, int)
{
    return 0;
}

void noopUnload(mLockstep*, int)
{
}
} // namespace

struct MgbaNativeLink::State
{
    GBASIOLockstep lockstep{};
    std::vector<std::unique_ptr<GBASIOLockstepNode>> nodes;
};

MgbaNativeLink::MgbaNativeLink()
    : m_state(std::make_unique<State>())
{
    Reset();
}

MgbaNativeLink::~MgbaNativeLink() = default;

void MgbaNativeLink::Reset()
{
    m_state->nodes.clear();

    // mGBA 的 GBA Link Cable 入口来自 GBASIOLockstep。
    // 这里先初始化 mLockstep，再初始化 GBA SIO lockstep 数据。
    mLockstepInit(&m_state->lockstep.d);
    m_state->lockstep.d.signal = noopSignal;
    m_state->lockstep.d.wait = noopWait;
    m_state->lockstep.d.addCycles = noopAddCycles;
    m_state->lockstep.d.useCycles = noopUseCycles;
    m_state->lockstep.d.unusedCycles = noopUnusedCycles;
    m_state->lockstep.d.unload = noopUnload;
    GBASIOLockstepInit(&m_state->lockstep);
}

bool MgbaNativeLink::AttachGbaCore(mCore* core)
{
    if (!core || !core->board || !core->platform || core->platform(core) != mPLATFORM_GBA)
        return false;

    auto* gba = static_cast<GBA*>(core->board);
    if (!gba)
        return false;

    auto node = std::make_unique<GBASIOLockstepNode>();
    GBASIOLockstepNodeCreate(node.get());
    if (!GBASIOLockstepAttachNode(&m_state->lockstep, node.get()))
        return false;

    // mGBA Qt 的 MultiplayerController 也使用同一路径：
    // 1. SIO_MULTI 处理多人 16-bit Link Cable
    // 2. SIO_NORMAL_32 处理普通 32-bit Link Cable
    GBASIOSetDriver(&gba->sio, &node->d, SIO_MULTI);
    GBASIOSetDriver(&gba->sio, &node->d, SIO_NORMAL_32);
    m_state->nodes.push_back(std::move(node));
    return true;
}

void MgbaNativeLink::DetachGbaCore(mCore* core)
{
    if (!core || !core->board || !core->platform || core->platform(core) != mPLATFORM_GBA)
        return;

    auto* gba = static_cast<GBA*>(core->board);
    auto* node = reinterpret_cast<GBASIOLockstepNode*>(gba->sio.drivers.multiplayer);

    GBASIOSetDriver(&gba->sio, nullptr, SIO_MULTI);
    GBASIOSetDriver(&gba->sio, nullptr, SIO_NORMAL_32);

    if (!node)
        return;

    GBASIOLockstepDetachNode(&m_state->lockstep, node);
    for (auto it = m_state->nodes.begin(); it != m_state->nodes.end(); ++it)
    {
        if (it->get() == node)
        {
            m_state->nodes.erase(it);
            break;
        }
    }
}

void MgbaNativeLink::SendLinkData(const beiklive::netplay::LinkDataPacket& packet)
{
    if (m_callback)
        m_callback->SendLinkPacket(&packet, sizeof(packet));
    beiklive::netplay::NetplayManager::instance().sendLinkData(packet);
}

bool MgbaNativeLink::ReceiveLinkData(beiklive::netplay::LinkDataPacket& packet)
{
    return beiklive::netplay::NetplayManager::instance().popIncomingLinkData(packet);
}

} // namespace beiklive::mgba_native
