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

#include "port.h"
#include "snes9x.h"
#include "cpuexec.h"
#include "dsp.h"
#include "sa1.h"
#include "spc7110.h"
#include "c4.h"
#include "obc1.h"
#include "seta.h"
#include "bsx.h"
#include "getset.h"

/* The five out-of-line slow-path helpers below MUST stay out of
 * line. They each have exactly one or two callers (the matching
 * inline wrappers in getset.h), and without an explicit noinline
 * GCC's IPA inliner notices the single-caller case and folds the
 * 200-700 byte switch body back into the wrapper - which then
 * grows past the inline threshold and stops being inlined at its
 * own call sites, defeating the entire hot/cold split.
 *
 * S9xSetByteToRegister was the case that exposed this: with no
 * noinline, the inliner merged it into S9xSetByte and grew that
 * wrapper to 848 B (the slow body inlined), at which point GCC
 * refused to inline S9xSetByte at hundreds of call sites. */
#ifdef __GNUC__
#define HWREG_NOINLINE __attribute__((noinline))
#else
#define HWREG_NOINLINE
#endif

extern uint8_t	OpenBus;

HWREG_NOINLINE uint8_t S9xGetByteFromRegister(uint8_t *GetAddress, uint32_t Address)
{
   uint8_t	byte;
   int32_t    speed    = memory_speed(Address);

   switch ((intptr_t) GetAddress)
   {
      case MAP_CPU:
         byte = S9xGetCPU(Address & 0xffff);
         break;

      case MAP_PPU:
         if (CPU.InDMAorHDMA && (Address & 0xff00) == 0x2100)
            return (OpenBus);

         byte = S9xGetPPU(Address & 0xffff);
         break;

      case MAP_LOROM_SRAM:
      case MAP_SA1RAM:
         // Address & 0x7fff   : offset into bank
         // Address & 0xff0000 : bank
         // bank >> 1 | offset : SRAM address, unbound
         // unbound & SRAMMask : SRAM offset
         byte = *(Memory.SRAM + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask));
         break;

      case MAP_LOROM_SRAM_B:
         byte = *(Multi.sramB + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Multi.sramMaskB));
         break;

      case MAP_HIROM_SRAM:
      case MAP_RONLY_SRAM:
         byte = *(Memory.SRAM + (((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask));
         break;

      case MAP_BWRAM:
         byte = *(Memory.BWRAM + ((Address & 0x7fff) - 0x6000));
         break;

      case MAP_DSP:
         byte = S9xGetDSP(Address & 0xffff);
         break;

      case MAP_SPC7110_ROM:
         byte = S9xGetSPC7110Byte(Address);
         break;

      case MAP_SPC7110_DRAM:
         byte = S9xGetSPC7110(0x4800);
         break;

      case MAP_C4:
         byte = S9xGetC4(Address & 0xffff);
         break;

      case MAP_OBC_RAM:
         byte = S9xGetOBC1(Address & 0xffff);
         break;

      case MAP_SETA_DSP:
         byte = S9xGetSetaDSP(Address);
         break;

      case MAP_SETA_RISC:
         byte = S9xGetST018(Address);
         break;

      case MAP_BSX:
         byte = S9xGetBSX(Address);
         break;

      case MAP_NONE:
      default:
         byte = OpenBus;
         break;
   }

   addCyclesInMemoryAccess;
   return (byte);
}

/* Out-of-line slow path for S9xGetWord. The inline wrapper in
 * getset.h handles the page/bank-cross fast path and the direct
 * RAM/ROM word read; this function handles the MAP_* register and
 * special-memory cases. Splitting the body keeps the inline portion
 * small enough that GCC will actually inline it at the ~679 call
 * sites it previously refused to. */
HWREG_NOINLINE uint16_t S9xGetWordFromRegister(uint8_t *GetAddress, uint32_t Address)
{
	uint16_t word;
	int32_t  speed = memory_speed(Address);

	switch ((intptr_t) GetAddress)
	{
		case MAP_CPU:
			word  = S9xGetCPU(Address & 0xffff);
			addCyclesInMemoryAccess;
			word |= S9xGetCPU((Address + 1) & 0xffff) << 8;
			addCyclesInMemoryAccess;
			break;

		case MAP_PPU:
			if (CPU.InDMAorHDMA)
			{
				OpenBus = S9xGetByte(Address);
				return (OpenBus | (S9xGetByte(Address + 1) << 8));
			}

			word  = S9xGetPPU(Address & 0xffff);
			addCyclesInMemoryAccess;
			word |= S9xGetPPU((Address + 1) & 0xffff) << 8;
			addCyclesInMemoryAccess;
			break;

		case MAP_LOROM_SRAM:
		case MAP_SA1RAM:
			if (Memory.SRAMMask >= MEMMAP_MASK)
				word = READ_WORD(Memory.SRAM + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask));
			else
				word = (*(Memory.SRAM + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask))) |
				      ((*(Memory.SRAM + (((((Address + 1) & 0xff0000) >> 1) | ((Address + 1) & 0x7fff)) & Memory.SRAMMask))) << 8);
			addCyclesInMemoryAccess_x2;
			break;

		case MAP_LOROM_SRAM_B:
			if (Multi.sramMaskB >= MEMMAP_MASK)
				word = READ_WORD(Multi.sramB + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Multi.sramMaskB));
			else
				word = (*(Multi.sramB + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Multi.sramMaskB))) |
				      ((*(Multi.sramB + (((((Address + 1) & 0xff0000) >> 1) | ((Address + 1) & 0x7fff)) & Multi.sramMaskB))) << 8);
			addCyclesInMemoryAccess_x2;
			break;

		case MAP_HIROM_SRAM:
		case MAP_RONLY_SRAM:
			if (Memory.SRAMMask >= MEMMAP_MASK)
				word = READ_WORD(Memory.SRAM + (((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask));
			else
				word = (*(Memory.SRAM + (((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask)) |
				       (*(Memory.SRAM + ((((Address + 1) & 0x7fff) - 0x6000 + (((Address + 1) & 0xf0000) >> 3)) & Memory.SRAMMask)) << 8));
			addCyclesInMemoryAccess_x2;
			break;

		case MAP_BWRAM:
			word = READ_WORD(Memory.BWRAM + ((Address & 0x7fff) - 0x6000));
			addCyclesInMemoryAccess_x2;
			break;

		case MAP_DSP:
			word  = S9xGetDSP(Address & 0xffff);
			addCyclesInMemoryAccess;
			word |= S9xGetDSP((Address + 1) & 0xffff) << 8;
			addCyclesInMemoryAccess;
			break;

		case MAP_SPC7110_ROM:
			word  = S9xGetSPC7110Byte(Address);
			addCyclesInMemoryAccess;
			word |= S9xGetSPC7110Byte(Address + 1) << 8;
			addCyclesInMemoryAccess;
			break;

		case MAP_SPC7110_DRAM:
			word  = S9xGetSPC7110(0x4800);
			addCyclesInMemoryAccess;
			word |= S9xGetSPC7110(0x4800) << 8;
			addCyclesInMemoryAccess;
			break;

		case MAP_C4:
			word  = S9xGetC4(Address & 0xffff);
			addCyclesInMemoryAccess;
			word |= S9xGetC4((Address + 1) & 0xffff) << 8;
			addCyclesInMemoryAccess;
			break;

		case MAP_OBC_RAM:
			word  = S9xGetOBC1(Address & 0xffff);
			addCyclesInMemoryAccess;
			word |= S9xGetOBC1((Address + 1) & 0xffff) << 8;
			addCyclesInMemoryAccess;
			break;

		case MAP_SETA_DSP:
			word  = S9xGetSetaDSP(Address);
			addCyclesInMemoryAccess;
			word |= S9xGetSetaDSP(Address + 1) << 8;
			addCyclesInMemoryAccess;
			break;

		case MAP_SETA_RISC:
			word  = S9xGetST018(Address);
			addCyclesInMemoryAccess;
			word |= S9xGetST018(Address + 1) << 8;
			addCyclesInMemoryAccess;
			break;

		case MAP_BSX:
			word  = S9xGetBSX(Address);
			addCyclesInMemoryAccess;
			word |= S9xGetBSX(Address + 1) << 8;
			addCyclesInMemoryAccess;
			break;

		case MAP_NONE:
		default:
			word = OpenBus | (OpenBus << 8);
			addCyclesInMemoryAccess_x2;
			break;
	}

	return (word);
}

/* Out-of-line slow path for S9xSetByte. */
HWREG_NOINLINE void S9xSetByteToRegister(uint8_t Byte, uint8_t *SetAddress, uint32_t Address)
{
	int32_t speed = memory_speed(Address);

	switch ((intptr_t) SetAddress)
	{
		case MAP_CPU:
			S9xSetCPU(Byte, Address & 0xffff);
			break;

		case MAP_PPU:
			if (CPU.InDMAorHDMA && (Address & 0xff00) == 0x2100)
				return;
			S9xSetPPU(Byte, Address & 0xffff);
			break;

		case MAP_LOROM_SRAM:
			if (Memory.SRAMMask)
				*(Memory.SRAM + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask)) = Byte;
			break;

		case MAP_LOROM_SRAM_B:
			if (Multi.sramMaskB)
				*(Multi.sramB + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Multi.sramMaskB)) = Byte;
			break;

		case MAP_HIROM_SRAM:
			if (Memory.SRAMMask)
				*(Memory.SRAM + (((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask)) = Byte;
			break;

		case MAP_BWRAM:
			*(Memory.BWRAM + ((Address & 0x7fff) - 0x6000)) = Byte;
			break;

		case MAP_SA1RAM:
			*(Memory.SRAM + (Address & 0xffff)) = Byte;
			break;

		case MAP_DSP:
			S9xSetDSP(Byte, Address & 0xffff);
			break;

		case MAP_C4:
			S9xSetC4(Byte, Address & 0xffff);
			break;

		case MAP_OBC_RAM:
			S9xSetOBC1(Byte, Address & 0xffff);
			break;

		case MAP_SETA_DSP:
			S9xSetSetaDSP(Byte, Address);
			break;

		case MAP_SETA_RISC:
			S9xSetST018(Byte, Address);
			break;

		case MAP_BSX:
			S9xSetBSX(Byte, Address);
			break;

		case MAP_NONE:
		default:
			break;
	}

	addCyclesInMemoryAccess;
}

/* Out-of-line slow path for S9xSetWord_Write0 (low byte first). */
HWREG_NOINLINE void S9xSetWordToRegister_Write0(uint16_t Word, uint8_t *SetAddress, uint32_t Address)
{
	int32_t speed = memory_speed(Address);

	switch ((intptr_t) SetAddress)
	{
		case MAP_CPU:
			S9xSetCPU((uint8_t) Word, Address & 0xffff);
			addCyclesInMemoryAccess;
			S9xSetCPU(Word >> 8, (Address + 1) & 0xffff);
			addCyclesInMemoryAccess;
			return;

		case MAP_PPU:
			if (CPU.InDMAorHDMA)
			{
				if ((Address & 0xff00) != 0x2100)
					S9xSetPPU((uint8_t) Word, Address & 0xffff);
				if (((Address + 1) & 0xff00) != 0x2100)
					S9xSetPPU(Word >> 8, (Address + 1) & 0xffff);
				return;
			}
			S9xSetPPU((uint8_t) Word, Address & 0xffff);
			addCyclesInMemoryAccess;
			S9xSetPPU(Word >> 8, (Address + 1) & 0xffff);
			addCyclesInMemoryAccess;
			return;

		case MAP_LOROM_SRAM:
			if (Memory.SRAMMask)
			{
				if (Memory.SRAMMask >= MEMMAP_MASK)
					WRITE_WORD(Memory.SRAM + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask), Word);
				else
				{
					*(Memory.SRAM + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask)) = (uint8_t) Word;
					*(Memory.SRAM + (((((Address + 1) & 0xff0000) >> 1) | ((Address + 1) & 0x7fff)) & Memory.SRAMMask)) = Word >> 8;
				}
			}
			addCyclesInMemoryAccess_x2;
			return;

		case MAP_LOROM_SRAM_B:
			if (Multi.sramMaskB)
			{
				if (Multi.sramMaskB >= MEMMAP_MASK)
					WRITE_WORD(Multi.sramB + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Multi.sramMaskB), Word);
				else
				{
					*(Multi.sramB + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Multi.sramMaskB)) = (uint8_t) Word;
					*(Multi.sramB + (((((Address + 1) & 0xff0000) >> 1) | ((Address + 1) & 0x7fff)) & Multi.sramMaskB)) = Word >> 8;
				}
			}
			addCyclesInMemoryAccess_x2;
			return;

		case MAP_HIROM_SRAM:
			if (Memory.SRAMMask)
			{
				if (Memory.SRAMMask >= MEMMAP_MASK)
					WRITE_WORD(Memory.SRAM + (((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask), Word);
				else
				{
					*(Memory.SRAM + (((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask)) = (uint8_t) Word;
					*(Memory.SRAM + ((((Address + 1) & 0x7fff) - 0x6000 + (((Address + 1) & 0xf0000) >> 3)) & Memory.SRAMMask)) = Word >> 8;
				}
			}
			addCyclesInMemoryAccess_x2;
			return;

		case MAP_BWRAM:
			WRITE_WORD(Memory.BWRAM + ((Address & 0x7fff) - 0x6000), Word);
			addCyclesInMemoryAccess_x2;
			return;

		case MAP_SA1RAM:
			WRITE_WORD(Memory.SRAM + (Address & 0xffff), Word);
			addCyclesInMemoryAccess_x2;
			return;

		case MAP_DSP:
			S9xSetDSP((uint8_t) Word, Address & 0xffff);
			addCyclesInMemoryAccess;
			S9xSetDSP(Word >> 8, (Address + 1) & 0xffff);
			addCyclesInMemoryAccess;
			return;

		case MAP_C4:
			S9xSetC4((uint8_t) Word, Address & 0xffff);
			addCyclesInMemoryAccess;
			S9xSetC4(Word >> 8, (Address + 1) & 0xffff);
			addCyclesInMemoryAccess;
			return;

		case MAP_OBC_RAM:
			S9xSetOBC1((uint8_t) Word, Address & 0xffff);
			addCyclesInMemoryAccess;
			S9xSetOBC1(Word >> 8, (Address + 1) & 0xffff);
			addCyclesInMemoryAccess;
			return;

		case MAP_SETA_DSP:
			S9xSetSetaDSP((uint8_t) Word, Address);
			addCyclesInMemoryAccess;
			S9xSetSetaDSP(Word >> 8, Address + 1);
			addCyclesInMemoryAccess;
			return;

		case MAP_SETA_RISC:
			S9xSetST018((uint8_t) Word, Address);
			addCyclesInMemoryAccess;
			S9xSetST018(Word >> 8, Address + 1);
			addCyclesInMemoryAccess;
			return;

		case MAP_BSX:
			S9xSetBSX((uint8_t) Word, Address);
			addCyclesInMemoryAccess;
			S9xSetBSX(Word >> 8, Address + 1);
			addCyclesInMemoryAccess;
			return;

		case MAP_NONE:
		default:
			addCyclesInMemoryAccess_x2;
			return;
	}
}

/* Out-of-line slow path for S9xSetWord_Write1 (high byte first). */
HWREG_NOINLINE void S9xSetWordToRegister_Write1(uint16_t Word, uint8_t *SetAddress, uint32_t Address)
{
	int32_t speed = memory_speed(Address);

	switch ((intptr_t) SetAddress)
	{
		case MAP_CPU:
			S9xSetCPU(Word >> 8, (Address + 1) & 0xffff);
			addCyclesInMemoryAccess;
			S9xSetCPU((uint8_t) Word, Address & 0xffff);
			addCyclesInMemoryAccess;
			return;

		case MAP_PPU:
			if (CPU.InDMAorHDMA)
			{
				if ((Address & 0xff00) != 0x2100)
					S9xSetPPU((uint8_t) Word, Address & 0xffff);
				if (((Address + 1) & 0xff00) != 0x2100)
					S9xSetPPU(Word >> 8, (Address + 1) & 0xffff);
				return;
			}
			S9xSetPPU(Word >> 8, (Address + 1) & 0xffff);
			addCyclesInMemoryAccess;
			S9xSetPPU((uint8_t) Word, Address & 0xffff);
			addCyclesInMemoryAccess;
			return;

		case MAP_LOROM_SRAM:
			if (Memory.SRAMMask)
			{
				if (Memory.SRAMMask >= MEMMAP_MASK)
					WRITE_WORD(Memory.SRAM + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask), Word);
				else
				{
					*(Memory.SRAM + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask)) = (uint8_t) Word;
					*(Memory.SRAM + (((((Address + 1) & 0xff0000) >> 1) | ((Address + 1) & 0x7fff)) & Memory.SRAMMask)) = Word >> 8;
				}
			}
			addCyclesInMemoryAccess_x2;
			return;

		case MAP_LOROM_SRAM_B:
			if (Multi.sramMaskB)
			{
				if (Multi.sramMaskB >= MEMMAP_MASK)
					WRITE_WORD(Multi.sramB + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Multi.sramMaskB), Word);
				else
				{
					*(Multi.sramB + ((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Multi.sramMaskB)) = (uint8_t) Word;
					*(Multi.sramB + (((((Address + 1) & 0xff0000) >> 1) | ((Address + 1) & 0x7fff)) & Multi.sramMaskB)) = Word >> 8;
				}
			}
			addCyclesInMemoryAccess_x2;
			return;

		case MAP_HIROM_SRAM:
			if (Memory.SRAMMask)
			{
				if (Memory.SRAMMask >= MEMMAP_MASK)
					WRITE_WORD(Memory.SRAM + (((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask), Word);
				else
				{
					*(Memory.SRAM + (((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask)) = (uint8_t) Word;
					*(Memory.SRAM + ((((Address + 1) & 0x7fff) - 0x6000 + (((Address + 1) & 0xf0000) >> 3)) & Memory.SRAMMask)) = Word >> 8;
				}
			}
			addCyclesInMemoryAccess_x2;
			return;

		case MAP_BWRAM:
			WRITE_WORD(Memory.BWRAM + ((Address & 0x7fff) - 0x6000), Word);
			addCyclesInMemoryAccess_x2;
			return;

		case MAP_SA1RAM:
			WRITE_WORD(Memory.SRAM + (Address & 0xffff), Word);
			addCyclesInMemoryAccess_x2;
			return;

		case MAP_DSP:
			S9xSetDSP(Word >> 8, (Address + 1) & 0xffff);
			addCyclesInMemoryAccess;
			S9xSetDSP((uint8_t) Word, Address & 0xffff);
			addCyclesInMemoryAccess;
			return;

		case MAP_C4:
			S9xSetC4(Word >> 8, (Address + 1) & 0xffff);
			addCyclesInMemoryAccess;
			S9xSetC4((uint8_t) Word, Address & 0xffff);
			addCyclesInMemoryAccess;
			return;

		case MAP_OBC_RAM:
			S9xSetOBC1(Word >> 8, (Address + 1) & 0xffff);
			addCyclesInMemoryAccess;
			S9xSetOBC1((uint8_t) Word, Address & 0xffff);
			addCyclesInMemoryAccess;
			return;

		case MAP_SETA_DSP:
			S9xSetSetaDSP(Word >> 8, Address + 1);
			addCyclesInMemoryAccess;
			S9xSetSetaDSP((uint8_t) Word, Address);
			addCyclesInMemoryAccess;
			return;

		case MAP_SETA_RISC:
			S9xSetST018(Word >> 8, Address + 1);
			addCyclesInMemoryAccess;
			S9xSetST018((uint8_t) Word, Address);
			addCyclesInMemoryAccess;
			return;

		case MAP_BSX:
			S9xSetBSX(Word >> 8, Address + 1);
			addCyclesInMemoryAccess;
			S9xSetBSX((uint8_t) Word, Address);
			addCyclesInMemoryAccess;
			return;

		case MAP_NONE:
		default:
			addCyclesInMemoryAccess_x2;
			return;
	}
}
