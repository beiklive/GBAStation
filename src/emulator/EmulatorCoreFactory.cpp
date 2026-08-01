#include "IEmulatorCore.hpp"
#include "emulator/CoreFceumm.hpp"
#include "emulator/CoreSnes9x.hpp"
#include "emulator/mgba_native/MgbaNativeCore.hpp"
#include "cores/genesis/GenesisCore.h"

namespace beiklive {

IEmulatorCore* CreateEmulatorCore(const beiklive::GameEntry& entry)
{
    const int platform = entry.platform;
    const std::string coreId = beiklive::NormalizeCoreId(platform, entry.core);
    switch (static_cast<beiklive::enums::EmuPlatform>(platform))
    {
    case beiklive::enums::EmuPlatform::EmuGBA:
        return new beiklive::mgba_native::MgbaNativeCore();
    case beiklive::enums::EmuPlatform::EmuGBC:
    case beiklive::enums::EmuPlatform::EmuGB:
        if (coreId == "gambatte")
            return new beiklive::fceumm::CoreFceumm(beiklive::CoreType::Gambatte, "Gambatte");
        return new beiklive::mgba_native::MgbaNativeCore();
    case beiklive::enums::EmuPlatform::EmuNES:
        if (coreId == "nestopia")
            return new beiklive::fceumm::CoreFceumm(beiklive::CoreType::Nestopia, "Nestopia");
        return new beiklive::fceumm::CoreFceumm(beiklive::CoreType::Fceumm, "FCEUmm");
    case beiklive::enums::EmuPlatform::EmuSNES:
        if (coreId == "snes9x2005")
            return new beiklive::snes9x::CoreSnes9x(beiklive::CoreType::Snes9x2005, "Snes9x 2005");
        if (coreId == "snes9x")
            return new beiklive::snes9x::CoreSnes9x(beiklive::CoreType::Snes9x, "Snes9x");
        return new beiklive::snes9x::CoreSnes9x(beiklive::CoreType::Snes9x2005, "Snes9x 2005");
    case beiklive::enums::EmuPlatform::EmuGenesis:
        return new beiklive::genesis::GenesisCore();
    case beiklive::enums::EmuPlatform::EmuNDS:
    case beiklive::enums::EmuPlatform::Emu3DS:
    case beiklive::enums::EmuPlatform::EmuArcade:
    case beiklive::enums::EmuPlatform::EmuDreamcast:
        return nullptr;
    default:
        return nullptr;
    }
}

} // namespace beiklive
