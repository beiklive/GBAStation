#include "main.h"

#include <stdio.h>
#include <dirent.h>
#include <assert.h>
#include <malloc.h>
#include <atomic>

#include <switch.h>

#include "NDS.h"
#include "GPU.h"
#include "GPU2D_Deko.h"
#include "SPU.h"
#include "FrontendUtil.h"
#include "Config.h"
#include "Platform.h"

#include "PlatformConfig.h"

#include "Gfx.h"
#include "BoxGui.h"

#include "Style.h"
#include "Filebrowser.h"
#include "StartMenu.h"
#include "KeyExplanations.h"
#include "SettingsDialog.h"
#include "ROMMetaDatabase.h"
#include "ErrorDialog.h"

#include "mm_vec/mm_vec.h"

#include <string.h>

#include "RetroAchievements.h"
#include "NotificationSystem.h"
#include "TriggerNotification.h"
#include "RATracker.h"
#include "InputConfig.h"

bool Done = false;
int CurrentUiScreen = uiScreen_Start;

PadState Pad;

namespace Profiler
{

double Sum;

void Clear()
{
    Sum = 0.0;
}

u64 SectionStartTimestamp;

void StartSection()
{
    SectionStartTimestamp = armGetSystemTick();
}

void EndSection()
{
    Sum += armTicksToNs(armGetSystemTick() - SectionStartTimestamp) * 0.000001;
}

}

namespace Overclocking
{

bool UsePCV;
ClkrstSession Session;

void ApplyOverclock(int setting)
{
    const int clockSpeeds[] = { 1020000000, 1224000000, 1581000000, 1785000000, 918000000, 816000000, 714000000 };
    // printf("applying overclock %d\n", clockSpeeds[setting]);
    if (UsePCV)
        pcvSetClockRate(PcvModule_CpuBus, clockSpeeds[setting]);
    else
        clkrstSetClockRate(&Session, clockSpeeds[setting]);
}

void OnAppletHook(AppletHookType hook)
{
    if ((hook == AppletHookType_OnOperationMode
        || hook == AppletHookType_OnPerformanceMode
        || hook == AppletHookType_OnResume
        || hook == AppletHookType_OnExitRequest)
        && Emulation::State == Emulation::emuState_Running)
    {
        ApplyOverclock(Config::SwitchOverclock);
    }
}

void Init()
{
    UsePCV = hosversionBefore(8, 0, 0);
    if (UsePCV)
    {
        pcvInitialize();
    }
    else
    {
        clkrstInitialize();
        clkrstOpenSession(&Session, PcvModuleId_CpuBus, 0);
    }
}

void DeInit()
{
    ApplyOverclock(0);
    if (UsePCV)
    {
        pcvExit();
    }
    else
    {
        clkrstCloseSession(&Session);
        clkrstExit();
    }
}

}

namespace Emulation
{

u64 PlatformKeysHeld;

u32 FramebufferTextures[2][2];
bool NewFrameReady = false;
u32 CurrentFrontBuffer;

const int FrametimeHistogramLen = 120;
int FrametimeHistogramNextValue;
float FrametimeHistogram[FrametimeHistogramLen];

int ScreensVisible, FirstBotScreen;
Gfx::Vector2f ScreenPoints[Frontend::MaxScreenTransforms][4];
BoxGui::Rect BotScreenRect;
int ScreenKinds[Frontend::MaxScreenTransforms];

Thread AudioThread;

int State;
std::atomic<int> StateAtomic;

const int AudioFrameSize = 768 * 2 * sizeof(s16);
AudioDriver AudioDrv;
void* AudMemPool = NULL;

bool TouchHeld;
Gfx::Vector2f TouchCursorVelocity;
Gfx::Vector2f TouchCursorPosition;

bool TouchDown;
int TouchFinalPositionX, TouchFinalPositionY;
float TouchScale;

int MainScreenPosition[3];
int AutoScreenSizing, LastAutoScreenSizing;

HidSixAxisSensorHandle JoyconSixaxisHandles[2];
HidSixAxisSensorHandle ConsoleSixAxisHandle;
HidSixAxisSensorHandle FullKeySixAxisHandle;

float GyroCalibrationUp[3];
float GyroCalibrationRight[3];
float GyroCalibrationForward[3];

bool UseRealTouchscreen;

bool LidClosed;

void RecalibrateGyro(HidSixAxisSensorState& values)
{
    xv3_cpy(GyroCalibrationRight, values.direction.direction[0]);
    xv3_cpy(GyroCalibrationForward, values.direction.direction[1]);
    xv3_cpy(GyroCalibrationUp, values.direction.direction[2]);
    xv_norm(GyroCalibrationUp, GyroCalibrationUp, 3);
    xv_norm(GyroCalibrationRight, GyroCalibrationRight, 3);
    xv_norm(GyroCalibrationForward, GyroCalibrationForward, 3);
}

void AudioOutput(void *args)
{
    int state = StateAtomic;

    AudioDriverWaveBuf buffers[2];
    memset(&buffers[0], 0, sizeof(AudioDriverWaveBuf) * 2);
    for (int i = 0; i < 2; i++)
    {
        buffers[i].data_pcm16 = (s16*)AudMemPool;
        buffers[i].size = AudioFrameSize;
        buffers[i].start_sample_offset = i * AudioFrameSize / 2 / sizeof(s16);
        buffers[i].end_sample_offset = buffers[i].start_sample_offset + AudioFrameSize / 2 / sizeof(s16);
    }

    while (state != emuState_Quit)
    {
        while (state != emuState_Running && state != emuState_Quit)
        {
            svcSleepThread(17000000); // a bit more than a frame...
            state = StateAtomic;
        }
        while (state == emuState_Running)
        {
            AudioDriverWaveBuf* refillBuf = NULL;
            for (int i = 0; i < 2; i++)
            {
                if (buffers[i].state == AudioDriverWaveBufState_Free || buffers[i].state == AudioDriverWaveBufState_Done)
                {
                    refillBuf = &buffers[i];
                    break;
                }
            }

            if (refillBuf)
            {
                s16* data = (s16*)AudMemPool + refillBuf->start_sample_offset * 2;

                int nSamples = 0;
                while (state == emuState_Running && !(nSamples = SPU::ReadOutput(data, 768)))
                {
                    svcSleepThread(10000);
                    state = StateAtomic;
                }

                if (nSamples > 0)
                {
                    u32 last = ((u32*)data)[nSamples - 1];
                    while (nSamples < 768)
                        ((u32*)data)[nSamples++] = last;

                    armDCacheFlush(data, nSamples * 2 * sizeof(u16));
                    refillBuf->end_sample_offset = refillBuf->start_sample_offset + nSamples;

                    audrvVoiceAddWaveBuf(&AudioDrv, 0, refillBuf);
                    audrvVoiceStart(&AudioDrv, 0);
                }
            }

            audrvUpdate(&AudioDrv);
            audrenWaitFrame();

            state = StateAtomic;
        }
    }
}

void Init()
{
    NDS::Init();
    GPU::InitRenderer(0);
    GPU::RenderSettings settings{true, 1, false};
    GPU::SetRenderSettings(0, settings);

    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < 2; i++)
        {
            FramebufferTextures[j][i] = Gfx::TextureCreateExternal(256, 192, 
                ((GPU2D::DekoRenderer*)GPU::GPU2D_Renderer.get())->GetFramebuffer(j, i));
        }
    }

    UpdateScreenLayout();

    const AudioRendererConfig arConfig =
    {
        .output_rate     = AudioRendererOutputRate_48kHz,
        .num_voices      = 4,
        .num_effects     = 0,
        .num_sinks       = 1,
        .num_mix_objs    = 1,
        .num_mix_buffers = 2,
    };

    Result code;
    if (!R_SUCCEEDED(code = audrenInitialize(&arConfig)))
    {
        printf("audren init failed! %d\n", code);
        abort();
    }

    if (!R_SUCCEEDED(code = audrvCreate(&AudioDrv, &arConfig, 2)))
    {
        printf("audrv create failed! %d\n", code);
        abort();
    }

    const int poolSize = (AudioFrameSize * 2 + (AUDREN_MEMPOOL_ALIGNMENT-1)) & ~(AUDREN_MEMPOOL_ALIGNMENT-1);
    AudMemPool = memalign(AUDREN_MEMPOOL_ALIGNMENT, poolSize);

    int mpid = audrvMemPoolAdd(&AudioDrv, AudMemPool, poolSize);
    audrvMemPoolAttach(&AudioDrv, mpid);

    static const u8 sink_channels[] = { 0, 1 };
    audrvDeviceSinkAdd(&AudioDrv, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);

    audrvUpdate(&AudioDrv);

    if (!R_SUCCEEDED(code = audrenStartAudioRenderer()))
        printf("audrv create failed! %d\n", code);

    if (!audrvVoiceInit(&AudioDrv, 0, 2, PcmFormat_Int16, 32823)) // cheating
        printf("failed to create voice\n");

    audrvVoiceSetDestinationMix(&AudioDrv, 0, AUDREN_FINAL_MIX_ID);
    audrvVoiceSetMixFactor(&AudioDrv, 0, 1.0f, 0, 0);
    audrvVoiceSetMixFactor(&AudioDrv, 0, 1.0f, 1, 1);
    audrvVoiceStart(&AudioDrv, 0);

    threadCreate(&AudioThread, AudioOutput, nullptr, nullptr, 1024*32, 0x20, 0);
    threadStart(&AudioThread);

    hidGetSixAxisSensorHandles(JoyconSixaxisHandles, 2, HidNpadIdType_No1, HidNpadStyleTag_NpadJoyDual);
    hidGetSixAxisSensorHandles(&ConsoleSixAxisHandle, 1, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    hidGetSixAxisSensorHandles(&FullKeySixAxisHandle, 1, HidNpadIdType_No1, HidNpadStyleTag_NpadFullKey);
    hidStartSixAxisSensor(JoyconSixaxisHandles[0]);
    hidStartSixAxisSensor(JoyconSixaxisHandles[1]);
    hidStartSixAxisSensor(ConsoleSixAxisHandle);
    hidStartSixAxisSensor(FullKeySixAxisHandle);
}

void DeInit()
{
    hidStopSixAxisSensor(JoyconSixaxisHandles[0]);
    hidStopSixAxisSensor(JoyconSixaxisHandles[1]);
    hidStopSixAxisSensor(ConsoleSixAxisHandle);
    hidStopSixAxisSensor(FullKeySixAxisHandle);

    State = emuState_Quit;
    StateAtomic = State;

    threadWaitForExit(&AudioThread);
    threadClose(&AudioThread);

    GPU::DeInitRenderer();
    NDS::DeInit();

    audrvClose(&AudioDrv);
    audrenExit();

    free(AudMemPool);

    for (u32 j = 0; j < 2; j++)
        for (u32 i = 0; i < 2; i++)
            Gfx::TextureDelete(FramebufferTextures[j][i]);
}

const u32 KeyMappings[] =
{
    HidNpadButton_A,
    HidNpadButton_B,
    HidNpadButton_Minus,
    HidNpadButton_Plus,
    HidNpadButton_Right | HidNpadButton_StickLRight,
    HidNpadButton_Left | HidNpadButton_StickLLeft,
    HidNpadButton_Up | HidNpadButton_StickLUp,
    HidNpadButton_Down | HidNpadButton_StickLDown,
    HidNpadButton_R,
    HidNpadButton_L,
    HidNpadButton_X,
    HidNpadButton_Y,
};

void UpdateAndDraw(u64& keysDown, u64& keysUp)
{
    bool touchUseCursor = !UseRealTouchscreen;

    if (State == emuState_Running)
    {
        padUpdate(&Pad);

        PlatformKeysHeld |= keysDown;
        PlatformKeysHeld &= ~keysUp;

        rc_client_process_ra();

        if ((PlatformKeysHeld & InputConfig::Pause) == InputConfig::Pause)
        {
            SetPause(true);
        }
        else
        {
            u32 rotatedKeyMappings[12];
            memcpy(rotatedKeyMappings, KeyMappings, 12*4);
            switch (Config::ScreenRotation)
            {
            case 0: // nothing needs to be handled
                break;
            case 1: // 90 degrees
                rotatedKeyMappings[4] = KeyMappings[6]; // right -> up
                rotatedKeyMappings[5] = KeyMappings[7]; // left -> down
                rotatedKeyMappings[6] = KeyMappings[5]; // up -> left
                rotatedKeyMappings[7] = KeyMappings[4]; // down -> right

                rotatedKeyMappings[0] = KeyMappings[10]; // X -> A
                rotatedKeyMappings[1] = KeyMappings[0]; // A -> B
                rotatedKeyMappings[10] = KeyMappings[11]; // X -> Y
                rotatedKeyMappings[11] = KeyMappings[1]; // Y -> B
                break;
            case 2: // 180 degrees
                rotatedKeyMappings[4] = KeyMappings[5]; // right -> left
                rotatedKeyMappings[5] = KeyMappings[4]; // left -> right
                rotatedKeyMappings[6] = KeyMappings[7]; // up -> down
                rotatedKeyMappings[7] = KeyMappings[6]; // down -> up

                rotatedKeyMappings[0] = KeyMappings[11]; // Y -> A
                rotatedKeyMappings[1] = KeyMappings[10]; // X -> B
                rotatedKeyMappings[10] = KeyMappings[1]; // B -> X
                rotatedKeyMappings[11] = KeyMappings[0]; // A -> Y
                break;
            case 3: // 270 degrees
                rotatedKeyMappings[4] = KeyMappings[7]; // right -> down
                rotatedKeyMappings[5] = KeyMappings[6]; // left -> up
                rotatedKeyMappings[6] = KeyMappings[4]; // up -> right
                rotatedKeyMappings[7] = KeyMappings[5]; // down -> left

                rotatedKeyMappings[0] = KeyMappings[1]; // A -> B
                rotatedKeyMappings[1] = KeyMappings[11]; // B -> Y
                rotatedKeyMappings[10] = KeyMappings[0]; // X -> A
                rotatedKeyMappings[11] = KeyMappings[10]; // Y -> X
                break;
            }

            HidTouchScreenState touchPosition;
            if (FirstBotScreen != -1)
            {
                Gfx::Vector2f botScreenCenter = BotScreenRect.Position + BotScreenRect.Size * 0.5f;
                if (Config::TouchscreenMode < 2)
                {
                    HidAnalogStickState rstick = padGetStickPos(&Pad, 1);

                    Gfx::Vector2f rstickVec = {(float)rstick.x / JOYSTICK_MAX, -(float)rstick.y / JOYSTICK_MAX};

                    touchUseCursor = true;
                    if (Config::TouchscreenMode == 0) // mouse mode
                    {
                        if (rstickVec.LengthSqr() < 0.1f * 0.1f)
                            TouchCursorVelocity = {0.f, 0.f};
                        else
                            UseRealTouchscreen = false;

                        float maxSpeed = std::max(BotScreenRect.Size.X, BotScreenRect.Size.Y) * 1.5f;
                        TouchCursorVelocity += rstickVec * std::max(BotScreenRect.Size.X, BotScreenRect.Size.Y) * 0.125f;
                        TouchCursorVelocity = TouchCursorVelocity.Clamp({-maxSpeed, -maxSpeed}, {maxSpeed, maxSpeed});

                        // allow for quick turns
                        if ((TouchCursorVelocity.X > 0.f && rstick.x < 0) || (TouchCursorVelocity.X < 0.f && rstick.x > 0))
                            TouchCursorVelocity.X = 0.f;
                        if ((TouchCursorVelocity.Y > 0.f && rstick.y > 0) || (TouchCursorVelocity.Y < 0.f && rstick.y < 0))
                            TouchCursorVelocity.Y = 0.f;
                        TouchCursorPosition += TouchCursorVelocity * Gfx::AnimationTimestep;
                    }
                    else // offset mode
                    {
                        // project ray from screen origin with
                        Gfx::Vector2f direction = rstickVec;
                        Gfx::Vector2f origin = botScreenCenter;

                        if (rstickVec.LengthSqr() < 0.1f * 0.1f)
                        {
                            TouchCursorPosition = origin;
                        }
                        else
                        {
                            UseRealTouchscreen = false;

                            // we project the ray from the center of the bottom screen with the direction of the analog stick
                            // onto the border of the bottom screen so we can calculate the distance
                            float topX = origin.X + direction.X * ((BotScreenRect.Position.Y - origin.Y) / direction.Y);
                            float bottomX = origin.X + direction.X * ((BotScreenRect.Position.Y + BotScreenRect.Size.Y - origin.Y) / direction.Y);
                            float leftY = origin.Y + direction.Y * ((BotScreenRect.Position.X - origin.X) / direction.X);
                            float rightY = origin.Y + direction.Y * ((BotScreenRect.Position.X + BotScreenRect.Size.X - origin.X) / direction.X);

                            Gfx::Vector2f hitPoint;
                            if (topX >= BotScreenRect.Position.X && topX < BotScreenRect.Position.X + BotScreenRect.Size.X)
                                hitPoint = {topX, BotScreenRect.Position.Y};
                            else if (bottomX >= BotScreenRect.Position.X && bottomX < BotScreenRect.Position.X + BotScreenRect.Size.X)
                                hitPoint = {bottomX, BotScreenRect.Position.Y + BotScreenRect.Size.Y};
                            else if (leftY >= BotScreenRect.Position.Y && leftY < BotScreenRect.Position.Y + BotScreenRect.Size.Y)
                                hitPoint = {BotScreenRect.Position.X, leftY};
                            else if (rightY >= BotScreenRect.Position.Y && rightY < BotScreenRect.Position.Y + BotScreenRect.Size.Y)
                                hitPoint = {BotScreenRect.Position.X + BotScreenRect.Size.X, rightY};

                            TouchCursorPosition = origin + rstickVec * sqrtf((hitPoint - origin).LengthSqr());
                        }
                    }
                }
                else
                {
                    touchUseCursor = true;
                    UseRealTouchscreen = false;

                    HidSixAxisSensorState sixaxisValues;
                    u32 styleSet = padGetStyleSet(&Pad);

                    float xAngle, yAngle;
                    if (styleSet & HidNpadStyleTag_NpadHandheld)
                    {
                        hidGetSixAxisSensorStates(ConsoleSixAxisHandle, &sixaxisValues, 1);
                        float up[3];
                        xv3_cpy(up, sixaxisValues.direction.direction[2]);
                        xv_norm(up, up, 3);

                        switch (Config::GlobalRotation)
                        {
                        case 0:
                            xAngle = xv3_dot(up, GyroCalibrationRight);
                            yAngle = xv3_dot(up, GyroCalibrationForward);
                            break;
                        case 1:
                            xAngle = -(xv3_dot(up, GyroCalibrationForward));
                            yAngle = xv3_dot(up, GyroCalibrationRight);
                            break;
                        case 2:
                            xAngle = -(xv3_dot(up, GyroCalibrationRight));
                            yAngle = -(xv3_dot(up, GyroCalibrationForward));
                            break;
                        case 3:
                            xAngle = xv3_dot(up, GyroCalibrationForward);
                            yAngle = -(xv3_dot(up, GyroCalibrationRight));
                            break;
                        }
                    }
                    else
                    {
                        HidSixAxisSensorHandle handle;
                        int preferedPad = Config::LeftHandedMode ? 0 : 1;
                        u32 preferedPadMask = Config::LeftHandedMode ? HidNpadAttribute_IsLeftConnected : HidNpadAttribute_IsRightConnected;
                        if (styleSet & HidNpadStyleTag_NpadFullKey)
                            handle = FullKeySixAxisHandle;
                        else if (padGetAttributes(&Pad) & preferedPadMask)
                            handle = JoyconSixaxisHandles[preferedPad];
                        else
                            handle = JoyconSixaxisHandles[preferedPad ^ 1];
                        hidGetSixAxisSensorStates(handle, &sixaxisValues, 1);

                        float forward[3];
                        xv3_cpy(forward, sixaxisValues.direction.direction[1]);
                        xv_norm(forward, forward, 3);

                        xAngle = xv3_dot(forward, GyroCalibrationRight);
                        yAngle = xv3_dot(forward, GyroCalibrationUp);
                    }

                    TouchCursorPosition = botScreenCenter
                        + Gfx::Vector2f{xAngle / (float)M_PI * 10.f, -yAngle / (float)M_PI * 10.f} * std::max(BotScreenRect.Size.X, BotScreenRect.Size.Y);

                    if (keysDown & (!Config::LeftHandedMode
                        ? HidNpadButton_ZL : HidNpadButton_ZR))
                        RecalibrateGyro(sixaxisValues);
                }

                if (hidGetTouchScreenStates(&touchPosition, 1) && touchPosition.count > 0)
                {
                    touchUseCursor = false;
                    UseRealTouchscreen = true;
                }

                if (TouchCursorPosition.X < BotScreenRect.Position.X)
                {
                    TouchCursorPosition.X = BotScreenRect.Position.X - 1.f;
                    touchUseCursor = false;
                }
                if (TouchCursorPosition.X >= BotScreenRect.Position.X + BotScreenRect.Size.X)
                {
                    TouchCursorPosition.X = BotScreenRect.Position.X + BotScreenRect.Size.X + 1.f;
                    touchUseCursor = false;
                }
                if (TouchCursorPosition.Y < BotScreenRect.Position.Y)
                {
                    TouchCursorPosition.Y = BotScreenRect.Position.Y - 1.f;
                    touchUseCursor = false;
                }
                if (TouchCursorPosition.Y >= BotScreenRect.Position.Y + BotScreenRect.Size.Y)
                {
                    TouchCursorPosition.Y = BotScreenRect.Position.Y + BotScreenRect.Size.Y + 1.f;
                    touchUseCursor = false;
                }
            }
            else
            {
                touchUseCursor = false;
            }

            u32 keyMask = 0xFFF;
            for (int i = 0; i < 12; i++)
                keyMask &= ~(!!(PlatformKeysHeld & rotatedKeyMappings[i]) << i);

            bool feedMicNoise = PlatformKeysHeld & InputConfig::MicNoise;
            if (keysDown & InputConfig::changeScreen)
            {
                switch (Config::ScreenSizing)
                {
                case 0:
                    Config::ScreenSizing = AutoScreenSizing == 0 ? 1 : AutoScreenSizing;
                    break;
                case 1:
                    Config::ScreenSizing = 2;
                    break;
                case 2:
                    Config::ScreenSizing = 1;
                    break;
                case 3:
                    Config::ScreenSizing = AutoScreenSizing == 2 ? 1 : 2;
                    break;
                case 4:
                    Config::ScreenSizing = 5;
                    break;
                case 5:
                    Config::ScreenSizing = 4;
                    break;
                }
                UpdateScreenLayout();
            }

            {
                MainScreenPosition[2] = MainScreenPosition[1];
                MainScreenPosition[1] = MainScreenPosition[0];
                MainScreenPosition[0] = NDS::PowerControl9 >> 15;
                int guess;
                if (MainScreenPosition[0] == MainScreenPosition[2] &&
                    MainScreenPosition[0] != MainScreenPosition[1])
                {
                    // constant flickering, likely displaying 3D on both screens
                    // TODO: when both screens are used for 2D only...???
                    guess = 0;
                }
                else
                {
                    if (MainScreenPosition[0] == 1)
                        guess = 1;
                    else
                        guess = 2;
                }
                AutoScreenSizing = guess;

                if (AutoScreenSizing != LastAutoScreenSizing)
                {
                    UpdateScreenLayout();
                    LastAutoScreenSizing = AutoScreenSizing;
                }
            }

            if (FirstBotScreen != -1)
            {
                if (!UseRealTouchscreen)
                {
                    u32 touchButtonMask = !Config::LeftHandedMode ? HidNpadButton_ZR : HidNpadButton_ZL;
                    if (Config::TouchscreenClickMode == 0)
                        TouchHeld = PlatformKeysHeld & touchButtonMask;
                    else if (keysDown & touchButtonMask)
                        TouchHeld ^= true;

                    TouchFinalPositionX = (int)(TouchCursorPosition.X * TouchScale);
                    TouchFinalPositionY = (int)(TouchCursorPosition.Y * TouchScale);
                    bool inside = Frontend::GetTouchCoords(TouchFinalPositionX, TouchFinalPositionY);
                    TouchDown = TouchHeld && inside;
                }
                else
                {
                    Gfx::Rotate90Deg(touchPosition.touches[0].x, touchPosition.touches[0].y, touchPosition.touches[0].x, touchPosition.touches[0].y, Config::GlobalRotation);
                    TouchFinalPositionX = (int)(touchPosition.touches[0].x * TouchScale);
                    TouchFinalPositionY = (int)(touchPosition.touches[0].y * TouchScale);
                    bool inside = Frontend::GetTouchCoords(TouchFinalPositionX, TouchFinalPositionY);
                    TouchDown = touchPosition.count > 0 && inside;
                }
            }

            if (feedMicNoise)
                Frontend::Mic_FeedNoise();
            else
                Frontend::Mic_FeedSilence();
            NDS::SetKeyMask(keyMask);
            if (TouchDown)
                NDS::TouchScreen((u16)TouchFinalPositionX, (u16)TouchFinalPositionY);
            else
                NDS::ReleaseScreen();
            if (LidClosed != NDS::IsLidClosed())
                NDS::SetLidClosed(LidClosed);

            bool fastForward = false;
            static bool fastForwardToggle = false;
            static bool prevKeyPressed = false;

            bool currentKeyPressed = Config::TouchscreenMode < 2 && (PlatformKeysHeld & InputConfig::fastForward);

            if (Config::FastForward) {
                // "Hold"
                fastForward = currentKeyPressed;
            } else {
                // "Toggle" on edge (press only)
                if (currentKeyPressed && !prevKeyPressed)
                    fastForwardToggle = !fastForwardToggle;
    
                fastForward = fastForwardToggle;
            }

            prevKeyPressed = currentKeyPressed;


            u64 totalFrameLength = 0;
            do {
                u64 frameStart = armGetSystemTick();
                NDS::RunFrame();
                u64 frameLength = armTicksToNs(armGetSystemTick() - frameStart);
                FrametimeHistogram[FrametimeHistogramNextValue] = (float)frameLength * 0.000001f;
                FrametimeHistogramNextValue++;
                if (FrametimeHistogramNextValue >= FrametimeHistogramLen)
                    FrametimeHistogramNextValue = 0;

                totalFrameLength += frameLength;
                //svcSleepThread(1000*1000*500);
            } while ((!Config::LimitFramerate || fastForward) && totalFrameLength < 1000000000 / 60);
        }
    }

    if (State != emuState_Nothing)
    {
        Gfx::WaitForFenceReady(((GPU2D::DekoRenderer*)GPU::GPU2D_Renderer.get())->FramebufferReady[GPU::FrontBuffer]);
        Gfx::SetSampler((Config::Filtering == 0 ? Gfx::sampler_Nearest : Gfx::sampler_Linear) | Gfx::sampler_ClampToEdge);
        for (int i = 0; i < ScreensVisible; i++)
        {
            Gfx::DrawRectangle(FramebufferTextures[GPU::FrontBuffer][ScreenKinds[i]], 
                ScreenPoints[i][0], ScreenPoints[i][1],
                ScreenPoints[i][2], ScreenPoints[i][3],
                {0.f, 0.f}, {256.f, 192.f});
        }
        
        Gfx::SignalFence(((GPU2D::DekoRenderer*)GPU::GPU2D_Renderer.get())->FramebufferPresented[GPU::FrontBuffer]);
    }

    if (State == emuState_Running && touchUseCursor)
    {
        // draw shadow first
        Gfx::DrawText(Gfx::SystemFontNintendoExt,
            TouchCursorPosition + Gfx::Vector2f{2.f, 3.f}, TextLineHeight * 2.f, DarkColorTransparent,
            Gfx::align_Center, Gfx::align_Center,
            TouchDown
                ? GFX_NINTENDOFONT_WII_HAND_HOLD
                : GFX_NINTENDOFONT_WII_HAND);
        Gfx::DrawText(Gfx::SystemFontNintendoExt,
            TouchCursorPosition, TextLineHeight * 2.f, WidgetColorBright,
            Gfx::align_Center, Gfx::align_Center,
            TouchDown
                ? GFX_NINTENDOFONT_WII_HAND_HOLD
                : GFX_NINTENDOFONT_WII_HAND);
    }

    if (Config::ShowPerformanceMetrics && State == emuState_Running)
    {
        Gfx::DrawRectangle({0.f, 0.f}, {4.f*FrametimeHistogramLen, TextLineHeight * 4.f}, DarkColorTransparent);
        float sum = 0.f, max = 0.f, min = infinityf();
        for (int i = 0; i < FrametimeHistogramLen; i++)
        {
            sum += FrametimeHistogram[i];
            max = std::max(max, FrametimeHistogram[i]);
            min = std::min(min, FrametimeHistogram[i]);
        }

        int idx = FrametimeHistogramNextValue;
        for (int i = 0; i < FrametimeHistogramLen; i++)
        {
            float frametime = FrametimeHistogram[idx];
            Gfx::DrawRectangle({i * 3.f, TextLineHeight}, {3.f, frametime / max * TextLineHeight}, WidgetColorBright);
            idx++;
            if (idx == FrametimeHistogramLen)
                idx = 0;
        }

        float averageFrametime = sum / (float)FrametimeHistogramLen;
        Gfx::DrawText(Gfx::SystemFontStandard, {0.f, 0.f}, TextLineHeight, WidgetColorBright, "avg: %.2fms min %.2fms max: %.2fms\n\nprof: %.2fms", averageFrametime, min, max, Profiler::Sum);
    }

    Profiler::Clear();

    if (State == emuState_Paused)
    {
        rc_client_paused();
        Gfx::DrawRectangle({0.f, 0.f}, {1280.f, 1280.f}, DarkColorTransparent);
    }
}

std::string GetLoadErrorStr(int error)
{
    switch (error)
    {
    case Frontend::Load_BIOS9Missing:
        return "DS ARM9 BIOS was not found or could not be accessed.\nIt should be named bios9.bin and be put into /switch/melonds";
    case Frontend::Load_BIOS9Bad:
        return "DS ARM9 BIOS is not a valid BIOS dump.";

    case Frontend::Load_BIOS7Missing:
        return "DS ARM7 BIOS was not found or could not be accessed.\nIt should be named bios7.bin and be put into /switch/melonds";
    case Frontend::Load_BIOS7Bad:
        return "DS ARM7 BIOS is not a valid BIOS dump.";

    case Frontend::Load_FirmwareMissing:
        return "DS firmware was not found or could not be accessed.\nIt should be named firmware.bin and be put into /switch/melonds";
    case Frontend::Load_FirmwareBad:
        return "DS firmware is not a valid firmware dump.";
    case Frontend::Load_FirmwareNotBootable:
        return "DS firmware is not bootable.";

    case Frontend::Load_DSiBIOS9Missing:
        return "DSi ARM9 BIOS was not found or could not be accessed.\nIt should be named biosdsi9.rom and be put into /switch/melonds";
    case Frontend::Load_DSiBIOS9Bad:
        return "DSi ARM9 BIOS is not a valid BIOS dump.";

    case Frontend::Load_DSiBIOS7Missing:
        return "DSi ARM7 BIOS was not found or could not be accessed.\nIt should be named biosdsi7.rom and be put into /switch/melonds";
    case Frontend::Load_DSiBIOS7Bad:
        return "DSi ARM7 BIOS is not a valid BIOS dump.";

    case Frontend::Load_DSiNANDMissing:
        return "DSi NAND was not found or could not be accessed.\nIt should be named nand.bin and be put into /switch/melonds";
    case Frontend::Load_DSiNANDBad:
        return "DSi NAND is not a valid NAND dump.";

    case Frontend::Load_ROMLoadError:
        return "Failed to load the ROM.";

    default: return "Unknown error during launch; smack Generic/RSDuck/catlover007.";
    }
}

void LoadROM(const char* file)
{
    Overclocking::ApplyOverclock(Config::SwitchOverclock);

    assert(State == emuState_Nothing);
    int res = Frontend::LoadROM(file, 0);
    if (res != Frontend::Load_OK)
    {
        ErrorDialog::Open(GetLoadErrorStr(res));
    }
    else
    {
        State = emuState_Running;
        StateAtomic = State;
        CurrentUiScreen = uiScreen_Start;
    }

    load_game_from_file(file);
}

void LoadBIOS()
{
    Overclocking::ApplyOverclock(Config::SwitchOverclock);

    assert(State == emuState_Nothing);
    int res = Frontend::LoadBIOS();
    if (res != Frontend::Load_OK)
    {
        ErrorDialog::Open(GetLoadErrorStr(res));
    }
    else
    {
        State = emuState_Running;
        StateAtomic = State;
        CurrentUiScreen = uiScreen_Start;
    }
}

void SetPause(bool pause)
{
    Overclocking::ApplyOverclock(pause ? 0 : Config::SwitchOverclock);

    PlatformKeysHeld = 0;
    assert(State == (pause ? emuState_Running : emuState_Paused));
    State = pause ? emuState_Paused : emuState_Running;
    StateAtomic = State;
    
    if (pause) {
        g_triggerNotifications.DeactivateAll();
    }
    else {
        g_triggerNotifications.ReactiveAll();
    }
}

void Stop()
{
    Frontend::UnloadROM(Frontend::ROMSlot_NDS);
    Frontend::UnloadROM(Frontend::ROMSlot_GBA);
    NDS::Stop();
    State = emuState_Nothing;
    StateAtomic = State;
}

void Reset()
{
    assert(State != emuState_Running);
    Frontend::Reset();
    State = emuState_Running;
    StateAtomic = State;
}

void DeriveScreenPoints(float* mat, Gfx::Vector2f* points, float scale)
{
    points[0] = {0.f, 0.f};
    points[1] = {256.f, 0.f};
    points[2] = {0.f, 192.f};
    points[3] = {256.f, 192.f};
    for (int i = 0; i < 4; i++)
    {
        Frontend::M23_Transform(mat, points[i].X, points[i].Y);
        points[i] *= scale;
    }
}

void UpdateScreenLayout()
{
    // we use the actual resolution otherwise integer scaling won't work properly
    int screenWidth = 1280;
    int screenHeight = 720;
    if (appletGetOperationMode() == AppletOperationMode_Console)
    {
        screenWidth = 1920;
        screenHeight = 1080;
    }
    TouchScale = screenHeight / 720.f;
    float invTouchScale = 720.f / screenHeight;
    if ((Config::GlobalRotation % 2) == 1)
        std::swap(screenWidth, screenHeight);
    const int screengap[] = {0, 1, 8, 16, 32, 64, 90, 128};
    const float aspectratios[] = {1, (16.f/9.f)/(4.f/3.f)};
    Frontend::SetupScreenLayout(screenWidth, screenHeight,
        Config::ScreenLayout,
        Config::ScreenRotation,
        Config::ScreenSizing == 3 ? AutoScreenSizing : Config::ScreenSizing,
        screengap[Config::ScreenGap],
        Config::IntegerScaling,
        Config::ScreenSwap,
        aspectratios[Config::ScreenAspectTop], aspectratios[Config::ScreenAspectBot]);
    float matrices[Frontend::MaxScreenTransforms][6];
    ScreensVisible = Frontend::GetScreenTransforms(matrices[0], ScreenKinds);
    FirstBotScreen = -1;
    for (int i = 0; i < ScreensVisible; i++)
    {
        DeriveScreenPoints(matrices[i], ScreenPoints[i], invTouchScale);
        if (FirstBotScreen == -1 && ScreenKinds[i] == 1)
        {
            FirstBotScreen = i;
            Gfx::Vector2f lowerBounds = {infinityf(), infinityf()};
            Gfx::Vector2f upperBounds = {-infinityf(), -infinityf()};
            for (int j = 0; j < 4; j++)
            {
                lowerBounds = lowerBounds.Min(ScreenPoints[i][j]);
                upperBounds = upperBounds.Max(ScreenPoints[i][j]);
            }
            BotScreenRect.Position = lowerBounds;
            BotScreenRect.Size = upperBounds - lowerBounds;
        }
    }
}

}

namespace Platform
{
extern char ExecutableDir[];
}

void OnAppletHook(AppletHookType hook, void *param)
{
    if (hook == AppletHookType_OnPerformanceMode)
        Emulation::UpdateScreenLayout();

    Overclocking::OnAppletHook(hook);
}

int main(int argc, const char* argv[])
{
    appletLockExit();

    socketInitializeDefault();
    nxlinkStdio();

    romfsInit();
    setInitialize();

    padConfigureInput(1, HidNpadStyleSet_NpadFullCtrl);
    padInitializeDefault(&Pad);

    hidInitializeTouchScreen();
    InputConfig::setupInputActions();

    Gfx::Init();

    Overclocking::Init();

    AppletHookCookie aptCookie;
    appletHook(&aptCookie, OnAppletHook, NULL);

    Config::Load();
    
    strcpy(Config::FirmwarePath, "firmware.bin");
    strcpy(Config::BIOS9Path, "bios9.bin");
    strcpy(Config::BIOS7Path, "bios7.bin");
    strcpy(Config::DSiBIOS9Path, "biosdsi9.rom");
    strcpy(Config::DSiBIOS7Path, "biosdsi7.rom");
    strcpy(Config::DSiFirmwarePath, "firmware_dsi.bin");
    strcpy(Config::DSiNANDPath, "nand.bin");
    strcpy(Config::DSiSDPath, "sd.bin");
    {
        FILE* f = Platform::OpenLocalFile("sd.bin", "rb");
        Config::DSiSDEnable = f != nullptr;
        if (f)
            fclose(f);
    }

    ROMMetaDatabase::Init();
    Filebrowser::Init();
    StartMenu::Init();

    Frontend::Init_ROM();
    Emulation::Init();

    bool argvLoaded = false;
    static bool pad_initialized = false;
    static PadState pad;

    if (!pad_initialized) {
            padConfigureInput(1, HidNpadStyleSet_NpadStandard);
            padInitializeAny(&pad);
            InputConfig::loadMappingFromFile("sdmc:/switch/melonDS/input.cfg");
            pad_initialized = true;
        }

    if (Config::RetroAchievementsUsername[0] != '\0')
        InitRetroAchievements(Config::RetroAchievementsUsername, Config::RetroAchievementsToken, true);
    
    while (appletMainLoop() && !Done)
    {
        Gfx::StartFrame();

        int rotation = Config::GlobalRotation;
        int screenWidth = 1280, screenHeight = 720;
        if ((rotation % 2) == 1)
            std::swap(screenWidth, screenHeight);

        Gfx::PushScissor(0, 0, screenWidth, screenHeight);
        BoxGui::Frame rootFrame{Gfx::Vector2f{(float)screenWidth, (float)screenHeight}};

        padUpdate(&Pad);
        u64 rawKeysDown = padGetButtonsDown(&Pad);
        u64 rawKeysUp = padGetButtonsUp(&Pad);

        u64 logicalKeysDown = InputConfig::GetLogicalKeysDown(rawKeysDown);
        u64 logicalKeysUp = InputConfig::GetLogicalKeysDown(rawKeysUp);

        if (Emulation::State != Emulation::emuState_Nothing)
        {
            Emulation::UpdateAndDraw(logicalKeysDown, logicalKeysUp);
        }

        switch (CurrentUiScreen)
        {
        case uiScreen_Start:
            if (Emulation::State != Emulation::emuState_Running)
                StartMenu::DoGui(rootFrame);
            break;
        case uiScreen_BrowseROM:
            Filebrowser::DoGui(rootFrame);
            break;
        case uiScreen_EmulationSettings:
        case uiScreen_DisplaySettings:
        case uiScreen_RetroAchievements:
        case uiScreen_InputSettings:
            SettingsDialog::DoGui(rootFrame);
            break;
        }

        if (!argvLoaded && argc == 2)
        {
            Emulation::LoadROM(argv[1]);
            argvLoaded = true;
        }

        if (!BoxGui::HasModalDialog())
            KeyExplanation::DoGui(rootFrame);
        else
            KeyExplanation::Reset();

        BoxGui::Update(rootFrame, rawKeysDown, rawKeysUp);

        g_notification.Render();
        g_triggerNotifications.RenderAll();
        
        for (auto& [id, tracker] : g_leaderboardTrackers) {
            tracker.Render();
        }

        Gfx::PopScissor();

        Gfx::EndFrame(Emulation::State == Emulation::emuState_Running
            ? Gfx::Color() : WallpaperColor, rotation);
    }

    Emulation::DeInit();
    Frontend::DeInit_ROM();

    resetRCClient();

    StartMenu::DeInit();
    Filebrowser::DeInit();

    Overclocking::DeInit();

    Config::Save();

    appletUnhook(&aptCookie);

    Gfx::DeInit();

    setExit();
    romfsExit();

    socketExit();

    appletUnlockExit();
    return 0;
}