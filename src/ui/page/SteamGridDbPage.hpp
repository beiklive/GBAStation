#pragma once

#include "core/enums.h"

#include <functional>
#include <string>

namespace beiklive
{
    void openSteamGridDbPage(
        const GameEntry& entry,
        std::function<void(const std::string&)> onCoverChanged = {});
}
