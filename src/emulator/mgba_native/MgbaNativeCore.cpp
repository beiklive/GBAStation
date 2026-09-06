#include "MgbaNativeCore.hpp"

#include "core/Tools.hpp"
#include "emulator/mgba_native/MgbaCheatSystem.hpp"

#include <mgba/core/blip_buf.h>
#include <mgba/core/cheats.h>
#include <mgba/core/config.h>
#include <mgba/core/cpu.h>
#include <mgba/core/core.h>
#include <mgba/core/serialize.h>
#include <mgba/internal/arm/arm.h>
#include <mgba/gb/interface.h>
#include <mgba/internal/gba/audio.h>
#include <mgba/internal/gba/cheats.h>
#include <mgba/internal/gb/cheats.h>
#include <mgba/internal/gb/overrides.h>
#include <mgba/internal/sm83/sm83.h>
#include <mgba-util/vfs.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <utility>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace beiklive::mgba_native
{
namespace
{
constexpr unsigned kMgbaAudioBuffers = 0x400;

uint32_t makeRGBA8888(uint8_t r, uint8_t g, uint8_t b)
{
    return static_cast<uint32_t>(r) |
           (static_cast<uint32_t>(g) << 8) |
           (static_cast<uint32_t>(b) << 16) |
           0xFF000000u;
}

uint32_t nativeColorToRgba(color_t px)
{
#if defined(COLOR_16_BIT) && defined(COLOR_5_6_5)
    const uint8_t r5 = static_cast<uint8_t>((px >> 11) & 0x1F);
    const uint8_t g6 = static_cast<uint8_t>((px >> 5) & 0x3F);
    const uint8_t b5 = static_cast<uint8_t>(px & 0x1F);
    return makeRGBA8888(
        static_cast<uint8_t>((r5 << 3) | (r5 >> 2)),
        static_cast<uint8_t>((g6 << 2) | (g6 >> 4)),
        static_cast<uint8_t>((b5 << 3) | (b5 >> 2)));
#elif defined(COLOR_16_BIT)
    const uint8_t r5 = static_cast<uint8_t>(px & 0x1F);
    const uint8_t g5 = static_cast<uint8_t>((px >> 5) & 0x1F);
    const uint8_t b5 = static_cast<uint8_t>((px >> 10) & 0x1F);
    return makeRGBA8888(
        static_cast<uint8_t>((r5 << 3) | (r5 >> 2)),
        static_cast<uint8_t>((g5 << 3) | (g5 >> 2)),
        static_cast<uint8_t>((b5 << 3) | (b5 >> 2)));
#else
    return static_cast<uint32_t>(px) | 0xFF000000u;
#endif
}

bool useBios()
{
    return GET_SETTING_KEY_STR("core.mgba_use_bios", "ON") == "ON";
}

bool skipBios()
{
    return GET_SETTING_KEY_STR("core.mgba_skip_bios", "OFF") == "ON";
}

bool settingEnabled(const char* key, const char* fallback, const char* enabledValue)
{
    return GET_SETTING_KEY_STR(key, fallback) == enabledValue;
}

int settingInt(const char* key, const char* fallback, int minValue, int maxValue)
{
    const std::string value = GET_SETTING_KEY_STR(key, fallback);
    int result = std::atoi(value.c_str());
    return std::clamp(result, minValue, maxValue);
}

GBModel gbModelFromSetting()
{
    const std::string model = GET_SETTING_KEY_STR("core.mgba_gb_model", "Autodetect");
    if (model == "Game Boy")
        return GB_MODEL_DMG;
    if (model == "Super Game Boy")
        return GB_MODEL_SGB;
    if (model == "Game Boy Color")
        return GB_MODEL_CGB;
    if (model == "Game Boy Advance")
        return GB_MODEL_AGB;
    return GB_MODEL_AUTODETECT;
}

const char* idleOptimizationFromSetting()
{
    const std::string mode = GET_SETTING_KEY_STR("core.mgba_idle_optimization", "Remove Known");
    if (mode == "Don't Remove")
        return "ignore";
    if (mode == "Detect and Remove")
        return "detect";
    return "remove";
}

void applyGbPalette(mCoreConfig* config)
{
    const std::string selected = GET_SETTING_KEY_STR("core.mgba_gb_colors", "Grayscale");
    const GBColorPreset* presets = nullptr;
    const size_t count = GBColorPresetList(&presets);
    for (size_t i = 0; i < count; ++i)
    {
        if (selected != presets[i].name)
            continue;

        for (size_t color = 0; color < 12; ++color)
        {
            const std::string key = "gb.pal[" + std::to_string(color) + "]";
            mCoreConfigSetOverrideUIntValue(config, key.c_str(), presets[i].colors[color] & 0xFFFFFFu);
        }
        return;
    }
}

std::string trimCopy(std::string text)
{
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return "";
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

bool hasOnlyHex(const std::string& text)
{
    return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char ch) {
        return std::isxdigit(ch) != 0;
    });
}

bool hasOnlyHexWithSize(const std::string& text, size_t size)
{
    return text.size() == size && hasOnlyHex(text);
}

bool isMgbaGamePlatform(int platform)
{
    using beiklive::enums::EmuPlatform;
    return platform == static_cast<int>(EmuPlatform::EmuGBA) ||
           platform == static_cast<int>(EmuPlatform::EmuGBC) ||
           platform == static_cast<int>(EmuPlatform::EmuGB);
}

bool isLikelyGbaRawAddress(const std::string& value)
{
    if (value.size() != 8 || !hasOnlyHex(value) || value[0] != '0')
        return false;
    const char high = static_cast<char>(std::toupper(static_cast<unsigned char>(value[1])));
    return high >= '2' && high <= 'E';
}

bool codeTypeForcesRaw(const std::string& value)
{
    return beiklive::mgba_native::cheats::NormalizeCodeType(value) == "RAW";
}

bool codeTypeForcesGsCb(const std::string& value)
{
    return beiklive::mgba_native::cheats::NormalizeCodeType(value) == "GS/CB";
}

std::vector<std::string> splitCheatTokens(const std::string& code)
{
    std::vector<std::string> tokens;
    std::string current;
    for (unsigned char ch : code)
    {
        if (std::isspace(ch) || ch == '+' || ch == ',' || ch == ';')
        {
            if (!current.empty())
            {
                tokens.push_back(trimCopy(current));
                current.clear();
            }
            continue;
        }
        current.push_back(static_cast<char>(ch));
    }
    if (!current.empty())
        tokens.push_back(trimCopy(current));
    return tokens;
}

bool addCheatLine(mCheatSet* set, const std::string& rawLine)
{
    const std::string line = trimCopy(rawLine);
    if (!set || line.empty())
        return false;
    return mCheatAddLine(set, line.c_str(), 0);
}

bool canTryMgbaCheat(const beiklive::CheatEntry& cheat)
{
    return cheat.payloadType == beiklive::CheatPayloadType::LibretroRaw ||
           cheat.payloadType == beiklive::CheatPayloadType::FrontendMemoryPatch;
}

std::vector<std::string> splitGbaCheatLines(const std::string& code, const std::string& codeType)
{
    std::vector<std::string> lines;
    const auto tokens = splitCheatTokens(code);
    lines.reserve(tokens.size());
    const bool forceRaw = codeTypeForcesRaw(codeType);
    const bool forceGsCb = codeTypeForcesGsCb(codeType);

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const std::string& token = tokens[i];
        if (token.empty())
            continue;

        std::string line = token;
        if (token.find(':') == std::string::npos)
        {
            if ((token.size() == 10 || token.size() == 12) && hasOnlyHex(token))
            {
                line = token.substr(0, 8) + ":" + token.substr(8);
            }
            else if (token.size() == 16 && hasOnlyHex(token))
            {
                line = token.substr(0, 8) + (forceRaw ? ":" : " ") + token.substr(8);
            }
            else if (hasOnlyHexWithSize(token, 8) && i + 1 < tokens.size() &&
                     (hasOnlyHexWithSize(tokens[i + 1], 2) ||
                      hasOnlyHexWithSize(tokens[i + 1], 4) ||
                      hasOnlyHexWithSize(tokens[i + 1], 8)))
            {
                const std::string& value = tokens[i + 1];
                const bool raw = forceRaw ||
                                 (!forceGsCb &&
                                  (value.size() == 2 ||
                                   (value.size() == 4 && isLikelyGbaRawAddress(token))));
                line = raw ? (token + ":" + value) : (token + " " + value);
                ++i;
            }
        }

        lines.push_back(std::move(line));
    }
    return lines;
}

size_t addGbCheatLines(mCheatSet* set, const std::string& code, size_t& rejected, std::string& failedLine)
{
    size_t accepted = 0;
    const auto tokens = splitCheatTokens(code);
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const std::string& token = tokens[i];
        if (token.empty())
            continue;

        std::string line = token;
        if (token.find(':') == std::string::npos && token.find('-') == std::string::npos)
        {
            if (token.size() == 9 && hasOnlyHex(token))
            {
                line = token.substr(0, 3) + "-" + token.substr(3, 3) + "-" + token.substr(6, 3);
            }
            else if (token.size() == 4 && hasOnlyHex(token) && i + 2 < tokens.size() &&
                     tokens[i + 1].size() == 2 && hasOnlyHex(tokens[i + 1]) &&
                     tokens[i + 2].size() == 2 && hasOnlyHex(tokens[i + 2]))
            {
                line = token + tokens[i + 1] + tokens[i + 2];
                i += 2;
            }
        }

        if (addCheatLine(set, line))
            ++accepted;
        else
        {
            ++rejected;
            if (failedLine.empty())
                failedLine = line;
        }
    }
    return accepted;
}

void clearMgbaCheats(mCheatDevice* device)
{
    if (!device)
        return;

    while (mCheatSetsSize(&device->cheats) > 0)
    {
        mCheatSet* set = *mCheatSetsGetPointer(&device->cheats, 0);
        if (!set)
        {
            mCheatSetsShift(&device->cheats, 0, 1);
            continue;
        }

        set->enabled = false;
        mCheatRefresh(device, set);
        mCheatRemoveSet(device, set);
        mCheatSetDeinit(set);
    }
}

} // namespace

MgbaNativeCore::~MgbaNativeCore()
{
    if (m_ready)
        Cleanup();
    else
        releaseCore();
}

bool MgbaNativeCore::SetupGame(beiklive::GameEntry gameEntry)
{
    Cleanup();
    m_gameEntry = std::move(gameEntry);
    m_loggedFirstAudio = false;
    m_audioProbeFrames = 0;
    m_audioSilentProbeFrames = 0;

    if (!isMgbaGamePlatform(m_gameEntry.platform))
    {
        brls::Logger::error("MgbaNativeCore: rejected non-mGBA platform={}", m_gameEntry.platform);
        return false;
    }

    brls::Logger::debug("MgbaNativeCore: SetupGame begin");
    if (!loadRom(m_gameEntry.runtimePath.empty() ? m_gameEntry.path : m_gameEntry.runtimePath))
        return false;

    brls::Logger::debug("MgbaNativeCore: SetupGame loadRom ok");
    Reset();
    brls::Logger::debug("MgbaNativeCore: SetupGame reset ok");
    loadCheats();
    brls::Logger::debug("MgbaNativeCore: SetupGame loadCheats ok");
    captureVideoFrame();
    brls::Logger::debug("MgbaNativeCore: SetupGame captureVideoFrame ok");

    m_ready = true;
    brls::Logger::info("MgbaNativeCore: ROM loaded: {} ({}x{} @ {:.2f} fps)",
                       m_gameEntry.path, m_width, m_height, m_fps);
    return true;
}

void MgbaNativeCore::Cleanup()
{
    if (m_ready)
    {
        m_ready = false;
        saveSram();
    }
    releaseCore();
}

void MgbaNativeCore::RunFrame()
{
    if (!m_ready || !m_core)
        return;

    updateKeys();
    m_core->runFrame(m_core);
    captureVideoFrame();
    if (!m_audioStreamEnabled)
        drainMgbaAudio();
}

void MgbaNativeCore::Reset()
{
    m_audioLowPassLeftPrev = 0;
    m_audioLowPassRightPrev = 0;
    if (m_core)
    {
        m_core->reset(m_core);
        if (m_core->reloadConfigOption)
        {
            m_core->reloadConfigOption(m_core, "mute", &m_core->config);
            m_core->reloadConfigOption(m_core, "volume", &m_core->config);
        }
    }
}

bool MgbaNativeCore::Serialize(std::vector<uint8_t>& outBuf) const
{
    if (!m_ready || !m_core)
        return false;

    VFile* vf = VFileMemChunk(nullptr, 0);
    if (!vf)
        return false;

    const bool ok = mCoreSaveStateNamed(m_core, vf, SAVESTATE_SAVEDATA | SAVESTATE_RTC);
    if (!ok)
    {
        vf->close(vf);
        return false;
    }

    const ssize_t size = vf->size(vf);
    if (size <= 0)
    {
        vf->close(vf);
        return false;
    }

    outBuf.resize(static_cast<size_t>(size));
    vf->seek(vf, 0, SEEK_SET);
    const ssize_t read = vf->read(vf, outBuf.data(), outBuf.size());
    vf->close(vf);
    return read == static_cast<ssize_t>(outBuf.size());
}

bool MgbaNativeCore::Unserialize(const std::vector<uint8_t>& buf)
{
    if (!m_ready || !m_core || buf.empty())
        return false;

    VFile* vf = VFileFromConstMemory(buf.data(), buf.size());
    if (!vf)
        return false;
    const bool ok = mCoreLoadStateNamed(m_core, vf, SAVESTATE_RTC);
    vf->close(vf);
    if (ok)
        captureVideoFrame();
    return ok;
}

LibretroLoader::VideoFrame MgbaNativeCore::GetVideoFrame() const
{
    std::lock_guard<std::mutex> lock(m_videoMutex);
    return m_videoFrame;
}

bool MgbaNativeCore::DrainAudio(std::vector<int16_t>& out)
{
#ifdef __SWITCH__
    if (m_switchAudioInitialized)
    {
        out.clear();
        return false;
    }
#endif
    std::lock_guard<std::mutex> lock(m_audioMutex);
    out.clear();
    if (m_audioBuffer.empty())
        return false;
    out = std::move(m_audioBuffer);
    m_audioBuffer.clear();
    return true;
}

bool MgbaNativeCore::HandlesAudioOutput() const
{
#ifdef __SWITCH__
    return m_switchAudioInitialized;
#else
    return false;
#endif
}

void MgbaNativeCore::SetAudioOutputEnabled(bool enabled)
{
    m_audioOutputEnabled = enabled;
    if (!enabled)
        FlushAudioOutput();
}

void MgbaNativeCore::SetAudioOutputSpeed(float speed)
{
    const float next = std::clamp(speed, 0.1f, 10.0f);
    if (std::fabs(next - m_audioOutputSpeed) < 0.001f)
        return;

    m_audioOutputSpeed = next;
    applyCoreAudioRates();
    brls::Logger::debug("MgbaNativeCore: audio output speed set to {:.2f}", m_audioOutputSpeed);
}

void MgbaNativeCore::FlushAudioOutput()
{
#ifdef __SWITCH__
    if (!m_switchAudioInitialized)
        return;

    constexpr uint64_t kDrainTimeoutNs = 16000000ULL;
    for (int i = 0; m_switchAudioEnqueued > 0 && i < kSwitchAudioBufferCount * 4; ++i)
        waitNativeAudioOutput(kDrainTimeoutNs);
    m_switchAudioEnqueued = 0;
    m_switchAudioActive = 0;
#else
    std::lock_guard<std::mutex> lock(m_audioMutex);
    m_audioBuffer.clear();
#endif
}

void MgbaNativeCore::SetButtonState(unsigned player, unsigned id, bool pressed)
{
    if (player >= kMaxInputPorts || id >= kMaxButtons)
        return;
    m_buttons[player][id] = pressed;
}

void MgbaNativeCore::SetButtonsFromSignal(unsigned player)
{
    if (player >= kMaxInputPorts)
        return;

    const uint32_t mask = GameSignal::instance().getGameButtonMask(player);
    for (unsigned i = 0; i < kMaxButtons; ++i)
        m_buttons[player][i] = ((mask >> i) & 1u) != 0;
}

void MgbaNativeCore::ApplyCheats(const std::vector<CheatEntry>& cheats)
{
    m_cheats = cheats;
    size_t enabled = 0;
    for (const auto& cheat : m_cheats)
    {
        if (cheat.enabled)
            ++enabled;
    }
    brls::Logger::info("MgbaNativeCore: ApplyCheats entries={} enabled={}",
                       m_cheats.size(), enabled);
    updateCheats();
}

void MgbaNativeCore::ReloadCheats()
{
    brls::Logger::info("MgbaNativeCore: ReloadCheats path={}", m_gameEntry.cheatPath);
    loadCheats();
}

void MgbaNativeCore::SetCheatPath(const std::string& path)
{
    m_gameEntry.cheatPath = path;
    brls::Logger::info("MgbaNativeCore: SetCheatPath path={}", path);
    loadCheats();
}

const void* MgbaNativeCore::getSramData() const
{
    m_sramSnapshot.clear();
    if (!m_core || !m_core->savedataClone)
        return nullptr;

    void* data = nullptr;
    const size_t size = m_core->savedataClone(m_core, &data);
    if (!data || size == 0)
        return nullptr;

    m_sramSnapshot.resize(size);
    std::memcpy(m_sramSnapshot.data(), data, size);
    std::free(data);
    return m_sramSnapshot.data();
}

size_t MgbaNativeCore::getSramSize() const
{
    getSramData();
    return m_sramSnapshot.size();
}

bool MgbaNativeCore::saveSram()
{
    if (!m_core || !m_core->savedataClone)
        return true;

    void* data = nullptr;
    const size_t size = m_core->savedataClone(m_core, &data);
    if (!data || size == 0)
        return true;

    const std::string path = saveFilePath();
    if (path.empty())
    {
        std::free(data);
        return true;
    }

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
    {
        brls::Logger::warning("MgbaNativeCore: failed to open SRAM file: {}", path);
        std::free(data);
        return true;
    }

    file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    std::free(data);
    if (!file)
    {
        brls::Logger::warning("MgbaNativeCore: failed to write SRAM file: {}", path);
        return true;
    }

    brls::Logger::debug("MgbaNativeCore: SRAM saved to {} ({} bytes)", path, size);
    return true;
}

bool MgbaNativeCore::loadRom(const std::string& romPath)
{
    brls::Logger::debug("MgbaNativeCore: loadRom begin");
    if (romPath.empty())
    {
        brls::Logger::error("MgbaNativeCore: ROM path is empty");
        return false;
    }

    releaseCore();
    const auto platform = static_cast<beiklive::enums::EmuPlatform>(m_gameEntry.platform);
    const mPlatform mgbaPlatform =
        platform == beiklive::enums::EmuPlatform::EmuGBA ? mPLATFORM_GBA : mPLATFORM_GB;
    brls::Logger::debug("MgbaNativeCore: creating core platform={}", static_cast<int>(mgbaPlatform));
    m_core = mCoreCreate(mgbaPlatform);
    if (!m_core)
    {
        brls::Logger::error("MgbaNativeCore: mCoreCreate failed");
        return false;
    }

    brls::Logger::debug("MgbaNativeCore: core created");
    if (!m_core->init(m_core))
    {
        brls::Logger::error("MgbaNativeCore: core init failed");
        releaseCore();
        return false;
    }
    m_coreInitialized = true;

    brls::Logger::debug("MgbaNativeCore: core init ok");
#ifdef __SWITCH__
    m_sampleRate = static_cast<double>(audoutGetSampleRate());
    if (m_sampleRate <= 0.0)
        m_sampleRate = 48000.0;
#else
    m_sampleRate = kDefaultSampleRate;
#endif
    initConfigDefaults();
    brls::Logger::debug("MgbaNativeCore: config defaults ok");
    installPeripherals();
    brls::Logger::debug("MgbaNativeCore: peripherals ok");
    applyConfig();
    brls::Logger::debug("MgbaNativeCore: pre-load config applied ok");

    brls::Logger::debug("MgbaNativeCore: loading ROM file");
    if (!mCoreLoadFile(m_core, romPath.c_str()))
    {
        brls::Logger::error("MgbaNativeCore: mCoreLoadFile failed: {}", romPath);
        releaseCore();
        return false;
    }
    brls::Logger::debug("MgbaNativeCore: ROM file loaded");
    applyConfig();
    brls::Logger::debug("MgbaNativeCore: post-load config applied ok");
    loadSram();
    brls::Logger::debug("MgbaNativeCore: SRAM setup ok");

    unsigned desiredW = 0;
    unsigned desiredH = 0;
    m_core->desiredVideoDimensions(m_core, &desiredW, &desiredH);
    m_width = desiredW > 0 ? desiredW : static_cast<unsigned>(beiklive::GetGamePixelWidth(m_gameEntry.platform));
    m_height = desiredH > 0 ? desiredH : static_cast<unsigned>(beiklive::GetGamePixelHeight(m_gameEntry.platform));
    m_bufferWidth = std::max(m_width, kMaxVideoWidth);
    m_bufferHeight = std::max(m_height, kMaxVideoHeight);
    m_videoBuffer.assign(static_cast<size_t>(m_bufferWidth) * m_bufferHeight, 0);
    m_core->setVideoBuffer(m_core, m_videoBuffer.data(), m_bufferWidth);
    brls::Logger::debug("MgbaNativeCore: video buffer configured visible={}x{} backing={}x{} bpp={}",
                        m_width, m_height, m_bufferWidth, m_bufferHeight, BYTES_PER_PIXEL);

    const int32_t cycles = m_core->frameCycles(m_core);
    const int32_t frequency = m_core->frequency(m_core);
    if (cycles > 0 && frequency > 0)
        m_fps = static_cast<double>(frequency) / static_cast<double>(cycles);

    configureAudioStream();
    return true;
}

void MgbaNativeCore::initConfigDefaults()
{
    if (!m_core)
        return;

    mCoreInitConfig(m_core, "BeikLiveStation");
    m_configInitialized = true;
    mCoreConfigSetDefaultIntValue(&m_core->config, "sampleRate", static_cast<int>(m_sampleRate));
    mCoreConfigSetDefaultUIntValue(&m_core->config, "audioBuffers", kMgbaAudioBuffers);
    mCoreConfigSetDefaultIntValue(&m_core->config, "volume", GBA_AUDIO_VOLUME_MAX);
    mCoreConfigSetDefaultIntValue(&m_core->config, "mute", 0);
    mCoreConfigSetDefaultIntValue(&m_core->config, "threadedVideo", 0);
    mCoreConfigSetDefaultIntValue(&m_core->config, "useBios", useBios() ? 1 : 0);
    mCoreConfigSetDefaultIntValue(&m_core->config, "skipBios", skipBios() ? 1 : 0);
    mCoreConfigSetDefaultIntValue(&m_core->config, "frameskip", settingInt("core.mgba_frameskip", "0", 0, 10));
    mCoreConfigSetDefaultIntValue(&m_core->config, "allowOpposingDirections",
                                  settingEnabled("core.mgba_allow_opposing_directions", "no", "yes") ? 1 : 0);
    mCoreConfigSetDefaultValue(&m_core->config, "idleOptimization", idleOptimizationFromSetting());
    mCoreConfigSetDefaultIntValue(&m_core->config, "gba.forceGbp",
                                  settingEnabled("core.mgba_force_gbp", "OFF", "ON") ? 1 : 0);

    const char* modelName = GBModelToName(gbModelFromSetting());
    mCoreConfigSetDefaultValue(&m_core->config, "gb.model", modelName);
    mCoreConfigSetDefaultValue(&m_core->config, "sgb.model", modelName);
    mCoreConfigSetDefaultValue(&m_core->config, "cgb.model", modelName);
    mCoreConfigSetDefaultIntValue(&m_core->config, "sgb.borders",
                                  settingEnabled("core.mgba_sgb_borders", "OFF", "ON") ? 1 : 0);
    applyGbPalette(&m_core->config);
    applyAudioLowPassSettings();
}

void MgbaNativeCore::applyConfig()
{
    if (!m_core)
        return;

    const int frameskip = settingInt("core.mgba_frameskip", "0", 0, 10);
    const bool allowOpposing = settingEnabled("core.mgba_allow_opposing_directions", "no", "yes");
    const bool sgbBorders = settingEnabled("core.mgba_sgb_borders", "OFF", "ON");
    const bool forceGbp = settingEnabled("core.mgba_force_gbp", "OFF", "ON");
    const char* modelName = GBModelToName(gbModelFromSetting());

    mCoreConfigSetOverrideIntValue(&m_core->config, "sampleRate", static_cast<int>(m_sampleRate));
    mCoreConfigSetOverrideUIntValue(&m_core->config, "audioBuffers", kMgbaAudioBuffers);
    mCoreConfigSetOverrideIntValue(&m_core->config, "volume", GBA_AUDIO_VOLUME_MAX);
    mCoreConfigSetOverrideIntValue(&m_core->config, "mute", 0);
    mCoreConfigSetOverrideIntValue(&m_core->config, "threadedVideo", 0);
    mCoreConfigSetOverrideIntValue(&m_core->config, "useBios", useBios() ? 1 : 0);
    mCoreConfigSetOverrideIntValue(&m_core->config, "skipBios", skipBios() ? 1 : 0);
    mCoreConfigSetOverrideIntValue(&m_core->config, "frameskip", frameskip);
    mCoreConfigSetOverrideIntValue(&m_core->config, "allowOpposingDirections", allowOpposing ? 1 : 0);
    mCoreConfigSetOverrideValue(&m_core->config, "idleOptimization", idleOptimizationFromSetting());
    mCoreConfigSetOverrideIntValue(&m_core->config, "gba.forceGbp", forceGbp ? 1 : 0);
    mCoreConfigSetOverrideValue(&m_core->config, "gb.model", modelName);
    mCoreConfigSetOverrideValue(&m_core->config, "sgb.model", modelName);
    mCoreConfigSetOverrideValue(&m_core->config, "cgb.model", modelName);
    mCoreConfigSetOverrideIntValue(&m_core->config, "sgb.borders", sgbBorders ? 1 : 0);
    applyGbPalette(&m_core->config);
    applyAudioLowPassSettings();
    updateLuxLevel();

    m_useSystemRtc = GET_SETTING_KEY_STR("core.mgba_rtc_mode", "persist") == "system";
    if (m_useSystemRtc)
        mCoreSetRTC(m_core, &m_rtcSource.d);
    else
        m_core->rtc.override = RTC_NO_OVERRIDE;

    mCoreLoadForeignConfig(m_core, &m_core->config);
    brls::Logger::info("MgbaNativeCore: config loaded sampleRate={} audioBuffers={} volume={} mute={}",
                       m_core->opts.sampleRate,
                       m_core->opts.audioBuffers,
                       m_core->opts.volume,
                       m_core->opts.mute ? 1 : 0);
}

void MgbaNativeCore::applyAudioLowPassSettings()
{
    const bool enabled = GET_SETTING_KEY_STR("core.mgba_audio_low_pass_filter", "disabled") == "enabled";
    const int range = settingInt("core.mgba_audio_low_pass_range", "60", 0, 100);
    const int32_t fixedRange = (range * 0x10000) / 100;
    if (enabled != m_audioLowPassEnabled || fixedRange != m_audioLowPassRange)
    {
        m_audioLowPassLeftPrev = 0;
        m_audioLowPassRightPrev = 0;
    }
    m_audioLowPassEnabled = enabled;
    m_audioLowPassRange = fixedRange;
}

void MgbaNativeCore::applyAudioLowPass(std::vector<int16_t>& samples)
{
    if (!m_audioLowPassEnabled || samples.size() < 2)
        return;

    applyAudioLowPassBuffer(samples.data(), samples.size() / 2);
}

void MgbaNativeCore::applyAudioLowPassBuffer(int16_t* samples, size_t frames)
{
    if (!m_audioLowPassEnabled || !samples || frames == 0)
        return;

    int32_t left = m_audioLowPassLeftPrev;
    int32_t right = m_audioLowPassRightPrev;
    const int32_t factorA = m_audioLowPassRange;
    const int32_t factorB = 0x10000 - factorA;

    for (size_t frame = 0; frame < frames; ++frame)
    {
        int16_t* sample = samples + frame * 2;
        left = (left * factorA) + (sample[0] * factorB);
        right = (right * factorA) + (sample[1] * factorB);
        left >>= 16;
        right >>= 16;
        sample[0] = static_cast<int16_t>(left);
        sample[1] = static_cast<int16_t>(right);
    }

    m_audioLowPassLeftPrev = left;
    m_audioLowPassRightPrev = right;
}

void MgbaNativeCore::installPeripherals()
{
    if (!m_core)
        return;

    m_luminanceSource.owner = this;
    m_luminanceSource.d.sample = &MgbaNativeCore::sampleLux;
    m_luminanceSource.d.readLuminance = &MgbaNativeCore::readLux;
    updateLuxLevel();

    m_rtcSource.owner = this;
    m_rtcSource.d.sample = &MgbaNativeCore::sampleRtc;
    m_rtcSource.d.unixTime = &MgbaNativeCore::readRtcUnixTime;
    m_rtcSource.d.serialize = nullptr;
    m_rtcSource.d.deserialize = nullptr;

    if (m_core->setPeripheral && m_core->platform(m_core) == mPLATFORM_GBA)
        m_core->setPeripheral(m_core, mPERIPH_GBA_LUMINANCE, &m_luminanceSource.d);
}

void MgbaNativeCore::updateLuxLevel()
{
    m_luxLevelIndex = settingInt("core.mgba_solar_sensor_level", "5", 0, 10);
    m_luxLevel = 0x16;
    if (m_luxLevelIndex > 0)
        m_luxLevel = static_cast<uint8_t>(m_luxLevel + GBA_LUX_LEVELS[m_luxLevelIndex - 1]);
}

bool MgbaNativeCore::loadSram()
{
    if (!m_core)
        return true;

    const std::string path = saveFilePath();
    if (path.empty())
        return true;

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    if (ec)
    {
        brls::Logger::warning("MgbaNativeCore: failed to create SRAM directory for {}: {}", path, ec.message());
        return true;
    }

    if (!mCoreLoadSaveFile(m_core, path.c_str(), false))
        brls::Logger::warning("MgbaNativeCore: failed to attach SRAM file: {}", path);
    else
        brls::Logger::info("MgbaNativeCore: SRAM file attached: {}", path);
    return true;
}

bool MgbaNativeCore::loadCheats()
{
    if (m_gameEntry.cheatPath.empty())
    {
        m_cheats.clear();
        brls::Logger::info("MgbaNativeCore: cleared cheats because cheatPath is empty");
        updateCheats();
        return true;
    }

    auto loaded = beiklive::mgba_native::cheats::LoadCheats(m_gameEntry.cheatPath);
    m_cheats = std::move(loaded.entries);
    brls::Logger::info("MgbaNativeCore: loaded {} mGBA cheats from {} editable={}",
                       m_cheats.size(), m_gameEntry.cheatPath,
                       loaded.editable);
    updateCheats();
    return true;
}

mCheatDevice* MgbaNativeCore::cheatDevice()
{
    if (!m_core)
        return nullptr;
    if (m_fallbackCheatDevice)
        return m_fallbackCheatDevice;
    if (m_core->cheatDevice)
    {
        if (mCheatDevice* device = m_core->cheatDevice(m_core))
            return device;
        brls::Logger::warning("MgbaNativeCore: native cheatDevice returned null, trying fallback");
    }
    if (!m_core->platform || !m_core->cpu)
    {
        brls::Logger::warning("MgbaNativeCore: cheat device fallback failed: missing platform or cpu");
        return nullptr;
    }

    const mPlatform platform = m_core->platform(m_core);
    if (platform == mPLATFORM_GBA)
    {
        auto* cpu = static_cast<ARMCore*>(m_core->cpu);
        if (!cpu || !cpu->components || CPU_COMPONENT_CHEAT_DEVICE >= cpu->numComponents)
        {
            brls::Logger::warning("MgbaNativeCore: cheat device fallback failed: invalid GBA cpu components");
            return nullptr;
        }
        m_fallbackCheatDevice = GBACheatDeviceCreate();
        if (!m_fallbackCheatDevice)
        {
            brls::Logger::warning("MgbaNativeCore: cheat device fallback failed: GBACheatDeviceCreate returned null");
            return nullptr;
        }
        m_fallbackCheatDevice->p = m_core;
        cpu->components[CPU_COMPONENT_CHEAT_DEVICE] = &m_fallbackCheatDevice->d;
        ARMHotplugAttach(cpu, CPU_COMPONENT_CHEAT_DEVICE);
        m_fallbackCheatPlatform = static_cast<int>(platform);
        m_fallbackCheatAttached = true;
        brls::Logger::info("MgbaNativeCore: cheat device fallback ok: GBA device attached");
        return m_fallbackCheatDevice;
    }

    if (platform == mPLATFORM_GB)
    {
        auto* cpu = static_cast<SM83Core*>(m_core->cpu);
        if (!cpu || !cpu->components || CPU_COMPONENT_CHEAT_DEVICE >= cpu->numComponents)
        {
            brls::Logger::warning("MgbaNativeCore: cheat device fallback failed: invalid GB cpu components");
            return nullptr;
        }
        m_fallbackCheatDevice = GBCheatDeviceCreate();
        if (!m_fallbackCheatDevice)
        {
            brls::Logger::warning("MgbaNativeCore: cheat device fallback failed: GBCheatDeviceCreate returned null");
            return nullptr;
        }
        m_fallbackCheatDevice->p = m_core;
        cpu->components[CPU_COMPONENT_CHEAT_DEVICE] = &m_fallbackCheatDevice->d;
        SM83HotplugAttach(cpu, CPU_COMPONENT_CHEAT_DEVICE);
        m_fallbackCheatPlatform = static_cast<int>(platform);
        m_fallbackCheatAttached = true;
        brls::Logger::info("MgbaNativeCore: cheat device fallback ok: GB device attached");
        return m_fallbackCheatDevice;
    }

    brls::Logger::warning("MgbaNativeCore: cheat device fallback failed: unsupported platform={}",
                          static_cast<int>(platform));
    return nullptr;
}

void MgbaNativeCore::releaseFallbackCheatDevice()
{
    if (!m_fallbackCheatDevice)
        return;

    if (m_fallbackCheatAttached && m_core && m_core->cpu)
    {
        if (m_fallbackCheatPlatform == static_cast<int>(mPLATFORM_GBA))
        {
            auto* cpu = static_cast<ARMCore*>(m_core->cpu);
            if (cpu && cpu->components && CPU_COMPONENT_CHEAT_DEVICE < cpu->numComponents &&
                cpu->components[CPU_COMPONENT_CHEAT_DEVICE] == &m_fallbackCheatDevice->d)
            {
                ARMHotplugDetach(cpu, CPU_COMPONENT_CHEAT_DEVICE);
                cpu->components[CPU_COMPONENT_CHEAT_DEVICE] = nullptr;
            }
        }
        else if (m_fallbackCheatPlatform == static_cast<int>(mPLATFORM_GB))
        {
            auto* cpu = static_cast<SM83Core*>(m_core->cpu);
            if (cpu && cpu->components && CPU_COMPONENT_CHEAT_DEVICE < cpu->numComponents &&
                cpu->components[CPU_COMPONENT_CHEAT_DEVICE] == &m_fallbackCheatDevice->d)
            {
                SM83HotplugDetach(cpu, CPU_COMPONENT_CHEAT_DEVICE);
                cpu->components[CPU_COMPONENT_CHEAT_DEVICE] = nullptr;
            }
        }
    }

    mCheatDeviceDestroy(m_fallbackCheatDevice);
    m_fallbackCheatDevice = nullptr;
    m_fallbackCheatAttached = false;
    m_fallbackCheatPlatform = -1;
}

void MgbaNativeCore::updateCheats()
{
    if (!m_core)
    {
        brls::Logger::warning("MgbaNativeCore: apply cheats skipped: core is null");
        return;
    }

    mCheatDevice* device = cheatDevice();
    if (!device)
    {
        brls::Logger::warning("MgbaNativeCore: apply cheats skipped: cheat device is null");
        return;
    }
    if (!device->createSet)
    {
        brls::Logger::warning("MgbaNativeCore: apply cheats skipped: cheat device createSet is null");
        return;
    }

    clearMgbaCheats(device);

    size_t enabledCount = 0;
    for (const auto& cheat : m_cheats)
    {
        if (cheat.enabled)
            ++enabledCount;
    }

    size_t createdSets = 0;
    size_t acceptedLines = 0;
    size_t rejectedLines = 0;
    size_t disabledEntries = 0;
    size_t invalidEntries = 0;
    size_t unsupportedEntries = 0;
    size_t categoryEntries = 0;
    size_t emptyCodeEntries = 0;
    const bool isGba = m_core->platform(m_core) == mPLATFORM_GBA;
    brls::Logger::info("MgbaNativeCore: apply begin total={} enabled={} platform={} deviceReady=1",
                       m_cheats.size(), enabledCount,
                       m_core->platform ? static_cast<int>(m_core->platform(m_core)) : -1);

    for (auto& cheat : m_cheats)
    {
        if (cheat.payloadType == beiklive::CheatPayloadType::Category)
        {
            ++categoryEntries;
            continue;
        }
        if (!cheat.enabled)
        {
            ++disabledEntries;
            continue;
        }
        if (cheat.code.empty())
        {
            ++emptyCodeEntries;
            continue;
        }
        if (!canTryMgbaCheat(cheat))
        {
            ++unsupportedEntries;
            continue;
        }
        if (!cheat.valid && cheat.payloadType != beiklive::CheatPayloadType::FrontendMemoryPatch)
        {
            ++invalidEntries;
            continue;
        }

        const std::string codeType = isGba
            ? beiklive::mgba_native::cheats::NormalizeCodeType(
                  cheat.codeType.empty()
                      ? beiklive::mgba_native::cheats::DetectCodeType(cheat.code)
                      : cheat.codeType)
            : cheat.codeType;
        cheat.codeType = codeType;
        cheat.valid = true;
        cheat.diagnostic.clear();
        const std::string setName = cheat.desc.empty() ? "BeikLiveStation cheat" : cheat.desc;
        mCheatSet* set = device->createSet(device, setName.c_str());
        if (!set)
        {
            brls::Logger::warning("MgbaNativeCore: createSet failed name={} codeType={} code={}",
                                  setName, codeType, cheat.code);
            cheat.valid = false;
            cheat.enabled = false;
            cheat.diagnostic = "createSet failed";
            ++invalidEntries;
            continue;
        }

        size_t setRejectedLines = 0;
        std::string failedLine;
        size_t setAcceptedLines = 0;
        if (isGba)
        {
            const auto lines = splitGbaCheatLines(cheat.code, codeType);
            if (lines.empty())
            {
                failedLine = "<empty>";
            }
            else
            {
                bool valid = true;
                for (const auto& line : lines)
                {
                    if (!addCheatLine(set, line))
                    {
                        valid = false;
                        failedLine = line;
                        break;
                    }
                    ++setAcceptedLines;
                }
                if (!valid)
                    setRejectedLines = 1;
            }
        }
        else
        {
            setAcceptedLines = addGbCheatLines(set, cheat.code, setRejectedLines, failedLine);
        }
        rejectedLines += setRejectedLines;

        if (setAcceptedLines == 0 || setRejectedLines > 0)
        {
            brls::Logger::warning("MgbaNativeCore: reject mGBA cheat name={} codeType={} failedLine={} rawCode={}",
                                  setName, codeType, failedLine, cheat.code);
            cheat.valid = false;
            cheat.enabled = false;
            cheat.diagnostic = failedLine.empty() ? "parse failed" : ("parse failed: " + failedLine);
            ++invalidEntries;
            mCheatSetDeinit(set);
            continue;
        }

        set->enabled = true;
        mCheatAddSet(device, set);
        mCheatRefresh(device, set);

        ++createdSets;
        acceptedLines += setAcceptedLines;
        brls::Logger::info("MgbaNativeCore: registered cheat name={} codeType={} lines={} rawCode={}",
                           setName, codeType, setAcceptedLines, cheat.code);
    }

    if (createdSets == 0)
    {
        if (rejectedLines > 0)
        {
            brls::Logger::warning("MgbaNativeCore: no mGBA cheat sets registered (total={} disabled={} rejectedLines={} invalid={} unsupported={} categories={} empty={})",
                                  m_cheats.size(), disabledEntries, rejectedLines, invalidEntries,
                                  unsupportedEntries, categoryEntries, emptyCodeEntries);
        }
        else
        {
            brls::Logger::info("MgbaNativeCore: no mGBA cheat sets to register (total={} disabled={} invalid={} unsupported={} categories={} empty={})",
                               m_cheats.size(), disabledEntries, invalidEntries, unsupportedEntries,
                               categoryEntries, emptyCodeEntries);
        }
        return;
    }

    brls::Logger::info("MgbaNativeCore: registered mGBA cheat sets total={} sets={} disabled={} acceptedLines={} rejectedLines={} invalid={} unsupported={} categories={} empty={}",
                       m_cheats.size(), createdSets, disabledEntries, acceptedLines, rejectedLines,
                       invalidEntries, unsupportedEntries, categoryEntries, emptyCodeEntries);
}

void MgbaNativeCore::configureAudioStream()
{
    if (!m_core)
        return;

    const bool isGbFamily = m_gameEntry.platform == static_cast<int>(
        beiklive::enums::EmuPlatform::EmuGB) ||
        m_gameEntry.platform == static_cast<int>(
        beiklive::enums::EmuPlatform::EmuGBC);
    const size_t bufferSamples = isGbFamily ? kGbAudioSamples : kSwitchAudioSamples;

#ifdef __SWITCH__
    // GBA supplies a stable number of samples per video frame, so it can be
    // drained once from RunFrame. GB/GBC explicitly require mGBA's fixed-size
    // postAudioBuffer callback: their generated sample count varies per frame.
    // Both paths still feed AudioManager, which owns hardware submission.
    m_audioStreamEnabled = isGbFamily;
    brls::Logger::info("MgbaNativeCore: Switch {} audio routed through AudioManager core2 worker",
                       isGbFamily ? "GB/GBC callback" : "GBA frame drain");
#else
    m_audioStreamEnabled = true;
#endif
    m_audioStream = {};
    m_audioStream.owner = this;
    m_audioStream.d.postAudioFrame = nullptr;
    m_audioStream.d.postAudioBuffer = &MgbaNativeCore::postAudioBuffer;
    m_core->setAVStream(m_core, &m_audioStream.d);

    m_core->setAudioBufferSize(m_core, bufferSamples);
    m_audioOutputSpeed = 1.0f;
    applyCoreAudioRates();
    brls::Logger::debug("MgbaNativeCore: native audio stream configured sampleRate={} bufferSamples={} fps={:.3f} callback={}",
                        static_cast<int>(m_sampleRate), bufferSamples, m_fps,
                        m_audioStreamEnabled ? 1 : 0);
}

void MgbaNativeCore::applyCoreAudioRates()
{
    if (!m_core || !m_core->getAudioChannel)
        return;

    const bool isGbFamily = m_gameEntry.platform == static_cast<int>(
        beiklive::enums::EmuPlatform::EmuGB) ||
        m_gameEntry.platform == static_cast<int>(
        beiklive::enums::EmuPlatform::EmuGBC);
    // The GB/GBC upstream frontend uses the requested output rate directly.
    // GBA retains its frame-clock correction, which is already verified by
    // the existing GBA output path.
    const double ratio = isGbFamily ? 1.0 : static_cast<double>(
        GBAAudioCalculateRatio(1.0f, static_cast<float>(m_fps), 1.0f));
    const double effectiveRate = (m_sampleRate * ratio) / std::max(0.1f, m_audioOutputSpeed);
    if (blip_t* left = m_core->getAudioChannel(m_core, 0))
        blip_set_rates(left, m_core->frequency(m_core), effectiveRate);
    if (blip_t* right = m_core->getAudioChannel(m_core, 1))
        blip_set_rates(right, m_core->frequency(m_core), effectiveRate);
}

bool MgbaNativeCore::initNativeAudioOutput()
{
#ifdef __SWITCH__
    if (m_switchAudioInitialized)
        return true;

    Result rc = audoutInitialize();
    if (R_FAILED(rc))
    {
        brls::Logger::warning("MgbaNativeCore: audoutInitialize failed rc={:#x}; falling back to drained audio", rc);
        return false;
    }

    rc = audoutStartAudioOut();
    if (R_FAILED(rc))
        brls::Logger::debug("MgbaNativeCore: audout already started/shared rc={:#x}", rc);

    m_switchAudioInitialized = true;
    for (int i = 0; i < kSwitchAudioBufferCount; ++i)
    {
        m_switchAudioBuffers[i] = static_cast<int16_t*>(std::aligned_alloc(0x1000, kSwitchAudioBufferBytes));
        if (!m_switchAudioBuffers[i])
        {
            brls::Logger::error("MgbaNativeCore: Switch direct audio buffer allocation failed");
            shutdownNativeAudioOutput();
            return false;
        }
        std::memset(m_switchAudioBuffers[i], 0, kSwitchAudioBufferBytes);
        m_switchAudioOutBuffers[i] = {};
        m_switchAudioOutBuffers[i].buffer = m_switchAudioBuffers[i];
        m_switchAudioOutBuffers[i].buffer_size = kSwitchAudioBufferBytes;
        m_switchAudioOutBuffers[i].data_size = kSwitchAudioSamples * 2 * sizeof(int16_t);
        m_switchAudioOutBuffers[i].data_offset = 0;
    }

    m_switchAudioActive = 0;
    m_switchAudioEnqueued = 0;
    m_loggedFirstSwitchAppend = false;
    m_loggedFirstNonZeroSwitchAudio = false;
    m_switchSilentProbeBuffers = 0;
    m_switchAudioCallbackCount = 0;
    m_switchAudioWaitDropCount = 0;
    brls::Logger::info("MgbaNativeCore: Switch direct audio output initialized sampleRate={} samples={}",
                       static_cast<int>(m_sampleRate), kSwitchAudioSamples);
    return true;
#else
    return false;
#endif
}

void MgbaNativeCore::shutdownNativeAudioOutput()
{
#ifdef __SWITCH__
    if (m_switchAudioInitialized)
    {
        FlushAudioOutput();
        audoutExit();
    }

    for (auto*& buffer : m_switchAudioBuffers)
    {
        std::free(buffer);
        buffer = nullptr;
    }
    for (auto& outBuffer : m_switchAudioOutBuffers)
        outBuffer = {};
    m_switchAudioActive = 0;
    m_switchAudioEnqueued = 0;
    m_switchAudioInitialized = false;
    m_loggedFirstSwitchAppend = false;
    m_loggedFirstNonZeroSwitchAudio = false;
    m_switchSilentProbeBuffers = 0;
    m_switchAudioCallbackCount = 0;
    m_switchAudioWaitDropCount = 0;
#endif
}

int MgbaNativeCore::waitNativeAudioOutput(uint64_t timeoutNs)
{
#ifdef __SWITCH__
    AudioOutBuffer* releasedBuffers = nullptr;
    uint32_t releasedCount = 0;
    Result rc = timeoutNs
        ? audoutWaitPlayFinish(&releasedBuffers, &releasedCount, timeoutNs)
        : audoutGetReleasedAudioOutBuffer(&releasedBuffers, &releasedCount);
    if (R_FAILED(rc) || !releasedBuffers || releasedCount == 0)
        return 0;

    int ownedReleased = 0;
    for (AudioOutBuffer* buffer = releasedBuffers; buffer; buffer = buffer->next)
    {
        for (auto& owned : m_switchAudioOutBuffers)
        {
            if (buffer == &owned)
            {
                ++ownedReleased;
                break;
            }
        }
    }
    if (ownedReleased > 0)
    {
        if (static_cast<uint32_t>(ownedReleased) > m_switchAudioEnqueued)
            m_switchAudioEnqueued = 0;
        else
            m_switchAudioEnqueued -= static_cast<uint32_t>(ownedReleased);
    }
    return ownedReleased;
#else
    (void)timeoutNs;
    return 0;
#endif
}

void MgbaNativeCore::postAudioBuffer(mAVStream* stream, blip_t* left, blip_t* right)
{
    if (!stream || !left || !right)
        return;

    auto* nativeStream = reinterpret_cast<NativeAudioStream*>(stream);
    MgbaNativeCore* owner = nativeStream->owner;
    if (!owner || !owner->m_audioStreamEnabled)
        return;

#ifdef __SWITCH__
    if (owner->m_switchAudioInitialized)
    {
        ++owner->m_switchAudioCallbackCount;
        const int availableLeft = blip_samples_avail(left);
        const int availableRight = blip_samples_avail(right);
        if (owner->m_switchAudioCallbackCount <= 3 ||
            owner->m_switchAudioCallbackCount == 60 ||
            owner->m_switchAudioCallbackCount == 180)
        {
            brls::Logger::info("MgbaNativeCore: direct audio callback #{} availL={} availR={} queued={} enabled={} ff={}",
                               owner->m_switchAudioCallbackCount,
                               availableLeft,
                               availableRight,
                               owner->m_switchAudioEnqueued,
                               owner->m_audioOutputEnabled ? 1 : 0,
                               owner->m_fastForwarding ? 1 : 0);
        }

        owner->waitNativeAudioOutput(0);
        if (!owner->m_audioOutputEnabled)
        {
            blip_clear(left);
            blip_clear(right);
            return;
        }
        constexpr uint64_t kAudioQueueWaitNs = 1000000ULL;
        constexpr int kAudioQueueWaitAttempts = 2;
        for (int attempt = 0;
             owner->m_switchAudioEnqueued >= kSwitchAudioBufferCount - 1 &&
             attempt < kAudioQueueWaitAttempts;
             ++attempt)
        {
            if (owner->waitNativeAudioOutput(kAudioQueueWaitNs) > 0)
                break;
        }
        if (owner->m_switchAudioEnqueued >= kSwitchAudioBufferCount - 1)
        {
            if (owner->m_fastForwarding)
            {
                std::vector<int16_t> drop(kSwitchAudioSamples * 2);
                blip_read_samples(left, drop.data(), kSwitchAudioSamples, true);
                blip_read_samples(right, drop.data() + 1, kSwitchAudioSamples, true);
                return;
            }
            blip_clear(left);
            blip_clear(right);
            if (owner->m_switchAudioWaitDropCount < 5)
            {
                ++owner->m_switchAudioWaitDropCount;
                brls::Logger::warning("MgbaNativeCore: direct audio queue full; dropped batch queued={} drops={}",
                                      owner->m_switchAudioEnqueued,
                                      owner->m_switchAudioWaitDropCount);
            }
            return;
        }

        const int index = owner->m_switchAudioActive;
        int16_t* samples = owner->m_switchAudioBuffers[index];
        if (!samples)
        {
            blip_clear(left);
            blip_clear(right);
            return;
        }

        const int produced = blip_read_samples(left, samples, kSwitchAudioSamples, true);
        if (produced <= 0)
        {
            brls::Logger::warning("MgbaNativeCore: direct audio callback produced no samples availL={} availR={}",
                                  availableLeft, availableRight);
            return;
        }
        const int producedRight = blip_read_samples(right, samples + 1, produced, true);
        if (producedRight < produced)
        {
            brls::Logger::warning("MgbaNativeCore: right audio channel short read left={} right={}",
                                  produced, producedRight);
        }
        if (static_cast<size_t>(produced) < kSwitchAudioSamples)
            std::memset(samples + produced * 2, 0, (kSwitchAudioSamples - static_cast<size_t>(produced)) * 2 * sizeof(int16_t));
        owner->applyAudioLowPassBuffer(samples, static_cast<size_t>(produced));
        armDCacheFlush(samples, kSwitchAudioBufferBytes);

        auto& outBuffer = owner->m_switchAudioOutBuffers[index];
        outBuffer.next = nullptr;
        outBuffer.data_size = kSwitchAudioSamples * 2 * sizeof(int16_t);
        Result rc = audoutAppendAudioOutBuffer(&outBuffer);
        if (R_SUCCEEDED(rc))
        {
            int peak = 0;
            if (!owner->m_loggedFirstNonZeroSwitchAudio || !owner->m_loggedFirstSwitchAppend)
            {
                for (size_t i = 0; i < kSwitchAudioSamples * 2; ++i)
                    peak = std::max(peak, samples[i] < 0 ? -static_cast<int>(samples[i]) : static_cast<int>(samples[i]));
            }
            if (!owner->m_loggedFirstSwitchAppend)
            {
                brls::Logger::info("MgbaNativeCore: first direct Switch audio buffer appended frames={} peak={} queued={}",
                                   produced, peak, owner->m_switchAudioEnqueued);
                owner->m_loggedFirstSwitchAppend = true;
            }
            if (!owner->m_loggedFirstNonZeroSwitchAudio)
            {
                if (peak > 0)
                {
                    brls::Logger::info("MgbaNativeCore: first non-zero Switch audio peak={} after {} silent buffers",
                                       peak, owner->m_switchSilentProbeBuffers);
                    owner->m_loggedFirstNonZeroSwitchAudio = true;
                }
                else if (++owner->m_switchSilentProbeBuffers == 180)
                {
                    brls::Logger::warning("MgbaNativeCore: first {} direct audio buffers were silent", owner->m_switchSilentProbeBuffers);
                }
            }
            owner->m_switchAudioActive = (owner->m_switchAudioActive + 1) % kSwitchAudioBufferCount;
            ++owner->m_switchAudioEnqueued;
        }
        else
        {
            brls::Logger::warning("MgbaNativeCore: audoutAppendAudioOutBuffer failed rc={:#x}", rc);
        }
        return;
    }
#endif

    const bool isGbFamily = owner->m_gameEntry.platform == static_cast<int>(
        beiklive::enums::EmuPlatform::EmuGB) ||
        owner->m_gameEntry.platform == static_cast<int>(
        beiklive::enums::EmuPlatform::EmuGBC);
    const size_t callbackSamples = isGbFamily ? kGbAudioSamples : kSwitchAudioSamples;
    std::vector<int16_t> samples(callbackSamples * 2);
    if (!owner->m_audioOutputEnabled)
    {
        blip_clear(left);
        blip_clear(right);
        return;
    }

    const int produced = blip_read_samples(left, samples.data(), callbackSamples, true);
    if (produced <= 0)
        return;
    blip_read_samples(right, samples.data() + 1, produced, true);

    if (!owner->m_loggedFirstAudio)
    {
        brls::Logger::info("MgbaNativeCore: first stream audio batch produced {} frames", produced);
        owner->m_loggedFirstAudio = true;
    }

    samples.resize(static_cast<size_t>(produced) * 2);
    owner->applyAudioLowPass(samples);

    std::lock_guard<std::mutex> lock(owner->m_audioMutex);
    if (owner->m_audioBuffer.size() + samples.size() > kAudioBufferCapacity)
    {
        const size_t overflow = (owner->m_audioBuffer.size() + samples.size()) - kAudioBufferCapacity;
        if (overflow >= owner->m_audioBuffer.size())
            owner->m_audioBuffer.clear();
        else
            owner->m_audioBuffer.erase(owner->m_audioBuffer.begin(),
                                       owner->m_audioBuffer.begin() + static_cast<std::ptrdiff_t>(overflow));
    }
    owner->m_audioBuffer.insert(owner->m_audioBuffer.end(), samples.begin(), samples.end());
}

void MgbaNativeCore::drainMgbaAudio()
{
    if (!m_core)
        return;

    blip_t* left = m_core->getAudioChannel(m_core, 0);
    blip_t* right = m_core->getAudioChannel(m_core, 1);
    if (!left || !right)
        return;

    const int available = blip_samples_avail(left);
    if (available <= 0)
    {
        if (m_audioProbeFrames < 60)
        {
            ++m_audioProbeFrames;
            ++m_audioSilentProbeFrames;
            if (m_audioProbeFrames == 60)
                brls::Logger::warning("MgbaNativeCore: no audio samples produced in first {} frames", m_audioSilentProbeFrames);
        }
        return;
    }

    std::vector<int16_t> samples(static_cast<size_t>(available) * 2);
    const int produced = blip_read_samples(left, samples.data(), available, true);
    if (produced <= 0)
        return;
    blip_read_samples(right, samples.data() + 1, produced, true);

    if (!m_loggedFirstAudio)
    {
        brls::Logger::info("MgbaNativeCore: first audio batch produced {} frames (available={}, silentProbeFrames={})",
                           produced, available, m_audioSilentProbeFrames);
        m_loggedFirstAudio = true;
    }
    if (m_audioProbeFrames < 60)
        ++m_audioProbeFrames;

    samples.resize(static_cast<size_t>(produced) * 2);
    applyAudioLowPass(samples);

    std::lock_guard<std::mutex> lock(m_audioMutex);
    if (m_audioBuffer.size() + samples.size() > kAudioBufferCapacity)
    {
        const size_t overflow = (m_audioBuffer.size() + samples.size()) - kAudioBufferCapacity;
        if (overflow >= m_audioBuffer.size())
            m_audioBuffer.clear();
        else
            m_audioBuffer.erase(m_audioBuffer.begin(), m_audioBuffer.begin() + static_cast<std::ptrdiff_t>(overflow));
    }
    m_audioBuffer.insert(m_audioBuffer.end(), samples.begin(), samples.end());
}

void MgbaNativeCore::captureVideoFrame()
{
    if (!m_core)
        return;

    const void* pixels = nullptr;
    size_t stride = 0;
    m_core->getPixels(m_core, &pixels, &stride);
    if (!pixels || m_width == 0 || m_height == 0)
        return;

    std::lock_guard<std::mutex> lock(m_videoMutex);
    m_videoFrame.width = m_width;
    m_videoFrame.height = m_height;
    m_videoFrame.pixels.resize(static_cast<size_t>(m_width) * m_height);

    const auto* src = static_cast<const color_t*>(pixels);
    for (unsigned y = 0; y < m_height; ++y)
    {
        const color_t* srcRow = src + static_cast<size_t>(y) * stride;
        uint32_t* dstRow = m_videoFrame.pixels.data() + static_cast<size_t>(y) * m_width;
        for (unsigned x = 0; x < m_width; ++x)
            dstRow[x] = nativeColorToRgba(srcRow[x]);
    }
}

void MgbaNativeCore::updateKeys()
{
    if (!m_core)
        return;

    const auto& b = m_buttons[0];
    uint32_t keys = 0;
    keys |= b[RETRO_DEVICE_ID_JOYPAD_A] ? (1u << 0) : 0;
    keys |= b[RETRO_DEVICE_ID_JOYPAD_B] ? (1u << 1) : 0;
    keys |= b[RETRO_DEVICE_ID_JOYPAD_SELECT] ? (1u << 2) : 0;
    keys |= b[RETRO_DEVICE_ID_JOYPAD_START] ? (1u << 3) : 0;
    keys |= b[RETRO_DEVICE_ID_JOYPAD_RIGHT] ? (1u << 4) : 0;
    keys |= b[RETRO_DEVICE_ID_JOYPAD_LEFT] ? (1u << 5) : 0;
    keys |= b[RETRO_DEVICE_ID_JOYPAD_UP] ? (1u << 6) : 0;
    keys |= b[RETRO_DEVICE_ID_JOYPAD_DOWN] ? (1u << 7) : 0;
    keys |= b[RETRO_DEVICE_ID_JOYPAD_R] ? (1u << 8) : 0;
    keys |= b[RETRO_DEVICE_ID_JOYPAD_L] ? (1u << 9) : 0;

    if (keys != m_keyMask)
    {
        m_keyMask = keys;
        m_core->setKeys(m_core, keys);
    }
}

void MgbaNativeCore::releaseCore()
{
    {
        std::lock_guard<std::mutex> lock(m_audioMutex);
        m_audioBuffer.clear();
    }
    {
        std::lock_guard<std::mutex> lock(m_videoMutex);
        m_videoFrame = {};
    }
    m_videoBuffer.clear();
    m_sramSnapshot.clear();
    m_width = 0;
    m_height = 0;
    m_bufferWidth = 0;
    m_bufferHeight = 0;
    m_fps = 60.0;
    m_sampleRate = kDefaultSampleRate;
    m_keyMask = 0;
    m_loggedFirstAudio = false;
    m_audioProbeFrames = 0;
    m_audioSilentProbeFrames = 0;

    if (!m_core)
    {
        m_audioStreamEnabled = false;
        m_audioStream = {};
        releaseFallbackCheatDevice();
        shutdownNativeAudioOutput();
        return;
    }

    if (m_coreInitialized)
    {
        if (m_core->setAVStream)
            m_core->setAVStream(m_core, nullptr);
        m_audioStreamEnabled = false;
        m_audioStream = {};
        if (m_fallbackCheatDevice)
        {
            clearMgbaCheats(m_fallbackCheatDevice);
            releaseFallbackCheatDevice();
        }

        m_core->unloadROM(m_core);
        if (m_configInitialized)
            mCoreConfigDeinit(&m_core->config);
        m_core->deinit(m_core);
    }
    else
    {
        releaseFallbackCheatDevice();
        std::free(m_core);
    }
    m_core = nullptr;
    m_coreInitialized = false;
    m_configInitialized = false;
    shutdownNativeAudioOutput();
}

std::string MgbaNativeCore::saveFilePath() const
{
    std::string dir = m_gameEntry.savePath;
    if (dir.empty())
        dir = beiklive::path::savePath();
    if (dir.empty() || m_gameEntry.path.empty())
        return {};

    return dir + beiklive::path::SPLIT_CHAR +
           beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".sav";
}

void MgbaNativeCore::sampleLux(GBALuminanceSource* source)
{
    auto* native = reinterpret_cast<NativeLuminanceSource*>(source);
    if (native && native->owner)
        native->owner->updateLuxLevel();
}

uint8_t MgbaNativeCore::readLux(GBALuminanceSource* source)
{
    auto* native = reinterpret_cast<NativeLuminanceSource*>(source);
    if (!native || !native->owner)
        return 0xFF - 0x16;
    return static_cast<uint8_t>(0xFF - native->owner->m_luxLevel);
}

void MgbaNativeCore::sampleRtc(mRTCSource* source)
{
    (void)source;
}

time_t MgbaNativeCore::readRtcUnixTime(mRTCSource* source)
{
    (void)source;
    return std::time(nullptr);
}

} // namespace beiklive::mgba_native
