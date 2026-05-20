#pragma once
#include <borealis.hpp>
#include <vector>
#include <string>
#include <functional>
#include <chrono>

#include "ListItem.hpp"
#include "core/common.h"

namespace beiklive {

class FileListDataSource : public brls::RecyclerDataSource {
public:
    int numberOfSections(brls::RecyclerFrame* recycler) override { return 1; }
    int numberOfRows(brls::RecyclerFrame* recycler, int section) override;
    brls::RecyclerCell* cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) override;
    void didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath) override;
    std::string titleForHeader(brls::RecyclerFrame* recycler, int section) override { return ""; }

    void setItems(const std::vector<beiklive::ListItem>& items);
    void appendItems(const std::vector<beiklive::ListItem>& items);
    void clear();
    const beiklive::ListItem& getItem(int index) const;
    int count() const { return (int)m_items.size(); }

    std::function<void(const beiklive::ListItem&)> onItemClicked;
    std::function<void(beiklive::ListItemCell&)> onBindCell;

private:
    std::vector<beiklive::ListItem> m_items;
};

class FileListView : public brls::Box {
public:
    FileListView();

    void appendItems(const std::vector<beiklive::ListItem>& items);
    void clearItems();
    void finishLoading();
    int itemCount() const;

    void moveUp();
    void moveDown();
    void movePageUp();
    void movePageDown();
    void moveTo(int index);

    void frame(brls::FrameContext* ctx) override;
    brls::View* getDefaultFocus() override;

    void setOnItemClicked(std::function<void(const beiklive::ListItem&)> cb);
    void setOnBindCell(std::function<void(beiklive::ListItemCell&)> cb);

    std::function<void(const beiklive::ListItem&)> onItemFocused;
    std::function<void(const beiklive::ListItem&)> onItemFocusLost;

private:
    brls::RecyclerFrame* m_recycler = nullptr;
    FileListDataSource* m_dataSource = nullptr;

    int m_focusedIndex = 0;
    int m_totalItemCount = 0;

    // Long press acceleration
    bool m_prevUp = false;
    bool m_prevDown = false;
    float m_holdUpTime = 0.f;
    float m_holdDownTime = 0.f;
    float m_holdUpRepeat = 0.f;
    float m_holdDownRepeat = 0.f;

    static constexpr float HOLD_INITIAL_DELAY = 0.3f;
    static constexpr float HOLD_REPEAT = 0.08f;
    static constexpr float HOLD_REPEAT_FAST = 0.03f;
    static constexpr float HOLD_ACCEL_TIME = 1.5f;
    static constexpr int PAGE_STEP = 12;

    // Delta time computation
    std::chrono::steady_clock::time_point m_lastFrameTime;

    void setFocusedIndex(int newIndex);
};

} // namespace beiklive
