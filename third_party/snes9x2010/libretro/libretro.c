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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <libretro.h>
#include <vfs/vfs.h>
#include <streams/file_stream.h>
#include "libretro_core_options.h"

#ifdef _3DS
void* linearMemAlign(size_t size, size_t alignment);
void linearFree(void* mem);
#endif

#include "../src/apu.h"
#include "../src/cheats.h"
#include "../src/controls.h"
#include "../src/cpuexec.h"
#include "../src/display.h"
#include "../src/fxemu.h"
#include "../src/memmap.h"
#include "../src/messages.h"
#include "../src/ppu.h"
#include "../src/snapshot.h"
#include "../src/snes9x.h"
#include "../src/srtc.h"
#include "../filter/snes_ntsc.h"

#define LR_MAP_BUTTON(id, name)		S9xMapButton((id), S9xGetCommandT((name)))
#define MAKE_BUTTON(pad, btn)			(((pad) << 4) | (btn))

#define BTN_POINTER		(RETRO_DEVICE_ID_JOYPAD_R + 1)
#define BTN_POINTER2		(BTN_POINTER + 1)

#define RETRO_DEVICE_JOYPAD_MULTITAP		RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)
#define RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE		RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 0)
#define RETRO_DEVICE_LIGHTGUN_JUSTIFIER		RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 1)
#define RETRO_DEVICE_LIGHTGUN_JUSTIFIERS		RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 2)

#define CORE_VERSION	"1.52.4"
#define LIBRETRO_LIB_NAME "Snes9x 2010"

#define VIDEO_REFRESH_RATE_PAL  (PAL_MASTER_CLOCK / (double)(SNES_CYCLES_PER_SCANLINE * SNES_MAX_PAL_VCOUNTER))
#define VIDEO_REFRESH_RATE_NTSC (NTSC_MASTER_CLOCK / (double)(SNES_CYCLES_PER_SCANLINE * SNES_MAX_NTSC_VCOUNTER))

/* Pre-zeroed silence buffer used in place of real SPC output when
   audio_muted is set. Sized for one worst-case PAL frame at the
   highest effective SPC rate (ticks=4 speedup, ~651 stereo). The
   '+ a bit' rounds up to a tidy power of two. RetroArch's audio_batch_cb
   only reads (frame_count * 2) shorts, so over-sizing is harmless and
   keeps the math simple if rates change.

   Declared non-const so it lives in BSS (zero file footprint, zero
   initialized at startup) rather than .rodata; we never write to it. */
#define MUTE_BUFFER_FRAMES 768
static int16_t mute_buffer[MUTE_BUFFER_FRAMES * 2];

/* Set when the frontend has signalled that audio should not be played
   (either user has disabled audio or a frontend-controlled fast-forward
   path has set RETRO_AVENABLE_HARD_DISABLE_AUDIO). When set, we still
   call audio_batch_cb every frame - skipping it would let RetroArch's
   audio ring drain and pull DRC out of steady-state tracking - but
   substitute the silence buffer for the SPC's actual output. */
static bool audio_muted;

enum
{
	ASPECT_RATIO_4_3,
	ASPECT_RATIO_1_1,
	ASPECT_RATIO_NTSC,
	ASPECT_RATIO_PAL,
	ASPECT_RATIO_AUTO
};

static retro_log_printf_t		log_cb = NULL;
static retro_video_refresh_t		video_cb = NULL;
static retro_input_poll_t			poll_cb = NULL;
static retro_input_state_t		input_cb = NULL;
static retro_audio_sample_batch_t			audio_batch_cb = NULL;
static retro_environment_t		environ_cb = NULL;

static unsigned frameskip_type = 0;
static unsigned frameskip_threshold = 0;
static uint16_t frameskip_counter = 0;
static bool retro_audio_buff_active = false;
static unsigned retro_audio_buff_occupancy = 0;
static bool retro_audio_buff_underrun = false;

/* Maximum number of consecutive frames that can be skipped */
#define FRAMESKIP_MAX 30

static unsigned retro_audio_latency = 0;
static bool update_audio_latency = false;

extern s9xcommand_t keymap[1024];
bool overclock_cycles = false;
bool reduce_sprite_flicker = false;
extern uint16_t joypad[8];
int one_c, slow_one_c, two_c;

static struct
{
	int16_t retro_mouse_state[2][2];
	int16_t retro_justifier_state[2][2];
	int16_t retro_scope_state[2];
	uint8_t turbo_state[5];
	uint8_t turbo_delay;
	uint8_t turbo_enable;
	bool switch_state;
} input_vars;

static uint8_t aspect_ratio_mode = ASPECT_RATIO_4_3;
static bool libretro_supports_option_categories = false;
static bool libretro_supports_bitmasks = false;
static bool libretro_supports_sw_fb = false;
static bool libretro_sw_fb_checked = false;

/* Direct-render state for the sw_fb optimization. When sw_fb_active is
   true for the current frame, GFX.Screen has been redirected to point at
   a frontend-supplied software framebuffer. The renderer writes pixels
   there directly, and S9xDeinitUpdate hands fb.data to video_cb instead
   of GFX.Screen. The acquire happens at the cpuexec.c hook (after this
   frame's resolution is known); abort happens at the ppu.c promotion
   sites if a mid-frame mode change forces a wider/taller buffer.
   Saved fields hold the originals for the abort/release-after-frame
   restoration. */
static bool      sw_fb_active        = false;
static void     *sw_fb_data          = NULL;
static unsigned  sw_fb_width         = 0;
static unsigned  sw_fb_height        = 0;
static size_t    sw_fb_pitch         = 0;
static uint16_t   *sw_fb_saved_screen  = NULL;
static int       sw_fb_saved_pitch   = 0;

/* Persistent buffer pointers. GFX.Screen is allowed to be temporarily
   overwritten by the sw_fb redirect and various swap paths; we always
   know the buffer the core actually owns by keeping a separate copy
   here, untouched after retro_init. retro_deinit frees these directly
   instead of GFX.Screen so the wrong pointer can't be freed even if
   some abnormal teardown path leaves the redirect state inconsistent. */
static void *owned_screen_buffer  = NULL;
static void *owned_ntsc_buffer    = NULL;
static uint32_t retro_devices[2];

// filter
static int snes_ntsc_filter = 0;
static snes_ntsc_t snes_ntsc;
static snes_ntsc_setup_t ntsc_setup;
static uint16_t *ntsc_screen_buffer = NULL;
/* Buffer width must fit both NTSC filter output (~604 px after
   rounding to a 4-pixel multiple) and the 4x Mode 7 hires renderer
   (1024 px). The buffer is sized to the max of these. Frames that
   don't use 4x hires write only the leftmost N px and the trailing
   columns are unused garbage. */
#define MAX_SNES_WIDTH_NTSC ((SNES_NTSC_OUT_WIDTH(256) + 3) / 4 * 4)
#define MAX_SNES_WIDTH_4X   1024
#define MAX_BUFFER_WIDTH    (MAX_SNES_WIDTH_4X > MAX_SNES_WIDTH_NTSC ? MAX_SNES_WIDTH_4X : MAX_SNES_WIDTH_NTSC)

static void update_geometry();

static void retro_audio_buff_status_cb(bool active, unsigned occupancy, bool underrun_likely)
{
	retro_audio_buff_active = active;
	retro_audio_buff_occupancy = occupancy;
	retro_audio_buff_underrun = underrun_likely;
}

static void retro_set_audio_buff_status_cb(void) {
	if (frameskip_type > 0)
	{
		struct retro_audio_buffer_status_callback buf_status_cb;
		buf_status_cb.callback = retro_audio_buff_status_cb;

		if (!environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK, &buf_status_cb))
		{
			if (log_cb)
				log_cb(RETRO_LOG_WARN, "Frameskip disabled - frontend does not support audio buffer status monitoring.\n");

			retro_audio_buff_active = false;
			retro_audio_buff_occupancy = 0;
			retro_audio_buff_underrun = false;
			retro_audio_latency = 0;
		}
		else
		{
			/* Frameskip is enabled - increase frontend audio latency to
			 * minimise potential buffer underruns */
			uint32_t frame_time_usec = Settings.FrameTimeNTSC;

			if (Settings.PAL)
				frame_time_usec = Settings.FrameTimePAL;

			/* Set latency to 6x current frame time... */
			retro_audio_latency = (unsigned)(6 * frame_time_usec / 1000);

			/* ...then round up to nearest multiple of 32 */
			retro_audio_latency = (retro_audio_latency + 0x1F) & ~0x1F;
		}
	}
	else
	{
		environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK, NULL);
		retro_audio_latency = 0;
	}

	update_audio_latency = true;
}

static void check_variables(bool first_run)
{
	bool reset_sfx = false;
	int old_filter = snes_ntsc_filter;
	struct retro_variable var;
	bool prev_force_ntsc;
	bool prev_force_pal;
	bool prev_frameskip_type;

	var.key = "snes9x_2010_region";
	var.value = NULL;

	prev_force_ntsc = Settings.ForceNTSC;
	prev_force_pal = Settings.ForcePAL;

	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		if (strcmp(var.value, "auto") == 0)
		{
			Settings.ForceNTSC = false;
			Settings.ForcePAL = false;
		}
		else if (strcmp(var.value, "ntsc") == 0)
		{
			Settings.ForceNTSC = true;
			Settings.ForcePAL = false;
			Settings.PAL = FALSE;
		}
		else if (strcmp(var.value, "pal") == 0)
		{
			Settings.ForceNTSC = false;
			Settings.ForcePAL = true;
			Settings.PAL = TRUE;
		}
	}

	var.key = "snes9x_2010_audio_interpolation";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		if (strcmp(var.value, "cubic") == 0)
			dsp_interp_mode = DSP_INTERP_CUBIC;
		else if (strcmp(var.value, "sinc") == 0)
			dsp_interp_mode = DSP_INTERP_SINC;
		else
			dsp_interp_mode = DSP_INTERP_GAUSSIAN;
	}
	else
		dsp_interp_mode = DSP_INTERP_GAUSSIAN;

	var.key = "snes9x_2010_frameskip";
	var.value = NULL;

	prev_frameskip_type = frameskip_type;
	frameskip_type = 0;

	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		if (strcmp(var.value, "auto") == 0)
			frameskip_type = 1;
		else if (strcmp(var.value, "manual") == 0)
			frameskip_type = 2;
	}

	var.key = "snes9x_2010_frameskip_threshold";
	var.value = NULL;

	frameskip_threshold = 33;

	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		frameskip_threshold = strtol(var.value, NULL, 10);

	var.key = "snes9x_2010_aspect";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		uint8_t newval = ASPECT_RATIO_AUTO;

		if (strcmp(var.value, "ntsc") == 0)
			newval = ASPECT_RATIO_NTSC;
		else if (strcmp(var.value, "pal") == 0)
			newval = ASPECT_RATIO_PAL;
		else if (strcmp(var.value, "4:3") == 0)
			newval = ASPECT_RATIO_4_3;
		else if (strcmp(var.value, "uncorrected") == 0)
			newval = ASPECT_RATIO_1_1;

		if (newval != aspect_ratio_mode)
			aspect_ratio_mode = newval;
	}

	var.key = "snes9x_2010_overclock";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		char *endptr;
		double freq = strtod(var.value, &endptr);

		/* There must be a space between the value and the unit. Therefore, we
		 * check that the character after the converter integer is a space. */
		if (*endptr != ' ' || freq == 0.0)
		{
			S9xMessage(S9X_MSG_WARN, S9X_CATEGORY_EXTERNAL, "Unable to obtain SuperFX overclock setting.");
			freq = 10.0;
		}

		/* Convert MHz value to Hz and multiply by required factors. */
		Settings.SuperFXSpeedPerLine = 0.417 * 1.5e6 * freq;
		reset_sfx = true;
	}

   	var.key = "snes9x_2010_turbodelay";
   	var.value = NULL;
   	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   	{
      		if (strcmp(var.value, "medium") == 0)
        		input_vars.turbo_delay = 5;
      		else if (strcmp(var.value, "slow") == 0)
        		input_vars.turbo_delay = 7;
      		else
        		input_vars.turbo_delay = 3;
   	}

	var.key = "snes9x_2010_overclock_cycles";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		if (strcmp(var.value, "compatible") == 0)
		{
			overclock_cycles = true;
			one_c = 4;
			slow_one_c = 5;
			two_c = 6;
		}
		else if (strcmp(var.value, "max") == 0)
		{
			overclock_cycles = true;
			one_c = 3;
			slow_one_c = 3;
			two_c = 3;
		}
		else if (strcmp(var.value, "light") == 0)
		{
			overclock_cycles = true;
			one_c = 6;
			slow_one_c = 6;
			two_c = 12;
		}
		else
			overclock_cycles = false;
	}

	var.key = "snes9x_2010_blargg";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		if (strcmp(var.value, "disabled") == 0)
			snes_ntsc_filter = 0;
		else
		{
			if (strcmp(var.value, "monochrome") == 0)
			{
				snes_ntsc_filter = 1;
				ntsc_setup = snes_ntsc_monochrome;
			}
			else if (strcmp(var.value, "rf") == 0)
			{
				snes_ntsc_filter = 2;
				ntsc_setup = snes_ntsc_composite;
				ntsc_setup.merge_fields = 0;
			}
			else if (strcmp(var.value, "composite") == 0)
			{
				snes_ntsc_filter = 3;
				ntsc_setup = snes_ntsc_composite;
			}
			else if (strcmp(var.value, "s-video") == 0)
			{
				snes_ntsc_filter = 4;
				ntsc_setup = snes_ntsc_svideo;
			}
			else if (strcmp(var.value, "rgb") == 0)
			{
				snes_ntsc_filter = 5;
				ntsc_setup = snes_ntsc_rgb;
			}
		}
	}

	var.key = "snes9x_2010_reduce_sprite_flicker";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		if (strcmp(var.value, "enabled") == 0)
			reduce_sprite_flicker = true;
		else
			reduce_sprite_flicker = false;
	}

	var.key = "snes9x_2010_mode7_hires";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		/* Order matters: the *_hv suffixes must be checked before the
		 * unsuffixed forms so that strcmp("4x_hv", "4x") doesn't match
		 * the wrong branch via strncmp logic if the parsing ever
		 * changes. We use full strcmp here so it's not actually a hazard
		 * today, but the order also makes the intent clear. */
		if (strcmp(var.value, "4x_hv") == 0)
		{
			Settings.Mode7Hires = 4;
			Settings.Mode7HiresVertical = 1;
		}
		else if (strcmp(var.value, "2x_hv") == 0)
		{
			Settings.Mode7Hires = 2;
			Settings.Mode7HiresVertical = 1;
		}
		else if (strcmp(var.value, "4x") == 0)
		{
			Settings.Mode7Hires = 4;
			Settings.Mode7HiresVertical = 0;
		}
		else if (strcmp(var.value, "2x") == 0 || strcmp(var.value, "enabled") == 0)
		{
			Settings.Mode7Hires = 2;
			Settings.Mode7HiresVertical = 0;
		}
		else
		{
			Settings.Mode7Hires = 0;
			Settings.Mode7HiresVertical = 0;
		}
	}

	var.key = "snes9x_2010_mode7_hires_bilinear";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		if (strcmp(var.value, "smooth") == 0)
			Settings.Mode7HiresBilinear = 2;
		else if (strcmp(var.value, "stable") == 0 || strcmp(var.value, "enabled") == 0)
			Settings.Mode7HiresBilinear = 1;
		else
			Settings.Mode7HiresBilinear = 0;
	}
	/* Reinitialise frameskipping, if required */
	if (!first_run &&
	    ((frameskip_type     != prev_frameskip_type) ||
	     (Settings.ForceNTSC != prev_force_ntsc)     ||
	     (Settings.ForcePAL  != prev_force_pal)))
      retro_set_audio_buff_status_cb();

//
	if (old_filter != snes_ntsc_filter)
		snes_ntsc_init(&snes_ntsc, &ntsc_setup);

	if (reset_sfx)
		S9xResetSuperFX();
}

void *retro_get_memory_data(unsigned type)
{
	uint8_t *data;

	switch (type)
	{
		case RETRO_MEMORY_SAVE_RAM:
			data = Memory.SRAM;
		break;
		case RETRO_MEMORY_RTC:
			data = RTCData.reg;
		break;
		case RETRO_MEMORY_SYSTEM_RAM:
			data = Memory.RAM;
		break;
		case RETRO_MEMORY_VIDEO_RAM:
			data = Memory.VRAM;
		break;
/*		case RETRO_MEMORY_ROM:
			data = Memory.ROM;
		break;		*/
		default:
			data = NULL;
		break;
	}

	return data;
}

size_t retro_get_memory_size(unsigned type)
{
	unsigned size;

	switch (type)
	{
		case RETRO_MEMORY_SAVE_RAM:
			size = (unsigned) (Memory.SRAMSize ? (1 << (Memory.SRAMSize + 3)) * 128 : 0);
			if (size > 0x20000)
				size = 0x20000;
		break;
		case RETRO_MEMORY_RTC:
			size = (Settings.SRTC || Settings.SPC7110RTC) ? 20 : 0;
		break;
		case RETRO_MEMORY_SYSTEM_RAM:
			size = 128 * 1024;
		break;
		case RETRO_MEMORY_VIDEO_RAM:
			size = 64 * 1024;
		break;
/*		case RETRO_MEMORY_ROM:
			data = Memory.CalculatedSize;
		break;		*/
		default:
			size = 0;
		break;
	}

	return size;
}

unsigned retro_api_version(void) { return RETRO_API_VERSION; }

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { (void) cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_cb = cb; }

void retro_set_environment(retro_environment_t cb)
{
        bool local_bool_val = false;
	static const struct retro_controller_description port_1[] = {
		{ "SNES Joypad", RETRO_DEVICE_JOYPAD },
		{ "SNES Mouse", RETRO_DEVICE_MOUSE },
		{ "RetroPad", RETRO_DEVICE_JOYPAD },
	};

	static const struct retro_controller_description port_2[] = {
		{ "SNES Joypad", RETRO_DEVICE_JOYPAD },
		{ "SNES Mouse", RETRO_DEVICE_MOUSE },
		{ "Multitap", RETRO_DEVICE_JOYPAD_MULTITAP },
		{ "SuperScope", RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE },
		{ "Justifier", RETRO_DEVICE_LIGHTGUN_JUSTIFIER },
		{ "Justifiers", RETRO_DEVICE_LIGHTGUN_JUSTIFIERS },
		{ "RetroPad", RETRO_DEVICE_JOYPAD },
	};

	static const struct retro_controller_info ports[] = {
		{ port_1, 3 },
		{ port_2, 7 },
		{ 0, 0 }
	};

	struct retro_vfs_interface_info vfs_iface_info;

	environ_cb = cb;

	libretro_supports_option_categories = false;

	libretro_set_core_options(environ_cb, &local_bool_val);
  
        libretro_supports_option_categories = local_bool_val;

	/* (No option-visibility callback: the bilinear and hires options
	   are now independent and both are always shown.) */

	environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

	vfs_iface_info.required_interface_version = 1;
	vfs_iface_info.iface                      = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
		filestream_vfs_init(&vfs_iface_info);
}

void retro_get_system_info(struct retro_system_info *info)
{
	info->need_fullpath    = false;
	info->valid_extensions = "smc|fig|sfc|gd3|gd7|dx2|bsx|swc";
#ifdef GIT_VERSION
	info->library_version = CORE_VERSION GIT_VERSION;
#else
	info->library_version  = CORE_VERSION;
#endif
	info->library_name     = LIBRETRO_LIB_NAME;
	info->block_extract    = false;
}

/* Drain one frame of audio from the SPC and hand it to the frontend in a
 * single audio_batch_cb call. Zero-copy: S9xDrainAudio returns a pointer
 * directly into the SPC's landing_buffer, which RetroArch reads
 * synchronously inside audio_batch_cb.
 *
 * libretro pulls audio exactly once per retro_run via the batch callback.
 * The mid-frame "samples available" hook (S9xSetSamplesAvailableCallback)
 * that upstream Snes9x exposes for SDL-style frontends is not wired here;
 * samples accumulate in landing_buffer across the whole frame and ship in
 * one batch when the frame ends.
 *
 * Sizing: ~534 stereo frames per NTSC frame, ~641 per PAL, plus up to
 * 1.6% more on speedup-hack carts (the SPC produces samples at
 * TEMPO_UNIT / timing_hack_denominator times its normal rate and we
 * report that higher rate to the frontend - see S9xGetAudioSampleRate).
 *
 * On mute, send silence at the same cadence to keep the frontend's
 * audio clock fed. Skipping the call would let RetroArch's audio ring
 * drain and pull DRC out of its steady-state tracking. */
static void audio_upload_samples(void)
{
	int n;
	const int16_t *src = S9xDrainAudio(&n);
	if (n <= 0)
		return;

	if (audio_muted)
	{
		src = mute_buffer;
		if (n > MUTE_BUFFER_FRAMES * 2)
			n = MUTE_BUFFER_FRAMES * 2;
	}

	audio_batch_cb(src, (size_t)n >> 1);
}

void retro_set_controller_port_device(unsigned in_port, unsigned device)
{
	int port = in_port;

	if (port >= 2)
		return;

	switch (device)
	{
		case RETRO_DEVICE_JOYPAD:
			retro_devices[port] = RETRO_DEVICE_JOYPAD;
			S9xSetController(port, CTL_JOYPAD, port, 0, 0, 0);
		break;
		case RETRO_DEVICE_JOYPAD_MULTITAP:
			retro_devices[port] = RETRO_DEVICE_JOYPAD_MULTITAP;
			S9xSetController(port, CTL_MP5, port+0, port+2, port+4, port+6);
		break;
		case RETRO_DEVICE_MOUSE:
			retro_devices[port] = RETRO_DEVICE_MOUSE;
			S9xSetController(port, CTL_MOUSE, port, 0, 0, 0);
			/* mapping pointers here */
			S9xMapPointer((BTN_POINTER), S9xGetCommandT("Pointer Mouse1+Superscope+Justifier1"));
			S9xMapPointer((BTN_POINTER2), S9xGetCommandT("Pointer Mouse2"));
			/* mapping extra buttons here */
			LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_SELECT), "Mouse1 L");
			LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_START), "Mouse1 R");
			LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_SELECT), "Mouse2 L");
			LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_START), "Mouse2 R");
		break;
		case RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE:
			S9xSetController(port, CTL_SUPERSCOPE, 0, 0, 0, 0);
			retro_devices[port] = RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE;
			/* mapping pointers here */
			S9xMapPointer((BTN_POINTER), S9xGetCommandT("Pointer Mouse1+Superscope+Justifier1"));
			S9xMapPointer((BTN_POINTER2), S9xGetCommandT("Pointer Mouse2"));
			/* mapping extra buttons here */
			LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_SELECT), "Superscope Fire");
			LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_START), "Superscope Cursor");
			LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_UP), "Superscope ToggleTurbo");
			LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_DOWN), "Superscope Pause");
		break;
		case RETRO_DEVICE_LIGHTGUN_JUSTIFIER:
			S9xSetController(port, CTL_JUSTIFIER, 0, 0, 0, 0);
			retro_devices[port] = RETRO_DEVICE_LIGHTGUN_JUSTIFIER;
			/* mapping extra buttons here */
			LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_SELECT), "Justifier1 Trigger");
			LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_START), "Justifier1 Start");
		break;
		case RETRO_DEVICE_LIGHTGUN_JUSTIFIERS:
			S9xSetController(port, CTL_JUSTIFIER, 1, 0, 0, 0);
			retro_devices[port] = RETRO_DEVICE_LIGHTGUN_JUSTIFIERS;
		break;
		default:
			S9xMessage(S9X_MSG_ERROR, S9X_CATEGORY_EXTERNAL, "Invalid device!");
		break;
	}

	if (((retro_devices[0] == RETRO_DEVICE_JOYPAD) && retro_devices[1] == RETRO_DEVICE_JOYPAD) ||
		((retro_devices[0] == RETRO_DEVICE_JOYPAD) && retro_devices[1] == RETRO_DEVICE_JOYPAD_MULTITAP) ||
			((retro_devices[0] == RETRO_DEVICE_JOYPAD_MULTITAP) && retro_devices[1] == RETRO_DEVICE_JOYPAD))
		Settings.NormalControls = 1;
}

static void map_buttons(void)
{
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_A), "Joypad1 A");
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_B), "Joypad1 B");
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_X), "Joypad1 X");
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_Y), "Joypad1 Y");
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_SELECT), "Joypad1 Select");
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_START), "Joypad1 Start");
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_L), "Joypad1 L");
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_R), "Joypad1 R");
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_LEFT), "Joypad1 Left");
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_RIGHT), "Joypad1 Right");
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_UP), "Joypad1 Up");
	LR_MAP_BUTTON(MAKE_BUTTON(1, RETRO_DEVICE_ID_JOYPAD_DOWN), "Joypad1 Down");

	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_A), "Joypad2 A");
	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_B), "Joypad2 B");
	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_X), "Joypad2 X");
	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_Y), "Joypad2 Y");
	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_SELECT), "Joypad2 Select");
	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_START), "Joypad2 Start");
	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_L), "Joypad2 L");
	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_R), "Joypad2 R");
	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_LEFT), "Joypad2 Left");
	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_RIGHT), "Joypad2 Right");
	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_UP), "Joypad2 Up");
	LR_MAP_BUTTON(MAKE_BUTTON(2, RETRO_DEVICE_ID_JOYPAD_DOWN), "Joypad2 Down");

	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_A), "Joypad3 A");
	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_B), "Joypad3 B");
	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_X), "Joypad3 X");
	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_Y), "Joypad3 Y");
	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_SELECT), "Joypad3 Select");
	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_START), "Joypad3 Start");
	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_L), "Joypad3 L");
	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_R), "Joypad3 R");
	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_LEFT), "Joypad3 Left");
	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_RIGHT), "Joypad3 Right");
	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_UP), "Joypad3 Up");
	LR_MAP_BUTTON(MAKE_BUTTON(3, RETRO_DEVICE_ID_JOYPAD_DOWN), "Joypad3 Down");

	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_A), "Joypad4 A");
	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_B), "Joypad4 B");
	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_X), "Joypad4 X");
	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_Y), "Joypad4 Y");
	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_SELECT), "Joypad4 Select");
	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_START), "Joypad4 Start");
	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_L), "Joypad4 L");
	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_R), "Joypad4 R");
	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_LEFT), "Joypad4 Left");
	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_RIGHT), "Joypad4 Right");
	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_UP), "Joypad4 Up");
	LR_MAP_BUTTON(MAKE_BUTTON(4, RETRO_DEVICE_ID_JOYPAD_DOWN), "Joypad4 Down");

	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_A), "Joypad5 A");
	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_B), "Joypad5 B");
	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_X), "Joypad5 X");
	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_Y), "Joypad5 Y");
	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_SELECT), "Joypad5 Select");
	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_START), "Joypad5 Start");
	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_L), "Joypad5 L");
	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_R), "Joypad5 R");
	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_LEFT), "Joypad5 Left");
	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_RIGHT), "Joypad5 Right");
	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_UP), "Joypad5 Up");
	LR_MAP_BUTTON(MAKE_BUTTON(5, RETRO_DEVICE_ID_JOYPAD_DOWN), "Joypad5 Down");
}

float get_aspect_ratio(unsigned width, unsigned height)
{
	double sample_frequency_ntsc;
	double sample_frequency_pal;
	double sample_freq;
	double dot_rate;
    double par;

	if (aspect_ratio_mode == ASPECT_RATIO_4_3)
		return (4.0f / 3.0f);
	else if (aspect_ratio_mode == ASPECT_RATIO_1_1)
		return (float)width / (float)height;

	// OV2: not sure if these really make sense - NTSC is similar to 4:3, PAL looks weird
	sample_frequency_ntsc = 135000000.0f / 11.0f;
	sample_frequency_pal = 14750000.0;
	sample_freq = (retro_get_region() == RETRO_REGION_NTSC) ? sample_frequency_ntsc : sample_frequency_pal;
	dot_rate = (Settings.PAL ? PAL_MASTER_CLOCK : NTSC_MASTER_CLOCK) / 4.0;

	if (aspect_ratio_mode == ASPECT_RATIO_NTSC) // ntsc
	{
		sample_freq = sample_frequency_ntsc;
		dot_rate = NTSC_MASTER_CLOCK / 4.0;
	}
	else if (aspect_ratio_mode == ASPECT_RATIO_PAL) // pal
	{
		sample_freq = sample_frequency_pal;
		dot_rate = PAL_MASTER_CLOCK / 4.0;
	}

	par = sample_freq / 2.0 / dot_rate;

	return (float)(width * par / height);
}

static void update_geometry(void)
{
	struct retro_system_av_info av_info;
	retro_get_system_av_info(&av_info);
	environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &av_info);
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	unsigned width  = SNES_WIDTH;
	unsigned height = PPU.ScreenHeight;

	info->geometry.base_width   = width;
	info->geometry.base_height  = height;
	info->geometry.max_width    = MAX_BUFFER_WIDTH;
	info->geometry.max_height   = MAX_SNES_HEIGHT;
	info->geometry.aspect_ratio = get_aspect_ratio(width, height);

	/* The SPC's effective output rate is its native 32040 Hz for ordinary
	   carts and ~32550 Hz (or similar) for carts that use the APU speedup
	   hack (~70 game IDs handled in memmap.c). Reporting the effective
	   rate here lets the frontend's resampler handle conversion to the
	   host audio rate. */
	info->timing.sample_rate    = (double)S9xGetAudioSampleRate();
	info->timing.fps            = (retro_get_region() == RETRO_REGION_NTSC) ?
			VIDEO_REFRESH_RATE_NTSC : VIDEO_REFRESH_RATE_PAL;
}

static void set_system_specs(void)
{
	unsigned level = 7;
	environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

void retro_init(void)
{
	struct retro_log_callback log;
	struct retro_variable var;
	enum retro_pixel_format rgb565;
	bool achievements = true;

	if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
		log_cb = log.log;
	else
		log_cb = NULL;

	/* State that the core supports achievements. */
	environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &achievements);

	rgb565 = RETRO_PIXEL_FORMAT_RGB565;
	if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565))
		S9xMessage(S9X_MSG_INFO, S9X_CATEGORY_EXTERNAL, "Frontend supports RGB565 - will use that instead of XRGB1555.");

	if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
		libretro_supports_bitmasks = true;

	memset(&input_vars, 0, sizeof(input_vars));
	memset(&Settings, 0, sizeof(Settings));

	Settings.SpeedhackGameID = SPEEDHACK_NONE;
	Settings.FrameTimePAL = 20000;
	Settings.FrameTimeNTSC = 16667;
	Settings.HDMATimingHack = 100;
	Settings.CartAName[0] = 0;
	Settings.CartBName[0] = 0;
	Settings.Crosshair = 1;
	Settings.BlockInvalidVRAMAccessMaster = TRUE;

	var.key = "snes9x_2010_block_invalid_vram_access";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		Settings.BlockInvalidVRAMAccessMaster = (strcmp(var.value, "disabled") == 0) ? FALSE : TRUE;

	CPU.Flags = 0;

	if (!Init() || !S9xInitAPU())
	{
		Deinit();
		S9xMessage(S9X_MSG_ERROR, S9X_CATEGORY_EXTERNAL, "Failed to init Memory or APU.");
		exit(1);
	}

	S9xInitSound();

	GFX.Pitch = MAX_BUFFER_WIDTH * sizeof(uint16_t);

	/* Defensive teardown: if retro_init is re-entered without an
	   intervening retro_deinit (statically linked frontends, console
	   re-init paths), the screen allocations below would orphan the
	   prior buffers. Free via the canonical owned_* handles so we
	   release the original allocations even if a sw_fb redirect had
	   rewritten GFX.Screen to the frontend's swapchain. */
#if defined(_3DS)
	if (owned_screen_buffer)
		linearFree(owned_screen_buffer);
	if (owned_ntsc_buffer)
		linearFree(owned_ntsc_buffer);
#else
	if (owned_screen_buffer)
		free(owned_screen_buffer);
	if (owned_ntsc_buffer)
		free(owned_ntsc_buffer);
#endif
	owned_screen_buffer = NULL;
	owned_ntsc_buffer   = NULL;
	GFX.Screen          = NULL;
	ntsc_screen_buffer  = NULL;

#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200112L) && !defined(GEKKO) && !defined(_3DS) && !defined(__SWITCH__) && !defined(VITA)
	/* GFX.Pitch is already in bytes (= MAX_BUFFER_WIDTH * sizeof(uint16_t));
	   buffer size is Pitch * lines, not Pitch * lines * sizeof(uint16_t) again.
	   request 128-bit alignment here if possible.
	   posix_memalign output goes through void* temporaries to avoid the
	   strict-aliasing violation that '(void**)&GFX.Screen' would create. */
	{
		void *tmp_screen = NULL;
		void *tmp_ntsc   = NULL;
		if (posix_memalign(&tmp_screen, 16, GFX.Pitch * 512) != 0)
			tmp_screen = NULL;
		if (posix_memalign(&tmp_ntsc, 16, GFX.Pitch * MAX_SNES_HEIGHT) != 0)
			tmp_ntsc = NULL;
		GFX.Screen         = (uint16_t *)tmp_screen;
		ntsc_screen_buffer = (uint16_t *)tmp_ntsc;
	}
#elif defined(_3DS)
	GFX.Screen = (uint16_t*) linearMemAlign(GFX.Pitch * 512, 0x80);
	ntsc_screen_buffer = (uint16_t*)linearMemAlign(GFX.Pitch * MAX_SNES_HEIGHT, 0x80);
#else
	GFX.Screen = (uint16_t*) calloc(1, GFX.Pitch * 512);
	ntsc_screen_buffer = (uint16_t *)calloc(1, GFX.Pitch * MAX_SNES_HEIGHT);
#endif
	if ((!GFX.Screen || !ntsc_screen_buffer) && log_cb)
		log_cb(RETRO_LOG_ERROR, "Failed to allocate screen buffers.\n");

	/* Stash the canonical pointers so retro_deinit always frees what we
	   allocated, even if the sw_fb redirect leaves GFX.Screen pointing
	   at the frontend's swapchain at teardown time. */
	owned_screen_buffer = GFX.Screen;
	owned_ntsc_buffer   = ntsc_screen_buffer;
	S9xGraphicsInit();

	retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
	retro_set_controller_port_device(1, RETRO_DEVICE_JOYPAD);

	S9xUnmapAllControls();
	map_buttons();

	/* Initialize SuperFX CPU to normal speed by default */
	Settings.SuperFXSpeedPerLine = 0.417 * 10.5e6;
}

/* libretro uses relative values for analogue devices. 
   S9x seems to use absolute values, but do convert these into relative values in the core. (Why?!)
   Hack around it. :) */

void retro_deinit(void)
{
	/* If the user closes content with pause-on-menu disabled, retro_deinit
	   can fire while a frame is mid-render and GFX.Screen still points at
	   the frontend's swapchain buffer. Restore for tidiness, but the
	   actual robustness comes from freeing owned_screen_buffer below
	   instead of GFX.Screen. */
	if (sw_fb_saved_screen)
	{
		GFX.Screen   = sw_fb_saved_screen;
		GFX.Pitch    = sw_fb_saved_pitch;
		sw_fb_saved_screen = NULL;
		sw_fb_active = false;
	}

	Deinit();
	S9xGraphicsDeinit();
	S9xUnmapAllControls();
#if defined(_3DS)
	if (owned_screen_buffer)
		linearFree(owned_screen_buffer);
	if (owned_ntsc_buffer)
		linearFree(owned_ntsc_buffer);
#else
	free(owned_screen_buffer);
	free(owned_ntsc_buffer);
#endif
	owned_screen_buffer = NULL;
	owned_ntsc_buffer   = NULL;
	GFX.Screen          = NULL;
	ntsc_screen_buffer  = NULL;

	/* Reset globals (required for static builds) */
	libretro_supports_option_categories = false;
	libretro_supports_bitmasks = false;
	libretro_supports_sw_fb    = false;
	libretro_sw_fb_checked     = false;
	frameskip_type             = 0;
	frameskip_threshold        = 0;
	frameskip_counter          = 0;
	retro_audio_buff_active    = false;
	retro_audio_buff_occupancy = 0;
	retro_audio_buff_underrun  = false;
	retro_audio_latency        = 0;
	update_audio_latency       = false;
}

void retro_reset (void)
{
	S9xSoftReset();
}

uint16_t snes_lut[] = {
	SNES_B_MASK,		// 32768
	SNES_Y_MASK,		// 16384
	SNES_SELECT_MASK,	// 8192
	SNES_START_MASK,	// 4096
	SNES_UP_MASK,		// 2048
	SNES_DOWN_MASK,		// 1024
	SNES_LEFT_MASK,		// 512
	SNES_RIGHT_MASK,	// 256
	SNES_A_MASK,		// 128
	SNES_X_MASK,		// 64
	SNES_TL_MASK,		// 32
	SNES_TR_MASK		// 16
};

#define SWITCH_L2	(input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
#define PRESSED_R2 	(input_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))

static void report_buttons(void)
{
	uint32_t i, port;
	int16_t ret;

	for (port = 0; port < 2; port++)
	{
		ret = 0;
		switch (retro_devices[port])
		{
			case RETRO_DEVICE_JOYPAD:
			{
				bool pressed;
				uint16_t button_press;

				if (libretro_supports_bitmasks)
					ret = input_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
				else
				{
					for (i = RETRO_DEVICE_ID_JOYPAD_B; i <= RETRO_DEVICE_ID_JOYPAD_R; i++)
						if (input_cb(port, RETRO_DEVICE_JOYPAD, 0, i))
							ret |= (1 << i);
				}

				for (i = RETRO_DEVICE_ID_JOYPAD_B; i <= RETRO_DEVICE_ID_JOYPAD_R; i++)
				{
					pressed = ret & (1 << i);
					button_press = snes_lut[i];

					if (pressed)
						joypad[port] |= button_press;
					else
						joypad[port] &= ~button_press;
				}

				if (input_vars.turbo_enable == 1)
				{
					if (PRESSED_R2)
					{
						if (input_vars.turbo_state[port])
							input_vars.turbo_state[port]--;
						else
						{
							input_vars.turbo_state[port] = input_vars.turbo_delay;
							joypad[port] |= 0x4000;		// Button Y
						}
					}
					else
						input_vars.turbo_state[port] = 0;
				}

				if (input_vars.turbo_enable == 2)
				{
					if (PRESSED_R2)
					{
						if (input_vars.turbo_state[port])
							input_vars.turbo_state[port]--;
						else
						{
							input_vars.turbo_state[port] = input_vars.turbo_delay;
							joypad[port] |= 0x8000;		// Button B
						}
					}
					else
						input_vars.turbo_state[port] = 0;
				}
			}
			break;
			case RETRO_DEVICE_JOYPAD_MULTITAP:
			{
				uint32_t j;
				for (j = 0; j < 4; j++)
				{
					unsigned new_port = Settings.CurrentROMisMultitapCompatible ? (port + j) : (port);
					if (libretro_supports_bitmasks)
						ret = input_cb(new_port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
					else
					{
						for (i = RETRO_DEVICE_ID_JOYPAD_B; i <= RETRO_DEVICE_ID_JOYPAD_R; i++)
							if (input_cb(new_port, RETRO_DEVICE_JOYPAD, 0, i))
								ret |= (1 << i);
					}

					for (i = RETRO_DEVICE_ID_JOYPAD_B; i <= RETRO_DEVICE_ID_JOYPAD_R; i++)
					{
						bool pressed = ret & (1 << i);
						uint16_t button_press = snes_lut[i];

						if (pressed)
							joypad[j * 2 + port] |= button_press;
						else
							joypad[j * 2 + port] &= ~button_press;
					}
				}
			}
			break;
			case RETRO_DEVICE_MOUSE:
			{
				int32_t _x, _y;
				_x = input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
				_y = input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
				input_vars.retro_mouse_state[port][0] += _x;
				input_vars.retro_mouse_state[port][1] += _y;
				S9xApplyCommand(keymap[BTN_POINTER + port], input_vars.retro_mouse_state[port][0], input_vars.retro_mouse_state[port][1]);
				for (i = RETRO_DEVICE_ID_MOUSE_LEFT; i <= RETRO_DEVICE_ID_MOUSE_RIGHT; i++)
					S9xApplyCommand(keymap[MAKE_BUTTON(port + 1, i)], input_cb(port, RETRO_DEVICE_MOUSE, 0, i), 0);
			}
			break;
			case RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE:
				input_vars.retro_scope_state[0] += input_cb(port, RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE, 0, RETRO_DEVICE_ID_LIGHTGUN_X);
				input_vars.retro_scope_state[1] += input_cb(port, RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE, 0, RETRO_DEVICE_ID_LIGHTGUN_Y);
				S9xApplyCommand(keymap[BTN_POINTER], input_vars.retro_scope_state[0], input_vars.retro_scope_state[1]);

				for (i = RETRO_DEVICE_ID_LIGHTGUN_TRIGGER; i <= RETRO_DEVICE_ID_LIGHTGUN_PAUSE; i++)
					S9xApplyCommand(keymap[MAKE_BUTTON(port + 1, i)], input_cb(port, RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE, 0, i), 0);
			break;
			case RETRO_DEVICE_LIGHTGUN_JUSTIFIER:
			case RETRO_DEVICE_LIGHTGUN_JUSTIFIERS:
				input_vars.retro_justifier_state[0][0] += input_cb(port, RETRO_DEVICE_LIGHTGUN_JUSTIFIER, 0, RETRO_DEVICE_ID_LIGHTGUN_X);
				input_vars.retro_justifier_state[0][1] += input_cb(port, RETRO_DEVICE_LIGHTGUN_JUSTIFIER, 0, RETRO_DEVICE_ID_LIGHTGUN_Y);
				S9xApplyCommand(keymap[BTN_POINTER], input_vars.retro_justifier_state[0][0], input_vars.retro_justifier_state[0][1]);

				for (i = RETRO_DEVICE_ID_LIGHTGUN_TRIGGER; i <= RETRO_DEVICE_ID_LIGHTGUN_START; i++)
					S9xApplyCommand(keymap[MAKE_BUTTON(port + 1, i)], input_cb(port, RETRO_DEVICE_LIGHTGUN_JUSTIFIER, 0, i), 0);
			break;
			default:
				S9xMessage(S9X_MSG_ERROR, S9X_CATEGORY_EXTERNAL,"Unknown input device.");
			break;
		}
	}

   	if (SWITCH_L2 && !input_vars.switch_state)
   	{
		struct retro_message message;
        input_vars.switch_state = true;
		message.frames = 120;
		message.msg = NULL;

		switch (input_vars.turbo_enable)
		{
			case 0:
				input_vars.turbo_enable = 1;
				message.msg = "The button Y is mapped to R2 and autofire enabled.";
			break;
			case 1:
				input_vars.turbo_enable = 2;
				message.msg = "The button B is mapped to R2 and autofire enabled.";
			break;
			case 2:
				input_vars.turbo_enable = 0;
				message.msg = "The button mapping has been cancelled.";
			break;
		}
		environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, (void *)&message);
   	}
   	else if (!SWITCH_L2 && input_vars.switch_state)
		input_vars.switch_state = false;
}
#undef SWITCH_L2
#undef PRESSED_R2

/* Direct-render path for RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER.
 *
 * Why this exists: the frontend reuses a single buffer in its swapchain,
 * so handing the core a write-through pointer to that buffer lets the
 * renderer skip the end-of-frame copy from GFX.Screen. The libretro spec
 * is strict about this path: when video_cb is called with a pointer
 * obtained from sw_fb, the (data, width, height, pitch) values must
 * match exactly what the env call returned. We therefore acquire the
 * buffer at the exact dimensions for THIS frame, not the worst case.
 *
 * The hook lives in cpuexec.c at the same point that recomputes
 * IPPU.RenderedScreenWidth/Height for the new frame (V_Counter ==
 * FIRST_VISIBLE_LINE). At that point the frame's planned width/height
 * are known and no scanline has rendered yet, so swapping GFX.Screen
 * is safe.
 *
 * Mid-frame mode changes can promote the rendered area beyond what we
 * acquired (BGMode 5/6 or pseudo-hires switched on after the frame
 * started, or interlace toggled on). The promotion code in ppu.c calls
 * S9xLibretroSwFbAbort() before its line-doubling loops; that copies
 * whatever has been rendered so far back to the persistent GFX.Screen
 * (which is sized for the worst case) and restores the GFX.* fields.
 * The promotion then proceeds in the persistent buffer and the frame
 * finishes on the no-sw_fb path. */
static void sw_fb_acquire_internal(unsigned width, unsigned height)
{
	struct retro_framebuffer fb;

	/* Defensive restore: if a previous frame left GFX.Screen pointing
	   at a frontend buffer (sw_fb_saved_screen non-NULL means we have
	   a valid restoration point cached from an earlier acquire that
	   wasn't cleanly released), put it back BEFORE we evaluate any
	   conditions for this frame. This ensures the renderer always
	   writes at the canonical pitch when the NTSC filter is on or
	   when the redirect would otherwise be skipped, regardless of
	   how state got desynced. */
	if (sw_fb_saved_screen)
	{
		GFX.Screen        = sw_fb_saved_screen;
		GFX.Pitch         = sw_fb_saved_pitch;
		GFX.RealPPL       = GFX.Pitch >> 1;
		GFX.PPL           = GFX.RealPPL << (IPPU.DoubleHeightPixels ? 1 : 0);
		sw_fb_saved_screen = NULL;
	}
	sw_fb_active = false;

	if (!libretro_supports_sw_fb || !IPPU.RenderThisFrame || snes_ntsc_filter)
		return;
	if (width == 0 || height == 0)
		return;

	memset(&fb, 0, sizeof(fb));
	fb.width        = width;
	fb.height       = height;
	fb.access_flags = RETRO_MEMORY_ACCESS_WRITE;

	if (!environ_cb(RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER, &fb))
		return;
	if (fb.format != RETRO_PIXEL_FORMAT_RGB565)
		return;
	if (!fb.data)
		return;
	/* Reject buffers wider than our persistent GFX.Screen: the renderer
	   indexes GFX.SubScreen with the same Offset += GFX.PPL stride it
	   uses for GFX.Screen, and SubScreen is sized at init for the
	   max-width pitch. A wider stride here would walk past SubScreen's
	   allocation. Smaller is fine - we just use a sub-region of
	   SubScreen. */
	if ((int)fb.pitch > GFX.Pitch)
		return;
	/* Sanity: pitch must accommodate the requested width. */
	if (fb.pitch < width * sizeof(uint16_t))
		return;

	sw_fb_saved_screen = GFX.Screen;
	sw_fb_saved_pitch  = GFX.Pitch;

	GFX.Screen   = (uint16_t *)fb.data;
	GFX.Pitch    = (int)fb.pitch;
	GFX.RealPPL  = GFX.Pitch >> 1;
	GFX.PPL      = GFX.RealPPL << (IPPU.DoubleHeightPixels ? 1 : 0);

	sw_fb_data   = fb.data;
	sw_fb_width  = width;
	sw_fb_height = height;
	sw_fb_pitch  = fb.pitch;
	sw_fb_active = true;
}

void S9xLibretroSwFbAcquire(int width, int height)
{
	sw_fb_acquire_internal((unsigned)width, (unsigned)height);
}

/* Abort an in-progress sw_fb redirect. Called from ppu.c when mid-frame
   resolution promotion forces us off the direct-render path. Copies any
   pixels rendered so far from fb.data back to the original GFX.Screen,
   then restores the GFX.* fields. After this returns sw_fb_active is
   false and GFX.Screen points at the persistent worst-case buffer. */
void S9xLibretroSwFbAbort(void)
{
	unsigned y;
	const uint8_t *src;
	uint8_t *dst;
	size_t copy_bytes;

	if (!sw_fb_active)
		return;

	/* Copy the partial render back. We don't know how much of the buffer
	   was actually written, but the renderer has been writing scanlines
	   0..(height-1) in order via GFX.S, and the post-render hires/
	   interlace promotion is the only thing that needs to re-read those
	   pixels. Copy the full acquired region; cost is bounded by
	   sw_fb_pitch * sw_fb_height = at most 1208 * 478 = 577 KB, only
	   when a mid-frame promotion fires. */
	src = (const uint8_t *)sw_fb_data;
	dst = (uint8_t *)sw_fb_saved_screen;
	copy_bytes = sw_fb_width * sizeof(uint16_t);
	for (y = 0; y < sw_fb_height; y++)
	{
		memcpy(dst, src, copy_bytes);
		src += sw_fb_pitch;
		dst += sw_fb_saved_pitch;
	}

	GFX.Screen   = sw_fb_saved_screen;
	GFX.Pitch    = sw_fb_saved_pitch;
	GFX.RealPPL  = GFX.Pitch >> 1;
	GFX.PPL      = GFX.RealPPL << (IPPU.DoubleHeightPixels ? 1 : 0);

	sw_fb_active = false;
	sw_fb_saved_screen = NULL;  /* mark restored */
}

/* Release the redirect after S9xMainLoop completes successfully (no
   abort). Restores GFX.* without copying back, since S9xDeinitUpdate
   already called video_cb on fb.data. */
static void sw_fb_release_after_frame(void)
{
	if (!sw_fb_active)
		return;

	GFX.Screen = sw_fb_saved_screen;
	GFX.Pitch  = sw_fb_saved_pitch;
	GFX.RealPPL = GFX.Pitch >> 1;
	GFX.PPL     = GFX.RealPPL << (IPPU.DoubleHeightPixels ? 1 : 0);
	sw_fb_active = false;
	sw_fb_saved_screen = NULL;  /* mark restored - prevents the
	                               defensive re-restore in the next
	                               acquire from undoing valid state */
}

void retro_run(void)
{
	int result = -1;
	bool okay = false;
	bool updated = false;

	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
	{
		check_variables(false);
		update_geometry();
	}

	okay = (environ_cb(RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE, &result));
	if (okay)
	{
		bool audioEnabled = 0 != (result & 0x02);
		bool videoEnabled = 0 != (result & 0x01);
		bool hardDisableAudio = 0 != (result & 0x08);
		IPPU.RenderThisFrame = videoEnabled;

		audio_muted = !audioEnabled || hardDisableAudio;
		Settings.HardDisableAudio = hardDisableAudio;
	}
	else
	{
		IPPU.RenderThisFrame = true;
		audio_muted = false;
		Settings.HardDisableAudio = false;
	}

	if ((frameskip_type > 0) &&
	    retro_audio_buff_active &&
	    IPPU.RenderThisFrame)
	{
		bool skip_frame;
		switch (frameskip_type)
		{
			case 1: /* auto */
				skip_frame = retro_audio_buff_underrun;
				break;
			case 2: /* manual */
				skip_frame = (retro_audio_buff_occupancy < frameskip_threshold);
				break;
			default:
				skip_frame = false;
				break;
		}

		if (skip_frame)
		{
			if (frameskip_counter < FRAMESKIP_MAX)
			{
				IPPU.RenderThisFrame = false;
				frameskip_counter++;
			}
			else
				frameskip_counter = 0;
		}
	}
	else
		frameskip_counter = 0;

	/* If frameskip/timing settings have changed,
	 * update frontend audio latency
	 * > Can do this before or after the frameskip
	 *   check, but doing it after means we at least
	 *   retain the current frame's audio output */
	if (update_audio_latency)
	{
		environ_cb(RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY, &retro_audio_latency);
		update_audio_latency = false;
  }

	poll_cb();
	report_buttons();

	S9xMainLoop();
	sw_fb_release_after_frame();

	audio_upload_samples();
}

size_t retro_serialize_size(void)
{
	int32_t size = SnapshotSize();

	if (size < 0)
		return 0;

	return (size_t)size;
}

bool retro_serialize(void *data, size_t size)
{
	int result = -1;
	if (environ_cb(RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE, &result))
		Settings.FastSavestates = 0 != (result & 0x04);

	memstream_set_buffer((uint8_t*)data, (uint64_t)size);

	if (S9xFreezeGame() == FALSE)
		return FALSE;

	return TRUE;
}

bool retro_unserialize(const void * data, size_t size)
{
	int result = -1;
	if (environ_cb(RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE, &result))
		Settings.FastSavestates = 0 != (result & 0x04);

	memstream_set_buffer((uint8_t*)data, (uint64_t)size);

	if (S9xUnfreezeGame() == FALSE)
		return FALSE;

	return TRUE;
}

void retro_cheat_reset(void)
{
	S9xDeleteCheats();
	S9xApplyCheats();
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	uint32_t address;
	uint8_t val;

	uint8_t sram;
	uint8_t bytes[3]; /* used only by GoldFinger, ignored for now */

	if (S9xGameGenieToRaw(code, &address, &val) != NULL &&
		S9xProActionReplayToRaw(code, &address, &val) != NULL &&
		S9xGoldFingerToRaw(code, &address, &sram, &val, bytes) != NULL)
	{
		/* Ignore bad codes. */
		return;
	}
	if (index > Cheat.num_cheats)
		return; /* Ignore cheat added in weird order. */

	if (index == Cheat.num_cheats)
		Cheat.num_cheats++;

	Cheat.c[index].address = address;
	Cheat.c[index].byte = val;
	Cheat.c[index].enabled = enabled;

	Cheat.c[index].saved = FALSE; /* it'll be saved next time cheats run */

	Settings.ApplyCheats = true;
	S9xApplyCheats();
}

#define MAX_MAPS 32
static struct retro_memory_descriptor memorydesc[MAX_MAPS];
static unsigned memorydesc_c;

static bool merge_mapping(void)
{
	struct retro_memory_descriptor *a, *b;
	uint32_t len;

	if (memorydesc_c == 1)
		return false; /* can't merge the only one */

	a = &memorydesc[MAX_MAPS - (memorydesc_c - 1)];
	b = &memorydesc[MAX_MAPS - memorydesc_c];

	if (a->flags != b->flags)
		return false;
	if (a->disconnect != b->disconnect)
		return false;
	if (a->len != b->len)
		return false;
	if (a->addrspace || b->addrspace)
		return false; /* we don't use these */

	if (((char*)a->ptr) + a->offset == ((char*)b->ptr) + b->offset && a->select == b->select)
	{
		a->select &= ~(a->start ^ b->start);
		memorydesc_c--;
		return true;
	}

	len = a->len;
	if (!len)
		len=(0x1000000 - a->select);

	if (len && ((len - 1) & (len | a->disconnect)) == 0 && ((char*)a->ptr) + a->offset + len == ((char*)b->ptr) + b->offset)
	{
		a->select &= ~len;
		a->disconnect &= ~len;
		memorydesc_c--;
		return true;
	}
	return false;
}

void S9xAppendMapping(struct retro_memory_descriptor *desc)
{
	/* do it backwards - snes9x defines the last one to win, while we define
	 * the first one to win */
	memcpy(&memorydesc[MAX_MAPS - (++memorydesc_c)], desc, sizeof(struct retro_memory_descriptor));
	while (merge_mapping())
	{
	}
}

static void init_descriptors(void)
{
#define describe_buttons(INDEX)	\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },	\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },		\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },	\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },		\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "B" },		\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "A" },			\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "X" },		\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Y" },			\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "L" },		\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "R" },			\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,   "Select" },	\
		{ INDEX, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,    "Start" },

	struct retro_input_descriptor desc[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Switch Autofire Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "AutoFire Button" },
		describe_buttons(0)
		describe_buttons(1)
		describe_buttons(2)
		describe_buttons(3)
		describe_buttons(4)
		{ 0, 0, 0, 0, NULL }
	};
	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
}

bool retro_load_game(const struct retro_game_info *game)
{
	int loaded;
	struct retro_memory_map map;

	init_descriptors();
	memorydesc_c = 0;

	/* Hack. S9x cannot do stuff from RAM. <_< */
	memstream_set_buffer((uint8_t*)game->data, (uint64_t)game->size);

	loaded = LoadROM();
	if (!loaded)
	{
		const char *const err_msg = "ROM loading failed.";
		struct retro_message msg = { err_msg, 360 };

		S9xMessage(S9X_MSG_ERROR, S9X_CATEGORY_EXTERNAL, err_msg);

		if (environ_cb)
			environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);

		return FALSE;
	}

	check_variables(true);

	retro_set_audio_buff_status_cb();
	set_system_specs();

	map.descriptors = memorydesc + MAX_MAPS - memorydesc_c;
	map.num_descriptors = memorydesc_c;

	environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &map);

	return TRUE;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
	(void)game_type;
	(void)info;
	(void)num_info;

	return false;
}

void retro_unload_game (void)
{
	/* See retro_deinit: with pause-on-menu disabled the frontend may
	   tear down while a frame is mid-render. Unwind any active sw_fb
	   redirect so subsequent code (including a re-load_game without an
	   intervening retro_deinit) sees GFX.Screen at its original value. */
	if (sw_fb_saved_screen)
	{
		GFX.Screen   = sw_fb_saved_screen;
		GFX.Pitch    = sw_fb_saved_pitch;
		sw_fb_saved_screen = NULL;
		sw_fb_active = false;
	}
}

unsigned retro_get_region (void)
{
	return Settings.PAL ? RETRO_REGION_PAL : RETRO_REGION_NTSC;
}

void S9xDeinitUpdate(int width, int height)
{
	if (!IPPU.RenderThisFrame)
		video_cb(NULL, width, height, GFX.Pitch);
	else if (snes_ntsc_filter)
	{
		/* Both blitters (lores and hires) produce the same number of
		   NTSC output pixels per scanline - the hires variant just
		   consumes input pixels at twice the rate per output chunk.
		   For a 256-wide lores frame and a 512-wide hires frame the
		   output is identical at SNES_NTSC_OUT_WIDTH(256) = 602
		   pixels. SNES_NTSC_OUT_WIDTH() is documented in
		   filter/snes_ntsc.h as the LOW-RES output width formula and
		   gives the wrong (1197) answer when applied to hires width
		   - using that as the video_cb width would tell the frontend
		   to display a 1197-wide frame whose rightmost ~595 pixels
		   were never written. */
		unsigned ntsc_out_width = SNES_NTSC_OUT_WIDTH(SNES_WIDTH);
		size_t   ntsc_out_pitch = (size_t)ntsc_out_width * sizeof(uint16_t);
		/* Tie the chroma burst phase to the emulated frame counter so
		   the NTSC output is reproducible across retro_init/_deinit
		   cycles and is unaffected by libretro frameskip (a skipped
		   frame doesn't visit this branch but still bumps ICPU.Frame
		   in cpuexec.c, so the phase advances either way). */
		int burst_phase = (int)(ICPU.Frame % 3);

		if (width == 512)
			snes_ntsc_blit_hires(&snes_ntsc, GFX.Screen, GFX.Pitch / 2, burst_phase, width, height, ntsc_screen_buffer, (long)ntsc_out_pitch);
		else
			snes_ntsc_blit(&snes_ntsc, GFX.Screen, GFX.Pitch / 2, burst_phase, width, height, ntsc_screen_buffer, (long)ntsc_out_pitch);

		video_cb(ntsc_screen_buffer, ntsc_out_width, height, ntsc_out_pitch);
	}
	else
	{
		/* Lazily probe sw_fb support on first non-NTSC frame. The probe
		   only sets the libretro_supports_sw_fb flag; subsequent frames
		   acquire via the cpuexec.c hook (S9xLibretroSwFbAcquire) at
		   start-of-frame, where this frame's exact dimensions are known. */
		if (!libretro_sw_fb_checked)
		{
			struct retro_framebuffer fb;
			memset(&fb, 0, sizeof(fb));
			fb.width         = width;
			fb.height        = height;
			fb.access_flags  = RETRO_MEMORY_ACCESS_WRITE;

			if (environ_cb(RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER, &fb)
					&& fb.format == RETRO_PIXEL_FORMAT_RGB565)
			{
				libretro_supports_sw_fb = true;
				if (log_cb)
					log_cb(RETRO_LOG_INFO, "Software framebuffer acquired successfully.\n");
			}

			libretro_sw_fb_checked = true;
		}

		if (sw_fb_active)
		{
			/* Renderer wrote directly into the frontend's buffer; just
			   present it. video_cb gets exactly the (data, width,
			   height, pitch) values returned by the env call. No copy. */
			video_cb(sw_fb_data, sw_fb_width, sw_fb_height, sw_fb_pitch);
		}
		else
		{
			/* Either sw_fb is unsupported, or this frame's acquire
			   was skipped/aborted. Hand GFX.Screen to the frontend;
			   the frontend's own copy into its swapchain is
			   unavoidable on this path. */
			video_cb(GFX.Screen, width, height, GFX.Pitch);
		}
	}
}

/* Dummy functions that should probably be implemented correctly later. */
const char* S9xGetDirectory(uint32_t dirtype)
{
	const char *directory = NULL;

	if (dirtype == BIOS_DIR)
	{
		if (!environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &directory) ||
		    !directory)
		{
			if (log_cb)
				log_cb(RETRO_LOG_WARN, "No system directory defined, unable to load bios files.\n");
		}
	}

	return directory;
}

static enum retro_log_level s9x_msg_priority_to_retro_log(S9xMessagePriority p)
{
   switch (p)
   {
      case S9X_MSG_VERBOSE:
         return RETRO_LOG_DEBUG;
      case S9X_MSG_INFO:
         return RETRO_LOG_INFO;
      case S9X_MSG_WARN:
         return RETRO_LOG_WARN;
      case S9X_MSG_ERROR:
         return RETRO_LOG_ERROR;
   }
   return RETRO_LOG_INFO;
}

void S9xMessage (S9xMessagePriority p, S9xMessageCategory c, const char *msg)
{
	const char *const S9xMessageCategoryStr[] = {
		"ROM", "PPU", "CPU", "APU", "MAP", "CONTROLS", "SNAPSHOT", "EXT"
	};
   enum retro_log_level p_info = s9x_msg_priority_to_retro_log(p);

	if (log_cb)
		log_cb(p_info, "%s: %s\n", S9xMessageCategoryStr[c], msg);
}
