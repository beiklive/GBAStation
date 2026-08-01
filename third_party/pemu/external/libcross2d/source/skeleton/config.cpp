//
// Created by cpasjuste on 17/10/18.
//

#include "cross2d/c2d.h"

using namespace c2d::config;

#define C2D_CONFIG_VERSION_ID 9950100

Config::Config(const std::string &name, const std::string &path, int version) : Group(name) {
    m_path = path;
    m_version = version;
    config_init(&m_config);
    // add version option
    addOption({"VERSION", version, C2D_CONFIG_VERSION_ID});
}

bool Config::load(const std::string &overridePath) {
    std::string p = overridePath.empty() ? m_path : overridePath;
    printf("Config::load: %s\n", p.c_str());
    if (config_read_file(&m_config, p.c_str()) == CONFIG_FALSE) {
        printf("Config::load: file not found: %s\n", p.c_str());
        return false;
    }

    return Group::load(config_root_setting(&m_config));
}

bool Config::loadString(const std::string &str) {
    if (config_read_string(&m_config, str.c_str()) == CONFIG_FALSE) {
        printf("Config::loadString: config_read_string failed\n");
        return false;
    }

    return Group::load(config_root_setting(&m_config));
}

bool Config::save() {
    // save groups/options
    Group::save(config_root_setting(&m_config));

    // save file
    return config_write_file(&m_config, m_path.c_str()) == 1;
}

int Config::getConfigVersion() {
    return getOption(C2D_CONFIG_VERSION_ID)->getInteger();
}

int Config::getAppVersion() {
    return m_version;
}

std::string Config::getPath() {
    return m_path;
}

config_setting_t *Config::libConfigGetOrAdd(config_setting_t *setting, const std::string &name, int type) {
    config_setting_t *s = config_setting_add(setting, name.empty() ? nullptr : name.c_str(), type);
    if (!s) {
        s = config_setting_get_member(setting, name.c_str());
    }

    return s;
}

config_setting_t *Config::libConfigAddOrGet(config_setting_t *setting, const std::string &name, int type) {
    config_setting_t *s = config_setting_get_member(setting, name.c_str());
    if (!s) {
        s = config_setting_add(setting, name.empty() ? nullptr : name.c_str(), type);
    }

    return s;
}

Config::~Config() {
    config_destroy(&m_config);
}
