#include "CoreSnes9x.hpp"
#include "core/CoreUtils.hpp"

namespace beiklive::snes9x {

CoreSnes9x::~CoreSnes9x()
{
    if (m_ready) Cleanup();
}

bool CoreSnes9x::SetupGame(beiklive::GameEntry GameEntry)
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

void CoreSnes9x::Cleanup()
{
    if (!m_ready) return;
    m_ready = false;
    _saveSram();
    m_core.unloadGame();
    m_core.deinitCore();
}

void CoreSnes9x::RunFrame()
{
    if (!m_ready) return;
    m_core.run();
}

void CoreSnes9x::Reset()
{
    if (!m_ready) return;
    m_core.reset();
}

bool CoreSnes9x::Serialize(std::vector<uint8_t>& outBuf) const
{
    if (!m_ready) return false;
    size_t sz = m_core.serializeSize();
    if (sz == 0) return false;
    outBuf.resize(sz);
    return m_core.serialize(outBuf.data(), sz);
}

bool CoreSnes9x::Unserialize(const std::vector<uint8_t>& buf)
{
    if (!m_ready || buf.empty()) return false;
    return m_core.unserialize(buf.data(), buf.size());
}

void CoreSnes9x::_initConfig()
{
    beiklive::ConfigManager* cfg = beiklive::SettingManager;
    if (!cfg) return;

    using CV = beiklive::ConfigValue;
    cfg->SetDefault("core.snes9x_overclock",          CV(std::string("disabled")));
    cfg->SetDefault("core.snes9x_frameskip",          CV(std::string("0")));
    cfg->SetDefault("core.snes9x_region",             CV(std::string("auto")));
    cfg->SetDefault("core.snes9x_aspect",             CV(std::string("4:3")));
    cfg->SetDefault("core.snes9x_blargg",             CV(std::string("disabled")));
    cfg->SetDefault("core.snes9x_layer_1",            CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_layer_2",            CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_layer_3",            CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_layer_4",            CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_layer_5",            CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_sndchan_1",          CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_sndchan_2",          CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_sndchan_3",          CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_sndchan_4",          CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_sndchan_5",          CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_sndchan_6",          CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_sndchan_7",          CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_sndchan_8",          CV(std::string("enabled")));
    cfg->SetDefault("core.snes9x_reduce_sprite_flicker", CV(std::string("disabled")));
    cfg->Save();

    m_core.setConfigManager(cfg);
    m_core.setSystemDirectory(beiklive::path::biosPath());
    m_core.setSaveDirectory(m_gameEntry.savePath.empty() ? beiklive::path::savePath() : m_gameEntry.savePath);
}

bool CoreSnes9x::_loadCore()
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
    // Keep input setup explicit across Snes9x variants.  Relying on the
    // core's default port device can leave a port as RETRO_DEVICE_NONE.
    m_core.setControllerPortDevice(0, RETRO_DEVICE_JOYPAD);
    m_core.setControllerPortDevice(1, RETRO_DEVICE_JOYPAD);
    return true;
}

bool CoreSnes9x::_loadRom(const std::string &romPath)
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

bool CoreSnes9x::_loadSram()
{
    const std::string savePath = m_gameEntry.savePath.empty()
        ? beiklive::tools::defaultGameSavePath(m_gameEntry.platform, m_gameEntry.path)
        : m_gameEntry.savePath;
    return core_utils::loadSram(m_core, savePath,
        beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path));
}

bool CoreSnes9x::_saveSram()
{
    const std::string savePath = m_gameEntry.savePath.empty()
        ? beiklive::tools::defaultGameSavePath(m_gameEntry.platform, m_gameEntry.path)
        : m_gameEntry.savePath;
    return core_utils::saveSram(m_core, savePath,
        beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path));
}

bool CoreSnes9x::_loadCheats()
{
    bool ok = core_utils::loadCheats(m_core, m_gameEntry.cheatPath, m_cheats);
    if (ok && !m_cheats.empty())
        brls::Logger::info("CoreSnes9x: loaded {} cheats from {}", m_cheats.size(), m_gameEntry.cheatPath);
    return ok;
}

void CoreSnes9x::_updateCheats()
{
    core_utils::updateCheats(m_core, m_cheats);
}

} // namespace beiklive::snes9x
