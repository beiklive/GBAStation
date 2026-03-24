#include "Frame.h"

beiklive::Frame::Frame()
{
    // 隐藏默认的 header 和 footer，使用自定义布局
    setHeaderVisibility(brls::Visibility::GONE);
    setFooterVisibility(brls::Visibility::GONE);
}

beiklive::Frame::~Frame()
{

}


void beiklive::Frame::Init()
{

}