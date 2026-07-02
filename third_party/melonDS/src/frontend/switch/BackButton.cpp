#include "BackButton.h"
#include "KeyExplanations.h"
#include "Style.h"
#include "main.h"

namespace BackButton
{

void DoGui(BoxGui::Frame& parent, const char* title)
{
    BoxGui::Frame backButton{parent, {{0.f, 0.f}, {BackButtonHeight, BackButtonHeight}}, {0.f, 0.f}, {5.f, 5.f}};

    bool selected = BoxGui::InputElement(backButton, BoxGui::MakeUniqueName("backbutton", 0));
    if (selected)
    {
        KeyExplanation::Explain(KeyExplanation::button_A, "Back");
        if (BoxGui::ConfirmPressed())
            GoBack();
    }

    Gfx::DrawRectangle({0.f, 0.f}, {parent.Area.Size.X, BackButtonHeight}, SeparatorColor, true);

    Gfx::DrawRectangle(backButton.Area.Position, backButton.Area.Size, selected ? WidgetColorVibrant : WidgetColorBright, !selected);
    Gfx::DrawText(Gfx::SystemFontNintendoExt, backButton.Area.Position + backButton.Area.Size * 0.5f, BackButtonHeight*0.5f, DarkColor,
        Gfx::align_Center, Gfx::align_Center, GFX_NINTENDOFONT_BACK);

    BoxGui::Frame titleText{parent, {{BackButtonHeight, 0.f}, {parent.Area.Size.X - BackButtonHeight, BackButtonHeight}}, {0.f, 0.f}, {0.f, 5.f}};
    Gfx::DrawRectangle(titleText.Area.Position, titleText.Area.Size, WidgetColorBright, true);

    Gfx::DrawText(Gfx::SystemFontStandard, titleText.Area.Position + Gfx::Vector2f{20.f, titleText.Area.Size.Y/2.f}, TextLineHeight,
        DarkColor, Gfx::align_Left, Gfx::align_Center, title);
}

void GoBack()
{
    CurrentUiScreen = uiScreen_Start;
}

}