#pragma once

#include <borealis.hpp>
#include <glad/glad.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "Control/GameInputController.hpp"
#include "Control/InputMapping.hpp"
#include "Retro/LibretroLoader.hpp"
#include "Video/DisplayConfig.hpp"
#include "Video/RenderChain.hpp"
#include "UI/Utils/GameMenu.hpp"

class GameView : public brls::Box
{
  public:
    /// 使用ROM文件路径构造，自动加载。
    explicit GameView(std::string romPath);
    GameView();
    ~GameView() override;

    void draw(NVGcontext* vg, float x, float y, float width, float height,
              brls::Style style, brls::FrameContext* ctx) override;
    void onFocusGained() override;
    void onFocusLost() override;
    void onLayout() override;

    void setGameMenu(GameMenu* menu);
    /**
     * 启用或禁用该视图的borealis输入系统。
     *
     * 启用时，屏蔽borealis UI事件分发（导航、提示、点击动画等），
     * 同时启用低层GameInputController以处理手柄热键。
     *
     * 禁用时，解除borealis输入屏蔽，暂停GameInputController。
     * 用于菜单覆盖在游戏之上时将控制权交还UI。
     *
     * 幂等操作：多次以相同值调用无副作用。
     *
     * @note 须在主（UI）线程调用。
     */
    void setGameInputEnabled(bool enabled);

    /**
     * 暂停或恢复游戏运行。
     *
     * 暂停时：游戏线程跳过核心执行和音频推送，所有游戏按键状态清零。
     * 恢复时：游戏线程继续正常运行。
     *
     * 线程安全：可在主（UI）线程调用。
     *
     * @param paused true 暂停游戏，false 恢复运行。
     */
    void setPaused(bool paused);

  private:
    std::string  m_romPath;
    std::string  m_romFileName;  ///< 从m_romPath提取的文件名（含扩展名）
    bool         m_initialized  = false;
    bool         m_coreFailed   = false;
    // ---- 游戏内菜单
    GameMenu*    m_gameMenu     = nullptr;

    // ---- libretro核心 -----------------------------------------------
    beiklive::LibretroLoader m_core;

    // ---- 游戏帧OpenGL纹理 ------------------------------------------
    GLuint   m_texture   = 0;
    unsigned m_texWidth  = 0;
    unsigned m_texHeight = 0;

    // ---- 封装GL纹理的NanoVG图像句柄 --------------------------------
    int  m_nvgImage = -1;

    // ---- 显示配置（缩放/滤波） --------------------------------------
    beiklive::DisplayConfig  m_display;
    beiklive::FilterMode     m_activeFilter = beiklive::FilterMode::Nearest;

    // ---- RenderChain（视频后处理管线） ------------------------------
    beiklive::RenderChain    m_renderChain;

    // ---- 独立游戏线程 -----------------------------------------------
    std::thread       m_gameThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_fastForward{false};

    // ---- 退出请求 ---------------------------------------------------
    std::atomic<bool> m_requestExit{false}; ///< 由游戏线程设置，draw()消费

    // ---- 退出时自动保存标志 -----------------------------------------
    /// 由菜单退出回调设置，cleanup() 中在卸载游戏前执行一次 doQuickSave(0)。
    std::atomic<bool> m_autoSaveOnExit{false};

    // ---- 打开菜单请求 -----------------------------------------------
    std::atomic<bool> m_requestOpenMenu{false}; ///< 由游戏线程设置，draw()消费

    // ---- 快速存读档 -------------------------------------------------
    /// 待存档槽号（游戏线程读取，主线程写入）。
    std::atomic<int>  m_pendingQuickSave{-1};
    /// 待读档槽号（游戏线程读取，主线程写入）。
    std::atomic<int>  m_pendingQuickLoad{-1};
    /// Current active quick-save slot (默认1，对应 .ss1).
    int  m_saveSlot = 1;

    // ---- 截图请求 ---------------------------------------------------
    /// 截图请求标志（游戏线程检测后截图）
    std::atomic<bool> m_pendingScreenshot{false};
    /// 存读档状态覆盖层消息（游戏线程写入，draw()读取）。
    mutable std::mutex  m_saveMsgMutex;
    std::string         m_saveMsg;
    std::chrono::steady_clock::time_point m_saveMsgTime;

    // ---- 按键绑定（按键映射 + 快进/倒带设置）-----------------------
    beiklive::InputMappingConfig m_inputMap;

    // ---- 低层手柄输入控制器 -----------------------------------------
    /// 手柄组合键动作，在initialize()中注册，游戏线程通过pollInput()调用。
    beiklive::GameInputController m_inputCtrl;

    // ---- 暂停状态 ---------------------------------------------------
    /// 游戏暂停标志：打开 GameMenu 时置 true，关闭菜单时置 false。
    /// 主线程写入，游戏线程读取。
    std::atomic<bool> m_paused{false};

    // ---- 输入忽略帧计数 ---------------------------------------------
    /// 菜单关闭后需忽略的游戏输入帧数。
    /// 防止关闭菜单的按键（如 B 键）被透传到游戏核心。
    /// 主线程写入，游戏线程递减并读取。
    std::atomic<int>  m_inputIgnoreFrames{0};

    // ---- 静音状态 ---------------------------------------------------
    /// 用户通过热键触发的静音开关（游戏线程读，主线程绘制覆盖层）
    std::atomic<bool> m_muted{false};

    // ---- 快进运行状态 -----------------------------------------------
    /// 手柄保持键是否当前被按下（保持模式，仅游戏线程）。
    bool m_ffPadHeld       = false;
    /// 专用切换键设置的快进切换状态（仅游戏线程）。
    bool m_ffToggled       = false;

    // ---- 倒带运行状态 -----------------------------------------------
    std::atomic<bool>            m_rewinding{false};
    /// 手柄倒带保持键是否当前被按下（仅游戏线程）。
    bool m_rewPadHeld       = false;
    /// 倒带切换状态（仅游戏线程）。
    bool m_rewindToggled    = false;
    std::deque<std::vector<uint8_t>> m_rewindBuffer; ///< 循环存档状态缓冲区
    mutable std::mutex           m_rewindMutex;

    // ---- FPS显示 ----------------------------------------------------
    bool     m_showFps           = false; ///< display.showFps
    bool     m_showFfOverlay     = true;  ///< display.showFfOverlay
    bool     m_showRewindOverlay = true;  ///< display.showRewindOverlay
    bool     m_showMuteOverlay   = true;  ///< display.showMuteOverlay
    mutable std::mutex m_fpsMutex;
    unsigned m_fpsFrameCount = 0;
    float    m_currentFps    = 0.0f;
    std::chrono::steady_clock::time_point m_fpsLastTime;

    // ---- UI输入屏蔽跟踪 --------------------------------------------
    /// 为true时表示有未解除的blockInputs()令牌（仅桌面端）。
    bool m_uiBlocked = false;

    // ---- 主线程输入快照（线程安全访问） ----------------------------
    struct InputSnapshot {
        brls::ControllerState           ctrlState{};
        std::unordered_map<int, bool>   kbState;   ///< 键盘扫描码 → 是否按下
    };
    mutable std::mutex m_inputSnapMutex;
    InputSnapshot      m_inputSnap;

    // ---- 金手指数据 --------------------------------------------------
    /// 当前已解析的金手指列表（由 loadCheats() 填充）。
    std::vector<CheatEntry> m_cheats;
    /// 当前使用的金手指文件完整路径（由 loadCheats() 填充）。
    std::string             m_cheatPath;

    // ---- Overlay (遮罩) -----------------------------------------------
    bool        m_overlayEnabled   = false; ///< display.overlay.enabled
    std::string m_overlayPath;              ///< resolved overlay PNG path for this game
    int         m_overlayNvgImage  = -1;    ///< NVG image handle for overlay texture
    /// 遮罩路径变更后需要重新加载纹理的标志（主线程写入，draw() 消费）。
    std::atomic<bool> m_overlayReloadPending{false};

    /// 加载（或重新加载）覆盖层纹理到m_overlayNvgImage。
    /// 若有旧图像则释放，失败时清空m_overlayNvgImage。
    void loadOverlayImage(NVGcontext* vg);

    // ---- 辅助方法 ---------------------------------------------------
    void initialize();
    void cleanup();

    /// 在draw()内从主线程刷新m_inputSnap。
    void refreshInputSnapshot();

    /// 向m_inputCtrl注册所有手柄热键动作，在initialize()中调用。
    void registerGamepadHotkeys();

    /// 解析存档文件（SRAM/状态/金手指）目录。
    /// @a customDir非空时返回 @a customDir/<游戏文件名> 子目录（防止存档堆积），
    /// 否则返回 @a romPath 所在目录。
    static std::string resolveSaveDir(const std::string& romPath,
                                       const std::string& customDir);

    /// 计算快速存档文件的完整路径。
    /// slot为-1时返回不含槽号后缀的基础路径（用于单游戏存档）。
    std::string quickSaveStatePath(int slot) const;

    /// 计算SRAM（.sav）文件的完整路径。
    std::string sramSavePath() const;

    /// 计算GB MBC3 RTC（.rtc）文件的完整路径。
    std::string rtcSavePath() const;

    /// 计算金手指（.cht）文件的完整路径。
    std::string cheatFilePath() const;

    /// 从磁盘加载SRAM数据到核心的存档RAM区域。
    void loadSram();

    /// 将核心存档RAM区域的SRAM数据保存到磁盘。
    void saveSram();

    /// 从磁盘加载GB MBC3 RTC数据到核心的RTC内存区域。
    void loadRtc();

    /// 将核心RTC内存区域的GB MBC3 RTC数据保存到磁盘。
    void saveRtc();

    /// 保存快速存档到 @a slot。silent为true时不显示状态覆盖层消息。
    void doQuickSave(int slot, bool silent = false);

    /// 从 @a slot 读取快速存档。
    void doQuickLoad(int slot);

    /// 截取当前GL帧缓冲（游戏帧+覆盖层）并保存为PNG。
    /// 须在主（draw）线程上、所有渲染完成后调用。
    void doScreenshot();

    /// 从磁盘加载金手指到核心，并填充 m_cheats / m_cheatPath。
    void loadCheats();

    /// 将 m_cheats 中所有条目的当前启用状态重新应用到核心，
    /// 并将更新后的状态写回磁盘上的 .cht 文件。
    void updateCheats();

    /// 启动独立模拟线程（在initialize()中调用）。
    void startGameThread();

    /// 停止并等待模拟线程退出（在cleanup()中调用）。
    void stopGameThread();

    /// 解析mgba_libretro共享库路径（.dll/.so/.dylib）。
    static std::string resolveCoreLibPath();

    /// 将libretro核心最新视频帧上传到m_texture。
    void uploadFrame(NVGcontext* vg, const beiklive::LibretroLoader::VideoFrame& frame);

    /// 将borealis手柄状态映射到libretro按钮状态。
    void pollInput();
};