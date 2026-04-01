#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include "game/control/GameInputManager.hpp"
#include "game/mgba/GameRun.hpp"
#include "game/render/GameRenderer.hpp"

#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>

namespace beiklive
{
    // 游戏视图，负责游戏的渲染显示，输入处理等功能
    class GameView : public brls::Box
    {
        public:
            GameView(beiklive::GameEntry gameData);
            ~GameView();

            void onFocusGained() override;
            void onFocusLost() override;

            void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override;

        private:
            bool _brls_inputLocked = false; ///< 输入锁定状态
            beiklive::GameEntry m_gameEntry; ///< 游戏条目数据

            // ---- libretro 核心 -----------------------------------------------
            beiklive::gba::CoreMgba* m_gba_core = nullptr; ///< mgba 核心实例

            // ---- 渲染器 -------------------------------------------------------
            beiklive::GameRenderer m_renderer; ///< 游戏帧渲染器（GL 纹理 + 直接绘制）
            bool m_rendererReady = false;      ///< 渲染器是否已初始化

            // ---- 最新视频帧（游戏线程写，UI 线程读）--------------------------
            mutable std::mutex          m_frameMutex;
            LibretroLoader::VideoFrame  m_pendingFrame; ///< 等待上传的最新帧
            bool                        m_frameReady = false; ///< 是否有新帧待上传

            // ---- 游戏线程 -----------------------------------------------------
            std::thread       m_gameThread;
            std::atomic<bool> m_running{false}; ///< 游戏线程运行标志

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
    };
}
