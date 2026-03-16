#pragma once

#include <borealis.hpp>
#include "common.hpp"


class TopTabFrame : public brls::Box
{
private:
    std::vector<std::pair<std::string, brls::View*>> tabs;
public:
    TopTabFrame();
    ~TopTabFrame();
    void applyTab();
    void addTab(const std::string& name, brls::View* content);
};






class DataPage : public beiklive::UI::BBox
{
private:
    /* data */
    brls::TabFrame* m_tabFrame = nullptr;
public:
    DataPage(/* args */);
    ~DataPage();

    void addTab(const std::string& name, brls::View* content);

};

