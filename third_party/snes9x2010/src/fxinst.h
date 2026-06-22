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


#ifndef _FXINST_H_
#define _FXINST_H_

/*
 * FxChip(GSU) register space specification
 * (Register address space 3000-32ff)
 *
 * The 16 generic 16 bit registers:
 * (Some have a special function in special circumstances)
 * 3000 - R0    default source/destination register
 * 3002 - R1    pixel plot X position register
 * 3004 - R2    pixel plot Y position register
 * 3006 - R3
 * 3008 - R4    lower 16 bit result of lmult
 * 300a - R5
 * 300c - R6    multiplier for fmult and lmult
 * 300e - R7    fixed point texel X position for merge
 * 3010 - R8    fixed point texel Y position for merge
 * 3012 - R9
 * 3014 - R10
 * 3016 - R11   return address set by link
 * 3018 - R12   loop counter
 * 301a - R13   loop point address
 * 301c - R14   rom address for getb, getbh, getbl, getbs
 * 301e - R15   program counter
 *
 * 3020-302f -  unused
 *
 * Other internal registers
 * 3030 - SFR   status flag register  (16bit)
 * 3032 - unused
 * 3033 - BRAMR Backup RAM register    (8bit)
 * 3034 - PBR   program bank register  (8bit)
 * 3035 - unused
 * 3036 - ROMBR rom bank register      (8bit)
 * 3037 - CFGR  control flags register (8bit)
 * 3038 - SCBR  screen base register   (8bit)
 * 3039 - CLSR  clock speed register   (8bit)
 * 303a - SCMR  screen mode register   (8bit)
 * 303b - VCR   version code register  (8bit) (read only)
 * 303c - RAMBR ram bank register      (8bit)
 * 303d - unused
 * 303e - CBR   cache base register   (16bit)
 *
 * 3040-30ff -  unused
 *
 * 3100-32ff -  CACHERAM 512 bytes of GSU cache memory
 *
 * SFR status flag register bits:
 *  0   -
 *  1   Z     Zero flag
 *  2   CY    Carry flag
 *  3   S     Sign flag
 *  4   OV    Overflow flag
 *  5   G     Go flag (set to 1 when the GSU is running)
 *  6   R     Set to 1 when reading ROM using R14 address
 *  7   -
 *  8   ALT1  Mode set-up flag for the next instruction
 *  9   ALT2  Mode set-up flag for the next instruction
 * 10   IL    Immediate lower 8-bit flag
 * 11   IH    Immediate higher 8-bit flag
 * 12   B     Set to 1 when the WITH instruction is executed
 * 13   -
 * 14   -
 * 15   IRQ   Set to 1 when GSU caused an interrupt
 *            Set to 0 when read by 658c16
 *
 * BRAMR = 0, BackupRAM is disabled
 * BRAMR = 1, BackupRAM is enabled
 *
 * CFGR control flags register bits:
 *  0   -
 *  1   -
 *  2   -
 *  3   -
 *  4   -
 *  5   MS0   Multiplier speed, 0=standard, 1=high speed
 *  6   -
 *  7   IRQ   Set to 1 when GSU interrupt request is masked
 *
 * CLSR clock speed register bits:
 *  0   CLSR  clock speed, 0 = 10.7Mhz, 1 = 21.4Mhz
 *
 * SCMR screen mode register bits:
 *  0	MD0   color depth mode bit 0
 *  1	MD1   color depth mode bit 1
 *  2	HT0   screen height bit 1
 *  3	RAN   RAM access control
 *  4	RON   ROM access control
 *  5	HT1   screen height bit 2
 *  6	-
 *  7	-
 *
 * RON = 0    SNES CPU has ROM access
 * RON = 1    GSU has ROM access
 *
 * RAN = 0    SNES has game pak RAM access
 * RAN = 1    GSU has game pak RAM access
 *
 * HT1  HT0   Screen height mode
 *  0    0    128 pixels high
 *  0    1    160 pixels high
 *  1    0    192 pixels high
 *  1    1    OBJ mode
 *
 * MD1  MD0   Color depth mode
 *  0    0    4   color mode
 *  0    1    16  color mode
 *  1    0    not used
 *  1    1    256 color mode
 *
 * CBR cache base register bits:
 * 15-4       Specify base address for data to cache from ROM or RAM
 *  3-0       Are 0 when address is read
 *
 * Write access to the program counter (301e) from
 * the SNES-CPU will start the GSU, and it will not
 * stop until it reaches a stop instruction.
 *
 */

/* GSU registers */
#define GSU_R0			0x000
#define GSU_R1			0x002
#define GSU_R2			0x004
#define GSU_R3			0x006
#define GSU_R4			0x008
#define GSU_R5			0x00a
#define GSU_R6			0x00c
#define GSU_R7			0x00e
#define GSU_R8			0x010
#define GSU_R9			0x012
#define GSU_R10			0x014
#define GSU_R11			0x016
#define GSU_R12			0x018
#define GSU_R13			0x01a
#define GSU_R14			0x01c
#define GSU_R15			0x01e
#define GSU_SFR			0x030
#define GSU_BRAMR		0x033
#define GSU_PBR			0x034
#define GSU_ROMBR		0x036
#define GSU_CFGR		0x037
#define GSU_SCBR		0x038
#define GSU_CLSR		0x039
#define GSU_SCMR		0x03a
#define GSU_VCR			0x03b
#define GSU_RAMBR		0x03c
#define GSU_CBR			0x03e
#define GSU_CACHERAM		0x100

/* SFR flags */
#define FLG_Z			2
#define FLG_CY			4
#define FLG_S			8
#define FLG_OV			16
#define FLG_G			32
#define FLG_R			64
#define FLG_ALT1		256
#define FLG_ALT2		512
#define FLG_IL			1024
#define FLG_IH			2048
#define FLG_B			4096
#define FLG_IRQ			32768

/* Number of banks in GSU RAM */
#define FX_RAM_BANKS	4

/* Emulate proper R14 ROM access (slower, but safer)
   Without this, Doom has garbled graphics */
#define FX_DO_ROMBUFFER

struct FxRegs_s
{
	/* FxChip registers */
	uint32_t	avReg[16];				/* 16 Generic registers */
	uint32_t	vColorReg;				/* Internal color register */
	uint32_t	vPlotOptionReg;				/* Plot option register */
	uint32_t	vStatusReg;				/* Status register */
	uint32_t	vPrgBankReg;				/* Program bank index register */
	uint32_t	vRomBankReg;				/* Rom bank index register */
	uint32_t	vRamBankReg;				/* Ram bank index register */
	uint32_t	vCacheBaseReg;				/* Cache base address register */
	uint32_t	vCacheFlags;				/* Saying what parts of the cache was written to */
	uint32_t	vLastRamAdr;				/* Last RAM address accessed */
	uint32_t	*pvDreg;				/* Pointer to current destination register */
	uint32_t	*pvSreg;				/* Pointer to current source register */
	uint8_t	vRomBuffer;				/* Current byte read by R14 */
	uint8_t	vPipe;					/* Instructionset pipe */
	uint32_t	vPipeAdr;				/* The address of where the pipe was read from */

	/* Status register optimization stuff */
	uint32_t	vSign;					/* v & 0x8000 */
	uint32_t	vZero;					/* v == 0 */
	uint32_t	vCarry;					/* a value of 1 or 0 */
	int32_t	vOverflow;				/* (v >= 0x8000 || v < -0x8000) */

	/* Other emulator variables */
	int32_t	vErrorCode;
	uint32_t	vIllegalAddress;

	uint8_t	bBreakPoint;
	uint32_t	vBreakPoint;
	uint32_t	vStepPoint;

	uint8_t	*pvRegisters;				/* 768 bytes located in the memory at address 0x3000 */
	uint32_t	nRamBanks;				/* Number of 64kb-banks in FxRam (Don't confuse it with SNES-Ram!!!) */
	uint8_t	*pvRam;					/* Pointer to FxRam */
	uint32_t	nRomBanks;				/* Number of 32kb-banks in Cart-ROM */
	uint8_t	*pvRom;					/* Pointer to Cart-ROM */

	uint32_t	vMode;					/* Color depth/mode */
	uint32_t	vPrevMode;				/* Previous depth */
	uint8_t	*pvScreenBase;
	uint8_t	*apvScreen[32];				/* Pointer to each of the 32 screen colums */
	int32_t	x[32];
	uint32_t	vScreenHeight;				/* 128, 160, 192 or 256 (could be overriden by cmode) */
	uint32_t	vScreenRealHeight;			/* 128, 160, 192 or 256 */
	uint32_t	vPrevScreenHeight;
	uint32_t	vScreenSize;

	uint8_t	*pvRamBank;				/* Pointer to current RAM-bank */
	uint8_t	*pvRomBank;				/* Pointer to current ROM-bank */
	uint8_t	*pvPrgBank;				/* Pointer to current program ROM-bank */

	uint8_t	*apvRamBank[FX_RAM_BANKS];		/* Ram bank table (max 256kb) */
	uint8_t	*apvRomBank[256];			/* Rom bank table */

	uint8_t	bCacheActive;
	uint8_t	*pvCache;				/* Pointer to the GSU cache */
	uint8_t	avCacheBackup[512];			/* Backup of ROM when the cache has replaced it */
	uint32_t	vCounter;
	uint32_t	vInstCount;
	uint32_t	vSCBRDirty;				/* If SCBR is written, our cached screen pointers need updating */
	
	uint8_t	*avRegAddr;				/* To reference avReg in snapshot.cpp */
};

extern struct FxRegs_s	GSU;

#ifdef FX_DO_ROMBUFFER
/* Read R14 */
#define READR14			GSU.vRomBuffer = ROM(R14)
/* Test and/or read R14 */
#define TESTR14			if (GSU.pvDreg == &R14) READR14
#else
/* Don't read R14 */
#define READR14
/* Don't test and/or read R14 */
#define TESTR14
#endif

/* Access to registers */
#define R0			GSU.avReg[0]
#define R1			GSU.avReg[1]
#define R2			GSU.avReg[2]
#define R3			GSU.avReg[3]
#define R4			GSU.avReg[4]
#define R5			GSU.avReg[5]
#define R6			GSU.avReg[6]
#define R7			GSU.avReg[7]
#define R8			GSU.avReg[8]
#define R9			GSU.avReg[9]
#define R10			GSU.avReg[10]
#define R11			GSU.avReg[11]
#define R12			GSU.avReg[12]
#define R13			GSU.avReg[13]
#define R14			GSU.avReg[14]
#define R15			GSU.avReg[15]
#define SFR			GSU.vStatusReg
#define PBR			GSU.vPrgBankReg
#define ROMBR			GSU.vRomBankReg
#define RAMBR			GSU.vRamBankReg
#define CBR			GSU.vCacheBaseReg
#define SCBR			USEX8(GSU.pvRegisters[GSU_SCBR])
#define SCMR			USEX8(GSU.pvRegisters[GSU_SCMR])
#define COLR			GSU.vColorReg
#define POR			GSU.vPlotOptionReg
#define BRAMR			USEX8(GSU.pvRegisters[GSU_BRAMR])
#define VCR			USEX8(GSU.pvRegisters[GSU_VCR])
#define CFGR			USEX8(GSU.pvRegisters[GSU_CFGR])
#define CLSR			USEX8(GSU.pvRegisters[GSU_CLSR])

#endif
