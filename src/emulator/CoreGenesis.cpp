#include "CoreGenesis.hpp"

namespace beiklive::genesis {

CoreGenesis::~CoreGenesis()
{
    if (m_ready) Cleanup();
}

bool CoreGenesis::SetupGame(beiklive::GameEntry GameEntry)
{
    m_gameEntry = std::move(GameEntry);
    if (_loadCore())
    {
        _initConfig();
        if (_loadRom(m_gameEntry.path))
        {
            _loadSram();
            _loadCheats();
            m_core.reset();
            m_ready = true;
            return true;
        }
    }
    return false;
}

void CoreGenesis::Cleanup()
{
    if (!m_ready) return;
    m_ready = false;
    _saveSram();
    m_core.unloadGame();
    // 不调用 deinitCore() — PicoDrive 不支持重复 retro_deinit/retro_init
}

void CoreGenesis::RunFrame()
{
    if (!m_ready) return;
    m_core.run();
}

void CoreGenesis::Reset()
{
    if (!m_ready) return;
    m_core.reset();
}

bool CoreGenesis::Serialize(std::vector<uint8_t>& outBuf) const
{
    if (!m_ready) return false;
    size_t sz = m_core.serializeSize();
    if (sz == 0) return false;
    outBuf.resize(sz);
    return m_core.serialize(outBuf.data(), sz);
}

bool CoreGenesis::Unserialize(const std::vector<uint8_t>& buf)
{
    if (!m_ready || buf.empty()) return false;
    return m_core.unserialize(buf.data(), buf.size());
}

void CoreGenesis::_initConfig()
{
    beiklive::ConfigManager* cfg = beiklive::SettingManager;
    if (!cfg) return;

    using CV = beiklive::ConfigValue;
    cfg->SetDefault("core.picodrive_region",              CV(std::string("Auto")));
    cfg->SetDefault("core.picodrive_sound_output",        CV(std::string("stereo")));
    cfg->SetDefault("core.picodrive_frameskip",           CV(std::string("0")));
    cfg->SetDefault("core.picodrive_render",              CV(std::string("single field")));
    cfg->SetDefault("core.picodrive_aspect",              CV(std::string("PAR")));
    cfg->SetDefault("core.picodrive_overclock",           CV(std::string("disabled")));
    cfg->SetDefault("core.picodrive_audio_filter",        CV(std::string("low-pass")));
    cfg->SetDefault("core.picodrive_lowpass_range",       CV(std::string("60")));
    cfg->Save();

    m_core.setConfigManager(cfg);
    m_core.setSystemDirectory(beiklive::path::biosPath());
}

bool CoreGenesis::_loadCore()
{
    if (!m_core.load(beiklive::CoreType::Genesis))
    {
        brls::Logger::error("Failed to static-load PicoDrive core");
        return false;
    }
    if (!m_core.initCore())
    {
        brls::Logger::error("retro_init() failed for PicoDrive");
        m_core.unload();
        return false;
    }
    return true;
}

bool CoreGenesis::_loadRom(const std::string &romPath)
{
    if (romPath.empty())
    {
        brls::Logger::error("ROM path is empty");
        m_core.deinitCore();
        m_core.unload();
        return false;
    }
    if (!std::filesystem::exists(romPath))
    {
        brls::Logger::error("ROM not found: {}", romPath);
        m_core.deinitCore();
        m_core.unload();
        return false;
    }
    if (!m_core.loadGame(romPath))
    {
        brls::Logger::error("retro_load_game() failed for: {}", romPath);
        m_core.deinitCore();
        m_core.unload();
        return false;
    }
    brls::Logger::info("ROM loaded: {} ({}x{} @ {:.2f} fps)",
                       romPath,
                       m_core.gameWidth(), m_core.gameHeight(),
                       m_core.fps());
    return true;
}

bool CoreGenesis::_loadSram()
{
    size_t sz = m_core.getMemorySize(RETRO_MEMORY_SAVE_RAM);
    if (sz == 0)
    {
        brls::Logger::info("CoreGenesis: no SRAM region in core, skipping SRAM load");
        return true;
    }

    std::string path = m_gameEntry.savePath + beiklive::path::SPLIT_CHAR
                     + beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".sav";
    if (path.empty()) return true;

    if (!std::filesystem::exists(path))
    {
        brls::Logger::info("CoreGenesis: no SRAM file found at {}, skipping", path);
        return true;
    }

    std::ifstream f(path, std::ios::binary);
    if (!f) return true;

    std::vector<uint8_t> buf(sz, 0);
    f.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(sz));
    std::streamsize got = f.gcount();

    void *sramPtr = m_core.getMemoryData(RETRO_MEMORY_SAVE_RAM);
    if (sramPtr)
    {
        std::memcpy(sramPtr, buf.data(), static_cast<size_t>(got));
        brls::Logger::debug("CoreGenesis: SRAM loaded from {} ({} bytes)", path, got);
    }
    return true;
}

bool CoreGenesis::_saveSram()
{
    size_t sz = m_core.getMemorySize(RETRO_MEMORY_SAVE_RAM);
    if (sz == 0) return true;

    const void *sramPtr = m_core.getMemoryData(RETRO_MEMORY_SAVE_RAM);
    if (!sramPtr) return true;

    std::string path = m_gameEntry.savePath + beiklive::path::SPLIT_CHAR
                     + beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".sav";
    if (path.empty()) return true;

    std::ofstream f(path, std::ios::binary);
    if (!f) return true;

    f.write(reinterpret_cast<const char *>(sramPtr), static_cast<std::streamsize>(sz));
    if (!f) return true;

    brls::Logger::info("CoreGenesis: SRAM saved to {} ({} bytes)", path, sz);
    return true;
}

bool CoreGenesis::_loadCheats()
{
    std::string path = m_gameEntry.cheatPath;
    if (path.empty()) return true;

    m_cheats = beiklive::parseChtFile(path);
    if (m_cheats.empty()) return true;

    brls::Logger::info("CoreGenesis: loaded {} cheats from {}", m_cheats.size(), path);

    m_core.cheatReset();
    for (size_t i = 0; i < m_cheats.size(); ++i)
    {
        if (m_cheats[i].enabled)
            m_core.cheatSet(static_cast<unsigned>(i), true, m_cheats[i].code);
    }
    return true;
}

void CoreGenesis::_updateCheats()
{
    m_core.cheatReset();
    for (size_t i = 0; i < m_cheats.size(); ++i)
    {
        if (m_cheats[i].enabled)
            m_core.cheatSet(static_cast<unsigned>(i), true, m_cheats[i].code);
    }
}

} // namespace beiklive::genesis
