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


#ifndef _SPC7110_H_
#define _SPC7110_H_

#define SPC7110_DECOMP_BUFFER_SIZE	64

typedef struct
{
	uint8_t	index;
	uint8_t	invert;
} ContextState;

/* for snapshot only */
struct SSPC7110Snapshot
{
	uint8_t	r4801;
	uint8_t	r4802;
	uint8_t	r4803;
	uint8_t	r4804;
	uint8_t	r4805;
	uint8_t	r4806;
	uint8_t	r4807;
	uint8_t	r4808;
	uint8_t	r4809;
	uint8_t	r480a;
	uint8_t	r480b;
	uint8_t	r480c;

	uint8_t	r4811;
	uint8_t	r4812;
	uint8_t	r4813;
	uint8_t	r4814;
	uint8_t	r4815;
	uint8_t	r4816;
	uint8_t	r4817;
	uint8_t	r4818;

	uint8_t	r481x;

	uint8_t	r4814_latch;			/* bool */
	uint8_t	r4815_latch;			/* bool */

	uint8_t	r4820;
	uint8_t	r4821;
	uint8_t	r4822;
	uint8_t	r4823;
	uint8_t	r4824;
	uint8_t	r4825;
	uint8_t	r4826;
	uint8_t	r4827;
	uint8_t	r4828;
	uint8_t	r4829;
	uint8_t	r482a;
	uint8_t	r482b;
	uint8_t	r482c;
	uint8_t	r482d;
	uint8_t	r482e;
	uint8_t	r482f;

	uint8_t	r4830;
	uint8_t	r4831;
	uint8_t	r4832;
	uint8_t	r4833;
	uint8_t	r4834;

	uint32_t	dx_offset;			/* unsigned */
	uint32_t	ex_offset;			/* unsigned */
	uint32_t	fx_offset;			/* unsigned */

	uint8_t	r4840;
	uint8_t	r4841;
	uint8_t	r4842;

	int32_t	rtc_state;			/* enum RTC_State */
	int32_t	rtc_mode;			/* enum RTC_Mode */
	uint32_t	rtc_index;			/* unsigned */

	uint32_t	decomp_mode;			/* unsigned */
	uint32_t	decomp_offset;			/* unsigned */

	uint8_t	decomp_buffer[SPC7110_DECOMP_BUFFER_SIZE];

	uint32_t	decomp_buffer_rdoffset;		/* unsigned */
	uint32_t	decomp_buffer_wroffset;		/* unsigned */
	uint32_t	decomp_buffer_length;		/* unsigned */

	ContextState context[32];
};

extern struct SSPC7110Snapshot	s7snap;

void S9xInitSPC7110 (void);
void S9xResetSPC7110 (void);
void S9xFreeSPC7110 (void);
void S9xSPC7110PreSaveState (void);
void S9xSPC7110PostLoadState (void);
void S9xSetSPC7110 (uint8_t Byte, uint16_t Address);
uint8_t S9xGetSPC7110 (uint16_t address);
uint8_t S9xGetSPC7110Byte (uint32_t address);
uint8_t * S9xGetBasePointerSPC7110 (uint32_t address);

#endif
