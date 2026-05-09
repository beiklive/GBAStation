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
        iconLabel->setText("\uE14A");
        iconLabel->setFontSize(36.f);
        iconLabel->setTextColor(nvgRGB(79, 193, 255));
        iconLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        iconLabel->setMarginBottom(12.f);
        iconLabel->setFocusable(false);
        card->addView(iconLabel);

        m_progressTitleLabel = new brls::Label();
        m_progressTitleLabel->setText("正在导入...");
        m_progressTitleLabel->setFontSize(24.f);
        m_progressTitleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        m_progressTitleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_progressTitleLabel->setMarginBottom(16.f);
        m_progressTitleLabel->setFocusable(false);
        card->addView(m_progressTitleLabel);

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

    void GameImportView::onSelectLpl(int platform)
    {
        if (m_importing) return;

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
                    startImport(selectedPath, platform);
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
        m_importing = true;
        showProgressLayout();
        m_progressTitleLabel->setText("正在解析LPL文件...");
        m_progressCountLabel->setText("");
        m_progressBar->setWidth(0.f);

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
        m_totalItems = (int)items.size();
        m_importedCount = 0;

        m_progressTitleLabel->setText("正在导入游戏数据...");

        namespace sk = beiklive::SettingKey;

        for (int i = 0; i < m_totalItems; ++i)
        {
            auto& item = items[i];
            std::string romPath = expandTilde(item.value("path", ""));
            std::string label = item.value("label", "");

            if (romPath.empty() || label.empty())
            {
                m_importedCount = i + 1;
                float progress = (float)m_importedCount / (float)m_totalItems;
                m_progressBar->setWidth(400.f * progress);
                std::ostringstream oss;
                oss << m_importedCount << " / " << m_totalItems;
                m_progressCountLabel->setText(oss.str());
                this->invalidate();
                continue;
            }

            std::string romStem = getStemFromPath(romPath);

            uint32_t crc = 0;
            if (fs::exists(romPath))
                crc = beiklive::tools::crc32(romPath);

            std::string logoPath;
            {
                std::string thumbPath = "/retroarch/thumbnails";
                    std::string logoFile = thumbPath + "/Named_Snaps/" + romStem + ".png";
#ifdef _WIN32
                    std::string altLogo = logoFile;
                    for (auto& c : altLogo) if (c == '/') c = '\\';
                    if (fs::exists(altLogo))
                        logoPath = altLogo;
                    else
#endif
                        logoPath = logoFile;

            std::string pDirName = platformDirName(platform);
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

            std::string overlayPath;
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
                overlayPath = GET_SETTING_KEY_STR(overlayKey.c_str(), "");

            std::string shaderPath;
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
                shaderPath = GET_SETTING_KEY_STR(shaderKey.c_str(), "");
            if (shaderPath.empty())
                shaderPath = GET_SETTING_KEY_STR(sk::KEY_DISPLAY_SHADER_PATH, "");

            beiklive::GameEntry entry;
            entry.path = romPath;
            entry.title = label;
            entry.platform = platform;
            entry.crc32 = (int)crc;
            entry.cheatPath = "";
            entry.logoPath = logoPath;
            entry.savePath = savePath;
            entry.overlayPath = overlayPath;
            entry.shaderPath = shaderPath;
            entry.overlayEnabled = GET_SETTING_KEY_INT(sk::KEY_DISPLAY_OVERLAY_ENABLED, 0) != 0;
            entry.shaderEnabled = GET_SETTING_KEY_INT(sk::KEY_DISPLAY_SHADER_ENABLED, 0) != 0;

            beiklive::GameDB->upsert(entry);

            m_importedCount = i + 1;

            float progress = (float)m_importedCount / (float)m_totalItems;
            m_progressBar->setWidth(400.f * progress);

            std::ostringstream oss;
            oss << m_importedCount << " / " << m_totalItems;
            m_progressCountLabel->setText(oss.str());

            this->invalidate();
        }

        beiklive::GameDB->flush();

        m_progressBar->setWidth(400.f);
        m_progressBar->setColor(nvgRGB(129, 199, 132));
        m_progressTitleLabel->setText("导入完成");
        m_progressCountLabel->setText("共导入 " + std::to_string(m_totalItems) + " 个游戏");

        auto* doneDialog = new brls::Dialog("导入完成\n\n共导入 " + std::to_string(m_totalItems) + " 个游戏");
        doneDialog->addButton("确认", [this]() {
            brls::Application::popActivity(brls::TransitionAnimation::NONE);
        });
        doneDialog->open();
    }

}
