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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "snes9x.h"
#include "memmap.h"
#include "getset.h"
#include "apu.h"
#include "fxinst.h"
#include "fxemu.h"
#include "sdd1.h"
#include "srtc.h"
#include "controls.h"
#include "cheats.h"
#include "display.h"
#include "spc7110dec.h"

#include <streams/file_stream.h>
#include <libretro.h>

void S9xAppendMapping(struct retro_memory_descriptor *desc);

/* Forward declarations. */
static uint8_t InitROM (void);

#define MAP_LIBRETRO_RAW(flags, ptr, offset, start, select, disconnect, len) \
	do { \
		struct retro_memory_descriptor desc={flags, ptr, offset, start, select, disconnect, len, NULL}; \
		S9xAppendMapping(&desc); \
	} while(0)

#define MAP_LIBRETRO(flags, ptr, offset, disconnect, len, bank_s,bank_e,addr_s,addr_e) \
	MAP_LIBRETRO_RAW(flags, ptr, offset, \
		(bank_s<<16 | addr_s), ((bank_s<<16 | addr_s) ^ (bank_e<<16 | addr_e) ^ 0xFFFFFF), \
		disconnect, len)


static const uint32_t	crc32Table[256] =
{
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/* deinterleave*/

static void S9xDeinterleaveType1 (int size, uint8_t *base)
{
	uint8_t *tmp;
	uint8_t	blocks[256], b;
	int nblocks;
	int	i, j;
	
	nblocks = size >> 16;

	for ( i = 0; i < nblocks; i++)
	{
		blocks[i * 2] = i + nblocks;
		blocks[i * 2 + 1] = i;
	}

	tmp = (uint8_t *) malloc(0x8000);
	if (tmp)
	{
		for ( i = 0; i < nblocks * 2; i++)
		{
			for ( j = i; j < nblocks * 2; j++)
			{
				if (blocks[j] == i)
				{
					memcpy(tmp, &base[blocks[j] * 0x8000], 0x8000);
					memmove(&base[blocks[j] * 0x8000], &base[blocks[i] * 0x8000], 0x8000);
					memcpy(&base[blocks[i] * 0x8000], tmp, 0x8000);
					b = blocks[j];
					blocks[j] = blocks[i];
					blocks[i] = b;
					break;
				}
			}
		}

		free(tmp);
	}
}

static void S9xDeinterleaveType2 (int size, uint8_t *base)
{
	/* for odd Super FX images */
	uint8_t *tmp;
	uint8_t	blocks[256], b;
	int nblocks, step, i, j;

	nblocks = size >> 16;
	step = 64;

	while (nblocks <= step)
		step >>= 1;
	nblocks = step;

	for ( i = 0; i < nblocks * 2; i++)
		blocks[i] = (i & ~0xf) | ((i & 3) << 2) | ((i & 12) >> 2);

	tmp = (uint8_t *) malloc(0x10000);
	if (tmp)
	{
		for ( i = 0; i < nblocks * 2; i++)
		{
			for ( j = i; j < nblocks * 2; j++)
			{
				if (blocks[j] == i)
				{
					memcpy(tmp, &base[blocks[j] * 0x10000], 0x10000);
					memmove(&base[blocks[j] * 0x10000], &base[blocks[i] * 0x10000], 0x10000);
					memcpy(&base[blocks[i] * 0x10000], tmp, 0x10000);
					b = blocks[j];
					blocks[j] = blocks[i];
					blocks[i] = b;
					break;
				}
			}
		}

		free(tmp);
	}
}

static void S9xDeinterleaveGD24 (int size, uint8_t *base)
{
	uint8_t *tmp;
	/* for 24Mbit images dumped with Game Doctor */
	if (size != 0x300000)
		return;

	tmp = (uint8_t *) malloc(0x80000);
	if (tmp)
	{
		memcpy(tmp, &base[0x180000], 0x80000);
		memcpy(&base[0x180000], &base[0x200000], 0x80000);
		memcpy(&base[0x200000], &base[0x280000], 0x80000);
		memcpy(&base[0x280000], tmp, 0x80000);

		free(tmp);

		S9xDeinterleaveType1(size, base);
	}
}

/* allocation and deallocation */

uint8_t Init (void)
{
	/* Defensive teardown: if Init() is re-entered without an intervening
	   Deinit() (statically linked frontends where retro_init can be
	   called twice, console ROM-swap flows that re-init the core), the
	   18 calloc calls below would orphan all prior allocations -
	   9MB+ of buffers. Deinit() is idempotent. */
	Deinit();

	Memory.RAM	 = (uint8_t *) calloc(1, 0x20000);
	Memory.SRAM = (uint8_t *) calloc(1, 0x80000);
	Memory.VRAM = (uint8_t *) calloc(1, 0x10000);
	Memory.ROM  = (uint8_t *) calloc(1, MAX_ROM_SIZE + 0x200 + 0x8000);

	IPPU.TileCache[TILE_2BIT]       = (uint8_t *) calloc(1, MAX_2BIT_TILES * 64);
	IPPU.TileCache[TILE_4BIT]       = (uint8_t *) calloc(1, MAX_4BIT_TILES * 64);
	IPPU.TileCache[TILE_8BIT]       = (uint8_t *) calloc(1, MAX_8BIT_TILES * 64);
	IPPU.TileCache[TILE_2BIT_EVEN]  = (uint8_t *) calloc(1, MAX_2BIT_TILES * 64);
	IPPU.TileCache[TILE_2BIT_ODD]   = (uint8_t *) calloc(1, MAX_2BIT_TILES * 64);
	IPPU.TileCache[TILE_4BIT_EVEN]  = (uint8_t *) calloc(1, MAX_4BIT_TILES * 64);
	IPPU.TileCache[TILE_4BIT_ODD]   = (uint8_t *) calloc(1, MAX_4BIT_TILES * 64);

	IPPU.TileCached[TILE_2BIT]      = (uint8_t *) calloc(1, MAX_2BIT_TILES);
	IPPU.TileCached[TILE_4BIT]      = (uint8_t *) calloc(1, MAX_4BIT_TILES);
	IPPU.TileCached[TILE_8BIT]      = (uint8_t *) calloc(1, MAX_8BIT_TILES);
	IPPU.TileCached[TILE_2BIT_EVEN] = (uint8_t *) calloc(1, MAX_2BIT_TILES);
	IPPU.TileCached[TILE_2BIT_ODD]  = (uint8_t *) calloc(1, MAX_2BIT_TILES);
	IPPU.TileCached[TILE_4BIT_EVEN] = (uint8_t *) calloc(1, MAX_4BIT_TILES);
	IPPU.TileCached[TILE_4BIT_ODD]  = (uint8_t *) calloc(1, MAX_4BIT_TILES);


	/* don't render subscreen speed hack - disable this by default by 
	turning the variable (RenderSub - opposite of don't render sub) on */

	PPU.RenderSub = TRUE;
	PPU.FullClipping = TRUE;
	coldata_update_screen = TRUE;

	if (!Memory.RAM || !Memory.SRAM || !Memory.VRAM || !Memory.ROM ||
			!IPPU.TileCache[TILE_2BIT]       ||
			!IPPU.TileCache[TILE_4BIT]       ||
			!IPPU.TileCache[TILE_8BIT]       ||
			!IPPU.TileCache[TILE_2BIT_EVEN]  ||
			!IPPU.TileCache[TILE_2BIT_ODD]   ||
			!IPPU.TileCache[TILE_4BIT_EVEN]  ||
			!IPPU.TileCache[TILE_4BIT_ODD]   ||
			!IPPU.TileCached[TILE_2BIT]      ||
			!IPPU.TileCached[TILE_4BIT]      ||
			!IPPU.TileCached[TILE_8BIT]      ||
			!IPPU.TileCached[TILE_2BIT_EVEN] ||
			!IPPU.TileCached[TILE_2BIT_ODD]  ||
			!IPPU.TileCached[TILE_4BIT_EVEN] ||
			!IPPU.TileCached[TILE_4BIT_ODD])
	{
		Deinit();
		return (FALSE);
	}

	/* FillRAM uses first 32K of ROM image area, otherwise space just
	wasted. Might be read by the SuperFX code. */

	Memory.FillRAM = Memory.ROM;

	/* Add 0x8000 to ROM image pointer to stop SuperFX code accessing
	unallocated memory (can cause crash on some ports). */

	Memory.ROM += 0x8000;

	Memory.C4RAM   = Memory.ROM + 0x400000 + 8192 * 8; /* C4 */
	Memory.OBC1RAM = Memory.ROM + 0x400000; /* OBC1 */
	Memory.BIOSROM = Memory.ROM + 0x300000; /* BS */
	Memory.BSRAM   = Memory.ROM + 0x400000; /* BS */

	SuperFX.pvRegisters = Memory.FillRAM + 0x3000;
	SuperFX.nRamBanks   = 2; /* Most only use 1.  1=64KB=512Mb, 2=128KB=1024Mb */
	SuperFX.pvRam       = Memory.SRAM;
	SuperFX.nRomBanks   = (2 * 1024 * 1024) / (32 * 1024);
	SuperFX.pvRom       = (uint8_t *) Memory.ROM;

	return (TRUE);
}

static char * Safe (const char *s)
{
	static char	*safe = NULL;
	static int	safe_len = 0;
	int		i, len;

	if (s == NULL)
	{
		if (safe)
		{
			free(safe);
			safe = NULL;
		}

		return (NULL);
	}

	len = strlen(s);
	if (!safe || len + 1 > safe_len)
	{
		if (safe)
			free(safe);

		safe_len = len + 1;
		safe = (char *) malloc(safe_len);
	}

	for ( i = 0; i < len; i++)
	{
		if (s[i] >= 32 && s[i] < 127)
			safe[i] = s[i];
		else
			safe[i] = '_';
	}

	safe[len] = 0;

	return (safe);
}

static char * SafeANK (uint8_t ROMRegion, const char *s)
{
	static char	*safe = NULL;
	static int	safe_len = 0;
	int		i, len;

	if (s == NULL)
	{
		if (safe)
		{
			free(safe);
			safe = NULL;
		}

		return (NULL);
	}

	len = strlen(s);
	if (!safe || len + 1 > safe_len)
	{
		if (safe)
			free(safe);

		safe_len = len + 1;
		safe = (char *) malloc(safe_len);
	}

	for ( i = 0; i < len; i++)
	{
		if (s[i] >= 32 && s[i] < 127) /* ASCII */
			safe [i] = s[i];
		else
		if (ROMRegion == 0 && ((uint8_t) s[i] >= 0xa0 && (uint8_t) s[i] < 0xe0)) /* JIS X 201 - Katakana */
			safe [i] = s[i];
		else
			safe [i] = '_';
	}

	safe [len] = 0;

	return (safe);
}

void Deinit (void)
{
	int t;
	if (Memory.RAM)
	{
		free(Memory.RAM);
		Memory.RAM = NULL;
	}

	if (Memory.SRAM)
	{
		free(Memory.SRAM);
		Memory.SRAM = NULL;
	}

	if (Memory.VRAM)
	{
		free(Memory.VRAM);
		Memory.VRAM = NULL;
	}

	if (Memory.ROM)
	{
		Memory.ROM -= 0x8000;
		free(Memory.ROM);
		Memory.ROM = NULL;
	}

	if(Settings.SPC7110 || Settings.SPC7110RTC)
	{
		S9xFreeSPC7110();
	}

	for ( t = 0; t < 7; t++)
	{
		if (IPPU.TileCache[t])
		{
			free(IPPU.TileCache[t]);
			IPPU.TileCache[t] = NULL;
		}

		if (IPPU.TileCached[t])
		{
			free(IPPU.TileCached[t]);
			IPPU.TileCached[t] = NULL;
		}
	}

	Safe(NULL);
	SafeANK(Memory.ROMRegion, NULL);
}

/* file management and ROM detection */

static uint8_t allASCII (uint8_t *b, int size)
{
	int i;
	for ( i = 0; i < size; i++)
	{
		if (b[i] < 32 || b[i] > 126)
			return (FALSE);
	}

	return (TRUE);
}

#if SNES_SUPPORT_MULTI_CART
static uint8_t is_SufamiTurbo_BIOS (uint8_t *data, uint32_t size)
{
	if (size == 0x40000 && strncmp((char *) data, "BANDAI SFC-ADX", 14) == 0
	&& strncmp((char * ) (data + 0x10), "SFC-ADX BACKUP", 14) == 0)
		return TRUE;
   return FALSE;
}

static uint8_t is_SufamiTurbo_Cart (uint8_t *data, uint32_t size)
{
	if (size >= 0x80000 && size <= 0x100000 && strncmp((char *) data, "BANDAI SFC-ADX", 14) == 0
	&& strncmp((char * ) (data + 0x10), "SFC-ADX BACKUP", 14) != 0)
		return TRUE;
   return FALSE;
}

static uint8_t is_SameGame_BIOS (uint8_t *data, uint32_t size)
{
	if (size == 0x100000 && strncmp((char *) (data + 0xffc0), "Same Game Tsume Game", 20) == 0)
		return (TRUE);
   return (FALSE);
}

static uint8_t is_SameGame_Add_On (uint8_t *data, uint32_t size)
{
	if (size == 0x80000)
		return (TRUE);
   return (FALSE);
}
#endif

static int ScoreHiROM (uint32_t calculated_size, uint8_t * rom,  uint8_t skip_header, int32_t romoff)
{
	uint8_t	*buf;
	int score;

	score = 0;
	buf = rom + 0xff00 + romoff + (skip_header ? 0x200 : 0);

	if (buf[0xd5] & 0x1)
		score += 2;

	/* Mode23 is SA-1 */
	if (buf[0xd5] == 0x23)
		score -= 2;

	if (buf[0xd4] == 0x20)
		score += 2;

	if ((buf[0xdc] + (buf[0xdd] << 8)) + (buf[0xde] + (buf[0xdf] << 8)) == 0xffff)
	{
		score += 2;
		if (0 != (buf[0xde] + (buf[0xdf] << 8)))
			score++;
	}

	if (buf[0xda] == 0x33)
		score += 2;

	if ((buf[0xd5] & 0xf) < 4)
		score += 2;

	if (!(buf[0xfd] & 0x80))
		score -= 6;

	if ((buf[0xfc] + (buf[0xfd] << 8)) > 0xffb0)
		score -= 2; /* reduced after looking at a scan by Cowering */

	if (calculated_size > 1024 * 1024 * 3)
		score += 4;

	if ((1 << (buf[0xd7] - 7)) > 48)
		score -= 1;

	if (!allASCII(&buf[0xb0], 6))
		score -= 1;

	if (!allASCII(&buf[0xc0], ROM_NAME_LEN - 1))
		score -= 1;

	return (score);
}

static int ScoreLoROM (uint32_t calculated_size, uint8_t * rom, uint8_t skip_header, int32_t romoff)
{
	uint8_t	*buf;
	int score;

	score = 0;
	buf = rom + 0x7f00 + romoff + (skip_header ? 0x200 : 0);

	if (!(buf[0xd5] & 0x1))
		score += 3;

	/* Mode23 is SA-1 */
	if (buf[0xd5] == 0x23)
		score += 2;

	if ((buf[0xdc] + (buf[0xdd] << 8)) + (buf[0xde] + (buf[0xdf] << 8)) == 0xffff)
	{
		score += 2;
		if (0 != (buf[0xde] + (buf[0xdf] << 8)))
			score++;
	}

	if (buf[0xda] == 0x33)
		score += 2;

	if ((buf[0xd5] & 0xf) < 4)
		score += 2;

	if (!(buf[0xfd] & 0x80))
		score -= 6;

	if ((buf[0xfc] + (buf[0xfd] << 8)) > 0xffb0)
		score -= 2; /* reduced per Cowering suggestion */

	if (calculated_size <= 1024 * 1024 * 16)
		score += 2;

	if ((1 << (buf[0xd7] - 7)) > 48)
		score -= 1;

	if (!allASCII(&buf[0xb0], 6))
		score -= 1;

	if (!allASCII(&buf[0xc0], ROM_NAME_LEN - 1))
		score -= 1;

	return (score);
}

static uint32_t HeaderRemove (uint32_t size, int32_t * headerCount, uint8_t *buf)
{
	uint32_t calc_size = (size / 0x2000) * 0x2000;

	if (size - calc_size == 512)
	{
		memmove(buf, buf + 512, calc_size);
		(*headerCount)++;
		size -= 512;
	}

	return (size);
}

static uint32_t FileLoader (uint8_t *buffer, int32_t maxsize)
{
	/* <- ROM size without header */
	/* ** Memory.HeaderCount */
	/* ** Memory.ROMFilename */

	uint8_t	*ptr;
	uint64_t	size      = 0;
	STREAM fp          = OPEN_STREAM(0);

	Memory.HeaderCount = 0;
	if (!fp)
		return (0);

	ptr                = buffer;

   size               = READ_STREAM(ptr, (uint64_t)(
            maxsize + 0x200), fp);
   CLOSE_STREAM(fp);

   return HeaderRemove(size, &Memory.HeaderCount, ptr);
}

static uint32_t caCRC32 (uint8_t *array, uint32_t size, uint32_t crc32)
{
	uint32_t i;

	for ( i = 0; i < size; i++)
		crc32 = ((crc32 >> 8) & 0x00FFFFFF) ^ crc32Table[(crc32 ^ array[i]) & 0xFF];

	return (~crc32);
}

uint8_t LoadROM (void)
{
	int	hi_score, lo_score, retry_count;
	uint8_t interleaved, tales;
	uint8_t * RomHeader;
	int32_t totalFileSize;

	retry_count = 0;

	memset(Memory.ROM, 0, MAX_ROM_SIZE);
	memset(&Multi, 0, sizeof(Multi));

again:
	Memory.CalculatedSize = 0;
	Memory.ExtendedFormat = NOPE;

	totalFileSize = FileLoader(Memory.ROM, MAX_ROM_SIZE);
	if (!totalFileSize)
	{
		S9xMessage(S9X_MSG_ERROR, S9X_CATEGORY_ROM,
				"Unable to load ROM");
		return FALSE;
	}

	hi_score = ScoreHiROM(Memory.CalculatedSize, Memory.ROM, FALSE, 0);
	lo_score = ScoreLoROM(Memory.CalculatedSize, Memory.ROM, FALSE, 0);

	if (Memory.HeaderCount == 0 && !Settings.ForceNoHeader &&
		((hi_score >  lo_score && ScoreHiROM(Memory.CalculatedSize, Memory.ROM, TRUE, 0) > hi_score) ||
		 (hi_score <= lo_score && ScoreLoROM(Memory.CalculatedSize, Memory.ROM, TRUE, 0) > lo_score)))
	{
		memmove(Memory.ROM, Memory.ROM + 512, totalFileSize - 512);
		totalFileSize -= 512;
		S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_ROM, "Try 'force no-header' option if the game doesn't work");
		/* modifying ROM, so we need to rescore */
		hi_score = ScoreHiROM(Memory.CalculatedSize, Memory.ROM, FALSE, 0);
		lo_score = ScoreLoROM(Memory.CalculatedSize, Memory.ROM, FALSE, 0);
	}

	Memory.CalculatedSize = (totalFileSize / 0x2000) * 0x2000;

	if (Memory.CalculatedSize > 0x400000 &&
		(Memory.ROM[0x7fd5] + (Memory.ROM[0x7fd6] << 8)) != 0x4332 && /* exclude S-DD1 */
		(Memory.ROM[0x7fd5] + (Memory.ROM[0x7fd6] << 8)) != 0x4532 &&
		(Memory.ROM[0xffd5] + (Memory.ROM[0xffd6] << 8)) != 0xF93a && /* exclude SPC7110 */
		(Memory.ROM[0xffd5] + (Memory.ROM[0xffd6] << 8)) != 0xF53a)
		Memory.ExtendedFormat = YEAH;

	/* if both vectors are invalid, it's type 1 interleaved LoROM */
	if (Memory.ExtendedFormat == NOPE &&
		((Memory.ROM[0x7ffc] + (Memory.ROM[0x7ffd] << 8)) < 0x8000) &&
		((Memory.ROM[0xfffc] + (Memory.ROM[0xfffd] << 8)) < 0x8000))
	{
		if (!Settings.ForceInterleaved && !Settings.ForceNotInterleaved)
			S9xDeinterleaveType1(totalFileSize, Memory.ROM);
	}

	/* CalculatedSize is now set, so rescore */
	hi_score = ScoreHiROM(Memory.CalculatedSize, Memory.ROM, FALSE, 0);
	lo_score = ScoreLoROM(Memory.CalculatedSize, Memory.ROM, FALSE, 0);

	RomHeader = Memory.ROM;

	if (Memory.ExtendedFormat != NOPE)
	{
		int	swappedhirom, swappedlorom;

		swappedhirom = ScoreHiROM(Memory.CalculatedSize, Memory.ROM, FALSE, 0x400000);
		swappedlorom = ScoreLoROM(Memory.CalculatedSize, Memory.ROM, FALSE, 0x400000);

		/* set swapped here */
		if (max(swappedlorom, swappedhirom) >= max(lo_score, hi_score))
		{
			Memory.ExtendedFormat = BIGFIRST;
			hi_score = swappedhirom;
			lo_score = swappedlorom;
			RomHeader += 0x400000;
		}
		else
			Memory.ExtendedFormat = SMALLFIRST;
	}

	interleaved = FALSE;
	tales = FALSE;

	interleaved = Settings.ForceInterleaved || Settings.ForceInterleaved2 || Settings.ForceInterleaveGD24;

	if (Settings.ForceLoROM || (!Settings.ForceHiROM && lo_score >= hi_score))
	{
		Memory.LoROM = TRUE;
		Memory.HiROM = FALSE;

		/* ignore map type byte if not 0x2x or 0x3x */
		if ((RomHeader[0x7fd5] & 0xf0) == 0x20 || (RomHeader[0x7fd5] & 0xf0) == 0x30)
		{
			switch (RomHeader[0x7fd5] & 0xf)
			{
				case 1:
					interleaved = TRUE;
					break;

				case 5:
					interleaved = TRUE;
					tales = TRUE;
					break;
			}
		}
	}
	else
	{
		Memory.LoROM = FALSE;
		Memory.HiROM = TRUE;

		if ((RomHeader[0xffd5] & 0xf0) == 0x20 || (RomHeader[0xffd5] & 0xf0) == 0x30)
		{
			switch (RomHeader[0xffd5] & 0xf)
			{
				case 0:
				case 3:
					interleaved = TRUE;
					break;
			}
		}
	}

	/* these two games fail to be detected otherwise */
	if (!Settings.ForceHiROM && !Settings.ForceLoROM)
	{
		if (strncmp((char *) &Memory.ROM[0x7fc0], "YUYU NO QUIZ DE GO!GO!", 22) == 0 ||
		   (strncmp((char *) &Memory.ROM[0xffc0], "BATMAN--REVENGE JOKER",  21) == 0))
		{
			Memory.LoROM = TRUE;
			Memory.HiROM = FALSE;
			interleaved = FALSE;
			tales = FALSE;
		}
	}

	if (!Settings.ForceNotInterleaved && interleaved)
	{
		S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_ROM, "ROM image is in interleaved format - converting...");

		if (tales)
		{
			if (Memory.ExtendedFormat == BIGFIRST)
			{
				S9xDeinterleaveType1(0x400000, Memory.ROM);
				S9xDeinterleaveType1(Memory.CalculatedSize - 0x400000, Memory.ROM + 0x400000);
			}
			else
			{
				S9xDeinterleaveType1(Memory.CalculatedSize - 0x400000, Memory.ROM);
				S9xDeinterleaveType1(0x400000, Memory.ROM + Memory.CalculatedSize - 0x400000);
			}

			Memory.LoROM = FALSE;
			Memory.HiROM = TRUE;
		}
		else
		if (Settings.ForceInterleaveGD24 && Memory.CalculatedSize == 0x300000)
		{
			uint8_t	t = Memory.LoROM;
			Memory.LoROM = Memory.HiROM;
			Memory.HiROM = t;
			S9xDeinterleaveGD24(Memory.CalculatedSize, Memory.ROM);
		}
		else
		if (Settings.ForceInterleaved2)
			S9xDeinterleaveType2(Memory.CalculatedSize, Memory.ROM);
		else
		{
			uint8_t	t = Memory.LoROM;
			Memory.LoROM = Memory.HiROM;
			Memory.HiROM = t;
			S9xDeinterleaveType1(Memory.CalculatedSize, Memory.ROM);
		}

		hi_score = ScoreHiROM(Memory.CalculatedSize, Memory.ROM, FALSE, 0);
		lo_score = ScoreLoROM(Memory.CalculatedSize, Memory.ROM, FALSE, 0);

		if ((Memory.HiROM && (lo_score >= hi_score || hi_score < 0)) ||
			(Memory.LoROM && (hi_score >  lo_score || lo_score < 0)))
		{
			if (retry_count == 0)
			{
				S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_ROM, "ROM lied about its type! Trying again.");
				Settings.ForceNotInterleaved = TRUE;
				Settings.ForceInterleaved = FALSE;
				retry_count++;
				goto again;
			}
		}
    }

	if (Memory.ExtendedFormat == SMALLFIRST)
		tales = TRUE;

	if (tales)
	{
		uint8_t	*tmp = (uint8_t *) malloc(Memory.CalculatedSize - 0x400000);
		if (tmp)
		{
			S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_ROM, "Fixing swapped ExHiROM...");
			memcpy(tmp, Memory.ROM, Memory.CalculatedSize - 0x400000);
			memmove(Memory.ROM, Memory.ROM + Memory.CalculatedSize - 0x400000, 0x400000);
			memcpy(Memory.ROM + 0x400000, tmp, Memory.CalculatedSize - 0x400000);
			free(tmp);
		}
	}

	memset(&SNESGameFixes, 0, sizeof(SNESGameFixes));
	SNESGameFixes.SRAMInitialValue = 0x60;

	if(InitROM() != TRUE)
		return (FALSE);

	S9xInitCheatData();
	S9xApplyCheats();

	S9xReset();

    return (TRUE);
}

#if SNES_SUPPORT_MULTI_CART
uint8_t LoadMultiCart (const char *cartA, const char *cartB)
{
	uint8_t r;

	r = TRUE;

	memset(Memory.ROM, 0, MAX_ROM_SIZE);
	memset(&Multi, 0, sizeof(Multi));

	Memory.CalculatedSize = 0;
	Memory.ExtendedFormat = NOPE;

	if (cartA && cartA[0])
		Multi.cartSizeA = FileLoader(Memory.ROM, cartA, MAX_ROM_SIZE);

	if (Multi.cartSizeA == 0)
	{
		if (cartB && cartB[0])
			Multi.cartSizeB = FileLoader(Memory.ROM, cartB, MAX_ROM_SIZE);
	}

	if (Multi.cartSizeA)
	{
		if (is_SufamiTurbo_Cart(Memory.ROM, Multi.cartSizeA))
			Multi.cartType = 4;
		else
		if (is_SameGame_BIOS(Memory.ROM, Multi.cartSizeA))
			Multi.cartType = 3;
	}
	else
	if (Multi.cartSizeB)
	{
		if (is_SufamiTurbo_Cart(Memory.ROM, Multi.cartSizeB))
			Multi.cartType = 4;
	}
	else
		Multi.cartType = 4; /* assuming BIOS only */

	switch (Multi.cartType)
	{
		case 4:
			r = LoadSufamiTurbo(cartA, cartB);
			break;

		case 3:
			r = LoadSameGame(cartA, cartB);
			break;

		default:
			r = FALSE;
	}

	if (!r)
	{
		memset(&Multi, 0, sizeof(Multi));
		return (FALSE);
	}

	memset(&SNESGameFixes, 0, sizeof(SNESGameFixes));
	SNESGameFixes.SRAMInitialValue = 0x60;

	if(InitROM() != TRUE)
		return (FALSE);

	S9xInitCheatData();
	S9xApplyCheats();

	S9xReset();

	return (TRUE);
}

uint8_t LoadSufamiTurbo (const char *cartA, const char *cartB)
{
	RFILE *fp = NULL;
	int64_t	size;
	char	path[PATH_MAX + 1];

	Multi.cartOffsetA = 0x100000;
	Multi.cartOffsetB = 0x200000;
	Multi.sramA = Memory.SRAM;
	Multi.sramB = Memory.SRAM + 0x10000;

	if (Multi.cartSizeA)
	{
		Multi.sramSizeA = 4; /* ROM[0x37]? */
		Multi.sramMaskA = Multi.sramSizeA ? ((1 << (Multi.sramSizeA + 3)) * 128 - 1) : 0;

		strcpy(Multi.fileNameA, cartA);
		memcpy(Memory.ROM + Multi.cartOffsetA, Memory.ROM, Multi.cartSizeA);
	}

	if (Multi.cartSizeA && !Multi.cartSizeB)
	{
		if (cartB && cartB[0])
			Multi.cartSizeB = FileLoader(Memory.ROM, cartB, MAX_ROM_SIZE);

		if (Multi.cartSizeB)
		{
			if (!is_SufamiTurbo_Cart(Memory.ROM, Multi.cartSizeB))
				Multi.cartSizeB = 0;
		}
	}

	if (Multi.cartSizeB)
	{
		Multi.sramSizeB = 4; /* ROM[0x37]? */
		Multi.sramMaskB = Multi.sramSizeB ? ((1 << (Multi.sramSizeB + 3)) * 128 - 1) : 0;

		strcpy(Multi.fileNameB, cartB);
		memcpy(Memory.ROM + Multi.cartOffsetB, Memory.ROM, Multi.cartSizeB);
	}

	strcpy(path, S9xGetDirectory(BIOS_DIR));
	strcat(path, SLASH_STR);
	strcat(path, "STBIOS.bin");

	fp = filestream_open(path, RETRO_VFS_FILE_ACCESS_READ,
			RETRO_VFS_FILE_ACCESS_HINT_NONE);

	if (fp)
	{
		size = filestream_read(fp, (void *)Memory.ROM, 0x40000);
		filestream_close(fp);

		if ((size <= 0) || !is_SufamiTurbo_BIOS(Memory.ROM, (size_t)size))
			return (FALSE);
	}
	else
		return (FALSE);

	Memory.LoROM = TRUE;
	Memory.HiROM = FALSE;
	Memory.CalculatedSize = 0x40000;

	return (TRUE);
}

uint8_t LoadSameGame (const char *cartA, const char *cartB)
{
	Multi.cartOffsetA = 0;
	Multi.cartOffsetB = 0x200000;
	Multi.sramA = Memory.SRAM;
	Multi.sramB = NULL;

	Multi.sramSizeA = Memory.ROM[0xffd8];
	Multi.sramMaskA = Multi.sramSizeA ? ((1 << (Multi.sramSizeA + 3)) * 128 - 1) : 0;
	Multi.sramSizeB = 0;
	Multi.sramMaskB = 0;

	strcpy(Multi.fileNameA, cartA);

	if (cartB && cartB[0])
		Multi.cartSizeB = FileLoader(Memory.ROM + Multi.cartOffsetB, cartB, MAX_ROM_SIZE - Multi.cartOffsetB);

	if (Multi.cartSizeB)
	{
		if (!is_SameGame_Add_On(Memory.ROM + Multi.cartOffsetB, Multi.cartSizeB))
			Multi.cartSizeB = 0;
		else
			strcpy(Multi.fileNameB, cartB);
	}

	Memory.LoROM = FALSE;
	Memory.HiROM = TRUE;
	Memory.CalculatedSize = Multi.cartSizeA;

	return (TRUE);
}
#endif /* SNES_SUPPORT_MULTI_CART */

#define MAP_INITIALIZE() \
{ \
	int c; \
	for ( c = 0; c < 0x1000; c++) \
	{ \
		Memory.Map[c]      = (uint8_t *) MAP_NONE; \
		Memory.WriteMap[c] = (uint8_t *) MAP_NONE; \
		Memory.BlockIsROM[c] = FALSE; \
		Memory.BlockIsRAM[c] = FALSE; \
	} \
}

static uint16_t checksum_calc_sum (uint8_t *data, uint32_t length)
{
	uint32_t i;
	uint16_t sum;

	sum = 0;

	for ( i = 0; i < length; i++)
		sum += data[i];

	return (sum);
}

static uint16_t checksum_mirror_sum (uint8_t *start, uint32_t * length, uint32_t mask)
{
	uint16_t part1, part2;
	uint32_t next_length;

	/* from NSRT */
	while (!(*length & mask))
		mask >>= 1;

	part1 = checksum_calc_sum(start, mask);
	part2 = 0;

	next_length = *length - mask;
	if (next_length)
	{
		part2 = checksum_mirror_sum(start + mask, &next_length, mask >> 1);

		while (next_length < mask)
		{
			next_length += next_length;
			part2 += part2;
		}

		*length = mask + mask;
	}

	return (part1 + part2);
}

static uint32_t map_mirror (uint32_t size, uint32_t pos)
{
	uint32_t mask;
	/* from bsnes */
	if (size == 0)
		return (0);
	if (pos < size)
		return (pos);

	mask = UINT32_C(1) << 31;
	while (!(pos & mask))
		mask >>= 1;

	if (size <= (pos & mask))
		return (map_mirror(size, pos - mask));
   return (mask + map_mirror(size - mask, pos - mask));
}

#define MATCH_NA(str) (strcmp(Memory.ROMName, str) == 0)
#define MATCH_NN(str) (strncmp(Memory.ROMName, str, strlen(str)) == 0)
#define MATCH_NC(str) (strncasecmp(Memory.ROMName, str, strlen(str)) == 0)
#define MATCH_ID(str) (strncmp(Memory.ROMId, str, strlen(str)) == 0)

#define MAP_HIROM(bank_s, bank_e, addr_s, addr_e, size, auto_export_map) \
{ \
	uint32_t i, c; \
	for ( c = bank_s; c <= bank_e; c++) \
	{ \
		for ( i = addr_s; i <= addr_e; i += 0x1000) \
		{ \
			uint32_t p = (c << 4) | (i >> 12); \
			uint32_t addr = c << 16; \
			Memory.Map[p] = Memory.ROM + map_mirror(size, addr); \
			Memory.BlockIsROM[p] = TRUE; \
			Memory.BlockIsRAM[p] = FALSE; \
		} \
	} \
	if (auto_export_map) \
	{ \
		MAP_LIBRETRO(RETRO_MEMDESC_CONST, Memory.ROM, map_mirror(size, addr_s), 0, \
			size - map_mirror(size, addr_s), bank_s,bank_e,addr_s,addr_e); \
	} \
}

#define MAP_INDEX(bank_s, bank_e, addr_s, addr_e, index, type, auto_export_map) \
{ \
	uint32_t i, c; \
	for ( c = bank_s; c <= bank_e; c++) \
	{ \
		for ( i = addr_s; i <= addr_e; i += 0x1000) \
		{ \
			uint32_t p = (c << 4) | (i >> 12); \
			Memory.Map[p] = (uint8_t *) index; \
			Memory.BlockIsROM[p] = ((type == MAP_TYPE_I_O) || (type == MAP_TYPE_RAM)) ? FALSE : TRUE; \
			Memory.BlockIsRAM[p] = ((type == MAP_TYPE_I_O) || (type == MAP_TYPE_ROM)) ? FALSE : TRUE; \
		} \
	} \
	if (auto_export_map) \
	{ \
		if (type==MAP_LOROM_SRAM || type==MAP_SA1RAM) \
			MAP_LIBRETRO(0, Memory.SRAM, 0, 0x8000, Memory.SRAMMask+1, bank_s,bank_e,addr_s,addr_e); \
		if (type==MAP_LOROM_SRAM_B) \
			MAP_LIBRETRO(0, Multi.sramB, 0, 0x8000, Multi.sramMaskB+1, bank_s,bank_e,addr_s,addr_e); \
		if (type==MAP_HIROM_SRAM || type==MAP_RONLY_SRAM) \
			MAP_LIBRETRO(0, Memory.SRAM, 0, 0xE000, Memory.SRAMMask+1, bank_s,bank_e,addr_s,addr_e); \
	} \
}

#define MAP_SPACE(bank_s, bank_e, addr_s, addr_e, data, auto_export_map) \
{ \
	uint32_t i,x; \
	for ( x = bank_s; x <= bank_e; x++) \
	{ \
		for ( i = addr_s; i <= addr_e; i += 0x1000) \
		{ \
			uint32_t p = (x << 4) | (i >> 12); \
			Memory.Map[p] = data; \
			Memory.BlockIsROM[p] = FALSE; \
			Memory.BlockIsRAM[p] = TRUE; \
		} \
	} \
	if (auto_export_map) \
	{ \
		MAP_LIBRETRO(0, data, 0, 0xFF0000, 0, bank_s,bank_e,addr_s,addr_e); \
	} \
}

#define MAP_LOROM_OFFSET(bank_s, bank_e, addr_s, addr_e, size, offset, auto_export_map) \
{ \
	uint32_t i, c; \
	for ( c = bank_s; c <= bank_e; c++) \
	{ \
		for ( i = addr_s; i <= addr_e; i += 0x1000) \
		{ \
			uint32_t p = (c << 4) | (i >> 12); \
			uint32_t addr = ((c - bank_s) & 0x7f) * 0x8000; \
			Memory.Map[p] = Memory.ROM + offset + map_mirror(size, addr) - (i & 0x8000); \
			Memory.BlockIsROM[p] = TRUE; \
			Memory.BlockIsRAM[p] = FALSE; \
		} \
	} \
	if (auto_export_map) \
	{ \
		MAP_LIBRETRO(RETRO_MEMDESC_CONST, Memory.ROM, offset, 0x8000, size, bank_s,bank_e,addr_s,addr_e); \
	} \
}

#define MAP_HIROM_OFFSET(bank_s, bank_e, addr_s, addr_e, size, offset, auto_export_map) \
{ \
	uint32_t i, c; \
	for ( c = bank_s; c <= bank_e; c++) \
	{ \
		for ( i = addr_s; i <= addr_e; i += 0x1000) \
		{ \
			uint32_t p = (c << 4) | (i >> 12); \
			uint32_t addr = (c - bank_s) << 16; \
			Memory.Map[p] = Memory.ROM + offset + map_mirror(size, addr); \
			Memory.BlockIsROM[p] = TRUE; \
			Memory.BlockIsRAM[p] = FALSE; \
		} \
	} \
	if (auto_export_map) \
	{ \
		MAP_LIBRETRO(RETRO_MEMDESC_CONST, Memory.ROM, map_mirror(size, offset + addr_s), 0, size, bank_s,bank_e,addr_s,addr_e); \
	} \
}

#define MAP_LOROMSRAM() \
	MAP_INDEX(0x70, 0x7f, 0x0000, 0x7fff, MAP_LOROM_SRAM, MAP_TYPE_RAM, true); \
	MAP_INDEX(0xf0, 0xff, 0x0000, 0x7fff, MAP_LOROM_SRAM, MAP_TYPE_RAM, true);

#define MAP_WRAM() \
	/* will overwrite others */ \
	MAP_SPACE(0x7e, 0x7e, 0x0000, 0xffff, Memory.RAM, true); \
	MAP_SPACE(0x7f, 0x7f, 0x0000, 0xffff, Memory.RAM + 0x10000, true);

#define MAP_SYSTEM() \
	/* will be overwritten */ \
	MAP_SPACE(0x00, 0x3f, 0x0000, 0x1fff, Memory.RAM, true); \
	MAP_INDEX(0x00, 0x3f, 0x2000, 0x3fff, MAP_PPU, MAP_TYPE_I_O, true); \
	MAP_INDEX(0x00, 0x3f, 0x4000, 0x5fff, MAP_CPU, MAP_TYPE_I_O, true); \
	MAP_SPACE(0x80, 0xbf, 0x0000, 0x1fff, Memory.RAM, true); \
	MAP_INDEX(0x80, 0xbf, 0x2000, 0x3fff, MAP_PPU, MAP_TYPE_I_O, true); \
	MAP_INDEX(0x80, 0xbf, 0x4000, 0x5fff, MAP_CPU, MAP_TYPE_I_O, true);

void map_WriteProtectROM (void)
{
	int c;
	memcpy((void *) Memory.WriteMap, (void *)Memory.Map, sizeof(Memory.Map));

	for ( c = 0; c < 0x1000; c++)
	{
		if (Memory.BlockIsROM[c])
			Memory.WriteMap[c] = (uint8_t *) MAP_NONE;
	}
}

#define MAP_JUMBOLOROMMAP() \
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_JumboLoROMMap"); \
	MAP_SYSTEM(); \
	MAP_LOROM_OFFSET(0x00, 0x3f, 0x8000, 0xffff, Memory.CalculatedSize - 0x400000, 0x400000, true); \
	MAP_LOROM_OFFSET(0x40, 0x7f, 0x0000, 0xffff, Memory.CalculatedSize - 0x600000, 0x600000, true); \
	MAP_LOROM_OFFSET(0x80, 0xbf, 0x8000, 0xffff, 0x400000, 0, true); \
	MAP_LOROM_OFFSET(0xc0, 0xff, 0x0000, 0xffff, 0x400000, 0x200000, true); \
	MAP_LOROMSRAM(); \
	MAP_WRAM(); \
	map_WriteProtectROM();

#define MAP_LOROM(bank_s, bank_e, addr_s, addr_e, size, auto_export_map) \
{ \
	uint32_t i, c; \
	for ( c = bank_s; c <= bank_e; c++) \
	{ \
		for ( i = addr_s; i <= addr_e; i += 0x1000) \
		{ \
			uint32_t p = (c << 4) | (i >> 12); \
			uint32_t addr = (c & 0x7f) * 0x8000; \
			Memory.Map[p] = Memory.ROM + map_mirror(size, addr) - (i & 0x8000); \
			Memory.BlockIsROM[p] = TRUE; \
			Memory.BlockIsRAM[p] = FALSE; \
		} \
	} \
	if (auto_export_map) \
	{ \
		MAP_LIBRETRO(RETRO_MEMDESC_CONST, Memory.ROM, 0, 0x8000, size, bank_s,bank_e,addr_s,addr_e); \
	} \
}

#define MAP_SRAM512KLOROMMAP() \
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_SRAM512KLoROMMap"); \
	MAP_SYSTEM(); \
	MAP_LOROM(0x00, 0x3f, 0x8000, 0xffff, Memory.CalculatedSize, true); \
	MAP_LOROM(0x40, 0x7f, 0x0000, 0xffff, Memory.CalculatedSize, true); \
	MAP_LOROM(0x80, 0xbf, 0x8000, 0xffff, Memory.CalculatedSize, true); \
	MAP_LOROM(0xc0, 0xff, 0x0000, 0xffff, Memory.CalculatedSize, true); \
	MAP_SPACE(0x70, 0x70, 0x0000, 0xffff, Memory.SRAM, true); \
	MAP_SPACE(0x71, 0x71, 0x0000, 0xffff, Memory.SRAM + 0x8000, true); \
	MAP_SPACE(0x72, 0x72, 0x0000, 0xffff, Memory.SRAM + 0x10000, true); \
	MAP_SPACE(0x73, 0x73, 0x0000, 0xffff, Memory.SRAM + 0x18000, true); \
	MAP_WRAM(); \
	map_WriteProtectROM();

#define MAP_DSP_() \
	switch (DSP0.maptype) \
	{ \
		case M_DSP1_LOROM_S: \
			MAP_INDEX(0x20, 0x3f, 0x8000, 0xffff, MAP_DSP, MAP_TYPE_I_O, true); \
			MAP_INDEX(0xa0, 0xbf, 0x8000, 0xffff, MAP_DSP, MAP_TYPE_I_O, true); \
			break; \
		case M_DSP1_LOROM_L: \
			MAP_INDEX(0x60, 0x6f, 0x0000, 0x7fff, MAP_DSP, MAP_TYPE_I_O, true); \
			MAP_INDEX(0xe0, 0xef, 0x0000, 0x7fff, MAP_DSP, MAP_TYPE_I_O, true); \
			break; \
		case M_DSP1_HIROM: \
			MAP_INDEX(0x00, 0x1f, 0x6000, 0x7fff, MAP_DSP, MAP_TYPE_I_O, true); \
			MAP_INDEX(0x80, 0x9f, 0x6000, 0x7fff, MAP_DSP, MAP_TYPE_I_O, true); \
			break; \
		case M_DSP2_LOROM: \
			MAP_INDEX(0x20, 0x3f, 0x6000, 0x6fff, MAP_DSP, MAP_TYPE_I_O, true); \
			MAP_INDEX(0x20, 0x3f, 0x8000, 0xbfff, MAP_DSP, MAP_TYPE_I_O, true); \
			MAP_INDEX(0xa0, 0xbf, 0x6000, 0x6fff, MAP_DSP, MAP_TYPE_I_O, true); \
			MAP_INDEX(0xa0, 0xbf, 0x8000, 0xbfff, MAP_DSP, MAP_TYPE_I_O, true); \
			break; \
		case M_DSP3_LOROM: \
			MAP_INDEX(0x20, 0x3f, 0x8000, 0xffff, MAP_DSP, MAP_TYPE_I_O, true); \
			MAP_INDEX(0xa0, 0xbf, 0x8000, 0xffff, MAP_DSP, MAP_TYPE_I_O, true); \
			break; \
		case M_DSP4_LOROM: \
			MAP_INDEX(0x30, 0x3f, 0x8000, 0xffff, MAP_DSP, MAP_TYPE_I_O, true); \
			MAP_INDEX(0xb0, 0xbf, 0x8000, 0xffff, MAP_DSP, MAP_TYPE_I_O, true); \
			break; \
	}

#define MAP_C4_() \
	MAP_INDEX(0x00, 0x3f, 0x6000, 0x7fff, MAP_C4, MAP_TYPE_I_O, true); \
	MAP_INDEX(0x80, 0xbf, 0x6000, 0x7fff, MAP_C4, MAP_TYPE_I_O, true);

#define MAP_OBC1() \
	MAP_INDEX(0x00, 0x3f, 0x6000, 0x7fff, MAP_OBC_RAM, MAP_TYPE_I_O, true); \
	MAP_INDEX(0x80, 0xbf, 0x6000, 0x7fff, MAP_OBC_RAM, MAP_TYPE_I_O, true);

#define MAP_SETARISC() \
	MAP_INDEX(0x00, 0x3f, 0x3000, 0x3fff, MAP_SETA_RISC, MAP_TYPE_I_O, true); \
	MAP_INDEX(0x80, 0xbf, 0x3000, 0x3fff, MAP_SETA_RISC, MAP_TYPE_I_O, true);

#define MAP_LOROMMAP() \
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_LoROMMap"); \
	MAP_SYSTEM(); \
	\
	MAP_LOROM(0x00, 0x3f, 0x8000, 0xffff, Memory.CalculatedSize, true); \
	MAP_LOROM(0x40, 0x7f, 0x0000, 0xffff, Memory.CalculatedSize, true); \
	MAP_LOROM(0x80, 0xbf, 0x8000, 0xffff, Memory.CalculatedSize, true); \
	MAP_LOROM(0xc0, 0xff, 0x0000, 0xffff, Memory.CalculatedSize, true); \
	if (Settings.DSP) \
	{ \
		MAP_DSP_(); \
	} \
	else if (Settings.C4) \
	{ \
		MAP_C4_(); \
	} \
	else if (Settings.OBC1) \
	{ \
		MAP_OBC1(); \
	} \
	else if (Settings.SETA == ST_018) \
	{ \
		MAP_SETARISC(); \
	} \
	MAP_LOROMSRAM(); \
	MAP_WRAM(); \
	map_WriteProtectROM();

#define MAP_HIROMSRAM() \
	MAP_INDEX(0x20, 0x3f, 0x6000, 0x7fff, MAP_HIROM_SRAM, MAP_TYPE_RAM, true); \
	MAP_INDEX(0xa0, 0xbf, 0x6000, 0x7fff, MAP_HIROM_SRAM, MAP_TYPE_RAM, true);

#define MAP_HIROMMAP() \
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_ROM, "Map_HiROMMap"); \
	MAP_SYSTEM(); \
	MAP_HIROM(0x00, 0x3f, 0x8000, 0xffff, Memory.CalculatedSize, true); \
	MAP_HIROM(0x40, 0x7f, 0x0000, 0xffff, Memory.CalculatedSize, true); \
	MAP_HIROM(0x80, 0xbf, 0x8000, 0xffff, Memory.CalculatedSize, true); \
	MAP_HIROM(0xc0, 0xff, 0x0000, 0xffff, Memory.CalculatedSize, true); \
	\
	if (Settings.DSP) \
	{ \
		MAP_DSP_(); \
	} \
	MAP_HIROMSRAM(); \
	MAP_WRAM(); \
	map_WriteProtectROM();

static const char * KartContents (uint8_t ROMType)
{
	static char			str[64];
	static const char	*contents[3] = { "ROM", "ROM+RAM", "ROM+RAM+BAT" };
	char	chip[16];

	if (ROMType == 0 && !Settings.BS)
		return ("ROM");

	if (Settings.BS)
		strcpy(chip, "+BS");
	else
	if (Settings.SuperFX)
		strcpy(chip, "+Super FX");
	else
	if (Settings.SDD1)
		strcpy(chip, "+S-DD1");
	else
	if (Settings.OBC1)
		strcpy(chip, "+OBC1");
	else
	if (Settings.SA1)
   {
		strcpy(chip, "+SA-1");
   }
	else
	if (Settings.SPC7110RTC)
		strcpy(chip, "+SPC7110+RTC");
	else
	if (Settings.SPC7110)
		strcpy(chip, "+SPC7110");
	else
	if (Settings.SRTC)
		strcpy(chip, "+S-RTC");
	else
	if (Settings.C4)
		strcpy(chip, "+C4");
	else
	if (Settings.SETA == ST_010)
		strcpy(chip, "+ST-010");
	else
	if (Settings.SETA == ST_011)
		strcpy(chip, "+ST-011");
	else
	if (Settings.SETA == ST_018)
		strcpy(chip, "+ST-018");
	else
	if (Settings.DSP)
		snprintf(chip, sizeof(chip), "+DSP-%d", Settings.DSP);
	else
		strcpy(chip, "");

	snprintf(str, sizeof(str), "%s%s", contents[(ROMType & 0xf) % 3], chip);

	return (str);
}

static void Map_NoMAD1LoROMMap (void)
{
	printf("Map_NoMAD1LoROMMap\n");
	MAP_SYSTEM();

	MAP_LOROM(0x00, 0x3f, 0x8000, 0xffff, Memory.CalculatedSize, true);
	MAP_LOROM(0x40, 0x7f, 0x0000, 0xffff, Memory.CalculatedSize, true);
	MAP_LOROM(0x80, 0xbf, 0x8000, 0xffff, Memory.CalculatedSize, true);
	MAP_LOROM(0xc0, 0xff, 0x0000, 0xffff, Memory.CalculatedSize, true);

	MAP_INDEX(0x70, 0x7f, 0x0000, 0xffff, MAP_LOROM_SRAM, MAP_TYPE_RAM, true);
	MAP_INDEX(0xf0, 0xff, 0x0000, 0xffff, MAP_LOROM_SRAM, MAP_TYPE_RAM, true);

	MAP_WRAM();

	map_WriteProtectROM();
}

static void Map_ROM24MBSLoROMMap (void)
{
	/* PCB: BSC-1A5M-01, BSC-1A7M-10*/
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_ROM24MBSLoROMMap");
	MAP_SYSTEM();

	MAP_LOROM_OFFSET(0x00, 0x1f, 0x8000, 0xffff, 0x100000, 0, true);
	MAP_LOROM_OFFSET(0x20, 0x3f, 0x8000, 0xffff, 0x100000, 0x100000, true);
	MAP_LOROM_OFFSET(0x80, 0x9f, 0x8000, 0xffff, 0x100000, 0x200000, true);
	MAP_LOROM_OFFSET(0xa0, 0xbf, 0x8000, 0xffff, 0x100000, 0x100000, true);

	MAP_LOROMSRAM();
	MAP_WRAM();

	map_WriteProtectROM();
}

static void Map_SufamiTurboLoROMMap (void)
{
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_SufamiTurboLoROMMap");
	MAP_SYSTEM();

	MAP_LOROM_OFFSET(0x00, 0x1f, 0x8000, 0xffff, 0x40000, 0, true);
	MAP_LOROM_OFFSET(0x20, 0x3f, 0x8000, 0xffff, Multi.cartSizeA, Multi.cartOffsetA, true);
	MAP_LOROM_OFFSET(0x40, 0x5f, 0x8000, 0xffff, Multi.cartSizeB, Multi.cartOffsetB, true);
	MAP_LOROM_OFFSET(0x80, 0x9f, 0x8000, 0xffff, 0x40000, 0, true);
	MAP_LOROM_OFFSET(0xa0, 0xbf, 0x8000, 0xffff, Multi.cartSizeA, Multi.cartOffsetA, true);
	MAP_LOROM_OFFSET(0xc0, 0xdf, 0x8000, 0xffff, Multi.cartSizeB, Multi.cartOffsetB, true);

	if (Multi.sramSizeA)
	{
		MAP_INDEX(0x60, 0x63, 0x8000, 0xffff, MAP_LOROM_SRAM, MAP_TYPE_RAM, true);
		MAP_INDEX(0xe0, 0xe3, 0x8000, 0xffff, MAP_LOROM_SRAM, MAP_TYPE_RAM, true);
	}

	if (Multi.sramSizeB)
	{
		MAP_INDEX(0x70, 0x73, 0x8000, 0xffff, MAP_LOROM_SRAM_B, MAP_TYPE_RAM, true);
		MAP_INDEX(0xf0, 0xf3, 0x8000, 0xffff, MAP_LOROM_SRAM_B, MAP_TYPE_RAM, true);
	}

	MAP_WRAM();

	map_WriteProtectROM();
}

static void Map_SufamiTurboPseudoLoROMMap (void)
{
	/* for combined images */
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_SufamiTurboPseudoLoROMMap");
	MAP_SYSTEM();

	MAP_LOROM_OFFSET(0x00, 0x1f, 0x8000, 0xffff, 0x40000, 0, true);
	MAP_LOROM_OFFSET(0x20, 0x3f, 0x8000, 0xffff, 0x100000, 0x100000, true);
	MAP_LOROM_OFFSET(0x40, 0x5f, 0x8000, 0xffff, 0x100000, 0x200000, true);
	MAP_LOROM_OFFSET(0x80, 0x9f, 0x8000, 0xffff, 0x40000, 0, true);
	MAP_LOROM_OFFSET(0xa0, 0xbf, 0x8000, 0xffff, 0x100000, 0x100000, true);
	MAP_LOROM_OFFSET(0xc0, 0xdf, 0x8000, 0xffff, 0x100000, 0x200000, true);

	/* I don't care :P */
	MAP_SPACE(0x60, 0x63, 0x8000, 0xffff, Memory.SRAM - 0x8000, true);
	MAP_SPACE(0xe0, 0xe3, 0x8000, 0xffff, Memory.SRAM - 0x8000, true);
	MAP_SPACE(0x70, 0x73, 0x8000, 0xffff, Memory.SRAM + 0x4000 - 0x8000, false);//these two seem to duplicate the above ones in
	MAP_SPACE(0xf0, 0xf3, 0x8000, 0xffff, Memory.SRAM + 0x4000 - 0x8000, false);//way that makes the duplicates hard to find.

	MAP_WRAM();

	map_WriteProtectROM();
}

static void Map_SuperFXLoROMMap (void)
{
	int c;
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_SuperFXLoROMMap");
	MAP_SYSTEM();

	/* Replicate the first 2Mbit of the ROM at ROM + 2Mbit such that each 32K
	   block is repeated twice in each 64K block. */
	for ( c = 0; c < 64; c++)
	{
		memcpy(&Memory.ROM[0x200000 + c * 0x10000], &Memory.ROM[c * 0x8000], 0x8000);
		memcpy(&Memory.ROM[0x208000 + c * 0x10000], &Memory.ROM[c * 0x8000], 0x8000);
	}

	MAP_LOROM(0x00, 0x3f, 0x8000, 0xffff, Memory.CalculatedSize, true);
	MAP_LOROM(0x80, 0xbf, 0x8000, 0xffff, Memory.CalculatedSize, true);

	MAP_HIROM_OFFSET(0x40, 0x7f, 0x0000, 0xffff, Memory.CalculatedSize, 0, true);
	MAP_HIROM_OFFSET(0xc0, 0xff, 0x0000, 0xffff, Memory.CalculatedSize, 0, true);

	MAP_SPACE(0x00, 0x3f, 0x6000, 0x7fff, Memory.SRAM - 0x6000, true);
	MAP_SPACE(0x80, 0xbf, 0x6000, 0x7fff, Memory.SRAM - 0x6000, true);
	MAP_SPACE(0x70, 0x70, 0x0000, 0xffff, Memory.SRAM, true);
	MAP_SPACE(0x71, 0x71, 0x0000, 0xffff, Memory.SRAM + 0x10000, true);

	MAP_WRAM();

	map_WriteProtectROM();
}

static void Map_SetaDSPLoROMMap (void)
{
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_SetaDSPLoROMMap");
	MAP_SYSTEM();

	MAP_LOROM(0x00, 0x3f, 0x8000, 0xffff, Memory.CalculatedSize, true);
	MAP_LOROM(0x40, 0x7f, 0x8000, 0xffff, Memory.CalculatedSize, true);
	MAP_LOROM(0x80, 0xbf, 0x8000, 0xffff, Memory.CalculatedSize, true);
	MAP_LOROM(0xc0, 0xff, 0x8000, 0xffff, Memory.CalculatedSize, true);

	/* where does the SETA chip access, anyway? */
	/* please confirm this? */
	MAP_INDEX(0x68, 0x6f, 0x0000, 0x7fff, MAP_SETA_DSP, MAP_TYPE_RAM, true);
	/* and this! */
	MAP_INDEX(0x60, 0x67, 0x0000, 0x3fff, MAP_SETA_DSP, MAP_TYPE_I_O, true);
	/* ST-0010: */
	/* MAP_INDEX(0x68, 0x6f, 0x0000, 0x0fff, MAP_SETA_DSP, ?); */
	
	MAP_LOROMSRAM();
	MAP_WRAM();

	map_WriteProtectROM();
}

static void Map_SDD1LoROMMap (void)
{
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_SDD1LoROMMap");
	MAP_SYSTEM();

	MAP_LOROM(0x00, 0x3f, 0x8000, 0xffff, Memory.CalculatedSize, true);
	MAP_LOROM(0x80, 0xbf, 0x8000, 0xffff, Memory.CalculatedSize, true);

	MAP_HIROM_OFFSET(0x40, 0x7f, 0x0000, 0xffff, Memory.CalculatedSize, 0, true);
	MAP_HIROM_OFFSET(0xc0, 0xff, 0x0000, 0xffff, Memory.CalculatedSize, 0, true); /* will be overwritten dynamically*/

	MAP_INDEX(0x70, 0x7f, 0x0000, 0x7fff, MAP_LOROM_SRAM, MAP_TYPE_RAM, true);

	MAP_WRAM();

	map_WriteProtectROM();
}

static void Map_SA1LoROMMap (void)
{
	int c;
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_SA1LoROMMap");
	MAP_SYSTEM();

	MAP_LOROM(0x00, 0x3f, 0x8000, 0xffff, Memory.CalculatedSize, true);
	MAP_LOROM(0x80, 0xbf, 0x8000, 0xffff, Memory.CalculatedSize, true);

	MAP_HIROM_OFFSET(0xc0, 0xff, 0x0000, 0xffff, Memory.CalculatedSize, 0, true);

	MAP_SPACE(0x00, 0x3f, 0x3000, 0x3fff, Memory.FillRAM, true);
	MAP_SPACE(0x80, 0xbf, 0x3000, 0x3fff, Memory.FillRAM, true);
	MAP_INDEX(0x00, 0x3f, 0x6000, 0x7fff, MAP_BWRAM, MAP_TYPE_I_O, true);
	MAP_INDEX(0x80, 0xbf, 0x6000, 0x7fff, MAP_BWRAM, MAP_TYPE_I_O, true);

	for ( c = 0x40; c < 0x80; c++)
	{
		MAP_SPACE(c, c, 0x0000, 0xffff, Memory.SRAM + (c & 1) * 0x10000, false);
	}

	MAP_LIBRETRO_RAW(0, Memory.SRAM, 0, 0x400000, 0xC00000, 0xFE0000, 0x20000);

	MAP_WRAM();

	map_WriteProtectROM();

	/* Now copy the map and correct it for the SA1 CPU. */
	memcpy((void *) SA1.Map, (void *) Memory.Map, sizeof(Memory.Map));
	memcpy((void *) SA1.WriteMap, (void *) Memory.WriteMap, sizeof(Memory.WriteMap));

	/* SA-1 Banks 00->3f and 80->bf */
	for ( c = 0x000; c < 0x400; c += 0x10)
	{
		SA1.Map[c + 0] = SA1.Map[c + 0x800] = Memory.FillRAM + 0x3000;
		SA1.Map[c + 1] = SA1.Map[c + 0x801] = (uint8_t *) MAP_NONE;
		SA1.WriteMap[c + 0] = SA1.WriteMap[c + 0x800] = Memory.FillRAM + 0x3000;
		SA1.WriteMap[c + 1] = SA1.WriteMap[c + 0x801] = (uint8_t *) MAP_NONE;
	}

	/* SA-1 Banks 60->6f */
	for ( c = 0x600; c < 0x700; c++)
		SA1.Map[c] = SA1.WriteMap[c] = (uint8_t *) MAP_BWRAM_BITMAP;

	Memory.BWRAM = Memory.SRAM;
}

static void Map_ExtendedHiROMMap (void)
{
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_ExtendedHiROMMap");
	MAP_SYSTEM();

	MAP_HIROM_OFFSET(0x00, 0x3f, 0x8000, 0xffff, Memory.CalculatedSize - 0x400000, 0x400000, true);
	MAP_HIROM_OFFSET(0x40, 0x7f, 0x0000, 0xffff, Memory.CalculatedSize - 0x400000, 0x400000, true);
	MAP_HIROM_OFFSET(0x80, 0xbf, 0x8000, 0xffff, 0x400000, 0, true);
	MAP_HIROM_OFFSET(0xc0, 0xff, 0x0000, 0xffff, 0x400000, 0, true);

	MAP_HIROMSRAM();
	MAP_WRAM();

	map_WriteProtectROM();
}

static void Map_SameGameHiROMMap (void)
{
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_SameGameHiROMMap");
	MAP_SYSTEM();

	MAP_HIROM_OFFSET(0x00, 0x1f, 0x8000, 0xffff, Multi.cartSizeA, Multi.cartOffsetA, true);
	MAP_HIROM_OFFSET(0x20, 0x3f, 0x8000, 0xffff, Multi.cartSizeB, Multi.cartOffsetB, true);
	MAP_HIROM_OFFSET(0x40, 0x5f, 0x0000, 0xffff, Multi.cartSizeA, Multi.cartOffsetA, true);
	MAP_HIROM_OFFSET(0x60, 0x7f, 0x0000, 0xffff, Multi.cartSizeB, Multi.cartOffsetB, true);
	MAP_HIROM_OFFSET(0x80, 0x9f, 0x8000, 0xffff, Multi.cartSizeA, Multi.cartOffsetA, true);
	MAP_HIROM_OFFSET(0xa0, 0xbf, 0x8000, 0xffff, Multi.cartSizeB, Multi.cartOffsetB, true);
	MAP_HIROM_OFFSET(0xc0, 0xdf, 0x0000, 0xffff, Multi.cartSizeA, Multi.cartOffsetA, true);
	MAP_HIROM_OFFSET(0xe0, 0xff, 0x0000, 0xffff, Multi.cartSizeB, Multi.cartOffsetB, true);

	MAP_HIROMSRAM();
	MAP_WRAM();

	map_WriteProtectROM();
}

static void Map_SPC7110HiROMMap (void)
{
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "Map_SPC7110HiROMMap");
	MAP_SYSTEM();

	MAP_INDEX(0x00, 0x00, 0x6000, 0x7fff, MAP_HIROM_SRAM, MAP_TYPE_RAM, true);
	MAP_HIROM(0x00, 0x0f, 0x8000, 0xffff, Memory.CalculatedSize, true);	
	MAP_INDEX(0x30, 0x30, 0x6000, 0x7fff, MAP_HIROM_SRAM, MAP_TYPE_RAM, true);
	MAP_INDEX(0x50, 0x50, 0x0000, 0xffff, MAP_SPC7110_DRAM, MAP_TYPE_ROM, true);
	MAP_HIROM(0x80, 0x8f, 0x8000, 0xffff, Memory.CalculatedSize, true);
	MAP_HIROM_OFFSET(0xc0, 0xcf, 0x0000, 0xffff, Memory.CalculatedSize, 0, true);
	MAP_INDEX(0xd0, 0xff, 0x0000, 0xffff, MAP_SPC7110_ROM,  MAP_TYPE_ROM, true);

	MAP_WRAM();

	map_WriteProtectROM();
}

static const char * StaticRAMSize (void)
{
	static char	str[20];

	if (Memory.SRAMSize > 16)
		strcpy(str, "Corrupt");
	else
		snprintf(str, sizeof(str), "%dKbits", 8 * (Memory.SRAMMask + 1) / 1024);

	return (str);
}

static const char * Size (void)
{
	static char	str[20];

	if (Multi.cartType == 4)
		strcpy(str, "N/A");
	else
	if (Memory.ROMSize < 7 || Memory.ROMSize - 7 > 23)
		strcpy(str, "Corrupt");
	else
		snprintf(str, sizeof(str), "%dMbits", 1 << (Memory.ROMSize - 7));

	return (str);
}

static uint8_t InitROM (void)
{
	int p;
	uint8_t bs, isChecksumOK;
	uint8_t *RomHeader;
	uint16_t sum;
	uint32_t identifier;
	char displayName[ROM_NAME_LEN], String[513];

	Settings.SuperFX = FALSE;
	Settings.DSP = 0;
	Settings.SA1 = FALSE;
	Settings.C4 = FALSE;
	Settings.SDD1 = FALSE;
	Settings.SPC7110 = FALSE;
	Settings.SPC7110RTC = FALSE;
	Settings.OBC1 = FALSE;
	Settings.SETA = 0;
	Settings.SRTC = FALSE;
	Settings.BS = FALSE;
	SuperFX.nRomBanks = Memory.CalculatedSize >> 15;
   
   Settings.SupportHiRes = TRUE;

	/* Parse ROM header and read ROM information */

	Memory.CompanyId = -1;
	memset(Memory.ROMId, 0, 5);

	RomHeader = Memory.ROM + 0x7FB0;
	if (Memory.ExtendedFormat == BIGFIRST)
		RomHeader += 0x400000;
	if (Memory.HiROM)
		RomHeader += 0x8000;

	S9xInitBSX(); /* Set BS header before parsing */

	/* Parse SNES Header */

	bs = Settings.BS & !Settings.BSXItself;

	strncpy(Memory.ROMName, (char *) &RomHeader[0x10], ROM_NAME_LEN - 1);
	if (bs)
		memset(Memory.ROMName + 16, 0x20, ROM_NAME_LEN - 17);

	if (bs)
	{
		if (!(((RomHeader[0x29] & 0x20) && Memory.CalculatedSize <  0x100000) ||
			 (!(RomHeader[0x29] & 0x20) && Memory.CalculatedSize == 0x100000)))
			S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, "BS: Size mismatch");

		/* FIXME*/
		p = 0;
		while ((1 << p) < (int) Memory.CalculatedSize)
			p++;
		Memory.ROMSize = p - 10;
	}
	else
		Memory.ROMSize = RomHeader[0x27];

	Memory.SRAMSize  = bs ? 5 /* BS-X */    : RomHeader[0x28];
	Memory.ROMSpeed  = bs ? RomHeader[0x28] : RomHeader[0x25];
	Memory.ROMType   = bs ? 0xE5 /* BS-X */ : RomHeader[0x26];
	Memory.ROMRegion = bs ? 0               : RomHeader[0x29];

	Memory.ROMChecksum           = RomHeader[0x2E] + (RomHeader[0x2F] << 8);
	Memory.ROMComplementChecksum = RomHeader[0x2C] + (RomHeader[0x2D] << 8);

	memcpy(Memory.ROMId, &RomHeader[0x02], 4);

	if (RomHeader[0x2A] != 0x33)
		Memory.CompanyId = ((RomHeader[0x2A] >> 4) & 0x0F) * 36 + (RomHeader[0x2A] & 0x0F);
	else
	if (isalnum(RomHeader[0x00]) && isalnum(RomHeader[0x01]))
	{
		int	l, r, l2, r2;
		l = toupper(RomHeader[0x00]);
		r = toupper(RomHeader[0x01]);
		l2 = (l > '9') ? l - '7' : l - '0';
		r2 = (r > '9') ? r - '7' : r - '0';
		Memory.CompanyId = l2 * 36 + r2;
	}

	if (Memory.SRAMSize > 16 || Memory.ROMSize < 7 || Memory.ROMSize - 7 > 23)
	{
		S9xMessage(S9X_MSG_ERROR, S9X_CATEGORY_ROM,
				"ROM is corrupt or invalid");
		return (FALSE);
	}

	/* End of Parse SNES Header */

	/* Detect and initialize chips
	   detection codes are compatible with NSRT */

	/* DSP1/2/3/4 */
	if (Memory.ROMType == 0x03)
	{
		if (Memory.ROMSpeed == 0x30)
			Settings.DSP = 4; /* DSP4 */
		else
			Settings.DSP = 1; /* DSP1 */
	}
	else if (Memory.ROMType == 0x05)
	{
		if (Memory.ROMSpeed == 0x20)
			Settings.DSP = 2; /* DSP2 */
		else
		if (Memory.ROMSpeed == 0x30 && RomHeader[0x2a] == 0xb2)
			Settings.DSP = 3; /* DSP3 */
		else
			Settings.DSP = 1; /* DSP1 */
	}

	switch (Settings.DSP)
	{
		case 1:	/* DSP1 */
			if (Memory.HiROM)
			{
				DSP0.boundary = 0x7000;
				DSP0.maptype = M_DSP1_HIROM;
			}
			else
			if (Memory.CalculatedSize > 0x100000)
			{
				DSP0.boundary = 0x4000;
				DSP0.maptype = M_DSP1_LOROM_L;
			}
			else
			{
				DSP0.boundary = 0xc000;
				DSP0.maptype = M_DSP1_LOROM_S;
			}

			SetDSP = &DSP1SetByte;
			GetDSP = &DSP1GetByte;
			break;

		case 2: /* DSP2 */
			DSP0.boundary = 0x10000;
			DSP0.maptype = M_DSP2_LOROM;
			SetDSP = &DSP2SetByte;
			GetDSP = &DSP2GetByte;
			break;

		case 3: /* DSP3 */
			DSP0.boundary = 0xc000;
			DSP0.maptype = M_DSP3_LOROM;
			SetDSP = &DSP3SetByte;
			GetDSP = &DSP3GetByte;
			break;

		case 4: /* DSP4 */
			DSP0.boundary = 0xc000;
			DSP0.maptype = M_DSP4_LOROM;
			SetDSP = &DSP4SetByte;
			GetDSP = &DSP4GetByte;
			break;

		default:
			SetDSP = NULL;
			GetDSP = NULL;
			break;
	}

	identifier = ((Memory.ROMType & 0xff) << 8) + (Memory.ROMSpeed & 0xff);

	switch (identifier)
	{
	    /* SRTC*/
		case 0x5535:
			Settings.SRTC = TRUE;
			S9xInitSRTC();
			break;

		/* SPC7110*/
		case 0xF93A:
			Settings.SPC7110RTC = TRUE;
			Settings.SPC7110    = TRUE;
			S9xInitSPC7110();
			break;
		case 0xF53A:
			Settings.SPC7110 = TRUE;
			S9xInitSPC7110();
			break;

		/* OBC1*/
		case 0x2530:
			Settings.OBC1 = TRUE;
			break;

		/* SA1*/
		case 0x3423:
		case 0x3523:
			Settings.SA1 = TRUE;
			break;

		/* SuperFX*/
		case 0x1320:
		case 0x1420:
		case 0x1520:
		case 0x1A20:
			Settings.SuperFX = TRUE;
			memset((uint8_t *) &GSU, 0, sizeof(struct FxRegs_s));
			if (Memory.ROM[0x7FDA] == 0x33)
				Memory.SRAMSize = Memory.ROM[0x7FBD];
			else
				Memory.SRAMSize = 5;
			break;

		/* SDD1*/
		case 0x4332:
		case 0x4532:
			Settings.SDD1 = TRUE;
			break;

		/* ST018*/
		case 0xF530:
			Settings.SETA = ST_018;
			SetSETA = NULL;
			GetSETA = NULL;
			Memory.SRAMSize = 2;
			SNESGameFixes.SRAMInitialValue = 0x00;
			break;

		/* ST010/011*/
		case 0xF630:
			if (Memory.ROM[0x7FD7] == 0x09)
			{
				Settings.SETA = ST_011;
				SetSETA = &S9xSetST011;
				GetSETA = &S9xGetST011;
			}
			else
			{
				Settings.SETA = ST_010;
				SetSETA = &S9xSetST010;
				GetSETA = &S9xGetST010;
			}

			Memory.SRAMSize = 2;
			SNESGameFixes.SRAMInitialValue = 0x00;
			break;

		/* C4*/
		case 0xF320:
			Settings.C4 = TRUE;
			break;
	}

	/* Map memory and calculate checksum */

	MAP_INITIALIZE();
	Memory.CalculatedChecksum = 0;

	/* SRAM size */
	Memory.SRAMMask = Memory.SRAMSize ? ((1 << (Memory.SRAMSize + 3)) * 128) - 1 : 0;

	if (Memory.HiROM)
	{
		if (Settings.BS)
			/* Do nothing */;
		else
			if (Settings.SPC7110)
				Map_SPC7110HiROMMap();
			else
				if (Memory.ExtendedFormat != NOPE)
					Map_ExtendedHiROMMap();
				else
					if (Multi.cartType == 3)
						Map_SameGameHiROMMap();
					else
					{
						MAP_HIROMMAP();
					}
	}
	else
	{
		if (Settings.BS)
			/* Do nothing */;
		else if (Settings.SETA && Settings.SETA != ST_018)
			Map_SetaDSPLoROMMap();
		else if (Settings.SuperFX)
			Map_SuperFXLoROMMap();
		else if (Settings.SA1)
			Map_SA1LoROMMap();
		else if (Settings.SDD1)
			Map_SDD1LoROMMap();
		else if (Memory.ExtendedFormat != NOPE)
		{
			MAP_JUMBOLOROMMAP();
		}
		else if (strncmp(Memory.ROMName, "WANDERERS FROM YS", 17) == 0)
			Map_NoMAD1LoROMMap();
		else if (strncmp(Memory.ROMName, "SOUND NOVEL-TCOOL", 17) == 0 ||
				strncmp(Memory.ROMName, "DERBY STALLION 96", 17) == 0)
			Map_ROM24MBSLoROMMap();
		else if (strncmp(Memory.ROMName, "THOROUGHBRED BREEDER3", 21) == 0 ||
				strncmp(Memory.ROMName, "RPG-TCOOL 2", 11) == 0)
		{
			MAP_SRAM512KLOROMMAP();
		}
		else if (strncmp(Memory.ROMName, "ADD-ON BASE CASSETE", 19) == 0)
		{
			if (Multi.cartType == 4)
			{
				Memory.SRAMSize = Multi.sramSizeA;
				Map_SufamiTurboLoROMMap();
			}
			else
			{
				Memory.SRAMSize = 5;
				Map_SufamiTurboPseudoLoROMMap();
			}
		}
		else
		{
			MAP_LOROMMAP();
		}
	}

	/* from NSRT */
	sum = 0;

	if (Settings.BS && !Settings.BSXItself)
		sum = checksum_calc_sum(Memory.ROM, Memory.CalculatedSize) - checksum_calc_sum(Memory.ROM + (Memory.HiROM ? 0xffb0 : 0x7fb0), 48);
	else
	if (Settings.SPC7110)
	{
		sum = checksum_calc_sum(Memory.ROM, Memory.CalculatedSize);
		if (Memory.CalculatedSize == 0x300000)
			sum += sum;
	}
	else
	{
		if (Memory.CalculatedSize & 0x7fff)
			sum = checksum_calc_sum(Memory.ROM, Memory.CalculatedSize);
		else
		{
			uint32_t	length = Memory.CalculatedSize;
			sum = checksum_mirror_sum(Memory.ROM, &length, 0x800000);
		}
	}

	Memory.CalculatedChecksum = sum;

	isChecksumOK = (Memory.ROMChecksum + Memory.ROMComplementChecksum == 0xffff) &
						 (Memory.ROMChecksum == Memory.CalculatedChecksum);

	/* Build more ROM information */

	/* CRC32 */
	if (!Settings.BS || Settings.BSXItself) /* Not BS Dump */
		Memory.ROMCRC32 = caCRC32(Memory.ROM, Memory.CalculatedSize, 0xffffffff);
	else /* Convert to correct format before scan */
	{
		int offset = Memory.HiROM ? 0xffc0 : 0x7fc0;
		/* Backup */
		uint8_t BSMagic0 = Memory.ROM[offset + 22];
		uint8_t BSMagic1 = Memory.ROM[offset + 23];

		/* uCONSRT standard */
		Memory.ROM[offset + 22] = 0x42;
		Memory.ROM[offset + 23] = 0x00;

		/* Calc */
		Memory.ROMCRC32 = caCRC32(Memory.ROM, Memory.CalculatedSize, 0xffffffff);

		/* Convert back */
		Memory.ROM[offset + 22] = BSMagic0;
		Memory.ROM[offset + 23] = BSMagic1;
	}

	/* NTSC/PAL */
	if (Settings.ForceNTSC)
		Settings.PAL = FALSE;
	else if (Settings.ForcePAL)
		Settings.PAL = TRUE;
	else if (!Settings.BS && (Memory.ROMRegion >= 2) && (Memory.ROMRegion <= 12))
		Settings.PAL = TRUE;
	else
		Settings.PAL = FALSE;

	/* truncate cart name */
	Memory.ROMName[ROM_NAME_LEN - 1] = 0;
	if (strlen(Memory.ROMName))
	{
		char *p = Memory.ROMName + strlen(Memory.ROMName);
		if (p > Memory.ROMName + 21 && Memory.ROMName[20] == ' ')
			p = Memory.ROMName + 21;
		while (p > Memory.ROMName && *(p - 1) == ' ')
			p--;
		*p = 0;
	}

	/* checksum */
	if (!isChecksumOK || ((uint32_t) Memory.CalculatedSize > (uint32_t) (((1 << (Memory.ROMSize - 7)) * 128) * 1024)))
	{
	}

	if (Multi.cartType == 4)
	{
	}

	/* Initialize emulation */

	Timings.H_Max_Master = SNES_CYCLES_PER_SCANLINE;
	Timings.H_Max        = Timings.H_Max_Master;
	Timings.HBlankStart  = SNES_HBLANK_START_HC;
	Timings.HBlankEnd    = SNES_HBLANK_END_HC;
	Timings.HDMAInit     = SNES_HDMA_INIT_HC;
	Timings.HDMAStart    = SNES_HDMA_START_HC;
	Timings.RenderPos    = SNES_RENDER_START_HC;
	Timings.V_Max_Master = Settings.PAL ? SNES_MAX_PAL_VCOUNTER : SNES_MAX_NTSC_VCOUNTER;
	Timings.V_Max        = Timings.V_Max_Master;

	/* From byuu: The total delay time for both the initial (H)DMA sync (to the DMA clock),
	   and the end (H)DMA sync (back to the last CPU cycle's mcycle rate (6, 8, or 12)) 
	   always takes between 12-24 mcycles.

	   Possible delays: { 12, 14, 16, 18, 20, 22, 24 }
	   XXX: Snes9x can't emulate this timing :( so let's use the average value... */

	Timings.DMACPUSync   = 18;

	/* If the CPU is halted (i.e. for DMA) while /NMI goes low, the NMI will trigger
	   after the DMA completes (even if /NMI goes high again before the DMA
	   completes). In this case, there is a 24-30 cycle delay between the end of DMA
	   and the NMI handler, time enough for an instruction or two.
	   
	   Examples: Wild Guns, Mighty Morphin Power Rangers - The Fighting Edition */

	Timings.NMIDMADelay  = 24;
	Timings.IRQPendCount = 0;

	/* Hack games */

	Settings.BlockInvalidVRAMAccess = Settings.BlockInvalidVRAMAccessMaster;

	/* Warnings */

	/* Reject strange hacked games */
	if ((Memory.ROMCRC32 == 0x6810aa95) ||
		(Memory.ROMCRC32 == 0x340f23e5) ||
		(Memory.ROMCRC32 == 0x77fd806a) ||
		(MATCH_NN("HIGHWAY BATTLE 2")) ||
		(MATCH_NA("FX SKIING NINTENDO 96") && (Memory.ROM[0x7fda] == 0)) ||
		(MATCH_NN("HONKAKUHA IGO GOSEI")   && (Memory.ROM[0xffd5] != 0x31)))
	{
	}

	/* APU timing hacks */

	Timings.APUSpeedup = 0;
	Timings.APUAllowTimeOverflow = FALSE;

	if (!Settings.DisableGameSpecificHacks)
	{
		if (MATCH_ID("AVCJ"))                                      /* Rendering Ranger R2*/
			Timings.APUSpeedup = 4;

		if (MATCH_NA("GAIA GENSOUKI 1 JPN")                     || /* Gaia Gensouki*/
			MATCH_ID("JG  ")                                    || /* Illusion of Gaia*/
			MATCH_ID("CQ  ")                                    || /* Stunt Race FX*/
			MATCH_NA("SOULBLADER - 1")                          || /* Soul Blader*/
			MATCH_NA("SOULBLAZER - 1 USA")                      || /* Soul Blazer*/
			MATCH_NA("SLAP STICK 1 JPN")                        || /* Slap Stick*/
			MATCH_ID("E9 ")                                     || /* Robotrek*/
			MATCH_NN("ACTRAISER")                               || /* Actraiser*/
			MATCH_NN("ActRaiser-2")                             || /* Actraiser 2*/
			MATCH_ID("AQT")                                     || /* Tenchi Souzou, Terranigma*/
			MATCH_ID("ATV")                                     || /* Tales of Phantasia*/
			MATCH_ID("ARF")                                     || /* Star Ocean*/
			MATCH_ID("APR")                                     || /* Zen-Nippon Pro Wrestling 2 - 3-4 Budoukan*/
			MATCH_ID("A4B")                                     || /* Super Bomberman 4*/
			MATCH_ID("Y7 ")                                     || /* U.F.O. Kamen Yakisoban - Present Ban*/
			MATCH_ID("Y9 ")                                     || /* U.F.O. Kamen Yakisoban - Shihan Ban*/
			MATCH_ID("APB")                                     || /* Super Bomberman - Panic Bomber W*/
			MATCH_NA("DARK KINGDOM")                            || /* Dark Kingdom*/
			MATCH_NA("ZAN3 SFC")                                || /* Zan III Spirits*/
			MATCH_NA("HIOUDEN")                                 || /* Hiouden - Mamono-tachi Tono Chikai*/
			MATCH_NA("\xC3\xDD\xBC\xC9\xB3\xC0")                || /* Tenshi no Uta*/
			MATCH_NA("FORTUNE QUEST")                           || /* Fortune Quest - Dice wo Korogase*/
			MATCH_NA("FISHING TO BASSING")                      || /* Shimono Masaki no Fishing To Bassing*/
			MATCH_NA("OHMONO BLACKBASS")                        || /* Oomono Black Bass Fishing - Jinzouko Hen*/
			MATCH_NA("MASTERS")                                 || /* Harukanaru Augusta 2 - Masters*/
			MATCH_NA("SFC \xB6\xD2\xDD\xD7\xB2\xC0\xDE\xB0")    || /* Kamen Rider*/
			MATCH_NA("ZENKI TENCHIMEIDOU")					    || /* Kishin Douji Zenki - Tenchi Meidou*/
			MATCH_NN("TokyoDome '95Battle 7")                   || /* Shin Nippon Pro Wrestling Kounin '95 - Tokyo Dome Battle 7*/
			MATCH_NN("SWORD WORLD SFC")                         || /* Sword World SFC/2*/
			MATCH_NN("LETs PACHINKO(")                          || /* BS Lets Pachinko Nante Gindama 1/2/3/4*/
			MATCH_NN("THE FISHING MASTER")                      || /* Mark Davis The Fishing Master*/
			MATCH_NN("Parlor")                                  || /* Parlor mini/2/3/4/5/6/7, Parlor Parlor!/2/3/4/5*/
			MATCH_NA("HEIWA Parlor!Mini8")                      || /* Parlor mini 8*/
			MATCH_NN("SANKYO Fever! \xCC\xA8\xB0\xCA\xDE\xB0!"))   /* SANKYO Fever! Fever!*/
			Timings.APUSpeedup = 1;

		if (MATCH_NA ("EARTHWORM JIM 2")			|| /* Earthworm Jim 2*/
			MATCH_NA ("NBA Hangtime")			|| /* NBA Hang Time*/
			MATCH_NA ("MSPACMAN")				|| /* Ms Pacman*/
			MATCH_NA ("THE MASK")				|| /* The Mask*/
			MATCH_NA ("PRIMAL RAGE")			|| /* Primal Rage*/
			MATCH_NA ("DOOM TROOPERS")			|| /* Doom Troopers*/
			MATCH_NA ("PORKY PIGS HAUNTED")			|| /* Porky Pig's Haunted Holiday*/
			MATCH_NA ("Big Sky Trooper")			|| /* Big Sky Trooper*/
			MATCH_NA ("A35"))				/* Mechwarrior 3050/Battle Tech 3050*/
			Timings.APUAllowTimeOverflow = TRUE;

		/* SWAP MOSAIC HI-RES WITH HI-RES 

		   We don't want Seiken Densetsu 3/Romancing Saga 3 unbearably 
		   slow in hi-res mosaic mode - no difference with these games 
		   between normal hi-res renderer and mosaic version - only double the FPS */

		PPU.DisableMosaicHack = !(
				MATCH_NN("SeikenDensetsu3") ||
				MATCH_NA("SeikenDensetsu3Sample1") ||	/* Seiken Densetsu 3 */
				MATCH_NA("ROMANCING SAGA3"));		/* Romancing Saga 3 */

		/* Speedup hack for Star Fox 2/Vortex */
		PPU.SFXSpeedupHack = (
            MATCH_NA("VORTEX") ||			/* Vortex */
            MATCH_NA("Super Street Fighter21") ||	/* Super Street Fighter II */
            MATCH_NA("STAR FOX") ||			/* Star Fox (US/JP)*/
            MATCH_NA("STAR WING") ||			/* Star Wing (EU)*/
            MATCH_NA("STAR FOX 2") ||			/* Star Fox 2 (Beta) */
            MATCH_NA("STARFOX2"));			/* Star Fox 2 (SNES Classic) */


	/* FORCIBLY DISABLE HIGH-RES */

            if (		
                  MATCH_NA("DONKEY KONG COUNTRY")		/* Donkey Kong Country */
                  //|| MATCH_ID("ADNE")			/* Donkey Kong Country 2 (US) */
                  || MATCH_ID("AD8")			/* Doom */
               )
               Settings.SupportHiRes = FALSE;


		/* DON'T RENDER SUBSCREEN */

		/* Don't render subscreen for games that don't switch to high-res mode 
		   in-game or don't use hi-res at all - and yet still need to do stuff 
		   with the subscreen FPS improvement ranges from 7/10fps to 20/25fps 
		   depending on the game ( +25/30fps in some occassions - benchmarked 
		   on PC).

			TODO: This glitches with rewind enabled (RetroArch), force-disable it
			when rewind is active */
		PPU.RenderSub = !(
				MATCH_NA("Super Metroid") 	/* Super Metroid*/
				|| MATCH_NA("DONKEY KONG COUNTRY") /* Donkey Kong Country 1 */
				|| MATCH_NA("AREA88")		/* Area 88 */
				|| MATCH_NA("U.N.Squadron")	/* UN Squadron */
				|| MATCH_NA("Perfect Eleven")	/* Perfect Eleven - ISS1 */
				//|| MATCH_ID("ADNE")			/* Donkey Kong Country 2 (US) */
				|| MATCH_ID("AWJ")		/* ISS Deluxe / World Soccer 2 Fighting Eleven */
				|| MATCH_NA("JIM POWER THE LOST DIO") /* Jim Power The Lost Dimension */
				|| MATCH_ID("A3C")		/* Donkey Kong Country 3*/
				|| MATCH_NA("Secret of MANA") /* Secret of Mana */
				|| MATCH_NA("Street Fighter2 Turbo1") /* Street Fighter II Turbo */
				|| MATCH_NA("Super Street Fighter21") /* Super Street Fighter II (US) */
				|| MATCH_ID("A2J")		/* Jurassic Park 2 */
				|| MATCH_ID("AJL")		/* Justice League */
				|| MATCH_NA("JOE AND MAC")	/* Joe and Mac */
				|| MATCH_NA("JOE & MAC 2")	/* Joe & Mac 2 */
				|| MATCH_ID("J3")		/* Joe & Mac 3 EU) */
			|| MATCH_NA("JYUTEI SENKI")	/* Jyutei Senki */
			|| MATCH_ID("CQ  ")		/* Stunt Race FX */
#if defined(_XBOX1) || defined(GEKKO)
			|| MATCH_ID("AD8")		/* Doom */
#endif
			|| MATCH_NA("T.M.H.T.4")	/* Teenage Mutant Hero Turtles 4 - Turtles In Time (E) */
			|| MATCH_NA("T.M.N.T.")		/* Teenage Mutant Ninja Turtles 4 - Turtles In Time (J) */
			|| MATCH_NA("T.M.N.T.4")	/* Teenage Mutant Ninja Turtles 4 - Turtles In Time (AU) */
			|| MATCH_NA("T.M.N.T. 4")	/* Teenage Mutant Ninja Turtles 4 - Turtles In Time (US) */
			|| MATCH_NA("T.M.N.T. MW")	/* Teenage Mutant Ninja Turtles - Mutant Warriors (J) */
			|| MATCH_NA("T.M.H.T TF")	/* Teenage Mutant Hero Turtles - Tour Fighters (EU) */
			|| MATCH_NA("T.M.N.T. TF")	/* Teenage Mutant Ninja Turtles - Tour Fighters (AU) */
			|| MATCH_NA("T.M.N.T.5")	/* Teenage Mutant Ninja Turtles - Tour Fighters (U) */
			|| MATCH_NA("SUPER CASTLEVANIA 4") /* Super Castlevania IV */
			|| MATCH_NA("AKUMAJO DRACULA") /* Super Castlevania IV */
			|| MATCH_ID("ATVE")		/* Tales of Phantasia (EN) (DeJap)*/
			|| MATCH_NA("MEGAMAN X")		/* Mega Man X (US) */
			|| MATCH_ID("A7R")		/* Rockman / Mega Man 7 */
			|| MATCH_NA("F-ZERO")		/* Rockman / Mega Man 7 */
			|| MATCH_NA("EQUINOX")		/* Equinox */
			|| MATCH_NA("SUPER MARIO KART") /* Super Mario Kart */
			|| MATCH_ID("AR3")		/* Mega Man X3 (JP/US) */
			|| MATCH_NA("SECRET OF EVERMORE") 	/* Secret of Evermore*/
			|| MATCH_ID("AKL")		/* Killer Instinct*/
			|| MATCH_NA("NBA JAM")		/* NBA Jam */
			|| MATCH_NA("PILOTWINGS")	/* Pilotwings */
			|| MATCH_NA("FINAL FIGHT")	/* Final Fight */
			|| MATCH_NA("FINAL FANTASY 6")	/* Final Fantasy VI (JP)*/
			|| MATCH_NA("FINAL FANTASY 3")	/* Final Fantasy III (US)*/
			|| MATCH_NA("ILLUSION OF GAIA USA") /* Illusion of Gaia (US)*/
			|| MATCH_NA("GAIA GENSOUKI 1 JPN")	/* Gaia Gensouki (JPN)*/
			|| MATCH_ID("AEJ") 		/* Earthworm Jim*/
			|| MATCH_ID("YI")		/* Yoshi's Island*/
			|| MATCH_NA("SUPER MARIOWORLD")	/* Super Mario World*/
			|| MATCH_NA("THE LEGEND OF ZELDA")	/* Zelda 3*/
			|| MATCH_NA("LA LEGENDE DE ZELDA")	/* Zelda 3 (FR)*/
			|| MATCH_NA("ZELDANODENSETSU")		/* Zelda 3 (JPN)*/
			|| MATCH_NA("SPARKSTER")		/* Sparkster*/
			|| MATCH_NA("SIMCITY")		/* Sim City */
			|| MATCH_NA("T2 ARCADE")	/* T2 Arcade */
			|| MATCH_NA("PLOK")		/* Plok!*/
			|| MATCH_NA("ALADDIN")		/* Aladdin */
			|| MATCH_NA("ActRaiser-2 JPN")	/* ActRaiser 2 (JPN) */
			|| MATCH_NA("ActRaiser-2 USA")	/* ActRaiser 2 (US) */
			|| MATCH_ID("A8")		/* ActRaiser 2 (EU) */
			|| MATCH_ID("APT")		/* Pac In Time */
			|| MATCH_ID("AXG")		/*NBA Hangtime */
			|| MATCH_NA("THE CHAOS ENGINE")	/* Chaos Engine */
			|| MATCH_ID("AWD")		/* Weaponlord */
			|| MATCH_NA("TALES OF PHANTASIA")       /* Tales of Phantasia */
			|| MATCH_NA("GS MIKAMI")		/* GS Mikami - Joreishi wa Nice Body*/
			|| MATCH_ID("ATQ")		/* Theme Park */
			|| MATCH_NA("TOTAL CARNAGE")	/* Total Carnage */
			|| MATCH_ID("AJOJ")		/* Jikkyou Oshaberi Parodius*/
			|| MATCH_NA("ROCK N' ROLL RACING") /* Rock 'n Roll Racing */
			|| MATCH_NA("RISE OF THE ROBOTS")	/* Rise Of The Robots*/
			|| MATCH_ID("A3T")		/* Top Gear 3000 */
			|| MATCH_NA("MORTAL KOMBAT")	/* Mortal Kombat I*/
			|| MATCH_NA("MORTAL KOMBAT II")	/* Mortal Kombat II*/
			|| MATCH_NA("STAR FOX")		/* Star Fox (US/JP)*/
			|| MATCH_ID("ST2")		/* Star Fox 2 */
			|| MATCH_NA("STAR WING")		/* Star Wing (EU)*/
			|| MATCH_NA("TINYTOON adventures") /* Buster Busts Loose */
			|| MATCH_ID("3Z")		/* Demon's Crest*/
			|| MATCH_NA("AXELAY")		/* Axelay*/
			|| MATCH_NA("JAKI CRUSH")	/* Jaki Crush */
			|| MATCH_NA("ZOMBIES")		/* Zombies (EU)*/
			|| MATCH_NA("THE JUNGLE BOOK")	/* Jungle Book */
			|| MATCH_NA("ZOMBIES ATE MY NEIGHB")	/* Zombies Ate My Neighbors*/
			|| MATCH_NA("UNIRACERS")		/* Uniracers*/
			|| MATCH_NA("UNIRALLY")		/* Unirally*/
			|| MATCH_NA("CHRONO TRIGGER")	/* Chrono Trigger*/
			|| MATCH_NA("JURASSIC PARK")	/* Jurassic Park*/
			|| MATCH_NA("THE MAGICAL QUEST")	/* The Magical Quest*/
			|| MATCH_NA("SOULBLAZER - 1 USA")	/* Soul Blazer (US)*/
			|| MATCH_NA("SOULBLAZER - 1 ENG")	/* Soul Blazer (PAL)*/
			|| MATCH_NA("SOULBLADER - 1")		/* Soul Blader*/
			|| MATCH_NA("GOKUJYOU PARODIUS")		/* Gokujou Parodius*/
			|| MATCH_ID("ABT")		/* Adventures of Batman and Robin*/
			|| MATCH_ID("5M")		/* Mario All-Stars + World*/
			|| MATCH_NA("SUPER MARIO ALL_STARS") /* Super Mario All-Stars (EU/US)*/
			|| MATCH_NA("SUPERMARIO COLLECTION") /* Super Mario Collection (JP)*/
			|| MATCH_ID("4Q")		/* Super Punch-Out*/
			|| MATCH_NA("HARVEST MOON")	/* Harvest Moon*/
			|| MATCH_ID("ADZ")		/* Dracula X*/
			|| MATCH_ID("A3M")		/* Mortal Kombat 3*/
			|| MATCH_ID("AM4J")		/* Do-Re-Mi Fantasy - Milon no Dokidoki Daibouken*/
			|| MATCH_NA("BT IN BATTLEMANIACS")	/* Battletoads in Battlemaniacs*/
			|| MATCH_NA("SPACE MEGAFORCE")	/* Space Megaforce (US)*/
			|| MATCH_ID("ARX")		/* Mega Man X2 */
			|| MATCH_NA("SUPER ALESTE")	/* Super Aleste (EU/JP)*/
			|| MATCH_NA("VALKEN")		/* Assault Suits Valken (JP)*/
			|| MATCH_NA("CYBERNATOR")	/* Cybernator (EU/US)*/
			|| MATCH_NA("SUPER BOMBERMAN")	/* Super Bomberman 1*/
			|| MATCH_NA("SUPER BOMBERMAN2")	/* Super Bomberman 2*/
			|| MATCH_ID("AS6")		/* Super Bomberman 3*/
			|| MATCH_ID("A4B")		/* Super Bomberman 4*/
			|| MATCH_ID("AYL")		/* Tetris Attack*/
			|| MATCH_NA("POCKY ROCKY")	/* Pocky & Rocky (US/EU)*/
			|| MATCH_NA("KIKIKAIKAI")	/* Kiki Kaikai (JP)*/
			|| MATCH_ID("ANI")		/* Lufia 2 / Estpolis Denki 2*/
			|| MATCH_ID("AQT")		/* Terranigma*/
			|| MATCH_NA("twinbee")		/* Twinbee Rainbow Bell Adventures*/
			|| MATCH_ID("AO7")		/* Tactics Ogre*/
			|| MATCH_NA("Ogre Battle USA")	/* Ogre Battle (US)*/
			|| MATCH_ID("AQ3")		/* Dragon Quest 3*/
			|| MATCH_NA("DRAGONQUEST5")	/* Dragon Quest 5*/
			|| MATCH_ID("AQ6J")		/* Dragon Quest 6*/
			|| MATCH_NA("DARIUS FORCE")	/* Darius Force*/
			|| MATCH_ID("AGC")		/* Front Mission*/
			|| MATCH_ID("AZGJ")		/* Front Mission Gun Hazard*/
			|| MATCH_NA("GANBARE GOEMON")	/* Ganbare Goemon (JP)*/
			|| MATCH_NA("GANBARE GOEMON 2")	/* Ganbare Goemon 2*/
			|| MATCH_NA("mystical ninja")	/* Legend of Mystical Ninja (US/EU)*/
			|| MATCH_NA("NOSFERATU")		/* Nosferatu*/
			|| MATCH_NA("PAC ATTACK")	/* Pac Attack*/
			|| MATCH_NA("PARODIUS")		/* Parodius 1*/
			|| MATCH_NA("PRINCE OF PERSIA")	/* Prince of Persia*/
			|| MATCH_NA("FINAL FANTASY 5")	/* Final Fantasy 5*/
			|| MATCH_NA("OUT OF THIS WORLD") /* Out Of This World (US)*/
			|| MATCH_NA("OUTER WORLD")	/* Outer World (JP)*/
			|| MATCH_NA("ANOTHER WORLD")	/* Another World (EU)*/
			|| MATCH_ID("ABZE")		/* Ballz 3D*/
			|| MATCH_ID("AXSP")		/* Winter Gold*/
			|| MATCH_ID("APJJ")		/* Wonder Project J*/
			|| MATCH_NA("KRUSTYS SUPER FUNHOUSE")	/* Krustys Super Fun House*/
			|| MATCH_NA("KRUSTYS SUPERFUNHOUSE")	/* Krustys Super Fun House*/
			|| MATCH_NA("LEMMINGS")		/* Lemmings 1*/
			|| MATCH_ID("A3Z")		/* Ultimate Mortal Kombat 3*/
			|| MATCH_ID("ARFJ")		/* Star Ocean*/
			|| MATCH_ID("AXBE")		/* Bahamut Lagoon*/
			|| MATCH_ID("ARQ")		/* Realm */
			|| MATCH_NA("S O S")		/* SOS */
			|| MATCH_NA("SUPER DONKEY KONG") /* Super Donkey Kong */
			|| MATCH_ID("AC6J")		/* Cu-On-Pa*/
			|| MATCH_ID("ASR")		/* Street Racer*/
			|| MATCH_NA("SUPER WIDGET")	/* Super Widget*/
			|| MATCH_NA("WILD GUNS")	/* Wild Guns */
			|| MATCH_NA("R-TYPE 3")		/* R-Type 3*/
			|| MATCH_NA("THE NINJAWARRIORS") /* Ninja Warriors */
			|| MATCH_ID("ARW")		/* Super Mario RPG*/
			|| MATCH_NA("SHVC FIREEMBLEM")	/* Fire Emblem - Monshou no Nazo*/
			|| MATCH_ID("BFRJ")		/* Fire Emblem - Thracia 776*/
			|| MATCH_ID("A32J")		/* Fire Emblem - Seisen no Keifu*/
			|| MATCH_ID("AR9")		/* Primal Rage*/
			|| MATCH_ID("APUE")		/* Prehistorik Man*/
			|| MATCH_ID("ALSJ")		/* Lady Stalker*/
			|| MATCH_NA("ROCKMAN&FORTE")	/* Rockman & Forte*/
			|| MATCH_NA("SUPER SWIV")	/* Super SWIV*/
			|| MATCH_NA("CONTRA3 THE ALIEN WARS")	/* Contra 3 The Alien Wars*/
			|| MATCH_NA("EARTHWORM JIM 2")	/* Earthworm Jim 2*/
			|| MATCH_NA("CHOHMAKAIMURA")	/* Chou Makai Mura*/
			|| MATCH_NA("SUPER GHOULS'N GHOSTS")	/* Super Ghouls 'n Ghosts*/
			|| MATCH_NA("X-KALIBER 2097")	/* X-Kaliber 2097*/
			|| MATCH_NA("PHALANX")		/* Phalanx */
			|| MATCH_NA("FINAL FIGHT 2")	/* Final Fight 2*/
			|| MATCH_NA("SUPER TURRICAN 2")	/* Super Turrican 2*/
			|| MATCH_NA("DUNGEON MASTER")	/* Dungeon Master*/
			|| MATCH_NA("DOOM TROOPERS")	/* Doom Troopers*/
			|| MATCH_NA("DOOM")		/* Doom */
			|| MATCH_NA("XAK 1")		/* Xak 1*/
			|| MATCH_NA("XARDION")		/* Xardion*/
			);

		coldata_update_screen = !(
				//MATCH_NA("Super Metroid") 	/* Super Metroid*/
				//|| MATCH_NA("DONKEY KONG COUNTRY") /* Donkey Kong Country 1 */
				//|| MATCH_NA("AREA88")		/* Area 88 */
				//|| MATCH_NA("U.N.Squadron")	/* UN Squadron */
				//|| MATCH_NA("Perfect Eleven")	/* Perfect Eleven - ISS1 */
				//MATCH_ID("ADNE")			/* Donkey Kong Country 2 (US) */
				//|| MATCH_ID("AWJ")		/* ISS Deluxe / World Soccer 2 Fighting Eleven */
				//|| MATCH_NA("JIM POWER THE LOST DIO") /* Jim Power The Lost Dimension */
				//|| MATCH_ID("A3C")		/* Donkey Kong Country 3*/
				//|| MATCH_NA("Secret of MANA") /* Secret of Mana */
				//|| MATCH_ID("A2J")		/* Jurassic Park 2 */
				//|| MATCH_ID("AJL")		/* Justice League */
				//|| MATCH_NA("JOE AND MAC")	/* Joe and Mac */
				//|| MATCH_NA("JOE & MAC 2")	/* Joe & Mac 2 */
				//|| MATCH_ID("J3")		/* Joe & Mac 3 EU) */
			//|| MATCH_NA("JYUTEI SENKI")	/* Jyutei Senki */
			//|| MATCH_ID("CQ  ")		/* Stunt Race FX */
#if defined(_XBOX1) || defined(GEKKO)
			//|| MATCH_ID("AD8")		/* Doom */
			//|| MATCH_ID("A3D")			/* Seiken Densetsu 3 */
			//|| MATCH_NA("SeikenDensetsu3Sample1")	/* Seiken Densetsu 3 */
			//|| MATCH_NA("ROMANCING SAGA3")		/* Romancing Saga 3 */
#endif
			//|| MATCH_NA("T.M.H.T.4")	/* Teenage Mutant Hero Turtles 4 - Turtles In Time (E) */
			//|| MATCH_NA("T.M.N.T.")		/* Teenage Mutant Ninja Turtles 4 - Turtles In Time (J) */
			//|| MATCH_NA("T.M.N.T.4")	/* Teenage Mutant Ninja Turtles 4 - Turtles In Time (AU) */
			//|| MATCH_NA("T.M.N.T. 4")	/* Teenage Mutant Ninja Turtles 4 - Turtles In Time (US) */
			MATCH_NA("T.M.N.T. MW")	/* Teenage Mutant Ninja Turtles - Mutant Warriors (J) */
			|| MATCH_NA("T.M.H.T TF")	/* Teenage Mutant Hero Turtles - Tour Fighters (EU) */
			|| MATCH_NA("T.M.N.T. TF")	/* Teenage Mutant Ninja Turtles - Tour Fighters (AU) */
			|| MATCH_NA("T.M.N.T.5")	/* Teenage Mutant Ninja Turtles - Tour Fighters (U) */
			//|| MATCH_NA("SUPER CASTLEVANIA 4") /* Super Castlevania IV */
			//|| MATCH_NA("AKUMAJO DRACULA") /* Super Castlevania IV */
			//|| MATCH_ID("ATVE")		/* Tales of Phantasia (EN) (DeJap)*/
			//|| MATCH_NA("MEGAMAN X")		/* Mega Man X (US) */
			//|| MATCH_ID("A7R")		/* Rockman / Mega Man 7 */
			//|| MATCH_NA("F-ZERO")		/* Rockman / Mega Man 7 */
			//|| MATCH_NA("EQUINOX")		/* Equinox */
			//|| MATCH_NA("SUPER MARIO KART") /* Super Mario Kart */
			//|| MATCH_ID("AR3")		/* Mega Man X3 (JP/US) */
			//|| MATCH_NA("SECRET OF EVERMORE") 	/* Secret of Evermore*/
			|| MATCH_ID("AKL")		/* Killer Instinct*/
			//|| MATCH_NA("NBA JAM")		/* NBA Jam */
			//|| MATCH_NA("PILOTWINGS")	/* Pilotwings */
			//|| MATCH_NA("FINAL FIGHT")	/* Final Fight */
			//|| MATCH_NA("FINAL FANTASY 6")	/* Final Fantasy VI (JP)*/
			//|| MATCH_NA("FINAL FANTASY 3")	/* Final Fantasy III (US)*/
			//|| MATCH_NA("ILLUSION OF GAIA USA") /* Illusion of Gaia (US)*/
			//|| MATCH_NA("GAIA GENSOUKI 1 JPN")	/* Gaia Gensouki (JPN)*/
			//|| MATCH_ID("AEJ") 		/* Earthworm Jim*/
			//|| MATCH_ID("AFJ")		/* Kirby's Dreamland 3 */
			|| MATCH_NA("SUPER MARIOWORLD")	/* Super Mario World*/
			//|| MATCH_NA("THE LEGEND OF ZELDA")	/* Zelda 3*/
			//|| MATCH_NA("LA LEGENDE DE ZELDA")	/* Zelda 3 (FR)*/
			//|| MATCH_NA("ZELDANODENSETSU")		/* Zelda 3 (JPN)*/
			//|| MATCH_NA("SPARKSTER")		/* Sparkster*/
			//|| MATCH_NA("SIMCITY")		/* Sim City */
			//|| MATCH_NA("T2 ARCADE")	/* T2 Arcade */
			//|| MATCH_NA("PLOK")		/* Plok!*/
			//|| MATCH_NA("ALADDIN")		/* Aladdin */
			//|| MATCH_NA("ActRaiser-2 JPN")	/* ActRaiser 2 (JPN) */
			//|| MATCH_NA("ActRaiser-2 USA")	/* ActRaiser 2 (US) */
			//|| MATCH_ID("A8")		/* ActRaiser 2 (EU) */
			//|| MATCH_ID("APT")		/* Pac In Time */
			//|| MATCH_ID("AXG")		/*NBA Hangtime */
			//|| MATCH_NA("THE CHAOS ENGINE")	/* Chaos Engine */
			//|| MATCH_ID("AWD")		/* Weaponlord */
			//|| MATCH_NA("TALES OF PHANTASIA")       /* Tales of Phantasia */
			//|| MATCH_NA("GS MIKAMI")		/* GS Mikami - Joreishi wa Nice Body*/
			//|| MATCH_ID("ATQ")		/* Theme Park */
			//|| MATCH_NA("TOTAL CARNAGE")	/* Total Carnage */
			|| MATCH_ID("AJOJ")		/* Jikkyou Oshaberi Parodius*/
			//|| MATCH_NA("ROCK N' ROLL RACING") /* Rock 'n Roll Racing */
			//|| MATCH_NA("RISE OF THE ROBOTS")	/* Rise Of The Robots*/
			//|| MATCH_ID("A3T")		/* Top Gear 3000 */
			//|| MATCH_NA("MORTAL KOMBAT")	/* Mortal Kombat I*/
			//|| MATCH_NA("MORTAL KOMBAT II")	/* Mortal Kombat II*/
			//|| MATCH_NA("STAR FOX")		/* Star Fox (US/JP)*/
			//|| MATCH_ID("ST2")		/* Star Fox 2 */
			//|| MATCH_NA("STAR WING")		/* Star Wing (EU)*/
			//|| MATCH_NA("TINYTOON adventures") /* Buster Busts Loose */
			//|| MATCH_ID("3Z")		/* Demon's Crest*/
			//|| MATCH_NA("AXELAY")		/* Axelay*/
			//|| MATCH_NA("JAKI CRUSH")	/* Jaki Crush */
			//|| MATCH_NA("ZOMBIES")		/* Zombies (EU)*/
			//|| MATCH_NA("THE JUNGLE BOOK")	/* Jungle Book */
			//|| MATCH_NA("ZOMBIES ATE MY NEIGHB")	/* Zombies Ate My Neighbors*/
			//|| MATCH_NA("UNIRACERS")		/* Uniracers*/
			//|| MATCH_NA("UNIRALLY")		/* Unirally*/
			//|| MATCH_NA("CHRONO TRIGGER")	/* Chrono Trigger*/
			//|| MATCH_NA("JURASSIC PARK")	/* Jurassic Park*/
			//|| MATCH_NA("THE MAGICAL QUEST")	/* The Magical Quest*/
			//|| MATCH_NA("SOULBLAZER - 1 USA")	/* Soul Blazer (US)*/
			//|| MATCH_NA("SOULBLAZER - 1 ENG")	/* Soul Blazer (PAL)*/
			//|| MATCH_NA("SOULBLADER - 1")		/* Soul Blader*/
			//|| MATCH_NA("GOKUJYOU PARODIUS")		/* Gokujou Parodius*/
			//|| MATCH_ID("ABT")		/* Adventures of Batman and Robin*/
			//|| MATCH_ID("5M")		/* Mario All-Stars + World*/
			//|| MATCH_NA("SUPER MARIO ALL_STARS") /* Super Mario All-Stars (EU/US)*/
			//|| MATCH_NA("SUPERMARIO COLLECTION") /* Super Mario Collection (JP)*/
			//|| MATCH_ID("4Q")		/* Super Punch-Out*/
			//|| MATCH_NA("HARVEST MOON")	/* Harvest Moon*/
			//|| MATCH_ID("ADZ")		/* Dracula X*/
			//|| MATCH_ID("A3M")		/* Mortal Kombat 3*/
			//|| MATCH_ID("AM4J")		/* Do-Re-Mi Fantasy - Milon no Dokidoki Daibouken*/
			//|| MATCH_NA("BT IN BATTLEMANIACS")	/* Battletoads in Battlemaniacs*/
			//|| MATCH_NA("SPACE MEGAFORCE")	/* Space Megaforce (US)*/
			//|| MATCH_ID("ARX")		/* Mega Man X2 */
			//|| MATCH_NA("SUPER ALESTE")	/* Super Aleste (EU/JP)*/
			//|| MATCH_NA("VALKEN")		/* Assault Suits Valken (JP)*/
			//|| MATCH_NA("CYBERNATOR")	/* Cybernator (EU/US)*/
			//|| MATCH_NA("SUPER BOMBERMAN")	/* Super Bomberman 1*/
			//|| MATCH_NA("SUPER BOMBERMAN2")	/* Super Bomberman 2*/
			//|| MATCH_ID("AS6")		/* Super Bomberman 3*/
			//|| MATCH_ID("A4B")		/* Super Bomberman 4*/
			//|| MATCH_ID("AYL")		/* Tetris Attack*/
			//|| MATCH_NA("POCKY ROCKY")	/* Pocky & Rocky (US/EU)*/
			//|| MATCH_NA("KIKIKAIKAI")	/* Kiki Kaikai (JP)*/
			//|| MATCH_ID("ANI")		/* Lufia 2 / Estpolis Denki 2*/
			//|| MATCH_ID("AQT")		/* Terranigma*/
			//|| MATCH_NA("twinbee")		/* Twinbee Rainbow Bell Adventures*/
			//|| MATCH_ID("AO7")		/* Tactics Ogre*/
			//|| MATCH_NA("Ogre Battle USA")	/* Ogre Battle (US)*/
			//|| MATCH_ID("AQ3")		/* Dragon Quest 3*/
			//|| MATCH_NA("DRAGONQUEST5")	/* Dragon Quest 5*/
			//|| MATCH_ID("AQ6J")		/* Dragon Quest 6*/
			//|| MATCH_NA("DARIUS FORCE")	/* Darius Force*/
			//|| MATCH_ID("AZGJ")		/* Front Mission Gun Hazard*/
			//|| MATCH_NA("GANBARE GOEMON")	/* Ganbare Goemon (JP)*/
			//|| MATCH_NA("GANBARE GOEMON 2")	/* Ganbare Goemon 2*/
			//|| MATCH_NA("mystical ninja")	/* Legend of Mystical Ninja (US/EU)*/
			//|| MATCH_NA("NOSFERATU")		/* Nosferatu*/
			//|| MATCH_NA("PAC ATTACK")	/* Pac Attack*/
			//|| MATCH_NA("PARODIUS")		/* Parodius 1*/
			//|| MATCH_NA("PRINCE OF PERSIA")	/* Prince of Persia*/
			//|| MATCH_NA("FINAL FANTASY 5")	/* Final Fantasy 5*/
			//|| MATCH_NA("OUT OF THIS WORLD") /* Out Of This World (US)*/
			//|| MATCH_NA("OUTER WORLD")	/* Outer World (JP)*/
			//|| MATCH_NA("ANOTHER WORLD")	/* Another World (EU)*/
			//|| MATCH_ID("ABZE")		/* Ballz 3D*/
			//|| MATCH_ID("AXSP")		/* Winter Gold*/
			//|| MATCH_ID("APJJ")		/* Wonder Project J*/
			//|| MATCH_NA("KRUSTYS SUPER FUNHOUSE")	/* Krustys Super Fun House*/
			//|| MATCH_NA("KRUSTYS SUPERFUNHOUSE")	/* Krustys Super Fun House*/
			//|| MATCH_NA("LEMMINGS")		/* Lemmings 1*/
			//|| MATCH_ID("A3Z")		/* Ultimate Mortal Kombat 3*/
			//|| MATCH_ID("ARFJ")		/* Star Ocean*/
			//|| MATCH_ID("AXBE")		/* Bahamut Lagoon*/
			//|| MATCH_ID("ARQ")		/* Realm */
			//|| MATCH_NA("S O S")		/* SOS */
			//|| MATCH_NA("SUPER DONKEY KONG") /* Super Donkey Kong */
			//|| MATCH_ID("AC6J")		/* Cu-On-Pa*/
			//|| MATCH_ID("ASR")		/* Street Racer*/
			//|| MATCH_NA("SUPER WIDGET")	/* Super Widget*/
			//|| MATCH_NA("WILD GUNS")	/* Wild Guns */
			//|| MATCH_NA("R-TYPE 3")		/* R-Type 3*/
			//|| MATCH_NA("THE NINJAWARRIORS") /* Ninja Warriors */
			//|| MATCH_ID("ARW")		/* Super Mario RPG*/
			//|| MATCH_NA("SHVC FIREEMBLEM")	/* Fire Emblem - Monshou no Nazo*/
			//|| MATCH_ID("BFRJ")		/* Fire Emblem - Thracia 776*/
			//|| MATCH_ID("A32J")		/* Fire Emblem - Seisen no Keifu*/
			//|| MATCH_ID("AR9")		/* Primal Rage*/
			//|| MATCH_ID("APUE")		/* Prehistorik Man*/
			//|| MATCH_ID("ALSJ")		/* Lady Stalker*/
			//|| MATCH_NA("ROCKMAN&FORTE")	/* Rockman & Forte*/
			//|| MATCH_NA("SUPER SWIV")	/* Super SWIV*/
			//|| MATCH_NA("CONTRA3 THE ALIEN WARS")	/* Contra 3 The Alien Wars*/
			//|| MATCH_NA("EARTHWORM JIM 2")	/* Earthworm Jim 2*/
			//|| MATCH_NA("CHOHMAKAIMURA")	/* Chou Makai Mura*/
			//|| MATCH_NA("SUPER GHOULS'N GHOSTS")	/* Super Ghouls 'n Ghosts*/
			//|| MATCH_NA("X-KALIBER 2097")	/* X-Kaliber 2097*/
			//|| MATCH_NA("PHALANX")		/* Phalanx */
			//|| MATCH_NA("FINAL FIGHT 2")	/* Final Fight 2*/
			//|| MATCH_NA("SUPER TURRICAN 2")	/* Super Turrican 2*/
			//|| MATCH_NA("DUNGEON MASTER")	/* Dungeon Master*/
			//|| MATCH_NA("DOOM TROOPERS")	/* Doom Troopers*/
			//|| MATCH_NA("DOOM")		/* Doom */
			//|| MATCH_NA("XAK 1")		/* Xak 1*/
			//|| MATCH_NA("XARDION")		/* Xardion*/
			);

		Settings.SpeedhackGameID = SPEEDHACK_NONE;

		/* Clipping hack - gains around 5-7 extra fps - only use it 
		   for specific games where nothing breaks with this hack on */

		PPU.FullClipping = !(
			MATCH_NA("FINAL FANTASY 6")	/* Final Fantasy VI (JP) */
			|| MATCH_NA("FINAL FANTASY 3")	/* Final Fantasy III (US) */
		);

		/* SPEED HACK PATHS */
		if(
				MATCH_NA("DONKEY KONG COUNTRY") /* Donkey Kong Country 1 */
		)
			Settings.SpeedhackGameID = SPEEDHACK_DKC1;

		if(
			MATCH_NA("FINAL FANTASY 6")	/* Final Fantasy VI (JP) */
			|| MATCH_NA("FINAL FANTASY 3")	/* Final Fantasy III (US) */
		)
			Settings.SpeedhackGameID = SPEEDHACK_FF6;

		/* Check if Chrono Trigger is loaded, if so, we need to set a variable to 
		   true to get rid of an annoying mid-frame resolution switch to 256x239 
		   which can cause an undesirable flicker/breakup of the screen for a 
		   split second - this happens whenever the game switches from normal 
		   mode to battle mode and vice versa */
		if(	MATCH_NC("CHRONO TRIGGER") ||	/* Chrono Trigger */
			MATCH_ID("ACT") ||
			MATCH_ID("AC9J")		/* Chrono Trigger (Sample) */
		  )
			Settings.SpeedhackGameID = SPEEDHACK_CT;

      if(
            MATCH_NA("STAR FOX") ||		/* Star Fox (US/JP)*/
            MATCH_NA("STAR WING"))		/* Star Wing (EU)*/
         Settings.SpeedhackGameID = SPEEDHACK_STAR_FOX_1;

		if( MATCH_NA("SUPER MARIOWORLD"))	/* Super Mario World*/
         Settings.SpeedhackGameID = SPEEDHACK_SUPER_MARIO_WORLD;

	}

	{
		unsigned i;
		const struct {
			const char *fmt;
			int val;
		} fmt_val[5] = {
			{ "PPU.RenderSub = %d", PPU.RenderSub },
			{ "PPU.FullClipping = %d", PPU.FullClipping },
			{ "Settings.SpeedhackGameID = %d", Settings.SpeedhackGameID },
			{ "PPU.SFXSpeedupHack = %d", PPU.SFXSpeedupHack },
			{ "coldata_update_screen = %d", coldata_update_screen }
		};

		for (i = 0; i < 5; i++)
		{
			snprintf(String, sizeof(String), fmt_val[i].fmt, fmt_val[i].val);
			S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, String);
		}
	}

	if (Settings.AccessoryAutoDetection == ACCESSORY_AUTODETECTION_CONFIRM)
	{
		/* Multitap accessory detection */
		Settings.CurrentROMisMultitapCompatible = (
				MATCH_NN("BARKLEY") ||				/* Barkley Shut Up and Jam!*/
				MATCH_ID("ABCJ") ||				/* Battle Cross*/
				MATCH_NA("Bill Walsh College FB1") ||		/* Bill Walsh College Football*/
				MATCH_ID("ANYE") ||				/* College Slam*/
				MATCH_ID("AC3J") ||				/* Crystal Beans from Dungeon Explorer*/
				MATCH_NC("Bruce Lee") ||				/* Dragon - Bruce Lee Story*/
				MATCH_ID("AVS") ||				/* Fever Pitch Soccer (EU) (*)*/
				/* Head-On Soccer (US) (*)*/
				MATCH_NA("FURI-FURI GIRLS") ||			/* Furi-Furi Girls*/
				MATCH_NA("HAT TRICK HERO 2") ||			/* Hat Trick Hero 2*/
				MATCH_NA("HEBEREKE NO PUZZLE") ||		/* Hebereke no Oishii Puzzle wa Irimasenka*/
				MATCH_ID("AWJ") ||				/* International Superstar Soccer Deluxe (EU/US) (*)*/
				/* Jikkyou World Soccer 2 - Fighting Eleven (JP) (*)*/
				MATCH_NA("J-LEAGUE SUPER SOCCER0") ||		/* J-League Super Soccer (JP)*/
				/* Virtual Soccer (EU)*/
				MATCH_NA("JIGSAW PARTY") ||			/* Jigsaw Party (US) (*)*/
				/* Pieces (JP)*/
				MATCH_NA("JIMMY CONNORS TENNIS") ||		/* Jimmy Connors Pro Tennis Tour*/
				MATCH_NA("LAS VEGAS DREAM") ||			/* Las Vegas Dream in Golden Paradise (JP) (*)*/
				/* Vegas Stakes (EU/US)*/
				MATCH_ID("ALT") ||				/* Looney Tunes B-Ball (US) (*)*/
				/* Looney Tunes Basketball (EU) (*)*/
				MATCH_ID("AYHJ") ||				/* Mizuki Shigeru no Youkai Hyakkiyakou*/
				MATCH_ID("A3VJ") ||				/* Multi Play Volleyball*/
				MATCH_NA("MUSCLE BOMBER") ||			/* Muscle Bomber: The Body Explosion (JP) (*)*/
				/* Saturday Night Slam Masters (US)*/
				MATCH_ID("CHAMP WRESTLING") ||			/* Natsume Championship Wrestling*/
				MATCH_ID("ANJ") || 				/* NBA Give 'n Go*/
				MATCH_ID("AXG") || 				/* NBA Hangtime*/
				MATCH_ID("AJT") || 				/* NBA Jam Tournament Edition (EU/US/JP)*/
				MATCH_NA("NBA JAM")   || 			/* NBA Jam*/
				MATCH_ID("AFIE") ||				/* NCAA Final Four Basketball*/
				MATCH_ID("AFBE") ||				/* NCAA Football*/
				MATCH_ID("Q9") ||				/* NFL Quarterback Club / NFL Quarterback Club '95*/
				MATCH_ID("AQB") ||				/* NFL Quarterback Club '96*/
				MATCH_NA("PIECES") ||
				MATCH_ID("Z5") ||				/* Pieces (JP) (*)*/
				/* Jigsaw Party (US)*/
				MATCH_ID("ARVE") ||				/* Rap Jam - Volume One*/
				MATCH_NA("RUSHING BEAT SYURA") ||		/* Rushing Beat Shura (JP) (*)*/
				/* The Peace Keepers (US)*/
				MATCH_NA("S.Night SLAM MASTERS") ||		/* Saturday Night Slam Masters (US) (*)*/
				/* Muscle Bomber: The Body Explosion (JP)*/
				MATCH_NA("Secret of MANA") ||			/* Secret of Mana (EU/US) (*)*/
				/* Seiken Densetsu 2 (JP)*/
				MATCH_NA("SeikenDensetsu 2") ||			/* Seiken Densetsu 2 (JP) (*) / */
				/* Secret of Mana (EU/US)*/
				MATCH_NN("SeikenDensetsu3") ||			/* Seiken Densetsu 3 (in case you use*/
				/* 3-player IPS patch)*/
				MATCH_NA("SMASH TENNIS") ||			/* Smash Tennis (EU) (*) / */
				/* Super Family Tennis (JP)*/
				/*FIXME: TODO - Add CRC32 for Super Family Tennis - CRC32: 2BCBFF26*/
				MATCH_ID("ASR") ||				/* Street Racer*/
				MATCH_NA("SUGOI HEBEREKE") ||			/* Sugoi Hebereke*/
				MATCH_NA("Sugoro Quest++") ||			/* Sugoro Quest++: Dicenics*/
				MATCH_ID("AO9") ||				/* Summer Olympics*/
				MATCH_ID("AS6") ||				/* Super Bomberman 3*/
				MATCH_ID("A4BJ") || 				/* Super Bomberman 4*/
				MATCH_ID("A5SJ") ||				/* Super Bomberman 5*/
				MATCH_ID("AK8J") ||				/* Super Bomberman 5 - Caravan Event Ban*/
				MATCH_ID("APBJ") ||				/* Super Bomberman Panic Bomber W*/
				MATCH_ID("AF5J") ||				/* Super Fire Pro Wrestling X*/
				MATCH_ID("AP4J") ||				/* Super Fire Pro Wrestling Special*/
				MATCH_ID("AQQJ") ||				/* Super Fire Pro Wrestling - Queen's Special*/
				MATCH_ID("A7PJ") ||				/* Super Puyo Puyo Tsuu Remix*/
				MATCH_ID("AT3J") ||				/* Super Tetris 3*/
				MATCH_ID("AFY") ||				/* Syndicate*/
				MATCH_NA("THE PEACE KEEPERS") ||			/* The Peace Keepers (US) (*)*/
				/* Rushing Beat Shura (JP)*/
				MATCH_NA("Tiny Toon Sports") ||			/* Tiny Toon Adventures: Dotabata Daiundoukai (JP) (*)*/
				/* Tiny Toon Adventures: Wacky Sports Challenge (US) (*)*/
				/* Tiny Toon Adventures: Wild & Wacky Sports (EU) (*)*/
				MATCH_ID("A3T") ||				/* Top Gear 3000 (EU/US) (*)*/
				/* Planet's Champ TG 3000 (JP) (*)*/
				MATCH_NA("VEGAS STAKES") ||			/* Vegas Stakes (EU/US) (*)*/
				/* Las Vegas Dream in Golden Paradise (JP)*/
				MATCH_NA("Virtual Soccer") ||			/* Virtual Soccer (EU) (*)*/
				/* J-League Super Soccer (JP)*/
				MATCH_ID("AWF") ||				/* WWF RAW*/

				/*Catch-alls*/
				MATCH_NC("SUPER BOMBERMAN") ||			/* All Bomberman games (FIXME: remove*/
				/* individual two Bomberman entries if this works)*/
				MATCH_NC("FIFA") ||				/* All FIFA games*/
				MATCH_NN("NHL") ||				/* All NHL games*/
				MATCH_NC("MADDEN") ||				/* All Madden games*/
				MATCH_NC("MICRO MACHINES") ||			/* All Micro Machines games*/
				MATCH_ID("Q4")					/* ?*/
				);

		/* Mouse accessory detection */
		Settings.CurrentROMisMouseCompatible = (
				MATCH_NC("ACME ANIMATION FACTOR") ||	/* ACME Animation Factory*/
				MATCH_ID("ACM") ||
				MATCH_NC("ALICE PAINT") ||		/* Alice no Paint Adventure*/
				MATCH_ID("AALJ") ||
				MATCH_NC("ARKANOID DOH IT AGAIN0") ||	/* Arkanoid: Doh It Again*/
				MATCH_ID("A6") ||
				MATCH_NA("SFC SAILORMOON S") ||	/* Bishojou Senshi Sailor Moon S: Kondowa Puzzle de*/
				/* Oshiokiyo!*/
				MATCH_NA("BRANDISH2 EXPERT") ||		/* Brandish 2: Expert*/
				MATCH_ID("AQIJ") ||				
				MATCH_NA("BREAKTRHU!") ||		/* BreakThru!*/
				MATCH_ID("ABXE") ||
				MATCH_NA("CIVILIZATION") ||		/* Civilization*/
				MATCH_ID("EQ") ||
				MATCH_NA("CAMELTRY") ||			/* Cameltry (JP) (*)*/
				/* On The Ball (EU/US)*/
				MATCH_NA("CANNON FODDER") ||		/* Cannon Fodder*/
				MATCH_ID("ACNP") ||				
				MATCH_ID("M Ve") ||			/* Dai-3-ji Super Robot Taisen*/
				MATCH_ID("AR4J") ||				/* Dai-4-ji Super Robot Taisen*/
				MATCH_NA("SUPER ROBOT WARS 4") ||
				MATCH_NA("DOOM") ||			/* Doom*/
				MATCH_ID("AD8") ||				
				MATCH_ID("A88J") ||			/* Doukyuusei 2 (JP) (NP)*/
				MATCH_NA("DRAGON KNIGHT 4") ||		/* Dragon Knight 4*/
				MATCH_ID("A87J") ||				
				MATCH_NA("Eye of the Beholder") ||	/* Eye of the Beholder*/
				MATCH_NA("FARLANDSTORY2") ||		/* Farland Story 2*/
				MATCH_ID("A2FJ") ||
				MATCH_NA("Fun 'N Games") ||		/* Fun 'N Games (EU/US)*/
				MATCH_NA("GALAXY ROBO") ||		/* Galaxy Robo*/
				MATCH_NA("HIOUDEN") ||			/* Hiouden: Mamono-tachi tono Chikai*/
				MATCH_NA("JIGSAW PARTY") ||		/* Jigsaw Party (US) (*)*/
				/* Pieces (JP)*/
				/*MATCH_NA("JURASSIC PARK") ||		// Jurassic Park*/
				MATCH_NA("King Arthurs World") ||	/* King Arthur's World (EU/US) (*)*/
				/* Royal Conquest (JP)*/
				MATCH_NA("KOUTETSU-NO KISHI") ||		/* Koutetsu No Kishi*/
				MATCH_NA("KOUTETSU-NO KISHI 2") ||	/* Koutetsu No Kishi 2*/
				MATCH_NA("KOUTETSU-NO KISHI 3") ||	/* Koutetsu No Kishi 3*/
				MATCH_ID("APZJ") ||
				MATCH_NA("LAMBORGHINI AMERICAN") ||	/* Lamborghini American Challenge (EU/US)*/
				MATCH_NA("LAS VEGAS DREAM") ||		/* Las Vegas Dream in Golden Paradise (JP) (*)*/
				/* Vegas Stakes (EU/US)*/
				MATCH_NA("Lemmings 2,The Tribes") ||	/* Lemmings 2: The Tribes*/
				MATCH_ID("L2") ||
				MATCH_NA("LORD MONARCH") ||		/* Lord Monarch*/
				MATCH_NA("Mario&Wario") ||		/* Mario & Wario*/
				MATCH_NA("SUPER PICROSS") ||		/* Mario no Super Picross*/
				MATCH_ID("ACXJ") ||
				MATCH_NC("MARIOPAINT") ||		/* Mario Paint*/
				MATCH_NA("MEY  Preschool Fun") ||	/* Mario's Early Years: Preschool Fun*/
				MATCH_ID("AMEE") ||
				MATCH_NA("MEGA LO MANIA") ||		/* Mega-Lo Mania*/
				MATCH_NA("MIGHT  AND MAGIC III") ||	/* Might And Magic III - Isles of Terra*/
				MATCH_NA("WONDER KITCHEN") ||		/* Motoko-chan no Wonder Kitchen*/
				MATCH_NA("Nobunaga's Ambition") ||	/* Nobunaga's Ambition (US) (*)*/
				/* Super Nobunaga no Yabou: Zenkokuban (JP)*/
				MATCH_NA("ON THE BALL") ||		/* On The Ball (EU/US) (*)*/
				/* Cameltry (JP)*/
				MATCH_NA("OPERATION THUNDERBOLT1") ||	/* Operation Thunderbolt*/
				MATCH_NA("PIECES") ||
				MATCH_ID("Z5") ||			/* Pieces (JP) (*)*/
				/* Jigsaw Party (US)*/
				MATCH_NC("POPULOUS II") ||		/* Populous II*/
				MATCH_NA("POWERMONGER") ||		/* Power Monger*/
				MATCH_NA("Revolution X") ||		/* Revolution X*/
				MATCH_ID("AXR") ||
				MATCH_NA("Royal Conquest") ||		/* Royal Conquest (JP) (*)*/
				/* King Arthur's World (EU/US)*/
				MATCH_NA("TENBU SPIRIT") ||		/* San Goku Shi Seishi: Tenbu Spirits*/
				MATCH_NA("SHIEN THE BLADE CHASE0") ||	/* Shien The Blade Chaser (JP) (*)*/
				MATCH_NA("SHIEN'S REVENGE") ||		/* Shien's Revenge (*)*/
				MATCH_NC("SimAnt") ||			/* SimAnt*/
				MATCH_NA("SNOOPY CONCERT") ||		/* Snoopy Concert*/
				MATCH_NA("SUPER CAESARS PALACE") ||	/* Super Caesar's Palace*/
				MATCH_NA("SUPER CASTLES") ||		/* Super Castles*/
				MATCH_ID("AVIJ") ||
				/*FIXME: TODO: Super Noah's Ark 3D: - bad checksum - CRC32: A2315A14*/
				MATCH_NA("SuperPachiSlotMahjong") ||	/* Super Pachi Slot Mahjong*/
				MATCH_NA("SUPER SOLITAIRE") ||		/* Super Solitaire*/
				MATCH_NA("SUPER ZENKOKUBAN") ||		/* Super Nobunaga no Yabou: Zenkokuban (JP) (*)*/
				/* Nobunaga's Ambition (US)*/
				MATCH_NA("TERMINATOR2 THE MOVIE0") ||	/* Terminator 2: The Arcade Game*/
				MATCH_NA("TINSTAR") ||			/* Tin Star*/
				MATCH_ID("9N") ||
				MATCH_NA("TOKIMEKI MEMORIAL") ||		/* Tokimeki Memorial*/
				MATCH_ID("AM8J") ||
				MATCH_NA("Troddlers") ||			/* Troddlers*/
				MATCH_NA("UTOPIA") ||			/* Utopia*/
				MATCH_NA("VEGAS STAKES") ||		/* Vegas Stakes (EU/US) (*)*/
				/* Las Vegas Dream in Golden Paradise (JP)*/
				MATCH_NC("WOLFENSTEIN 3D") ||		/* Wolfenstein 3D*/
				MATCH_NA("WONDERPROJECT J") ||		/* Wonder Project J*/
				MATCH_ID("APJJ") ||				
				MATCH_NA("ZAN2 SPIRITS") ||		/* Zan 2: Spirits*/
				MATCH_NA("ZAN3 SFC")			/* Zan 3: Spirits*/
				);

		/* Super Scope accessory detection */
		Settings.CurrentROMisSuperScopeCompatible = (
				MATCH_NA("BATTLE CLASH") ||		/* Battle Clash (EU/US) (*)*/
										/* Space Bazooka (JP)*/
				MATCH_NA("SPACE BAZOOKA") ||		/* Space Bazooka (JP) (*)*/
										/* Battle Clash (EU/US)*/
				/*FIXME: TODO: Add The Hunt for Red October - used for bonus game*/
				/*FIXME: TODO: Add Lamborghini American Challenge - if needed*/
				MATCH_NA("METAL COMBAT") ||		/* Metal Combat: Falcon's Revenge*/
				MATCH_NA("OPERATION THUNDERBOLT1") ||	/* Operation Thunderbolt*/
				MATCH_NA("SUPER SCOPE 6") ||		/* Super Scope 6*/
				/*MATCH_NA("TERMINATOR2 THE MOVIE0") ||	// Terminator 2: The Arcade Game*/
				MATCH_NA("TINSTAR") ||			/* Tin Star*/
				MATCH_ID("9N") ||
				MATCH_NA("X ZONE") ||			/* X-Zone*/
				/*FIXME: TODO: Add Yoshi no Road Hunting - CRC32: 52948F3C*/
				MATCH_NA("YOSHI'S SAFARI")		/* Yoshi's Safari*/
			);
	}

	S9xAPUTimingSetSpeedup(Timings.APUSpeedup);
	S9xAPUAllowTimeOverflow(Timings.APUAllowTimeOverflow);

	/* Other timing hacks */

	Timings.HDMAStart   = SNES_HDMA_START_HC + Settings.HDMATimingHack - 100;
	Timings.HBlankStart = SNES_HBLANK_START_HC + Timings.HDMAStart - SNES_HDMA_START_HC;

	if (!Settings.DisableGameSpecificHacks)
	{
		/* The delay to sync CPU and DMA which Snes9x cannot emulate.
		   Some games need really severe delay timing... */

		/* Battle Grandprix */
		if (MATCH_NA("BATTLE GRANDPRIX"))
		{
			Timings.DMACPUSync = 20;
			snprintf(String, sizeof(String),
				 "DMA sync: %d", Timings.DMACPUSync);
			S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, String);
		}
	}

	if (!Settings.DisableGameSpecificHacks)
	{
		/* Opcode-based emulators cannot escape from "reading $4211/BPL" infinite loop...
		   The true IRQ can be triggered inside an opcode. */
		if (MATCH_NA("TRAVERSE")) /* Traverse - Starlight & Prairie*/
		{
			Timings.IRQPendCount = 1;
			snprintf(String, sizeof(String),
				 "IRQ count hack: %d", Timings.IRQPendCount);
			S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, String);
		}

		/* An infinite loop reads $4212 and waits V-blank end, whereas 
		   VIRQ is set V=0.

		   If Snes9x succeeds to escape from the loop before jumping 
		   into the IRQ handler, the game goes further.

		   If Snes9x jumps into the IRQ handler before escaping from 
		   the loop, Snes9x cannot escape from the loop permanently 
		   because the RTI is in the next V-blank. */

		if (MATCH_NA("Aero the AcroBat 2"))
		{
			Timings.IRQPendCount = 2;
			snprintf(String, sizeof(String),
				"IRQ count hack: %d", Timings.IRQPendCount);
			S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, String);
		}

		if (MATCH_NA("BATTLE BLAZE"))
		{
			Timings.IRQPendCount = 1;
			snprintf(String, sizeof(String),
				 "IRQ count hack: %d", Timings.IRQPendCount);
			S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP, String);
		}
	}

	if (!Settings.DisableGameSpecificHacks)
	{
		/* XXX: What's happening? */

		/* Spider-Man and the X-Men */
		if (MATCH_NA("X-MEN"))
		{
			Settings.BlockInvalidVRAMAccess = FALSE;
			S9xMessage(S9X_MSG_WARN, S9X_CATEGORY_MAP,
				   "Invalid VRAM access hack");
		}
	}

	/* SRAM initial value */

	if (!Settings.DisableGameSpecificHacks)
	{
		if (MATCH_NA("HITOMI3"))
		{
			Memory.SRAMSize = 1;
			Memory.SRAMMask = ((1 << (Memory.SRAMSize + 3)) * 128) - 1;
		}

		/* SRAM value fixes */
		if (MATCH_NA("SUPER DRIFT OUT")      || /* Super Drift Out*/
			MATCH_NA("SATAN IS OUR FATHER!") ||
			MATCH_NA("goemon 4"))               /* Ganbare Goemon Kirakira Douchuu*/
			SNESGameFixes.SRAMInitialValue = 0x00;

		/* Additional game fixes by sanmaiwashi ...*/
		/* XXX: unnecessary?*/
		if (MATCH_NA("SFX \xC5\xB2\xC4\xB6\xDE\xDD\xC0\xDE\xD1\xD3\xC9\xB6\xDE\xC0\xD8 1")) /* SD Gundam Gaiden - Knight Gundam Monogatari*/
			SNESGameFixes.SRAMInitialValue = 0x6b;

		/* others: BS and ST-01x games are 0x00.*/
	}

	/* OAM hacks */

	if (!Settings.DisableGameSpecificHacks)
	{
		/* OAM hacks because we don't fully understand the behavior of 
		   the SNES.

		   Totally wacky display in 2P mode...

		   seems to need a disproven behavior, so we're definitely 
		   overlooking some other bug? */

		/* Uniracers */
		if (MATCH_NN("UNIRACERS"))
		{
			SNESGameFixes.Uniracers = TRUE;
			S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_MAP,
				   "Applied Uniracers hack.");
		}
	}

	/* Show ROM information */

	strcpy(Memory.RawROMName, Memory.ROMName);
	snprintf(displayName, sizeof(displayName), "%s",
		 SafeANK(Memory.ROMRegion, Memory.ROMName));
	snprintf(Memory.ROMName, sizeof(Memory.ROMName),
		 "%s", Safe(Memory.ROMName));
	snprintf(Memory.ROMId, sizeof(Memory.ROMId), "%s", Safe(Memory.ROMId));

	snprintf(String, sizeof(String), "\"%s\" [%s] %s, %s, %s, %s, SRAM:%s, ID:%s, CRC32:%08X",
		displayName, isChecksumOK ? "checksum ok" : ((Multi.cartType == 4) ? "no checksum" : "bad checksum"),
		(Memory.HiROM ? ((Memory.ExtendedFormat != NOPE) ? "ExHiROM": "HiROM") : "LoROM"), Size(), KartContents(Memory.ROMType), Settings.PAL ? "PAL" : "NTSC", StaticRAMSize(), Memory.ROMId, Memory.ROMCRC32);
	S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_ROM, String);

	Settings.ForceLoROM = FALSE;
	Settings.ForceHiROM = FALSE;
	Settings.ForceHeader = FALSE;
	Settings.ForceNoHeader = FALSE;
	Settings.ForceInterleaved = FALSE;
	Settings.ForceInterleaved2 = FALSE;
	Settings.ForceInterleaveGD24 = FALSE;
	Settings.ForceNotInterleaved = FALSE;
	Settings.ForcePAL = FALSE;
	Settings.ForceNTSC = FALSE;

	S9xVerifyControllers();

	return (TRUE);
}
