//
// Created by cpasjuste on 17/10/18.
//

#include <algorithm>
#include "cross2d/skeleton/config.h"
#include "cross2d/skeleton/config_group.h"

using namespace c2d::config;

Group::Group(const std::string &name, int id) {
    m_name = name;
    p_id = id;
}

std::string Group::getName() const {
    return m_name;
}

int Group::getId() const {
    return p_id;
}

void Group::setId(int i) {
    p_id = i;
}

/// childs options
Option *Group::addOption(const Option &option) {
    m_options.push_back(option);
    return (Option *) &m_options.at(m_options.size() - 1);
}

bool Group::removeOption(const std::string &n) {
    auto found = std::find_if(m_options.begin(), m_options.end(), [&n](Option const &option) {
        return n == option.getName();
    });

    if (found != m_options.end()) {
        m_options.erase(found);
        return true;
    }

    return false;
}

bool Group::removeOption(int i) {
    auto found = std::find_if(m_options.begin(), m_options.end(), [&i](Option const &option) {
        return i == option.getId();
    });

    if (found != m_options.end()) {
        m_options.erase(found);
        return true;
    }

    return false;
}

Option *Group::getOption(const std::string &n) {
    for (Option &option: m_options) {
        if (option.getName() == n) {
            return &option;
        }
    }

    for (Group &group: m_groups) {
        Option *opt = group.getOption(n);
        if (opt) return opt;
    }

    return nullptr;
}

Option *Group::getOption(int i) {
    for (Option &option: m_options) {
        if (option.getId() == i) {
            return &option;
        }
    }

    for (Group &group: m_groups) {
        Option *opt = group.getOption(i);
        if (opt) return opt;
    }

    return nullptr;
}

Option *Group::getOption(const std::string &groupName, const std::string &optionName) {
    Group *group = getGroup(groupName);
    if (group != nullptr) {
        return group->getOption(optionName);
    }
    return nullptr;
}

Option *Group::getOption(int groupId, int optionId) {
    Group *group = getGroup(groupId);
    if (group != nullptr) {
        return group->getOption(optionId);
    }
    return nullptr;
}

std::vector<Option> *Group::getOptions() {
    return &m_options;
}

/// childs group
void Group::addGroup(const Group &group) {
    m_groups.push_back(group);
}

bool Group::removeGroup(const std::string &n) {
    auto found = std::find_if(m_groups.begin(), m_groups.end(), [&n](Group const &group) {
        return n == group.getName();
    });

    if (found != m_groups.end()) {
        m_groups.erase(found);
        return true;
    }

    return false;
}

bool Group::removeGroup(int i) {
    auto found = std::find_if(m_groups.begin(), m_groups.end(), [&i](Group const &group) {
        return i == group.getId();
    });

    if (found != m_groups.end()) {
        m_groups.erase(found);
        return true;
    }

    return false;
}

Group *Group::getGroup(const std::string &name) {
    if (name == m_name) {
        // used for recursive lookup
        return this;
    }

    for (Group &group: m_groups) {
        Group *s = group.getGroup(name);
        if (s != nullptr) {
            return s;
        }
    }

    return nullptr;
}

Group *Group::getGroup(int id) {
    if (id == p_id) {
        // used for recursive lookup
        return this;
    }

    for (Group &group: m_groups) {
        Group *s = group.getGroup(id);
        if (s != nullptr) {
            return s;
        }
    }

    return nullptr;
}

std::vector<Group> *Group::getGroups() {
    return &m_groups;
}

///
/// PROTECTED
///
bool Group::load(config_setting_t *parent) {
    if (!parent) {
        printf("Config::Group::load: could not find root config: %s\n", m_name.c_str());
        return false;
    }

    config_setting_t *settings = config_setting_lookup(parent, m_name.c_str());
    if (!settings) {
        printf("Config::Group::load: group not found, skipping: %s\n", m_name.c_str());
        return false;
    }

    for (auto &option: m_options) {
        bool found = false;

        if (option.getType() == Option::Type::String) {
            const char *value;
            if (config_setting_lookup_string(settings, option.getName().c_str(), &value)) {
                option.setString(value);
                found = true;
            }
        } else if (option.getType() == Option::Type::Integer) {
            int value = 0;
            if (config_setting_lookup_int(settings, option.getName().c_str(), &value)) {
                option.setInteger(value);
                found = true;
            }
        } else if (option.getType() == Option::Type::Float) {
            double value = 0;
            if (config_setting_lookup_float(settings, option.getName().c_str(), &value)) {
                option.setFloat((float) value);
                found = true;
            }
        } else if (option.getType() == Option::Type::Vector2f) {
            std::vector<float> values = {option.getVector2f().x, option.getVector2f().y};
            config_setting_t *sub = config_setting_lookup(settings, option.getName().c_str());
            if (sub && sub->type == CONFIG_TYPE_ARRAY) {
                for (int i = 0; i < 2; i++) {
                    values[i] = (float) config_setting_get_float_elem(sub, i);
                }
                option.setVector2f({values[0], values[1]});
                found = true;
            }
        } else if (option.getType() == Option::Type::FloatRect) {
            FloatRect r = option.getFloatRect();
            std::vector<float> values = {r.left, r.top, r.width, r.height};
            config_setting_t *sub = config_setting_lookup(settings, option.getName().c_str());
            if (sub && sub->type == CONFIG_TYPE_ARRAY) {
                for (int i = 0; i < 4; i++) {
                    values[i] = (float) config_setting_get_float_elem(sub, i);
                }
                option.setFloatRect({values[0], values[1], values[2], values[3]});
                found = true;
            }
        } else if (option.getType() == Option::Type::Color) {
            FloatRect r = option.getFloatRect();
            std::vector<uint8_t> values = {(uint8_t) r.left, (uint8_t) r.top, (uint8_t) r.width, (uint8_t) r.height};
            config_setting_t *sub = config_setting_lookup(settings, option.getName().c_str());
            if (sub && sub->type == CONFIG_TYPE_ARRAY) {
                for (int i = 0; i < 4; i++) {
                    values[i] = (uint8_t) config_setting_get_int_elem(sub, i);
                }
                option.setColor({values[0], values[1], values[2], values[3]});
                found = true;
            }
        } else if (option.getType() == Option::Type::Array) {
            const char *value;
            if (config_setting_lookup_string(settings, option.getName().c_str(), &value)) {
                option.setArrayIndex(value);
                found = true;
            }
        }

        // fallback to string type...
        if (!found) {
            // fallback to string type
            const char *value;
            if (config_setting_lookup_string(settings, option.getName().c_str(), &value)) {
                //printf("%s: %s (fallback)\n", name.c_str(), value);
                option.setType(Option::Type::String);
                option.setString(value);
            }
        }
    }

    // load children, if any
    for (Group &group: m_groups) {
        group.load(settings);
    }

    return true;
}

bool Group::save(config_setting_t *parent) {
    if (!parent) {
        printf("Config::Group::save: could not save group (%s), no parent\n", m_name.c_str());
        return false;
    }

    config_setting_t *root = Config::libConfigGetOrAdd(parent, m_name, CONFIG_TYPE_GROUP);
    if (!root) {
        printf("Config::Group::save: config_setting_get_or_add failed (%s)\n", m_name.c_str());
        return false;
    }

    for (Option &option: m_options) {
        if (option.getType() == Option::Type::String) {
            config_setting_t *setting = Config::libConfigGetOrAdd(root, option.getName(), CONFIG_TYPE_STRING);
            config_setting_set_string(setting, option.getString().c_str());
        } else if (option.getType() == Option::Type::Integer) {
            config_setting_t *setting = Config::libConfigGetOrAdd(root, option.getName(), CONFIG_TYPE_INT);
            config_setting_set_int(setting, option.getInteger());
        } else if (option.getType() == Option::Type::Float) {
            config_setting_t *setting = Config::libConfigGetOrAdd(root, option.getName(), CONFIG_TYPE_FLOAT);
            config_setting_set_float(setting, (double) option.getFloat());
        } else if (option.getType() == Option::Type::Vector2f) {
            config_setting_t *array = Config::libConfigGetOrAdd(root, option.getName(), CONFIG_TYPE_ARRAY);
            config_setting_t *subset = Config::libConfigGetOrAdd(array, "", CONFIG_TYPE_FLOAT);
            config_setting_set_float(subset, option.getVector2f().x);
            subset = Config::libConfigGetOrAdd(array, "", CONFIG_TYPE_FLOAT);
            config_setting_set_float(subset, option.getVector2f().y);
        } else if (option.getType() == Option::Type::FloatRect) {
            config_setting_t *array = Config::libConfigGetOrAdd(root, option.getName(), CONFIG_TYPE_ARRAY);
            config_setting_t *subset = Config::libConfigGetOrAdd(array, "", CONFIG_TYPE_FLOAT);
            config_setting_set_float(subset, option.getFloatRect().left);
            subset = Config::libConfigGetOrAdd(array, "", CONFIG_TYPE_FLOAT);
            config_setting_set_float(subset, option.getFloatRect().top);
            subset = Config::libConfigGetOrAdd(array, "", CONFIG_TYPE_FLOAT);
            config_setting_set_float(subset, option.getFloatRect().width);
            subset = Config::libConfigGetOrAdd(array, "", CONFIG_TYPE_FLOAT);
            config_setting_set_float(subset, option.getFloatRect().height);
        } else if (option.getType() == Option::Type::Color) {
            config_setting_t *array = Config::libConfigGetOrAdd(root, option.getName(), CONFIG_TYPE_ARRAY);
            config_setting_t *subset = Config::libConfigGetOrAdd(array, "", CONFIG_TYPE_INT);
            config_setting_set_int(subset, (int) option.getFloatRect().left);
            subset = Config::libConfigGetOrAdd(array, "", CONFIG_TYPE_INT);
            config_setting_set_int(subset, (int) option.getFloatRect().top);
            subset = Config::libConfigGetOrAdd(array, "", CONFIG_TYPE_INT);
            config_setting_set_int(subset, (int) option.getFloatRect().width);
            subset = Config::libConfigGetOrAdd(array, "", CONFIG_TYPE_INT);
            config_setting_set_int(subset, (int) option.getFloatRect().height);
        } else if (option.getType() == Option::Type::Array) {
            config_setting_t *setting = Config::libConfigGetOrAdd(root, option.getName(), CONFIG_TYPE_STRING);
            config_setting_set_string(setting, option.getString().c_str());
        }
    }

    // save childs, if any
    for (Group &group: m_groups) {
        group.save(root);
    }

    return true;
}

void Group::copy(Group *group) {
    if (!group) return;

    group->m_name = m_name;
    group->m_groups = m_groups;
    group->m_options = m_options;
    group->p_id = p_id;
}
