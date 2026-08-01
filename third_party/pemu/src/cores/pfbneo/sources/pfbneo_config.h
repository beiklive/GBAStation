//
// Created by cpasjuste on 29/05/18.
//

#ifndef PFBN_CONFIG_H
#define PFBN_CONFIG_H

#include "skeleton/pemu_config.h"
#include "pfbneo_utility.h"
#include "burner.h"

class PFBAConfig final : public PEMUConfig {
public:
    PFBAConfig(Renderer *renderer, const int version);

    std::vector<int> getCoreHiddenOptionToEnable() override {
        return {
                UI_FILTER_CLONES,
                UI_FILTER_SYSTEM
        };
    }

    std::string getCoreVersion() override {
        return "fbneo: " + std::string(szAppBurnVer);
    }

    std::vector<std::string> getCoreSupportedExt() override {
        return {".zip"};
    }
};

#endif //PFBN_CONFIG_H
