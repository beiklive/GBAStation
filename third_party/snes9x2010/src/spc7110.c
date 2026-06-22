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

/*****
 * SPC7110 emulator - version 0.03 (2008-08-10)
 * Copyright (c) 2008, byuu and neviksti
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * The software is provided "as is" and the author disclaims all warranties
 * with regard to this software including all implied warranties of
 * merchantibility and fitness, in no event shall the author be liable for
 * any special, direct, indirect, or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether in an
 * action of contract, negligence or other tortious action, arising out of
 * or in connection with the use or performance of this software.
 *****/

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "snes9x.h"
#include "memmap.h"
#include "getset.h"
#include "srtc.h"
#include "display.h"

#include "spc7110emu.h"
#include "spc7110dec.h"

/*read() will spool chunks half the size of SPC7110_DECOMP_BUFFER_SIZE*/
uint8_t* decomp_buffer;

static unsigned decomp_mode;
static unsigned decomp_offset;

static unsigned decomp_buffer_rdoffset;
static unsigned decomp_buffer_wroffset;
static unsigned decomp_buffer_length;

ContextState context[32];

#define memory_cartrom_read(a)		Memory.ROM[(a)]
#define memory_cartrtc_read(a)		RTCData.reg[(a)]
#define memory_cartrtc_write(a, b)	{ RTCData.reg[(a)] = (b); }

/*==================*/
/*decompression unit*/
/*==================*/
uint8_t r4801; /*compression table low*/
uint8_t r4802; /*compression table high*/
uint8_t r4803; /*compression table bank*/
uint8_t r4804; /*compression table index*/
uint8_t r4805; /*decompression buffer index low*/
uint8_t r4806; /*decompression buffer index high*/
uint8_t r4807; /*???*/
uint8_t r4808; /*???*/
uint8_t r4809; /*compression length low*/
uint8_t r480a; /*compression length high*/
uint8_t r480b; /*decompression control register*/
uint8_t r480c; /*decompression status*/

/*==============*/
/*data port unit*/
/*==============*/
uint8_t r4811; /*data pointer low*/
uint8_t r4812; /*data pointer high*/
uint8_t r4813; /*data pointer bank*/
uint8_t r4814; /*data adjust low*/
uint8_t r4815; /*data adjust high*/
uint8_t r4816; /*data increment low*/
uint8_t r4817; /*data increment high*/
uint8_t r4818; /*data port control register*/

uint8_t r481x;

uint8_t r4814_latch;
uint8_t r4815_latch;

/*=========*/
/*math unit*/
/*=========*/
uint8_t r4820; /*16-bit multiplicand B0, 32-bit dividend B0*/
uint8_t r4821; /*16-bit multiplicand B1, 32-bit dividend B1*/
uint8_t r4822; /*32-bit dividend B2*/
uint8_t r4823; /*32-bit dividend B3*/
uint8_t r4824; /*16-bit multiplier B0*/
uint8_t r4825; /*16-bit multiplier B1*/
uint8_t r4826; /*16-bit divisor B0*/
uint8_t r4827; /*16-bit divisor B1*/
uint8_t r4828; /*32-bit product B0, 32-bit quotient B0*/
uint8_t r4829; /*32-bit product B1, 32-bit quotient B1*/
uint8_t r482a; /*32-bit product B2, 32-bit quotient B2*/
uint8_t r482b; /*32-bit product B3, 32-bit quotient B3*/
uint8_t r482c; /*16-bit remainder B0*/
uint8_t r482d; /*16-bit remainder B1*/
uint8_t r482e; /*math control register*/
uint8_t r482f; /*math status*/

/*===================*/
/*memory mapping unit*/
/*===================*/
uint8_t r4830; /*SRAM write enable*/
uint8_t r4831; /*$[d0-df]:[0000-ffff] mapping*/
uint8_t r4832; /*$[e0-ef]:[0000-ffff] mapping*/
uint8_t r4833; /*$[f0-ff]:[0000-ffff] mapping*/
uint8_t r4834; /*???*/

unsigned dx_offset;
unsigned ex_offset;
unsigned fx_offset;

/*====================*/
/*real-time clock unit*/
/*====================*/
uint8_t r4840; /*RTC latch*/
uint8_t r4841; /*RTC index/data port*/
uint8_t r4842; /*RTC status*/

#define RTCS_INACTIVE 0
#define RTCS_MODESELECT 1
#define RTCS_INDEXSELECT 2
#define RTCS_WRITE 3

#define  RTCM_LINEAR 0x03
#define RTCM_INDEXED 0x0c
static uint32_t spc7110_rtc_mode;
static uint32_t rtc_state;
unsigned rtc_index;


/* Reverse Morton lookup tables.

   morton16[plane][byte] returns a 16-bit value where the 8 input bits
   are interleaved with one zero bit each: morton16[1] places input bits
   in odd output positions (15,13,...,1), morton16[0] places them in
   even positions (14,12,...,0). Adding morton16[0][lo]+morton16[1][hi]
   reconstructs a 2-plane interleaved pixel from two byte halves.

   morton32[plane][byte] is the analogous 4-plane variant: each input
   byte spreads its 8 bits across one of four bit-position groups in
   a 32-bit output (planes 3,2,1,0 occupy bits 31/23/15/7, 29/21/13/5,
   27/19/11/3, 25/17/9/1 respectively, plus the same pattern shifted
   one less for the low nibble of each input bit). Adding all four
   morton32[N][byteN] values reconstructs a 4-plane interleaved pixel.

   The values were previously computed at every spc7110_decomp_start
   call from a bit-shuffle expression. They are deterministic and
   small (6KB total), so they live in .rodata as static const. */
static const unsigned morton16[2][256] =
{
	{
		0x0000, 0x0001, 0x0100, 0x0101, 0x0002, 0x0003, 0x0102, 0x0103,
		0x0200, 0x0201, 0x0300, 0x0301, 0x0202, 0x0203, 0x0302, 0x0303,
		0x0004, 0x0005, 0x0104, 0x0105, 0x0006, 0x0007, 0x0106, 0x0107,
		0x0204, 0x0205, 0x0304, 0x0305, 0x0206, 0x0207, 0x0306, 0x0307,
		0x0400, 0x0401, 0x0500, 0x0501, 0x0402, 0x0403, 0x0502, 0x0503,
		0x0600, 0x0601, 0x0700, 0x0701, 0x0602, 0x0603, 0x0702, 0x0703,
		0x0404, 0x0405, 0x0504, 0x0505, 0x0406, 0x0407, 0x0506, 0x0507,
		0x0604, 0x0605, 0x0704, 0x0705, 0x0606, 0x0607, 0x0706, 0x0707,
		0x0008, 0x0009, 0x0108, 0x0109, 0x000a, 0x000b, 0x010a, 0x010b,
		0x0208, 0x0209, 0x0308, 0x0309, 0x020a, 0x020b, 0x030a, 0x030b,
		0x000c, 0x000d, 0x010c, 0x010d, 0x000e, 0x000f, 0x010e, 0x010f,
		0x020c, 0x020d, 0x030c, 0x030d, 0x020e, 0x020f, 0x030e, 0x030f,
		0x0408, 0x0409, 0x0508, 0x0509, 0x040a, 0x040b, 0x050a, 0x050b,
		0x0608, 0x0609, 0x0708, 0x0709, 0x060a, 0x060b, 0x070a, 0x070b,
		0x040c, 0x040d, 0x050c, 0x050d, 0x040e, 0x040f, 0x050e, 0x050f,
		0x060c, 0x060d, 0x070c, 0x070d, 0x060e, 0x060f, 0x070e, 0x070f,
		0x0800, 0x0801, 0x0900, 0x0901, 0x0802, 0x0803, 0x0902, 0x0903,
		0x0a00, 0x0a01, 0x0b00, 0x0b01, 0x0a02, 0x0a03, 0x0b02, 0x0b03,
		0x0804, 0x0805, 0x0904, 0x0905, 0x0806, 0x0807, 0x0906, 0x0907,
		0x0a04, 0x0a05, 0x0b04, 0x0b05, 0x0a06, 0x0a07, 0x0b06, 0x0b07,
		0x0c00, 0x0c01, 0x0d00, 0x0d01, 0x0c02, 0x0c03, 0x0d02, 0x0d03,
		0x0e00, 0x0e01, 0x0f00, 0x0f01, 0x0e02, 0x0e03, 0x0f02, 0x0f03,
		0x0c04, 0x0c05, 0x0d04, 0x0d05, 0x0c06, 0x0c07, 0x0d06, 0x0d07,
		0x0e04, 0x0e05, 0x0f04, 0x0f05, 0x0e06, 0x0e07, 0x0f06, 0x0f07,
		0x0808, 0x0809, 0x0908, 0x0909, 0x080a, 0x080b, 0x090a, 0x090b,
		0x0a08, 0x0a09, 0x0b08, 0x0b09, 0x0a0a, 0x0a0b, 0x0b0a, 0x0b0b,
		0x080c, 0x080d, 0x090c, 0x090d, 0x080e, 0x080f, 0x090e, 0x090f,
		0x0a0c, 0x0a0d, 0x0b0c, 0x0b0d, 0x0a0e, 0x0a0f, 0x0b0e, 0x0b0f,
		0x0c08, 0x0c09, 0x0d08, 0x0d09, 0x0c0a, 0x0c0b, 0x0d0a, 0x0d0b,
		0x0e08, 0x0e09, 0x0f08, 0x0f09, 0x0e0a, 0x0e0b, 0x0f0a, 0x0f0b,
		0x0c0c, 0x0c0d, 0x0d0c, 0x0d0d, 0x0c0e, 0x0c0f, 0x0d0e, 0x0d0f,
		0x0e0c, 0x0e0d, 0x0f0c, 0x0f0d, 0x0e0e, 0x0e0f, 0x0f0e, 0x0f0f,

	},
	{
		0x0000, 0x0010, 0x1000, 0x1010, 0x0020, 0x0030, 0x1020, 0x1030,
		0x2000, 0x2010, 0x3000, 0x3010, 0x2020, 0x2030, 0x3020, 0x3030,
		0x0040, 0x0050, 0x1040, 0x1050, 0x0060, 0x0070, 0x1060, 0x1070,
		0x2040, 0x2050, 0x3040, 0x3050, 0x2060, 0x2070, 0x3060, 0x3070,
		0x4000, 0x4010, 0x5000, 0x5010, 0x4020, 0x4030, 0x5020, 0x5030,
		0x6000, 0x6010, 0x7000, 0x7010, 0x6020, 0x6030, 0x7020, 0x7030,
		0x4040, 0x4050, 0x5040, 0x5050, 0x4060, 0x4070, 0x5060, 0x5070,
		0x6040, 0x6050, 0x7040, 0x7050, 0x6060, 0x6070, 0x7060, 0x7070,
		0x0080, 0x0090, 0x1080, 0x1090, 0x00a0, 0x00b0, 0x10a0, 0x10b0,
		0x2080, 0x2090, 0x3080, 0x3090, 0x20a0, 0x20b0, 0x30a0, 0x30b0,
		0x00c0, 0x00d0, 0x10c0, 0x10d0, 0x00e0, 0x00f0, 0x10e0, 0x10f0,
		0x20c0, 0x20d0, 0x30c0, 0x30d0, 0x20e0, 0x20f0, 0x30e0, 0x30f0,
		0x4080, 0x4090, 0x5080, 0x5090, 0x40a0, 0x40b0, 0x50a0, 0x50b0,
		0x6080, 0x6090, 0x7080, 0x7090, 0x60a0, 0x60b0, 0x70a0, 0x70b0,
		0x40c0, 0x40d0, 0x50c0, 0x50d0, 0x40e0, 0x40f0, 0x50e0, 0x50f0,
		0x60c0, 0x60d0, 0x70c0, 0x70d0, 0x60e0, 0x60f0, 0x70e0, 0x70f0,
		0x8000, 0x8010, 0x9000, 0x9010, 0x8020, 0x8030, 0x9020, 0x9030,
		0xa000, 0xa010, 0xb000, 0xb010, 0xa020, 0xa030, 0xb020, 0xb030,
		0x8040, 0x8050, 0x9040, 0x9050, 0x8060, 0x8070, 0x9060, 0x9070,
		0xa040, 0xa050, 0xb040, 0xb050, 0xa060, 0xa070, 0xb060, 0xb070,
		0xc000, 0xc010, 0xd000, 0xd010, 0xc020, 0xc030, 0xd020, 0xd030,
		0xe000, 0xe010, 0xf000, 0xf010, 0xe020, 0xe030, 0xf020, 0xf030,
		0xc040, 0xc050, 0xd040, 0xd050, 0xc060, 0xc070, 0xd060, 0xd070,
		0xe040, 0xe050, 0xf040, 0xf050, 0xe060, 0xe070, 0xf060, 0xf070,
		0x8080, 0x8090, 0x9080, 0x9090, 0x80a0, 0x80b0, 0x90a0, 0x90b0,
		0xa080, 0xa090, 0xb080, 0xb090, 0xa0a0, 0xa0b0, 0xb0a0, 0xb0b0,
		0x80c0, 0x80d0, 0x90c0, 0x90d0, 0x80e0, 0x80f0, 0x90e0, 0x90f0,
		0xa0c0, 0xa0d0, 0xb0c0, 0xb0d0, 0xa0e0, 0xa0f0, 0xb0e0, 0xb0f0,
		0xc080, 0xc090, 0xd080, 0xd090, 0xc0a0, 0xc0b0, 0xd0a0, 0xd0b0,
		0xe080, 0xe090, 0xf080, 0xf090, 0xe0a0, 0xe0b0, 0xf0a0, 0xf0b0,
		0xc0c0, 0xc0d0, 0xd0c0, 0xd0d0, 0xc0e0, 0xc0f0, 0xd0e0, 0xd0f0,
		0xe0c0, 0xe0d0, 0xf0c0, 0xf0d0, 0xe0e0, 0xe0f0, 0xf0e0, 0xf0f0,

	},
};

static const unsigned morton32[4][256] =
{
	{
		0x00000000, 0x00000001, 0x00000100, 0x00000101,
		0x00010000, 0x00010001, 0x00010100, 0x00010101,
		0x01000000, 0x01000001, 0x01000100, 0x01000101,
		0x01010000, 0x01010001, 0x01010100, 0x01010101,
		0x00000002, 0x00000003, 0x00000102, 0x00000103,
		0x00010002, 0x00010003, 0x00010102, 0x00010103,
		0x01000002, 0x01000003, 0x01000102, 0x01000103,
		0x01010002, 0x01010003, 0x01010102, 0x01010103,
		0x00000200, 0x00000201, 0x00000300, 0x00000301,
		0x00010200, 0x00010201, 0x00010300, 0x00010301,
		0x01000200, 0x01000201, 0x01000300, 0x01000301,
		0x01010200, 0x01010201, 0x01010300, 0x01010301,
		0x00000202, 0x00000203, 0x00000302, 0x00000303,
		0x00010202, 0x00010203, 0x00010302, 0x00010303,
		0x01000202, 0x01000203, 0x01000302, 0x01000303,
		0x01010202, 0x01010203, 0x01010302, 0x01010303,
		0x00020000, 0x00020001, 0x00020100, 0x00020101,
		0x00030000, 0x00030001, 0x00030100, 0x00030101,
		0x01020000, 0x01020001, 0x01020100, 0x01020101,
		0x01030000, 0x01030001, 0x01030100, 0x01030101,
		0x00020002, 0x00020003, 0x00020102, 0x00020103,
		0x00030002, 0x00030003, 0x00030102, 0x00030103,
		0x01020002, 0x01020003, 0x01020102, 0x01020103,
		0x01030002, 0x01030003, 0x01030102, 0x01030103,
		0x00020200, 0x00020201, 0x00020300, 0x00020301,
		0x00030200, 0x00030201, 0x00030300, 0x00030301,
		0x01020200, 0x01020201, 0x01020300, 0x01020301,
		0x01030200, 0x01030201, 0x01030300, 0x01030301,
		0x00020202, 0x00020203, 0x00020302, 0x00020303,
		0x00030202, 0x00030203, 0x00030302, 0x00030303,
		0x01020202, 0x01020203, 0x01020302, 0x01020303,
		0x01030202, 0x01030203, 0x01030302, 0x01030303,
		0x02000000, 0x02000001, 0x02000100, 0x02000101,
		0x02010000, 0x02010001, 0x02010100, 0x02010101,
		0x03000000, 0x03000001, 0x03000100, 0x03000101,
		0x03010000, 0x03010001, 0x03010100, 0x03010101,
		0x02000002, 0x02000003, 0x02000102, 0x02000103,
		0x02010002, 0x02010003, 0x02010102, 0x02010103,
		0x03000002, 0x03000003, 0x03000102, 0x03000103,
		0x03010002, 0x03010003, 0x03010102, 0x03010103,
		0x02000200, 0x02000201, 0x02000300, 0x02000301,
		0x02010200, 0x02010201, 0x02010300, 0x02010301,
		0x03000200, 0x03000201, 0x03000300, 0x03000301,
		0x03010200, 0x03010201, 0x03010300, 0x03010301,
		0x02000202, 0x02000203, 0x02000302, 0x02000303,
		0x02010202, 0x02010203, 0x02010302, 0x02010303,
		0x03000202, 0x03000203, 0x03000302, 0x03000303,
		0x03010202, 0x03010203, 0x03010302, 0x03010303,
		0x02020000, 0x02020001, 0x02020100, 0x02020101,
		0x02030000, 0x02030001, 0x02030100, 0x02030101,
		0x03020000, 0x03020001, 0x03020100, 0x03020101,
		0x03030000, 0x03030001, 0x03030100, 0x03030101,
		0x02020002, 0x02020003, 0x02020102, 0x02020103,
		0x02030002, 0x02030003, 0x02030102, 0x02030103,
		0x03020002, 0x03020003, 0x03020102, 0x03020103,
		0x03030002, 0x03030003, 0x03030102, 0x03030103,
		0x02020200, 0x02020201, 0x02020300, 0x02020301,
		0x02030200, 0x02030201, 0x02030300, 0x02030301,
		0x03020200, 0x03020201, 0x03020300, 0x03020301,
		0x03030200, 0x03030201, 0x03030300, 0x03030301,
		0x02020202, 0x02020203, 0x02020302, 0x02020303,
		0x02030202, 0x02030203, 0x02030302, 0x02030303,
		0x03020202, 0x03020203, 0x03020302, 0x03020303,
		0x03030202, 0x03030203, 0x03030302, 0x03030303,

	},
	{
		0x00000000, 0x00000004, 0x00000400, 0x00000404,
		0x00040000, 0x00040004, 0x00040400, 0x00040404,
		0x04000000, 0x04000004, 0x04000400, 0x04000404,
		0x04040000, 0x04040004, 0x04040400, 0x04040404,
		0x00000008, 0x0000000c, 0x00000408, 0x0000040c,
		0x00040008, 0x0004000c, 0x00040408, 0x0004040c,
		0x04000008, 0x0400000c, 0x04000408, 0x0400040c,
		0x04040008, 0x0404000c, 0x04040408, 0x0404040c,
		0x00000800, 0x00000804, 0x00000c00, 0x00000c04,
		0x00040800, 0x00040804, 0x00040c00, 0x00040c04,
		0x04000800, 0x04000804, 0x04000c00, 0x04000c04,
		0x04040800, 0x04040804, 0x04040c00, 0x04040c04,
		0x00000808, 0x0000080c, 0x00000c08, 0x00000c0c,
		0x00040808, 0x0004080c, 0x00040c08, 0x00040c0c,
		0x04000808, 0x0400080c, 0x04000c08, 0x04000c0c,
		0x04040808, 0x0404080c, 0x04040c08, 0x04040c0c,
		0x00080000, 0x00080004, 0x00080400, 0x00080404,
		0x000c0000, 0x000c0004, 0x000c0400, 0x000c0404,
		0x04080000, 0x04080004, 0x04080400, 0x04080404,
		0x040c0000, 0x040c0004, 0x040c0400, 0x040c0404,
		0x00080008, 0x0008000c, 0x00080408, 0x0008040c,
		0x000c0008, 0x000c000c, 0x000c0408, 0x000c040c,
		0x04080008, 0x0408000c, 0x04080408, 0x0408040c,
		0x040c0008, 0x040c000c, 0x040c0408, 0x040c040c,
		0x00080800, 0x00080804, 0x00080c00, 0x00080c04,
		0x000c0800, 0x000c0804, 0x000c0c00, 0x000c0c04,
		0x04080800, 0x04080804, 0x04080c00, 0x04080c04,
		0x040c0800, 0x040c0804, 0x040c0c00, 0x040c0c04,
		0x00080808, 0x0008080c, 0x00080c08, 0x00080c0c,
		0x000c0808, 0x000c080c, 0x000c0c08, 0x000c0c0c,
		0x04080808, 0x0408080c, 0x04080c08, 0x04080c0c,
		0x040c0808, 0x040c080c, 0x040c0c08, 0x040c0c0c,
		0x08000000, 0x08000004, 0x08000400, 0x08000404,
		0x08040000, 0x08040004, 0x08040400, 0x08040404,
		0x0c000000, 0x0c000004, 0x0c000400, 0x0c000404,
		0x0c040000, 0x0c040004, 0x0c040400, 0x0c040404,
		0x08000008, 0x0800000c, 0x08000408, 0x0800040c,
		0x08040008, 0x0804000c, 0x08040408, 0x0804040c,
		0x0c000008, 0x0c00000c, 0x0c000408, 0x0c00040c,
		0x0c040008, 0x0c04000c, 0x0c040408, 0x0c04040c,
		0x08000800, 0x08000804, 0x08000c00, 0x08000c04,
		0x08040800, 0x08040804, 0x08040c00, 0x08040c04,
		0x0c000800, 0x0c000804, 0x0c000c00, 0x0c000c04,
		0x0c040800, 0x0c040804, 0x0c040c00, 0x0c040c04,
		0x08000808, 0x0800080c, 0x08000c08, 0x08000c0c,
		0x08040808, 0x0804080c, 0x08040c08, 0x08040c0c,
		0x0c000808, 0x0c00080c, 0x0c000c08, 0x0c000c0c,
		0x0c040808, 0x0c04080c, 0x0c040c08, 0x0c040c0c,
		0x08080000, 0x08080004, 0x08080400, 0x08080404,
		0x080c0000, 0x080c0004, 0x080c0400, 0x080c0404,
		0x0c080000, 0x0c080004, 0x0c080400, 0x0c080404,
		0x0c0c0000, 0x0c0c0004, 0x0c0c0400, 0x0c0c0404,
		0x08080008, 0x0808000c, 0x08080408, 0x0808040c,
		0x080c0008, 0x080c000c, 0x080c0408, 0x080c040c,
		0x0c080008, 0x0c08000c, 0x0c080408, 0x0c08040c,
		0x0c0c0008, 0x0c0c000c, 0x0c0c0408, 0x0c0c040c,
		0x08080800, 0x08080804, 0x08080c00, 0x08080c04,
		0x080c0800, 0x080c0804, 0x080c0c00, 0x080c0c04,
		0x0c080800, 0x0c080804, 0x0c080c00, 0x0c080c04,
		0x0c0c0800, 0x0c0c0804, 0x0c0c0c00, 0x0c0c0c04,
		0x08080808, 0x0808080c, 0x08080c08, 0x08080c0c,
		0x080c0808, 0x080c080c, 0x080c0c08, 0x080c0c0c,
		0x0c080808, 0x0c08080c, 0x0c080c08, 0x0c080c0c,
		0x0c0c0808, 0x0c0c080c, 0x0c0c0c08, 0x0c0c0c0c,

	},
	{
		0x00000000, 0x00000010, 0x00001000, 0x00001010,
		0x00100000, 0x00100010, 0x00101000, 0x00101010,
		0x10000000, 0x10000010, 0x10001000, 0x10001010,
		0x10100000, 0x10100010, 0x10101000, 0x10101010,
		0x00000020, 0x00000030, 0x00001020, 0x00001030,
		0x00100020, 0x00100030, 0x00101020, 0x00101030,
		0x10000020, 0x10000030, 0x10001020, 0x10001030,
		0x10100020, 0x10100030, 0x10101020, 0x10101030,
		0x00002000, 0x00002010, 0x00003000, 0x00003010,
		0x00102000, 0x00102010, 0x00103000, 0x00103010,
		0x10002000, 0x10002010, 0x10003000, 0x10003010,
		0x10102000, 0x10102010, 0x10103000, 0x10103010,
		0x00002020, 0x00002030, 0x00003020, 0x00003030,
		0x00102020, 0x00102030, 0x00103020, 0x00103030,
		0x10002020, 0x10002030, 0x10003020, 0x10003030,
		0x10102020, 0x10102030, 0x10103020, 0x10103030,
		0x00200000, 0x00200010, 0x00201000, 0x00201010,
		0x00300000, 0x00300010, 0x00301000, 0x00301010,
		0x10200000, 0x10200010, 0x10201000, 0x10201010,
		0x10300000, 0x10300010, 0x10301000, 0x10301010,
		0x00200020, 0x00200030, 0x00201020, 0x00201030,
		0x00300020, 0x00300030, 0x00301020, 0x00301030,
		0x10200020, 0x10200030, 0x10201020, 0x10201030,
		0x10300020, 0x10300030, 0x10301020, 0x10301030,
		0x00202000, 0x00202010, 0x00203000, 0x00203010,
		0x00302000, 0x00302010, 0x00303000, 0x00303010,
		0x10202000, 0x10202010, 0x10203000, 0x10203010,
		0x10302000, 0x10302010, 0x10303000, 0x10303010,
		0x00202020, 0x00202030, 0x00203020, 0x00203030,
		0x00302020, 0x00302030, 0x00303020, 0x00303030,
		0x10202020, 0x10202030, 0x10203020, 0x10203030,
		0x10302020, 0x10302030, 0x10303020, 0x10303030,
		0x20000000, 0x20000010, 0x20001000, 0x20001010,
		0x20100000, 0x20100010, 0x20101000, 0x20101010,
		0x30000000, 0x30000010, 0x30001000, 0x30001010,
		0x30100000, 0x30100010, 0x30101000, 0x30101010,
		0x20000020, 0x20000030, 0x20001020, 0x20001030,
		0x20100020, 0x20100030, 0x20101020, 0x20101030,
		0x30000020, 0x30000030, 0x30001020, 0x30001030,
		0x30100020, 0x30100030, 0x30101020, 0x30101030,
		0x20002000, 0x20002010, 0x20003000, 0x20003010,
		0x20102000, 0x20102010, 0x20103000, 0x20103010,
		0x30002000, 0x30002010, 0x30003000, 0x30003010,
		0x30102000, 0x30102010, 0x30103000, 0x30103010,
		0x20002020, 0x20002030, 0x20003020, 0x20003030,
		0x20102020, 0x20102030, 0x20103020, 0x20103030,
		0x30002020, 0x30002030, 0x30003020, 0x30003030,
		0x30102020, 0x30102030, 0x30103020, 0x30103030,
		0x20200000, 0x20200010, 0x20201000, 0x20201010,
		0x20300000, 0x20300010, 0x20301000, 0x20301010,
		0x30200000, 0x30200010, 0x30201000, 0x30201010,
		0x30300000, 0x30300010, 0x30301000, 0x30301010,
		0x20200020, 0x20200030, 0x20201020, 0x20201030,
		0x20300020, 0x20300030, 0x20301020, 0x20301030,
		0x30200020, 0x30200030, 0x30201020, 0x30201030,
		0x30300020, 0x30300030, 0x30301020, 0x30301030,
		0x20202000, 0x20202010, 0x20203000, 0x20203010,
		0x20302000, 0x20302010, 0x20303000, 0x20303010,
		0x30202000, 0x30202010, 0x30203000, 0x30203010,
		0x30302000, 0x30302010, 0x30303000, 0x30303010,
		0x20202020, 0x20202030, 0x20203020, 0x20203030,
		0x20302020, 0x20302030, 0x20303020, 0x20303030,
		0x30202020, 0x30202030, 0x30203020, 0x30203030,
		0x30302020, 0x30302030, 0x30303020, 0x30303030,

	},
	{
		0x00000000, 0x00000040, 0x00004000, 0x00004040,
		0x00400000, 0x00400040, 0x00404000, 0x00404040,
		0x40000000, 0x40000040, 0x40004000, 0x40004040,
		0x40400000, 0x40400040, 0x40404000, 0x40404040,
		0x00000080, 0x000000c0, 0x00004080, 0x000040c0,
		0x00400080, 0x004000c0, 0x00404080, 0x004040c0,
		0x40000080, 0x400000c0, 0x40004080, 0x400040c0,
		0x40400080, 0x404000c0, 0x40404080, 0x404040c0,
		0x00008000, 0x00008040, 0x0000c000, 0x0000c040,
		0x00408000, 0x00408040, 0x0040c000, 0x0040c040,
		0x40008000, 0x40008040, 0x4000c000, 0x4000c040,
		0x40408000, 0x40408040, 0x4040c000, 0x4040c040,
		0x00008080, 0x000080c0, 0x0000c080, 0x0000c0c0,
		0x00408080, 0x004080c0, 0x0040c080, 0x0040c0c0,
		0x40008080, 0x400080c0, 0x4000c080, 0x4000c0c0,
		0x40408080, 0x404080c0, 0x4040c080, 0x4040c0c0,
		0x00800000, 0x00800040, 0x00804000, 0x00804040,
		0x00c00000, 0x00c00040, 0x00c04000, 0x00c04040,
		0x40800000, 0x40800040, 0x40804000, 0x40804040,
		0x40c00000, 0x40c00040, 0x40c04000, 0x40c04040,
		0x00800080, 0x008000c0, 0x00804080, 0x008040c0,
		0x00c00080, 0x00c000c0, 0x00c04080, 0x00c040c0,
		0x40800080, 0x408000c0, 0x40804080, 0x408040c0,
		0x40c00080, 0x40c000c0, 0x40c04080, 0x40c040c0,
		0x00808000, 0x00808040, 0x0080c000, 0x0080c040,
		0x00c08000, 0x00c08040, 0x00c0c000, 0x00c0c040,
		0x40808000, 0x40808040, 0x4080c000, 0x4080c040,
		0x40c08000, 0x40c08040, 0x40c0c000, 0x40c0c040,
		0x00808080, 0x008080c0, 0x0080c080, 0x0080c0c0,
		0x00c08080, 0x00c080c0, 0x00c0c080, 0x00c0c0c0,
		0x40808080, 0x408080c0, 0x4080c080, 0x4080c0c0,
		0x40c08080, 0x40c080c0, 0x40c0c080, 0x40c0c0c0,
		0x80000000, 0x80000040, 0x80004000, 0x80004040,
		0x80400000, 0x80400040, 0x80404000, 0x80404040,
		0xc0000000, 0xc0000040, 0xc0004000, 0xc0004040,
		0xc0400000, 0xc0400040, 0xc0404000, 0xc0404040,
		0x80000080, 0x800000c0, 0x80004080, 0x800040c0,
		0x80400080, 0x804000c0, 0x80404080, 0x804040c0,
		0xc0000080, 0xc00000c0, 0xc0004080, 0xc00040c0,
		0xc0400080, 0xc04000c0, 0xc0404080, 0xc04040c0,
		0x80008000, 0x80008040, 0x8000c000, 0x8000c040,
		0x80408000, 0x80408040, 0x8040c000, 0x8040c040,
		0xc0008000, 0xc0008040, 0xc000c000, 0xc000c040,
		0xc0408000, 0xc0408040, 0xc040c000, 0xc040c040,
		0x80008080, 0x800080c0, 0x8000c080, 0x8000c0c0,
		0x80408080, 0x804080c0, 0x8040c080, 0x8040c0c0,
		0xc0008080, 0xc00080c0, 0xc000c080, 0xc000c0c0,
		0xc0408080, 0xc04080c0, 0xc040c080, 0xc040c0c0,
		0x80800000, 0x80800040, 0x80804000, 0x80804040,
		0x80c00000, 0x80c00040, 0x80c04000, 0x80c04040,
		0xc0800000, 0xc0800040, 0xc0804000, 0xc0804040,
		0xc0c00000, 0xc0c00040, 0xc0c04000, 0xc0c04040,
		0x80800080, 0x808000c0, 0x80804080, 0x808040c0,
		0x80c00080, 0x80c000c0, 0x80c04080, 0x80c040c0,
		0xc0800080, 0xc08000c0, 0xc0804080, 0xc08040c0,
		0xc0c00080, 0xc0c000c0, 0xc0c04080, 0xc0c040c0,
		0x80808000, 0x80808040, 0x8080c000, 0x8080c040,
		0x80c08000, 0x80c08040, 0x80c0c000, 0x80c0c040,
		0xc0808000, 0xc0808040, 0xc080c000, 0xc080c040,
		0xc0c08000, 0xc0c08040, 0xc0c0c000, 0xc0c0c040,
		0x80808080, 0x808080c0, 0x8080c080, 0x8080c0c0,
		0x80c08080, 0x80c080c0, 0x80c0c080, 0x80c0c0c0,
		0xc0808080, 0xc08080c0, 0xc080c080, 0xc080c0c0,
		0xc0c08080, 0xc0c080c0, 0xc0c0c080, 0xc0c0c0c0,

	},
};

static const uint8_t evolution_table[53][4] =
{
	/*{ prob, nextlps, nextmps, toggle invert },*/

	{ 0x5a,  1,  1, 1 },
	{ 0x25,  6,  2, 0 },
	{ 0x11,  8,  3, 0 },
	{ 0x08, 10,  4, 0 },
	{ 0x03, 12,  5, 0 },
	{ 0x01, 15,  5, 0 },

	{ 0x5a,  7,  7, 1 },
	{ 0x3f, 19,  8, 0 },
	{ 0x2c, 21,  9, 0 },
	{ 0x20, 22, 10, 0 },
	{ 0x17, 23, 11, 0 },
	{ 0x11, 25, 12, 0 },
	{ 0x0c, 26, 13, 0 },
	{ 0x09, 28, 14, 0 },
	{ 0x07, 29, 15, 0 },
	{ 0x05, 31, 16, 0 },
	{ 0x04, 32, 17, 0 },
	{ 0x03, 34, 18, 0 },
	{ 0x02, 35,  5, 0 },

	{ 0x5a, 20, 20, 1 },
	{ 0x48, 39, 21, 0 },
	{ 0x3a, 40, 22, 0 },
	{ 0x2e, 42, 23, 0 },
	{ 0x26, 44, 24, 0 },
	{ 0x1f, 45, 25, 0 },
	{ 0x19, 46, 26, 0 },
	{ 0x15, 25, 27, 0 },
	{ 0x11, 26, 28, 0 },
	{ 0x0e, 26, 29, 0 },
	{ 0x0b, 27, 30, 0 },
	{ 0x09, 28, 31, 0 },
	{ 0x08, 29, 32, 0 },
	{ 0x07, 30, 33, 0 },
	{ 0x05, 31, 34, 0 },
	{ 0x04, 33, 35, 0 },
	{ 0x04, 33, 36, 0 },
	{ 0x03, 34, 37, 0 },
	{ 0x02, 35, 38, 0 },
	{ 0x02, 36,  5, 0 },

	{ 0x58, 39, 40, 1 },
	{ 0x4d, 47, 41, 0 },
	{ 0x43, 48, 42, 0 },
	{ 0x3b, 49, 43, 0 },
	{ 0x34, 50, 44, 0 },
	{ 0x2e, 51, 45, 0 },
	{ 0x29, 44, 46, 0 },
	{ 0x25, 45, 24, 0 },

	{ 0x56, 47, 48, 1 },
	{ 0x4f, 47, 49, 0 },
	{ 0x47, 48, 50, 0 },
	{ 0x41, 49, 51, 0 },
	{ 0x3c, 50, 52, 0 },
	{ 0x37, 51, 43, 0 },
};

const uint8_t mode2_context_table[32][2] = {
/*{ next 0, next 1 },*/

  {  1,  2 },

  {  3,  8 },
  { 13, 14 },

  { 15, 16 },
  { 17, 18 },
  { 19, 20 },
  { 21, 22 },
  { 23, 24 },
  { 25, 26 },
  { 25, 26 },
  { 25, 26 },
  { 25, 26 },
  { 25, 26 },
  { 27, 28 },
  { 29, 30 },

  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },
  { 31, 31 },

  { 31, 31 },
};

/*reverse morton lookup: de-interleave two 8-bit values*/
/*15, 13, 11,  9,  7,  5,  3,  1 -> 15- 8*/
/*14, 12, 10,  8,  6,  4,  2,  0 ->  7- 0*/
#define MORTON_2X8(data) (morton16[0][(data >>  0) & 255] + morton16[1][(data >>  8) & 255])

/*reverse morton lookup: de-interleave four 8-bit values*/
/*31, 27, 23, 19, 15, 11,  7,  3 -> 31-24*/
/*30, 26, 22, 18, 14, 10,  6,  2 -> 23-16*/
/*29, 25, 21, 17, 13,  9,  5,  1 -> 15- 8*/
/*28, 24, 20, 16, 12,  8,  4,  0 ->  7- 0*/
#define MORTON_4X8(data) (morton32[0][(data >>  0) & 255] + morton32[1][(data >>  8) & 255] + morton32[2][(data >> 16) & 255] + morton32[3][(data >> 24) & 255])

#define PROBABILITY(n) (evolution_table[context[n].index][0])
#define NEXT_LPS(n) (evolution_table[context[n].index][1])
#define NEXT_MPS(n) (evolution_table[context[n].index][2])
#define TOGGLE_INVERT(n) (evolution_table[context[n].index][3])

static uint8_t spc7110_decomp_dataread (void)
{
	unsigned size = Memory.CalculatedSize - 0x100000;
	while(decomp_offset >= size)
		decomp_offset -= size;
	return memory_cartrom_read(0x100000 + decomp_offset++);
}

void spc7110_decomp_write(uint8_t data)
{
	decomp_buffer[decomp_buffer_wroffset++] = data;
	decomp_buffer_wroffset &= SPC7110_DECOMP_BUFFER_SIZE - 1;
	decomp_buffer_length++;
}

static void spc7110_decomp_mode2(uint8_t init)
{
	unsigned i, pixel, data;
	static unsigned pixelorder[16], realorder[16];
	static uint8_t bitplanebuffer[16], buffer_index;
	static uint8_t in, val, span;
	static int out0, out1, inverts, lps, in_count;

	if(init == TRUE)
	{
		for( i = 0; i < 16; i++)
			pixelorder[i] = i;

		buffer_index = 0;
		out0 = out1 = inverts = lps = 0;
		span = 0xff;
		val = spc7110_decomp_dataread();
		in = spc7110_decomp_dataread();
		in_count = 8;
		return;
	}

	while(decomp_buffer_length < (SPC7110_DECOMP_BUFFER_SIZE >> 1))
	{
		unsigned bit, a, b, c, con, refcon, m, n, prob, flag_lps,
		invertbit, shift;
		for( pixel = 0; pixel < 8; pixel++)
		{
			/*get first symbol context*/
			a = ((out0 >> (0 * 4)) & 15);
			b = ((out0 >> (7 * 4)) & 15);
			c = ((out1 >> (0 * 4)) & 15);
			con = 0;
			refcon = (a == b) ? (b != c) : (b == c) ? 2 : 4 - (a == c);

			/*update pixel order*/
			for(m = 0; m < 16; m++)
				if(pixelorder[m] == a)
					break;
			for(n = m; n >  0; n--)
				pixelorder[n] = pixelorder[n - 1];
			pixelorder[0] = a;

			/*calculate the real pixel order*/
			for(m = 0; m < 16; m++)
				realorder[m] = pixelorder[m];

			/*rotate reference pixel c value to top*/
			for(m = 0; m < 16; m++)
				if(realorder[m] == c)
					break;
			for(n = m; n >  0; n--) realorder[n] = realorder[n - 1];
			realorder[0] = c;

			/*rotate reference pixel b value to top*/
			for(m = 0; m < 16; m++) if(realorder[m] == b) break;
			for(n = m; n >  0; n--) realorder[n] = realorder[n - 1];
			realorder[0] = b;

			/*rotate reference pixel a value to top*/
			for(m = 0; m < 16; m++) if(realorder[m] == a) break;
			for(n = m; n >  0; n--) realorder[n] = realorder[n - 1];
			realorder[0] = a;

			/*get 4 symbols*/
			for( bit = 0; bit < 4; bit++)
			{
				/*get prob*/
				prob = PROBABILITY(con);

				/*get symbol*/
				if(val <= span - prob) { /*mps*/
					span = span - prob;
					flag_lps = 0;
				} else { /*lps*/
					val = val - (span - (prob - 1));
					span = prob - 1;
					flag_lps = 1;
				}

				/*renormalize*/
				shift = 0;
				while(span < 0x7f) {
					shift++;

					span = (span << 1) + 1;
					val = (val << 1) + (in >> 7);

					in <<= 1;
					if(--in_count == 0) {
						in = spc7110_decomp_dataread();
						in_count = 8;
					}
				}

				/*update processing info*/
				lps = (lps << 1) + flag_lps;
				invertbit = context[con].invert;
				inverts = (inverts << 1) + invertbit;

				/*update context state*/
				if(flag_lps & TOGGLE_INVERT(con)) context[con].invert ^= 1;
				if(flag_lps) context[con].index = NEXT_LPS(con);
				else if(shift) context[con].index = NEXT_MPS(con);

				/*get next context*/
				con = mode2_context_table[con][flag_lps ^ invertbit] + (con == 1 ? refcon : 0);
			}

			/*get pixel*/
			b = realorder[(lps ^ inverts) & 0x0f];
			out1 = (out1 << 4) + ((out0 >> 28) & 0x0f);
			out0 = (out0 << 4) + b;
		}

		/*convert pixel data into bitplanes*/
		data = MORTON_4X8(out0);
		spc7110_decomp_write(data >> 24);
		spc7110_decomp_write(data >> 16);
		bitplanebuffer[buffer_index++] = data >> 8;
		bitplanebuffer[buffer_index++] = data;

		if(buffer_index == 16)
		{
			for( i = 0; i < 16; i++)
				spc7110_decomp_write(bitplanebuffer[i]);
			buffer_index = 0;
		}
	}
}

void spc7110_decomp_mode1(uint8_t init)
{
	unsigned i, bit, pixel, data;
	static unsigned pixelorder[4], realorder[4];
	static uint8_t in, val, span;
	static int out, inverts, lps, in_count;

	if(init == TRUE)
	{
		for( i = 0; i < 4; i++) pixelorder[i] = i;
		out = inverts = lps = 0;
		span = 0xff;
		val = spc7110_decomp_dataread();
		in = spc7110_decomp_dataread();
		in_count = 8;
		return;
	}

	while(decomp_buffer_length < (SPC7110_DECOMP_BUFFER_SIZE >> 1))
	{
		for( pixel = 0; pixel < 8; pixel++)
		{
			unsigned a, b, c, con, m, n, prob, flag_lps, shift;
			/*get first symbol context*/
			a = ((out >> (1 * 2)) & 3);
			b = ((out >> (7 * 2)) & 3);
			c = ((out >> (8 * 2)) & 3);
			con = (a == b) ? (b != c) : (b == c) ? 2 : 4 - (a == c);

			/*update pixel order*/
			for(m = 0; m < 4; m++) if(pixelorder[m] == a) break;
			for(n = m; n > 0; n--) pixelorder[n] = pixelorder[n - 1];
			pixelorder[0] = a;

			/*calculate the real pixel order*/
			for(m = 0; m < 4; m++) realorder[m] = pixelorder[m];

			/*rotate reference pixel c value to top*/
			for(m = 0; m < 4; m++) if(realorder[m] == c) break;
			for(n = m; n > 0; n--) realorder[n] = realorder[n - 1];
			realorder[0] = c;

			/*rotate reference pixel b value to top*/
			for(m = 0; m < 4; m++) if(realorder[m] == b) break;
			for(n = m; n > 0; n--) realorder[n] = realorder[n - 1];
			realorder[0] = b;

			/*rotate reference pixel a value to top*/
			for(m = 0; m < 4; m++) if(realorder[m] == a) break;
			for(n = m; n > 0; n--) realorder[n] = realorder[n - 1];
			realorder[0] = a;

			/*get 2 symbols*/
			for( bit = 0; bit < 2; bit++)
			{
				/*get prob*/
				prob = PROBABILITY(con);

				/*get symbol*/
				if(val <= span - prob) { /*mps*/
					span = span - prob;
					flag_lps = 0;
				} else { /*lps*/
					val = val - (span - (prob - 1));
					span = prob - 1;
					flag_lps = 1;
				}

				/*renormalize*/
				shift = 0;
				while(span < 0x7f) {
					shift++;

					span = (span << 1) + 1;
					val = (val << 1) + (in >> 7);

					in <<= 1;
					if(--in_count == 0)
					{
						in = spc7110_decomp_dataread();
						in_count = 8;
					}
				}

				/*update processing info*/
				lps = (lps << 1) + flag_lps;
				inverts = (inverts << 1) + context[con].invert;

				/*update context state*/
				if(flag_lps & TOGGLE_INVERT(con))
					context[con].invert ^= 1;
				if(flag_lps)
					context[con].index = NEXT_LPS(con);
				else if(shift)
					context[con].index = NEXT_MPS(con);

				/*get next context*/
				con = 5 + (con << 1) + ((lps ^ inverts) & 1);
			}

			/*get pixel*/
			b = realorder[(lps ^ inverts) & 3];
			out = (out << 2) + b;
		}

		/*turn pixel data into bitplanes*/
		data = MORTON_2X8(out);
		spc7110_decomp_write(data >> 8);
		spc7110_decomp_write(data);
	}
}

static void spc7110_decomp_mode0(uint8_t init)
{
	unsigned bit;
	static uint8_t val, in, span;
	static int out, inverts, lps, in_count;

	if(init == TRUE)
	{
		out = inverts = lps = 0;
		span = 0xff;
		val = spc7110_decomp_dataread();
		in = spc7110_decomp_dataread();
		in_count = 8;
		return;
	}

	while(decomp_buffer_length < (SPC7110_DECOMP_BUFFER_SIZE >> 1))
	{
		for( bit = 0; bit < 8; bit++)
		{
			unsigned prob, mps, flag_lps, shift;
			/*get context*/
			uint8_t mask = (1 << (bit & 3)) - 1;
			uint8_t con = mask + ((inverts & mask) ^ (lps & mask));
			if(bit > 3) con += 15;

			/*get prob and mps*/
			prob = PROBABILITY(con);
			mps = (((out >> 15) & 1) ^ context[con].invert);

			/*get bit*/
			if(val <= span - prob) { /*mps*/
				span = span - prob;
				out = (out << 1) + mps;
				flag_lps = 0;
			} else { /*lps*/
				val = val - (span - (prob - 1));
				span = prob - 1;
				out = (out << 1) + 1 - mps;
				flag_lps = 1;
			}

			/*renormalize*/
			shift = 0;
			while(span < 0x7f) {
				shift++;

				span = (span << 1) + 1;
				val = (val << 1) + (in >> 7);

				in <<= 1;
				if(--in_count == 0) {
					in = spc7110_decomp_dataread();
					in_count = 8;
				}
			}

			/*update processing info*/
			lps = (lps << 1) + flag_lps;
			inverts = (inverts << 1) + context[con].invert;

			/*update context state*/
			if(flag_lps & TOGGLE_INVERT(con))
				context[con].invert ^= 1;
			if(flag_lps)
				context[con].index = NEXT_LPS(con);
			else if(shift)
				context[con].index = NEXT_MPS(con);
		}

		/*save byte*/
		spc7110_decomp_write(out);
	}
}

uint8_t spc7110_decomp_read (void)
{
	uint8_t data;

	if(decomp_buffer_length == 0)
	{
		/*decompress at least (SPC7110_DECOMP_BUFFER_SIZE / 2) bytes to the buffer*/
		switch(decomp_mode)
		{
			case 0:
				spc7110_decomp_mode0(FALSE);
				break;
			case 1:
				spc7110_decomp_mode1(FALSE);
				break;
			case 2:
				spc7110_decomp_mode2(FALSE);
				break;
			default:
				return 0x00;
		}
	}

	data = decomp_buffer[decomp_buffer_rdoffset++];
	decomp_buffer_rdoffset &= SPC7110_DECOMP_BUFFER_SIZE - 1;
	decomp_buffer_length--;
	return data;
}

static void spc7110_decomp_init(unsigned mode, unsigned offset, unsigned index)
{
	unsigned i;
	decomp_mode = mode;
	decomp_offset = offset;

	decomp_buffer_rdoffset = 0;
	decomp_buffer_wroffset = 0;
	decomp_buffer_length   = 0;

	/*reset context states*/
	for( i = 0; i < 32; i++)
	{
		context[i].index  = 0;
		context[i].invert = 0;
	}

	switch(decomp_mode)
	{
		case 0:
			spc7110_decomp_mode0(TRUE);
			break;
		case 1:
			spc7110_decomp_mode1(TRUE);
			break;
		case 2:
			spc7110_decomp_mode2(TRUE);
			break;
	}

	/*decompress up to requested output data index*/
	while(index--)
		spc7110_decomp_read();
}

void spc7110_decomp_reset (void)
{
	/*mode 3 is invalid; this is treated as a special case to always return 0x00*/
	/*set to mode 3 so that reading decomp port before starting first decomp will return 0x00*/
	decomp_mode = 3;

	decomp_buffer_rdoffset = 0;
	decomp_buffer_wroffset = 0;
	decomp_buffer_length   = 0;
}

void spc7110_decomp_start (void)
{
	if (decomp_buffer)
		free(decomp_buffer);
	decomp_buffer = (uint8_t*)malloc(SPC7110_DECOMP_BUFFER_SIZE);
	spc7110_decomp_reset();
}

static const unsigned months[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

unsigned s7_datarom_addr(unsigned addr)
{
	unsigned size = Memory.CalculatedSize - 0x100000;
	while(addr >= size)
		addr -= size;
	return addr + 0x100000;
}

unsigned s7_data_adjust (void)
{
	return r4814 + (r4815 << 8);
}

static unsigned s7_data_pointer (void)
{
	return r4811 + (r4812 << 8) + (r4813 << 16);
}

static void s7_set_data_pointer(unsigned addr)
{
	r4811 = addr;
	r4812 = addr >> 8;
	r4813 = addr >> 16;
}

static void s7_update_time(int offset)
{
	uint8_t update;
	time_t rtc_time, current_time, diff;

	rtc_time = (memory_cartrtc_read(16) <<  0)
		| (memory_cartrtc_read(17) <<  8)
		| (memory_cartrtc_read(18) << 16)
		| (memory_cartrtc_read(19) << 24);

	current_time = time(0) - offset;

	/*sizeof(time_t) is platform-dependent; though memory::cartrtc needs to be platform-agnostic.*/
	/*yet platforms with 32-bit signed time_t will overflow every ~68 years. handle this by*/
	/*accounting for overflow at the cost of 1-bit precision (to catch underflow). this will allow*/
	/*memory::cartrtc timestamp to remain valid for up to ~34 years from the last update, even if*/
	/*time_t overflows. calculation should be valid regardless of number representation, time_t size,*/
	/*or whether time_t is signed or unsigned.*/
	diff = (current_time >= rtc_time)
		? (current_time - rtc_time)
		: ((time_t)-1 - rtc_time + current_time + 1);  /*compensate for overflow*/
	if(diff > (time_t)-1 / 2)
		diff = 0;            /*compensate for underflow*/

	update = TRUE;

	if(memory_cartrtc_read(13) & 1)
		update = FALSE;  /*do not update if CR0 timer disable flag is set*/
	if(memory_cartrtc_read(15) & 3)
		update = FALSE;  /*do not update if CR2 timer disable flags are set*/

	if(diff > 0 && update == TRUE)
	{
		unsigned second, minute, hour, day, days, month, year, weekday;
		uint8_t leapyear;

		second  = memory_cartrtc_read( 0) + memory_cartrtc_read( 1) * 10;
		minute  = memory_cartrtc_read( 2) + memory_cartrtc_read( 3) * 10;
		hour    = memory_cartrtc_read( 4) + memory_cartrtc_read( 5) * 10;
		day     = memory_cartrtc_read( 6) + memory_cartrtc_read( 7) * 10;
		month   = memory_cartrtc_read( 8) + memory_cartrtc_read( 9) * 10;
		year    = memory_cartrtc_read(10) + memory_cartrtc_read(11) * 10;
		weekday = memory_cartrtc_read(12);

		day--;
		month--;
		year += (year >= 90) ? 1900 : 2000;  /*range = 1990-2089*/

		second += diff;
		while(second >= 60)
		{
			second -= 60;

			minute++;
			if(minute < 60)
				continue;
			minute = 0;

			hour++;
			if(hour < 24)
				continue;
			hour = 0;

			day++;
			weekday = (weekday + 1) % 7;
			days = months[month % 12];
			if(days == 28)
			{
				leapyear = FALSE;
				if((year % 4) == 0)
				{
					leapyear = TRUE;
					if((year % 100) == 0 && (year % 400) != 0)
						leapyear = FALSE;
				}

				if(leapyear)
					days++;
			}
			if(day < days)
				continue;
			day = 0;

			month++;
			if(month < 12)
				continue;
			month = 0;

			year++;
		}

		day++;
		month++;
		year %= 100;

		memory_cartrtc_write( 0, second % 10);
		memory_cartrtc_write( 1, second / 10);
		memory_cartrtc_write( 2, minute % 10);
		memory_cartrtc_write( 3, minute / 10);
		memory_cartrtc_write( 4, hour % 10);
		memory_cartrtc_write( 5, hour / 10);
		memory_cartrtc_write( 6, day % 10);
		memory_cartrtc_write( 7, day / 10);
		memory_cartrtc_write( 8, month % 10);
		memory_cartrtc_write( 9, month / 10);
		memory_cartrtc_write(10, year % 10);
		memory_cartrtc_write(11, (year / 10) % 10);
		memory_cartrtc_write(12, weekday % 7);
	}

	memory_cartrtc_write(16, current_time >>  0);
	memory_cartrtc_write(17, current_time >>  8);
	memory_cartrtc_write(18, current_time >> 16);
	memory_cartrtc_write(19, current_time >> 24);
}

static void s7_mmio_write(unsigned addr, uint8_t data)
{
	addr &= 0xffff;

	switch(addr)
	{
		/*==================*/
		/*decompression unit*/
		/*==================*/

		case 0x4801: r4801 = data; break;
		case 0x4802: r4802 = data; break;
		case 0x4803: r4803 = data; break;
		case 0x4804: r4804 = data; break;
		case 0x4805: r4805 = data; break;
		case 0x4806:
			     {
				     unsigned table, index, addr, mode, offset;
				     r4806 = data;

				     table   = (r4801 + (r4802 << 8) + (r4803 << 16));
				     index   = (r4804 << 2);
				     /*unsigned length  = (r4809 + (r480a << 8));*/
				     addr    = s7_datarom_addr(table + index);
				     mode    = (memory_cartrom_read(addr + 0));
				     offset  = (memory_cartrom_read(addr + 1) << 16)
					     + (memory_cartrom_read(addr + 2) <<  8)
					     + (memory_cartrom_read(addr + 3) <<  0);

				     spc7110_decomp_init(mode, offset, (r4805 + (r4806 << 8)) << mode);
				     r480c = 0x80;
			     } break;

		case 0x4807: r4807 = data; break;
		case 0x4808: r4808 = data; break;
		case 0x4809: r4809 = data; break;
		case 0x480a: r480a = data; break;
		case 0x480b: r480b = data; break;

			     /*==============*/
			     /*data port unit*/
			     /*==============*/

		case 0x4811:
			r4811 = data;
			r481x |= 0x01;
			break;
		case 0x4812:
			r4812 = data;
			r481x |= 0x02;
			break;
		case 0x4813:
			r4813 = data;
			r481x |= 0x04;
			break;
		case 0x4814:
			{
				unsigned increment;

				r4814 = data;
				r4814_latch = TRUE;
				if(!r4815_latch)
					break;
				if(!(r4818 & 2))
					break;
				if(r4818 & 0x10)
					break;

				if((r4818 & 0x60) == 0x20)
				{
					increment = s7_data_adjust() & 0xff;
					if(r4818 & 8)
						increment = (int8_t)increment;  /*8-bit sign extend*/
					s7_set_data_pointer(s7_data_pointer() + increment);
				}
				else if((r4818 & 0x60) == 0x40)
				{
					increment = s7_data_adjust();
					if(r4818 & 8)
						increment = (int16_t)increment;  /*16-bit sign extend*/
					s7_set_data_pointer(s7_data_pointer() + increment);
				}
			}
			break;
		case 0x4815:
			{
				unsigned increment;

				r4815 = data;
				r4815_latch = TRUE;
				if(!r4814_latch)
					break;
				if(!(r4818 & 2))
					break;
				if(r4818 & 0x10)
					break;

				if((r4818 & 0x60) == 0x20)
				{
					increment = s7_data_adjust() & 0xff;
					if(r4818 & 8) increment = (int8_t)increment;  /*8-bit sign extend*/
					s7_set_data_pointer(s7_data_pointer() + increment);
				}
				else if((r4818 & 0x60) == 0x40)
				{
					increment = s7_data_adjust();
					if(r4818 & 8)
						increment = (int16_t)increment;  /*16-bit sign extend*/
					s7_set_data_pointer(s7_data_pointer() + increment);
				}
			}
			break;
		case 0x4816:
			r4816 = data;
			break;
		case 0x4817:
			r4817 = data;
			break;
		case 0x4818:
			{
				if(r481x != 0x07)
					break;

				r4818 = data;
				r4814_latch = r4815_latch = FALSE;
			}
			break;

			/*=========*/
			/*math unit*/
			/*=========*/
		case 0x4820:
			r4820 = data;
			break;
		case 0x4821:
			r4821 = data;
			break;
		case 0x4822:
			r4822 = data;
			break;
		case 0x4823:
			r4823 = data;
			break;
		case 0x4824:
			r4824 = data;
			break;
		case 0x4825:
			{
				signed result;
				r4825 = data;

				if(r482e & 1)
				{
					/*signed 16-bit x 16-bit multiplication*/
					int16_t r0 = (int16_t)(r4824 + (r4825 << 8));
					int16_t r1 = (int16_t)(r4820 + (r4821 << 8));

					result = r0 * r1;
					r4828 = result;
					r4829 = result >> 8;
					r482a = result >> 16;
					r482b = result >> 24;
				}
				else
				{
					/*unsigned 16-bit x 16-bit multiplication*/
					uint16_t r0 = (uint16_t)(r4824 + (r4825 << 8));
					uint16_t r1 = (uint16_t)(r4820 + (r4821 << 8));

					result = r0 * r1;
					r4828 = result;
					r4829 = result >> 8;
					r482a = result >> 16;
					r482b = result >> 24;
				}

				r482f = 0x80;
			} break;
		case 0x4826:
			r4826 = data;
			break;
		case 0x4827:
			{
				r4827 = data;

				if(r482e & 1)
				{
					int32_t dividend, quotient;
					int16_t divisor, remainder;
					/*signed 32-bit x 16-bit division*/
					dividend = (int32_t)(r4820 + (r4821 << 8) + (r4822 << 16) + (r4823 << 24));
					divisor  = (int16_t)(r4826 + (r4827 << 8));

					if(divisor)
					{
						quotient  = (int32_t)(dividend / divisor);
						remainder = (int32_t)(dividend % divisor);
					}
					else
					{
						/*illegal division by zero*/
						quotient  = 0;
						remainder = dividend & 0xffff;
					}

					r4828 = quotient;
					r4829 = quotient >> 8;
					r482a = quotient >> 16;
					r482b = quotient >> 24;

					r482c = remainder;
					r482d = remainder >> 8;
				}
				else
				{
					uint32_t dividend, quotient;
					uint16_t divisor, remainder;
					/*unsigned 32-bit x 16-bit division*/
					dividend = (uint32_t)(r4820 + (r4821 << 8) + (r4822 << 16) + (r4823 << 24));
					divisor  = (uint16_t)(r4826 + (r4827 << 8));

					if(divisor)
					{
						quotient  = (uint32_t)(dividend / divisor);
						remainder = (uint16_t)(dividend % divisor);
					}
					else
					{
						/*illegal division by zero*/
						quotient  = 0;
						remainder = dividend & 0xffff;
					}

					r4828 = quotient;
					r4829 = quotient >> 8;
					r482a = quotient >> 16;
					r482b = quotient >> 24;

					r482c = remainder;
					r482d = remainder >> 8;
				}

				r482f = 0x80;
			}
			break;
		case 0x482e:
			{
				/*reset math unit*/
				r4820 = r4821 = r4822 = r4823 = 0;
				r4824 = r4825 = r4826 = r4827 = 0;
				r4828 = r4829 = r482a = r482b = 0;
				r482c = r482d = 0;

				r482e = data;
			}
			break;
			/*===================*/
			/*memory mapping unit*/
			/*===================*/
		case 0x4830:
			r4830 = data;
			break;
		case 0x4831:
			{
				r4831 = data;
				dx_offset = s7_datarom_addr((data & 7) * 0x100000);
			}
			break;
		case 0x4832:
			{
				r4832 = data;
				ex_offset = s7_datarom_addr((data & 7) * 0x100000);
			}
			break;
		case 0x4833:
			{
				r4833 = data;
				fx_offset = s7_datarom_addr((data & 7) * 0x100000);
			}
			break;
		case 0x4834:
			r4834 = data; break;
			/*====================*/
			/*real-time clock unit*/
			/*====================*/
		case 0x4840:
			{
				r4840 = data;
				if(!(r4840 & 1))
				{
					/*disable RTC*/
					rtc_state = RTCS_INACTIVE;
					s7_update_time(0);
				}
				else
				{
					/*enable RTC*/
					r4842 = 0x80;
					rtc_state = RTCS_MODESELECT;
				}
			}
			break;
		case 0x4841:
			     {
				     r4841 = data;

				     switch(rtc_state)
				     {
					     case RTCS_MODESELECT:
						     {
							     if(data == RTCM_LINEAR || data == RTCM_INDEXED)
							     {
								     r4842 = 0x80;
								     rtc_state = RTCS_INDEXSELECT;
								     spc7110_rtc_mode  = data;
								     rtc_index = 0;
							     }
						     }
						     break;
					     case RTCS_INDEXSELECT:
						     {
							     r4842 = 0x80;
							     rtc_index = data & 15;
							     if(spc7110_rtc_mode == RTCM_LINEAR)
								     rtc_state = RTCS_WRITE;
						     }
						     break;
					     case RTCS_WRITE:
						     {
							     r4842 = 0x80;

							     /*control register 0*/
							     if(rtc_index == 13)
							     {
								     /*increment second counter*/
								     if(data & 2)
								     	s7_update_time(+1);

								     /*round minute counter*/
								     if(data & 8)
								     {
									     unsigned second;
									     s7_update_time(0);

									     second = memory_cartrtc_read( 0) + memory_cartrtc_read( 1) * 10;
									     /*clear seconds*/
									     memory_cartrtc_write(0, 0);
									     memory_cartrtc_write(1, 0);

									     if(second >= 30)
										     s7_update_time(+60);
								     }
							     }

							     /*control register 2*/
							     if(rtc_index == 15)
							     {
								     /*disable timer and clear second counter*/
								     if((data & 1) && !(memory_cartrtc_read(15) & 1))
								     {
									     s7_update_time(0);

									     /*clear seconds*/
									     memory_cartrtc_write(0, 0);
									     memory_cartrtc_write(1, 0);
								     }

								     /*disable timer*/
								     if((data & 2) && !(memory_cartrtc_read(15) & 2))
								     {
									     s7_update_time(0);
								     }
							     }

							     memory_cartrtc_write(rtc_index, data & 15);
							     rtc_index = (rtc_index + 1) & 15;
						     }
						     break;
					     case RTCS_INACTIVE:
						     break;
				     } /*switch(rtc_state)*/
			     }
			     break;
	}
}

static void s7_power (void)
{
	r4801 = 0x00;
	r4802 = 0x00;
	r4803 = 0x00;
	r4804 = 0x00;
	r4805 = 0x00;
	r4806 = 0x00;
	r4807 = 0x00;
	r4808 = 0x00;
	r4809 = 0x00;
	r480a = 0x00;
	r480b = 0x00;
	r480c = 0x00;

	spc7110_decomp_reset();

	r4811 = 0x00;
	r4812 = 0x00;
	r4813 = 0x00;
	r4814 = 0x00;
	r4815 = 0x00;
	r4816 = 0x00;
	r4817 = 0x00;
	r4818 = 0x00;

	r481x = 0x00;
	r4814_latch = FALSE;
	r4815_latch = FALSE;

	r4820 = 0x00;
	r4821 = 0x00;
	r4822 = 0x00;
	r4823 = 0x00;
	r4824 = 0x00;
	r4825 = 0x00;
	r4826 = 0x00;
	r4827 = 0x00;
	r4828 = 0x00;
	r4829 = 0x00;
	r482a = 0x00;
	r482b = 0x00;
	r482c = 0x00;
	r482d = 0x00;
	r482e = 0x00;
	r482f = 0x00;

	r4830 = 0x00;
	s7_mmio_write(0x4831, 0);
	s7_mmio_write(0x4832, 1);
	s7_mmio_write(0x4833, 2);
	r4834 = 0x00;

	r4840 = 0x00;
	r4841 = 0x00;
	r4842 = 0x00;

	if(Settings.SPC7110RTC)
	{
		rtc_state = RTCS_INACTIVE;
		spc7110_rtc_mode  = RTCM_LINEAR;
		rtc_index = 0;
	}
}




static unsigned s7_data_increment (void)
{
	return r4816 + (r4817 << 8);
}


static void s7_set_data_adjust(unsigned addr)
{
	r4814 = addr;
	r4815 = addr >> 8;
}


static uint8_t s7_mmio_read(unsigned addr)
{
	addr &= 0xffff;

	switch(addr)
	{
		/*==================*/
		/*decompression unit*/
		/*==================*/

		case 0x4800:
			{
				uint16_t counter = (r4809 + (r480a << 8));
				counter--;
				r4809 = counter;
				r480a = counter >> 8;
				return spc7110_decomp_read();
			}
		case 0x4801:
			return r4801;
		case 0x4802:
			return r4802;
		case 0x4803:
			return r4803;
		case 0x4804:
			return r4804;
		case 0x4805:
			return r4805;
		case 0x4806:
			return r4806;
		case 0x4807:
			return r4807;
		case 0x4808:
			return r4808;
		case 0x4809:
			return r4809;
		case 0x480a:
			return r480a;
		case 0x480b:
			return r480b;
		case 0x480c:
			{
				uint8_t status = r480c;
				r480c &= 0x7f;
				return status;
			}

			/*==============*/
			/*data port unit*/
			/*==============*/
		case 0x4810:
			{
				unsigned addr, adjust, adjustaddr, increment;
				uint8_t data;

				if(r481x != 0x07)
					return 0x00;

				addr = s7_data_pointer();
				adjust = s7_data_adjust();
				if(r4818 & 8)
					adjust = (int16_t)adjust;  /*16-bit sign extend*/

				adjustaddr = addr;
				if(r4818 & 2)
				{
					adjustaddr += adjust;
					s7_set_data_adjust(adjust + 1);
				}

				data = memory_cartrom_read(s7_datarom_addr(adjustaddr));
				if(!(r4818 & 2))
				{
					increment = (r4818 & 1) ? s7_data_increment() : 1;
					if(r4818 & 4)
						increment = (int16_t)increment;  /*16-bit sign extend*/

					if((r4818 & 16) == 0)
						s7_set_data_pointer(addr + increment);
					else
						s7_set_data_adjust(adjust + increment);
				}

				return data;
			}
		case 0x4811:
			return r4811;
		case 0x4812:
			return r4812;
		case 0x4813:
			return r4813;
		case 0x4814:
			return r4814;
		case 0x4815:
			return r4815;
		case 0x4816:
			return r4816;
		case 0x4817:
			return r4817;
		case 0x4818:
			return r4818;
		case 0x481a:
			{
				unsigned addr, adjust;
				uint8_t data;

				if(r481x != 0x07)
					return 0x00;

				addr = s7_data_pointer();
				adjust = s7_data_adjust();

				if(r4818 & 8)
					adjust = (int16_t)adjust;  /*16-bit sign extend*/

				data = memory_cartrom_read(s7_datarom_addr(addr + adjust));
				if((r4818 & 0x60) == 0x60)
				{
					if((r4818 & 16) == 0)
						s7_set_data_pointer(addr + adjust);
					else
						s7_set_data_adjust(adjust + adjust);
				}

				return data;
			}

			     /*=========*/
			     /*math unit*/
			     /*=========*/

		case 0x4820: return r4820;
		case 0x4821: return r4821;
		case 0x4822: return r4822;
		case 0x4823: return r4823;
		case 0x4824: return r4824;
		case 0x4825: return r4825;
		case 0x4826: return r4826;
		case 0x4827: return r4827;
		case 0x4828: return r4828;
		case 0x4829: return r4829;
		case 0x482a: return r482a;
		case 0x482b: return r482b;
		case 0x482c: return r482c;
		case 0x482d: return r482d;
		case 0x482e: return r482e;
		case 0x482f:
			     {
				     uint8_t status;
				     status = r482f;
				     r482f &= 0x7f;
				     return status;
			     }

			     /*===================*/
			     /*memory mapping unit*/
			     /*===================*/

		case 0x4830: return r4830;
		case 0x4831: return r4831;
		case 0x4832: return r4832;
		case 0x4833: return r4833;
		case 0x4834: return r4834;

			     /*====================*/
			     /*real-time clock unit*/
			     /*====================*/

		case 0x4840: return r4840;
		case 0x4841:
			     {
				     uint8_t data;

				     if(rtc_state == RTCS_INACTIVE || rtc_state == RTCS_MODESELECT)
					     return 0x00;

				     r4842 = 0x80;
				     data = memory_cartrtc_read(rtc_index);
				     rtc_index = (rtc_index + 1) & 15;
				     return data;
			     }
		case 0x4842:
			     {
				     uint8_t status;
				     status = r4842;
				     r4842 &= 0x7f;
				     return status;
			     }
	}

	return OpenBus;
}

void S9xInitSPC7110 (void)
{
	spc7110_decomp_start();
	s7_power();
	memset(RTCData.reg, 0, 20);
}

void S9xResetSPC7110 (void)
{
	s7_power();
}

void S9xFreeSPC7110 (void)
{
	if(decomp_buffer)
		free(decomp_buffer);
}

static void SetSPC7110SRAMMap (uint8_t newstate)
{
	if (newstate & 0x80)
	{
		Memory.Map[0x006] = (uint8_t *) MAP_HIROM_SRAM;
		Memory.Map[0x007] = (uint8_t *) MAP_HIROM_SRAM;
		Memory.Map[0x306] = (uint8_t *) MAP_HIROM_SRAM;
		Memory.Map[0x307] = (uint8_t *) MAP_HIROM_SRAM;
	}
	else
	{
		Memory.Map[0x006] = (uint8_t *) MAP_RONLY_SRAM;
		Memory.Map[0x007] = (uint8_t *) MAP_RONLY_SRAM;
		Memory.Map[0x306] = (uint8_t *) MAP_RONLY_SRAM;
		Memory.Map[0x307] = (uint8_t *) MAP_RONLY_SRAM;
	}
}

uint8_t * S9xGetBasePointerSPC7110 (uint32_t address)
{
	uint32_t	i;

	switch (address & 0xf00000)
	{
		case 0xd00000:
			i = dx_offset;
			break;

		case 0xe00000:
			i = ex_offset;
			break;

		case 0xf00000:
			i = fx_offset;
			break;

		default:
			i = 0;
			break;
	}

	i += address & 0x0f0000;

	return (&Memory.ROM[i]);
}

uint8_t S9xGetSPC7110Byte (uint32_t address)
{
	uint32_t i;

	i = 0;

	switch (address & 0xf00000)
	{
		case 0xd00000:
			i = dx_offset;
			break;

		case 0xe00000:
			i = ex_offset;
			break;

		case 0xf00000:
			i = fx_offset;
			break;
	}

	i += address & 0x0fffff;

	return (Memory.ROM[i]);
}

uint8_t S9xGetSPC7110 (uint16_t address)
{
	if (!Settings.SPC7110RTC && address > 0x483f)
		return (OpenBus);

	return (s7_mmio_read(address));
}

void S9xSetSPC7110 (uint8_t byte, uint16_t address)
{
	if (!Settings.SPC7110RTC && address > 0x483f)
		return;

	if (address == 0x4830)
		SetSPC7110SRAMMap(byte);

	s7_mmio_write(address, byte);
}

void S9xSPC7110PreSaveState (void)
{
	int i;
	s7snap.r4801 = r4801;
	s7snap.r4802 = r4802;
	s7snap.r4803 = r4803;
	s7snap.r4804 = r4804;
	s7snap.r4805 = r4805;
	s7snap.r4806 = r4806;
	s7snap.r4807 = r4807;
	s7snap.r4808 = r4808;
	s7snap.r4809 = r4809;
	s7snap.r480a = r480a;
	s7snap.r480b = r480b;
	s7snap.r480c = r480c;

	s7snap.r4811 = r4811;
	s7snap.r4812 = r4812;
	s7snap.r4813 = r4813;
	s7snap.r4814 = r4814;
	s7snap.r4815 = r4815;
	s7snap.r4816 = r4816;
	s7snap.r4817 = r4817;
	s7snap.r4818 = r4818;

	s7snap.r481x = r481x;

	s7snap.r4814_latch = r4814_latch ? TRUE : FALSE;
	s7snap.r4815_latch = r4815_latch ? TRUE : FALSE;

	s7snap.r4820 = r4820;
	s7snap.r4821 = r4821;
	s7snap.r4822 = r4822;
	s7snap.r4823 = r4823;
	s7snap.r4824 = r4824;
	s7snap.r4825 = r4825;
	s7snap.r4826 = r4826;
	s7snap.r4827 = r4827;
	s7snap.r4828 = r4828;
	s7snap.r4829 = r4829;
	s7snap.r482a = r482a;
	s7snap.r482b = r482b;
	s7snap.r482c = r482c;
	s7snap.r482d = r482d;
	s7snap.r482e = r482e;
	s7snap.r482f = r482f;

	s7snap.r4830 = r4830;
	s7snap.r4831 = r4831;
	s7snap.r4832 = r4832;
	s7snap.r4833 = r4833;
	s7snap.r4834 = r4834;

	s7snap.dx_offset = (uint32_t) dx_offset;
	s7snap.ex_offset = (uint32_t) ex_offset;
	s7snap.fx_offset = (uint32_t) fx_offset;

	s7snap.r4840 = r4840;
	s7snap.r4841 = r4841;
	s7snap.r4842 = r4842;

	s7snap.rtc_state = (int32_t)  rtc_state;
	s7snap.rtc_mode  = (int32_t)  spc7110_rtc_mode;
	s7snap.rtc_index = (uint32_t) rtc_index;

	s7snap.decomp_mode   = (uint32_t) decomp_mode;
	s7snap.decomp_offset = (uint32_t) decomp_offset;

	for ( i = 0; i < SPC7110_DECOMP_BUFFER_SIZE; i++)
		s7snap.decomp_buffer[i] = decomp_buffer[i];

	s7snap.decomp_buffer_rdoffset = (uint32_t) decomp_buffer_rdoffset;
	s7snap.decomp_buffer_wroffset = (uint32_t) decomp_buffer_wroffset;
	s7snap.decomp_buffer_length   = (uint32_t) decomp_buffer_length;

	for ( i = 0; i < 32; i++)
	{
		s7snap.context[i].index  = context[i].index;
		s7snap.context[i].invert = context[i].invert;
	}
}

void S9xSPC7110PostLoadState (void)
{
	r4801 = s7snap.r4801;
	r4802 = s7snap.r4802;
	r4803 = s7snap.r4803;
	r4804 = s7snap.r4804;
	r4805 = s7snap.r4805;
	r4806 = s7snap.r4806;
	r4807 = s7snap.r4807;
	r4808 = s7snap.r4808;
	r4809 = s7snap.r4809;
	r480a = s7snap.r480a;
	r480b = s7snap.r480b;
	r480c = s7snap.r480c;

	r4811 = s7snap.r4811;
	r4812 = s7snap.r4812;
	r4813 = s7snap.r4813;
	r4814 = s7snap.r4814;
	r4815 = s7snap.r4815;
	r4816 = s7snap.r4816;
	r4817 = s7snap.r4817;
	r4818 = s7snap.r4818;

	r481x = s7snap.r481x;

	r4814_latch = s7snap.r4814_latch ? TRUE : FALSE;
	r4815_latch = s7snap.r4815_latch ? TRUE : FALSE;

	r4820 = s7snap.r4820;
	r4821 = s7snap.r4821;
	r4822 = s7snap.r4822;
	r4823 = s7snap.r4823;
	r4824 = s7snap.r4824;
	r4825 = s7snap.r4825;
	r4826 = s7snap.r4826;
	r4827 = s7snap.r4827;
	r4828 = s7snap.r4828;
	r4829 = s7snap.r4829;
	r482a = s7snap.r482a;
	r482b = s7snap.r482b;
	r482c = s7snap.r482c;
	r482d = s7snap.r482d;
	r482e = s7snap.r482e;
	r482f = s7snap.r482f;

	r4830 = s7snap.r4830;
	r4831 = s7snap.r4831;
	r4832 = s7snap.r4832;
	r4833 = s7snap.r4833;
	r4834 = s7snap.r4834;

	dx_offset = (unsigned) s7snap.dx_offset;
	ex_offset = (unsigned) s7snap.ex_offset;
	fx_offset = (unsigned) s7snap.fx_offset;

	r4840 = s7snap.r4840;
	r4841 = s7snap.r4841;
	r4842 = s7snap.r4842;

	rtc_state = s7snap.rtc_state;
	spc7110_rtc_mode  = s7snap.rtc_mode;
	rtc_index = (unsigned)           s7snap.rtc_index;

	decomp_mode   = (unsigned) s7snap.decomp_mode;
	decomp_offset = (unsigned) s7snap.decomp_offset;

   memcpy(decomp_buffer,s7snap.decomp_buffer,SPC7110_DECOMP_BUFFER_SIZE);

	decomp_buffer_rdoffset = (unsigned) s7snap.decomp_buffer_rdoffset;
	decomp_buffer_wroffset = (unsigned) s7snap.decomp_buffer_wroffset;
	decomp_buffer_length   = (unsigned) s7snap.decomp_buffer_length;

   memcpy(context,s7snap.context,sizeof(ContextState) * 32);

	s7_update_time(0);
}
