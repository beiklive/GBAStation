#pragma once

#include "core/enums.h"

#include <string>

namespace beiklive::forwarder
{
    struct InstallResult
    {
        bool success = false;
        unsigned int result = 0;
        std::string message;
    };

    bool isSupported();
    InstallResult installGame(const beiklive::GameEntry& entry);
    void showInstallDialog(const beiklive::GameEntry& entry);
}
