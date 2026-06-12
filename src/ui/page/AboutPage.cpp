#include "ui/page/AboutPage.hpp"
#include "ui/page/UpdatePage.hpp"
#include "ui/widget/UpdateDialog.hpp"
#include "ui/widget/DetailCell.hpp"
#include "ui/utils/CheatMatcher.hpp"
#include "core/AppUpdater.hpp"
#include "core/Tools.hpp"
#include <borealis/views/applet_frame.hpp>
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

static std::string readTextFile(const std::string& path, const std::string& fallback = "") {
    std::ifstream file(path);
    if (!file)
        return fallback;

    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

static void openChangelogApplet(const std::string& title, const std::string& content) {
    auto* scroll = new brls::ScrollingFrame();
    scroll->setGrow(1.0f);
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    scroll->setScrollingIndicatorVisible(false);

    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setWidthPercentage(100.f);
    box->setHeightPercentage(100.f);
    box->setPadding(26.f, 34.f, 26.f, 34.f);
    box->setAlignItems(brls::AlignItems::FLEX_START);
    box->setJustifyContent(brls::JustifyContent::FLEX_START);

    auto* bodyLabel = new brls::Label();
    bodyLabel->setText(content.empty() ? "暂无更新日志" : content);
    bodyLabel->setFontSize(18.f);
    bodyLabel->setWidthPercentage(100.f);
    bodyLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    bodyLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    bodyLabel->setSingleLine(false);
    bodyLabel->setIsWrapping(true);
    bodyLabel->setFocusable(true);
    bodyLabel->registerAction("确认", brls::BUTTON_A, [](brls::View*) -> bool {
        brls::Application::popActivity(brls::TransitionAnimation::NONE);
        return true;
    });
    bodyLabel->registerAction("返回", brls::BUTTON_B, [](brls::View*) -> bool {
        brls::Application::popActivity(brls::TransitionAnimation::NONE);
        return true;
    });
    box->addView(bodyLabel);

    scroll->setContentView(box);

    auto* frame = new brls::AppletFrame(scroll);
    frame->setTitle(title);

    brls::Application::pushActivity(new brls::Activity(frame), brls::TransitionAnimation::NONE);
    brls::Application::giveFocus(bodyLabel);
}

AboutPage::AboutPage() {
    brls::sync([this]() {
        this->showFooter(true);
        this->showHeader(false);
        this->registerAction("返回", brls::BUTTON_B, [this](brls::View*) { 
            beiklive::popActivity(this);
            return true;
        });
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

    std::string changelogText = localChangelog;
    if (changelogText.empty())
        changelogText = readTextFile(BK_RES("changelog"), "暂无更新日志");

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

    auto* changelogBtn = new beiklive::DetailCell();
    changelogBtn->setLeftText("查看当前版本更新内容");
    changelogBtn->setRightText("\uE14A");
    changelogBtn->registerAction("打开", brls::BUTTON_A,
        [localVersion, changelogText](brls::View*) -> bool {
            openChangelogApplet(
                "当前版本更新内容  " + localVersion,
                changelogText.empty() ? "暂无更新日志" : changelogText);
            return true;
        });
    box->addView(changelogBtn);

    // // 更新金手指数据库按钮
    // auto* cheatBtn = new brls::Button();
    // cheatBtn->setText("更新金手指数据库");
    // cheatBtn->setWidthPercentage(100.f);
    // cheatBtn->setMarginBottom(12.f);
    // cheatBtn->registerClickAction([this](brls::View*) -> bool {
    //     _updateCheatDatabase();
    //     return true;
    // });
    // box->addView(cheatBtn);

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
                    brls::sync([]() {
                        auto* dialog = new UpdatePage();
                        dialog->open();
                        brls::sync([dialog]() {
                            dialog->startDownload();
                        });
                    });
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
    auto* cancelFlag = new std::atomic<bool>(false);

    auto* prog = new beiklive::ProgressDialog("正在更新金手指数据库...",
        [cancelFlag]() { cancelFlag->store(true); });
    auto* frame = new brls::AppletFrame(prog);
    HIDE_BRLS_BAR(frame);
    brls::Application::pushActivity(new brls::Activity(frame),
                                    brls::TransitionAnimation::NONE);

    new std::thread([prog, cancelFlag]() {
        static const char* kUrl = "https://cdn.jsdelivr.net/gh/beiklive/GBAStation_Release@main/cheat_db/cheat_db.zip";

        std::string dbDir = beiklive::path::dbsPath();
        std::error_code ec;
        std::filesystem::create_directories(dbDir, ec);

        std::string zipPath = dbDir + beiklive::path::SPLIT_CHAR + "cheat_db.zip";

        // ── 下载 ──
        brls::sync([prog]() { prog->setStatus("正在下载..."); });

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

        if (cancelFlag->load()) {
            brls::sync([prog, cancelFlag]() { delete cancelFlag; prog->close(); });
            return;
        }

        if (!downloadOk) {
            brls::sync([prog, cancelFlag]() {
                delete cancelFlag;
                prog->showResult("下载失败，请稍后重试或者去网盘手动下载");
            });
            return;
        }

        // ── 解压 ──
        int extractCount = 0;
        std::string nestedZipPath;
        {
            brls::sync([prog]() { prog->setStatus("正在解压..."); });

            mz_zip_archive zip;
            memset(&zip, 0, sizeof(zip));
            if (mz_zip_reader_init_file(&zip, zipPath.c_str(), 0)) {
                mz_uint numFiles = mz_zip_reader_get_num_files(&zip);
                for (mz_uint i = 0; i < numFiles && !cancelFlag->load(); ++i) {
                    char filename[256];
                    mz_zip_reader_get_filename(&zip, i, filename, sizeof(filename));
                    std::string name = filename;

                    if (name == "RetroArch.zip") {
                        nestedZipPath = dbDir + beiklive::path::SPLIT_CHAR + "RetroArch.zip";
                        if (mz_zip_reader_extract_to_file(&zip, i, nestedZipPath.c_str(), 0))
                            ++extractCount;
                    } else {
                        std::string outPath = dbDir + beiklive::path::SPLIT_CHAR + name;
                        if (mz_zip_reader_extract_to_file(&zip, i, outPath.c_str(), 0))
                            ++extractCount;
                    }
                }
                mz_zip_reader_end(&zip);
            }
        }

        // 解压嵌套的 RetroArch.zip
        if (!nestedZipPath.empty() && !cancelFlag->load()) {
            brls::sync([prog]() { prog->setStatus("正在解压 RetroArch.zip..."); });

            std::string retroArchDir = beiklive::path::cheatPath() + "/RetroArch";
            std::filesystem::create_directories(retroArchDir, ec);

            mz_zip_archive nestedZip;
            memset(&nestedZip, 0, sizeof(nestedZip));
            if (mz_zip_reader_init_file(&nestedZip, nestedZipPath.c_str(), 0)) {
                mz_uint nestedCount = mz_zip_reader_get_num_files(&nestedZip);
                for (mz_uint i = 0; i < nestedCount && !cancelFlag->load(); ++i) {
                    char fn[512];
                    mz_zip_reader_get_filename(&nestedZip, i, fn, sizeof(fn));
                    std::string outPath = retroArchDir + "/" + fn;
                    mz_zip_reader_extract_to_file(&nestedZip, i, outPath.c_str(), 0);
                }
                mz_zip_reader_end(&nestedZip);
            }
            std::filesystem::remove(nestedZipPath, ec);
        }

        std::filesystem::remove(zipPath, ec);

        if (cancelFlag->load()) {
            brls::sync([prog, cancelFlag]() { delete cancelFlag; prog->close(); });
            return;
        }

        brls::sync([prog, cancelFlag, extractCount]() {
            delete cancelFlag;
            prog->setText("更新完成");
            std::string msg = "数据库已更新（解压 " + std::to_string(extractCount) + " 个文件）";
            if (extractCount > 0)
                msg = "数据库已更新（解压 " + std::to_string(extractCount) + " 个文件），\n金手指文件已就绪";
            prog->showResult(msg);
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
