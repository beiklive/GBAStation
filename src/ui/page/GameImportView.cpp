#include "ui/page/GameImportView.hpp"
#include "ui/page/FileListPage.hpp"
#include "ui/utils/ButtonBox.hpp"
#include "core/Tools.hpp"

#include <borealis/views/label.hpp>
#include <borealis/views/header.hpp>
#include <borealis/views/dialog.hpp>
#include <borealis/views/rectangle.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace beiklive
{

    static std::string expandTilde(const std::string& path)
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

    static std::string getFileNameFromPath(const std::string& p)
    {
        fs::path fp(p);
        return fp.filename().string();
    }

    static std::string getStemFromPath(const std::string& p)
    {
        fs::path fp(p);
        return fp.stem().string();
    }

    static std::string getParentPath(const std::string& p)
    {
        fs::path fp(p);
        return fp.parent_path().string();
    }

    GameImportView::GameImportView()
    {
        this->getHeader()->setTitle("游戏导入");
        this->showHeader(true);
        this->showFooter(true);

        this->getContentBox()->setAxis(brls::Axis::COLUMN);
        this->getContentBox()->setAlignItems(brls::AlignItems::CENTER);
        this->getContentBox()->setJustifyContent(brls::JustifyContent::CENTER);
        this->getContentBox()->setGrow(1.0f);

        setupButtonLayout();
        setupProgressLayout();
        showButtonLayout();
    }

    GameImportView::~GameImportView()
    {
        if (m_importThread.joinable())
            m_importThread.join();
    }

    std::string GameImportView::platformName(int platform)
    {
        switch (static_cast<beiklive::enums::EmuPlatform>(platform))
        {
        case beiklive::enums::EmuPlatform::EmuGBA: return "GBA";
        case beiklive::enums::EmuPlatform::EmuGBC: return "GBC";
        case beiklive::enums::EmuPlatform::EmuGB:  return "GB";
        default: return "未知";
        }
    }

    std::string GameImportView::platformDirName(int platform)
    {
        return platformName(platform);
    }

    void GameImportView::setupButtonLayout()
    {
        m_layoutBox = new brls::Box(brls::Axis::COLUMN);
        m_layoutBox->setFocusable(false);
        m_layoutBox->setAlignItems(brls::AlignItems::STRETCH);
        m_layoutBox->setJustifyContent(brls::JustifyContent::CENTER);
        m_layoutBox->setGrow(1.0f);
        m_layoutBox->setWidth(600.f);

        auto* header = new brls::Header();
        header->setTitle("导入 RetroArch 整合包");
        m_layoutBox->addView(header);

        auto* gbaBtn = new beiklive::ButtonBox();
        gbaBtn->setText("选择GBA游戏的lpl文件");
        gbaBtn->setIcon(BK_RES("img/ui/icon_gba.png"));
        gbaBtn->registerAction("选择", brls::BUTTON_A,
            [this](brls::View*) -> bool {
                onSelectLpl((int)beiklive::enums::EmuPlatform::EmuGBA);
                return true;
            });
        m_layoutBox->addView(gbaBtn);

        auto* gbcBtn = new beiklive::ButtonBox();
        gbcBtn->setText("选择GBC游戏的lpl文件");
        gbcBtn->setIcon(BK_RES("img/ui/icon_gb.png"));
        gbcBtn->registerAction("选择", brls::BUTTON_A,
            [this](brls::View*) -> bool {
                onSelectLpl((int)beiklive::enums::EmuPlatform::EmuGBC);
                return true;
            });
        m_layoutBox->addView(gbcBtn);

        auto* gbBtn = new beiklive::ButtonBox();
        gbBtn->setText("选择GB游戏的lpl文件");
        gbBtn->setIcon(BK_RES("img/ui/icon_gb.png"));
        gbBtn->registerAction("选择", brls::BUTTON_A,
            [this](brls::View*) -> bool {
                onSelectLpl((int)beiklive::enums::EmuPlatform::EmuGB);
                return true;
            });
        m_layoutBox->addView(gbBtn);

        auto* hint = new brls::Label();
        hint->setText("lpl文件通常在 /retroarch/playlists 目录下");
        hint->setFontSize(16.f);
        hint->setTextColor(nvgRGB(154, 154, 154));
        hint->setMarginTop(20.f);
        hint->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        hint->setFocusable(false);
        m_layoutBox->addView(hint);

        this->getContentBox()->addView(m_layoutBox);
    }

    void GameImportView::setupProgressLayout()
    {
        m_progressBox = new brls::Box(brls::Axis::COLUMN);
        m_progressBox->setFocusable(false);
        m_progressBox->setAlignItems(brls::AlignItems::CENTER);
        m_progressBox->setJustifyContent(brls::JustifyContent::CENTER);
        m_progressBox->setGrow(1.0f);
        m_progressBox->setVisibility(brls::Visibility::GONE);

        auto* card = new brls::Box(brls::Axis::COLUMN);
        card->setFocusable(false);
        card->setCornerRadius(16.f);
        card->setBackgroundColor(nvgRGBA(30, 30, 35, 200));
        card->setShadowType(brls::ShadowType::GENERIC);
        card->setShadowVisibility(true);
        card->setAlignItems(brls::AlignItems::CENTER);
        card->setPadding(40.f, 60.f, 40.f, 60.f);
        card->setWidth(560.f);

        auto* iconLabel = new brls::Label();
        iconLabel->setText("\uE131");
        iconLabel->setFontSize(36.f);
        iconLabel->setTextColor(nvgRGB(79, 193, 255));
        iconLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        iconLabel->setMarginBottom(12.f);
        iconLabel->setFocusable(false);
        card->addView(iconLabel);

        m_progressTitleLabel = new brls::Label();
        m_progressTitleLabel->setText("正在导入...");
        m_progressTitleLabel->setFontSize(22.f);
        m_progressTitleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        m_progressTitleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_progressTitleLabel->setMarginBottom(16.f);
        m_progressTitleLabel->setFocusable(false);
        card->addView(m_progressTitleLabel);

        m_progressNameLabel = new brls::Label();
        m_progressNameLabel->setText(" ");
        m_progressNameLabel->setFontSize(26.f);
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
        m_progressCountLabel->setMarginBottom(20.f);
        m_progressCountLabel->setFocusable(false);
        card->addView(m_progressCountLabel);

        m_progressBar = new brls::Rectangle(nvgRGB(79, 193, 255));
        m_progressBar->setWidth(0.f);
        m_progressBar->setHeight(8.f);
        m_progressBar->setCornerRadius(4.f);
        m_progressBar->setFocusable(false);
        m_progressBar->setMarginBottom(8.f);
        card->addView(m_progressBar);

        m_progressBox->addView(card);

        this->getContentBox()->addView(m_progressBox);
    }

    void GameImportView::showButtonLayout()
    {
        m_layoutBox->setVisibility(brls::Visibility::VISIBLE);
        m_progressBox->setVisibility(brls::Visibility::GONE);
    }

    void GameImportView::showProgressLayout()
    {
        m_layoutBox->setVisibility(brls::Visibility::GONE);
        m_progressBox->setVisibility(brls::Visibility::VISIBLE);
    }

    void GameImportView::draw(NVGcontext* vg, float x, float y, float w, float h,
                              brls::Style style, brls::FrameContext* ctx)
    {
        brls::Box::draw(vg, x, y, w, h, style, ctx);

        if (m_importing.load(std::memory_order_acquire))
        {
            int cur = m_progress.load(std::memory_order_acquire);
            int tot = m_total.load(std::memory_order_acquire);

            float frac = (tot > 0) ? (float)cur / (float)tot : 0.f;
            m_progressBar->setWidth(400.f * frac);

            std::ostringstream oss;
            oss << cur << " / " << tot;
            m_progressCountLabel->setText(oss.str());

            if (m_importError.load(std::memory_order_acquire))
            {
                m_importing.store(false, std::memory_order_release);
                m_progressTitleLabel->setText("导入失败");

                std::lock_guard<std::mutex> lock(m_errorMutex);
                m_progressCountLabel->setText(
                    m_errorMsg.size() > 60 ? m_errorMsg.substr(0, 60) + "..." : m_errorMsg);

                if (!m_completionShown)
                {
                    m_completionShown = true;
                    auto* dialog = new brls::Dialog("导入失败\n\n" + m_errorMsg);
                    dialog->addButton("确认", [this]() {
                        brls::Application::popActivity(brls::TransitionAnimation::NONE);
                    });
                    dialog->open();
                }
            }
            else if (m_importDone.load(std::memory_order_acquire))
            {
                m_importing.store(false, std::memory_order_release);
                m_progressBar->setWidth(400.f);
                m_progressBar->setColor(nvgRGB(129, 199, 132));
                m_progressTitleLabel->setText("正在保存数据库...");
                m_progressCountLabel->setText("共导入 " + std::to_string(tot) + " 个游戏");

                if (m_importThread.joinable())
                    m_importThread.join();

                beiklive::GameDB->flush();

                if (!m_completionShown)
                {
                    m_completionShown = true;
                    m_progressTitleLabel->setText("导入完成");
                    auto* doneDialog = new brls::Dialog("导入完成\n\n共导入 " + std::to_string(tot) + " 个游戏");
                    doneDialog->addButton("确认", [this]() {
                        brls::Application::popActivity(brls::TransitionAnimation::NONE);
                    });
                    doneDialog->open();
                }
            }
            else
            {
                invalidate();
            }
        }
    }

    void GameImportView::onSelectLpl(int platform)
    {
        if (m_importing.load(std::memory_order_acquire)) return;

        auto* flPage = new beiklive::FileListPage();
        flPage->setFliter(beiklive::enums::FilterMode::Whitelist, {"lpl"});
        flPage->onFileSelected = [this, platform](beiklive::DirListData item)
        {
            if (item.itemType != beiklive::enums::FileType::DRIVE &&
                item.itemType != beiklive::enums::FileType::DIRECTORY)
            {
                std::string selectedPath = item.fullPath;
                brls::Application::popActivity(brls::TransitionAnimation::NONE);
                auto* dialog = new brls::Dialog(
                    "导入游戏库\n\n文件: " + getFileNameFromPath(selectedPath));
                dialog->addButton("取消", []() {});
                dialog->addButton("确定导入", [this, selectedPath, platform]() {
                    brls::Application::blockInputs(false);
                    startImport(selectedPath, platform);
                    brls::Application::unblockInputs();

                });
                dialog->open();
            }
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

    void GameImportView::startImport(const std::string& lplPath, int platform)
    {
        m_completionShown = false;
        m_importDone.store(false, std::memory_order_release);
        m_importError.store(false, std::memory_order_release);
        m_progress.store(0, std::memory_order_release);

        showProgressLayout();
        m_progressTitleLabel->setText("正在解析LPL文件...");
        m_progressCountLabel->setText("");
        m_progressBar->setWidth(0.f);
        m_progressBar->setColor(nvgRGB(79, 193, 255));

        brls::Application::giveFocus(this);
        this->invalidate();

        std::string realPath = expandTilde(lplPath);
        std::ifstream ifs(realPath);
        if (!ifs.is_open())
        {
            m_progressTitleLabel->setText("无法打开LPL文件");
            m_progressCountLabel->setText(getFileNameFromPath(lplPath));
            auto* dialog = new brls::Dialog("无法打开LPL文件");
            dialog->addButton("确认", [this]() {
                brls::Application::popActivity(brls::TransitionAnimation::NONE);
            });
            dialog->open();
            return;
        }

        std::stringstream buffer;
        buffer << ifs.rdbuf();
        std::string content = buffer.str();

        json lplJson;
        try
        {
            lplJson = json::parse(content);
        }
        catch (const std::exception& e)
        {
            m_progressTitleLabel->setText("LPL文件解析失败");
            std::string errMsg = e.what();
            m_progressCountLabel->setText(errMsg.size() > 60 ? errMsg.substr(0, 60) + "..." : errMsg);
            auto* dialog = new brls::Dialog("LPL文件解析失败");
            dialog->addButton("确认", [this]() {
                brls::Application::popActivity(brls::TransitionAnimation::NONE);
            });
            dialog->open();
            return;
        }

        if (!lplJson.contains("items") || !lplJson["items"].is_array())
        {
            m_progressTitleLabel->setText("LPL文件无数据");
            m_progressCountLabel->setText("items为空");
            auto* dialog = new brls::Dialog("LPL文件无数据");
            dialog->addButton("确认", [this]() {
                brls::Application::popActivity(brls::TransitionAnimation::NONE);
            });
            dialog->open();
            return;
        }

        auto items = lplJson["items"];
        int itemCount = (int)items.size();
        m_total.store(itemCount, std::memory_order_release);

        std::vector<ImportItem> importItems;
        importItems.reserve(itemCount);
        for (int i = 0; i < itemCount; ++i)
        {
            auto& item = items[i];
            ImportItem it;
            it.romPath = item.value("path", "");
            it.label = item.value("label", "");
            importItems.push_back(std::move(it));
        }

        namespace sk = beiklive::SettingKey;
        ImportSharedConfig config;
        config.platform = platform;
        {
            std::string overlayKey;
            switch (static_cast<beiklive::enums::EmuPlatform>(platform))
            {
            case beiklive::enums::EmuPlatform::EmuGBA:
                overlayKey = sk::KEY_DISPLAY_OVERLAY_GBA_PATH; break;
            case beiklive::enums::EmuPlatform::EmuGBC:
                overlayKey = sk::KEY_DISPLAY_OVERLAY_GBC_PATH; break;
            case beiklive::enums::EmuPlatform::EmuGB:
                overlayKey = sk::KEY_DISPLAY_OVERLAY_GB_PATH; break;
            default: break;
            }
            if (!overlayKey.empty())
                config.overlayPath = GET_SETTING_KEY_STR(overlayKey.c_str(), "");
            config.overlayEnabled = GET_SETTING_KEY_INT(sk::KEY_DISPLAY_OVERLAY_ENABLED, 0) != 0;
        }
        {
            std::string shaderKey;
            switch (static_cast<beiklive::enums::EmuPlatform>(platform))
            {
            case beiklive::enums::EmuPlatform::EmuGBA:
                shaderKey = sk::KEY_DISPLAY_SHADER_GBA_PATH; break;
            case beiklive::enums::EmuPlatform::EmuGBC:
                shaderKey = sk::KEY_DISPLAY_SHADER_GBC_PATH; break;
            case beiklive::enums::EmuPlatform::EmuGB:
                shaderKey = sk::KEY_DISPLAY_SHADER_GB_PATH; break;
            default: break;
            }
            if (!shaderKey.empty())
                config.shaderPath = GET_SETTING_KEY_STR(shaderKey.c_str(), "");
            if (config.shaderPath.empty())
                config.shaderPath = GET_SETTING_KEY_STR(sk::KEY_DISPLAY_SHADER_PATH, "");
            config.shaderEnabled = GET_SETTING_KEY_INT(sk::KEY_DISPLAY_SHADER_ENABLED, 0) != 0;
        }

        m_progressTitleLabel->setText("正在导入游戏数据，请勿操作");

        m_importing.store(true, std::memory_order_release);

        if (m_importThread.joinable())
            m_importThread.join();

        m_importThread = std::thread([this, importItems = std::move(importItems), config, lplPath]()
        {
            int count = (int)importItems.size();
            for (int i = 0; i < count; ++i)
            {
                const auto& it = importItems[i];
                std::string romPath = expandTilde(it.romPath);

                if (romPath.empty() || it.label.empty())
                {
                    m_progress.store(i + 1, std::memory_order_release);
                    continue;
                }
                brls::sync([this, it](){
                    m_progressNameLabel->setText(it.label);
                });
                std::string romStem = getStemFromPath(romPath);

                uint32_t crc = 0;
                if (fs::exists(romPath))
                    crc = beiklive::tools::crc32(romPath);

                std::string logoPath;
                {
                    std::string thumbPath = romPath;
                    size_t romsPos = thumbPath.find("roms");
                    if (romsPos != std::string::npos)
                    {
                        thumbPath.replace(romsPos, 4, "retroarch/thumbnails");
                        std::string thumbDir = getParentPath(thumbPath);
                        std::string logoFile = thumbDir + "/Named_Snaps/" + romStem + ".png";
#ifdef _WIN32
                        std::string altLogo = logoFile;
                        for (auto& c : altLogo) if (c == '/') c = '\\';
                        if (fs::exists(altLogo))
                            logoPath = altLogo;
                        else
#endif
                            logoPath = logoFile;
                    }
                }

                std::string pDirName = beiklive::tools::getFileNameWithoutExtension(lplPath);
                std::string savePath = beiklive::path::ROOT + beiklive::path::SPLIT_CHAR +
                                       std::string(beiklive::path::PROGRAM_NAME) +
                                       beiklive::path::SPLIT_CHAR + "saves" +
                                       beiklive::path::SPLIT_CHAR + "retroarch" +
                                       beiklive::path::SPLIT_CHAR + pDirName +
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
                entry.title = it.label;
                entry.platform = config.platform;
                entry.crc32 = (int)crc;
                entry.cheatPath = "";
                entry.logoPath = logoPath;
                entry.savePath = savePath;
                entry.overlayPath = config.overlayPath;
                entry.shaderPath = config.shaderPath;
                entry.overlayEnabled = config.overlayEnabled;
                entry.shaderEnabled = config.shaderEnabled;

                beiklive::GameDB->upsert(entry);

                m_progress.store(i + 1, std::memory_order_release);
            }

            m_importDone.store(true, std::memory_order_release);
        });
    }

}
