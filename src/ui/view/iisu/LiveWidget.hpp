#pragma once

#include <memory>
#include <string>

#include "LiveDataProvider.hpp"
#include "Widget.hpp"

namespace beiklive
{
    /// 动态组件基类：持有 widgetId 与实时数据提供者，不直接写业务
    class LiveWidget : public Widget
    {
    public:
        explicit LiveWidget(std::string widgetId);

        const std::string& widgetId() const { return m_widgetId; }

        void update(float delta) override;

        std::string typeName() const override { return "live"; }
        std::string dataId() const override { return m_widgetId; }

    protected:
        std::string m_widgetId;
        std::shared_ptr<LiveDataProvider> m_provider;
    };
} // namespace beiklive
