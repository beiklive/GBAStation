#include "CoreFceumm.hpp"
#include "core/CoreUtils.hpp"

namespace beiklive::fceumm {

CoreFceumm::~CoreFceumm()
{
    if (m_ready) Cleanup();
}

bool CoreFceumm::SetupGame(beiklive::GameEntry GameEntry)
{
    m_gameEntry = std::move(GameEntry);
    _initConfig();
    if (_loadCore())
    {
        if (_loadRom(m_gameEntry.path))
        {
            m_core.reset();
            _loadSram();
            _loadCheats();
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
    m_core.deinitCore();
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
    m_core.setSaveDirectory(m_gameEntry.savePath.empty() ? beiklive::path::savePath() : m_gameEntry.savePath);
}

bool CoreFceumm::_loadCore()
{
    if (!m_core.load(m_coreType))
    {
        brls::Logger::error("Failed to static-load {} core", m_coreName);
        return false;
    }
    if (!m_core.initCore())
    {
        brls::Logger::error("retro_init() failed for {}", m_coreName);
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
        m_core.unload();
        return false;
    }
    if (!std::filesystem::exists(romPath))
    {
        brls::Logger::error("ROM not found: {}", romPath);
        m_core.unload();
        return false;
    }
    if (!m_core.loadGame(romPath))
    {
        brls::Logger::error("retro_load_game() failed for: {}", romPath);
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
    return core_utils::loadSram(m_core, m_gameEntry.savePath,
        beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path));
}

bool CoreFceumm::_saveSram()
{
    return core_utils::saveSram(m_core, m_gameEntry.savePath,
        beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path));
}

bool CoreFceumm::_loadCheats()
{
    bool ok = core_utils::loadCheats(m_core, m_gameEntry.cheatPath, m_cheats);
    if (ok && !m_cheats.empty())
        brls::Logger::info("CoreFceumm: loaded {} cheats from {}", m_cheats.size(), m_gameEntry.cheatPath);
    return ok;
}

void CoreFceumm::_updateCheats()
{
    core_utils::updateCheats(m_core, m_cheats);
}

} // namespace beiklive::fceumm
