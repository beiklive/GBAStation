#include "StartMenu.h"

#include "Style.h"
#include "KeyExplanations.h"
#include "main.h"
#include "ROMMetaDatabase.h"
#include "ErrorDialog.h"

#include "PlatformConfig.h"
#include "RetroAchievements.h"
#include "../FrontendUtil.h"

#include "stb_image/stb_image.h"

#include <string.h>

#include <string>
#include <vector>
#include <filesystem>

namespace StartMenu
{

struct LastPlayedROM
{
    int TitleIconIdx;
    std::string Path;
};
std::vector<LastPlayedROM> LastPlayedROMs;

u32 MelonLogoTexture;
u32 SavestateMask;

void Init()
{
    MelonLogoTexture = Gfx::TextureCreate(128, 128, DkImageFormat_RGBA8_Unorm);
    int w, h, c;
    u8* melonData = stbi_load("romfs:/melon_128x128.png", &w, &h, &c, 4);
    Gfx::TextureUpload(MelonLogoTexture, 0, 0, 128, 128, melonData, 128*4);

    for (int i = 0; i < 5; i++)
    {
        FILE* f = fopen(Config::LastROMPath[i], "rb");
        if (!f) continue;
        fclose(f);

        const char* romname = strrchr(Config::LastROMPath[i], '/') + 1;
        LastPlayedROMs.push_back({ROMMetaDatabase::QueryMeta(romname, Config::LastROMPath[i]), Config::LastROMPath[i]});
    }

    ROMMetaDatabase::UpdateTexture();
}

void PushLastPlayed(const std::string& newPath, int titleIconIdx)
{
    SavestateMask = 0;
    for (int i = 0; i < 8; i++)
    {
        if (Frontend::SavestateExists(i + 1))
            SavestateMask |= 1 << i;
    }

    for (u32 i = 0; i < LastPlayedROMs.size(); i++)
    {
        if (LastPlayedROMs[i].Path == newPath)
        {
            // it's already in there, move it to the top
            LastPlayedROM entry = LastPlayedROMs[i];
            LastPlayedROMs.erase(LastPlayedROMs.begin() + i);
            LastPlayedROMs.insert(LastPlayedROMs.begin(), entry);
            return;
        }
    }
    if (LastPlayedROMs.size() >= 5)
        LastPlayedROMs.pop_back();
    LastPlayedROMs.insert(LastPlayedROMs.begin(), {titleIconIdx, newPath});
}

void DeInit()
{
    for (int i = 0; i < 5; i++)
    {
        if (i >= LastPlayedROMs.size())
            strcpy(Config::LastROMPath[i], "");
        else
            strcpy(Config::LastROMPath[i], LastPlayedROMs[i].Path.c_str());
    }

    Gfx::TextureDelete(MelonLogoTexture);
}

bool SideBarEntry(BoxGui::Frame& optionsFrame, BoxGui::Skewer& optionSkewer, const char* name, bool last = false)
{
    BoxGui::Frame buttonFrame{optionsFrame, optionSkewer.Spit({optionsFrame.Area.Size.X, UIRowHeight}, Gfx::align_Right),
        {0.f, 5.f}, {0.f, 5.f}};

    bool selected = BoxGui::InputElement(buttonFrame, BoxGui::MakeUniqueName("sidebar", (u64)name));

    if (selected)
    {
        Gfx::DrawRectangle(buttonFrame.Area.Position, buttonFrame.Area.Size, WidgetColorVibrant);
        KeyExplanation::Explain(KeyExplanation::button_A, "Select");
    }

    BoxGui::Skewer buttonSkewer{buttonFrame, buttonFrame.Area.Size.Y/2.f, BoxGui::direction_Horizontal};
    buttonSkewer.AlignLeft(20.f);
    Gfx::DrawText(Gfx::SystemFontStandard, buttonSkewer.CurrentPosition(), TextLineHeight, DarkColor,
        Gfx::align_Left, Gfx::align_Center,
        name);

    Gfx::DrawRectangle(buttonFrame.Area.Position + Gfx::Vector2f{10.f, -(5.f + 1.f)},
        {buttonFrame.Area.Size.X - 2*10.f, 2.f},
        SeparatorColor);
    if (last)
    {
        Gfx::DrawRectangle(buttonFrame.Area.Position + Gfx::Vector2f{10.f, buttonFrame.Area.Size.Y + 5.f - 1.f},
            {buttonFrame.Area.Size.X - 2*10.f, 2.f},
            SeparatorColor);
    }

    return selected && BoxGui::ConfirmPressed();
}

void DoGui(BoxGui::Frame& parent)
{
    BoxGui::Skewer skewer{parent, 0.f, BoxGui::direction_Horizontal};

    {
        BoxGui::Frame sideBarFrame{parent, skewer.Spit({320.f, parent.Area.Size.Y}, Gfx::align_Right), {5.f, 0.f}, {5.f, 0.f}};
        Gfx::DrawRectangle(sideBarFrame.Area.Position, sideBarFrame.Area.Size, WidgetColorBright, true);

        BoxGui::Skewer sideBarSkewer{sideBarFrame, 0.f, BoxGui::direction_Vertical};

        const float spacing = UIRowHeight/2.f;
        sideBarSkewer.AlignLeft(spacing);

        if (Emulation::State == Emulation::emuState_Paused)
        {
            if (SideBarEntry(sideBarFrame, sideBarSkewer, "Continue", true))
            {
                Emulation::SetPause(false);
            }
            sideBarSkewer.Advance(spacing);
            if (SideBarEntry(sideBarFrame, sideBarSkewer, Emulation::LidClosed ? "Open lid" : "Close lid"))
            {
                Emulation::LidClosed ^= true;
                Emulation::SetPause(false);
            }
            if (SideBarEntry(sideBarFrame, sideBarSkewer, "Reset", true))
            {
                Emulation::Reset();
            }
            sideBarSkewer.Advance(spacing);
            if (isConnected() && SideBarEntry(sideBarFrame, sideBarSkewer, "RetroAchievements List")) 
            {
                CurrentUiScreen = uiScreen_RetroAchievements;   
            }
            if (SideBarEntry(sideBarFrame, sideBarSkewer, "Display settings"))
            {
                CurrentUiScreen = uiScreen_DisplaySettings;
            }
            if (SideBarEntry(sideBarFrame, sideBarSkewer, "Input settings", true))
            {
                CurrentUiScreen = uiScreen_InputSettings;
            }
            sideBarSkewer.Advance(spacing);
            if (SideBarEntry(sideBarFrame, sideBarSkewer, "Close", true))
            {
                Emulation::Stop();
                g_loadAchievements = true;
            }

            KeyExplanation::Explain(KeyExplanation::button_B, "Unpause");
            if (BoxGui::CancelPressed())
                Emulation::SetPause(false);
        }
        else
        {
            if (SideBarEntry(sideBarFrame, sideBarSkewer, "Browse", true))
            {
                CurrentUiScreen = uiScreen_BrowseROM;
            }
            if (SideBarEntry(sideBarFrame, sideBarSkewer, "Boot firmware", true))
            {
                Emulation::LoadBIOS();
            }
            sideBarSkewer.Advance(spacing);
            if (SideBarEntry(sideBarFrame, sideBarSkewer, "Emulation settings"))
            {
                CurrentUiScreen = uiScreen_EmulationSettings;
            }
            if (SideBarEntry(sideBarFrame, sideBarSkewer, "Display settings"))
            {
                CurrentUiScreen = uiScreen_DisplaySettings;
            }
            if (SideBarEntry(sideBarFrame, sideBarSkewer, "Input settings", true))
            {
                CurrentUiScreen = uiScreen_InputSettings;
            }
            sideBarSkewer.Advance(spacing);
            if (SideBarEntry(sideBarFrame, sideBarSkewer, "Exit", true))
            {
                Done = true;
            }
        }
    }

    {
        BoxGui::Frame mainFrame{parent, skewer.Spit({skewer.RemainingLength(), parent.Area.Size.Y}, Gfx::align_Right),
            {5.f, 0.f}, {5.f, 0.f}};
        Gfx::DrawRectangle(mainFrame.Area.Position, mainFrame.Area.Size, WidgetColorBright, true);

        BoxGui::Skewer vskewer{mainFrame, 0.f, BoxGui::direction_Vertical};

        if (Emulation::State == Emulation::emuState_Nothing)
        {
            BoxGui::Frame logoFrame{mainFrame, vskewer.Spit({160.f, 160.f}, Gfx::align_Right), {15.f, 15.f}, {15.f, 15.f}};
            Gfx::DrawRectangle(MelonLogoTexture, logoFrame.Area.Position, logoFrame.Area.Size, {}, {128.f, 128.f}, {1.f, 1.f, 1.f, 1.f});
            Gfx::DrawText(Gfx::SystemFontStandard,
                {logoFrame.Area.Position.X + logoFrame.Area.Size.X + 25.f,
                logoFrame.Area.Position.Y + logoFrame.Area.Size.Y / 2.f}, 
                TextLineHeight * 2.f,
                DarkColor,
                Gfx::align_Left, Gfx::align_Center,
                "melonDS");

            vskewer.Advance(20.f);

            BoxGui::Frame titleFrame{mainFrame, vskewer.Spit({0.f, TextLineHeight * 3.f}, Gfx::align_Right), {15.f, 15.f}, {15.f, 15.f}};
            Gfx::DrawText(Gfx::SystemFontStandard, titleFrame.Area.Position, TextLineHeight * 2.5f, DarkColor, "Last played...");

            vskewer.Advance(15.f);

            int selectedEntry = -1;
            for (int i = 0; i < LastPlayedROMs.size(); i++)
            {
                BoxGui::Frame entryFrame{mainFrame, vskewer.Spit({mainFrame.Area.Size.X, UIRowHeight}, Gfx::align_Right), {5.f, 5.f}, {5.f, 5.f}};

                if (BoxGui::InputElement(entryFrame, BoxGui::MakeUniqueName("lastplayed_rom", i)))
                {
                    Gfx::DrawRectangle(entryFrame.Area.Position, entryFrame.Area.Size, WidgetColorVibrant);
                    selectedEntry = i;
                }

                BoxGui::Skewer entrySkewer{entryFrame, entryFrame.Area.Size.Y / 2.f, BoxGui::direction_Horizontal};

                entrySkewer.AlignLeft(20.f);

                ROMMetaDatabase::ROMMeta& meta = ROMMetaDatabase::Database[LastPlayedROMs[i].TitleIconIdx];
                if (meta.HasIcon)
                {
                    BoxGui::Frame imageFrame{entryFrame, entrySkewer.Spit({entryFrame.Area.Size.Y * 0.8f, entryFrame.Area.Size.Y * 0.8f})};
                    Gfx::DrawRectangle(meta.Icon.AtlasTexture, 
                        imageFrame.Area.Position, imageFrame.Area.Size, 
                        {(float)meta.Icon.PackX, (float)meta.Icon.PackY}, {32.f, 32.f},
                        {1.f, 1.f, 1.f, 1.f});
                }
                entrySkewer.Advance(20.f);

                Gfx::DrawText(Gfx::SystemFontStandard,
                    entrySkewer.CurrentPosition(), TextLineHeight,
                    DarkColor,
                    Gfx::align_Left, Gfx::align_Center,
                    meta.Title(ROMMetaDatabase::TitleLanguage));

                if (i > 0)
                {
                    // draw separator
                    Gfx::DrawRectangle(entryFrame.Area.Position + Gfx::Vector2f{10.f, -(5.f + 1.f)},
                        {entryFrame.Area.Size.X - 2*10.f, 2.f},
                        SeparatorColor);
                }
            }

            if (selectedEntry != -1)
            {
                KeyExplanation::Explain(KeyExplanation::button_A, "Start");
                if (BoxGui::ConfirmPressed())
                {
                    Emulation::LoadROM(LastPlayedROMs[selectedEntry].Path.c_str());
                    PushLastPlayed(LastPlayedROMs[selectedEntry].Path, LastPlayedROMs[selectedEntry].TitleIconIdx);
                }
            }
        }
        else
        {
            BoxGui::Frame titleFrame{mainFrame, vskewer.Spit({0.f, TextLineHeight * 3.f}, Gfx::align_Right), {15.f, 15.f}, {15.f, 15.f}};
            if (!Config::hardcoreMode) {
                Gfx::DrawText(Gfx::SystemFontStandard, titleFrame.Area.Position, TextLineHeight * 2.5f, DarkColor, "Savestates");

                vskewer.Advance(20.f);
    
                BoxGui::Frame saveTitleFrame{mainFrame, vskewer.Spit({0.f, TextLineHeight * 2.f}, Gfx::align_Right), {15.f, 0.f}};
                Gfx::DrawText(Gfx::SystemFontStandard, saveTitleFrame.Area.Position, TextLineHeight*1.5f, DarkColor, "Save state");
                vskewer.Advance(10.f);
    
                BoxGui::Frame savestateFrame{mainFrame,
                    vskewer.Spit({mainFrame.Area.Size.X, TextLineHeight * 4.f}, Gfx::align_Right),
                    {0.f, 0.f}, {0.f, 0.f},
                    BoxGui::direction_Horizontal, BoxGui::MakeUniqueName("savestates", 42),
                    false, false};
                Gfx::PushScissor(savestateFrame.Area.Position.X, savestateFrame.Area.Position.Y, savestateFrame.Area.Size.X, savestateFrame.Area.Size.Y);
                BoxGui::Skewer savestateSkewer{savestateFrame, 0.f, BoxGui::direction_Horizontal};
                savestateSkewer.AlignLeft(20.f);
    
                int savestateSelected = -1;
                for (u32 i = 0; i < 8; i++)
                {
                    BoxGui::Frame savebutton{savestateFrame, savestateSkewer.Spit({TextLineHeight*4.f, TextLineHeight*4.f}, Gfx::align_Right), {5.f, 5.f}, {5.f, 5.f}};
                    Gfx::Color fontColor = SeparatorColor;
                    if (Config::ConsoleType != 1)
                    {
                        if (BoxGui::InputElement(savebutton, BoxGui::MakeUniqueName("savestate", i)))
                        {
                            savestateSelected = i;
                            Gfx::DrawRectangle(savebutton.Area.Position, savebutton.Area.Size, WidgetColorVibrant, false);
                        }
                        fontColor = DarkColor;
                    }
                    char label[2] = {'1', '\0'};
                    label[0] += i;
                    Gfx::DrawText(Gfx::SystemFontStandard,
                        savebutton.Area.Position+savebutton.Area.Size*0.5f, TextLineHeight*1.5f,
                        fontColor,
                        Gfx::align_Center, Gfx::align_Center,
                        label);
                
                    if (i > 0)
                    {
                        Gfx::DrawRectangle(savebutton.Area.Position - Gfx::Vector2f{5.f, 0.f}, {2.f, savebutton.Area.Size.Y-2.f*2.f}, SeparatorColor);
                    }
                }
                Gfx::PopScissor();
                if (savestateSelected != -1)
                {
                    KeyExplanation::Explain(KeyExplanation::button_A, "Save state");
                    if (BoxGui::ConfirmPressed())
                    {
                        char filename[512];
                        Frontend::GetSavestateName(savestateSelected + 1, filename, 512);
                        if (Frontend::SaveState(filename))
                            SavestateMask |= 1<<savestateSelected;
                        else
                            ErrorDialog::Open("Failed to create savestate");
                    }
                }
                vskewer.Advance(15.f);
    
                BoxGui::Frame loadTitleFrame{mainFrame, vskewer.Spit({0.f, TextLineHeight * 2.f}, Gfx::align_Right), {15.f, 0.f}};
                Gfx::DrawText(Gfx::SystemFontStandard, loadTitleFrame.Area.Position, TextLineHeight*1.5f, DarkColor, "Load state");
                vskewer.Advance(10.f);
    
                BoxGui::Frame loadstateFrame{mainFrame,
                    vskewer.Spit({mainFrame.Area.Size.X, TextLineHeight * 4.f}, Gfx::align_Right),
                    {0.f, 0.f}, {0.f, 0.f},
                    BoxGui::direction_Horizontal,
                    BoxGui::MakeUniqueName("savestates", 42),
                    false, false};
                Gfx::PushScissor(loadstateFrame.Area.Position.X, loadstateFrame.Area.Position.Y, loadstateFrame.Area.Size.X, loadstateFrame.Area.Size.Y);
                BoxGui::Skewer loadstateSkewer{loadstateFrame, 0.f, BoxGui::direction_Horizontal};
                loadstateSkewer.AlignLeft(20.f);
    
                int loadstateSelected = -1;
                for (u32 i = 0; i < 9; i++)
                {
                    BoxGui::Frame loadbutton{loadstateFrame, loadstateSkewer.Spit({TextLineHeight*4.f, TextLineHeight*4.f}, Gfx::align_Right), {5.f, 5.f}, {5.f, 5.f}};
    
                    Gfx::Color fontColor = SeparatorColor;
                    bool enabled = i < 8
                        ? SavestateMask & (1<<i)
                        : Frontend::SavestateLoaded;
                    if (enabled && Config::ConsoleType != 1)
                    {
                        if (BoxGui::InputElement(loadbutton, BoxGui::MakeUniqueName("loadstate", i)))
                        {
                            loadstateSelected = i;
                            Gfx::DrawRectangle(loadbutton.Area.Position, loadbutton.Area.Size, WidgetColorVibrant, false);
                        }
                        fontColor = DarkColor;
                    }
                    if (i == 8)
                    {
                        Gfx::DrawText(Gfx::SystemFontStandard,
                            loadbutton.Area.Position+loadbutton.Area.Size*0.5f, TextLineHeight,
                            fontColor,
                            Gfx::align_Center, Gfx::align_Center,
                            "Undo\nload");
                    }
                    else
                    {
                        char label[2] = {'1', '\0'};
                        label[0] += i;
                        Gfx::DrawText(Gfx::SystemFontStandard,
                            loadbutton.Area.Position+loadbutton.Area.Size*0.5f, TextLineHeight*1.5f,
                            fontColor,
                            Gfx::align_Center, Gfx::align_Center,
                            label);
                    }
    
                    if (i > 0)
                    {
                        Gfx::DrawRectangle(loadbutton.Area.Position - Gfx::Vector2f{5.f, 0.f}, {2.f, loadbutton.Area.Size.Y-2.f*2.f}, SeparatorColor);
                    }
                }
                Gfx::PopScissor();
    
                if (loadstateSelected != -1)
                {
                    KeyExplanation::Explain(KeyExplanation::button_A, "Load state");
                    if (BoxGui::ConfirmPressed())
                    {
                        bool loadedSuccessfully;
                        if (loadstateSelected < 8)
                        {
                            char filename[512];
                            Frontend::GetSavestateName(loadstateSelected + 1, filename, 512);
                            loadedSuccessfully = Frontend::LoadState(filename);
                        }
                        else
                        {
                            Frontend::UndoStateLoad();
                            loadedSuccessfully = true;
                        }
    
                        if (loadedSuccessfully)
                        {
                            Emulation::SetPause(false);
                            BoxGui::ForceSelecton(BoxGui::MakeUniqueName("sidebar", 0));
                        }
                        else
                        {
                            ErrorDialog::Open("Couldn't load savefile");
                        }
                    }
                }
            }
        }
    }
}

}