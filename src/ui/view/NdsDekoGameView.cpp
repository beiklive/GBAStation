#include "ui/view/NdsDekoGameView.hpp"

#include "emulator/melonds/deko/NdsDekoProbe.hpp"

#include <borealis/views/label.hpp>

namespace beiklive {

namespace {
constexpr int kSafeProbeMaxLevel = 6;
}

NdsDekoGameView::NdsDekoGameView(beiklive::GameEntry gameEntry)
    : m_gameEntry(std::move(gameEntry))
{
    _initLayout();
}

void NdsDekoGameView::_initLayout()
{
    showHeader(false);
    showFooter(false);
    showBackground(false);
    showShader(false);

    setWidthPercentage(100.f);
    setHeightPercentage(100.f);
    setAxis(brls::Axis::COLUMN);
    setAlignItems(brls::AlignItems::CENTER);
    setJustifyContent(brls::JustifyContent::CENTER);
    setFocusable(true);
    setBackground(brls::ViewBackground::NONE);

    m_statusLabel = new brls::Label();
    m_statusLabel->setText("NDS Deko3D 实验模式\n阶段0：专属页面已启用\nY：运行下一级 Deko probe    +：菜单");
    m_statusLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_statusLabel->setFontSize(24.f);
    getContentBox()->addView(m_statusLabel);

    registerAction("菜单", brls::BUTTON_START, [this](brls::View*) -> bool {
        if (m_onOpenMenu)
            m_onOpenMenu();
        return true;
    });

    registerAction("Deko测试", brls::BUTTON_Y, [this](brls::View*) -> bool {
        _runProbeLevel(m_nextProbeLevel);
        return true;
    });
}

void NdsDekoGameView::startProbe()
{
    brls::Logger::info("NdsDekoGameView: manual probe ready for {}", m_gameEntry.title);
    if (m_statusLabel)
        m_statusLabel->setText("NDS Deko3D 实验模式\n阶段0：专属页面分流已启用\n按 Y 运行 level 1 probe，按 + 打开菜单");
}

void NdsDekoGameView::_runProbeLevel(int level)
{
    if (level < 1)
        level = 1;
    if (level > kSafeProbeMaxLevel)
        level = kSafeProbeMaxLevel;

    brls::Logger::info("NdsDekoGameView: stage1 probe start for {}, level={}", m_gameEntry.title, level);
    if (m_statusLabel)
        m_statusLabel->setText("NDS Deko3D 实验模式\n阶段1：Deko3D level " + std::to_string(level) + " 运行中");

    const auto result = beiklive::RunNdsDekoProbe({level, 60});

    brls::Logger::info("NdsDekoGameView: stage1 probe result supported={} success={} requestedLevel={} reachedLevel={} frames={} message={}",
                       result.supported,
                       result.success,
                       result.requestedLevel,
                       result.reachedLevel,
                       result.presentedFrames,
                       result.message);

    if (!m_statusLabel)
        return;

    if (!result.supported)
    {
        m_statusLabel->setText("NDS Deko3D 实验模式\n阶段1：当前平台不支持 Deko3D probe\n按 + 打开专属菜单");
    }
    else if (result.success)
    {
        if (result.reachedLevel >= kSafeProbeMaxLevel)
        {
            m_nextProbeLevel = kSafeProbeMaxLevel;
            m_statusLabel->setText("NDS Deko3D 实验模式\n阶段1：level 1-6 已通过\nlevel 7 已确认会在 swapchain.create() 崩溃，软切换路线阻塞\n+：菜单");
            return;
        }

        if (result.reachedLevel >= m_nextProbeLevel && m_nextProbeLevel < kSafeProbeMaxLevel)
            m_nextProbeLevel = result.reachedLevel + 1;
        m_statusLabel->setText("NDS Deko3D 实验模式\n阶段1：Deko3D probe 已完成\n级别 " +
                               std::to_string(result.reachedLevel) + " 通过，Y：level " +
                               std::to_string(m_nextProbeLevel) + "，+：菜单");
    }
    else
    {
        m_statusLabel->setText("NDS Deko3D 实验模式\n阶段1：Deko3D probe 失败\n请求级别 " +
                               std::to_string(result.requestedLevel) + "，按 + 打开专属菜单");
    }
}

} // namespace beiklive
