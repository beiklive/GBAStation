#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

#include "core/enums.h"
#include "ui/widget/Box.hpp"
#include "ui/view/GameDataView.hpp"

namespace beiklive
{
    class GameDataPage : public beiklive::Box
    {
    public:
        explicit GameDataPage(beiklive::GameEntry entry);
        ~GameDataPage() override;

    private:
        beiklive::GameEntry m_entry;
        beiklive::GameDataView* m_view = nullptr;
        std::vector<std::filesystem::path> m_screenshotPaths;
        std::vector<std::filesystem::path> m_backupPaths;
        std::shared_ptr<std::atomic<bool>> m_alive =
            std::make_shared<std::atomic<bool>>(true);
        bool m_closing = false;

        std::string _saveDir() const;
        std::string _statePath(int slot) const;
        std::string _stateThumbPath(int slot) const;
        std::string _savPath() const;
        bool _isThreeDs() const;
        std::string _threeDsId() const;
        std::string _backupDir() const;

        void _initView();
        void _closeAnimated();
        void _refreshStateList();
        void _refreshScreenshotList();
        void _refreshBackupList();
        void _confirmDeleteState(int slot);
        void _confirmDeleteScreenshot(int index);
        void _confirmSetScreenshotAsCover(int index);
        void _exportSav();
        void _importSav();
        void _backupSav();
        void _confirmRestoreBackup(int index);
        void _confirmDeleteBackup(int index);
    };
}
