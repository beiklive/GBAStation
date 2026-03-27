#pragma once 

#include "core/common.h"
#include "ui/layout/SwitchLayout.hpp"
#include "ui/page/FileListPage.hpp" 
#include "ui/page/GamePage.hpp" 
#include "ui/utils/Box.hpp"


// 起始页，所有页面的通用接口在这里定义调用

class StartPage : public beiklive::Box
{
public:
    StartPage();
    ~StartPage();

    void Init();
    void onResume();

private:
    void _useSwitchLayout();
    void _openFileList();

    beiklive::FileListPage* m_fileListPage = nullptr;
    beiklive::SwitchLayout* switchLayout = nullptr;
    beiklive::GamePage* m_gamePage = nullptr;
};