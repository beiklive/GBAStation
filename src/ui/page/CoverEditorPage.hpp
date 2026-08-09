#pragma once

#include "core/enums.h"

#include <functional>
#include <string>

namespace beiklive
{
    // Opens the full-screen NanoVG cover cropper. The callback is invoked only
    // after the cropped image has been written and persisted to GameDB.
    void openCoverEditorPage(
        const GameEntry& entry, const std::string& sourcePath,
        std::function<void(const std::string&)> onCoverChanged = {});
}

