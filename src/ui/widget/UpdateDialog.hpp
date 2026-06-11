#pragma once

#include "core/common.h"
#include <borealis/views/dialog.hpp>
#include <functional>

namespace beiklive {

class UpdateDialog : public brls::Dialog {
public:
    UpdateDialog(const std::string& title, const std::string& body);

    void addButton(const std::string& label, std::function<void()> cb);

    void open();

    void close();

    void setCancelable(bool cancelable);
};

} // namespace beiklive
