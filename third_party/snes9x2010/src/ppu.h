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

#ifndef _PPU_H_
#define _PPU_H_

#include "memmap.h"

#define FIRST_VISIBLE_LINE	1

#define TILE_2BIT		0
#define TILE_4BIT		1
#define TILE_8BIT		2
#define TILE_2BIT_EVEN		3
#define TILE_2BIT_ODD		4
#define TILE_4BIT_EVEN		5
#define TILE_4BIT_ODD		6

#define MAX_2BIT_TILES		4096
#define MAX_4BIT_TILES		2048
#define MAX_8BIT_TILES		1024

#define CLIP_OR			0
#define CLIP_AND		1
#define CLIP_XOR		2
#define CLIP_XNOR		3

#define PPU_IRQ_SOURCE		2
#define GSU_IRQ_SOURCE		4
#define SA1_DMA_IRQ_SOURCE	32
#define SA1_IRQ_SOURCE		128

/* PPU memory-mapped registers */

#define MEM_PPU_INIDISP 0x2100
#define MEM_PPU_OBSEL   0x2101
#define MEM_PPU_OAMADDL 0x2102
#define MEM_PPU_OAMADDH 0x2103
#define MEM_PPU_OAMDATA 0x2104
#define MEM_PPU_BGMODE  0x2105
#define MEM_PPU_MOSAIC  0x2106
#define MEM_PPU_BG1SC   0x2107
#define MEM_PPU_BG2SC   0x2108
#define MEM_PPU_BG3SC   0x2109
#define MEM_PPU_BG4SC   0x210A
#define MEM_PPU_BG12NBA 0x210B
#define MEM_PPU_BG34NBA 0x210C
#define MEM_PPU_BG1HOFS 0x210D

#define MEM_PPU_TM	0x212C
#define MEM_PPU_TS	0x212D
#define MEM_PPU_TMW	0x212E
#define MEM_PPU_TSW	0x212F

#define MEM_PPU_CGWSEL	0x2130
#define MEM_PPU_CGADSUB 0x2131
#define MEM_PPU_SETINI  0x2133

#define MEM_PPU_APUIO0	0x2140

#define MEM_PPU_STAT77	0x213E
#define MEM_PPU_STAT78	0x213F

#define MEM_PPU_WMDATA	0x2180

#define MEM_PPU_WMADDH	0x2183

/* CPU memory-mapped registers */

#define MEM_CPU_JOYSER0 0x4016
#define MEM_CPU_JOYSER1 0x4017
#define MEM_CPU_NMITIMEN 0x4200
#define MEM_CPU_JOY1L	0x4218 // JOY1L
#define MEM_CPU_JOY1H	0x4219 // JOY1H
#define MEM_CPU_JOY2L	0x421A // JOY2L
#define MEM_CPU_JOY2H	0x421B // JOY2H
#define MEM_CPU_JOY3L	0x421C // JOY3L
#define MEM_CPU_JOY3H	0x421D // JOY3H
#define MEM_CPU_JOY4L	0x421E
#define MEM_CPU_JOY4H	0x421F

/* DMA memory-mapped registers */

#define MEM_DMA_DMAP0	0x4300

/* SA-1 memory-mapped registers */

#define MEM_SA1_CIVL	0x2207
#define MEM_SA1_CIVH	0x2208
#define MEM_SA1_SCNT	0x2209
#define MEM_SA1_SNVL	0x220C
#define MEM_SA1_SNVH	0x220D
#define MEM_SA1_SIVL	0x220E
#define MEM_SA1_SIVH	0x220F

struct ClipData
{
	uint16_t	Left[6];
	uint16_t	Right[6];
	uint8_t	Count;
	uint8_t	DrawMode[6];
};

struct InternalPPU
{
	uint8_t	*TileCache[7];
	uint8_t	*TileCached[7];
	const uint8_t	*XB;
	int	RenderedScreenWidth;
	int	RenderedScreenHeight;
	int	CurrentLine;
	int	PreviousLine;
	struct ClipData Clip[2][6];         /* uint16_t alignment */
	uint16_t	ScreenColors[256];
#ifdef CORRECT_VRAM_READS
	uint16_t	VRAMReadBuffer;
#else
	uint8_t	FirstVRAMRead;
#endif
	uint8_t	OBJChanged;
	uint8_t	DirectColourMapsNeedRebuild;
	uint8_t	Interlace;
	uint8_t	InterlaceOBJ;
	uint8_t	PseudoHires;
	uint8_t	DoubleWidthPixels;
	/* Set when this frame's buffer is at 4x SNES width (Mode 7 hires
	   = 4x). DoubleWidthPixels is also TRUE in this case; consumers
	   that only need to know "is the buffer wider than native" can
	   keep checking DoubleWidthPixels alone. */
	uint8_t	QuadWidthPixels;
	uint8_t	DoubleHeightPixels;
	/* Mode 7 vertical-2x post-pass control. Set to a non-negative value
	   when the current frame should be expanded to 2x vertical at end-
	   of-frame: the value is the source-Y row at which the M7 plane
	   begins (rows below this are HUD/Mode 1 and get row-replicated;
	   rows from this index onward are M7 and get bilinear-Y-interpolated
	   between adjacent source rows). Set to -1 to disable the post-pass
	   for this frame. Reset at frame start; set by the frame-start setup
	   in cpuexec.c (when the frame begins in M7) or by the mid-frame
	   mode-switch hook in ppu.c (when the frame switches into M7
	   mid-frame). */
	int32_t	M7VertStartY;
	uint8_t RenderThisFrame;
};

struct SOBJ
{
	int16_t	HPos;
	uint16_t	VPos;
	uint16_t	Name;
	uint8_t	HFlip;
	uint8_t	VFlip;
	uint8_t	Priority;
	uint8_t	Palette;
	uint8_t	Size;
};

struct SPPU
{
	struct
	{
		uint16_t	Address;
		uint16_t	Mask1;
		uint16_t	FullGraphicCount;
		uint16_t	Shift;
		uint8_t	Increment;
		uint8_t	High;
	}	VMA;

	uint32_t	WRAM;

	struct
	{
		uint16_t	SCBase;
		uint16_t	HOffset;
		uint16_t	VOffset;
		uint16_t	NameBase;
		uint16_t	SCSize;
		uint8_t	BGSize;
	}	BG[4];


	short	HTimerPosition;
	short	VTimerPosition;
	short	MatrixA;
	short	MatrixB;
	short	MatrixC;
	short	MatrixD;
	short	CentreX;
	short	CentreY;
	short	M7HOFS;
	short	M7VOFS;
	struct SOBJ OBJ[128]; /* int16_t alignment */
	uint16_t	OBJNameBase;
	uint16_t	OBJNameSelect;
	uint16_t	CGDATA[256];
	uint16_t	OAMAddr;
	uint16_t	SavedOAMAddr;
	uint16_t	OAMTileAddress;
	uint16_t	OAMWriteRegister;
	uint16_t	IRQHBeamPos;
	uint16_t	IRQVBeamPos;
	uint16_t	HBeamPosLatched;
	uint16_t	VBeamPosLatched;
	uint16_t	GunHLatch;
	uint16_t	GunVLatch;
	uint16_t	ScreenHeight;
	uint8_t	BGMode;
	uint8_t	BG3Priority;
	uint8_t	CGFLIPRead;
	uint8_t	CGADD;
	uint8_t	OBJSizeSelect;
	uint8_t	OAMPriorityRotation;
	uint8_t	OAMFlip;
	uint8_t	OAMReadFlip;
	uint8_t	FirstSprite;
	uint8_t	LastSprite;
	uint8_t	RangeTimeOver;
	uint8_t	OAMData[512 + 32];
	uint8_t	HBeamFlip;
	uint8_t	VBeamFlip;
	uint8_t	HVBeamCounterLatched;
	uint8_t	Mode7Repeat;
	uint8_t	Mosaic;
	uint8_t	MosaicStart;
	uint8_t	Window1Left;
	uint8_t	Window1Right;
	uint8_t	Window2Left;
	uint8_t	Window2Right;
	uint8_t	ClipCounts[6];
	uint8_t	ClipWindowOverlapLogic[6];
	uint8_t	ClipWindow1Enable[6];
	uint8_t	ClipWindow2Enable[6];
	uint8_t	FixedColourRed;
	uint8_t	FixedColourGreen;
	uint8_t	FixedColourBlue;
	uint8_t	Brightness;
	uint8_t	BGnxOFSbyte;
	uint8_t	M7byte;

	uint8_t	HDMA;
	uint8_t	HDMAEnded;

	uint8_t	OpenBus1;
	uint8_t	OpenBus2;

	uint8_t	CGFLIP;
	uint8_t	RenderSub;
	uint8_t	OBJThroughMain;
	uint8_t	OBJThroughSub;
	uint8_t	OBJAddition;
	uint8_t	HTimerEnabled;
	uint8_t	VTimerEnabled;
	uint8_t	Mode7HFlip;
	uint8_t	Mode7VFlip;
	uint8_t	BGMosaic[4];
	uint8_t	DisableMosaicHack;
	uint8_t	SFXSpeedupHack;
	uint8_t	RecomputeClipWindows;
	uint8_t	ClipWindow1Inside[6];
	uint8_t	ClipWindow2Inside[6];
	uint8_t	ForcedBlanking;
	uint8_t	Need16x8Mulitply;
	uint8_t	FullClipping;
};

extern struct SPPU		PPU;
extern struct InternalPPU	IPPU;

void S9xResetPPU (void);
void S9xResetPPUFast (void);
void S9xSoftResetPPU (void);
void S9xSetPPU (uint8_t Byte, uint16_t Address);
uint8_t S9xGetPPU (uint16_t Address);
void S9xSetCPU (uint8_t Byte, uint16_t Address);
uint8_t S9xGetCPU (uint16_t Address);
void S9xFixColourBrightness (void);
void S9xDoAutoJoypad (void);


#define MAX_5C77_VERSION	0x01
#define MAX_5C78_VERSION	0x03
#define MAX_5A22_VERSION	0x02

#define FLUSH_REDRAW() \
	if (IPPU.PreviousLine != IPPU.CurrentLine) \
		S9xUpdateScreen();

struct SGFX
{
	struct
	{
		int16_t	Tiles;
		uint8_t	RTOFlags;

		struct
		{
			int8_t	Sprite;
			uint8_t	Line;
		}	OBJ[32];
	}	OBJLines[SNES_HEIGHT_EXTENDED];

	void	(*DrawBackdropMath) (uint32_t, uint32_t, uint32_t);
	void	(*DrawBackdropNomath) (uint32_t, uint32_t, uint32_t);
	void	(*DrawTileMath) (uint32_t, uint32_t, uint32_t, uint32_t);
	void	(*DrawTileNomath) (uint32_t, uint32_t, uint32_t, uint32_t);
	void	(*DrawClippedTileMath) (uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	void	(*DrawClippedTileNomath) (uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	void	(*DrawMosaicPixelMath) (uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	void	(*DrawMosaicPixelNomath) (uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	void	(*DrawMode7BG1Math) (uint32_t, uint32_t, int);
	void	(*DrawMode7BG1Nomath) (uint32_t, uint32_t, int);
	void	(*DrawMode7BG2Math) (uint32_t, uint32_t, int);
	void	(*DrawMode7BG2Nomath) (uint32_t, uint32_t, int);
	struct ClipData	*Clip;
	uint16_t	*Screen;
	uint16_t	*SubScreen;
	uint8_t	*ZBuffer;
	uint8_t	*SubZBuffer;
	uint16_t	*S;
	uint8_t	*DB;
	uint16_t	*X2;
	uint16_t	*ZERO;
	const uint16_t	*ScreenColors;		/* screen colors for rendering main */
	const uint16_t	*RealScreenColors;	/* screen colors, ignoring color window clipping */
	uint32_t	Pitch;
	uint32_t	ScreenSize;
	uint32_t	RealPPL;		/* true PPL of Screen buffer */
	uint32_t	PPL;			/* number of pixels on each of Screen buffer */
	uint32_t	LinesPerTile;		/* number of lines in 1 tile (4 or 8 due to interlace) */
	uint32_t	FixedColour;
	uint32_t	StartY;
	uint32_t	EndY;
	uint8_t	OBJWidths[128];
	uint8_t	OBJVisibleTiles[128];
	uint8_t	Z1;			/* depth for comparison */
	uint8_t	Z2;			/* depth to save */
	uint8_t	DoInterlace;
	uint8_t	InterlaceFrame;
	uint8_t	ClipColors;
};

struct SBG
{
	uint8_t	(*ConvertTile) (uint8_t *, uint32_t, uint32_t);
	uint8_t	(*ConvertTileFlip) (uint8_t *, uint32_t, uint32_t);

	uint8_t	*Buffer;
	uint8_t	*BufferFlip;
	uint8_t	*Buffered;
	uint8_t	*BufferedFlip;

	uint32_t	TileSizeH;
	uint32_t	TileSizeV;
	uint32_t	OffsetSizeH;
	uint32_t	OffsetSizeV;
	uint32_t	TileShift;
	uint32_t	TileAddress;
	uint32_t	NameSelect;
	uint32_t	SCBase;

	uint32_t	StartPalette;
	uint32_t	PaletteShift;
	uint32_t	PaletteMask;

	uint8_t	EnableMath;
	uint8_t	InterlaceLine;

	uint8_t	DirectColourMode;
};

struct SLineData
{
	struct
	{
		uint16_t	VOffset;
		uint16_t	HOffset;
	}	BG[4];
};

struct SLineMatrixData
{
	short	MatrixA;
	short	MatrixB;
	short	MatrixC;
	short	MatrixD;
	short	CentreX;
	short	CentreY;
	short	M7HOFS;
	short	M7VOFS;
};

extern const uint8_t	mul_brightness[16][32];
extern struct SBG	BG;
extern struct SGFX	GFX;

#define H_FLIP		0x4000
#define V_FLIP		0x8000
#define BLANK_TILE	2

#define COLOR_ADD1_2(C1, C2) \
	((((((C1) & RGB_REMOVE_LOW_BITS_MASK) + \
	((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1) + \
	((C1) & (C2) & RGB_LOW_BITS_MASK)) | ALPHA_BITS_MASK)

#define COLOR_ADD(C1, C2) \
	(GFX.X2[((((C1) & RGB_REMOVE_LOW_BITS_MASK) + \
	((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1) + \
	((C1) & (C2) & RGB_LOW_BITS_MASK)] | \
	(((C1) ^ (C2)) & RGB_LOW_BITS_MASK))

#define COLOR_SUB1_2(C1, C2) \
	GFX.ZERO[(((C1) | RGB_HI_BITS_MASKx2) - \
	((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1]

/* Per-channel saturating RGB subtraction (subtractive color-math
 * path). Kept here with its three siblings so all four COLOR_*
 * macros are defined together and visible -- in the right order --
 * to every translation unit that token-pastes them (REGMATH /
 * MATHF1_2 / MATHS1_2 in tile.c expand COLOR_##Op and
 * COLOR_##Op##1_2). It previously lived in tile.c, which made its
 * visibility depend on include/definition ordering and broke some
 * builds with an implicit-declaration error for COLOR_SUB /
 * COLOR_SUB1_2.
 *
 * Plain expression macro: C1 and C2 are each referenced several
 * times, like the ADD siblings above. All call sites pass simple,
 * side-effect-free expressions (variable reads, struct members,
 * ScreenColors lookups), so the repeats are identical
 * sub-expressions the compiler folds. Do NOT pass side-effecting
 * arguments here. (Left as a macro on purpose: it is instantiated
 * across many DrawTile* template variants and must inline at every
 * site.) */
#define COLOR_SUB(C1, C2) \
	((uint16_t) (ALPHA_BITS_MASK \
		+ ((((C1) & FIRST_COLOR_MASK)  > ((C2) & FIRST_COLOR_MASK))  \
			? (uint16_t) (((C1) & FIRST_COLOR_MASK)  - ((C2) & FIRST_COLOR_MASK))  \
			: (uint16_t) 0) \
		+ ((((C1) & SECOND_COLOR_MASK) > ((C2) & SECOND_COLOR_MASK)) \
			? (uint16_t) (((C1) & SECOND_COLOR_MASK) - ((C2) & SECOND_COLOR_MASK)) \
			: (uint16_t) 0) \
		+ ((((C1) & THIRD_COLOR_MASK)  > ((C2) & THIRD_COLOR_MASK))  \
			? (uint16_t) (((C1) & THIRD_COLOR_MASK)  - ((C2) & THIRD_COLOR_MASK))  \
			: (uint16_t) 0)))

void S9xUpdateScreen (void);
void S9xMode7VertResample (void);

/* external port interface which must be initialised for each port */

uint8_t S9xGraphicsInit (void);
void S9xGraphicsDeinit (void);
void SetupOBJ (void);

extern uint8_t coldata_update_screen;

#endif
