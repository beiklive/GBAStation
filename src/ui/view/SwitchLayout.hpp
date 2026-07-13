#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <borealis.hpp>

#include "Layout.hpp"
#include "core/common.h"

namespace beiklive
{
    class SwitchLayout : public beiklive::Layout
    {
    public:
        SwitchLayout();
        ~SwitchLayout() override;

        void refreshGameList(beiklive::GameList gameList) override;
        brls::Box* getContentBox() { return this; }
        void restoreCardFocus(bool animated = false);
        void resetCardFocusToFirst();
        void removeGameByPath(const std::string& path);
        void completeGameRemoval(std::function<void()> completion = {});
        void cancelGameRemoval();
        bool isDeleteAnimationRunning() const
        {
            return m_deleteWaiting || m_deleteCollapsing || m_reflowRunning;
        }
        int acquireSelectedCoverTexture();
        void releaseSelectedCoverTexture();
        void playEntranceAnimation();
        void playExitAnimation(std::function<void()> completion = {});

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
        enum class FocusRow
        {
            GAMES,
            FUNCTIONS,
        };

        struct HomeGame
        {
            beiklive::GameEntry entry;
            float focus = 0.f;
        };

        struct FunctionItem
        {
            std::string label;
            std::string imagePath;
            int imageHandle = 0;
        };

        struct DecodedTexture
        {
            std::string path;
            uint64_t generation = 0;
            int width = 0;
            int height = 0;
            std::vector<unsigned char> pixels;
            bool failed = false;
        };

        struct TextureLoaderState
        {
            std::atomic<bool> alive{true};
            std::mutex mutex;
            std::deque<DecodedTexture> ready;
            uint64_t generation = 0;
            std::unordered_set<std::string> wanted;
            std::unordered_map<std::string, uint64_t> pending;
            int activeDecodes = 0;
        };

        std::vector<HomeGame> m_games;
        std::vector<FunctionItem> m_functions;
        std::vector<float> m_functionFocus;
        std::vector<float> m_slotFocus;
        int m_selectedGame = 0;
        int m_selectedFunction = 0;
        FocusRow m_focusRow = FocusRow::GAMES;

        float m_scrollX = 0.f;
        float m_targetScrollX = 0.f;
        float m_pageEntrance = 0.f;
        float m_contentEntrance = 0.f;
        float m_time = 0.f;
        float m_statusRefreshTimer = 1.f;
        bool m_loading = true;
        bool m_snapScroll = true;
        bool m_exitAnimationRunning = false;
        bool m_exitCompletionArmed = false;
        std::function<void()> m_exitCompletion;
        bool m_fastScroll = false;
        bool m_functionClickAnimating = false;
        int m_functionClickIndex = -1;
        float m_functionClickTime = 0.f;
        bool m_deleteWaiting = false;
        bool m_deleteCollapsing = false;
        bool m_deleteBackendFinished = false;
        bool m_reflowRunning = false;
        float m_deleteAnimationTime = 0.f;
        float m_deleteCollapseProgress = 0.f;
        float m_reflowProgress = 1.f;
        int m_deleteIndex = -1;
        int m_reflowStartIndex = -1;
        int m_newBlankIndex = -1;
        bool m_hasPendingGameList = false;
        beiklive::GameList m_pendingGameList;
        std::function<void()> m_deleteCompletion;

        int m_fontId = -1;
        int m_materialFontId = -1;
        int m_switchIconFontId = -1;
        std::chrono::steady_clock::time_point m_lastFrameTime;

        std::shared_ptr<TextureLoaderState> m_textureLoader;
        std::unordered_map<std::string, int> m_textureCache;
        std::unordered_set<std::string> m_failedTextures;
        bool m_textureCacheDirty = false;
        std::string m_pinnedTexturePath;
        bool m_pinnedTextureInvalidated = false;
        int m_pinnedTextureReferences = 0;
        bool m_networkConnected = false;
        std::string m_clockText;

        bool m_prevLeft = false;
        bool m_prevRight = false;
        bool m_prevUp = false;
        bool m_prevDown = false;
        bool m_prevA = false;
        float m_holdLeft = 0.f;
        float m_holdRight = 0.f;
        float m_repeatLeft = 0.f;
        float m_repeatRight = 0.f;

        void _captureInputState();
        void _updateStatusIndicators(float dt);
        void _handleInput(float dt);
        void _moveHorizontal(int direction);
        void _moveVertical(int direction);
        void _activateCurrent();
        void _activateFunction(int index);
        void _updateTargetScroll(float width);

        void _resetTextureRequests();
        void _requestTexture(const std::string& path);
        void _requestTexturesByPriority();
        void _uploadDecodedTextures(NVGcontext* vg);
        void _evictUnusedTextures(NVGcontext* vg);

        void _drawGames(NVGcontext* vg, float x, float y, float w, float h);
        void _drawGameCard(NVGcontext* vg, const HomeGame& game, int index,
                           float x, float y, float w, float h, float entrance);
        void _drawEmptyCard(NVGcontext* vg, int index, float x, float y,
                            float w, float h, float entrance, float scale = 1.f);
        void _drawDeletingCard(NVGcontext* vg, float x, float y,
                               float w, float h, float entrance);
        void _drawCover(NVGcontext* vg, const std::string& path,
                        float x, float y, float w, float h, float alpha);
        void _drawFunctions(NVGcontext* vg, float x, float y, float w, float h);
        void _drawFooterHint(NVGcontext* vg, float x, float y, float w, float h);
        void _drawMaterialIcon(NVGcontext* vg, char32_t icon,
                               float x, float y, float size, NVGcolor color);
    };
} // namespace beiklive
