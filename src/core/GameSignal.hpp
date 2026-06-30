#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>
#include "core/Singleton.hpp"

namespace beiklive {

/// 游戏信号控制单例
///
/// 提供 UI 线程与游戏主线程之间的原子信号通信机制。
/// UI 线程（事件响应、菜单操作等）通过本类修改信号值；
/// 游戏主线程每帧轮询信号并执行对应操作，执行后自动清除信号。
///
/// 所有成员均为 std::atomic，确保跨线程访问的内存安全。
///
/// 典型用法：
/// @code
///   // UI 线程（如按键回调）
///   GameSignal::instance().requestPause(true);
///   GameSignal::instance().requestFastForward(true);
///
///   // 游戏主线程（每帧）
///   auto& sig = GameSignal::instance();
///   if (sig.isPaused())       { /* 跳过帧执行 */ }
///   if (sig.isFastForward())  { /* 加速运行 */ }
///   if (int slot = sig.consumeQuickSave(); slot >= 0) { doQuickSave(slot); }
/// @endcode
class GameSignal : public Singleton<GameSignal> {
    friend class Singleton<GameSignal>;

public:
    static constexpr unsigned kMaxPlayers = 2;

    // ---- 暂停信号 -------------------------------------------------------

    /// UI 线程调用：设置游戏暂停状态。
    void requestPause(bool paused) { m_paused.store(paused, std::memory_order_release); }

    /// 游戏线程调用：查询是否处于暂停状态。
    bool isPaused() const { return m_paused.load(std::memory_order_acquire); }

    // ---- 快进信号 -------------------------------------------------------

    /// UI 线程调用：开启或关闭快进。
    void requestFastForward(bool enable) { m_fastForward.store(enable, std::memory_order_release); }

    /// 游戏线程调用：查询是否启用快进。
    bool isFastForward() const { return m_fastForward.load(std::memory_order_acquire); }

    // ---- 倒带信号 -------------------------------------------------------

    /// UI 线程调用：开启或关闭倒带。
    void requestRewind(bool enable) { m_rewind.store(enable, std::memory_order_release); }

    /// 游戏线程调用：查询是否启用倒带。
    bool isRewinding() const { return m_rewind.load(std::memory_order_acquire); }

    // ---- 快速存档信号 ---------------------------------------------------

    /// UI 线程调用：请求快速保存至指定槽号（-1 表示无请求）。
    void requestQuickSave(int slot) { m_pendingQuickSave.store(slot, std::memory_order_release); }

    /// 游戏线程调用：获取并消费快速存档请求（返回槽号，-1 表示无请求）。
    /// 调用后自动将请求重置为 -1。
    int consumeQuickSave() {
        return m_pendingQuickSave.exchange(-1, std::memory_order_acq_rel);
    }

    // ---- 快速读档信号 ---------------------------------------------------

    /// UI 线程调用：请求从指定槽号快速读档（-1 表示无请求）。
    void requestQuickLoad(int slot) { m_pendingQuickLoad.store(slot, std::memory_order_release); }

    /// 游戏线程调用：获取并消费快速读档请求（返回槽号，-1 表示无请求）。
    /// 调用后自动将请求重置为 -1。
    int consumeQuickLoad() {
        return m_pendingQuickLoad.exchange(-1, std::memory_order_acq_rel);
    }

    // ---- 截图信号 -------------------------------------------------------

    /// UI 线程调用：请求保存当前游戏画面截图。
    void requestScreenshot() { m_pendingScreenshot.store(true, std::memory_order_release); }

    /// 游戏线程调用：获取并消费截图请求。
    bool consumeScreenshot() {
        return m_pendingScreenshot.exchange(false, std::memory_order_acq_rel);
    }

    // ---- 重置信号 -------------------------------------------------------

    /// UI 线程调用：请求重置游戏核心。
    void requestReset() { m_pendingReset.store(true, std::memory_order_release); }

    /// 游戏线程调用：获取并消费重置请求（true = 需要重置）。
    /// 调用后自动清除请求。
    bool consumeReset() {
        return m_pendingReset.exchange(false, std::memory_order_acq_rel);
    }

    // ---- 静音信号 -------------------------------------------------------

    /// UI 线程调用：切换静音状态。
    void requestMute(bool muted) { m_muted.store(muted, std::memory_order_release); }

    /// 游戏线程调用：查询是否处于静音状态。
    bool isMuted() const { return m_muted.load(std::memory_order_acquire); }

    // ---- 退出信号 -------------------------------------------------------

    /// 游戏线程调用：请求退出游戏（通常由菜单触发后通知 UI 线程销毁视图）。
    void requestExit() { m_requestExit.store(true, std::memory_order_release); }

    /// UI 线程调用：检查并消费退出请求。
    bool consumeExit() {
        return m_requestExit.exchange(false, std::memory_order_acq_rel);
    }

    // ---- 打开菜单信号 ---------------------------------------------------

    /// 游戏线程调用：请求打开游戏菜单。
    void requestOpenMenu() { m_requestOpenMenu.store(true, std::memory_order_release); }

    /// UI 线程调用：检查并消费打开菜单请求。
    bool consumeOpenMenu() {
        return m_requestOpenMenu.exchange(false, std::memory_order_acq_rel);
    }

    // ---- 打开倒带UI信号 -------------------------------------------------

    /// 输入处理调用：请求打开可视化倒带界面。
    void requestOpenRewindUI() { m_requestOpenRewindUI.store(true, std::memory_order_release); }

    /// UI 线程调用：检查并消费打开倒带UI请求。
    bool consumeOpenRewindUI() {
        return m_requestOpenRewindUI.exchange(false, std::memory_order_acq_rel);
    }

    // ---- 倒带帧恢复信号 -------------------------------------------------

    /// UI 线程调用：请求游戏线程恢复到指定帧（-1表示无请求）。
    void requestRewindRestore(int frameIndex) { m_pendingRewindRestore.store(frameIndex, std::memory_order_release); }

    /// 游戏线程调用：获取并消费倒带恢复请求（返回帧索引，-1表示无请求）。
    int consumeRewindRestore() {
        return m_pendingRewindRestore.exchange(-1, std::memory_order_acq_rel);
    }

    // ---- 游戏按键状态（位掩码，RETRO_DEVICE_ID_JOYPAD_* 对应位）----------

    /// UI 线程调用：按下指定 retro 按钮（id < 16）。
    void pressGameButton(unsigned id) {
        pressGameButton(0, id);
    }

    /// UI 线程调用：按下指定玩家的 retro 按钮（id < 16）。
    void pressGameButton(unsigned player, unsigned id) {
        if (player < kMaxPlayers && id < 16)
            m_gameButtonMasks[player].fetch_or(1u << id, std::memory_order_release);
    }

    /// UI 线程调用：释放指定 retro 按钮（id < 16）。
    void releaseGameButton(unsigned id) {
        releaseGameButton(0, id);
    }

    /// UI 线程调用：释放指定玩家的 retro 按钮（id < 16）。
    void releaseGameButton(unsigned player, unsigned id) {
        if (player < kMaxPlayers && id < 16)
            m_gameButtonMasks[player].fetch_and(~(1u << id), std::memory_order_release);
    }

    /// 游戏线程调用：获取当前按键位掩码。
    uint32_t getGameButtonMask() const {
        return getGameButtonMask(0);
    }

    /// 游戏线程调用：获取指定玩家的按键位掩码。
    uint32_t getGameButtonMask(unsigned player) const {
        if (player >= kMaxPlayers)
            return 0;
        return m_gameButtonMasks[player].load(std::memory_order_acquire);
    }

    /// 重置指定玩家的按键位掩码。
    void clearGameButtonMask(unsigned player) {
        if (player < kMaxPlayers)
            m_gameButtonMasks[player].store(0, std::memory_order_release);
    }

    /// 直接设置指定玩家的按键位掩码。
    void setGameButtonMask(unsigned player, uint32_t mask) {
        if (player < kMaxPlayers)
            m_gameButtonMasks[player].store(mask, std::memory_order_release);
    }

    // ---- 金手指切换信号 -------------------------------------------------

    struct CheatToggleReq { int idx = -1; bool enabled = false; bool pending = false; };

    /// UI 线程调用：请求切换指定金手指的启用状态。
    void requestCheatToggle(int idx, bool enabled) {
        std::lock_guard<std::mutex> lock(m_cheatToggleMutex);
        m_pendingCheatToggles.push_back({idx, enabled, true});
    }

    /// 游戏线程调用：获取并消费金手指切换请求（返回 {idx, hasRequest}）。
    /// 调用后自动清除请求。
    CheatToggleReq consumeCheatToggle() {
        std::lock_guard<std::mutex> lock(m_cheatToggleMutex);
        if (m_pendingCheatToggles.empty())
            return {};
        CheatToggleReq req = m_pendingCheatToggles.front();
        m_pendingCheatToggles.pop_front();
        return req;
    }

    /// 游戏线程调用：一次性获取并消费当前所有金手指切换请求。
    std::vector<CheatToggleReq> consumeCheatToggles() {
        std::lock_guard<std::mutex> lock(m_cheatToggleMutex);
        std::vector<CheatToggleReq> requests(
            m_pendingCheatToggles.begin(),
            m_pendingCheatToggles.end());
        m_pendingCheatToggles.clear();
        return requests;
    }

    // ---- 自动存档信号（退出时使用）-----------------------------------

    /// 调用方：请求在游戏线程保存到指定槽位（-1 表示无请求）。
    void requestAutoSave(int slot) {
        m_autoSaveDone.store(false, std::memory_order_release);
        m_pendingAutoSave.store(slot, std::memory_order_release);
    }

    /// 游戏线程调用：获取并消费自动存档请求（返回槽号，-1 表示无请求）。
    int consumeAutoSave() {
        return m_pendingAutoSave.exchange(-1, std::memory_order_acq_rel);
    }

    /// 游戏线程调用：通知退出自动存档已经完成（成功或失败都算请求处理完毕）。
    void markAutoSaveDone() { m_autoSaveDone.store(true, std::memory_order_release); }

    /// UI 线程调用：消费退出自动存档完成标记。
    bool consumeAutoSaveDone() {
        return m_autoSaveDone.exchange(false, std::memory_order_acq_rel);
    }

    // ---- 重载金手指信号 -------------------------------------------------

    /// UI 线程调用：请求从文件重新加载全部金手指到核心。
    void requestReloadCheats() { m_pendingReloadCheats.store(true, std::memory_order_release); }

    /// 游戏线程调用：获取并消费金手指重载请求。
    bool consumeReloadCheats() {
        return m_pendingReloadCheats.exchange(false, std::memory_order_acq_rel);
    }

    // ---- 核心配置刷新信号 -----------------------------------------------

    /// UI 线程调用：请求游戏线程重新读取核心配置。
    void requestConfigUpdate() { m_pendingConfigUpdate.store(true, std::memory_order_release); }

    /// 游戏线程调用：获取并消费核心配置刷新请求。
    bool consumeConfigUpdate() {
        return m_pendingConfigUpdate.exchange(false, std::memory_order_acq_rel);
    }

    // ---- 全部重置 -------------------------------------------------------

    /// 重置所有信号到初始状态（一般在游戏启动前调用）。
    void resetAll() {
        m_paused.store(false, std::memory_order_relaxed);
        m_fastForward.store(false, std::memory_order_relaxed);
        m_rewind.store(false, std::memory_order_relaxed);
        m_pendingQuickSave.store(-1, std::memory_order_relaxed);
        m_pendingQuickLoad.store(-1, std::memory_order_relaxed);
        m_pendingScreenshot.store(false, std::memory_order_relaxed);
        m_pendingReset.store(false, std::memory_order_relaxed);
        m_muted.store(false, std::memory_order_relaxed);
        m_requestExit.store(false, std::memory_order_relaxed);
        m_requestOpenMenu.store(false, std::memory_order_relaxed);
        m_requestOpenRewindUI.store(false, std::memory_order_relaxed);
        m_pendingRewindRestore.store(-1, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lock(m_cheatToggleMutex);
            m_pendingCheatToggles.clear();
        }
        m_pendingReloadCheats.store(false, std::memory_order_relaxed);
        m_pendingAutoSave.store(-1, std::memory_order_relaxed);
        m_autoSaveDone.store(false, std::memory_order_relaxed);
        m_pendingConfigUpdate.store(false, std::memory_order_relaxed);
        for (auto& mask : m_gameButtonMasks)
            mask.store(0, std::memory_order_relaxed);
    }

private:
    std::atomic<bool> m_paused{false};          ///< 暂停标志
    std::atomic<bool> m_fastForward{false};     ///< 快进标志
    std::atomic<bool> m_rewind{false};          ///< 倒带标志
    std::atomic<int>  m_pendingQuickSave{-1};   ///< 待存档槽号（-1=无）
    std::atomic<int>  m_pendingQuickLoad{-1};   ///< 待读档槽号（-1=无）
    std::atomic<bool> m_pendingScreenshot{false}; ///< 待截图请求
    std::atomic<bool> m_pendingReset{false};    ///< 重置请求
    std::atomic<bool> m_muted{false};           ///< 静音标志
    std::atomic<bool> m_requestExit{false};     ///< 退出请求
    std::atomic<bool> m_requestOpenMenu{false}; ///< 打开菜单请求
    std::atomic<bool> m_requestOpenRewindUI{false}; ///< 打开倒带UI请求
    std::atomic<int>  m_pendingRewindRestore{-1};   ///< 待恢复的倒带帧索引（-1=无）
    std::mutex m_cheatToggleMutex;
    std::deque<CheatToggleReq> m_pendingCheatToggles; ///< 待切换的金手指队列
    std::atomic<bool> m_pendingReloadCheats{false};   ///< 待重载全部金手指
    std::atomic<int>  m_pendingAutoSave{-1};            ///< 待自动存档槽位（-1=无）
    std::atomic<bool> m_autoSaveDone{false};             ///< 退出自动存档是否已处理完毕
    std::atomic<bool> m_pendingConfigUpdate{false};    ///< 待刷新核心配置
    std::atomic<uint32_t> m_gameButtonMasks[kMaxPlayers]{};  ///< 游戏按键位掩码（bit i = RETRO_DEVICE_ID_JOYPAD_* i）
};

} // namespace beiklive
