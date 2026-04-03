#include "UI/Pages/DataPage.hpp"

#include <borealis/views/cells/cell_detail.hpp>
#include <borealis/views/header.hpp>
#include <borealis/views/scrolling_frame.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/tab_frame.hpp>
#include <borealis/views/dialog.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "UI/Pages/ImageView.hpp"
#include "UI/Utils/Utils.hpp"
#include "UI/Utils/ProImage.hpp"

namespace fs = std::filesystem;
using namespace brls::literals;

// ──────────────────────────────────────────────────────────────────────────────
//  辅助函数
// ──────────────────────────────────────────────────────────────────────────────

/// 从 gamedataManager 收集所有 gamepath 条目，返回 (fileName, gamePath) 对列表
static std::vector<std::pair<std::string, std::string>> collectGameEntries()
{
    std::vector<std::pair<std::string, std::string>> result;
    if (!gamedataManager) return result;

    static const std::string SUFFIX = std::string(".") + GAMEDATA_FIELD_GAMEPATH;
    for (const auto& key : gamedataManager->GetAllKeys()) {
        if (key.size() <= SUFFIX.size()) continue;
        if (key.compare(key.size() - SUFFIX.size(), SUFFIX.size(), SUFFIX) != 0) continue;
        auto v = gamedataManager->Get(key);
        if (!v) continue;
        auto s = v->AsString();
        if (!s || s->empty()) continue;
        // key 格式: <前缀>.gamepath，前缀即不含后缀的文件名
        // 但 gamedataManager 使用的 key 前缀由 gamedataKeyPrefix 决定，
        // 其从 fileName 去掉后缀得到，因此无法直接还原完整 fileName（含后缀）。
        // 使用 gamepath 中的文件名代替：
        std::string fileName = fs::path(*s).filename().string();
        result.push_back({ fileName, *s });
    }
    return result;
}

/// 获取游戏的显示名称：优先从 NameMappingManager 查询，否则返回文件名（不含后缀）
static std::string getDisplayName(const std::string& fileName)
{
    if (NameMappingManager) {
        std::string mapKey = gamedataKeyPrefix(fileName);
        auto mv = NameMappingManager->Get(mapKey);
        if (mv && mv->AsString() && !mv->AsString()->empty())
            return *mv->AsString();
    }
    return fs::path(fileName).stem().string();
}

/// 获取存档缩略图目录路径（saves/游戏文件名（不含后缀）/）
static std::string getSavesDir(const std::string& gameStem)
{
    std::string stateCustomDir;
    if (SettingManager) {
        auto v = SettingManager->Get("save.stateDir");
        if (v && v->AsString()) stateCustomDir = *v->AsString();
    }
    if (!stateCustomDir.empty())
        return (fs::path(stateCustomDir) / gameStem).string();
    // 默认：BK_APP_ROOT_DIR/saves，但 GameView 实际可能把文件存在 ROM 目录，
    // DataPage 仅展示使用模拟器统一 saves 目录时的存档。
    return std::string(BK_APP_ROOT_DIR) + "saves/" + gameStem;
}

/// 获取相册目录路径（albums/游戏文件名（不含后缀）/）
static std::string getAlbumsDir(const std::string& gameStem)
{
    return std::string(BK_APP_ROOT_DIR) + "albums/" + gameStem;
}

/// 收集指定目录中所有图片文件路径（按文件名排序）
static std::vector<std::string> collectImages(const std::string& dir)
{
    std::vector<std::string> result;
    static const std::vector<std::string> IMAGE_EXTS = {"png", "jpg", "jpeg", "bmp", "webp", "tga"};
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return result;
    fs::directory_iterator it(dir, ec);
    if (ec) return result;
    for (const auto& entry : it) {
        std::error_code ec2;
        if (!entry.is_regular_file(ec2)) continue;
        std::string ext = entry.path().extension().string();
        if (!ext.empty()) ext = ext.substr(1); // 去掉前导 '.'
        for (const auto& e : IMAGE_EXTS) {
            if (beiklive::string::iequals(ext, e)) {
                result.push_back(entry.path().string());
                break;
            }
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

// ──────────────────────────────────────────────────────────────────────────────
//  TopTabFrame
// ──────────────────────────────────────────────────────────────────────────────

TopTabFrame::TopTabFrame()
{
    setAxis(brls::Axis::COLUMN);
    setGrow(1.0f);

    // 顶部选项卡按钮行
    m_tabBar = new brls::Box(brls::Axis::ROW);
    m_tabBar->setPaddingLeft(10.f);
    m_tabBar->setPaddingRight(10.f);
    m_tabBar->setHeight(50.f);
    m_tabBar->setAlignItems(brls::AlignItems::CENTER);
    addView(m_tabBar);

    // 内容区域
    m_contentArea = new brls::Box(brls::Axis::COLUMN);
    m_contentArea->setGrow(1.0f);
    addView(m_contentArea);
}

void TopTabFrame::addTab(const std::string& label, std::function<brls::View*()> creator)
{
    int idx = static_cast<int>(m_tabs.size());

    // 创建选项卡按钮
    auto* btn = new brls::Button();
    btn->setText(label);
    btn->setMarginRight(8.f);
    btn->setFocusable(true);
    btn->setGrow(1.0f);
    // 点击或 A 键切换到此选项卡
    btn->registerAction("", brls::BUTTON_A, [this, idx](brls::View*) -> bool {
        activateTab(idx);
        return true;
    });
    btn->getFocusEvent()->subscribe([this, idx](brls::View*) {
        activateTab(idx);
    });

    m_tabBar->addView(btn);

    TabInfo info;
    info.label   = label;
    info.creator = std::move(creator);
    info.button  = btn;
    m_tabs.push_back(std::move(info));

    // 第一个选项卡自动激活
    if (static_cast<int>(m_tabs.size()) == 1)
        activateTab(0);
}

void TopTabFrame::focusTab(int index)
{
    activateTab(index);
}

void TopTabFrame::activateTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;
    if (index == m_activeIndex) return;

    m_activeIndex = index;

    // 清空内容区域
    m_contentArea->clearViews();

    // 创建新内容
    brls::View* content = m_tabs[index].creator();
    if (content) {
        content->setGrow(1.0f);
        m_contentArea->addView(content);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  DataPage
// ──────────────────────────────────────────────────────────────────────────────

DataPage::DataPage()
{
    setAxis(brls::Axis::COLUMN);
    setGrow(1.0f);

    // ── 标题栏 ──────────────────────────────────────────────────────────────
    auto* header = new beiklive::UI::BrowserHeader();
    header->setTitle("beiklive/datapage/title"_i18n);
    addView(header);

    // ── TabFrame（侧边栏 = 游戏列表，内容 = TopTabFrame）────────────────────
    m_tabFrame = new brls::TabFrame();
    m_tabFrame->setGrow(1.0f);
    addView(m_tabFrame);

    // 遍历 gamedataManager 中所有 gamepath 条目
    auto entries = collectGameEntries();
    if (entries.empty()) {
        // 没有游戏数据时显示提示
        m_tabFrame->addTab("beiklive/datapage/no_games"_i18n, []() -> brls::View* {
            auto* lbl = new brls::Label();
            lbl->setText("beiklive/datapage/no_games"_i18n);
            lbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);
            return lbl;
        });
        return;
    }

    for (const auto& [fileName, gamePath] : entries) {
        std::string displayName = getDisplayName(fileName);
        // 捕获变量用于 lambda
        std::string captFileName = fileName;
        std::string captGamePath = gamePath;
        m_tabFrame->addTab(displayName, [this, captFileName, captGamePath]() -> brls::View* {
            return buildGamePanel(captFileName, captGamePath);
        });
    }

    addView(new brls::BottomBar());
}

TopTabFrame* DataPage::buildGamePanel(const std::string& fileName, const std::string& /*gamePath*/)
{
    auto* topTab = new TopTabFrame();

    std::string stem = fs::path(fileName).stem().string();
    std::string captFileName = fileName;
    std::string captStem = stem;

    // 存档选项卡
    topTab->addTab("beiklive/datapage/tab_saves"_i18n, [this, captStem]() -> brls::View* {
        return buildSavesPanel(captStem);
    });

    // 相册选项卡
    topTab->addTab("beiklive/datapage/tab_album"_i18n, [this, captStem]() -> brls::View* {
        return buildAlbumPanel(captStem);
    });

    // 金手指选项卡
    topTab->addTab("beiklive/datapage/tab_cheats"_i18n, [this, captFileName]() -> brls::View* {
        return buildCheatPanel(captFileName);
    });

    return topTab;
}

// ──────────────────────────────────────────────────────────────────────────────
//  存档子面板
// ──────────────────────────────────────────────────────────────────────────────

brls::View* DataPage::buildSavesPanel(const std::string& gameStem)
{
    std::string dir = getSavesDir(gameStem);
    std::vector<std::string> images = collectImages(dir);

    auto* scroll = new brls::ScrollingFrame();
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);

    auto* contentBox = new brls::Box(brls::Axis::COLUMN);
    contentBox->setPadding(10.f, 20.f, 20.f, 20.f);

    if (images.empty()) {
        auto* lbl = new brls::Label();
        lbl->setText("beiklive/datapage/no_saves"_i18n);
        lbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        lbl->setMarginTop(30.f);
        contentBox->addView(lbl);
    } else {
        static constexpr int COLS      = 3;
        static constexpr float IMG_SZ  = 300.f;
        static constexpr float IMG_GAP = 10.f;

        brls::Box* rowBox = nullptr;
        for (int i = 0; i < static_cast<int>(images.size()); ++i) {
            if (i % COLS == 0) {
                rowBox = new brls::Box(brls::Axis::ROW);
                rowBox->setAlignItems(brls::AlignItems::CENTER);
                rowBox->setMarginBottom(IMG_GAP);
                contentBox->addView(rowBox);
            }

            const std::string imgPath = images[i];

            auto* img = new beiklive::UI::ProImage();
            img->setWidth(IMG_SZ*0.8f);
            img->setHeight(IMG_SZ*0.6f);
            img->setCornerRadius(8.f);
            img->setMarginRight(IMG_GAP);
            img->setFocusable(true);
            img->setHideHighlightBackground(true);
            img->setScalingType(brls::ImageScalingType::FIT);
            img->setImageFromFileAsync(imgPath);

            // A 键：打开图片查看器
            img->registerAction("beiklive/hints/start"_i18n, brls::BUTTON_A, [imgPath](brls::View*) -> bool {
                auto* viewer = new ImageView(imgPath);
                auto* frame  = new brls::AppletFrame(viewer);
                frame->setBackground(brls::ViewBackground::NONE);
                brls::Application::pushActivity(new brls::Activity(frame));
                return true;
            });

            // X 键：删除确认
            img->registerAction("beiklive/datapage/delete_hint"_i18n, brls::BUTTON_X, [img, imgPath](brls::View*) -> bool {
                auto* dialog = new brls::Dialog("beiklive/datapage/delete_confirm"_i18n);
                dialog->addButton("beiklive/hints/cancel"_i18n, []() {});
                dialog->addButton("beiklive/hints/confirm"_i18n, [img, imgPath]() {
                    // 删除图片和同名状态文件（去掉图片扩展名）
                    std::error_code ec;
                    fs::remove(imgPath, ec);
                    // 尝试删除对应的状态文件（如 game.ss0.png -> game.ss0）
                    std::string ext = fs::path(imgPath).extension().string();
                    if (!ext.empty()) {
                        std::string statePath = imgPath.substr(0, imgPath.size() - ext.size());
                        fs::remove(statePath, ec);
                    }
                    // 从 UI 中隐藏已删除的图片
                    img->setVisibility(brls::Visibility::GONE);
                });
                dialog->open();
                return true;
            });

            rowBox->addView(img);
        }

        // 末行不足 COLS 个时，用 Padding 占位，防止最后几项被拉伸填满行宽。
        if (rowBox) {
            int total = static_cast<int>(images.size());
            int remainder = total % COLS;
            int padCount  = (remainder == 0) ? 0 : (COLS - remainder);
            for (int p = 0; p < padCount; ++p) {
                auto* pad = new brls::Padding();
                pad->setWidth(IMG_SZ * 0.8f);
                pad->setHeight(IMG_SZ * 0.6f);
                pad->setMarginRight(IMG_GAP);
                rowBox->addView(pad);
            }
        }
    }

    scroll->setContentView(contentBox);
    return scroll;
}

// ──────────────────────────────────────────────────────────────────────────────
//  相册子面板
// ──────────────────────────────────────────────────────────────────────────────

brls::View* DataPage::buildAlbumPanel(const std::string& gameStem)
{
    std::string dir = getAlbumsDir(gameStem);
    std::vector<std::string> images = collectImages(dir);

    auto* scroll = new brls::ScrollingFrame();
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);

    auto* contentBox = new brls::Box(brls::Axis::COLUMN);
    contentBox->setPadding(10.f, 20.f, 20.f, 20.f);

    if (images.empty()) {
        auto* lbl = new brls::Label();
        lbl->setText("beiklive/datapage/no_album"_i18n);
        lbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        lbl->setMarginTop(30.f);
        contentBox->addView(lbl);
    } else {
        static constexpr int COLS      = 3;
        static constexpr float IMG_SZ  = 300.f;
        static constexpr float IMG_GAP = 10.f;

        brls::Box* rowBox = nullptr;
        for (int i = 0; i < static_cast<int>(images.size()); ++i) {
            if (i % COLS == 0) {
                rowBox = new brls::Box(brls::Axis::ROW);
                rowBox->setAlignItems(brls::AlignItems::CENTER);
                rowBox->setMarginBottom(IMG_GAP);
                contentBox->addView(rowBox);
            }

            const std::string imgPath = images[i];

            auto* img = new beiklive::UI::ProImage();
            img->setWidth(IMG_SZ*0.8f);
            img->setHeight(IMG_SZ*0.6f);
            img->setCornerRadius(8.f);
            img->setMarginRight(IMG_GAP);
            img->setFocusable(true);
            img->setHideHighlightBackground(true);
            img->setScalingType(brls::ImageScalingType::FILL);
            img->setImageFromFileAsync(imgPath);

            // A 键：打开图片查看器
            img->registerAction("beiklive/hints/start"_i18n, brls::BUTTON_A, [imgPath](brls::View*) -> bool {
                auto* viewer = new ImageView(imgPath);
                auto* frame  = new brls::AppletFrame(viewer);
                frame->setBackground(brls::ViewBackground::NONE);
                brls::Application::pushActivity(new brls::Activity(frame));
                return true;
            });

            // X 键：删除确认
            img->registerAction("beiklive/datapage/delete_hint"_i18n, brls::BUTTON_X, [img, imgPath](brls::View*) -> bool {
                auto* dialog = new brls::Dialog("beiklive/datapage/delete_confirm"_i18n);
                dialog->addButton("beiklive/hints/cancel"_i18n, []() {});
                dialog->addButton("beiklive/hints/confirm"_i18n, [img, imgPath]() {
                    std::error_code ec;
                    fs::remove(imgPath, ec);
                    // 从 UI 中隐藏已删除的图片
                    img->setVisibility(brls::Visibility::GONE);
                });
                dialog->open();
                return true;
            });

            rowBox->addView(img);
        }

        // 末行不足 COLS 个时，用 Padding 占位，防止最后几项被拉伸填满行宽。
        if (rowBox) {
            int total = static_cast<int>(images.size());
            int remainder = total % COLS;
            int padCount  = (remainder == 0) ? 0 : (COLS - remainder);
            for (int p = 0; p < padCount; ++p) {
                auto* pad = new brls::Padding();
                pad->setWidth(IMG_SZ * 0.8f);
                pad->setHeight(IMG_SZ * 0.6f);
                pad->setMarginRight(IMG_GAP);
                rowBox->addView(pad);
            }
        }
    }

    scroll->setContentView(contentBox);
    return scroll;
}

// ──────────────────────────────────────────────────────────────────────────────
//  金手指子面板
// ──────────────────────────────────────────────────────────────────────────────

/// 解析 .cht 金手指文件（RetroArch 格式），返回 (desc, code, enabled) 列表
struct CheatItem {
    std::string desc;
    std::string code;
    bool enabled = true;
};

static std::vector<CheatItem> parseCheatFile(const std::string& path)
{
    std::vector<CheatItem> result;
    if (path.empty() || !fs::exists(path)) return result;

    std::ifstream f(path);
    if (!f) return result;

    std::ostringstream oss;
    oss << f.rdbuf();
    std::string content = oss.str();

    if (content.find("cheats = ") != std::string::npos ||
        content.find("cheats=")   != std::string::npos)
    {
        // RetroArch .cht 格式
        std::unordered_map<std::string, std::string> kv;
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            auto trim = [](std::string s) -> std::string {
                size_t b = s.find_first_not_of(" \t\"");
                size_t e = s.find_last_not_of(" \t\"");
                if (b == std::string::npos) return "";
                return s.substr(b, e - b + 1);
            };
            kv[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
        }
        unsigned total = 0;
        if (auto it = kv.find("cheats"); it != kv.end()) {
            try { total = static_cast<unsigned>(std::stoi(it->second)); } catch (...) {}
        }
        for (unsigned i = 0; i < total; ++i) {
            std::string is = std::to_string(i);
            auto cit = kv.find("cheat" + is + "_code");
            if (cit == kv.end()) continue;
            CheatItem item;
            item.code    = cit->second;
            item.desc    = "cheat" + is;
            item.enabled = true;
            if (auto eit = kv.find("cheat" + is + "_enable"); eit != kv.end()) {
                std::string ev = eit->second;
                for (char& c : ev) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                item.enabled = (ev == "true" || ev == "1" || ev == "yes");
            }
            if (auto dit = kv.find("cheat" + is + "_desc"); dit != kv.end())
                item.desc = dit->second;
            result.push_back(std::move(item));
        }
    }
    return result;
}

/// 保存修改后的金手指条目回 .cht 文件
static bool saveCheatFile(const std::string& path, const std::vector<CheatItem>& entries)
{
    std::ofstream f(path);
    if (!f) return false;
    f << "cheats = " << entries.size() << "\n\n";
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        f << "cheat" << i << "_desc = \"" << e.desc << "\"\n";
        f << "cheat" << i << "_enable = " << (e.enabled ? "true" : "false") << "\n";
        f << "cheat" << i << "_code = " << e.code << "\n\n";
    }
    return true;
}

/// 获取游戏金手指文件路径
static std::string resolveCheatPath(const std::string& fileName)
{
    // 优先从 gamedataManager 读取已保存的路径
    std::string path = getGameDataStr(fileName, GAMEDATA_FIELD_CHEATPATH, "");
    if (!path.empty()) return path;

    // 回退：使用 cheat.dir 配置或默认目录
    std::string cheatDir;
    if (SettingManager) {
        auto v = SettingManager->Get("cheat.dir");
        if (v && v->AsString()) cheatDir = *v->AsString();
    }
    std::string stem = fs::path(fileName).stem().string();
    if (!cheatDir.empty())
        return (fs::path(cheatDir) / (stem + ".cht")).string();
    return std::string(BK_APP_ROOT_DIR) + "cheats/" + stem + ".cht";
}

brls::View* DataPage::buildCheatPanel(const std::string& fileName)
{
    auto* scroll = new brls::ScrollingFrame();
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);

    auto* contentBox = new brls::Box(brls::Axis::COLUMN);
    contentBox->setPadding(10.f, 20.f, 20.f, 20.f);

    std::string cheatPath = resolveCheatPath(fileName);
    auto cheats = parseCheatFile(cheatPath);

    if (cheats.empty()) {
        auto* lbl = new brls::Label();
        lbl->setText("beiklive/datapage/no_cheats"_i18n);
        lbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        lbl->setMarginTop(30.f);
        contentBox->addView(lbl);
    } else {
        for (int i = 0; i < static_cast<int>(cheats.size()); ++i) {
            auto* cell = new brls::Button();
            cell->setText(cheats[i].desc);
            cell->setFocusable(true);

            // A 键：弹出文本输入框，修改 code
            int captIdx      = i;
            std::string captPath = cheatPath;
            std::string captCode = cheats[i].code;

            cell->registerAction("beiklive/datapage/edit_code"_i18n, brls::BUTTON_A,
                [captIdx, captPath, captCode, cell](brls::View*) -> bool {
                    brls::Application::getPlatform()->getImeManager()->openForText(
                        [captIdx, captPath, captCode, cell](const std::string& newCode) {
                            if (newCode.empty()) return;
                            // 重新读取文件，修改指定条目后保存
                            auto entries = parseCheatFile(captPath);
                            if (captIdx < static_cast<int>(entries.size())) {
                                entries[captIdx].code = newCode;
                                saveCheatFile(captPath, entries);
                                // 更新 UI 显示
                            }
                        },
                        "beiklive/datapage/edit_code"_i18n,
                        "",
                        256,
                        captCode);
                    return true;
                });

            contentBox->addView(cell);
        }
    }

    scroll->setContentView(contentBox);
    return scroll;
}
