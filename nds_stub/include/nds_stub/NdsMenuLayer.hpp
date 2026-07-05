#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "nds_stub/NdsStubTypes.hpp"

namespace beiklive::nds_stub {

enum class NdsMenuAction {
    None,
    SaveState,
    LoadState,
    DeleteState,
    DisplaySettingsChanged,
    CustomLayoutChanged,
    CustomLayoutCommitted,
    OverlaySettingsChanged,
    OverlaySettingsCommitted,
    OverlayPathSelected,
    ShaderSettingsChanged,
    ShaderSettingsCommitted,
    CheatSettingsChanged,
    ResetGame,
    ExitGame,
};

struct NdsMenuResult {
    NdsMenuAction action = NdsMenuAction::None;
    int slot = -1;
    std::string path;
};

struct NdsStateSlotInfo {
    bool exists = false;
    std::string statePath;
    std::string thumbnailPath;
    std::string modifiedTime;
    std::uint32_t thumbnailTexture = 0;
    int thumbnailWidth = 0;
    int thumbnailHeight = 0;
    bool thumbnailLoadAttempted = false;
    bool thumbnailAvailable = false;
};

struct NdsDisplaySettings {
    float fastForwardMultiplier = 1.0f;
    bool linearFiltering = false;
    bool integerScale = false;
    int layout = 0;
    int orientation = 0;
    int screenGap = 0;
    bool overlayEnabled = false;
    std::string overlayPath;
    bool shaderEnabled = false;
    std::string ndsShaderType = "dot";
    NdsCustomLayoutSettings customLayout {};
};

struct NdsFilePickerEntry {
    std::string name;
    std::string path;
    std::string modifiedTime;
    std::uint64_t size = 0;
    bool isDirectory = false;
};

struct NdsCheatItem {
    enum class Type {
        Category,
        Code,
    };

    Type type = Type::Code;
    std::string name;
    int parent = -1;
    int depth = 0;
    bool expanded = false;
    bool enabled = false;
    std::vector<std::uint32_t> words;
};

class NdsMenuLayer {
public:
    enum class Item {
        Resume,
        SaveState,
        LoadState,
        Cheats,
        Display,
        Reset,
        Exit,
        Count,
    };

    NdsMenuResult update(std::uint64_t buttonsDown, std::uint64_t buttonsHeld);
    void draw() const;
    void setStateSlots(const std::array<NdsStateSlotInfo, 10>& slots);
    void setCheatItems(const std::vector<NdsCheatItem>& cheats);
    const std::vector<NdsCheatItem>& cheatItems() const { return m_cheats; }
    bool consumeCheatSettingsDirty();

    void open();
    void close();
    void toggle();
    bool visible() const { return m_visible; }
    bool active() const;
    bool linearFiltering() const { return m_display.linearFiltering; }
    bool integerScale() const { return m_display.integerScale; }
    int screenLayout() const { return m_display.layout; }
    float fastForwardMultiplier() const { return m_display.fastForwardMultiplier; }
    void setFastForwardMultiplier(float multiplier);
    void setDisplaySettings(const NdsDisplaySettings& settings);
    const NdsDisplaySettings& displaySettings() const { return m_display; }
    void setCustomLayoutSettings(const NdsCustomLayoutSettings& settings);
    const NdsCustomLayoutSettings& customLayoutSettings() const { return m_display.customLayout; }

private:
    enum class FocusScope {
        Tabs,
        Content,
    };

    bool cycleCurrentSetting(int direction);
    bool cycleCustomLayoutSetting(int direction);
    bool cycleOverlaySetting(int direction);
    bool cycleShaderSetting(int direction);
    bool resetCustomLayoutSetting();
    bool activateDisplayControl();
    bool activateCheatControl();
    void beginCustomLayoutEditor();
    void beginOverlaySidebar();
    void beginShaderSidebar();
    void beginFilePicker();
    void closeOverlaySidebar(bool returnToMenu);
    void closeShaderSidebar(bool returnToMenu);
    void closeFilePicker(bool returnToOverlay);
    void reloadFilePickerEntries(const std::string& directory, const std::string& focusPath = {});
    void ensureFilePickerPreview();
    void releaseFilePickerPreview();
    void beginSelectionAnimation(int oldSelected, int newSelected);
    void beginPanelAnimation(bool opening);
    float panelProgress() const;
    float customLayoutEditorProgress() const;
    bool itemHasContent(Item item) const;
    int contentControlCount(Item item) const;
    const std::vector<int>& visibleCheatIndices() const;
    int visibleCheatIndex(int visibleRow) const;
    void invalidateVisibleCheatCache();
    void rebuildVisibleCheatCache() const;
    int nextFocusableDisplayRow(int from, int direction) const;
    bool updateHeldSelector(std::uint64_t buttonsHeld);
    bool updateHeldCustomSelector(std::uint64_t buttonsHeld);
    bool updateHeldShaderSelector(std::uint64_t buttonsHeld);
    std::uint64_t updateHeldNavigation(std::uint64_t buttonsDown, std::uint64_t buttonsHeld);
    void openDeleteDialog();
    void closeDeleteDialog();
    float targetContentScrollY() const;
    float smoothedContentScrollY() const;
    void resetContentScroll();

    bool m_visible = false;
    int m_selected = 0;
    FocusScope m_focusScope = FocusScope::Tabs;
    int m_contentFocus = 0;
    NdsDisplaySettings m_display {};
    std::array<NdsStateSlotInfo, 10> m_slots {};
    std::vector<NdsCheatItem> m_cheats {};
    mutable std::vector<int> m_visibleCheatCache {};
    mutable bool m_visibleCheatCacheDirty = true;
    bool m_cheatSettingsDirty = false;
    int m_previousSelected = 0;
    std::uint64_t m_selectionAnimStartTick = 0;
    bool m_selectionAnimating = false;
    std::uint64_t m_panelAnimStartTick = 0;
    bool m_panelAnimating = false;
    bool m_panelOpening = false;
    bool m_deleteDialogVisible = false;
    int m_deleteSlot = -1;
    bool m_customLayoutEditorVisible = false;
    bool m_customLayoutEditorClosing = false;
    bool m_customLayoutReturnToMenu = false;
    std::uint64_t m_customLayoutAnimStartTick = 0;
    int m_customLayoutFocus = 0;
    bool m_overlaySidebarVisible = false;
    bool m_overlaySidebarClosing = false;
    bool m_overlaySidebarReturnToMenu = false;
    std::uint64_t m_overlaySidebarAnimStartTick = 0;
    int m_overlaySidebarFocus = 0;
    bool m_shaderSidebarVisible = false;
    bool m_shaderSidebarClosing = false;
    bool m_shaderSidebarReturnToMenu = false;
    std::uint64_t m_shaderSidebarAnimStartTick = 0;
    int m_shaderSidebarFocus = 0;
    bool m_filePickerVisible = false;
    bool m_filePickerClosing = false;
    bool m_filePickerReturnToOverlay = false;
    std::uint64_t m_filePickerAnimStartTick = 0;
    std::string m_filePickerDirectory;
    std::vector<NdsFilePickerEntry> m_filePickerEntries;
    int m_filePickerFocus = 0;
    mutable float m_filePickerScrollY = 0.0f;
    mutable std::uint64_t m_filePickerScrollLastTick = 0;
    std::uint32_t m_filePickerPreviewTexture = 0;
    int m_filePickerPreviewWidth = 0;
    int m_filePickerPreviewHeight = 0;
    std::string m_filePickerPreviewPath;
    bool m_filePickerPreviewAttempted = false;
    bool m_filePickerImagePreviewVisible = false;
    std::uint64_t m_selectorRepeatStartTick = 0;
    std::uint64_t m_selectorLastStepTick = 0;
    int m_selectorDirection = 0;
    std::uint64_t m_navRepeatStartTick = 0;
    std::uint64_t m_navLastStepTick = 0;
    int m_navDirection = 0;
    mutable float m_contentScrollY = 0.0f;
    mutable std::uint64_t m_contentScrollLastTick = 0;
};

} // namespace beiklive::nds_stub
