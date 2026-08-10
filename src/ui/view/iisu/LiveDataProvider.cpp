#include "LiveDataProvider.hpp"

#include <ctime>
#include <set>

#include "core/Translation.hpp"
#include "core/common.h"

namespace beiklive
{
    void RecentGameProvider::update(float delta)
    {
        m_refreshTimer += delta;
        if (m_refreshTimer < 5.f)
            return;
        m_refreshTimer = 0.f;

        m_games.clear();
        if (!beiklive::GameDB)
            return;
        constexpr int maxCount = 5;
        const auto entries = beiklive::GameDB->getRecentPlayed(maxCount);
        m_games.reserve(entries.size());
        for (const auto& entry : entries) {
            GameInfo info;
            info.id = entry.path;
            info.title = entry.title;
            info.coverPath = entry.logoPath;
            info.platform = entry.platform;
            info.playTime = entry.playTime;
            info.lastPlayed = entry.lastPlayed;
            m_games.push_back(std::move(info));
        }
    }

    void SystemInfoProvider::update(float delta)
    {
        // UI 帧率（指数平滑）
        if (delta > 0.f && delta < 0.25f)
            m_smoothDt = m_smoothDt * 0.92f + delta * 0.08f;
        m_fps = m_smoothDt > 0.f ? 1.f / m_smoothDt : 0.f;

        // 游戏库统计（低频刷新）
        m_refreshTimer += delta;
        if (m_refreshTimer < 5.f)
            return;
        m_refreshTimer = 0.f;
        m_gameCount = 0;
        m_platformCount = 0;
        if (!beiklive::GameDB)
            return;
        const auto all = beiklive::GameDB->getAll();
        m_gameCount = static_cast<int>(all.size());
        std::set<int> platforms;
        for (const auto& entry : all) {
            if (entry.platform > 0)
                platforms.insert(entry.platform);
        }
        m_platformCount = static_cast<int>(platforms.size());
    }

    void ClockProvider::update(float delta)
    {
        m_timer += delta;
        if (m_timer < 1.f)
            return;
        m_timer = 0.f;

        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &nowTime);
#else
        localtime_r(&nowTime, &localTime);
#endif

        char timeBuf[8]{};
        std::strftime(timeBuf, sizeof(timeBuf), "%H:%M", &localTime);
        m_timeText = timeBuf;

        static const std::string weekdays[] = {
            L("星期日"), L("星期一"), L("星期二"), L("星期三"),
            L("星期四"), L("星期五"), L("星期六"),
        };
        const std::string& weekday =
            localTime.tm_wday >= 0 && localTime.tm_wday <= 6
            ? weekdays[localTime.tm_wday]
            : std::string();
        char dateBuf[32]{};
        std::snprintf(dateBuf, sizeof(dateBuf), "%d月%d日 %s",
                      localTime.tm_mon + 1, localTime.tm_mday,
                      weekday.c_str());
        m_dateText = dateBuf;
    }
} // namespace beiklive
