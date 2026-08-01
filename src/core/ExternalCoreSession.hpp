#pragma once

#include <string>

namespace beiklive
{
    std::string makeExternalCoreSessionToken(const std::string& romPath);
    bool beginExternalCoreSession(const std::string& romPath, int platform,
                                  const std::string& token);
    bool finishExternalCoreSession(const std::string& token);
    std::string externalCoreReturnToken(int argc, char* argv[]);
}
