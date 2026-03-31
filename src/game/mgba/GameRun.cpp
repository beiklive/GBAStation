#include "GameRun.hpp"

namespace beiklive::gba
{
    bool CoreMgba::SetupGame(beiklive::GameEntry GameEntry)
    {
        m_gameEntry = std::move(GameEntry);
        if (_loadCore(beiklive::GetCorePath(m_gameEntry.platform)))
        {
            if (_loadRom(m_gameEntry.path))
            {
                _loadSram();
                _loadRtc();
                _loadCheats();
                return true;
            }
        }
        return false;
    }

    bool CoreMgba::_loadCore(const std::string &corePath)
    {
        // 初始化核心
        if (!m_core.load(corePath))
        {
            brls::Logger::error("Failed to load libretro core from: {}", corePath);
            return false;
        }

        // 检查核心状态
        if (!m_core.initCore())
        {
            brls::Logger::error("retro_init() failed");
            m_core.unload();
            return false;
        }
        return true;
    }

    bool CoreMgba::_loadRom(const std::string &romPath)
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
    bool CoreMgba::_loadSram()
    {
        size_t sz = m_core.getMemorySize(RETRO_MEMORY_SAVE_RAM);
        if (sz == 0)
        {
            brls::Logger::info("CoreMgba: no SRAM region in core, skipping SRAM load");
            return true; // 核心无 SRAM 区域，非错误
        }

        std::string path = m_gameEntry.savePath;
        if (path.empty())
        {
            brls::Logger::warning("CoreMgba: no save path specified for game {}, skipping SRAM load", m_gameEntry.title);
            return true; // 没有指定存档路径，非错误
        }

        if (!std::filesystem::exists(path))
        {
            brls::Logger::info("CoreMgba: no SRAM file found at {}, skipping SRAM load", path);
            return true; // 没有找到存档文件，非错误
        }

        std::ifstream f(path, std::ios::binary);
        if (!f)
        {
            brls::Logger::warning("CoreMgba: failed to open SRAM file: {}, skipping SRAM load", path);
            return true; // 无法打开存档文件，非错误
        }
        std::vector<uint8_t> buf(sz, 0);
        f.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(sz));
        std::streamsize got = f.gcount();
        void *sramPtr = m_core.getMemoryData(RETRO_MEMORY_SAVE_RAM);

        // 加载存档到核心的 SRAM 区域
        if (sramPtr)
        {
            std::memcpy(sramPtr, buf.data(), static_cast<size_t>(got));
            brls::Logger::debug("CoreMgba: SRAM loaded from {} ({} bytes)", path, got);
            return true;
        }
        else
        {
            brls::Logger::warning("CoreMgba: SRAM pointer is null, cannot load SRAM");
            return true; // 核心 SRAM 指针无效，不影响核心运行
        }
    }
    bool CoreMgba::_loadRtc()
    {
        size_t sz = m_core.getMemorySize(RETRO_MEMORY_RTC);
        if (sz == 0)
        {
            brls::Logger::debug("CoreMgba: no RTC region in core, skipping RTC load");
            return true; // 核心无 RTC 区域，非错误
        }
        // GBMBCRTCSaveBuffer 内存布局（mGBA）：
        //   偏移  0: sec         (uint32)
        //   偏移  4: min         (uint32)
        //   偏移  8: hour        (uint32)
        //   偏移 12: days        (uint32)
        //   偏移 16: daysHi      (uint32)
        //   偏移 20: latchedSec  (uint32)
        //   偏移 24: latchedMin  (uint32)
        //   偏移 28: latchedHour (uint32)
        //   偏移 32: latchedDays (uint32)
        //   偏移 36: latchedDaysHi (uint32)
        //   偏移 40: unixTime    (uint64)  ← GBMBCRTCRead 将其读入 rtcLastLatch
        static constexpr size_t k_rtcUnixTimeOffset = 10 * sizeof(uint32_t); // = 40

        // 使用系统时间

        if (sz >= k_rtcUnixTimeOffset + sizeof(uint64_t))
        {
            void *rtcPtr = m_core.getMemoryData(RETRO_MEMORY_RTC);
            if (rtcPtr)
            {
                // 使用 std::chrono 获取 Unix 时间
                auto now = std::chrono::system_clock::now();
                auto duration = now.time_since_epoch();

                uint64_t nowUnix = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
                // 将当前 Unix 时间写入 RTC 内存区域的 unixTime 字段
                std::memcpy(static_cast<uint8_t *>(rtcPtr) + k_rtcUnixTimeOffset,
                            &nowUnix, sizeof(uint64_t));

                brls::Logger::debug("CoreMgba: RTC seeded unixTime={}", nowUnix);
            }
            return true;
        }

        return false;
    }
    bool CoreMgba::_loadCheats()
    {
        std::string path = m_gameEntry.cheatPath;
        if (path.empty())
        {
            brls::Logger::warning("CoreMgba: no cheat path specified for game {}, skipping cheat load", m_gameEntry.title);
            return true; // 没有指定金手指路径，非错误
        }

        m_cheats = beiklive::parseChtFile(path);

        if (m_cheats.empty())
        {
            brls::Logger::warning("CoreMgba: no cheats found in {}, skipping cheat load", path);
            return true; // 没有找到金手指，非错误
        }
        brls::Logger::info("CoreMgba: loaded {} cheats from {}", m_cheats.size(), path);

        // 将金手指注册到核心，只注册激活的金手指
        m_core.cheatReset();
        for (size_t i = 0; i < m_cheats.size(); ++i)
        {
            if (m_cheats[i].enabled)
            {
                m_core.cheatSet(static_cast<unsigned>(i), true, m_cheats[i].code);
            }
        }
        return true;
    }

    void CoreMgba::_updateCheats()
    {
        m_core.cheatReset();
        // 重新注册所有金手指，保持启用状态不变
        for (size_t i = 0; i < m_cheats.size(); ++i)
        {
            if (m_cheats[i].enabled)
            {
                m_core.cheatSet(static_cast<unsigned>(i), true, m_cheats[i].code);
            }
        }
        std::string path = m_gameEntry.cheatPath;

        if (!path.empty())
        {
            beiklive::saveChtFile(path, m_cheats);
        }
    }

    bool CoreMgba::_saveSram()
    {
        size_t sz = m_core.getMemorySize(RETRO_MEMORY_SAVE_RAM);
        if (sz == 0)
        {
            return true; // 核心无 SRAM 区域，非错误
        }
        const void *sramPtr = m_core.getMemoryData(RETRO_MEMORY_SAVE_RAM);
        if (!sramPtr)
        {
            brls::Logger::warning("CoreMgba: SRAM pointer is null, cannot save SRAM");
            return true; // 核心 SRAM 指针无效，不影响核心运行
        }

        std::string path = m_gameEntry.savePath;
        if (path.empty())
        {
            brls::Logger::warning("CoreMgba: no save path specified for game {}, cannot save SRAM", m_gameEntry.title);
            return true; // 没有指定存档路径，非错误
        }

        std::ofstream f(path, std::ios::binary);
        if (!f)
        {
            brls::Logger::warning("CoreMgba: failed to open SRAM file: {}, cannot save SRAM", path);
            return true; // 无法打开存档文件，非错误
        }

        f.write(reinterpret_cast<const char *>(sramPtr), static_cast<std::streamsize>(sz));
        if (!f)
        {
            brls::Logger::warning("CoreMgba: failed to write SRAM file: {}", path);
            return true; // 写入存档文件失败，非错误
        }

        brls::Logger::info("CoreMgba: SRAM saved to {} ({} bytes)", path, sz);
        return true;
    }
    bool CoreMgba::_saveRtc()
    {
        return false;
    }
}