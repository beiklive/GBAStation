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
    class GameMenuView;         // 前置声明
    class RewindSelectorView;   // 前置声明

    /// 倒带帧：包含核心序列化状态和可选的缩略图（RGB565 格式）
    struct RewindFrame {
        std::vector<uint8_t>  state;  ///< 核心序列化状态（~128KB）
        std::vector<uint16_t> thumb;  ///< RGB565 缩略图（120×80，可能为空）

        static constexpr unsigned THUMB_W = 120; ///< 缩略图宽度（像素）
        static constexpr unsigned THUMB_H = 80;  ///< 缩略图高度（像素）
    };

    /// 可视化倒带快照：包含缓冲区索引（用于恢复）、秒数（用于显示）和缩略图
    struct RewindThumbSnapshot {
        int bufferIdx;                ///< m_rewindBuffer 中的索引（0=最新帧）
        int secondsAgo;               ///< 距当前的秒数（用于显示 "-X秒"）
        std::vector<uint16_t> thumb;  ///< RGB565 缩略图数据（可能为空）
    };

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

            /// 设置关联的倒带选择视图（由 GamePage 调用）
            void setRewindSelectorView(RewindSelectorView* view) { m_rewindSelectorView = view; }

            // ---- 即时存档公共接口 -------------------------------------------

            /// 计算即时存档文件路径（slot=0 为自动存档，slot=1~9 为手动存档）
            std::string getStatePath(int slot) const;

            /// 计算即时存档缩略图路径（存档路径 + ".png"）
            std::string getStateThumbPath(int slot) const;

            /// 检查指定槽位是否存在存档文件
            bool stateExists(int slot) const;

            // ---- 倒带缓冲区快照（在游戏暂停后由 UI 线程调用）----------------

            /// 获取当前倒带缓冲区的快照（缩略图 + 帧索引），供 RewindSelectorView 使用。
            /// 自动根据保存间隔计算 item 数量（每往前 1 秒对应一个 item）。
            /// @return RewindThumbSnapshot 列表，最旧帧在前、最新帧在后
            std::vector<RewindThumbSnapshot>
            snapshotRewindThumbs() const;

            /// 恢复指定倒带帧（弹出缓冲区到该帧并反序列化），供 RewindSelectorView 调用。
            /// 需在游戏线程中调用（通过 GameSignal 传递请求）。
            void requestRestoreRewindFrame(int frameIndex);

            /// 请求更新金手指文件路径（UI线程调用）
            void requestCheatPathUpdate(const std::string& path);

            /// 着色器开关（UI线程调用）
            void _onShaderToggle(bool on);
            /// 着色器路径变更（UI线程调用）
            void _onShaderPathChange(const std::string& path);
            /// 画面模式变更（UI线程调用）
            void _onDisplayModeChange(const std::string& mode);
            /// 纹理过滤变更（UI线程调用）
            void _onFilterChange(const std::string& filter);

        private:
            // ---- 游戏线程常量 ------------------------------------------------
            static constexpr double   MAX_REASONABLE_FPS      = 240.0;  ///< 核心上报 FPS 的安全上限
            static constexpr double   SPIN_GUARD_SEC           = 0.002;  ///< 每帧自旋等待预算（秒）
            static constexpr double   FPS_UPDATE_INTERVAL      = 1.0;   ///< FPS 计数器更新间隔（秒）
            static constexpr unsigned REWIND_STEP              = 2;     ///< 每次倒带弹出的帧数
            static constexpr unsigned FF_MULTIPLIER            = 4;     ///< 快进倍率（每迭代运行的帧数）

            bool _brls_inputLocked = false; ///< 输入锁定状态
            beiklive::GameEntry m_gameEntry; ///< 游戏条目数据

            // ---- 倒带设置（从配置中读取，游戏启动时初始化）------------------
            int  m_rewindSaveInterval = 1;     ///< 每 N 帧保存一次倒带状态
            unsigned m_rewindBufferSize = 600; ///< 倒带缓冲区最大条目数（从配置读取）
            bool m_rewindShowUI       = false;  ///< 是否启用可视化倒带界面

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

            // ---- 倒带缓冲区（游戏线程写，暂停时 UI 线程可读）------------------
            mutable std::mutex         m_rewindMutex;  ///< 保护倒带缓冲区的互斥锁
            std::deque<RewindFrame>    m_rewindBuffer; ///< 倒带帧环形缓冲区（最新帧在队首）
            unsigned                   m_rewindFrameCounter = 0; ///< 帧计数器（用于间隔保存控制）

            // ---- 视图（由 GamePage 注入）-------------------------------------
            GameMenuView*       m_gameMenuView       = nullptr;
            RewindSelectorView* m_rewindSelectorView = nullptr;

            // ---- 杂项 --------------------------------------------------------
            std::string m_playTimeTempPath;    ///< 时长临时文件路径，退出时合并到 GameDB
            int m_cachedThumbCompression = 0;  ///< 缓存缩略图压缩模式，避免每帧读取配置
            std::chrono::steady_clock::time_point m_playStartTime; ///< 计时起点

            // ---- SRAM 自动落盘 -------------------------------------------------
            uint32_t    m_sramLastCRC   = 0;    ///< 上次检测的 SRAM CRC32
            bool        m_sramDirty     = false; ///< SRAM 是否有未保存变更
            std::chrono::steady_clock::time_point m_sramLastCheck; ///< 上次 CRC 检查时间
            std::chrono::steady_clock::time_point m_sramDirtyTime; ///< 标记 dirty 的时间
            static constexpr double SRAM_CHECK_INTERVAL = 1.0;  ///< CRC 检查间隔（秒）
            static constexpr double SRAM_FLUSH_DELAY    = 2.0;  ///< dirty 后延迟写盘（秒）

            static uint32_t _crc32Sram(const void* data, size_t size);
            void _checkAndAutoSaveSram();

            // ---- 辅助方法 ----------------------------------------------------
            void _registerGameInput();
            void _registerGameRuntime();

            /// 初始化游戏时长追踪（启动时检查并合并遗留的临时文件）
            void _initPlayTimeTracking();

            /// 将当前累加时长写入临时文件并提交到 GameDB（退出时调用）
            void _saveAndCommitPlayTime();

            /// 将当前累加时长写入临时文件（暂停/存档点调用）
            void _savePlayTimeCheckpoint();

            /// 自动存档计时起点
            std::chrono::steady_clock::time_point m_autoSaveTimer;

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

            /// 帧率限制器：使用 nextFrameTarget 累加模式，严格对齐目标帧率，防止漂移
            void _throttleFrameRate(bool ff,
                                    std::chrono::steady_clock::time_point& nextTarget,
                                    std::chrono::nanoseconds frameDurNs,
                                    std::chrono::nanoseconds spinGuardNs);

            // ---- 即时存档（仅在游戏线程中调用）------------------------------

            /// 序列化核心状态到文件并保存缩略图（slot=0 为自动存档）
            void _doSaveState(int slot);

            /// 从文件反序列化核心状态（slot=0 为自动存档）
            void _doLoadState(int slot);

            // ---- 缩略图工具（仅在游戏线程中调用）----------------------------

            /// 将 RGBA8888 视频帧降采样并转换为 RGB565 缩略图
            std::vector<uint16_t> _downsampleToRGB565(
                const std::vector<uint32_t>& src,
                unsigned srcW, unsigned srcH,
                unsigned dstW, unsigned dstH);
    };
}
