#pragma once
#include <switch.h>
#include <unordered_map>
#include <functional>

namespace InputConfig {

    inline u64 ButtonA = HidNpadButton_A;
    inline u64 ButtonB = HidNpadButton_B;
    inline u64 ButtonX = HidNpadButton_X;
    inline u64 ButtonY = HidNpadButton_Y;
    inline u64 ButtonStickL= HidNpadButton_StickL;
    inline u64 ButtonStickR = HidNpadButton_StickR;
    inline u64 ButtonL = HidNpadButton_L;
    inline u64 ButtonR = HidNpadButton_R;
    inline u64 ButtonZL = HidNpadButton_ZL;
    inline u64 ButtonZR = HidNpadButton_ZR;
    inline u64 ButtonStart = HidNpadButton_Plus;
    inline u64 ButtonSelect = HidNpadButton_Minus;
    inline u64 ButtonUp = HidNpadButton_Up;
    inline u64 ButtonDown = HidNpadButton_Down;
    inline u64 ButtonLeft = HidNpadButton_Left;
    inline u64 ButtonRight = HidNpadButton_Right;
    inline u64 ButtonStickLLeft = HidNpadButton_StickLLeft;
    inline u64 ButtonStickLUp = HidNpadButton_StickLUp;
    inline u64 ButtonStickLRight = HidNpadButton_StickLRight;
    inline u64 ButtonStickLDown = HidNpadButton_StickLDown;
    inline u64 ButtonStickRLeft = HidNpadButton_StickRLeft;
    inline u64 ButtonStickRUp = HidNpadButton_StickRUp;
    inline u64 ButtonStickRRight = HidNpadButton_StickRRight;
    inline u64 ButtonStickRDown = HidNpadButton_StickRDown;
    inline u64 ButtonLeftSL = HidNpadButton_LeftSL;
    inline u64 ButtonLeftSR = HidNpadButton_LeftSR;
    inline u64 ButtonRightSL = HidNpadButton_RightSL;
    inline u64 ButtonRightSR = HidNpadButton_RightSR;

    // Hotkeys Buttons Combination

    inline u64 Pause = (HidNpadButton_ZL|HidNpadButton_ZR);
    inline u64 MicNoise = HidNpadButton_StickL;
    inline u64 changeScreen = HidNpadButton_StickR;
    inline u64 fastForward= HidNpadButton_ZR;


    inline void ResetToDefault() {
        ButtonA = HidNpadButton_A;
        ButtonB = HidNpadButton_B;
        ButtonX = HidNpadButton_X;
        ButtonY = HidNpadButton_Y;
        ButtonStart = HidNpadButton_Plus;
        ButtonSelect = HidNpadButton_Minus;
        ButtonStickL= HidNpadButton_StickL;
        ButtonStickR = HidNpadButton_StickR;
        ButtonL = HidNpadButton_L;
        ButtonR = HidNpadButton_R;
        ButtonZL = HidNpadButton_ZL;
        ButtonZR = HidNpadButton_ZR;
        ButtonUp = HidNpadButton_Up;
        ButtonDown = HidNpadButton_Down;
        ButtonLeft = HidNpadButton_Left;
        ButtonRight = HidNpadButton_Right;
        ButtonStickLLeft = HidNpadButton_StickLLeft;
        ButtonStickLUp = HidNpadButton_StickLUp;
        ButtonStickLRight = HidNpadButton_StickLRight;
        ButtonStickLDown = HidNpadButton_StickLDown;
        ButtonStickRLeft = HidNpadButton_StickRLeft;
        ButtonStickRUp = HidNpadButton_StickRUp;
        ButtonStickRRight = HidNpadButton_StickRRight;
        ButtonStickRDown = HidNpadButton_StickRDown;
        ButtonLeftSL = HidNpadButton_LeftSL;
        ButtonLeftSR = HidNpadButton_LeftSR;
        ButtonRightSL = HidNpadButton_RightSL;
        ButtonRightSR = HidNpadButton_RightSR;
        Pause = (HidNpadButton_ZL|HidNpadButton_ZR);
        MicNoise = HidNpadButton_StickL;
        changeScreen = HidNpadButton_StickR;
        fastForward = HidNpadButton_ZR;
    }


    enum LogicalButton {
        LogicalA        = 1 << 0,
        LogicalB        = 1 << 1,
        LogicalX        = 1 << 2,
        LogicalY        = 1 << 3,
        LogicalStickL   = 1 << 4,
        LogicalStickR   = 1 << 5,
        LogicalL        = 1 << 6,
        LogicalR        = 1 << 7,
        LogicalZL       = 1 << 8,
        LogicalZR       = 1 << 9,
        LogicalStart    = 1 << 10,
        LogicalSelect   = 1 << 11,
        LogicalLeft     = 1 << 12,
        LogicalUp       = 1 << 13,
        LogicalRight    = 1 << 14,
        LogicalDown     = 1 << 15,
        LogicalStickLLeft   = 1 << 16,
        LogicalStickLUp     = 1 << 17,
        LogicalStickLRight  = 1 << 18,
        LogicalStickLDown   = 1 << 19,
        LogicalStickRLeft   = 1 << 20,
        LogicalStickRUp     = 1 << 21,
        LogicalStickRRight  = 1 << 22,
        LogicalStickRDown   = 1 << 23,
        LogicalLeftSL       = 1 << 24,
        LogicalLeftSR       = 1 << 25,
        LogicalRightSL      = 1 << 26,
        LogicalRightSR      = 1 << 27,
    };

    inline u64 GetLogicalKeysDown(u64 physicalKeysDown) {
        u64 logical = 0;
        if (physicalKeysDown & ButtonA) logical |= LogicalA;
        if (physicalKeysDown & ButtonB) logical |= LogicalB;
        if (physicalKeysDown & ButtonX) logical |= LogicalX;
        if (physicalKeysDown & ButtonY) logical |= LogicalY;

        if (physicalKeysDown & ButtonStickL) logical |= LogicalStickL;
        if (physicalKeysDown & ButtonStickR) logical |= LogicalStickR;

        if (physicalKeysDown & ButtonL) logical |= LogicalL;
        if (physicalKeysDown & ButtonR) logical |= LogicalR;
        if (physicalKeysDown & ButtonZL) logical |= LogicalZL;
        if (physicalKeysDown & ButtonZR) logical |= LogicalZR;

        if (physicalKeysDown & ButtonStart) logical |= LogicalStart;
        if (physicalKeysDown & ButtonSelect) logical |= LogicalSelect;
        
        if (physicalKeysDown & ButtonUp) logical |= LogicalUp;
        if (physicalKeysDown & ButtonDown) logical |= LogicalDown;
        if (physicalKeysDown & ButtonLeft) logical |= LogicalLeft;
        if (physicalKeysDown & ButtonRight) logical |= LogicalRight;

        if (physicalKeysDown & ButtonStickLLeft) logical |= LogicalStickLLeft;
        if (physicalKeysDown & ButtonStickLUp) logical |= LogicalStickLUp;
        if (physicalKeysDown & ButtonStickLRight) logical |= LogicalStickLRight;
        if (physicalKeysDown & ButtonStickLDown) logical |= LogicalStickLDown;

        if (physicalKeysDown & ButtonStickRLeft) logical |= LogicalStickRLeft;
        if (physicalKeysDown & ButtonStickRUp) logical |= LogicalStickRUp;
        if (physicalKeysDown & ButtonStickRRight) logical |= LogicalStickRRight;
        if (physicalKeysDown & ButtonStickRDown) logical |= LogicalStickRDown;

        if (physicalKeysDown & ButtonLeftSL) logical |= LogicalLeftSL;
        if (physicalKeysDown & ButtonLeftSR) logical |= LogicalLeftSR;
        if (physicalKeysDown & ButtonRightSL) logical |= LogicalRightSL;
        if (physicalKeysDown & ButtonRightSR) logical |= LogicalRightSR;

        return logical;
    }

    using LogicalAction = LogicalButton;

    u64 getPhysicalKeysFromLogical(u64 logicalKeys);

    void saveMappingToFile(const char* path);
    void loadMappingFromFile(const char* path);



    extern std::unordered_map<LogicalAction, std::function<void()>> actionMap;
    void setupInputActions();
}
