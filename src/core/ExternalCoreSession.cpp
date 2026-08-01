#include "ExternalCoreSession.hpp"

#include "common.h"
#include "constexpr.h"
#include "Tools.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>

namespace beiklive
{
    namespace
    {
        namespace fs = std::filesystem;

        fs::path sessionPath()
        {
            return fs::path(beiklive::path::configPath()) / "external_core_session.json";
        }

        int64_t unixSecondsNow()
        {
            return std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }

        bool writeSessionFile(const nlohmann::json& session)
        {
            std::error_code ec;
            const fs::path target = sessionPath();
            const fs::path temp = target.string() + ".tmp";
            fs::create_directories(target.parent_path(), ec);

            std::ofstream out(temp, std::ios::trunc);
            if (!out)
                return false;
            out << session.dump(2) << '\n';
            out.close();
            if (!out)
            {
                fs::remove(temp, ec);
                return false;
            }

            fs::remove(target, ec);
            ec.clear();
            fs::rename(temp, target, ec);
            if (!ec)
                return true;

            ec.clear();
            fs::copy_file(temp, target, fs::copy_options::overwrite_existing, ec);
            fs::remove(temp, ec);
            return !ec;
        }
    }

    std::string makeExternalCoreSessionToken(const std::string& romPath)
    {
        const auto now = unixSecondsNow();
        const auto hash = std::hash<std::string>{}(romPath + std::to_string(now));
        return std::to_string(now) + "-" + std::to_string(hash);
    }

    bool beginExternalCoreSession(const std::string& romPath, int platform,
                                  const std::string& token)
    {
        if (!GameDB || romPath.empty() || token.empty())
            return false;

        nlohmann::json session = {
            {"token", token},
            {"romPath", romPath},
            {"platform", platform},
            {"startedAt", unixSecondsNow()},
        };
        if (!writeSessionFile(session))
            return false;

        auto entry = GameDB->findByPath(romPath);
        if (!entry)
            return false;

        entry->lastPlayed = beiklive::tools::getTimestampString();
        entry->playCount = std::max(0, entry->playCount) + 1;
        GameDB->upsertByPath(*entry);
        GameDB->flush();
        return true;
    }

    bool finishExternalCoreSession(const std::string& token)
    {
        if (!GameDB || token.empty())
            return false;

        const fs::path path = sessionPath();
        std::ifstream in(path);
        if (!in)
            return false;

        nlohmann::json session;
        try
        {
            in >> session;
        }
        catch (...)
        {
            return false;
        }

        if (session.value("token", std::string()) != token)
            return false;

        const std::string romPath = session.value("romPath", std::string());
        const int64_t startedAt = session.value("startedAt", int64_t{0});
        auto entry = GameDB->findByPath(romPath);
        if (!entry || startedAt <= 0)
            return false;

        constexpr int64_t kMaximumSessionSeconds = 7 * 24 * 60 * 60;
        const int elapsed = static_cast<int>(std::clamp<int64_t>(
            unixSecondsNow() - startedAt, 0, kMaximumSessionSeconds));
        entry->playTime = std::max(0, entry->playTime) + elapsed;
        GameDB->upsertByPath(*entry);
        GameDB->flush();

        std::error_code ec;
        fs::remove(path, ec);
        return true;
    }

    std::string externalCoreReturnToken(int argc, char* argv[])
    {
        for (int i = 1; i + 1 < argc; ++i)
        {
            if (argv[i] && std::string(argv[i]) == "--external-return" && argv[i + 1])
                return argv[i + 1];
        }
        return {};
    }
}
