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


#ifndef _PIXFORM_H_
#define _PIXFORM_H_

/* RGB565 format */

#define MAX_RED_RGB565			31
#define MAX_GREEN_RGB565		63
#define MAX_BLUE_RGB565			31
#define RED_LOW_BIT_MASK_RGB565		0x0800
#define GREEN_LOW_BIT_MASK_RGB565	0x0020
#define BLUE_LOW_BIT_MASK_RGB565	0x0001
#define RED_HI_BIT_MASK_RGB565		0x8000
#define GREEN_HI_BIT_MASK_RGB565	0x0400
#define BLUE_HI_BIT_MASK_RGB565		0x0010
#define FIRST_COLOR_MASK_RGB565		0xF800
#define SECOND_COLOR_MASK_RGB565	0x07E0
#define THIRD_COLOR_MASK_RGB565		0x001F
#define ALPHA_BITS_MASK_RGB565		0x0000

#define BUILD_PIXEL(R, G, B)			(((int) (R) << 11) | ((int) (G) << 6) | (int) (B))
#define BUILD_PIXEL2(R, G, B)			(((int) (R) << 11) | ((int) (G) << 5) | (int) (B))

#define MAX_RED					MAX_RED_RGB565
#define MAX_GREEN				MAX_GREEN_RGB565
#define MAX_BLUE				MAX_BLUE_RGB565
#define RED_LOW_BIT_MASK			RED_LOW_BIT_MASK_RGB565
#define GREEN_LOW_BIT_MASK			GREEN_LOW_BIT_MASK_RGB565
#define BLUE_LOW_BIT_MASK			BLUE_LOW_BIT_MASK_RGB565
#define RED_HI_BIT_MASK				RED_HI_BIT_MASK_RGB565
#define GREEN_HI_BIT_MASK			GREEN_HI_BIT_MASK_RGB565
#define BLUE_HI_BIT_MASK			BLUE_HI_BIT_MASK_RGB565
#define FIRST_COLOR_MASK			FIRST_COLOR_MASK_RGB565
#define SECOND_COLOR_MASK			SECOND_COLOR_MASK_RGB565
#define THIRD_COLOR_MASK			THIRD_COLOR_MASK_RGB565
#define ALPHA_BITS_MASK				ALPHA_BITS_MASK_RGB565

#define GREEN_HI_BIT				((MAX_GREEN + 1) >> 1)
#define RGB_LOW_BITS_MASK			(RED_LOW_BIT_MASK | GREEN_LOW_BIT_MASK | BLUE_LOW_BIT_MASK)
#define RGB_HI_BITS_MASK			(RED_HI_BIT_MASK | GREEN_HI_BIT_MASK | BLUE_HI_BIT_MASK)
#define RGB_HI_BITS_MASKx2			((RED_HI_BIT_MASK | GREEN_HI_BIT_MASK | BLUE_HI_BIT_MASK) << 1)
#define RGB_REMOVE_LOW_BITS_MASK		(~RGB_LOW_BITS_MASK)

#endif
