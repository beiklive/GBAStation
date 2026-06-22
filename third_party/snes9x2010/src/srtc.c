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

/*****
 * S-RTC emulation code
 * Copyright (c) byuu
 *****/

#include <string.h>
#include <time.h>

#include "snes9x.h"
#include "memmap.h"
#include "getset.h"
#include "srtc.h"
#include "display.h"

#define MEMORY_CARTRTC_READ(a)		RTCData.reg[(a)]
#define MEMORY_CARTRTC_WRITE(a, b)	{ RTCData.reg[(a)] = (b); }

#define RTCM_READY	(0)
#define RTCM_COMMAND	(1)
#define RTCM_READ	(2)
#define RTCM_WRITE	(3)

static signed srtc_index;

static uint32_t srtc_mode;

static const unsigned srtc_months[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static void srtcemu_update_time (void)
{
	unsigned days;
	time_t current_time, rtc_time, diff;

	rtc_time = (MEMORY_CARTRTC_READ(16) <<  0) | (MEMORY_CARTRTC_READ(17) <<  8) | (MEMORY_CARTRTC_READ(18) << 16) | (MEMORY_CARTRTC_READ(19) << 24);
	current_time = time(0);

	/*sizeof(time_t) is platform-dependent; though memory::cartrtc needs to be platform-agnostic.*/
	/*yet platforms with 32-bit signed time_t will overflow every ~68 years. handle this by*/
	/*accounting for overflow at the cost of 1-bit precision (to catch underflow). this will allow*/
	/*memory::cartrtc timestamp to remain valid for up to ~34 years from the last update, even if*/
	/*time_t overflows. calculation should be valid regardless of number representation, time_t size,*/
	/*or whether time_t is signed or unsigned.*/

	diff = (current_time >= rtc_time) ? (current_time - rtc_time) : ((time_t)-1 - rtc_time + current_time + 1);
	/*compensate for overflow*/

	if(diff > (time_t)-1 / 2)
		diff = 0;            /*compensate for underflow*/

	if(diff > 0)
	{
		unsigned second, minute, hour, day, month, year, weekday;

		second  = MEMORY_CARTRTC_READ( 0) + MEMORY_CARTRTC_READ( 1) * 10;
		minute  = MEMORY_CARTRTC_READ( 2) + MEMORY_CARTRTC_READ( 3) * 10;
		hour    = MEMORY_CARTRTC_READ( 4) + MEMORY_CARTRTC_READ( 5) * 10;
		day     = MEMORY_CARTRTC_READ( 6) + MEMORY_CARTRTC_READ( 7) * 10;
		month   = MEMORY_CARTRTC_READ( 8);
		year    = MEMORY_CARTRTC_READ( 9) + MEMORY_CARTRTC_READ(10) * 10 + MEMORY_CARTRTC_READ(11) * 100;
		weekday = MEMORY_CARTRTC_READ(12);

		day--;
		month--;
		year += 1000;

		second += diff;
		while(second >= 60)
		{
			second -= 60;

			minute++;
			if(minute < 60)
				continue;
			minute = 0;

			hour++;
			if(hour < 24)
				continue;
			hour = 0;

			day++;
			weekday = (weekday + 1) % 7;
			days = srtc_months[month % 12];
			if(days == 28)
			{
				uint8_t leapyear = FALSE;
				if((year % 4) == 0)
				{
					leapyear = TRUE;
					if((year % 100) == 0 && (year % 400) != 0)
						leapyear = FALSE;
				}
				if(leapyear)
					days++;
			}
			if(day < days)
				continue;
			day = 0;

			month++;
			if(month < 12)
				continue;
			month = 0;

			year++;
		}

		day++;
		month++;
		year -= 1000;

		MEMORY_CARTRTC_WRITE( 0, second % 10);
		MEMORY_CARTRTC_WRITE( 1, second / 10);
		MEMORY_CARTRTC_WRITE( 2, minute % 10);
		MEMORY_CARTRTC_WRITE( 3, minute / 10);
		MEMORY_CARTRTC_WRITE( 4, hour % 10);
		MEMORY_CARTRTC_WRITE( 5, hour / 10);
		MEMORY_CARTRTC_WRITE( 6, day % 10);
		MEMORY_CARTRTC_WRITE( 7, day / 10);
		MEMORY_CARTRTC_WRITE( 8, month);
		MEMORY_CARTRTC_WRITE( 9, year % 10);
		MEMORY_CARTRTC_WRITE(10, (year / 10) % 10);
		MEMORY_CARTRTC_WRITE(11, year / 100);
		MEMORY_CARTRTC_WRITE(12, weekday % 7);
	}

	MEMORY_CARTRTC_WRITE(16, current_time >>  0);
	MEMORY_CARTRTC_WRITE(17, current_time >>  8);
	MEMORY_CARTRTC_WRITE(18, current_time >> 16);
	MEMORY_CARTRTC_WRITE(19, current_time >> 24);
}

/*returns day of week for specified date*/
/*eg 0 = Sunday, 1 = Monday, ... 6 = Saturday*/
/*usage: weekday(2008, 1, 1) returns weekday of January 1st, 2008*/
static unsigned srtcemu_weekday(unsigned year, unsigned month, unsigned day)
{
	unsigned y, m, sum, days;

	y = 1900;
	m = 1;		/*epoch is 1900-01-01*/
	sum = 0;	/*number of days passed since epoch*/

	year = max(1900, year);
	month = max(1, min(12, month));
	day = max(1, min(31, day));

	while(y < year)
	{
		uint8_t leapyear = FALSE;
		if((y % 4) == 0)
		{
			leapyear = TRUE;
			if((y % 100) == 0 && (y % 400) != 0)
				leapyear = FALSE;
		}
		sum += leapyear ? 366 : 365;
		y++;
	}

	while(m < month)
	{
		days = srtc_months[m - 1];
		if(days == 28)
		{
			uint8_t leapyear = FALSE;
			if((y % 4) == 0)
			{
				leapyear = TRUE;
				if((y % 100) == 0 && (y % 400) != 0)
					leapyear = FALSE;
			}
			if(leapyear)
				days++;
		}
		sum += days;
		m++;
	}

	sum += day - 1;
	return (sum + 1) % 7;  /*1900-01-01 was a Monday*/
}

void S9xInitSRTC (void)
{
	srtc_mode = RTCM_READ;
	srtc_index = -1;
	srtcemu_update_time();
	memset(RTCData.reg, 0, 20);
}

void S9xResetSRTC (void)
{
	srtc_mode = RTCM_READ;
	srtc_index = -1;
	srtcemu_update_time();
}

void S9xSetSRTC (uint8_t data, uint16_t address)
{
	unsigned i;
	address &= 0xffff;

	if(address == 0x2801)
	{
		data &= 0x0f;  /*only the low four bits are used*/

		if(data == 0x0d)
		{
			srtc_mode = RTCM_READ;
			srtc_index = -1;
			return;
		}

		if(data == 0x0e) {
			srtc_mode = RTCM_COMMAND;
			return;
		}

		if(data == 0x0f)
			return;  /*unknown behavior*/

		if(srtc_mode == RTCM_WRITE)
		{
			if(srtc_index >= 0 && srtc_index < 12)
			{
				MEMORY_CARTRTC_WRITE(srtc_index++, data);

				if(srtc_index == 12)
				{
					unsigned day, month, year;
					/*day of week is automatically calculated and written*/
					day   = MEMORY_CARTRTC_READ( 6) + MEMORY_CARTRTC_READ( 7) * 10;
					month = MEMORY_CARTRTC_READ( 8);
					year  = MEMORY_CARTRTC_READ( 9) + MEMORY_CARTRTC_READ(10) * 10 + MEMORY_CARTRTC_READ(11) * 100;
					year += 1000;

					MEMORY_CARTRTC_WRITE(srtc_index++, srtcemu_weekday(year, month, day));
				}
			}
		}
		else if(srtc_mode == RTCM_COMMAND)
		{
			if(data == 0)
			{
				srtc_mode = RTCM_WRITE;
				srtc_index = 0;
			}
			else if(data == 4)
			{
				srtc_mode = RTCM_READY;
				srtc_index = -1;
				for( i = 0; i < 13; i++)
					MEMORY_CARTRTC_WRITE(i, 0);
			}
			else
				srtc_mode = RTCM_READY;	/*unknown behavior*/
		}
	}
}

uint8_t S9xGetSRTC (uint16_t address)
{
	address &= 0xffff;

	if(address == 0x2800)
	{
		if(srtc_mode != RTCM_READ)
			return 0x00;

		if(srtc_index < 0)
		{
			srtcemu_update_time();
			srtc_index++;
			return 0x0f;
		}
		else if(srtc_index > 12)
		{
			srtc_index = -1;
			return 0x0f;
		}
		else
			return MEMORY_CARTRTC_READ(srtc_index++);
	}

	return OpenBus;
}

void S9xSRTCPreSaveState (void)
{
	srtcsnap.rtc_mode  = (int32_t) srtc_mode;
	srtcsnap.rtc_index = (int32_t) srtc_index;
}

void S9xSRTCPostLoadState (void)
{
	srtc_mode  = srtcsnap.rtc_mode;
	srtc_index = (signed)         srtcsnap.rtc_index;

	srtcemu_update_time();
}
