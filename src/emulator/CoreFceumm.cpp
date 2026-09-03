#include "CoreFceumm.hpp"
#include "core/CoreUtils.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace beiklive::fceumm {
namespace {

constexpr const char* kFdsBiosFileName = "disksys.rom";
constexpr const char* kFdsBiosMd5 = "ca30b50f880eb660a320674ed365ef7a";
constexpr uintmax_t kFdsBiosSize = 8192;

uint32_t md5LeftRotate(uint32_t value, uint32_t bits)
{
    return (value << bits) | (value >> (32 - bits));
}

std::string md5Hex(const std::vector<uint8_t>& input)
{
    static constexpr uint32_t s[] = {
        7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
    };
    static constexpr uint32_t k[] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
    };

    std::vector<uint8_t> msg = input;
    const uint64_t bitLen = static_cast<uint64_t>(msg.size()) * 8u;
    msg.push_back(0x80);
    while ((msg.size() % 64) != 56)
        msg.push_back(0);
    for (int i = 0; i < 8; ++i)
        msg.push_back(static_cast<uint8_t>((bitLen >> (8 * i)) & 0xff));

    uint32_t a0 = 0x67452301;
    uint32_t b0 = 0xefcdab89;
    uint32_t c0 = 0x98badcfe;
    uint32_t d0 = 0x10325476;

    for (size_t offset = 0; offset < msg.size(); offset += 64)
    {
        uint32_t m[16];
        for (int i = 0; i < 16; ++i)
        {
            const size_t j = offset + static_cast<size_t>(i) * 4;
            m[i] = static_cast<uint32_t>(msg[j]) |
                   (static_cast<uint32_t>(msg[j + 1]) << 8) |
                   (static_cast<uint32_t>(msg[j + 2]) << 16) |
                   (static_cast<uint32_t>(msg[j + 3]) << 24);
        }

        uint32_t a = a0;
        uint32_t b = b0;
        uint32_t c = c0;
        uint32_t d = d0;

        for (uint32_t i = 0; i < 64; ++i)
        {
            uint32_t f = 0;
            uint32_t g = 0;
            if (i < 16)
            {
                f = (b & c) | ((~b) & d);
                g = i;
            }
            else if (i < 32)
            {
                f = (d & b) | ((~d) & c);
                g = (5 * i + 1) % 16;
            }
            else if (i < 48)
            {
                f = b ^ c ^ d;
                g = (3 * i + 5) % 16;
            }
            else
            {
                f = c ^ (b | (~d));
                g = (7 * i) % 16;
            }

            const uint32_t temp = d;
            d = c;
            c = b;
            b = b + md5LeftRotate(a + f + k[i] + m[g], s[i]);
            a = temp;
        }

        a0 += a;
        b0 += b;
        c0 += c;
        d0 += d;
    }

    std::array<uint32_t, 4> digest = {a0, b0, c0, d0};
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (uint32_t word : digest)
        for (int i = 0; i < 4; ++i)
            out << std::setw(2) << ((word >> (8 * i)) & 0xff);
    return out.str();
}

bool readFileBytes(const std::filesystem::path& path, std::vector<uint8_t>& out)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;
    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    if (size < 0)
        return false;
    file.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    return out.empty() || static_cast<bool>(file.read(reinterpret_cast<char*>(out.data()), size));
}

} // namespace

CoreFceumm::~CoreFceumm()
{
    if (m_ready) Cleanup();
}

bool CoreFceumm::SetupGame(beiklive::GameEntry GameEntry)
{
    m_gameEntry = std::move(GameEntry);
    m_lastError.clear();
    const std::string& runtimePath = m_gameEntry.runtimePath.empty() ? m_gameEntry.path : m_gameEntry.runtimePath;
    if (!_checkFdsBios(runtimePath))
        return false;
    _initConfig();
    if (_loadCore())
    {
        if (_loadRom(runtimePath))
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
        m_lastError = "FC 核心加载失败";
        return false;
    }
    if (!m_core.initCore())
    {
        brls::Logger::error("retro_init() failed for {}", m_coreName);
        m_lastError = "FC 核心初始化失败";
        m_core.unload();
        return false;
    }
    m_core.setControllerPortDevice(0, RETRO_DEVICE_JOYPAD);
    m_core.setControllerPortDevice(1, RETRO_DEVICE_JOYPAD);
    return true;
}

bool CoreFceumm::_loadRom(const std::string &romPath)
{
    if (romPath.empty())
    {
        brls::Logger::error("ROM path is empty");
        m_lastError = "ROM 路径为空";
        m_core.unload();
        return false;
    }
    if (!std::filesystem::exists(romPath))
    {
        brls::Logger::error("ROM not found: {}", romPath);
        m_lastError = "ROM 文件不存在:\n" + romPath;
        m_core.unload();
        return false;
    }
    if (!m_core.loadGame(romPath))
    {
        brls::Logger::error("retro_load_game() failed for: {}", romPath);
        if (m_lastError.empty())
            m_lastError = "FC/FDS 游戏加载失败:\n" + romPath;
        m_core.unload();
        return false;
    }
    brls::Logger::info("ROM loaded: {} ({}x{} @ {:.2f} fps)",
                       romPath,
                       m_core.gameWidth(), m_core.gameHeight(),
                       m_core.fps());
    return true;
}

bool CoreFceumm::_checkFdsBios(const std::string& romPath)
{
    if (beiklive::tools::getFileExtension(romPath) != "fds")
        return true;

    const std::filesystem::path biosPath =
        std::filesystem::path(beiklive::path::biosPath()) / kFdsBiosFileName;
    const std::string displayPath = "/GBAStation/bios/disksys.rom";

    std::error_code ec;
    if (!std::filesystem::exists(biosPath, ec) || ec)
    {
        m_lastError =
            "FDS BIOS 缺失\n\n"
            "请将文件放到:\n" + displayPath + "\n\n"
            "文件名: disksys.rom\n"
            "大小: 8192 bytes\n"
            "MD5: ca30b50f880eb660a320674ed365ef7a";
        brls::Logger::error("CoreFceumm: missing FDS BIOS: {}", biosPath.string());
        return false;
    }

    const auto fileSize = std::filesystem::file_size(biosPath, ec);
    if (ec || fileSize != kFdsBiosSize)
    {
        m_lastError =
            "FDS BIOS 文件大小不正确\n\n"
            "路径: " + displayPath + "\n"
            "需要大小: 8192 bytes\n"
            "当前大小: " + (ec ? std::string("无法读取") : std::to_string(fileSize) + " bytes") + "\n"
            "MD5 应为: ca30b50f880eb660a320674ed365ef7a";
        brls::Logger::error("CoreFceumm: invalid FDS BIOS size path={} size={}",
                            biosPath.string(), ec ? 0 : fileSize);
        return false;
    }

    std::vector<uint8_t> bytes;
    if (!readFileBytes(biosPath, bytes))
    {
        m_lastError =
            "FDS BIOS 无法读取\n\n"
            "路径: " + displayPath + "\n"
            "请确认文件名为 disksys.rom，且没有被占用或损坏";
        brls::Logger::error("CoreFceumm: failed to read FDS BIOS: {}", biosPath.string());
        return false;
    }

    const std::string actualMd5 = md5Hex(bytes);
    if (actualMd5 != kFdsBiosMd5)
    {
        m_lastError =
            "FDS BIOS 校验不正确\n\n"
            "路径: " + displayPath + "\n"
            "需要 MD5: ca30b50f880eb660a320674ed365ef7a\n"
            "当前 MD5: " + actualMd5 + "\n\n"
            "请更换正确的 disksys.rom";
        brls::Logger::error("CoreFceumm: invalid FDS BIOS md5 path={} md5={}",
                            biosPath.string(), actualMd5);
        return false;
    }

    return true;
}

bool CoreFceumm::_loadSram()
{
    const std::string savePath = m_gameEntry.savePath.empty()
        ? beiklive::tools::defaultGameSavePath(m_gameEntry.platform, m_gameEntry.path)
        : m_gameEntry.savePath;
    return core_utils::loadSram(m_core, savePath,
        beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path));
}

bool CoreFceumm::_saveSram()
{
    const std::string savePath = m_gameEntry.savePath.empty()
        ? beiklive::tools::defaultGameSavePath(m_gameEntry.platform, m_gameEntry.path)
        : m_gameEntry.savePath;
    return core_utils::saveSram(m_core, savePath,
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
