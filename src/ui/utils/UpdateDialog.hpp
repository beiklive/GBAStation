#pragma once

#include "core/common.h"
#include "ui/utils/Box.hpp"

#include <borealis.hpp>
#include <vector>
#include <functional>

namespace beiklive {

class UpdateDialog : public beiklive::Box {
public:
    UpdateDialog(const std::string& title, const std::string& body);

    void addButton(const std::string& label, std::function<void()> cb);

    void open();

    void close();

    void setCancelable(bool cancelable);

private:
    brls::Label* m_titleLabel = nullptr;
    brls::Label* m_bodyLabel = nullptr;
    brls::Box* m_buttonBox = nullptr;
    bool m_cancelable = true;
};

} // namespace beiklive
