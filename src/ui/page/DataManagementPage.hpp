#pragma once

#include "core/common.h"
#include "ui/widget/Box.hpp"
#include "ui/widget/TabFrame.hpp"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace beiklive
{
    /**
     * DataManagementPage – 数据管理页面
     *
     * 布局：使用 TabFrame 展示子页面。
     * 当前提供“扫描导入”“整合包导入”“数据处理”三个标签页。
     */
    class DataManagementPage : public beiklive::Box
    {
    public:
        DataManagementPage();
        ~DataManagementPage();

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;

    private:
        enum class ProgressTask
        {
            Import,
            Cleanup,
        };

        beiklive::TabFrame* m_tabframe = nullptr;
        brls::View* m_scanDefaultFocus = nullptr;
        brls::View* m_bundleDefaultFocus = nullptr;
        brls::View* m_processDefaultFocus = nullptr;
        brls::View* m_focusBeforeModal = nullptr;

        brls::Box* m_progressOverlay = nullptr;
        brls::Label* m_progressTitleLabel = nullptr;
        brls::Label* m_progressCountLabel = nullptr;
        brls::Label* m_progressNameLabel = nullptr;
        brls::Rectangle* m_progressBar = nullptr;

        bool m_autoSubDir = true;
        bool m_useNameMapping = true;
        bool m_scanGBA = true;
        bool m_scanGBC = true;
        bool m_scanGB = true;
        bool m_scanNES = true;
        bool m_scanSNES = true;
        bool m_scanNDS = true;

        std::thread m_importThread;
        std::atomic<bool> m_importing{false};
        std::atomic<bool> m_importDone{false};
        std::atomic<bool> m_importError{false};
        std::atomic<int> m_progress{0};
        std::atomic<int> m_total{0};
        std::atomic<int> m_cleanupRemoved{0};
        std::atomic<bool> m_alive{true};
        std::mutex m_statusMutex;
        std::string m_errorMsg;
        std::string m_progressName;
        bool m_completionShown = false;
        ProgressTask m_progressTask = ProgressTask::Import;

        brls::View* buildScanImportTab();
        brls::View* buildBundleImportTab();
        brls::View* buildDataProcessingTab();
        void setupProgressOverlay();
        void showProgressOverlay();
        void hideProgressOverlay();
        void rememberFocusBeforeModal();
        void restoreFocusAfterModal();
        brls::View* getFallbackFocus();
        void init();
        void resetProgressUi(const std::string& title);
        void onSelectLpl(int platform);
        void startImport(const std::string& lplPath, int platform);
        void selectRomDir();
        void startDirImport(const std::string& dirPath);
        void removeInvalidGames();
        void clearGameLibrary();
        void startWebService();
        void updateProgressName(const std::string& name);
        void setErrorMessage(const std::string& msg);
        void finishWorker();
    };

} // namespace beiklive
