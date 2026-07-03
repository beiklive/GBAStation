#pragma once

#include <string>

namespace beiklive::nds_stub {

struct DekoRunOptions {
    std::string romPath;
    std::string title;
    std::string savePath;
    std::string returnNroPath;
};

bool ShouldUseDekoRuntime();
int RunDekoRuntime(const DekoRunOptions& options);

} // namespace beiklive::nds_stub
