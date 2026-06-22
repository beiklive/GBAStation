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


#ifndef _CPUMACRO_H_
#define _CPUMACRO_H_

/* The OP-family templates instantiate one static function per opcode.
 * Every call below threads the access mode as a *token* suffix on the
 * addressing-function name (e.g. ADDR##_R, ADDR##_W, ADDR##_M) so that
 * the per-mode arms of the former `if (a & READ)` etc. tests inside
 * the addressing functions are stripped at preprocess time instead of
 * relying on the optimiser's constant propagation. See cpuaddr.h. */
#define rOP8(OP, ADDR, WRAP, FUNC) \
static void Op##OP (void) \
{ \
	uint8_t	val = OpenBus = S9xGetByte(ADDR##_R()); \
	FUNC##8(val); \
}

#define rOP16(OP, ADDR, WRAP, FUNC) \
static void Op##OP (void) \
{ \
	uint16_t	val = S9xGetWord(ADDR##_R(), WRAP); \
	OpenBus = (uint8_t) (val >> 8); \
	FUNC(val); \
}

#define rOPC(OP, COND, ADDR, WRAP, FUNC) \
static void Op##OP (void) \
{ \
	if (Check##COND()) \
	{ \
		uint8_t	val = OpenBus = S9xGetByte(ADDR##_R()); \
		FUNC##8(val); \
	} \
	else \
	{ \
		uint16_t	val = S9xGetWord(ADDR##_R(), WRAP); \
		OpenBus = (uint8_t) (val >> 8); \
		FUNC##16(val); \
	} \
}

#define rOPM(OP, ADDR, WRAP, FUNC) \
rOPC(OP, Memory, ADDR, WRAP, FUNC)

#define rOPX(OP, ADDR, WRAP, FUNC) \
rOPC(OP, Index, ADDR, WRAP, FUNC)

#define wOP8(OP, ADDR, WRAP, FUNC) \
static void Op##OP (void) \
{ \
	FUNC##8(ADDR##_W()); \
}

#define wOP16(OP, ADDR, WRAP, FUNC) \
static void Op##OP (void) \
{ \
	FUNC##16(ADDR##_W(), WRAP); \
}

#define wOPC(OP, COND, ADDR, WRAP, FUNC) \
static void Op##OP (void) \
{ \
	if (Check##COND()) \
		FUNC##8(ADDR##_W()); \
	else \
		FUNC##16(ADDR##_W(), WRAP); \
}

#define wOPM(OP, ADDR, WRAP, FUNC) \
wOPC(OP, Memory, ADDR, WRAP, FUNC)

#define wOPX(OP, ADDR, WRAP, FUNC) \
wOPC(OP, Index, ADDR, WRAP, FUNC)

#define mOP8(OP, ADDR, WRAP, FUNC) \
static void Op##OP (void) \
{ \
	FUNC##8(ADDR##_M()); \
}

#define mOP16(OP, ADDR, WRAP, FUNC) \
static void Op##OP (void) \
{ \
	FUNC##16(ADDR##_M(), WRAP); \
}

#define mOPC(OP, COND, ADDR, WRAP, FUNC) \
static void Op##OP (void) \
{ \
	if (Check##COND()) \
		FUNC##8(ADDR##_M()); \
	else \
		FUNC##16(ADDR##_M(), WRAP); \
}

#define mOPM(OP, ADDR, WRAP, FUNC) \
mOPC(OP, Memory, ADDR, WRAP, FUNC)

/* bOP's `E` argument is a literal 0 or 1 at the fast call sites and
 * only controls whether the page-cross extra cycle test fires. Token
 * paste to either emit the test or strip it entirely so that
 * `if (E && ...)` never survives as `if (0 && ...)` in the output.
 *
 * The Slow variants pass `CheckEmulation()` for E (a real runtime
 * check), so they get their own macro that keeps the test runtime. */
#define BRANCH_PCH_CHECK_0(newPC)
#define BRANCH_PCH_CHECK_1(newPC) \
	if (Registers.PCh != newPC.B.h) \
		AddCycles(ONE_CYCLE);
#define BRANCH_PCH_CHECK(E, newPC) BRANCH_PCH_CHECK_##E(newPC)

#define bOP(OP, REL, COND, CHK, E) \
static void Op##OP (void) \
{ \
	pair	newPC; \
	newPC.W = REL##_J(); \
	if (COND) \
	{ \
		AddCycles(ONE_CYCLE); \
		BRANCH_PCH_CHECK(E, newPC) \
		if ((Registers.PCw & ~MEMMAP_MASK) != (newPC.W & ~MEMMAP_MASK)) \
			S9xSetPCBase(ICPU.ShiftedPB + newPC.W); \
		else \
			Registers.PCw = newPC.W; \
	} \
}

#define bOPSlow(OP, REL, COND, CHK, E) \
static void Op##OP (void) \
{ \
	pair	newPC; \
	newPC.W = REL##_J(); \
	if (COND) \
	{ \
		AddCycles(ONE_CYCLE); \
		if ((E) && Registers.PCh != newPC.B.h) \
			AddCycles(ONE_CYCLE); \
		if ((Registers.PCw & ~MEMMAP_MASK) != (newPC.W & ~MEMMAP_MASK)) \
			S9xSetPCBase(ICPU.ShiftedPB + newPC.W); \
		else \
			Registers.PCw = newPC.W; \
	} \
}

/* JMP-family templates. The 16-bit form (jOP) keeps the existing PB
 * and OR's in a 16-bit destination. The long form (jOPL) takes a
 * 24-bit destination that already encodes the new bank. */
#define jOP(OP, ADDR) \
static void Op##OP (void) \
{ \
	S9xSetPCBase(ICPU.ShiftedPB + ((uint16_t) ADDR##_J())); \
}

#define jOPL(OP, ADDR) \
static void Op##OP (void) \
{ \
	S9xSetPCBase(ADDR##_J()); \
}


#define SetZN16(Work16) \
	ICPU._Zero = Work16 != 0; \
	ICPU._Negative = (uint8_t) (Work16 >> 8);

#define SetZN8(Work8) \
	ICPU._Zero = Work8; \
	ICPU._Negative = Work8;
	
static INLINE void ADC16(uint16_t Work16)
{
	if (CheckDecimal())
	{
		uint16_t Ans16;
		uint16_t	A1 = Registers.A.W & 0x000F;
		uint16_t	A2 = Registers.A.W & 0x00F0;
		uint16_t	A3 = Registers.A.W & 0x0F00;
		uint32_t	A4 = Registers.A.W & 0xF000;
		uint16_t	W1 = Work16 & 0x000F;
		uint16_t	W2 = Work16 & 0x00F0;
		uint16_t	W3 = Work16 & 0x0F00;
		uint16_t	W4 = Work16 & 0xF000;

		A1 += W1 + CheckCarry();
		if (A1 > 0x0009)
		{
			A1 -= 0x000A;
			A1 &= 0x000F;
			A2 += 0x0010;
		}

		A2 += W2;
		if (A2 > 0x0090)
		{
			A2 -= 0x00A0;
			A2 &= 0x00F0;
			A3 += 0x0100;
		}

		A3 += W3;
		if (A3 > 0x0900)
		{
			A3 -= 0x0A00;
			A3 &= 0x0F00;
			A4 += 0x1000;
		}

		A4 += W4;
		if (A4 > 0x9000)
		{
			A4 -= 0xA000;
			A4 &= 0xF000;
			SetCarry();
		}
		else
			ClearCarry();

		Ans16 = A4 | A3 | A2 | A1;

		if (~(Registers.A.W ^ Work16) & (Work16 ^ Ans16) & 0x8000)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.W = Ans16;
	}
	else
	{
		uint32_t	Ans32 = Registers.A.W + Work16 + CheckCarry();

		ICPU._Carry = Ans32 >= 0x10000;

		if (~(Registers.A.W ^ Work16) & (Work16 ^ (uint16_t) Ans32) & 0x8000)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.W = (uint16_t) Ans32;
	}
	SetZN16(Registers.A.W);
}

static INLINE void ADC8(uint8_t Work8)
{
	if (CheckDecimal())
	{
		uint8_t Ans8;
		uint8_t  A1 = Registers.A.W & 0x0F;
		uint16_t A2 = Registers.A.W & 0xF0;
		uint8_t  W1 = Work8 & 0x0F;
		uint8_t  W2 = Work8 & 0xF0;

		A1 += W1 + CheckCarry();
		if (A1 > 0x09)
		{
			A1 -= 0x0A;
			A1 &= 0x0F;
			A2 += 0x10;
		}

		A2 += W2;
		if (A2 > 0x90)
		{
			A2 -= 0xA0;
			A2 &= 0xF0;
			SetCarry();
		}
		else
			ClearCarry();

		Ans8 = A2 | A1;

		if (~(Registers.AL ^ Work8) & (Work8 ^ Ans8) & 0x80)
			SetOverflow();
		else
			ClearOverflow();

		Registers.AL = Ans8;
	}
	else
	{
		uint16_t	Ans16 = Registers.AL + Work8 + CheckCarry();

		ICPU._Carry = Ans16 >= 0x100;

		if (~(Registers.AL ^ Work8) & (Work8 ^ (uint8_t) Ans16) & 0x80)
			SetOverflow();
		else
			ClearOverflow();

		Registers.AL = (uint8_t) Ans16;
	}
	SetZN8(Registers.AL);
}

static INLINE void AND16(uint16_t Work16)
{
	Registers.A.W &= Work16;
	SetZN16(Registers.A.W);
}

static INLINE void AND8(uint8_t Work8)
{
	Registers.AL &= Work8;
	SetZN8(Registers.AL);
}

static INLINE void ASL16 (uint32_t OpAddress, uint32_t w)
{
	uint16_t	Work16 = S9xGetWord(OpAddress, w);
	ICPU._Carry = (Work16 & 0x8000) != 0;
	Work16 <<= 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord_Write1(Work16, OpAddress, w);
	OpenBus = Work16 & 0xff;
	SetZN16(Work16);
}

static INLINE void ASL8 (uint32_t OpAddress)
{
	uint8_t	Work8 = S9xGetByte(OpAddress);
	ICPU._Carry = (Work8 & 0x80) != 0;
	Work8 <<= 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN8(Work8);
}

static INLINE void BIT16(uint16_t Work16)
{
	ICPU._Overflow = (Work16 & 0x4000) != 0;
	ICPU._Negative = (uint8_t) (Work16 >> 8);
	ICPU._Zero = (Work16 & Registers.A.W) != 0;
}

static INLINE void BIT8(uint8_t Work8)
{
	ICPU._Overflow = (Work8 & 0x40) != 0;
	ICPU._Negative = Work8;
	ICPU._Zero = Work8 & Registers.AL;
}

static INLINE void CMP16(uint16_t val)
{
	int16_t Int16;
	int32_t Int32 = (int32_t) Registers.A.W - (int32_t) val;
	ICPU._Carry = Int32 >= 0;
	Int16 = Int32;
	SetZN16(Int16);
}

static INLINE void CMP8(uint8_t val)
{
	uint8_t Int8;
	int16_t Int16 = (int16_t) Registers.AL - (int16_t) val;
	ICPU._Carry = Int16 >= 0;
	Int8 = Int16;
	SetZN8(Int8);
}

static INLINE void CPX16(uint16_t val)
{
	uint16_t Int16;
	int32_t Int32 = (int32_t) Registers.X.W - (int32_t) val;
	ICPU._Carry = Int32 >= 0;
	Int16 = Int32;
	SetZN16(Int16);
}

static INLINE void CPX8(uint8_t val)
{
	uint8_t Int8;
	int16_t Int16 = (int16_t) Registers.XL - (int16_t) val;
	ICPU._Carry = Int16 >= 0;
	Int8 = Int16;
	SetZN8(Int8);
}

static INLINE void CPY16(uint16_t val)
{
	uint16_t Int16;
	int32_t Int32 = (int32_t) Registers.Y.W - (int32_t) val;
	ICPU._Carry = Int32 >= 0;
	Int16 = Int32;
	SetZN16(Int16);
}

static INLINE void CPY8(uint8_t val)
{
	uint8_t Int8;
	int16_t Int16 = (int16_t) Registers.YL - (int16_t) val;
	ICPU._Carry = Int16 >= 0;
	Int8 = Int16;
	SetZN8(Int8);
}

static INLINE void DEC16 (uint32_t OpAddress, uint32_t w)
{
	uint16_t Work16 = S9xGetWord(OpAddress, w) - 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord_Write1(Work16, OpAddress, w);
	OpenBus = Work16 & 0xff;
	SetZN16(Work16);
}

static INLINE void DEC8 (uint32_t OpAddress)
{
	uint8_t	Work8 = S9xGetByte(OpAddress) - 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN8(Work8);
}

static INLINE void EOR16(uint16_t val)
{
	Registers.A.W ^= val;
	SetZN16(Registers.A.W);
}

static INLINE void EOR8(uint8_t val)
{
	Registers.AL ^= val;
	SetZN8(Registers.AL);
}

static INLINE void INC16 (uint32_t OpAddress, uint32_t w)
{
	uint16_t	Work16 = S9xGetWord(OpAddress, w) + 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord_Write1(Work16, OpAddress, w);
	OpenBus = Work16 & 0xff;
	SetZN16(Work16);
}

static INLINE void INC8 (uint32_t OpAddress)
{
	uint8_t	Work8 = S9xGetByte(OpAddress) + 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN8(Work8);
}

static INLINE void LDA16(uint16_t val)
{
	Registers.A.W = val;
	SetZN16(Registers.A.W);
}

static INLINE void LDA8(uint8_t val)
{
	Registers.AL = val;
	SetZN8(Registers.AL);
}

static INLINE void LDX16(uint16_t val)
{
	Registers.X.W = val;
	SetZN16(Registers.X.W);
}

static INLINE void LDX8(uint8_t val)
{
	Registers.XL = val;
	SetZN8(Registers.XL);
}

static INLINE void LDY16(uint16_t val)
{
	Registers.Y.W = val;
	SetZN16(Registers.Y.W);
}

static INLINE void LDY8(uint8_t val)
{
	Registers.YL = val;
	SetZN8(Registers.YL);
}

static INLINE void LSR16 (uint32_t OpAddress, uint32_t w)
{
	uint16_t	Work16 = S9xGetWord(OpAddress, w);
	ICPU._Carry = Work16 & 1;
	Work16 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord_Write1(Work16, OpAddress, w);
	OpenBus = Work16 & 0xff;
	SetZN16(Work16);
}

static INLINE void LSR8 (uint32_t OpAddress)
{
	uint8_t	Work8 = S9xGetByte(OpAddress);
	ICPU._Carry = Work8 & 1;
	Work8 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN8(Work8);
}

static INLINE void ORA16(uint16_t val)
{
	Registers.A.W |= val;
	SetZN16(Registers.A.W);
}

static INLINE void ORA8(uint8_t val)
{
	Registers.AL |= val;
	SetZN8(Registers.AL);
}

static INLINE void ROL16 (uint32_t OpAddress, uint32_t w)
{
	uint32_t	Work32 = (((uint32_t) S9xGetWord(OpAddress, w)) << 1) | CheckCarry();
	ICPU._Carry = Work32 >= 0x10000;
	AddCycles(ONE_CYCLE);
	S9xSetWord_Write1((uint16_t) Work32, OpAddress, w);
	OpenBus = Work32 & 0xff;
	SetZN16((uint16_t) Work32);
}

static INLINE void ROL8 (uint32_t OpAddress)
{
	uint16_t	Work16 = (((uint16_t) S9xGetByte(OpAddress)) << 1) | CheckCarry();
	ICPU._Carry = Work16 >= 0x100;
	AddCycles(ONE_CYCLE);
	S9xSetByte((uint8_t) Work16, OpAddress);
	OpenBus = Work16 & 0xff;
	SetZN8((uint8_t) Work16);
}

static INLINE void ROR16 (uint32_t OpAddress, uint32_t w)
{
	uint32_t	Work32 = ((uint32_t) S9xGetWord(OpAddress, w)) | (((uint32_t) CheckCarry()) << 16);
	ICPU._Carry = Work32 & 1;
	Work32 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord_Write1((uint16_t) Work32, OpAddress, w);
	OpenBus = Work32 & 0xff;
	SetZN16((uint16_t) Work32);
}

static INLINE void ROR8 (uint32_t OpAddress)
{
	uint16_t	Work16 = ((uint16_t) S9xGetByte(OpAddress)) | (((uint16_t) CheckCarry()) << 8);
	ICPU._Carry = Work16 & 1;
	Work16 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte((uint8_t) Work16, OpAddress);
	OpenBus = Work16 & 0xff;
	SetZN8((uint8_t) Work16);
}

static INLINE void SBC16(uint16_t Work16)
{
	if (CheckDecimal())
	{
		uint16_t Ans16;
		uint16_t	A1 = Registers.A.W & 0x000F;
		uint16_t	A2 = Registers.A.W & 0x00F0;
		uint16_t	A3 = Registers.A.W & 0x0F00;
		uint32_t	A4 = Registers.A.W & 0xF000;
		uint16_t	W1 = Work16 & 0x000F;
		uint16_t	W2 = Work16 & 0x00F0;
		uint16_t	W3 = Work16 & 0x0F00;
		uint16_t	W4 = Work16 & 0xF000;

		A1 -= W1 + !CheckCarry();
		A2 -= W2;
		A3 -= W3;
		A4 -= W4;

		if (A1 > 0x000F)
		{
			A1 += 0x000A;
			A1 &= 0x000F;
			A2 -= 0x0010;
		}

		if (A2 > 0x00F0)
		{
			A2 += 0x00A0;
			A2 &= 0x00F0;
			A3 -= 0x0100;
		}

		if (A3 > 0x0F00)
		{
			A3 += 0x0A00;
			A3 &= 0x0F00;
			A4 -= 0x1000;
		}

		if (A4 > 0xF000)
		{
			A4 += 0xA000;
			A4 &= 0xF000;
			ClearCarry();
		}
		else
			SetCarry();

		Ans16 = A4 | A3 | A2 | A1;

		if ((Registers.A.W ^ Work16) & (Registers.A.W ^ Ans16) & 0x8000)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.W = Ans16;
	}
	else
	{
		int32_t	Int32 = (int32_t) Registers.A.W - (int32_t) Work16 + (int32_t) CheckCarry() - 1;

		ICPU._Carry = Int32 >= 0;

		if ((Registers.A.W ^ Work16) & (Registers.A.W ^ (uint16_t) Int32) & 0x8000)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.W = (uint16_t) Int32;
	}
	SetZN16(Registers.A.W);
}

static INLINE void SBC8(uint8_t Work8)
{
	if (CheckDecimal())
	{
		uint8_t Ans8;
		uint8_t  A1 = Registers.A.W & 0x0F;
		uint16_t A2 = Registers.A.W & 0xF0;
		uint8_t  W1 = Work8 & 0x0F;
		uint8_t  W2 = Work8 & 0xF0;

		A1 -= W1 + !CheckCarry();
		A2 -= W2;

		if (A1 > 0x0F)
		{
			A1 += 0x0A;
			A1 &= 0x0F;
			A2 -= 0x10;
		}

		if (A2 > 0xF0)
		{
			A2 += 0xA0;
			A2 &= 0xF0;
			ClearCarry();
		}
		else
			SetCarry();

		Ans8 = A2 | A1;

		if ((Registers.AL ^ Work8) & (Registers.AL ^ Ans8) & 0x80)
			SetOverflow();
		else
			ClearOverflow();

		Registers.AL = Ans8;
	}
	else
	{
		int16_t	Int16 = (int16_t) Registers.AL - (int16_t) Work8 + (int16_t) CheckCarry() - 1;

		ICPU._Carry = Int16 >= 0;

		if ((Registers.AL ^ Work8) & (Registers.AL ^ (uint8_t) Int16) & 0x80)
			SetOverflow();
		else
			ClearOverflow();

		Registers.AL = (uint8_t) Int16;
	}
	SetZN8(Registers.AL);
}

static INLINE void STA16 (uint32_t OpAddress, uint32_t w)
{
	S9xSetWord_Write0(Registers.A.W, OpAddress, w);
	OpenBus = Registers.AH;
}

static INLINE void STA8 (uint32_t OpAddress)
{
	S9xSetByte(Registers.AL, OpAddress);
	OpenBus = Registers.AL;
}

static INLINE void STX16 (uint32_t OpAddress, uint32_t w)
{
	S9xSetWord_Write0(Registers.X.W, OpAddress, w);
	OpenBus = Registers.XH;
}

static INLINE void STX8 (uint32_t OpAddress)
{
	S9xSetByte(Registers.XL, OpAddress);
	OpenBus = Registers.XL;
}

static INLINE void STY16 (uint32_t OpAddress, uint32_t w)
{
	S9xSetWord_Write0(Registers.Y.W, OpAddress, w);
	OpenBus = Registers.YH;
}

static INLINE void STY8 (uint32_t OpAddress)
{
	S9xSetByte(Registers.YL, OpAddress);
	OpenBus = Registers.YL;
}

static INLINE void STZ16 (uint32_t OpAddress, uint32_t w)
{
	S9xSetWord_Write0(0, OpAddress, w);
	OpenBus = 0;
}

static INLINE void STZ8 (uint32_t OpAddress)
{
	S9xSetByte(0, OpAddress);
	OpenBus = 0;
}

static INLINE void TSB16 (uint32_t OpAddress, uint32_t w)
{
	uint16_t Work16 = S9xGetWord(OpAddress, w);
	ICPU._Zero = (Work16 & Registers.A.W) != 0;
	Work16 |= Registers.A.W;
	AddCycles(ONE_CYCLE);
	S9xSetWord_Write1(Work16, OpAddress, w);
	OpenBus = Work16 & 0xff;
}

static INLINE void TSB8 (uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(OpAddress);
	ICPU._Zero = Work8 & Registers.AL;
	Work8 |= Registers.AL;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
}

static INLINE void TRB16 (uint32_t OpAddress, uint32_t w)
{
	uint16_t Work16 = S9xGetWord(OpAddress, w);
	ICPU._Zero = (Work16 & Registers.A.W) != 0;
	Work16 &= ~Registers.A.W;
	AddCycles(ONE_CYCLE);
	S9xSetWord_Write1(Work16, OpAddress, w);
	OpenBus = Work16 & 0xff;
}

static INLINE void TRB8 (uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(OpAddress);
	ICPU._Zero = Work8 & Registers.AL;
	Work8 &= ~Registers.AL;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
}

#endif
