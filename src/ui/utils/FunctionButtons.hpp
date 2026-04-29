#pragma once

#include "core/common.h"
#include <functional>
#include <string>
#include <vector>

namespace beiklive
{

    // 按钮1: 切换按钮 (Switch/Toggle button)
    class SwitchButton : public brls::Box
    {
    public:
        SwitchButton();

        void setText(const std::string& text);
        void setState(bool on);
        bool getState() const;
        void setOnImage(const std::string& path);
        void setOffImage(const std::string& path);
        void setOnToggle(std::function<void(bool)> callback);

    private:
        brls::Label* m_label = nullptr;
        brls::Image* m_image = nullptr;
        bool m_state = false;
        std::string m_onImagePath;
        std::string m_offImagePath;
        std::function<void(bool)> m_onToggle;

        void updateImage();
    };

    // 按钮2: 选择按钮 (Selector with left/right arrows)
    class SelectorButton : public brls::Box
    {
    public:
        SelectorButton();

        void setText(const std::string& text);
        void setOptions(const std::vector<std::string>& options, int defaultIndex = 0);
        int  getSelectedIndex() const;
        std::string getSelectedValue() const;
        void setOnSelect(std::function<void(int)> callback);

    private:
        brls::Label* m_label      = nullptr;
        brls::Label* m_arrowLeft  = nullptr;
        brls::Label* m_valueLabel = nullptr;
        brls::Label* m_arrowRight = nullptr;
        std::vector<std::string> m_options;
        int m_index = -1;
        std::function<void(int)> m_onSelect;

        void updateValue();
        void previous();
        void next();
    };

    // 按钮3: 数字按钮 (Number input with increment/decrement and dialog)
    class NumberButton : public brls::Box
    {
    public:
        NumberButton();

        void   setText(const std::string& text);
        void   setValue(double value);
        double getValue() const;
        void   setStep(double step);
        void   setDecimal(int precision);
        void   setOnChange(std::function<void(double)> callback);

    private:
        brls::Label* m_label      = nullptr;
        brls::Label* m_valueLabel = nullptr;
        double m_value     = 0.0;
        double m_step      = 1.0;
        int    m_precision = 0;
        std::function<void(double)> m_onChange;

        void updateValue();
        void increment();
        void decrement();
        void openEditDialog();
    };

} // namespace beiklive
