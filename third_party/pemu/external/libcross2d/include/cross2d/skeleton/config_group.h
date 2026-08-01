//
// Created by cpasjuste on 17/10/18.
//

#ifndef C2D_CONFIG_GROUP_H
#define C2D_CONFIG_GROUP_H

#include <string>
#include <vector>
#include <libconfig.h>

#include "cross2d/skeleton/config_option.h"

namespace c2d::config {
    class Group {
    public:
        explicit Group(const std::string &name = "none", int id = -1);

        [[nodiscard]] std::string getName() const;

        [[nodiscard]] int getId() const;

        void setId(int id);

        // child options
        Option *addOption(const Option &option);

        bool removeOption(const std::string &name);

        bool removeOption(int id);

        Option *getOption(const std::string &name);

        Option *getOption(int id);

        Option *getOption(const std::string &groupName, const std::string &optionName);

        Option *getOption(int groupId, int optionId);

        std::vector<Option> *getOptions();

        // child groups
        void addGroup(const Group &group);

        bool removeGroup(const std::string &name);

        bool removeGroup(int id);

        Group *getGroup(const std::string &name);

        Group *getGroup(int id);

        std::vector<Group> *getGroups();

        void copy(Group *group);

    protected:
        bool load(config_setting_t *root);

        bool save(config_setting_t *root);

    private:
        std::string m_name;
        std::vector<Group> m_groups;
        std::vector<Option> m_options;
        int p_id = 0;
    };
}

#endif //C2D_CONFIG_GROUP_H
