#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include <functional>
#include "ui/widget/TabFrame.hpp"
#include "ui/widget/GridBox.hpp"
#include "ui/widget/Box.hpp"
#include "ui/widget/GridItem.hpp"
#include "ui/widget/DetailCell.hpp"
#include "ui/widget/FunctionButtons.hpp"
#include "game/render/GLSLPParser.hpp"
#include "game/retro/LibretroLoader.hpp"
#include <borealis/views/cells/cell_selector.hpp>
namespace beiklive
{
    /// 存档槽位状态信息
    struct StateSlotInfo {
        bool        exists    = false;
        std::string thumbPath;
        std::string timeStr;
    };

    /// 游戏菜单视图
    class GameMenuView : public beiklive::Box
    {
        public:
            GameMenuView(beiklive::GameEntry gameData);
            ~GameMenuView();

            void draw(NVGcontext* vg, float x, float y, float w, float h,
                      brls::Style style, brls::FrameContext* ctx) override;
            void onShow();

            void setOnResume(std::function<void()> cb) { m_onResume = std::move(cb); }
            void setOnReset(std::function<void()> cb)  { m_onReset = std::move(cb); }
            void setOnExit(std::function<void()> cb)   { m_onExit = std::move(cb); }

            void setSaveStateCallback(std::function<void(int)> cb) { m_saveStateCallback = std::move(cb); }
            void setLoadStateCallback(std::function<void(int)> cb) { m_loadStateCallback = std::move(cb); }
            void setStateInfoCallback(std::function<StateSlotInfo(int)> cb) { m_stateInfoCallback = std::move(cb); }
            void setDeleteStateCallback(std::function<void(int)> cb) { m_deleteStateCallback = std::move(cb); }

            /// 重新扫描指定槽位并更新两个网格的显示状态
            void refreshSlotState(int slot);

            /// 设置金手指切换回调 (index, enabled)
            void setCheatToggleCallback(std::function<void(int, bool)> cb) { m_cheatToggleCallback = std::move(cb); }
            /// 设置金手指文件变更回调 (newPath)
            void setCheatPathCallback(std::function<void(const std::string&)> cb) { m_cheatPathCallback = std::move(cb); }
            void setCheatsChangedCallback(std::function<void(const std::vector<CheatEntry>&)> cb) { m_cheatsChangedCallback = std::move(cb); }
            const std::vector<CheatEntry>& getCheats() const { return m_cheats; }
            void setDiskStateCallback(std::function<LibretroLoader::DiskControlState()> cb) { m_diskStateCallback = std::move(cb); }
            void setDiskEjectCallback(std::function<void(bool)> cb) { m_diskEjectCallback = std::move(cb); }
            void setDiskIndexCallback(std::function<void(unsigned)> cb) { m_diskIndexCallback = std::move(cb); }

            /// 向画面设置页追加由具体游戏视图拥有的平台专属控件。
            void addCoreDisplaySettingView(brls::View* view);
            void requestConfigSave();

            /// 画面设置回调
            void setDisplayModeCallback(std::function<void(const std::string&)> cb) { m_displayModeCallback = std::move(cb); }
            void setIntegerScaleCallback(std::function<void(float)> cb) { m_integerScaleCallback = std::move(cb); }
            void setShaderToggleCallback(std::function<void(bool)> cb) { m_shaderToggleCallback = std::move(cb); }
            void setShaderPathCallback(std::function<void(const std::string&)> cb) { m_shaderPathCallback = std::move(cb); }
            void setShaderParamsCallback(std::function<std::vector<ShaderParamInfo>()> cb) { m_shaderParamsCallback = std::move(cb); }
            void setShaderParamCallback(std::function<void(const std::string&, float)> cb) { m_shaderParamCallback = std::move(cb); }

            /// 自定义缩放/偏移变更回调：将 x/y/scale 同步到 GameView
            void setCustomScaleCallback(std::function<void(float, float, float)> cb) { m_customScaleCallback = std::move(cb); }
            /// 遮罩开关回调
            void setOverlayToggleCallback(std::function<void(bool)> cb) { m_overlayToggleCallback = std::move(cb); }
            /// 遮罩路径变更回调
            void setOverlayPathCallback(std::function<void(const std::string&)> cb) { m_overlayPathCallback = std::move(cb); }

        private:
            beiklive::GameEntry m_gameEntry;
            std::function<void()> m_onResume, m_onReset, m_onExit;
            std::function<void(int)> m_saveStateCallback, m_loadStateCallback;
            std::function<void(int)> m_deleteStateCallback;
            std::function<StateSlotInfo(int)> m_stateInfoCallback;
            std::function<void(int, bool)> m_cheatToggleCallback;
            std::function<void(const std::string&)> m_cheatPathCallback;
            std::function<void(const std::vector<CheatEntry>&)> m_cheatsChangedCallback;
            std::function<LibretroLoader::DiskControlState()> m_diskStateCallback;
            std::function<void(bool)> m_diskEjectCallback;
            std::function<void(unsigned)> m_diskIndexCallback;
            std::function<void(const std::string&)> m_displayModeCallback;
            std::function<void(float)> m_integerScaleCallback; ///< 整数倍缩放变更回调 (newScale)
            std::function<void(bool)> m_shaderToggleCallback;
            std::function<void(const std::string&)> m_shaderPathCallback;
            std::function<std::vector<ShaderParamInfo>()> m_shaderParamsCallback;
            std::function<void(const std::string&, float)> m_shaderParamCallback;
            std::function<void(float, float, float)> m_customScaleCallback; ///< custom x/y/scale 变更回调
            std::function<void(bool)> m_overlayToggleCallback;        ///< 遮罩开关回调
            std::function<void(const std::string&)> m_overlayPathCallback; ///< 遮罩路径变更回调
            size_t m_configSaveDelayId = 0;
            bool m_configSavePending = false;

            beiklive::TabFrame* m_panel = nullptr;
            brls::Label* m_title = nullptr;

            brls::View* m_savePanel = nullptr;
            brls::View* m_loadPanel = nullptr;

            std::vector<beiklive::GridItem*> m_saveItems, m_loadItems;
            beiklive::GridBox* m_saveGrid = nullptr;
            beiklive::GridBox* m_loadGrid = nullptr;

            // 金手指面板
            beiklive::ButtonBox* selectChtBtn = nullptr;

            brls::Box* m_cheatItemBox = nullptr;
            brls::Label* m_cheatCountLabel = nullptr;
            brls::Label* cheatPathLabel = nullptr;
            std::vector<beiklive::SwitchButton*> m_cheatSwitches;
            std::vector<CheatEntry> m_cheats;
            bool m_cheatFileReadOnly = false;
            brls::Label* m_diskStatusLabel = nullptr;
            beiklive::DetailCell* m_diskSelectCell = nullptr;

            void _initLayout();
            brls::View* _createSaveStatePanel();
            brls::View* _createLoadStatePanel();
            void _clearGridItemsFocus();
            void _refreshStatePanel(bool isSave);
            static std::string _slotName(int slot);

            /// 金手指面板
            brls::View* _createCheatPanel();
            void _loadCheatsFromPath(const std::string& path);
            void _saveEditableCheats();
            void _notifyCheatsChanged();
            void _rebuildCheatItems();
            void _updateCheatCount();

            /// 画面设置面板
            brls::View* _createDisplayPanel();
            void _openShaderSettings();
            void _openOverlaySettings();
            void _openCustomScaleSettings();
            void _rebuildShaderParamUI();
            brls::View* _createControllerPanel();
            brls::View* _createDiskControlPanel();
            void _refreshDiskControlPanel();
            bool _isFdsGame() const;
            brls::View* _createNesPlayerBox(int player);
            void _openNesKeyCapture(beiklive::DetailCell* cell, const std::string& cfgKey);
            void _notifyPressedController();
            std::vector<std::string> _controllerOptions() const;

            /// 清除当前侧边栏面板
            void _dismissSidePanel(int idx = -1); // idx=-1=全部

            /// 同步设置到同平台其他游戏
            void _syncDisplaySettings();
            void _syncOverlayPath();
            void _syncShaderPath();
            std::string _getPlatformOverlayKey() const;
            std::string _getPlatformShaderKey() const;



            brls::DetailCell* shaderPathcell = nullptr;
            brls::Box* m_coreDisplaySettingsBox = nullptr;
            brls::Box* m_ShaderParamBox = nullptr; ///< 着色器参数面板
            brls::Box* m_ShaderSidePanel = nullptr;  ///< 当前打开的侧边栏 overlay
            brls::Box* m_OverlaySidePanel = nullptr;  ///< 当前打开的侧边栏 overlay
            brls::Box* m_CustomSidePanel = nullptr;  ///< 当前打开的侧边栏 overlay
    };

} // namespace beiklive
