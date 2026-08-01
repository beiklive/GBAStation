//
// Created by cpasjuste on 01/06/18.
//

#include "burner.h"
#include "burnint.h"

#include "skeleton/pemu.h"
#include "pfbneo_ui_emu.h"
#include "pfbneo_io.h"
#include "pfbneo_ui_video.h"
#include "pfbneo_utility.h"
#include "retro_input_wrapper.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace c2d;
using namespace pemu;

int nVidFullscreen = 0;
INT32 bVidUseHardwareGamma = 1;

UINT32 (__cdecl *VidHighCol)(INT32 r, INT32 g, INT32 b, INT32 i);

INT32 VidRecalcPal() { return BurnRecalcPal(); }

#ifdef __PFBA_ARM__
extern int nSekCpuCore;

static bool isHardware(int hardware, int type) {
    return (((hardware | HARDWARE_PREFIX_CARTRIDGE) ^ HARDWARE_PREFIX_CARTRIDGE)
            & 0xff000000) == (unsigned int) type;
}

#endif

static UINT32 myHighCol16(int r, int g, int b, int /* i */) {
    UINT32 t;
    t = (r << 8) & 0xf800;
    t |= (g << 3) & 0x07e0;
    t |= (b >> 3) & 0x001f;
    return t;
}

static UiMain *uiInstance;

namespace {

std::string trim(const std::string &text) {
    const auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (begin >= end) return "";
    return std::string(begin, end);
}

std::string unescapeConfigValue(const std::string &text) {
    std::string out;
    out.reserve(text.size());
    bool escaped = false;
    for (char ch: text) {
        if (escaped) {
            out.push_back(ch);
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else {
            out.push_back(ch);
        }
    }
    if (escaped) out.push_back('\\');
    return out;
}

std::unordered_map<std::string, std::string> loadGbastationConfig() {
    std::unordered_map<std::string, std::string> values;
    const std::vector<std::string> paths = {
        "sdmc:/GBAStation/config/config.cfg",
        "./GBAStation/config/config.cfg",
        "GBAStation/config/config.cfg"
    };

    std::ifstream in;
    for (const auto &path: paths) {
        in.open(path);
        if (in.is_open()) break;
        in.clear();
    }
    if (!in.is_open()) return values;

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string encoded = trim(line.substr(eq + 1));
        if (key.empty()) continue;

        if (encoded.rfind("s|", 0) == 0) {
            values[key] = unescapeConfigValue(encoded.substr(2));
        } else {
            values[key] = encoded;
        }
    }

    return values;
}

std::string mappingValue(const std::unordered_map<std::string, std::string> &values,
                         const std::string &key,
                         const std::string &fallback) {
    auto it = values.find(key);
    if (it == values.end() || it->second.empty()) return fallback;
    return it->second;
}

std::vector<std::string> firstComboTokens(const std::string &value) {
    std::vector<std::string> tokens;
    if (value.empty() || value == "none") return tokens;

    const auto firstAltEnd = value.find('|');
    const std::string firstAlt = value.substr(0, firstAltEnd);
    size_t start = 0;
    while (start <= firstAlt.size()) {
        const auto end = firstAlt.find('+', start);
        std::string token = trim(firstAlt.substr(
            start, end == std::string::npos ? std::string::npos : end - start));
        if (!token.empty() && token != "none") tokens.push_back(token);
        if (end == std::string::npos) break;
        start = end + 1;
    }

    return tokens;
}

int tokenToPemuButton(const std::string &token, int fallback) {
    if (token == "PAD_A") return KEY_JOY_A_DEFAULT;
    if (token == "PAD_B") return KEY_JOY_B_DEFAULT;
    if (token == "PAD_X") return KEY_JOY_X_DEFAULT;
    if (token == "PAD_Y") return KEY_JOY_Y_DEFAULT;
    if (token == "PAD_UP") return KEY_JOY_UP_DEFAULT;
    if (token == "PAD_DOWN") return KEY_JOY_DOWN_DEFAULT;
    if (token == "PAD_LEFT") return KEY_JOY_LEFT_DEFAULT;
    if (token == "PAD_RIGHT") return KEY_JOY_RIGHT_DEFAULT;
    if (token == "PAD_LB" || token == "PAD_L") return KEY_JOY_LB_DEFAULT;
    if (token == "PAD_RB" || token == "PAD_R") return KEY_JOY_RB_DEFAULT;
    if (token == "PAD_LT" || token == "PAD_ZL") return KEY_JOY_LT_DEFAULT;
    if (token == "PAD_RT" || token == "PAD_ZR") return KEY_JOY_RT_DEFAULT;
    if (token == "PAD_LSB" || token == "PAD_L3") return KEY_JOY_LS_DEFAULT;
    if (token == "PAD_RSB" || token == "PAD_R3") return KEY_JOY_RS_DEFAULT;
    if (token == "PAD_START") return KEY_JOY_START_DEFAULT;
    if (token == "PAD_BACK" || token == "PAD_SELECT") return KEY_JOY_SELECT_DEFAULT;
    return fallback;
}

void setOptionFromMapping(PEMUConfig *config,
                          const std::unordered_map<std::string, std::string> &values,
                          PEMUConfig::OptId optionId,
                          const std::string &key,
                          const std::string &fallback) {
    auto option = config->get(optionId, true);
    if (!option) return;

    const auto tokens = firstComboTokens(mappingValue(values, key, fallback));
    if (tokens.empty()) {
        option->setInteger(-1);
        return;
    }

    option->setInteger(tokenToPemuButton(tokens.front(), option->getInteger()));
}

void setMenuHotkeyFromMapping(PEMUConfig *config,
                              const std::unordered_map<std::string, std::string> &values) {
    auto menu1 = config->get(PEMUConfig::OptId::JOY_MENU1, true);
    auto menu2 = config->get(PEMUConfig::OptId::JOY_MENU2, true);
    if (!menu1 || !menu2) return;

    const auto tokens = firstComboTokens(
        mappingValue(values, "arcade.hotkey.menu.pad", "PAD_LT+PAD_RT"));
    if (tokens.empty()) {
        menu1->setInteger(KEY_JOY_LT_DEFAULT);
        menu2->setInteger(KEY_JOY_RT_DEFAULT);
        return;
    }

    menu1->setInteger(tokenToPemuButton(tokens[0], menu1->getInteger()));
    menu2->setInteger(tokens.size() > 1 ? tokenToPemuButton(tokens[1], menu2->getInteger()) : -1);
}

void applyGbastationArcadeInputMapping(PEMUConfig *config) {
    if (!config) return;

    const auto values = loadGbastationConfig();
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_UP, "arcade.handle.up", "PAD_UP");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_DOWN, "arcade.handle.down", "PAD_DOWN");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_LEFT, "arcade.handle.left", "PAD_LEFT");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_RIGHT, "arcade.handle.right", "PAD_RIGHT");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_A, "arcade.handle.a", "PAD_A");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_B, "arcade.handle.b", "PAD_B");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_X, "arcade.handle.x", "PAD_X");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_Y, "arcade.handle.y", "PAD_Y");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_LT, "arcade.handle.l", "PAD_LB");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_RT, "arcade.handle.r", "PAD_RB");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_LB, "arcade.handle.l2", "PAD_LT");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_RB, "arcade.handle.r2", "PAD_RT");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_SELECT, "arcade.handle.select", "PAD_BACK");
    setOptionFromMapping(config, values, PEMUConfig::OptId::JOY_START, "arcade.handle.start", "PAD_START");
    setMenuHotkeyFromMapping(config, values);
}

}

PFBAUiEmu::PFBAUiEmu(UiMain *ui) : UiEmu(ui) {
    printf("PFBAUiEmu()\n");
    uiInstance = ui;
}

#ifdef __PFBA_ARM__

int PFBAUiEmu::getSekCpuCore() {
    int sekCpuCore = 0; // SEK_CORE_C68K: USE CYCLONE ARM ASM M68K CORE

    std::vector<std::string> zipList;
    int hardware = BurnDrvGetHardwareCode();

    std::string bios = pMain->getConfig()->get(PEMUConfig::OptId::EMU_NEOBIOS, true)->getString();
    if (isHardware(hardware, HARDWARE_PREFIX_SNK) && Utility::contains(bios, "UNIBIOS")) {
        sekCpuCore = 1; // SEK_CORE_M68K: USE C M68K CORE
        pMain->getUiMessageBox()->show(
                "WARNING", "UNIBIOS DOESNT SUPPORT THE M68K ASM CORE\n"
                           "CYCLONE ASM CORE DISABLED", "OK");
    }

    if (isHardware(hardware, HARDWARE_PREFIX_SEGA_MEGADRIVE)) {
        sekCpuCore = 1; // SEK_CORE_M68K: USE C M68K CORE
    } else if (isHardware(hardware, HARDWARE_PREFIX_SEGA)) {
        if (hardware & HARDWARE_SEGA_FD1089A_ENC
            || hardware & HARDWARE_SEGA_FD1089B_ENC
            || hardware & HARDWARE_SEGA_MC8123_ENC
            || hardware & HARDWARE_SEGA_FD1094_ENC
            || hardware & HARDWARE_SEGA_FD1094_ENC_CPU2) {
            sekCpuCore = 1; // SEK_CORE_M68K: USE C M68K CORE
            pMain->getUiMessageBox()->show(
                    "WARNING", "ROM IS CRYPTED, USE DECRYPTED ROM (CLONE)\n"
                               "TO ENABLE CYCLONE ASM CORE (FASTER)", "OK");
        }
    } else if (isHardware(hardware, HARDWARE_PREFIX_TOAPLAN)) {
        zipList.emplace_back("batrider");
        zipList.emplace_back("bbakraid");
        zipList.emplace_back("bgaregga");
    } else if (isHardware(hardware, HARDWARE_PREFIX_SNK)) {
        zipList.emplace_back("kof97");
        zipList.emplace_back("kof98");
        zipList.emplace_back("kof99");
        zipList.emplace_back("kof2000");
        zipList.emplace_back("kof2001");
        zipList.emplace_back("kof2002");
        zipList.emplace_back("kf2k3pcb");
        //zipList.push_back("kof2003"); // WORKS
    }

    std::string zip = BurnDrvGetTextA(DRV_NAME);
    for (unsigned int i = 0; i < zipList.size(); i++) {
        if (zipList[i].compare(0, zip.length(), zip) == 0) {
            pMain->getUiStatusBox()->show("THIS GAME DOES NOT SUPPORT THE M68K ASM CORE\n"
                                       "CYCLONE ASM CORE DISABLED");
            sekCpuCore = 1; // SEK_CORE_M68K: USE C M68K CORE
            break;
        }
    }

    zipList.clear();

    return sekCpuCore;
}

#endif

int PFBAUiEmu::load(const ss_api::Game &game) {
    currentGame = game;
    BurnPathsSetGame(pMain->getIo(), Utility::removeExt(Utility::baseName(game.path)));
    applyGbastationArcadeInputMapping(pMain->getConfig());

    PFBNEOUtility::setDriverActive(game);
    if (nBurnDrvActive >= nBurnDrvCount) {
        printf("PFBAUiEmu::load: driver not found\n");
        pMain->getUiProgressBox()->setVisibility(Visibility::Hidden);
        pMain->getUiMessageBox()->show("ERROR", "THIS GAME IS NOT SUPPORTED BY FBNEO...", "OK");
        return -1;
    }

#ifdef __PFBA_ARM__
    nSekCpuCore = getSekCpuCore();
    printf("nSekCpuCore: %s\n", nSekCpuCore > 0 ? "M68K" : "C68K (ASM)");
#endif

    int audio_freq = pMain->getConfig()->get(PEMUConfig::OptId::EMU_AUDIO_FREQ, true)->getInteger();
    nInterpolation = pMain->getConfig()->get(PEMUConfig::OptId::EMU_AUDIO_INTERPOLATION, true)->getInteger();
    nFMInterpolation = pMain->getConfig()->get(PEMUConfig::OptId::EMU_AUDIO_FMINTERPOLATION, true)->getInteger();
    bForce60Hz = pMain->getConfig()->get(PEMUConfig::OptId::EMU_FORCE_60HZ, true)->getInteger();
    if (bForce60Hz) {
        nBurnFPS = 6000;
    }

    ///////////////
    // FBA DRIVER
    ///////////////
    EnableHiscores = 1;

    printf("PFBAUiEmu::load: initialize driver...\n");
    // some drivers require audio buffer to be allocated for DrvInit, add a "dummy" one for now...
    auto *aud = new Audio(audio_freq);
    nBurnSoundRate = aud->getSampleRate();
    nBurnSoundLen = aud->getSamples();
    pBurnSoundOut = (INT16 *) malloc(aud->getSamplesSize());
    if (DrvInit((int) nBurnDrvActive, false) != 0) {
        printf("\nPFBAUiEmu::load: driver initialisation failed\n");
        delete (aud);
        pMain->getUiProgressBox()->setVisibility(Visibility::Hidden);
        pMain->getUiMessageBox()->show("ERROR", "DRIVER INIT FAILED", "OK");
        stop();
        return -1;
    }
    delete (aud);
    free(pBurnSoundOut);
    nFramesEmulated = 0;
    nFramesRendered = 0;
    nCurrentFrame = 0;
    ///////////////
    // FBA DRIVER
    ///////////////

    ///////////
    // AUDIO
    //////////
    addAudio(audio_freq, Audio::toSamples(audio_freq, (float) nBurnFPS / 100.0f));
    if (audio->isAvailable()) {
        nBurnSoundRate = audio->getSampleRate();
        nBurnSoundLen = audio->getSamples();
        pBurnSoundOut = (INT16 *) malloc(audio->getSamplesSize());
    }
    audio_sync = !bForce60Hz;
    targetFps = (float) nBurnFPS / 100.0f;
    printf("PFBAUiEmu::load: FORCE_60HZ: %i, AUDIO_SYNC: %i, FPS: %f\n", bForce60Hz, audio_sync, targetFps);
    ///////////
    // AUDIO
    //////////

    //////////
    // VIDEO
    //////////
    Vector2i size, aspect;
    BurnDrvGetFullSize(&size.x, &size.y);
    BurnDrvGetAspect(&aspect.x, &aspect.y);
    nBurnBpp = 2;
    BurnHighCol = myHighCol16;
    BurnRecalcPal();
    // video may already be initialized from fbneo driver (Reinitialise)
    if (!video) {
        auto v = new PFBAVideo(pMain, &pBurnDraw, &nBurnPitch, size, aspect);
        addVideo(v);
        printf("PFBAUiEmu::load: size: %i x %i, aspect: %i x %i, pitch: %i\n",
               size.x, size.y, aspect.x, aspect.y, nBurnPitch);
    } else {
        printf("PFBAUiEmu::load: video already initialized, skipped\n");
    }
    //////////
    // VIDEO
    //////////

    return UiEmu::load(game);
}

// need for some games
void Reinitialise(void) {
    Vector2i size, aspect;
    BurnDrvGetFullSize(&size.x, &size.y);
    BurnDrvGetAspect(&aspect.x, &aspect.y);
    auto v = new PFBAVideo(uiInstance, &pBurnDraw, &nBurnPitch, size, aspect);
    uiInstance->getUiEmu()->addVideo(v);
    printf("PFBAUiEmu::Reinitialise: size: %i x %i, aspect: %i x %i\n",
           size.x, size.y, aspect.x, aspect.y);
}

void PFBAUiEmu::stop() {
    DrvExit();
    if (pBurnSoundOut) {
        free(pBurnSoundOut);
    }
    UiEmu::stop();
}

bool PFBAUiEmu::onInput(c2d::Input::Player *players) {
    if (pMain->getUiMenu()->isVisible() || pMain->getUiStateMenu()->isVisible()) {
        pMain->getInput()->setRotation(Input::Rotation::R0, Input::Rotation::R0);
        return UiEmu::onInput(players);
    }

    // rotation config:
    // 0 > "OFF"
    // 1 > "ON"
    // 2 > "FLIP"
    // 3 > "CAB" (vita/switch)
    int rotation = getUi()->getConfig()->get(PEMUConfig::OptId::EMU_ROTATION, true)->getArrayIndex();
    if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
        if (rotation == 0) {
            pMain->getInput()->setRotation(Input::Rotation::R90, Input::Rotation::R0);
        } else if (rotation == 1) {
            pMain->getInput()->setRotation(Input::Rotation::R0, Input::Rotation::R0);
        } else if (rotation == 2) {
            pMain->getInput()->setRotation(Input::Rotation::R270, Input::Rotation::R0);
        } else {
            pMain->getInput()->setRotation(Input::Rotation::R270, Input::Rotation::R270);
        }
    }

    return UiEmu::onInput(players);
}

void PFBAUiEmu::onUpdate() {
    if (isPaused()) {
        return;
    }

    // update fbneo inputs
    InputMake(true);

    // GBAStation handles Arcade menus and hotkeys from the frontend. Do not
    // let FBNeo long-press Start/Select enter its original service/reset menu.
    clock.restart();

    // update fbneo video buffer and audio
#ifdef __VITA__
    int skip = pMain->getConfig()->get(PEMUConfig::OptId::EMU_FRAMESKIP, true)->getInteger();
#else
    int skip = 0;
#endif

    pBurnDraw = nullptr;
    frameskip++;

    if (frameskip > skip) {
        video->getTexture()->lock(&pBurnDraw, &nBurnPitch);
        nFramesRendered++;
    }

    BurnDrvFrame();
    nCurrentFrame++;

    if (frameskip > skip) {
        video->getTexture()->unlock();
        frameskip = 0;
    }

    if (audio) {
#if 0
        int queued = audio->getSampleBufferQueued();
        int capacity = audio->getSampleBufferCapacity();
        if (audio->getSamples() + queued > capacity) {
            printf("WARNING: samples: %i, queued: %i, capacity: %i (fps: %f)\n",
                   audio->getSamples(), queued, capacity, targetFps);
        }
#endif
        audio->play(pBurnSoundOut, audio->getSamples(),
                    audio_sync ? Audio::SyncMode::LowLatency : Audio::SyncMode::None);
    }

    UiEmu::onUpdate();
}
