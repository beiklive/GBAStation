#include "KeyExplanations.h"
#include "Style.h"

#include <string>

namespace KeyExplanation
{

int Hold = 0;
u32 KeysExplained, KeysExplainedThisFrame;
std::string KeyExplanations[buttons_Count];

void Explain(int button, const char* explanation)
{
    Hold = 2;
    KeysExplainedThisFrame |= 1 << button;
    KeyExplanations[button] = explanation;
}

void DoGui(BoxGui::Frame& parent)
{
    // hack: when switching between gui states it can happen that the currently selected items is reset
    // so for one frame there's no item selected which results in a small flicker because there's no key explanation
    if (KeysExplainedThisFrame == 0)
    {
        Hold--;
        if (Hold < 0)
            KeysExplained = 0;
    }
    else
    {
        KeysExplained = KeysExplainedThisFrame;
    }
    KeysExplainedThisFrame = 0;

    if (KeysExplained == 0)
        return;

    const Gfx::Vector2f Padding = {15.f, 10.f};

    u32 i = 0;
    u32 keysExplained = KeysExplained;
    BoxGui::Skewer skewer{parent, parent.Area.Size.Y - TextLineHeight / 2.f - Padding.Y, BoxGui::direction_Horizontal};
    skewer.AlignRight(0.f);
    while (keysExplained)
    {
        int button = 31 - __builtin_clz(keysExplained);
        keysExplained &= ~(1 << button);

        const char* buttonIcon = "\uE009";
        switch (button)
        {
        case button_A: buttonIcon = GFX_NINTENDOFONT_A_BUTTON; break;
        case button_B: buttonIcon = GFX_NINTENDOFONT_B_BUTTON; break;
        case button_X: buttonIcon = GFX_NINTENDOFONT_X_BUTTON; break;
        case button_Y: buttonIcon = GFX_NINTENDOFONT_Y_BUTTON; break;
        case button_Plus: buttonIcon = GFX_NINTENDOFONT_PLUS_BUTTON; break;
        case button_Minus: buttonIcon = GFX_NINTENDOFONT_MINUS_BUTTON; break;
        default: break;
        }

        const char* explanation = KeyExplanations[button].c_str();
        Gfx::Vector2f textSize = Gfx::MeasureText(Gfx::SystemFontStandard, TextLineHeight, explanation) + Gfx::Vector2f{28.f, 0.f};
        BoxGui::Frame frame{parent, skewer.Spit(textSize + Padding * 2.f), Padding, Padding};

        Gfx::DrawRectangle(frame.UnpaddedArea().Position, frame.UnpaddedArea().Size, DarkColor, true);
        Gfx::DrawText(Gfx::SystemFontNintendoExt, frame.Area.Position, TextLineHeight, WidgetColorBright, Gfx::align_Left, Gfx::align_Left, buttonIcon);
        Gfx::DrawText(Gfx::SystemFontStandard, frame.Area.Position + Gfx::Vector2f{28.f, 0.f}, TextLineHeight, WidgetColorBright, Gfx::align_Left, Gfx::align_Left, explanation);

        i++;
    }
}

void Reset()
{
    KeysExplainedThisFrame = 0;
}

}
