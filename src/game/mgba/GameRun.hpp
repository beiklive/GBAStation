#pragma once

#include "core/common.h"
#include "game/retro/LibretroLoader.hpp"

namespace beiklive::gba
{

    class CoreMgba
    {
    public:
        CoreMgba() = default;
        ~CoreMgba() = default;
        bool SetupGame(beiklive::GameEntry GameEntry);
        // void unloadCore();
        // bool isCoreLoaded() const;

        // bool initCore();
        // void deinitCore();
        // unsigned apiVersion() const;
        // void getSystemInfo(retro_system_info *info) const;
        // void getSystemAvInfo(retro_system_av_info *info) const;
        // bool loadGame(const std::string &romPath);
        // void unloadGame();
        // void run();
        // void reset();
        // size_t serializeSize() const;
        // bool serialize(void *data, size_t size) const;
        // bool unserialize(const void *data, size_t size);
    private:
        beiklive::GameEntry m_gameEntry; // 游戏条目数据，包含路径、标题等信息
        beiklive::LibretroLoader m_core;
        std::vector<beiklive::CheatEntry> m_cheats;


        bool _loadCore(const std::string &corePath);
        bool _loadRom(const std::string &romPath);

        bool _loadSram();
        bool _loadRtc();
        bool _loadCheats();
        void _updateCheats();

        bool _saveSram();
        bool _saveRtc();

    };
} // namespace beiklive
