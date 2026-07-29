#include "FileListPage.hpp"
#include "ui/utils/AnimationHelper.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace beiklive
{
    const float DETAIL_IMAGE_HEIGHT = 160.f;


    namespace
    {
        class FilePanelSurface final : public brls::Box
        {
        public:
            explicit FilePanelSurface(brls::Axis axis = brls::Axis::COLUMN)
                : brls::Box(axis)
            {
                setBackground(brls::ViewBackground::NONE);
            }

            void draw(NVGcontext* vg, float x, float y, float w, float h,
                      brls::Style style, brls::FrameContext* ctx) override
            {
                const NVGpaint shadow = nvgBoxGradient(
                    vg, x + 5.f, y + 6.f, w, h, 8.f, 5.f,
                    nvgRGBA(0, 0, 0, 76), nvgRGBA(0, 0, 0, 0));
                nvgBeginPath(vg);
                nvgRect(vg, x - 3.f, y - 3.f, w + 16.f, h + 17.f);
                nvgRoundedRect(vg, x, y, w, h, 8.f);
                nvgPathWinding(vg, NVG_HOLE);
                nvgFillPaint(vg, shadow);
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, x, y, w, h, 8.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, 7));
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, x + 1.f, y + 1.f, w - 2.f, h - 2.f, 7.f);
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 42));
                nvgStrokeWidth(vg, 1.5f);
                nvgStroke(vg);
                brls::Box::draw(vg, x, y, w, h, style, ctx);
            }
        };

        std::string resolveFileListIcon(beiklive::enums::FileType fileType,
                                        const std::string& fullPath,
                                        const std::string& fallbackIcon)
        {
            if (fileType != beiklive::enums::FileType::NDS_ROM)
                return fallbackIcon;

            std::string ndsIcon = beiklive::GetOrCreateNdsIconPath(fullPath);
            return ndsIcon.empty() ? fallbackIcon : ndsIcon;
        }


        void imageScaleFit(brls::Image* image)
        {
            auto width = image->getOriginalImageWidth();
            auto height = image->getOriginalImageHeight();
            float aspectRatio = width / height;
            image->setHeight(DETAIL_IMAGE_HEIGHT);
            image->setWidth(DETAIL_IMAGE_HEIGHT * aspectRatio);
        }

    }

    FileListPage::FileListPage()
    {
        brls::Logger::debug("FileListPage initialized");
        m_lastFrameTime = std::chrono::steady_clock::now();

        // ── 左侧：文件列表 ──
        auto* leftPanel = new brls::Box(brls::Axis::COLUMN);
        leftPanel->setGrow(1.f);
        leftPanel->setHeightPercentage(100.f);
        leftPanel->setPadding(4.f, 10.f, 4.f, 0.f);
        leftPanel->setShrink(1.f);

        auto* listCard = new brls::Box(brls::Axis::COLUMN);
        listCard->setGrow(1.f);
        listCard->setClipsToBounds(true);
        listCard->setBackground(brls::ViewBackground::NONE);

        fileListView = new beiklive::FileListView();
        fileListView->setGrow(1.f);
        fileListView->setWidthPercentage(100.f);

        fileListView->onItemClicked = [this](const beiklive::ListItem& item) {
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
        };

        fileListView->onItemFocused = [this](const beiklive::ListItem& item) {
            m_focusedFullPath = item.data;
            int idx = 0;
            for (const auto& d : m_dirItems) {
                if (d.fullPath == item.data) {
                    _updateDetailPanel(d);
                    m_positionText = std::to_string(idx + 1) + " / "
                        + std::to_string(m_dirItems.size());
                    invalidate();
                    break;
                }
                idx++;
            }
        };

        fileListView->onItemFocusLost = [this](const beiklive::ListItem& item) {
            brls::Logger::debug("Item focus lost: {}", item.text);
        };

        listCard->addView(fileListView);
        leftPanel->addView(listCard);

        _setupDetailPanel();

        auto* mainRow = new brls::Box(brls::Axis::ROW);
        mainRow->setGrow(1.f);
        mainRow->addView(leftPanel);
        mainRow->addView(m_detailPanel);

        this->getContentBox()->setPadding(104.f, 32.f, 62.f, 32.f);
        this->getContentBox()->addView(mainRow);

        this->showHeader(false);
        this->showFooter(false);

        // B = 返回上一级
        this->registerAction("返回", brls::BUTTON_B,
            [this](brls::View*) {
                if (fileListView->hasActiveFilter()) {
                    fileListView->removeFilter();
                    return true;
                }
                if (m_isAtDriveList || m_currentPath.empty()
                    || fs::path(m_currentPath).parent_path().string() == m_currentPath) {
                    requestClose();
                    return true;
                }
                navigateUp();
                return true;
            },
            false, false, brls::SOUND_BACK);

        // X = 设置映射名
        this->registerAction("设置映射名", brls::BUTTON_X,
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

        // RB = 切换详情面板
        this->registerAction("面板", brls::BUTTON_RB, [this](brls::View*) -> bool {
            _cancelThumbnail();
            m_panelVisible = !m_panelVisible;
            if (m_panelVisible) {
                beiklive::AnimationHelper::slideInFromRight(m_detailPanel, 72.f, 220);
                beiklive::AnimationHelper::fadeIn(m_detailPanel, 180);
            } else {
                beiklive::AnimationHelper::fadeOut(m_detailPanel, 160, true);
            }
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

        // ZR = 搜索
        this->registerAction("搜索", brls::BUTTON_RT, [this](brls::View*) -> bool {
            auto* ime = brls::Application::getPlatform()->getImeManager();
            if (!ime) return true;

            ime->openForText(
                [this](std::string keyword) {
                    brls::sync([this, keyword = std::move(keyword)]() {
                        if (keyword.empty()) {
                            fileListView->removeFilter();
                            return;
                        }
                        fileListView->applyFilter(keyword);
                        if (fileListView->itemCount() == 0) {
                            fileListView->removeFilter();
                            auto* dlg = new brls::Dialog("未搜索到匹配项");
                            dlg->addButton("确定", []() {});
                            dlg->open();
                        }
                    });
                },
                "搜索文件", "", 64, "",
                brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            return true;
        });
    }

    // ============================================================
    // _setupDetailPanel
    // ============================================================
    void FileListPage::_setupDetailPanel()
    {
        m_detailPanel = new brls::Box(brls::Axis::COLUMN);
        m_detailPanel->setWidthPercentage(34.f);
        m_detailPanel->setHeightPercentage(100.f);
        m_detailPanel->setPadding(4.f, 0.f, 4.f, 10.f);

        auto* detailCard = new FilePanelSurface(brls::Axis::COLUMN);
        detailCard->setGrow(1.f);
        detailCard->setPadding(20.f);
        detailCard->setAlignItems(brls::AlignItems::CENTER);
        detailCard->setBackground(brls::ViewBackground::NONE);
        detailCard->setClipsToBounds(true);

        m_detailImage = new brls::Image();
        m_detailImage->setWidth(DETAIL_IMAGE_HEIGHT);
        m_detailImage->setHeight(DETAIL_IMAGE_HEIGHT);
        m_detailImage->setCornerRadius(8.f);
        m_detailImage->setScalingType(brls::ImageScalingType::FIT);
        m_detailImage->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_detailImage->setMarginBottom(14.f);
        m_detailImage->setVisibility(brls::Visibility::GONE);
        m_detailImage->setFocusable(false);
        detailCard->addView(m_detailImage);

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

        m_detailSubtitle = new brls::Label();
        m_detailSubtitle->setFontSize(14.f);
        m_detailSubtitle->setTextColor(nvgRGBA(248, 123, 108, 255));
        m_detailSubtitle->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_detailSubtitle->setSingleLine(true);
        m_detailSubtitle->setWidthPercentage(80.f);
        m_detailSubtitle->setMarginBottom(10.f);
        m_detailSubtitle->setFocusable(false);
        detailCard->addView(m_detailSubtitle);

        auto* div = new brls::Rectangle(nvgRGBA(255, 255, 255, 40));
        div->setWidthPercentage(100.f);
        div->setHeight(1.f);
        div->setMarginBottom(14.f);
        detailCard->addView(div);

        m_detailInfoBox = new brls::Box(brls::Axis::COLUMN);
        m_detailInfoBox->setAlignItems(brls::AlignItems::STRETCH);
        m_detailInfoBox->setWidthPercentage(100.f);
        m_detailInfoBox->setGrow(1.f);
        detailCard->addView(m_detailInfoBox);

        m_detailPanel->addView(detailCard);
    }

    void FileListPage::_clearDetailInfo()
    {
        if (m_detailInfoBox) m_detailInfoBox->clearViews(true);
        if (m_detailImage) m_detailImage->setVisibility(brls::Visibility::GONE);
        if (m_detailTitle) m_detailTitle->setText(" ");
        if (m_detailSubtitle) m_detailSubtitle->setText(" ");
        _cancelThumbnail();
    }

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

    std::string FileListPage::_platformName(int platform)
    {
        auto name = beiklive::tools::platformName(platform);
        return name.empty() ? "未知" : name;
    }

    std::string FileListPage::_formatPlayTime(int seconds)
    {
        if (seconds <= 0) return "未游玩";
        return beiklive::tools::formatPlayTime(seconds);
    }

    std::string FileListPage::_formatFileSizeStr(const std::string& path)
    {
        return beiklive::tools::getFileSizeString(path);
    }

    void FileListPage::_updateDetailPanel(const beiklive::DirListData& data)
    {
        if (!m_panelVisible) return;
        _clearDetailInfo();

        auto ft = data.itemType;
        if (ft == beiklive::enums::FileType::GBA_ROM ||
            ft == beiklive::enums::FileType::GBC_ROM ||
            ft == beiklive::enums::FileType::GB_ROM  ||
            ft == beiklive::enums::FileType::NES_ROM ||
            ft == beiklive::enums::FileType::SNES_ROM ||
            ft == beiklive::enums::FileType::NDS_ROM ||
            ft == beiklive::enums::FileType::THREEDS_ROM ||
            ft == beiklive::enums::FileType::GENESIS_ROM)
        {
            auto entryOpt = beiklive::GameDB ? beiklive::GameDB->findByPath(data.fullPath) : std::nullopt;
            if (entryOpt)
                _showGameDBDetail(data, *entryOpt);
            else
                _showGameNoDBDetail(data);
        }
        else if (ft == beiklive::enums::FileType::IMAGE_FILE)
        {
            _showImageDetail(data);
        }
        else if (ft == beiklive::enums::FileType::DIRECTORY || ft == beiklive::enums::FileType::DRIVE || ft == beiklive::enums::FileType::NONE)
        {
            _showFolderDetail(data);
        }
        else
        {
            _showFileDetail(data);
        }
    }

    void FileListPage::_showGameDBDetail(const beiklive::DirListData& data, const beiklive::GameEntry& entry)
    {
        m_detailTitle->setText(entry.title.empty() ? data.fileName : entry.title);
        m_detailSubtitle->setText(beiklive::tools::getFileNameWithoutExtension(data.fullPath));

        m_detailImage->setVisibility(brls::Visibility::VISIBLE);
        if (!entry.logoPath.empty())
        {
            _requestThumbnail(entry.logoPath);

        }
        else
        {

            m_detailImage->setImageFromFile(data.iconPath.empty()
            ? beiklive::tools::getIconPath(data.itemType)
            : data.iconPath);
        }

        std::string ext = beiklive::tools::getFileExtension(data.fullPath);
        _addBadge(ext, nvgRGBA(79, 193, 255, 200), nvgRGBA(255,255,255,255));
        _addHighlightRow(std::string("游戏时长 ") + _formatPlayTime(entry.playTime),
            entry.playTime > 0 ? nvgRGBA(121, 201, 249, 255) : GET_THEME_COLOR("brls/text_disabled"));

        _addInfoRow("容量", data.fileSize, nvgRGB(173, 168, 255));

        std::string lastPlayed = entry.lastPlayed.empty()
            ? "从未游玩"
            : beiklive::tools::formatTimestampForDisplay(entry.lastPlayed);
        _addInfoRow("最后游玩", lastPlayed, nvgRGB(144, 164, 174));
        _addInfoRow("打开次数", std::to_string(entry.playCount), nvgRGB(129, 199, 132));
        _addInfoRow("路径", data.fullPath, nvgRGB(255, 183, 77));
    }

    void FileListPage::_showGameNoDBDetail(const beiklive::DirListData& data)
    {
        m_detailTitle->setText(data.fileName);
        m_detailSubtitle->setText("未录入数据库");

        m_detailImage->setVisibility(brls::Visibility::VISIBLE);
        m_detailImage->setImageFromFile(data.iconPath.empty()
            ? beiklive::tools::getIconPath(data.itemType)
            : data.iconPath);

        if(!data.iconPath.empty())
            imageScaleFit(m_detailImage);

        std::string ext = beiklive::tools::getFileExtension(data.fullPath);
        _addBadge(ext, nvgRGBA(79, 193, 255, 200), nvgRGBA(255,255,255,255));
        _addInfoRow("容量", data.fileSize, nvgRGB(255, 183, 77));

        _addInfoRow("文件名", beiklive::tools::getFileNameWithoutExtension(data.fullPath), nvgRGB(255, 183, 77));
        _addInfoRow("路径", data.fullPath, nvgRGB(129, 199, 132));
    }

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

    void FileListPage::_showFolderDetail(const beiklive::DirListData& data)
    {
        m_detailTitle->setText(data.fileName);
        m_detailSubtitle->setText("文件夹");

        m_detailImage->setVisibility(brls::Visibility::VISIBLE);
        m_detailImage->setImageFromFile(beiklive::tools::getIconPath(data.itemType));

        _addHighlightRow("文件夹", nvgRGBA(121, 201, 249, 255));
    }

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
            imageScaleFit(m_detailImage);
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

    void FileListPage::updatePath()
    {
        this->getHeader()->setPath(m_currentPath);
        invalidate();
    }

    void FileListPage::setDirSelectionMode(bool on)
    {
        m_dirSelectionMode = on;
        invalidate();
    }

    void FileListPage::setFliter(beiklive::enums::FilterMode mode, std::vector<std::string> extensions)
    {
        m_filterMode = mode;
        m_filterExtensions = extensions;
    }

    void FileListPage::setInitialFocusFilename(const std::string& filename)
    {
        m_pendingFocusFilename = filename;
    }

    FileListPage::~FileListPage()
    {
        _cancelThumbnail();
        brls::Logger::debug("FileListPage destroyed.");
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
        fileListView->saveFocusState(m_currentPath);
        setPath(parentPath);
    }

    // ============================================================
    //  setPath – 后台扫描全部条目，一次性提交
    // ============================================================
    void FileListPage::setPath(const std::string path)
    {
        brls::Application::blockInputs();
        fileListView->saveFocusState(m_currentPath);
        fileListView->setInteractionDisabled(true);
        fileListView->setLoading(true);
        m_previousPath = m_currentPath;
        m_currentPath = path;
        m_isAtDriveList = false;

        fileListView->clearItems();
        m_dirItems.clear();
        m_positionText.clear();
        updatePath();
        fileListView->restoreFocusState(m_currentPath);

        std::string iconPrefix = beiklive::tools::getIconPathPrefix();

        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, path, iconPrefix]() {
            std::vector<beiklive::DirListData> dirData;
            std::vector<beiklive::ListItem> items;

            // ".." 返回上一级
            std::string parentPath = fs::path(path).parent_path().string();
            if (parentPath != path) {
                std::string upIcon = beiklive::tools::getIconPathWithPrefix(
                    beiklive::enums::FileType::NONE, iconPrefix);
                dirData.push_back({"..", path, upIcon,
                    beiklive::enums::FileType::NONE, "返回上一级", 0});
                items.push_back({"..", "返回上一级", upIcon, path});
            }

            std::error_code ec;
            if (fs::exists(path, ec) && fs::is_directory(path, ec)) {
                struct RawEntry { std::string name, fullPath; bool isDir; };
                std::vector<RawEntry> dirs, files;

                for (const auto& entry : fs::directory_iterator(
                        path, fs::directory_options::skip_permission_denied, ec)) {
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
                    }else{
                        // 目录不需要提取扩展名，直接映射整个目录名
                        name = GET_MAPPING_KEY_STR(p.filename().string(), p.filename().string());
                    }

                    if (isDir) dirs.push_back({name, std::move(fullPath), true});
                    else       files.push_back({name + "." + beiklive::tools::getFileExtension(p.filename().string()), std::move(fullPath), false});
                }

                auto nameLess = [](const RawEntry& a, const RawEntry& b) {
                    std::string la = a.name, lb = b.name;
                    for (auto& c : la) c = static_cast<char>(std::tolower((unsigned char)c));
                    for (auto& c : lb) c = static_cast<char>(std::tolower((unsigned char)c));
                    return la < lb;
                };
                std::sort(dirs.begin(), dirs.end(), nameLess);
                std::sort(files.begin(), files.end(), nameLess);

                for (const auto& raw : dirs) {
                    auto fileType = beiklive::tools::getFileType(raw.fullPath);
                    std::string ip = beiklive::tools::getIconPathWithPrefix(fileType, iconPrefix);
                    dirData.push_back({raw.name, raw.fullPath, ip, fileType, "", 0});
                    items.push_back({raw.name, "文件夹", ip, raw.fullPath});
                }

                for (const auto& raw : files) {
                    auto fileType = beiklive::tools::getFileType(raw.fullPath);
                    std::string ip = beiklive::tools::getIconPathWithPrefix(fileType, iconPrefix);
                    ip = resolveFileListIcon(fileType, raw.fullPath, ip);
                    std::string sizeStr = beiklive::tools::getFileSizeString(raw.fullPath);
                    dirData.push_back({raw.name, raw.fullPath, ip, fileType, sizeStr, 0});
                    items.push_back({raw.name, sizeStr, ip, raw.fullPath});
                }
            }

            ASYNC_RELEASE
            brls::sync([this, dd = std::move(dirData), it = std::move(items)]() {
                m_dirItems = std::move(dd);
                fileListView->setItems(it);
                m_positionText = m_dirItems.empty()
                    ? "0 / 0" : "1 / " + std::to_string(m_dirItems.size());
                if (!m_pendingFocusFilename.empty()) {
                    fileListView->focusItemByFilename(m_pendingFocusFilename);
                    m_pendingFocusFilename.clear();
                }
                fileListView->setInteractionDisabled(false);
                fileListView->setLoading(false);
                brls::Application::unblockInputs();
            });
        });
    }

    void FileListPage::frame(brls::FrameContext* ctx)
    {
        beiklive::Box::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.25f) dt = 0.016f;
        m_animTime += dt;
        if (m_closing)
        {
            m_pageEntrance = std::max(0.f, m_pageEntrance - dt * 4.8f);
            if (m_pageEntrance <= 0.f && !m_closeQueued)
            {
                m_closeQueued = true;
                const auto close = onRequestClose;
                brls::sync([close]() {
                    if (close)
                        close();
                    else
                        brls::Application::popActivity(brls::TransitionAnimation::FADE);
                });
            }
        }
        else
        {
            m_pageEntrance = std::min(1.f, m_pageEntrance + dt * 3.8f);
        }
        const float eased = 1.f - std::pow(1.f - m_pageEntrance, 3.f);
        if (auto* content = getContentBox())
        {
            content->setAlpha(std::max(0.f, std::min(1.f, m_pageEntrance)));
            content->setTranslationY((1.f - eased) * 24.f);
        }
        invalidate();
    }

    void FileListPage::requestClose()
    {
        if (m_closing)
            return;
        m_closing = true;
        fileListView->setInteractionDisabled(true);
        invalidate();
    }

    void FileListPage::draw(NVGcontext* vg, float x, float y, float w, float h,
                            brls::Style style, brls::FrameContext* ctx)
    {
        beiklive::Box::draw(vg, x, y, w, h, style, ctx);
        if (m_defaultFont < 0)
            m_defaultFont = brls::Application::getDefaultFont();
        if (m_switchFont < 0)
            m_switchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
        const float alpha = std::max(0.f, std::min(1.f, m_pageEntrance));
        const float eased = 1.f - std::pow(1.f - m_pageEntrance, 3.f);

        nvgSave(vg);
        nvgGlobalAlpha(vg, alpha);
        nvgTranslate(vg, 0.f, -(1.f - eased) * 52.f);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 27.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + 36.f, y + 42.f, "文件浏览", nullptr);

        nvgFontSize(vg, 16.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 190));
        const std::string pathText = m_currentPath.empty() ? "驱动器" : m_currentPath;
        nvgSave(vg);
        nvgIntersectScissor(vg, x + 190.f, y + 22.f,
                           std::max(20.f, w - 370.f), 46.f);
        nvgText(vg, x + 190.f, y + 45.f, pathText.c_str(), nullptr);
        nvgRestore(vg);

        nvgFontSize(vg, 16.f);
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(225, 230, 238, 205));
        nvgText(vg, x + w - 36.f, y + 45.f, m_positionText.c_str(), nullptr);

        const float dividerX = x + 32.f;
        const float dividerY = y + 94.f;
        const float dividerW = w - 64.f;
        const NVGpaint dividerShadow = nvgBoxGradient(
            vg, dividerX + 2.f, dividerY + 2.f, dividerW, 1.f, 0.5f, 5.f,
            nvgRGBA(0, 0, 0, 62), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, dividerX - 3.f, dividerY - 3.f, dividerW + 10.f, 10.f);
        nvgRect(vg, dividerX, dividerY - 0.5f, dividerW, 1.f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, dividerShadow);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, dividerX, dividerY);
        nvgLineTo(vg, dividerX + dividerW, dividerY);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 46));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        nvgRestore(vg);

        auto drawHint = [this, vg](brls::ControllerButton button,
                                   const char* label, float& cursor, float hintY) {
            const std::string glyph = brls::Hint::getKeyIcon(button);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 17.f);
            float bounds[4]{};
            nvgTextBounds(vg, 0.f, 0.f, label, nullptr, bounds);
            cursor -= bounds[2] - bounds[0] + 40.f;
            nvgFontFaceId(vg, m_switchFont);
            nvgFontSize(vg, 24.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 245));
            nvgText(vg, cursor + 12.f, hintY, glyph.c_str(), nullptr);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 17.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(230, 234, 241, 225));
            nvgText(vg, cursor + 28.f, hintY, label, nullptr);
            cursor -= 13.f;
        };

        nvgSave(vg);
        nvgGlobalAlpha(vg, alpha);
        const float hintY = y + h - 27.f + (1.f - eased) * 44.f;
        float cursor = x + w - 30.f;
        const bool atRoot = m_isAtDriveList || m_currentPath.empty()
            || fs::path(m_currentPath).parent_path().string() == m_currentPath;
        drawHint(brls::BUTTON_B,
                 fileListView->hasActiveFilter() ? "关闭搜索"
                     : (atRoot ? "返回" : "上一级"),
                 cursor, hintY);
        drawHint(brls::BUTTON_A, "打开/选择", cursor, hintY);
        if (m_dirSelectionMode)
            drawHint(brls::BUTTON_Y, "选择目录", cursor, hintY);
        drawHint(brls::BUTTON_X, "映射名称", cursor, hintY);
        drawHint(brls::BUTTON_RT, "搜索", cursor, hintY);
        drawHint(brls::BUTTON_RB, m_panelVisible ? "隐藏详情" : "显示详情",
                 cursor, hintY);
        nvgRestore(vg);
    }

    void FileListPage::showDriveList()
    {
#ifndef _WIN32
        setPath("/");
        return;
#endif
        fileListView->setInteractionDisabled(true);
        fileListView->setLoading(true);
        brls::Application::blockInputs();
        m_isAtDriveList = true;
        m_currentPath = "";
        m_positionText.clear();

        fileListView->clearItems();
        m_dirItems.clear();

        std::string iconPrefix = beiklive::tools::getIconPathPrefix();
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, iconPrefix]() {
            std::vector<std::string> drives = beiklive::tools::getLogicalDrives();
            const std::string driveIcon = beiklive::tools::getIconPathWithPrefix(
                beiklive::enums::FileType::DRIVE, iconPrefix);

            std::vector<beiklive::DirListData> dirData;
            std::vector<beiklive::ListItem> items;
            for (const auto& drive : drives) {
                dirData.push_back({drive, drive, driveIcon,
                    beiklive::enums::FileType::DRIVE, "", 0});
                items.push_back({drive, "本地磁盘", driveIcon, drive});
            }

            ASYNC_RELEASE
            brls::sync([this, dd = std::move(dirData), it = std::move(items)]() {
                m_dirItems = std::move(dd);
                fileListView->setItems(it);
                m_positionText = m_dirItems.empty()
                    ? "0 / 0" : "1 / " + std::to_string(m_dirItems.size());
                fileListView->setInteractionDisabled(false);
                fileListView->setLoading(false);
                brls::Application::unblockInputs();
            });
        });
    }

} // namespace beiklive
