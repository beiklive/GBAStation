#pragma once

#include <string>
#include <vector>

#include "GameDataProvider.hpp"

namespace beiklive
{
    /// 实时数据提供者抽象：由 LiveWidget 驱动刷新
    class LiveDataProvider
    {
    public:
        virtual ~LiveDataProvider() = default;
        virtual void update(float delta) = 0;
    };

    /// 最近游戏数据：来自 GameDB 游玩历史
    class RecentGameProvider : public LiveDataProvider
    {
    public:
        void update(float delta) override;
        const std::vector<GameInfo>& games() const { return m_games; }

    private:
        std::vector<GameInfo> m_games;
        float m_refreshTimer = 5.f;
    };

    /// 系统信息数据：UI 帧率 + 游戏库统计
    class SystemInfoProvider : public LiveDataProvider
    {
    public:
        void update(float delta) override;

        float fps() const { return m_fps; }
        int gameCount() const { return m_gameCount; }
        int platformCount() const { return m_platformCount; }

    private:
        float m_fps = 0.f;
        float m_smoothDt = 0.0166f;
        float m_refreshTimer = 5.f;
        int m_gameCount = 0;
        int m_platformCount = 0;
    };

    /// 时钟数据：时间 + 日期 + 星期
    class ClockProvider : public LiveDataProvider
    {
    public:
        void update(float delta) override;

        const std::string& timeText() const { return m_timeText; }
        const std::string& dateText() const { return m_dateText; }

    private:
        std::string m_timeText;
        std::string m_dateText;
        float m_timer = 1.f;
    };
} // namespace beiklive
