#include "ui/utils/FunctionButtons.hpp"
#include <sstream>
#include <iomanip>

namespace beiklive
{

// ============================================================
// SwitchButton 切换按钮
// ============================================================
SwitchButton::SwitchButton()
    : brls::Box(brls::Axis::ROW)
{
    this->setAlignItems(brls::AlignItems::CENTER);
    this->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
    this->setFocusable(true);
    this->setHideHighlightBackground(true);
    this->setHeight(60.f);
    this->setPadding(16.f, 24.f, 16.f, 24.f);

    m_onImagePath  = BK_RES("img/ui/b_on.png");
    m_offImagePath = BK_RES("img/ui/b_off.png");

    m_label = new brls::Label();
    m_label->setFontSize(20.f);
    m_label->setTextColor(GET_THEME_COLOR("brls/text"));
    m_label->setFocusable(false);

    m_image = new brls::Image();
    m_image->setWidth(50.f);
    m_image->setHeight(50.f);
    m_image->setScalingType(brls::ImageScalingType::FIT);
    m_image->setInterpolation(brls::ImageInterpolation::LINEAR);
    m_image->setFocusable(false);
    updateImage();

    this->addView(m_label);
    this->addView(new brls::Padding());
    this->addView(m_image);

    this->registerClickAction([this](brls::View*) -> bool {
        m_state = !m_state;
        updateImage();
        if (m_onToggle)
            m_onToggle(m_state);
        return true;
    });
}

void SwitchButton::setText(const std::string& text) { m_label->setText(text); }
void SwitchButton::setState(bool on) { m_state = on; updateImage(); }
bool SwitchButton::getState() const { return m_state; }
void SwitchButton::setOnImage(const std::string& path) { m_onImagePath = path; updateImage(); }
void SwitchButton::setOffImage(const std::string& path) { m_offImagePath = path; updateImage(); }
void SwitchButton::setOnToggle(std::function<void(bool)> callback) { m_onToggle = std::move(callback); }

void SwitchButton::updateImage()
{
    m_image->setImageFromFile(m_state ? m_onImagePath : m_offImagePath);
}

// ============================================================
// SelectorButton 选择按钮
// ============================================================
SelectorButton::SelectorButton()
    : brls::Box(brls::Axis::ROW)
{
    this->setAlignItems(brls::AlignItems::CENTER);
    this->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
    this->setFocusable(true);
    this->setHideHighlightBackground(true);
    this->setHeight(60.f);
    this->setPadding(16.f, 24.f, 16.f, 24.f);

    m_label = new brls::Label();
    m_label->setFontSize(20.f);
    m_label->setTextColor(GET_THEME_COLOR("brls/text"));
    m_label->setFocusable(false);

    auto* rightBox = new brls::Box(brls::Axis::ROW);
    rightBox->setAlignItems(brls::AlignItems::CENTER);
    rightBox->setFocusable(false);

    m_arrowLeft = new brls::Label();
    m_arrowLeft->setFontSize(22.f);
    m_arrowLeft->setTextColor(GET_THEME_COLOR("brls/text"));
    m_arrowLeft->setText("\u25C0");
    m_arrowLeft->setFocusable(false);
    m_arrowLeft->setMarginRight(8.f);

    m_valueLabel = new brls::Label();
    m_valueLabel->setFontSize(20.f);
    m_valueLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_valueLabel->setFocusable(false);
    m_valueLabel->setWidth(160.f);
    m_valueLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);

    m_arrowRight = new brls::Label();
    m_arrowRight->setFontSize(22.f);
    m_arrowRight->setTextColor(GET_THEME_COLOR("brls/text"));
    m_arrowRight->setText("\u25B6");
    m_arrowRight->setFocusable(false);
    m_arrowRight->setMarginLeft(8.f);

    rightBox->addView(m_arrowLeft);
    rightBox->addView(m_valueLabel);
    rightBox->addView(m_arrowRight);

    this->addView(m_label);
    this->addView(new brls::Padding());
    this->addView(rightBox);

    this->registerAction(
        "", brls::BUTTON_LEFT, [this](brls::View*) -> bool {
            previous();
            return true;
        }, false, false, brls::SOUND_CLICK);

    this->registerAction(
        "", brls::BUTTON_RIGHT, [this](brls::View*) -> bool {
            next();
            return true;
        }, false, false, brls::SOUND_CLICK);
}

void SelectorButton::setText(const std::string& text) { m_label->setText(text); }
int  SelectorButton::getSelectedIndex() const { return m_index; }

std::string SelectorButton::getSelectedValue() const
{
    if (m_index >= 0 && m_index < (int)m_options.size())
        return m_options[m_index];
    return "";
}

void SelectorButton::setOnSelect(std::function<void(int)> callback) { m_onSelect = std::move(callback); }

void SelectorButton::setOptions(const std::vector<std::string>& options, int defaultIndex)
{
    m_options = options;
    if (options.empty())
    {
        m_index = -1;
    }
    else
    {
        m_index = (defaultIndex >= 0 && defaultIndex < (int)options.size())
                      ? defaultIndex
                      : 0;
    }
    updateValue();
}

void SelectorButton::updateValue()
{
    if (m_index >= 0 && m_index < (int)m_options.size())
        m_valueLabel->setText(m_options[m_index]);
    else
        m_valueLabel->setText("");
}

void SelectorButton::previous()
{
    if (m_options.empty()) return;
    m_index = (m_index - 1 + (int)m_options.size()) % (int)m_options.size();
    updateValue();
    if (m_onSelect) m_onSelect(m_index);
}

void SelectorButton::next()
{
    if (m_options.empty()) return;
    m_index = (m_index + 1) % (int)m_options.size();
    updateValue();
    if (m_onSelect) m_onSelect(m_index);
}

// ============================================================
// NumberButton 数字按钮
// ============================================================
NumberButton::NumberButton()
    : brls::Box(brls::Axis::ROW)
{
    this->setAlignItems(brls::AlignItems::CENTER);
    this->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
    this->setFocusable(true);
    this->setHideHighlightBackground(true);
    this->setHeight(60.f);
    this->setPadding(16.f, 24.f, 16.f, 24.f);

    m_label = new brls::Label();
    m_label->setFontSize(20.f);
    m_label->setTextColor(GET_THEME_COLOR("brls/text"));
    m_label->setFocusable(false);

    m_valueLabel = new brls::Label();
    m_valueLabel->setFontSize(20.f);
    m_valueLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_valueLabel->setFocusable(false);
    m_valueLabel->setHorizontalAlign(brls::HorizontalAlign::RIGHT);
    updateValue();

    this->addView(m_label);
    this->addView(new brls::Padding());
    this->addView(m_valueLabel);

    this->registerClickAction([this](brls::View*) -> bool {
        openEditDialog();
        return true;
    });
}

void NumberButton::frame(brls::FrameContext* ctx)
{
    brls::Box::frame(ctx);

    if (!this->isFocused())
    {
        m_lbHeld = false;
        m_rbHeld = false;
        return;
    }

    auto now   = std::chrono::steady_clock::now();
    auto state = brls::Application::getControllerState();
    int  lbIdx = static_cast<int>(brls::BUTTON_LB);
    int  rbIdx = static_cast<int>(brls::BUTTON_RB);

    // 左肩键 (LB) — 递减
    if (lbIdx >= 0 && lbIdx < static_cast<int>(brls::_BUTTON_MAX) && state.buttons[lbIdx])
    {
        if (!m_lbHeld)
        {
            m_lbHeld     = true;
            m_lbHoldStart = now;
            m_lbLastStep  = now;
            decrement();
        }
        else
        {
            double holdElapsed  = std::chrono::duration<double>(now - m_lbHoldStart).count();
            double stepInterval = getStepInterval(holdElapsed);
            double sinceStep    = std::chrono::duration<double>(now - m_lbLastStep).count();
            if (sinceStep >= stepInterval)
            {
                decrement();
                m_lbLastStep = now;
            }
        }
    }
    else
    {
        m_lbHeld = false;
    }

    // 右肩键 (RB) — 递增
    if (rbIdx >= 0 && rbIdx < static_cast<int>(brls::_BUTTON_MAX) && state.buttons[rbIdx])
    {
        if (!m_rbHeld)
        {
            m_rbHeld     = true;
            m_rbHoldStart = now;
            m_rbLastStep  = now;
            increment();
        }
        else
        {
            double holdElapsed  = std::chrono::duration<double>(now - m_rbHoldStart).count();
            double stepInterval = getStepInterval(holdElapsed);
            double sinceStep    = std::chrono::duration<double>(now - m_rbLastStep).count();
            if (sinceStep >= stepInterval)
            {
                increment();
                m_rbLastStep = now;
            }
        }
    }
    else
    {
        m_rbHeld = false;
    }
}

void NumberButton::onFocusLost()
{
    brls::Box::onFocusLost();
    m_lbHeld = false;
    m_rbHeld = false;
}

double NumberButton::getStepInterval(double holdSeconds)
{
    if (holdSeconds < 0.5)  return 0.30;
    if (holdSeconds > 2.0)  return 0.05;
    return 0.30 - (holdSeconds - 0.5) / 1.5 * 0.25;
}

void NumberButton::setText(const std::string& text) { m_label->setText(text); }
double NumberButton::getValue() const { return m_value; }
void NumberButton::setStep(double step) { m_step = step; }
void NumberButton::setDecimal(int precision) { m_precision = precision; updateValue(); }
void NumberButton::setOnChange(std::function<void(double)> callback) { m_onChange = std::move(callback); }

void NumberButton::setValue(double value)
{
    m_value = value;
    updateValue();
    if (m_onChange) m_onChange(m_value);
}

void NumberButton::updateValue()
{
    std::ostringstream oss;
    if (m_precision < 0)
    {
        oss << (long)m_value;
    }
    else
    {
        oss << std::fixed << std::setprecision(m_precision) << m_value;
    }
    m_valueLabel->setText(oss.str());
}

void NumberButton::increment()
{
    setValue(m_value + m_step);
}

void NumberButton::decrement()
{
    setValue(m_value - m_step);
}

void NumberButton::openEditDialog()
{
    auto* ime = brls::Application::getImeManager();
    if (!ime) return;

    std::string initialText;
    if (m_precision < 0)
        initialText = std::to_string((long)m_value);
    else
        initialText = std::to_string(m_value);

    ime->openForText(
        [this](std::string text) {
            try
            {
                double val = std::stod(text);
                setValue(val);
            }
            catch (...)
            {
            }
        },
        m_label->getFullText(),
        "",
        32,
        initialText,
        brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
}

} // namespace beiklive
