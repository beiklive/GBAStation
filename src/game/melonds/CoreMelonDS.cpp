#include "CoreMelonDS.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <chrono>

#include "third_party/mgba/src/platform/libretro/libretro.h"

#include "NDS.h"
#include "SPU.h"
#include "GPU.h"
#include "types.h"
#include "RTC.h"
#include "SPI.h"
#include "Platform.h"
#include "Config.h"

namespace beiklive::melonds
{

static constexpr unsigned k_retroToDS[16] = {
    [RETRO_DEVICE_ID_JOYPAD_B]      = 1,
    [RETRO_DEVICE_ID_JOYPAD_Y]      = 17,
    [RETRO_DEVICE_ID_JOYPAD_SELECT] = 2,
    [RETRO_DEVICE_ID_JOYPAD_START]  = 3,
    [RETRO_DEVICE_ID_JOYPAD_UP]     = 6,
    [RETRO_DEVICE_ID_JOYPAD_DOWN]   = 7,
    [RETRO_DEVICE_ID_JOYPAD_LEFT]   = 5,
    [RETRO_DEVICE_ID_JOYPAD_RIGHT]  = 4,
    [RETRO_DEVICE_ID_JOYPAD_A]      = 0,
    [RETRO_DEVICE_ID_JOYPAD_X]      = 16,
    [RETRO_DEVICE_ID_JOYPAD_L]      = 9,
    [RETRO_DEVICE_ID_JOYPAD_R]      = 8,
};

static std::vector<u8> readFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    size_t size = static_cast<size_t>(f.tellg());
    f.seekg(0, std::ios::beg);
    std::vector<u8> data(size);
    f.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

CoreMelonDS::CoreMelonDS()
{
}

CoreMelonDS::~CoreMelonDS()
{
    if (m_ready)
        Cleanup();
}

bool CoreMelonDS::SetupGame(beiklive::GameEntry entry)
{
    m_gameEntry = std::move(entry);

    if (!_loadROMFile(m_gameEntry.path))
        return false;

    std::string biosDir = beiklive::path::biosPath();
    std::string firmwarePath = biosDir + beiklive::path::SPLIT_CHAR + "firmware.bin";
    std::string savePath = m_gameEntry.savePath + beiklive::path::SPLIT_CHAR
        + beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".sav";

    strncpy(Config::FirmwarePath, firmwarePath.c_str(), sizeof(Config::FirmwarePath) - 1);
    strncpy(Config::BIOS9Path, (biosDir + beiklive::path::SPLIT_CHAR + "bios9.bin").c_str(), sizeof(Config::BIOS9Path) - 1);
    strncpy(Config::BIOS7Path, (biosDir + beiklive::path::SPLIT_CHAR + "bios7.bin").c_str(), sizeof(Config::BIOS7Path) - 1);

    if (!m_instance.Init())
    {
        brls::Logger::error("CoreMelonDS: MelonDSInstance::Init() failed");
        return false;
    }

    if (!m_instance.LoadROM(m_romData.data(), static_cast<u32>(m_romData.size()),
                              savePath.c_str()))
    {
        brls::Logger::error("CoreMelonDS: MelonDSInstance::LoadROM() failed");
        return false;
    }

    brls::Logger::info("CoreMelonDS: LoadROM done, setting stop callback");

    m_instance.SetStopCallback([this]() {
        m_ready = false;
    });

    brls::Logger::info("CoreMelonDS: starting instance");

    m_instance.Start();

    brls::Logger::info("CoreMelonDS: creating sub-components");

    m_video = std::make_unique<MelonDSVideo>(m_instance);
    brls::Logger::info("CoreMelonDS: video created");
    m_composer = std::make_unique<FrameComposer>();
    brls::Logger::info("CoreMelonDS: composer created");
    m_audio = std::make_unique<MelonDSAudio>(m_instance);
    brls::Logger::info("CoreMelonDS: audio created");
    m_input = std::make_unique<MelonDSInput>(m_instance);
    brls::Logger::info("CoreMelonDS: input created");
    m_save = std::make_unique<MelonDSSave>(m_instance);
    brls::Logger::info("CoreMelonDS: save created");

    std::string saveDir = m_gameEntry.savePath + beiklive::path::SPLIT_CHAR;
    std::string romName = beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path);
    m_save->setSavePath(saveDir + romName + ".sav", romName);
    m_save->setFirmwarePath(firmwarePath);

    m_ready = true;

    brls::Logger::info("CoreMelonDS: initialized for: {}", m_gameEntry.title);
    return true;
}

void CoreMelonDS::Cleanup()
{
    if (!m_ready) return;
    m_ready = false;

    SaveNDSSave();
    m_instance.Shutdown();

    m_video.reset();
    m_composer.reset();
    m_audio.reset();
    m_input.reset();
    m_save.reset();
}

bool CoreMelonDS::LoadRom(const std::string& path)
{
    beiklive::GameEntry entry;
    entry.path = path;
    entry.title = path;
    entry.platform = static_cast<int>(beiklive::enums::EmuPlatform::EmuDS);
    return SetupGame(std::move(entry));
}

void CoreMelonDS::Reset()
{
    if (!m_ready) return;
    Cleanup();
    beiklive::GameEntry entry = m_gameEntry;
    m_gameEntry = {};
    SetupGame(std::move(entry));
}

void CoreMelonDS::RunFrame()
{
    if (!m_ready) return;
    m_instance.RunFrame();
}

void CoreMelonDS::Stop()
{
    Cleanup();
}

bool CoreMelonDS::IsRunning() const
{
    return m_ready;
}

const uint32_t* CoreMelonDS::GetTopScreen()
{
    if (!m_video) return nullptr;
    return m_video->topBuffer();
}

const uint32_t* CoreMelonDS::GetBottomScreen()
{
    if (!m_video) return nullptr;
    return m_video->bottomBuffer();
}

int CoreMelonDS::GetScreenWidth() const
{
    return 256;
}

int CoreMelonDS::GetScreenHeight() const
{
    return 192 * 2;
}

int CoreMelonDS::GetTopScreenHeight() const
{
    return 192;
}

int CoreMelonDS::GetBottomScreenHeight() const
{
    return 192;
}

void CoreMelonDS::PushInput(int key, bool pressed)
{
    if (!m_input) return;
    m_input->setButtonState(key, pressed);
}

void CoreMelonDS::SaveState(const std::string& path)
{
    if (!m_save) return;
    m_save->saveState(path);
}

void CoreMelonDS::LoadState(const std::string& path)
{
    if (!m_save) return;
    m_save->loadState(path);
}

const uint32_t* CoreMelonDS::GetTopFramebuffer() const
{
    if (!m_video) return nullptr;
    return m_video->topBuffer();
}

const uint32_t* CoreMelonDS::GetBottomFramebuffer() const
{
    if (!m_video) return nullptr;
    return m_video->bottomBuffer();
}

int CoreMelonDS::ReadAudio(int16_t* data, int samples)
{
    if (!m_audio) return 0;
    return m_audio->readSamples(data, samples);
}

void CoreMelonDS::SetButtonState(unsigned id, bool pressed)
{
    if (!m_input) return;
    m_input->setButtonState(id, pressed);
}

void CoreMelonDS::SetButtonsFromSignal()
{
    if (!m_ready || !m_input) return;
    uint32_t mask = GameSignal::instance().getGameButtonMask();
    for (unsigned i = 0; i < 16; ++i)
        m_input->setButtonState(i, (mask >> i) & 1u);
}

void CoreMelonDS::TouchScreen(uint16_t x, uint16_t y)
{
    if (!m_input) return;
    m_input->touchScreen(x, y);
}

void CoreMelonDS::ReleaseScreen()
{
    if (!m_input) return;
    m_input->releaseTouch();
}

void CoreMelonDS::SaveNDSSave()
{
    if (!m_ready) return;
    m_instance.FlushSave();
}

void CoreMelonDS::LoadNDSSave()
{
}

void CoreMelonDS::ApplyCheats(const std::vector<CheatEntry>& cheats)
{
    m_cheats = cheats;
}

bool CoreMelonDS::_loadROMFile(const std::string& romPath)
{
    if (romPath.empty())
    {
        brls::Logger::error("CoreMelonDS: ROM path is empty");
        return false;
    }

    m_romData = readFile(romPath);
    if (m_romData.empty())
    {
        brls::Logger::error("CoreMelonDS: failed to open ROM: {}", romPath);
        return false;
    }

    brls::Logger::info("CoreMelonDS: ROM loaded {} ({} bytes)", romPath, m_romData.size());
    return true;
}

} // namespace beiklive::melonds
