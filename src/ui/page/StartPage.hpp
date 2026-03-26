#pragma once 

#include "core/common.h"
#include "UI/layout/SwitchLayout.hpp"
#include "UI/page/FileListPage.hpp"


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