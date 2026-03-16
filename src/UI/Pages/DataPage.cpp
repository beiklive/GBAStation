#include "UI/Pages/DataPage.hpp"

TopTabFrame::TopTabFrame(/* args */)
{
}

TopTabFrame::~TopTabFrame()
{
}

void TopTabFrame::addTab(const std::string& name, brls::View* content)
{
    tabs.emplace_back(name, content);
}

void TopTabFrame::applyTab()
{
    if (tabs.empty()) return;
    this->clearViews(true);
    auto* HBox = new brls::Box(brls::Axis::ROW);
    auto* BodyBox = new brls::Box(brls::Axis::COLUMN);
    for (const auto& [name, view] : tabs) {
        auto *btn = new brls::Button();
        btn->setText(name);
        HBox->addView(btn);
        BodyBox->addView(view);
        view->setVisibility(brls::Visibility::GONE);
        btn->getFocusEvent()->subscribe([this, view](brls::View*) {
            view->setVisibility(brls::Visibility::VISIBLE);
        });
        btn->getFocusLostEvent()->subscribe([this, view](brls::View*) {
            view->setVisibility(brls::Visibility::GONE);
        });
    }
}
// ===================================================================================


DataPage::DataPage(/* args */)
{

}

DataPage::~DataPage()
{

}

void DataPage::addTab(const std::string& name, brls::View* content)
{
    if (!m_tabFrame) {
        m_tabFrame = new brls::TabFrame();
        this->addView(m_tabFrame);
    }
    m_tabFrame->addTab(name, [content]() { return content; });
}