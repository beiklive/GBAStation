/***********************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.

  (c) Copyright 1996 - 2002  Gary Henderson (gary.henderson@ntlworld.com),
                             Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2002 - 2004  Matthew Kendora

  (c) Copyright 2002 - 2005  Peter Bortas (peter@bortas.org)

  (c) Copyright 2004 - 2005  Joel Yliluoma (http://iki.fi/bisqwit/)

  (c) Copyright 2001 - 2006  John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2006  funkyass (funkyass@spam.shaw.ca),
                             Kris Bleakley (codeviolation@hotmail.com)

  (c) Copyright 2002 - 2010  Brad Jorsch (anomie@users.sourceforge.net),
                             Nach (n-a-c-h@users.sourceforge.net),
                             zones (kasumitokoduck@yahoo.com)

  (c) Copyright 2006 - 2007  nitsuja

  (c) Copyright 2009 - 2010  BearOso,
                             OV2


  BS-X C emulator code
  (c) Copyright 2005 - 2006  Dreamer Nom,
                             zones

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003  _Demo_ (_demo_@zsnes.com),
                             Nach,
                             zsKnight (zsknight@zsnes.com)

  C4 C++ code
  (c) Copyright 2003 - 2006  Brad Jorsch,
                             Nach

  DSP-1 emulator code
  (c) Copyright 1998 - 2006  _Demo_,
                             Andreas Naive (andreasnaive@gmail.com),
                             Gary Henderson,
                             Ivar (ivar@snes9x.com),
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora,
                             Nach,
                             neviksti (neviksti@hotmail.com)

  DSP-2 emulator code
  (c) Copyright 2003         John Weidman,
                             Kris Bleakley,
                             Lord Nightmare (lord_nightmare@users.sourceforge.net),
                             Matthew Kendora,
                             neviksti

  DSP-3 emulator code
  (c) Copyright 2003 - 2006  John Weidman,
                             Kris Bleakley,
                             Lancer,
                             z80 gaiden

  DSP-4 emulator code
  (c) Copyright 2004 - 2006  Dreamer Nom,
                             John Weidman,
                             Kris Bleakley,
                             Nach,
                             z80 gaiden

  OBC1 emulator code
  (c) Copyright 2001 - 2004  zsKnight,
                             pagefault (pagefault@zsnes.com),
                             Kris Bleakley
                             Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code used in 1.39-1.51
  (c) Copyright 2002         Matthew Kendora with research by
                             zsKnight,
                             John Weidman,
                             Dark Force

  SPC7110 and RTC C++ emulator code used in 1.52+
  (c) Copyright 2009         byuu,
                             neviksti

  S-DD1 C emulator code
  (c) Copyright 2003         Brad Jorsch with research by
                             Andreas Naive,
                             John Weidman

  S-RTC C emulator code
  (c) Copyright 2001 - 2006  byuu,
                             John Weidman

  ST010 C++ emulator code
  (c) Copyright 2003         Feather,
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora

  Super FX x86 assembler emulator code
  (c) Copyright 1998 - 2003  _Demo_,
                             pagefault,
                             zsKnight

  Super FX C emulator code
  (c) Copyright 1997 - 1999  Ivar,
                             Gary Henderson,
                             John Weidman

  Sound emulator code used in 1.5-1.51
  (c) Copyright 1998 - 2003  Brad Martin
  (c) Copyright 1998 - 2006  Charles Bilyue'

  Sound emulator code used in 1.52+
  (c) Copyright 2004 - 2007  Shay Green (gblargg@gmail.com)

  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004  Marcus Comstedt (marcus@mc.pp.se)

  2xSaI filter
  (c) Copyright 1999 - 2001  Derek Liauw Kie Fa

  HQ2x, HQ3x, HQ4x filters
  (c) Copyright 2003         Maxim Stepin (maxim@hiend3d.com)

  NTSC filter
  (c) Copyright 2006 - 2007  Shay Green

  GTK+ GUI code
  (c) Copyright 2004 - 2010  BearOso

  Win32 GUI code
  (c) Copyright 2003 - 2006  blip,
                             funkyass,
                             Matthew Kendora,
                             Nach,
                             nitsuja
  (c) Copyright 2009 - 2010  OV2

  Mac OS GUI code
  (c) Copyright 1998 - 2001  John Stiles
  (c) Copyright 2001 - 2010  zones

  (c) Copyright 2010 - 2016 Daniel De Matteis. (UNDER NO CIRCUMSTANCE 
  WILL COMMERCIAL RIGHTS EVER BE APPROPRIATED TO ANY PARTY)

  (c) Copyright 2020         Mahyar Koshkouei

  Specific ports contains the works of other authors. See headers in
  individual files.


  Snes9x homepage: http://www.snes9x.com/

  Permission to use, copy, modify and/or distribute Snes9x in both binary
  and source form, for non-commercial purposes, is hereby granted without
  fee, providing that this license information and copyright notice appear
  with all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software or it's derivatives.

  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes,
  but is not limited to, charging money for Snes9x or software derived from
  Snes9x, including Snes9x or derivatives in commercial game bundles, and/or
  using Snes9x as a promotion for your commercial product.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
 ***********************************************************************************/


#ifndef _SNES9X_H_
#define _SNES9X_H_

#ifndef VERSION
#define VERSION	"2010"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include "65c816.h"
#include "messages.h"

#define ACCESSORY_AUTODETECTION_CONFIRM   0
#define ACCESSORY_AUTODETECTION_ENABLED   1
#define ACCESSORY_AUTODETECTION_NONE      2

#define S9X_ACCURACY_LEVEL		3


#ifndef SNES_SUPPORT_MULTI_CART
#define SNES_SUPPORT_MULTI_CART		0
#endif

#include <boolean.h>
#include <streams/memory_stream.h>

 /* Create some sort of abstraction for files 
  * using memory only since libretro API is memory oriented.
  *
  * Very hacky, but at least allows us to avoid hacking up 
  * the whole source or go through the file system every time 
  * we want to do something. */

#define STREAM memstream_t *
#define READ_STREAM(p, l, s)     memstream_read(s, p, l)
#define WRITE_STREAM(p, l, s)    memstream_write(s, p, l)
#define GETS_STREAM(p, l, s)     memstream_gets(s, p, l)
#define GETC_STREAM(s)           memstream_getc(s)
#define OPEN_STREAM(w)           memstream_open(w)
#define FIND_STREAM(f)           memstream_pos(f)
#define REVERT_STREAM(f, o, s)   memstream_seek(f, o, s)
#define CLOSE_STREAM(s)          memstream_close(s)

#define SNES_WIDTH		256
#define SNES_HEIGHT		224
#define SNES_HEIGHT_EXTENDED	239

/* MAX_SNES_WIDTH = SNES_WIDTH * 2 */
#define MAX_SNES_WIDTH		512

/* MAX_SNES_HEIGHT = SNES_HEIGHT_EXTENDED * 2 */
#define MAX_SNES_HEIGHT		478

#define	NTSC_MASTER_CLOCK	21477272.0
#define	PAL_MASTER_CLOCK	21281370.0

#define SNES_MAX_NTSC_VCOUNTER	262
#define SNES_MAX_PAL_VCOUNTER	312
#define SNES_HCOUNTER_MAX	341

#define ONE_CYCLE						(overclock_cycles ? one_c : 6)
#define SLOW_ONE_CYCLE				(overclock_cycles ? slow_one_c : 8)
#define TWO_CYCLES					(overclock_cycles ? two_c : 12)
#define	ONE_DOT_CYCLE		4
#define ONE_DOT_CYCLE_DIV_2	2

#define SNES_CYCLES_PER_SCANLINE	1364

#define SNES_WRAM_REFRESH_HC_v2		538
#define SNES_WRAM_REFRESH_HC_v2_MIN_ONE_DOT_CYCLE	534
#define SNES_WRAM_REFRESH_CYCLES	40

#define SNES_HBLANK_START_HC		1096		/* H=274 */
#define	SNES_HDMA_START_HC		1106		/* FIXME: not true */
#define	SNES_HBLANK_END_HC		4		/* H=1 */
#define	SNES_HDMA_INIT_HC		20		/* FIXME: not true */

/* SNES_RENDER_START_HC = 48 * ONE_DOT_CYCLE */
#define	SNES_RENDER_START_HC		192		/* FIXME: Snes9x renders a line at a time. */

#define SNES_TR_MASK			16
#define SNES_TL_MASK			32
#define SNES_X_MASK			64
#define SNES_A_MASK			128
#define SNES_RIGHT_MASK			256
#define SNES_LEFT_MASK			512
#define SNES_DOWN_MASK			1024
#define SNES_UP_MASK			2048
#define SNES_START_MASK			4096
#define SNES_SELECT_MASK		8192
#define SNES_Y_MASK			16384
#define SNES_B_MASK			32768

#define DEBUG_MODE_FLAG			1	/* debugger */
#define TRACE_FLAG			2	/* debugger */
#define SINGLE_STEP_FLAG		4	/* debugger */
#define BREAK_FLAG			8	/* debugger */
#define SCAN_KEYS_FLAG			16	/* CPU */
#define NMI_FLAG			128	/* CPU */
#define IRQ_FLAG			2048	/* CPU */
#define HALTED_FLAG			4096	/* APU */

extern bool overclock_cycles;
extern int one_c, slow_one_c, two_c;

struct SCPUState
{
	int32_t	Cycles;
	int32_t	PrevCycles;
	int32_t	V_Counter;
	uint32_t	Flags;
	uint8_t	*PCBase;
	uint8_t	IRQActive;
	int32_t	IRQPending;
	int32_t	MemSpeed;
	int32_t	MemSpeedx2;
	int32_t	FastROMSpeed;
	uint8_t	InDMA;
	uint8_t	InHDMA;
	uint8_t	InDMAorHDMA;
	uint8_t	InWRAMDMAorHDMA;
	uint8_t	HDMARanInDMA;
	int32_t	CurrentDMAorHDMAChannel;
	uint8_t	WhichEvent;
	int32_t	NextEvent;
	uint8_t	WaitingForInterrupt;
};

#define HC_HBLANK_START_EVENT	1
#define HC_IRQ_1_3_EVENT	2
#define HC_HDMA_START_EVENT	3
#define	HC_IRQ_3_5_EVENT	4
#define HC_HCOUNTER_MAX_EVENT	5
#define HC_IRQ_5_7_EVENT	6
#define HC_HDMA_INIT_EVENT	7
#define HC_IRQ_7_9_EVENT	8
#define HC_RENDER_EVENT		9
#define HC_IRQ_9_A_EVENT	10
#define HC_WRAM_REFRESH_EVENT	11
#define HC_IRQ_A_1_EVENT	12

struct STimings
{
	int32_t	H_Max_Master;
	int32_t	H_Max;
	int32_t	V_Max_Master;
	int32_t	V_Max;
	int32_t	HBlankStart;
	int32_t	HBlankEnd;
	int32_t	HDMAInit;
	int32_t	HDMAStart;
	int32_t	NMITriggerPos;
	int32_t	WRAMRefreshPos;
	int32_t	RenderPos;
	uint8_t	InterlaceField;
	int32_t	DMACPUSync;	/* The cycles to synchronize DMA and CPU. Snes9x cannot emulate correctly. */
	int32_t	NMIDMADelay;	/* The delay of NMI trigger after DMA transfers. Snes9x cannot emulate correctly. */
	int32_t	IRQPendCount;	/* This value is just a hack, because Snes9x cannot emulate any events in an opcode. */
	int32_t	APUSpeedup;
	uint8_t	APUAllowTimeOverflow;
};

struct SSettings
{
	uint8_t	SuperFX;
	uint8_t	DSP;
	uint8_t	SA1;
	uint8_t	C4;
	uint8_t	SDD1;
	uint8_t	SPC7110;
	uint8_t	SPC7110RTC;
	uint8_t	OBC1;
	uint8_t	SETA;
	uint8_t	SRTC;
	uint8_t	BS;
	uint8_t	BSXItself;
	uint8_t	BSXBootup;

	uint8_t	ForceLoROM;
	uint8_t	ForceHiROM;
	uint8_t	ForceHeader;
	uint8_t	ForceNoHeader;
	uint8_t	ForceInterleaved;
	uint8_t	ForceInterleaved2;
	uint8_t	ForceInterleaveGD24;
	uint8_t	ForceNotInterleaved;
	uint8_t	ForcePAL;
	uint8_t	ForceNTSC;
	uint8_t	ApplyCheats;
	uint8_t	NoPatch;
	uint8_t	PAL;
	uint32_t	FrameTimePAL;
	uint32_t	FrameTimeNTSC;

	uint32_t  SpeedhackGameID;

	uint8_t	Multi;
	char	CartAName[PATH_MAX + 1];
	char	CartBName[PATH_MAX + 1];

	uint8_t	DisableGameSpecificHacks;
	uint8_t	BlockInvalidVRAMAccessMaster;
	uint8_t	BlockInvalidVRAMAccess;
	int32_t	HDMATimingHack;

	uint32_t	Throttled;
	int32_t	Crosshair;

	uint32_t	AccessoryType;
	int32_t		AccessoryAutoDetection;
	uint8_t		CurrentROMisMultitapCompatible;
	uint8_t		CurrentROMisMouseCompatible;
	uint8_t		CurrentROMisSuperScopeCompatible;
	uint8_t		CurrentROMisJustifierCompatible;
   uint8_t    NormalControls;
	uint8_t		SupportHiRes;
	/* Mode 7 hires upsample factor: 0 = off (native 256), 2 = 2x
	   horizontal (512), 4 = 4x horizontal (1024). Buffer width is
	   promoted accordingly when in Mode 7. */
	int32_t		Mode7Hires;
	/* Mode 7 hires vertical doubling: 0 = off, 1 = on (output buffer
	   is 2x tall, 224 -> 448 rows). Effective only when Mode7Hires is
	   2 or 4 AND Mode7HiresBilinear is 2 (smooth) -- the smooth
	   bilinear renderer family produces genuinely-new bilinear samples
	   between the native source rows for the doubled output. With any
	   other combination the option is a no-op (stable bilinear's
	   X-only blend would just duplicate rows; HR mode has no
	   interpolation either). When effective, M7 BG renders at 2x
	   vertical resolution; non-M7 layers (sprites, HUD, BG2 if used
	   under M7) get duplicated to 2 output rows by the buffer
	   promotion path, which keeps the overall layout correct at the
	   cost of doubled-pixel HUD text. */
	int32_t		Mode7HiresVertical;
	/* Mode 7 bilinear filter mode: 0 = off, 1 = stable (X-only blend
	   with floor-Y), 2 = smooth (4-corner bilinear with a row-contrast
	   guard that falls back to stable's blend on dissimilar row pairs,
	   so it doesn't produce muddy averaged seams on content like Tiny
	   Toons rainbow rings). Stable matches HD-no-BL's Y-axis sampling
	   and only smooths X. */
	int32_t		Mode7HiresBilinear;
   float    SuperFXSpeedPerLine;
   uint8_t	FastSavestates;
   uint8_t	HardDisableAudio;
};

struct SSNESGameFixes
{
	uint8_t	SRAMInitialValue;
	uint8_t	Uniracers;
};

void S9xMessage (S9xMessagePriority p, S9xMessageCategory c, const char *msg);

extern struct SSettings			Settings;
extern struct SCPUState			CPU;
extern struct STimings			Timings;
extern struct SSNESGameFixes	SNESGameFixes;

/* Use when writing to $4016. */
extern void S9xSetJoypadLatch (uint8_t latch);

/* Use when reading $4016/7 (JOYSER0 and JOYSER1). */
extern uint8_t S9xReadJOYSERn (int n);

extern uint8_t pad_read;

#ifdef __cplusplus
}
#endif

#endif
