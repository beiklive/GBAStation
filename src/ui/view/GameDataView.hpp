#pragma once

#include <borealis.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/enums.h"

namespace beiklive
{
    class GameDataView : public brls::View
    {
    public:
        enum class Section : int
        {
            STATES = 0,
            SCREENSHOTS,
            BATTERY,
            CHEATS,
            LOAD_CONTENT,
            ADDONS,
        };

        struct StateSlot
        {
            bool exists = false;
            std::string title;
            std::string time;
            std::string thumbnail;
        };

        struct MediaItem
        {
            std::string path;
            std::string title;
            std::string time;
        };

        struct CheatItem
        {
            std::string name;
            std::string code;
            std::string comments;
            bool enabled = false;
        };

        struct ManagedContentItem
        {
            std::string label;
            std::string emptyText;
            std::string enabledPath;
            std::string disabledPath;
            bool enabledExists = false;
            bool disabledExists = false;
            std::size_t enabledFileCount = 0;
            std::size_t disabledFileCount = 0;
        };

        explicit GameDataView(beiklive::GameEntry entry);
        ~GameDataView() override;

        void setStateSlots(std::vector<StateSlot> slots);
        void setScreenshots(std::vector<MediaItem> screenshots);
        void setBackups(std::vector<MediaItem> backups, bool batterySaveExists);
        void setCheats(std::vector<CheatItem> cheats);
        void setLoadContent(ManagedContentItem textures, ManagedContentItem mods);
        void setAddons(ManagedContentItem update, ManagedContentItem dlc);
        void setCoverPath(const std::string& path);
        void openImagePreview(int index);
        void restoreFocus();
        void playExitAnimation(std::function<void()> completion);

        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;

        std::function<void()> onBack;
        std::function<void(Section)> onSectionChanged;
        std::function<void(int)> onDeleteState;
        std::function<void(int)> onDeleteScreenshot;
        std::function<void(int)> onSetScreenshotCover;
        std::function<void()> onExportSave;
        std::function<void()> onImportSave;
        std::function<void()> onBackupSave;
        std::function<void()> onClearShaderCache;
        std::function<void(int)> onRestoreBackup;
        std::function<void(int)> onDeleteBackup;
        std::function<void()> onAddCheat;
        std::function<void(int)> onCheatOptions;
        std::function<void(Section, int)> onToggleManagedContent;
        std::function<void(Section, int)> onDeleteManagedContent;

    private:
        beiklive::GameEntry m_entry;
        std::vector<StateSlot> m_states;
        std::vector<MediaItem> m_screenshots;
        std::vector<MediaItem> m_backups;
        std::vector<CheatItem> m_cheats;
        std::array<ManagedContentItem, 2> m_loadContent;
        std::array<ManagedContentItem, 2> m_addons;
        bool m_batterySaveExists = false;

        Section m_section = Section::STATES;
        int m_stateIndex = 0;
        int m_screenshotIndex = 0;
        int m_actionIndex = 0;
        int m_backupIndex = 0;
        int m_batteryPane = 0;
        int m_cheatPane = 0;
        int m_cheatIndex = 0;
        int m_managedIndex = 0;
        int m_managedAction = 0;
        int m_sectionDirection = 0;

        float m_pageEntrance = 0.f;
        float m_contentTransition = 0.f;
        float m_scrollY = 0.f;
        float m_targetScrollY = 0.f;
        float m_focusTime = 0.f;
        bool m_previewActive = false;
        bool m_previewClosing = false;
        bool m_previewNearest = false;
        int m_previewIndex = -1;
        float m_previewTransition = 0.f;
        float m_previewZoom = 1.f;
        float m_previewOffsetX = 0.f;
        float m_previewOffsetY = 0.f;
        bool m_exitAnimationRunning = false;
        bool m_exitCompletionArmed = false;
        std::function<void()> m_exitCompletion;

        bool m_wasFocused = false;
        bool m_prevUp = false;
        bool m_prevDown = false;
        bool m_prevLeft = false;
        bool m_prevRight = false;
        float m_holdUp = 0.f;
        float m_holdDown = 0.f;
        float m_holdLeft = 0.f;
        float m_holdRight = 0.f;
        float m_repeatUp = 0.f;
        float m_repeatDown = 0.f;
        float m_repeatLeft = 0.f;
        float m_repeatRight = 0.f;

        int m_fontId = -1;
        int m_materialFontId = -1;
        int m_switchIconFontId = -1;
        std::unordered_map<std::string, int> m_imageCache;
        std::unordered_map<std::string, int> m_nearestImageCache;
        int m_imageLoadsThisFrame = 0;
        std::chrono::steady_clock::time_point m_lastFrameTime;

        void _switchSection(int direction);
        bool _moveUp();
        bool _moveDown();
        bool _moveLeft();
        bool _moveRight();
        void _activate();
        void _secondaryAction();
        void _tertiaryAction();
        void _closeImagePreview();
        void _resetImagePreview();
        void _captureDirections();
        void _handleDirectionInput(float dt);
        void _updateScrollTarget(float viewportHeight);
        int _currentScreenshotColumns() const { return 4; }
        bool _isThreeDs() const;

        void _drawHeader(NVGcontext* vg, float x, float y, float w);
        void _drawSummary(NVGcontext* vg, float x, float y, float w, float h);
        void _drawStates(NVGcontext* vg, float x, float y, float w, float h);
        void _drawScreenshots(NVGcontext* vg, float x, float y, float w, float h);
        void _drawBattery(NVGcontext* vg, float x, float y, float w, float h);
        void _drawCheats(NVGcontext* vg, float x, float y, float w, float h);
        void _drawManagedContent(NVGcontext* vg, float x, float y, float w, float h,
                                 const std::array<ManagedContentItem, 2>& items);
        void _drawFooter(NVGcontext* vg, float x, float y, float w, float h);
        void _drawImagePreview(NVGcontext* vg, float x, float y, float w, float h);
        void _drawPanel(NVGcontext* vg, float x, float y, float w, float h,
                        float radius, bool filled = true, float alpha = 1.f);
        void _drawFocus(NVGcontext* vg, float x, float y, float w, float h,
                        float radius, float alpha = 1.f);
        void _drawMaterialIcon(NVGcontext* vg, char32_t icon, float x, float y,
                               float size, NVGcolor color);
        void _drawSwitchButton(NVGcontext* vg, brls::ControllerButton button,
                               float x, float y, float size, NVGcolor color);
        void _drawHint(NVGcontext* vg, float x, float y,
                       brls::ControllerButton button, const std::string& label);
        void _drawImageCover(NVGcontext* vg, const std::string& path,
                             float x, float y, float w, float h, float radius);
        int _getImage(NVGcontext* vg, const std::string& path, bool nearest = false);
    };
}
