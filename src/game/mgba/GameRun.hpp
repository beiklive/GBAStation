#pragma once

#include "core/common.h"
#include "game/retro/LibretroLoader.hpp"

namespace beiklive::gba
{

    class CoreMgba
    {
    public:
        CoreMgba() = default;
        ~CoreMgba();

        /// 加载核心与 ROM，执行 SRAM/RTC/金手指初始化，返回是否成功
        bool SetupGame(beiklive::GameEntry GameEntry);

        /// 清理核心资源（保存 SRAM/RTC，卸载游戏与核心）
        void Cleanup();

        // ---- 游戏运行 -------------------------------------------------------

        /// 执行一帧游戏逻辑（调用 retro_run()）
        void RunFrame();

        /// 重置核心（相当于按复位键）
        void Reset();

        // ---- 状态序列化（快速存读档）----------------------------------------

        /// 序列化核心状态到缓冲区，返回是否成功
        bool Serialize(std::vector<uint8_t>& outBuf) const;

        /// 从缓冲区反序列化核心状态，返回是否成功
        bool Unserialize(const std::vector<uint8_t>& buf);

        // ---- 视频帧 ----------------------------------------------------------

        /// 获取最新视频帧（线程安全）
        LibretroLoader::VideoFrame GetVideoFrame() const { return m_core.getVideoFrame(); }

        // ---- 音频 -----------------------------------------------------------

        /// 取出音频缓冲区数据（线程安全）
        bool DrainAudio(std::vector<int16_t>& out) { return m_core.drainAudio(out); }

        // ---- 输入 -----------------------------------------------------------

        /// 设置按钮状态（RETRO_DEVICE_ID_JOYPAD_*）
        void SetButtonState(unsigned id, bool pressed) { m_core.setButtonState(id, pressed); }

        // ---- 几何信息 --------------------------------------------------------

        unsigned GameWidth()  const { return m_core.gameWidth();  }
        unsigned GameHeight() const { return m_core.gameHeight(); }
        double   Fps()        const { return m_core.fps();        }

        // ---- 快进 -----------------------------------------------------------

        void SetFastForwarding(bool ff) { m_core.setFastForwarding(ff); }

        // ---- 金手指管理（外部调用）------------------------------------------

        const std::vector<beiklive::CheatEntry>& GetCheats() const { return m_cheats; }
        void UpdateCheats() { _updateCheats(); }

        // ---- 运行状态查询 ---------------------------------------------------

        bool IsReady() const { return m_ready; }

    private:
        beiklive::GameEntry m_gameEntry; ///< 游戏条目数据，包含路径、标题等信息
        beiklive::LibretroLoader m_core; ///< libretro 核心封装
        std::vector<beiklive::CheatEntry> m_cheats;
        bool m_ready = false; ///< SetupGame 成功后为 true

        // ---- 内部初始化 -----------------------------------------------------
        bool _loadCore(const std::string &corePath);
        bool _loadRom(const std::string &romPath);
        void _initConfig();   ///< 向核心注册 mgba 默认配置项

        bool _loadSram();
        bool _loadRtc();
        bool _loadCheats();
        void _updateCheats();

        bool _saveSram();
        bool _saveRtc();
    };
} // namespace beiklive
