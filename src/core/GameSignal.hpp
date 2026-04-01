#pragma once

#include <atomic>
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

    // ---- 全部重置 -------------------------------------------------------

    /// 重置所有信号到初始状态（一般在游戏启动前调用）。
    void resetAll() {
        m_paused.store(false, std::memory_order_relaxed);
        m_fastForward.store(false, std::memory_order_relaxed);
        m_rewind.store(false, std::memory_order_relaxed);
        m_pendingQuickSave.store(-1, std::memory_order_relaxed);
        m_pendingQuickLoad.store(-1, std::memory_order_relaxed);
        m_pendingReset.store(false, std::memory_order_relaxed);
        m_muted.store(false, std::memory_order_relaxed);
        m_requestExit.store(false, std::memory_order_relaxed);
        m_requestOpenMenu.store(false, std::memory_order_relaxed);
    }

private:
    std::atomic<bool> m_paused{false};          ///< 暂停标志
    std::atomic<bool> m_fastForward{false};     ///< 快进标志
    std::atomic<bool> m_rewind{false};          ///< 倒带标志
    std::atomic<int>  m_pendingQuickSave{-1};   ///< 待存档槽号（-1=无）
    std::atomic<int>  m_pendingQuickLoad{-1};   ///< 待读档槽号（-1=无）
    std::atomic<bool> m_pendingReset{false};    ///< 重置请求
    std::atomic<bool> m_muted{false};           ///< 静音标志
    std::atomic<bool> m_requestExit{false};     ///< 退出请求
    std::atomic<bool> m_requestOpenMenu{false}; ///< 打开菜单请求
};

} // namespace beiklive
