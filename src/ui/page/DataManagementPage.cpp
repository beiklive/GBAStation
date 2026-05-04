#include "DataManagementPage.hpp"
#include "GameDetailPage.hpp"

namespace beiklive
{
    DataManagementPage::DataManagementPage()
    {
        this->showHeader(true);
        this->showFooter(true);
        this->getHeader()->setTitle("数据管理");
        this->setFocusable(false);

        // 4 列网格布局
        m_grid = new beiklive::GridBox(6);
        m_grid->setGrow(1.f);

        this->getContentBox()->addView(m_grid);

        _loadEntries();
    }

    void DataManagementPage::_loadEntries()
    {
        brls::Application::blockInputs(true);
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN]() {
            m_entries = beiklive::GameDB
                ? beiklive::GameDB->getAll()
                : std::vector<beiklive::GameEntry>{};

            ASYNC_RELEASE
            brls::sync([this]() {
                _rebuildGrid();
                brls::Application::unblockInputs();
            });
        });
    }

    void DataManagementPage::_rebuildGrid()
    {
        brls::Application::giveFocus(nullptr);
        m_grid->clearItems();

        for (int i = 0; i < static_cast<int>(m_entries.size()); ++i)
        {
            const beiklive::GameEntry& entry = m_entries[i];

            m_grid->addItem([this, entry]() -> brls::View* {
                auto* item = new beiklive::GameGridItem(entry);



                return item;
            });
        }
                //         item->onItemClicked = [this](const beiklive::GameEntry& e) {
                //     auto* detailPage = new beiklive::GameDetailPage(e);
                //     auto* frame = new brls::AppletFrame(detailPage);
                //     HIDE_BRLS_BAR(frame);
                //     brls::sync([frame]() {
                //         brls::Application::pushActivity(new brls::Activity(frame));
                //     });
                // };
        m_grid->onItemClicked = [this](int slot) {
            
        };

        // 焦点
        brls::Application::giveFocus(m_grid->getDefaultFocus());
    }

} // namespace beiklive
