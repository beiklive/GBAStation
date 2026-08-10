#pragma once

#include <optional>
#include <string>

#include "GameDataProvider.hpp"
#include "Widget.hpp"

namespace beiklive
{
    /// 游戏封面 Widget：封面图 + 标题 + 平台标识 + 游玩时间 + 焦点效果
    /// 不保存游戏数据，只持有 gameId，通过 GameDataProvider 查询
    class GameCoverWidget : public Widget
    {
    public:
        explicit GameCoverWidget(std::string gameId);
        ~GameCoverWidget() override;

        void setGameId(const std::string& id);
        void setGameDataProvider(GameDataProvider* provider);

        void draw(NVGcontext* vg, const GridRect& rect) override;
        void onFocus() override;
        void onBlur() override;

        std::string typeName() const override { return "game_cover"; }
        std::string dataId() const override { return m_gameId; }
        std::string displayName() override;

    private:
        bool loadCover(NVGcontext* vg);

        std::string m_gameId;
        GameDataProvider* m_provider = nullptr;

        std::optional<GameInfo> m_info;
        bool m_infoResolved = false;

        std::string m_coverPath;
        int m_coverTexture = 0;
        bool m_textureRequested = false;
        bool m_focused = false;
    };
} // namespace beiklive
