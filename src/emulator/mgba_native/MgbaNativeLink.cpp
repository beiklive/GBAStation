#include "emulator/mgba_native/MgbaNativeLink.hpp"

#include "network/netplay/NetplayManager.hpp"

#include <borealis.hpp>

#include <mgba/core/core.h>
#include <mgba/core/lockstep.h>
#include <mgba/core/timing.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>
#include <mgba/internal/gba/sio.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <memory>

namespace beiklive::mgba_native
{
namespace
{
constexpr uint8_t LINK_FLAG_TRANSFER = 1;
constexpr uint8_t LINK_FLAG_PHASE_SHIFT = 1;
constexpr uint8_t LINK_FLAG_PHASE_MASK = 0x0E;
constexpr size_t LINK_PLAYERS = beiklive::netplay::MAX_LINK_PLAYERS;
constexpr size_t HOST_FINISHED_CACHE_SIZE = 32;
constexpr int HOST_MULTI_RESPONSE_POLL_CYCLES = 32768;
constexpr auto HOST_MULTI_RESEND_INTERVAL = std::chrono::milliseconds(33);
constexpr auto HOST_MULTI_WAIT_LOG_INTERVAL = std::chrono::seconds(3);

const char* sioModeName(uint8_t mode)
{
    switch (mode)
    {
    case SIO_NORMAL_8: return "SIO_NORMAL_8";
    case SIO_NORMAL_32: return "SIO_NORMAL_32";
    case SIO_MULTI: return "SIO_MULTI";
    case SIO_UART: return "SIO_UART";
    case SIO_GPIO: return "SIO_GPIO";
    case SIO_JOYBUS: return "SIO_JOYBUS";
    default: return "SIO_UNKNOWN";
    }
}

bool shouldLogSioCounter(uint64_t count)
{
    return count <= 24 || (count % 300) == 0;
}

const char* lockstepPhaseName(mLockstepPhase phase)
{
    switch (phase)
    {
    case TRANSFER_IDLE: return "IDLE";
    case TRANSFER_STARTING: return "STARTING";
    case TRANSFER_STARTED: return "STARTED";
    case TRANSFER_FINISHING: return "FINISHING";
    case TRANSFER_FINISHED: return "FINISHED";
    default: return "UNKNOWN";
    }
}

uint8_t linkFlagsForPhase(uint8_t baseFlags, mLockstepPhase phase)
{
    return static_cast<uint8_t>((baseFlags & ~LINK_FLAG_PHASE_MASK) |
                                ((static_cast<uint8_t>(phase) << LINK_FLAG_PHASE_SHIFT) & LINK_FLAG_PHASE_MASK));
}

mLockstepPhase linkPhaseFromFlags(uint8_t flags)
{
    const uint8_t raw = static_cast<uint8_t>((flags & LINK_FLAG_PHASE_MASK) >> LINK_FLAG_PHASE_SHIFT);
    if (raw > static_cast<uint8_t>(TRANSFER_FINISHED))
        return TRANSFER_IDLE;
    return static_cast<mLockstepPhase>(raw);
}

struct NetplayRuntime;
void hostMultiFinishEvent(struct mTiming* timing, void* context, uint32_t cyclesLate);
void remoteMultiFinishEvent(struct mTiming* timing, void* context, uint32_t cyclesLate);
void finishPendingRemoteMultiTransfer(NetplayRuntime& runtime, const char* reason, uint32_t cyclesLate);

struct NetplaySIODriver
{
    GBASIODriver d{};
    NetplayRuntime* runtime = nullptr;
    GBASIOMode mode = SIO_MULTI;
};

struct NetplayRuntime
{
    MgbaNativeLink* owner = nullptr;
    NetplaySIODriver multiDriver{};
    NetplaySIODriver normalDriver{};
    std::array<uint16_t, LINK_PLAYERS> multiRecv{};
    std::array<uint32_t, LINK_PLAYERS> normalRecv{};
    std::array<uint8_t, LINK_PLAYERS> normalMode{};
    std::array<bool, LINK_PLAYERS> hasMulti{};
    std::array<bool, LINK_PLAYERS> hasNormal{};
    std::array<bool, LINK_PLAYERS> pendingMultiTransfer{};
    std::array<bool, LINK_PLAYERS> pendingNormalTransfer{};
    std::array<uint64_t, LINK_PLAYERS> pendingMultiCycle{};
    std::array<uint16_t, LINK_PLAYERS> pendingMultiSiocnt{};
    std::array<mLockstepPhase, LINK_PLAYERS> pendingMultiPhase{};
    uint64_t nextMultiTransferId = 1;
    uint64_t lastRespondedMasterCycle = 0;
    uint16_t lastRespondedMasterData = 0xFFFF;
    uint64_t lastFinishedMasterCycle = 0;
    uint16_t lastFinishedMasterData = 0xFFFF;
    struct FinishedMultiCacheEntry
    {
        uint64_t cycle = 0;
        uint16_t data = 0xFFFF;
        uint16_t siocnt = 0;
        uint16_t rcnt = 0;
        bool valid = false;
    };
    std::array<FinishedMultiCacheEntry, HOST_FINISHED_CACHE_SIZE> finishedMultiCache{};
    size_t finishedMultiCacheNext = 0;
    bool hostMultiPending = false;
    bool hostMultiResponseReady = false;
    bool hostMultiFinishSent = false;
    bool hostMultiFinishAckReady = false;
    bool hostMultiFinishScheduled = false;
    bool hostMultiFinishDue = false;
    mTimingEvent hostMultiFinishEvent{};
    uint64_t hostMultiCycle = 0;
    uint64_t hostMultiStartCycle = 0;
    uint16_t hostMultiSiocnt = 0;
    uint16_t hostMultiData = 0xFFFF;
    std::chrono::steady_clock::time_point hostMultiNextResend{};
    std::chrono::steady_clock::time_point hostMultiNextWaitLog{};
    bool remoteMultiPending = false;
    bool remoteMultiFinishScheduled = false;
    mTimingEvent remoteMultiFinishEvent{};
    uint64_t remoteMultiMasterCycle = 0;
    uint16_t remoteMultiSiocnt = 0;
    uint16_t remoteMultiLocalData = 0xFFFF;
    std::array<uint16_t, LINK_PLAYERS> remoteMultiValues{};
    uint16_t localMultiSend = 0xFFFF;
    uint16_t localNormalLo = 0xFFFF;
    uint16_t localNormalHi = 0xFFFF;
    bool hasLocalNormalLo = false;
    bool hasLocalNormalHi = false;
    uint8_t localPlayerId = 0;
    bool attached = false;
    GBA* gba = nullptr;
    uint64_t generatedPackets = 0;
    uint64_t consumedPackets = 0;
    uint64_t ignoredPackets = 0;
    uint64_t multiTransfers = 0;
    uint64_t normalTransfers = 0;
    uint64_t remoteDrivenTransfers = 0;
    uint64_t remoteDuplicateResponses = 0;
    uint64_t hostMultiWaitLogs = 0;
    uint64_t hostMultiResends = 0;
    uint64_t hostMultiPolls = 0;
    uint64_t multiSendWrites = 0;
    uint64_t normalDataWrites = 0;
};

NetplaySIODriver* asNetplayDriver(GBASIODriver* driver)
{
    return reinterpret_cast<NetplaySIODriver*>(driver);
}

uint64_t sendLinkData(NetplaySIODriver* driver, uint32_t data, uint8_t mode, uint8_t flags, uint64_t forcedCycle);
void rememberFinishedMulti(NetplayRuntime& runtime, uint64_t cycle, uint16_t data, uint16_t siocnt, uint16_t rcnt);
const NetplayRuntime::FinishedMultiCacheEntry* findFinishedMulti(const NetplayRuntime& runtime, uint64_t cycle);
void sendHostFinished(NetplayRuntime& runtime, const char* reason);

uint64_t currentCycle(GBASIODriver* driver)
{
    if (!driver || !driver->p || !driver->p->p)
        return 0;
    return mTimingGlobalTime(&driver->p->p->timing);
}

void resetRuntimeCaches(NetplayRuntime& runtime)
{
    if (runtime.gba && runtime.hostMultiFinishScheduled &&
        mTimingIsScheduled(&runtime.gba->timing, &runtime.hostMultiFinishEvent))
    {
        mTimingDeschedule(&runtime.gba->timing, &runtime.hostMultiFinishEvent);
    }
    if (runtime.gba && runtime.remoteMultiFinishScheduled &&
        mTimingIsScheduled(&runtime.gba->timing, &runtime.remoteMultiFinishEvent))
    {
        mTimingDeschedule(&runtime.gba->timing, &runtime.remoteMultiFinishEvent);
    }
    runtime.multiRecv.fill(0xFFFF);
    runtime.normalRecv.fill(0xFFFFFFFFu);
    runtime.normalMode.fill(0);
    runtime.hasMulti.fill(false);
    runtime.hasNormal.fill(false);
    runtime.pendingMultiTransfer.fill(false);
    runtime.pendingNormalTransfer.fill(false);
    runtime.pendingMultiCycle.fill(0);
    runtime.pendingMultiSiocnt.fill(0);
    runtime.pendingMultiPhase.fill(TRANSFER_IDLE);
    runtime.localMultiSend = 0xFFFF;
    runtime.localNormalLo = 0xFFFF;
    runtime.localNormalHi = 0xFFFF;
    runtime.hasLocalNormalLo = false;
    runtime.hasLocalNormalHi = false;
    runtime.generatedPackets = 0;
    runtime.consumedPackets = 0;
    runtime.ignoredPackets = 0;
    runtime.multiTransfers = 0;
    runtime.normalTransfers = 0;
    runtime.remoteDrivenTransfers = 0;
    runtime.lastRespondedMasterCycle = 0;
    runtime.lastRespondedMasterData = 0xFFFF;
    runtime.lastFinishedMasterCycle = 0;
    runtime.lastFinishedMasterData = 0xFFFF;
    runtime.finishedMultiCache = {};
    runtime.finishedMultiCacheNext = 0;
    runtime.remoteDuplicateResponses = 0;
    runtime.nextMultiTransferId = 1;
    runtime.hostMultiPending = false;
    runtime.hostMultiResponseReady = false;
    runtime.hostMultiFinishSent = false;
    runtime.hostMultiFinishAckReady = false;
    runtime.hostMultiFinishScheduled = false;
    runtime.hostMultiFinishDue = false;
    runtime.hostMultiCycle = 0;
    runtime.hostMultiStartCycle = 0;
    runtime.hostMultiSiocnt = 0;
    runtime.hostMultiData = 0xFFFF;
    runtime.hostMultiNextResend = {};
    runtime.hostMultiNextWaitLog = {};
    runtime.hostMultiWaitLogs = 0;
    runtime.hostMultiResends = 0;
    runtime.hostMultiPolls = 0;
    runtime.multiSendWrites = 0;
    runtime.normalDataWrites = 0;
    runtime.remoteMultiPending = false;
    runtime.remoteMultiFinishScheduled = false;
    runtime.remoteMultiMasterCycle = 0;
    runtime.remoteMultiSiocnt = 0;
    runtime.remoteMultiLocalData = 0xFFFF;
    runtime.remoteMultiValues.fill(0xFFFF);
}

void rememberFinishedMulti(NetplayRuntime& runtime, uint64_t cycle, uint16_t data, uint16_t siocnt, uint16_t rcnt)
{
    if (cycle == 0)
        return;

    auto& entry = runtime.finishedMultiCache[runtime.finishedMultiCacheNext % runtime.finishedMultiCache.size()];
    entry.cycle = cycle;
    entry.data = data;
    entry.siocnt = siocnt;
    entry.rcnt = rcnt;
    entry.valid = true;
    runtime.finishedMultiCacheNext = (runtime.finishedMultiCacheNext + 1) % runtime.finishedMultiCache.size();
}

const NetplayRuntime::FinishedMultiCacheEntry* findFinishedMulti(const NetplayRuntime& runtime, uint64_t cycle)
{
    if (cycle == 0)
        return nullptr;

    for (const auto& entry : runtime.finishedMultiCache)
    {
        if (entry.valid && entry.cycle == cycle)
            return &entry;
    }
    return nullptr;
}

void refreshLocalPlayerId(NetplayRuntime& runtime)
{
    const auto snapshot = beiklive::netplay::NetplayManager::instance().snapshot();
    runtime.localPlayerId = std::min<uint8_t>(snapshot.localPlayerId, LINK_PLAYERS - 1);
    brls::Logger::info("[Netplay][SIO] refresh localPlayerId={} state={} hosting={} tx={} rx={}",
                       static_cast<int>(runtime.localPlayerId), beiklive::netplay::toString(snapshot.state),
                       snapshot.hosting ? 1 : 0, snapshot.txPackets, snapshot.rxPackets);
}

void drainIncoming(NetplayRuntime& runtime)
{
    if (!runtime.owner)
        return;

    beiklive::netplay::LinkDataPacket packet;
    while (runtime.owner->ReceiveLinkData(packet))
    {
        if (packet.playerId >= LINK_PLAYERS || packet.playerId == runtime.localPlayerId)
        {
            ++runtime.ignoredPackets;
            if (shouldLogSioCounter(runtime.ignoredPackets))
            {
                brls::Logger::warning("[Netplay][SIO] ignore incoming packet player={} local={} mode={} cycle={} data={:#x}",
                                      static_cast<int>(packet.playerId), static_cast<int>(runtime.localPlayerId),
                                      sioModeName(packet.mode), packet.cycle, packet.data);
            }
            continue;
        }

        if (packet.mode == SIO_MULTI)
        {
            const mLockstepPhase phase = linkPhaseFromFlags(packet.flags);
            runtime.multiRecv[packet.playerId] = static_cast<uint16_t>(packet.data & 0xFFFF);
            runtime.hasMulti[packet.playerId] = true;
            if (packet.flags & LINK_FLAG_TRANSFER)
            {
                runtime.pendingMultiTransfer[packet.playerId] = true;
                runtime.pendingMultiCycle[packet.playerId] = packet.cycle;
                runtime.pendingMultiSiocnt[packet.playerId] = packet.siocnt;
                runtime.pendingMultiPhase[packet.playerId] = phase;
                if (runtime.localPlayerId == 0 && runtime.hostMultiPending &&
                    packet.cycle == runtime.hostMultiCycle && phase == TRANSFER_STARTED)
                {
                    runtime.hostMultiResponseReady = true;
                }
                else if (runtime.localPlayerId == 0 && runtime.hostMultiPending &&
                         packet.cycle == runtime.hostMultiCycle && phase == TRANSFER_FINISHED)
                {
                    runtime.hostMultiFinishAckReady = true;
                }
                else if (runtime.localPlayerId == 0 && phase == TRANSFER_STARTED)
                {
                    const auto* finished = findFinishedMulti(runtime, packet.cycle);
                    if (finished)
                    {
                        ++runtime.hostMultiResends;
                        sendLinkData(&runtime.multiDriver, finished->data, SIO_MULTI,
                                     linkFlagsForPhase(LINK_FLAG_TRANSFER, TRANSFER_FINISHED), packet.cycle);
                        if (shouldLogSioCounter(runtime.hostMultiResends))
                        {
                            brls::Logger::info("[Netplay][SIO] host resend cached FINISHED #{} oldId={} data={:#x} currentPending={} currentId={}",
                                               runtime.hostMultiResends, packet.cycle, finished->data,
                                               runtime.hostMultiPending ? 1 : 0, runtime.hostMultiCycle);
                        }
                    }
                    else if (runtime.hostMultiPending && packet.cycle < runtime.hostMultiCycle)
                    {
                        brls::Logger::warning("[Netplay][SIO] host missing cached FINISHED oldId={} currentId={} cacheNext={}",
                                              packet.cycle, runtime.hostMultiCycle, runtime.finishedMultiCacheNext);
                    }
                }
            }
            ++runtime.consumedPackets;
            if (shouldLogSioCounter(runtime.consumedPackets))
            {
                brls::Logger::info("[Netplay][SIO] consume MULTI packet #{} player={} id={} data={:#x} siocnt={:#x} flags={:#x} phase={}",
                                   runtime.consumedPackets, static_cast<int>(packet.playerId), packet.cycle,
                                   packet.data, packet.siocnt, static_cast<int>(packet.flags),
                                   lockstepPhaseName(phase));
            }
            if (runtime.hostMultiResponseReady || runtime.hostMultiFinishAckReady)
                break;
            if (runtime.localPlayerId != 0 && (packet.flags & LINK_FLAG_TRANSFER))
                break;
        }
        else if (packet.mode == SIO_NORMAL_8 || packet.mode == SIO_NORMAL_32)
        {
            runtime.normalRecv[packet.playerId] = packet.data;
            runtime.normalMode[packet.playerId] = packet.mode;
            runtime.hasNormal[packet.playerId] = true;
            if (packet.flags & LINK_FLAG_TRANSFER)
                runtime.pendingNormalTransfer[packet.playerId] = true;
            ++runtime.consumedPackets;
            if (shouldLogSioCounter(runtime.consumedPackets))
            {
                brls::Logger::info("[Netplay][SIO] consume NORMAL packet #{} player={} mode={} cycle={} data={:#x} siocnt={:#x} flags={:#x}",
                                   runtime.consumedPackets, static_cast<int>(packet.playerId),
                                   sioModeName(packet.mode), packet.cycle, packet.data,
                                   packet.siocnt, static_cast<int>(packet.flags));
            }
        }
        else
        {
            ++runtime.ignoredPackets;
            brls::Logger::warning("[Netplay][SIO] ignore unsupported incoming mode={} player={} data={:#x}",
                                  static_cast<int>(packet.mode), static_cast<int>(packet.playerId), packet.data);
        }
    }
}

uint64_t sendLinkData(NetplaySIODriver* driver, uint32_t data, uint8_t mode, uint8_t flags, uint64_t forcedCycle = 0)
{
    if (!driver || !driver->runtime || !driver->runtime->owner || !driver->d.p)
        return 0;

    beiklive::netplay::LinkDataPacket packet;
    packet.cycle = forcedCycle != 0 ? forcedCycle : currentCycle(&driver->d);
    packet.data = data;
    packet.siocnt = driver->d.p->siocnt;
    packet.rcnt = driver->d.p->rcnt;
    packet.flags = flags;
    packet.playerId = driver->runtime->localPlayerId;
    packet.mode = mode;
    ++driver->runtime->generatedPackets;
    if (shouldLogSioCounter(driver->runtime->generatedPackets))
    {
        brls::Logger::info("[Netplay][SIO] generate packet #{} player={} mode={} id={} data={:#x} siocnt={:#x} rcnt={:#x} flags={:#x} phase={}",
                           driver->runtime->generatedPackets, static_cast<int>(packet.playerId),
                           sioModeName(packet.mode), packet.cycle, packet.data, packet.siocnt,
                           packet.rcnt, static_cast<int>(packet.flags),
                           lockstepPhaseName(linkPhaseFromFlags(packet.flags)));
    }
    driver->runtime->owner->SendLinkData(packet);
    return packet.cycle;
}

void raiseSioIrqIfNeeded(GBASIODriver* driver, bool irq)
{
    if (irq && driver && driver->p && driver->p->p)
        GBARaiseIRQ(driver->p->p, GBA_IRQ_SIO, 0);
}

void applyMultiplayerIdentity(GBASIO* sio, uint8_t localPlayerId)
{
    if (!sio)
        return;

    sio->siocnt = GBASIOMultiplayerSetSlave(sio->siocnt, localPlayerId > 0);
    sio->siocnt = GBASIOMultiplayerSetId(sio->siocnt, localPlayerId);
    if (localPlayerId > 0)
        sio->rcnt |= 4;
    else
        sio->rcnt &= ~4;
    if (sio->p)
        sio->p->memory.io[REG_SIOCNT >> 1] = sio->siocnt;
}

void serviceRemoteMultiTransfer(NetplayRuntime& runtime)
{
    if (!runtime.gba)
        return;
    if (runtime.localPlayerId == 0)
        return;

    constexpr size_t MASTER_PLAYER = 0;
    if (!runtime.pendingMultiTransfer[MASTER_PLAYER])
        return;

    const uint64_t masterCycle = runtime.pendingMultiCycle[MASTER_PLAYER];
    const uint16_t masterSiocnt = runtime.pendingMultiSiocnt[MASTER_PLAYER];
    const mLockstepPhase phase = runtime.pendingMultiPhase[MASTER_PLAYER];
    runtime.pendingMultiTransfer[MASTER_PLAYER] = false;

    if (phase == TRANSFER_FINISHED)
    {
        if (!runtime.remoteMultiPending && masterCycle == runtime.lastRespondedMasterCycle)
        {
            ++runtime.remoteDuplicateResponses;
            sendLinkData(&runtime.multiDriver, runtime.lastRespondedMasterData, SIO_MULTI,
                         linkFlagsForPhase(LINK_FLAG_TRANSFER, TRANSFER_FINISHED), masterCycle);
            if (shouldLogSioCounter(runtime.remoteDuplicateResponses))
            {
                brls::Logger::info("[Netplay][SIO] duplicate master FINISHED #{} id={} resendFinishAck={:#x}",
                                   runtime.remoteDuplicateResponses, masterCycle,
                                   runtime.lastRespondedMasterData);
            }
            return;
        }
        if (runtime.remoteMultiPending && masterCycle == runtime.remoteMultiMasterCycle)
            finishPendingRemoteMultiTransfer(runtime, "master-finished", 0);
        return;
    }

    if (phase != TRANSFER_STARTED)
        return;

    if (runtime.remoteMultiPending)
    {
        if (masterCycle == runtime.remoteMultiMasterCycle)
        {
            ++runtime.remoteDuplicateResponses;
            sendLinkData(&runtime.multiDriver, runtime.remoteMultiLocalData, SIO_MULTI,
                         linkFlagsForPhase(LINK_FLAG_TRANSFER, TRANSFER_STARTED), masterCycle);
            if (shouldLogSioCounter(runtime.remoteDuplicateResponses))
            {
                brls::Logger::info("[Netplay][SIO] duplicate active master MULTI #{} id={} localPlayer={} resendAck={:#x}",
                                   runtime.remoteDuplicateResponses, masterCycle,
                                   static_cast<int>(runtime.localPlayerId), runtime.remoteMultiLocalData);
            }
            return;
        }
        ++runtime.remoteDuplicateResponses;
        sendLinkData(&runtime.multiDriver, runtime.remoteMultiLocalData, SIO_MULTI,
                     linkFlagsForPhase(LINK_FLAG_TRANSFER, TRANSFER_STARTED), runtime.remoteMultiMasterCycle);
        if (shouldLogSioCounter(runtime.remoteDuplicateResponses))
        {
            brls::Logger::warning("[Netplay][SIO] defer new remote MULTI while busy localPlayer={} currentId={} nextId={} resendCurrentAck={:#x}",
                                  static_cast<int>(runtime.localPlayerId), runtime.remoteMultiMasterCycle,
                                  masterCycle, runtime.remoteMultiLocalData);
        }
        return;
    }
    if (masterCycle != 0 && masterCycle == runtime.lastRespondedMasterCycle)
    {
        ++runtime.remoteDuplicateResponses;
        if (runtime.multiDriver.d.p)
            sendLinkData(&runtime.multiDriver, runtime.lastRespondedMasterData, SIO_MULTI,
                         linkFlagsForPhase(LINK_FLAG_TRANSFER, TRANSFER_STARTED), masterCycle);
        if (shouldLogSioCounter(runtime.remoteDuplicateResponses))
        {
            brls::Logger::info("[Netplay][SIO] duplicate completed master MULTI #{} id={} resendAck={:#x}",
                               runtime.remoteDuplicateResponses, masterCycle,
                               runtime.lastRespondedMasterData);
        }
        return;
    }

    auto* gba = runtime.gba;
    auto* sio = &gba->sio;
    runtime.localMultiSend = gba->memory.io[REG_SIOMLT_SEND >> 1];

    runtime.remoteMultiValues.fill(0xFFFF);
    runtime.remoteMultiValues[runtime.localPlayerId] = runtime.localMultiSend;
    for (size_t i = 0; i < runtime.remoteMultiValues.size(); ++i)
    {
        if (runtime.hasMulti[i])
            runtime.remoteMultiValues[i] = runtime.multiRecv[i];
    }
    runtime.remoteMultiLocalData = runtime.localMultiSend;
    runtime.remoteMultiMasterCycle = masterCycle;
    runtime.remoteMultiSiocnt = masterSiocnt != 0 ? masterSiocnt : sio->siocnt;
    runtime.remoteMultiPending = true;

    gba->memory.io[REG_SIOMULTI0 >> 1] = 0xFFFF;
    gba->memory.io[REG_SIOMULTI1 >> 1] = 0xFFFF;
    gba->memory.io[REG_SIOMULTI2 >> 1] = 0xFFFF;
    gba->memory.io[REG_SIOMULTI3 >> 1] = 0xFFFF;
    sio->rcnt &= ~1;
    sio->siocnt = GBASIOMultiplayerFillBusy(sio->siocnt);
    gba->memory.io[REG_SIOCNT >> 1] = sio->siocnt;

    sendLinkData(&runtime.multiDriver, runtime.remoteMultiLocalData, SIO_MULTI,
                 linkFlagsForPhase(LINK_FLAG_TRANSFER, TRANSFER_STARTED), masterCycle);
    gba->earlyExit = true;
    if (shouldLogSioCounter(runtime.remoteDrivenTransfers + 1))
    {
        brls::Logger::info("[Netplay][SIO] remote MULTI STARTED localPlayer={} id={} localSend={:#x} recv=[{:#x},{:#x},{:#x},{:#x}] siocnt={:#x}",
                           static_cast<int>(runtime.localPlayerId), masterCycle,
                           runtime.localMultiSend, runtime.remoteMultiValues[0], runtime.remoteMultiValues[1],
                           runtime.remoteMultiValues[2], runtime.remoteMultiValues[3], sio->siocnt);
    }
}

void finishPendingRemoteMultiTransfer(NetplayRuntime& runtime, const char* reason, uint32_t cyclesLate)
{
    if (!runtime.remoteMultiPending || !runtime.gba)
        return;

    auto* gba = runtime.gba;
    auto* sio = &gba->sio;
    const auto values = runtime.remoteMultiValues;
    const uint64_t masterCycle = runtime.remoteMultiMasterCycle;

    runtime.remoteMultiPending = false;
    runtime.remoteMultiFinishScheduled = false;
    runtime.remoteMultiMasterCycle = 0;
    runtime.remoteMultiSiocnt = 0;

    gba->memory.io[REG_SIOMULTI0 >> 1] = values[0];
    gba->memory.io[REG_SIOMULTI1 >> 1] = values[1];
    gba->memory.io[REG_SIOMULTI2 >> 1] = values[2];
    gba->memory.io[REG_SIOMULTI3 >> 1] = values[3];

    sio->rcnt |= 1;
    sio->siocnt = GBASIOMultiplayerSetId(sio->siocnt, runtime.localPlayerId);
    sio->siocnt = GBASIOMultiplayerClearBusy(sio->siocnt);
    gba->memory.io[REG_SIOCNT >> 1] = sio->siocnt;

    runtime.lastRespondedMasterCycle = masterCycle;
    runtime.lastRespondedMasterData = runtime.remoteMultiLocalData;

    ++runtime.remoteDrivenTransfers;
    if (shouldLogSioCounter(runtime.remoteDrivenTransfers))
    {
        brls::Logger::info("[Netplay][SIO] remote MULTI finish by {} #{} localPlayer={} masterCycle={} localSend={:#x} recv=[{:#x},{:#x},{:#x},{:#x}] siocnt={:#x} cyclesLate={}",
                           reason ? reason : "unknown", runtime.remoteDrivenTransfers,
                           static_cast<int>(runtime.localPlayerId), masterCycle, runtime.remoteMultiLocalData,
                           values[0], values[1], values[2], values[3], sio->siocnt, cyclesLate);
    }

    raiseSioIrqIfNeeded(&runtime.multiDriver.d, GBASIOMultiplayerIsIrq(sio->siocnt));

    sendLinkData(&runtime.multiDriver, runtime.lastRespondedMasterData, SIO_MULTI,
                 linkFlagsForPhase(LINK_FLAG_TRANSFER, TRANSFER_FINISHED), masterCycle);
}

void remoteMultiFinishEvent(struct mTiming*, void* context, uint32_t cyclesLate)
{
    auto* runtime = static_cast<NetplayRuntime*>(context);
    if (!runtime)
        return;
    runtime->remoteMultiFinishScheduled = false;
    finishPendingRemoteMultiTransfer(*runtime, "timing", cyclesLate);
}

uint16_t finishMultiTransfer(NetplaySIODriver* driver, uint16_t value, uint16_t localTransferData)
{
    auto& runtime = *driver->runtime;
    auto* sio = driver->d.p;
    auto* gba = sio->p;
    drainIncoming(runtime);

    std::array<uint16_t, LINK_PLAYERS> values{};
    values.fill(0xFFFF);
    values[runtime.localPlayerId] = localTransferData;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (runtime.hasMulti[i])
            values[i] = runtime.multiRecv[i];
    }

    gba->memory.io[REG_SIOMULTI0 >> 1] = values[0];
    gba->memory.io[REG_SIOMULTI1 >> 1] = values[1];
    gba->memory.io[REG_SIOMULTI2 >> 1] = values[2];
    gba->memory.io[REG_SIOMULTI3 >> 1] = values[3];

    ++runtime.multiTransfers;
    if (shouldLogSioCounter(runtime.multiTransfers))
    {
        brls::Logger::info("[Netplay][SIO] finish MULTI #{} localPlayer={} send={:#x} recv=[{:#x},{:#x},{:#x},{:#x}] has=[{},{},{},{}] siocntIn={:#x}",
                           runtime.multiTransfers, static_cast<int>(runtime.localPlayerId), localTransferData,
                           values[0], values[1], values[2], values[3],
                           runtime.hasMulti[0] ? 1 : 0, runtime.hasMulti[1] ? 1 : 0,
                           runtime.hasMulti[2] ? 1 : 0, runtime.hasMulti[3] ? 1 : 0, value);
    }

    sio->rcnt |= 1;
    value = GBASIOMultiplayerClearBusy(sio->siocnt);
    value = GBASIOMultiplayerSetId(value, runtime.localPlayerId);
    sio->siocnt = value;
    gba->memory.io[REG_SIOCNT >> 1] = value;
    raiseSioIrqIfNeeded(&driver->d, GBASIOMultiplayerIsIrq(value));
    return value;
}

void clearHostMultiState(NetplayRuntime& runtime)
{
    if (runtime.gba && runtime.hostMultiFinishScheduled &&
        mTimingIsScheduled(&runtime.gba->timing, &runtime.hostMultiFinishEvent))
    {
        mTimingDeschedule(&runtime.gba->timing, &runtime.hostMultiFinishEvent);
    }
    runtime.hostMultiPending = false;
    runtime.hostMultiResponseReady = false;
    runtime.hostMultiFinishSent = false;
    runtime.hostMultiFinishAckReady = false;
    runtime.hostMultiFinishScheduled = false;
    runtime.hostMultiFinishDue = false;
    runtime.hostMultiCycle = 0;
    runtime.hostMultiStartCycle = 0;
    runtime.hostMultiSiocnt = 0;
    runtime.hostMultiData = 0xFFFF;
    runtime.hostMultiNextResend = {};
    runtime.hostMultiNextWaitLog = {};
    runtime.hostMultiPolls = 0;
}

void sendHostFinished(NetplayRuntime& runtime, const char* reason)
{
    if (!runtime.hostMultiPending || !runtime.gba)
        return;

    runtime.hostMultiFinishSent = true;
    runtime.hostMultiNextResend = std::chrono::steady_clock::now() + HOST_MULTI_RESEND_INTERVAL;
    sendLinkData(&runtime.multiDriver, runtime.hostMultiData, SIO_MULTI,
                 linkFlagsForPhase(LINK_FLAG_TRANSFER, TRANSFER_FINISHED), runtime.hostMultiCycle);
    runtime.gba->earlyExit = true;
    if (shouldLogSioCounter(runtime.hostMultiResends + 1))
    {
        brls::Logger::info("[Netplay][SIO] host MULTI send FINISHED by {} id={} data={:#x}",
                           reason ? reason : "unknown", runtime.hostMultiCycle, runtime.hostMultiData);
    }
}

void finishPendingHostMultiTransfer(NetplayRuntime& runtime, const char* reason, uint32_t cyclesLate)
{
    if (!runtime.hostMultiPending)
        return;

    const uint16_t siocnt = runtime.hostMultiSiocnt;
    const uint16_t localTransferData = runtime.hostMultiData;
    const uint64_t transferCycle = runtime.hostMultiCycle;
    const bool responseReady = runtime.hostMultiResponseReady;
    const bool finishAckReady = runtime.hostMultiFinishAckReady;

    clearHostMultiState(runtime);
    if (shouldLogSioCounter(runtime.multiTransfers + 1))
    {
        brls::Logger::info("[Netplay][SIO] host MULTI finish by {} cycle={} local={:#x} responseReady={} finishAck={} cyclesLate={}",
                           reason ? reason : "unknown", transferCycle, localTransferData,
                           responseReady ? 1 : 0, finishAckReady ? 1 : 0, cyclesLate);
    }
    finishMultiTransfer(&runtime.multiDriver, siocnt, localTransferData);
    runtime.lastFinishedMasterCycle = transferCycle;
    runtime.lastFinishedMasterData = localTransferData;
    rememberFinishedMulti(runtime, transferCycle, localTransferData, siocnt,
                          runtime.multiDriver.d.p ? runtime.multiDriver.d.p->rcnt : 0);
}

void hostMultiFinishEvent(struct mTiming*, void* context, uint32_t cyclesLate)
{
    auto* runtime = static_cast<NetplayRuntime*>(context);
    if (!runtime)
        return;
    runtime->hostMultiFinishScheduled = false;
    runtime->hostMultiFinishDue = true;
    drainIncoming(*runtime);
    if (runtime->hostMultiResponseReady)
    {
        sendHostFinished(*runtime, "timing");
        return;
    }
    if (runtime->hostMultiPending && runtime->gba)
    {
        runtime->gba->earlyExit = true;
        runtime->hostMultiFinishScheduled = true;
        mTimingSchedule(&runtime->gba->timing, &runtime->hostMultiFinishEvent, HOST_MULTI_RESPONSE_POLL_CYCLES);
        ++runtime->hostMultiPolls;
        if (runtime->hostMultiPolls == 1 && shouldLogSioCounter(runtime->multiTransfers + 1))
        {
            brls::Logger::info("[Netplay][SIO] host MULTI wait STARTED ack cycle={} pollCycles={} cyclesLate={}",
                               runtime->hostMultiCycle, HOST_MULTI_RESPONSE_POLL_CYCLES, cyclesLate);
        }
    }
}

uint16_t beginHostMultiTransfer(NetplaySIODriver* driver, uint16_t value)
{
    auto& runtime = *driver->runtime;
    auto* sio = driver->d.p;
    auto* gba = sio->p;

    if (runtime.hostMultiPending)
    {
        return GBASIOMultiplayerFillBusy(driver->d.p ? driver->d.p->siocnt : value);
    }

    runtime.localMultiSend = gba->memory.io[REG_SIOMLT_SEND >> 1];
    gba->memory.io[REG_SIOMULTI0 >> 1] = 0xFFFF;
    gba->memory.io[REG_SIOMULTI1 >> 1] = 0xFFFF;
    gba->memory.io[REG_SIOMULTI2 >> 1] = 0xFFFF;
    gba->memory.io[REG_SIOMULTI3 >> 1] = 0xFFFF;
    sio->rcnt &= ~1;

    runtime.hostMultiResponseReady = false;
    runtime.hostMultiFinishSent = false;
    runtime.hostMultiFinishAckReady = false;
    runtime.hostMultiFinishDue = false;
    runtime.hostMultiSiocnt = value;
    runtime.hostMultiData = runtime.localMultiSend;
    runtime.hostMultiStartCycle = currentCycle(&driver->d);
    runtime.hostMultiCycle = runtime.nextMultiTransferId++;
    if (runtime.hostMultiCycle == 0)
        runtime.hostMultiCycle = runtime.nextMultiTransferId++;
    sendLinkData(driver, runtime.localMultiSend, SIO_MULTI,
                 linkFlagsForPhase(LINK_FLAG_TRANSFER, TRANSFER_STARTED), runtime.hostMultiCycle);
    runtime.hostMultiPending = true;
    runtime.hostMultiNextResend = std::chrono::steady_clock::now() + HOST_MULTI_RESEND_INTERVAL;
    runtime.hostMultiNextWaitLog = std::chrono::steady_clock::now() + HOST_MULTI_WAIT_LOG_INTERVAL;
    if (runtime.hostMultiPending)
    {
        const unsigned baud = GBASIOMultiplayerGetBaud(value);
        const int transferCycles = std::max(1, GBASIOCyclesPerTransfer[baud][1]);
        if (mTimingIsScheduled(&gba->timing, &runtime.hostMultiFinishEvent))
            mTimingDeschedule(&gba->timing, &runtime.hostMultiFinishEvent);
        runtime.hostMultiFinishScheduled = true;
        mTimingSchedule(&gba->timing, &runtime.hostMultiFinishEvent, transferCycles);
        if (shouldLogSioCounter(runtime.multiTransfers + 1))
        {
            brls::Logger::info("[Netplay][SIO] host MULTI STARTED schedule finish cycles={} baud={} startCycle={} id={} data={:#x}",
                               transferCycles, baud, runtime.hostMultiStartCycle,
                               runtime.hostMultiCycle, runtime.hostMultiData);
        }
    }

    value = GBASIOMultiplayerFillBusy(value);
    value = GBASIOMultiplayerSetId(value, 0);
    return value;
}

void serviceHostMultiTransfer(NetplayRuntime& runtime)
{
    if (!runtime.gba || runtime.localPlayerId != 0 || !runtime.hostMultiPending)
        return;

    if (runtime.hostMultiFinishAckReady)
    {
        finishPendingHostMultiTransfer(runtime, "finish-ack", 0);
        return;
    }

    if (runtime.hostMultiResponseReady && runtime.hostMultiFinishDue && !runtime.hostMultiFinishSent)
    {
        sendHostFinished(runtime, "pump");
        return;
    }

    if (!runtime.hostMultiResponseReady || runtime.hostMultiFinishSent)
    {
        const auto nowWall = std::chrono::steady_clock::now();
        if (nowWall >= runtime.hostMultiNextResend)
        {
            ++runtime.hostMultiResends;
            const bool resendFinished = runtime.hostMultiFinishSent;
            sendLinkData(&runtime.multiDriver, runtime.hostMultiData, SIO_MULTI,
                         linkFlagsForPhase(LINK_FLAG_TRANSFER,
                                           resendFinished ? TRANSFER_FINISHED : TRANSFER_STARTED),
                         runtime.hostMultiCycle);
            runtime.hostMultiNextResend = nowWall + HOST_MULTI_RESEND_INTERVAL;
            if (shouldLogSioCounter(runtime.hostMultiResends))
            {
                brls::Logger::info("[Netplay][SIO] host MULTI resend {} #{} id={} data={:#x}",
                                   resendFinished ? "FINISHED" : "STARTED",
                                   runtime.hostMultiResends, runtime.hostMultiCycle,
                                   runtime.hostMultiData);
            }
        }
        if (nowWall >= runtime.hostMultiNextWaitLog)
        {
            ++runtime.hostMultiWaitLogs;
            brls::Logger::warning("[Netplay][SIO] host MULTI still waiting {} #{} id={} latest=[{:#x},{:#x},{:#x},{:#x}] has=[{},{},{},{}]",
                                  runtime.hostMultiFinishSent ? "FINISHED ack" : "STARTED ack",
                                  runtime.hostMultiWaitLogs, runtime.hostMultiCycle,
                                  runtime.multiRecv[0], runtime.multiRecv[1], runtime.multiRecv[2], runtime.multiRecv[3],
                                  runtime.hasMulti[0] ? 1 : 0, runtime.hasMulti[1] ? 1 : 0,
                                  runtime.hasMulti[2] ? 1 : 0, runtime.hasMulti[3] ? 1 : 0);
            runtime.hostMultiNextWaitLog = nowWall + HOST_MULTI_WAIT_LOG_INTERVAL;
        }
        return;
    }
}

uint32_t localNormalData(NetplayRuntime& runtime, GBASIODriver* driver, uint8_t mode)
{
    if (!driver || !driver->p || !driver->p->p)
        return 0xFFFFFFFFu;

    auto* gba = driver->p->p;
    if (mode == SIO_NORMAL_8)
        return gba->memory.io[REG_SIODATA8 >> 1] & 0xFF;

    const uint16_t lo = runtime.hasLocalNormalLo
                            ? runtime.localNormalLo
                            : gba->memory.io[REG_SIODATA32_LO >> 1];
    const uint16_t hi = runtime.hasLocalNormalHi
                            ? runtime.localNormalHi
                            : gba->memory.io[REG_SIODATA32_HI >> 1];
    return static_cast<uint32_t>(lo) | (static_cast<uint32_t>(hi) << 16);
}

uint32_t latestRemoteNormal(NetplayRuntime& runtime, uint8_t mode)
{
    for (size_t i = 0; i < runtime.hasNormal.size(); ++i)
    {
        if (i == runtime.localPlayerId || !runtime.hasNormal[i])
            continue;
        if (runtime.normalMode[i] == mode)
            return runtime.normalRecv[i];
    }
    return mode == SIO_NORMAL_8 ? 0xFFu : 0xFFFFFFFFu;
}

uint16_t finishNormalTransfer(NetplaySIODriver* driver, uint16_t value)
{
    auto& runtime = *driver->runtime;
    auto* sio = driver->d.p;
    auto* gba = sio->p;
    const uint8_t mode = static_cast<uint8_t>(sio->mode);
    drainIncoming(runtime);

    const uint32_t remoteData = latestRemoteNormal(runtime, mode);
    ++runtime.normalTransfers;
    if (shouldLogSioCounter(runtime.normalTransfers))
    {
        brls::Logger::info("[Netplay][SIO] finish NORMAL #{} mode={} localPlayer={} local={:#x} remote={:#x} siocntIn={:#x}",
                           runtime.normalTransfers, sioModeName(mode), static_cast<int>(runtime.localPlayerId),
                           localNormalData(runtime, &driver->d, mode), remoteData, value);
    }
    if (mode == SIO_NORMAL_8)
    {
        gba->memory.io[REG_SIODATA8 >> 1] = static_cast<uint16_t>(remoteData & 0xFF);
    }
    else
    {
        gba->memory.io[REG_SIODATA32_LO >> 1] = static_cast<uint16_t>(remoteData & 0xFFFF);
        gba->memory.io[REG_SIODATA32_HI >> 1] = static_cast<uint16_t>((remoteData >> 16) & 0xFFFF);
    }

    value &= 0xFF8B;
    value = GBASIONormalClearStart(value);
    value = GBASIONormalSetSi(value, true);
    sio->siocnt = value;
    gba->memory.io[REG_SIOCNT >> 1] = value;
    raiseSioIrqIfNeeded(&driver->d, GBASIONormalIsIrq(value));
    return value;
}

bool netplayDriverInit(GBASIODriver* driver)
{
    auto* netDriver = asNetplayDriver(driver);
    if (!netDriver || !netDriver->runtime)
        return false;

    refreshLocalPlayerId(*netDriver->runtime);
    if (driver->p && netDriver->mode == SIO_MULTI)
    {
        applyMultiplayerIdentity(driver->p, netDriver->runtime->localPlayerId);
    }
    brls::Logger::info("[Netplay][SIO] driver init driverMode={} coreMode={} localPlayer={} siocnt={:#x}",
                       sioModeName(netDriver->mode),
                       driver->p ? sioModeName(static_cast<uint8_t>(driver->p->mode)) : "no-sio",
                       static_cast<int>(netDriver->runtime->localPlayerId),
                       driver->p ? driver->p->siocnt : 0);
    return true;
}

void netplayDriverDeinit(GBASIODriver* driver)
{
    auto* netDriver = asNetplayDriver(driver);
    brls::Logger::info("[Netplay][SIO] driver deinit driverMode={}",
                       netDriver ? sioModeName(netDriver->mode) : "unknown");
}

bool netplayDriverLoad(GBASIODriver* driver)
{
    auto* netDriver = asNetplayDriver(driver);
    if (!netDriver || !netDriver->runtime)
        return false;

    refreshLocalPlayerId(*netDriver->runtime);
    brls::Logger::info("[Netplay][SIO] driver load driverMode={} coreMode={} localPlayer={}",
                       sioModeName(netDriver->mode),
                       driver->p ? sioModeName(static_cast<uint8_t>(driver->p->mode)) : "no-sio",
                       static_cast<int>(netDriver->runtime->localPlayerId));
    if (netDriver->mode == SIO_MULTI)
    {
        if (driver->p)
        {
            applyMultiplayerIdentity(driver->p, netDriver->runtime->localPlayerId);
            driver->p->siocnt = GBASIOMultiplayerSetReady(driver->p->siocnt, true);
            driver->p->p->memory.io[REG_SIOCNT >> 1] = driver->p->siocnt;
        }
        driver->writeRegister = [](GBASIODriver* active, uint32_t address, uint16_t value) -> uint16_t {
            auto* activeDriver = asNetplayDriver(active);
            auto& runtime = *activeDriver->runtime;

            if (address == REG_SIOMLT_SEND)
            {
                runtime.localMultiSend = value;
                ++runtime.multiSendWrites;
                if (shouldLogSioCounter(runtime.multiSendWrites))
                    brls::Logger::info("[Netplay][SIO] write SIOMLT_SEND value={:#x}", value);
                return value;
            }

            if (address != REG_SIOCNT)
                return value;

            if (active->p)
                applyMultiplayerIdentity(active->p, runtime.localPlayerId);

            value &= 0xFF83;
            if (active->p)
                value |= active->p->siocnt & 0x00FC;
            if (value & 0x0080)
            {
                if (runtime.localPlayerId != 0)
                    return value;

                if (active->p && active->p->p)
                    runtime.localMultiSend = active->p->p->memory.io[REG_SIOMLT_SEND >> 1];
                if (!runtime.hostMultiPending && shouldLogSioCounter(runtime.multiTransfers + 1))
                    brls::Logger::info("[Netplay][SIO] MULTI start siocnt={:#x} send={:#x}",
                                       value, runtime.localMultiSend);
                return beginHostMultiTransfer(activeDriver, value);
            }

            return value;
        };
    }
    else
    {
        driver->writeRegister = [](GBASIODriver* active, uint32_t address, uint16_t value) -> uint16_t {
            auto* activeDriver = asNetplayDriver(active);
            auto& runtime = *activeDriver->runtime;

            if (address == REG_SIODATA8)
            {
                ++runtime.normalDataWrites;
                if (shouldLogSioCounter(runtime.normalDataWrites))
                    brls::Logger::info("[Netplay][SIO] write SIODATA8 value={:#x}", value & 0xFF);
                sendLinkData(activeDriver, value & 0xFF, SIO_NORMAL_8, 0);
                return value;
            }
            if (address == REG_SIODATA32_LO)
            {
                runtime.localNormalLo = value;
                runtime.hasLocalNormalLo = true;
                ++runtime.normalDataWrites;
                if (shouldLogSioCounter(runtime.normalDataWrites))
                    brls::Logger::info("[Netplay][SIO] write SIODATA32_LO value={:#x}", value);
                sendLinkData(activeDriver, localNormalData(runtime, active, SIO_NORMAL_32), SIO_NORMAL_32, 0);
                return value;
            }
            if (address == REG_SIODATA32_HI)
            {
                runtime.localNormalHi = value;
                runtime.hasLocalNormalHi = true;
                ++runtime.normalDataWrites;
                if (shouldLogSioCounter(runtime.normalDataWrites))
                    brls::Logger::info("[Netplay][SIO] write SIODATA32_HI value={:#x}", value);
                sendLinkData(activeDriver, localNormalData(runtime, active, SIO_NORMAL_32), SIO_NORMAL_32, 0);
                return value;
            }
            if (address != REG_SIOCNT)
                return value;

            value &= 0xFF8B;
            value = GBASIONormalSetSi(value, true);
            if ((value & 0x0081) == 0x0081)
            {
                const uint8_t mode = active->p ? static_cast<uint8_t>(active->p->mode) : SIO_NORMAL_32;
                if (shouldLogSioCounter(runtime.normalTransfers + 1))
                    brls::Logger::info("[Netplay][SIO] NORMAL start mode={} siocnt={:#x} local={:#x}",
                                       sioModeName(mode), value, localNormalData(runtime, active, mode));
                sendLinkData(activeDriver, localNormalData(runtime, active, mode), mode, LINK_FLAG_TRANSFER);
                return finishNormalTransfer(activeDriver, value);
            }
            return value;
        };
    }
    return true;
}

bool netplayDriverUnload(GBASIODriver* driver)
{
    auto* netDriver = asNetplayDriver(driver);
    brls::Logger::info("[Netplay][SIO] driver unload driverMode={} coreMode={}",
                       netDriver ? sioModeName(netDriver->mode) : "unknown",
                       driver && driver->p ? sioModeName(static_cast<uint8_t>(driver->p->mode)) : "no-sio");
    if (driver)
        driver->writeRegister = nullptr;
    return true;
}

void initDriver(NetplayRuntime& runtime, NetplaySIODriver& driver, GBASIOMode mode)
{
    driver = {};
    driver.runtime = &runtime;
    driver.mode = mode;
    driver.d.init = netplayDriverInit;
    driver.d.deinit = netplayDriverDeinit;
    driver.d.load = netplayDriverLoad;
    driver.d.unload = netplayDriverUnload;
    driver.d.writeRegister = nullptr;
}
} // namespace

struct MgbaNativeLink::State
{
    NetplayRuntime runtime{};
};

MgbaNativeLink::MgbaNativeLink()
    : m_state(std::make_unique<State>())
{
    m_state->runtime.owner = this;
    Reset();
}

MgbaNativeLink::~MgbaNativeLink() = default;

void MgbaNativeLink::Reset()
{
    auto& runtime = m_state->runtime;
    runtime.owner = this;
    GBA* attachedGba = runtime.gba;
    resetRuntimeCaches(runtime);
    runtime.gba = attachedGba;
    refreshLocalPlayerId(runtime);
    initDriver(runtime, runtime.multiDriver, SIO_MULTI);
    initDriver(runtime, runtime.normalDriver, SIO_NORMAL_32);
    runtime.hostMultiFinishEvent.context = &runtime;
    runtime.hostMultiFinishEvent.callback = hostMultiFinishEvent;
    runtime.hostMultiFinishEvent.name = "BeikLive Netplay SIO Multi";
    runtime.remoteMultiFinishEvent.context = &runtime;
    runtime.remoteMultiFinishEvent.callback = remoteMultiFinishEvent;
    runtime.remoteMultiFinishEvent.name = "BeikLive Netplay SIO Remote Multi";
    brls::Logger::info("[Netplay][SIO] runtime reset localPlayer={}", static_cast<int>(runtime.localPlayerId));
}

void MgbaNativeLink::Pump()
{
    auto& runtime = m_state->runtime;
    if (!runtime.attached || !runtime.gba)
        return;

    drainIncoming(runtime);
    serviceHostMultiTransfer(runtime);
    serviceRemoteMultiTransfer(runtime);
}

bool MgbaNativeLink::IsWaitingForPeer() const
{
    const auto& runtime = m_state->runtime;
    if (!runtime.attached || !runtime.gba)
        return false;
    if (runtime.localPlayerId == 0)
        return runtime.hostMultiPending;
    return runtime.remoteMultiPending;
}

bool MgbaNativeLink::AttachGbaCore(mCore* core)
{
    if (!core || !core->board || !core->platform || core->platform(core) != mPLATFORM_GBA)
        return false;

    auto* gba = static_cast<GBA*>(core->board);
    if (!gba)
        return false;

    Reset();
    auto& runtime = m_state->runtime;
    runtime.gba = gba;
    GBASIOSetDriver(&gba->sio, &runtime.multiDriver.d, SIO_MULTI);
    GBASIOSetDriver(&gba->sio, &runtime.normalDriver.d, SIO_NORMAL_32);
    runtime.attached = true;
    brls::Logger::info("[Netplay][SIO] attached to GBA core localPlayer={} activeMode={} siocnt={:#x} rcnt={:#x}",
                       static_cast<int>(runtime.localPlayerId), sioModeName(static_cast<uint8_t>(gba->sio.mode)),
                       gba->sio.siocnt, gba->sio.rcnt);
    return true;
}

void MgbaNativeLink::DetachGbaCore(mCore* core)
{
    if (!core || !core->board || !core->platform || core->platform(core) != mPLATFORM_GBA)
        return;

    auto& runtime = m_state->runtime;
    if (!runtime.attached)
        return;

    auto* gba = static_cast<GBA*>(core->board);
    if (!gba)
        return;

    if (gba->sio.drivers.multiplayer == &runtime.multiDriver.d)
        GBASIOSetDriver(&gba->sio, nullptr, SIO_MULTI);
    if (gba->sio.drivers.normal == &runtime.normalDriver.d)
        GBASIOSetDriver(&gba->sio, nullptr, SIO_NORMAL_32);
    runtime.attached = false;
    runtime.gba = nullptr;
    brls::Logger::info("[Netplay][SIO] detached generated={} consumed={} ignored={} multiTransfers={} normalTransfers={}",
                       runtime.generatedPackets, runtime.consumedPackets, runtime.ignoredPackets,
                       runtime.multiTransfers, runtime.normalTransfers);
    resetRuntimeCaches(runtime);
}

void MgbaNativeLink::SendLinkData(const beiklive::netplay::LinkDataPacket& packet)
{
    if (m_callback)
        m_callback->SendLinkPacket(&packet, sizeof(packet));
    if (shouldLogSioCounter(m_state->runtime.generatedPackets))
    {
        brls::Logger::info("[Netplay][SIO] hand packet to manager player={} mode={} cycle={} data={:#x}",
                           static_cast<int>(packet.playerId), sioModeName(packet.mode), packet.cycle, packet.data);
    }
    beiklive::netplay::NetplayManager::instance().sendLinkData(packet);
}

bool MgbaNativeLink::ReceiveLinkData(beiklive::netplay::LinkDataPacket& packet)
{
    const bool ok = beiklive::netplay::NetplayManager::instance().popIncomingLinkData(packet);
    if (ok && shouldLogSioCounter(m_state->runtime.consumedPackets + 1))
    {
        brls::Logger::info("[Netplay][SIO] manager packet available player={} mode={} cycle={} data={:#x}",
                           static_cast<int>(packet.playerId), sioModeName(packet.mode), packet.cycle, packet.data);
    }
    return ok;
}

} // namespace beiklive::mgba_native
