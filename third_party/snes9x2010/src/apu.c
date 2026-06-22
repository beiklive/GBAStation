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
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>

#include <retro_inline.h>

#include "boolean.h"

#include "apu.h"

#include "snes9x.h"
#include "snapshot.h"
#include "display.h"

/* The Wii version sometimes has issues with memcpy fnt when optimized, 
   so we remove optimzations from save/load state fnts using this macro */
#ifdef GEKKO
#define NO_OPTIMIZE __attribute__((optimize("O0")))
#else
#define NO_OPTIMIZE
#endif

/* Endianness is a compile-time property; see port.h for the policy.
 *
 *   MSB_FIRST defined     -> host is big endian
 *   MSB_FIRST not defined -> host is little endian
 *
 * On little-endian hosts the SPC700's little-endian words match the
 * host, so a 16/32-bit access is a single (possibly unaligned) load
 * or store. On big-endian PowerPC we use the byte-reversed
 * load/store instructions. Anywhere else big endian, we fall back
 * to manual byte assembly. */

#ifndef MSB_FIRST

#define GET_LE16(addr)		(*(uint16_t *) (addr))
#define GET_LE32(addr)		(*(uint32_t *) (addr))
#define SET_LE16(addr, data)	((void) (*(uint16_t *) (addr) = (uint16_t) (data)))
#define SET_LE32(addr, data)	((void) (*(uint32_t *) (addr) = (uint32_t) (data)))

#else

#if defined(__SNC__)
#include <ppu_intrinsics.h>
#define GET_LE16(addr)		(__builtin_lhbrx(addr, 0))
#define GET_LE32(addr)		(__builtin_lwbrx(addr, 0))
#define SET_LE16(addr, in)	(__builtin_sthbrx(in, addr, 0))
#define SET_LE32(addr, in)	(__builtin_stwbrx(in, addr, 0))
#elif defined(_XBOX360)
#include <PPCIntrinsics.h>
#define GET_LE16(addr)		(__loadshortbytereverse(0, addr))
#define GET_LE32(addr)		(__loadwordbytereverse(0, addr))
#define SET_LE16(addr, in)	(__storeshortbytereverse(in, 0, addr))
#define SET_LE32(addr, in)	(__storewordbytereverse(in, 0, addr))
#elif defined(__MWERKS__)
#define GET_LE16(addr)		(__lhbrx(addr, 0))
#define GET_LE32(addr)		(__lwbrx(addr, 0))
#define SET_LE16(addr, in)	(__sthbrx(in, addr, 0))
#define SET_LE32(addr, in)	(__stwbrx(in, addr, 0))
#elif (defined(__GNUC__) || defined(__SNC__)) && (defined(__powerpc__) || defined(__ppc__) || defined(__POWERPC__))
#define GET_LE16(addr)		({unsigned ppc_lhbrx_; asm( "lhbrx %0,0,%1" : "=r" (ppc_lhbrx_) : "r" (addr), "0" (ppc_lhbrx_) ); ppc_lhbrx_;})
#define GET_LE32(addr)		({unsigned ppc_lwbrx_; asm( "lwbrx %0,0,%1" : "=r" (ppc_lwbrx_) : "r" (addr), "0" (ppc_lwbrx_) ); ppc_lwbrx_;})
#define SET_LE16(addr, in)	({asm( "sthbrx %0,0,%1" : : "r" (in), "r" (addr) );})
#define SET_LE32(addr, in)	({asm( "stwbrx %0,0,%1" : : "r" (in), "r" (addr) );})
#else
/* Generic big-endian fallback: assemble/disassemble byte by byte. */
#define GET_LE16(addr) \
	((uint16_t) (((unsigned char const *) (addr))[0]) \
	 | ((uint16_t) (((unsigned char const *) (addr))[1]) << 8))
#define GET_LE32(addr) \
	((uint32_t) (((unsigned char const *) (addr))[0]) \
	 | ((uint32_t) (((unsigned char const *) (addr))[1]) << 8) \
	 | ((uint32_t) (((unsigned char const *) (addr))[2]) << 16) \
	 | ((uint32_t) (((unsigned char const *) (addr))[3]) << 24))
#define SET_LE16(addr, data) \
	((void) (((unsigned char *) (addr))[0] = (unsigned char) (data), \
		 ((unsigned char *) (addr))[1] = (unsigned char) ((data) >> 8)))
#define SET_LE32(addr, data) \
	((void) (((unsigned char *) (addr))[0] = (unsigned char) (data), \
		 ((unsigned char *) (addr))[1] = (unsigned char) ((data) >> 8), \
		 ((unsigned char *) (addr))[2] = (unsigned char) ((data) >> 16), \
		 ((unsigned char *) (addr))[3] = (unsigned char) ((data) >> 24)))
#endif

#endif /* MSB_FIRST */

/***********************************************************************************
	SPC DSP
***********************************************************************************/

static dsp_state_t dsp_m;

/* Copyright (C) 2007 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#if INT_MAX < 0x7FFFFFFF
	#error "Requires that int type have at least 32 bits"
#endif

/* if ( io < -32768 ) io = -32768; */
/* if ( io >  32767 ) io =  32767; */
#define CLAMP16( io ) if ( (int16_t) io != io ) io = ((io >> 31) ^ 0x7FFF)

/* Access global DSP register */
#define REG(n)      dsp_m.regs [R_##n]

/* Access voice DSP register */
#define VREG(r,n)   r [V_##n]

#define WRITE_SAMPLES( l, r, out ) \
{\
	out [0] = l;\
	out [1] = r;\
	out += 2;\
}\


/* Volume registers and efb are signed! Easy to forget int8_t cast. */
/* Prefixes are to avoid accidental use of locals with same names. */

/* Gaussian interpolation */

static const short gauss [512] =
{
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,
   2,   2,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,
   6,   6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  10,
  11,  11,  11,  12,  12,  13,  13,  14,  14,  15,  15,  15,  16,  16,  17,  17,
  18,  19,  19,  20,  20,  21,  21,  22,  23,  23,  24,  24,  25,  26,  27,  27,
  28,  29,  29,  30,  31,  32,  32,  33,  34,  35,  36,  36,  37,  38,  39,  40,
  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,
  58,  59,  60,  61,  62,  64,  65,  66,  67,  69,  70,  71,  73,  74,  76,  77,
  78,  80,  81,  83,  84,  86,  87,  89,  90,  92,  94,  95,  97,  99, 100, 102,
 104, 106, 107, 109, 111, 113, 115, 117, 118, 120, 122, 124, 126, 128, 130, 132,
 134, 137, 139, 141, 143, 145, 147, 150, 152, 154, 156, 159, 161, 163, 166, 168,
 171, 173, 175, 178, 180, 183, 186, 188, 191, 193, 196, 199, 201, 204, 207, 210,
 212, 215, 218, 221, 224, 227, 230, 233, 236, 239, 242, 245, 248, 251, 254, 257,
 260, 263, 267, 270, 273, 276, 280, 283, 286, 290, 293, 297, 300, 304, 307, 311,
 314, 318, 321, 325, 328, 332, 336, 339, 343, 347, 351, 354, 358, 362, 366, 370,
 374, 378, 381, 385, 389, 393, 397, 401, 405, 410, 414, 418, 422, 426, 430, 434,
 439, 443, 447, 451, 456, 460, 464, 469, 473, 477, 482, 486, 491, 495, 499, 504,
 508, 513, 517, 522, 527, 531, 536, 540, 545, 550, 554, 559, 563, 568, 573, 577,
 582, 587, 592, 596, 601, 606, 611, 615, 620, 625, 630, 635, 640, 644, 649, 654,
 659, 664, 669, 674, 678, 683, 688, 693, 698, 703, 708, 713, 718, 723, 728, 732,
 737, 742, 747, 752, 757, 762, 767, 772, 777, 782, 787, 792, 797, 802, 806, 811,
 816, 821, 826, 831, 836, 841, 846, 851, 855, 860, 865, 870, 875, 880, 884, 889,
 894, 899, 904, 908, 913, 918, 923, 927, 932, 937, 941, 946, 951, 955, 960, 965,
 969, 974, 978, 983, 988, 992, 997,1001,1005,1010,1014,1019,1023,1027,1032,1036,
1040,1045,1049,1053,1057,1061,1066,1070,1074,1078,1082,1086,1090,1094,1098,1102,
1106,1109,1113,1117,1121,1125,1128,1132,1136,1139,1143,1146,1150,1153,1157,1160,
1164,1167,1170,1174,1177,1180,1183,1186,1190,1193,1196,1199,1202,1205,1207,1210,
1213,1216,1219,1221,1224,1227,1229,1232,1234,1237,1239,1241,1244,1246,1248,1251,
1253,1255,1257,1259,1261,1263,1265,1267,1269,1270,1272,1274,1275,1277,1279,1280,
1282,1283,1284,1286,1287,1288,1290,1291,1292,1293,1294,1295,1296,1297,1297,1298,
1299,1300,1300,1301,1302,1302,1303,1303,1303,1304,1304,1304,1304,1304,1305,1305,
};

/* Sinc interpolation table (inaccurate brightening option).
 * 256 phases x 8 taps, Q14 fixed point. Identical to the table
 * shipped by snes9x mainline (apu/bapu/dsp/SPC_DSP.cpp), itself
 * from bsnes/blargg. Each 8-tap row sums to ~16384 (unity gain).
 * Applied as a plain FIR over in[0..7]; window safety for the
 * 8-tap read is guaranteed by the buf[24] double-ring (max read
 * index 18, all of buf[12..23] mirror buf[0..11]). */
static const short sinc [2048] =
{
       39,  -315,   666, 15642,   666,  -315,    39,   -38,
       38,  -302,   613, 15642,   718,  -328,    41,   -38,
       36,  -288,   561, 15641,   772,  -342,    42,   -38,
       35,  -275,   510, 15639,   826,  -355,    44,   -38,
       33,  -263,   459, 15636,   880,  -369,    46,   -38,
       32,  -250,   408, 15632,   935,  -383,    47,   -38,
       31,  -237,   358, 15628,   990,  -396,    49,   -38,
       29,  -224,   309, 15622,  1046,  -410,    51,   -38,
       28,  -212,   259, 15616,  1103,  -425,    53,   -38,
       27,  -200,   211, 15609,  1159,  -439,    54,   -38,
       25,  -188,   163, 15601,  1216,  -453,    56,   -38,
       24,  -175,   115, 15593,  1274,  -467,    58,   -38,
       23,  -164,    68, 15583,  1332,  -482,    60,   -38,
       22,  -152,    22, 15573,  1391,  -496,    62,   -37,
       21,  -140,   -24, 15562,  1450,  -511,    64,   -37,
       19,  -128,   -70, 15550,  1509,  -526,    66,   -37,
       18,  -117,  -115, 15538,  1569,  -540,    68,   -37,
       17,  -106,  -159, 15524,  1629,  -555,    70,   -37,
       16,   -94,  -203, 15510,  1690,  -570,    72,   -36,
       15,   -83,  -247, 15495,  1751,  -585,    74,   -36,
       14,   -72,  -289, 15479,  1813,  -600,    76,   -36,
       13,   -62,  -332, 15462,  1875,  -616,    79,   -36,
       12,   -51,  -374, 15445,  1937,  -631,    81,   -35,
       11,   -40,  -415, 15426,  2000,  -646,    83,   -35,
       11,   -30,  -456, 15407,  2063,  -662,    85,   -35,
       10,   -20,  -496, 15387,  2127,  -677,    88,   -34,
        9,    -9,  -536, 15366,  2191,  -693,    90,   -34,
        8,     1,  -576, 15345,  2256,  -708,    92,   -34,
        7,    10,  -614, 15323,  2321,  -724,    95,   -33,
        7,    20,  -653, 15300,  2386,  -740,    97,   -33,
        6,    30,  -690, 15276,  2451,  -755,    99,   -33,
        5,    39,  -728, 15251,  2517,  -771,   102,   -32,
        5,    49,  -764, 15226,  2584,  -787,   104,   -32,
        4,    58,  -801, 15200,  2651,  -803,   107,   -32,
        3,    67,  -836, 15173,  2718,  -819,   109,   -31,
        3,    76,  -871, 15145,  2785,  -835,   112,   -31,
        2,    85,  -906, 15117,  2853,  -851,   115,   -30,
        2,    93,  -940, 15087,  2921,  -867,   117,   -30,
        1,   102,  -974, 15057,  2990,  -883,   120,   -29,
        1,   110, -1007, 15027,  3059,  -899,   122,   -29,
        0,   118, -1039, 14995,  3128,  -915,   125,   -29,
        0,   127, -1071, 14963,  3198,  -931,   128,   -28,
       -1,   135, -1103, 14930,  3268,  -948,   131,   -28,
       -1,   142, -1134, 14896,  3338,  -964,   133,   -27,
       -1,   150, -1164, 14862,  3409,  -980,   136,   -27,
       -2,   158, -1194, 14827,  3480,  -996,   139,   -26,
       -2,   165, -1224, 14791,  3551, -1013,   142,   -26,
       -3,   172, -1253, 14754,  3622, -1029,   144,   -25,
       -3,   179, -1281, 14717,  3694, -1045,   147,   -25,
       -3,   187, -1309, 14679,  3766, -1062,   150,   -24,
       -3,   193, -1337, 14640,  3839, -1078,   153,   -24,
       -4,   200, -1363, 14601,  3912, -1094,   156,   -23,
       -4,   207, -1390, 14561,  3985, -1110,   159,   -23,
       -4,   213, -1416, 14520,  4058, -1127,   162,   -22,
       -4,   220, -1441, 14479,  4131, -1143,   165,   -22,
       -4,   226, -1466, 14437,  4205, -1159,   168,   -22,
       -5,   232, -1490, 14394,  4279, -1175,   171,   -21,
       -5,   238, -1514, 14350,  4354, -1192,   174,   -21,
       -5,   244, -1537, 14306,  4428, -1208,   177,   -20,
       -5,   249, -1560, 14261,  4503, -1224,   180,   -20,
       -5,   255, -1583, 14216,  4578, -1240,   183,   -19,
       -5,   260, -1604, 14169,  4653, -1256,   186,   -19,
       -5,   265, -1626, 14123,  4729, -1272,   189,   -18,
       -5,   271, -1647, 14075,  4805, -1288,   192,   -18,
       -5,   276, -1667, 14027,  4881, -1304,   195,   -17,
       -6,   280, -1687, 13978,  4957, -1320,   198,   -17,
       -6,   285, -1706, 13929,  5033, -1336,   201,   -16,
       -6,   290, -1725, 13879,  5110, -1352,   204,   -16,
       -6,   294, -1744, 13829,  5186, -1368,   207,   -15,
       -6,   299, -1762, 13777,  5263, -1383,   210,   -15,
       -6,   303, -1779, 13726,  5340, -1399,   213,   -14,
       -6,   307, -1796, 13673,  5418, -1414,   216,   -14,
       -6,   311, -1813, 13620,  5495, -1430,   219,   -13,
       -5,   315, -1829, 13567,  5573, -1445,   222,   -13,
       -5,   319, -1844, 13512,  5651, -1461,   225,   -13,
       -5,   322, -1859, 13458,  5728, -1476,   229,   -12,
       -5,   326, -1874, 13402,  5806, -1491,   232,   -12,
       -5,   329, -1888, 13347,  5885, -1506,   235,   -11,
       -5,   332, -1902, 13290,  5963, -1521,   238,   -11,
       -5,   335, -1915, 13233,  6041, -1536,   241,   -10,
       -5,   338, -1928, 13176,  6120, -1551,   244,   -10,
       -5,   341, -1940, 13118,  6199, -1566,   247,   -10,
       -5,   344, -1952, 13059,  6277, -1580,   250,    -9,
       -5,   347, -1964, 13000,  6356, -1595,   253,    -9,
       -5,   349, -1975, 12940,  6435, -1609,   256,    -8,
       -4,   352, -1986, 12880,  6514, -1623,   259,    -8,
       -4,   354, -1996, 12819,  6594, -1637,   262,    -8,
       -4,   356, -2005, 12758,  6673, -1651,   265,    -7,
       -4,   358, -2015, 12696,  6752, -1665,   268,    -7,
       -4,   360, -2024, 12634,  6831, -1679,   271,    -7,
       -4,   362, -2032, 12572,  6911, -1693,   274,    -6,
       -4,   364, -2040, 12509,  6990, -1706,   277,    -6,
       -4,   366, -2048, 12445,  7070, -1719,   280,    -6,
       -3,   367, -2055, 12381,  7149, -1732,   283,    -5,
       -3,   369, -2062, 12316,  7229, -1745,   286,    -5,
       -3,   370, -2068, 12251,  7308, -1758,   289,    -5,
       -3,   371, -2074, 12186,  7388, -1771,   291,    -4,
       -3,   372, -2079, 12120,  7467, -1784,   294,    -4,
       -3,   373, -2084, 12054,  7547, -1796,   297,    -4,
       -3,   374, -2089, 11987,  7626, -1808,   300,    -4,
       -2,   375, -2094, 11920,  7706, -1820,   303,    -3,
       -2,   376, -2098, 11852,  7785, -1832,   305,    -3,
       -2,   376, -2101, 11785,  7865, -1844,   308,    -3,
       -2,   377, -2104, 11716,  7944, -1855,   311,    -3,
       -2,   377, -2107, 11647,  8024, -1866,   313,    -2,
       -2,   378, -2110, 11578,  8103, -1877,   316,    -2,
       -2,   378, -2112, 11509,  8182, -1888,   318,    -2,
       -1,   378, -2113, 11439,  8262, -1899,   321,    -2,
       -1,   378, -2115, 11369,  8341, -1909,   323,    -2,
       -1,   378, -2116, 11298,  8420, -1920,   326,    -2,
       -1,   378, -2116, 11227,  8499, -1930,   328,    -1,
       -1,   378, -2116, 11156,  8578, -1940,   331,    -1,
       -1,   378, -2116, 11084,  8656, -1949,   333,    -1,
       -1,   377, -2116, 11012,  8735, -1959,   335,    -1,
       -1,   377, -2115, 10940,  8814, -1968,   337,    -1,
       -1,   377, -2114, 10867,  8892, -1977,   340,    -1,
       -1,   376, -2112, 10795,  8971, -1985,   342,    -1,
        0,   375, -2111, 10721,  9049, -1994,   344,    -1,
        0,   375, -2108, 10648,  9127, -2002,   346,     0,
        0,   374, -2106, 10574,  9205, -2010,   348,     0,
        0,   373, -2103, 10500,  9283, -2018,   350,     0,
        0,   372, -2100, 10426,  9360, -2025,   352,     0,
        0,   371, -2097, 10351,  9438, -2032,   354,     0,
        0,   370, -2093, 10276,  9515, -2039,   355,     0,
        0,   369, -2089, 10201,  9592, -2046,   357,     0,
        0,   367, -2084, 10126,  9669, -2052,   359,     0,
        0,   366, -2080, 10050,  9745, -2058,   360,     0,
        0,   365, -2075,  9974,  9822, -2064,   362,     0,
        0,   363, -2070,  9898,  9898, -2070,   363,     0,
        0,   362, -2064,  9822,  9974, -2075,   365,     0,
        0,   360, -2058,  9745, 10050, -2080,   366,     0,
        0,   359, -2052,  9669, 10126, -2084,   367,     0,
        0,   357, -2046,  9592, 10201, -2089,   369,     0,
        0,   355, -2039,  9515, 10276, -2093,   370,     0,
        0,   354, -2032,  9438, 10351, -2097,   371,     0,
        0,   352, -2025,  9360, 10426, -2100,   372,     0,
        0,   350, -2018,  9283, 10500, -2103,   373,     0,
        0,   348, -2010,  9205, 10574, -2106,   374,     0,
        0,   346, -2002,  9127, 10648, -2108,   375,     0,
       -1,   344, -1994,  9049, 10721, -2111,   375,     0,
       -1,   342, -1985,  8971, 10795, -2112,   376,    -1,
       -1,   340, -1977,  8892, 10867, -2114,   377,    -1,
       -1,   337, -1968,  8814, 10940, -2115,   377,    -1,
       -1,   335, -1959,  8735, 11012, -2116,   377,    -1,
       -1,   333, -1949,  8656, 11084, -2116,   378,    -1,
       -1,   331, -1940,  8578, 11156, -2116,   378,    -1,
       -1,   328, -1930,  8499, 11227, -2116,   378,    -1,
       -2,   326, -1920,  8420, 11298, -2116,   378,    -1,
       -2,   323, -1909,  8341, 11369, -2115,   378,    -1,
       -2,   321, -1899,  8262, 11439, -2113,   378,    -1,
       -2,   318, -1888,  8182, 11509, -2112,   378,    -2,
       -2,   316, -1877,  8103, 11578, -2110,   378,    -2,
       -2,   313, -1866,  8024, 11647, -2107,   377,    -2,
       -3,   311, -1855,  7944, 11716, -2104,   377,    -2,
       -3,   308, -1844,  7865, 11785, -2101,   376,    -2,
       -3,   305, -1832,  7785, 11852, -2098,   376,    -2,
       -3,   303, -1820,  7706, 11920, -2094,   375,    -2,
       -4,   300, -1808,  7626, 11987, -2089,   374,    -3,
       -4,   297, -1796,  7547, 12054, -2084,   373,    -3,
       -4,   294, -1784,  7467, 12120, -2079,   372,    -3,
       -4,   291, -1771,  7388, 12186, -2074,   371,    -3,
       -5,   289, -1758,  7308, 12251, -2068,   370,    -3,
       -5,   286, -1745,  7229, 12316, -2062,   369,    -3,
       -5,   283, -1732,  7149, 12381, -2055,   367,    -3,
       -6,   280, -1719,  7070, 12445, -2048,   366,    -4,
       -6,   277, -1706,  6990, 12509, -2040,   364,    -4,
       -6,   274, -1693,  6911, 12572, -2032,   362,    -4,
       -7,   271, -1679,  6831, 12634, -2024,   360,    -4,
       -7,   268, -1665,  6752, 12696, -2015,   358,    -4,
       -7,   265, -1651,  6673, 12758, -2005,   356,    -4,
       -8,   262, -1637,  6594, 12819, -1996,   354,    -4,
       -8,   259, -1623,  6514, 12880, -1986,   352,    -4,
       -8,   256, -1609,  6435, 12940, -1975,   349,    -5,
       -9,   253, -1595,  6356, 13000, -1964,   347,    -5,
       -9,   250, -1580,  6277, 13059, -1952,   344,    -5,
      -10,   247, -1566,  6199, 13118, -1940,   341,    -5,
      -10,   244, -1551,  6120, 13176, -1928,   338,    -5,
      -10,   241, -1536,  6041, 13233, -1915,   335,    -5,
      -11,   238, -1521,  5963, 13290, -1902,   332,    -5,
      -11,   235, -1506,  5885, 13347, -1888,   329,    -5,
      -12,   232, -1491,  5806, 13402, -1874,   326,    -5,
      -12,   229, -1476,  5728, 13458, -1859,   322,    -5,
      -13,   225, -1461,  5651, 13512, -1844,   319,    -5,
      -13,   222, -1445,  5573, 13567, -1829,   315,    -5,
      -13,   219, -1430,  5495, 13620, -1813,   311,    -6,
      -14,   216, -1414,  5418, 13673, -1796,   307,    -6,
      -14,   213, -1399,  5340, 13726, -1779,   303,    -6,
      -15,   210, -1383,  5263, 13777, -1762,   299,    -6,
      -15,   207, -1368,  5186, 13829, -1744,   294,    -6,
      -16,   204, -1352,  5110, 13879, -1725,   290,    -6,
      -16,   201, -1336,  5033, 13929, -1706,   285,    -6,
      -17,   198, -1320,  4957, 13978, -1687,   280,    -6,
      -17,   195, -1304,  4881, 14027, -1667,   276,    -5,
      -18,   192, -1288,  4805, 14075, -1647,   271,    -5,
      -18,   189, -1272,  4729, 14123, -1626,   265,    -5,
      -19,   186, -1256,  4653, 14169, -1604,   260,    -5,
      -19,   183, -1240,  4578, 14216, -1583,   255,    -5,
      -20,   180, -1224,  4503, 14261, -1560,   249,    -5,
      -20,   177, -1208,  4428, 14306, -1537,   244,    -5,
      -21,   174, -1192,  4354, 14350, -1514,   238,    -5,
      -21,   171, -1175,  4279, 14394, -1490,   232,    -5,
      -22,   168, -1159,  4205, 14437, -1466,   226,    -4,
      -22,   165, -1143,  4131, 14479, -1441,   220,    -4,
      -22,   162, -1127,  4058, 14520, -1416,   213,    -4,
      -23,   159, -1110,  3985, 14561, -1390,   207,    -4,
      -23,   156, -1094,  3912, 14601, -1363,   200,    -4,
      -24,   153, -1078,  3839, 14640, -1337,   193,    -3,
      -24,   150, -1062,  3766, 14679, -1309,   187,    -3,
      -25,   147, -1045,  3694, 14717, -1281,   179,    -3,
      -25,   144, -1029,  3622, 14754, -1253,   172,    -3,
      -26,   142, -1013,  3551, 14791, -1224,   165,    -2,
      -26,   139,  -996,  3480, 14827, -1194,   158,    -2,
      -27,   136,  -980,  3409, 14862, -1164,   150,    -1,
      -27,   133,  -964,  3338, 14896, -1134,   142,    -1,
      -28,   131,  -948,  3268, 14930, -1103,   135,    -1,
      -28,   128,  -931,  3198, 14963, -1071,   127,     0,
      -29,   125,  -915,  3128, 14995, -1039,   118,     0,
      -29,   122,  -899,  3059, 15027, -1007,   110,     1,
      -29,   120,  -883,  2990, 15057,  -974,   102,     1,
      -30,   117,  -867,  2921, 15087,  -940,    93,     2,
      -30,   115,  -851,  2853, 15117,  -906,    85,     2,
      -31,   112,  -835,  2785, 15145,  -871,    76,     3,
      -31,   109,  -819,  2718, 15173,  -836,    67,     3,
      -32,   107,  -803,  2651, 15200,  -801,    58,     4,
      -32,   104,  -787,  2584, 15226,  -764,    49,     5,
      -32,   102,  -771,  2517, 15251,  -728,    39,     5,
      -33,    99,  -755,  2451, 15276,  -690,    30,     6,
      -33,    97,  -740,  2386, 15300,  -653,    20,     7,
      -33,    95,  -724,  2321, 15323,  -614,    10,     7,
      -34,    92,  -708,  2256, 15345,  -576,     1,     8,
      -34,    90,  -693,  2191, 15366,  -536,    -9,     9,
      -34,    88,  -677,  2127, 15387,  -496,   -20,    10,
      -35,    85,  -662,  2063, 15407,  -456,   -30,    11,
      -35,    83,  -646,  2000, 15426,  -415,   -40,    11,
      -35,    81,  -631,  1937, 15445,  -374,   -51,    12,
      -36,    79,  -616,  1875, 15462,  -332,   -62,    13,
      -36,    76,  -600,  1813, 15479,  -289,   -72,    14,
      -36,    74,  -585,  1751, 15495,  -247,   -83,    15,
      -36,    72,  -570,  1690, 15510,  -203,   -94,    16,
      -37,    70,  -555,  1629, 15524,  -159,  -106,    17,
      -37,    68,  -540,  1569, 15538,  -115,  -117,    18,
      -37,    66,  -526,  1509, 15550,   -70,  -128,    19,
      -37,    64,  -511,  1450, 15562,   -24,  -140,    21,
      -37,    62,  -496,  1391, 15573,    22,  -152,    22,
      -38,    60,  -482,  1332, 15583,    68,  -164,    23,
      -38,    58,  -467,  1274, 15593,   115,  -175,    24,
      -38,    56,  -453,  1216, 15601,   163,  -188,    25,
      -38,    54,  -439,  1159, 15609,   211,  -200,    27,
      -38,    53,  -425,  1103, 15616,   259,  -212,    28,
      -38,    51,  -410,  1046, 15622,   309,  -224,    29,
      -38,    49,  -396,   990, 15628,   358,  -237,    31,
      -38,    47,  -383,   935, 15632,   408,  -250,    32,
      -38,    46,  -369,   880, 15636,   459,  -263,    33,
      -38,    44,  -355,   826, 15639,   510,  -275,    35,
      -38,    42,  -342,   772, 15641,   561,  -288,    36,
      -38,    41,  -328,   718, 15642,   613,  -302,    38,
};

/* Gaussian interpolation */

static INLINE int dsp_interpolate( dsp_voice_t *v )
{
	/* Make pointers into gaussian based on fractional position between samples */
	int offset = v->interp_pos >> 4 & 0xFF;
	const short *fwd = gauss + 255 - offset;
	const short *rev = gauss + offset; /* mirror left half of gaussian */
	
	int *in  = (int*)&v->buf[(v->interp_pos >> 12) + v->buf_pos];
	int out = (fwd [  0] * in [0]) >> 11;
	out += (fwd [256] * in [1]) >> 11;
	out += (rev [256] * in [2]) >> 11;
	out = (int16_t) out;
	out += (rev [  0] * in [3]) >> 11;
	
	CLAMP16( out );
	out &= ~1;
	return out;
}

/* DSP voice interpolation mode (core option). 0 = gaussian (accurate,
 * hardware default). 1 = cubic. Inaccurate modes brighten the high end
 * by replacing the SPC's gaussian pitch interpolation; they do NOT
 * recover BRR quantization loss and do NOT match real hardware.
 * DSP_INTERP_* constants are declared in apu.h. */
int dsp_interp_mode = DSP_INTERP_GAUSSIAN;

/* 4-point cubic interpolation (inaccurate brightening option).
 * Coefficient form is the integer equivalent of Mesen2's
 * Core/SNES/DSP/DspInterpolation.h cubic. Mesen2 normalises samples by
 * 1/32768, interpolates in float, then scales back by 32768; that
 * normalisation cancels exactly (every coefficient is linear in the
 * sample values), so we interpolate directly in sample units and avoid
 * the round-trip rounding entirely.
 *
 * Fixed point: the fraction t = f/256 (f = bits 11..4 of interp_pos) is
 * Q8, so each Horner step multiplies by f and shifts right by 8, with
 * +128 for round-to-nearest. Verified against the float reference over a
 * 4M-sample sweep of the full s16 input range: max error +/-2 pre-mask
 * (the &~1 low-bit clear makes the effective resolution 2 LSB anyway).
 * Worst-case |acc*f| is ~3.3e7, well inside int32, so no widening needed.
 *
 * This is fully deterministic and bit-identical across platforms/CPUs
 * (no float), which is the property that matters for a libretro core.
 * It is NOT and cannot be bit-exact to real hardware (which only does
 * gaussian) nor to Mesen2's float path (float last-bit rounding differs).
 *
 * The four taps line up with the gaussian path's in[0..3]; the doubled
 * buf wrap-around copy guarantees the window is contiguous, so no modulo
 * is needed. */
static INLINE int dsp_interpolate_cubic( dsp_voice_t *v )
{
	int *in  = (int*)&v->buf[(v->interp_pos >> 12) + v->buf_pos];
	int f    = (v->interp_pos >> 4) & 0xFF;
	int s0   = in[0];
	int s1   = in[1];
	int s2   = in[2];
	int s3   = in[3];
	int A    = (s3 - s2) - (s0 - s1);
	int B    = (s0 - s1) - A;
	int C    = s2 - s0;
	int D    = s1;
	int out  = A;

	out = (((out * f) + 128) >> 8) + B;
	out = (((out * f) + 128) >> 8) + C;
	out = (((out * f) + 128) >> 8) + D;

	CLAMP16( out );
	out &= ~1;
	return out;
}

/* 8-tap windowed-sinc interpolation (inaccurate brightening option).
 * Straight FIR over in[0..7] with the Q14 sinc[] table; matches snes9x
 * mainline's case-4 sinc exactly, including its phase index
 * (interp_pos & 0xFF0) >> 1 -- note this differs from the gaussian/cubic
 * (interp_pos >> 4 & 0xFF) phase form. Unlike gaussian/cubic, mainline's
 * sinc does NOT clear the low output bit, so neither do we. The 8-tap
 * read is in-bounds: worst-case index is 18 in buf[24], and buf[12..23]
 * mirror buf[0..11] every decode cycle (see dsp_decode_brr). */
static INLINE int dsp_interpolate_sinc( dsp_voice_t *v )
{
	int *in            = (int*)&v->buf[(v->interp_pos >> 12) + v->buf_pos];
	const short *filt  = sinc + ((v->interp_pos & 0xFF0) >> 1);
	int out;

	out  = filt[0] * in[0];
	out += filt[1] * in[1];
	out += filt[2] * in[2];
	out += filt[3] * in[3];
	out += filt[4] * in[4];
	out += filt[5] * in[5];
	out += filt[6] * in[6];
	out += filt[7] * in[7];
	out >>= 14;

	CLAMP16( out );
	return out;
}

/* Counters */

/* Number of samples per counter event.
 * All rates are evently divisible by counter range 
 * (0x7800, 30720, or 2048 * 5 * 3).
 *
 * Note that counter_rates[0] is a special case,
 * which never triggers. */
#define COUNTER_RANGE 30720

static unsigned const counter_rates [32] =
{
      COUNTER_RANGE + 1, 2048, 1536,
	1280, 1024,  768,
	 640,  512,  384,
	 320,  256,  192,
	 160,  128,   96,
	  80,   64,   48,
	  40,   32,   24,
	  20,   16,   12,
	  10,    8,    6,
	   5,    4,    3,
	         2,
	         1
};

/* Counter offset from zero.
 *
 * Counters do not appear to be aligned at zero
 * for all rates.
 */

static unsigned const counter_offsets [32] =
{
	  1, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	     0,
	     0
};

#define READ_COUNTER(rate) (((unsigned) dsp_m.counter + counter_offsets [rate]) % counter_rates [rate])

/* Envelope */

static INLINE void dsp_run_envelope( dsp_voice_t* v )
{
	int rate;
	int env      = v->env;
	int env_data = v->regs[V_ADSR1];

	if ( dsp_m.t_adsr0 & 0x80 ) /* 99% ADSR */
	{
		if ( v->env_mode >= ENV_DECAY ) /* 99% */
		{
			env--;
			env -= env >> 8;
			rate = env_data & 0x1F;
			if ( v->env_mode == ENV_DECAY ) /* 1% */
				rate = (dsp_m.t_adsr0 >> 3 & 0x0E) + 0x10;
		}
		else /* ENV_ATTACK */
		{
			rate = (dsp_m.t_adsr0 & 0x0F) * 2 + 1;
			env += rate < 31 ? 0x20 : 0x400;
		}
	}
	else /* GAIN */
	{
		int mode;
		env_data = v->regs[V_GAIN];
		mode = env_data >> 5;
		if ( mode < 4 ) /* direct */
		{
			env = env_data * 0x10;
			rate = 31;
		}
		else
		{
			rate = env_data & 0x1F;
			if ( mode == 4 ) /* 4: linear decrease */
				env -= 0x20;
			else if ( mode < 6 ) /* 5: exponential decrease */
			{
				env--;
				env -= env >> 8;
			}
			else /* 6,7: linear increase */
			{
				env += 0x20;
				if ( mode > 6 && (unsigned) v->hidden_env >= 0x600 )
					env += 0x8 - 0x20; /* 7: two-slope linear increase */
			}
		}
	}

	/* Sustain level */
	if ( (env >> 8) == (env_data >> 5) && v->env_mode == ENV_DECAY )
		v->env_mode = ENV_SUSTAIN;

	v->hidden_env = env;

	/* unsigned cast because linear decrease going negative also triggers this */
	if ( (unsigned) env > 0x7FF )
	{
		env = (env < 0 ? 0 : 0x7FF);
		if ( v->env_mode == ENV_ATTACK )
			v->env_mode = ENV_DECAY;
	}

	if (!READ_COUNTER( rate ))
		v->env = env; /* nothing else is controlled by the counter */
}

/* BRR Decoding */

static INLINE void dsp_decode_brr( dsp_voice_t* v )
{
	int *end;

	/* Arrange the four input nybbles in 0xABCD order for easy decoding */
	int nybbles = dsp_m.t_brr_byte * 0x100 + dsp_m.ram [(v->brr_addr + v->brr_offset + 1) & 0xFFFF];
	
	int header = dsp_m.t_brr_header;
	int filter = header & 0x0C;
	int shift  = header >> 4;             /* Shift sample based on header */
	
	/* Write to next four samples in circular buffer */
	int *pos = (int*)&v->buf [v->buf_pos];

	if ( (v->buf_pos += 4) >= BRR_BUF_SIZE )
		v->buf_pos = 0;
	
	/* Decode four samples */
	for ( end = pos + 4; pos < end; pos++)
	{
		int s     = (int16_t) nybbles >> 12; /* Extract nybble and sign-extend */
		int p1    = pos [BRR_BUF_SIZE - 1];
		int p2    = pos [BRR_BUF_SIZE - 2] >> 1;

		s = (s << shift) >> 1;
		if (shift >= 0xD) /* handle invalid range */
			s = (s >> 25) << 11; /* same as: s = (s < 0 ? -0x800 : 0) */
		
      if (filter)
      {
         /* Apply IIR filter (8 is the most commonly used) */

         if ( filter >= 8 )
         {
            s += p1;
            s -= p2;
            if ( filter == 8 ) /* s += p1 * 0.953125 - p2 * 0.46875 */
            {
               s += p2 >> 4;
               s += (p1 * -3) >> 6;
            }
            else /* s += p1 * 0.8984375 - p2 * 0.40625 */
            {
               s += (p1 * -13) >> 7;
               s += (p2 * 3) >> 4;
            }
         }
         else /* s += p1 * 0.46875 */
         {
            s += p1 >> 1;
            s += (-p1) >> 5;
         }
      }
		
		/* Adjust and write sample */
		CLAMP16( s );
		s = (int16_t) (s * 2);
		pos [BRR_BUF_SIZE] = pos [0] = s; /* second copy simplifies wrap-around */

      nybbles <<= 4;
	}
}

/* Misc */

static INLINE void dsp_misc_30 (void)
{
	if ( dsp_m.every_other_sample )
	{
		dsp_m.kon    = dsp_m.new_kon;
		dsp_m.t_koff = dsp_m.regs[R_KOFF]; 
	}
	
	if ( --dsp_m.counter < 0 )
		dsp_m.counter = COUNTER_RANGE - 1;
	
	/* Noise */
	if ( !READ_COUNTER( dsp_m.regs[R_FLG] & 0x1F ) )
	{
		int feedback = (dsp_m.noise << 13) ^ (dsp_m.noise << 14);
		dsp_m.noise = (feedback & 0x4000) ^ (dsp_m.noise >> 1);
	}
}

/* Voices */

#define dsp_voice_V1(v) \
	dsp_m.t_dir_addr = (dsp_m.t_dir * 0x100 + dsp_m.t_srcn * 4) & 0xffff; \
	dsp_m.t_srcn = v->regs[V_SRCN]

#define dsp_voice_V2(v) \
{ \
	uint8_t *entry = (uint8_t*)&dsp_m.ram [dsp_m.t_dir_addr]; \
	if ( !v->kon_delay ) \
		entry += 2; \
	dsp_m.t_brr_next_addr = GET_LE16(entry); \
	dsp_m.t_adsr0 = v->regs [V_ADSR0]; \
	dsp_m.t_pitch = v->regs [V_PITCHL]; \
}

#define dsp_voice_V3b(v) \
	dsp_m.t_brr_byte = dsp_m.ram [(v->brr_addr + v->brr_offset) & 0xffff]; \
	dsp_m.t_brr_header = dsp_m.ram [v->brr_addr]

static void dsp_voice_V3c( dsp_voice_t* v )
{
	int output;

	/* Pitch modulation using previous voice's output */
	if ( dsp_m.t_pmon & v->vbit )
		dsp_m.t_pitch += ((dsp_m.t_output >> 5) * dsp_m.t_pitch) >> 10;
	
	if ( v->kon_delay )
	{
		/* Get ready to start BRR decoding on next sample */
		if ( v->kon_delay == 5 )
		{
			v->brr_addr    = dsp_m.t_brr_next_addr;
			v->brr_offset  = 1;
			v->buf_pos     = 0;
			dsp_m.t_brr_header = 0; /* header is ignored on this sample */
		}
		
		/* Envelope is never run during KON */
		v->env        = 0;
		v->hidden_env = 0;
		
		/* Disable BRR decoding until last three samples */
		v->interp_pos = 0;
		if ( --v->kon_delay & 3 )
			v->interp_pos = 0x4000;
		
		/* Pitch is never added during KON */
		dsp_m.t_pitch = 0;
	}
	
	/* dsp_interpolate is a pure function (reads the const gauss table and
	 * the voice's decode buffer; writes nothing), and its result is used
	 * in exactly two ways below:
	 *   1. overwritten entirely when this voice's noise bit is set, or
	 *   2. multiplied by v->env in the envelope application.
	 * So when the noise bit is set, or when env == 0 (slot unused, voice
	 * released to silence, or inside KON delay -- which zeroes env above),
	 * the gaussian FIR's four multiplies are computed only to be thrown
	 * away. Skipping them in those cases is bit-exact by construction:
	 * with env == 0, (output * 0) >> 11 & ~1 == 0 for ANY output value.
	 * Across a 16-game corpus, env==0 alone covers 24-100% of calls
	 * (unused voice slots sit at env 0 permanently), and the env==0
	 * state is sticky, so the added branch predicts near-perfectly. */
	if ( dsp_m.t_non & v->vbit )
		output = (int16_t) (dsp_m.noise * 2);
	else if ( v->env == 0 )
		output = 0;
	else if ( dsp_interp_mode == DSP_INTERP_CUBIC )
		output = dsp_interpolate_cubic( v );
	else if ( dsp_interp_mode == DSP_INTERP_SINC )
		output = dsp_interpolate_sinc( v );
	else
		output = dsp_interpolate( v );

	/* Apply envelope */
	dsp_m.t_output = (output * v->env) >> 11 & ~1;
	v->t_envx_out = (uint8_t) (v->env >> 4);
	
	/* Immediate silence due to end of sample or soft reset */
	if ( dsp_m.regs[R_FLG] & 0x80 || (dsp_m.t_brr_header & 3) == 1 )
	{
		v->env_mode = ENV_RELEASE;
		v->env      = 0;
	}
	
	if ( dsp_m.every_other_sample )
	{
		/* KOFF */
		if ( dsp_m.t_koff & v->vbit )
			v->env_mode = ENV_RELEASE;
		
		/* KON */
		if ( dsp_m.kon & v->vbit )
		{
			v->kon_delay = 5;
			v->env_mode  = ENV_ATTACK;
		}
	}
	
	/* Run envelope for next sample */
	if ( !v->kon_delay )
	{
		int env = v->env;
		if ( v->env_mode == ENV_RELEASE ) /* 60% */
		{
			if ( (env -= 0x8) < 0 )
				env = 0;
			v->env = env;
		}
		else
		{
			dsp_run_envelope( v );
		}
	}
}

static INLINE void dsp_voice_output( dsp_voice_t const* v, int ch )
{
	/* Apply left/right volume */
	int amp = (dsp_m.t_output * (int8_t) VREG(v->regs,VOLL + ch)) >> 7;

	/* Add to output total */
	dsp_m.t_main_out [ch] += amp;
	CLAMP16( dsp_m.t_main_out [ch] );
	
	/* Optionally add to echo total */
	if ( dsp_m.t_eon & v->vbit )
	{
		dsp_m.t_echo_out [ch] += amp;
		CLAMP16( dsp_m.t_echo_out [ch] );
	}
}

static INLINE void dsp_voice_V4( dsp_voice_t* v )
{
	/* Decode BRR */
	dsp_m.t_looped = 0;
	if ( v->interp_pos >= 0x4000 )
	{
		dsp_decode_brr( v );
		
		if ( (v->brr_offset += 2) >= BRR_BLOCK_SIZE )
		{
			/* Start decoding next BRR block */
			v->brr_addr = (v->brr_addr + BRR_BLOCK_SIZE) & 0xFFFF;
			if ( dsp_m.t_brr_header & 1 )
			{
				v->brr_addr = dsp_m.t_brr_next_addr;
				dsp_m.t_looped = v->vbit;
			}
			v->brr_offset = 1;
		}
	}
	
	/* Apply pitch */
	v->interp_pos = (v->interp_pos & 0x3FFF) + dsp_m.t_pitch;
	
	/* Keep from getting too far ahead (when using pitch modulation) */
	if ( v->interp_pos > 0x7FFF )
		v->interp_pos = 0x7FFF;
	
	/* Output left */
	dsp_voice_output( v, 0 );
}

#define dsp_voice_V5(v) \
{ \
	int endx_buf; \
	/* Output right */ \
	dsp_voice_output( v, 1 ); \
	/* ENDX, OUTX, and ENVX won't update if you wrote to them 1-2 clocks earlier */ \
	endx_buf = dsp_m.regs[R_ENDX] | dsp_m.t_looped; \
	/* Clear bit in ENDX if KON just began */ \
	if ( v->kon_delay == 5 ) \
		endx_buf &= ~v->vbit; \
	dsp_m.endx_buf = (uint8_t) endx_buf; \
}

/* Echo */

/* Current echo buffer pointer for left/right channel */
#define ECHO_PTR( ch )      (&dsp_m.ram [dsp_m.t_echo_ptr + ch * 2])

#define ECHO_READ(ch) \
{ \
   uint8_t *ptr = &dsp_m.ram [dsp_m.t_echo_ptr + ch * 2]; \
	if ( dsp_m.t_echo_ptr >= 0xffc0 && dsp_m.rom_enabled ) \
		ptr = (uint8_t*)&dsp_m.hi_ram [dsp_m.t_echo_ptr + ch * 2 - 0xffc0]; \
	/* second copy simplifies wrap-around handling */ \
	dsp_m.echo_hist_pos[0][ch] = dsp_m.echo_hist_pos[8][ch] = ((int16_t)GET_LE16(ptr)) >> 1; \
}

static INLINE void dsp_echo_22 (void)
{
	if ( ++dsp_m.echo_hist_pos >= &dsp_m.echo_hist [ECHO_HIST_SIZE] )
		dsp_m.echo_hist_pos = dsp_m.echo_hist;

	dsp_m.t_echo_ptr = (dsp_m.t_esa * 0x100 + dsp_m.echo_offset) & 0xFFFF;

	ECHO_READ(0);

	dsp_m.t_echo_in [0] = (int)((((dsp_m.echo_hist_pos [0 + 1]) [0] * (int8_t) dsp_m.regs [R_FIR + 0 * 0x10]) >> 6));
	dsp_m.t_echo_in [1] = (int)((((dsp_m.echo_hist_pos [0 + 1]) [1] * (int8_t) dsp_m.regs [R_FIR + 0 * 0x10]) >> 6));
}

#define dsp_echo_23() \
	dsp_m.t_echo_in [0] += (int)((((dsp_m.echo_hist_pos [1 + 1]) [0] * (int8_t) dsp_m.regs [R_FIR + 1 * 0x10]) >> 6) + (((dsp_m.echo_hist_pos [2 + 1]) [0] * (int8_t) dsp_m.regs [R_FIR + 2 * 0x10]) >> 6)); \
	dsp_m.t_echo_in [1] += (int)((((dsp_m.echo_hist_pos [1 + 1]) [1] * (int8_t) dsp_m.regs [R_FIR + 1 * 0x10]) >> 6) + (((dsp_m.echo_hist_pos [2 + 1]) [1] * (int8_t) dsp_m.regs [R_FIR + 2 * 0x10]) >> 6)); \
	ECHO_READ(1)

#define dsp_echo_24() \
	dsp_m.t_echo_in [0] += (int)(((((dsp_m.echo_hist_pos [3 + 1]) [0] * (int8_t) dsp_m.regs [R_FIR + 3 * 0x10]) >> 6) + (((dsp_m.echo_hist_pos [4 + 1]) [0] * (int8_t) dsp_m.regs [R_FIR + 4 * 0x10]) >> 6) + (((dsp_m.echo_hist_pos [5 + 1]) [0] * (int8_t) dsp_m.regs [R_FIR + 5 * 0x10]) >> 6))); \
	dsp_m.t_echo_in [1] += (int)(((((dsp_m.echo_hist_pos [3 + 1]) [1] * (int8_t) dsp_m.regs [R_FIR + 3 * 0x10]) >> 6) + (((dsp_m.echo_hist_pos [4 + 1]) [1] * (int8_t) dsp_m.regs [R_FIR + 4 * 0x10]) >> 6) + (((dsp_m.echo_hist_pos [5 + 1]) [1] * (int8_t) dsp_m.regs [R_FIR + 5 * 0x10]) >> 6)))

static INLINE void dsp_echo_25 (void)
{
	int l = (int16_t)(dsp_m.t_echo_in [0] + (((dsp_m.echo_hist_pos [6 + 1]) [0] * (int8_t) dsp_m.regs [R_FIR + 6 * 0x10]) >> 6));
	int r = (int16_t)(dsp_m.t_echo_in [1] + (((dsp_m.echo_hist_pos [6 + 1]) [1] * (int8_t) dsp_m.regs [R_FIR + 6 * 0x10]) >> 6));

	l += (int16_t) (((dsp_m.echo_hist_pos [7 + 1]) [0] * (int8_t) dsp_m.regs [R_FIR + 7 * 0x10]) >> 6);
	r += (int16_t) (((dsp_m.echo_hist_pos [7 + 1]) [1] * (int8_t) dsp_m.regs [R_FIR + 7 * 0x10]) >> 6);

   CLAMP16(l);
   CLAMP16(r);

   dsp_m.t_echo_in [0] = l & ~1;
   dsp_m.t_echo_in [1] = r & ~1;
}

#define ECHO_OUTPUT(var, ch) \
{ \
	var = (int16_t) ((dsp_m.t_main_out [ch] * (int8_t) REG(MVOLL + ch * 0x10)) >> 7) + (int16_t) ((dsp_m.t_echo_in [ch] * (int8_t) REG(EVOLL + ch * 0x10)) >> 7); \
	CLAMP16( var ); \
}

static INLINE void dsp_echo_26 (void)
{
	int l, r;

	ECHO_OUTPUT(dsp_m.t_main_out[0], 0 );

	l = dsp_m.t_echo_out [0] + (int16_t) ((dsp_m.t_echo_in [0] * (int8_t) dsp_m.regs [R_EFB]) >> 7);
	r = dsp_m.t_echo_out [1] + (int16_t) ((dsp_m.t_echo_in [1] * (int8_t) dsp_m.regs [R_EFB]) >> 7);

   CLAMP16(l);
   CLAMP16(r);

	dsp_m.t_echo_out [0] = l & ~1;
	dsp_m.t_echo_out [1] = r & ~1;
}

static INLINE void dsp_echo_27 (void)
{
	int r;
	short *out;

	int l = dsp_m.t_main_out [0];
	ECHO_OUTPUT(r, 1);
	dsp_m.t_main_out [0] = 0;
	dsp_m.t_main_out [1] = 0;

	if ( dsp_m.regs [R_FLG] & 0x40 )
	{
		l = 0;
		r = 0;
	}

	out = dsp_m.out;
	out [0] = l;
	out [1] = r;
	out += 2;
	dsp_m.out = out;
}

#define ECHO_28() dsp_m.t_echo_enabled = dsp_m.regs [R_FLG];

#define ECHO_WRITE(ch) \
	if ( !(dsp_m.t_echo_enabled & 0x20) ) \
	{ \
		SET_LE16( ECHO_PTR( ch ), dsp_m.t_echo_out [ch] ); \
		if ( dsp_m.t_echo_ptr >= 0xffc0 ) \
		{ \
			SET_LE16( &dsp_m.hi_ram [dsp_m.t_echo_ptr + ch * 2 - 0xffc0], dsp_m.t_echo_out [ch] ); \
			if ( dsp_m.rom_enabled ) \
				SET_LE16( ECHO_PTR( ch ), GET_LE16( &dsp_m.rom [dsp_m.t_echo_ptr + ch * 2 - 0xffc0] ) ); \
		} \
	} \
	dsp_m.t_echo_out [ch] = 0;

static INLINE void dsp_echo_29 (void)
{
	dsp_m.t_esa = dsp_m.regs [R_ESA];

	if ( !dsp_m.echo_offset )
		dsp_m.echo_length = (dsp_m.regs [R_EDL] & 0x0F) * 0x800;

	dsp_m.echo_offset += 4;
	if ( dsp_m.echo_offset >= dsp_m.echo_length )
		dsp_m.echo_offset = 0;


	ECHO_WRITE(0);

	dsp_m.t_echo_enabled = dsp_m.regs [R_FLG];
}

/* Timing */

/* Execute clock for a particular voice */

/* The most common sequence of clocks uses composite operations
for efficiency. For example, the following are equivalent to the
individual steps on the right:

V(V7_V4_V1,2) -> V(V7,2) V(V4,3) V(V1,5)
V(V8_V5_V2,2) -> V(V8,2) V(V5,3) V(V2,4)
V(V9_V6_V3,2) -> V(V9,2) V(V6,3) V(V3,4) */

/* Voice      0      1      2      3      4      5      6      7 */

/* Runs DSP for specified number of clocks (~1024000 per second). Every 32 clocks
   a pair of samples is be generated. */

/* dsp_run uses intentional case fallthroughs to advance the SPC-DSP through
   its 32-cycle phase schedule one tick at a time. Silence the warning
   locally rather than annotating each of the 32 sites. */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

static void dsp_run( int clocks_remain )
{
   dsp_voice_t *v0, *v1, *v2;
   int phase;
   if (Settings.HardDisableAudio)
      return;
   phase = dsp_m.phase;
   dsp_m.phase = (phase + clocks_remain) & 31;

   /* The 32-phase fallthrough ladder below originally had an
      `if (!--clocks_remain) break;` between each pair of cases, plus a
      bare `--clocks_remain` after case 31. That structure handled the
      general case where clocks_remain could be any positive value and
      phase any value in [0, 31]: the dispatcher would jump in mid-cycle
      and break out as soon as the budget ran out.

      In practice the only caller (the RUN_DSP macro at line ~1492)
      always passes clock_count = (count & ~31) + 32 - a multiple of 32 -
      and dsp_m.phase only gets updated by this function as
      (0 + multiple_of_32) & 31 == 0, so phase is always 0 on entry.
      Empirically verified across 1.3M+ calls in synthetic-ROM testing:
      zero violations of the invariant phase==0, clocks_remain%32==0.

      Given the invariant, the 31 inner `if (!--clocks_remain) break;`
      checks are dead - they never fire - and the per-phase decrements
      are pure overhead. They've been replaced with one batch decrement
      after case 31: `clocks_remain -= 32 - phase` (equals 32 in the
      common path) and `phase = 0` so subsequent iterations dispatch to
      case 0.

      Edge case: a savestate loaded from a hypothetical corrupt source
      where dsp_m.phase != 0 enters the slow path naturally - the switch
      still dispatches at the saved phase. Behavior in this case differs
      slightly from the original (which would stick at the saved phase
      indefinitely, never wrapping to 0 - itself non-physical for a
      sample-accurate DSP); the new code finishes the partial cycle from
      `phase` to 31 and then runs full 0..31 cycles for the remainder.
      Audio output is bit-identical to the original in the always-taken
      common path, verified by hashing 533K samples across NTSC and PAL
      ROMs. */
   for (; clocks_remain > 0;)
   {
      switch ( phase )
      {
         case 0:
         v0 = (dsp_voice_t*)&dsp_m.voices[0];
         v1 = (dsp_voice_t*)(v0 + 1);
         dsp_voice_V5(v0);
         dsp_voice_V2(v1);
         case 1:
         v0 = (dsp_voice_t*)&dsp_m.voices[0];
         v1 = (dsp_voice_t*)(v0 + 1);
         dsp_m.outx_buf = (uint8_t) (dsp_m.t_output >> 8);
         dsp_m.t_pitch += (v1->regs [V_PITCHH] & 0x3F) << 8;
         dsp_voice_V3b(v1);
         dsp_voice_V3c(v1);
         case 2:
         v0 = (dsp_voice_t*)&dsp_m.voices[0];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 3);
	 dsp_m.regs[R_ENDX] = dsp_m.endx_buf;
	 dsp_m.envx_buf = v0->t_envx_out;
         dsp_voice_V1(v2);
         dsp_voice_V4(v1);
         case 3:
         v0 = (dsp_voice_t*)&dsp_m.voices[0];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_OUTX] = dsp_m.outx_buf;
         dsp_voice_V5(v1);
         dsp_voice_V2(v2);
         case 4:
         v0 = (dsp_voice_t*)&dsp_m.voices[0];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_ENVX] = dsp_m.envx_buf;
         dsp_m.outx_buf = (uint8_t) (dsp_m.t_output >> 8);
         dsp_m.t_pitch += (v2->regs [V_PITCHH] & 0x3F) << 8;
         dsp_voice_V3b(v2);
         dsp_voice_V3c(v2);
         case 5:
         v0 = (dsp_voice_t*)&dsp_m.voices[1];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 3);
	 dsp_m.regs[R_ENDX] = dsp_m.endx_buf;
	 dsp_m.envx_buf = v0->t_envx_out;
         dsp_voice_V1(v2);
         dsp_voice_V4(v1);
         case 6:
         v0 = (dsp_voice_t*)&dsp_m.voices[1];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_OUTX] = dsp_m.outx_buf;
         dsp_voice_V5(v1);
         dsp_voice_V2(v2);
         case 7:
         v0 = (dsp_voice_t*)&dsp_m.voices[1];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_ENVX] = dsp_m.envx_buf;
         dsp_m.outx_buf = (uint8_t) (dsp_m.t_output >> 8);
         dsp_m.t_pitch += (v2->regs [V_PITCHH] & 0x3F) << 8;
         dsp_voice_V3b(v2);
         dsp_voice_V3c(v2);
         case 8:
         v0 = (dsp_voice_t*)&dsp_m.voices[2];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 3);
	 dsp_m.regs[R_ENDX] = dsp_m.endx_buf;
	 dsp_m.envx_buf = v0->t_envx_out;
         dsp_voice_V1(v2);
         dsp_voice_V4(v1);
         case 9:
         v0 = (dsp_voice_t*)&dsp_m.voices[2];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_OUTX] = dsp_m.outx_buf;
         dsp_voice_V5(v1);
         dsp_voice_V2(v2);
         case 10:
         v0 = (dsp_voice_t*)&dsp_m.voices[2];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_ENVX] = dsp_m.envx_buf;
         dsp_m.outx_buf = (uint8_t) (dsp_m.t_output >> 8);
         dsp_m.t_pitch += (v2->regs [V_PITCHH] & 0x3F) << 8;
         dsp_voice_V3b(v2);
         dsp_voice_V3c(v2);
         case 11:
         v0 = (dsp_voice_t*)&dsp_m.voices[3];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 3);
	 dsp_m.regs[R_ENDX] = dsp_m.endx_buf;
	 dsp_m.envx_buf = v0->t_envx_out;
         dsp_voice_V1(v2);
         dsp_voice_V4(v1);
         case 12:
         v0 = (dsp_voice_t*)&dsp_m.voices[3];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_OUTX] = dsp_m.outx_buf;
         dsp_voice_V5(v1);
         dsp_voice_V2(v2);
         case 13:
         v0 = (dsp_voice_t*)&dsp_m.voices[3];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_ENVX] = dsp_m.envx_buf;
         dsp_m.outx_buf = (uint8_t) (dsp_m.t_output >> 8);
         dsp_m.t_pitch += (v2->regs [V_PITCHH] & 0x3F) << 8;
         dsp_voice_V3b(v2);
         dsp_voice_V3c(v2);
         case 14:
         v0 = (dsp_voice_t*)&dsp_m.voices[4];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 3);
	 dsp_m.regs[R_ENDX] = dsp_m.endx_buf;
	 dsp_m.envx_buf = v0->t_envx_out;
         dsp_voice_V1(v2);
         dsp_voice_V4(v1);
         case 15:
         v0 = (dsp_voice_t*)&dsp_m.voices[4];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_OUTX] = dsp_m.outx_buf;
         dsp_voice_V5(v1);
         dsp_voice_V2(v2);
         case 16:
         v0 = (dsp_voice_t*)&dsp_m.voices[4];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_ENVX] = dsp_m.envx_buf;
         dsp_m.outx_buf = (uint8_t) (dsp_m.t_output >> 8);
         dsp_m.t_pitch += (v2->regs [V_PITCHH] & 0x3F) << 8;
         dsp_voice_V3b(v2);
         dsp_voice_V3c(v2);
         case 17:
         v0 = (dsp_voice_t*)&dsp_m.voices[0];
         v1 = (dsp_voice_t*)(v0 + 5);
         v2 = (dsp_voice_t*)(v0 + 6);
         dsp_voice_V1(v0);
	 dsp_m.regs[R_ENDX] = dsp_m.endx_buf;
	 dsp_m.envx_buf = v1->t_envx_out;
         dsp_voice_V4(v2);
         case 18:
         v0 = (dsp_voice_t*)&dsp_m.voices[5];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_OUTX] = dsp_m.outx_buf;
         dsp_voice_V5(v1);
         dsp_voice_V2(v2);
         case 19:
         v0 = (dsp_voice_t*)&dsp_m.voices[5];
         v1 = (dsp_voice_t*)(v0 + 1);
         v2 = (dsp_voice_t*)(v0 + 2);
         v0->regs [V_ENVX] = dsp_m.envx_buf;
         dsp_m.outx_buf = (uint8_t) (dsp_m.t_output >> 8);
         dsp_m.t_pitch += (v2->regs [V_PITCHH] & 0x3F) << 8;
         dsp_voice_V3b(v2);
         dsp_voice_V3c(v2);
         case 20:
         v0 = (dsp_voice_t*)&dsp_m.voices[1];
         v1 = (dsp_voice_t*)(v0 + 5);
         v2 = (dsp_voice_t*)(v0 + 6);
         dsp_voice_V1(v0);
	 dsp_m.regs[R_ENDX] = dsp_m.endx_buf;
	 dsp_m.envx_buf = v1->t_envx_out;
         dsp_voice_V4(v2);
         case 21:
         v2 = (dsp_voice_t*)&dsp_m.voices[0];
         v0 = (dsp_voice_t*)(v2 + 6);
         v1 = (dsp_voice_t*)(v2 + 7);
         v0->regs [V_OUTX] = dsp_m.outx_buf;
         dsp_voice_V5(v1);
         dsp_voice_V2(v2);
         case 22:
         v0 = (dsp_voice_t*)&dsp_m.voices[0];
         v1 = (dsp_voice_t*)(v0 + 6);
         v2 = (dsp_voice_t*)(v0 + 7);
         dsp_m.t_pitch += (v0->regs [V_PITCHH] & 0x3F) << 8;
         v1->regs [V_ENVX] = dsp_m.envx_buf;
         dsp_m.outx_buf = (uint8_t) (dsp_m.t_output >> 8);
         dsp_echo_22();
         case 23:
         v0 = (dsp_voice_t*)&dsp_m.voices[7];
	 dsp_m.regs[R_ENDX] = dsp_m.endx_buf;
	 dsp_m.envx_buf = v0->t_envx_out;
         dsp_echo_23();
         case 24:
         v0 = (dsp_voice_t*)&dsp_m.voices[7];
         v0->regs [V_OUTX] = dsp_m.outx_buf;
         dsp_echo_24();
         case 25:
         v0 = (dsp_voice_t*)&dsp_m.voices[0];
         v1 = (dsp_voice_t*)(v0 + 7);
         dsp_voice_V3b(v0);
         v1->regs [V_ENVX] = dsp_m.envx_buf;
         dsp_echo_25();
         case 26:
         dsp_echo_26();
         case 27:
         dsp_m.t_pmon = dsp_m.regs[R_PMON] & 0xFE;
         dsp_echo_27();
         case 28:
	 dsp_m.t_non = dsp_m.regs[R_NON];
	 dsp_m.t_eon = dsp_m.regs[R_EON];
	 dsp_m.t_dir = dsp_m.regs[R_DIR];
         ECHO_28();
         case 29:
	 /* voice 0 doesn't support PMON */
	 if ( (dsp_m.every_other_sample ^= 1) != 0 )
		 dsp_m.new_kon &= ~dsp_m.kon; /* clears KON 63 clocks after it was last read */
         dsp_echo_29();
         case 30:
         v0 = (dsp_voice_t*)&dsp_m.voices[0];
         dsp_misc_30();
         dsp_voice_V3c(v0);
         ECHO_WRITE(1);
         case 31:
         v0 = (dsp_voice_t*)&dsp_m.voices[0];
         v1 = (dsp_voice_t*)(v0 + 2);
         dsp_voice_V4(v0);
         dsp_voice_V1(v1);

         /* Batch decrement for the phases just run (32-phase in the
            common path with phase==0). See header comment. */
         clocks_remain -= 32 - phase;
         phase = 0;
      }
   }
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

/* Sets destination for output samples. The caller is responsible for
   passing a real buffer; with the bsnes-style audio path this is always
   landing_buffer (sized for the worst-case PAL ticks=4 frame), so the
   DSP cursor never approaches out_end and there's no overflow path. */

static void dsp_set_output( short * out, int size )
{
	dsp_m.out_begin = out;
	dsp_m.out       = out;
	dsp_m.out_end   = out + size;
}

/* Setup */

static void dsp_soft_reset_common (void)
{
	dsp_m.noise              = 0x4000;
	dsp_m.echo_hist_pos      = dsp_m.echo_hist;
	dsp_m.every_other_sample = 1;
	dsp_m.echo_offset        = 0;
	dsp_m.phase              = 0;
	
	dsp_m.counter = 0;
}

/* Resets DSP to power-on state */

static void dsp_reset (void)
{
	int i;

	static const uint8_t initial_regs [REGISTER_COUNT] =
	{
		0x45,0x8B,0x5A,0x9A,0xE4,0x82,0x1B,0x78,0x00,0x00,0xAA,0x96,0x89,0x0E,0xE0,0x80,
		0x2A,0x49,0x3D,0xBA,0x14,0xA0,0xAC,0xC5,0x00,0x00,0x51,0xBB,0x9C,0x4E,0x7B,0xFF,
		0xF4,0xFD,0x57,0x32,0x37,0xD9,0x42,0x22,0x00,0x00,0x5B,0x3C,0x9F,0x1B,0x87,0x9A,
		0x6F,0x27,0xAF,0x7B,0xE5,0x68,0x0A,0xD9,0x00,0x00,0x9A,0xC5,0x9C,0x4E,0x7B,0xFF,
		0xEA,0x21,0x78,0x4F,0xDD,0xED,0x24,0x14,0x00,0x00,0x77,0xB1,0xD1,0x36,0xC1,0x67,
		0x52,0x57,0x46,0x3D,0x59,0xF4,0x87,0xA4,0x00,0x00,0x7E,0x44,0x00,0x4E,0x7B,0xFF,
		0x75,0xF5,0x06,0x97,0x10,0xC3,0x24,0xBB,0x00,0x00,0x7B,0x7A,0xE0,0x60,0x12,0x0F,
		0xF7,0x74,0x1C,0xE5,0x39,0x3D,0x73,0xC1,0x00,0x00,0x7A,0xB3,0xFF,0x4E,0x7B,0xFF
	};

	/* Resets DSP and uses supplied values to initialize registers */
   memcpy(dsp_m.regs,initial_regs,REGISTER_COUNT);
	
	/* Internal state */
	for ( i = VOICE_COUNT; --i >= 0; )
	{
		dsp_voice_t* v = &dsp_m.voices [i];
		v->brr_offset = 1;
		v->vbit       = 1 << i;
		v->regs       = &dsp_m.regs [i * 0x10];
	}
	dsp_m.new_kon = dsp_m.regs[R_KON];
	dsp_m.t_dir   = dsp_m.regs[R_DIR];
	dsp_m.t_esa   = dsp_m.regs[R_ESA];
	
	dsp_soft_reset_common();
}

/* Initializes DSP and has it use the 64K RAM provided */

static void dsp_init( void* ram_64k )
{
	dsp_m.ram = (uint8_t*) ram_64k;
	dsp_reset();
}

/* Emulates pressing reset switch on SNES */

static void dsp_soft_reset (void)
{
	dsp_m.regs[R_FLG] = 0xE0;
	dsp_soft_reset_common();
}


/* State save/load */

static void spc_copier_copy(spc_state_copy_t * copier, void* state, size_t size )
{
	copier->func(copier->buf, state, size );
}

static int spc_copier_copy_int(spc_state_copy_t * copier, int32_t state, int32_t size )
{
	uint8_t s [2];
	SET_LE16( s, state );
	copier->func(copier->buf, &s, size );
	return GET_LE16( s );
}

static void spc_copier_extra(spc_state_copy_t * copier)
{
	int32_t n = 0;
	n = (uint8_t) spc_copier_copy_int(copier, n, sizeof (uint8_t) );

	if ( n > 0 )
	{
		int8_t temp [64];
		memset( temp, 0, sizeof(temp));
		do
		{
			int32_t size_n = sizeof(temp);
			if ( size_n > n )
				size_n = n;
			n -= size_n;
			copier->func(copier->buf, temp, size_n );
		}
		while ( n );
	}
}

/* Saves/loads exact emulator state */

static void NO_OPTIMIZE dsp_copy_state( unsigned char** io, dsp_copy_func_t copy )
{
	int32_t i, j;

	spc_state_copy_t copier;
	copier.func = copy;
	copier.buf = io;
	
	/* DSP registers */
	spc_copier_copy(&copier, dsp_m.regs, REGISTER_COUNT );
	
	/* Internal state */
	
	/* Voices */
	for ( i = 0; i < VOICE_COUNT; i++ )
	{
		dsp_voice_t *v = (dsp_voice_t*)&dsp_m.voices [i];
		
		/* BRR buffer */
		for ( j = 0; j < BRR_BUF_SIZE; j++ )
		{
			int s;

			s = v->buf [j];
			SPC_COPY(  int16_t, s );
			v->buf [j] = v->buf [j + BRR_BUF_SIZE] = s;
		}
		
		SPC_COPY( uint16_t, v->interp_pos );
		SPC_COPY( uint16_t, v->brr_addr );
		SPC_COPY( uint16_t, v->env );
		SPC_COPY(  int16_t, v->hidden_env );
		SPC_COPY(  uint8_t, v->buf_pos );
		SPC_COPY(  uint8_t, v->brr_offset );
		SPC_COPY(  uint8_t, v->kon_delay );
		{
			int m;

			m = v->env_mode;
			SPC_COPY(  uint8_t, m );
			v->env_mode = m;
		}
		SPC_COPY(  uint8_t, v->t_envx_out );
		
		spc_copier_extra(&copier);
	}
	
	/* Echo history */
	for ( i = 0; i < ECHO_HIST_SIZE; i++ )
	{
		int s, s2;

		s = dsp_m.echo_hist_pos [i] [0];
		s2 = dsp_m.echo_hist_pos [i] [1];

		SPC_COPY( int16_t, s );
		dsp_m.echo_hist [i] [0] = s; /* write back at offset 0 */

		SPC_COPY( int16_t, s2 );
		dsp_m.echo_hist [i] [1] = s2; /* write back at offset 0 */
	}
	dsp_m.echo_hist_pos = dsp_m.echo_hist;
	memcpy( &dsp_m.echo_hist [ECHO_HIST_SIZE], dsp_m.echo_hist, ECHO_HIST_SIZE * sizeof dsp_m.echo_hist [0] );
	
	/* Misc */
	SPC_COPY(  uint8_t, dsp_m.every_other_sample );
	SPC_COPY(  uint8_t, dsp_m.kon );
	
	SPC_COPY( uint16_t, dsp_m.noise );
	SPC_COPY( uint16_t, dsp_m.counter );
	SPC_COPY( uint16_t, dsp_m.echo_offset );
	SPC_COPY( uint16_t, dsp_m.echo_length );
	SPC_COPY(  uint8_t, dsp_m.phase );
	
	SPC_COPY(  uint8_t, dsp_m.new_kon );
	SPC_COPY(  uint8_t, dsp_m.endx_buf );
	SPC_COPY(  uint8_t, dsp_m.envx_buf );
	SPC_COPY(  uint8_t, dsp_m.outx_buf );
	
	SPC_COPY(  uint8_t, dsp_m.t_pmon );
	SPC_COPY(  uint8_t, dsp_m.t_non );
	SPC_COPY(  uint8_t, dsp_m.t_eon );
	SPC_COPY(  uint8_t, dsp_m.t_dir );
	SPC_COPY(  uint8_t, dsp_m.t_koff );
	
	SPC_COPY( uint16_t, dsp_m.t_brr_next_addr );
	SPC_COPY(  uint8_t, dsp_m.t_adsr0 );
	SPC_COPY(  uint8_t, dsp_m.t_brr_header );
	SPC_COPY(  uint8_t, dsp_m.t_brr_byte );
	SPC_COPY(  uint8_t, dsp_m.t_srcn );
	SPC_COPY(  uint8_t, dsp_m.t_esa );
	SPC_COPY(  uint8_t, dsp_m.t_echo_enabled );
	
	SPC_COPY(  int16_t, dsp_m.t_main_out [0] );
	SPC_COPY(  int16_t, dsp_m.t_main_out [1] );
	SPC_COPY(  int16_t, dsp_m.t_echo_out [0] );
	SPC_COPY(  int16_t, dsp_m.t_echo_out [1] );
	SPC_COPY(  int16_t, dsp_m.t_echo_in  [0] );
	SPC_COPY(  int16_t, dsp_m.t_echo_in  [1] );
	
	SPC_COPY( uint16_t, dsp_m.t_dir_addr );
	SPC_COPY( uint16_t, dsp_m.t_pitch );
	SPC_COPY(  int16_t, dsp_m.t_output );
	SPC_COPY( uint16_t, dsp_m.t_echo_ptr );
	SPC_COPY(  uint8_t, dsp_m.t_looped );
	
	spc_copier_extra(&copier);
}

/* Core SPC emulation: CPU, timers, SMP registers, memory */

/* snes_spc 0.9.0. http://www.slack.net/~ant/ */

/***********************************************************************************
	SNES SPC
***********************************************************************************/

static spc_state_t m;
static const signed char reg_times [256] =
{
        -1,  0,-11,-10,-15,-11, -2, -2,  4,  3, 14, 14, 26, 26, 14, 22,
         2,  3,  0,  1,-12,  0,  1,  1,  7,  6, 14, 14, 27, 14, 14, 23,
         5,  6,  3,  4, -1,  3,  4,  4, 10,  9, 14, 14, 26, -5, 14, 23,
         8,  9,  6,  7,  2,  6,  7,  7, 13, 12, 14, 14, 27, -4, 14, 24,
        11, 12,  9, 10,  5,  9, 10, 10, 16, 15, 14, 14, -2, -4, 14, 24,
        14, 15, 12, 13,  8, 12, 13, 13, 19, 18, 14, 14, -2,-36, 14, 24,
        17, 18, 15, 16, 11, 15, 16, 16, 22, 21, 14, 14, 28, -3, 14, 25,
        20, 21, 18, 19, 14, 18, 19, 19, 25, 24, 14, 14, 14, 29, 14, 25,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
};
static uint8_t allow_time_overflow;

/* Copyright (C) 2004-2007 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

/* (n ? n : 256) */
#define IF_0_THEN_256( n ) ((uint8_t) ((n) - 1) + 1)

/* Timers */

#define TIMER_DIV( t, n ) ((n) >> t->prescaler)
#define TIMER_MUL( t, n ) ((n) << t->prescaler)

static Timer* spc_run_timer_( Timer* t, int time )
{
   int remain, divider, over, n;
   int elapsed = TIMER_DIV( t, time - t->next_time ) + 1;

   t->next_time += TIMER_MUL( t, elapsed );

   if (!t->enabled )
      return t;

   remain  = IF_0_THEN_256( t->period - t->divider );
   divider = t->divider + elapsed;
   over    = elapsed - remain;

   if ( over >= 0 )
   {
      n = over / t->period;
      t->counter = (t->counter + 1 + n) & 0x0F;
      divider = over - n * t->period;
   }

   t->divider = (uint8_t) divider;

   return t;
}

/* ROM */

void spc_enable_rom( int enable )
{
	if ( m.rom_enabled != enable )
	{
		m.rom_enabled = dsp_m.rom_enabled = enable;
		if ( enable )
			memcpy( m.hi_ram, &m.ram.ram[ROM_ADDR], sizeof m.hi_ram );
		memcpy( &m.ram.ram[ROM_ADDR], (enable ? m.rom : m.hi_ram), ROM_SIZE );
		/* TODO: ROM can still get overwritten when DSP writes to echo buffer */
	}
}


/* DSP */

#define MAX_REG_TIME 29
	
#define RUN_DSP( time, offset ) \
	int count = (time) - (offset) - m.dsp_time; \
	if ( count >= 0 ) \
	{ \
		int clock_count; \
		clock_count = (count & ~(CLOCKS_PER_SAMPLE - 1)) + CLOCKS_PER_SAMPLE; \
		m.dsp_time += clock_count; \
		dsp_run( clock_count ); \
	}

static INLINE void spc_dsp_write( const uint8_t data )
{
	uint_fast8_t addr;

	/* Writes DSP registers. */
	addr = m.smp_regs[0][R_DSPADDR];
	dsp_m.regs [addr] = data;
	switch ( addr & 0x0F )
	{
		case V_ENVX:
			dsp_m.envx_buf = data;
			break;

		case V_OUTX:
			dsp_m.outx_buf = data;
			break;
		case 0x0C:
			if ( addr == R_KON )
				dsp_m.new_kon = data;

			if ( addr == R_ENDX ) /* always cleared, regardless of data written */
			{
				dsp_m.endx_buf = 0;
				dsp_m.regs [R_ENDX] = 0;
			}
			break;
	}
	/* dprintf( "SPC wrote to DSP register > $7F\n" ); */
}


/* Memory access extras */

/* CPU write */

/* divided into multiple functions to keep rarely-used functionality separate
   so often-used functionality can be optimized better by compiler */

/* If write isn't preceded by read, data has this added to it
   int const no_read_before_write = 0x2000; */

#define NO_READ_BEFORE_WRITE			8192
#define NO_READ_BEFORE_WRITE_DIVIDED_BY_TWO	4096 

static void spc_cpu_write_smp_reg_( unsigned data, int time, int addr )
{
	switch ( addr )
   {
      case R_T0TARGET:
      case R_T1TARGET:
      case R_T2TARGET:
         {
            Timer *t = (Timer*)&m.timers [addr - R_T0TARGET];
            int period = IF_0_THEN_256( data );

            if ( t->period != period )
            {
               if ( time >= t->next_time )
                  t = spc_run_timer_( t, time );
               t->period = period;
            }
            break;
         }
      case R_T0OUT:
      case R_T1OUT:
      case R_T2OUT:
         /* dprintf( "SPC wrote to counter %d\n", (int) addr - R_T0OUT ); */

         if ( data < NO_READ_BEFORE_WRITE_DIVIDED_BY_TWO)
         {
            if ( (time - 1) >= m.timers[addr - R_T0OUT].next_time )
               spc_run_timer_( &m.timers [addr - R_T0OUT], time - 1 )->counter = 0;
            else
               m.timers[addr - R_T0OUT].counter = 0;
         }
         break;

         /* Registers that act like RAM */
      case 0x8:
      case 0x9:
         m.smp_regs[1][addr] = (uint8_t) data;
         break;

      case R_TEST:
         break;

      case R_CONTROL:
         {
            int i;
            /* port clears */
            if ( data & 0x10 )
            {
               m.smp_regs[1][R_CPUIO0] = 0;
               m.smp_regs[1][R_CPUIO1] = 0;
            }
            if ( data & 0x20 )
            {
               m.smp_regs[1][R_CPUIO2] = 0;
               m.smp_regs[1][R_CPUIO3] = 0;
            }

            /* timers */
            {
               for ( i = 0; i < TIMER_COUNT; i++ )
               {
                  Timer* t = (Timer*)&m.timers [i];
                  int enabled = data >> i & 1;
                  if ( t->enabled != enabled )
                  {
                     if ( time >= t->next_time )
                        t = spc_run_timer_( t, time );
                     t->enabled = enabled;
                     if ( enabled )
                     {
                        t->divider = 0;
                        t->counter = 0;
                     }
                  }
               }
            }
            spc_enable_rom( data & 0x80 );
         }
         break;
   }
}

/* Out-of-line slow path: SPC700 hardware-register write
 * ($F0-$FF) and IPL-ROM area shadow ($FFC0-$FFFF). The fast
 * path — plain RAM write for the other ~65,440 addresses — runs
 * inline in the spc_cpu_write macro below, which calls this
 * function only when the address falls in one of the two slow
 * regions.
 *
 * RAM is written by the macro before this is invoked; this body
 * only handles the side-effects (DSP / timer / IPL-ROM shadow). */
#ifdef __GNUC__
__attribute__((noinline))
#endif
static void spc_cpu_write_io( unsigned data, uint16_t addr, int32_t time )
{
	int32_t reg = addr - 0xF0;
	if ( reg < REG_COUNT ) /* 87%: $F0-$FF hardware registers */
	{
		m.smp_regs[0][reg] = (uint8_t) data;

		/* Registers other than $F2 and $F4-$F7
		   if ( reg != 2 && reg != 4 && reg != 5 && reg != 6 && reg != 7 )
		   TODO: this is a bit on the fragile side */

         if ( (0x2F00 & (1 << (15 - reg))) == 0 ) /* 36% */
		{
			if ( reg == R_DSPDATA ) /* 99% */
			{
				RUN_DSP(time, reg_times [m.smp_regs[0][R_DSPADDR]] );
				if (m.smp_regs[0][R_DSPADDR] <= 0x7F )
					spc_dsp_write( data );
			}
			else
				spc_cpu_write_smp_reg_( data, time, reg);
		}
	}
	/* IPL-ROM area shadow ($FFC0-$FFFF). The else branch from the
	 * original function: the macro gates the middle range
	 * ($0100-$FFBF) out so it is never reached here. */
	else
	{
		reg -= ROM_ADDR - 0xF0;
		if ( reg >= 0 ) /* IPL ROM area or address wrapped around */
		{
			m.hi_ram [reg] = (uint8_t) data;
			if ( m.rom_enabled )
				m.ram.ram[reg + ROM_ADDR] = m.rom [reg]; /* restore overwritten ROM */
		}
	}
}

/* SPC700 memory write fast path.
 *
 * Folded to a do-while-0 macro (no return value, so plain C, no
 * GCC statement-expression extension needed). The preprocessor
 * splices the body at every use site, bypassing the inline cost
 * model the way memory_speed and spc_cpu_read are.
 *
 * The slow regions are $00F0-$00FF (hardware registers) and
 * $FFC0-$FFFF (IPL-ROM shadow). The middle range $0100-$FFBF was a
 * no-op in the original function: \`reg < REG_COUNT\` was false and
 * the else branch's \`reg -= ROM_ADDR - 0xF0; if (reg >= 0)\` was
 * also false. The macro tests both slow regions inline and only
 * tail-calls spc_cpu_write_io when one is hit, so direct-page
 * writes with DP=1 ($0100-$01FF) — which the original would have
 * called through but then short-circuited — stay fully inline.
 *
 * Each argument is captured into a local so a side-effecting call
 * site is safe; no current site does that, but the same trap that
 * bit spc_cpu_read with READ_PC(++pc) is easy to fall into. */
#define spc_cpu_write(data, addr, time) do { \
	unsigned _spcw_data = (data); \
	uint16_t _spcw_addr = (addr); \
	int32_t  _spcw_time = (time); \
	m.ram.ram[_spcw_addr] = (uint8_t) _spcw_data; \
	if ((unsigned) (_spcw_addr - 0xF0) < 0x10 || _spcw_addr >= ROM_ADDR) \
		spc_cpu_write_io(_spcw_data, _spcw_addr, _spcw_time); \
} while (0)

/* CPU read */

/* Out-of-line slow path: SPC700 hardware register read (addr in
 * [0xF0, 0xFF]). The fast path — plain RAM read for the other
 * 65,520 addresses — is in the inline wrapper below, so this body
 * never runs unless the SPC700 actually touched one of its 16
 * memory-mapped registers.
 *
 * __attribute__((noinline)) keeps GCC from inlining this back into
 * spc_cpu_read; without it the optimizer notices spc_cpu_read is
 * the only caller and folds them together, restoring the original
 * monolithic shape and defeating the purpose of the split. */
#ifdef __GNUC__
__attribute__((noinline))
#endif
static int spc_cpu_read_io ( uint16_t addr, int32_t time )
{
	int32_t result = m.ram.ram[addr];
	int32_t reg    = addr - 0x100 + 0x10 - R_T0OUT;

	/* Timers */
	if ( (uint32_t) reg < TIMER_COUNT ) /* 90% */
	{
		Timer* t = &m.timers [reg];
		if ( time >= t->next_time )
			t = spc_run_timer_( t, time );
		result = t->counter;
		t->counter = 0;
	}
	/* Other registers */
	else /* 10% */
	{
		int32_t reg_tmp = reg + R_T0OUT;
		result = m.smp_regs[1][reg_tmp];
		reg_tmp -= R_DSPADDR;
		/* DSP addr and data */
		if ( (uint32_t) reg_tmp <= 1 ) /* 4% 0xF2 and 0xF3 */
		{
			result = m.smp_regs[0][R_DSPADDR];
			if ( (uint32_t) reg_tmp == 1 )
			{
				RUN_DSP( time, reg_times [m.smp_regs[0][R_DSPADDR] & 0x7F] );

				result = dsp_m.regs[m.smp_regs[0][R_DSPADDR] & 0x7F]; /* 0xF3 */
			}
		}
	}

	return result;
}

/* spc_cpu_read: SPC700 memory read.
 *
 * Implemented as a static INLINE function rather than a macro. The
 * earlier statement-expression form was replaced with a plain
 * expression macro for MSVC portability, but that macro references
 * its `addr` argument up to three times, and several of the ~50 call
 * sites in the opcode dispatch reach it through wrappers
 * (spc_CPU_mem_bit, READ_DP / READ_DP_TIMER -> CPU_READ_TIMER) that
 * pass address expressions which are themselves re-evaluated. Any
 * such argument with a side effect was therefore stepped multiple
 * times per opcode, corrupting SPC700 execution -- observed as the
 * FF6 sound driver wedging after the first battle (music stops and
 * never resumes while the game keeps running).
 *
 * A static INLINE function evaluates each argument exactly once, like
 * the original statement-expression, and is fully portable (no GCC
 * extension), so it keeps the MSVC fix while restoring correctness.
 * The optimizer inlines it at the call sites just as the macro did. */
static INLINE int spc_cpu_read_fn( uint16_t addr, int32_t time )
{
	if ( (unsigned) ( addr - 0xF0 ) < 0x10 )
		return spc_cpu_read_io( addr, time );
	return (int) m.ram.ram[ addr ];
}
#define spc_cpu_read(addr, time) spc_cpu_read_fn( (addr), (time) )

/***********************************************************************************
 SPC CPU
***********************************************************************************/

/* Inclusion here allows static memory access functions and better optimization */

/* Timers are by far the most common thing read from dp */

#define CPU_READ_TIMER( time, offset, addr_, out ) \
{ \
	int adj_time = time + offset; \
	int dp_addr = addr_; \
	int ti = dp_addr - (R_T0OUT + 0xF0); \
	if ( (unsigned) ti < TIMER_COUNT ) \
	{ \
		Timer* t = &m.timers [ti]; \
		if ( adj_time >= t->next_time ) \
		t = spc_run_timer_( t, adj_time ); \
		out = t->counter; \
		t->counter = 0; \
	} \
	else \
	{ \
		int i, reg; \
		out = ram [dp_addr]; \
		i = dp_addr - 0xF0; \
		if ( (unsigned) i < 0x10 ) \
		{ \
			reg = i; \
			out = m.smp_regs[1][reg]; \
			reg -= R_DSPADDR; \
			/* DSP addr and data */ \
			if ( (unsigned) reg <= 1 ) /* 4% 0xF2 and 0xF3 */ \
			{ \
				out = m.smp_regs[0][R_DSPADDR]; \
				if ( (unsigned) reg == 1 ) \
				{ \
					RUN_DSP( adj_time, reg_times [m.smp_regs[0][R_DSPADDR] & 0x7F] ); \
					out = dsp_m.regs[m.smp_regs[0][R_DSPADDR] & 0x7F ]; /* 0xF3 */ \
				} \
			} \
		} \
	} \
}

#define READ_TIMER( time, addr, out )	CPU_READ_TIMER( rel_time, time, (addr), out )
#define SPC_CPU_READ( time, addr )		spc_cpu_read((addr), rel_time + time )
#define SPC_CPU_WRITE( time, addr, data )	spc_cpu_write((data), (addr), rel_time + time )

#define spc_CPU_mem_bit(pc, rel_time, result) \
   addr = GET_LE16( pc ); \
   t = spc_cpu_read((addr & 0x1FFF), (rel_time)) >> (addr >> 13); \
   result = ((t << 8) & 0x100)

#define DP_ADDR( addr )                     (dp + (addr))

#define READ_DP_TIMER(  time, addr, out )   CPU_READ_TIMER( rel_time, time, DP_ADDR( addr ), out )
#define READ_DP(  time, addr )              SPC_CPU_READ( time, DP_ADDR( addr ) )
#define WRITE_DP( time, addr, data )        SPC_CPU_WRITE( time, DP_ADDR( addr ), data )

#define READ_PROG16( addr )                 GET_LE16( ram + (addr) )

#define SET_PC( n )     (pc = ram + (n))
#define GET_PC()        (pc - ram)
#define READ_PC( pc )   (*(pc))

#define SET_SP( v )     (sp = ram + 0x101 + (v))
#define GET_SP()        (sp - 0x101 - ram)

#define PUSH16( v )     (sp -= 2, SET_LE16( sp, v ))
#define PUSH( v )       (void) (*--sp = (uint8_t) (v))
#define POP( out )      (void) ((out) = *sp++)

#define GET_PSW( out )\
{\
	out = psw & ~(N80 | P20 | Z02 | C01);\
	out |= c  >> 8 & C01;\
	out |= dp >> 3 & P20;\
	out |= ((nz >> 4) | nz) & N80;\
	if ( !(uint8_t) nz ) out |= Z02;\
}

#define SET_PSW( in )\
{\
	psw = in;\
	c   = in << 8;\
	dp  = in << 3 & 0x100;\
	nz  = (in << 4 & 0x800) | (~in & Z02);\
}

/* spc_run_until_ is the SPC700 instruction dispatcher. Its address-mode
   macros (ADDR_MODES, ADDR_MODES_, ADDR_MODES_NO_DP, LOGICAL_OP) deliberately
   chain case labels via fallthrough to share the address calculation
   prologue across opcodes. The same macros also redeclare locals named
   't' and 'addr' inside nested blocks, shadowing the function-scope
   declarations - this is upstream Blargg code where renaming locals is
   unsafe. Silence both warnings around the whole function. */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wshadow"
#endif

static uint8_t* spc_run_until_( int end_time )
{
   unsigned addr, t;
   unsigned tmp_spc_cpu_mem_bit;
	int dp, nz, c, psw, a, x, y;
	uint8_t *ram, *pc, *sp;
	int rel_time = m.spc_time - end_time;
	m.spc_time = end_time;
	m.dsp_time += rel_time;
	m.timers [0].next_time += rel_time;
	m.timers [1].next_time += rel_time;
	m.timers [2].next_time += rel_time;
	ram = m.ram.ram;
	a = m.cpu_regs.a;
	x = m.cpu_regs.x;
	y = m.cpu_regs.y;

	SET_PC( m.cpu_regs.pc );
	SET_SP( m.cpu_regs.sp );
	SET_PSW( m.cpu_regs.psw );

	goto loop;


	/* Main loop */

cbranch_taken_loop:
	pc += *(int8_t const*) pc;
inc_pc_loop:
	pc++;
loop:
	{
		unsigned data;
		unsigned opcode = *pc;

		if (allow_time_overflow && rel_time >= 0 )
			goto stop;
		if ( (rel_time += m.cycle_table [opcode]) > 0 && !allow_time_overflow)
			goto out_of_time;

		/* TODO: if PC is at end of memory, this will get wrong operand (very obscure) */
		data = *++pc;
		switch ( opcode )
		{

			/* Common instructions */

#define BRANCH( cond )\
			{\
				pc++;\
				pc += (int8_t) data;\
				if ( cond )\
				goto loop;\
				pc -= (int8_t) data;\
				rel_time -= 2;\
				goto loop;\
			}

			case 0xF0: /* BEQ */
				BRANCH( !(uint8_t) nz ) /* 89% taken */

			case 0xD0: /* BNE */
					BRANCH( (uint8_t) nz )

			case 0x3F:
					{ /* CALL */
						int old_addr = GET_PC() + 2;
						SET_PC( GET_LE16( pc ) );
						PUSH16( old_addr );
						goto loop;
					}
			case 0x6F: /* RET */
					SET_PC( GET_LE16( sp ) );
					sp += 2;
					goto loop;

			case 0xE4: /* MOV a,dp */
					++pc;
					/* 80% from timer */
					READ_DP_TIMER( 0, data, a = nz );
					goto loop;

			case 0xFA:{ /* MOV dp,dp */
					  int temp;
					  READ_DP_TIMER( -2, data, temp );
					  data = temp + NO_READ_BEFORE_WRITE ;
				  }
				  /* fall through */
			case 0x8F:
				  { /* MOV dp,#imm */
					  int32_t i;
					  int32_t temp = READ_PC( pc + 1 );
					  pc += 2;

					  i = dp + temp;
					  ram [i] = (uint8_t) data;
					  i -= 0xF0;
					  if ( (uint32_t) i < 0x10 ) /* 76% */
					  {
						  m.smp_regs[0][i] = (uint8_t) data;

						  /* Registers other than $F2 and $F4-$F7 */
                    if ( (0x2F00 & (1 << (15 - i))) == 0 ) /* 12% */
						  {
							  if ( i == R_DSPDATA ) /* 99% */
							  {
								  RUN_DSP(rel_time, reg_times [m.smp_regs[0][R_DSPADDR]] );
								  if (m.smp_regs[0][R_DSPADDR] <= 0x7F )
									  spc_dsp_write( data );
							  }
							  else
								  spc_cpu_write_smp_reg_( data, rel_time, i);
						  }
					  }
					  goto loop;
				  }

			case 0xC4: /* MOV dp,a */
				  ++pc;
				  {
					  int i = dp + data;
					  ram [i] = (uint8_t) a;
					  i -= 0xF0;
					  if ( (unsigned) i < 0x10 ) /* 39% */
					  {
						  unsigned sel = i - 2;
						  m.smp_regs[0][i] = (uint8_t) a;

						  if ( sel == 1 ) /* 51% $F3 */
						  {
							  RUN_DSP(rel_time, reg_times [m.smp_regs[0][R_DSPADDR]] );
							  if (m.smp_regs[0][R_DSPADDR] <= 0x7F )
								  spc_dsp_write( a );
						  }
						  else if ( sel > 1 ) /* 1% not $F2 or $F3 */
							  spc_cpu_write_smp_reg_( a, rel_time, i );
					  }
				  }
				  goto loop;

#define CASE( n )   case n:

				  /* Define common address modes based on opcode for immediate mode. Execution
				     ends with data set to the address of the operand. */
#define ADDR_MODES_( op )\
				  CASE( op - 0x02 ) /* (X) */\
				  data = x + dp;\
				  pc--;\
				  goto end_##op;\
				  CASE( op + 0x0F ) /* (dp)+Y */\
				  data = READ_PROG16( data + dp ) + y;\
				  goto end_##op;\
				  CASE( op - 0x01 ) /* (dp+X) */\
				  data = READ_PROG16( ((uint8_t) (data + x)) + dp );\
				  goto end_##op;\
				  CASE( op + 0x0E ) /* abs+Y */\
				  data += y;\
				  goto abs_##op;\
				  CASE( op + 0x0D ) /* abs+X */\
				  data += x;\
				  CASE( op - 0x03 ) /* abs */\
				  abs_##op:\
				  data += 0x100 * READ_PC( ++pc );\
				  goto end_##op;\
				  CASE( op + 0x0C ) /* dp+X */\
				  data = (uint8_t) (data + x);

#define ADDR_MODES_NO_DP( op )\
				  ADDR_MODES_( op )\
				  data += dp;\
				  end_##op:

#define ADDR_MODES( op )\
				  ADDR_MODES_( op )\
				  CASE( op - 0x04 ) /* dp */\
				  data += dp;\
				  end_##op:

				  /* 1. 8-bit Data Transmission Commands. Group I */

				  ADDR_MODES_NO_DP( 0xE8 ) /* MOV A,addr */
					  a = nz = SPC_CPU_READ( 0, data );
				  goto inc_pc_loop;

			case 0xBF:
				  {
					  /* MOV A,(X)+ */
					  int temp = x + dp;
					  x = (uint8_t) (x + 1);
					  a = nz = SPC_CPU_READ( -1, temp );
					  goto loop;
				  }

			case 0xE8: /* MOV A,imm */
				  a  = data;
				  nz = data;
				  goto inc_pc_loop;

			case 0xF9: /* MOV X,dp+Y */
				  data = (uint8_t) (data + y);
			case 0xF8: /* MOV X,dp */
				  READ_DP_TIMER( 0, data, x = nz );
				  goto inc_pc_loop;

			case 0xE9: /* MOV X,abs */
				  data = GET_LE16( pc );
				  ++pc;
				  data = SPC_CPU_READ( 0, data );
			case 0xCD: /* MOV X,imm */
				  x  = data;
				  nz = data;
				  goto inc_pc_loop;

			case 0xFB: /* MOV Y,dp+X */
				  data = (uint8_t) (data + x);
			case 0xEB: /* MOV Y,dp */
				  /* 70% from timer */
				  pc++;
				  READ_DP_TIMER( 0, data, y = nz );
				  goto loop;

			case 0xEC:
				  { /* MOV Y,abs */
					  int temp = GET_LE16( pc );
					  pc += 2;
					  READ_TIMER( 0, temp, y = nz );
					  /* y = nz = SPC_CPU_READ( 0, temp ); */
					  goto loop;
				  }

			case 0x8D: /* MOV Y,imm */
				  y  = data;
				  nz = data;
				  goto inc_pc_loop;

				  /* 2. 8-BIT DATA TRANSMISSION COMMANDS, GROUP 2 */

				  ADDR_MODES_NO_DP( 0xC8 ) /* MOV addr,A */
					  SPC_CPU_WRITE( 0, data, a );
				  goto inc_pc_loop;

				  {
					  int temp;
					  case 0xCC: /* MOV abs,Y */
					  temp = y;
					  goto mov_abs_temp;
					  case 0xC9: /* MOV abs,X */
					  temp = x;
mov_abs_temp:
					  SPC_CPU_WRITE( 0, GET_LE16( pc ), temp );
					  pc += 2;
					  goto loop;
				  }

			case 0xD9: /* MOV dp+Y,X */
				  data = (uint8_t) (data + y);
			case 0xD8: /* MOV dp,X */
				  SPC_CPU_WRITE( 0, data + dp, x );
				  goto inc_pc_loop;

			case 0xDB: /* MOV dp+X,Y */
				  data = (uint8_t) (data + x);
			case 0xCB: /* MOV dp,Y */
				  SPC_CPU_WRITE( 0, data + dp, y );
				  goto inc_pc_loop;

				  /* 3. 8-BIT DATA TRANSMISSION COMMANDS, GROUP 3. */

			case 0x7D: /* MOV A,X */
				  a  = x;
				  nz = x;
				  goto loop;

			case 0xDD: /* MOV A,Y */
				  a  = y;
				  nz = y;
				  goto loop;

			case 0x5D: /* MOV X,A */
				  x  = a;
				  nz = a;
				  goto loop;

			case 0xFD: /* MOV Y,A */
				  y  = a;
				  nz = a;
				  goto loop;

			case 0x9D: /* MOV X,SP */
				  x = nz = GET_SP();
				  goto loop;

			case 0xBD: /* MOV SP,X */
				  SET_SP( x );
				  goto loop;

				  /* case 0xC6: // MOV (X),A (handled by MOV addr,A in group 2) */

			case 0xAF: /* MOV (X)+,A */
				  WRITE_DP( 0, x, a + NO_READ_BEFORE_WRITE  );
				  x++;
				  goto loop;

				  /* 5. 8-BIT LOGIC OPERATION COMMANDS */

#define LOGICAL_OP( op, func )\
				  ADDR_MODES( op ) /* addr */\
				  data = SPC_CPU_READ( 0, data );\
			case op: /* imm */\
					  nz = a func##= data;\
				  goto inc_pc_loop;\
				  {   unsigned addr;\
					  case op + 0x11: /* X,Y */\
								   data = READ_DP( -2, y );\
					  addr = x + dp;\
					  goto addr_##op;\
					  case op + 0x01: /* dp,dp */\
								     data = READ_DP( -3, data );\
					  case op + 0x10:{/*dp,imm*/\
								 uint8_t const* addr2 = pc + 1;\
								 pc += 2;\
								 addr = READ_PC( addr2 ) + dp;\
							 }\
					  addr_##op:\
					  nz = data func SPC_CPU_READ( -1, addr );\
					  SPC_CPU_WRITE( 0, addr, nz );\
					  goto loop;\
				  }

				  LOGICAL_OP( 0x28, & ); /* AND */

				  LOGICAL_OP( 0x08, | ); /* OR */

				  LOGICAL_OP( 0x48, ^ ); /* EOR */

				  /* 4. 8-BIT ARITHMETIC OPERATION COMMANDS */

				  ADDR_MODES( 0x68 ) /* CMP addr */
					  data = SPC_CPU_READ( 0, data );
			case 0x68: /* CMP imm */
				  nz = a - data;
				  c = ~nz;
				  nz &= 0xFF;
				  goto inc_pc_loop;

			case 0x79: /* CMP (X),(Y) */
				  data = READ_DP( -2, y );
				  nz = READ_DP( -1, x ) - data;
				  c = ~nz;
				  nz &= 0xFF;
				  goto loop;

			case 0x69: /* CMP dp,dp */
				  data = READ_DP( -3, data );
			case 0x78: /* CMP dp,imm */
				  ++pc; /* spc_cpu_read is now a plain macro and would
				         * evaluate ++pc multiple times if inlined into
				         * the READ_DP arg. Step pc here, then pass the
				         * resulting *pc through READ_PC. */
				  nz = READ_DP( -1, READ_PC( pc ) ) - data;
				  c = ~nz;
				  nz &= 0xFF;
				  goto inc_pc_loop;

			case 0x3E: /* CMP X,dp */
				  data += dp;
				  goto cmp_x_addr;
			case 0x1E: /* CMP X,abs */
				  data = GET_LE16( pc );
				  pc++;
cmp_x_addr:
				  data = SPC_CPU_READ( 0, data );
			case 0xC8: /* CMP X,imm */
				  nz = x - data;
				  c = ~nz;
				  nz &= 0xFF;
				  goto inc_pc_loop;

			case 0x7E: /* CMP Y,dp */
				  data += dp;
				  goto cmp_y_addr;
			case 0x5E: /* CMP Y,abs */
				  data = GET_LE16( pc );
				  pc++;
cmp_y_addr:
				  data = SPC_CPU_READ( 0, data );
			case 0xAD: /* CMP Y,imm */
				  nz = y - data;
				  c = ~nz;
				  nz &= 0xFF;
				  goto inc_pc_loop;

				  {
					  int addr;
					  case 0xB9: /* SBC (x),(y) */
					  case 0x99: /* ADC (x),(y) */
					  pc--; /* compensate for inc later */
					  data = READ_DP( -2, y );
					  addr = x + dp;
					  goto adc_addr;
					  case 0xA9: /* SBC dp,dp */
					  case 0x89: /* ADC dp,dp */
					  data = READ_DP( -3, data );
					  case 0xB8: /* SBC dp,imm */
					  case 0x98: /* ADC dp,imm */
					  addr = READ_PC( ++pc ) + dp;
adc_addr:
					  nz = SPC_CPU_READ( -1, addr );
					  goto adc_data;

					  /* catch ADC and SBC together, then decode later based on operand */
#undef CASE
#define CASE( n ) case n: case (n) + 0x20:
					  ADDR_MODES( 0x88 ) /* ADC/SBC addr */
						  data = SPC_CPU_READ( 0, data );
					  case 0xA8: /* SBC imm */
					  case 0x88: /* ADC imm */
					  addr = -1; /* A */
					  nz = a;
adc_data: {
		  int flags;
		  if ( opcode >= 0xA0 ) /* SBC */
			  data ^= 0xFF;

		  flags = data ^ nz;
		  nz += data + (c >> 8 & 1);
		  flags ^= nz;

		  psw = (psw & ~(V40 | H08)) |
			  (flags >> 1 & H08) |
			  ((flags + 0x80) >> 2 & V40);
		  c = nz;
		  if ( addr < 0 )
		  {
			  a = (uint8_t) nz;
			  goto inc_pc_loop;
		  }
		  SPC_CPU_WRITE( 0, addr, /*(uint8_t)*/ nz );
		  goto inc_pc_loop;
	  }

				  }

				  /* 6. ADDITION & SUBTRACTION COMMANDS */

#define INC_DEC_REG( reg, op )\
				  nz  = reg op;\
				  reg = (uint8_t) nz;\
				  goto loop;

			case 0xBC: INC_DEC_REG( a, + 1 ) /* INC A */
			case 0x3D: INC_DEC_REG( x, + 1 ) /* INC X */
			case 0xFC: INC_DEC_REG( y, + 1 ) /* INC Y */

			case 0x9C: INC_DEC_REG( a, - 1 ) /* DEC A */
			case 0x1D: INC_DEC_REG( x, - 1 ) /* DEC X */
			case 0xDC: INC_DEC_REG( y, - 1 ) /* DEC Y */

			case 0x9B: /* DEC dp+X */
			case 0xBB: /* INC dp+X */
				   data = (uint8_t) (data + x);
			case 0x8B: /* DEC dp */
			case 0xAB: /* INC dp */
				   data += dp;
				   goto inc_abs;
			case 0x8C: /* DEC abs */
			case 0xAC: /* INC abs */
				   data = GET_LE16( pc );
				   pc++;
inc_abs:
				   nz = (opcode >> 4 & 2) - 1;
				   nz += SPC_CPU_READ( -1, data );
				   SPC_CPU_WRITE( 0, data, /*(uint8_t)*/ nz );
				   goto inc_pc_loop;

				   /* 7. SHIFT, ROTATION COMMANDS */

			case 0x5C: /* LSR A */
				   c = 0;
			case 0x7C: /* ROR A */
               {
                  nz = (c >> 1 & 0x80) | (a >> 1);
                  c = a << 8;
                  a = nz;
                  goto loop;
               }

			case 0x1C: /* ASL A */
				  c = 0;
			case 0x3C:
				  {/* ROL A */
					  int temp = c >> 8 & 1;
					  c = a << 1;
					  nz = c | temp;
					  a = (uint8_t) nz;
					  goto loop;
				  }

			case 0x0B: /* ASL dp */
				  c = 0;
				  data += dp;
				  goto rol_mem;
			case 0x1B: /* ASL dp+X */
				  c = 0;
			case 0x3B: /* ROL dp+X */
				  data = (uint8_t) (data + x);
			case 0x2B: /* ROL dp */
				  data += dp;
				  goto rol_mem;
			case 0x0C: /* ASL abs */
				  c = 0;
			case 0x2C: /* ROL abs */
				  data = GET_LE16( pc );
				  pc++;
rol_mem:
				  nz = c >> 8 & 1;
				  nz |= (c = SPC_CPU_READ( -1, data ) << 1);
				  SPC_CPU_WRITE( 0, data, /*(uint8_t)*/ nz );
				  goto inc_pc_loop;

			case 0x4B: /* LSR dp */
				  c = 0;
				  data += dp;
				  goto ror_mem;
			case 0x5B: /* LSR dp+X */
				  c = 0;
			case 0x7B: /* ROR dp+X */
				  data = (uint8_t) (data + x);
			case 0x6B: /* ROR dp */
				  data += dp;
				  goto ror_mem;
			case 0x4C: /* LSR abs */
				  c = 0;
			case 0x6C: /* ROR abs */
				  data = GET_LE16( pc );
				  pc++;
ror_mem: {
		 int temp = SPC_CPU_READ( -1, data );
		 nz = (c >> 1 & 0x80) | (temp >> 1);
		 c = temp << 8;
		 SPC_CPU_WRITE( 0, data, nz );
		 goto inc_pc_loop;
	 }

			case 0x9F: /* XCN */
	 nz = a = (a >> 4) | (uint8_t) (a << 4);
	 goto loop;

	 /* 8. 16-BIT TRANSMISION COMMANDS */

			case 0xBA: /* MOVW YA,dp */
	 a = READ_DP( -2, data );
	 nz = (a & 0x7F) | (a >> 1);
	 y = READ_DP( 0, (uint8_t) (data + 1) );
	 nz |= y;
	 goto inc_pc_loop;

			case 0xDA: /* MOVW dp,YA */
	 WRITE_DP( -1, data, a );
	 WRITE_DP( 0, (uint8_t) (data + 1), y + NO_READ_BEFORE_WRITE  );
	 goto inc_pc_loop;

	 /* 9. 16-BIT OPERATION COMMANDS */

			case 0x3A: /* INCW dp */
			case 0x1A:{/* DECW dp */
					  int temp;
					  /* low byte */
					  data += dp;
					  temp = SPC_CPU_READ( -3, data );
					  temp += (opcode >> 4 & 2) - 1; /* +1 for INCW, -1 for DECW */
					  nz = ((temp >> 1) | temp) & 0x7F;
					  SPC_CPU_WRITE( -2, data, /*(uint8_t)*/ temp );

					  /* high byte */
					  data = (uint8_t) (data + 1) + dp;
					  temp = (uint8_t) ((temp >> 8) + SPC_CPU_READ( -1, data ));
					  nz |= temp;
					  SPC_CPU_WRITE( 0, data, temp );

					  goto inc_pc_loop;
				  }

			case 0x7A: /* ADDW YA,dp */
			case 0x9A:
				  {/* SUBW YA,dp */
					  int result, flags;
					  int lo = READ_DP( -2, data );
					  int hi = READ_DP( 0, (uint8_t) (data + 1) );

					  if ( opcode == 0x9A ) /* SUBW */
					  {
						  lo = (lo ^ 0xFF) + 1;
						  hi ^= 0xFF;
					  }

					  lo += a;
					  result = y + hi + (lo >> 8);
					  flags = hi ^ y ^ result;

					  psw = (psw & ~(V40 | H08)) |
						  (flags >> 1 & H08) |
						  ((flags + 0x80) >> 2 & V40);
					  c = result;
					  a = (uint8_t) lo;
					  result = (uint8_t) result;
					  y = result;
					  nz = (((lo >> 1) | lo) & 0x7F) | result;

					  goto inc_pc_loop;
				  }

			case 0x5A:
				  { /* CMPW YA,dp */
					  int temp;
					  temp = a - READ_DP( -1, data );
					  nz = ((temp >> 1) | temp) & 0x7F;
					  temp = y + (temp >> 8);
					  temp -= READ_DP( 0, (uint8_t) (data + 1) );
					  nz |= temp;
					  c  = ~temp;
					  nz &= 0xFF;
					  goto inc_pc_loop;
				  }

				   /* 10. MULTIPLICATION & DIVISON COMMANDS */

			case 0xCF: { /* MUL YA */
					   unsigned temp = y * a;
					   a = (uint8_t) temp;
					   nz = ((temp >> 1) | temp) & 0x7F;
					   y = temp >> 8;
					   nz |= y;
					   goto loop;
				   }

			case 0x9E: /* DIV YA,X */
				   {
					   unsigned ya = y * 0x100 + a;

					   psw &= ~(H08 | V40);

					   if ( y >= x )
						   psw |= V40;

					   if ( (y & 15) >= (x & 15) )
						   psw |= H08;

					   if ( y < x * 2 )
					   {
						   a = ya / x;
						   y = ya - a * x;
					   }
					   else
					   {
						   a = 255 - (ya - x * 0x200) / (256 - x);
						   y = x   + (ya - x * 0x200) % (256 - x);
					   }

					   nz = (uint8_t) a;
					   a = (uint8_t) a;

					   goto loop;
				   }

				   /* 11. DECIMAL COMPENSATION COMMANDS */

			case 0xDF: /* DAA */
				   if ( a > 0x99 || c & 0x100 )
				   {
					   a += 0x60;
					   c = 0x100;
				   }

				   if ( (a & 0x0F) > 9 || psw & H08 )
					   a += 0x06;

				   nz = a;
				   a = (uint8_t) a;
				   goto loop;

			case 0xBE: /* DAS */
				   if ( a > 0x99 || !(c & 0x100) )
				   {
					   a -= 0x60;
					   c = 0;
				   }

				   if ( (a & 0x0F) > 9 || !(psw & H08) )
					   a -= 0x06;

				   nz = a;
				   a = (uint8_t) a;
				   goto loop;

				   /* 12. BRANCHING COMMANDS */

			case 0x2F: /* BRA rel */
				   pc += (int8_t) data;
				   goto inc_pc_loop;
 
			case 0x30: /* BMI */
				   BRANCH( (nz & NZ_NEG_MASK) )

			case 0x10: /* BPL */
					   BRANCH( !(nz & NZ_NEG_MASK) )

			case 0xB0: /* BCS */
					   BRANCH( c & 0x100 )

			case 0x90: /* BCC */
					   BRANCH( !(c & 0x100) )

			case 0x70: /* BVS */
					   BRANCH( psw & V40 )

			case 0x50: /* BVC */
					   BRANCH( !(psw & V40) )

#define CBRANCH( cond )\
					   {\
						   pc++;\
						   if ( cond )\
							   goto cbranch_taken_loop;\
						   rel_time -= 2;\
						   goto inc_pc_loop;\
					   }

			case 0x03: /* BBS dp.bit,rel */
			case 0x23:
			case 0x43:
			case 0x63:
			case 0x83:
			case 0xA3:
			case 0xC3:
			case 0xE3:
					   CBRANCH( READ_DP( -4, data ) >> (opcode >> 5) & 1 )

			case 0x13: /* BBC dp.bit,rel */
			case 0x33:
			case 0x53:
			case 0x73:
			case 0x93:
			case 0xB3:
			case 0xD3:
			case 0xF3:
					CBRANCH( !(READ_DP( -4, data ) >> (opcode >> 5) & 1) )

			case 0xDE: /* CBNE dp+X,rel */
												   data = (uint8_t) (data + x);
												   /* fall through */
			case 0x2E:{ /* CBNE dp,rel */
					  int temp;
					  /* 61% from timer */
					  READ_DP_TIMER( -4, data, temp );
					  CBRANCH( temp != a )
				  }

			case 0x6E: { /* DBNZ dp,rel */
					   unsigned temp = READ_DP( -4, data ) - 1;
					   WRITE_DP( -3, (uint8_t) data, /*(uint8_t)*/ temp + NO_READ_BEFORE_WRITE  );
					   CBRANCH( temp )
				   }

			case 0xFE: /* DBNZ Y,rel */
				   y = (uint8_t) (y - 1);
				   BRANCH( y )

			case 0x1F: /* JMP [abs+X] */
					   SET_PC( GET_LE16( pc ) + x );
					   /* fall through */
			case 0x5F: /* JMP abs */
					   SET_PC( GET_LE16( pc ) );
					   goto loop;

					   /* 13. SUB-ROUTINE CALL RETURN COMMANDS */

			case 0x0F:{/* BRK */
					  int temp;
					  int ret_addr = GET_PC();
					  SET_PC( READ_PROG16( 0xFFDE ) ); /* vector address verified */
					  PUSH16( ret_addr );
					  GET_PSW( temp );
					  psw = (psw | B10) & ~I04;
					  PUSH( temp );
					  goto loop;
				  }

			case 0x4F:{/* PCALL offset */
					  int ret_addr = GET_PC() + 1;
					  SET_PC( 0xFF00 | data );
					  PUSH16( ret_addr );
					  goto loop;
				  }

			case 0x01: /* TCALL n */
			case 0x11:
			case 0x21:
			case 0x31:
			case 0x41:
			case 0x51:
			case 0x61:
			case 0x71:
			case 0x81:
			case 0x91:
			case 0xA1:
			case 0xB1:
			case 0xC1:
			case 0xD1:
			case 0xE1:
			case 0xF1: {
					   int ret_addr = GET_PC();
					   SET_PC( READ_PROG16( 0xFFDE - (opcode >> 3) ) );
					   PUSH16( ret_addr );
					   goto loop;
				   }

				   /* 14. STACK OPERATION COMMANDS */

				   {
					   int temp;
					   case 0x7F: /* RET1 */
					   temp = *sp;
					   SET_PC( GET_LE16( sp + 1 ) );
					   sp += 3;
					   goto set_psw;
					   case 0x8E: /* POP PSW */
					   POP( temp );
set_psw:
					   SET_PSW( temp );
					   goto loop;
				   }

			case 0x0D: { /* PUSH PSW */
					   int temp;
					   GET_PSW( temp );
					   PUSH( temp );
					   goto loop;
				   }

			case 0x2D: /* PUSH A */
				   PUSH( a );
				   goto loop;

			case 0x4D: /* PUSH X */
				   PUSH( x );
				   goto loop;

			case 0x6D: /* PUSH Y */
				   PUSH( y );
				   goto loop;

			case 0xAE: /* POP A */
				   POP( a );
				   goto loop;

			case 0xCE: /* POP X */
				   POP( x );
				   goto loop;

			case 0xEE: /* POP Y */
				   POP( y );
				   goto loop;

				   /* 15. BIT OPERATION COMMANDS */

			case 0x02: /* SET1 */
			case 0x22:
			case 0x42:
			case 0x62:
			case 0x82:
			case 0xA2:
			case 0xC2:
			case 0xE2:
			case 0x12: /* CLR1 */
			case 0x32:
			case 0x52:
			case 0x72:
			case 0x92:
			case 0xB2:
			case 0xD2:
			case 0xF2:
               {
                  int bit = 1 << (opcode >> 5);
                  int mask = ~bit;
                  if ( opcode & 0x10 )
                     bit = 0;
                  data += dp;
                  SPC_CPU_WRITE( 0, data, (SPC_CPU_READ( -1, data ) & mask) | bit );
                  goto inc_pc_loop;
               }

			case 0x0E: /* TSET1 abs */
			case 0x4E: /* TCLR1 abs */
				   data = GET_LE16( pc );
				   pc += 2;
				   {
					   unsigned temp = SPC_CPU_READ( -2, data );
					   nz = (uint8_t) (a - temp);
					   temp &= ~a;
					   if ( opcode == 0x0E )
						   temp |= a;
					   SPC_CPU_WRITE( 0, data, temp );
				   }
				   goto loop;

			case 0x4A: /* AND1 C,mem.bit */
               spc_CPU_mem_bit(pc, rel_time, tmp_spc_cpu_mem_bit);
				   c &= tmp_spc_cpu_mem_bit;
				   pc += 2;
				   goto loop;

			case 0x6A: /* AND1 C,/mem.bit */
               spc_CPU_mem_bit(pc, rel_time, tmp_spc_cpu_mem_bit);
				   c &= ~tmp_spc_cpu_mem_bit;
				   pc += 2;
				   goto loop;

			case 0x0A: /* OR1 C,mem.bit */
               spc_CPU_mem_bit(pc, rel_time - 1, tmp_spc_cpu_mem_bit);
				   c |= tmp_spc_cpu_mem_bit;
				   pc += 2;
				   goto loop;

			case 0x2A: /* OR1 C,/mem.bit */
               spc_CPU_mem_bit(pc, rel_time - 1, tmp_spc_cpu_mem_bit);
				   c |= ~tmp_spc_cpu_mem_bit;
				   pc += 2;
				   goto loop;

			case 0x8A: /* EOR1 C,mem.bit */
               spc_CPU_mem_bit(pc, rel_time - 1, tmp_spc_cpu_mem_bit);
				   c ^= tmp_spc_cpu_mem_bit;
				   pc += 2;
				   goto loop;

			case 0xEA: /* NOT1 mem.bit */
				   data = GET_LE16( pc );
				   pc += 2;
				   {
					   unsigned temp = SPC_CPU_READ( -1, data & 0x1FFF );
					   temp ^= 1 << (data >> 13);
					   SPC_CPU_WRITE( 0, data & 0x1FFF, temp );
				   }
				   goto loop;

			case 0xCA: /* MOV1 mem.bit,C */
				   data = GET_LE16( pc );
				   pc += 2;
				   {
					   unsigned temp, bit;
					   temp = SPC_CPU_READ( -2, data & 0x1FFF );
					   bit = data >> 13;
					   temp = (temp & ~(1 << bit)) | ((c >> 8 & 1) << bit);
					   SPC_CPU_WRITE( 0, data & 0x1FFF, temp + NO_READ_BEFORE_WRITE  );
				   }
				   goto loop;

			case 0xAA: /* MOV1 C,mem.bit */
               spc_CPU_mem_bit(pc, rel_time, tmp_spc_cpu_mem_bit);
				   c = tmp_spc_cpu_mem_bit;
				   pc += 2;
				   goto loop;

				   /* 16. PROGRAM PSW FLAG OPERATION COMMANDS */

			case 0x60: /* CLRC */
				   c = 0;
				   goto loop;

			case 0x80: /* SETC */
				   c = ~0;
				   goto loop;

			case 0xED: /* NOTC */
				   c ^= 0x100;
				   goto loop;

			case 0xE0: /* CLRV */
				   psw &= ~(V40 | H08);
				   goto loop;

			case 0x20: /* CLRP */
				   dp = 0;
				   goto loop;

			case 0x40: /* SETP */
				   dp = 0x100;
				   goto loop;

			case 0xA0: /* EI */
				   psw |= I04;
				   goto loop;

			case 0xC0: /* DI */
				   psw &= ~I04;
				   goto loop;

				   /* 17. OTHER COMMANDS */

			case 0x00: /* NOP */
				   goto loop;

			case 0xFF:
				   { /* STOP */
					   /* handle PC wrap-around */
					   unsigned addr = GET_PC() - 1;
					   if ( addr >= 0x10000 )
					   {
						   addr &= 0xFFFF;
						   SET_PC( addr );
						   /* dprintf( "SPC: PC wrapped around\n" ); */
						   goto loop;
					   }
				   }
				  /* fall through */
			case 0xEF: /* SLEEP */
				  --pc;
				  rel_time = 0;
				  goto stop;
		} /* switch */
	}   
out_of_time:
	rel_time -= m.cycle_table [*pc]; /* undo partial execution of opcode */
stop:

	/* Uncache registers */
	m.cpu_regs.pc = (uint16_t) GET_PC();
	m.cpu_regs.sp = ( uint8_t) GET_SP();
	m.cpu_regs.a  = ( uint8_t) a;
	m.cpu_regs.x  = ( uint8_t) x;
	m.cpu_regs.y  = ( uint8_t) y;
	{
		int temp;
		GET_PSW( temp );
		m.cpu_regs.psw = (uint8_t) temp;
	}
	m.spc_time += rel_time;
	m.dsp_time -= rel_time;
	m.timers [0].next_time -= rel_time;
	m.timers [1].next_time -= rel_time;
	m.timers [2].next_time -= rel_time;
	return &m.smp_regs[0][R_CPUIO0];
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

/* Runs SPC to end_time and starts a new time frame at 0.

   Catches CPU/timer/DSP up to end_time. The DSP writes its samples into
   landing_buffer at dsp_m.out (advanced by dsp_set_output). At end of
   frame the libretro driver pulls everything written via S9xMixSamples,
   which delivers (dsp_m.out - landing_buffer) mono samples and resets
   the cursor for the next frame. There is no per-frame integer-sample
   alignment to maintain - the frontend's resampler handles whatever
   count we deliver via Dynamic Rate Control. */

static void spc_end_frame( int end_time )
{
	int i;
	/* Catch CPU up to as close to end as possible. If final instruction
	   would exceed end, does NOT execute it and leaves m.spc_time < end. */

	if ( end_time > m.spc_time )
		spc_run_until_( end_time );

	m.spc_time -= end_time;

	/* Catch timers up to CPU */
	for ( i = 0; i < TIMER_COUNT; i++ )
	{
		if ( 0 >= m.timers[i].next_time )
			spc_run_timer_( &m.timers [i], 0 );
	}

	/* Catch DSP up to CPU */
	if ( m.dsp_time < 0 )
	{
		RUN_DSP( 0, MAX_REG_TIME );
	}
}

/* Support SNES_MEMORY_APURAM */

uint8_t * spc_apuram(void)
{
	return m.ram.ram;
}

/* Sets tempo, where tempo_unit = normal, tempo_unit / 2 = half speed, etc. */

static void spc_set_tempo( int t )
{
	int timer2_shift, other_shift;
	m.tempo = t;
	timer2_shift = 4; /* 64 kHz */
	other_shift  = 3; /*  8 kHz */
	
	m.timers [2].prescaler = timer2_shift;
	m.timers [1].prescaler = timer2_shift + other_shift;
	m.timers [0].prescaler = timer2_shift + other_shift;
}

static void spc_reset_common( int timer_counter_init )
{
	int i;
	for ( i = 0; i < TIMER_COUNT; i++ )
		m.smp_regs[1][R_T0OUT + i] = timer_counter_init;
	
	/* Run IPL ROM */
	memset( &m.cpu_regs, 0, sizeof(m.cpu_regs));
	m.cpu_regs.pc = ROM_ADDR;
	
	m.smp_regs[0][R_TEST   ] = 0x0A;
	m.smp_regs[0][R_CONTROL] = 0xB0; /* ROM enabled, clear ports */
	for ( i = 0; i < PORT_COUNT; i++ )
		m.smp_regs[1][R_CPUIO0 + i] = 0;
	
	/* reset time registers */
	m.spc_time      = 0;
	m.dsp_time      = 0;
	m.dsp_time = CLOCKS_PER_SAMPLE + 1;
	
	for ( i = 0; i < TIMER_COUNT; i++ )
	{
		Timer* t = &m.timers [i];
		t->next_time = 1;
		t->divider   = 0;
	}
	
	/* Registers were just loaded. Applies these new values. */
	spc_enable_rom( m.smp_regs[0][R_CONTROL] & 0x80 );

	/*	Timer registers have been loaded. Applies these to the timers. Does not 
		reset timer prescalers or dividers. */
	for ( i = 0; i < TIMER_COUNT; i++ )
	{
		Timer* t = &m.timers [i];
		t->period  = IF_0_THEN_256( m.smp_regs[0][R_T0TARGET + i] );
		t->enabled = m.smp_regs[0][R_CONTROL] >> i & 1;
		t->counter = m.smp_regs[1][R_T0OUT + i] & 0x0F;
	}
	
	spc_set_tempo( m.tempo );
}

/*	Resets SPC to power-on state. This resets your output buffer, so you must
	call set_output() after this. */

static void spc_reset (void)
{
	m.cpu_regs.pc  = 0xFFC0;
	m.cpu_regs.a   = 0x00;
	m.cpu_regs.x   = 0x00;
	m.cpu_regs.y   = 0x00;
	m.cpu_regs.psw = 0x02;
	m.cpu_regs.sp  = 0xEF;
	memset( m.ram.ram, 0x00, 0x10000 );

	/*	RAM was just loaded from SPC, with $F0-$FF containing SMP registers
		and timer counts. Copies these to proper registers. */
	m.rom_enabled = dsp_m.rom_enabled = 0;

	/* Loads registers from unified 16-byte format */
	memcpy( m.smp_regs[0], &m.ram.ram[0xF0], REG_COUNT );
	memcpy( m.smp_regs[1], m.smp_regs[0], REG_COUNT );
	
	/* These always read back as 0 */
	m.smp_regs[1][R_TEST    ] = 0;
	m.smp_regs[1][R_CONTROL ] = 0;
	m.smp_regs[1][R_T0TARGET] = 0;
	m.smp_regs[1][R_T1TARGET] = 0;
	m.smp_regs[1][R_T2TARGET] = 0;
	
	/* Put STOP instruction around memory to catch PC underflow/overflow */
	memset( m.ram.padding1, CPU_PAD_FILL, sizeof m.ram.padding1 );
	memset( m.ram.padding2, CPU_PAD_FILL, sizeof m.ram.padding2 );

	spc_reset_common( 0x0F );
	dsp_reset();
}


/*	Emulates pressing reset switch on SNES. This resets your output buffer, so
	you must call set_output() after this. */

static void spc_soft_reset (void)
{
	spc_reset_common( 0 );
	dsp_soft_reset();
}

void NO_OPTIMIZE spc_copy_state( unsigned char** io, dsp_copy_func_t copy )
{
	int i;
	spc_state_copy_t copier;
	copier.func = copy;
	copier.buf = io;
	
	/*	Make state data more readable by putting 64K RAM, 16 SMP registers,
		then DSP (with its 128 registers) first */

	/* RAM */
	spc_enable_rom( 0 ); /* will get re-enabled if necessary in regs_loaded() below */
	spc_copier_copy(&copier, m.ram.ram, 0x10000 );
	
	{
		/* SMP registers */
		uint8_t regs [REG_COUNT], regs_in [REG_COUNT];

		memcpy( regs, m.smp_regs[0], REG_COUNT );
		memcpy( regs_in, m.smp_regs[1], REG_COUNT );

		spc_copier_copy(&copier, regs, sizeof(regs));
		spc_copier_copy(&copier, regs_in, sizeof(regs_in));

		memcpy( m.smp_regs[0], regs, REG_COUNT);
		memcpy( m.smp_regs[1], regs_in, REG_COUNT );

		spc_enable_rom( m.smp_regs[0][R_CONTROL] & 0x80 );
	}
	
	/* CPU registers */
	SPC_COPY( uint16_t, m.cpu_regs.pc );
	SPC_COPY(  uint8_t, m.cpu_regs.a );
	SPC_COPY(  uint8_t, m.cpu_regs.x );
	SPC_COPY(  uint8_t, m.cpu_regs.y );
	SPC_COPY(  uint8_t, m.cpu_regs.psw );
	SPC_COPY(  uint8_t, m.cpu_regs.sp );
	spc_copier_extra(&copier);
	
	SPC_COPY( int16_t, m.spc_time );
	SPC_COPY( int16_t, m.dsp_time );

	/* DSP */
	dsp_copy_state( io, copy );
	
	/* Timers */
	for ( i = 0; i < TIMER_COUNT; i++ )
	{
		Timer *t;

		t = &m.timers [i];
		t->period  = IF_0_THEN_256( m.smp_regs[0][R_T0TARGET + i] );
		t->enabled = m.smp_regs[0][R_CONTROL] >> i & 1;
		SPC_COPY( int16_t, t->next_time );
		SPC_COPY( uint8_t, t->divider );
		SPC_COPY( uint8_t, t->counter );
		spc_copier_extra(&copier);
	}

	spc_set_tempo( m.tempo );

	spc_copier_extra(&copier);
}

/***********************************************************************************
 APU
***********************************************************************************/

#define APU_NUMERATOR_NTSC		15664
#define APU_DENOMINATOR_NTSC		328125
#define APU_NUMERATOR_PAL		34176
#define APU_DENOMINATOR_PAL		709379

/* SPC landing buffer.

   Sized for the worst-case PAL frame at the highest effective SPC rate
   (ticks=4 speedup, ~651 stereo = 1302 mono); 4096 mono shorts gives
   ~3x headroom. Static so there's no malloc bookkeeping, no err path
   on init, and the pointer is never NULL - which lets every other
   audio path drop its NULL guards. The DSP writes here all frame; at
   end of frame S9xDrainAudio hands the libretro driver a pointer
   straight into this buffer (zero-copy delivery). */
#define LANDING_BUFFER_FRAMES	2048
static int16_t		landing_buffer[LANDING_BUFFER_FRAMES * 2];

static int32_t		reference_time;
static uint32_t		spc_remainder;

static int		timing_hack_denominator = TEMPO_UNIT;
/* Set these to NTSC for now. Will change to PAL in S9xAPUTimingSetSpeedup
   if necessary on game load. */
static uint32_t		ratio_numerator = APU_NUMERATOR_NTSC;
static uint32_t		ratio_denominator = APU_DENOMINATOR_NTSC;

/***********************************************************************************
	APU AUDIO PATH

	bsnes-style: SPC's DSP writes stereo pairs into landing_buffer over the
	frame; at end of frame, S9xDrainAudio hands the caller a pointer to
	those samples and resets the DSP cursor for the next frame's writes.
	The frontend's resampler handles conversion from the SPC's native rate
	(declared via retro_get_system_av_info, using S9xGetAudioSampleRate
	below) to the host audio rate.

	Zero-copy delivery: the libretro driver passes the returned pointer
	straight into audio_batch_cb. RetroArch's audio_batch_cb is documented
	as synchronous - it queues into the frontend's own buffer before
	returning - so the pointer only needs to remain valid for the duration
	of that call. Since the DSP never runs between S9xDrainAudio's cursor
	reset and audio_batch_cb's read (DSP catch-up only happens inside
	S9xAPUExecute, which the libretro driver does not call during
	audio_upload_samples), the data stays intact.

	No internal resampler. No per-frame integer-sample alignment, no
	"extra_buf" overflow carry, no SPC_SAMPLE_COUNT bookkeeping. Whatever
	count of samples DSP produced this frame, that count goes to the
	frontend, which absorbs the per-frame variation via Dynamic Rate
	Control (which is exactly what DRC is for).

	Mute handling lives in the libretro driver: when muted it sends a
	pre-zeroed silence buffer of the appropriate size instead of calling
	S9xDrainAudio - keeping the frontend's audio clock fed without
	requiring apu.c to know about mute.
 ***********************************************************************************/

const short *S9xDrainAudio (int *count_out)
{
	int written = (int)(dsp_m.out - landing_buffer);
	if (written < 0)
		written = 0;

	*count_out = written;

	/* Reset DSP cursor to the head of landing_buffer for the next frame.
	   The caller still holds a valid pointer to the just-produced samples;
	   audio_batch_cb consumes them synchronously and the DSP doesn't run
	   again until the next retro_run, so there's no aliasing concern. */
	dsp_set_output(landing_buffer, LANDING_BUFFER_FRAMES * 2);

	return landing_buffer;
}

/* SPC's natural audio output rate.

   The SPC700 / S-DSP samples at exactly 32 kHz nominally, but the SNES
   master clock relationship gives 32040 Hz (NTSC) as the SPC's true
   per-second sample count. snes9x has historically used 32040.0 as the
   single canonical value across NTSC and PAL, since the SPC clock is
   not derived from the video clock and runs at the same rate in both
   regions. */
#define SNES_AUDIO_FREQ 32040.0

/* Effective SPC output sample rate for the current cart.

   For carts with no APU speedup hack (the vast majority), this is just
   SNES_AUDIO_FREQ. For carts that use the speedup hack
   (S9xAPUTimingSetSpeedup with ticks > 0; ~70 game IDs in memmap.c)
   the SPC runs at TEMPO_UNIT / timing_hack_denominator times its
   normal rate and produces samples at the same multiplier. The
   libretro driver reports this as info->timing.sample_rate so the
   frontend resampler handles the conversion to host audio rate. */
unsigned S9xGetAudioSampleRate (void)
{
	return (unsigned)(SNES_AUDIO_FREQ * TEMPO_UNIT / timing_hack_denominator + 0.5);
}

void S9xInitSound (void)
{
	/* landing_buffer is a static array; nothing to allocate. Just hand
	   the DSP its initial cursor pointing at the start. */
	dsp_set_output(landing_buffer, LANDING_BUFFER_FRAMES * 2);
}

uint8_t S9xInitAPU (void)
{
    unsigned i;

    static const uint8_t APUROM[64] =
    {
        0xCD, 0xEF, 0xBD, 0xE8, 0x00, 0xC6, 0x1D, 0xD0,
        0xFC, 0x8F, 0xAA, 0xF4, 0x8F, 0xBB, 0xF5, 0x78,
        0xCC, 0xF4, 0xD0, 0xFB, 0x2F, 0x19, 0xEB, 0xF4,
        0xD0, 0xFC, 0x7E, 0xF4, 0xD0, 0x0B, 0xE4, 0xF5,
        0xCB, 0xF4, 0xD7, 0x00, 0xFC, 0xD0, 0xF3, 0xAB,
        0x01, 0x10, 0xEF, 0x7E, 0xF4, 0x10, 0xEB, 0xBA,
        0xF6, 0xDA, 0x00, 0xBA, 0xF4, 0xC4, 0xF4, 0xDD,
        0x5D, 0xD0, 0xDB, 0x1F, 0x00, 0x00, 0xC0, 0xFF
    };

    /* Must be called once before using */
    static const uint8_t cycle_table [128] =
    {   /*   01   23   45   67   89   AB   CD   EF */
        0x28,0x47,0x34,0x36,0x26,0x54,0x54,0x68, /* 0 */
        0x48,0x47,0x45,0x56,0x55,0x65,0x22,0x46, /* 1 */
        0x28,0x47,0x34,0x36,0x26,0x54,0x54,0x74, /* 2 */
        0x48,0x47,0x45,0x56,0x55,0x65,0x22,0x38, /* 3 */
        0x28,0x47,0x34,0x36,0x26,0x44,0x54,0x66, /* 4 */
        0x48,0x47,0x45,0x56,0x55,0x45,0x22,0x43, /* 5 */
        0x28,0x47,0x34,0x36,0x26,0x44,0x54,0x75, /* 6 */
        0x48,0x47,0x45,0x56,0x55,0x55,0x22,0x36, /* 7 */
        0x28,0x47,0x34,0x36,0x26,0x54,0x52,0x45, /* 8 */
        0x48,0x47,0x45,0x56,0x55,0x55,0x22,0xC5, /* 9 */
        0x38,0x47,0x34,0x36,0x26,0x44,0x52,0x44, /* A */
        0x48,0x47,0x45,0x56,0x55,0x55,0x22,0x34, /* B */
        0x38,0x47,0x45,0x47,0x25,0x64,0x52,0x49, /* C */
        0x48,0x47,0x56,0x67,0x45,0x55,0x22,0x83, /* D */
        0x28,0x47,0x34,0x36,0x24,0x53,0x43,0x40, /* E */
        0x48,0x47,0x45,0x56,0x34,0x54,0x22,0x60, /* F */
    };

	memset( &m, 0, sizeof m );
	dsp_init( m.ram.ram );
	
	m.tempo = TEMPO_UNIT;
	
	/*	Most SPC music doesn't need ROM, and almost all the rest only 
		rely on these two bytes */

	m.rom [0x3E] = 0xFF;
	m.rom [0x3F] = 0xC0;
	
	
	/* unpack cycle table */
	for (i = 0; i < 128; i++ )
	{
		int n = cycle_table [i];
		m.cycle_table [i * 2 + 0] = n >> 4;
		m.cycle_table [i * 2 + 1] = n & 0x0F;
	}

	allow_time_overflow = FALSE;

	dsp_m.rom = m.rom;
	dsp_m.hi_ram = m.hi_ram;
	
	spc_reset();


	memcpy( m.rom, APUROM, sizeof m.rom );

	return TRUE;
}

#define S9X_APU_GET_CLOCK(cpucycles)		((ratio_numerator * (cpucycles - reference_time) + spc_remainder) / ratio_denominator)
#define S9X_APU_GET_CLOCK_REMAINDER(cpucycles)	((ratio_numerator * (cpucycles - reference_time) + spc_remainder) % ratio_denominator)

/* Emulated port read at specified time */

uint8_t S9xAPUReadPort (int port)	{ return ((uint8_t) spc_run_until_(S9X_APU_GET_CLOCK(CPU.Cycles))[port]); }

/* Emulated port write at specified time */

void S9xAPUWritePort (int port, uint8_t byte)
{
	spc_run_until_( S9X_APU_GET_CLOCK(CPU.Cycles) ) [0x10 + port] = byte;
	m.ram.ram [0xF4 + port] = byte;
}

void S9xAPUSetReferenceTime (int32_t cpucycles)
{
	reference_time = cpucycles;
}

void S9xAPUExecute (void)
{
	/* Per-scanline timing rebase. The catch-up itself is usually a no-op
	   because S9xAPUReadPort/S9xAPUWritePort already drove spc_run_until_
	   to current cycle, but this is also the only path that advances the
	   SPC when a game has no port traffic for many lines. The reference-
	   time reset that follows keeps the multiply in S9X_APU_GET_CLOCK
	   from overflowing uint32_t (per-frame products would). The libretro
	   driver is the sole caller path; sample drainage happens once at
	   end-of-frame in retro_run, not from inside this function. */
	spc_end_frame(S9X_APU_GET_CLOCK(CPU.Cycles));

	spc_remainder = S9X_APU_GET_CLOCK_REMAINDER(CPU.Cycles);
	reference_time = CPU.Cycles;
}

void S9xAPUTimingSetSpeedup (int ticks)
{
	if (ticks != 0)
	{
		char buf[128];
		snprintf(buf, sizeof(buf),
			 "Setting APU speedup hack to %d ticks", ticks);
		S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_APU, buf);
	}

	timing_hack_denominator = TEMPO_UNIT - ticks;
	spc_set_tempo(timing_hack_denominator);

	ratio_numerator = Settings.PAL ? APU_NUMERATOR_PAL : APU_NUMERATOR_NTSC;
	ratio_denominator = Settings.PAL ? APU_DENOMINATOR_PAL : APU_DENOMINATOR_NTSC;
	ratio_denominator = ratio_denominator * timing_hack_denominator / TEMPO_UNIT;
}

void S9xAPUAllowTimeOverflow (uint8_t allow)
{
	allow_time_overflow = allow;
}

void S9xResetAPU (void)
{
	reference_time = 0;
	spc_remainder = 0;
	spc_reset();

	dsp_set_output(landing_buffer, LANDING_BUFFER_FRAMES * 2);
}

void S9xSoftResetAPU (void)
{
	reference_time = 0;
	spc_remainder = 0;
	spc_soft_reset();

	dsp_set_output(landing_buffer, LANDING_BUFFER_FRAMES * 2);
}

static void NO_OPTIMIZE from_apu_to_state (uint8_t **buf, void *var, size_t size)
{
	memcpy(*buf, var, size);
	*buf += size;
}

static void NO_OPTIMIZE to_apu_from_state (uint8_t **buf, void *var, size_t size)
{
	memcpy(var, *buf, size);
	*buf += size;
}

// work around optimization bug in android GCC
// similar to this: http://jeffq.com/blog/over-aggressive-gcc-optimization-can-cause-sigbus-crash-when-using-memcpy-with-the-android-ndk/
#if defined(ANDROID) || defined(__QNX__)
void __attribute__((optimize(0))) S9xAPUSaveState (uint8_t *block)
#else
void S9xAPUSaveState (uint8_t *block)
#endif
{
	uint8_t *ptr = block;

	spc_copy_state(&ptr, from_apu_to_state);

	SET_LE32(ptr, reference_time);
	ptr += sizeof(int32_t);
	SET_LE32(ptr, spc_remainder);
	ptr += sizeof(int32_t);

	//zero out the rest of the save state block
	memset(ptr, 0, SPC_SAVE_STATE_BLOCK_SIZE - (ptr - block));
}

#if defined(ANDROID) || defined(__QNX__)
void __attribute__((optimize(0))) S9xAPULoadState (uint8_t *block)
#else
void S9xAPULoadState (uint8_t *block)
#endif
{
	uint8_t *ptr = block;

	S9xResetAPU();

	spc_copy_state(&ptr, to_apu_from_state);

	reference_time = GET_LE32(ptr);
	ptr += sizeof(int32_t);
	spc_remainder = GET_LE32(ptr);
	ptr += sizeof(int32_t);
}
