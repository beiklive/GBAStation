#pragma once
#include <borealis.hpp>
#include "ui/page/StartPage.hpp"

namespace beiklive
{
    class MyActivity : public brls::Activity
    {
    public:
        MyActivity(brls::View* view);
        ~MyActivity() = default;

        void setPageView(beiklive::StartPage* view);


        void onResume() override;

    
    private:
        // 这里可以添加一些成员变量来管理页面状态
        beiklive::StartPage* m_pageView = nullptr;
    
    };


} // namespace beiklive
