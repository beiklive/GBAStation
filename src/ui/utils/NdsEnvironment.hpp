#pragma once

#include <borealis.hpp>

#ifdef __SWITCH__
#include <cstdio>
#endif

namespace beiklive
{
    inline void notifyNdsEnvironmentError(const char* message)
    {
        brls::Application::notify(message);
    }

    inline const char* ndsCorePath()
    {
#ifdef __SWITCH__
        constexpr const char* path = "sdmc:/GBAStation/core/GBAStationNDSStub.nro";
        FILE* file = std::fopen(path, "rb");
        if (file)
        {
            std::fclose(file);
            return path;
        }
#endif
        return "sdmc:/GBAStation/core/GBAStationNDSStub.nro";
    }

    inline bool ensureNdsEnvironmentReady()
    {
#ifdef __SWITCH__
        const char* corePath = ndsCorePath();
        constexpr const char* requiredFiles[] = {
            "sdmc:/GBAStation/bios/nds/bios7.bin",
            "sdmc:/GBAStation/bios/nds/bios9.bin",
            "sdmc:/GBAStation/bios/nds/firmware.bin",
        };

        FILE* core = std::fopen(corePath, "rb");
        if (!core)
        {
            notifyNdsEnvironmentError("nds运行核心不存在，请到关于页面下载nds核心");
            return false;
        }
        std::fclose(core);

        for (const char* path : requiredFiles)
        {
            FILE* file = std::fopen(path, "rb");
            if (file)
            {
                std::fclose(file);
                continue;
            }

            notifyNdsEnvironmentError("nds运行环境不完整，请到关于页面下载nds固件和核心");
            return false;
        }
#endif
        return true;
    }
}
