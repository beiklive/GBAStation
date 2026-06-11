#pragma once

#include "core/common.h"
#include "ui/utils/Box.hpp"
#include "ui/utils/FunctionButtons.hpp"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_set>

namespace beiklive
{
    struct ImportItem
    {
        std::string romPath;
        std::string label;
    };

    struct ImportSharedConfig
    {
        int platform;
        std::string overlayPath;
        std::string shaderPath;
        bool overlayEnabled;
        bool shaderEnabled;
    };

    class GameImportView : public beiklive::Box
    {
    public:
        GameImportView();
        ~GameImportView();

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;

    private:
        void setupButtonLayout();
        void setupProgressLayout();
        void showButtonLayout();
        void showProgressLayout();

        void onSelectLpl(int platform);
        void startImport(const std::string& lplPath, int platform);

        void _selectRomDir();
        void _startDirImport(const std::string& dirPath);

        static std::string platformName(int platform);
        static std::string platformDirName(int platform);

        brls::Box* m_layoutBox = nullptr;
        brls::Box* m_progressBox = nullptr;
        brls::Label* m_progressTitleLabel = nullptr;
        brls::Label* m_progressCountLabel = nullptr;
        brls::Label* m_progressNameLabel = nullptr;
        brls::Rectangle* m_progressBar = nullptr;

        brls::Box* m_leftPanel = nullptr;
        brls::Box* m_rightPanel = nullptr;
        bool m_autoSubDir = true;
        bool m_useNameMapping = true;
        bool m_scanGBA = true;
        bool m_scanGBC = true;
        bool m_scanGB = true;
        bool m_scanNES = true;
        bool m_scanSNES = true;

        std::thread m_importThread;
        std::atomic<bool> m_importing{false};
        std::atomic<bool> m_importDone{false};
        std::atomic<bool> m_importError{false};
        std::atomic<int> m_progress{0};
        std::atomic<int> m_total{0};
        std::mutex m_errorMutex;
        std::string m_errorMsg;
        bool m_completionShown = false;
    };
}
