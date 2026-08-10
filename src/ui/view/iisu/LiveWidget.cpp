#include "LiveWidget.hpp"

namespace beiklive
{
    LiveWidget::LiveWidget(std::string widgetId)
        : m_widgetId(std::move(widgetId))
    {
    }

    void LiveWidget::update(float delta)
    {
        if (m_provider)
            m_provider->update(delta);
    }
} // namespace beiklive
