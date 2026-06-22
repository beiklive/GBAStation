# tile.c — internals walkthrough

`src/tile.c` is the heart of the software renderer. It generates several
hundred small tile-rendering functions through a self-recursive include
trick, and then a runtime dispatcher (`S9xSelectTileRenderers`) picks
one function per BG layer per frame based on the SNES PPU's current
state. This document describes how that templating works and what it
produces, so a contributor reading the file knows what they are
looking at.

This is a *descriptive* document — it documents what is in the source
today. It does not propose changes.


## Why so much templating

A tile-rendering hot path needs to vary along several orthogonal axes:

  - WHICH renderer family (sprites? backdrop? Mode 7 BG1? Mode 7 BG1
    with bilinear filtering? mosaic?) — call this `NAME1`.
  - WHICH output buffer width / interlace pattern (1× native, 2× hires
    / mode-5-6 / pseudohires, 4× Mode 7 hires, interlace, hires
    interlace) — call this `NAME2`.
  - WHICH color-math operation (none, add, sub, ½ blend, ½ blend with
    fixed colour) — call this `MATH`.

If you wrote each combination as one function, the pixel-write
arithmetic would be repeated everywhere; if you put the choice
behind a runtime branch in the inner loop, you'd pay for it on
every pixel. C's pre-processor lets you have it both ways: write
each variation once at the macro level, then materialise every
combination as its own specialised function at compile time.

The cost is that the file is bizarre to read: a single source file
includes itself three times.


## The three-level self-include

The file starts with `#ifndef _NEWTILE_CPP / #define _NEWTILE_CPP`.
That guard is set on the first compile, so when
`tile.c` `#include "tile.c"`s itself, the `_NEWTILE_CPP` arm is
skipped and the pre-processor falls through to the level-1
machinery instead.

Subsequent levels are gated by which of `NAME1` / `NAME2` are
defined:

```
   _NEWTILE_CPP undefined  →  level 0 (top-level): preamble
                              math primitives, lookup tables,
                              palette helpers, then a single
                              #include "tile.c" to enter level 1.

   _NEWTILE_CPP defined,
   NAME1 undefined         →  level 1: declare DRAW_TILE,
                              ARGS, Z1, Z2 (and Mode-7-specific
                              MASK/DCMODE/BG) for one renderer
                              family; then #include "tile.c" to
                              enter level 2.

   NAME1 defined,
   NAME2 undefined         →  level 2: for each output-width /
                              interlace pattern, declare
                              DRAW_PIXEL (the per-pixel writer)
                              and NAME2; then #include "tile.c"
                              to enter level 3.

   NAME1 and NAME2 both
   defined                 →  level 3: emit the actual function
                              bodies (one per MATH op) and the
                              7-entry dispatch array.
```

The whole flow is one file pretending to be three files.

```
            ┌──── tile.c (level 0) ────┐
            │ preamble: math, tables   │
            │ #include "tile.c"  ──────┼─→ level 1
            └──────────────────────────┘
                                         ┌──── tile.c (level 1) ────┐
                                         │ #define NAME1, ARGS,     │
                                         │   DRAW_TILE, Z1, Z2      │
                                         │ #include "tile.c"  ──────┼─→ level 2
                                         │ #undef ...               │
                                         │ (next family: redefine,  │
                                         │   include again, undef)  │
                                         │ ...                      │
                                         │ S9xSelectTileRenderers() │
                                         └──────────────────────────┘
                                                                       ┌──── tile.c (level 2) ────┐
                                                                       │ #define NAME2, DRAW_PIXEL│
                                                                       │ #include "tile.c"  ──────┼─→ level 3
                                                                       │ #undef ...               │
                                                                       │ (next NAME2 variant)     │
                                                                       └──────────────────────────┘
                                                                                                    ┌──── tile.c (level 3) ────┐
                                                                                                    │ static fn for each MATH op│
                                                                                                    │ (7 variants), then a      │
                                                                                                    │ Renderers_NAME1NAME2[7]   │
                                                                                                    │ dispatch array.           │
                                                                                                    └──────────────────────────┘
```


## Level 0 — the preamble

Lines 1 through 657. Runs once. Contents:

  - License header.
  - Includes: `snes9x.h`, `ppu.h`, `tile.h`, `retro_inline.h`.
  - Inline functions: `COLOR_ADD`, `COLOR_SUB`, `COLOR_ADD1_2`,
    `COLOR_SUB1_2`. These are the actual color-math primitives,
    operating on RGB565 pixel pairs with the SNES's "intensity-half"
    semantics.
  - Lookup-table builders for the COLOR_* fast paths.
  - The 2-/4-/8-bpp tile decoders (`ConvertTile2`, `ConvertTile4`,
    `ConvertTile8`), called lazily at draw time when a particular
    tile is touched.
  - Helper macros used by every later level: `GET_CACHED_TILE`,
    `IS_BLANK_TILE`, `SELECT_PALETTE`.
  - The four MATH-operator selector macros:
    `NOMATH(Op, Main, Sub, SD)`,
    `REGMATH(Op, Main, Sub, SD)`,
    `MATHF1_2(Op, Main, Sub, SD)`,
    `MATHS1_2(Op, Main, Sub, SD)`.
    These are textual selectors, not runtime functions: when
    `MATH(...)` resolves to `REGMATH(ADD, ...)`, the Op argument
    becomes a token-paste prefix and `COLOR_##Op` becomes
    `COLOR_ADD`. The level-3 emit picks one of the four selectors
    plus an `ADD` or `SUB` op.

The preamble ends with `#include "tile.c"` (line 660), which
re-enters at level 1.

After that include returns control, the preamble continues with the
runtime dispatcher functions: `S9xSelectTileRenderers_SFXSpeedup`,
`S9xSelectTileRenderers`, and the `S9xSelectTileConverter_DepthN`
helpers. None of those are templated; they reference the dispatch
arrays generated at level 3.


## Level 1 — the renderer families (NAME1)

Lines 994 through 2766. Wrapped in `#ifndef NAME1`. There are
**18** NAME1 families. They fall into four groups.

### 1. Non-Mode-7 tile renderers

These work on the SNES's regular 8×8 tile layers (BG modes 0-6,
sprites, etc). Each loops 8 lines × 8 pixels and calls
`DRAW_PIXEL(n, Pix=...)` for each pixel.

| NAME1 | DRAW_TILE | Notes |
|-------|-----------|-------|
| `DrawTile16`        | unclipped 8×8 walk          | full tile drawn |
| `DrawClippedTile16` | clipped 8×8 walk            | left/right cropping |
| `DrawMosaicPixel16` | walk over a mosaic block    | replicates 1 pixel × N |
| `DrawBackdrop16`    | fill a horizontal span      | 1 colour per scanline |

`Z1` / `Z2` for these are `GFX.Z1` / `GFX.Z2` — set per call by
the rendering pipeline.

`ARGS` differ:
  - `DrawTile16`: `(uint32 Tile, uint32 Offset, uint32 StartLine, uint32 LineCount)`
  - `DrawClippedTile16`: adds `StartPixel, Width`
  - `DrawMosaicPixel16`: same as clipped tile but argument order varies
  - `DrawBackdrop16`: `(uint32 Offset, uint32 Left, uint32 Right)`

`DrawBackdrop16` defines `NO_INTERLACE` — backdrop is always
non-interlaced and always depth 1 (`Z1 = Z2 = 1`, `Pix = 0`). This
suppresses the level-2 expansion of the Interlace and
HiresInterlace NAME2 variants.

### 2. Mode 7 BG1 / BG2 renderers (native)

| NAME1 | DRAW_TILE | Notes |
|-------|-----------|-------|
| `DrawMode7BG1`        | `DRAW_TILE_NORMAL()`      | per-scanline matrix walk |
| `DrawMode7BG2`        | `DRAW_TILE_NORMAL()`      | with EXTBG priority bit |
| `DrawMode7MosaicBG1`  | `DRAW_TILE_MOSAIC()`      | mosaic blocks of BG1 |
| `DrawMode7MosaicBG2`  | `DRAW_TILE_MOSAIC()`      | mosaic blocks of BG2 |

Mode 7 has `NO_INTERLACE = 1` for its whole block (line 1336 to
2765). All four families skip the Interlace and HiresInterlace
NAME2 variants.

`Z1`/`Z2` for these are computed differently per BG:
  - **BG1** uses `Z1 = Z2 = (D + 7)` — flat priority.
  - **BG2** uses `Z1 = Z2 = (D + ((b & 0x80) ? 11 : 3))` — the
    EXTBG per-pixel priority bit. The raw byte is exposed as
    local `b` so the macro can reach into it.

`MASK` and `DCMODE` also differ:
  - **BG1**: `MASK = 0xff`, `DCMODE = (Memory.FillRAM[0x2130] & 1)`.
  - **BG2**: `MASK = 0x7f`, `DCMODE = 0`.

`ARGS` is `(uint32 Left, uint32 Right, int D)` for all Mode 7
families.

### 3. Mode 7 hires renderers (HR / BL / HR4X / BL4X / BL1X)

These are wrapped in `M7HIRES_ONLY` (line 2531-2573 for HR,
2597-2756 for everything else). The `M7HIRES_ONLY` define gates the
level-2 expansion to emit **only the Normal1x1 NAME2 variant**.
The reason: hires Mode 7 renderers handle output indexing
themselves inside their DRAW_TILE body (writing to `2*x`, `4*x`,
or `x`), so they don't need a NAME2 plotter that does width-tier
replication. They pair with the Normal1x1 NAME2 (which is just
`GFX.S[Offset + N] = ...`), and the family-specific DRAW_TILE
takes care of the tiered indexing.

| NAME1 | DRAW_TILE | What it does |
|-------|-----------|--------------|
| `DrawMode7BG1HR`    | `DRAW_TILE_NORMAL_M7HIRES()`             | 2× horizontal, BG1, no BL |
| `DrawMode7BG2HR`    | `DRAW_TILE_NORMAL_M7HIRES()`             | 2× horizontal, BG2 |
| `DrawMode7BG1BL`    | `DRAW_TILE_NORMAL_M7HIRES_BILINEAR()`    | 2× horizontal, BG1, with BL |
| `DrawMode7BG2BL`    | `DRAW_TILE_NORMAL_M7HIRES_BILINEAR()`    | 2× horizontal, BG2 |
| `DrawMode7BG1HR4X`  | `DRAW_TILE_NORMAL_M7HIRES_4X()`          | 4× horizontal, BG1, no BL |
| `DrawMode7BG2HR4X`  | `DRAW_TILE_NORMAL_M7HIRES_4X()`          | 4× horizontal, BG2 |
| `DrawMode7BG1BL4X`  | `DRAW_TILE_NORMAL_M7HIRES_4X_BILINEAR()` | 4× horizontal, BG1, with BL |
| `DrawMode7BG2BL4X`  | `DRAW_TILE_NORMAL_M7HIRES_4X_BILINEAR()` | 4× horizontal, BG2 |
| `DrawMode7BG1BL1X`  | `DRAW_TILE_NORMAL_M7BL_1X()`             | 1× (native), BG1, with BL |
| `DrawMode7BG2BL1X`  | `DRAW_TILE_NORMAL_M7BL_1X()`             | 1× (native), BG2 |

Bilinear filter mode (stable vs smooth) is **not** a separate
NAME1 — it's a runtime branch inside `M7HR_BLEND_AND_WRITE` based
on `Settings.Mode7HiresBilinear == 2`. So one BL renderer body
covers both filter modes.


## Level 2 — the output-width plotters (NAME2)

Lines 2767 through 2911. Wrapped in `#ifndef NAME2`. There are up
to **6** NAME2 variants per NAME1, but most NAME1 families enable
only a subset.

| NAME2 | DRAW_PIXEL writes to | When emitted |
|-------|----------------------|--------------|
| `Normal1x1`      | `Offset + N`                                                 | always |
| `Normal2x1`      | `Offset + 2*N` and `+2*N+1` (replicated)                     | not in M7HIRES_ONLY |
| `Normal4x1`      | `Offset + 4*N..+4*N+3` (replicated)                          | not in M7HIRES_ONLY |
| `Hires`          | `Offset + 2*N` and `+2*N+1` (separate main/sub composition)  | not in M7HIRES_ONLY |
| `Interlace`      | `Offset + 2*N` and `+2*N+1` (replicated)                     | not in M7HIRES_ONLY, not in NO_INTERLACE |
| `HiresInterlace` | `Offset + 2*N` and `+2*N+1` (Hires composition)              | not in M7HIRES_ONLY, not in NO_INTERLACE |

Each NAME2 also redefines `BPSTART` and `PITCH` for the inner
walk:

  - `Normal*` and `Hires`: `BPSTART = StartLine`, `PITCH = 1`
  - `Interlace*`: `BPSTART = StartLine * 2 + BG.InterlaceLine`,
    `PITCH = 2` (every other line from the cached tile).

The 4×1 plotter is unique to the post-`fcaa527` branch and was
added together with the 4× Mode 7 hires renderer; before that
commit, only 2×1 existed.


## Level 3 — math variants and the dispatch array

Lines 2913 through 2984. Each `(NAME1, NAME2)` pair emits 7
function bodies (one per MATH op) plus a 7-entry dispatch array
named `Renderers_NAME1NAME2`.

The 7 MATH operators:

| Index | Macro selector | Op  | Meaning |
|-------|----------------|-----|---------|
| 0     | `NOMATH(x, ...)`              | —    | Color math disabled / not the math layer |
| 1     | `REGMATH(ADD, ...)`           | +    | Add subscreen / fixed colour |
| 2     | `MATHF1_2(ADD, ...)`          | +/2  | Add fixed colour, half intensity |
| 3     | `MATHS1_2(ADD, ...)`          | +/2  | Add subscreen if available else fixed, half intensity |
| 4     | `REGMATH(SUB, ...)`           | −    | Subtract subscreen / fixed colour |
| 5     | `MATHF1_2(SUB, ...)`          | −/2  | Subtract fixed colour, half intensity |
| 6     | `MATHS1_2(SUB, ...)`          | −/2  | Subtract subscreen if available else fixed, half intensity |

The level-3 emit (lines 2920-2967) materialises seven copies of
the same DRAW_TILE body, each with a different `MATH(...)`
expansion. The compiler's optimizer is what makes this efficient:
because the MATH macro folds away into a known operation with
known operands, each variant compiles to a tight loop without any
per-pixel branching on the math op.

After the seven function definitions, line 2969 emits the
dispatch array:

```c
static void (*Renderers_NAME1NAME2[7]) (ARGS) =
{
    NAME1_NAME2,            /* index 0: Nomath */
    NAME1Add_NAME2,         /* index 1: REGMATH ADD */
    NAME1AddF1_2_NAME2,     /* index 2: MATHF1_2 ADD */
    NAME1AddS1_2_NAME2,     /* index 3: MATHS1_2 ADD */
    NAME1Sub_NAME2,         /* index 4: REGMATH SUB */
    NAME1SubF1_2_NAME2,     /* index 5: MATHF1_2 SUB */
    NAME1SubS1_2_NAME2      /* index 6: MATHS1_2 SUB */
};
```

`MAKENAME(A, B, C)` is `A##B##C` — token-pasting via two macros to
work around C's preprocessor not concatenating macro arguments
directly.


## Symbol generation count

Putting it all together. Each NAME1 emits one or more NAME2 variants
and each `(NAME1, NAME2)` pair emits 7 functions + 1 array. The
NAME1 → NAME2 fan-out:

| NAME1                | NAME2 emitted (at compile time) | Function count | Array count |
|----------------------|---------------------------------|----------------|-------------|
| `DrawTile16`         | Normal1x1, Normal2x1, Normal4x1, Hires, Interlace, HiresInterlace | 6 × 7 = 42  | 6 |
| `DrawClippedTile16`  | Normal1x1, Normal2x1, Normal4x1, Hires, Interlace, HiresInterlace | 6 × 7 = 42  | 6 |
| `DrawMosaicPixel16`  | Normal1x1, Normal2x1, Normal4x1, Hires, Interlace, HiresInterlace | 6 × 7 = 42  | 6 |
| `DrawBackdrop16`     | Normal1x1, Normal2x1, Normal4x1, Hires (NO_INTERLACE skips interlace variants) | 4 × 7 = 28 | 4 |
| `DrawMode7BG1`       | Normal1x1, Normal2x1, Normal4x1, Hires (NO_INTERLACE)            | 4 × 7 = 28  | 4 |
| `DrawMode7BG2`       | Normal1x1, Normal2x1, Normal4x1, Hires (NO_INTERLACE)            | 4 × 7 = 28  | 4 |
| `DrawMode7MosaicBG1` | Normal1x1, Normal2x1, Normal4x1, Hires (NO_INTERLACE)            | 4 × 7 = 28  | 4 |
| `DrawMode7MosaicBG2` | Normal1x1, Normal2x1, Normal4x1, Hires (NO_INTERLACE)            | 4 × 7 = 28  | 4 |
| `DrawMode7BG1HR`     | Normal1x1 only (M7HIRES_ONLY)                                    | 7           | 1 |
| `DrawMode7BG2HR`     | Normal1x1 only (M7HIRES_ONLY)                                    | 7           | 1 |
| `DrawMode7BG1BL`     | Normal1x1 only (M7HIRES_ONLY)                                    | 7           | 1 |
| `DrawMode7BG2BL`     | Normal1x1 only (M7HIRES_ONLY)                                    | 7           | 1 |
| `DrawMode7BG1HR4X`   | Normal1x1 only (M7HIRES_ONLY)                                    | 7           | 1 |
| `DrawMode7BG2HR4X`   | Normal1x1 only (M7HIRES_ONLY)                                    | 7           | 1 |
| `DrawMode7BG1BL4X`   | Normal1x1 only (M7HIRES_ONLY)                                    | 7           | 1 |
| `DrawMode7BG2BL4X`   | Normal1x1 only (M7HIRES_ONLY)                                    | 7           | 1 |
| `DrawMode7BG1BL1X`   | Normal1x1 only (M7HIRES_ONLY)                                    | 7           | 1 |
| `DrawMode7BG2BL1X`   | Normal1x1 only (M7HIRES_ONLY)                                    | 7           | 1 |
| **Totals**           |                                                                  | **322**     | **46** |

So **322 functions** in **46 dispatch arrays** at the `.o` level
(verified via `nm` on the per-TU object).

After LTO link, several symbols are dead-code-eliminated because
they're never referenced from `S9xSelectTileRenderers`. Notably:

  - `Renderers_DrawMode7BG{1,2}Normal4x1` — Mode 7 BG at 4x width
    is served exclusively by the M7HR_4X / M7BL_4X variants
    (which write to 4x indices via their own DRAW_TILE bodies);
    the dispatcher never selects the native Mode 7 renderer at
    quad width.
  - `Renderers_DrawMode7MosaicBG{1,2}Normal4x1` — analogous
    reason.

After DCE, the linked `.so` has roughly **44** Renderers_ arrays
visible to `nm`. The dropped ones are not a bug — they are
genuinely unreachable code paths, generated by the level-2
expansion's "emit Normal4x1 unless M7HIRES_ONLY is set" rule but
not selected by any dispatch site. Removing them would require
either gating the level-2 Normal4x1 emit on a more specific flag
or rearranging the level-1 NAME1 instantiations.


## The runtime dispatch path

`S9xSelectTileRenderers(BGMode, sub, obj)` is called at the start
of each scanline (or at hires-promotion mid-frame). It picks one
function per renderer slot:

  1. Determine width tier from `IPPU.QuadWidthPixels` and
     `IPPU.DoubleWidthPixels`. Pick the matching NAME2 family of
     base renderers (`Normal4x1`, `Hires`, `Interlace`,
     `Normal2x1`, or `Normal1x1`).
  2. Wire `DT`, `DCT`, `DMP`, `DB`, `DM7BG1`, `DM7BG2` to point at
     the corresponding 7-entry arrays (e.g.
     `Renderers_DrawTile16Normal2x1`).
  3. If the frame is in BG mode 7 and the user has Mode 7 hires
     and/or bilinear enabled, override `DM7BG1` and `DM7BG2` to
     point at the M7-specific arrays (e.g.
     `Renderers_DrawMode7BG1BL4XNormal1x1`).
  4. Pick an array index `i` in [0..6] from the SNES PPU's
     CGADSUB / CGWSEL register state:
       - bit 7 of $2131 selects sub (4) vs add (1) baseline.
       - bit 6 of $2131 adds 1 (half-blend mode).
       - bit 1 of $2130 adds 1 again (subscreen-source mode).
     Result: i ∈ {0=Nomath, 1=Add, 2=AddF, 3=AddS, 4=Sub, 5=SubF,
     6=SubS}.
  5. Cache `GFX.DrawTileMath = DT[i]`, `GFX.DrawTileNomath = DT[0]`,
     and similar for the other slots.

The actual rendering path then calls `GFX.DrawTileMath(...)` or
`GFX.DrawTileNomath(...)` per tile, with `Math` used for layers
that participate in colour math and `Nomath` for layers that
don't. The `i` index updates whenever the PPU's math registers
are touched, so per-pixel cost is just an indirect call.


## Worked example: `DrawMode7BG1BLAddF1_2_Normal1x1`

This is the function that runs when:
  - Frame is in BG mode 7
  - Mode 7 hires is on at 2× (or off, since this is the 2× variant)
  - Mode 7 bilinear is on
  - Layer being drawn is BG1 (not BG2 EXTBG)
  - Color math is "add fixed colour, half intensity" — CGADSUB has
    bits set for {add, half, math-outside-window-uses-fixed-colour}
  - PPU is not in mosaic mode for BG1
  - Buffer width tier is 2× (so NAME2 is `Normal1x1` for the
    M7HIRES_ONLY family, but the renderer body writes 2× indices
    inside its own loop)

Step-by-step expansion:

1. Level 1 sets:
     `NAME1 = DrawMode7BG1BL`
     `ARGS = (uint32 Left, uint32 Right, int D)`
     `Z1 = Z2 = (D + 7)` (BG1, no priority bit)
     `MASK = 0xff`
     `DCMODE = (Memory.FillRAM[0x2130] & 1)`
     `BG = 0`
     `DRAW_TILE() = DRAW_TILE_NORMAL_M7HIRES_BILINEAR()`
     `M7HIRES_ONLY` is defined.

2. Level 2 (only Normal1x1 because of M7HIRES_ONLY) sets:
     `NAME2 = Normal1x1`
     `BPSTART = StartLine`, `PITCH = 1` (unused for Mode 7)
     `DRAW_PIXEL(N, M) = if (Z1 > GFX.DB[Offset+N] && (M)) {
                          GFX.S[Offset+N] = MATH(GFX.ScreenColors[Pix], ...);
                          GFX.DB[Offset+N] = Z2;
                        }`

3. Level 3 emits seven function bodies. For the `AddF1_2` variant:

   ```c
   static void DrawMode7BG1BLAddF1_2_Normal1x1
       (uint32 Left, uint32 Right, int D)
   {
       /* MATH(A, B, C) = MATHF1_2(ADD, A, B, C)
        *              = (GFX.ClipColors
        *                  ? COLOR_ADD(A, GFX.FixedColour)
        *                  : COLOR_ADD1_2(A, GFX.FixedColour))
        */
       /* DRAW_TILE expands to DRAW_TILE_NORMAL_M7HIRES_BILINEAR()
        * which loops over scanlines inside Left..Right, walks the
        * Mode 7 affine matrix at half-step, and at each output
        * sample calls M7HR_BLEND_AND_WRITE which expands DRAW_PIXEL
        * which invokes MATH(...) above. */
       ...
   }
   ```

4. The dispatch array is emitted:

   ```c
   static void (*Renderers_DrawMode7BG1BLNormal1x1[7]) (uint32, uint32, int) =
   {
       DrawMode7BG1BL_Normal1x1,
       DrawMode7BG1BLAdd_Normal1x1,
       DrawMode7BG1BLAddF1_2_Normal1x1,    /* this one */
       DrawMode7BG1BLAddS1_2_Normal1x1,
       DrawMode7BG1BLSub_Normal1x1,
       DrawMode7BG1BLSubF1_2_Normal1x1,
       DrawMode7BG1BLSubS1_2_Normal1x1,
   };
   ```

5. At runtime, `S9xSelectTileRenderers` notices BG mode 7 + Mode 7
   hires + bilinear, picks `Renderers_DrawMode7BG1BLNormal1x1` as
   the BG1 array, and stores `[2]` (AddF1_2) into
   `GFX.DrawMode7BG1Math` based on the CGADSUB/CGWSEL register
   state.

6. The rendering pipeline invokes `GFX.DrawMode7BG1Math(Left,
   Right, D)` per scanline.


## What this means for de-templating

The macro abstraction is doing real work — it specialises 332
function bodies that share a lot of structure, and the result is
~7 functions that run at native speed because every hot-path
choice (MATH op, output width, BG, math-or-not) was resolved at
compile time.

Replacing the templating with explicit hand-written code means
either:

  - Writing all 332 functions out (massive code volume; tedious;
    must preserve every implicit choice the macros encoded).
  - Collapsing several axes back into runtime choices and trusting
    the optimizer to re-specialise via inlining and cloning. The
    .so size grows but the source shrinks. (LTO will probably do
    most of this for free in practice.)
  - Picking specific axes to keep templated and others to fold.
    For instance: the MATH axis could become a runtime switch
    inside DRAW_PIXEL (cost: 1 branch per pixel; LTO may
    re-specialise per-call-site anyway), while NAME1 and NAME2
    stay specialised because their structure differs more deeply.

Each approach is a different design trade-off and will have to be
evaluated against the audio-bit-identical / valgrind-clean
invariants.


## Glossary

| Term             | Meaning |
|------------------|---------|
| **NAME1**        | Renderer family. 18 of them. Determines what kind of thing is being drawn (sprite tile, backdrop, Mode 7 BG, etc.). |
| **NAME2**        | Output-width / interlace pattern. 6 of them. Determines how each pixel is written into the output buffer. |
| **MATH op**      | Color-math operation. 7 of them. Determines whether the pixel is composited with the subscreen / fixed colour, additively or subtractively, and whether at half intensity. |
| **MASK**         | Palette index mask. `0xff` for BG1 (full 8-bit index), `0x7f` for BG2 (7-bit index, top bit is the EXTBG priority bit). |
| **DCMODE**       | "Direct Colour Mode" enable. SNES feature where the palette index is interpreted as an RGB332 color directly. Reads bit 0 of `$2130` (CGWSEL). |
| **BG**           | Layer index, 0 or 1, used to read the right clip-window data from `GFX.Clip[BG]`. |
| **Z1, Z2**       | Z-buffer values. `Z1 > GFX.DB[N]` is the depth test; on success, `GFX.DB[N] = Z2`. Same for Mode 7 BG1, but for BG2 EXTBG `Z2` varies per pixel based on the priority bit. |
| **DRAW_TILE**    | The per-NAME1 macro that loops over a tile's pixels and calls `DRAW_PIXEL(N, condition)` for each. |
| **DRAW_PIXEL**   | The per-NAME2 macro that takes `(N, condition)` and writes one or more buffer pixels with the result of `MATH(...)`. |
| **MATH**         | The per-MATH-op macro that takes `(Main, Sub, SubDepth)` and returns a `uint16` colour to write. Empty/NOP for `Nomath`. |
| **M7HIRES_ONLY** | Compile-time flag that limits the level-2 NAME2 expansions to just `Normal1x1`. Set inside the Mode 7 hires renderer scaffolds. |
| **NO_INTERLACE** | Compile-time flag that suppresses the level-2 `Interlace` and `HiresInterlace` NAME2 expansions. Set for backdrop and all Mode 7 renderers. |
| **PITCH**        | 1 for non-interlaced rendering, 2 for interlaced (which reads every other line from the cached tile). |
| **BPSTART**      | First byte to read from the cached tile. Includes the interlace-line offset for interlaced rendering. |
| **GFX.S**        | Pointer to the output framebuffer. Pixels are RGB565 `uint16`. |
| **GFX.DB**       | Z-buffer matching GFX.S. |
| **GFX.PPL**      | Pixels per line in the framebuffer (`GFX.RealPPL` doubled if interlaced). |
| **Renderers_NAME1NAME2** | Static array of 7 function pointers, one per MATH op, used by `S9xSelectTileRenderers` for runtime dispatch. |


## Related files

  - `src/tile.h` — public typedefs and prototypes; very small.
  - `src/gfx.h` — the `GFX` and `IPPU` structs.
  - `src/ppu.h` — the `PPU` struct, which has all the Mode 7
    matrix data, mosaic state, and BG-mode register mirrors.
  - `src/cpuexec.c` — `RenderLine()` is the per-scanline driver
    that calls into the dispatched function pointers.
  - `src/ppu.c` — `S9xUpdateScreen()` does mid-frame width
    promotion and the final composite pass.
