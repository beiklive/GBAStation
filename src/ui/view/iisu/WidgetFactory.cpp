#include "WidgetFactory.hpp"

namespace beiklive
{
    std::shared_ptr<Widget> WidgetFactory::create(WidgetType type)
    {
        // TODO: 后续实现 Image/GameCover/Folder/Live/Gif 组件
        switch (type) {
            case WidgetType::Image:    break;
            case WidgetType::GameCover: break;
            case WidgetType::Folder:   break;
            case WidgetType::Live:     break;
            case WidgetType::Gif:      break;
            case WidgetType::Empty:
            default: break;
        }
        return nullptr;
    }
} // namespace beiklive
