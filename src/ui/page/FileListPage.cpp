#include "FileListPage.hpp"
#include "ui/utils/AnimationHelper.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace beiklive
{

    FileListPage::FileListPage()
    {
        brls::Logger::debug("FileListPage initialized");

        // ── 左侧：文件列表（grow=1，自动填充剩余空间）──
        auto* leftPanel = new brls::Box(brls::Axis::COLUMN);
        leftPanel->setGrow(1.f);
        leftPanel->setHeightPercentage(100.f);
        leftPanel->setPadding(12.f, 8.f, 12.f, 0.f);
        leftPanel->setShrink(1.f);

        auto* listCard = new brls::Box(brls::Axis::COLUMN);
        listCard->setCornerRadius(12.f);
        listCard->setBackgroundColor(nvgRGBA(255, 255, 255, 10));
        listCard->setShadowType(brls::ShadowType::GENERIC);
        listCard->setShadowVisibility(true);

        // listCard->setBackgroundColor(nvgRGBA(0, 0, 0, 20));
        listCard->setGrow(1.f);
        // listCard->setPadding(8.f);

        fileListView = new beiklive::FileListView();
        fileListView->setItemClickListener([this](const beiklive::ListItem& item) {
            for (const auto& dirItem : m_dirItems) {
                if (dirItem.fullPath == item.data) {
                    if (dirItem.itemType == beiklive::enums::FileType::DIRECTORY ||
                        dirItem.itemType == beiklive::enums::FileType::DRIVE) {
                        setPath(item.data);
                    } else if (dirItem.itemType == beiklive::enums::FileType::NONE) {
                        navigateUp();
                    } else {
                        if (onFileSelected) onFileSelected(dirItem);
                    }
                    break;
                }
            }
        });

        fileListView->onItemActionBind = [this](beiklive::ListItemCell& cell) {
            cell.registerAction(
                "设置映射名", brls::BUTTON_X,
                [this](brls::View*) {
                    if (m_focusedFullPath.empty()) return true;
                    auto* ime = brls::Application::getPlatform()->getImeManager();
                    if (!ime) return true;
                    std::string filename = beiklive::tools::getFileNameWithoutExtension(m_focusedFullPath);
                    std::string curName = GET_MAPPING_KEY_STR(filename,
                        beiklive::tools::getFileNameWithoutExtension(m_focusedFullPath));
                    std::string fullPath = m_focusedFullPath;
                    ime->openForText(
                        [filename, fullPath](std::string text) {
                            if (!text.empty()) {
                                beiklive::NameMappingManager->Set(filename, text, true);
                                beiklive::NameMappingManager->Save();
                                auto entryOpt = beiklive::GameDB ? beiklive::GameDB->findByPath(fullPath) : std::nullopt;
                                if (entryOpt) {
                                    beiklive::GameDB->set(fullPath, "title", nlohmann::json(text));
                                    beiklive::GameDB->flush();
                                }
                            }
                        },
                        "设置映射名称", "", 128, curName,
                        brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
                    return true;
                });

            cell.registerAction("返回上一级", brls::BUTTON_B,
                [this](brls::View*) { navigateUp(); return true; });
        };

        fileListView->onItemFocused = [this](std::string title, std::string fullPath) {
            m_focusedFullPath = fullPath;
            updateIndex(fullPath);
            for (const auto& item : m_dirItems) {
                if (item.fullPath == fullPath) {
                    _updateDetailPanel(item);
                    break;
                }
            }
        };

        fileListView->onItemFocusLost = [this](std::string title, std::string fullPath) {
            brls::Logger::debug("Item focus lost: " + title);
        };

        fileListView->Init();
        listCard->addView(fileListView);
        leftPanel->addView(listCard);

        // ── 右侧 30%：详情面板 ──
        _setupDetailPanel();

        // ── 左右总布局（ROW, left=grow, right=30%）──
        auto* mainRow = new brls::Box(brls::Axis::ROW);
        mainRow->setGrow(1.f);
        mainRow->addView(leftPanel);
        mainRow->addView(m_detailPanel);

        this->getContentBox()->addView(mainRow);

        // ── 页头 ──
        this->showHeader(true);
        this->showFooter(true);
        this->getHeader()->setTitle("文件浏览");

        // ── ZR 切换详情面板 ──
        this->registerAction("面板", brls::BUTTON_RB, [this](brls::View*) -> bool {
            _cancelThumbnail();
            m_panelVisible = !m_panelVisible;
            m_detailPanel->setVisibility(
                m_panelVisible ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
            // 面板重新打开时根据当前聚焦路径查找并恢复详情
            if (m_panelVisible && !m_focusedFullPath.empty()) {
                for (const auto& item : m_dirItems) {
                    if (item.fullPath == m_focusedFullPath) {
                        _updateDetailPanel(item);
                        break;
                    }
                }
            }
            return true;
        });
    }

    // ============================================================
    // _setupDetailPanel – 右侧详情面板结构
    // ============================================================
    void FileListPage::_setupDetailPanel()
    {
        m_detailPanel = new brls::Box(brls::Axis::COLUMN);
        m_detailPanel->setWidthPercentage(30.f);
        m_detailPanel->setHeightPercentage(100.f);
        m_detailPanel->setPadding(12.f, 12.f, 12.f, 8.f);

        auto* detailCard = new brls::Box(brls::Axis::COLUMN);
        detailCard->setGrow(1.f);
        detailCard->setPadding(16.f);
        detailCard->setAlignItems(brls::AlignItems::CENTER);
        detailCard->setBackground(brls::ViewBackground::NONE);
        detailCard->setClipsToBounds(true);
        // ── 缩略图（固定宽高，FIT 保持比例）──
        m_detailImage = new brls::Image();
        m_detailImage->setWidth(160.f);
        m_detailImage->setHeight(160.f);
        m_detailImage->setCornerRadius(8.f);
        m_detailImage->setScalingType(brls::ImageScalingType::FILL);
        m_detailImage->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_detailImage->setMarginBottom(14.f);
        m_detailImage->setVisibility(brls::Visibility::GONE);
        m_detailImage->setFocusable(false);
        detailCard->addView(m_detailImage);

        // ── 标题 ──
        m_detailTitle = new brls::Label();
        m_detailTitle->setFontSize(22.f);
        m_detailTitle->setTextColor(GET_THEME_COLOR("brls/text"));
        m_detailTitle->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_detailTitle->setWidthPercentage(80.f);
        m_detailTitle->setSingleLine(true);
        m_detailTitle->setAnimated(true);
        m_detailTitle->setAutoAnimate(true);
        m_detailTitle->setMarginBottom(10.f);
        m_detailTitle->setFocusable(false);

        detailCard->addView(m_detailTitle);

        // ── 副标题 ──
        m_detailSubtitle = new brls::Label();
        m_detailSubtitle->setFontSize(14.f);
        m_detailSubtitle->setTextColor(nvgRGBA(248, 123, 108, 255));
        m_detailSubtitle->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_detailSubtitle->setSingleLine(true);
        m_detailSubtitle->setWidthPercentage(80.f);
        m_detailSubtitle->setMarginBottom(10.f);
        m_detailSubtitle->setFocusable(false);
        detailCard->addView(m_detailSubtitle);

        // ── 分隔线 ──
        auto* div = new brls::Rectangle(nvgRGBA(255, 255, 255, 40));
        div->setWidthPercentage(100.f);
        div->setHeight(1.f);
        div->setMarginBottom(14.f);
        detailCard->addView(div);

        // ── 信息区（可滚动）──
        m_detailInfoBox = new brls::Box(brls::Axis::COLUMN);
        m_detailInfoBox->setAlignItems(brls::AlignItems::STRETCH);
        m_detailInfoBox->setWidthPercentage(100.f);
        m_detailInfoBox->setGrow(1.f);
        detailCard->addView(m_detailInfoBox);

        m_detailPanel->addView(detailCard);
    }

    // ============================================================
    // _clearDetailInfo – 清空信息区
    // ============================================================
    void FileListPage::_clearDetailInfo()
    {
        if (m_detailInfoBox) m_detailInfoBox->clearViews(true);
        if (m_detailImage) m_detailImage->setVisibility(brls::Visibility::GONE);
        if (m_detailTitle) m_detailTitle->setText(" ");
        if (m_detailSubtitle) m_detailSubtitle->setText(" ");
        _cancelThumbnail();
    }

    // ============================================================
    // _addInfoRow – 添加信息行
    // ============================================================
    void FileListPage::_addInfoRow(const std::string& label, const std::string& value, NVGcolor labelColor)
    {
        auto* row = new brls::Box(brls::Axis::ROW);
        row->setFocusable(false);
        row->setMarginBottom(12.f);
        row->setAlignItems(brls::AlignItems::CENTER);

        auto* lbl = new brls::Label();
        lbl->setText(label);
        lbl->setFontSize(16.f);
        lbl->setTextColor(labelColor);
        lbl->setWidth(70.f);
        lbl->setFocusable(false);
        lbl->setMarginRight(8.f);
        row->addView(lbl);


        auto* val = new brls::Label();
        val->setText(value);
        val->setFontSize(16.f);
        val->setAnimated(true);
        val->setAutoAnimate(true);
        val->setGrow(1.f);
        val->setTextColor(GET_THEME_COLOR("brls/text"));
        val->setHorizontalAlign(brls::HorizontalAlign::RIGHT);
        val->setFocusable(false);
        val->setSingleLine(true);
        row->addView(val);

        m_detailInfoBox->addView(row);
    }

    void FileListPage::_addHighlightRow(const std::string& text, NVGcolor color)
    {
        auto* row = new brls::Box(brls::Axis::ROW);
        row->setFocusable(false);
        row->setMarginBottom(12.f);
        row->setAlignItems(brls::AlignItems::CENTER);
        row->setJustifyContent(brls::JustifyContent::CENTER);

        auto* val = new brls::Label();
        val->setText(text);
        val->setFontSize(22.f);
        val->setTextColor(color);
        val->setFocusable(false);
        val->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        row->addView(val);

        m_detailInfoBox->addView(row);
    }

    void FileListPage::_addBadge(const std::string& text, NVGcolor bgColor, NVGcolor textColor)
    {
        auto* badge = new brls::Box(brls::Axis::ROW);
        badge->setFocusable(false);
        badge->setCornerRadius(4.f);
        badge->setBackgroundColor(bgColor);
        badge->setPadding(3.f, 10.f, 3.f, 10.f);
        badge->setAlignItems(brls::AlignItems::CENTER);
        badge->setJustifyContent(brls::JustifyContent::CENTER);
        badge->setMarginBottom(12.f);

        auto* label = new brls::Label();
        label->setText(text);
        label->setFontSize(14.f);
        label->setTextColor(textColor);
        label->setFocusable(false);
        badge->addView(label);

        auto* wrapper = new brls::Box(brls::Axis::ROW);
        wrapper->setFocusable(false);
        wrapper->setJustifyContent(brls::JustifyContent::CENTER);
        wrapper->addView(badge);
        m_detailInfoBox->addView(wrapper);
    }

    // ============================================================
    // _platformName
    // ============================================================
    std::string FileListPage::_platformName(int platform)
    {
        auto name = beiklive::tools::platformName(platform);
        return name.empty() ? "未知" : name;
    }

    // ============================================================
    // _formatPlayTime
    // ============================================================
    std::string FileListPage::_formatPlayTime(int seconds)
    {
        if (seconds <= 0) return "未游玩";
        return beiklive::tools::formatPlayTime(seconds);
    }

    std::string FileListPage::_formatFileSizeStr(const std::string& path)
    {
        return beiklive::tools::getFileSizeString(path);
    }

    // ============================================================
    // _updateDetailPanel – 根据文件类型更新详情
    // ============================================================
    void FileListPage::_updateDetailPanel(const beiklive::DirListData& data)
    {
        if (!m_panelVisible) return;
        _clearDetailInfo();

        auto ft = data.itemType;
        // 游戏文件
        if (ft == beiklive::enums::FileType::GBA_ROM ||
            ft == beiklive::enums::FileType::GBC_ROM ||
            ft == beiklive::enums::FileType::GB_ROM)
        {
            auto entryOpt = beiklive::GameDB ? beiklive::GameDB->findByPath(data.fullPath) : std::nullopt;
            if (entryOpt)
                _showGameDBDetail(data, *entryOpt);
            else
                _showGameNoDBDetail(data);
        }
        // 图片
        else if (ft == beiklive::enums::FileType::IMAGE_FILE)
        {
            _showImageDetail(data);
        }
        // 文件夹
        else if (ft == beiklive::enums::FileType::DIRECTORY || ft == beiklive::enums::FileType::DRIVE || ft == beiklive::enums::FileType::NONE)
        {
            _showFolderDetail(data);
        }
        // 其他普通文件
        else
        {
            _showFileDetail(data);
        }
    }

    // ============================================================
    // _showGameDBDetail – 数据库已有该游戏
    // ============================================================
    void FileListPage::_showGameDBDetail(const beiklive::DirListData& data, const beiklive::GameEntry& entry)
    {
        m_detailTitle->setText(entry.title.empty() ? data.fileName : entry.title);
        m_detailSubtitle->setText(beiklive::tools::getFileNameWithoutExtension(data.fullPath));

        m_detailImage->setVisibility(brls::Visibility::VISIBLE);
        if (!entry.logoPath.empty())
            _requestThumbnail(entry.logoPath);
        else
            m_detailImage->setImageFromFile(beiklive::tools::getIconPath(data.itemType));

        std::string ext = beiklive::tools::getFileExtension(data.fullPath);
        _addBadge(ext, nvgRGBA(79, 193, 255, 200), nvgRGBA(255,255,255,255));
        _addHighlightRow(std::string("游戏时长 ") + _formatPlayTime(entry.playTime),
            entry.playTime > 0 ? nvgRGBA(121, 201, 249, 255) : GET_THEME_COLOR("brls/text_disabled"));

        _addInfoRow("容量", data.fileSize, nvgRGB(173, 168, 255));


        std::string lastPlayed = entry.lastPlayed.empty()
            ? "从未游玩"
            : beiklive::tools::formatTimestampForDisplay(entry.lastPlayed);
        _addInfoRow("最后游玩", lastPlayed, nvgRGB(144, 164, 174));      // 蓝灰
        _addInfoRow("打开次数", std::to_string(entry.playCount), nvgRGB(129, 199, 132)); // 绿
        _addInfoRow("路径", data.fullPath, nvgRGB(255, 183, 77));        // 橙
    }

    // ============================================================
    // _showGameNoDBDetail – 数据库无该游戏
    // ============================================================
    void FileListPage::_showGameNoDBDetail(const beiklive::DirListData& data)
    {
        m_detailTitle->setText(data.fileName);
        m_detailSubtitle->setText("未录入数据库");

        m_detailImage->setVisibility(brls::Visibility::VISIBLE);
        m_detailImage->setImageFromFile(beiklive::tools::getIconPath(data.itemType));

        std::string ext = beiklive::tools::getFileExtension(data.fullPath);
        _addBadge(ext, nvgRGBA(79, 193, 255, 200), nvgRGBA(255,255,255,255));
        _addInfoRow("容量", data.fileSize, nvgRGB(255, 183, 77));

        _addInfoRow("文件名", beiklive::tools::getFileNameWithoutExtension(data.fullPath), nvgRGB(255, 183, 77));
        _addInfoRow("路径", data.fullPath, nvgRGB(129, 199, 132));
    }

    // ============================================================
    // _showImageDetail
    // ============================================================
    void FileListPage::_showImageDetail(const beiklive::DirListData& data)
    {
        m_detailTitle->setText(data.fileName);
        m_detailSubtitle->setText("");

        m_detailImage->setVisibility(brls::Visibility::VISIBLE);
        _requestThumbnail(data.fullPath);

        std::string ext = beiklive::tools::getFileExtension(data.fullPath);
        _addBadge(ext, nvgRGBA(0, 168, 107, 200), nvgRGBA(255,255,255,255));
        _addInfoRow("容量", data.fileSize, nvgRGB(255, 183, 77));
        _addInfoRow("路径", data.fullPath, nvgRGB(144, 164, 174));
    }

    // ============================================================
    // _showFolderDetail
    // ============================================================
    void FileListPage::_showFolderDetail(const beiklive::DirListData& data)
    {
        m_detailTitle->setText(data.fileName);
        m_detailSubtitle->setText("文件夹");

        m_detailImage->setVisibility(brls::Visibility::VISIBLE);
        m_detailImage->setImageFromFile(beiklive::tools::getIconPath(data.itemType));

        _addHighlightRow(std::to_string(data.childCount) + " 个项目",
            nvgRGBA(121, 201, 249, 255));
    }

    // ============================================================
    // _showFileDetail – 其他普通文件
    // ============================================================
    void FileListPage::_showFileDetail(const beiklive::DirListData& data)
    {
        m_detailTitle->setText(data.fileName);
        m_detailSubtitle->setText(" ");

        m_detailImage->setVisibility(brls::Visibility::VISIBLE);
        m_detailImage->setImageFromFile(beiklive::tools::getIconPath(data.itemType));

        std::string ext = beiklive::tools::getFileExtension(data.fullPath);
        if (!ext.empty())
            _addBadge(ext, nvgRGBA(128,128,128,200), nvgRGBA(255,255,255,255));
        _addInfoRow("容量", data.fileSize, nvgRGB(255, 183, 77));
        _addInfoRow("路径", data.fullPath, nvgRGB(144, 164, 174));
    }

    // ============================================================
    // 缩略图加载（100ms 防抖，防止快速切换闪烁）
    // ============================================================
    void FileListPage::_requestThumbnail(const std::string& path)
    {
        if (path.empty() || !m_detailImage) return;
        _cancelThumbnail();

        m_thumbPendingPath = path;
        int reqId = ++m_thumbReqId;
        size_t delayId = brls::delay(100, [this, path, reqId]() {
            if (reqId != m_thumbReqId) return;
            if (!m_detailImage) return;
            m_detailImage->setImageFromFile(path);
            m_detailImage->setVisibility(brls::Visibility::VISIBLE);
        });
        m_thumbDelayId = (int)delayId;
    }

    void FileListPage::_cancelThumbnail()
    {
        if (m_thumbDelayId > 0) {
            brls::cancelDelay((size_t)m_thumbDelayId);
            m_thumbDelayId = 0;
        }
        ++m_thumbReqId;
    }

    // ============================================================
    //  路径/过滤 逻辑（不变）
    // ============================================================

    void FileListPage::updatePath()
    {
        this->getHeader()->setPath(m_currentPath);
    }

    void FileListPage::updateIndex(std::string fullPath)
    {
        int index = 0;
        for (auto& item : m_dirItems) {
            if (item.fullPath == fullPath) {
                this->getHeader()->setInfo(std::to_string(index + 1) + "/" + std::to_string(m_dirItems.size()));
                break;
            }
            index++;
        }
    }

    void FileListPage::setFliter(beiklive::enums::FilterMode mode, std::vector<std::string> extensions)
    {
        m_filterMode = mode;
        m_filterExtensions = extensions;
    }

    FileListPage::~FileListPage()
    {
        _cancelThumbnail();
        brls::Logger::debug("FileListPage destroyed.");
    }

    void FileListPage::setPath(const std::string path)
    {
        brls::Application::blockInputs();
        m_previousPath = m_currentPath;
        m_currentPath = path;
        m_isAtDriveList = false;

        std::string iconPrefix = beiklive::tools::getIconPathPrefix();
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, path, iconPrefix]() {
            try {
                beiklive::ListItemList* items = new beiklive::ListItemList();
                refreshDirList(path, items, iconPrefix);
                ASYNC_RELEASE
                brls::sync([this, items]() {
                    fileListView->setListItems(items);
                    updatePath();
                    brls::Application::unblockInputs();
                });
            } catch (const std::exception& e) {
                brls::Logger::error("refreshDirList exception: " + std::string(e.what()));
            }
        });
    }

    bool FileListPage::passesFilter(const std::string suffix)
    {
        if (m_filterMode == beiklive::enums::FilterMode::None) return true;
        if (m_filterMode == beiklive::enums::FilterMode::Whitelist) {
            for (const auto& ext : m_filterExtensions)
                if (suffix == ext) return true;
            return false;
        } else if (m_filterMode == beiklive::enums::FilterMode::Blacklist) {
            for (const auto& ext : m_filterExtensions)
                if (suffix == ext) return false;
            return true;
        }
        return true;
    }

    void FileListPage::navigateUp()
    {
        if (m_currentPath.empty() || m_isAtDriveList) return;
        std::string parentPath = fs::path(m_currentPath).parent_path().string();
        if (parentPath == m_currentPath) {
#ifdef _WIN32
            showDriveList();
#endif
            return;
        }
        setPath(parentPath);
    }

    void FileListPage::showDriveList()
    {
#ifndef _WIN32
        setPath("/");
        return;
#endif
        m_isAtDriveList = true;
        m_currentPath = "";
        std::string iconPrefix = beiklive::tools::getIconPathPrefix();
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, iconPrefix]() {
            std::vector<std::string> drives = beiklive::tools::getLogicalDrives();
            const std::string driveIcon = beiklive::tools::getIconPathWithPrefix(
                beiklive::enums::FileType::DRIVE, iconPrefix);
            m_dirItems.clear();
            beiklive::ListItemList* items = new beiklive::ListItemList();
            for (const auto& drive : drives) {
                m_dirItems.push_back({drive, drive, driveIcon,
                    beiklive::enums::FileType::DRIVE, "", 0});
                items->push_back({drive, "本地磁盘", driveIcon, drive});
            }
            ASYNC_RELEASE
            brls::sync([this, items]() {
                fileListView->setListItems(items);
            });
        });
    }

    void FileListPage::refreshDirList(const std::string dirPath, beiklive::ListItemList* items,
                                       const std::string& iconPrefix)
    {
        std::error_code ec;
        if (!fs::exists(dirPath, ec) || !fs::is_directory(dirPath, ec)) return;

        m_previousPath = m_currentPath;
        m_currentPath = dirPath;
        m_dirItems.clear();

        std::string parentPath = fs::path(m_currentPath).parent_path().string();
        if (parentPath != m_currentPath) {
            std::string upIcon = beiklive::tools::getIconPathWithPrefix(
                beiklive::enums::FileType::NONE, iconPrefix);
            m_dirItems.push_back({"..", m_currentPath, upIcon,
                beiklive::enums::FileType::NONE, "返回上一级", 0});
            items->push_back({"..", "返回上一级", upIcon, m_currentPath});
        }

        struct RawEntry { std::string name, fullPath; bool isDir; };
        std::vector<RawEntry> dirs, files;

        for (const auto& entry : fs::directory_iterator(
                dirPath, fs::directory_options::skip_permission_denied, ec)) {
            if (ec) { ec.clear(); continue; }
            std::error_code entryEc;
            bool isDir = entry.is_directory(entryEc);
            if (entryEc) continue;

            const auto& p = entry.path();
            std::string name = p.filename().string();
            name = GET_MAPPING_KEY_STR(
                beiklive::tools::getFileNameWithoutExtension(name),
                beiklive::tools::getFileNameWithoutExtension(name));
            std::string fullPath = p.string();

            if (!isDir) {
                if (!passesFilter(beiklive::tools::getFileExtension(p)))
                    continue;
            }
            if (isDir) dirs.push_back({std::move(name), std::move(fullPath), true});
            else       files.push_back({std::move(name), std::move(fullPath), false});
        }

        auto nameLess = [](const RawEntry& a, const RawEntry& b) {
            std::string la = a.name, lb = b.name;
            for (auto& c : la) c = static_cast<char>(std::tolower((unsigned char)c));
            for (auto& c : lb) c = static_cast<char>(std::tolower((unsigned char)c));
            return la < lb;
        };
        std::sort(dirs.begin(), dirs.end(), nameLess);
        std::sort(files.begin(), files.end(), nameLess);

        auto appendEntry = [&](const RawEntry& raw) {
            auto fileType = beiklive::tools::getFileType(raw.fullPath);
            std::string ip = beiklive::tools::getIconPathWithPrefix(fileType, iconPrefix);
            std::string sizeStr;
            size_t entryCount = 0;
            if (raw.isDir) entryCount = beiklive::tools::countEntries(raw.fullPath);
            else           sizeStr = beiklive::tools::getFileSizeString(raw.fullPath);

            m_dirItems.push_back({raw.name, raw.fullPath, ip, fileType, sizeStr, entryCount});
            items->push_back({raw.name,
                raw.isDir ? (std::to_string(entryCount) + " items") : sizeStr,
                ip, raw.fullPath});
        };

        for (const auto& d : dirs)  appendEntry(d);
        for (const auto& f : files) appendEntry(f);
    }

} // namespace beiklive
