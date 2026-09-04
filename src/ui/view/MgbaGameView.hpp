#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include "core/GameTimer.hpp"
#include "ui/view/GameViewBase.hpp"
#include "game/control/GameInputManager.hpp"
#include "emulator/IEmulatorCore.hpp"
#include "emulator/IEmulatorStopRequest.hpp"
#include "emulator/IEmulatorTouchInput.hpp"
#include "emulator/IEmulatorVideoFrameMode.hpp"
#include "emulator/IEmulatorVideoTexture.hpp"
#include "game/render/GameRenderer.hpp"
#include "ui/utils/GameOverlayRenderer.hpp"

#include <atomic>
#include <array>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace beiklive
{
    class GameMenuView;         // 前置声明
    class RewindSelectorView;   // 前置声明
    class IEmulatorAudioOutput;

    // mGBA 游戏视图，负责 GBA/GBC/GB 的 native mGBA 运行、渲染、音频和输入
    class MgbaGameView : public GameViewBase
    {
        public:
            MgbaGameView(beiklive::GameEntry gameData);
            ~MgbaGameView();

            void onFocusGained() override;
            void onFocusLost() override;

            void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override;

            /// 退出游戏前显式停止游戏线程并释放模拟器核心，避免 Activity 切换动画期间卡顿。
            void prepareExitCleanup();

            /// 设置关联的游戏菜单视图（由 GamePage 调用）
            void setGameMenuView(GameMenuView* menuView) override;

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

            /// 预览指定倒带帧的缩略图画面（UI线程调用，不改变核心状态）。
            void requestPreviewRewindFrame(int frameIndex);

            /// 恢复指定倒带帧（弹出缓冲区到该帧并反序列化），供 RewindSelectorView 调用。
            /// 需在游戏线程中调用（通过 GameSignal 传递请求）。
            void requestRestoreRewindFrame(int frameIndex);

            /// 请求更新金手指文件路径（UI线程调用）
            void requestCheatPathUpdate(const std::string& path);
            void applyCheatsUpdate(const std::vector<CheatEntry>& cheats);

            /// 着色器开关（UI线程调用）
            void _onShaderToggle(bool on);
            /// 着色器路径变更（UI线程调用）
            void _onShaderPathChange(const std::string& path);
            /// 画面模式变更（UI线程调用）
            void _onDisplayModeChange(const std::string& mode);
            /// 整数倍缩放倍率变更（UI线程调用）
            void _onIntegerScaleChange(float scale);
            /// 自定义缩放/偏移变更（UI线程调用）
            void _onCustomValuesChanged(float x, float y, float scale);
            /// 遮罩开关变更（UI线程调用）
            void _onOverlayToggle(bool enabled);
            /// 遮罩路径变更（UI线程调用）
            void _onOverlayPathChange(const std::string& path);
            /// 纹理过滤变更（UI线程调用）
            void _onFilterChange(const std::string& filter);
            /// 配置变更通知（UI线程调用，通知核心重读变量）
            void _onConfigUpdated();
            /// 获取着色器参数列表
            std::vector<ShaderParamInfo> _getShaderParams() const;
            /// 设置着色器参数
            void _setShaderParam(const std::string& name, float val);

            void _scheduleDisplaySettingsSave();
            void _flushDisplaySettings();

        private:
            // ---- 游戏线程常量 ------------------------------------------------
            static constexpr double   MAX_REASONABLE_FPS      = 240.0;  ///< 核心上报 FPS 的安全上限
            static constexpr double   SPIN_GUARD_SEC           = 0.002;  ///< 每帧自旋等待预算（秒）
            static constexpr double   FPS_UPDATE_INTERVAL      = 1.0;   ///< FPS 计数器更新间隔（秒）
            static constexpr double   PLAYTIME_SUSPEND_GAP_SEC = 5.0;   ///< 超过该间隔视为系统挂起/后台恢复
            static constexpr unsigned REWIND_STEP              = 2;     ///< 每次倒带弹出的帧数

            bool _brls_inputLocked = false; ///< 输入锁定状态
            beiklive::GameEntry m_gameEntry; ///< 游戏条目数据

            // ---- 快进倍率（从配置读取，支持慢动作）--------------------------
            float m_ffMultiplier = 2.0f;     ///< 快进倍率（>1=加速, <1=慢动作）
            float m_ffSlowAccum  = 0.0f;     ///< 慢动作帧累加器
            bool  m_ffMute       = false;    ///< 快进时静音（缓存配置避免每帧读取）
            bool  m_rewindEnabled = false;   ///< 是否启用倒带状态缓存
            int  m_rewindSaveInterval = 1;     ///< 每 N 帧保存一次倒带状态
            unsigned m_rewindBufferSize = 600; ///< 倒带缓冲区最大条目数（从配置读取）
            bool m_rewindShowUI       = false;  ///< 是否启用可视化倒带界面

            // ---- libretro 核心 -----------------------------------------------
            IEmulatorCore* m_core = nullptr;
            IEmulatorAudioOutput* m_coreAudioOutput = nullptr; ///< mGBA/Switch 原生音频直出缓存指针

            // ---- 渲染器 -------------------------------------------------------
            beiklive::GameRenderer m_renderer; ///< 游戏帧渲染器（GL 纹理 + 直接绘制）
            beiklive::GameRenderer m_ndsTopRenderer; ///< NDS 着色器模式：上屏独立渲染器
            beiklive::GameRenderer m_ndsBottomRenderer; ///< NDS 着色器模式：下屏独立渲染器
            bool m_rendererReady = false;      ///< 渲染器是否已初始化
            bool m_ndsSplitShaderRenderer = false; ///< NDS 是否使用上下屏拆分着色器渲染

            // ---- 画面模式 ----------------------------------------------------
            beiklive::ScreenMode m_screenMode = beiklive::ScreenMode::Fit; ///< 当前画面缩放模式
            beiklive::DisplayRect m_gameDrawRect; ///< 当前游戏画面在视图中的绘制区域
            beiklive::DisplayRect m_ndsTouchRect; ///< NDS 下屏在视图中的绘制区域
            std::string m_ndsLayout = "vertical"; ///< NDS 双屏布局
            std::string m_ndsScreenOrientation = "0"; ///< NDS 屏幕旋转角度（0/90/180/270）
            size_t m_displaySettingsSaveDelayId = 0;
            bool m_displaySettingsSavePending = false;
            bool m_ndsIntegerScale = true; ///< NDS 是否自动最大整数倍缩放
            bool m_ndsScreensSwapped = false; ///< NDS 是否交换上下屏显示位置
            bool m_ndsTouchActive = false; ///< NDS 原始触摸轮询是否处于按下状态
            bool m_ndsVirtualPointerMode = false; ///< NDS 虚拟指针模式是否开启
            bool m_ndsVirtualPointerClickHeld = false; ///< NDS 虚拟指针点击键是否按住
            bool m_ndsVirtualPointerTouchDown = false; ///< NDS 虚拟指针是否已向核心提交触摸按下
            float m_ndsVirtualPointerX = 128.f; ///< NDS 虚拟指针 X（0..255）
            float m_ndsVirtualPointerY = 96.f; ///< NDS 虚拟指针 Y（0..191）
            std::chrono::steady_clock::time_point m_ndsVirtualPointerLastUpdate; ///< 虚拟指针移动计时

            // ---- 遮罩 --------------------------------------------------------
            brls::Image* m_overlayImage = nullptr; ///< 遮罩图片

            // ---- 最新视频帧（游戏线程写，UI 线程读）--------------------------
            mutable std::mutex          m_frameMutex;
            LibretroLoader::VideoFrame  m_pendingFrame; ///< 等待上传的最新帧
            LibretroLoader::VideoFrame  m_lastRawFrame; ///< 最近一次核心原始帧，供暂停时重建渲染器后重传
            LibretroLoader::VideoFrame  m_ndsTopUploadFrame; ///< NDS 着色器模式复用上传帧
            LibretroLoader::VideoFrame  m_ndsBottomUploadFrame; ///< NDS 着色器模式复用上传帧
            bool                        m_frameReady = false; ///< 是否有新帧待上传
            bool                        m_hasLastRawFrame = false; ///< 是否已缓存可重排的原始帧

            // ---- 音频排空缓冲（复用避免每帧分配）-----------------------------
            std::vector<int16_t> m_audioDrainBuf;
            bool m_audioOutputSuppressed = false; ///< 静音/快进静音/倒带静音状态是否已清过缓冲
            bool m_loggedFirstAudioPush = false; ///< 诊断：是否已记录第一次音频推送
            unsigned m_audioEmptyLogCount = 0; ///< 诊断：启动阶段 DrainAudio 为空次数
            float m_audioSpeed = 1.0f; ///< 当前通用 AudioManager 倍速，按 MgbaGameView 实例隔离
            std::atomic<bool> m_firstFrameUploaded{false}; ///< mGBA 首帧上传前延后音频，避免先出声后出画

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
            std::string m_playTimeTempPath;    ///< 时长容灾检查点路径，退出时合并到 GameDB
            int m_cachedThumbCompression = 0;  ///< 缓存缩略图压缩模式，避免每帧读取配置
            std::chrono::steady_clock::time_point m_playStartTime; ///< 最近一次真实游玩时长累计时刻
            std::chrono::steady_clock::time_point m_nextPlayTimeCheckpoint; ///< 下一次后台容灾检查点
            double m_playTimeFraction = 0.0;   ///< 未满 1 秒的真实游玩时长累积
#ifdef __SWITCH__
            AppletHookCookie m_appletHookCookie{};
            std::atomic<int> m_switchFocusState{AppletFocusState_InFocus};
            bool m_switchAppletHooked = false;
            bool m_switchBackgroundPaused = false;
            bool m_switchPauseBeforeBackground = false;
#endif

            // ---- 连发（Turbo）状态 -------------------------------------------
            std::atomic<bool> m_turboAheld{false};  ///< Turbo A 按键是否按住
            std::atomic<bool> m_turboBheld{false};  ///< Turbo B 按键是否按住
            bool m_turboAon = false;                ///< Turbo A 当前帧是否按下
            bool m_turboBon = false;                ///< Turbo B 当前帧是否按下
            int  m_turboFrameCount = 0;             ///< Turbo 帧计数器
            int  m_turboToggleInterval = 3;         ///< Turbo 切换间隔帧数（60fps 下 10Hz）

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
            bool _useNdsSplitShader() const;
            bool _useNdsAcceleratedTexture() const;
            void _syncNdsVideoFrameMode();
            void _applySavedShaderParams(beiklive::GameRenderer& renderer) const;
            bool _initGameRenderers(unsigned gw, unsigned gh, const std::string& shaderPath);
            void _clearGameViewBackground(float x, float y, float w, float h,
                                          float windowScale, int windowW, int windowH);
            bool _drawNdsAcceleratedTexture(const beiklive::DisplayRect& rect,
                                            float windowScale, int windowW, int windowH);
            void _uploadNdsSplitShaderFrame(const LibretroLoader::VideoFrame& frame);
            struct NdsScreenDrawRect {
                bool topScreen = true;
                beiklive::DisplayRect rect;
            };
            std::vector<NdsScreenDrawRect> _computeNdsScreenDrawRects(
                const beiklive::DisplayRect& layoutRect) const;
            bool _mapNdsSourceScreen(bool layoutTopScreen) const;
            beiklive::DisplayRect _rotateNdsScreenRect(
                const beiklive::DisplayRect& screenRect,
                const beiklive::DisplayRect& layoutRect,
                const beiklive::DisplayRect& orientedRect) const;
            std::array<float, 8> _ndsOrientationUv() const;
            beiklive::DisplayRect _unrotateNdsRect(
                const beiklive::DisplayRect& orientedRect, const beiklive::DisplayRect& layoutRect) const;
            bool _mapNdsPointToUnrotated(float x, float y,
                                         const beiklive::DisplayRect& orientedRect,
                                         const beiklive::DisplayRect& layoutRect,
                                         float& outX, float& outY) const;

            /// 初始化游戏时长追踪（启动时检查并合并遗留的临时文件）
            void _initPlayTimeTracking();

            /// 将当前累加时长写入临时文件并提交到 GameDB（退出时调用）
            void _saveAndCommitPlayTime();

            /// 将当前累加时长写入临时文件（暂停/存档点调用）
            void _savePlayTimeCheckpoint();

            /// 按真实运行时间累计游玩时长（忽略暂停，快进不放大，卡顿不少计）
            void _accumulatePlayTime();

#ifdef __SWITCH__
            static void _appletHook(AppletHookType hook, void* param);
            void _registerAppletHook();
            void _unregisterAppletHook();
            void _updateSwitchFocusState();
#endif

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

            /// 使用最近的原始帧立即重新上传，避免暂停时重建渲染器后画面变空
            void _requestLastFrameUpload();

            /// 在视图上绘制状态覆盖层（FPS/快进/倒带/暂停/静音）
            void _drawOverlays(NVGcontext* vg, float x, float y, float w, float h);
            void _registerTouchInput();
            void _pollNdsTouchInput();
            void _submitTouchPoint(float x, float y, bool down);
            void _toggleNdsVirtualPointerMode();
            void _setNdsVirtualPointerClick(bool down);
            void _updateNdsVirtualPointer();
            void _drawNdsVirtualPointer(NVGcontext* vg);
            void _releaseNdsVirtualPointerTouch();

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

            /// 按当前 NDS 双屏布局重排视频帧
            LibretroLoader::VideoFrame _layoutNdsFrame(const LibretroLoader::VideoFrame& frame) const;

            /// 根据当前 NDS 布局和绘制区域更新下屏触摸区域
            void _updateNdsTouchRect(const beiklive::DisplayRect& rect);

            /// 根据当前核心和配置初始化 AudioManager。
            void _initAudioForCore(double fps, double sampleRate);

            /// 等待 UI 音效释放共享 audout，避免游戏音频启动时抢完成事件。
            void _waitForUiAudioPlayer();

            /// 清空音频缓冲并按设置为下一段音频加短淡入。
            void _flushAudioForTransition();
            void _pauseAudioForTransition();
            void _resumeAudioForTransition();

            /// 推送音频数据到 AudioManager（ff=true 时限制推送量，避免缓冲区溢出）
            void _pushFrameAudio(bool ff);

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

            /// 保存当前游戏画面截图到存档目录
            void _doScreenshot();

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
