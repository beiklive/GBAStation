#include "ErrorDialog.h"

#include "Gfx.h"
#include "BoxGui.h"
#include "Style.h"
#include "KeyExplanations.h"

namespace ErrorDialog
{

void Open(const std::string& message)
{
    struct Dialog
    {
        std::string Message;

        double StartTimestamp;
        double EndTimestamp = -INFINITY;

        bool operator()(BoxGui::Frame& rootFrame)
        {
            Gfx::Color color = DarkColor;
            // fade in
            color.A = (float)std::min((Gfx::AnimationTimestamp - StartTimestamp) * 5.0, 0.8);
            Gfx::DrawRectangle(rootFrame.Area.Position, rootFrame.Area.Size, color);

            Gfx::Vector2f Size = rootFrame.Area.Size * 0.5f;
            Size.X = std::max(Size.X, 720.f);
            BoxGui::Frame dialogFrame{rootFrame, rootFrame.Area.CenteredChild(Size)};

            Gfx::DrawRectangle(dialogFrame.Area.Position, dialogFrame.Area.Size, WidgetColorBright, true);

            BoxGui::Skewer vskewer{dialogFrame, 0.f, BoxGui::direction_Vertical};
            vskewer.Advance(15.f);

            BoxGui::Frame messageFrame{dialogFrame, vskewer.Spit({dialogFrame.Area.Size.X, UIRowHeight}, Gfx::align_Right), {15.f, 15.f}, {15.f, 15.f}};
            Gfx::DrawText(Gfx::SystemFontStandard,
                messageFrame.Area.Position, TextLineHeight,
                DarkColor,
                Gfx::align_Left, Gfx::align_Left,
                Message.c_str());
            
            vskewer.Advance(50.f);

            BoxGui::Frame okFrame{dialogFrame, vskewer.Spit({dialogFrame.Area.Size.X, UIRowHeight}, Gfx::align_Right), {15.f, 15.f}, {15.f, 15.f}};
            // I know that's cheating, but we don't need an InputElement if there's only one element
            Gfx::DrawRectangle(okFrame.Area.Position, okFrame.Area.Size, WidgetColorVibrant);
            Gfx::DrawText(Gfx::SystemFontStandard,
                okFrame.Area.Position + Gfx::Vector2f{15.f, okFrame.Area.Size.Y/2}, TextLineHeight,
                DarkColor,
                Gfx::align_Left, Gfx::align_Center,
                "Ok");
            
            KeyExplanation::Explain(KeyExplanation::button_A, "Ok");

            const double fadeoutLength = 0.25;
            if (BoxGui::ConfirmPressed())
                EndTimestamp = 0.f;
            return EndTimestamp < 0.0 || Gfx::AnimationTimestamp - EndTimestamp < fadeoutLength;
        }
    };
    BoxGui::OpenModalDialog(Dialog{message});
}

}