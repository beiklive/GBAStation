#pragma once 

#include "core/common.h"
#include "ui/layout/SwitchLayout.hpp"
#include "ui/page/FileListPage.hpp"
#include "ui/utils/Box.hpp"


class StartPage : public beiklive::Box
{
public:
    StartPage();
    ~StartPage();

    void Init();


private:
    void _useSwitchLayout();
    void _openFileList();

    beiklive::FileListPage* m_fileListPage = nullptr;

};