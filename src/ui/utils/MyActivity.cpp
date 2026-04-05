#include "MyActivity.hpp"

namespace beiklive
{
    MyActivity::MyActivity(brls::View *view) : brls::Activity(view) {}

    void MyActivity::setPageView(beiklive::StartPage *view)
    {
        m_pageView = view;
    }

    void MyActivity::onResume()
    {
        brls::Activity::onResume();
        if (m_pageView)
        {
            m_pageView->onResume();
        }
    }

} // namespace beiklive
