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

  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004  Marcus Comstedt (marcus@mc.pp.se)

  NTSC filter
  (c) Copyright 2006 - 2007  Shay Green

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


#ifndef _GETSET_H_
#define _GETSET_H_

#include "cpuexec.h"
#include "dsp.h"
#include "sa1.h"
#include "spc7110.h"
#include "c4.h"
#include "obc1.h"
#include "seta.h"
#include "bsx.h"
#include "port.h"

extern uint8_t	OpenBus;

uint8_t S9xGetByteFromRegister(uint8_t *GetAddress, uint32_t Address);
uint16_t S9xGetWordFromRegister(uint8_t *GetAddress, uint32_t Address);
void S9xSetByteToRegister(uint8_t Byte, uint8_t *SetAddress, uint32_t Address);
void S9xSetWordToRegister_Write0(uint16_t Word, uint8_t *SetAddress, uint32_t Address);
void S9xSetWordToRegister_Write1(uint16_t Word, uint8_t *SetAddress, uint32_t Address);

static INLINE uint8_t S9xGetByte (uint32_t Address)
{
   int		block    = (Address & 0xffffff) >> MEMMAP_SHIFT;
	uint8_t	*GetAddress = Memory.Map[block];

	if (GetAddress >= (uint8_t *) MAP_LAST)
	{
      uint8_t byte;
      int32_t    speed    = memory_speed(Address);
	#ifdef CPU_SHUTDOWN
		if (Memory.BlockIsRAM[block])
			CPU.WaitAddress = CPU.PBPCAtOpcodeStart;
	#endif
		byte = *(GetAddress + (Address & 0xffff));
		addCyclesInMemoryAccess;
		return (byte);
	}

   return S9xGetByteFromRegister(GetAddress, Address);
}
 
static INLINE uint16_t S9xGetWord (uint32_t Address, uint32_t w)
{
	uint8_t  *GetAddress;
	int       block;
	uint16_t  word;
	int32_t   speed;
	uint32_t  mask = MEMMAP_MASK & (w == WRAP_PAGE ? 0xff : (w == WRAP_BANK ? 0xffff : 0xffffff));

	/* Page- or bank-crossing word read: split into two byte reads. */
	if ((Address & mask) == mask)
	{
		PC_t     a;
		uint32_t addr_tmp = Address + 1;

		OpenBus = S9xGetByte(Address);

		switch (w)
		{
			case WRAP_PAGE:
				a.xPBPC = Address;
				a.B.xPCl++;
				addr_tmp = a.xPBPC;
				break;
			case WRAP_BANK:
				a.xPBPC = Address;
				a.W.xPC++;
				addr_tmp = a.xPBPC;
				break;
		}
		return (OpenBus | (S9xGetByte(addr_tmp) << 8));
	}

	block      = (Address & 0xffffff) >> MEMMAP_SHIFT;
	GetAddress = Memory.Map[block];

	/* Fast path: direct RAM/ROM word read. */
	if (GetAddress >= (uint8_t *) MAP_LAST)
	{
	#ifdef CPU_SHUTDOWN
		if (Memory.BlockIsRAM[block])
			CPU.WaitAddress = CPU.PBPCAtOpcodeStart;
	#endif
		word  = READ_WORD(GetAddress + (Address & 0xffff));
		speed = memory_speed(Address);
		addCyclesInMemoryAccess_x2;
		return (word);
	}

	/* Slow path: memory-mapped registers, mirrored SRAM, special chips. */
	return S9xGetWordFromRegister(GetAddress, Address);
}

static INLINE void S9xSetByte (uint8_t Byte, uint32_t Address)
{
	int      block      = (Address & 0xffffff) >> MEMMAP_SHIFT;
	uint8_t *SetAddress = Memory.WriteMap[block];

#ifdef CPU_SHUTDOWN
	CPU.WaitAddress = 0xffffffff;
#endif

	/* Fast path: direct RAM/ROM write. */
	if (SetAddress >= (uint8_t *) MAP_LAST)
	{
		int32_t speed = memory_speed(Address);
	#ifdef CPU_SHUTDOWN
		SetAddress += (Address & 0xffff);
		*SetAddress = Byte;
		addCyclesInMemoryAccess;

		if (Settings.SA1)
		{
			if (SetAddress == SA1.WaitByteAddress1 || SetAddress == SA1.WaitByteAddress2)
			{
				SA1.Executing = SA1.S9xOpcodes != NULL;
				SA1.WaitCounter = 0;
			}
		}
	#else
		*(SetAddress + (Address & 0xffff)) = Byte;
		addCyclesInMemoryAccess;
	#endif
		return;
	}

	/* Slow path: memory-mapped registers, mirrored SRAM, special chips. */
	S9xSetByteToRegister(Byte, SetAddress, Address);
}

static INLINE void S9xSetWord_Write0(uint16_t Word, uint32_t Address, uint32_t w)
{
	int       block;
	uint8_t  *SetAddress;
	int32_t   speed;

	/* Page- or bank-crossing word write: split into two byte writes
	 * (low byte first for Write0 semantics). */
	if ((Address & (MEMMAP_MASK & w)) == (MEMMAP_MASK & w))
	{
		PC_t     a;
		uint32_t addr_tmp = Address + 1;

		S9xSetByte((uint8_t) Word, Address);

		switch (w)
		{
			case WRAP_PAGE:
				a.xPBPC = Address;
				a.B.xPCl++;
				addr_tmp = a.xPBPC;
				break;
			case WRAP_BANK:
				a.xPBPC = Address;
				a.W.xPC++;
				addr_tmp = a.xPBPC;
				break;
		}

		S9xSetByte(Word >> 8, addr_tmp);
		return;
	}

	block      = (Address & 0xffffff) >> MEMMAP_SHIFT;
	SetAddress = Memory.WriteMap[block];

	/* Fast path: direct RAM/ROM word write. */
	if (SetAddress >= (uint8_t *) MAP_LAST)
	{
		WRITE_WORD(SetAddress + (Address & 0xffff), Word);
		speed = memory_speed(Address);
		addCyclesInMemoryAccess_x2;
		return;
	}

	/* Slow path. */
	S9xSetWordToRegister_Write0(Word, SetAddress, Address);
}


static INLINE void S9xSetWord_Write1(uint16_t Word, uint32_t Address, uint32_t w)
{
	int       block;
	uint8_t  *SetAddress;
	int32_t   speed;

	/* Page- or bank-crossing word write: split into two byte writes
	 * (high byte first for Write1 semantics). */
	if ((Address & (MEMMAP_MASK & w)) == (MEMMAP_MASK & w))
	{
		PC_t     a;
		uint32_t addr_tmp = Address + 1;

		switch (w)
		{
			case WRAP_PAGE:
				a.xPBPC = Address;
				a.B.xPCl++;
				addr_tmp = a.xPBPC;
				break;
			case WRAP_BANK:
				a.xPBPC = Address;
				a.W.xPC++;
				addr_tmp = a.xPBPC;
				break;
		}
		S9xSetByte(Word >> 8, addr_tmp);
		S9xSetByte((uint8_t) Word, Address);
		return;
	}

	block      = (Address & 0xffffff) >> MEMMAP_SHIFT;
	SetAddress = Memory.WriteMap[block];

	/* Fast path: direct RAM/ROM word write. */
	if (SetAddress >= (uint8_t *) MAP_LAST)
	{
		WRITE_WORD(SetAddress + (Address & 0xffff), Word);
		speed = memory_speed(Address);
		addCyclesInMemoryAccess_x2;
		return;
	}

	/* Slow path. */
	S9xSetWordToRegister_Write1(Word, SetAddress, Address);
}

/* S9xSetPCBase_cold: special-chip MAP_* dispatch for S9xSetPCBase.
 *
 * Pulled out of the hot wrapper so the wrapper - which is the
 * branch that runs on every JSR / JSL / RTS / RTL / BRK / COP /
 * NMI / IRQ / BBR / BBS / JMP / JML when PC ends up in ROM or
 * work-RAM (the common case) - is small enough for the inliner to
 * consider at small caller sites. The original combined form was
 * 811 B; the 9 switch cases below (SRAM, BWRAM, SA1 RAM, SPC7110,
 * C4, OBC1, BSX) dominate the body and are runtime-cold for
 * normal gameplay code, since PC almost always points at a region
 * whose Map[] entry is a direct memory pointer (>= MAP_LAST).
 *
 * __attribute__((noinline)) is mandatory: without it the optimizer
 * merges this back into the wrapper, defeating the split. Same
 * pattern as HWREG_NOINLINE in hwregisters.c and the explicit
 * noinline on spc_cpu_write_io. */
#ifdef __GNUC__
__attribute__((noinline))
#endif
static void S9xSetPCBase_cold (uint32_t Address, uint8_t *GetAddress)
{
	CPU.PCBase = NULL;

	if ((intptr_t) GetAddress == MAP_NONE)
		return;

	switch ((intptr_t) GetAddress)
	{
		case MAP_LOROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) == MEMMAP_MASK)
				CPU.PCBase = Memory.SRAM + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask) - (Address & 0xffff);
			break;

		case MAP_LOROM_SRAM_B:
			if ((Multi.sramMaskB & MEMMAP_MASK) == MEMMAP_MASK)
				CPU.PCBase = Multi.sramB + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Multi.sramMaskB) - (Address & 0xffff);
			break;

		case MAP_HIROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) == MEMMAP_MASK)
				CPU.PCBase = Memory.SRAM + (((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask) - (Address & 0xffff);
			break;

		case MAP_BWRAM:
			CPU.PCBase = Memory.BWRAM - 0x6000 - (Address & 0x8000);
			break;

		case MAP_SA1RAM:
			CPU.PCBase = Memory.SRAM;
			break;

		case MAP_SPC7110_ROM:
			CPU.PCBase = S9xGetBasePointerSPC7110(Address);
			break;

		case MAP_C4:
			CPU.PCBase = S9xGetBasePointerC4(Address & 0xffff);
			break;

		case MAP_OBC_RAM:
			CPU.PCBase = S9xGetBasePointerOBC1(Address & 0xffff);
			break;

		case MAP_BSX:
			CPU.PCBase = S9xGetBasePointerBSX(Address);
			break;
	}
}

static INLINE void S9xSetPCBase (uint32_t Address)
{
	int block;
	uint8_t *GetAddress;

	Registers.PBPC = Address & 0xffffff;
	ICPU.ShiftedPB = Address & 0xff0000;

	block          = ((Address & 0xffffff) >> MEMMAP_SHIFT);
	GetAddress     = Memory.Map[block];

	CPU.MemSpeed   = memory_speed(Address);
	CPU.MemSpeedx2 = CPU.MemSpeed << 1;

	if (GetAddress >= (uint8_t *) MAP_LAST)
	{
		CPU.PCBase = GetAddress;
		return;
	}

	S9xSetPCBase_cold(Address, GetAddress);
}

static INLINE uint8_t * S9xGetBasePointer (uint32_t Address)
{
	uint8_t *GetAddress = Memory.Map[(Address & 0xffffff) >> MEMMAP_SHIFT];

	if (GetAddress >= (uint8_t *) MAP_LAST)
		return (GetAddress);

	switch ((intptr_t) GetAddress)
	{
		case MAP_LOROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) == MEMMAP_MASK)
            return (Memory.SRAM + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask) - (Address & 0xffff));
         break;

		case MAP_LOROM_SRAM_B:
			if ((Multi.sramMaskB & MEMMAP_MASK) == MEMMAP_MASK)
            return (Multi.sramB + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Multi.sramMaskB) - (Address & 0xffff));
         break;

		case MAP_HIROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) == MEMMAP_MASK)
            return (Memory.SRAM + (((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask) - (Address & 0xffff));
         break;

		case MAP_BWRAM:
			return (Memory.BWRAM - 0x6000 - (Address & 0x8000));

		case MAP_SA1RAM:
			return (Memory.SRAM);

		case MAP_SPC7110_ROM:
			return (S9xGetBasePointerSPC7110(Address));

		case MAP_C4:
			return (S9xGetBasePointerC4(Address & 0xffff));

		case MAP_OBC_RAM:
			return (S9xGetBasePointerOBC1(Address & 0xffff));

		case MAP_NONE:
		default:
         break;
	}
   return (NULL);
}

static INLINE uint8_t * S9xGetMemPointer (uint32_t Address)
{
	uint8_t *GetAddress = Memory.Map[(Address & 0xffffff) >> MEMMAP_SHIFT];

	if (GetAddress >= (uint8_t *) MAP_LAST)
		return (GetAddress + (Address & 0xffff));

	switch ((intptr_t) GetAddress)
	{
		case MAP_LOROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) == MEMMAP_MASK)
            return (Memory.SRAM + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask));
         break;
		case MAP_LOROM_SRAM_B:
			if ((Multi.sramMaskB & MEMMAP_MASK) == MEMMAP_MASK)
            return (Multi.sramB + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Multi.sramMaskB));
         break;
		case MAP_HIROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) == MEMMAP_MASK)
            return (Memory.SRAM + (((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask));
         break;

		case MAP_BWRAM:
			return (Memory.BWRAM - 0x6000 + (Address & 0x7fff));

		case MAP_SA1RAM:
			return (Memory.SRAM + (Address & 0xffff));

		case MAP_SPC7110_ROM:
			return (S9xGetBasePointerSPC7110(Address) + (Address & 0xffff));

		case MAP_C4:
			return (S9xGetMemPointerC4(Address & 0xffff));

		case MAP_OBC_RAM:
			return (S9xGetMemPointerOBC1(Address & 0xffff));

		case MAP_NONE:
		default:
         break;
	}

	return (NULL);
}

#endif
