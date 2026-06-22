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


#ifndef _APU_H_
#define _APU_H_

#include "snes9x.h"

typedef void (*dsp_copy_func_t)( unsigned char** io, void* state, size_t );

#define ECHO_HIST_SIZE		8
#define ECHO_HIST_SIZE_X2	16

/* Sound control */

/* Mutes voices corresponding to non-zero bits in mask (issues repeated KOFF events).
   Reduces emulation accuracy. */

#define VOICE_COUNT		8
#define BRR_BUF_SIZE		12
#define BRR_BUF_SIZE_X2		24
#define BRR_BLOCK_SIZE		9

/* DSP register addresses */

/* Global registers */

#define R_MVOLL 0x0C
#define R_MVOLR 0x1C
#define R_EVOLL 0x2C
#define R_EVOLR 0x3C
#define R_KON	0x4C
#define R_KOFF	0x5C
#define R_FLG	0x6C
#define R_ENDX	0x7C
#define R_EFB	0x0D
#define R_EON	0x4D
#define R_PMON	0x2D
#define R_NON	0x3D
#define R_DIR	0x5D
#define R_ESA	0x6D
#define R_EDL	0x7D
#define R_FIR	0x0F	/* 8 coefficients at 0x0F, 0x1F ... 0x7F */

/* Voice registers */
#define V_VOLL		0x00
#define V_VOLR		0x01
#define V_PITCHL	0x02
#define V_PITCHH	0x03
#define V_SRCN		0x04
#define V_ADSR0		0x05
#define V_ADSR1		0x06
#define V_GAIN		0x07
#define V_ENVX		0x08
#define V_OUTX		0x09

/* Status flag handling */

/* Hex value in name to clarify code and bit shifting.
   Flag stored in indicated variable during emulation */

#define N80 0x80	/* nz */
#define V40 0x40	/* psw */
#define P20 0x20	/* dp */
#define B10 0x10	/* psw */
#define H08 0x08	/* psw */
#define I04 0x04	/* psw */
#define Z02 0x02	/* nz */
#define C01 0x01	/* c */

#define NZ_NEG_MASK 0x880	/* either bit set indicates N flag set */

#define REGISTER_COUNT 128

#define ENV_RELEASE	0
#define ENV_ATTACK	1
#define ENV_DECAY	2
#define ENV_SUSTAIN	3

typedef struct
{
	/* pointer to voice's DSP registers */
	uint8_t* regs;
	/* decoded samples (twice the size to simplify wrap handling) */
	int buf [BRR_BUF_SIZE_X2];
	/* place in buffer where next samples will be decoded */
	int buf_pos;
	/* relative fractional position in sample (0x1000 = 1.0) */
	int interp_pos;
	/* address of current BRR block */
	int brr_addr;
	/* current decoding offset in BRR block */
	int brr_offset;
	/* bitmask for voice: 0x01 for voice 0, 0x02 for voice 1, etc. */
	int vbit;
	/* KON delay/current setup phase */
	int kon_delay;
	int env_mode;
	/* current envelope level */
	int env;
	/* used by GAIN mode 7, very obscure quirk */
	int hidden_env;
	uint8_t t_envx_out;
} dsp_voice_t;

typedef struct
{
	int (*echo_hist_pos) [2]; /* &echo_hist [0 to 7] */
   dsp_voice_t voices [VOICE_COUNT]; /* ptr alignment */

	uint8_t *rom, *hi_ram;
	uint8_t* ram; /* 64K shared RAM between DSP and SMP */
	short* out;
	short* out_end;
	short* out_begin;

	/* Echo history keeps most recent 8 samples 
           (twice the size to simplify wrap handling) */

	int echo_hist [ECHO_HIST_SIZE_X2] [2];


	int every_other_sample; /* toggles every sample */
	int kon;                /* KON value when last checked */
	int noise;
	int counter;
	int echo_offset;        /* offset from ESA in echo buffer */
	int echo_length;        /* number of bytes that echo_offset will stop at */
	int phase;              /* next clock cycle to run (0-31) */

	/* Hidden registers also written to when main register is written to */
	int new_kon;

	/* Temporary state between clocks */

	/* read once per sample */
	int t_pmon;
	int t_non;
	int t_eon;
	int t_dir;
	int t_koff;

	/* read a few clocks ahead then used */
	int t_brr_next_addr;
	int t_adsr0;
	int t_brr_header;
	int t_brr_byte;
	int t_srcn;
	int t_esa;
	int t_echo_enabled;

	/* internal state that is recalculated every sample */
	int t_dir_addr;
	int t_pitch;
	int t_output;
	int t_looped;
	int t_echo_ptr;

	/* left/right sums */
	int t_main_out [2];
	int t_echo_out [2];
	int t_echo_in  [2];

	int rom_enabled;

	uint8_t endx_buf;
	uint8_t envx_buf;
	uint8_t outx_buf;
	uint8_t regs [REGISTER_COUNT];
} dsp_state_t;

typedef struct {
	dsp_copy_func_t func;
	unsigned char** buf;
} spc_state_copy_t;

#define SPC_COPY( type, state ) state = (type) spc_copier_copy_int(&copier, state, sizeof (type) );

#define REG_COUNT	0x10
#define PORT_COUNT	4
#define TEMPO_UNIT	0x100
#define STATE_SIZE	68 * 1024L /* maximum space needed when saving */
#define TIMER_COUNT	3
#define ROM_SIZE	0x40
#define ROM_ADDR	0xFFC0

/* 1024000 SPC clocks per second, sample pair every 32 clocks */
#define CLOCKS_PER_SAMPLE 32

#define R_TEST		0x0
#define R_CONTROL	0x1
#define R_DSPADDR	0x2
#define R_DSPDATA	0x3
#define R_CPUIO0	0x4
#define R_CPUIO1	0x5
#define R_CPUIO2	0x6
#define R_CPUIO3	0x7
#define R_F8		0x8
#define R_F9		0x9
#define R_T0TARGET	0xA
#define R_T1TARGET	0xB
#define R_T2TARGET	0xC
#define R_T0OUT		0xD
#define R_T1OUT		0xE
#define R_T2OUT		0xF

/* Value that padding should be filled with */
#define CPU_PAD_FILL 0xFF

/* Saves/loads state */
void spc_copy_state( unsigned char** io, dsp_copy_func_t );

/* rel_time_t - Time relative to m_spc_time. Speeds up code a bit by eliminating
   need to constantly add m_spc_time to time from CPU. CPU uses time that ends 
   at 0 to eliminate reloading end time every instruction. It pays off. */

typedef struct
{
	int next_time; /* time of next event */
	int prescaler;
	int period;
	int divider;
	int enabled;
	int counter;
} Timer;

/* Support SNES_MEMORY_APURAM */
uint8_t *spc_apuram (void);

typedef struct
{
	struct
	{
		int pc;
		int a;
		int x;
		int y;
		int psw;
		int sp;
	} cpu_regs;

	int  dsp_time;
	int  spc_time;

	int         tempo;

	int         rom_enabled;

	Timer timers [TIMER_COUNT]; /* int alignment */

	uint8_t     rom    [ROM_SIZE];
	uint8_t     hi_ram [ROM_SIZE];

	uint8_t smp_regs [2] [REG_COUNT];

	unsigned char cycle_table [256];

	struct
	{
		/* TODO: Remove padding. */
		/* padding to neutralize address overflow */
		union {
			uint8_t padding1 [0x100];
			uint16_t align; /* makes compiler align data for 16-bit access */
		} padding1 [1];
		uint8_t ram      [0x10000];
		uint8_t padding2 [0x100];
	} ram;

} spc_state_t;

#define SPC_SAVE_STATE_BLOCK_SIZE	(STATE_SIZE + 8)

uint8_t S9xInitAPU (void);
/* DSP voice interpolation mode core option: gaussian (accurate),
 * cubic (inaccurate, brighter). dsp_interp_mode is defined in apu.c. */
#define DSP_INTERP_GAUSSIAN 0
#define DSP_INTERP_CUBIC    1
#define DSP_INTERP_SINC     2
extern int dsp_interp_mode;

void S9xResetAPU (void);
void S9xSoftResetAPU (void);
uint8_t S9xAPUReadPort (int port);
void S9xAPUWritePort (int port, uint8_t byte);
void S9xAPUExecute (void);
void S9xAPUSetReferenceTime (int32_t cpucycles);
void S9xAPUTimingSetSpeedup (int ticks);
void S9xAPUAllowTimeOverflow (uint8_t allow);
void S9xAPULoadState (uint8_t * block);
void S9xAPUSaveState (uint8_t * block);

void S9xInitSound (void);

const short *S9xDrainAudio (int *count_out);
unsigned S9xGetAudioSampleRate (void);

#endif
