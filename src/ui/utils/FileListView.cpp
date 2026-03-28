#include "FileListView.hpp"

namespace beiklive
{

    int FileListDataSource::numberOfSections(brls::RecyclerFrame *recycler)
    {
        return 1;
    }

    int FileListDataSource::numberOfRows(brls::RecyclerFrame *recycler, int section)
    {
        return m_items ? m_items->size() : 0;
    }

    beiklive::ListItemCell *FileListDataSource::cellForRow(brls::RecyclerFrame *recycler, brls::IndexPath index)
    {
        beiklive::ListItemCell *cell = (beiklive::ListItemCell *)recycler->dequeueReusableCell("Cell");
        const ListItem &item = m_items->at(index.row);
        cell->setTitle(item.text);
        cell->setSubTitle(item.subText);
        cell->setIcon(item.iconPath);
        brls::Logger::info("Creating cell for row " + std::to_string(index.row) + ": " + item.text);
        return cell;
    }

    void FileListDataSource::didSelectRowAt(brls::RecyclerFrame *recycler, brls::IndexPath indexPath)
    {
        if(onItemClicked && m_items && indexPath.row < (int)m_items->size())
        {
            const ListItem &item = m_items->at(indexPath.row);
            onItemClicked(item);
        }
    }

    std::string FileListDataSource::titleForHeader(brls::RecyclerFrame *recycler, int section)
    {
        return std::string("");
    }

    void FileListDataSource::setListItems(ListItemList *items)
    {
        brls::Logger::info("Setting list items, count: " + std::to_string(items ? items->size() : 0));
        if (m_items)
        {
            delete m_items;
            m_items = nullptr;
        }
        m_items = items;
    }

    FileListView::FileListView()
    {
        // this->setWireframeEnabled(true);
        this->setAxis(brls::Axis::COLUMN);
        this->setWidthPercentage(100);
        this->setGrow(1.f);

        m_recycler = new brls::RecyclerFrame();
        m_recycler->setPadding(20.f);

        m_recycler->setGrow(1.f);
        m_dataSource = new FileListDataSource();



        this->addView(m_recycler);
    }

    void FileListView::setItemClickListener(std::function<void(const beiklive::ListItem &)> listener)
    {
        if (m_dataSource)
            m_dataSource->onItemClicked = listener;
    }

    void FileListView::setListItems(ListItemList *items)
    {
        if (m_dataSource)
            m_dataSource->setListItems(items);

        // 调用方已保证在主线程（brls::sync）中调用此函数，直接刷新
        if (m_recycler)
            m_recycler->reloadData();
        brls::Application::giveFocus(m_recycler);
		brls::Logger::info("List items set and UI reloaded.");
    }

    void FileListView::Init()
    {
        m_recycler->estimatedRowHeight = 70;
        m_recycler->registerCell("Cell", [this]()
                                {
                                    auto* cell = new beiklive::ListItemCell(); 
                                    // 给item绑定按键事件
                                    if(onItemActionBind)
                                        onItemActionBind(*cell);
                                    // 给item绑定额外焦点事件
                                    cell->onFocusGainedCallback = [this](std::string title)
                                    {
                                        if(onItemFocused)
                                            onItemFocused(title);
                                    };
                                    // 给item绑定失去焦点事件
                                    cell->onFocusLostCallback = [this](std::string title)
                                    {
                                        if(onItemFocusLost)
                                            onItemFocusLost(title);
                                    };


                                    return cell; 
                                });


        m_recycler->setDataSource(m_dataSource);
    }


} // namespace beiklive
