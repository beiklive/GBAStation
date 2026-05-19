#include "CoreMelonDS.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>

#include "third_party/mgba/src/platform/libretro/libretro.h"

#include "NDS.h"
#include "NDSCart.h"
#include "SPU.h"
#include "GPU.h"
#include "Args.h"
#include "types.h"
#include "FreeBIOS.h"
#include "SPI_Firmware.h"
#include "RTC.h"
#include "SPI.h"

using namespace melonDS;

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

static size_t fileSize(const std::string& path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    return f ? static_cast<size_t>(f.tellg()) : 0;
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

    if (!_loadROM(m_gameEntry.path))
        return false;

    _loadBIOS();
    _loadFirmware();

    std::vector<u8> saveBuf;
    {
        std::string path = m_gameEntry.savePath + beiklive::path::SPLIT_CHAR
            + beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".sav";
        saveBuf = readFile(path);
    }

#ifdef JIT_ENABLED
    melonDS::JITArgs jitargs {
        .MaxBlockSize = 32,
        .LiteralOptimizations = true,
        .BranchOptimizations = true,
        .FastMemory = true,
    };
#endif

    auto cart = melonDS::NDSCart::ParseROM(std::move(m_romData), m_romLen);
    if (!cart)
    {
        brls::Logger::error("CoreMelonDS: failed to parse ROM");
        return false;
    }

    if (!saveBuf.empty())
        cart->SetSaveMemory(saveBuf.data(), static_cast<u32>(saveBuf.size()));

    melonDS::NDSArgs ndsargs {
        .NDSROM = std::move(cart),
        .ARM9BIOS = m_arm9bios,
        .ARM7BIOS = m_arm7bios,
        .Firmware = std::move(m_firmware),
#ifdef JIT_ENABLED
        .JIT = jitargs,
#else
        .JIT = std::nullopt,
#endif
    };

    m_nds = std::make_unique<melonDS::NDS>(std::move(ndsargs));

    m_nds->Reset();

    melonDS::NDS::Current = m_nds.get();

    // 强制 DirectBoot 跳过 BIOS 引导动画，直接启动游戏
    {
        std::string romName = beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path);
        m_nds->SetupDirectBoot(romName);
        brls::Logger::info("CoreMelonDS: direct boot enabled for {}", romName);
    }

    melonDS::Platform::SetStopCallback([this]() {
        m_ready = false;
    });

    m_nds->Start();

    m_nds->SPI.GetPowerMan()->SetBatteryLevelOkay(true);

    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm* tm = std::localtime(&t);
        m_nds->RTC.SetDateTime(
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);
    }

    melonDS::Platform::SetNDSSavePath(m_gameEntry.savePath + beiklive::path::SPLIT_CHAR
        + beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".sav");
    melonDS::Platform::SetFirmwarePath(beiklive::path::biosPath() + beiklive::path::SPLIT_CHAR + "firmware.bin");

    m_keyMask = 0x03FF03FF;
    m_touchDown = false;
    m_ready = true;

    brls::Logger::info("CoreMelonDS: NDS initialized for: {}", m_gameEntry.title);
    return true;
}

void CoreMelonDS::Cleanup()
{
    if (!m_ready) return;
    m_ready = false;

    SaveNDSSave();
    m_nds.reset();
}

void CoreMelonDS::RunFrame()
{
    if (!m_ready) return;
    m_nds->RunFrame();
}

void CoreMelonDS::Reset()
{
    if (!m_ready) return;
    Cleanup();
    beiklive::GameEntry entry = m_gameEntry;
    m_gameEntry = {};
    SetupGame(std::move(entry));
}

const uint32_t* CoreMelonDS::GetTopFramebuffer() const
{
    if (!m_ready) return nullptr;
    int fb = m_nds->GPU.FrontBuffer;
    return m_nds->GPU.Framebuffer[0][fb].get();
}

const uint32_t* CoreMelonDS::GetBottomFramebuffer() const
{
    if (!m_ready) return nullptr;
    int fb = m_nds->GPU.FrontBuffer;
    return m_nds->GPU.Framebuffer[1][fb].get();
}

int CoreMelonDS::ReadAudio(int16_t* data, int samples)
{
    if (!m_ready) return 0;
    return m_nds->SPU.ReadOutput(data, samples);
}

void CoreMelonDS::SetButtonState(unsigned id, bool pressed)
{
    if (!m_ready || id >= 16) return;

    unsigned bit = k_retroToDS[id];
    if (pressed)
        m_keyMask &= ~(1u << bit);
    else
        m_keyMask |= (1u << bit);

    m_nds->SetKeyMask(m_keyMask);
}

void CoreMelonDS::SetButtonsFromSignal()
{
    if (!m_ready) return;
    uint32_t mask = GameSignal::instance().getGameButtonMask();
    for (unsigned i = 0; i < 16; ++i)
        SetButtonState(i, (mask >> i) & 1u);
}

void CoreMelonDS::TouchScreen(uint16_t x, uint16_t y)
{
    if (!m_ready) return;
    m_touchDown = true;
    m_nds->TouchScreen(x, y);
}

void CoreMelonDS::ReleaseScreen()
{
    if (!m_ready) return;
    m_touchDown = false;
    m_nds->ReleaseScreen();
}

void CoreMelonDS::SaveNDSSave()
{
    if (!m_ready) return;

    const u8* savedata = m_nds->GetNDSSave();
    u32 savelen = m_nds->GetNDSSaveLength();
    if (!savedata || savelen == 0) return;

    std::string path = m_gameEntry.savePath + beiklive::path::SPLIT_CHAR
        + beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".sav";

    std::ofstream f(path, std::ios::binary);
    if (!f) return;
    f.write(reinterpret_cast<const char*>(savedata), savelen);
}

void CoreMelonDS::LoadNDSSave()
{
    if (!m_ready) return;

    u32 savelen = m_nds->GetNDSSaveLength();
    if (savelen == 0) return;

    std::string path = m_gameEntry.savePath + beiklive::path::SPLIT_CHAR
        + beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".sav";

    std::ifstream f(path, std::ios::binary);
    if (!f) return;

    std::vector<u8> buf(savelen, 0);
    f.read(reinterpret_cast<char*>(buf.data()), savelen);
    m_nds->SetNDSSave(buf.data(), static_cast<u32>(f.gcount()));
}

void CoreMelonDS::ApplyCheats(const std::vector<CheatEntry>& cheats)
{
    m_cheats = cheats;
}

bool CoreMelonDS::_loadROM(const std::string& romPath)
{
    if (romPath.empty())
    {
        brls::Logger::error("CoreMelonDS: ROM path is empty");
        return false;
    }

    std::vector<u8> data = readFile(romPath);
    if (data.empty())
    {
        brls::Logger::error("CoreMelonDS: failed to open ROM: {}", romPath);
        return false;
    }

    m_romLen = static_cast<u32>(data.size());
    m_romData = std::make_unique<u8[]>(m_romLen);
    std::memcpy(m_romData.get(), data.data(), m_romLen);

    brls::Logger::info("CoreMelonDS: ROM loaded {} ({} bytes)", romPath, m_romLen);
    return true;
}

bool CoreMelonDS::_loadBIOS()
{
    std::string biosPath = beiklive::path::biosPath();

    static const char* arm9Names[] = {"biosnds9.bin", "bios9.bin", "biosnds9.rom", "nds_bios_arm9.bin"};
    static const char* arm7Names[] = {"biosnds7.bin", "bios7.bin", "biosnds7.rom", "nds_bios_arm7.bin"};

    auto tryLoad = [](const std::string& dir, const char** names, int count, u8* out, size_t expectedSize) -> bool {
        for (int i = 0; i < count; ++i)
        {
            std::string path = dir + beiklive::path::SPLIT_CHAR + names[i];
            std::ifstream f(path, std::ios::binary);
            if (!f) continue;
            f.seekg(0, std::ios::end);
            size_t sz = static_cast<size_t>(f.tellg());
            f.seekg(0, std::ios::beg);
            if (sz < expectedSize) continue;
            f.read(reinterpret_cast<char*>(out), expectedSize);
            brls::Logger::info("CoreMelonDS: loaded BIOS from {}", path);
            return true;
        }
        return false;
    };

    bool arm9ok = tryLoad(biosPath, arm9Names, 4, m_arm9bios.data(), m_arm9bios.size());
    bool arm7ok = tryLoad(biosPath, arm7Names, 4, m_arm7bios.data(), m_arm7bios.size());

    if (!arm9ok)
        brls::Logger::warning("CoreMelonDS: ARM9 BIOS not found, using FreeBIOS");
    if (!arm7ok)
        brls::Logger::warning("CoreMelonDS: ARM7 BIOS not found, using FreeBIOS");

    return true;
}

bool CoreMelonDS::_loadFirmware()
{
    std::string fwPath = beiklive::path::biosPath() + beiklive::path::SPLIT_CHAR + "firmware.bin";

    std::vector<u8> fwData = readFile(fwPath);
    if (!fwData.empty())
    {
        m_firmware = melonDS::Firmware(fwData.data(), static_cast<u32>(fwData.size()));
        brls::Logger::info("CoreMelonDS: loaded firmware from {}", fwPath);
    }
    else
    {
        m_firmware = melonDS::Firmware(0);
        brls::Logger::info("CoreMelonDS: using generated NDS firmware");
    }

    return true;
}

} // namespace beiklive::melonds
