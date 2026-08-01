//
// Created by cpasjuste on 29/05/18.
//

#include <string>
#include "skeleton/pemu.h"
#include "pfbneo_config.h"

using namespace c2d;
using namespace pemu;

#define C2D_CONFIG_RESTART_EMU_NEEDED "YOU NEED TO RESTART EMULATION AFTER CHANGING THIS OPTION"

PFBAConfig::PFBAConfig(Renderer *renderer, const int version) : PEMUConfig(renderer, "PFBNEO", version) {
    printf("PFBNConfig(%s, v%i)\n", getPath().c_str(), version);

    c2d::Io *io = renderer->getIo();

    /// MAIN OPTIONS
    get(UI_SHOW_ZIP_NAMES)->setArrayIndex(0);

    /// ROMS OPTIONS
    const auto group = getGroup(EMULATION);
    if (!group) {
        printf("PFBNConfig: error, group not found (MENU_ROM_OPTIONS)\n");
        return;
    }
    group->addOption(
            {"FORCE_60HZ", {"OFF", "ON"}, 1, EMU_FORCE_60HZ, C2D_CONFIG_RESTART_EMU_NEEDED});
    group->addOption({"AUDIO_FREQUENCY", {"11025", "22050", "32000", "44100", "48000"},
                      3, EMU_AUDIO_FREQ, C2D_CONFIG_RESTART_EMU_NEEDED});
    group->addOption({"AUDIO_INTERPOLATION", {"0", "1", "3"},
                      2, EMU_AUDIO_INTERPOLATION, C2D_CONFIG_RESTART_EMU_NEEDED});
    group->addOption({"AUDIO_FM_INTERPOLATION", {"0", "1", "3"},
                      2, EMU_AUDIO_FMINTERPOLATION, C2D_CONFIG_RESTART_EMU_NEEDED});
#ifdef __VITA__
    group->addOption({"ROTATION", {"OFF", "ON", "FLIP", "CAB MODE"},
                      1, EMU_ROTATION, C2D_CONFIG_RESTART_EMU_NEEDED});
#else
    group->addOption({"ROTATION", {"OFF", "ON", "FLIP"},
                      1, EMU_ROTATION, C2D_CONFIG_RESTART_EMU_NEEDED});
#endif
    group->addOption(
            {"NEOBIOS", {"UNIBIOS_4_0", "UNIBIOS_3_3", "UNIBIOS_3_2", "UNIBIOS_3_1",
                         "MVS_ASIA_EUR_V6S1", "MVS_ASIA_EUR_V5S1", "MVS_ASIA_EUR_V3S4",
                         "MVS_USA_V5S2", "MVS_USA_V5S4", "MVS_USA_V5S6", "MVS_JPN_V6",
                         "MVS_JPN_V5", "MVS_JPN_V3S4", "MVS_JPN_J3", "AES_ASIA",
                         "AES_JAPAN", "NEO_MVH_MV1CA", "NEO_MVH_MV1CJ", "DECK_V6", "DEVKIT"},
             0, EMU_NEOBIOS, C2D_CONFIG_RESTART_EMU_NEEDED});
#ifdef __PFBA_ARM__
    // do not use unibios as default on vita for cyclone asm compatibility
    group->getOption(EMU_NEOBIOS)->setArrayIndex(4);
    group->addOption({"FRAMESKIP", {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"},
                      0, EMU_FRAMESKIP, C2D_CONFIG_RESTART_EMU_NEEDED});
#endif

#if defined(__PS4__) || defined(ANDROID)
    // force 48000hz audio output
    get(EMU_AUDIO_FREQ)->setArrayIndex(4);
    get(EMU_AUDIO_FREQ)->setFlags(PEMUConfig::Flags::HIDDEN);
#endif

    // "romlist.cpp" (RomList::build) will also reload config, but we need new roms paths
    PEMUConfig::load();

    // add custom rom path
    addRomPath("ARCADE", io->getDataPath() + "arcade/", {HARDWARE_PREFIX_ARCADE, 0, "Arcade"});
#ifndef __PFBN_LIGHT__
    addRomPath("CHANNELF", io->getDataPath() + "channelf/", {HARDWARE_PREFIX_CHANNELF, 0, "Channel F"});
    addRomPath("COLECO", io->getDataPath() + "coleco/", {HARDWARE_PREFIX_COLECO, 0, "ColecoVision"});
    addRomPath("FDS", io->getDataPath() + "fds/", {HARDWARE_PREFIX_FDS, 0, "NES FDS"});
    addRomPath("GAMEGEAR", io->getDataPath() + "gamegear/",
                           {HARDWARE_PREFIX_SEGA_GAME_GEAR, 0, "Sega Game Gear"});
    addRomPath("MEGADRIVE", io->getDataPath() + "megadrive/",
                           {HARDWARE_PREFIX_SEGA_MEGADRIVE, 0, "Sega Megadrive"});
    addRomPath("MSX", io->getDataPath() + "msx/", {HARDWARE_PREFIX_MSX, 0, "MSX"});
    addRomPath("NES", io->getDataPath() + "nes/", {HARDWARE_PREFIX_NES, 0, "NES"});
    addRomPath("NGP", io->getDataPath() + "ngp/", {HARDWARE_PREFIX_NGP, 0, "Neo Geo Pocket"});
    addRomPath("PCE", io->getDataPath() + "pce/", {HARDWARE_PCENGINE_PCENGINE, 0, "PC Engine"});
    addRomPath("SG1000", io->getDataPath() + "sg1000/", {HARDWARE_PREFIX_SEGA_SG1000, 0, "Sega SG-1000"});
    addRomPath("SGX", io->getDataPath() + "sgx/", {HARDWARE_PCENGINE_SGX, 0, "PC Engine SGX"});
    addRomPath("MASTERSYSTEM", io->getDataPath() + "sms/",
                           {HARDWARE_PREFIX_SEGA_MASTER_SYSTEM, 0, "Sega Master System"});
    addRomPath("SPECTRUM", io->getDataPath() + "spectrum/", {HARDWARE_PREFIX_SPECTRUM, 0, "ZX Spectrum"});
    addRomPath("TG16", io->getDataPath() + "tg16/", {HARDWARE_PCENGINE_TG16, 0, "PC Engine TG16"});
#endif

    // save newly added roms paths
    PEMUConfig::save();

    // create roms paths if needed
    const auto paths = getRomPaths();
    for (const auto &path: paths) {
        io->create(path.path);
    }
}
