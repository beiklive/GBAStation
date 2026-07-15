#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace beiklive::pico8
{
    struct GameEntry
    {
        std::string name;
        std::string path;
        std::string coverPath;
    };

    class Filesystem
    {
    public:
        static std::string rootPath();
        static std::string gamesPath();
        static std::string corePath();
        static std::string cachePath();
        static std::string cartDataPath();
        static std::string statesPath();
        static std::string quickStatePath(const std::string& cartPath);
        static std::string runtimePath();
        static std::string fontPath();

        static bool ensureDirectories();
        static std::vector<GameEntry> scanGames();
        static std::vector<std::string> listGamePaths();
        static std::string resolveCover(const GameEntry& game);

    private:
        static std::string buildLabelCache(const std::string& cartPath,
                                           const std::string& gameName);
    };

    namespace host_bridge
    {
        void setInput(uint8_t down, uint8_t held);
    }
}
