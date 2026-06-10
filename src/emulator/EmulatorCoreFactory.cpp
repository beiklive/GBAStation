#include "IEmulatorCore.hpp"
#include "emulator/CoreFceumm.hpp"
#include "emulator/CoreSnes9x.hpp"
#include "emulator/CorePicoDrive.hpp"
#include "game/mgba/GameRun.hpp"

namespace beiklive {

IEmulatorCore* CreateEmulatorCore(int platform)
{
    switch (static_cast<beiklive::enums::EmuPlatform>(platform))
    {
    case beiklive::enums::EmuPlatform::EmuGBA:
    case beiklive::enums::EmuPlatform::EmuGBC:
    case beiklive::enums::EmuPlatform::EmuGB:
        return new beiklive::gba::CoreMgba();
    case beiklive::enums::EmuPlatform::EmuNES:
        return new beiklive::fceumm::CoreFceumm();
    case beiklive::enums::EmuPlatform::EmuSNES:
        return new beiklive::snes9x::CoreSnes9x();
    case beiklive::enums::EmuPlatform::EmuGenesis:
        return new beiklive::picodrive::CorePicoDrive();
    default:
        return nullptr;
    }
}

} // namespace beiklive
