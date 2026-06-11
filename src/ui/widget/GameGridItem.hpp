#pragma once

#include <borealis.hpp>
#include <functional>
#include "core/common.h"

namespace beiklive {

class GameGridItem : public brls::Box {
public:
    static constexpr float ITEM_W = 200.f;
    static constexpr float ITEM_H = 250.f;
    static constexpr float IMAGE_S = 170.f;

    explicit GameGridItem(const beiklive::GameEntry& entry);
    ~GameGridItem() = default;

    void setImagePath(const std::string& path);

    void onParentFocusGained(brls::View* focusedView) override;
    void onParentFocusLost(brls::View* focusedView) override;

    std::function<void(const beiklive::GameEntry&)> onItemClicked;

private:
    beiklive::GameEntry m_entry;
    brls::Box* imgBox = nullptr;
    brls::Image* m_image = nullptr;
    brls::Label* m_title = nullptr;
};

} // namespace beiklive
