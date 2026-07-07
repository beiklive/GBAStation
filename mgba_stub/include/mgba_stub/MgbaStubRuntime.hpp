#pragma once

#include <string>

namespace beiklive::mgba_stub {

struct RunOptions {
    std::string romPath;
    std::string title;
    std::string savePath;
    std::string returnNroPath;
    int platform = 1;
};

int RunRuntime(const RunOptions& options);

} // namespace beiklive::mgba_stub
