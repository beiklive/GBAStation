#include "ui/page/AboutPage.hpp"
#include "ui/page/UpdatePage.hpp"
#include "ui/utils/UpdateDialog.hpp"
#include "core/AppUpdater.hpp"
#include "core/Tools.hpp"
#include <curl/curl.h>
#include <miniz.h>

namespace beiklive {

static std::string formatSize(size_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
        return buf;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.0 * 1024.0));
    return buf;
}

AboutPage::AboutPage() {
    brls::sync([this]() {
        this->showFooter(true);
        this->showHeader(false);

        m_tabFrame = new beiklive::TabFrame();
        this->getContentBox()->addView(m_tabFrame);

        m_tabFrame->addTab(
            "关于本项目",
            BK_RES("img/ui/setting/emu.png"),
            nullptr, nullptr, nullptr,
            _buildInfoTab()
        );
        m_tabFrame->addTab(
            "更新",
            BK_RES("img/ui/setting/debug.png"),
            nullptr, nullptr, nullptr,
            _buildUpdateTab()
        );
        m_tabFrame->addTab(
            "支持作者",
            BK_RES("img/ui/setting/display.png"),
            nullptr, nullptr, nullptr,
            _buildSupportTab()
        );
        m_tabFrame->addFinish();
    });
}

// ── 关于本项目 ─────────────────────────────────────────────

brls::View* AboutPage::_buildInfoTab() {
    auto* scroll = new brls::ScrollingFrame();
    scroll->setGrow(1.0f);
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    scroll->setScrollingIndicatorVisible(false);
    scroll->setFocusable(false);

    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setPadding(20.f, 40.f, 30.f, 40.f);

    // 作者卡片
    auto* authorCard = new brls::Box(brls::Axis::ROW);
    authorCard->setCornerRadius(16.f);
    authorCard->setBackgroundColor(nvgRGBA(0, 0, 0, 20));
    authorCard->setShadowVisibility(true);
    authorCard->setShadowType(brls::ShadowType::GENERIC);
    authorCard->setPadding(24.f, 36.f, 24.f, 36.f);
    authorCard->setAlignItems(brls::AlignItems::CENTER);
    authorCard->setFocusable(true);
    authorCard->setHideHighlightBackground(true);
    authorCard->setHideHighlightBorder(true);
    authorCard->setHeight(brls::View::AUTO);

    auto* authorImage = new brls::Image();
    authorImage->setImageFromFile(BK_RES("img/beiklive.png"));
    authorImage->setWidth(80.f);
    authorImage->setHeight(80.f);
    authorImage->setCornerRadius(40.f);
    authorImage->setScalingType(brls::ImageScalingType::FIT);
    authorImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    authorImage->setFocusable(false);
    authorImage->setMarginRight(30.f);

    auto* infoBox = new brls::Box(brls::Axis::COLUMN);
    infoBox->setAlignItems(brls::AlignItems::FLEX_START);
    infoBox->setJustifyContent(brls::JustifyContent::CENTER);
    infoBox->setFocusable(false);

    auto* nameLabel = new brls::Label();
    nameLabel->setText("beiklive");
    nameLabel->setFontSize(28.f);
    nameLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    nameLabel->setMarginBottom(16.f);
    nameLabel->setFocusable(false);

    auto* githubLabel = new brls::Label();
    githubLabel->setText("GitHub:  https://github.com/beiklive/GBAStation");
    githubLabel->setFontSize(18.f);
    githubLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    githubLabel->setFocusable(false);

    auto* githubBadge = new brls::Box(brls::Axis::ROW);
    githubBadge->setCornerRadius(8.f);
    githubBadge->setBackgroundColor(nvgRGBA(79, 193, 255, 30));
    githubBadge->setPadding(6.f, 12.f, 6.f, 12.f);
    githubBadge->setMarginBottom(10.f);
    githubBadge->setFocusable(false);
    githubBadge->setHideHighlightBackground(true);
    githubBadge->addView(githubLabel);

    auto* biliLabel = new brls::Label();
    biliLabel->setText("BiliBili:   BEIKLIVE");
    biliLabel->setFontSize(18.f);
    biliLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    biliLabel->setFocusable(false);

    auto* biliBadge = new brls::Box(brls::Axis::ROW);
    biliBadge->setCornerRadius(8.f);
    biliBadge->setBackgroundColor(nvgRGBA(0, 168, 107, 30));
    biliBadge->setPadding(6.f, 12.f, 6.f, 12.f);
    biliBadge->setFocusable(false);
    biliBadge->setHideHighlightBackground(true);
    biliBadge->addView(biliLabel);

    infoBox->addView(nameLabel);
    infoBox->addView(githubBadge);
    infoBox->addView(biliBadge);

    authorCard->addView(authorImage);
    authorCard->addView(infoBox);
    box->addView(authorCard);

    // 项目说明
    auto* sectionHeader = new brls::Header();
    sectionHeader->setTitle("关于本项目");
    sectionHeader->setMarginTop(30.f);
    sectionHeader->setMarginBottom(15.f);
    box->addView(sectionHeader);

    auto* descCard = new brls::Box(brls::Axis::COLUMN);
    descCard->setCornerRadius(16.f);
    descCard->setBackgroundColor(nvgRGBA(0, 0, 0, 20));
    descCard->setShadowVisibility(true);
    descCard->setShadowType(brls::ShadowType::GENERIC);
    descCard->setPadding(20.f, 24.f, 20.f, 24.f);
    descCard->setFocusable(false);
    descCard->setHideHighlightBackground(true);
    descCard->setHideHighlightBorder(true);
    descCard->setHeight(brls::View::AUTO);

    std::vector<std::string> descLines = {
        "本项目基于 libretro 核心接口构建，目前内置 mGBA 模拟器核心。",
        "",
        "目前已实现功能：",
        "  •  游戏库功能（运行过的游戏会被自动添加到游戏库中）",
        "  •  定时存档功能",
        "  •  键位自定义",
        "  •  金手指功能",
        "  •  封面设置",
        "  •  游戏时间统计",
        "  •  RA 着色器及参数修改支持（还不完善）",
        "  •  遮罩功能",
        "  •  RA 游戏库导入",
        "  •  快进倒带"
    };

    for (const auto& line : descLines) {
        auto* label = new brls::Label();
        label->setText(line);
        label->setFontSize(20.f);
        label->setHeight(line.empty() ? 8.f : 26.f);
        label->setWidth(brls::View::AUTO);
        label->setTextColor(GET_THEME_COLOR("brls/text"));
        label->setFocusable(false);
        descCard->addView(label);
    }

    box->addView(descCard);
    box->addView(new brls::Padding());
    scroll->setContentView(box);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

// ── 更新 ──────────────────────────────────────────────────

brls::View* AboutPage::_buildUpdateTab() {
    auto* scroll = new brls::ScrollingFrame();
    scroll->setGrow(1.0f);
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    scroll->setScrollingIndicatorVisible(false);
    scroll->setFocusable(false);

    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setAlignItems(brls::AlignItems::CENTER);
    box->setWidthPercentage(100.f);
    box->setPadding(20.f, 40.f, 30.f, 40.f);

    // 读取本地 version.json
    std::string localVersion = "未知";
    std::string localChangelog = "";
    size_t localSize = 0;
        std::string localPath = beiklive::path::configPath() + "/version.json";
        std::ifstream f(localPath);
        if (f) {
            std::string content((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
            auto j = nlohmann::json::parse(content);
            localVersion = j.value("version", "未知");
            localChangelog = j.value("changelog", "");
            localSize = j.value("size", size_t(0));
        }

    // 版本信息卡片
    auto* versionCard = new brls::Box(brls::Axis::COLUMN);
    versionCard->setCornerRadius(14.f);
    versionCard->setBackgroundColor(nvgRGBA(0, 0, 0, 20));
    versionCard->setShadowVisibility(true);
    versionCard->setShadowType(brls::ShadowType::GENERIC);
    versionCard->setPadding(20.f, 24.f, 20.f, 24.f);
    versionCard->setMarginBottom(10.f);
    versionCard->setWidthPercentage(100.f);
    versionCard->setFocusable(false);
    versionCard->setHideHighlightBackground(true);

    auto* verTitle = new brls::Label();
    verTitle->setText("当前版本信息");
    verTitle->setFontSize(22.f);
    verTitle->setTextColor(GET_THEME_COLOR("brls/text"));
    verTitle->setMarginBottom(14.f);
    verTitle->setFocusable(false);
    versionCard->addView(verTitle);

    auto addInfoRow = [&](const std::string& label, const std::string& value) {
        auto* row = new brls::Box(brls::Axis::ROW);
        row->setFocusable(false);
        row->setMarginBottom(6.f);

        auto* lbl = new brls::Label();
        lbl->setText(label);
        lbl->setFontSize(17.f);
        lbl->setTextColor(nvgRGBA(200, 200, 200, 200));
        lbl->setWidth(80.f);
        lbl->setFocusable(false);
        row->addView(lbl);

        auto* val = new brls::Label();
        val->setText(value);
        val->setFontSize(17.f);
        val->setTextColor(GET_THEME_COLOR("brls/text"));
        val->setFocusable(false);
        row->addView(val);

        versionCard->addView(row);
    };

    addInfoRow("版本号", localVersion);
    addInfoRow("文件大小", formatSize(localSize));

    box->addView(versionCard);

    // 更新金手指数据库按钮
    auto* cheatBtn = new brls::Button();
    cheatBtn->setText("更新金手指数据库");
    cheatBtn->setWidthPercentage(100.f);
    cheatBtn->setMarginBottom(12.f);
    cheatBtn->registerClickAction([this](brls::View*) -> bool {
        _updateCheatDatabase();
        return true;
    });
    box->addView(cheatBtn);

    // 检测模拟器更新按钮
    auto* checkBtn = new brls::Button();
    checkBtn->setText("检测模拟器更新");
    checkBtn->setWidthPercentage(100.f);
    checkBtn->registerClickAction([this](brls::View*) -> bool {
        _checkUpdate();
        return true;
    });

    box->addView(checkBtn);

    auto* hint = new brls::Label();
    hint->setText("连接到服务器检测最新版本，如有更新可自动下载安装");
    hint->setFontSize(14.f);
    hint->setTextColor(nvgRGBA(200, 200, 200, 200));
    hint->setMarginTop(15.f);
    hint->setMarginLeft(20.f);
    hint->setFocusable(false);
    box->addView(hint);
    // 更新日志
    if (!localChangelog.empty()) {
        auto* changelogHeader = new brls::Header();
        changelogHeader->setTitle("更新日志");
        changelogHeader->setMarginTop(24.f);
        changelogHeader->setMarginBottom(10.f);
        box->addView(changelogHeader);

        auto* changelogCard = new brls::ScrollingFrame();
        changelogCard->setWidthPercentage(100.f);
        changelogCard->setHeight(280.f);


        auto* lablebox = new brls::Box(brls::Axis::COLUMN);
        lablebox->setWidthPercentage(100.f);
        lablebox->setHeightPercentage(100.f);
        lablebox->setFocusable(true);

        auto* m_bodyLabel = new brls::Label();
        m_bodyLabel->setFontSize(15);
        m_bodyLabel->setWidthPercentage(100.f);
        m_bodyLabel->setHeightPercentage(100.f);
        m_bodyLabel->setTextColor(nvgRGBA(200, 200, 210, 255));
        m_bodyLabel->setFocusable(true);
        UP_DOWN_NAVIGATION(m_bodyLabel, m_bodyLabel);
        m_bodyLabel->registerAction("返回", brls::BUTTON_B, [this, checkBtn](brls::View*) {
            brls::Application::giveFocus(checkBtn);
            return true;
        });
        lablebox->addView(m_bodyLabel);

        changelogCard->addView(lablebox);

        m_bodyLabel->setText(R"(
v0.1.12
    bug修复和优化
    1. 修复倍速状态下游戏声音出现杂音的问题
    2. 调整着色器设置界面透明度，减少视觉色差
    
    新功能
    1. 新增游戏菜单快捷存档删除功能
    2. 新增游戏菜单GB颜色调整功能，仅GB游戏可用

        
v0.1.11
    bug修复和优化
    1. 修复着色器参数设置好后下次打开游戏又复原的bug
    2. 添加历史帧功能，带拖影效果的着色器可以正常显示拖影效果了

v0.1.10
    bug修复和优化
    1. 修复启动模拟器后立即退出时，因为版本后台检测线程未退出导致的崩溃问题
    2. 重写文件列表，优化浏览效果，去掉了列表声音
    
    新功能
    1. 文件列表增加搜索功能, 按 ZR 键对所在目录搜索,按B键关闭搜索结果

v0.1.9
    bug修复和优化
    1. 修复金手指输入和修改时不能输入多行的bug

v0.1.8
    bug修复和优化
    1. 修复着色器参数修改后重启游戏参数修改无效的bug
    2. 优化crc32计算逻辑，加快了RetroArch导入速度

v0.1.7
    1. 进一步优化游戏循环，减少卡顿掉帧
    2. 关于界面添加QQ群号

v0.1.6
    bug修复和优化
    1. 倒带时静音和快进时静音功能之前无效，现在可正常使用
    2. 开启倍速后会出现掉帧情况，与音频缓冲有关，现已优化并提升了倍速性能，如有问题请在评论区反馈

v0.1.5
    bug修复
    1. 修复设置同步功能未同步到其他游戏的bug
    2. 修复进入游戏库有概率按键被禁用的bug

v0.1.4
    bug修复和优化
    1. 优化游戏库界面加载速度降低内存消耗s

    新功能：
    1. 游戏菜单金手指条目支持删除

v0.1.3
    bug修复和优化
    修复 sRGB FBO 缺少 GL_FRAMEBUFFER_SRGB 导致画面偏暗
    修复 scalefx/hqx 等高清化着色器的画面放大和偏移
    起始页菜单加载速度优化

v0.1.2
    新功能
    1. 添加GBA BIOS画面，需要在网盘中下载GBA BIOS文件，并将gba_bios.bin放到 /GBAStation/bios 目录下
    2. 设置--模拟器--GB配色 功能可用，可以通过该项设置调节gb游戏的调色板

    bug修复
    1. 修复 scalefx/hqx 等高清化着色器的画面放大和偏移

v0.1.1
    bug修复
    1. 修复RetroArch着色器的支持，反射滤镜可以正常使用

v0.1.0
    新功能
    1. 模拟器新增更新检测功能，默认启动后自动检测(可关闭), 也可以在 关于-更新 中检测更新
    2. 添加 A B键连发功能，在 设置-按键 中设置
    3. 游戏菜单-显示设置 添加同平台游戏设置同步功能，一键将着色器、遮罩、画面等设置同步到游戏库中同平台的其他游戏中
    4. 快进功能倍速增加 1.25倍 1.5倍 1.75倍
    5. 模拟器设置中 自动保存状态、自动加载、退出自动保存 添加了存档位的选择，不再固定位档位0

    bug修复
    1. 修复从RetroArch导入的不同平台游戏文件名称相同时会使用同一个存档目录的bug
    2. 修复游戏菜单中选择金手指文件后，路径没有保存的bug
    3. 修复倒带功能触发方式设置为保持时，触发效果仍然为切换效果的bug
        
    )");

        box->addView(changelogCard);
    }



    box->addView(new brls::Padding());
    scroll->setContentView(box);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

void AboutPage::_checkUpdate() {
    // 显示检测中弹窗
    auto* dlg = new brls::Dialog("正在检测更新...\n\n请稍候");
    dlg->setFocusable(true);
    HIDE_BRLS_HIGHLIGHT(dlg);
    dlg->open();

    new std::thread([this, dlg]() {
        auto& updater = AppUpdater::instance();

		// 从 config/version.json 读取本地版本号，不存在则用 APP_VERSION
		std::string localVersion = APP_VERSION;
		{
			std::ifstream f(beiklive::path::configPath() + "/version.json");
			if (f.is_open()) {
				nlohmann::json j;
				f >> j;
				std::string ver = j.value("version", "");
				brls::Logger::info("本地版本号: {}", ver);
				if (!ver.empty())
					localVersion = ver;
			}
		}

        updater.checkSync(localVersion);

        brls::sync([this, dlg]() {
            // 关闭检测中弹窗
            dlg->close([]{});

            auto& info = AppUpdater::instance().info();
            if (info.hasUpdate) {
                auto* confirmDlg = new beiklive::UpdateDialog(
                    "版本更新  " + info.version,
                    info.changelog
                );
                confirmDlg->addButton("更新", []() {
                    auto* page = new UpdatePage();
                    auto* frame = new brls::AppletFrame(page);
                    HIDE_BRLS_BAR(frame);
                    brls::Application::pushActivity(
                        new brls::Activity(frame), brls::TransitionAnimation::NONE);
                    page->startDownload();
                });
                confirmDlg->addButton("取消", []() {});
                confirmDlg->open();
            } else {
                auto* okDlg = new brls::Dialog("已是最新版本");
                okDlg->addButton("确定", []() {});
                okDlg->open();
            }
        });
    });
}

void AboutPage::_updateCheatDatabase() {
    auto* dlg = new brls::Dialog("正在更新金手指数据库...\n\n请稍候");
    auto* cancelFlag = new std::atomic<bool>(false);
    dlg->addButton("取消", [cancelFlag]() { cancelFlag->store(true); });
    dlg->setCancelable(false);
    dlg->open();

    ASYNC_RETAIN
    new std::thread([ASYNC_TOKEN, dlg, cancelFlag]() {
        static const char* kUrl = "https://cdn.jsdelivr.net/gh/beiklive/GBAStation_Release@main/cheat_db/cheat_db.zip";

        std::string dbDir = beiklive::path::dbsPath();
        std::error_code ec;
        std::filesystem::create_directories(dbDir, ec);

        std::string zipPath = dbDir + beiklive::path::SPLIT_CHAR + "cheat_db.zip";

        // ── 下载 ──
        bool downloadOk = false;
        {
            CURL* curl = curl_easy_init();
            if (curl && !cancelFlag->load()) {
                std::vector<uint8_t> body;
                curl_easy_setopt(curl, CURLOPT_URL, kUrl);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                    static_cast<size_t(*)(void*, size_t, size_t, void*)>(
                        [](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                            auto* v = static_cast<std::vector<uint8_t>*>(userdata);
                            v->insert(v->end(), (uint8_t*)ptr, (uint8_t*)ptr + size * nmemb);
                            return size * nmemb;
                        }));
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

                if (!cancelFlag->load()) {
                    CURLcode res = curl_easy_perform(curl);
                    long code = 0;
                    if (res == CURLE_OK)
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
                    if (res == CURLE_OK && code == 200 && !body.empty()) {
                        std::ofstream out(zipPath, std::ios::binary | std::ios::trunc);
                        if (out) {
                            out.write(reinterpret_cast<const char*>(body.data()), body.size());
                            out.close();
                            downloadOk = true;
                        }
                    }
                }
                curl_easy_cleanup(curl);
            }
        }

        // ── 解压 ──
        int extractCount = 0;
        if (downloadOk && !cancelFlag->load()) {
            mz_zip_archive zip;
            memset(&zip, 0, sizeof(zip));
            if (mz_zip_reader_init_file(&zip, zipPath.c_str(), 0)) {
                mz_uint numFiles = mz_zip_reader_get_num_files(&zip);
                for (mz_uint i = 0; i < numFiles && !cancelFlag->load(); ++i) {
                    char filename[256];
                    mz_zip_reader_get_filename(&zip, i, filename, sizeof(filename));
                    std::string outPath = dbDir + beiklive::path::SPLIT_CHAR + filename;
                    if (mz_zip_reader_extract_to_file(&zip, i, outPath.c_str(), 0))
                        ++extractCount;
                }
                mz_zip_reader_end(&zip);
            }
            std::filesystem::remove(zipPath, ec);
        }

        brls::sync([ASYNC_TOKEN, dlg, cancelFlag, downloadOk, extractCount]() {
            ASYNC_RELEASE
            bool cancelled = cancelFlag->load();
            dlg->close([]{});
            delete cancelFlag;
            if (cancelled && !downloadOk) {
                auto* okDlg = new brls::Dialog("已取消更新");
                okDlg->addButton("确定", []() {});
                okDlg->open();
            } else if (!downloadOk) {
                auto* okDlg = new brls::Dialog("下载失败，请稍后重试或者去网盘手动下载");
                okDlg->addButton("确定", []() {});
                okDlg->open();
            } else {
                auto* okDlg = new brls::Dialog("更新完成（已解压 " +
                    std::to_string(extractCount) + " 个文件）");
                okDlg->addButton("确定", []() {});
                okDlg->open();
            }
        });
    });
}

// ── 支持作者 ─────────────────────────────────────────────

brls::View* AboutPage::_buildSupportTab() {
    auto* scroll = new brls::ScrollingFrame();
    scroll->setGrow(1.0f);
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    scroll->setScrollingIndicatorVisible(false);
    scroll->setFocusable(false);

    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setPadding(20.f, 40.f, 30.f, 40.f);
    box->setAlignItems(brls::AlignItems::CENTER);
    box->setJustifyContent(brls::JustifyContent::CENTER);
    box->setFocusable(false);

    auto* label1 = new brls::Label();
    label1->setText("喜欢这个项目的话，不妨请作者喝杯咖啡吧");
    label1->setFontSize(20.f);
    label1->setTextColor(GET_THEME_COLOR("brls/text"));
    label1->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    label1->setMarginBottom(16.f);
    label1->setFocusable(false);
    box->addView(label1);

    auto* label2 = new brls::Label();
    label2->setText("也许下一次更新的灵感，就来自这杯咖啡里的能量");
    label2->setFontSize(14.f);
    label2->setTextColor(nvgRGBA(200, 200, 200, 200));
    label2->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    label2->setMarginBottom(32.f);
    label2->setFocusable(false);
    box->addView(label2);

    auto* QQImage = new brls::Image();
    QQImage->setImageFromFile(BK_RES("img/QQ.png"));
    QQImage->setScalingType(brls::ImageScalingType::FIT);
    QQImage->setInterpolation(brls::ImageInterpolation::NEAREST);
    QQImage->setCornerRadius(16.f);
    QQImage->setWidth(400.f);
    QQImage->setHeight(150.f);
    QQImage->setFocusable(false);
    box->addView(QQImage);


    auto* payImage = new brls::Image();
    payImage->setImageFromFile(BK_RES("img/pay.png"));
    payImage->setScalingType(brls::ImageScalingType::FIT);
    payImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    payImage->setCornerRadius(16.f);
    payImage->setWidth(800.f);
    payImage->setHeight(400.f);
    payImage->setFocusable(false);
    box->addView(payImage);

    box->addView(new brls::Padding());
    scroll->setContentView(box);

    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}


} // namespace beiklive
