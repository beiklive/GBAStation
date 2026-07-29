#include "GenesisCore.h"

#include "core/Tools.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <utility>

extern "C"
{
#include "shared.h"
void gpgx_configure_defaults(void);
}

namespace beiklive::genesis
{
namespace
{
constexpr unsigned kRetroB = 0;
constexpr unsigned kRetroY = 1;
constexpr unsigned kRetroSelect = 2;
constexpr unsigned kRetroStart = 3;
constexpr unsigned kRetroUp = 4;
constexpr unsigned kRetroDown = 5;
constexpr unsigned kRetroLeft = 6;
constexpr unsigned kRetroRight = 7;
constexpr unsigned kRetroA = 8;
constexpr unsigned kRetroX = 9;
constexpr unsigned kRetroL = 10;
constexpr unsigned kRetroR = 11;

uint32_t rgb565ToRgba(uint16_t pixel)
{
    const uint8_t r5 = static_cast<uint8_t>((pixel >> 11) & 0x1F);
    const uint8_t g6 = static_cast<uint8_t>((pixel >> 5) & 0x3F);
    const uint8_t b5 = static_cast<uint8_t>(pixel & 0x1F);
    const uint8_t r = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
    const uint8_t g = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
    const uint8_t b = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
    return static_cast<uint32_t>(r) |
           (static_cast<uint32_t>(g) << 8) |
           (static_cast<uint32_t>(b) << 16) |
           0xFF000000u;
}
}

GenesisCore::~GenesisCore()
{
    Cleanup();
}

bool GenesisCore::SetupGame(beiklive::GameEntry gameEntry)
{
    Cleanup();
    m_gameEntry = std::move(gameEntry);

    if (m_gameEntry.platform != static_cast<int>(beiklive::enums::EmuPlatform::EmuGenesis))
    {
        brls::Logger::error("GenesisCore: rejected platform={}", m_gameEntry.platform);
        return false;
    }

    if (m_gameEntry.path.empty() || !std::filesystem::exists(m_gameEntry.path))
    {
        brls::Logger::error("GenesisCore: ROM not found: {}", m_gameEntry.path);
        return false;
    }

    m_bitmapStorage.assign(static_cast<size_t>(kBitmapWidth) * kBitmapHeight, 0);
    std::memset(&bitmap, 0, sizeof(bitmap));
    bitmap.width = static_cast<int>(kBitmapWidth);
    bitmap.height = static_cast<int>(kBitmapHeight);
    bitmap.pitch = static_cast<int>(kBitmapWidth * sizeof(uint16_t));
    bitmap.data = reinterpret_cast<uint8*>(m_bitmapStorage.data());

    gpgx_configure_defaults();
    system_hw = 0;

    std::vector<char> mutablePath(m_gameEntry.path.begin(), m_gameEntry.path.end());
    mutablePath.push_back('\0');
    if (load_rom(mutablePath.data()) <= 0 || system_hw != SYSTEM_MD)
    {
        brls::Logger::error("GenesisCore: failed to load MD ROM: {}", m_gameEntry.path);
        clearRuntimeState();
        return false;
    }

    if (audio_init(static_cast<int>(kSampleRate), 0.0) != 0)
    {
        brls::Logger::error("GenesisCore: audio initialization failed");
        clearRuntimeState();
        return false;
    }
    m_audioInitialized = true;

    system_init();
    system_reset();
    loadSram();

    m_width = bitmap.viewport.w > 0 ? static_cast<unsigned>(bitmap.viewport.w) : 320u;
    m_height = bitmap.viewport.h > 0 ? static_cast<unsigned>(bitmap.viewport.h) : 224u;
    const double masterClock = vdp_pal ? static_cast<double>(MCLOCK_PAL) : static_cast<double>(MCLOCK_NTSC);
    const double lines = vdp_pal ? 313.0 : 262.0;
    m_fps = masterClock / (static_cast<double>(MCYCLES_PER_LINE) * lines);
    m_ready = true;
    captureVideoFrame();

    brls::Logger::info("GenesisCore: ROM loaded: {} ({}x{} @ {:.2f} fps)",
                       m_gameEntry.path, m_width, m_height, m_fps);
    return true;
}

void GenesisCore::Cleanup()
{
    if (m_ready)
        saveSram();
    m_ready = false;

    if (m_audioInitialized)
    {
        audio_shutdown();
        m_audioInitialized = false;
    }

    system_hw = 0;
    clearRuntimeState();
}

void GenesisCore::RunFrame()
{
    if (!m_ready)
        return;

    updateInput();
    system_frame_gen(0);

    std::array<int16_t, 4096 * 2> frameAudio{};
    const int frames = audio_update(frameAudio.data());
    if (frames > 0)
    {
        const size_t sampleCount = std::min(frameAudio.size(), static_cast<size_t>(frames) * 2);
        std::lock_guard<std::mutex> lock(m_audioMutex);
        if (m_audioBuffer.size() + sampleCount > kMaxAudioSamples)
        {
            const size_t excess = m_audioBuffer.size() + sampleCount - kMaxAudioSamples;
            if (excess >= m_audioBuffer.size())
                m_audioBuffer.clear();
            else
                m_audioBuffer.erase(m_audioBuffer.begin(),
                                    m_audioBuffer.begin() + static_cast<std::ptrdiff_t>(excess));
        }
        m_audioBuffer.insert(m_audioBuffer.end(), frameAudio.begin(), frameAudio.begin() + sampleCount);
    }

    captureVideoFrame();
}

void GenesisCore::Reset()
{
    if (!m_ready)
        return;
    system_reset();
    std::lock_guard<std::mutex> lock(m_audioMutex);
    m_audioBuffer.clear();
}

bool GenesisCore::Serialize(std::vector<uint8_t>& outBuf) const
{
    if (!m_ready)
        return false;

    outBuf.resize(STATE_SIZE);
    const int size = state_save(outBuf.data());
    if (size <= 0 || size > STATE_SIZE)
    {
        outBuf.clear();
        return false;
    }
    outBuf.resize(static_cast<size_t>(size));
    return true;
}

bool GenesisCore::Unserialize(const std::vector<uint8_t>& buf)
{
    if (!m_ready || buf.size() < 16 || buf.size() > STATE_SIZE)
        return false;

    std::vector<uint8_t> state(STATE_SIZE, 0);
    std::copy(buf.begin(), buf.end(), state.begin());
    if (state_load(state.data()) <= 0)
        return false;

    {
        std::lock_guard<std::mutex> lock(m_audioMutex);
        m_audioBuffer.clear();
    }
    captureVideoFrame();
    return true;
}

LibretroLoader::VideoFrame GenesisCore::GetVideoFrame() const
{
    std::lock_guard<std::mutex> lock(m_videoMutex);
    return m_videoFrame;
}

bool GenesisCore::DrainAudio(std::vector<int16_t>& out)
{
    std::lock_guard<std::mutex> lock(m_audioMutex);
    if (m_audioBuffer.empty())
        return false;
    out.swap(m_audioBuffer);
    m_audioBuffer.clear();
    return true;
}

void GenesisCore::SetButtonState(unsigned player, unsigned id, bool pressed)
{
    if (player < kInputPorts && id < kInputButtons)
        m_buttons[player][id] = pressed;
}

void GenesisCore::SetButtonsFromSignal(unsigned player)
{
    if (player >= kInputPorts)
        return;
    const uint32_t mask = beiklive::GameSignal::instance().getGameButtonMask(player);
    for (unsigned id = 0; id < kInputButtons; ++id)
        m_buttons[player][id] = ((mask >> id) & 1u) != 0;
}

void GenesisCore::ApplyCheats(const std::vector<CheatEntry>& cheats)
{
    const size_t enabled = static_cast<size_t>(std::count_if(cheats.begin(), cheats.end(),
        [](const CheatEntry& cheat) { return cheat.enabled; }));
    if (enabled > 0)
        brls::Logger::warning("GenesisCore: native cheat application is not available yet ({} enabled)", enabled);
}

const void* GenesisCore::getSramData() const
{
    return (m_ready && sram.on) ? sram.sram : nullptr;
}

size_t GenesisCore::getSramSize() const
{
    return (m_ready && sram.on) ? sizeof(sram.sram) : 0;
}

bool GenesisCore::saveSram()
{
    if (!sram.on)
        return true;

    const std::string path = saveFilePath();
    if (path.empty())
        return true;

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    if (ec)
    {
        brls::Logger::warning("GenesisCore: failed to create SRAM directory: {}", ec.message());
        return false;
    }

    size_t size = sizeof(sram.sram);
    while (size > 0 && sram.sram[size - 1] == 0xFF)
        --size;

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
        return false;
    file.write(reinterpret_cast<const char*>(sram.sram), static_cast<std::streamsize>(size));
    const bool ok = static_cast<bool>(file);
    if (ok)
        brls::Logger::debug("GenesisCore: SRAM saved: {} ({} bytes)", path, size);
    return ok;
}

bool GenesisCore::loadSram()
{
    if (!sram.on)
        return true;

    const std::string path = saveFilePath();
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return true;

    std::fill(std::begin(sram.sram), std::end(sram.sram), 0xFF);
    file.read(reinterpret_cast<char*>(sram.sram), static_cast<std::streamsize>(sizeof(sram.sram)));
    if (!file.eof() && !file)
        return false;
    brls::Logger::info("GenesisCore: SRAM loaded: {}", path);
    return true;
}

void GenesisCore::updateInput()
{
    for (unsigned player = 0; player < kInputPorts; ++player)
    {
        const auto& buttons = m_buttons[player];
        uint16_t pad = 0;
        pad |= buttons[kRetroUp] ? INPUT_UP : 0;
        pad |= buttons[kRetroDown] ? INPUT_DOWN : 0;
        pad |= buttons[kRetroLeft] ? INPUT_LEFT : 0;
        pad |= buttons[kRetroRight] ? INPUT_RIGHT : 0;
        pad |= buttons[kRetroY] ? INPUT_A : 0;
        pad |= buttons[kRetroB] ? INPUT_B : 0;
        pad |= buttons[kRetroA] ? INPUT_C : 0;
        pad |= buttons[kRetroStart] ? INPUT_START : 0;
        pad |= buttons[kRetroL] ? INPUT_X : 0;
        pad |= buttons[kRetroX] ? INPUT_Y : 0;
        pad |= buttons[kRetroR] ? INPUT_Z : 0;
        pad |= buttons[kRetroSelect] ? INPUT_MODE : 0;
        input.pad[player] = pad;
    }
}

void GenesisCore::captureVideoFrame()
{
    if (!bitmap.data || bitmap.viewport.w <= 0 || bitmap.viewport.h <= 0)
        return;

    m_width = static_cast<unsigned>(bitmap.viewport.w);
    m_height = static_cast<unsigned>(bitmap.viewport.h);
    std::lock_guard<std::mutex> lock(m_videoMutex);
    m_videoFrame.width = m_width;
    m_videoFrame.height = m_height;
    m_videoFrame.pixels.resize(static_cast<size_t>(m_width) * m_height);

    for (unsigned y = 0; y < m_height; ++y)
    {
        const auto* src = reinterpret_cast<const uint16_t*>(bitmap.data + static_cast<size_t>(y) * bitmap.pitch);
        uint32_t* dst = m_videoFrame.pixels.data() + static_cast<size_t>(y) * m_width;
        for (unsigned x = 0; x < m_width; ++x)
            dst[x] = rgb565ToRgba(src[x]);
    }
}

void GenesisCore::clearRuntimeState()
{
    {
        std::lock_guard<std::mutex> lock(m_videoMutex);
        m_videoFrame = {};
    }
    {
        std::lock_guard<std::mutex> lock(m_audioMutex);
        m_audioBuffer.clear();
    }
    m_bitmapStorage.clear();
    m_buttons = {};
    m_width = 320;
    m_height = 224;
    m_fps = 60.0;
}

std::string GenesisCore::saveFilePath() const
{
    std::string directory = m_gameEntry.savePath;
    if (directory.empty())
        directory = beiklive::tools::defaultGameSavePath(m_gameEntry.platform, m_gameEntry.path);
    if (directory.empty() || m_gameEntry.path.empty())
        return {};

    return (std::filesystem::path(directory) /
            (beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".srm")).string();
}

} // namespace beiklive::genesis
