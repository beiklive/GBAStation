//
// Created by cpasjuste on 17/10/18.
//

#ifndef C2D_CONFIG_H
#define C2D_CONFIG_H

#include <string>
#include <vector>
#include <libconfig.h>

#include "config_group.h"

namespace c2d::config {
    class Config : public Group {
    public:
        Config(const std::string &name, const std::string &path, int version = 1);

        virtual ~Config();

        virtual bool load(const std::string &overridePath = {});

        virtual bool loadString(const std::string &str);

        virtual bool save();

        int getConfigVersion();

        int getAppVersion();

        std::string getPath();

        config_t *libConfigGetInstance() { return &m_config; };

        static config_setting_t *libConfigGetOrAdd(config_setting_t *setting, const std::string &name, int type);

        static config_setting_t *libConfigAddOrGet(config_setting_t *setting, const std::string &name, int type);

    private:
        std::string m_path;
        config_t m_config{};
        int m_version = 0;
    };
}

#endif //C2D_CONFIG_H
