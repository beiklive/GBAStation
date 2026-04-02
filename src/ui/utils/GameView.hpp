#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include "core/GameTimer.hpp"
#include "game/control/GameInputManager.hpp"
#include "game/mgba/GameRun.hpp"
#include "game/render/GameRenderer.hpp"
#include "ui/utils/GameOverlayRenderer.hpp"

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace beiklive
{
    class GameMenuView; // 前置声明

    // 游戏视图，负责游戏的渲染显示，输入处理等功能
    class GameView : public brls::Box
    {
        public:
            GameView(beiklive::GameEntry gameData);
            ~GameView();

            void onFocusGained() override;
            void onFocusLost() override;

            void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override;

            /// 设置关联的游戏菜单视图（由 GamePage 调用）
            void setGameMenuView(GameMenuView* menuView) { m_gameMenuView = menuView; }

        private:
            // ---- 游戏线程常量 ------------------------------------------------
            static constexpr double   MAX_REASONABLE_FPS      = 240.0;  ///< 核心上报 FPS 的安全上限
            static constexpr double   SPIN_GUARD_SEC           = 0.002;  ///< 每帧自旋等待预算（秒）
            static constexpr double   FPS_UPDATE_INTERVAL      = 1.0;   ///< FPS 计数器更新间隔（秒）
            static constexpr int      PLAY_TIME_SAVE_INTERVAL  = 180;   ///< 游戏时长保存间隔（秒，3分钟） TODO: 后续改为设置项读取
            static constexpr unsigned REWIND_BUFFER_SIZE       = 600;   ///< 倒带缓冲区最大帧数（约10秒） TODO: 后续改为设置项读取
            static constexpr unsigned REWIND_STEP              = 2;     ///< 每次倒带弹出的帧数         TODO: 后续改为设置项读取
            static constexpr unsigned FF_MULTIPLIER            = 4;     ///< 快进倍率（每迭代运行的帧数） TODO: 后续改为设置项读取

            bool _brls_inputLocked = false; ///< 输入锁定状态
            beiklive::GameEntry m_gameEntry; ///< 游戏条目数据

            // ---- libretro 核心 -----------------------------------------------
            beiklive::gba::CoreMgba* m_gba_core = nullptr; ///< mgba 核心实例

            // ---- 渲染器 -------------------------------------------------------
            beiklive::GameRenderer m_renderer; ///< 游戏帧渲染器（GL 纹理 + 直接绘制）
            bool m_rendererReady = false;      ///< 渲染器是否已初始化

            // ---- 画面模式 ----------------------------------------------------
            beiklive::ScreenMode m_screenMode = beiklive::ScreenMode::Fit; ///< 当前画面缩放模式

            // ---- 最新视频帧（游戏线程写，UI 线程读）--------------------------
            mutable std::mutex          m_frameMutex;
            LibretroLoader::VideoFrame  m_pendingFrame; ///< 等待上传的最新帧
            bool                        m_frameReady = false; ///< 是否有新帧待上传

            // ---- 游戏线程 -----------------------------------------------------
            std::thread       m_gameThread;
            std::atomic<bool> m_running{false}; ///< 游戏线程运行标志

            // ---- FPS 统计（游戏线程写，UI 线程读）-----------------------------
            mutable std::mutex m_fpsMutex;
            unsigned m_fpsFrameCount = 0;
            float    m_currentFps    = 0.0f;
            std::chrono::steady_clock::time_point m_fpsLastTime;

            // ---- 倒带缓冲区（仅游戏线程访问，无需互斥锁）--------------------
            std::deque<std::vector<uint8_t>> m_rewindBuffer; ///< 倒带状态环形缓冲区（最新帧在队首）

            // ---- 菜单视图（由 GamePage 注入）---------------------------------
            GameMenuView* m_gameMenuView = nullptr;

            // ---- 辅助方法 ----------------------------------------------------
            void _registerGameInput();
            void _registerGameRuntime();

            /// 启动游戏主循环线程
            void _startGameThread();

            /// 停止游戏主循环线程并等待退出
            void _stopGameThread();

            /// 游戏主循环函数（在独立线程中执行）
            void _gameLoop();

            /// 将待上传帧数据提交到 GPU（在 UI/draw 线程调用）
            void _uploadPendingFrame();

            /// 在视图上绘制状态覆盖层（FPS/快进/倒带/暂停/静音）
            void _drawOverlays(NVGcontext* vg, float x, float y, float w, float h);

            // ---- 游戏循环内部分段辅助方法（仅在游戏线程中调用）--------------

            /// 将当前核心状态序列化并存入倒带缓冲区（超出上限时自动淘汰最旧帧）
            void _saveRewindState();

            /// 执行一次倒带操作：从缓冲区弹出 REWIND_STEP 帧并反序列化，返回是否成功
            bool _stepRewind();

            /// 执行正常或快进帧：保存倒带状态，运行核心（ff=true 时运行 FF_MULTIPLIER 帧）
            /// @return 本次迭代实际运行的帧数
            unsigned _stepFrame(bool ff);

            /// 从核心取出最新视频帧并暂存，等待 UI 线程上传 GPU
            void _captureVideoFrame();

            /// 推送音频数据到 AudioManager（ff=true 时限制推送量，避免缓冲区溢出）
            void _pushFrameAudio(bool ff, unsigned framesRan);

            /// 更新 FPS 统计计数器（游戏线程侧）
            void _updateFpsStats(unsigned framesRan,
                                 std::chrono::steady_clock::time_point& lastTime,
                                 unsigned& counter);

            /// 更新游戏时长记录（每 PLAY_TIME_SAVE_INTERVAL 秒精确累加一次）
            void _updatePlayTime(std::chrono::steady_clock::time_point& lastTime);

            /// 帧率限制器：使用 nextFrameTarget 累加模式，严格对齐目标帧率，防止漂移
            void _throttleFrameRate(bool ff,
                                    std::chrono::steady_clock::time_point& nextTarget,
                                    std::chrono::nanoseconds frameDurNs,
                                    std::chrono::nanoseconds spinGuardNs);
    };
}

