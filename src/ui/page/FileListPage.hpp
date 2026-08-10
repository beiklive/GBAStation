#pragma once
#include "core/common.h"
#include "ui/widget/Box.hpp"
#include "ui/view/FileListView.hpp"
#include "core/Tools.hpp"
#include <atomic>
#include <chrono>
#include <functional>

namespace beiklive
{

    class FileListPage : public beiklive::Box
    {
    private:
        beiklive::enums::FilterMode m_filterMode = beiklive::enums::FilterMode::None;
        std::vector<std::string>    m_filterExtensions;
        std::string                 m_currentPath;
        std::string                 m_previousPath;
        std::string                 m_pendingFocusFilename;
        bool                        m_isAtDriveList = false;
        std::vector<beiklive::DirListData> m_dirItems;
        beiklive::FileListView*     fileListView;

        bool passesFilter(const std::string suffix);
        void navigateUp();
        void updatePath();

        // ── 右侧详情面板 ──
        brls::Box*      m_detailPanel      = nullptr;
        brls::Image*    m_detailImage      = nullptr;
        brls::Label*    m_detailTitle      = nullptr;
        brls::Label*    m_detailSubtitle   = nullptr;
        brls::Box*      m_detailInfoBox    = nullptr;
        bool m_panelVisible     = true;
        bool m_dirSelectionMode = false;
        std::string m_focusedFullPath;
        std::string m_positionText;
        int m_defaultFont = -1;
        int m_switchFont = -1;
        float m_pageEntrance = 0.f;
        float m_animTime = 0.f;
        bool m_closing = false;
        bool m_closeQueued = false;
        std::chrono::steady_clock::time_point m_lastFrameTime;

        void _setupDetailPanel();
        void _updateDetailPanel(const beiklive::DirListData& data);
        void _clearDetailInfo();

        void _showGameDBDetail(const beiklive::DirListData& data, const beiklive::GameEntry& entry);
        void _showGameNoDBDetail(const beiklive::DirListData& data);
        void _showImageDetail(const beiklive::DirListData& data);
        void _showFolderDetail(const beiklive::DirListData& data);
        void _showFileDetail(const beiklive::DirListData& data);

        void _addInfoRow(const std::string& label, const std::string& value, NVGcolor labelColor = brls::TRANSPARENT);
        void _addHighlightRow(const std::string& text, NVGcolor color);
        void _addBadge(const std::string& text, NVGcolor bgColor, NVGcolor textColor);
        std::string _platformName(int platform);
        std::string _formatPlayTime(int seconds);
        static std::string _formatFileSizeStr(const std::string& path);

        std::atomic<int>        m_thumbDelayId{0};
        int                     m_thumbReqId   = 0;
        std::string             m_thumbPendingPath;
        bool                    m_pickerActive = false;

        void _requestThumbnail(const std::string& path);
        void _cancelThumbnail();

    public:
        FileListPage();
        ~FileListPage();

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;
        void frame(brls::FrameContext* ctx) override;

        void showDriveList();
        void setFliter(beiklive::enums::FilterMode mode, std::vector<std::string> extensions);
        void setPath(const std::string path);
        void setInitialFocusFilename(const std::string& filename);
        void setDirSelectionMode(bool on);
        void requestClose();
        void setInteractionDisabled(bool disabled);
        void setPickerActive(bool active);

        std::function<void(beiklive::DirListData)> onFileSelected;
        std::function<void()> onRequestClose;
    };

} // namespace beiklive
