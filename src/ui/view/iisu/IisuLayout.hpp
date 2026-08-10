#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include <borealis.hpp>

#include "../Layout.hpp"
#include "../../../core/common.h"
#include "GridDebugRenderer.hpp"

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
        bool isDeleteAnimationRunning() const { return false; }

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
        struct FunctionItem
        {
            std::string label;
            std::string imagePath;
            int imageHandle = 0;
        };

        beiklive::GameList m_games;
        bool m_pico8ShortcutVisible = true;
        int m_fontId = -1;
        GridDebugRenderer m_grid;

        std::vector<FunctionItem> m_functions;
        std::vector<float> m_functionFocus;
        int m_selectedFunction = 0;
        float m_time = 0.f;
        std::chrono::steady_clock::time_point m_lastFrameTime;

        bool m_functionClickAnimating = false;
        int m_functionClickIndex = -1;
        float m_functionClickTime = 0.f;

        bool m_prevLeft = false;
        bool m_prevRight = false;
        bool m_prevA = false;
        float m_holdLeft = 0.f;
        float m_holdRight = 0.f;
        float m_repeatLeft = 0.f;
        float m_repeatRight = 0.f;

        void _captureInputState();
        void _handleInput(float dt);
        void _moveHorizontal(int direction);
        void _activateCurrent();
        void _activateFunction(int index);

        void _drawFunctions(NVGcontext* vg, float x, float y, float w, float h);
    };
} // namespace beiklive
