#include "ui/page/NdsDekoGamePage.hpp"

#include "core/Tools.hpp"
#include "ui/utils/AnimationHelper.hpp"

#include <borealis/views/dialog.hpp>

#include <filesystem>

namespace beiklive {

namespace {
constexpr int kMenuSlideMs = 160;
constexpr int kMenuOffset = 80;
}

NdsDekoGamePage::NdsDekoGamePage(beiklive::DirListData gameData)
    : m_gameData(std::move(gameData))
{
    if (!beiklive::tools::isFileExists(m_gameData.fullPath)) {
        brls::Application::notify("文件不存在: " + m_gameData.fileName);
        brls::sync([]() { brls::Application::popActivity(); });
        return;
    }
    _initGameEntryFromDir();
}

NdsDekoGamePage::NdsDekoGamePage(beiklive::GameEntry gameEntry)
    : m_gameEntry(std::move(gameEntry))
{
    if (!beiklive::tools::isFileExists(m_gameEntry.path)) {
        brls::Application::notify("文件不存在: " + m_gameEntry.title);
        brls::sync([]() { brls::Application::popActivity(); });
        return;
    }
    _initGameEntryPaths();
    _updateGameCount();
}

NdsDekoGamePage::~NdsDekoGamePage()
{
    brls::Logger::debug("NdsDekoGamePage: destructor for {}", m_gameEntry.title);
}

void NdsDekoGamePage::_initGameEntryFromDir()
{
    if (!beiklive::GameDB)
        return;

    const auto platform = static_cast<int>(m_gameData.itemType);
    if (!beiklive::GameDB->findByPath(m_gameData.fullPath).has_value()) {
        beiklive::GameEntry minimal;
        minimal.path = m_gameData.fullPath;
        minimal.platform = platform;
        minimal.core = beiklive::GetDefaultCoreId(platform);
        minimal.title = GET_MAPPING_KEY_STR(
            beiklive::tools::getFileNameWithoutExtension(m_gameData.fileName),
            beiklive::tools::getFileNameWithoutExtension(m_gameData.fileName));
        minimal.savePath = beiklive::tools::defaultGameSavePath(platform, minimal.path);
        minimal.logoPath = beiklive::tools::getDefaultLogoPath(
            static_cast<beiklive::enums::EmuPlatform>(platform));
        minimal.ndsScreenLayout = "vertical";
        minimal.ndsScreenOrientation = "0";
        std::filesystem::create_directories(minimal.savePath);
        beiklive::GameDB->upsertByPath(minimal);
    }

    m_gameEntry = beiklive::GameDB->findByPath(m_gameData.fullPath).value();
    _initGameEntryPaths();
    _updateGameCount();
}

void NdsDekoGamePage::_initGameEntryPaths()
{
    if (m_gameEntry.savePath.empty()) {
        m_gameEntry.savePath = beiklive::tools::defaultGameSavePath(m_gameEntry.platform, m_gameEntry.path);
        std::filesystem::create_directories(m_gameEntry.savePath);
    }
    if (m_gameEntry.core.empty())
        m_gameEntry.core = beiklive::GetDefaultCoreId(m_gameEntry.platform);
    m_gameEntry.core = beiklive::NormalizeCoreId(m_gameEntry.platform, m_gameEntry.core);

    if (m_gameEntry.ndsScreenLayout.empty())
        m_gameEntry.ndsScreenLayout = "vertical";
    if (m_gameEntry.ndsScreenOrientation.empty())
        m_gameEntry.ndsScreenOrientation = "0";
    m_gameEntry.ndsInternalResolution = 1;

    if (m_gameEntry.logoPath.empty()) {
        m_gameEntry.logoPath = beiklive::tools::getDefaultLogoPath(
            static_cast<beiklive::enums::EmuPlatform>(m_gameEntry.platform));
    }
    if (m_gameEntry.screenShotPath.empty())
        m_gameEntry.screenShotPath = beiklive::path::screenshotPath();

    if (beiklive::GameDB && !m_gameEntry.path.empty())
        beiklive::GameDB->upsertByPath(m_gameEntry);
}

void NdsDekoGamePage::_updateGameCount()
{
    if (!beiklive::GameDB || m_gameEntry.path.empty())
        return;

    m_gameEntry.lastPlayed = beiklive::tools::getTimestampString();
    m_gameEntry.playCount += 1;
    beiklive::GameDB->set(m_gameEntry.path, "lastPlayed", m_gameEntry.lastPlayed);
    beiklive::GameDB->set(m_gameEntry.path, "playCount", m_gameEntry.playCount);
    if (beiklive::GameDB)
        beiklive::GameDB->flush();
}

void NdsDekoGamePage::_pageInit()
{
    showFooter(false);
    showHeader(false);
    showBackground(false);
    showShader(false);

    setAxis(brls::Axis::COLUMN);
    setAlignItems(brls::AlignItems::CENTER);
    setJustifyContent(brls::JustifyContent::CENTER);
    setFocusable(false);
    setBackground(brls::ViewBackground::NONE);
    setWidthPercentage(100.f);
    setHeightPercentage(100.f);
    getContentBox()->setMarginRight(0.f);
    getContentBox()->setMarginLeft(0.f);
}

void NdsDekoGamePage::_setupGame()
{
    _pageInit();

    m_gameView = new NdsDekoGameView(m_gameEntry);
    m_gameView->setWidthPercentage(100.f);
    m_gameView->setHeightPercentage(100.f);
    m_gameView->setPositionType(brls::PositionType::ABSOLUTE);
    m_gameView->setPositionTop(0);
    m_gameView->setPositionLeft(0);
    m_gameView->setOnOpenMenu([this]() { _openMenu(); });
    getContentBox()->addView(m_gameView);

    m_gameMenuView = new NdsDekoGameMenuView(m_gameEntry);
    m_gameMenuView->setWidthPercentage(100.f);
    m_gameMenuView->setHeightPercentage(100.f);
    m_gameMenuView->setPositionType(brls::PositionType::ABSOLUTE);
    m_gameMenuView->setPositionTop(0);
    m_gameMenuView->setPositionLeft(0);
    m_gameMenuView->setVisibility(brls::Visibility::GONE);
    m_gameMenuView->setOnResume([this]() { _closeMenu(); });
    m_gameMenuView->setOnExit([this]() { _exitToPreviousPage(); });
    getContentBox()->addView(m_gameMenuView);

    brls::sync([this]() { brls::Application::giveFocus(m_gameView); });
}

void NdsDekoGamePage::startGame()
{
    if (!m_started) {
        m_started = true;
        _setupGame();
    }
    if (m_gameView)
        m_gameView->startProbe();
}

void NdsDekoGamePage::_openMenu()
{
    if (!m_gameMenuView || m_gameMenuView->getVisibility() == brls::Visibility::VISIBLE)
        return;

    brls::Logger::info("NdsDekoGamePage: open stage0 menu");
    m_gameMenuView->setVisibility(brls::Visibility::VISIBLE);
    m_gameMenuView->onShow();
    m_gameView->setFocusable(false);
    m_gameMenuView->setFocusable(true);
    AnimationHelper::slideInFromBottom(m_gameMenuView, static_cast<float>(kMenuOffset), kMenuSlideMs);
    brls::Application::giveFocus(m_gameMenuView);
}

void NdsDekoGamePage::_closeMenu()
{
    if (!m_gameMenuView)
        return;

    if (beiklive::GameDB)
        beiklive::GameDB->flush();
    m_gameView->setFocusable(true);
    AnimationHelper::slideOutToBottom(m_gameMenuView,
                                      static_cast<float>(kMenuOffset),
                                      kMenuSlideMs,
                                      true,
                                      [this]() { brls::Application::giveFocus(m_gameView); });
}

void NdsDekoGamePage::_exitToPreviousPage()
{
    if (m_exitRequested)
        return;
    m_exitRequested = true;

    brls::Logger::info("NdsDekoGamePage: exit stage0 page");
    if (beiklive::GameDB)
        beiklive::GameDB->flush();

    brls::sync([this]() { beiklive::popActivity(this); });
}

} // namespace beiklive
