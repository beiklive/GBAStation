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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "snes9x.h"
#include "memmap.h"
#include "getset.h"
#include "cheats.h"

static uint8_t S9xAllHex (const char *code, int len)
{
	int i;
	for ( i = 0; i < len; i++)
		if ((code[i] < '0' || code[i] > '9') && (code[i] < 'a' || code[i] > 'f') && (code[i] < 'A' || code[i] > 'F'))
			return (FALSE);

	return (TRUE);
}

const char * S9xProActionReplayToRaw (const char *code, uint32_t * address, uint8_t * byte)
{
	uint32_t	data = 0;

	if (strlen(code) != 8 || !S9xAllHex(code, 8) || sscanf(code, "%x", &data) != 1)
		return ("Invalid Pro Action Replay code - should be 8 hex digits in length.");

	*address = data >> 8;
	*byte = (uint8_t) data;

	return (NULL);
}

const char * S9xGoldFingerToRaw (const char *code, uint32_t * address, uint8_t * sram, uint8_t * num_bytes, uint8_t bytes[3])
{
	char	tmp[15];
	int		i;

	if (strlen(code) != 14)
		return ("Invalid Gold Finger code - should be 14 hex digits in length.");

	strncpy(tmp, code, 5);
	tmp[5] = 0;
	if (sscanf(tmp, "%x", address) != 1)
		return ("Invalid Gold Finger code.");

	for (i = 0; i < 3; i++)
	{
		unsigned int	byte;

		strncpy(tmp, code + 5 + i * 2, 2);
		tmp[2] = 0;
		if (sscanf(tmp, "%x", &byte) != 1)
			break;
		bytes[i] = (uint8_t) byte;
	}

	*num_bytes = i;
	*sram = code[13] == '1';

	return (NULL);
}

static const char	*real_hex  = "0123456789ABCDEF";
static const char	*genie_hex = "DF4709156BC8A23E";

const char * S9xGameGenieToRaw (const char *code, uint32_t * address, uint8_t * byte)
{
	int i, j;
	uint32_t data;
	char	new_code[12];

	if (strlen(code) != 9 || *(code + 4) != '-' || !S9xAllHex(code, 4) || !S9xAllHex(code + 5, 4))
		return ("Invalid Game Genie(tm) code - should be 'xxxx-xxxx'.");

	strcpy(new_code, "0x");
	strncpy(new_code + 2, code, 4);
	strcpy(new_code + 6, code + 5);


	for ( i = 2; i < 10; i++)
	{
		if (islower(new_code[i]))
			new_code[i] = toupper(new_code[i]);

		for (j = 0; j < 16; j++)
		{
			if (new_code[i] == genie_hex[j])
			{
				new_code[i] = real_hex[j];
				break;
			}
		}

		if (j == 16)
			return ("Invalid hex-character in Game Genie(tm) code.");
	}

	data = 0;
	sscanf(new_code, "%x", &data);
	*byte = (uint8_t) (data >> 24);
	*address = data & 0xffffff;
	*address = ((*address & 0x003c00) << 10) +
			  ((*address & 0x00003c) << 14) +
			  ((*address & 0xf00000) >>  8) +
			  ((*address & 0x000003) << 10) +
			  ((*address & 0x00c000) >>  6) +
			  ((*address & 0x0f0000) >> 12) +
			  ((*address & 0x0003c0) >>  6);

	return NULL;
}

static uint8_t S9xGetByteFree (uint32_t address)
{
	uint32_t	Cycles = CPU.Cycles;
	uint8_t	byte;

	byte = S9xGetByte(address);

	CPU.Cycles = Cycles;

	return (byte);
}

static void S9xSetByteFree (uint8_t byte, uint32_t address)
{
	uint32_t	Cycles = CPU.Cycles;

	S9xSetByte(byte, address);

	CPU.Cycles = Cycles;
}

void S9xInitCheatData (void)
{
	Cheat.RAM = Memory.RAM;
	Cheat.SRAM = Memory.SRAM;
	Cheat.FillRAM = Memory.FillRAM;
}

void S9xAddCheat (uint8_t enable, uint8_t save_current_value, uint32_t address, uint8_t byte)
{
	if (Cheat.num_cheats < sizeof(Cheat.c) / sizeof(Cheat.c[0]))
	{
		Cheat.c[Cheat.num_cheats].address = address;
		Cheat.c[Cheat.num_cheats].byte = byte;
		Cheat.c[Cheat.num_cheats].enabled = enable;

		if (save_current_value)
		{
			Cheat.c[Cheat.num_cheats].saved_byte = S9xGetByteFree(address);
			Cheat.c[Cheat.num_cheats].saved = TRUE;
		}

		Cheat.num_cheats++;
	}
}

const char * S9xGetCheatName(uint8_t cheat_no)
{
	return Cheat.c[cheat_no].name;
}

void S9xDeleteCheat (uint32_t which1)
{
	if (which1 < Cheat.num_cheats)
	{
		if (Cheat.c[which1].enabled)
			S9xRemoveCheat(which1);

		memmove(&Cheat.c[which1], &Cheat.c[which1 + 1], sizeof(Cheat.c[0]) * (Cheat.num_cheats - which1 - 1));

		Cheat.num_cheats--;
	}
}

void S9xDeleteCheats (void)
{
	S9xRemoveCheats();
	Cheat.num_cheats = 0;
}

void S9xRemoveCheat (uint32_t which1)
{
	if (Cheat.c[which1].saved)
	{
		uint32_t	address = Cheat.c[which1].address;

		int		block = (address & 0xffffff) >> MEMMAP_SHIFT;
		uint8_t	*ptr = Memory.Map[block];

		if (ptr >= (uint8_t *) MAP_LAST)
			*(ptr + (address & 0xffff)) = Cheat.c[which1].saved_byte;
		else
			S9xSetByteFree(Cheat.c[which1].saved_byte, address);
	}
}

void S9xRemoveCheats (void)
{
	uint32_t i;
	for ( i = 0; i < Cheat.num_cheats; i++)
		if (Cheat.c[i].enabled)
			S9xRemoveCheat(i);
}

void S9xEnableCheat (uint32_t which1)
{
	if (which1 < Cheat.num_cheats && !Cheat.c[which1].enabled)
	{
		Cheat.c[which1].enabled = TRUE;
		S9xApplyCheat(which1);
	}
}

uint8_t S9xCheatEnabled (uint32_t which1)
{
	return Cheat.c[which1].enabled;
}

void S9xDisableCheat (uint32_t which1)
{
	if (which1 < Cheat.num_cheats && Cheat.c[which1].enabled)
	{
		S9xRemoveCheat(which1);
		Cheat.c[which1].enabled = FALSE;
	}
}

void S9xApplyCheat (uint32_t which1)
{
	int block;
	uint8_t *ptr;
	uint32_t	address = Cheat.c[which1].address;

	if (!Cheat.c[which1].saved)
	{
		Cheat.c[which1].saved_byte = S9xGetByteFree(address);
		Cheat.c[which1].saved = TRUE;
	}

	block = (address & 0xffffff) >> MEMMAP_SHIFT;
	ptr = Memory.Map[block];

	if (ptr >= (uint8_t *) MAP_LAST)
		*(ptr + (address & 0xffff)) = Cheat.c[which1].byte;
	else
		S9xSetByteFree(Cheat.c[which1].byte, address);
}

void S9xApplyCheats (void)
{
	if (Settings.ApplyCheats)
	{
		uint32_t i;
		for ( i = 0; i < Cheat.num_cheats; i++)
			if (Cheat.c[i].enabled)
				S9xApplyCheat(i);
	}
}
