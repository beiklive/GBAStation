#include "CorePicoDrive.hpp"
#include "core/CoreUtils.hpp"

namespace beiklive::picodrive {

CorePicoDrive::~CorePicoDrive()
{
    if (m_ready) Cleanup();
}

bool CorePicoDrive::SetupGame(beiklive::GameEntry GameEntry)
{
    brls::Logger::debug("[CorePicoDrive] SetupGame: path={}", GameEntry.path);
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
            brls::Logger::debug("[CorePicoDrive] SetupGame OK");
            return true;
        }
    }
    brls::Logger::error("[CorePicoDrive] SetupGame failed");
    return false;
}

void CorePicoDrive::Cleanup()
{
    brls::Logger::debug("[CorePicoDrive] Cleanup");
    if (!m_ready) return;
    m_ready = false;
    _saveSram();
    m_core.unloadGame();
}

bool CorePicoDrive::_loadCore()
{
    brls::Logger::debug("[CorePicoDrive] _loadCore: loading PicoDrive");
    if (!m_core.load(beiklive::CoreType::Genesis))
    {
        brls::Logger::error("[CorePicoDrive] _loadCore: load failed");
        return false;
    }
    if (!m_core.initCore())
    {
        brls::Logger::error("[CorePicoDrive] _loadCore: initCore failed");
        m_core.unload();
        return false;
    }
    brls::Logger::debug("[CorePicoDrive] _loadCore OK");
    return true;
}

bool CorePicoDrive::_loadRom(const std::string &romPath)
{
    brls::Logger::debug("[CorePicoDrive] _loadRom: {}", romPath);
    if (romPath.empty())
    {
        brls::Logger::error("[CorePicoDrive] _loadRom: empty path");
        m_core.unload();
        return false;
    }
    if (!std::filesystem::exists(romPath))
    {
        brls::Logger::error("[CorePicoDrive] _loadRom: file not found");
        m_core.unload();
        return false;
    }
    if (!m_core.loadGame(romPath))
    {
        brls::Logger::error("[CorePicoDrive] _loadRom: loadGame failed");
        m_core.unload();
        return false;
    }
    brls::Logger::info("ROM loaded: {} ({}x{} @ {:.2f} fps)",
                       romPath,
                       m_core.gameWidth(), m_core.gameHeight(),
                       m_core.fps());
    return true;
}

void CorePicoDrive::RunFrame()
{
    if (!m_ready) return;
    m_core.run();
}

void CorePicoDrive::Reset()
{
    if (!m_ready) return;
    m_core.reset();
}

bool CorePicoDrive::Serialize(std::vector<uint8_t>& outBuf) const
{
    if (!m_ready) return false;
    size_t sz = m_core.serializeSize();
    if (sz == 0) return false;
    outBuf.resize(sz);
    return m_core.serialize(outBuf.data(), sz);
}

bool CorePicoDrive::Unserialize(const std::vector<uint8_t>& buf)
{
    if (!m_ready || buf.empty()) return false;
    return m_core.unserialize(buf.data(), buf.size());
}

void CorePicoDrive::_initConfig()
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

bool CorePicoDrive::_loadSram()
{
    return core_utils::loadSram(m_core, m_gameEntry.savePath,
        beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path));
}

bool CorePicoDrive::_saveSram()
{
    return core_utils::saveSram(m_core, m_gameEntry.savePath,
        beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path));
}

bool CorePicoDrive::_loadCheats()
{
    bool ok = core_utils::loadCheats(m_core, m_gameEntry.cheatPath, m_cheats);
    if (ok && !m_cheats.empty())
        brls::Logger::info("[CorePicoDrive] loaded {} cheats from {}", m_cheats.size(), m_gameEntry.cheatPath);
    return ok;
}

void CorePicoDrive::_updateCheats()
{
    core_utils::updateCheats(m_core, m_cheats);
}

} // namespace beiklive::picodrive
