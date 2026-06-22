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


#ifndef _DSP1_H_
#define _DSP1_H_

enum
{
	M_DSP1_LOROM_S,
	M_DSP1_LOROM_L,
	M_DSP1_HIROM,
	M_DSP2_LOROM,
	M_DSP3_LOROM,
	M_DSP4_LOROM
};

struct SDSP0
{
	uint32_t	maptype;
	uint32_t	boundary;
};

struct SDSP1
{
	uint8_t	waiting4command;
	uint8_t	first_parameter;
	uint8_t	command;
	uint32_t	in_count;
	uint32_t	in_index;
	uint32_t	out_count;
	uint32_t	out_index;
	uint8_t	parameters[512];
	uint8_t	output[512];

	int16_t	CentreX;
	int16_t	CentreY;
	int16_t	VOffset;

	int16_t	VPlane_C;
	int16_t	VPlane_E;

	/* Azimuth and Zenith angles */
	int16_t	SinAas;
	int16_t	CosAas;
	int16_t	SinAzs;
	int16_t	CosAzs;

	/* Clipped Zenith angle */
	int16_t	SinAZS;
	int16_t	CosAZS;
	int16_t	SecAZS_C1;
	int16_t	SecAZS_E1;
	int16_t	SecAZS_C2;
	int16_t	SecAZS_E2;

	int16_t	Nx;
	int16_t	Ny;
	int16_t	Nz;
	int16_t	Gx;
	int16_t	Gy;
	int16_t	Gz;
	int16_t	C_Les;
	int16_t	E_Les;
	int16_t	G_Les;

	int16_t	matrixA[3][3];
	int16_t	matrixB[3][3];
	int16_t	matrixC[3][3];

	int16_t	Op00Multiplicand;
	int16_t	Op00Multiplier;
	int16_t	Op00Result;

	int16_t	Op20Multiplicand;
	int16_t	Op20Multiplier;
	int16_t	Op20Result;

	int16_t	Op10Coefficient;
	int16_t	Op10Exponent;
	int16_t	Op10CoefficientR;
	int16_t	Op10ExponentR;

	int16_t	Op04Angle;
	int16_t	Op04Radius;
	int16_t	Op04Sin;
	int16_t	Op04Cos;

	int16_t	Op0CA;
	int16_t	Op0CX1;
	int16_t	Op0CY1;
	int16_t	Op0CX2;
	int16_t	Op0CY2;

	int16_t	Op02FX;
	int16_t	Op02FY;
	int16_t	Op02FZ;
	int16_t	Op02LFE;
	int16_t	Op02LES;
	int16_t	Op02AAS;
	int16_t	Op02AZS;
	int16_t	Op02VOF;
	int16_t	Op02VVA;
	int16_t	Op02CX;
	int16_t	Op02CY;

	int16_t	Op0AVS;
	int16_t	Op0AA;
	int16_t	Op0AB;
	int16_t	Op0AC;
	int16_t	Op0AD;

	int16_t	Op06X;
	int16_t	Op06Y;
	int16_t	Op06Z;
	int16_t	Op06H;
	int16_t	Op06V;
	int16_t	Op06M;

	int16_t	Op01m;
	int16_t	Op01Zr;
	int16_t	Op01Xr;
	int16_t	Op01Yr;

	int16_t	Op11m;
	int16_t	Op11Zr;
	int16_t	Op11Xr;
	int16_t	Op11Yr;

	int16_t	Op21m;
	int16_t	Op21Zr;
	int16_t	Op21Xr;
	int16_t	Op21Yr;

	int16_t	Op0DX;
	int16_t	Op0DY;
	int16_t	Op0DZ;
	int16_t	Op0DF;
	int16_t	Op0DL;
	int16_t	Op0DU;

	int16_t	Op1DX;
	int16_t	Op1DY;
	int16_t	Op1DZ;
	int16_t	Op1DF;
	int16_t	Op1DL;
	int16_t	Op1DU;

	int16_t	Op2DX;
	int16_t	Op2DY;
	int16_t	Op2DZ;
	int16_t	Op2DF;
	int16_t	Op2DL;
	int16_t	Op2DU;

	int16_t	Op03F;
	int16_t	Op03L;
	int16_t	Op03U;
	int16_t	Op03X;
	int16_t	Op03Y;
	int16_t	Op03Z;

	int16_t	Op13F;
	int16_t	Op13L;
	int16_t	Op13U;
	int16_t	Op13X;
	int16_t	Op13Y;
	int16_t	Op13Z;

	int16_t	Op23F;
	int16_t	Op23L;
	int16_t	Op23U;
	int16_t	Op23X;
	int16_t	Op23Y;
	int16_t	Op23Z;

	int16_t	Op14Zr;
	int16_t	Op14Xr;
	int16_t	Op14Yr;
	int16_t	Op14U;
	int16_t	Op14F;
	int16_t	Op14L;
	int16_t	Op14Zrr;
	int16_t	Op14Xrr;
	int16_t	Op14Yrr;

	int16_t	Op0EH;
	int16_t	Op0EV;
	int16_t	Op0EX;
	int16_t	Op0EY;

	int16_t	Op0BX;
	int16_t	Op0BY;
	int16_t	Op0BZ;
	int16_t	Op0BS;

	int16_t	Op1BX;
	int16_t	Op1BY;
	int16_t	Op1BZ;
	int16_t	Op1BS;

	int16_t	Op2BX;
	int16_t	Op2BY;
	int16_t	Op2BZ;
	int16_t	Op2BS;

	int16_t	Op28X;
	int16_t	Op28Y;
	int16_t	Op28Z;
	int16_t	Op28R;

	int16_t	Op1CX;
	int16_t	Op1CY;
	int16_t	Op1CZ;
	int16_t	Op1CXBR;
	int16_t	Op1CYBR;
	int16_t	Op1CZBR;
	int16_t	Op1CXAR;
	int16_t	Op1CYAR;
	int16_t	Op1CZAR;
	int16_t	Op1CX1;
	int16_t	Op1CY1;
	int16_t	Op1CZ1;
	int16_t	Op1CX2;
	int16_t	Op1CY2;
	int16_t	Op1CZ2;

	uint16_t	Op0FRamsize;
	uint16_t	Op0FPass;

	int16_t	Op2FUnknown;
	int16_t	Op2FSize;

	int16_t	Op08X;
	int16_t	Op08Y;
	int16_t	Op08Z;
	int16_t	Op08Ll;
	int16_t	Op08Lh;

	int16_t	Op18X;
	int16_t	Op18Y;
	int16_t	Op18Z;
	int16_t	Op18R;
	int16_t	Op18D;

	int16_t	Op38X;
	int16_t	Op38Y;
	int16_t	Op38Z;
	int16_t	Op38R;
	int16_t	Op38D;
};

struct SDSP2
{
	uint8_t	waiting4command;
	uint8_t	command;
	uint32_t	in_count;
	uint32_t	in_index;
	uint32_t	out_count;
	uint32_t	out_index;
	uint8_t	parameters[512];
	uint8_t	output[512];

	uint8_t	Op05HasLen;
	int32_t	Op05Len;
	uint8_t	Op05Transparent;

	uint8_t	Op06HasLen;
	int32_t	Op06Len;

	uint16_t	Op09Word1;
	uint16_t	Op09Word2;

	uint8_t	Op0DHasLen;
	int32_t	Op0DOutLen;
	int32_t	Op0DInLen;
};

struct SDSP3
{
	uint16_t	DR;
	uint16_t	SR;
	uint16_t	MemoryIndex;

	int16_t	WinLo;
	int16_t	WinHi;
	int16_t	AddLo;
	int16_t	AddHi;

	uint16_t	Codewords;
	uint16_t	Outwords;
	uint16_t	Symbol;
	uint16_t	BitCount;
	uint16_t	Index;
	uint16_t	Codes[512];
	uint16_t	BitsLeft;
	uint16_t	ReqBits;
	uint16_t	ReqData;
	uint16_t	BitCommand;
	uint8_t	BaseLength;
	uint16_t	BaseCodes;
	uint16_t	BaseCode;
	uint8_t	CodeLengths[8];
	uint16_t	CodeOffsets[8];
	uint16_t	LZCode;
	uint8_t	LZLength;

	uint16_t	X;
	uint16_t	Y;

	uint8_t	Bitmap[8];
	uint8_t	Bitplane[8];
	uint16_t	BMIndex;
	uint16_t	BPIndex;
	uint16_t	Count;

	int16_t	op3e_x;
	int16_t	op3e_y;

	int16_t	op1e_terrain[0x2000];
	int16_t	op1e_cost[0x2000];
	int16_t	op1e_weight[0x2000];

	int16_t	op1e_cell;
	int16_t	op1e_turn;
	int16_t	op1e_search;

	int16_t	op1e_x;
	int16_t	op1e_y;

	int16_t	op1e_min_radius;
	int16_t	op1e_max_radius;

	int16_t	op1e_max_search_radius;
	int16_t	op1e_max_path_radius;

	int16_t	op1e_lcv_radius;
	int16_t	op1e_lcv_steps;
	int16_t	op1e_lcv_turns;
};

struct SDSP4
{
	uint8_t	waiting4command;
	uint8_t	half_command;
	uint16_t	command;
	uint32_t	in_count;
	uint32_t	in_index;
	uint32_t	out_count;
	uint32_t	out_index;
	uint8_t	parameters[512];
	uint8_t	output[512];
	uint8_t	byte;
	uint16_t	address;

	/* op control */
	int8_t	Logic;				/* controls op flow */

	/* projection format */
	int16_t	lcv;				/* loop-control variable */
	int16_t	distance;			/* z-position into virtual world */
	int16_t	raster;				/* current raster line */
	int16_t	segments;			/* number of raster lines drawn */

	/* 1.15.16 or 1.15.0 [sign, integer, fraction] */
	int32_t	world_x;			/* line of x-projection in world */
	int32_t	world_y;			/* line of y-projection in world */
	int32_t	world_dx;			/* projection line x-delta */
	int32_t	world_dy;			/* projection line y-delta */
	int16_t	world_ddx;			/* x-delta increment */
	int16_t	world_ddy;			/* y-delta increment */
	int32_t	world_xenv;			/* world x-shaping factor */
	int16_t	world_yofs;			/* world y-vertical scroll */
	int16_t	view_x1;			/* current viewer-x */
	int16_t	view_y1;			/* current viewer-y */
	int16_t	view_x2;			/* future viewer-x */
	int16_t	view_y2;			/* future viewer-y */
	int16_t	view_dx;			/* view x-delta factor */
	int16_t	view_dy;			/* view y-delta factor */
	int16_t	view_xofs1;			/* current viewer x-vertical scroll */
	int16_t	view_yofs1;			/* current viewer y-vertical scroll */
	int16_t	view_xofs2;			/* future viewer x-vertical scroll */
	int16_t	view_yofs2;			/* future viewer y-vertical scroll */
	int16_t	view_yofsenv;			/* y-scroll shaping factor */
	int16_t	view_turnoff_x;			/* road turnoff data */
	int16_t	view_turnoff_dx;		/* road turnoff delta factor */

	/* drawing area */
	int16_t	viewport_cx;			/* x-center of viewport window */
	int16_t	viewport_cy;			/* y-center of render window */
	int16_t	viewport_left;			/* x-left of viewport */
	int16_t	viewport_right;			/* x-right of viewport */
	int16_t	viewport_top;			/* y-top of viewport */
	int16_t	viewport_bottom;		/* y-bottom of viewport */

	/* sprite structure */
	int16_t	sprite_x;			/* projected x-pos of sprite */
	int16_t	sprite_y;			/* projected y-pos of sprite */
	int16_t	sprite_attr;			/* obj attributes */
	uint8_t	sprite_size;			/* sprite size: 8x8 or 16x16 */
	int16_t	sprite_clipy;			/* visible line to clip pixels off */
	int16_t	sprite_count;

	/* generic projection variables designed for two solid polygons + two polygon sides */
	int16_t	poly_clipLf[2][2];		/* left clip boundary */
	int16_t	poly_clipRt[2][2];		/* right clip boundary */
	int16_t	poly_ptr[2][2];			/* HDMA structure pointers */
	int16_t	poly_raster[2][2];		/* current raster line below horizon */
	int16_t	poly_top[2][2];			/* top clip boundary */
	int16_t	poly_bottom[2][2];		/* bottom clip boundary */
	int16_t	poly_cx[2][2];			/* center for left/right points */
	int16_t	poly_start[2];			/* current projection points */
	int16_t	poly_plane[2];			/* previous z-plane distance */

	/* OAM */
	int16_t	OAM_attr[16];			/* OAM (size, MSB) data */
	int16_t	OAM_index;			/* index into OAM table */
	int16_t	OAM_bits;			/* offset into OAM table */
	int16_t	OAM_RowMax;			/* maximum number of tiles per 8 aligned pixels (row) */
	int16_t	OAM_Row[32];			/* current number of tiles per row */
};

extern struct SDSP0	DSP0;
extern struct SDSP1	DSP1;
extern struct SDSP2	DSP2;
extern struct SDSP3	DSP3;
extern struct SDSP4	DSP4;

uint8_t S9xGetDSP (uint16_t);
void S9xSetDSP (uint8_t, uint16_t);
void S9xResetDSP (void);
uint8_t DSP1GetByte (uint16_t);
void DSP1SetByte (uint8_t, uint16_t);
uint8_t DSP2GetByte (uint16_t);
void DSP2SetByte (uint8_t, uint16_t);
uint8_t DSP3GetByte (uint16_t);
void DSP3SetByte (uint8_t, uint16_t);
uint8_t DSP4GetByte (uint16_t);
void DSP4SetByte (uint8_t, uint16_t);
void DSP3_Reset (void);

extern uint8_t (*GetDSP) (uint16_t);
extern void (*SetDSP) (uint8_t, uint16_t);

#endif
