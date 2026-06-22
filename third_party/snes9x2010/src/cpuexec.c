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

#include <string.h>
#include "snes9x.h"
#include "memmap.h"
#include "getset.h"
#include "cpuops.h"
#include "dma.h"
#include "apu.h"
#include "fxinst.h"
#include "fxemu.h"
#include "boolean.h"
#include "controls.h"
#include "cheats.h"
#include "ppu.h"
#include "snapshot.h"

extern struct SLineData		LineData[240];
extern struct SLineMatrixData	LineMatrixData[240];
extern uint8_t	*HDMAMemPointers[8];
static uint32_t idle_loop_target_pc;
static uint8_t idle_loop_elimination_enable;

static uint8_t finishedFrame = false;

void S9xMainLoop (void)
{
	do
	{
		do
		{
			register uint8_t	Op;
			register struct	SOpcodes *Opcodes;

			/* Speedhack - skip idle loop if exists. */
			if (idle_loop_elimination_enable &&
            (Registers.PBPC == idle_loop_target_pc) && (CPU.Cycles < CPU.NextEvent))
         CPU.Cycles = CPU.NextEvent;

			if (CPU.Flags)
			{
				if (CPU.Flags & NMI_FLAG)
				{
					if (Timings.NMITriggerPos <= CPU.Cycles)
					{
						CPU.Flags &= ~NMI_FLAG;
						Timings.NMITriggerPos = 0xffff;
						if (CPU.WaitingForInterrupt)
						{
							CPU.WaitingForInterrupt = FALSE;
							Registers.PCw++;
						}

						S9xOpcode_NMI();
					}
				}


				if (CPU.Flags & IRQ_FLAG)
				{
					if (CPU.IRQPending)
						CPU.IRQPending--;	/* FIXME: In case of IRQ during WRAM refresh */
					else
					{
						if (CPU.WaitingForInterrupt)
						{
							CPU.WaitingForInterrupt = FALSE;
							Registers.PCw++;
						}

						if (CPU.IRQActive)
						{
							/* in IRQ handler $4211 is supposed to be read, so IRQ_FLAG should be cleared. */
							if (!CheckFlag(IRQ))
								S9xOpcode_IRQ();
						}
						else
							CPU.Flags &= ~IRQ_FLAG;
					}
				}

				if (CPU.Flags & SCAN_KEYS_FLAG)
					break;

			}

			Opcodes = S9xOpcodesSlow;

			CPU.PrevCycles = CPU.Cycles;

			if (CPU.PCBase)
			{
				Op = CPU.PCBase[Registers.PCw];
				CPU.Cycles += CPU.MemSpeed;
				Opcodes = ICPU.S9xOpcodes;
			}
			else
			{
				Op = S9xGetByte(Registers.PBPC);
				OpenBus = Op;
			}

			if ((Registers.PCw & MEMMAP_MASK) + ICPU.S9xOpLengths[Op] >= MEMMAP_BLOCK_SIZE)
			{
				uint8_t	*oldPCBase = CPU.PCBase;

				CPU.PCBase = S9xGetBasePointer(ICPU.ShiftedPB + ((uint16_t) (Registers.PCw + 4)));
				if (oldPCBase != CPU.PCBase || (Registers.PCw & ~MEMMAP_MASK) == (0xffff & ~MEMMAP_MASK))
					Opcodes = S9xOpcodesSlow;
			}

			Registers.PCw++;
			(*Opcodes[Op].S9xOpcode)();


			if (Settings.SA1)
				S9xSA1MainLoop();

		#if (S9X_ACCURACY_LEVEL <= 2)
			while (CPU.Cycles >= CPU.NextEvent)
				S9xDoHEventProcessing();
		#endif

			if (finishedFrame)
				break;

		} while (1);

		if (!finishedFrame)
		{
			S9xPackStatus();

			if (CPU.Flags & SCAN_KEYS_FLAG)
			{
				CPU.Flags &= ~SCAN_KEYS_FLAG;
			}
		}
		else
		{
			finishedFrame = false;
			break;
		}
	} while (!finishedFrame);
}


static void S9xCheckMissingHTimerPosition (void)
{
	if (PPU.HTimerEnabled && (!PPU.VTimerEnabled || (CPU.V_Counter == PPU.VTimerPosition)))
	{
		S9X_SET_IRQ(PPU_IRQ_SOURCE);
	}
	else if (PPU.VTimerEnabled && (CPU.V_Counter == PPU.VTimerPosition))
	{
		S9X_SET_IRQ(PPU_IRQ_SOURCE);
	}
}

static void S9xCheckMissingHTimerHalt(void)
{
	if (PPU.HTimerEnabled && (!PPU.VTimerEnabled || (CPU.V_Counter == PPU.VTimerPosition)))
		CPU.IRQPending = 1;
	else if (PPU.VTimerEnabled && (CPU.V_Counter == PPU.VTimerPosition))
		CPU.IRQPending = 1;
}

static INLINE void speedhacks_manager (void)
{
	uint8_t var_mem, var_mem2, var_mem3;
   
   idle_loop_target_pc = 0x00;
   idle_loop_elimination_enable = FALSE;

	switch(Settings.SpeedhackGameID)
   {
      case SPEEDHACK_CT:
         /* Chrono Trigger mid-frame overscan hack - field to battle transition */
         if (IPPU.RenderedScreenHeight == 239)
            IPPU.RenderedScreenHeight = 224;
         break;
      case SPEEDHACK_DKC1:
         PPU.SFXSpeedupHack = FALSE;
         var_mem = Memory.RAM[0x003E]; /* current level - 7E003E */
         if(var_mem == 49 || var_mem == 217 || var_mem == 66 || var_mem == 67)
            PPU.SFXSpeedupHack = TRUE;
         break;
      case SPEEDHACK_FF6:
         PPU.FullClipping = FALSE;
         coldata_update_screen = FALSE;

         var_mem = Memory.RAM[0x0063]; /* menu on/off - 7E0063 */
         var_mem2 = Memory.RAM[0x3A8F]; /* battle active - 3A8F */
         var_mem3 = Memory.RAM[0x00BA]; /* covers window toggle - 8991 */
         if(var_mem)
            PPU.FullClipping = var_mem;

         /* check if window toggle is on */
         if(var_mem3 == 1)
            coldata_update_screen = TRUE;
         /* check if menu or battle screen is on */
         if(var_mem2 == 1)
            coldata_update_screen = TRUE;
         if(var_mem)
            coldata_update_screen = TRUE;
         break;
      case SPEEDHACK_STAR_FOX_1:
         PPU.SFXSpeedupHack = (PPU.BGMode == 1 || PPU.BGMode == 2);
         break;
      case SPEEDHACK_SUPER_MARIO_WORLD:
         idle_loop_target_pc = 0x00806B;
         idle_loop_elimination_enable = TRUE;
         break;
      default:
         break;
   }
}

static void S9xEndScreenRefresh (void)
{
	if (IPPU.RenderThisFrame)
	{
		FLUSH_REDRAW();
		/* M7 vertical-2x post-pass: if armed by frame-start setup or
		 * the mid-frame mode-switch hook in ppu.c, expand the 224-row
		 * frame in place to 448 rows. Bumps IPPU.RenderedScreenHeight
		 * to PPU.ScreenHeight*2 so the next call to S9xDeinitUpdate
		 * picks up the new size. No-op if M7VertStartY is -1. */
		S9xMode7VertResample();
	}

	if (!(GFX.DoInterlace && GFX.InterlaceFrame == 0))
	{
		S9xDeinitUpdate(IPPU.RenderedScreenWidth, IPPU.RenderedScreenHeight);
		finishedFrame = true;
	}
	PPU.GunVLatch = 1000; /* i.e., never latch */
	PPU.GunHLatch = 0;

	if (!Settings.NormalControls && pad_read)
		S9xControlEOF();

	pad_read = FALSE;

	if (Settings.SpeedhackGameID > SPEEDHACK_NONE)
		speedhacks_manager();

	S9xApplyCheats();
}

static void RenderLine (uint8_t C)
{
	if (IPPU.RenderThisFrame)
	{
		LineData[C].BG[0].VOffset = PPU.BG[0].VOffset + 1;
		LineData[C].BG[0].HOffset = PPU.BG[0].HOffset;
		LineData[C].BG[1].VOffset = PPU.BG[1].VOffset + 1;
		LineData[C].BG[1].HOffset = PPU.BG[1].HOffset;

		if (PPU.BGMode == 7)
		{
			struct SLineMatrixData *p = &LineMatrixData[C];
			p->MatrixA = PPU.MatrixA;
			p->MatrixB = PPU.MatrixB;
			p->MatrixC = PPU.MatrixC;
			p->MatrixD = PPU.MatrixD;
			p->CentreX = PPU.CentreX;
			p->CentreY = PPU.CentreY;
			p->M7HOFS  = PPU.M7HOFS;
			p->M7VOFS  = PPU.M7VOFS;
		}
		else
		{
			LineData[C].BG[2].VOffset = PPU.BG[2].VOffset + 1;
			LineData[C].BG[2].HOffset = PPU.BG[2].HOffset;
			LineData[C].BG[3].VOffset = PPU.BG[3].VOffset + 1;
			LineData[C].BG[3].HOffset = PPU.BG[3].HOffset;
		}

		IPPU.CurrentLine = C + 1;
	}
	else
	{
		// if we're not rendering this frame, we still need to update this
		// XXX: Check ForceBlank? Or anything else?
		if (IPPU.OBJChanged)
			SetupOBJ();
		PPU.RangeTimeOver |= GFX.OBJLines[C].RTOFlags;
	}
}

static INLINE void S9xReschedule (void)
{
	uint8_t next;
	int32_t hpos;

	next = 0;
	hpos = 0;

	switch (CPU.WhichEvent)
	{
		case HC_HBLANK_START_EVENT:
		case HC_IRQ_1_3_EVENT:
			next = HC_HDMA_START_EVENT;
			hpos = Timings.HDMAStart;
			break;

		case HC_HDMA_START_EVENT:
		case HC_IRQ_3_5_EVENT:
			next = HC_HCOUNTER_MAX_EVENT;
			hpos = Timings.H_Max;
			break;

		case HC_HCOUNTER_MAX_EVENT:
		case HC_IRQ_5_7_EVENT:
			next = HC_HDMA_INIT_EVENT;
			hpos = Timings.HDMAInit;
			break;

		case HC_HDMA_INIT_EVENT:
		case HC_IRQ_7_9_EVENT:
			next = HC_RENDER_EVENT;
			hpos = Timings.RenderPos;
			break;

		case HC_RENDER_EVENT:
		case HC_IRQ_9_A_EVENT:
			next = HC_WRAM_REFRESH_EVENT;
			hpos = Timings.WRAMRefreshPos;
			break;

		case HC_WRAM_REFRESH_EVENT:
		case HC_IRQ_A_1_EVENT:
			next = HC_HBLANK_START_EVENT;
			hpos = Timings.HBlankStart;
			break;
	}

	if (((int32_t) PPU.HTimerPosition > CPU.NextEvent) && ((int32_t) PPU.HTimerPosition < hpos))
	{
		hpos = (int32_t) PPU.HTimerPosition;

		switch (next)
		{
			case HC_HDMA_START_EVENT:
				next = HC_IRQ_1_3_EVENT;
				break;

			case HC_HCOUNTER_MAX_EVENT:
				next = HC_IRQ_3_5_EVENT;
				break;

			case HC_HDMA_INIT_EVENT:
				next = HC_IRQ_5_7_EVENT;
				break;

			case HC_RENDER_EVENT:
				next = HC_IRQ_7_9_EVENT;
				break;

			case HC_WRAM_REFRESH_EVENT:
				next = HC_IRQ_9_A_EVENT;
				break;

			case HC_HBLANK_START_EVENT:
				next = HC_IRQ_A_1_EVENT;
				break;
		}
	}

	CPU.NextEvent  = hpos;
	CPU.WhichEvent = next;
}


static INLINE uint8_t HDMAReadLineCount (int d)
{
	/* CPU.InDMA is set, so S9xGetXXX() / S9xSetXXX() incur no charges. */

	uint8_t	line;

	line = S9xGetByte((DMA[d].ABank << 16) + DMA[d].Address);
	CPU.Cycles += SLOW_ONE_CYCLE;

	DMA[d].LineCount = 128;

	if (!line)
	{
		DMA[d].Repeat = FALSE;

		if (DMA[d].HDMAIndirectAddressing)
		{
			if (PPU.HDMA & (0xfe << d))
			{
				DMA[d].Address++;
				CPU.Cycles += (SLOW_ONE_CYCLE << 1);
			}
			else
				CPU.Cycles += SLOW_ONE_CYCLE;

			DMA[d].IndirectAddress = S9xGetWord((DMA[d].ABank << 16) + DMA[d].Address, WRAP_NONE);
			DMA[d].Address++;
		}

		DMA[d].Address++;
		HDMAMemPointers[d] = NULL;

		return (FALSE);
	}
	else
	if (line == 0x80)
		DMA[d].Repeat = TRUE;
	else
	{
		DMA[d].Repeat = !(line & 0x80);
		DMA[d].LineCount = line & 0x7f;
	}

	DMA[d].Address++;
	DMA[d].DoTransfer = TRUE;

	if (DMA[d].HDMAIndirectAddressing)
	{
		CPU.Cycles += (SLOW_ONE_CYCLE << 1);
		DMA[d].IndirectAddress = S9xGetWord((DMA[d].ABank << 16) + DMA[d].Address, WRAP_NONE);
		DMA[d].Address += 2;
		HDMAMemPointers[d] = S9xGetMemPointer((DMA[d].IndirectBank << 16) + DMA[d].IndirectAddress);
	}
	else
		HDMAMemPointers[d] = S9xGetMemPointer((DMA[d].ABank << 16) + DMA[d].Address);

	return (TRUE);
}

static void S9xStartHDMA (void)
{
	uint8_t i;
	int32_t tmpch;

	PPU.HDMA = Memory.FillRAM[0x420c];
	PPU.HDMAEnded = 0;

	CPU.InHDMA = TRUE;
	CPU.InDMAorHDMA = TRUE;

	tmpch = CPU.CurrentDMAorHDMAChannel;

	/* XXX: Not quite right... */
	if (PPU.HDMA != 0)
		CPU.Cycles += Timings.DMACPUSync;

	for ( i = 0; i < 8; i++)
	{
		if (PPU.HDMA & (1 << i))
		{
			CPU.CurrentDMAorHDMAChannel = i;

			DMA[i].Address = DMA[i].AAddress;

			if (!HDMAReadLineCount(i))
			{
				PPU.HDMA &= ~(1 << i);
				PPU.HDMAEnded |= (1 << i);
			}
		}
		else
			DMA[i].DoTransfer = FALSE;
	}

	CPU.InHDMA = FALSE;
	CPU.InDMAorHDMA = CPU.InDMA;
	CPU.HDMARanInDMA = CPU.InDMA ? PPU.HDMA : 0;
	CPU.CurrentDMAorHDMAChannel = tmpch;
}

static const int HDMA_ModeByteCounts[8] =
{
	1, 2, 2, 4, 4, 4, 2, 4
};

static uint8_t S9xDoHDMA (uint8_t byte)
{
	uint8_t mask;
	uint32_t	ShiftedIBank;
	uint16_t	IAddr;
	uint8_t	temp;
	int32_t	tmpch;
	int d;
	struct SDMA *p;

	p = &DMA[0];

	d = 0;

	CPU.InHDMA = TRUE;
	CPU.InDMAorHDMA = TRUE;
	CPU.HDMARanInDMA = CPU.InDMA ? byte : 0;
	temp = CPU.InWRAMDMAorHDMA;
	tmpch = CPU.CurrentDMAorHDMAChannel;

	/* XXX: Not quite right... */
	CPU.Cycles += Timings.DMACPUSync;

	for ( mask = 1; mask; mask <<= 1, p++, d++)
	{
		if (byte & mask)
		{
			CPU.InWRAMDMAorHDMA = FALSE;
			CPU.CurrentDMAorHDMAChannel = d;

			if (p->HDMAIndirectAddressing)
			{
				ShiftedIBank = (p->IndirectBank << 16);
				IAddr = p->IndirectAddress;
			}
			else
			{
				ShiftedIBank = (p->ABank << 16);
				IAddr = p->Address;
			}

			if (!HDMAMemPointers[d])
				HDMAMemPointers[d] = S9xGetMemPointer(ShiftedIBank + IAddr);

			if (p->DoTransfer)
			{
				/* XXX: Hack for Uniracers, because we don't understand 
				OAM Address Invalidation */
				if (p->BAddress == 0x04 && SNESGameFixes.Uniracers)
				{
					PPU.OAMAddr = 0x10c;
					PPU.OAMFlip = 0;
				}


				if (!p->ReverseTransfer)
				{
					if ((IAddr & MEMMAP_MASK) + HDMA_ModeByteCounts[p->TransferMode] >= MEMMAP_BLOCK_SIZE)
					{
						/* HDMA REALLY-SLOW PATH */
						HDMAMemPointers[d] = NULL;

						#define DOBYTE(Addr, RegOff) \
							CPU.InWRAMDMAorHDMA = (ShiftedIBank == 0x7e0000 || ShiftedIBank == 0x7f0000 || \
								(!(ShiftedIBank & 0x400000) && ((uint16_t) (Addr)) < 0x2000)); \
							S9xSetPPU(S9xGetByte(ShiftedIBank + ((uint16_t) (Addr))), 0x2100 + p->BAddress + (RegOff));

						DOBYTE(IAddr, 0);
						CPU.Cycles += SLOW_ONE_CYCLE;
						switch (p->TransferMode)
						{
							case 0:
								break;

							case 5:
								DOBYTE(IAddr + 1, 1);
								CPU.Cycles += SLOW_ONE_CYCLE;
								DOBYTE(IAddr + 2, 0);
								CPU.Cycles += SLOW_ONE_CYCLE;
								DOBYTE(IAddr + 3, 1);
								CPU.Cycles += SLOW_ONE_CYCLE;
								break;

							case 1:
								DOBYTE(IAddr + 1, 1);
								CPU.Cycles += SLOW_ONE_CYCLE;
								break;

							case 2:
							case 6:
								DOBYTE(IAddr + 1, 0);
								CPU.Cycles += SLOW_ONE_CYCLE;
								break;

							case 3:
							case 7:
								DOBYTE(IAddr + 1, 0);
								CPU.Cycles += SLOW_ONE_CYCLE;
								DOBYTE(IAddr + 2, 1);
								CPU.Cycles += SLOW_ONE_CYCLE;
								DOBYTE(IAddr + 3, 1);
								CPU.Cycles += SLOW_ONE_CYCLE;
								break;

							case 4:
								DOBYTE(IAddr + 1, 1);
								CPU.Cycles += SLOW_ONE_CYCLE;
								DOBYTE(IAddr + 2, 2);
								CPU.Cycles += SLOW_ONE_CYCLE;
								DOBYTE(IAddr + 3, 3);
								CPU.Cycles += SLOW_ONE_CYCLE;
								break;
						}

						#undef DOBYTE
					}
					else
					{
						CPU.InWRAMDMAorHDMA = (ShiftedIBank == 0x7e0000 || ShiftedIBank == 0x7f0000 ||
							(!(ShiftedIBank & 0x400000) && IAddr < 0x2000));

						if (!HDMAMemPointers[d])
						{
							/* HDMA SLOW PATH */
							uint32_t	Addr = ShiftedIBank + IAddr;

							switch (p->TransferMode)
							{
								case 0:
									S9xSetPPU(S9xGetByte(Addr), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									break;

								case 5:
									S9xSetPPU(S9xGetByte(Addr + 0), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(S9xGetByte(Addr + 1), 0x2101 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									Addr += 2;
									/* fall through */
								case 1:
									S9xSetPPU(S9xGetByte(Addr + 0), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(S9xGetByte(Addr + 1), 0x2101 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									break;

								case 2:
								case 6:
									S9xSetPPU(S9xGetByte(Addr + 0), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(S9xGetByte(Addr + 1), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									break;

								case 3:
								case 7:
									S9xSetPPU(S9xGetByte(Addr + 0), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(S9xGetByte(Addr + 1), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(S9xGetByte(Addr + 2), 0x2101 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(S9xGetByte(Addr + 3), 0x2101 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									break;

								case 4:
									S9xSetPPU(S9xGetByte(Addr + 0), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(S9xGetByte(Addr + 1), 0x2101 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(S9xGetByte(Addr + 2), 0x2102 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(S9xGetByte(Addr + 3), 0x2103 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									break;
							}
						}
						else
						{
							/* HDMA FAST PATH */
							switch (p->TransferMode)
							{
								case 0:
									S9xSetPPU(*HDMAMemPointers[d]++, 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									break;

								case 5:
									S9xSetPPU(*(HDMAMemPointers[d]), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(*(HDMAMemPointers[d] + 1), 0x2101 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									HDMAMemPointers[d] += 2;
									/* fall through */
								case 1:
									S9xSetPPU(*(HDMAMemPointers[d]), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(*(HDMAMemPointers[d] + 1), 0x2101 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									HDMAMemPointers[d] += 2;
									break;

								case 2:
								case 6:
									S9xSetPPU(*(HDMAMemPointers[d]), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(*(HDMAMemPointers[d] + 1), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									HDMAMemPointers[d] += 2;
									break;

								case 3:
								case 7:
									S9xSetPPU(*(HDMAMemPointers[d]), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(*(HDMAMemPointers[d] + 1), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(*(HDMAMemPointers[d] + 2), 0x2101 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(*(HDMAMemPointers[d] + 3), 0x2101 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									HDMAMemPointers[d] += 4;
									break;

								case 4:
									S9xSetPPU(*(HDMAMemPointers[d]), 0x2100 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(*(HDMAMemPointers[d] + 1), 0x2101 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(*(HDMAMemPointers[d] + 2), 0x2102 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									S9xSetPPU(*(HDMAMemPointers[d] + 3), 0x2103 + p->BAddress);
									CPU.Cycles += SLOW_ONE_CYCLE;
									HDMAMemPointers[d] += 4;
									break;
							}
						}
					}
				}

				if (p->HDMAIndirectAddressing)
					p->IndirectAddress += HDMA_ModeByteCounts[p->TransferMode];
				else
					p->Address += HDMA_ModeByteCounts[p->TransferMode];
			}

			p->DoTransfer = !p->Repeat;

			if (!--p->LineCount)
			{
				if (!HDMAReadLineCount(d))
				{
					byte &= ~mask;
					PPU.HDMAEnded |= mask;
					p->DoTransfer = FALSE;
					continue;
				}
			}
			else
				CPU.Cycles += SLOW_ONE_CYCLE;
		}
	}

	CPU.InHDMA = FALSE;
	CPU.InDMAorHDMA = CPU.InDMA;
	CPU.InWRAMDMAorHDMA = temp;
	CPU.CurrentDMAorHDMAChannel = tmpch;

	return (byte);
}

void S9xDoHEventProcessing (void)
{
	uint8_t tmp;

	switch (CPU.WhichEvent)
	{
		case HC_HBLANK_START_EVENT:
			if (PPU.HTimerPosition == Timings.HBlankStart)
				S9xCheckMissingHTimerPosition();
			S9xReschedule();
			break;

		case HC_HDMA_START_EVENT:
			if (PPU.HTimerPosition == Timings.HDMAStart)
				S9xCheckMissingHTimerPosition();
			S9xReschedule();

			if (PPU.HDMA && CPU.V_Counter <= PPU.ScreenHeight)
				PPU.HDMA = S9xDoHDMA(PPU.HDMA);

			break;

		case HC_HCOUNTER_MAX_EVENT:
			if (Settings.SuperFX && !SuperFX.oneLineDone && CHECK_EXEC_SUPERFX())
				S9xSuperFXExec();
			SuperFX.oneLineDone = FALSE; // do this even without SFX

			S9xAPUExecute();
			CPU.Cycles -= Timings.H_Max;
			S9xAPUSetReferenceTime(CPU.Cycles);

			if ((Timings.NMITriggerPos != 0xffff) && (Timings.NMITriggerPos >= Timings.H_Max))
				Timings.NMITriggerPos -= Timings.H_Max;

			CPU.V_Counter++;
			if (CPU.V_Counter >= Timings.V_Max)	/* V ranges from 0 to Timings.V_Max - 1 */
			{
				CPU.V_Counter = 0;
				Timings.InterlaceField ^= 1;

				/* From byuu:
				   [NTSC]
				   interlace mode has 525 scanlines: 263 on 
				   the even frame, and 262 on the odd.

				   non-interlace mode has 524 scanlines: 
				   262 scanlines on both even and odd frames.
				   
				   [PAL] <PAL info is unverified on hardware>
				   interlace mode has 625 scanlines: 313 on 
				   the even frame, and 312 on the odd.
				   
				   non-interlace mode has 624 scanlines: 
				   312 scanlines on both even and odd frames. */

				Timings.V_Max = Timings.V_Max_Master;	/* 262 (NTSC), 312?(PAL) */

				if (IPPU.Interlace && !Timings.InterlaceField)
					Timings.V_Max += 1;	/* 263 (NTSC), 313?(PAL) */

				Memory.FillRAM[0x213F] ^= 0x80;
				PPU.RangeTimeOver = 0;

				/* FIXME: reading $4210 will wait 2 cycles, 
				then perform reading, then wait 4 more cycles. */

				Memory.FillRAM[0x4210] = MAX_5A22_VERSION;
				CPU.Flags &= ~NMI_FLAG;
				Timings.NMITriggerPos = 0xffff;

				ICPU.Frame++;
				PPU.HVBeamCounterLatched = 0;
				CPU.Flags |= SCAN_KEYS_FLAG;
			}

			/* From byuu:
			   In non-interlace mode, there are 341 dots 
			   per scanline, and 262 scanlines per frame.
			   
			   On odd frames, scanline 240 is one dot short.
			   In interlace mode, there are always 341 dots 
			   per scanline. Even frames have 263 scanlines,
			   and odd frames have 262 scanlines.
			   
			   Interlace mode scanline 240 on odd frames is 
			   not missing a dot. */

			Timings.H_Max = Timings.H_Max_Master;					/* HC=1364 */

			if (CPU.V_Counter == 240 && !IPPU.Interlace && Timings.InterlaceField)	/* V=240 */
				Timings.H_Max -= ONE_DOT_CYCLE;					/* HC=1360 */

			if (CPU.V_Counter != 240 || IPPU.Interlace || !Timings.InterlaceField)	/* V=240 */
			{
				if (Timings.WRAMRefreshPos == SNES_WRAM_REFRESH_HC_v2_MIN_ONE_DOT_CYCLE) /* HC=534 */
					Timings.WRAMRefreshPos = SNES_WRAM_REFRESH_HC_v2; /* HC=538 */
				else
					Timings.WRAMRefreshPos = SNES_WRAM_REFRESH_HC_v2_MIN_ONE_DOT_CYCLE; /* HC=534 */
			}

			if (PPU.HTimerPosition == 0)
				S9xCheckMissingHTimerPosition();

			if (CPU.V_Counter == PPU.ScreenHeight + FIRST_VISIBLE_LINE) /* VBlank starts from V=225(240). */
			{
				S9xEndScreenRefresh();
					
				PPU.HDMA = 0;
				/* Bits 7 and 6 of $4212 are computed when read in S9xGetPPU. */
				PPU.ForcedBlanking = (Memory.FillRAM[0x2100] >> 7) & 1;

				if (!PPU.ForcedBlanking)
				{
					PPU.OAMAddr = PPU.SavedOAMAddr;

					tmp = 0;

					if (PPU.OAMPriorityRotation)
						tmp = (PPU.OAMAddr & 0xFE) >> 1;
					if ((PPU.OAMFlip & 1) || PPU.FirstSprite != tmp)
					{
						PPU.FirstSprite = tmp;
						IPPU.OBJChanged = TRUE;
					}

					PPU.OAMFlip = 0;
				}

				/* FIXME: writing to $4210 will wait 6 cycles. */
				Memory.FillRAM[0x4210] = 0x80 | MAX_5A22_VERSION;
				if (Memory.FillRAM[0x4200] & 0x80)
				{
					/* FIXME: triggered at HC=6, checked just 
					before the final CPU cycle, then, when to 
					call S9xOpcode_NMI()? */
					CPU.Flags |= NMI_FLAG;
					Timings.NMITriggerPos = 6 + 6;
				}

			}

			/* FIXME: not true */
			if ((CPU.V_Counter == PPU.ScreenHeight + 3) && (Memory.FillRAM[0x4200] & 1))
			{
				S9xDoAutoJoypad();
			}

			/* V = 1 */
			if (CPU.V_Counter == FIRST_VISIBLE_LINE)
			{
				if (IPPU.RenderThisFrame)
				{
					GFX.InterlaceFrame = !GFX.InterlaceFrame;

					if (!GFX.DoInterlace || !GFX.InterlaceFrame)
					{
						/* S9x Start Screen Refresh */
						uint8_t cond_1, cond_2, cond_q;

						GFX.DoInterlace -= (GFX.DoInterlace == TRUE);

						IPPU.Interlace    = Memory.FillRAM[0x2133] & 1;
						IPPU.InterlaceOBJ = Memory.FillRAM[0x2133] & 2;
						IPPU.PseudoHires = Memory.FillRAM[0x2133] & 8;

						/* cond_1: this frame should render at 2x or wider.
						   Triggered by Mode 5/6 / PseudoHires as on hardware,
						   plus the Mode7Hires option which forces a wider
						   buffer when the frame starts in Mode 7 so the
						   M7Hires/M7HR4X renderers have room to write into.

						   cond_q: this frame should render at 4x. Only set
						   for Mode 7 with the 4x hires setting. */
						cond_1 = (Settings.SupportHiRes && (PPU.BGMode == 5 || PPU.BGMode == 6 || IPPU.PseudoHires
								|| (Settings.Mode7Hires && PPU.BGMode == 7)));
						cond_q = (Settings.SupportHiRes && PPU.BGMode == 7 && Settings.Mode7Hires == 4);
						cond_2 = (Settings.SupportHiRes && IPPU.Interlace);

						GFX.RealPPL = GFX.Pitch >> 1;
						/* Width factor: 4 for Mode 7 + 4x hires, 2 for any
						   other hires path, 1 for native. */
						{
							int width_factor = cond_q ? 4 : (cond_1 ? 2 : 1);
							IPPU.RenderedScreenWidth = SNES_WIDTH * width_factor;
						}
						IPPU.RenderedScreenHeight = PPU.ScreenHeight << cond_2;
						IPPU.DoubleWidthPixels = cond_1;
						IPPU.QuadWidthPixels = cond_q;
						IPPU.DoubleHeightPixels = cond_2;

						GFX.PPL = GFX.RealPPL << cond_2;
						GFX.DoInterlace += cond_2;

						/* M7 vertical-2x post-pass: arm if the user opted
						 * in and the frame begins in BG mode 7 with hires
						 * enabled. M7VertStartY = 0 means the entire
						 * 0..PPU.ScreenHeight range gets bilinear-Y
						 * interpolation at end-of-frame. Frames that
						 * switch to mode 7 mid-frame are armed by the
						 * mode-switch hook in ppu.c. */
						IPPU.M7VertStartY = -1;
						if (cond_1
						 && PPU.BGMode == 7
						 && Settings.Mode7HiresVertical)
							IPPU.M7VertStartY = 0;

						/* Backend hook: try to redirect GFX.Screen at the
						   frontend's swapchain buffer for zero-copy
						   presentation. May silently rewrite GFX.Screen,
						   GFX.Pitch, GFX.RealPPL and GFX.PPL. No-op when
						   redirect is not possible (NTSC filter on,
						   frontend doesn't support sw_fb, etc.). */
						S9xLibretroSwFbAcquire(IPPU.RenderedScreenWidth,
								IPPU.RenderedScreenHeight);
					}

					PPU.MosaicStart = 0;
					PPU.RecomputeClipWindows = TRUE;
					IPPU.PreviousLine = IPPU.CurrentLine = 0;

					/* Clear only the byte range the renderer actually touches.

					   Every render path in ppu.c / tile.c iterates
					   `for (Y = StartY; Y <= EndY; Y++) Offset = Y * GFX.PPL`,
					   with EndY clamped to PPU.ScreenHeight - 1, so the
					   number of scanlines the renderer writes to the
					   Z-buffers is IPPU.RenderedScreenHeight (which equals
					   PPU.ScreenHeight in non-interlaced modes and twice
					   that with interlace, because the interlaced renderer
					   writes twice as many scanlines into the same buffer).
					   Each scanline is RealPPL bytes (the buffer's true row
					   stride, before the interlace PPL doubling).

					   For non-interlaced 224-line NTSC content this is
					   ~229 KB instead of GFX.ScreenSize's ~489 KB - a ~53%
					   reduction. Doubled because we clear two buffers (Z
					   and SubZ), so ~520 KB / frame avoided. At 60 fps
					   that's ~31 MB/sec less zero-writing on weak hardware.
					   Interlaced and ticks=4 hires modes hit the buffer's
					   full extent, so the savings shrink to a few percent
					   in those cases - the optimization just gracefully
					   degrades to the full clear.

					   high_water tracks the running max over the session
					   so a transition from a wider footprint (interlace,
					   bigger sw_fb pitch) to a narrower one still clears
					   any stale Z values at byte positions that were
					   written by the wider footprint. Never decreases;
					   in pathological cases this means we keep clearing
					   the high-water size after a one-time hires frame,
					   which matches the behavior before this change. */
					{
						static size_t high_water = 0;
						size_t footprint = (size_t)IPPU.RenderedScreenHeight * GFX.RealPPL;
						if (footprint > high_water)
							high_water = footprint;
						if (high_water > GFX.ScreenSize)
							high_water = GFX.ScreenSize;
						memset(GFX.ZBuffer,    0, high_water);
						memset(GFX.SubZBuffer, 0, high_water);
					}
				}
			}

			CPU.NextEvent = -1;
			S9xReschedule();

			break;

		case HC_HDMA_INIT_EVENT:
			if (PPU.HTimerPosition == Timings.HDMAInit)
				S9xCheckMissingHTimerPosition();
			S9xReschedule();

			if (CPU.V_Counter == 0)
				S9xStartHDMA();

			break;

		case HC_RENDER_EVENT:
			if (CPU.V_Counter >= FIRST_VISIBLE_LINE && CPU.V_Counter <= PPU.ScreenHeight)
				RenderLine((uint8_t) (CPU.V_Counter - FIRST_VISIBLE_LINE));

			if (PPU.HTimerPosition == Timings.RenderPos)
				S9xCheckMissingHTimerPosition();
			S9xReschedule();

			break;

		case HC_WRAM_REFRESH_EVENT:

			if ((PPU.HTimerPosition >= Timings.WRAMRefreshPos) && (PPU.HTimerPosition < (Timings.WRAMRefreshPos + SNES_WRAM_REFRESH_CYCLES)))
				S9xCheckMissingHTimerHalt();
			CPU.Cycles += SNES_WRAM_REFRESH_CYCLES;

			if (PPU.HTimerPosition == Timings.WRAMRefreshPos)
				S9xCheckMissingHTimerPosition();
			S9xReschedule();

			break;

		case HC_IRQ_1_3_EVENT:
		case HC_IRQ_3_5_EVENT:
		case HC_IRQ_5_7_EVENT:
		case HC_IRQ_7_9_EVENT:
		case HC_IRQ_9_A_EVENT:
		case HC_IRQ_A_1_EVENT:
			if ((PPU.HTimerEnabled && (!PPU.VTimerEnabled || (CPU.V_Counter == PPU.VTimerPosition))) || (PPU.VTimerEnabled && (CPU.V_Counter == PPU.VTimerPosition)))
			{
				S9X_SET_IRQ(PPU_IRQ_SOURCE);
			}

			S9xReschedule();
			break;
	}

}

#include "cpuops_.h"
