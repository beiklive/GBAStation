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


#ifndef _CONTROLS_H_
#define _CONTROLS_H_

#define S9xNoMapping			0
#define S9xButtonJoypad			1
#define S9xButtonMouse			2
#define S9xButtonSuperscope		3
#define S9xButtonJustifier		4
#define S9xPointer			8

#define S9xBadMapping			255
#define InvalidControlID		((uint32_t) -1)

#define CONTROLS_NONE			(-2)
#define MP5				(-1)
#define JOYPAD0				0
#define JOYPAD1				1
#define JOYPAD2				2
#define JOYPAD3				3
#define JOYPAD4				4
#define JOYPAD5				5
#define JOYPAD6				6
#define JOYPAD7				7
#define MOUSE0				8
#define MOUSE1				9
#define SUPERSCOPE			10
#define ONE_JUSTIFIER			11
#define TWO_JUSTIFIERS			12
#define NUMCTLS				13 /* This must be LAST */

typedef struct
{
	uint8_t	type;

	union
	{
		union
		{
			uint16_t joypad;	/* Which buttons to actuate. Use SNES_*_MASK constants from snes9x.h */

			struct
			{
				uint8_t	idx:1;			/* Mouse number 0-1 */
				uint8_t	left:1;			/* buttons */
				uint8_t	right:1;
			}	mouse;

			struct
			{
				uint8_t	fire:1;
				uint8_t	cursor:1;
				uint8_t	turbo:1;
				uint8_t	pause:1;
				/* Pretend we're pointing the gun offscreen (ignore the pointer)*/
				uint8_t	aim_offscreen:1;
			}	scope;

			struct
			{
				uint8_t	idx:3;			/* Pseudo-pointer number 0-7 */
				uint8_t	speed_type:2;		/* 0=variable, 1=slow, 2=med, 3=fast */
				int8_t	UD:2;			/* -1=up, 1=down, 0=no vertical motion */
				int8_t	LR:2;			/* -1=left, 1=right, 0=no horizontal motion */
			}	pointer;

			struct
			{
				uint8_t	idx:1;			/* Justifier number 0-1 */
				uint8_t	trigger:1;		/* buttons */
				uint8_t	start:1;
				/* Pretend we're pointing the gun offscreen (ignore the pointer) */
				uint8_t	aim_offscreen:1;
			}	justifier;

			uint16_t	command;
		} button;

		struct	/* Which SNES-pointers to control with this pointer */
		{
			uint16_t	aim_mouse0:1;
			uint16_t	aim_mouse1:1;
			uint16_t	aim_scope:1;
			uint16_t	aim_justifier0:1;
			uint16_t	aim_justifier1:1;
		}	pointer;

		uint8_t	port[4];
	} commandunion;
}	s9xcommand_t;

/* Starting out... */

void S9xUnmapAllControls (void);

/* Setting which controllers are plugged in. */

#define CTL_NONE 0		/* all ids ignored */
#define CTL_JOYPAD 1		/* use id1 to specify 0-7 */
#define CTL_MOUSE 2		/* use id1 to specify 0-1 */
#define CTL_SUPERSCOPE 3
#define CTL_JUSTIFIER 4		/* use id1: 0=one justifier, 1=two justifiers */
#define CTL_MP5 5		/* use id1-id4 to specify pad 0-7 (or -1) */

void S9xSetController (int port, unsigned controller, int8_t id1, int8_t id2, int8_t id3, int8_t id4); /* port=0-1 */

/* Call this when you're done with S9xSetController, or if you change any of 
   the controller Settings.*Master flags. 

   Returns true if something was disabled. */

uint8_t S9xVerifyControllers (void);

/* Functions for translating s9xcommand_t's into strings, and vice versa.
   free() the returned string after you're done with it. */

s9xcommand_t S9xGetCommandT (const char *name);

/* Generic mapping functions */

/* Button mapping functions.
   
   snes9x will assume you will call S9xReportButton() whenever the 
   button state changes.
   S9xMapButton() will fail and return FALSE if mapping.type isn't an S9xButton* type. */

uint8_t S9xMapButton (uint32_t id, s9xcommand_t mapping);
void S9xReportButton (uint32_t id, uint8_t pressed);

/* Pointer mapping functions.

   snes9x will assume you will call S9xReportPointer() whenever the
   pointer position changes.

   S9xMapPointer() will fail and return FALSE if mapping.type isn't an 
   S9xPointer* type.
   
   Note that position [0,0] is considered the upper-left corner of the 'screen',
   and either [255,223] or [255,239] is the lower-right.
   
   Note that the SNES mouse doesn't aim at a particular point,
   so the SNES's idea of where the mouse pointer is will probably differ
   from your OS's idea. */

uint8_t S9xMapPointer (uint32_t id, s9xcommand_t mapping);
void S9xReportPointer (uint32_t id, int16_t x, int16_t y);

/* Do whatever the s9xcommand_t says to do.
   If cmd.type is a button type, data1 should be TRUE (non-0) or FALSE (0)
   to indicate whether the 'button' is pressed or released.

   If cmd.type is an axis, data1 holds the deflection value.

   If cmd.type is a pointer, data1 and data2 are the positions of the pointer. */

void S9xApplyCommand (s9xcommand_t cmd, int16_t data1, int16_t data2);

/* These functions are called from snes9x into this subsystem. No need to use them from a port. */

/* Use when resetting snes9x. */

void S9xControlsReset (void);
void S9xControlsSoftReset (void);

/* End-Of-Frame processing. Sets gun latch variables and tries to draw crosshairs */

void S9xControlEOF (void);

/* Functions and a structure for snapshot. */

struct SControlSnapshot
{
	uint8_t	ver;
	uint8_t	port1_read_idx[2];
	uint8_t	dummy1[4];		/* for future expansion */
	uint8_t	port2_read_idx[2];
	uint8_t	dummy2[4];
	uint8_t	mouse_speed[2];
	uint8_t	justifier_select;
	uint8_t	dummy3[8];
	uint8_t	pad_read;
	uint8_t	internal[60];		/* yes, we need to save this! */
};

void S9xControlPreSaveState (struct SControlSnapshot *s);
void S9xControlPostLoadState (struct SControlSnapshot *s);

#endif
