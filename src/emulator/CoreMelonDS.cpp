#include "CoreMelonDS.hpp"
#include "core/CoreUtils.hpp"

namespace beiklive::melonds {

CoreMelonDS::~CoreMelonDS()
{
    if (m_ready) Cleanup();
}

bool CoreMelonDS::SetupGame(beiklive::GameEntry GameEntry)
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

void CoreMelonDS::Cleanup()
{
    if (!m_ready) return;
    m_ready = false;
    _saveSram();
    m_core.unloadGame();
    m_core.deinitCore();
}

void CoreMelonDS::RunFrame()
{
    if (!m_ready) return;
    m_core.run();
}

void CoreMelonDS::Reset()
{
    if (!m_ready) return;
    m_core.reset();
}

bool CoreMelonDS::Serialize(std::vector<uint8_t>& outBuf) const
{
    if (!m_ready) return false;
    size_t sz = m_core.serializeSize();
    if (sz == 0) return false;
    outBuf.resize(sz);
    return m_core.serialize(outBuf.data(), sz);
}

bool CoreMelonDS::Unserialize(const std::vector<uint8_t>& buf)
{
    if (!m_ready || buf.empty()) return false;
    return m_core.unserialize(buf.data(), buf.size());
}

void CoreMelonDS::_initConfig()
{
    beiklive::ConfigManager* cfg = beiklive::SettingManager;
    if (!cfg) return;

    using CV = beiklive::ConfigValue;
    cfg->SetDefault("core.melonds_console_mode", CV(std::string("ds")));
    cfg->SetDefault("core.melonds_sysfile_mode", CV(std::string("builtin")));
    cfg->SetDefault("core.melonds_boot_mode", CV(std::string("direct")));
    cfg->SetDefault("core.melonds_touch_mode", CV(std::string("auto")));
    cfg->SetDefault("core.melonds_show_cursor", CV(std::string("timeout")));
    cfg->SetDefault("core.melonds_cursor_timeout", CV(std::string("3")));
    cfg->SetDefault("core.melonds_screen_layout1", CV(std::string("top-bottom")));
    cfg->SetDefault("core.melonds_screen_layout2", CV(std::string("left-right")));
    cfg->SetDefault("core.melonds_number_of_screen_layouts", CV(std::string("2")));
    cfg->SetDefault("core.melonds_screen_gap", CV(std::string("0")));
    cfg->SetDefault("core.melonds_render_mode", CV(std::string("software")));
    cfg->Save();

    m_core.setConfigManager(cfg);
    std::filesystem::path systemDir = std::filesystem::absolute(
        std::filesystem::path(beiklive::path::biosPath()) / "nds");
    std::filesystem::path saveDir = std::filesystem::absolute(m_gameEntry.savePath.empty()
        ? beiklive::path::savePath()
        : m_gameEntry.savePath);
    std::filesystem::create_directories(systemDir);
    std::filesystem::create_directories(saveDir);

    m_core.setSystemDirectory(systemDir.string());
    m_core.setSaveDirectory(saveDir.string());
}

bool CoreMelonDS::_loadCore()
{
    if (!m_core.load(beiklive::CoreType::MelonDS))
    {
        brls::Logger::error("Failed to static-load melonDS core");
        return false;
    }
    if (!m_core.initCore())
    {
        brls::Logger::error("retro_init() failed for melonDS");
        m_core.unload();
        return false;
    }
    return true;
}

bool CoreMelonDS::_loadRom(const std::string& romPath)
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
        brls::Logger::error("melonDS retro_load_game() failed for: {}", romPath);
        m_core.unload();
        return false;
    }
    brls::Logger::info("NDS ROM loaded: {} ({}x{} @ {:.2f} fps)",
                       romPath,
                       m_core.gameWidth(), m_core.gameHeight(),
                       m_core.fps());
    return true;
}

bool CoreMelonDS::_loadSram()
{
    return core_utils::loadSram(m_core, m_gameEntry.savePath,
        beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path));
}

bool CoreMelonDS::_saveSram()
{
    return core_utils::saveSram(m_core, m_gameEntry.savePath,
        beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path));
}

bool CoreMelonDS::_loadCheats()
{
    bool ok = core_utils::loadCheats(m_core, m_gameEntry.cheatPath, m_cheats);
    if (ok && !m_cheats.empty())
        brls::Logger::info("CoreMelonDS: loaded {} cheats from {}", m_cheats.size(), m_gameEntry.cheatPath);
    return ok;
}

void CoreMelonDS::_updateCheats()
{
    core_utils::updateCheats(m_core, m_cheats);
}

} // namespace beiklive::melonds
