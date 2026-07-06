#pragma once

#include <array>
#include <cstddef>

#include <pulsar.h>

#include "nds_stub/NdsMenuLayer.hpp"

namespace beiklive::nds_stub {

class NdsUiAudioPlayer {
public:
    NdsUiAudioPlayer();
    ~NdsUiAudioPlayer();

    bool play(NdsMenuSound sound, float pitch = 1.0f);

private:
    enum class SoundSlot {
        Focus,
        Click,
        Back,
        Error,
        Slider,
        Count,
    };

    bool load(SoundSlot slot);
    static SoundSlot slotForMenuSound(NdsMenuSound sound);
    static const char* soundName(SoundSlot slot);

    bool m_init = false;
    PLSR_BFSAR m_qlaunchBfsar {};
    std::array<PLSR_PlayerSoundId, static_cast<std::size_t>(SoundSlot::Count)> m_sounds {};
};

} // namespace beiklive::nds_stub
