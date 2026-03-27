#pragma once
#include <borealis.hpp>

namespace beiklive
{
    class MyActivity : public brls::Activity
    {
    public:
        MyActivity(View* view) : brls::Activity(view) {}
        ~MyActivity() = default;

        void setPageView(View* view)
        {
            m_pageView = view;
        }

        void onResume() override
        {
            brls::Activity::onResume();
            if (m_pageView)
            {
                m_pageView->onResume();
            }
        }
    
    private:
        // 这里可以添加一些成员变量来管理页面状态
        View* m_pageView = nullptr;
    
    };


} // namespace beiklive
