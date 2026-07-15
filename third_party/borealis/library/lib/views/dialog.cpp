/*
    Copyright 2019-2021 natinusala
    Copyright 2021 XITRIX

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <borealis/core/application.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/thread.hpp>
#include <borealis/core/util.hpp>
#include <borealis/views/dialog.hpp>
#include <borealis/views/label.hpp>

#include <algorithm>
#include <cmath>

using namespace brls::literals;

// TODO: different open animation?

namespace brls
{

const std::string dialogXML = R"xml(
    <brls:Box
        width="auto"
        height="auto"
        axis="column"
        justifyContent="center"
        alignItems="center"
        backgroundColor="@theme/brls/backdrop">

        <brls:AppletFrame
            id="brls/dialog/applet"
            width="760"
            height="auto"
            headerHidden="true"
            footerHidden="true"
            cornerRadius="8"
            backgroundColor="@theme/brls/background">

            <brls:Box
                width="auto"
                height="auto"
                grow="1"
                axis="column">

                <brls:Box
                    id="brls/dialog/container"
                    width="auto"
                    height="auto"
                    grow="1"
                    axis="column"/>

                <brls:Rectangle
                    id="brls/dialog/button3/separator"
                    width="auto"
                    height="2"
                    color="@theme/brls/sidebar/separator"
                    visibility="gone" />

                <brls:Button
                    id="brls/dialog/button3"
                    width="auto"
                    height="72"
                    axis="column"
                    focusable="true"
                    justifyContent="center"
                    alignItems="center"
                    highlightCornerRadius="6"
                    fontSize="@style/brls/dialog/fontSize"
                    style="borderless"
                    textColor="@theme/brls/accent"
                    text="Continue"
                    visibility="gone"/>

                <brls:Box
                    width="auto"
                    height="72"
                    axis="row"
                    justifyContent="spaceEvenly"
                    alignItems="stretch"
                    lineTop="2px"
                    lineColor="@theme/brls/sidebar/separator"
                    visibility="gone">

                    <brls:Button
                        id="brls/dialog/button1"
                        width="0"
                        height="auto"
                        grow="1"
                        focusable="true"
                        justifyContent="center"
                        alignItems="center"
                        highlightCornerRadius="6"
                        text="Continue"
                        style="borderless"
                        fontSize="@style/brls/dialog/fontSize"
                        textColor="@theme/brls/accent"
                        visibility="gone"/>

                    <brls:Rectangle
                        id="brls/dialog/button2/separator"
                        width="2"
                        height="auto"
                        color="@theme/brls/sidebar/separator"
                        visibility="gone" />

                    <brls:Button
                        id="brls/dialog/button2"
                        width="0"
                        height="auto"
                        grow="1"
                        focusable="true"
                        justifyContent="center"
                        alignItems="center"
                        highlightCornerRadius="6"
                        text="Continue"
                        style="borderless"
                        fontSize="@style/brls/dialog/fontSize"
                        textColor="@theme/brls/accent"
                        visibility="gone"/>

                </brls:Box>
            
            </brls:Box>

        </brls:AppletFrame>

    </brls:Box>
)xml";

Dialog::Dialog(Box* contentView)
{
    this->inflateFromXMLString(dialogXML);
    this->applyModernStyle();
    container->addView(contentView);

    appletFrame->registerAction(
        "hints/back"_i18n, BUTTON_B, [this](View* view) {
            if (cancelable)
                this->dismiss();
            return cancelable;
        },
        false, false, SOUND_BACK);
}

Dialog::Dialog(std::string text)
{
    Style style = Application::getStyle();

    Label* label = new Label();
    label->setText(text);
    label->setFontSize(style["brls/dialog/fontSize"]);
    label->setHorizontalAlign(HorizontalAlign::CENTER);
    label->setSingleLine(false);

    Box* box = new Box();
    box->addView(label);
    box->setAlignItems(AlignItems::CENTER);
    box->setJustifyContent(JustifyContent::CENTER);
    box->setPadding(style["brls/dialog/paddingTopBottom"], style["brls/dialog/paddingLeftRight"], style["brls/dialog/paddingTopBottom"], style["brls/dialog/paddingLeftRight"]);

    this->inflateFromXMLString(dialogXML);
    this->applyModernStyle();
    container->addView(box);

    appletFrame->registerAction(
        "hints/back"_i18n, BUTTON_B, [this](View* view) {
            if (cancelable)
                this->dismiss();
            return cancelable;
        },
        false, false, SOUND_BACK);
}

void Dialog::addButton(std::string label, VoidEvent::Callback cb)
{
    if (this->buttons.size() >= 3)
        return;

    DialogButton* button = new DialogButton();
    button->label        = label;
    button->cb           = cb;

    this->buttons.push_back(button);

    this->rebuildButtons();
}

void Dialog::clearButtons()
{
    for (auto& button : this->buttons)
    {
        delete button;
    }
    this->buttons.clear();
    this->rebuildButtons();
}

void Dialog::open()
{
    this->openTimeUsec = getCPUTimeUsec();
    this->actionPending = false;
    Application::pushActivity(new Activity(this));
}

void Dialog::close(std::function<void(void)> cb)
{
    Box::dismiss(cb);
}

void Dialog::setCancelable(bool cancelable)
{
    this->cancelable = cancelable;
}

void Dialog::rebuildButtons()
{
    if (this->buttons.size() > 0)
    {
        setLastFocusedView(button1);
        button1->getParent()->setVisibility(Visibility::VISIBLE);

        button1->setVisibility(Visibility::VISIBLE);
        button1->setText(buttons[0]->label);
        button1->setFontSize(20.0f);
        button1->registerClickAction([this](View* view) {
            buttonClick(buttons[0]);
            return true;
        });
    }

    if (this->buttons.size() > 1)
    {
        button2separator->setVisibility(Visibility::VISIBLE);
        button2->setVisibility(Visibility::VISIBLE);
        button2->setText(buttons[1]->label);
        button2->setFontSize(20.0f);
        button2->registerClickAction([this](View* view) {
            buttonClick(buttons[1]);
            return true;
        });
    }

    if (this->buttons.size() > 2)
    {
        button3separator->setVisibility(Visibility::VISIBLE);
        button3->setVisibility(Visibility::VISIBLE);
        button3->setText(buttons[2]->label);
        button3->setFontSize(20.0f);
        button3->registerClickAction([this](View* view) {
            buttonClick(buttons[2]);
            return true;
        });
    }

    button2separator->setVisibility(Visibility::GONE);
    button3separator->setVisibility(Visibility::GONE);
}

void Dialog::buttonClick(DialogButton* button)
{
    if (this->actionPending)
        return;
    this->actionPending = true;
    if (auto* focused = dynamic_cast<Button*>(Application::getCurrentFocus()))
        focused->playClickAnimation(false, true, true);
    Application::blockInputs();
    brls::delay(105, [this, button]() {
        Application::unblockInputs();
        dismiss([button] {
            button->cb();
        });
    });
}

void Dialog::applyModernStyle()
{
    this->setBackgroundColor(RGBA(0, 0, 0, 190));
    this->appletFrame->setBackground(ViewBackground::NONE);
    this->appletFrame->setCornerRadius(8.0f);
    this->appletFrame->setBorderThickness(0.0f);
    this->appletFrame->setShadowType(ShadowType::NONE);

    Button* dialogButtons[] = {this->button1, this->button2, this->button3};
    for (Button* button : dialogButtons)
    {
        button->setStyle(&BUTTONSTYLE_BORDERLESS);
        button->setBackground(ViewBackground::NONE);
        button->setBorderThickness(0.0f);
        button->setCornerRadius(7.0f);
        button->setHighlightCornerRadius(7.0f);
        button->setHideHighlight(true);
        button->setShadowType(ShadowType::NONE);
        button->setTextColor(RGBA(234, 240, 248, 235));
    }
    this->button2separator->setVisibility(Visibility::GONE);
    this->button3separator->setVisibility(Visibility::GONE);
}

void Dialog::drawButtonSurface(NVGcontext* vg, Button* button, bool focused,
                               float animationTime)
{
    if (!button || button->getVisibility() != Visibility::VISIBLE)
        return;
    Rect frame = button->getFrame();
    const float insetX = 8.0f;
    const float insetY = 7.0f;
    const float bx = frame.getMinX() + insetX;
    const float by = frame.getMinY() + insetY;
    const float bw = std::max(1.0f, frame.getWidth() - insetX * 2.0f);
    const float bh = std::max(1.0f, frame.getHeight() - insetY * 2.0f);

    NVGpaint shadow = nvgBoxGradient(vg, bx + 4.0f, by + 5.0f, bw, bh,
        7.0f, 5.0f, RGBA(0, 0, 0, focused ? 85 : 52), TRANSPARENT);
    nvgBeginPath(vg);
    nvgRect(vg, bx - 3.0f, by - 3.0f, bw + 14.0f, bh + 15.0f);
    nvgRoundedRect(vg, bx, by, bw, bh, 7.0f);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, shadow);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, bx, by, bw, bh, 7.0f);
    nvgFillColor(vg, focused ? RGBA(79, 193, 255, 34) : RGBA(255, 255, 255, 8));
    nvgFill(vg);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, bx + 1.0f, by + 1.0f, bw - 2.0f, bh - 2.0f, 6.0f);
    nvgStrokeColor(vg, focused ? RGBA(119, 211, 255, 125) : RGBA(255, 255, 255, 34));
    nvgStrokeWidth(vg, 1.0f);
    nvgStroke(vg);

    if (focused)
    {
        const float phase = std::fmod(animationTime, 1.0f);
        const float wave = 0.5f + 0.5f * std::sin(phase * 6.2831853f);
        NVGpaint border = nvgLinearGradient(vg,
            bx + bw * wave, by,
            bx + bw * (1.0f - wave), by + bh,
            RGBA(79, 193, 255, 255), RGBA(255, 132, 189, 245));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bx, by, bw, bh, 7.0f);
        nvgStrokePaint(vg, border);
        nvgStrokeWidth(vg, 3.0f);
        nvgStroke(vg);
    }
}

void Dialog::draw(NVGcontext* vg, float x, float y, float width, float height,
                  Style style, FrameContext* ctx)
{
    const uint64_t now = getCPUTimeUsec();
    const float elapsed = this->openTimeUsec == 0
        ? 1.0f : static_cast<float>(now - this->openTimeUsec) / 1000000.0f;
    const float raw = std::max(0.0f, std::min(1.0f, elapsed * 4.8f));
    const float eased = 1.0f - std::pow(1.0f - raw, 3.0f);
    const float animationTime = static_cast<float>(now % 1800000ULL) / 1800000.0f;

    Rect panel = this->appletFrame->getFrame();
    const float centerX = panel.getMinX() + panel.getWidth() * 0.5f;
    const float centerY = panel.getMinY() + panel.getHeight() * 0.5f;
    const float scale = 0.92f + eased * 0.08f;

    nvgSave(vg);
    nvgGlobalAlpha(vg, raw);
    nvgTranslate(vg, centerX, centerY + (1.0f - eased) * 28.0f);
    nvgScale(vg, scale, scale);
    nvgTranslate(vg, -centerX, -centerY);

    NVGpaint panelShadow = nvgBoxGradient(vg,
        panel.getMinX() + 6.0f, panel.getMinY() + 8.0f,
        panel.getWidth(), panel.getHeight(), 8.0f, 6.0f,
        RGBA(0, 0, 0, 140), TRANSPARENT);
    nvgBeginPath(vg);
    nvgRect(vg, panel.getMinX() - 4.0f, panel.getMinY() - 4.0f,
        panel.getWidth() + 20.0f, panel.getHeight() + 22.0f);
    nvgRoundedRect(vg, panel.getMinX(), panel.getMinY(),
        panel.getWidth(), panel.getHeight(), 8.0f);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, panelShadow);
    nvgFill(vg);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, panel.getMinX(), panel.getMinY(),
        panel.getWidth(), panel.getHeight(), 8.0f);
    nvgFillColor(vg, RGBA(22, 27, 34, 242));
    nvgFill(vg);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, panel.getMinX() + 1.0f, panel.getMinY() + 1.0f,
        panel.getWidth() - 2.0f, panel.getHeight() - 2.0f, 7.0f);
    nvgStrokeColor(vg, RGBA(255, 255, 255, 52));
    nvgStrokeWidth(vg, 1.5f);
    nvgStroke(vg);

    this->drawButtonSurface(vg, this->button1,
        Application::getCurrentFocus() == this->button1, animationTime);
    this->drawButtonSurface(vg, this->button2,
        Application::getCurrentFocus() == this->button2, animationTime);
    this->drawButtonSurface(vg, this->button3,
        Application::getCurrentFocus() == this->button3, animationTime);

    Box::draw(vg, x, y, width, height, style, ctx);
    nvgRestore(vg);
    if (raw < 1.0f || this->isChildFocused())
        this->invalidate();
}

AppletFrame* Dialog::getAppletFrame()
{
    return appletFrame;
}

Dialog::~Dialog()
{
    for(auto& i: this->buttons){
        delete i;
    }
}

} // namespace brls
