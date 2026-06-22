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


#ifndef _CPUADDR_H_
#define _CPUADDR_H_

/* Each addressing function used to take a runtime parameter `a` whose
 * value was one of the constants below; the body branched on bits of
 * `a` to decide whether to update OpenBus, eat an extra cycle, etc.
 * `a` was always a compile-time literal at every call site, so the
 * branches folded under -O2 + LTO, but they appeared as `if (1) / if
 * (0)` in the preprocessed translation unit and stuck around in
 * cold paths where the inliner gave up.
 *
 * Instead we now token-paste the access mode onto the function name
 * and generate one specialised variant per mode. The
 * IF_R / IF_W / IF_W_ELSE / IF_JSR helpers below decide at macro
 * expansion time which arm of each former runtime branch to emit:
 *
 *   IF_R   (mode, x)      emit x iff (mode & READ) would be true
 *   IF_W   (mode, x)      emit x iff (mode & WRITE) would be true
 *   IF_W_ELSE(mode, w, n) emit w if write/modify, else n
 *   IF_JSR (mode, x)      emit x iff mode == JSR
 *
 * Supported mode tokens (with the matching bits of the old runtime
 * constant in brackets): N [0b000 = NONE], R [0b001 = READ],
 * W [0b010 = WRITE], M [0b011 = MODIFY], J [0b101 = JUMP],
 * JSR [0b1000]. */

#define IF_R_N(x)
#define IF_R_R(x)		x
#define IF_R_W(x)
#define IF_R_M(x)		x
#define IF_R_J(x)		x
#define IF_R_JSR(x)
#define IF_R(m, x)		IF_R_##m(x)

#define IF_W_N(x)
#define IF_W_R(x)
#define IF_W_W(x)		x
#define IF_W_M(x)		x
#define IF_W_J(x)
#define IF_W_JSR(x)
#define IF_W(m, x)		IF_W_##m(x)

#define IF_W_ELSE_N(w, n)	n
#define IF_W_ELSE_R(w, n)	n
#define IF_W_ELSE_W(w, n)	w
#define IF_W_ELSE_M(w, n)	w
#define IF_W_ELSE_J(w, n)	n
#define IF_W_ELSE_JSR(w, n)	n
#define IF_W_ELSE(m, w, n)	IF_W_ELSE_##m(w, n)

#define IF_JSR_N(x)
#define IF_JSR_R(x)
#define IF_JSR_W(x)
#define IF_JSR_M(x)
#define IF_JSR_J(x)
#define IF_JSR_JSR(x)		x
#define IF_JSR(m, x)		IF_JSR_##m(x)


#define DEFINE_Immediate8Slow(m) \
static INLINE uint8_t Immediate8Slow_##m (void) \
{ \
	uint8_t	val = S9xGetByte(Registers.PBPC); \
	IF_R(m, OpenBus = val;) \
	Registers.PCw++; \
	return (val); \
}
DEFINE_Immediate8Slow(N)
DEFINE_Immediate8Slow(R)
DEFINE_Immediate8Slow(W)
DEFINE_Immediate8Slow(M)
DEFINE_Immediate8Slow(J)
DEFINE_Immediate8Slow(JSR)
#undef DEFINE_Immediate8Slow

#define DEFINE_Immediate8(m) \
static INLINE uint8_t Immediate8_##m (void) \
{ \
	uint8_t	val = CPU.PCBase[Registers.PCw]; \
	IF_R(m, OpenBus = val;) \
	AddCycles(CPU.MemSpeed); \
	Registers.PCw++; \
	return (val); \
}
DEFINE_Immediate8(N)
DEFINE_Immediate8(R)
DEFINE_Immediate8(W)
DEFINE_Immediate8(M)
DEFINE_Immediate8(J)
DEFINE_Immediate8(JSR)
#undef DEFINE_Immediate8

#define DEFINE_Immediate16Slow(m) \
static INLINE uint16_t Immediate16Slow_##m (void) \
{ \
	uint16_t	val = S9xGetWord(Registers.PBPC, WRAP_BANK); \
	IF_R(m, OpenBus = (uint8_t) (val >> 8);) \
	Registers.PCw += 2; \
	return (val); \
}
DEFINE_Immediate16Slow(N)
DEFINE_Immediate16Slow(R)
DEFINE_Immediate16Slow(W)
DEFINE_Immediate16Slow(M)
DEFINE_Immediate16Slow(J)
DEFINE_Immediate16Slow(JSR)
#undef DEFINE_Immediate16Slow

#define DEFINE_Immediate16(m) \
static INLINE uint16_t Immediate16_##m (void) \
{ \
	uint16_t	val = READ_WORD(CPU.PCBase + Registers.PCw); \
	IF_R(m, OpenBus = (uint8_t) (val >> 8);) \
	AddCycles(CPU.MemSpeedx2); \
	Registers.PCw += 2; \
	return (val); \
}
DEFINE_Immediate16(N)
DEFINE_Immediate16(R)
DEFINE_Immediate16(W)
DEFINE_Immediate16(M)
DEFINE_Immediate16(J)
DEFINE_Immediate16(JSR)
#undef DEFINE_Immediate16

/* Relative / RelativeLong are called with JUMP from branch ops and
 * (for the long form) with NONE from PER, which pushes the computed
 * address rather than dereferencing it. */
static INLINE uint32_t RelativeSlow_J (void)		/* branch $xx */
{
	int8_t	offset = Immediate8Slow_J();
	return ((int16_t) Registers.PCw + offset) & 0xffff;
}

static INLINE uint32_t Relative_J (void)		/* branch $xx */
{
	int8_t	offset = Immediate8_J();
	return ((int16_t) Registers.PCw + offset) & 0xffff;
}

static INLINE uint32_t RelativeLongSlow_J (void)	/* BRL $xxxx */
{
	int16_t	offset = Immediate16Slow_J();
	return ((int32_t) Registers.PCw + offset) & 0xffff;
}

static INLINE uint32_t RelativeLong_J (void)		/* BRL $xxxx */
{
	int16_t	offset = Immediate16_J();
	return ((int32_t) Registers.PCw + offset) & 0xffff;
}

static INLINE uint32_t RelativeLongSlow_N (void)	/* PER */
{
	int16_t	offset = Immediate16Slow_N();
	return ((int32_t) Registers.PCw + offset) & 0xffff;
}

static INLINE uint32_t RelativeLong_N (void)		/* PER */
{
	int16_t	offset = Immediate16_N();
	return ((int32_t) Registers.PCw + offset) & 0xffff;
}

/* AbsoluteIndexedIndirect (a,X) is only ever called with JUMP / JSR.
 * The JSR variant pushes the old PC in the middle of the load. */
#define DEFINE_AbsoluteIndexedIndirectSlow(m) \
static INLINE uint32_t AbsoluteIndexedIndirectSlow_##m (void) \
{ \
	uint16_t addr, addr2; \
	addr = Immediate16Slow_R(); \
	IF_JSR(m, \
		addr = Immediate8Slow_R(); \
		OpenBus = Registers.PCl; \
		addr |= Immediate8Slow_R() << 8;) \
	AddCycles(ONE_CYCLE); \
	addr += Registers.X.W; \
	/* Address load wraps within the bank */ \
	addr2 = S9xGetWord(ICPU.ShiftedPB | addr, WRAP_BANK); \
	OpenBus = addr2 >> 8; \
	return (addr2); \
}
DEFINE_AbsoluteIndexedIndirectSlow(J)
DEFINE_AbsoluteIndexedIndirectSlow(JSR)
#undef DEFINE_AbsoluteIndexedIndirectSlow

static INLINE uint32_t AbsoluteIndexedIndirect_J (void)		/* (a,X) */
{
	uint16_t	addr, addr2;
	addr = Immediate16Slow_R();
	addr += Registers.X.W;
	/* Address load wraps within the bank */
	addr2 = S9xGetWord(ICPU.ShiftedPB | addr, WRAP_BANK);
	OpenBus = addr2 >> 8;
	return (addr2);
}

#define AbsoluteIndexedIndirect_JSR AbsoluteIndexedIndirect_J

static INLINE uint32_t AbsoluteIndirectLongSlow_J (void)	/* [a] */
{
	uint16_t	addr  = Immediate16Slow_R();
	/* No info on wrapping, but it doesn't matter anyway due to mirroring */
	uint32_t	addr2 = S9xGetWord(addr, WRAP_NONE);
	OpenBus  = addr2 >> 8;
	addr2   |= (OpenBus = S9xGetByte(addr + 2)) << 16;
	return (addr2);
}

static INLINE uint32_t AbsoluteIndirectLong_J (void)		/* [a] */
{
	uint16_t	addr  = Immediate16_R();
	uint32_t	addr2 = S9xGetWord(addr, WRAP_NONE);
	OpenBus  = addr2 >> 8;
	addr2   |= (OpenBus = S9xGetByte(addr + 2)) << 16;
	return (addr2);
}

static INLINE uint32_t AbsoluteIndirectSlow_J (void)		/* (a) */
{
	uint16_t	addr2 = S9xGetWord(Immediate16Slow_R(), WRAP_NONE);
	OpenBus = addr2 >> 8;
	return (addr2);
}

static INLINE uint32_t AbsoluteIndirect_J (void)		/* (a) */
{
	uint16_t	addr2 = S9xGetWord(Immediate16_R(), WRAP_NONE);
	OpenBus = addr2 >> 8;
	return (addr2);
}

#define DEFINE_AbsoluteSlow(m) \
static INLINE uint32_t AbsoluteSlow_##m (void) \
{ \
	return (ICPU.ShiftedDB | Immediate16Slow_##m()); \
}
DEFINE_AbsoluteSlow(N)
DEFINE_AbsoluteSlow(R)
DEFINE_AbsoluteSlow(W)
DEFINE_AbsoluteSlow(M)
DEFINE_AbsoluteSlow(J)
#undef DEFINE_AbsoluteSlow

#define DEFINE_Absolute(m) \
static INLINE uint32_t Absolute_##m (void) \
{ \
	return (ICPU.ShiftedDB | Immediate16_##m()); \
}
DEFINE_Absolute(N)
DEFINE_Absolute(R)
DEFINE_Absolute(W)
DEFINE_Absolute(M)
DEFINE_Absolute(J)
#undef DEFINE_Absolute

#define ABSOLUTESLOW_MACRO(m) (ICPU.ShiftedDB | Immediate16Slow_##m())
#define ABSOLUTE_MACRO(m)     (ICPU.ShiftedDB | Immediate16_##m())

/* AbsoluteLong: only the JSR variant needs to capture the old PB
 * because JSR l pushes it in the middle of loading the new bank. */
#define DEFINE_AbsoluteLongSlow(m) \
static INLINE uint32_t AbsoluteLongSlow_##m (void) \
{ \
	uint32_t	addr = Immediate16Slow_R(); \
	IF_JSR(m, OpenBus = Registers.PB;) \
	addr |= Immediate8Slow_##m() << 16; \
	return (addr); \
}
DEFINE_AbsoluteLongSlow(R)
DEFINE_AbsoluteLongSlow(W)
DEFINE_AbsoluteLongSlow(M)
DEFINE_AbsoluteLongSlow(J)
DEFINE_AbsoluteLongSlow(JSR)
#undef DEFINE_AbsoluteLongSlow

#define DEFINE_AbsoluteLong(m) \
static INLINE uint32_t AbsoluteLong_##m (void) \
{ \
	uint32_t	addr = READ_3WORD(CPU.PCBase + Registers.PCw); \
	AddCycles(CPU.MemSpeedx2 + CPU.MemSpeed); \
	IF_R(m, OpenBus = addr >> 16;) \
	Registers.PCw += 3; \
	return (addr); \
}
DEFINE_AbsoluteLong(R)
DEFINE_AbsoluteLong(W)
DEFINE_AbsoluteLong(M)
DEFINE_AbsoluteLong(J)
DEFINE_AbsoluteLong(JSR)
#undef DEFINE_AbsoluteLong

#define DEFINE_DirectSlow(m) \
static INLINE uint32_t DirectSlow_##m (void) \
{ \
	uint16_t	addr = Immediate8Slow_##m() + Registers.D.W; \
	if (Registers.DL != 0) \
		AddCycles(ONE_CYCLE); \
	return (addr); \
}
DEFINE_DirectSlow(N)
DEFINE_DirectSlow(R)
DEFINE_DirectSlow(W)
DEFINE_DirectSlow(M)
#undef DEFINE_DirectSlow

#define DEFINE_Direct(m) \
static INLINE uint32_t Direct_##m (void) \
{ \
	uint16_t	addr = Immediate8_##m() + Registers.D.W; \
	if (Registers.DL != 0) \
		AddCycles(ONE_CYCLE); \
	return (addr); \
}
DEFINE_Direct(N)
DEFINE_Direct(R)
DEFINE_Direct(W)
DEFINE_Direct(M)
#undef DEFINE_Direct

#define DEFINE_DirectIndirectSlow(m) \
static INLINE uint32_t DirectIndirectSlow_##m (void) \
{ \
	uint32_t	addr = S9xGetWord(DirectSlow_R(), (!CheckEmulation() || Registers.DL) ? WRAP_BANK : WRAP_PAGE); \
	IF_R(m, OpenBus = (uint8_t) (addr >> 8);) \
	addr |= ICPU.ShiftedDB; \
	return (addr); \
}
DEFINE_DirectIndirectSlow(N)
DEFINE_DirectIndirectSlow(R)
DEFINE_DirectIndirectSlow(W)
DEFINE_DirectIndirectSlow(M)
#undef DEFINE_DirectIndirectSlow

#define DEFINE_DirectIndirectE0(m) \
static INLINE uint32_t DirectIndirectE0_##m (void) \
{ \
	uint32_t	addr = S9xGetWord(Direct_R(), WRAP_NONE); \
	IF_R(m, OpenBus = (uint8_t) (addr >> 8);) \
	addr |= ICPU.ShiftedDB; \
	return (addr); \
}
DEFINE_DirectIndirectE0(N)
DEFINE_DirectIndirectE0(R)
DEFINE_DirectIndirectE0(W)
DEFINE_DirectIndirectE0(M)
#undef DEFINE_DirectIndirectE0

#define DEFINE_DirectIndirectE1(m) \
static INLINE uint32_t DirectIndirectE1_##m (void) \
{ \
	uint32_t	addr = S9xGetWord(DirectSlow_R(), Registers.DL ? WRAP_BANK : WRAP_PAGE); \
	IF_R(m, OpenBus = (uint8_t) (addr >> 8);) \
	addr |= ICPU.ShiftedDB; \
	return (addr); \
}
DEFINE_DirectIndirectE1(N)
DEFINE_DirectIndirectE1(R)
DEFINE_DirectIndirectE1(W)
DEFINE_DirectIndirectE1(M)
#undef DEFINE_DirectIndirectE1

/* (d),Y: a write or a page-crossing read costs an extra cycle. In
 * write/modify modes the W check short-circuits the runtime probe. */
#define DEFINE_DirectIndirectIndexedSlow(m) \
static INLINE uint32_t DirectIndirectIndexedSlow_##m (void) \
{ \
	uint32_t	addr = DirectIndirectSlow_##m(); \
	IF_W_ELSE(m, \
		AddCycles(ONE_CYCLE);, \
		if (!CheckIndex() || (addr & 0xff) + Registers.YL >= 0x100) \
			AddCycles(ONE_CYCLE);) \
	return (addr + Registers.Y.W); \
}
DEFINE_DirectIndirectIndexedSlow(R)
DEFINE_DirectIndirectIndexedSlow(W)
DEFINE_DirectIndirectIndexedSlow(M)
#undef DEFINE_DirectIndirectIndexedSlow

#define DEFINE_DirectIndirectIndexedE0X0(m) \
static INLINE uint32_t DirectIndirectIndexedE0X0_##m (void) \
{ \
	uint32_t	addr = DirectIndirectE0_##m(); \
	AddCycles(ONE_CYCLE); \
	return (addr + Registers.Y.W); \
}
DEFINE_DirectIndirectIndexedE0X0(R)
DEFINE_DirectIndirectIndexedE0X0(W)
DEFINE_DirectIndirectIndexedE0X0(M)
#undef DEFINE_DirectIndirectIndexedE0X0

#define DEFINE_DirectIndirectIndexedE0X1(m) \
static INLINE uint32_t DirectIndirectIndexedE0X1_##m (void) \
{ \
	uint32_t	addr = DirectIndirectE0_##m(); \
	IF_W_ELSE(m, \
		AddCycles(ONE_CYCLE);, \
		if ((addr & 0xff) + Registers.YL >= 0x100) \
			AddCycles(ONE_CYCLE);) \
	return (addr + Registers.Y.W); \
}
DEFINE_DirectIndirectIndexedE0X1(R)
DEFINE_DirectIndirectIndexedE0X1(W)
DEFINE_DirectIndirectIndexedE0X1(M)
#undef DEFINE_DirectIndirectIndexedE0X1

#define DEFINE_DirectIndirectIndexedE1(m) \
static INLINE uint32_t DirectIndirectIndexedE1_##m (void) \
{ \
	uint32_t	addr = DirectIndirectE1_##m(); \
	IF_W_ELSE(m, \
		AddCycles(ONE_CYCLE);, \
		if ((addr & 0xff) + Registers.YL >= 0x100) \
			AddCycles(ONE_CYCLE);) \
	return (addr + Registers.Y.W); \
}
DEFINE_DirectIndirectIndexedE1(R)
DEFINE_DirectIndirectIndexedE1(W)
DEFINE_DirectIndirectIndexedE1(M)
#undef DEFINE_DirectIndirectIndexedE1

/* [d] / [d],Y: read-only OpenBus update happens inside the inner load. */
static INLINE uint32_t DirectIndirectLongSlow_common (void)
{
	uint16_t	addr  = DirectSlow_R();
	uint32_t	addr2 = S9xGetWord(addr, WRAP_NONE);
	OpenBus  = addr2 >> 8;
	addr2   |= (OpenBus = S9xGetByte(addr + 2)) << 16;
	return (addr2);
}

static INLINE uint32_t DirectIndirectLong_common (void)
{
	uint16_t	addr  = Direct_R();
	uint32_t	addr2 = S9xGetWord(addr, WRAP_NONE);
	OpenBus  = addr2 >> 8;
	addr2   |= (OpenBus = S9xGetByte(addr + 2)) << 16;
	return (addr2);
}

#define DirectIndirectLongSlow_R		DirectIndirectLongSlow_common
#define DirectIndirectLongSlow_W		DirectIndirectLongSlow_common
#define DirectIndirectLongSlow_M		DirectIndirectLongSlow_common
#define DirectIndirectLong_R			DirectIndirectLong_common
#define DirectIndirectLong_W			DirectIndirectLong_common
#define DirectIndirectLong_M			DirectIndirectLong_common

#define DirectIndirectIndexedLongSlow_R()	(DirectIndirectLongSlow_common() + Registers.Y.W)
#define DirectIndirectIndexedLongSlow_W()	(DirectIndirectLongSlow_common() + Registers.Y.W)
#define DirectIndirectIndexedLongSlow_M()	(DirectIndirectLongSlow_common() + Registers.Y.W)
#define DirectIndirectIndexedLong_R()		(DirectIndirectLong_common() + Registers.Y.W)
#define DirectIndirectIndexedLong_W()		(DirectIndirectLong_common() + Registers.Y.W)
#define DirectIndirectIndexedLong_M()		(DirectIndirectLong_common() + Registers.Y.W)

#define DEFINE_DirectIndexedXSlow(m) \
static INLINE uint32_t DirectIndexedXSlow_##m (void) \
{ \
	pair	addr; \
	addr.W = DirectSlow_##m(); \
	if (!CheckEmulation() || Registers.DL) \
		addr.W += Registers.X.W; \
	else \
		addr.B.l += Registers.XL; \
	AddCycles(ONE_CYCLE); \
	return (addr.W); \
}
DEFINE_DirectIndexedXSlow(R)
DEFINE_DirectIndexedXSlow(W)
DEFINE_DirectIndexedXSlow(M)
#undef DEFINE_DirectIndexedXSlow

#define DEFINE_DirectIndexedXE0(m) \
static INLINE uint32_t DirectIndexedXE0_##m (void) \
{ \
	uint16_t	addr = Direct_##m() + Registers.X.W; \
	AddCycles(ONE_CYCLE); \
	return (addr); \
}
DEFINE_DirectIndexedXE0(R)
DEFINE_DirectIndexedXE0(W)
DEFINE_DirectIndexedXE0(M)
#undef DEFINE_DirectIndexedXE0

#define DEFINE_DirectIndexedXE1(m) \
static INLINE uint32_t DirectIndexedXE1_##m (void) \
{ \
	pair	addr; \
	if (Registers.DL) \
		return (DirectIndexedXE0_##m()); \
	addr.W = Direct_##m(); \
	addr.B.l += Registers.XL; \
	AddCycles(ONE_CYCLE); \
	return (addr.W); \
}
DEFINE_DirectIndexedXE1(R)
DEFINE_DirectIndexedXE1(W)
DEFINE_DirectIndexedXE1(M)
#undef DEFINE_DirectIndexedXE1

#define DEFINE_DirectIndexedYSlow(m) \
static INLINE uint32_t DirectIndexedYSlow_##m (void) \
{ \
	pair	addr; \
	addr.W = DirectSlow_##m(); \
	if (!CheckEmulation() || Registers.DL) \
		addr.W += Registers.Y.W; \
	else \
		addr.B.l += Registers.YL; \
	AddCycles(ONE_CYCLE); \
	return (addr.W); \
}
DEFINE_DirectIndexedYSlow(R)
DEFINE_DirectIndexedYSlow(W)
DEFINE_DirectIndexedYSlow(M)
#undef DEFINE_DirectIndexedYSlow

#define DEFINE_DirectIndexedYE0(m) \
static INLINE uint32_t DirectIndexedYE0_##m (void) \
{ \
	uint16_t	addr = Direct_##m() + Registers.Y.W; \
	AddCycles(ONE_CYCLE); \
	return (addr); \
}
DEFINE_DirectIndexedYE0(R)
DEFINE_DirectIndexedYE0(W)
DEFINE_DirectIndexedYE0(M)
#undef DEFINE_DirectIndexedYE0

#define DEFINE_DirectIndexedYE1(m) \
static INLINE uint32_t DirectIndexedYE1_##m (void) \
{ \
	pair	addr; \
	if (Registers.DL) \
		return (DirectIndexedYE0_##m()); \
	addr.W = Direct_##m(); \
	addr.B.l += Registers.YL; \
	AddCycles(ONE_CYCLE); \
	return (addr.W); \
}
DEFINE_DirectIndexedYE1(R)
DEFINE_DirectIndexedYE1(W)
DEFINE_DirectIndexedYE1(M)
#undef DEFINE_DirectIndexedYE1

#define DEFINE_DirectIndexedIndirectSlow(m) \
static INLINE uint32_t DirectIndexedIndirectSlow_##m (void) \
{ \
	uint32_t	addr = S9xGetWord(DirectIndexedXSlow_R(), (!CheckEmulation() || Registers.DL) ? WRAP_BANK : WRAP_PAGE); \
	IF_R(m, OpenBus = (uint8_t) (addr >> 8);) \
	return (ICPU.ShiftedDB | addr); \
}
DEFINE_DirectIndexedIndirectSlow(R)
DEFINE_DirectIndexedIndirectSlow(W)
DEFINE_DirectIndexedIndirectSlow(M)
#undef DEFINE_DirectIndexedIndirectSlow

#define DEFINE_DirectIndexedIndirectE0(m) \
static INLINE uint32_t DirectIndexedIndirectE0_##m (void) \
{ \
	uint32_t	addr = S9xGetWord(DirectIndexedXE0_R(), WRAP_NONE); \
	IF_R(m, OpenBus = (uint8_t) (addr >> 8);) \
	return (ICPU.ShiftedDB | addr); \
}
DEFINE_DirectIndexedIndirectE0(R)
DEFINE_DirectIndexedIndirectE0(W)
DEFINE_DirectIndexedIndirectE0(M)
#undef DEFINE_DirectIndexedIndirectE0

#define DEFINE_DirectIndexedIndirectE1(m) \
static INLINE uint32_t DirectIndexedIndirectE1_##m (void) \
{ \
	uint32_t	addr = S9xGetWord(DirectIndexedXE1_R(), Registers.DL ? WRAP_BANK : WRAP_PAGE); \
	IF_R(m, OpenBus = (uint8_t) (addr >> 8);) \
	return (ICPU.ShiftedDB | addr); \
}
DEFINE_DirectIndexedIndirectE1(R)
DEFINE_DirectIndexedIndirectE1(W)
DEFINE_DirectIndexedIndirectE1(M)
#undef DEFINE_DirectIndexedIndirectE1

/* a,X / a,Y page-cross: write/modify short-circuits the page check. */
#define DEFINE_AbsoluteIndexedXSlow(m) \
static INLINE uint32_t AbsoluteIndexedXSlow_##m (void) \
{ \
	uint32_t	addr = ABSOLUTESLOW_MACRO(m); \
	IF_W_ELSE(m, \
		AddCycles(ONE_CYCLE);, \
		if (!CheckIndex() || (addr & 0xff) + Registers.XL >= 0x100) \
			AddCycles(ONE_CYCLE);) \
	return (addr + Registers.X.W); \
}
DEFINE_AbsoluteIndexedXSlow(R)
DEFINE_AbsoluteIndexedXSlow(W)
DEFINE_AbsoluteIndexedXSlow(M)
#undef DEFINE_AbsoluteIndexedXSlow

#define DEFINE_AbsoluteIndexedXX0(m) \
static INLINE uint32_t AbsoluteIndexedXX0_##m (void) \
{ \
	uint32_t	addr = ABSOLUTE_MACRO(m); \
	AddCycles(ONE_CYCLE); \
	return (addr + Registers.X.W); \
}
DEFINE_AbsoluteIndexedXX0(R)
DEFINE_AbsoluteIndexedXX0(W)
DEFINE_AbsoluteIndexedXX0(M)
#undef DEFINE_AbsoluteIndexedXX0

#define DEFINE_AbsoluteIndexedXX1(m) \
static INLINE uint32_t AbsoluteIndexedXX1_##m (void) \
{ \
	uint32_t	addr = ABSOLUTE_MACRO(m); \
	IF_W_ELSE(m, \
		AddCycles(ONE_CYCLE);, \
		if ((addr & 0xff) + Registers.XL >= 0x100) \
			AddCycles(ONE_CYCLE);) \
	return (addr + Registers.X.W); \
}
DEFINE_AbsoluteIndexedXX1(R)
DEFINE_AbsoluteIndexedXX1(W)
DEFINE_AbsoluteIndexedXX1(M)
#undef DEFINE_AbsoluteIndexedXX1

#define DEFINE_AbsoluteIndexedYSlow(m) \
static INLINE uint32_t AbsoluteIndexedYSlow_##m (void) \
{ \
	uint32_t	addr = ABSOLUTESLOW_MACRO(m); \
	IF_W_ELSE(m, \
		AddCycles(ONE_CYCLE);, \
		if (!CheckIndex() || (addr & 0xff) + Registers.YL >= 0x100) \
			AddCycles(ONE_CYCLE);) \
	return (addr + Registers.Y.W); \
}
DEFINE_AbsoluteIndexedYSlow(R)
DEFINE_AbsoluteIndexedYSlow(W)
DEFINE_AbsoluteIndexedYSlow(M)
#undef DEFINE_AbsoluteIndexedYSlow

#define DEFINE_AbsoluteIndexedYX0(m) \
static INLINE uint32_t AbsoluteIndexedYX0_##m (void) \
{ \
	uint32_t	addr = ABSOLUTE_MACRO(m); \
	AddCycles(ONE_CYCLE); \
	return (addr + Registers.Y.W); \
}
DEFINE_AbsoluteIndexedYX0(R)
DEFINE_AbsoluteIndexedYX0(W)
DEFINE_AbsoluteIndexedYX0(M)
#undef DEFINE_AbsoluteIndexedYX0

#define DEFINE_AbsoluteIndexedYX1(m) \
static INLINE uint32_t AbsoluteIndexedYX1_##m (void) \
{ \
	uint32_t	addr = ABSOLUTE_MACRO(m); \
	IF_W_ELSE(m, \
		AddCycles(ONE_CYCLE);, \
		if ((addr & 0xff) + Registers.YL >= 0x100) \
			AddCycles(ONE_CYCLE);) \
	return (addr + Registers.Y.W); \
}
DEFINE_AbsoluteIndexedYX1(R)
DEFINE_AbsoluteIndexedYX1(W)
DEFINE_AbsoluteIndexedYX1(M)
#undef DEFINE_AbsoluteIndexedYX1

#define DEFINE_AbsoluteLongIndexedXSlow(m) \
static INLINE uint32_t AbsoluteLongIndexedXSlow_##m (void) \
{ \
	return (AbsoluteLongSlow_##m() + Registers.X.W); \
}
DEFINE_AbsoluteLongIndexedXSlow(R)
DEFINE_AbsoluteLongIndexedXSlow(W)
DEFINE_AbsoluteLongIndexedXSlow(M)
#undef DEFINE_AbsoluteLongIndexedXSlow

#define DEFINE_AbsoluteLongIndexedX(m) \
static INLINE uint32_t AbsoluteLongIndexedX_##m (void) \
{ \
	return (AbsoluteLong_##m() + Registers.X.W); \
}
DEFINE_AbsoluteLongIndexedX(R)
DEFINE_AbsoluteLongIndexedX(W)
DEFINE_AbsoluteLongIndexedX(M)
#undef DEFINE_AbsoluteLongIndexedX

#define DEFINE_StackRelativeSlow(m) \
static INLINE uint32_t StackRelativeSlow_##m (void) \
{ \
	uint16_t	addr = Immediate8Slow_##m() + Registers.S.W; \
	AddCycles(ONE_CYCLE); \
	return (addr); \
}
DEFINE_StackRelativeSlow(R)
DEFINE_StackRelativeSlow(W)
DEFINE_StackRelativeSlow(M)
#undef DEFINE_StackRelativeSlow

#define DEFINE_StackRelative(m) \
static INLINE uint32_t StackRelative_##m (void) \
{ \
	uint16_t	addr = Immediate8_##m() + Registers.S.W; \
	AddCycles(ONE_CYCLE); \
	return (addr); \
}
DEFINE_StackRelative(R)
DEFINE_StackRelative(W)
DEFINE_StackRelative(M)
#undef DEFINE_StackRelative

#define DEFINE_StackRelativeIndirectIndexedSlow(m) \
static INLINE uint32_t StackRelativeIndirectIndexedSlow_##m (void) \
{ \
	uint16_t	addr_stack_relative_slow = Immediate8Slow_R() + Registers.S.W; \
	uint32_t	addr; \
	AddCycles(ONE_CYCLE); \
	addr = S9xGetWord(addr_stack_relative_slow, WRAP_NONE); \
	IF_R(m, OpenBus = (uint8_t) (addr >> 8);) \
	addr = (addr + Registers.Y.W + ICPU.ShiftedDB) & 0xffffff; \
	AddCycles(ONE_CYCLE); \
	return (addr); \
}
DEFINE_StackRelativeIndirectIndexedSlow(R)
DEFINE_StackRelativeIndirectIndexedSlow(W)
DEFINE_StackRelativeIndirectIndexedSlow(M)
#undef DEFINE_StackRelativeIndirectIndexedSlow

#define DEFINE_StackRelativeIndirectIndexed(m) \
static INLINE uint32_t StackRelativeIndirectIndexed_##m (void) \
{ \
	uint16_t	addr_stack_relative = Immediate8Slow_R() + Registers.S.W; \
	uint32_t	addr; \
	AddCycles(ONE_CYCLE); \
	addr = S9xGetWord(addr_stack_relative, WRAP_NONE); \
	IF_R(m, OpenBus = (uint8_t) (addr >> 8);) \
	addr = (addr + Registers.Y.W + ICPU.ShiftedDB) & 0xffffff; \
	AddCycles(ONE_CYCLE); \
	return (addr); \
}
DEFINE_StackRelativeIndirectIndexed(R)
DEFINE_StackRelativeIndirectIndexed(W)
DEFINE_StackRelativeIndirectIndexed(M)
#undef DEFINE_StackRelativeIndirectIndexed

#endif
