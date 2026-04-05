#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include <functional>
#include "ButtonBox.hpp"
#include "GridBox.hpp"
#include "GridItem.hpp"
namespace beiklive
{
    /// 存档槽位状态信息
    struct StateSlotInfo {
        bool        exists    = false; ///< 存档文件是否存在
        std::string thumbPath;         ///< 缩略图路径（存在时非空）
        std::string timeStr;           ///< 存档文件的修改时间字符串
    };

    /// 游戏菜单视图
    ///
    /// 以半透明覆盖层形式显示在游戏画面之上，提供：
    ///   - 返回游戏按钮（将焦点交还给 GameView）
    ///   - 保存状态按钮（打开保存状态面板）
    ///   - 读取状态按钮（打开读取状态面板）
    ///   - 金手指设置按钮（打开金手指面板，待实现）
    ///   - 画面设置按钮（打开画面设置面板，待实现）
    ///   - 退出游戏按钮（触发退出信号）
    ///
    /// 由 GamePage 创建并注入回调后使用。
    class GameMenuView : public brls::Box
    {
        public:
            GameMenuView(beiklive::GameEntry gameData);
            ~GameMenuView();

            void draw(NVGcontext* vg, float x, float y, float w, float h,
                      brls::Style style, brls::FrameContext* ctx) override;

            /// 设置"返回游戏"回调（由 GamePage 注入）
            void setOnResume(std::function<void()> cb) { m_onResume = std::move(cb); }
            /// 设置"退出游戏"回调（由 GamePage 注入）
            void setOnExit(std::function<void()> cb) { m_onExit = std::move(cb); }

            /// 设置保存状态回调（slot: 0=自动存档，1-9=手动槽位）
            void setSaveStateCallback(std::function<void(int)> cb) { m_saveStateCallback = std::move(cb); }
            /// 设置读取状态回调
            void setLoadStateCallback(std::function<void(int)> cb) { m_loadStateCallback = std::move(cb); }
            /// 设置槽位信息查询回调（由 GamePage 注入，供面板扫描存档时使用）
            void setStateInfoCallback(std::function<StateSlotInfo(int)> cb) { m_stateInfoCallback = std::move(cb); }

        private:
            beiklive::GameEntry m_gameEntry;
            std::function<void()> m_onResume;
            std::function<void()> m_onExit;
            std::function<void(int)> m_saveStateCallback;
            std::function<void(int)> m_loadStateCallback;
            std::function<StateSlotInfo(int)> m_stateInfoCallback;

            brls::Box*   m_panel     = nullptr; ///< 居中面板容器

            brls::Box*   m_contrlPanel = nullptr; ///< 按钮容器
            brls::Box*   m_viewPanel   = nullptr; ///< 页面容器

            brls::Label* m_title     = nullptr; ///< 标题文字
            beiklive::ButtonBox*   m_btnResume = nullptr; ///< "返回游戏"按钮
            beiklive::ButtonBox*   m_btnExit   = nullptr; ///< "退出游戏"按钮

            /// 所有子面板（用于 focus 时统一隐藏其他面板）
            std::vector<brls::View*> m_allPanels;
            brls::View* m_savePanel = nullptr; ///< 保存状态面板（用于 focus 时区分刷新）
            brls::View* m_loadPanel = nullptr; ///< 读取状态面板

            /// GridItem 列表，用于存档/读档面板更新
            std::vector<beiklive::GridItem*> m_saveItems; ///< 保存状态面板 GridItem 列表
            std::vector<beiklive::GridItem*> m_loadItems; ///< 读取状态面板 GridItem 列表

            void _initLayout();

            /// 隐藏所有子面板
            void _hideAllPanels();

            /**
             * 创建菜单按钮
             * @param text      按钮文字
             * @param onClick   按钮点击回调（无 sonPanel 时直接调用）
             * @param sonPanel  绑定的子面板（为空则为无面板按钮）
             */
            beiklive::ButtonBox* _createMenuButton(const std::string& text,
                                                   std::function<void()> onClick,
                                                   brls::View* sonPanel = nullptr);

            /// 创建保存状态面板（GridBox，2 列，10 个 GridItem）
            brls::View* _createSaveStatePanel();
            /// 创建读取状态面板（GridBox，2 列，10 个 GridItem）
            brls::View* _createLoadStatePanel();

            /// 异步扫描存档目录并更新指定面板的 GridItem
            void _refreshStatePanel(bool isSave);

            /// 获取槽位显示名称（0="自动存档"，1-9="槽位 N"）
            static std::string _slotName(int slot);
    };

}
