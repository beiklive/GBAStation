#pragma once

#include "core/common.h"
#include "ui/widget/Box.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace beiklive
{
    /**
     * DataManagementPage – 数据管理页面
     *
     * 布局：使用统一绘制画布展示“整合包导入”“扫描导入”
     * 和“数据处理”三个标签页。
     */
    class DataManagementPage : public beiklive::Box
    {
    public:
        DataManagementPage();
        ~DataManagementPage();

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;

        friend class ScanProgressDialogView;

    private:
        enum class ProgressTask
        {
            Import,
            Cleanup,
        };

        brls::View* m_mainCanvas = nullptr;
        brls::View* m_bundleDefaultFocus = nullptr;
        brls::View* m_processDefaultFocus = nullptr;
        brls::View* m_focusBeforeModal = nullptr;

        bool m_autoSubDir = true;
        bool m_useNameMapping = true;
        // 各机型的 ROM 扫描目录（空 = 不扫描该机型），持久化到配置 scan.path.*
        std::string m_scanPathNES;
        std::string m_scanPathSNES;
        std::string m_scanPathGB;
        std::string m_scanPathGBC;
        std::string m_scanPathGBA;
        std::string m_scanPathNDS;
        std::string m_scanPath3DS;
        std::string m_scanPathArcade;
        std::string m_scanPathDC;
        std::string m_scanPathGenesis;
        std::string m_scanPathPSP;
        std::string m_scanPathPS1;
        std::string m_scanPathSaturn;
        std::string m_scanPathDolphin;
        int m_scanTabIndex = 1; // Canvas 标签页顺序：bundle=0, scan=1, process=2

        std::thread m_importThread;
        std::atomic<bool> m_importing{false};
        std::atomic<bool> m_importDone{false};
        std::atomic<bool> m_importError{false};
        std::atomic<int> m_progress{0};
        std::atomic<int> m_total{0};
        std::atomic<int> m_importSkipped{0};
        std::atomic<int> m_cleanupRemoved{0};
        std::atomic<bool> m_alive{true};
        std::mutex m_statusMutex;
        std::string m_errorMsg;
        std::string m_progressName;
        bool m_completionShown = false;
        ProgressTask m_progressTask = ProgressTask::Import;

        brls::View* buildBundleImportTab();
        brls::View* buildDataProcessingTab();
        void openLplPlatformSelector();
        void openScanDirectoryManager();
        void rememberFocusBeforeModal();
        void restoreFocusAfterModal();
        brls::View* getFallbackFocus();
        void init();
        void onSelectLpl(int platform);
        void startImport(const std::string& lplPath, int platform);
        void startScanAll();
        void pickScanDir(int platformIndex, std::function<void()> onChanged = {});
        void refreshScanTab();
        void removeInvalidGames();
        void clearGameLibrary();
        void startWebService();
        void launchCiaInstaller();
        void updateProgressName(const std::string& name);
        void setErrorMessage(const std::string& msg);
        void finishWorker();
        void showProgressDialog();
        std::string scanPathFor(int platformIndex) const;
        void setScanPath(int platformIndex, const std::string& path);
        int scanOnePlatform(const std::vector<std::filesystem::path>& roms,
                            const std::string& dirPath,
                            int platform,
                            int startIndex);
    };

} // namespace beiklive
