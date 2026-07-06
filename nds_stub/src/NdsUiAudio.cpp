#include "nds_stub/NdsUiAudio.hpp"

#include <cstdint>
#include <cstdio>

#include <switch.h>

#include "nds_stub/StubLog.hpp"

namespace beiklive::nds_stub {
namespace {

constexpr std::uint64_t kQlaunchProgramId = 0x0100000000001000ULL;
constexpr const char* kQlaunchMountPoint = "qlaunch";
constexpr const char* kRomfsMountPoint = "romfs";
constexpr const char* kBfsarPath = "/sound/qlaunch.bfsar";

} // namespace

NdsUiAudioPlayer::NdsUiAudioPlayer()
{
    m_sounds.fill(PLSR_PLAYER_INVALID_SOUND);

    PLSR_RC rc = plsrPlayerInit();
    if (PLSR_RC_FAILED(rc))
    {
        appendStubLog("GBAStationNDSStub: ui audio init failed rc=%#x", rc);
        return;
    }

    char bfsarPath[64] = {};
    std::uint64_t programId = 0;
    svcGetInfo(&programId, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);
    if (programId != kQlaunchProgramId)
    {
        const Result mountResult = romfsMountDataStorageFromProgram(kQlaunchProgramId, kQlaunchMountPoint);
        if (!R_SUCCEEDED(mountResult))
        {
            appendStubLog("GBAStationNDSStub: ui audio qlaunch romfs mount failed rc=%#x", mountResult);
            plsrPlayerExit();
            return;
        }
        std::snprintf(bfsarPath, sizeof(bfsarPath), "%s:%s", kQlaunchMountPoint, kBfsarPath);
    }
    else
    {
        std::snprintf(bfsarPath, sizeof(bfsarPath), "%s:%s", kRomfsMountPoint, kBfsarPath);
    }

    rc = plsrBFSAROpen(bfsarPath, &m_qlaunchBfsar);
    if (PLSR_RC_FAILED(rc))
    {
        appendStubLog("GBAStationNDSStub: ui audio bfsar open failed path=%s rc=%#x", bfsarPath, rc);
        plsrPlayerExit();
        return;
    }

    m_init = true;
}

NdsUiAudioPlayer::~NdsUiAudioPlayer()
{
    if (!m_init)
        return;

    for (auto sound : m_sounds)
    {
        if (sound != PLSR_PLAYER_INVALID_SOUND)
            plsrPlayerFree(sound);
    }

    plsrBFSARClose(&m_qlaunchBfsar);
    plsrPlayerExit();
}

NdsUiAudioPlayer::SoundSlot NdsUiAudioPlayer::slotForMenuSound(NdsMenuSound sound)
{
    switch (sound)
    {
    case NdsMenuSound::Focus: return SoundSlot::Focus;
    case NdsMenuSound::Click: return SoundSlot::Click;
    case NdsMenuSound::Back: return SoundSlot::Back;
    case NdsMenuSound::Error: return SoundSlot::Error;
    case NdsMenuSound::Slider: return SoundSlot::Slider;
    default: return SoundSlot::Error;
    }
}

const char* NdsUiAudioPlayer::soundName(SoundSlot slot)
{
    switch (slot)
    {
    case SoundSlot::Focus: return "SeNaviFocus";
    case SoundSlot::Click: return "SeBtnDecide";
    case SoundSlot::Back: return "SeFooterDecideFinish";
    case SoundSlot::Error: return "SeKeyErrorCursor";
    case SoundSlot::Slider: return "SeSliderTickOver";
    default: return "";
    }
}

bool NdsUiAudioPlayer::load(SoundSlot slot)
{
    if (!m_init)
        return false;

    const std::size_t index = static_cast<std::size_t>(slot);
    if (index >= m_sounds.size())
        return false;
    if (m_sounds[index] != PLSR_PLAYER_INVALID_SOUND)
        return true;

    const char* name = soundName(slot);
    if (!name || !name[0])
        return false;

    PLSR_RC rc = plsrPlayerLoadSoundByName(&m_qlaunchBfsar, name, &m_sounds[index]);
    if (PLSR_RC_FAILED(rc))
    {
        appendStubLog("GBAStationNDSStub: ui audio load failed name=%s rc=%#x", name, rc);
        m_sounds[index] = PLSR_PLAYER_INVALID_SOUND;
        return false;
    }

    return true;
}

bool NdsUiAudioPlayer::play(NdsMenuSound sound, float pitch)
{
    if (!m_init)
        return false;

    const SoundSlot slot = slotForMenuSound(sound);
    const std::size_t index = static_cast<std::size_t>(slot);
    if (index >= m_sounds.size())
        return false;
    if (m_sounds[index] == PLSR_PLAYER_INVALID_SOUND && !load(slot))
        return false;

    plsrPlayerSetPitch(m_sounds[index], pitch);
    const PLSR_RC rc = plsrPlayerPlay(m_sounds[index]);
    return !PLSR_RC_FAILED(rc);
}

} // namespace beiklive::nds_stub
