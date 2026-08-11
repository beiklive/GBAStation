#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <borealis.hpp>

#include "../Layout.hpp"
#include "../../../core/common.h"
#include "Editor/LayoutEditor.hpp"
#include "GridDebugRenderer.hpp"
#include "UIContext.hpp"

namespace beiklive
{
    /// IISU 布局主页（占位实现，接口与 SwitchLayout 对齐）
    class IisuLayout : public beiklive::Layout
    {
    public:
        IisuLayout();
        ~IisuLayout() override;

        void refreshGameList(beiklive::GameList gameList) override;
        brls::Box* getContentBox() { return this; }

        void restoreCardFocus(bool animated = false);
        void resetCardFocusToFirst();
        void removeGameByPath(const std::string& path);
        void completeGameRemoval(std::function<void()> completion = {});
        void cancelGameRemoval();
        bool isDeleteAnimationRunning() const { return m_deleteLayout != nullptr; }

        int acquireSelectedCoverTexture();
        void releaseSelectedCoverTexture();

        void playEntranceAnimation();
        void playExitAnimation(std::function<void()> completion = {});
        void playPico8ExitAnimation(std::function<void()> completion = {});
        void beginPico8ReturnAnimation();
        void setPico8ReturnProgress(float progress);
        void finishPico8ReturnAnimation();

        void setPico8ShortcutVisible(bool visible);
        bool isPico8ShortcutVisible() const { return m_pico8ShortcutVisible; }

        /// 从 START 菜单进入布局编辑状态
        void enterEditMode();
        /// 从 START 菜单请求卡片设置（聚焦在卡片上时弹出占位面板）
        void requestCardSettings();

        std::function<void()> onPico8Opened;

        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;
        brls::View* getDefaultFocus() override { return this; }
        brls::View* getNextFocus(brls::FocusDirection, brls::View*) override
        {
            return this;
        }
        brls::View* getParentNavigationDecision(
            brls::View*, brls::View*, brls::FocusDirection) override
        {
            return this;
        }

    private:
        enum class FocusArea
        {
            GRID,      // 布局主体网格
            FUNCTIONS, // 底部功能按钮区
        };

        struct FunctionItem
        {
            std::string label;
            std::string imagePath;
            int imageHandle = 0;
        };

        LayoutManager& _layout() { return m_uiContext.layout(); }
        /// 当前可交互布局：文件夹浮层打开时导向浮层，否则主界面
        LayoutManager& _activeLayout();

        // 卡片操作浮层状态
        bool m_cardPanelOpen = false;
        LayoutItem* m_cardPanelItem = nullptr;
        int m_cardPanelSelected = 0;
        int m_cardPanelImage = 0;
        std::string m_cardPanelImagePath;
        bool m_cardPanelTextureRequested = false;
        // 卡片编辑占位面板状态
        bool m_cardEditOpen = false;
        std::string m_cardEditName;
        LayoutItem* m_cardEditItem = nullptr;
        int m_cardSpeedIndex = 0;
        beiklive::GameList m_games;
        bool m_pico8ShortcutVisible = true;
        int m_fontId = -1;

        // 页面进出场与 PICO-8 转场共用的淡入淡出状态。
        float m_pageOpacity = 1.f;
        bool m_exitAnimationRunning = false;
        std::function<void()> m_exitCompletion;

        // 游戏选项侧栏预览图持有的纹理引用。
        std::string m_selectedCoverPath;
        int m_selectedCoverReferences = 0;

        // 删除动画目标；布局在动画完成前保持不修改，避免焦点指针失效。
        LayoutManager* m_deleteLayout = nullptr;
        size_t m_deleteIndex = 0;
        std::string m_deletePath;
        std::function<void()> m_deleteCompletion;

        UIContext m_uiContext;
        LayoutEditor m_editor;

        std::vector<FunctionItem> m_functions;
        std::vector<float> m_functionFocus;
        int m_selectedFunction = 0;
        FocusArea m_focusArea = FocusArea::GRID;
        float m_time = 0.f;
        std::chrono::steady_clock::time_point m_lastFrameTime;

        bool m_functionClickAnimating = false;
        int m_functionClickIndex = -1;
        float m_functionClickTime = 0.f;

        bool m_prevLeft = false;
        bool m_prevRight = false;
        bool m_prevUp = false;
        bool m_prevDown = false;
        bool m_prevA = false;
        bool m_prevB = false;
        bool m_prevMinus = false;
        float m_holdLeft = 0.f;
        float m_holdRight = 0.f;
        float m_repeatLeft = 0.f;
        float m_repeatRight = 0.f;

        void _captureInputState();
        void _handleInput(float dt);
        void _moveLeft();
        void _moveRight();
        void _moveUp();
        void _moveDown();
        void _handleBack();
        void _toggleEditMode();
        void _exitEditMode();
        void _handleEditInput(float dt);
        void _applyEditShake();
        void _moveFunctionHorizontal(int direction);
        void _activateCurrent();
        void _activateFunction(int index);
        std::optional<beiklive::GameEntry> _currentGameEntry() const;
        void _animateEntrance(LayoutManager& layout, float delay = 0.f);

        // 卡片操作浮层（非文件夹 Widget 按 A 弹出）
        void _openCardPanel();
        void _closeCardPanel();
        void _handleCardPanelInput(float dt);
        void _drawCardPanel(NVGcontext* vg, float x, float y, float w, float h);
        // 卡片编辑占位面板（START 菜单 → 卡片设置）
        void _openCardEditPanel();
        void _closeCardEditPanel();
        void _drawCardEditPanel(NVGcontext* vg, float x, float y, float w, float h);
        // GIF 播放速度调节
        int _snapSpeedIndex(float speed);
        static const std::vector<float>& _speedOptions();
        void _adjustCardSpeed(int dir);
        // 圆角图片绘制（面板缩略图复用）
        void _drawRoundedImage(NVGcontext* vg, int texture, const GridRect& rect);

        void _drawFunctions(NVGcontext* vg, float x, float y, float w, float h);
    };
} // namespace beiklive
