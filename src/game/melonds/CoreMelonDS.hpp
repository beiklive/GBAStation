#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <functional>

#include "core/common.h"
#include "core/Tools.hpp"
#include "core/GameSignal.hpp"

#include "types.h"
#include "MemConstants.h"
#include "FreeBIOS.h"
#include "SPI_Firmware.h"

namespace melonDS
{
class NDS;
namespace NDSCart { class CartCommon; }
}

namespace melonDS::Platform
{
void SetNDSSavePath(const std::string& path);
void SetGBASavePath(const std::string& path);
void SetFirmwarePath(const std::string& path);
void SetStopCallback(std::function<void()> cb);
}

namespace beiklive::melonds
{

class CoreMelonDS
{
public:
    CoreMelonDS();
    ~CoreMelonDS();

    bool SetupGame(beiklive::GameEntry entry);
    void Cleanup();

    void RunFrame();

    void Reset();

    // ---- 视频 -----------------------------------------------------------

    const uint32_t* GetTopFramebuffer() const;
    const uint32_t* GetBottomFramebuffer() const;

    // ---- 音频 -----------------------------------------------------------

    int ReadAudio(int16_t* data, int samples);

    // ---- 输入 -----------------------------------------------------------

    void SetButtonState(unsigned id, bool pressed);
    void SetButtonsFromSignal();

    void TouchScreen(uint16_t x, uint16_t y);
    void ReleaseScreen();

    // ---- 几何信息 -------------------------------------------------------

    unsigned GameWidth()  const { return 256; }
    unsigned GameHeight() const { return 192 * 2; }
    double   Fps()        const { return 60.0; }

    // ---- 快进 -----------------------------------------------------------

    void SetFastForwarding(bool ff) { m_fastForward = ff; }

    // ---- 存档 -----------------------------------------------------------

    void SaveNDSSave();
    void LoadNDSSave();

    // ---- 金手指 ---------------------------------------------------------

    void ApplyCheats(const std::vector<CheatEntry>& cheats);
    const std::vector<CheatEntry>& GetCheats() const { return m_cheats; }

    // ---- 状态 -----------------------------------------------------------

    bool IsReady() const { return m_ready; }

private:
    beiklive::GameEntry m_gameEntry;
    std::unique_ptr<melonDS::NDS> m_nds;
    std::vector<CheatEntry> m_cheats;

    std::unique_ptr<melonDS::u8[]> m_romData;
    melonDS::u32 m_romLen = 0;

    std::array<melonDS::u8, melonDS::ARM9BIOSSize> m_arm9bios = melonDS::bios_arm9_bin;
    std::array<melonDS::u8, melonDS::ARM7BIOSSize> m_arm7bios = melonDS::bios_arm7_bin;
    melonDS::Firmware m_firmware{0};

    uint32_t m_keyMask = 0x03FF03FF;
    bool m_touchDown = false;
    bool m_ready = false;
    bool m_fastForward = false;

    bool _loadROM(const std::string& romPath);
    bool _loadBIOS();
    bool _loadFirmware();
    void _loadSram();
    void _saveSram();
};

} // namespace beiklive::melonds
