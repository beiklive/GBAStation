#include "CoreFceumm.hpp"

namespace beiklive::fceumm {

CoreFceumm::~CoreFceumm()
{
    if (m_ready) Cleanup();
}

bool CoreFceumm::SetupGame(beiklive::GameEntry GameEntry)
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

void CoreFceumm::Cleanup()
{
    if (!m_ready) return;
    m_ready = false;
    _saveSram();
    m_core.unloadGame();
}

void CoreFceumm::RunFrame()
{
    if (!m_ready) return;
    m_core.run();
}

void CoreFceumm::Reset()
{
    if (!m_ready) return;
    m_core.reset();
}

bool CoreFceumm::Serialize(std::vector<uint8_t>& outBuf) const
{
    if (!m_ready) return false;
    size_t sz = m_core.serializeSize();
    if (sz == 0) return false;
    outBuf.resize(sz);
    return m_core.serialize(outBuf.data(), sz);
}

bool CoreFceumm::Unserialize(const std::vector<uint8_t>& buf)
{
    if (!m_ready || buf.empty()) return false;
    return m_core.unserialize(buf.data(), buf.size());
}

void CoreFceumm::_initConfig()
{
    beiklive::ConfigManager* cfg = beiklive::SettingManager;
    if (!cfg) return;

    using CV = beiklive::ConfigValue;
    cfg->SetDefault("core.fceumm_overclocking",          CV(std::string("disabled")));
    cfg->SetDefault("core.fceumm_region",                CV(std::string("Auto")));
    cfg->SetDefault("core.fceumm_ntsc_palette",          CV(std::string("default")));
    cfg->SetDefault("core.fceumm_palette",               CV(std::string("default")));
    cfg->SetDefault("core.fceumm_sndlowpass",            CV(std::string("disabled")));
    cfg->SetDefault("core.fceumm_swapduty",              CV(std::string("disabled")));
    cfg->SetDefault("core.fceumm_turbo_enable",          CV(std::string("None")));
    cfg->SetDefault("core.fceumm_turbo_delay",           CV(std::string("3")));
    cfg->SetDefault("core.fceumm_zapper_mode",           CV(std::string("lightgun")));
    cfg->SetDefault("core.fceumm_show_crosshair",        CV(std::string("enabled")));
    cfg->SetDefault("core.fceumm_game_genie",            CV(std::string("disabled")));
    cfg->SetDefault("core.fceumm_ramstate",              CV(std::string("fill $ff")));
    cfg->SetDefault("core.fceumm_fds_auto_insert",       CV(std::string("enabled")));
    cfg->SetDefault("core.fceumm_fastforward_sound",     CV(std::string("disabled")));
    cfg->SetDefault("core.fceumm_frameskip",             CV(std::string("0")));
    cfg->Save();

    m_core.setConfigManager(cfg);
    m_core.setSystemDirectory(beiklive::path::biosPath());
}

bool CoreFceumm::_loadCore()
{
    if (!m_core.load(beiklive::CoreType::Fceumm))
    {
        brls::Logger::error("Failed to static-load FCEUmm core");
        return false;
    }
    if (!m_core.initCore())
    {
        brls::Logger::error("retro_init() failed for FCEUmm");
        m_core.unload();
        return false;
    }
    return true;
}

bool CoreFceumm::_loadRom(const std::string &romPath)
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

bool CoreFceumm::_loadSram()
{
    size_t sz = m_core.getMemorySize(RETRO_MEMORY_SAVE_RAM);
    if (sz == 0)
    {
        brls::Logger::info("CoreFceumm: no SRAM region in core, skipping SRAM load");
        return true;
    }

    std::string path = m_gameEntry.savePath + beiklive::path::SPLIT_CHAR
                     + beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".sav";
    if (path.empty()) return true;

    if (!std::filesystem::exists(path))
    {
        brls::Logger::info("CoreFceumm: no SRAM file found at {}, skipping", path);
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
        brls::Logger::debug("CoreFceumm: SRAM loaded from {} ({} bytes)", path, got);
    }
    return true;
}

bool CoreFceumm::_saveSram()
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

    brls::Logger::info("CoreFceumm: SRAM saved to {} ({} bytes)", path, sz);
    return true;
}

bool CoreFceumm::_loadCheats()
{
    std::string path = m_gameEntry.cheatPath;
    if (path.empty()) return true;

    m_cheats = beiklive::parseChtFile(path);
    if (m_cheats.empty()) return true;

    brls::Logger::info("CoreFceumm: loaded {} cheats from {}", m_cheats.size(), path);

    m_core.cheatReset();
    for (size_t i = 0; i < m_cheats.size(); ++i)
    {
        if (m_cheats[i].enabled)
            m_core.cheatSet(static_cast<unsigned>(i), true, m_cheats[i].code);
    }
    return true;
}

void CoreFceumm::_updateCheats()
{
    m_core.cheatReset();
    for (size_t i = 0; i < m_cheats.size(); ++i)
    {
        if (m_cheats[i].enabled)
            m_core.cheatSet(static_cast<unsigned>(i), true, m_cheats[i].code);
    }
}

} // namespace beiklive::fceumm
