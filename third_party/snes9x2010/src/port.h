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

  Super FX C emulator code
  (c) Copyright 1997 - 1999  Ivar,
                             Gary Henderson,
                             John Weidman

  Sound emulator code used in 1.5-1.51
  (c) Copyright 1998 - 2003  Brad Martin
  (c) Copyright 1998 - 2006  Charles Bilyue'

  Sound emulator code used in 1.52+
  (c) Copyright 2004 - 2007  Shay Green (gblargg@gmail.com)

  NTSC filter
  (c) Copyright 2006 - 2007  Shay Green

  GTK+ GUI code
  (c) Copyright 2004 - 2010  BearOso

  (c) Copyright 2009 - 2010  OV2

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


#ifndef _PORT_H_
#define _PORT_H_

#include <boolean.h>
#include <limits.h>
#include <sys/types.h>

#include <stdint.h>

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#ifndef PATH_MAX
#define PATH_MAX	1024
#endif

#ifndef _MAX_DRIVE
#define _MAX_DRIVE	1
#endif

#ifndef _MAX_PATH
#define _MAX_PATH	PATH_MAX
#endif

#ifdef __DJGPP
#define SLASH_STR	"\\"
#define SLASH_CHAR	'\\'
#else
#define SLASH_STR	"/"
#define SLASH_CHAR	'/'
#endif

/* Endianness is a compile-time property.
 *
 *   MSB_FIRST defined     -> host is big endian
 *   MSB_FIRST not defined -> host is little endian
 *
 * Big-endian targets (PS3, Wii, GameCube, ...) define MSB_FIRST from
 * their build files. As a safety net, also honour the compiler's
 * __BYTE_ORDER__ macro when it is available so a big-endian build can
 * never silently fall through to the little-endian fast paths. */
#if !defined(MSB_FIRST) && defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define MSB_FIRST
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#ifndef MSB_FIRST
/* Little-endian host: SNES memory layout matches the host, so word
 * accesses are plain (possibly unaligned) loads and stores. */
#define READ_WORD(s)		(*(uint16_t *) (s))
#define WRITE_WORD(s, d)	(*(uint16_t *) (s) = (uint16_t) (d))
#define READ_3WORD(s)		(*(uint32_t *) (s) & 0x00ffffff)
#define READ_DWORD(s)		(*(uint32_t *) (s))
#define WRITE_3WORD(s, d)	(*(uint16_t *) (s) = (uint16_t) (d), \
				 *((uint8_t *) (s) + 2) = (uint8_t) ((d) >> 16))
#else
/* Big-endian host: SNES data is little endian, so it must be byte
 * swapped. PowerPC has byte-reversed load/store instructions; use them
 * where available and fall back to manual assembly otherwise. */
#if defined(__SNC__)
#include <ppu_intrinsics.h>
#define READ_WORD(s)		(__builtin_lhbrx(s, 0))
#define WRITE_WORD(s, d)	(__builtin_sthbrx(d, s, 0))
#elif defined(_XBOX360)
#include <PPCIntrinsics.h>
#define READ_WORD(s)		(__loadshortbytereverse(0, s))
#define WRITE_WORD(s, d)	(__storeshortbytereverse(d, 0, s))
#elif defined(GEKKO) || defined(__PPC__) || defined(__powerpc__)
#define READ_WORD(addr)		({unsigned ppc_lhbrx_; asm( "lhbrx %0,0,%1" : "=r" (ppc_lhbrx_) : "r" (addr), "0" (ppc_lhbrx_) ); ppc_lhbrx_;})
#define WRITE_WORD(addr, in)	({asm( "sthbrx %0,0,%1" : : "r" (in), "r" (addr) );})
#else
#define READ_WORD(s)		(*(uint8_t *) (s) | (*((uint8_t *) (s) + 1) << 8))
#define WRITE_WORD(s, d)	(*(uint8_t *) (s) = (uint8_t) (d), \
				 *((uint8_t *) (s) + 1) = (uint8_t) ((d) >> 8))
#endif
#define READ_3WORD(s)		(*(uint8_t *) (s) | (*((uint8_t *) (s) + 1) << 8) | (*((uint8_t *) (s) + 2) << 16))
#define READ_DWORD(s)		(*(uint8_t *) (s) | (*((uint8_t *) (s) + 1) << 8) | (*((uint8_t *) (s) + 2) << 16) | (*((uint8_t *) (s) + 3) << 24))
#define WRITE_3WORD(s, d)	(*(uint8_t *) (s) = (uint8_t) (d), \
				 *((uint8_t *) (s) + 1) = (uint8_t) ((d) >> 8), \
				 *((uint8_t *) (s) + 2) = (uint8_t) ((d) >> 16))
#endif

#include "pixform.h"

#if (S9X_ACCURACY_LEVEL >= 2)

#define addCyclesInMemoryAccess \
	if (!CPU.InDMAorHDMA) \
	{ \
		CPU.Cycles += speed; \
		while (CPU.Cycles >= CPU.NextEvent) \
			S9xDoHEventProcessing(); \
	}

#define addCyclesInMemoryAccess_x2 \
	if (!CPU.InDMAorHDMA) \
	{ \
		CPU.Cycles += speed << 1; \
		while (CPU.Cycles >= CPU.NextEvent) \
			S9xDoHEventProcessing(); \
	}

#else

#define addCyclesInMemoryAccess \
	if (!CPU.InDMAorHDMA) \
		CPU.Cycles += speed;

#define addCyclesInMemoryAccess_x2 \
	if (!CPU.InDMAorHDMA) \
		CPU.Cycles += speed << 1;

#endif

#endif
