#pragma once

#include <borealis.hpp>

#include <functional>
#include <string>

namespace beiklive::ui {

/// 创建提示标签（灰色小字，不可聚焦）
inline brls::Label* makeHint(const std::string& text) {
    auto* lbl = new brls::Label();
    lbl->setText(text);
    lbl->setFontSize(16.f);
    lbl->setTextColor(nvgRGB(154, 154, 154));
    lbl->setMarginBottom(10.f);
    lbl->setMarginTop(10.f);
    lbl->setMarginLeft(20.f);
    lbl->setFocusable(false);
    return lbl;
}

/// 创建可滚动标签页容器
inline brls::ScrollingFrame* makeScrollTab() {
    auto* scroll = new brls::ScrollingFrame();
    scroll->setGrow(1.0f);
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    scroll->setScrollingIndicatorVisible(false);
    scroll->setFocusable(false);
    return scroll;
}

/// 创建标签页内容容器（纵向排列）
inline brls::Box* makeContentBox() {
    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setPadding(20.f, 40.f, 30.f, 40.f);
    return box;
}

/// 创建分区标题
inline brls::Header* makeHeader(const std::string& title) {
    auto* h = new brls::Header();
    h->setTitle(title);
    return h;
}

/// 显示确认对话框（取消 + 确认）
inline void showConfirmDialog(const std::string& title, const std::string& message,
                               std::function<void()> onConfirm) {
    auto* dlg = new brls::Dialog(title + "\n\n" + message);
    dlg->addButton("取消", []() {});
    dlg->addButton("确认", [onConfirm = std::move(onConfirm)]() { onConfirm(); });
    dlg->open();
}

/// 显示信息对话框（仅有确认按钮）
inline void showInfoDialog(const std::string& title, const std::string& message) {
    auto* dlg = new brls::Dialog(title + "\n\n" + message);
    dlg->addButton("确定", []() {});
    dlg->open();
}

/// 线程安全通知：在 UI 线程上显示提示消息
inline void notifyInfo(const std::string& msg) {
    brls::sync([msg]() { brls::Application::notify(msg); });
}

} // namespace beiklive::ui
