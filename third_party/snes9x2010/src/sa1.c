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

#include <stdlib.h>
#include <string.h>
#include "snes9x.h"
#include "memmap.h"
#include "getset.h"

static uint8_t	SA1OpenBus;

void S9xSA1Init (void)
{
	SA1.IRQActive = FALSE;
	SA1.WaitingForInterrupt = FALSE;
	SA1.Flags = 0;
	memset(&Memory.FillRAM[0x2200], 0, 0x200);
	Memory.FillRAM[0x2200] = 0x20;
	Memory.FillRAM[0x2220] = 0x00;
	Memory.FillRAM[0x2221] = 0x01;
	Memory.FillRAM[0x2222] = 0x02;
	Memory.FillRAM[0x2223] = 0x03;
	Memory.FillRAM[0x2228] = 0xff;
	SA1.op1 = 0;
	SA1.op2 = 0;
	SA1.arithmetic_op = 0;
	SA1.sum = 0;
	SA1.overflow = FALSE;
	SA1.S9xOpcodes = NULL;
	SA1.PCBase = NULL;
}

static void S9xSA1SetPCBase (uint32_t address)
{
	uint8_t	*GetAddress;

	SA1Registers.PBPC = address & 0xffffff;
	SA1.ShiftedPB = address & 0xff0000;

	GetAddress = SA1.Map[(address & 0xffffff) >> MEMMAP_SHIFT];

	if (GetAddress >= (uint8_t *) MAP_LAST)
	{
		SA1.PCBase = GetAddress;
		return;
	}

	switch ((intptr_t) GetAddress)
	{
		case MAP_LOROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) != MEMMAP_MASK)
				SA1.PCBase = NULL;
			else
				SA1.PCBase = (Memory.SRAM + ((((address & 0xff0000) >> 1) | (address & 0x7fff)) & Memory.SRAMMask)) - (address & 0xffff);
			return;

		case MAP_HIROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) != MEMMAP_MASK)
				SA1.PCBase = NULL;
			else
				SA1.PCBase = (Memory.SRAM + (((address & 0x7fff) - 0x6000 + ((address & 0xf0000) >> 3)) & Memory.SRAMMask)) - (address & 0xffff);
			return;

		case MAP_BWRAM:
			SA1.PCBase = SA1.BWRAM - 0x6000 - (address & 0x8000);
			return;

		case MAP_SA1RAM:
			SA1.PCBase = Memory.SRAM;
			return;

		default:
			SA1.PCBase = NULL;
			return;
	}
}

static INLINE void S9xSA1UnpackStatus (void)
{
	SA1._Zero = (SA1Registers.PL & Zero) == 0;
	SA1._Negative = (SA1Registers.PL & Negative);
	SA1._Carry = (SA1Registers.PL & Carry);
	SA1._Overflow = (SA1Registers.PL & Overflow) >> 6;
}

static INLINE void S9xSA1FixCycles (void)
{
	if (SA1CheckEmulation())
	{
		SA1.S9xOpcodes = S9xSA1OpcodesM1X1;
		SA1.S9xOpLengths = S9xOpLengthsM1X1;
	}
	else
	if (SA1CheckMemory())
	{
		if (SA1CheckIndex())
		{
			SA1.S9xOpcodes = S9xSA1OpcodesM1X1;
			SA1.S9xOpLengths = S9xOpLengthsM1X1;
		}
		else
		{
			SA1.S9xOpcodes = S9xSA1OpcodesM1X0;
			SA1.S9xOpLengths = S9xOpLengthsM1X0;
		}
	}
	else
	{
		if (SA1CheckIndex())
		{
			SA1.S9xOpcodes = S9xSA1OpcodesM0X1;
			SA1.S9xOpLengths = S9xOpLengthsM0X1;
		}
		else
		{
			SA1.S9xOpcodes = S9xSA1OpcodesM0X0;
			SA1.S9xOpLengths = S9xOpLengthsM0X0;
		}
	}
}

static void S9xSA1Reset (void)
{
	SA1Registers.PBPC = 0;
	SA1Registers.PB = 0;
	SA1Registers.PCw = Memory.FillRAM[0x2203] | (Memory.FillRAM[0x2204] << 8);
	SA1Registers.D.W = 0;
	SA1Registers.DB = 0;
	SA1Registers.SH = 1;
	SA1Registers.SL = 0xFF;
	SA1Registers.XH = 0;
	SA1Registers.YH = 0;
	SA1Registers.P.W = 0;

	SA1.ShiftedPB = 0;
	SA1.ShiftedDB = 0;
	SA1SetFlags(MemoryFlag | IndexFlag | IRQ | Emulation);
	SA1ClearFlags(Decimal);

	SA1.WaitingForInterrupt = FALSE;
	SA1.PCBase = NULL;
	S9xSA1SetPCBase(SA1Registers.PBPC);
	SA1.S9xOpcodes = S9xSA1OpcodesM1X1;
	SA1.S9xOpLengths = S9xOpLengthsM1X1;

	S9xSA1UnpackStatus();
	S9xSA1FixCycles();
	SA1.BWRAM = Memory.SRAM;
	Memory.FillRAM[0x2225] = 0;
}

static void S9xSA1SetBWRAMMemMap (uint8_t val)
{
	int c;
	if (val & 0x80)
	{
		for ( c = 0; c < 0x400; c += 16)
		{
			SA1.Map[c + 6] = SA1.Map[c + 0x806] = (uint8_t *) MAP_BWRAM_BITMAP2;
			SA1.Map[c + 7] = SA1.Map[c + 0x807] = (uint8_t *) MAP_BWRAM_BITMAP2;
			SA1.WriteMap[c + 6] = SA1.WriteMap[c + 0x806] = (uint8_t *) MAP_BWRAM_BITMAP2;
			SA1.WriteMap[c + 7] = SA1.WriteMap[c + 0x807] = (uint8_t *) MAP_BWRAM_BITMAP2;
		}

		SA1.BWRAM = Memory.SRAM + (val & 0x7f) * 0x2000 / 4;
	}
	else
	{
		for ( c = 0; c < 0x400; c += 16)
		{
			SA1.Map[c + 6] = SA1.Map[c + 0x806] = (uint8_t *) MAP_BWRAM;
			SA1.Map[c + 7] = SA1.Map[c + 0x807] = (uint8_t *) MAP_BWRAM;
			SA1.WriteMap[c + 6] = SA1.WriteMap[c + 0x806] = (uint8_t *) MAP_BWRAM;
			SA1.WriteMap[c + 7] = SA1.WriteMap[c + 0x807] = (uint8_t *) MAP_BWRAM;
		}

		SA1.BWRAM = Memory.SRAM + (val & 7) * 0x2000;
	}
}

void S9xSA1PostLoadState (void)
{
	SA1.ShiftedPB = (uint32_t) SA1Registers.PB << 16;
	SA1.ShiftedDB = (uint32_t) SA1Registers.DB << 16;

	S9xSA1SetPCBase(SA1Registers.PBPC);
	S9xSA1UnpackStatus();
	S9xSA1FixCycles();
	SA1.VirtualBitmapFormat = (Memory.FillRAM[0x223f] & 0x80) ? 2 : 4;
	Memory.BWRAM = Memory.SRAM + (Memory.FillRAM[0x2224] & 7) * 0x2000;
	S9xSA1SetBWRAMMemMap(Memory.FillRAM[0x2225]);
}

static void S9xSetSA1MemMap (uint32_t which1, uint8_t map)
{
	unsigned c, i;
	unsigned start  = which1 * 0x100 + 0xc00;
	unsigned start2 = which1 * 0x200;

	if (which1 >= 2)
		start2 += 0x400;

	for (c = 0; c < 0x100; c += 16)
	{
		uint8_t	*block = &Memory.ROM[(map & 7) * 0x100000 + (c << 12)];
		for (i = c; i < c + 16; i++)
			Memory.Map[start  + i] = SA1.Map[start  + i] = block;
	}

	for (c = 0; c < 0x200; c += 16)
	{
		int32_t offset = (((map & 0x80) ? map : which1) & 7) * 0x100000 + (c << 11) - 0x8000;
		uint8_t *block = &Memory.ROM[offset];

		for (i = c + 8; i < c + 16; i++)
			Memory.Map[start2 + i] = SA1.Map[start2 + i] = block;
	}
}

static uint8_t S9xSA1GetByteFromRegister (uint8_t *GetAddress, uint32_t address);

/* S9xSA1GetByte: SA1 memory read fast-path wrapper.
 *
 * Folded to a statement-expression macro for the same reason
 * memory_speed and spc_cpu_read were folded: once the slow path
 * moved to an out-of-line *FromRegister helper, the wrapper itself
 * is small enough to inline everywhere, but GCC's inline cost
 * model still kept ~270 \`call S9xSA1GetByte\` instructions in the
 * production .so. A macro removes the optimizer's discretion.
 *
 * Plain expression macro: `address` is referenced four times and
 * the SA1.Map[] lookup is replayed three times. All current call
 * sites pass simple expressions (variables, struct members, +1
 * offsets) so the duplication is CSE'd by the compiler. Do NOT
 * pass side-effecting expressions through this macro. The earlier
 * statement-expression form captured into locals for safety but
 * relied on a GCC extension that older MSVC rejects. */
#define S9xSA1GetByte(address) \
	((SA1.Map[(((address) & 0xffffff) >> MEMMAP_SHIFT)] >= (uint8_t *) MAP_LAST) \
		? *(SA1.Map[(((address) & 0xffffff) >> MEMMAP_SHIFT)] + ((address) & 0xffff)) \
		: S9xSA1GetByteFromRegister(SA1.Map[(((address) & 0xffffff) >> MEMMAP_SHIFT)], (address)))

#ifdef __GNUC__
__attribute__((noinline))
#endif
static uint8_t S9xSA1GetByteFromRegister (uint8_t *GetAddress, uint32_t address)
{
	switch ((intptr_t) GetAddress)
	{
		case MAP_PPU:
			return (S9xGetSA1(address & 0xffff));

		case MAP_LOROM_SRAM:
		case MAP_SA1RAM:
			return (*(Memory.SRAM + (address & 0xffff)));

		case MAP_BWRAM:
			return (*(SA1.BWRAM + ((address & 0x7fff) - 0x6000)));

		case MAP_BWRAM_BITMAP:
			address -= 0x600000;
			if (SA1.VirtualBitmapFormat == 2)
				return ((Memory.SRAM[(address >> 2) & 0xffff] >> ((address & 3) << 1)) &  3);
			else
				return ((Memory.SRAM[(address >> 1) & 0xffff] >> ((address & 1) << 2)) & 15);

		case MAP_BWRAM_BITMAP2:
			address = (address & 0xffff) - 0x6000;
			if (SA1.VirtualBitmapFormat == 2)
				return ((SA1.BWRAM[(address >> 2) & 0xffff] >> ((address & 3) << 1)) &  3);
			else
				return ((SA1.BWRAM[(address >> 1) & 0xffff] >> ((address & 1) << 2)) & 15);

		default:
			return (SA1OpenBus);
	}
}

static INLINE uint16_t S9xSA1GetWord (uint32_t address, uint32_t w)
{
	PC_t	a;

	SA1OpenBus = S9xSA1GetByte(address);

	switch (w)
	{
		case WRAP_PAGE:
			a.xPBPC = address;
			a.B.xPCl++;
			return (SA1OpenBus | (S9xSA1GetByte(a.xPBPC) << 8));

		case WRAP_BANK:
			a.xPBPC = address;
			a.W.xPC++;
			return (SA1OpenBus | (S9xSA1GetByte(a.xPBPC) << 8));

		case WRAP_NONE:
		default:
			return (SA1OpenBus | (S9xSA1GetByte(address + 1) << 8));
	}
}

static void S9xSA1ReadVariableLengthData (uint8_t inc, uint8_t no_shift)
{
	uint32_t addr, data;
	uint8_t shift, s;

	addr  = Memory.FillRAM[0x2259] | (Memory.FillRAM[0x225a] << 8) | (Memory.FillRAM[0x225b] << 16);
	shift = Memory.FillRAM[0x2258] & 15;

	if (no_shift)
		shift = 0;
	else
	if (shift == 0)
		shift = 16;

	s = shift + SA1.variable_bit_pos;

	if (s >= 16)
	{
		addr += (s >> 4) << 1;
		s &= 15;
	}

	data = S9xSA1GetWord(addr, WRAP_NONE) | (S9xSA1GetWord(addr + 2, WRAP_NONE) << 16);

	data >>= s;
	Memory.FillRAM[0x230c] = (uint8_t) data;
	Memory.FillRAM[0x230d] = (uint8_t) (data >> 8);

	if (inc)
	{
		SA1.variable_bit_pos = (SA1.variable_bit_pos + shift) & 15;
		Memory.FillRAM[0x2259] = (uint8_t) addr;
		Memory.FillRAM[0x225a] = (uint8_t) (addr >> 8);
		Memory.FillRAM[0x225b] = (uint8_t) (addr >> 16);
	}
}

uint8_t S9xGetSA1 (uint32_t address)
{
	switch (address)
	{
		case 0x2300:
			return ((uint8_t) ((Memory.FillRAM[0x2209] & 0x5f) | (CPU.IRQActive & (SA1_IRQ_SOURCE | SA1_DMA_IRQ_SOURCE))));

		case 0x2301:
			return ((Memory.FillRAM[0x2200] & 0xf) | (Memory.FillRAM[0x2301] & 0xf0));

		case 0x2306:
			return ((uint8_t)  SA1.sum);

		case 0x2307:
			return ((uint8_t) (SA1.sum >>  8));

		case 0x2308:
			return ((uint8_t) (SA1.sum >> 16));

		case 0x2309:
			return ((uint8_t) (SA1.sum >> 24));

		case 0x230a:
			return ((uint8_t) (SA1.sum >> 32));

		case 0x230c:
			return (Memory.FillRAM[0x230c]);

		case 0x230d:
		{
			uint8_t	byte = Memory.FillRAM[0x230d];

			if (Memory.FillRAM[0x2258] & 0x80)
				S9xSA1ReadVariableLengthData(TRUE, FALSE);

			return (byte);
		}

		default:
			/*printf("R: %04x\n", address);*/
			break;
	}

	return (Memory.FillRAM[address]);
}

static void S9xSA1CharConv2 (void)
{
	uint32_t dest, offset;
	uint8_t *p, *q;
	int depth, bytes_per_char, l, b;

	dest           = Memory.FillRAM[0x2235] | (Memory.FillRAM[0x2236] << 8);
	offset         = (SA1.in_char_dma & 7) ? 0 : 1;
	depth          = (Memory.FillRAM[0x2231] & 3) == 0 ? 8 : (Memory.FillRAM[0x2231] & 3) == 1 ? 4 : 2;
	bytes_per_char = 8 * depth;
	p             = &Memory.FillRAM[0x3000] + (dest & 0x7ff) + offset * bytes_per_char;
	q             = &Memory.ROM[MAX_ROM_SIZE - 0x10000] + offset * 64;

	switch (depth)
	{
		case 2:
			for ( l = 0; l < 8; l++, q += 8)
			{
				for ( b = 0; b < 8; b++)
				{
					uint8_t	r = *(q + b);
					*(p +  0) = (*(p +  0) << 1) | (r & 1);
					*(p +  1) = (*(p +  1) << 1) | ((r >> 1) & 1);
				}

				p += 2;
			}

			break;

		case 4:
			for ( l = 0; l < 8; l++, q += 8)
			{
				for ( b = 0; b < 8; b++)
				{
					uint8_t	r = *(q + b);
					*(p +  0) = (*(p +  0) << 1) | (r & 1);
					*(p +  1) = (*(p +  1) << 1) | ((r >> 1) & 1);
					*(p + 16) = (*(p + 16) << 1) | ((r >> 2) & 1);
					*(p + 17) = (*(p + 17) << 1) | ((r >> 3) & 1);
				}

				p += 2;
			}

			break;

		case 8:
			for ( l = 0; l < 8; l++, q += 8)
			{
				for ( b = 0; b < 8; b++)
				{
					uint8_t	r = *(q + b);
					*(p +  0) = (*(p +  0) << 1) | (r & 1);
					*(p +  1) = (*(p +  1) << 1) | ((r >> 1) & 1);
					*(p + 16) = (*(p + 16) << 1) | ((r >> 2) & 1);
					*(p + 17) = (*(p + 17) << 1) | ((r >> 3) & 1);
					*(p + 32) = (*(p + 32) << 1) | ((r >> 4) & 1);
					*(p + 33) = (*(p + 33) << 1) | ((r >> 5) & 1);
					*(p + 48) = (*(p + 48) << 1) | ((r >> 6) & 1);
					*(p + 49) = (*(p + 49) << 1) | ((r >> 7) & 1);
				}

				p += 2;
			}

			break;
	}
}

static void S9xSA1DMA (void)
{
	uint32_t src, dst, len;
	uint8_t	*s, *d;

	src = Memory.FillRAM[0x2232] | (Memory.FillRAM[0x2233] << 8) | (Memory.FillRAM[0x2234] << 16);
	dst = Memory.FillRAM[0x2235] | (Memory.FillRAM[0x2236] << 8) | (Memory.FillRAM[0x2237] << 16);
	len = Memory.FillRAM[0x2238] | (Memory.FillRAM[0x2239] << 8);

	switch (Memory.FillRAM[0x2230] & 3)
	{
		case 0: /* ROM*/
			s = SA1.Map[((src & 0xffffff) >> MEMMAP_SHIFT)];
			if (s >= (uint8_t *) MAP_LAST)
				s += (src & 0xffff);
			else
				s = Memory.ROM + (src & 0xffff);
			break;

		case 1: /* BW-RAM*/
			src &= Memory.SRAMMask;
			len &= Memory.SRAMMask;
			s = Memory.SRAM + src;
			break;

		default:
		case 2:
			src &= 0x3ff;
			len &= 0x3ff;
			s = &Memory.FillRAM[0x3000] + src;
			break;
	}

	if (Memory.FillRAM[0x2230] & 4)
	{
		dst &= Memory.SRAMMask;
		len &= Memory.SRAMMask;
		d = Memory.SRAM + dst;
	}
	else
	{
		dst &= 0x3ff;
		len &= 0x3ff;
		d = &Memory.FillRAM[0x3000] + dst;
	}

	memmove(d, s, len);
	Memory.FillRAM[0x2301] |= 0x20;

	if (Memory.FillRAM[0x220a] & 0x20)
	{
		SA1.Flags |= IRQ_FLAG;
		SA1.IRQActive |= DMA_IRQ_SOURCE;
	}
}

void S9xSetSA1 (uint8_t byte, uint32_t address)
{
	switch (address)
	{
		case 0x2200:
			if (!(byte & 0x20) && (Memory.FillRAM[0x2200] & 0x20))
				S9xSA1Reset();

			if (byte & 0x80)
			{
				Memory.FillRAM[0x2301] |= 0x80;
				if (Memory.FillRAM[0x220a] & 0x80)
				{
					SA1.Flags |= IRQ_FLAG;
					SA1.IRQActive |= SNES_IRQ_SOURCE;
				}
			}

			if (byte & 0x10)
			{
				Memory.FillRAM[0x2301] |= 0x10;
				if (Memory.FillRAM[0x220a] & 0x10)
				{
					SA1.Flags |= NMI_FLAG;
				}
			}

			break;

		case 0x2201:
			if (((byte ^ Memory.FillRAM[0x2201]) & 0x80) && (Memory.FillRAM[0x2300] & byte & 0x80))
			{
				S9X_SET_IRQ(SA1_IRQ_SOURCE);
			}

			if (((byte ^ Memory.FillRAM[0x2201]) & 0x20) && (Memory.FillRAM[0x2300] & byte & 0x20))
			{
				S9X_SET_IRQ(SA1_DMA_IRQ_SOURCE);
			}

			break;

		case 0x2202:
			if (byte & 0x80)
			{
				Memory.FillRAM[0x2300] &= ~0x80;
				S9X_CLEAR_IRQ(SA1_IRQ_SOURCE);
			}

			if (byte & 0x20)
			{
				Memory.FillRAM[0x2300] &= ~0x20;
				S9X_CLEAR_IRQ(SA1_DMA_IRQ_SOURCE);
			}

			break;

		case 0x2203:
			/*printf("SA1 reset vector: %04x\n", byte | (Memory.FillRAM[0x2204] << 8));*/
		case 0x2204:
			/*printf("SA1 reset vector: %04x\n", (byte << 8) | Memory.FillRAM[0x2203]);*/
		case 0x2205:
			/*printf("SA1 NMI vector: %04x\n", byte | (Memory.FillRAM[0x2206] << 8));*/
		case 0x2206:
			/*printf("SA1 NMI vector: %04x\n", (byte << 8) | Memory.FillRAM[0x2205]);*/
		case 0x2207:
			/*printf("SA1 IRQ vector: %04x\n", byte | (Memory.FillRAM[0x2208] << 8));*/
		case 0x2208:
			/*printf("SA1 IRQ vector: %04x\n", (byte << 8) | Memory.FillRAM[0x2207]);*/
			break;
		case 0x2209:
			Memory.FillRAM[0x2209] = byte;

			if (byte & 0x80)
				Memory.FillRAM[0x2300] |= 0x80;

			if (byte & Memory.FillRAM[0x2201] & 0x80)
			{
				S9X_SET_IRQ(SA1_IRQ_SOURCE);
			}

			break;

		case 0x220a:
			if (((byte ^ Memory.FillRAM[0x220a]) & 0x80) && (Memory.FillRAM[0x2301] & byte & 0x80))
			{
				SA1.Flags |= IRQ_FLAG;
				SA1.IRQActive |= SNES_IRQ_SOURCE;
			}

			if (((byte ^ Memory.FillRAM[0x220a]) & 0x40) && (Memory.FillRAM[0x2301] & byte & 0x40))
			{
				SA1.Flags |= IRQ_FLAG;
				SA1.IRQActive |= TIMER_IRQ_SOURCE;
			}

			if (((byte ^ Memory.FillRAM[0x220a]) & 0x20) && (Memory.FillRAM[0x2301] & byte & 0x20))
			{
				SA1.Flags |= IRQ_FLAG;
				SA1.IRQActive |= DMA_IRQ_SOURCE;
			}

			if (((byte ^ Memory.FillRAM[0x220a]) & 0x10) && (Memory.FillRAM[0x2301] & byte & 0x10))
			{
				SA1.Flags |= NMI_FLAG;
			}

			break;

		case 0x220b:
			if (byte & 0x80)
			{
				SA1.IRQActive &= ~SNES_IRQ_SOURCE;
				Memory.FillRAM[0x2301] &= ~0x80;
			}

			if (byte & 0x40)
			{
				SA1.IRQActive &= ~TIMER_IRQ_SOURCE;
				Memory.FillRAM[0x2301] &= ~0x40;
			}

			if (byte & 0x20)
			{
				SA1.IRQActive &= ~DMA_IRQ_SOURCE;
				Memory.FillRAM[0x2301] &= ~0x20;
			}

			if (byte & 0x10)
				Memory.FillRAM[0x2301] &= ~0x10;

			if (!SA1.IRQActive)
				SA1.Flags &= ~IRQ_FLAG;

			break;

		case 0x220c:
			/*printf("SNES NMI vector: %04x\n", byte | (Memory.FillRAM[0x220d] << 8));*/
		case 0x220d:
			/*printf("SNES NMI vector: %04x\n", (byte << 8) | Memory.FillRAM[0x220c]);*/
		case 0x220e:
			/*printf("SNES IRQ vector: %04x\n", byte | (Memory.FillRAM[0x220f] << 8));*/
		case 0x220f:
			/*printf("SNES IRQ vector: %04x\n", (byte << 8) | Memory.FillRAM[0x220e]);*/
		case 0x2210:
		case 0x2211:
			/*printf("Timer reset\n");*/
		case 0x2212:
			/*printf("H-Timer %04x\n", byte | (Memory.FillRAM[0x2213] << 8));*/
		case 0x2213:
			/*printf("H-Timer %04x\n", (byte << 8) | Memory.FillRAM[0x2212]);*/
		case 0x2214:
			/*printf("V-Timer %04x\n", byte | (Memory.FillRAM[0x2215] << 8));*/
		case 0x2215:
			/*printf("V-Timer %04x\n", (byte << 8) | Memory.FillRAM[0x2214]);*/
			break;
		case 0x2220:
		case 0x2221:
		case 0x2222:
		case 0x2223:
			/*printf("MMC: %02x\n", byte);*/
			S9xSetSA1MemMap(address - 0x2220, byte);
			break;

		case 0x2224:
			/*printf("BWRAM image SNES %02x -> 0x6000\n", byte);*/
			Memory.BWRAM = Memory.SRAM + (byte & 7) * 0x2000;
			break;

		case 0x2225:
			/*printf("BWRAM image SA1 %02x -> 0x6000 (%02x)\n", byte, Memory.FillRAM[0x2225]);*/
			if (byte != Memory.FillRAM[0x2225])
				S9xSA1SetBWRAMMemMap(byte);
			break;

		case 0x2226:
			/*printf("BW-RAM SNES write %s\n", (byte & 0x80) ? "enabled" : "disabled");*/
		case 0x2227:
			/*printf("BW-RAM SA1 write %s\n", (byte & 0x80) ? "enabled" : "disabled");*/
		case 0x2228:
			/*printf("BW-RAM write protect area %02x\n", byte);*/
		case 0x2229:
			/*printf("I-RAM SNES write protect area %02x\n", byte);*/
		case 0x222a:
			/*printf("I-RAM SA1 write protect area %02x\n", byte);*/
		case 0x2230:
			break;
		case 0x2231:
			if (byte & 0x80)
				SA1.in_char_dma = FALSE;
			break;

		case 0x2232:
		case 0x2233:
		case 0x2234:
			Memory.FillRAM[address] = byte;
			break;

		case 0x2235:
			Memory.FillRAM[0x2235] = byte;
			break;

		case 0x2236:
			Memory.FillRAM[0x2236] = byte;

			if ((Memory.FillRAM[0x2230] & 0xa4) == 0x80) /* Normal DMA to I-RAM*/
				S9xSA1DMA();
			else
			if ((Memory.FillRAM[0x2230] & 0xb0) == 0xb0)
			{
				Memory.FillRAM[0x2300] |= 0x20;
				if (Memory.FillRAM[0x2201] & 0x20)
				{
					S9X_SET_IRQ(SA1_DMA_IRQ_SOURCE);
				}
				SA1.in_char_dma = TRUE;
			}

			break;

		case 0x2237:
			Memory.FillRAM[0x2237] = byte;

			if ((Memory.FillRAM[0x2230] & 0xa4) == 0x84) /* Normal DMA to BW-RAM*/
				S9xSA1DMA();
			break;

		case 0x2238:
		case 0x2239:
			Memory.FillRAM[address] = byte;
			break;

		case 0x223f:
			/*printf("virtual VRAM depth %d\n", (byte & 0x80) ? 2 : 4);*/
			SA1.VirtualBitmapFormat = (byte & 0x80) ? 2 : 4;
			break;

		case 0x2240:
		case 0x2241:
		case 0x2242:
		case 0x2243:
		case 0x2244:
		case 0x2245:
		case 0x2246:
		case 0x2247:
		case 0x2248:
		case 0x2249:
		case 0x224a:
		case 0x224b:
		case 0x224c:
		case 0x224d:
		case 0x224e:
			Memory.FillRAM[address] = byte;
			break;

		case 0x224f:
			Memory.FillRAM[0x224f] = byte;

			if ((Memory.FillRAM[0x2230] & 0xb0) == 0xa0) /* Char conversion 2 DMA enabled*/
			{
				memcpy(&Memory.ROM[MAX_ROM_SIZE - 0x10000] + SA1.in_char_dma * 16, &Memory.FillRAM[0x2240], 16);
				SA1.in_char_dma = (SA1.in_char_dma + 1) & 7;
				if ((SA1.in_char_dma & 3) == 0)
					S9xSA1CharConv2();
			}

			break;

		case 0x2250:
			if (byte & 2)
				SA1.sum = 0;
			SA1.arithmetic_op = byte & 3;
			break;

		case 0x2251:
			SA1.op1 = (SA1.op1 & 0xff00) | byte;
			break;

		case 0x2252:
			SA1.op1 = (SA1.op1 & 0xff) | (byte << 8);
			break;

		case 0x2253:
			SA1.op2 = (SA1.op2 & 0xff00) | byte;
			break;

		case 0x2254:
			SA1.op2 = (SA1.op2 & 0xff) | (byte << 8);

			switch (SA1.arithmetic_op)
			{
				case 0:	/* multiply*/
					SA1.sum = SA1.op1 * SA1.op2;
					break;

				case 1: /* divide*/
					if (SA1.op2 == 0)
						SA1.sum = SA1.op1 << 16;
					else
						SA1.sum = (SA1.op1 / (int) ((uint16_t) SA1.op2)) | ((SA1.op1 % (int) ((uint16_t) SA1.op2)) << 16);
					break;

				case 2: /* cumulative sum*/
				default:
					SA1.sum += SA1.op1 * SA1.op2;
					if (SA1.sum & ((int64_t) 0xffffff << 32))
						SA1.overflow = TRUE;
					break;
			}

			break;

		case 0x2258: /* Variable bit-field length/auto inc/start.*/
			Memory.FillRAM[0x2258] = byte;
			S9xSA1ReadVariableLengthData(TRUE, FALSE);
			return;

		case 0x2259: /* Variable bit-field start address*/
		case 0x225a:
		case 0x225b:
			Memory.FillRAM[address] = byte;
			/* XXX: ???*/
			SA1.variable_bit_pos = 0;
			S9xSA1ReadVariableLengthData(FALSE, TRUE);
			return;

		default:
			/*printf("W: %02x->%04x\n", byte, address);*/
			break;
	}

	if (address >= 0x2200 && address <= 0x22ff)
		Memory.FillRAM[address] = byte;
}



static void S9xSA1SetByteToRegister (uint8_t byte, uint8_t *SetAddress, uint32_t address);

/* S9xSA1SetByte: SA1 memory write fast-path wrapper.
 *
 * Folded to a do-while(0) macro (rather than a statement expression
 * — there's no return value to produce, and do-while-0 is plain C
 * without needing __extension__). Both arguments are captured into
 * locals so the macro is multiple-evaluation-safe. */
#define S9xSA1SetByte(byte, address) do { \
	uint8_t   _sa1_sb_byte = (byte); \
	uint32_t  _sa1_sb_addr = (address); \
	uint8_t  *_sa1_sb_p    = SA1.WriteMap[(_sa1_sb_addr & 0xffffff) >> MEMMAP_SHIFT]; \
	if (_sa1_sb_p >= (uint8_t *) MAP_LAST) \
		*(_sa1_sb_p + (_sa1_sb_addr & 0xffff)) = _sa1_sb_byte; \
	else \
		S9xSA1SetByteToRegister(_sa1_sb_byte, _sa1_sb_p, _sa1_sb_addr); \
} while (0)

#ifdef __GNUC__
__attribute__((noinline))
#endif
static void S9xSA1SetByteToRegister (uint8_t byte, uint8_t *SetAddress, uint32_t address)
{
	switch ((intptr_t) SetAddress)
	{
		case MAP_PPU:
			S9xSetSA1(byte, address & 0xffff);
			return;

		case MAP_LOROM_SRAM:
		case MAP_SA1RAM:
			*(Memory.SRAM + (address & 0xffff)) = byte;
			return;

		case MAP_BWRAM:
			*(SA1.BWRAM + ((address & 0x7fff) - 0x6000)) = byte;
			return;

		case MAP_BWRAM_BITMAP:
			address -= 0x600000;
			if (SA1.VirtualBitmapFormat == 2)
			{
				uint8_t	*ptr = &Memory.SRAM[(address >> 2) & 0xffff];
				*ptr &= ~(3  << ((address & 3) << 1));
				*ptr |= (byte &  3) << ((address & 3) << 1);
			}
			else
			{
				uint8_t	*ptr = &Memory.SRAM[(address >> 1) & 0xffff];
				*ptr &= ~(15 << ((address & 1) << 2));
				*ptr |= (byte & 15) << ((address & 1) << 2);
			}

			return;

		case MAP_BWRAM_BITMAP2:
			address = (address & 0xffff) - 0x6000;
			if (SA1.VirtualBitmapFormat == 2)
			{
				uint8_t	*ptr = &SA1.BWRAM[(address >> 2) & 0xffff];
				*ptr &= ~(3  << ((address & 3) << 1));
				*ptr |= (byte &  3) << ((address & 3) << 1);
			}
			else
			{
				uint8_t	*ptr = &SA1.BWRAM[(address >> 1) & 0xffff];
				*ptr &= ~(15 << ((address & 1) << 2));
				*ptr |= (byte & 15) << ((address & 1) << 2);
			}

			return;

		default:
			return;
	}
}

static INLINE void S9xSA1SetWord_Write0(uint16_t Word, uint32_t address, uint32_t w)
{
	PC_t	a;

	S9xSA1SetByte((uint8_t) Word, address);

	switch (w)
	{
		case WRAP_PAGE:
			a.xPBPC = address;
			a.B.xPCl++;
			S9xSA1SetByte(Word >> 8, a.xPBPC);
			break;

		case WRAP_BANK:
			a.xPBPC = address;
			a.W.xPC++;
			S9xSA1SetByte(Word >> 8, a.xPBPC);
			break;

		case WRAP_NONE:
		default:
			S9xSA1SetByte(Word >> 8, address + 1);
			break;
	}
}

static INLINE void S9xSA1SetWord_Write1(uint16_t Word, uint32_t address, uint32_t w)
{
	PC_t	a;

	switch (w)
	{
		case WRAP_PAGE:
			a.xPBPC = address;
			a.B.xPCl++;
			S9xSA1SetByte(Word >> 8, a.xPBPC);
			break;

		case WRAP_BANK:
			a.xPBPC = address;
			a.W.xPC++;
			S9xSA1SetByte(Word >> 8, a.xPBPC);
			break;

		case WRAP_NONE:
		default:
			S9xSA1SetByte(Word >> 8, address + 1);
			break;
	}

	S9xSA1SetByte((uint8_t) Word, address);
}

#define CPU						SA1
#define ICPU						SA1
#define Registers					SA1Registers
#define OpenBus						SA1OpenBus
#define S9xGetByte					S9xSA1GetByte
#define S9xGetWord					S9xSA1GetWord
#define S9xSetByte					S9xSA1SetByte
#define S9xSetWord_Write0				S9xSA1SetWord_Write0
#define S9xSetWord_Write1				S9xSA1SetWord_Write1
#define S9xSetPCBase					S9xSA1SetPCBase
#define S9xOpcodesM1X1					S9xSA1OpcodesM1X1
#define S9xOpcodesM1X0					S9xSA1OpcodesM1X0
#define S9xOpcodesM0X1					S9xSA1OpcodesM0X1
#define S9xOpcodesM0X0					S9xSA1OpcodesM0X0
#define S9xOpcodesE1					S9xSA1OpcodesE1
#define S9xOpcodesSlow					S9xSA1OpcodesSlow
#define S9xOpcode_IRQ					S9xSA1Opcode_IRQ
#define S9xOpcode_NMI					S9xSA1Opcode_NMI
#undef S9xUnpackStatus
#define S9xUnpackStatus					S9xSA1UnpackStatus
#undef S9xPackStatus
#define S9xPackStatus					S9xSA1PackStatus
#define S9xFixCycles					S9xSA1FixCycles
#define Immediate8					SA1Immediate8
#define Immediate16					SA1Immediate16
#define Relative					SA1Relative
#define RelativeLong					SA1RelativeLong
#define Absolute					SA1Absolute
#define AbsoluteLong					SA1AbsoluteLong
#define AbsoluteIndirect				SA1AbsoluteIndirect
#define AbsoluteIndirectLong				SA1AbsoluteIndirectLong
#define AbsoluteIndexedIndirect				SA1AbsoluteIndexedIndirect
#define Direct						SA1Direct
#define DirectIndirectIndexed				SA1DirectIndirectIndexed
#define DirectIndirectIndexedLong			SA1DirectIndirectIndexedLong
#define DirectIndexedIndirect				SA1DirectIndexedIndirect
#define DirectIndexedX					SA1DirectIndexedX
#define DirectIndexedY					SA1DirectIndexedY
#define AbsoluteIndexedX				SA1AbsoluteIndexedX
#define AbsoluteIndexedY				SA1AbsoluteIndexedY
#define AbsoluteLongIndexedX				SA1AbsoluteLongIndexedX
#define DirectIndirect					SA1DirectIndirect
#define DirectIndirectLong				SA1DirectIndirectLong
#define StackRelative					SA1StackRelative
#define StackRelativeIndirectIndexed			SA1StackRelativeIndirectIndexed

#define SA1_OPCODES

#include "cpuops_.h"

void S9xSA1MainLoop (void)
{
	int i;
	uint8_t sa1_quit;

	if (SA1.Flags & NMI_FLAG)
	{
		if (Memory.FillRAM[0x2200] & 0x10)
		{
			SA1.Flags &= ~NMI_FLAG;
			Memory.FillRAM[0x2301] |= 0x10;

			if (SA1.WaitingForInterrupt)
			{
				SA1.WaitingForInterrupt = FALSE;
				SA1Registers.PCw++;
			}

			S9xSA1Opcode_NMI();
		}
	}

	if (SA1.Flags & IRQ_FLAG)
	{
		if (SA1.IRQActive)
		{
			if (SA1.WaitingForInterrupt)
			{
				SA1.WaitingForInterrupt = FALSE;
				SA1Registers.PCw++;
			}

			if (!SA1CheckFlag(IRQ))
				S9xSA1Opcode_IRQ();
		}
		else
			SA1.Flags &= ~IRQ_FLAG;
	}

	sa1_quit = Memory.FillRAM[0x2200] & 0x60;

	for ( i = 0; i < 3 && !sa1_quit; i++)
	{
		register uint8_t			Op;
		register struct SOpcodes	*Opcodes;

		if (SA1.PCBase)
		{
			SA1OpenBus = Op = SA1.PCBase[Registers.PCw];
			Opcodes = SA1.S9xOpcodes;
		}
		else
		{
			Op = S9xSA1GetByte(Registers.PBPC);
			Opcodes = S9xOpcodesSlow;
		}

		if ((SA1Registers.PCw & MEMMAP_MASK) + SA1.S9xOpLengths[Op] >= MEMMAP_BLOCK_SIZE)
		{
			uint32_t	oldPC = SA1Registers.PBPC;
			S9xSA1SetPCBase(SA1Registers.PBPC);
			SA1Registers.PBPC = oldPC;
			Opcodes = S9xSA1OpcodesSlow;
		}

		Registers.PCw++;
		(*Opcodes[Op].S9xOpcode)();
	}
}
