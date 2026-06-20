#include "emulator/melonds/MelonDSCore.h"

#include "Args.h"
#include "GPU3D_Soft.h"
#include "MemConstants.h"
#include "NDS.h"
#include "NDSCart.h"
#include "SPI_Firmware.h"
#include "Savestate.h"
#include "core/GameSignal.hpp"
#include "core/Tools.hpp"

#include <borealis.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>

namespace beiklive::melonds {

namespace {

std::string joinPath(const std::string& dir, const std::string& name)
{
    return (std::filesystem::path(dir) / name).string();
}

bool readWholeFile(const std::string& path, std::vector<uint8_t>& out)
{
    return LoadBinaryVector(path, out);
}

} // namespace

MelonDSCore::MelonDSCore() = default;

MelonDSCore::~MelonDSCore()
{
    Cleanup();
}

bool MelonDSCore::SetupGame(beiklive::GameEntry GameEntry)
{
    m_gameEntry = std::move(GameEntry);
    m_stopRequested.store(false, std::memory_order_release);
    std::filesystem::create_directories(defaultSaveDir());
    std::string saveDir = m_gameEntry.savePath.empty() ? defaultSaveDir() : m_gameEntry.savePath;
    std::filesystem::create_directories(saveDir);
    m_saveFile = joinPath(saveDir, beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".sav");
    m_stateFile = joinPath(saveDir, beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".state");
    m_platformData.savePath = m_saveFile;

    if (!Initialize())
        return false;
    if (!LoadGame(m_gameEntry.path))
        return false;

    m_ready.store(true, std::memory_order_release);
    brls::Logger::info("melonDS: ROM loaded: {} ({}x{} @ {:.2f} fps)",
                       m_gameEntry.path, GameWidth(), GameHeight(), Fps());
    return true;
}

void MelonDSCore::Cleanup()
{
    if (m_ready.load(std::memory_order_acquire))
        saveSram();

    Stop();
    m_nds.reset();
    m_romData.clear();
    m_audio.Reset();
    m_input.Reset();
    m_video.Reset();
    m_ready.store(false, std::memory_order_release);
    m_initialized.store(false, std::memory_order_release);
}

bool MelonDSCore::Initialize()
{
    m_audio.Reset();
    m_input.Reset();
    m_video.Reset();

    melonDS::NDSArgs args;
#ifdef __SWITCH__
    melonDS::JITArgs jitArgs;
    jitArgs.FastMemory = false;
    args.JIT = jitArgs;
#else
    args.JIT = std::nullopt;
#endif
    args.OutputSampleRate = 48000.0;
    args.Renderer3D = std::make_unique<melonDS::SoftRenderer>(true);

    if (!loadBiosFiles(args))
        return false;

    m_nds = std::make_unique<melonDS::NDS>(std::move(args), &m_platformData);
    if (!m_nds)
        return false;

#ifdef __SWITCH__
    if (!m_nds->IsJITEnabled())
    {
        brls::Logger::warning("melonDS: ARM64 JIT requested but not enabled; refusing silent fallback");
        m_nds.reset();
        return false;
    }
    brls::Logger::info("ARM64 JIT enabled");
    brls::Logger::info("melonDS: JIT FastMemory disabled on Switch");
#else
    brls::Logger::info("melonDS: JIT disabled on desktop build; using interpreter");
#endif

    const auto& soft = static_cast<const melonDS::SoftRenderer&>(m_nds->GPU.GetRenderer3D());
    if (!soft.IsThreaded())
        brls::Logger::warning("melonDS: Threaded Renderer requested but not enabled");
    else
        brls::Logger::info("melonDS: Threaded Renderer enabled");

    m_initialized.store(true, std::memory_order_release);
    return true;
}

bool MelonDSCore::LoadGame(const std::string& path)
{
    if (!m_nds)
        return false;
    if (path.empty())
    {
        brls::Logger::error("melonDS: ROM path is empty");
        return false;
    }

    std::unique_ptr<uint8_t[]> romData;
    std::uintmax_t fileSize = 0;
    {
        std::error_code ec;
        fileSize = std::filesystem::file_size(path, ec);
        if (ec || fileSize == 0 || fileSize > std::numeric_limits<melonDS::u32>::max())
        {
            brls::Logger::error("melonDS: invalid ROM size: {}", path);
            return false;
        }
    }

    std::ifstream rom(path, std::ios::binary);
    if (!rom)
    {
        brls::Logger::error("melonDS: failed to open ROM: {}", path);
        return false;
    }

    romData = std::make_unique<uint8_t[]>(static_cast<size_t>(fileSize));
    rom.read(reinterpret_cast<char*>(romData.get()), static_cast<std::streamsize>(fileSize));
    if (rom.gcount() != static_cast<std::streamsize>(fileSize))
    {
        brls::Logger::error("melonDS: failed to read ROM: {}", path);
        return false;
    }

    melonDS::NDSCart::NDSCartArgs cartArgs;
    loadBatterySave(cartArgs);

    brls::Logger::debug("melonDS: parsing ROM");
    auto cart = melonDS::NDSCart::ParseROM(std::move(romData),
                                           static_cast<melonDS::u32>(fileSize),
                                           &m_platformData,
                                           std::move(cartArgs));
    if (!cart)
    {
        brls::Logger::error("melonDS: failed to parse NDS ROM: {}", path);
        return false;
    }

    if (m_stopRequested.load(std::memory_order_acquire))
        return false;

    brls::Logger::debug("melonDS: SetNDSCart begin");
    m_nds->SetNDSCart(std::move(cart));
    brls::Logger::debug("melonDS: SetNDSCart end");
    if (m_stopRequested.load(std::memory_order_acquire))
        return false;

    brls::Logger::debug("melonDS: Reset begin");
    m_nds->Reset();
    brls::Logger::debug("melonDS: Reset end");
    if (m_stopRequested.load(std::memory_order_acquire))
        return false;

    brls::Logger::debug("melonDS: SetupDirectBoot begin");
    m_nds->SetupDirectBoot(std::filesystem::path(path).filename().string());
    brls::Logger::debug("melonDS: SetupDirectBoot end");
    if (m_stopRequested.load(std::memory_order_acquire))
        return false;

    brls::Logger::debug("melonDS: GPU.StartFrame begin");
    m_nds->GPU.StartFrame();
    brls::Logger::debug("melonDS: GPU.StartFrame end");
    brls::Logger::debug("melonDS: Start begin");
    m_nds->Start();
    brls::Logger::debug("melonDS: Start end");
    return true;
}

void MelonDSCore::RunFrame()
{
    if (!m_ready.load(std::memory_order_acquire) || m_paused.load(std::memory_order_acquire) || !m_nds)
        return;

    m_input.Apply(*m_nds);
    const auto frameStart = std::chrono::steady_clock::now();
    const auto scanlines = m_nds->RunFrame();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - frameStart).count();
    if (elapsedMs > 1000)
        brls::Logger::warning("melonDS: RunFrame took {} ms (scanlines={})", elapsedMs, scanlines);

    std::array<int16_t, 4096> temp {};
    int available = m_nds->SPU.GetOutputSize();
    while (available > 0)
    {
        const int toRead = std::min<int>(available, static_cast<int>(temp.size() / 2));
        const int read = m_nds->SPU.ReadOutput(temp.data(), toRead);
        if (read <= 0)
            break;
        m_audio.Push(temp.data(), static_cast<size_t>(read) * 2);
        available = m_nds->SPU.GetOutputSize();
    }

    m_video.Capture(*m_nds);
}

void MelonDSCore::Reset()
{
    if (!m_ready.load(std::memory_order_acquire) || !m_nds)
        return;
    m_nds->Reset();
    m_nds->SetupDirectBoot(std::filesystem::path(m_gameEntry.path).filename().string());
    m_nds->GPU.StartFrame();
    m_nds->Start();
}

void MelonDSCore::Stop()
{
    if (m_nds && m_nds->IsRunning())
        m_nds->Stop();
    m_paused.store(false, std::memory_order_release);
}

void MelonDSCore::RequestStop()
{
    m_stopRequested.store(true, std::memory_order_release);
    if (m_nds && m_nds->IsRunning())
        m_nds->Halt();
    m_paused.store(false, std::memory_order_release);
}

void MelonDSCore::Pause(bool paused)
{
    m_paused.store(paused, std::memory_order_release);
}

bool MelonDSCore::Serialize(std::vector<uint8_t>& outBuf) const
{
    if (!m_ready.load(std::memory_order_acquire) || !m_nds)
        return false;

    melonDS::Savestate state;
    if (!m_nds->DoSavestate(&state) || state.Error)
        return false;
    state.Finish();
    if (state.Error || state.Length() == 0)
        return false;

    const auto* data = static_cast<const uint8_t*>(state.Buffer());
    outBuf.assign(data, data + state.Length());
    return true;
}

bool MelonDSCore::Unserialize(const std::vector<uint8_t>& buf)
{
    if (!m_ready.load(std::memory_order_acquire) || !m_nds || buf.empty() || buf.size() > std::numeric_limits<melonDS::u32>::max())
        return false;

    melonDS::Savestate state(const_cast<uint8_t*>(buf.data()), static_cast<melonDS::u32>(buf.size()), false);
    if (state.Error)
        return false;
    return m_nds->DoSavestate(&state) && !state.Error;
}

bool MelonDSCore::SaveState(const std::string& path)
{
    std::vector<uint8_t> data;
    if (!Serialize(data))
        return false;
    return WriteBinaryFile(path.empty() ? m_stateFile : path, data.data(), data.size());
}

bool MelonDSCore::LoadState(const std::string& path)
{
    std::vector<uint8_t> data;
    if (!readWholeFile(path.empty() ? m_stateFile : path, data))
        return false;
    return Unserialize(data);
}

void MelonDSCore::SetButtonsFromSignal()
{
    m_input.SetButtonsFromMask(GameSignal::instance().getGameButtonMask());
}

void MelonDSCore::SetButton(int key, bool pressed)
{
    if (key >= 0)
        m_input.SetButton(static_cast<unsigned>(key), pressed);
}

void MelonDSCore::SetTouch(int x, int y, bool down)
{
    m_input.SetTouch(x, y, down);
}

void MelonDSCore::ToggleCheat(int idx, bool enabled)
{
    if (idx < 0 || idx >= static_cast<int>(m_cheats.size()))
        return;
    m_cheats[static_cast<size_t>(idx)].enabled = enabled;
}

const void* MelonDSCore::getSramData() const
{
    return m_nds ? m_nds->GetNDSSave() : nullptr;
}

size_t MelonDSCore::getSramSize() const
{
    return m_nds ? static_cast<size_t>(m_nds->GetNDSSaveLength()) : 0;
}

bool MelonDSCore::saveSram()
{
    const void* data = getSramData();
    const size_t size = getSramSize();
    if (!data || size == 0)
        return true;
    return WriteBinaryFile(m_saveFile, data, size);
}

bool MelonDSCore::loadBiosFiles(melonDS::NDSArgs& args)
{
    m_biosDir = defaultBiosDir();
    m_platformData.firmwarePath = joinPath(m_biosDir, "firmware.bin");

    auto arm9 = std::make_unique<melonDS::ARM9BIOSImage>();
    auto arm7 = std::make_unique<melonDS::ARM7BIOSImage>();
    const std::string bios9 = joinPath(m_biosDir, "bios9.bin");
    const std::string bios7 = joinPath(m_biosDir, "bios7.bin");
    const std::string firmwarePath = joinPath(m_biosDir, "firmware.bin");

    if (!LoadBinaryFile(bios9, arm9->data(), arm9->size()))
    {
        brls::Logger::error("melonDS: missing or invalid BIOS: {}", bios9);
        return false;
    }
    if (!LoadBinaryFile(bios7, arm7->data(), arm7->size()))
    {
        brls::Logger::error("melonDS: missing or invalid BIOS: {}", bios7);
        return false;
    }

    std::vector<uint8_t> firmwareData;
    if (!LoadBinaryVector(firmwarePath, firmwareData))
    {
        brls::Logger::error("melonDS: missing firmware: {}", firmwarePath);
        return false;
    }

    args.ARM9BIOS = std::move(arm9);
    args.ARM7BIOS = std::move(arm7);
    args.Firmware = melonDS::Firmware(firmwareData.data(), static_cast<melonDS::u32>(firmwareData.size()));
    return true;
}

bool MelonDSCore::loadBatterySave(melonDS::NDSCart::NDSCartArgs& args) const
{
    std::vector<uint8_t> data;
    if (!LoadBinaryVector(m_saveFile, data) || data.empty())
        return false;

    args.SRAMLength = static_cast<melonDS::u32>(data.size());
    args.SRAM = std::make_unique<melonDS::u8[]>(args.SRAMLength);
    std::memcpy(args.SRAM.get(), data.data(), data.size());
    return true;
}

std::string MelonDSCore::defaultSaveDir() const
{
    return beiklive::tools::defaultGameSavePath(
        static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS),
        m_gameEntry.path);
}

std::string MelonDSCore::defaultBiosDir() const
{
#ifdef __SWITCH__
    return "sdmc:/GBAStation/bios/nds";
#else
    return (std::filesystem::path(beiklive::path::biosPath()) / "nds").string();
#endif
}

} // namespace beiklive::melonds
