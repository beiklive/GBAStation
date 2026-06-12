#include "DataManagementPage.hpp"

#include "ui/page/FileListPage.hpp"
#include "ui/utils/UiHelper.hpp"
#include "ui/widget/DetailCell.hpp"
#include "core/Tools.hpp"

#include <borealis/views/applet_frame.hpp>
#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/dialog.hpp>
#include <borealis/views/header.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/rectangle.hpp>

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace fs = std::filesystem;

namespace
{

struct ImportItem
{
    std::string romPath;
    std::string label;
};

struct ImportSharedConfig
{
    int platform = -1;
    std::string overlayPath;
    std::string shaderPath;
    bool overlayEnabled = false;
    bool shaderEnabled = false;
};

std::string expandTilde(const std::string& path)
{
    if (!path.empty() && path[0] == '~')
    {
        const char* home = nullptr;
#ifdef _WIN32
        home = std::getenv("USERPROFILE");
#else
        home = std::getenv("HOME");
#endif
        if (home)
            return std::string(home) + path.substr(1);
    }
    return path;
}

std::string fileNameFromPath(const std::string& path)
{
    return fs::path(path).filename().string();
}

std::string stemFromPath(const std::string& path)
{
    return fs::path(path).stem().string();
}

std::string parentPath(const std::string& path)
{
    return fs::path(path).parent_path().string();
}

std::string normalizeExtension(std::string ext)
{
    if (ext.size() > 1 && ext[0] == '.')
        ext = ext.substr(1);

    for (auto& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    return ext;
}

int platformFromExtension(const std::string& ext)
{
    if (ext == "gba") return static_cast<int>(beiklive::enums::EmuPlatform::EmuGBA);
    if (ext == "gbc") return static_cast<int>(beiklive::enums::EmuPlatform::EmuGBC);
    if (ext == "gb") return static_cast<int>(beiklive::enums::EmuPlatform::EmuGB);
    if (ext == "nes" || ext == "fds") return static_cast<int>(beiklive::enums::EmuPlatform::EmuNES);
    if (ext == "sfc" || ext == "smc") return static_cast<int>(beiklive::enums::EmuPlatform::EmuSNES);
    return -1;
}

std::string overlayKeyForPlatform(int platform)
{
    namespace sk = beiklive::SettingKey;
    switch (static_cast<beiklive::enums::EmuPlatform>(platform))
    {
    case beiklive::enums::EmuPlatform::EmuGBA: return sk::KEY_DISPLAY_OVERLAY_GBA_PATH;
    case beiklive::enums::EmuPlatform::EmuGBC: return sk::KEY_DISPLAY_OVERLAY_GBC_PATH;
    case beiklive::enums::EmuPlatform::EmuGB: return sk::KEY_DISPLAY_OVERLAY_GB_PATH;
    case beiklive::enums::EmuPlatform::EmuNES: return sk::KEY_DISPLAY_OVERLAY_NES_PATH;
    case beiklive::enums::EmuPlatform::EmuSNES: return sk::KEY_DISPLAY_OVERLAY_SNES_PATH;
    default: return "";
    }
}

std::string shaderKeyForPlatform(int platform)
{
    namespace sk = beiklive::SettingKey;
    switch (static_cast<beiklive::enums::EmuPlatform>(platform))
    {
    case beiklive::enums::EmuPlatform::EmuGBA: return sk::KEY_DISPLAY_SHADER_GBA_PATH;
    case beiklive::enums::EmuPlatform::EmuGBC: return sk::KEY_DISPLAY_SHADER_GBC_PATH;
    case beiklive::enums::EmuPlatform::EmuGB: return sk::KEY_DISPLAY_SHADER_GB_PATH;
    case beiklive::enums::EmuPlatform::EmuNES: return sk::KEY_DISPLAY_SHADER_NES_PATH;
    case beiklive::enums::EmuPlatform::EmuSNES: return sk::KEY_DISPLAY_SHADER_SNES_PATH;
    default: return "";
    }
}

ImportSharedConfig buildSharedConfig(int platform)
{
    namespace sk = beiklive::SettingKey;

    ImportSharedConfig config;
    config.platform = platform;
    config.overlayEnabled = GET_SETTING_KEY_INT(sk::KEY_DISPLAY_OVERLAY_ENABLED, 0) != 0;
    config.shaderEnabled = GET_SETTING_KEY_INT(sk::KEY_DISPLAY_SHADER_ENABLED, 0) != 0;

    std::string overlayKey = overlayKeyForPlatform(platform);
    if (!overlayKey.empty())
        config.overlayPath = GET_SETTING_KEY_STR(overlayKey.c_str(), "");

    std::string shaderKey = shaderKeyForPlatform(platform);
    if (!shaderKey.empty())
        config.shaderPath = GET_SETTING_KEY_STR(shaderKey.c_str(), "");
    if (config.shaderPath.empty())
        config.shaderPath = GET_SETTING_KEY_STR(sk::KEY_DISPLAY_SHADER_PATH, "");

    return config;
}

void applyDisplayDefaults(beiklive::GameEntry& entry)
{
    std::string mode = GET_SETTING_KEY_STR("display.mode", "original");
    if (mode == "fill")
        entry.displayMode = 1;
    else if (mode == "integer")
        entry.displayMode = 2;
    else if (mode == "custom")
        entry.displayMode = 3;
    else
        entry.displayMode = 0;

    entry.integerAspectRatio =
        static_cast<float>(GET_SETTING_KEY_INT("display.integer_scale_mult", 0));
}

} // namespace

namespace beiklive
{

DataManagementPage::DataManagementPage()
{
    this->showHeader(true);
    this->showFooter(true);
    this->getHeader()->setTitle("数据管理");
    this->setFocusable(false);

    this->registerAction("返回", brls::BUTTON_B, [this](brls::View*) { 
        beiklive::popActivity(this);
        return true;
    });


    m_tabframe = new beiklive::TabFrame();
    this->getContentBox()->addView(m_tabframe);
    setupProgressOverlay();
    init();
}

DataManagementPage::~DataManagementPage()
{
    m_alive.store(false, std::memory_order_release);
    finishWorker();
}

void DataManagementPage::draw(
    NVGcontext* vg, float x, float y, float w, float h,
    brls::Style style, brls::FrameContext* ctx)
{
    beiklive::Box::draw(vg, x, y, w, h, style, ctx);

    if (!m_progressOverlay)
        return;

    if (m_importing.load(std::memory_order_acquire))
    {
        int cur = m_progress.load(std::memory_order_acquire);
        int tot = m_total.load(std::memory_order_acquire);
        float frac = (tot > 0) ? static_cast<float>(cur) / static_cast<float>(tot) : 0.f;

        {
            std::lock_guard<std::mutex> lock(m_statusMutex);
            m_progressNameLabel->setText(m_progressName.empty() ? " " : m_progressName);
        }

        m_progressBar->setWidth(400.f * frac);
        m_progressCountLabel->setText(std::to_string(cur) + " / " + std::to_string(tot));

        if (m_importError.load(std::memory_order_acquire))
        {
            m_importing.store(false, std::memory_order_release);
            m_progressTitleLabel->setText(
                m_progressTask == ProgressTask::Cleanup ? "处理失败" : "导入失败");
            {
                std::lock_guard<std::mutex> lock(m_statusMutex);
                std::string err = m_errorMsg.empty() ? "未知错误" : m_errorMsg;
                if (err.size() > 60)
                    err = err.substr(0, 60) + "...";
                m_progressCountLabel->setText(err);
            }
            hideProgressOverlay();

            if (!m_completionShown)
            {
                m_completionShown = true;
                std::string err;
                {
                    std::lock_guard<std::mutex> lock(m_statusMutex);
                    err = m_errorMsg.empty() ? "未知错误" : m_errorMsg;
                }
                rememberFocusBeforeModal();
                auto* dialog = new brls::Dialog(
                    std::string(m_progressTask == ProgressTask::Cleanup ? "处理失败\n\n"
                                                                       : "导入失败\n\n") +
                    err);
                dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
                dialog->open();
            }
        }
        else if (m_importDone.load(std::memory_order_acquire))
        {
            m_importing.store(false, std::memory_order_release);
            finishWorker();
            if (beiklive::GameDB && m_progressTask == ProgressTask::Import)
                beiklive::GameDB->flush();

            int total = m_total.load(std::memory_order_acquire);
            m_progressBar->setWidth(400.f);
            m_progressBar->setColor(nvgRGB(129, 199, 132));
            if (m_progressTask == ProgressTask::Cleanup)
            {
                int removed = m_cleanupRemoved.load(std::memory_order_acquire);
                m_progressTitleLabel->setText("处理完成");
                m_progressCountLabel->setText(
                    "已扫描 " + std::to_string(total) + " 个游戏，移除 " +
                    std::to_string(removed) + " 个无效记录");
            }
            else
            {
                m_progressTitleLabel->setText("导入完成");
                m_progressCountLabel->setText("共处理 " + std::to_string(total) + " 个游戏");
            }
            hideProgressOverlay();

            if (!m_completionShown)
            {
                m_completionShown = true;
                rememberFocusBeforeModal();
                std::string dialogText;
                if (m_progressTask == ProgressTask::Cleanup)
                {
                    int removed = m_cleanupRemoved.load(std::memory_order_acquire);
                    dialogText = removed > 0
                        ? "处理完成\n\n已移除 " + std::to_string(removed) + " 个无效游戏记录"
                        : "处理完成\n\n没有发现无效游戏记录";
                }
                else
                {
                    dialogText = "导入完成\n\n共处理 " + std::to_string(total) + " 个游戏";
                }
                auto* dialog = new brls::Dialog(dialogText);
                dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
                dialog->open();
            }
        }
        else
        {
            invalidate();
        }
    }
}

void DataManagementPage::init()
{
    m_tabframe->addTab(
        "扫描导入",
        BK_RES("img/ui/setting/game.png"),
        nullptr,
        nullptr,
        nullptr,
        buildScanImportTab(),
        m_scanDefaultFocus);
    m_tabframe->addTab(
        "整合包导入",
        BK_RES("img/ui/setting/emu.png"),
        nullptr,
        nullptr,
        nullptr,
        buildBundleImportTab(),
        m_bundleDefaultFocus);
    m_tabframe->addTab(
        "数据处理",
        BK_RES("img/ui/setting/debug.png"),
        nullptr,
        nullptr,
        nullptr,
        buildDataProcessingTab(),
        m_processDefaultFocus);
    m_tabframe->addFinish();
}

void DataManagementPage::setupProgressOverlay()
{
    m_progressOverlay = new brls::Box(brls::Axis::COLUMN);
    m_progressOverlay->setVisibility(brls::Visibility::GONE);
    m_progressOverlay->setFocusable(true);
    m_progressOverlay->setHideHighlight(true);
    m_progressOverlay->setPositionType(brls::PositionType::ABSOLUTE);
    m_progressOverlay->setPositionTop(0);
    m_progressOverlay->setPositionLeft(0);
    m_progressOverlay->setWidthPercentage(100.f);
    m_progressOverlay->setHeightPercentage(100.f);
    m_progressOverlay->setBackgroundColor(nvgRGBA(0, 0, 0, 140));
    m_progressOverlay->setJustifyContent(brls::JustifyContent::CENTER);
    m_progressOverlay->setAlignItems(brls::AlignItems::CENTER);
    m_progressOverlay->registerAction("返回", brls::BUTTON_B, [](brls::View*) { 
        return true; 
    
    });

    auto* card = new brls::Box(brls::Axis::COLUMN);
    card->setFocusable(false);
    card->setCornerRadius(18.f);
    card->setBackgroundColor(nvgRGBA(30, 30, 35, 235));
    card->setShadowType(brls::ShadowType::GENERIC);
    card->setShadowVisibility(true);
    card->setPadding(34.f, 44.f, 34.f, 44.f);
    card->setWidth(560.f);
    card->setAlignItems(brls::AlignItems::CENTER);

    m_progressTitleLabel = new brls::Label();
    m_progressTitleLabel->setText("准备就绪");
    m_progressTitleLabel->setFontSize(24.f);
    m_progressTitleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_progressTitleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_progressTitleLabel->setMarginBottom(16.f);
    m_progressTitleLabel->setFocusable(false);
    card->addView(m_progressTitleLabel);

    m_progressNameLabel = new brls::Label();
    m_progressNameLabel->setText(" ");
    m_progressNameLabel->setFontSize(28.f);
    m_progressNameLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_progressNameLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_progressNameLabel->setMarginBottom(16.f);
    m_progressNameLabel->setFocusable(false);
    card->addView(m_progressNameLabel);

    m_progressCountLabel = new brls::Label();
    m_progressCountLabel->setText("0 / 0");
    m_progressCountLabel->setFontSize(18.f);
    m_progressCountLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
    m_progressCountLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_progressCountLabel->setMarginBottom(18.f);
    m_progressCountLabel->setFocusable(false);
    card->addView(m_progressCountLabel);

    auto* progressTrack = new brls::Box(brls::Axis::ROW);
    progressTrack->setWidth(400.f);
    progressTrack->setHeight(8.f);
    progressTrack->setCornerRadius(4.f);
    progressTrack->setBackgroundColor(nvgRGBA(255, 255, 255, 30));
    progressTrack->setFocusable(false);

    m_progressBar = new brls::Rectangle(nvgRGB(79, 193, 255));
    m_progressBar->setWidth(0.f);
    m_progressBar->setHeight(8.f);
    m_progressBar->setCornerRadius(4.f);
    m_progressBar->setFocusable(false);
    progressTrack->addView(m_progressBar);

    card->addView(progressTrack);
    m_progressOverlay->addView(card);
    this->addView(m_progressOverlay);
}

void DataManagementPage::showProgressOverlay()
{
    if (!m_progressOverlay)
        return;
    rememberFocusBeforeModal();
    m_progressOverlay->setVisibility(brls::Visibility::VISIBLE);
    brls::Application::giveFocus(m_progressOverlay);
}

void DataManagementPage::hideProgressOverlay()
{
    if (!m_progressOverlay)
        return;
    m_progressOverlay->setVisibility(brls::Visibility::GONE);
    restoreFocusAfterModal();
}

void DataManagementPage::rememberFocusBeforeModal()
{
    brls::View* currentFocus = brls::Application::getCurrentFocus();
    if (!currentFocus || currentFocus == m_progressOverlay || currentFocus->isHidden())
        return;

    m_focusBeforeModal = currentFocus;
}

brls::View* DataManagementPage::getFallbackFocus()
{
    if (m_scanDefaultFocus && !m_scanDefaultFocus->isHidden())
        return m_scanDefaultFocus;

    if (m_bundleDefaultFocus && !m_bundleDefaultFocus->isHidden())
        return m_bundleDefaultFocus;

    if (m_processDefaultFocus && !m_processDefaultFocus->isHidden())
        return m_processDefaultFocus;

    if (m_scanDefaultFocus)
        return m_scanDefaultFocus;

    if (m_bundleDefaultFocus)
        return m_bundleDefaultFocus;

    return m_processDefaultFocus;
}

void DataManagementPage::restoreFocusAfterModal()
{
    brls::View* targetFocus = nullptr;
    if (m_focusBeforeModal && !m_focusBeforeModal->isHidden())
        targetFocus = m_focusBeforeModal;
    else
        targetFocus = getFallbackFocus();

    m_focusBeforeModal = nullptr;

    if (targetFocus)
        brls::Application::giveFocus(targetFocus);
}

brls::View* DataManagementPage::buildScanImportTab()
{
    using beiklive::ui::makeContentBox;
    using beiklive::ui::makeHeader;
    using beiklive::ui::makeHint;
    using beiklive::ui::makeScrollTab;

    auto* scroll = makeScrollTab();
    auto* box = makeContentBox();

    box->addView(makeHeader("扫描目录并导入"));

    auto* scanBtn = new beiklive::DetailCell();
    scanBtn->setLeftText("选择ROM目录并导入");
    scanBtn->setRightText("\uE14A");
    scanBtn->registerAction("选择", brls::BUTTON_A, [this](brls::View*) -> bool {
        if (m_importing.load(std::memory_order_acquire))
            return true;
        selectRomDir();
        return true;
    });
    box->addView(scanBtn);
    m_scanDefaultFocus = scanBtn;

    box->addView(makeHint("扫描时会根据下面的开关确认扫描对象，默认全部类型都导入，可按自己需要开关"));

    auto* subDirSwitch = new brls::BooleanCell();
    subDirSwitch->init("自动扫描子目录", m_autoSubDir, [this](bool on) { m_autoSubDir = on; });
    box->addView(subDirSwitch);

    auto* nameMapSwitch = new brls::BooleanCell();
    nameMapSwitch->init("自动读取映射名称(如果存在)", m_useNameMapping, [this](bool on) { m_useNameMapping = on; });
    box->addView(nameMapSwitch);

    auto* gbaSwitch = new brls::BooleanCell();
    gbaSwitch->init("扫描GBA游戏", m_scanGBA, [this](bool on) { m_scanGBA = on; });
    box->addView(gbaSwitch);

    auto* gbcSwitch = new brls::BooleanCell();
    gbcSwitch->init("扫描GBC游戏", m_scanGBC, [this](bool on) { m_scanGBC = on; });
    box->addView(gbcSwitch);

    auto* gbSwitch = new brls::BooleanCell();
    gbSwitch->init("扫描GB游戏", m_scanGB, [this](bool on) { m_scanGB = on; });
    box->addView(gbSwitch);

    auto* nesSwitch = new brls::BooleanCell();
    nesSwitch->init("扫描FC游戏", m_scanNES, [this](bool on) { m_scanNES = on; });
    box->addView(nesSwitch);

    auto* snesSwitch = new brls::BooleanCell();
    snesSwitch->init("扫描SFC游戏", m_scanSNES, [this](bool on) { m_scanSNES = on; });
    box->addView(snesSwitch);


    scroll->setContentView(box);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

brls::View* DataManagementPage::buildBundleImportTab()
{
    using beiklive::ui::makeContentBox;
    using beiklive::ui::makeHeader;
    using beiklive::ui::makeHint;
    using beiklive::ui::makeScrollTab;

    auto* scroll = makeScrollTab();
    auto* box = makeContentBox();
    box->addView(makeHeader("导入 RetroArch 整合包"));

    struct LplButtonConfig
    {
        const char* text;
        const char* icon;
        int platform;
    };

    const LplButtonConfig configs[] = {
        {"选择GBA游戏的lpl文件",  "img/ui/icon_gba.png", static_cast<int>(beiklive::enums::EmuPlatform::EmuGBA)},
        {"选择GBC游戏的lpl文件",  "img/ui/icon_gb.png",  static_cast<int>(beiklive::enums::EmuPlatform::EmuGBC)},
        {"选择GB游戏的lpl文件",   "img/ui/icon_gb.png",  static_cast<int>(beiklive::enums::EmuPlatform::EmuGB)},
        {"选择FC游戏的lpl文件",   "img/ui/icon_gba.png", static_cast<int>(beiklive::enums::EmuPlatform::EmuNES)},
        {"选择SFC游戏的lpl文件",  "img/ui/icon_gba.png", static_cast<int>(beiklive::enums::EmuPlatform::EmuSNES)},
    };
    
    box->addView(makeHint("lpl 文件通常位于 RetroArch 的 playlists 目录下，不动lpl文件语法规则不要自行删改"));
    for (size_t i = 0; i < sizeof(configs) / sizeof(configs[0]); ++i)
    {
        const auto& config = configs[i];
        auto* btn = new beiklive::DetailCell();
        btn->setLeftText(config.text);
        btn->setRightText("\uE14A");
        btn->registerAction("选择", brls::BUTTON_A, [this, config](brls::View*) -> bool {
            if (m_importing.load(std::memory_order_acquire))
                return true;
            onSelectLpl(config.platform);
            return true;
        });
        box->addView(btn);

        if (i == 0)
            m_bundleDefaultFocus = btn;
    }


    scroll->setContentView(box);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

brls::View* DataManagementPage::buildDataProcessingTab()
{
    using beiklive::ui::makeContentBox;
    using beiklive::ui::makeHeader;
    using beiklive::ui::makeHint;
    using beiklive::ui::makeScrollTab;

    auto* scroll = makeScrollTab();
    auto* box = makeContentBox();

    box->addView(makeHeader("库数据处理"));

    auto* cleanCell = new beiklive::DetailCell();
    cleanCell->setLeftText("从库中移除无效游戏");
    cleanCell->setRightText("\uE14A");
    cleanCell->registerAction("打开", brls::BUTTON_A, [this](brls::View*) -> bool {
        removeInvalidGames();
        return true;
    });
    box->addView(cleanCell);
    m_processDefaultFocus = cleanCell;

    box->addView(makeHint("移除游戏库中仍有记录，但 ROM 文件已经不存在的游戏。"));

    scroll->setContentView(box);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

void DataManagementPage::resetProgressUi(const std::string& title)
{
    m_completionShown = false;
    m_cleanupRemoved.store(0, std::memory_order_release);
    m_importDone.store(false, std::memory_order_release);
    m_importError.store(false, std::memory_order_release);
    m_progress.store(0, std::memory_order_release);
    m_total.store(0, std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_errorMsg.clear();
        m_progressName.clear();
    }

    showProgressOverlay();
    m_progressTitleLabel->setText(title);
    m_progressNameLabel->setText(" ");
    m_progressCountLabel->setText("0 / 0");
    m_progressBar->setWidth(0.f);
    m_progressBar->setColor(nvgRGB(79, 193, 255));

    invalidate();
}

void DataManagementPage::updateProgressName(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    m_progressName = name;
}

void DataManagementPage::setErrorMessage(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(m_statusMutex);
    m_errorMsg = msg;
}

void DataManagementPage::finishWorker()
{
    if (m_importThread.joinable())
        m_importThread.join();
}

void DataManagementPage::onSelectLpl(int platform)
{
    auto* flPage = new beiklive::FileListPage();
    flPage->setFliter(beiklive::enums::FilterMode::Whitelist, {"lpl"});
    flPage->onFileSelected = [this, platform](beiklive::DirListData item) {
        if (item.itemType == beiklive::enums::FileType::DRIVE ||
            item.itemType == beiklive::enums::FileType::DIRECTORY)
            return;

        std::string selectedPath = item.fullPath;
        brls::Application::popActivity(brls::TransitionAnimation::NONE);

        rememberFocusBeforeModal();
        auto* dialog = new brls::Dialog(
            "导入游戏库\n\n文件: " + fileNameFromPath(selectedPath));
        dialog->addButton("取消", [this]() { restoreFocusAfterModal(); });
        dialog->addButton("确定导入", [this, selectedPath, platform]() {
            startImport(selectedPath, platform);
        });
        dialog->open();
    };

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->addView(flPage);
    container->registerAction("关闭"_i18n, brls::BUTTON_START,
                              [](brls::View*) { brls::Application::popActivity(); return true; });

    auto* frame = new brls::AppletFrame(container);
    frame->setHeaderVisibility(brls::Visibility::GONE);
    frame->setFooterVisibility(brls::Visibility::GONE);
    frame->setBackground(brls::ViewBackground::NONE);
    brls::Application::pushActivity(new brls::Activity(frame));

    flPage->showDriveList();
}

void DataManagementPage::startImport(const std::string& lplPath, int platform)
{
    m_progressTask = ProgressTask::Import;
    resetProgressUi("正在解析LPL文件...");
    finishWorker();

    std::string realPath = expandTilde(lplPath);
    std::ifstream ifs(realPath);
    if (!ifs.is_open())
    {
        hideProgressOverlay();
        rememberFocusBeforeModal();
        auto* dialog = new brls::Dialog("无法打开LPL文件");
        dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
        dialog->open();
        return;
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();

    json lplJson;
    try
    {
        lplJson = json::parse(buffer.str());
    }
    catch (const std::exception& e)
    {
        hideProgressOverlay();
        rememberFocusBeforeModal();
        auto* dialog = new brls::Dialog("LPL文件解析失败");
        dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
        dialog->open();
        return;
    }

    if (!lplJson.contains("items") || !lplJson["items"].is_array())
    {
        hideProgressOverlay();
        rememberFocusBeforeModal();
        auto* dialog = new brls::Dialog("LPL文件无数据");
        dialog->addButton("确认", [this]() { restoreFocusAfterModal(); });
        dialog->open();
        return;
    }

    std::vector<ImportItem> importItems;
    for (const auto& item : lplJson["items"])
    {
        importItems.push_back({
            item.value("path", ""),
            item.value("label", ""),
        });
    }

    m_total.store(static_cast<int>(importItems.size()), std::memory_order_release);
    m_progressCountLabel->setText("0 / " + std::to_string(importItems.size()));
    m_progressTitleLabel->setText("正在导入游戏数据，请勿操作");

    ImportSharedConfig config = buildSharedConfig(platform);
    m_importing.store(true, std::memory_order_release);

    m_importThread = std::thread([this, importItems = std::move(importItems), config, lplPath]() {
        for (int i = 0; i < static_cast<int>(importItems.size()); ++i)
        {
            const auto& item = importItems[i];
            std::string romPath = expandTilde(item.romPath);

            if (romPath.empty() || item.label.empty() || !fs::exists(romPath))
            {
                m_progress.store(i + 1, std::memory_order_release);
                continue;
            }

            updateProgressName(item.label);
            std::string romStem = stemFromPath(romPath);

            std::string logoPath;
            std::string thumbPath = romPath;
            size_t romsPos = thumbPath.find("roms");
            if (romsPos != std::string::npos)
            {
                thumbPath.replace(romsPos, 4, "retroarch/thumbnails");
                std::string thumbDir = parentPath(thumbPath);
                std::string logoFile = thumbDir + "/Named_Snaps/" + romStem + ".png";
#ifdef _WIN32
                std::string altLogo = logoFile;
                for (auto& c : altLogo)
                    if (c == '/') c = '\\';
                if (fs::exists(altLogo))
                    logoPath = altLogo;
                else
#endif
                    logoPath = logoFile;
            }

            if (logoPath.empty() || !fs::exists(logoPath))
            {
                logoPath = beiklive::tools::getDefaultLogoPath(
                    static_cast<beiklive::enums::EmuPlatform>(config.platform));
            }

            std::string playlistName = beiklive::tools::getFileNameWithoutExtension(lplPath);
            std::string savePath = beiklive::path::ROOT + beiklive::path::SPLIT_CHAR +
                                   std::string(beiklive::path::PROGRAM_NAME) +
                                   beiklive::path::SPLIT_CHAR + "saves" +
                                   beiklive::path::SPLIT_CHAR + "retroarch" +
                                   beiklive::path::SPLIT_CHAR + playlistName +
                                   beiklive::path::SPLIT_CHAR + romStem;

            try
            {
                fs::create_directories(savePath);
            }
            catch (...)
            {
            }

            beiklive::GameEntry entry;
            entry.path = romPath;
            entry.title = item.label;
            entry.platform = config.platform;
            entry.logoPath = logoPath;
            entry.savePath = savePath;
            entry.overlayPath = config.overlayPath;
            entry.shaderPath = config.shaderPath;
            entry.overlayEnabled = config.overlayEnabled;
            entry.shaderEnabled = config.shaderEnabled;
            applyDisplayDefaults(entry);

            beiklive::GameDB->upsertByPath(entry);
            m_progress.store(i + 1, std::memory_order_release);
        }

        m_importDone.store(true, std::memory_order_release);
    });
}

void DataManagementPage::selectRomDir()
{
    auto* flPage = new beiklive::FileListPage();
    flPage->setDirSelectionMode(true);
    flPage->registerAction("选择目录", brls::BUTTON_Y, [this, flPage](brls::View*) -> bool {
        std::string dirPath = flPage->getHeader()->getPath();
        if (dirPath.empty())
            return true;

        brls::Application::popActivity(brls::TransitionAnimation::NONE);
        startDirImport(dirPath);
        return true;
    });

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->addView(flPage);
    container->registerAction("关闭", brls::BUTTON_START,
                              [](brls::View*) { brls::Application::popActivity(); return true; });

    auto* frame = new brls::AppletFrame(container);
    frame->setHeaderVisibility(brls::Visibility::GONE);
    frame->setFooterVisibility(brls::Visibility::GONE);
    frame->setBackground(brls::ViewBackground::NONE);
    brls::Application::pushActivity(new brls::Activity(frame));

    flPage->showDriveList();
}

void DataManagementPage::startDirImport(const std::string& dirPath)
{
    m_progressTask = ProgressTask::Import;
    resetProgressUi("正在扫描ROM文件...");
    finishWorker();

    std::unordered_set<std::string> exts;
    if (m_scanGBA) exts.insert("gba");
    if (m_scanGBC) exts.insert("gbc");
    if (m_scanGB) exts.insert("gb");
    if (m_scanNES) { exts.insert("nes"); exts.insert("fds"); }
    if (m_scanSNES) { exts.insert("sfc"); exts.insert("smc"); }

    m_importing.store(true, std::memory_order_release);

    m_importThread = std::thread([this, dirPath, exts = std::move(exts)]() {
        std::vector<fs::path> roms;
        try
        {
            if (m_autoSubDir)
            {
                for (auto& entry : fs::recursive_directory_iterator(dirPath))
                {
                    if (!entry.is_regular_file())
                        continue;

                    std::string ext = normalizeExtension(entry.path().extension().string());
                    if (exts.count(ext))
                        roms.push_back(entry.path());
                }
            }
            else
            {
                for (auto& entry : fs::directory_iterator(dirPath))
                {
                    if (!entry.is_regular_file())
                        continue;

                    std::string ext = normalizeExtension(entry.path().extension().string());
                    if (exts.count(ext))
                        roms.push_back(entry.path());
                }
            }
        }
        catch (const std::exception& e)
        {
            setErrorMessage(e.what());
            m_importError.store(true, std::memory_order_release);
            m_importDone.store(true, std::memory_order_release);
            return;
        }

        m_total.store(static_cast<int>(roms.size()), std::memory_order_release);

        for (int i = 0; i < static_cast<int>(roms.size()); ++i)
        {
            const auto& romPath = roms[i];
            std::string path = romPath.string();
            std::string romStem = romPath.stem().string();
            std::string ext = normalizeExtension(romPath.extension().string());
            int platform = platformFromExtension(ext);
            if (platform < 0)
            {
                m_progress.store(i + 1, std::memory_order_release);
                continue;
            }

            std::string displayName = romStem;
            if (m_useNameMapping)
            {
                auto nameVal = beiklive::NameMappingManager->Get(romStem);
                if (nameVal)
                {
                    auto nameStr = nameVal->AsString();
                    if (nameStr && !nameStr->empty())
                        displayName = *nameStr;
                }
            }

            updateProgressName(displayName);
            ImportSharedConfig config = buildSharedConfig(platform);

            beiklive::GameEntry entry;
            entry.path = path;
            entry.title = displayName;
            entry.platform = platform;
            entry.logoPath = beiklive::tools::getDefaultLogoPath(
                static_cast<beiklive::enums::EmuPlatform>(platform));
            entry.overlayEnabled = config.overlayEnabled;
            entry.shaderEnabled = config.shaderEnabled;
            entry.overlayPath = config.overlayPath;
            entry.shaderPath = config.shaderPath;

            std::string savePath = beiklive::path::ROOT + beiklive::path::SPLIT_CHAR +
                                   std::string(beiklive::path::PROGRAM_NAME) +
                                   beiklive::path::SPLIT_CHAR + "saves" +
                                   beiklive::path::SPLIT_CHAR + "dirms" +
                                   beiklive::path::SPLIT_CHAR + romStem;
            try
            {
                fs::create_directories(savePath);
            }
            catch (...)
            {
            }
            entry.savePath = savePath;
            applyDisplayDefaults(entry);

            auto existing = beiklive::GameDB->findByPath(path);
            if (existing)
            {
                entry.playCount = existing->playCount;
                entry.playTime = existing->playTime;
                entry.lastPlayed = existing->lastPlayed;
                entry.favourite = existing->favourite;
                if (!existing->cheatPath.empty())
                    entry.cheatPath = existing->cheatPath;
                if (!existing->logoPath.empty())
                    entry.logoPath = existing->logoPath;
                if (!existing->savePath.empty())
                    entry.savePath = existing->savePath;
            }

            beiklive::GameDB->upsertByPath(entry);
            m_progress.store(i + 1, std::memory_order_release);
        }

        m_importDone.store(true, std::memory_order_release);
    });
}

void DataManagementPage::removeInvalidGames()
{
    rememberFocusBeforeModal();

    auto* dialog = new brls::Dialog(
        "确定要从游戏库中移除无效游戏吗？\n\n此操作将删除数据库中 ROM 文件已不存在的游戏记录。");
    dialog->addButton("取消", [this]() { restoreFocusAfterModal(); });
    dialog->addButton("确认移除", [this]() {
        m_progressTask = ProgressTask::Cleanup;
        resetProgressUi("正在扫描无效游戏...");
        finishWorker();
        m_progressTitleLabel->setText("正在扫描无效游戏，请勿操作");
        m_importing.store(true, std::memory_order_release);

        m_importThread = std::thread([this]() {
            auto entries = beiklive::GameDB ? beiklive::GameDB->getAll()
                                            : std::vector<beiklive::GameEntry>{};
            int total = static_cast<int>(entries.size());
            int removed = 0;
            m_total.store(total, std::memory_order_release);

            try
            {
                if (beiklive::GameDB)
                {
                    for (int i = 0; i < total; ++i)
                    {
                        const auto& entry = entries[i];
                        updateProgressName(entry.title.empty() ? fileNameFromPath(entry.path)
                                                               : entry.title);

                        if (!fs::exists(entry.path) &&
                            beiklive::GameDB->removeByPath(entry.path))
                            removed++;

                        m_progress.store(i + 1, std::memory_order_release);
                    }

                    if (removed == total && total > 0)
                        beiklive::GameDB->clearAll();
                    else if (removed > 0)
                        beiklive::GameDB->flush();
                }
            }
            catch (const std::exception& e)
            {
                setErrorMessage(e.what());
                m_importError.store(true, std::memory_order_release);
                m_importDone.store(true, std::memory_order_release);
                return;
            }

            m_cleanupRemoved.store(removed, std::memory_order_release);
            m_importDone.store(true, std::memory_order_release);
        });
    });
    dialog->open();
}

} // namespace beiklive
