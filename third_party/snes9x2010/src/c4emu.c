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

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <retro_inline.h>

#include "snes9x.h"
#include "memmap.h"

#ifndef SAR
#define SAR(b, n)	((b) >> (n))
#endif

#define	C4_PI	3.14159265

static int16_t	C4WFXVal;
static int16_t	C4WFYVal;
static int16_t	C4WFZVal;
static int16_t	C4WFX2Val;
static int16_t	C4WFY2Val;
static int16_t	C4WFDist;
static int16_t	C4WFScale;
static int16_t	C41FXVal;
static int16_t	C41FYVal;
static int16_t	C41FAngleRes;
static int16_t	C41FDist;
static int16_t	C41FDistVal;

static double	tanval;
static double	c4x, c4y, c4z;
static double	c4x2, c4y2, c4z2;

static int16_t	C4SinTable[512] =
{
	     0,    402,    804,   1206,   1607,   2009,   2410,   2811,
	  3211,   3611,   4011,   4409,   4808,   5205,   5602,   5997,
	  6392,   6786,   7179,   7571,   7961,   8351,   8739,   9126,
	  9512,   9896,  10278,  10659,  11039,  11416,  11793,  12167,
	 12539,  12910,  13278,  13645,  14010,  14372,  14732,  15090,
	 15446,  15800,  16151,  16499,  16846,  17189,  17530,  17869,
	 18204,  18537,  18868,  19195,  19519,  19841,  20159,  20475,
	 20787,  21097,  21403,  21706,  22005,  22301,  22594,  22884,
	 23170,  23453,  23732,  24007,  24279,  24547,  24812,  25073,
	 25330,  25583,  25832,  26077,  26319,  26557,  26790,  27020,
	 27245,  27466,  27684,  27897,  28106,  28310,  28511,  28707,
	 28898,  29086,  29269,  29447,  29621,  29791,  29956,  30117,
	 30273,  30425,  30572,  30714,  30852,  30985,  31114,  31237,
	 31357,  31471,  31581,  31685,  31785,  31881,  31971,  32057,
	 32138,  32214,  32285,  32351,  32413,  32469,  32521,  32568,
	 32610,  32647,  32679,  32706,  32728,  32745,  32758,  32765,
	 32767,  32765,  32758,  32745,  32728,  32706,  32679,  32647,
	 32610,  32568,  32521,  32469,  32413,  32351,  32285,  32214,
	 32138,  32057,  31971,  31881,  31785,  31685,  31581,  31471,
	 31357,  31237,  31114,  30985,  30852,  30714,  30572,  30425,
	 30273,  30117,  29956,  29791,  29621,  29447,  29269,  29086,
	 28898,  28707,  28511,  28310,  28106,  27897,  27684,  27466,
	 27245,  27020,  26790,  26557,  26319,  26077,  25832,  25583,
	 25330,  25073,  24812,  24547,  24279,  24007,  23732,  23453,
	 23170,  22884,  22594,  22301,  22005,  21706,  21403,  21097,
	 20787,  20475,  20159,  19841,  19519,  19195,  18868,  18537,
	 18204,  17869,  17530,  17189,  16846,  16499,  16151,  15800,
	 15446,  15090,  14732,  14372,  14010,  13645,  13278,  12910,
	 12539,  12167,  11793,  11416,  11039,  10659,  10278,   9896,
	  9512,   9126,   8739,   8351,   7961,   7571,   7179,   6786,
	  6392,   5997,   5602,   5205,   4808,   4409,   4011,   3611,
	  3211,   2811,   2410,   2009,   1607,   1206,    804,    402,
	     0,   -402,   -804,  -1206,  -1607,  -2009,  -2410,  -2811,
	 -3211,  -3611,  -4011,  -4409,  -4808,  -5205,  -5602,  -5997,
	 -6392,  -6786,  -7179,  -7571,  -7961,  -8351,  -8739,  -9126,
	 -9512,  -9896, -10278, -10659, -11039, -11416, -11793, -12167,
	-12539, -12910, -13278, -13645, -14010, -14372, -14732, -15090,
	-15446, -15800, -16151, -16499, -16846, -17189, -17530, -17869,
	-18204, -18537, -18868, -19195, -19519, -19841, -20159, -20475,
	-20787, -21097, -21403, -21706, -22005, -22301, -22594, -22884,
	-23170, -23453, -23732, -24007, -24279, -24547, -24812, -25073,
	-25330, -25583, -25832, -26077, -26319, -26557, -26790, -27020,
	-27245, -27466, -27684, -27897, -28106, -28310, -28511, -28707,
	-28898, -29086, -29269, -29447, -29621, -29791, -29956, -30117,
	-30273, -30425, -30572, -30714, -30852, -30985, -31114, -31237,
	-31357, -31471, -31581, -31685, -31785, -31881, -31971, -32057,
	-32138, -32214, -32285, -32351, -32413, -32469, -32521, -32568,
	-32610, -32647, -32679, -32706, -32728, -32745, -32758, -32765,
	-32767, -32765, -32758, -32745, -32728, -32706, -32679, -32647,
	-32610, -32568, -32521, -32469, -32413, -32351, -32285, -32214,
	-32138, -32057, -31971, -31881, -31785, -31685, -31581, -31471,
	-31357, -31237, -31114, -30985, -30852, -30714, -30572, -30425,
	-30273, -30117, -29956, -29791, -29621, -29447, -29269, -29086,
	-28898, -28707, -28511, -28310, -28106, -27897, -27684, -27466,
	-27245, -27020, -26790, -26557, -26319, -26077, -25832, -25583,
	-25330, -25073, -24812, -24547, -24279, -24007, -23732, -23453,
	-23170, -22884, -22594, -22301, -22005, -21706, -21403, -21097,
	-20787, -20475, -20159, -19841, -19519, -19195, -18868, -18537,
	-18204, -17869, -17530, -17189, -16846, -16499, -16151, -15800,
	-15446, -15090, -14732, -14372, -14010, -13645, -13278, -12910,
	-12539, -12167, -11793, -11416, -11039, -10659, -10278,  -9896,
	 -9512,  -9126,  -8739,  -8351,  -7961,  -7571,  -7179,  -6786,
	 -6392,  -5997,  -5602,  -5205,  -4808,  -4409,  -4011,  -3611,
	 -3211,  -2811,  -2410,  -2009,  -1607,  -1206,   -804,   -402
};

static int16_t	C4CosTable[512] =
{
	 32767,  32765,  32758,  32745,  32728,  32706,  32679,  32647,
	 32610,  32568,  32521,  32469,  32413,  32351,  32285,  32214,
	 32138,  32057,  31971,  31881,  31785,  31685,  31581,  31471,
	 31357,  31237,  31114,  30985,  30852,  30714,  30572,  30425,
	 30273,  30117,  29956,  29791,  29621,  29447,  29269,  29086,
	 28898,  28707,  28511,  28310,  28106,  27897,  27684,  27466,
	 27245,  27020,  26790,  26557,  26319,  26077,  25832,  25583,
	 25330,  25073,  24812,  24547,  24279,  24007,  23732,  23453,
	 23170,  22884,  22594,  22301,  22005,  21706,  21403,  21097,
	 20787,  20475,  20159,  19841,  19519,  19195,  18868,  18537,
	 18204,  17869,  17530,  17189,  16846,  16499,  16151,  15800,
	 15446,  15090,  14732,  14372,  14010,  13645,  13278,  12910,
	 12539,  12167,  11793,  11416,  11039,  10659,  10278,   9896,
	  9512,   9126,   8739,   8351,   7961,   7571,   7179,   6786,
	  6392,   5997,   5602,   5205,   4808,   4409,   4011,   3611,
	  3211,   2811,   2410,   2009,   1607,   1206,    804,    402,
	     0,   -402,   -804,  -1206,  -1607,  -2009,  -2410,  -2811,
	 -3211,  -3611,  -4011,  -4409,  -4808,  -5205,  -5602,  -5997,
	 -6392,  -6786,  -7179,  -7571,  -7961,  -8351,  -8739,  -9126,
	 -9512,  -9896, -10278, -10659, -11039, -11416, -11793, -12167,
	-12539, -12910, -13278, -13645, -14010, -14372, -14732, -15090,
	-15446, -15800, -16151, -16499, -16846, -17189, -17530, -17869,
	-18204, -18537, -18868, -19195, -19519, -19841, -20159, -20475,
	-20787, -21097, -21403, -21706, -22005, -22301, -22594, -22884,
	-23170, -23453, -23732, -24007, -24279, -24547, -24812, -25073,
	-25330, -25583, -25832, -26077, -26319, -26557, -26790, -27020,
	-27245, -27466, -27684, -27897, -28106, -28310, -28511, -28707,
	-28898, -29086, -29269, -29447, -29621, -29791, -29956, -30117,
	-30273, -30425, -30572, -30714, -30852, -30985, -31114, -31237,
	-31357, -31471, -31581, -31685, -31785, -31881, -31971, -32057,
	-32138, -32214, -32285, -32351, -32413, -32469, -32521, -32568,
	-32610, -32647, -32679, -32706, -32728, -32745, -32758, -32765,
	-32767, -32765, -32758, -32745, -32728, -32706, -32679, -32647,
	-32610, -32568, -32521, -32469, -32413, -32351, -32285, -32214,
	-32138, -32057, -31971, -31881, -31785, -31685, -31581, -31471,
	-31357, -31237, -31114, -30985, -30852, -30714, -30572, -30425,
	-30273, -30117, -29956, -29791, -29621, -29447, -29269, -29086,
	-28898, -28707, -28511, -28310, -28106, -27897, -27684, -27466,
	-27245, -27020, -26790, -26557, -26319, -26077, -25832, -25583,
	-25330, -25073, -24812, -24547, -24279, -24007, -23732, -23453,
	-23170, -22884, -22594, -22301, -22005, -21706, -21403, -21097,
	-20787, -20475, -20159, -19841, -19519, -19195, -18868, -18537,
	-18204, -17869, -17530, -17189, -16846, -16499, -16151, -15800,
	-15446, -15090, -14732, -14372, -14010, -13645, -13278, -12910,
	-12539, -12167, -11793, -11416, -11039, -10659, -10278,  -9896,
	 -9512,  -9126,  -8739,  -8351,  -7961,  -7571,  -7179,  -6786,
	 -6392,  -5997,  -5602,  -5205,  -4808,  -4409,  -4011,  -3611,
	 -3211,  -2811,  -2410,  -2009,  -1607,  -1206,   -804,   -402,
	     0,    402,    804,   1206,   1607,   2009,   2410,   2811,
	  3211,   3611,   4011,   4409,   4808,   5205,   5602,   5997,
	  6392,   6786,   7179,   7571,   7961,   8351,   8739,   9126,
	  9512,   9896,  10278,  10659,  11039,  11416,  11793,  12167,
	 12539,  12910,  13278,  13645,  14010,  14372,  14732,  15090,
	 15446,  15800,  16151,  16499,  16846,  17189,  17530,  17869,
	 18204,  18537,  18868,  19195,  19519,  19841,  20159,  20475,
	 20787,  21097,  21403,  21706,  22005,  22301,  22594,  22884,
	 23170,  23453,  23732,  24007,  24279,  24547,  24812,  25073,
	 25330,  25583,  25832,  26077,  26319,  26557,  26790,  27020,
	 27245,  27466,  27684,  27897,  28106,  28310,  28511,  28707,
	 28898,  29086,  29269,  29447,  29621,  29791,  29956,  30117,
	 30273,  30425,  30572,  30714,  30852,  30985,  31114,  31237,
	 31357,  31471,  31581,  31685,  31785,  31881,  31971,  32057,
	 32138,  32214,  32285,  32351,  32413,  32469,  32521,  32568,
	 32610,  32647,  32679,  32706,  32728,  32745,  32758,  32765
};

static uint8_t	C4TestPattern[12 * 4] =
{
	0x00, 0x00, 0x00, 0xff,
	0xff, 0xff, 0x00, 0xff,
	0x00, 0x00, 0x00, 0xff,
	0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0x00, 0x00,
	0x80, 0xff, 0xff, 0x7f,
	0x00, 0x80, 0x00, 0xff,
	0x7f, 0x00, 0xff, 0x7f,
	0xff, 0x7f, 0xff, 0xff,
	0x00, 0x00, 0x01, 0xff,
	0xff, 0xfe, 0x00, 0x01,
	0x00, 0xff, 0xfe, 0x00
};

static INLINE uint8_t * C4GetMemPointer (uint32_t Address)
{
	return (Memory.ROM + ((Address & 0xff0000) >> 1) + (Address & 0x7fff));
}

static void C4ConvOAM (void)
{
	uint8_t *i, *OAMptr, *OAMptr2, *srcptr, *sprptr;
	uint8_t	SprName, SprAttr, SprCount, offset;
	int16_t	SprX, SprY;
	uint16_t	globalX, globalY;
	int j;

	OAMptr = Memory.C4RAM + (Memory.C4RAM[0x626] << 2);
	for (i = Memory.C4RAM + 0x1fd; i > OAMptr; i -= 4)
		*i = 0xe0; /* Clear OAM-to-be */


	globalX = READ_WORD(Memory.C4RAM + 0x0621);
	globalY = READ_WORD(Memory.C4RAM + 0x0623);
	OAMptr2 = Memory.C4RAM + 0x200 + (Memory.C4RAM[0x626] >> 2);

	if (Memory.C4RAM[0x0620] != 0)
	{
		SprCount = 128 - Memory.C4RAM[0x626];

		offset = (Memory.C4RAM[0x626] & 3) * 2;
		srcptr = Memory.C4RAM + 0x220;

		for ( j = Memory.C4RAM[0x0620]; j > 0 && SprCount > 0; j--, srcptr += 16)
		{
			SprX = READ_WORD(srcptr)     - globalX;
			SprY = READ_WORD(srcptr + 2) - globalY;
			SprName = srcptr[5];
			SprAttr = srcptr[4] | srcptr[0x06]; /* XXX: mask bits? */

			sprptr = C4GetMemPointer(READ_3WORD(srcptr + 7));
			if (*sprptr != 0)
			{
				int16_t	X, Y;
				int SprCnt;

				for ( SprCnt = *sprptr++; SprCnt > 0 && SprCount > 0; SprCnt--, sprptr += 4)
				{
					X = (int8_t) sprptr[1];
					if (SprAttr & 0x40)
						X = -X - ((sprptr[0] & 0x20) ? 16 : 8); /* flip X */
					X += SprX;

					if (X >= -16 && X <= 272)
					{
						Y = (int8_t) sprptr[2];
						if (SprAttr & 0x80)
							Y = -Y - ((sprptr[0] & 0x20) ? 16 : 8);
						Y += SprY;

						if (Y >= -16 && Y <= 224)
						{
							OAMptr[0] = X & 0xff;
							OAMptr[1] = (uint8_t) Y;
							OAMptr[2] = SprName + sprptr[3];
							OAMptr[3] = SprAttr ^ (sprptr[0] & 0xc0); /* XXX: Carry from SprName addition? */

							*OAMptr2 &= ~(3 << offset);
							if (X & 0x100)
								*OAMptr2 |= 1 << offset;
							if (sprptr[0] & 0x20)
								*OAMptr2 |= 2 << offset;

							OAMptr += 4;
							SprCount--;

							offset = (offset + 2) & 6;
							if (offset == 0)
								OAMptr2++;
						}
					}
				}
			}
			else
			if (SprCount > 0)
			{
				/* XXX: Should we be testing -16<=SprX<=272 and -16<=SprY<=224? */
				OAMptr[0] = (uint8_t) SprX;
				OAMptr[1] = (uint8_t) SprY;
				OAMptr[2] = SprName;
				OAMptr[3] = SprAttr;

				*OAMptr2 &= ~(3 << offset);
				if (SprX & 0x100)
					*OAMptr2 |= 3 << offset;
				else
					*OAMptr2 |= 2 << offset;

				OAMptr += 4;
				SprCount--;

				offset = (offset + 2) & 6;
				if (offset == 0)
					OAMptr2++;
			}
		}
	}
}

static void C4DoScaleRotate (int row_padding)
{
	uint8_t	w, h, byte, bit;
	int16_t	A, B, C, D;
	int32_t	XScale, YScale, Cx, Cy, LineX, LineY;
	uint32_t	X, Y;
	int	x, y, outidx;

	/* Calculate matrix */
	XScale = READ_WORD(Memory.C4RAM + 0x1f8f);
	if (XScale & 0x8000)
		XScale = 0x7fff;

	YScale = READ_WORD(Memory.C4RAM + 0x1f92);
	if (YScale & 0x8000)
		YScale = 0x7fff;

	if (READ_WORD(Memory.C4RAM + 0x1f80) == 0)	/* no rotation */
	{
		/* XXX: only do this for C and D? */
		/* XXX: and then only when YScale is 0x1000? */
		A = (int16_t) XScale;
		B = 0;
		C = 0;
		D = (int16_t) YScale;
	}
	else
	if (READ_WORD(Memory.C4RAM + 0x1f80) == 128)	/* 90 degree rotation */
	{
		/* XXX: Really do this? */
		A = 0;
		B = (int16_t) (-YScale);
		C = (int16_t) XScale;
		D = 0;
	}
	else
	if (READ_WORD(Memory.C4RAM + 0x1f80) == 256)	/* 180 degree rotation */
	{
		/* XXX: Really do this? */
		A = (int16_t) (-XScale);
		B = 0;
		C = 0;
		D = (int16_t) (-YScale);
	}
	else
	if (READ_WORD(Memory.C4RAM + 0x1f80) == 384)	/* 270 degree rotation */
	{
		/* XXX: Really do this? */
		A = 0;
		B = (int16_t) YScale;
		C = (int16_t) (-XScale);
		D = 0;
	}
	else
	{
		A = (int16_t)   SAR(C4CosTable[READ_WORD(Memory.C4RAM + 0x1f80) & 0x1ff] * XScale, 15);
		B = (int16_t) (-SAR(C4SinTable[READ_WORD(Memory.C4RAM + 0x1f80) & 0x1ff] * YScale, 15));
		C = (int16_t)   SAR(C4SinTable[READ_WORD(Memory.C4RAM + 0x1f80) & 0x1ff] * XScale, 15);
		D = (int16_t)   SAR(C4CosTable[READ_WORD(Memory.C4RAM + 0x1f80) & 0x1ff] * YScale, 15);
	}

	/* Calculate Pixel Resolution */
	w = Memory.C4RAM[0x1f89] & ~7;
	h = Memory.C4RAM[0x1f8c] & ~7;

	/* Clear the output RAM */
	memset(Memory.C4RAM, 0, (w + row_padding / 4) * h / 2);

	Cx = (int16_t) READ_WORD(Memory.C4RAM + 0x1f83);
	Cy = (int16_t) READ_WORD(Memory.C4RAM + 0x1f86);

	/* Calculate start position (i.e. (Ox, Oy) = (0, 0))
	   The low 12 bits are fractional, so (Cx<<12) gives us the Cx we want in the function.
	   We do Cx*A etc normally because the matrix parameters already have the fractional parts. */
	LineX = (Cx << 12) - Cx * A - Cx * B;
	LineY = (Cy << 12) - Cy * C - Cy * D;

	/* Start loop */
	outidx = 0;
	bit = 0x80;

	for ( y = 0; y < h; y++)
	{
		X = LineX;
		Y = LineY;

		for ( x = 0; x < w; x++)
		{
			if ((X >> 12) >= w || (Y >> 12) >= h)
				byte = 0;
			else
			{
				uint32_t	addr = (Y >> 12) * w + (X >> 12);
				byte = Memory.C4RAM[0x600 + (addr >> 1)];
				if (addr & 1)
					byte >>= 4;
			}

			/* De-bitplanify */
			if (byte & 1)
				Memory.C4RAM[outidx]      |= bit;
			if (byte & 2)
				Memory.C4RAM[outidx + 1]  |= bit;
			if (byte & 4)
				Memory.C4RAM[outidx + 16] |= bit;
			if (byte & 8)
				Memory.C4RAM[outidx + 17] |= bit;

			bit >>= 1;
			if (bit == 0)
			{
				bit = 0x80;
				outidx += 32;
			}

			X += A; /* Add 1 to output x => add an A and a C */
			Y += C;
		}

		outidx += 2 + row_padding;
		if (outidx & 0x10)
			outidx &= ~0x10;
		else
			outidx -= w * 4 + row_padding;

		LineX += B; /* Add 1 to output y => add a B and a D */
		LineY += D;
	}
}

static void C4CalcWireFrame (void)
{
	C4WFXVal = C4WFX2Val - C4WFXVal;
	C4WFYVal = C4WFY2Val - C4WFYVal;

	if (abs(C4WFXVal) > abs(C4WFYVal))
	{
		C4WFDist = abs(C4WFXVal) + 1;
		C4WFYVal = (int16_t) (256 * (double) C4WFYVal / abs(C4WFXVal));
		if (C4WFXVal < 0)
			C4WFXVal = -256;
		else
			C4WFXVal =  256;
	}
	else
	{
		if (C4WFYVal != 0)
		{
			C4WFDist = abs(C4WFYVal) + 1;
			C4WFXVal = (int16_t) (256 * (double) C4WFXVal / abs(C4WFYVal));
			if (C4WFYVal < 0)
				C4WFYVal = -256;
			else
				C4WFYVal =  256;
		}
		else
			C4WFDist = 0;
	}
}

static void C4TransfWireFrame2 (void)
{
	c4x = (double) C4WFXVal;
	c4y = (double) C4WFYVal;
	c4z = (double) C4WFZVal;

	/* Rotate X */
	tanval = -(double) C4WFX2Val * C4_PI * 2 / 128;
	c4y2 = c4y  *  cos(tanval) - c4z  * sin(tanval);
	c4z2 = c4y  *  sin(tanval) + c4z  * cos(tanval);

	/* Rotate Y */
	tanval = -(double) C4WFY2Val * C4_PI * 2 / 128;
	c4x2 = c4x  *  cos(tanval) + c4z2 * sin(tanval);
	c4z  = c4x  * -sin(tanval) + c4z2 * cos(tanval);

	/* Rotate Z */
	tanval = -(double) C4WFDist  * C4_PI * 2 / 128;
	c4x  = c4x2 *  cos(tanval) - c4y2 * sin(tanval);
	c4y  = c4x2 *  sin(tanval) + c4y2 * cos(tanval);

	/* Scale */
	C4WFXVal = (int16_t) (c4x * (double) C4WFScale / 0x100);
	C4WFYVal = (int16_t) (c4y * (double) C4WFScale / 0x100);
}

static void C4DrawLine (int32_t X1, int32_t Y1, int16_t Z1, int32_t X2, int32_t Y2, int16_t Z2, uint8_t Color)
{
	int i;
	/* Transform coordinates */
	C4WFXVal  = (int16_t) X1;
	C4WFYVal  = (int16_t) Y1;
	C4WFZVal  = Z1;
	C4WFScale = Memory.C4RAM[0x1f90];
	C4WFX2Val = Memory.C4RAM[0x1f86];
	C4WFY2Val = Memory.C4RAM[0x1f87];
	C4WFDist  = Memory.C4RAM[0x1f88];
	C4TransfWireFrame2();
	X1 = (C4WFXVal + 48) << 8;
	Y1 = (C4WFYVal + 48) << 8;

	C4WFXVal  = (int16_t) X2;
	C4WFYVal  = (int16_t) Y2;
	C4WFZVal  = Z2;
	C4TransfWireFrame2();
	X2 = (C4WFXVal + 48) << 8;
	Y2 = (C4WFYVal + 48) << 8;

	/* Get line info */
	C4WFXVal  = (int16_t) (X1 >> 8);
	C4WFYVal  = (int16_t) (Y1 >> 8);
	C4WFX2Val = (int16_t) (X2 >> 8);
	C4WFY2Val = (int16_t) (Y2 >> 8);
	C4CalcWireFrame();
	X2 = (int16_t) C4WFXVal;
	Y2 = (int16_t) C4WFYVal;

	/* Render line */
	for ( i = C4WFDist ? C4WFDist : 1; i > 0; i--)
	{
		if (X1 > 0xff && Y1 > 0xff && X1 < 0x6000 && Y1 < 0x6000)
		{
			uint16_t	addr = (((Y1 >> 8) >> 3) << 8) - (((Y1 >> 8) >> 3) << 6) + (((X1 >> 8) >> 3) << 4) + ((Y1 >> 8) & 7) * 2;
			uint8_t	bit = 0x80 >> ((X1 >> 8) & 7);

			Memory.C4RAM[addr + 0x300] &= ~bit;
			Memory.C4RAM[addr + 0x301] &= ~bit;
			if (Color & 1)
				Memory.C4RAM[addr + 0x300] |= bit;
			if (Color & 2)
				Memory.C4RAM[addr + 0x301] |= bit;
		}

		X1 += X2;
		Y1 += Y2;
	}
}

static void C4DrawWireFrame (void)
{
	uint8_t	*line = C4GetMemPointer(READ_3WORD(Memory.C4RAM + 0x1f80));
	uint8_t	*point1, *point2;
	int16_t	X1, Y1, Z1;
	int16_t	X2, Y2, Z2;
	uint8_t	Color;
	int	i;

	for ( i = Memory.C4RAM[0x0295]; i > 0; i--, line += 5)
	{
		if (line[0] == 0xff && line[1] == 0xff)
		{
			uint8_t	*tmp = line - 5;
			while (tmp[2] == 0xff && tmp[3] == 0xff)
				tmp -= 5;
			point1 = C4GetMemPointer((Memory.C4RAM[0x1f82] << 16) | (tmp[2]  << 8) |  tmp[3]);
		}
		else
			point1 = C4GetMemPointer((Memory.C4RAM[0x1f82] << 16) | (line[0] << 8) | line[1]);

		point2 = C4GetMemPointer((Memory.C4RAM[0x1f82] << 16) | (line[2] << 8) | line[3]);

		X1 = (point1[0] << 8) | point1[1];
		Y1 = (point1[2] << 8) | point1[3];
		Z1 = (point1[4] << 8) | point1[5];
		X2 = (point2[0] << 8) | point2[1];
		Y2 = (point2[2] << 8) | point2[3];
		Z2 = (point2[4] << 8) | point2[5];

		Color = line[4];

		C4DrawLine(X1, Y1, Z1, X2, Y2, Z2, Color);
	}
}

static void C4TransfWireFrame (void)
{
	c4x = (double) C4WFXVal;
	c4y = (double) C4WFYVal;
	c4z = (double) C4WFZVal - 0x95;

	/* Rotate X */
	tanval = -(double) C4WFX2Val * C4_PI * 2 / 128;
	c4y2 = c4y  *  cos(tanval) - c4z  * sin(tanval);
	c4z2 = c4y  *  sin(tanval) + c4z  * cos(tanval);

	/* Rotate Y */
	tanval = -(double) C4WFY2Val * C4_PI * 2 / 128;
	c4x2 = c4x  *  cos(tanval) + c4z2 * sin(tanval);
	c4z  = c4x  * -sin(tanval) + c4z2 * cos(tanval);

	/* Rotate Z */
	tanval = -(double) C4WFDist  * C4_PI * 2 / 128;
	c4x  = c4x2 *  cos(tanval) - c4y2 * sin(tanval);
	c4y  = c4x2 *  sin(tanval) + c4y2 * cos(tanval);

	/* Scale */
	C4WFXVal = (int16_t) (c4x * (double) C4WFScale / (0x90 * (c4z + 0x95)) * 0x95);
	C4WFYVal = (int16_t) (c4y * (double) C4WFScale / (0x90 * (c4z + 0x95)) * 0x95);
}

static void C4TransformLines (void)
{
	uint8_t	*ptr, *ptr2;
	int	i;
	C4WFX2Val = Memory.C4RAM[0x1f83];
	C4WFY2Val = Memory.C4RAM[0x1f86];
	C4WFDist  = Memory.C4RAM[0x1f89];
	C4WFScale = Memory.C4RAM[0x1f8c];

	/* Transform vertices */
	ptr = Memory.C4RAM;

	for ( i = READ_WORD(Memory.C4RAM + 0x1f80); i > 0; i--, ptr += 0x10)
	{
		C4WFXVal = READ_WORD(ptr + 1);
		C4WFYVal = READ_WORD(ptr + 5);
		C4WFZVal = READ_WORD(ptr + 9);
		C4TransfWireFrame();

		/* Displace */
		WRITE_WORD(ptr + 1, C4WFXVal + 0x80);
		WRITE_WORD(ptr + 5, C4WFYVal + 0x50);
	}

	WRITE_WORD(Memory.C4RAM + 0x600,       23);
	WRITE_WORD(Memory.C4RAM + 0x602,     0x60);
	WRITE_WORD(Memory.C4RAM + 0x605,     0x40);
	WRITE_WORD(Memory.C4RAM + 0x600 + 8,   23);
	WRITE_WORD(Memory.C4RAM + 0x602 + 8, 0x60);
	WRITE_WORD(Memory.C4RAM + 0x605 + 8, 0x40);

	ptr = Memory.C4RAM + 0xb02;
	ptr2 = Memory.C4RAM;

	for ( i = READ_WORD(Memory.C4RAM + 0xb00); i > 0; i--, ptr += 2, ptr2 += 8)
	{
		C4WFXVal  = READ_WORD(Memory.C4RAM + (ptr[0] << 4) + 1);
		C4WFYVal  = READ_WORD(Memory.C4RAM + (ptr[0] << 4) + 5);
		C4WFX2Val = READ_WORD(Memory.C4RAM + (ptr[1] << 4) + 1);
		C4WFY2Val = READ_WORD(Memory.C4RAM + (ptr[1] << 4) + 5);
		C4CalcWireFrame();

		WRITE_WORD(ptr2 + 0x600, C4WFDist ? C4WFDist : 1);
		WRITE_WORD(ptr2 + 0x602, C4WFXVal);
		WRITE_WORD(ptr2 + 0x605, C4WFYVal);
	}
}

static void C4BitPlaneWave (void)
{
	int i, j;

	static uint16_t bmpdata[] =
	{
		0x0000, 0x0002, 0x0004, 0x0006, 0x0008, 0x000A, 0x000C, 0x000E,
		0x0200, 0x0202, 0x0204, 0x0206, 0x0208, 0x020A, 0x020C, 0x020E,
		0x0400, 0x0402, 0x0404, 0x0406, 0x0408, 0x040A, 0x040C, 0x040E,
		0x0600, 0x0602, 0x0604, 0x0606, 0x0608, 0x060A, 0x060C, 0x060E,
		0x0800, 0x0802, 0x0804, 0x0806, 0x0808, 0x080A, 0x080C, 0x080E
	};

	uint8_t	*dst = Memory.C4RAM;
	uint32_t	waveptr = Memory.C4RAM[0x1f83];
	uint16_t	mask1 = 0xc0c0;
	uint16_t	mask2 = 0x3f3f;

	for ( j = 0; j < 0x10; j++)
	{
		do
		{
			int16_t	height = -((int8_t) Memory.C4RAM[waveptr + 0xb00]) - 16;

			for ( i = 0; i < 40; i++)
			{
				uint16_t	tmp = READ_WORD(dst + bmpdata[i]) & mask2;
				if (height >= 0)
				{
					if (height < 8)
						tmp |= mask1 & READ_WORD(Memory.C4RAM + 0xa00 + height * 2);
					else
						tmp |= mask1 & 0xff00;
				}

				WRITE_WORD(dst + bmpdata[i], tmp);

				height++;
			}

			waveptr = (waveptr + 1) & 0x7f;
			mask1 = (mask1 >> 2) | (mask1 << 6);
			mask2 = (mask2 >> 2) | (mask2 << 6);
		}
		while (mask1 != 0xc0c0);

		dst += 16;

		do
		{
			int16_t	height = -((int8_t) Memory.C4RAM[waveptr + 0xb00]) - 16;

			for ( i = 0; i < 40; i++)
			{
				uint16_t	tmp = READ_WORD(dst + bmpdata[i]) & mask2;
				if (height >= 0)
				{
					if (height < 8)
						tmp |= mask1 & READ_WORD(Memory.C4RAM + 0xa10 + height * 2);
					else
						tmp |= mask1 & 0xff00;
				}

				WRITE_WORD(dst + bmpdata[i], tmp);

				height++;
			}

			waveptr = (waveptr + 1) & 0x7f;
			mask1 = (mask1 >> 2) | (mask1 << 6);
			mask2 = (mask2 >> 2) | (mask2 << 6);
		}
		while (mask1 != 0xc0c0);

		dst += 16;
	}
}

static void C4SprDisintegrate (void)
{
	uint8_t	*src;
	uint8_t	width, height;
	uint32_t	StartX, StartY;
	int32_t	scaleX, scaleY;
	int32_t	Cx, Cy;
	uint32_t 	i, j, x, y;

	width  = Memory.C4RAM[0x1f89];
	height = Memory.C4RAM[0x1f8c];
	Cx = (int16_t) READ_WORD(Memory.C4RAM + 0x1f80);
	Cy = (int16_t) READ_WORD(Memory.C4RAM + 0x1f83);


	scaleX = (int16_t) READ_WORD(Memory.C4RAM + 0x1f86);
	scaleY = (int16_t) READ_WORD(Memory.C4RAM + 0x1f8f);
	StartX = -Cx * scaleX + (Cx << 8);
	StartY = -Cy * scaleY + (Cy << 8);

	src = Memory.C4RAM + 0x600;

	memset(Memory.C4RAM, 0, width * height / 2);

	for ( y = StartY, i = 0; i < height; i++, y += scaleY)
	{
		for ( x = StartX, j = 0; j < width; j++, x += scaleX)
		{
			if ((x >> 8) < width && (y >> 8) < height && (y >> 8) * width + (x >> 8) < 0x2000)
			{
				uint8_t	pixel = (j & 1) ? (*src >> 4) : *src;
				int		idx   = (y >> 11) * width * 4 + (x >> 11) * 32 + ((y >> 8) & 7) * 2;
				uint8_t	mask  = 0x80 >> ((x >> 8) & 7);

				if (pixel & 1)
					Memory.C4RAM[idx]      |= mask;
				if (pixel & 2)
					Memory.C4RAM[idx + 1]  |= mask;
				if (pixel & 4)
					Memory.C4RAM[idx + 16] |= mask;
				if (pixel & 8)
					Memory.C4RAM[idx + 17] |= mask;
			}

			if (j & 1)
				src++;
		}
	}
}

static void C4ProcessSprites (void)
{
	switch (Memory.C4RAM[0x1f4d])
	{
		case 0x00: /* Build OAM */
			C4ConvOAM();
			break;

		case 0x03: /* Scale/Rotate */
			C4DoScaleRotate(0);
			break;

		case 0x05: /* Transform Lines */
			C4TransformLines();
			break;

		case 0x07: /* Scale/Rotate */
			C4DoScaleRotate(64);
			break;

		case 0x08: /* Draw wireframe */
			C4DrawWireFrame();
			break;

		case 0x0b: /* Disintegrate */
			C4SprDisintegrate();
			break;

		case 0x0c: /* Wave */
			C4BitPlaneWave();
			break;

		default:
			break;
	}
}

void S9xInitC4 (void)
{
	memset(Memory.C4RAM, 0, 0x2000);
}

uint8_t S9xGetC4 (uint16_t Address)
{
	if (Address == 0x7f5e)
		return (0);

	return (Memory.C4RAM[Address - 0x6000]);
}

static void C4Op0D (void)
{
	tanval = sqrt((double) C41FYVal * C41FYVal + (double) C41FXVal * C41FXVal);
	tanval = C41FDistVal / tanval;
	C41FYVal = (int16_t) (C41FYVal * tanval * 0.99);
	C41FXVal = (int16_t) (C41FXVal * tanval * 0.98);
}

static void C4Op1F (void)
{
	if (C41FXVal == 0)
	{
		if (C41FYVal > 0)
			C41FAngleRes = 0x80;
		else
			C41FAngleRes = 0x180;
	}
	else
	{
		tanval = (double) C41FYVal / C41FXVal;
		C41FAngleRes = (int16_t) (atan(tanval) / (C4_PI * 2) * 512);
		if (C41FXVal< 0)
			C41FAngleRes += 0x100;
		C41FAngleRes &= 0x1FF;
	}
}

static void S9XSetC4Square(void)
{
   int64_t b = (int64_t) READ_3WORD(Memory.C4RAM + 0x1f80);
   int64_t c = b << 40;
   int64_t a = SAR(c, 30);
   a = SAR(c, 10);
   a *= a;
   WRITE_3WORD(Memory.C4RAM + 0x1f83, a);
   WRITE_3WORD(Memory.C4RAM + 0x1f86, (a >> 24));
}

void S9xSetC4 (uint8_t byte, uint16_t Address)
{
	Memory.C4RAM[Address - 0x6000] = byte;

	if (Address == 0x7f4f)
	{
		if (Memory.C4RAM[0x1f4d] == 0x0e && byte < 0x40 && (byte & 3) == 0)
		{
			Memory.C4RAM[0x1f80] = byte >> 2;
		}
		else
		{
			switch (byte)
			{
				case 0x00: /* Sprite */
					C4ProcessSprites();
					break;

				case 0x01: /* Draw wireframe */
					memset(Memory.C4RAM + 0x300, 0, 16 * 12 * 3 * 4);
					C4DrawWireFrame();
					break;

				case 0x05: /* Propulsion (?) */
				{
					int32_t	tmp = 0x10000;
					if (READ_WORD(Memory.C4RAM + 0x1f83))
						tmp = SAR((tmp / READ_WORD(Memory.C4RAM + 0x1f83)) * READ_WORD(Memory.C4RAM + 0x1f81), 8);

					WRITE_WORD(Memory.C4RAM + 0x1f80, (uint16_t) tmp);
					break;
				}

				case 0x0d: /* Set vector length */
					C41FXVal    = READ_WORD(Memory.C4RAM + 0x1f80);
					C41FYVal    = READ_WORD(Memory.C4RAM + 0x1f83);
					C41FDistVal = READ_WORD(Memory.C4RAM + 0x1f86);
					C4Op0D();
					WRITE_WORD(Memory.C4RAM + 0x1f89, C41FXVal);
					WRITE_WORD(Memory.C4RAM + 0x1f8c, C41FYVal);
					break;

				case 0x10: /* Polar to rectangular */
				{
					int32_t	tmp;
					int32_t   r1;
					r1 = READ_WORD(Memory.C4RAM + 0x1f83);
					if (r1 & 0x8000)
					        r1 |= ~0x7fff;
					else
					        r1 &= 0x7fff;				
										
					tmp = SAR(r1 * C4CosTable[READ_WORD(Memory.C4RAM + 0x1f80) & 0x1ff] * 2, 16);
					WRITE_3WORD(Memory.C4RAM + 0x1f86, tmp);
					tmp = SAR(r1 * C4SinTable[READ_WORD(Memory.C4RAM + 0x1f80) & 0x1ff] * 2, 16);
					WRITE_3WORD(Memory.C4RAM + 0x1f89, (tmp - SAR(tmp, 6)));
					break;
				}

				case 0x13: /* Polar to rectangular */
				{
					int32_t	tmp;
					tmp = SAR((int32_t) READ_WORD(Memory.C4RAM + 0x1f83) * C4CosTable[READ_WORD(Memory.C4RAM + 0x1f80) & 0x1ff] * 2, 8);
					WRITE_3WORD(Memory.C4RAM + 0x1f86, tmp);
					tmp = SAR((int32_t) READ_WORD(Memory.C4RAM + 0x1f83) * C4SinTable[READ_WORD(Memory.C4RAM + 0x1f80) & 0x1ff] * 2, 8);
					WRITE_3WORD(Memory.C4RAM + 0x1f89, tmp);
					break;
				}

				case 0x15: /* Pythagorean */
					C41FXVal = READ_WORD(Memory.C4RAM + 0x1f80);
					C41FYVal = READ_WORD(Memory.C4RAM + 0x1f83);
					C41FDist = (int16_t) sqrt((double) C41FXVal * C41FXVal + (double) C41FYVal * C41FYVal);
					WRITE_WORD(Memory.C4RAM + 0x1f80, C41FDist);
					break;

				case 0x1f: /* atan */
					C41FXVal = READ_WORD(Memory.C4RAM + 0x1f80);
					C41FYVal = READ_WORD(Memory.C4RAM + 0x1f83);
					C4Op1F();
					WRITE_WORD(Memory.C4RAM + 0x1f86, C41FAngleRes);
					break;

				case 0x22: /* Trapezoid */
				{
					int16_t	angle1 = READ_WORD(Memory.C4RAM + 0x1f8c) & 0x1ff;
					int16_t	angle2 = READ_WORD(Memory.C4RAM + 0x1f8f) & 0x1ff;


					int32_t	tan1 = (C4CosTable[angle1] != 0) ? ((((int32_t) C4SinTable[angle1]) << 16) / C4CosTable[angle1]) : 0x80000000;
					int32_t	tan2 = (C4CosTable[angle2] != 0) ? ((((int32_t) C4SinTable[angle2]) << 16) / C4CosTable[angle2]) : 0x80000000;

					int16_t	y = READ_WORD(Memory.C4RAM + 0x1f83) - READ_WORD(Memory.C4RAM + 0x1f89);
					int16_t	left, right;
					int j;

					for ( j = 0; j < 225; j++)
					{
						if (y >= 0)
						{
							left  = SAR((int32_t) tan1 * y, 16) - READ_WORD(Memory.C4RAM + 0x1f80) + READ_WORD(Memory.C4RAM + 0x1f86);
							right = SAR((int32_t) tan2 * y, 16) - READ_WORD(Memory.C4RAM + 0x1f80) + READ_WORD(Memory.C4RAM + 0x1f86) + READ_WORD(Memory.C4RAM + 0x1f93);

							if (left < 0 && right < 0)
							{
								left  = 1;
								right = 0;
							}
							else
							if (left  < 0)
								left  = 0;
							else
							if (right < 0)
								right = 0;

							if (left > 255 && right > 255)
							{
								left  = 255;
								right = 254;
							}
							else
							if (left  > 255)
								left  = 255;
							else
							if (right > 255)
								right = 255;
						}
						else
						{
							left  = 1;
							right = 0;
						}

						Memory.C4RAM[j + 0x800] = (uint8_t) left;
						Memory.C4RAM[j + 0x900] = (uint8_t) right;

						y++;
					}

					break;
				}

				case 0x25: /* Multiply */
				{
					int32_t	foo = READ_3WORD(Memory.C4RAM + 0x1f80);
					int32_t	bar = READ_3WORD(Memory.C4RAM + 0x1f83);
					foo *= bar;
					WRITE_3WORD(Memory.C4RAM + 0x1f80, foo);
					break;
				}

				case 0x2d: /* Transform Coords */
					C4WFXVal  = READ_WORD(Memory.C4RAM + 0x1f81);
					C4WFYVal  = READ_WORD(Memory.C4RAM + 0x1f84);
					C4WFZVal  = READ_WORD(Memory.C4RAM + 0x1f87);
					C4WFX2Val = Memory.C4RAM[0x1f89];
					C4WFY2Val = Memory.C4RAM[0x1f8a];
					C4WFDist  = Memory.C4RAM[0x1f8b];
					C4WFScale = READ_WORD(Memory.C4RAM + 0x1f90);
					C4TransfWireFrame2();
					WRITE_WORD(Memory.C4RAM + 0x1f80, C4WFXVal);
					WRITE_WORD(Memory.C4RAM + 0x1f83, C4WFYVal);
					break;

				case 0x40: /* Sum */
				{
					uint16_t	sum = 0;
					int i;
					for ( i = 0; i < 0x800; sum += Memory.C4RAM[i++]) ;
					WRITE_WORD(Memory.C4RAM + 0x1f80, sum);
					break;
				}

				case 0x54: /* Square */
            S9XSetC4Square();
            break;

				case 0x5c: /* Immediate Reg */
				{
					int i;
					for ( i = 0; i < 12 * 4; i++)
						Memory.C4RAM[i] = C4TestPattern[i];
				}
					break;

				case 0x89: /* Immediate ROM */
					Memory.C4RAM[0x1f80] = 0x36;
					Memory.C4RAM[0x1f81] = 0x43;
					Memory.C4RAM[0x1f82] = 0x05;
					break;

				default:
					break;
			}
		}
	}
	else
	if (Address == 0x7f47)
	{
		memmove(Memory.C4RAM + (READ_WORD(Memory.C4RAM + 0x1f45) & 0x1fff), C4GetMemPointer(READ_3WORD(Memory.C4RAM + 0x1f40)), READ_WORD(Memory.C4RAM + 0x1f43));
	}
}

uint8_t * S9xGetBasePointerC4 (uint16_t Address)
{
	if (Address >= 0x7f40 && Address <= 0x7f5e)
		return (NULL);
	return (Memory.C4RAM - 0x6000);
}

uint8_t * S9xGetMemPointerC4 (uint16_t Address)
{
	if (Address >= 0x7f40 && Address <= 0x7f5e)
		return (NULL);
	return (Memory.C4RAM - 0x6000 + (Address & 0xffff));
}
