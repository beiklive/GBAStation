#include "CoreGenesis.hpp"
#include "core/CoreUtils.hpp"

namespace beiklive::genesis {

CoreGenesis::~CoreGenesis()
{
    if (m_ready) Cleanup();
}

bool CoreGenesis::SetupGame(beiklive::GameEntry GameEntry)
{
    brls::Logger::debug("[CoreGenesis] SetupGame: path={}", GameEntry.path);
    m_gameEntry = std::move(GameEntry);
    if (_loadCore())
    {
        _initConfig();
        if (_loadRom(m_gameEntry.path))
        {
            m_core.reset();
            _loadSram();
            _loadCheats();
            m_ready = true;
            brls::Logger::debug("[CoreGenesis] SetupGame OK");
            return true;
        }
    }
    brls::Logger::error("[CoreGenesis] SetupGame failed");
    return false;
}

void CoreGenesis::Cleanup()
{
    brls::Logger::debug("[CoreGenesis] Cleanup");
    if (!m_ready) return;
    m_ready = false;
    _saveSram();
    m_core.unloadGame();
    m_core.deinitCore();
}

bool CoreGenesis::_loadCore()
{
    brls::Logger::debug("[CoreGenesis] _loadCore: loading Genesis Plus GX");
    if (!m_core.load(beiklive::CoreType::Genesis))
    {
        brls::Logger::error("[CoreGenesis] _loadCore: load failed");
        return false;
    }
    if (!m_core.initCore())
    {
        brls::Logger::error("[CoreGenesis] _loadCore: initCore failed");
        m_core.unload();
        return false;
    }
    brls::Logger::debug("[CoreGenesis] _loadCore OK");
    return true;
}

bool CoreGenesis::_loadRom(const std::string &romPath)
{
    brls::Logger::debug("[CoreGenesis] _loadRom: {}", romPath);
    if (romPath.empty())
    {
        brls::Logger::error("[CoreGenesis] _loadRom: empty path");
        m_core.unload();
        return false;
    }
    if (!std::filesystem::exists(romPath))
    {
        brls::Logger::error("[CoreGenesis] _loadRom: file not found");
        m_core.unload();
        return false;
    }
    if (!m_core.loadGame(romPath))
    {
        brls::Logger::error("[CoreGenesis] _loadRom: loadGame failed");
        m_core.unload();
        return false;
    }
    brls::Logger::info("ROM loaded: {} ({}x{} @ {:.2f} fps)",
                       romPath,
                       m_core.gameWidth(), m_core.gameHeight(),
                       m_core.fps());
    return true;
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
    cfg->SetDefault("core.genesis_plus_gx_system_hw",           CV(std::string("auto")));
    cfg->SetDefault("core.genesis_plus_gx_region_detect",       CV(std::string("auto")));
    cfg->SetDefault("core.genesis_plus_gx_vdp_mode",            CV(std::string("auto")));
    cfg->SetDefault("core.genesis_plus_gx_bios",                CV(std::string("disabled")));
    cfg->SetDefault("core.genesis_plus_gx_system_bram",         CV(std::string("per bios")));
    cfg->SetDefault("core.genesis_plus_gx_cart_bram",           CV(std::string("per game")));
    cfg->SetDefault("core.genesis_plus_gx_add_on",              CV(std::string("auto")));
    cfg->SetDefault("core.genesis_plus_gx_aspect_ratio",        CV(std::string("auto")));
    cfg->SetDefault("core.genesis_plus_gx_overscan",            CV(std::string("disabled")));
    cfg->SetDefault("core.genesis_plus_gx_gg_extra",            CV(std::string("disabled")));
    cfg->SetDefault("core.genesis_plus_gx_blargg_ntsc_filter",  CV(std::string("disabled")));
    cfg->SetDefault("core.genesis_plus_gx_lcd_filter",          CV(std::string("disabled")));
    cfg->SetDefault("core.genesis_plus_gx_render",              CV(std::string("single field")));
    cfg->SetDefault("core.genesis_plus_gx_frameskip",           CV(std::string("disabled")));
    cfg->SetDefault("core.genesis_plus_gx_ym2413",              CV(std::string("auto")));
    cfg->SetDefault("core.genesis_plus_gx_ym2612",              CV(std::string("auto")));
    cfg->SetDefault("core.genesis_plus_gx_sound_output",        CV(std::string("stereo")));
    cfg->SetDefault("core.genesis_plus_gx_audio_filter",        CV(std::string("low-pass")));
    cfg->SetDefault("core.genesis_plus_gx_lowpass_range",       CV(std::string("60")));
    cfg->Save();

    m_core.setConfigManager(cfg);
    m_core.setSystemDirectory(beiklive::path::biosPath());
}

bool CoreGenesis::_loadSram()
{
    return core_utils::loadSram(m_core, m_gameEntry.savePath,
        beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path));
}

bool CoreGenesis::_saveSram()
{
    return core_utils::saveSram(m_core, m_gameEntry.savePath,
        beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path));
}

bool CoreGenesis::_loadCheats()
{
    bool ok = core_utils::loadCheats(m_core, m_gameEntry.cheatPath, m_cheats);
    if (ok && !m_cheats.empty())
        brls::Logger::info("[CoreGenesis] loaded {} cheats from {}", m_cheats.size(), m_gameEntry.cheatPath);
    return ok;
}

void CoreGenesis::_updateCheats()
{
    core_utils::updateCheats(m_core, m_cheats);
}

} // namespace beiklive::genesis
