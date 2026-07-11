#pragma once

#include <borealis.hpp>

#ifdef __SWITCH__
#include <cstdio>
#endif

namespace beiklive
{
    inline bool ensureNdsEnvironmentReady()
    {
#ifdef __SWITCH__
        constexpr const char* requiredFiles[] = {
            "sdmc:/GBAStation/core/GBAStationNDSStub.nro",
            "sdmc:/GBAStation/bios/nds/bios7.bin",
            "sdmc:/GBAStation/bios/nds/bios9.bin",
            "sdmc:/GBAStation/bios/nds/firmware.bin",
        };

        for (const char* path : requiredFiles)
        {
            FILE* file = std::fopen(path, "rb");
            if (file)
            {
                std::fclose(file);
                continue;
            }

            auto* dialog = new brls::Dialog(
                "nds运行环境不完整，请到关于页面下载nds固件和核心");
            dialog->addButton("确认", []() {});
            dialog->open();
            return false;
        }
#endif
        return true;
    }
}
