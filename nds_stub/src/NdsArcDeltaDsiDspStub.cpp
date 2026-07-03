#include "../../third_party/ArcDelta_melonDS/src/types.h"
#include "../../third_party/ArcDelta_melonDS/src/Savestate.h"
#include "../../third_party/ArcDelta_melonDS/src/DSi_DSP.h"

namespace DSi_DSP {

u16 DSP_PDATA = 0;
u16 DSP_PADR = 0;
u16 DSP_PCFG = 0;
u16 DSP_PSTS = 0x0100;
u16 DSP_PSEM = 0;
u16 DSP_PMASK = 0xff;
u16 DSP_PCLEAR = 0;
u16 DSP_SEM = 0;
u16 DSP_CMD[3] = {};
u16 DSP_REP[3] = {};

bool Init()
{
    Reset();
    return true;
}

void DeInit() {}

void Reset()
{
    DSP_PDATA = 0;
    DSP_PADR = 0;
    DSP_PCFG = 0;
    DSP_PSTS = 0x0100;
    DSP_PSEM = 0;
    DSP_PMASK = 0xff;
    DSP_PCLEAR = 0;
    DSP_SEM = 0;
    DSP_CMD[0] = DSP_CMD[1] = DSP_CMD[2] = 0;
    DSP_REP[0] = DSP_REP[1] = DSP_REP[2] = 0;
}

void DoSavestate(Savestate*) {}

bool IsRstReleased()
{
    return false;
}

void SetRstLine(bool) {}

void OnMBKCfg(char, u32, u8, u8, u8*) {}

u8 Read8(u32 addr)
{
    const u16 value = Read16(addr & ~1u);
    return (addr & 1) ? static_cast<u8>(value >> 8) : static_cast<u8>(value);
}

void Write8(u32 addr, u8 val)
{
    const u16 old = Read16(addr & ~1u);
    const u16 value = (addr & 1) ? static_cast<u16>((old & 0x00ff) | (val << 8))
                                 : static_cast<u16>((old & 0xff00) | val);
    Write16(addr & ~1u, value);
}

u16 Read16(u32 addr)
{
    switch (addr & 0x3f)
    {
    case 0x00: return DSP_PDATA;
    case 0x04: return DSP_PADR;
    case 0x08: return DSP_PCFG;
    case 0x0c: return DSP_PSTS;
    case 0x10: return DSP_PSEM;
    case 0x14: return DSP_PMASK;
    case 0x18: return DSP_PCLEAR;
    case 0x1c: return DSP_SEM;
    case 0x20: return DSP_CMD[0];
    case 0x24: return DSP_CMD[1];
    case 0x28: return DSP_CMD[2];
    case 0x30: return DSP_REP[0];
    case 0x34: return DSP_REP[1];
    case 0x38: return DSP_REP[2];
    default: return 0;
    }
}

void Write16(u32 addr, u16 val)
{
    switch (addr & 0x3f)
    {
    case 0x00: DSP_PDATA = val; break;
    case 0x04: DSP_PADR = val; break;
    case 0x08: DSP_PCFG = val; break;
    case 0x10: DSP_PSEM = val; break;
    case 0x14: DSP_PMASK = val; break;
    case 0x18: DSP_PCLEAR = val; break;
    case 0x1c: DSP_SEM = val; break;
    case 0x20: DSP_CMD[0] = val; break;
    case 0x24: DSP_CMD[1] = val; break;
    case 0x28: DSP_CMD[2] = val; break;
    default: break;
    }
}

u32 Read32(u32 addr)
{
    return static_cast<u32>(Read16(addr)) | (static_cast<u32>(Read16(addr + 2)) << 16);
}

void Write32(u32 addr, u32 val)
{
    Write16(addr, static_cast<u16>(val));
    Write16(addr + 2, static_cast<u16>(val >> 16));
}

void Run(u32) {}

} // namespace DSi_DSP
