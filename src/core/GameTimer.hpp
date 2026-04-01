#pragma once

#include <chrono>
#include "core/Singleton.hpp"

namespace beiklive {

/// 游戏计时器单例
///
/// 提供游戏帧级别的时间测量功能：
///   - 获取当前时间点
///   - 开始计时（记录起始时间）
///   - 结束计时（记录终止时间）
///   - 获取计时时长（秒/毫秒/微秒）
///
/// 典型用法：
/// @code
///   auto& timer = GameTimer::instance();
///   timer.start();
///   // ... 执行某些操作 ...
///   timer.stop();
///   double elapsed = timer.elapsedSeconds();
/// @endcode
class GameTimer : public Singleton<GameTimer> {
    friend class Singleton<GameTimer>;

public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration  = std::chrono::duration<double>;

    /// 返回当前时间点。
    static TimePoint now() { return Clock::now(); }

    /// 开始计时（记录起始时间）。
    void start() { m_startTime = Clock::now(); }

    /// 结束计时（记录终止时间）。
    void stop() { m_stopTime = Clock::now(); }

    /// 返回从 start() 到 stop() 之间的时长（秒）。
    double elapsedSeconds() const {
        return std::chrono::duration<double>(m_stopTime - m_startTime).count();
    }

    /// 返回从 start() 到 stop() 之间的时长（毫秒）。
    double elapsedMilliseconds() const {
        return std::chrono::duration<double, std::milli>(m_stopTime - m_startTime).count();
    }

    /// 返回从 start() 到 stop() 之间的时长（微秒）。
    double elapsedMicroseconds() const {
        return std::chrono::duration<double, std::micro>(m_stopTime - m_startTime).count();
    }

    /// 返回从 start() 到当前时刻的时长（秒），不调用 stop()。
    double elapsedSecondsNow() const {
        return std::chrono::duration<double>(Clock::now() - m_startTime).count();
    }

    /// 返回两个时间点之间的时长（秒）。
    static double secondsBetween(TimePoint a, TimePoint b) {
        return std::chrono::duration<double>(b - a).count();
    }

    /// 将时间点转换为 Unix 时间戳（秒）。
    static int64_t toUnixSeconds(TimePoint tp) {
        return std::chrono::duration_cast<std::chrono::seconds>(
            tp.time_since_epoch()).count();
    }

private:
    TimePoint m_startTime{};
    TimePoint m_stopTime{};
};

} // namespace beiklive
