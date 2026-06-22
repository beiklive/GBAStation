# tile-untangle persistent state

**Read me first at the start of every session before doing anything else.**

This is the long-running checklist for the de-templating + folding work
on `src/tile.c` in libretro/snes9x2010. The work spans many sessions
and Claude's context window will compact between sessions; this file
is the source of truth for "where are we" and "what's next."

If anything in this file contradicts conversation history or memory,
**this file wins** because it's updated at every commit boundary.

---

## Current state

**Branch:** `master` (the `tile-untangle` work is merged).
**Base:** `7fb5b58` — "Mode 7 hires: 4x horizontal mode, bilinear stable/smooth modes, BL at 1x" (merge-base of the tile-untangle branch with master).
**Status:** Tile-untangle effort merged to master via PR #167 (`66a907a`). Subsequent post-merge work has been: de-unroll, VRAM hoist, plotter-macro consolidation, and BL helper extraction. tile.c is no longer a templating engine: 313 explicit renderer functions, 44 dispatch arrays, all bodies inlined, no self-recursion.
**Last code commit on this effort:** `3752ba0` — tile.c: hoist endpix decl in DrawClippedTile16 for C89/MSVC 2010 compat.
**Working tree:** dirty (this state-file update pending; will ride along with the next code commit on master).

**Recent post-merge commits** (newest first):

| SHA       | Subject                                                  |
|-----------|----------------------------------------------------------|
| `3752ba0` | hoist endpix decl in DrawClippedTile16 for C89 compat    |
| `4ff591a` | extract m7hr_blend helper from M7HR_BLEND_AND_WRITE      |
| `d56a8eb` | consolidate DT/CT/MP_PIXEL pixel-plotter macros          |
| `5acc09d` | hoist Memory.VRAM to a function-entry local in M7        |
| `58f0bd4` | de-unroll switch (StartPixel) in DrawClippedTile16       |
| `beb6437` | apu: collapse Finalize/GetSampleCount/MixSamples         |

**.so size trajectory** (Linux x86-64, `-O2 -flto`):

| Build                                  | .so size        |
|----------------------------------------|-----------------|
| Pristine `7fb5b58` (pre-untangle)      | (not measured)  |
| After de-unroll + VRAM hoist (e3a1550) | 2,317,152 bytes |
| After plotter consolidation (d56a8eb)  | 2,317,152 bytes (no change — byte-identical compiled output) |
| After BL helper refactor (4ff591a)     | 2,239,424 bytes (−77,728 / −3.4%) |

State file lives in-tree at `docs/tile-renderer-state.md` (originally
`tile-untangle-state.md`, renamed once the de-templating effort
finished and the doc became a general tile.c / M7 renderer state
file). It served as a scratchpad during the de-templating effort
(continuity across multiple sessions, intent-of-each-commit doc,
byte-identical anchor record). Now that work is merged the doc
functions as a written record of what was done and why, and as the
in-flight scratchpad for ongoing M7HR / vert-2x / renderer-touch
work.

**All renderer families are de-templated and the dispatch scaffold
is gone.** The Mode 7 group: HR4X (`0cbca9e`), BL4X (`69f75a1`),
BL1X (`0e147ff`), BL (`6fde805`), HR (`624492c`), native BG
(`f5894be`), mosaic + dead-array cleanup (`3c7ad4f`). The non-Mode-7
families: DrawBackdrop16 (`9ac00cb`), DrawMosaicPixel16
(`bb3cacf`), DrawClippedTile16 (`8906302`), DrawTile16 + scaffold
removal (`a40fa37`).

**Commit A (`f5720b6`) inlines all 11 body macros into the
per-function bodies.** Every renderer function now contains its
full body verbatim — no more `TILE_BODY_*(...)` macro invocation
on the second line of each function. The file grew from ~4,900
lines to ~28,600 lines as a result. Plotters, color-math, and
file-scope helpers stay as macros per the user's instruction.

What stays as macros:
- Pixel plotters (`DRAW_PIXEL_*` for tile/clipped/mosaic;
  `BACKDROP_PIXEL_*`; `M7N_PIXEL_*`).
- Color-math (`NOMATH`, `REGMATH`, `MATHF1_2`, `MATHS1_2`,
  `COLOR_ADD*` / `COLOR_SUB*`).
- File-scope helpers (`GET_CACHED_TILE`, `IS_BLANK_TILE`,
  `SELECT_PALETTE`, `H_FLIP`, `V_FLIP`, `CLIP_10_BIT_SIGNED`).
- Mode 7 hires bilinear helpers (`M7HR_LOOKUP_4`,
  `M7HR_BLEND_AND_WRITE`, `M7HR_SAMPLE_BILINEAR`,
  `M7HR_LOOKUP_4_FILL`).

What's gone (compared to `7af3f84`):
- `TILE_BODY_UNCLIPPED`, `TILE_BODY_CLIPPED`,
  `TILE_BODY_MOSAIC_PIXEL`, `TILE_BODY_BACKDROP`.
- `TILE_BODY_NORMAL_M7`, `TILE_BODY_MOSAIC_M7`.
- `TILE_BODY_M7_HR`, `TILE_BODY_M7_HR4X`, `TILE_BODY_M7_BL`,
  `TILE_BODY_M7_BL4X`, `TILE_BODY_M7_BL1X`.

The dead `#ifndef NAME1` wrapper at line 996 (always true since
NAME1 is never defined) is also removed (Stage 3.5). The outer
`#ifndef _NEWTILE_CPP / #else` split and the line-660 self-include
**have now also been removed (`af28f95`)**. `tile.c` is laid out
linearly: includes, then helpers and tile-cache conversion routines
(lines 197-659 in the previous numbering), then the renderers (the
former `#else` arm), then the `S9xSelect*` dispatch helpers (what
used to come after the self-include).

`tile.c` no longer self-includes recursively at any level. The
per-NAME2 BPSTART / PITCH / DRAW_PIXEL macros and the per-(NAME1,
NAME2) 7-fold MAKENAME fan-out are gone (Stage 4). The level-2 /
level-3 `#ifndef NAME2` / `#else / Third-level` blocks are gone
(Stage 3). The line-660 `#include "tile.c"` self-recursion is gone
(`af28f95`). The only top-level preprocessor scaffold that remains
is `#ifdef MSB_FIRST` at ~line 253 selecting between two `pixbit[][]`
lookup tables for big- vs little-endian compiles -- this is real
endianness conditional, not templating.

Optional follow-up (Stage 4) is profile-driven: cross-cutting
plotter consolidation (the `MP_PIXEL_*` / `CT_PIXEL_*` /
`DT_PIXEL_*` plotters are byte-identical and could share a
single `TILE_PIXEL_*` definition across the three sections).
Worth doing only if it pays off in maintainability without
compromising codegen.

The level-1 Mode 7 templating scaffold is gone: file-scope
`Z1`/`Z2`/`MASK`/`DCMODE`/`BG`/`NO_INTERLACE` `#define`s for Mode 7
removed. The `DRAW_TILE_NORMAL` and `DRAW_TILE_MOSAIC` macros are
deleted. The level-2 dispatch in `tile.c` is still alive because
DrawMosaicPixel16, DrawClippedTile16, and DrawTile16 still use it.

## Invariants — must hold at every commit

These are non-negotiable. Verified externally by Lib (the user) before
each commit lands. Claude does *not* fabricate verification; if a hash
or harness wasn't actually run, say so.

- **Audio bit-identical** with all hires/BL options off:
  - `NTSC = 0xe2cacc592d6fc085`
  - `PAL  = 0x4274c83d71cb4b45`
- **Save state compatibility**: a savestate from `7fb5b58` loads
  cleanly on the current commit and produces byte-identical
  subsequent frames (verifies no SRAM/CPU/PPU state shape change).
- **Valgrind clean** on the 9 standing harnesses.
- **Symbol set preserved**: every public symbol that was in
  `7fb5b58` is still in the .so. Internal symbols may change shape
  (LTO will inline), but the dispatch arrays
  (`Renderers_DrawTile16Normal1x1` etc.) must still resolve and
  point at functions with the same observable behavior.
- **7 distinct math variants** per (NAME1, NAME2) combination. Lib
  has a checker for this; if any two collapse to the same code,
  that's a regression.

## Documentation rule — applies at every conversion

Every commit that de-templates a renderer family **also** adds C
comments explaining what the resulting code does. The macro
abstraction was carrying a lot of implicit knowledge (which of
NAME1 / NAME2 / MATH this code is specialized for, what the
fractional-step arithmetic means, why a particular dispatch i-index
maps to a particular math op, what the per-corner opacity bitmap
means, etc). When the macros go, that knowledge has to land
somewhere readable in the C source. It does not get to live only in
git history or in this state file.

Required comment shape per de-templated function:

  /* DrawMode7BG1HR4X_AddF1_2_Normal1x1
   *
   * Mode 7 BG1, 4x horizontal hires, no bilinear, color math is
   * "add fixed colour, half intensity" (CGADSUB add + ½ + math
   * outside window selects fixed colour). Output buffer is at 4x
   * native width; this renderer writes 4 sub-samples per native
   * pixel into GFX.S[Offset + 4*x .. + 4*x+3].
   *
   * Z1/Z2 are constants (D + 7) for BG1 because BG1 priority does
   * not depend on the EXTBG per-pixel priority bit.
   * ...
   */
  static void DrawMode7BG1HR4X_AddF1_2_Normal1x1 (uint32 Left, uint32 Right, int D)
  {
      ...
  }

Required comment shape at the module level (top of file or
section header):

  /* === Mode 7 BG1 hires renderers ===
   *
   * Six closely-related renderer families:
   *   HR  = 2x horizontal, nearest-neighbour
   *   BL  = 2x horizontal, bilinear (stable or smooth at runtime)
   *   HR4X = 4x horizontal, nearest-neighbour
   *   BL4X = 4x horizontal, bilinear
   *   BL1X = native (1x), bilinear (no upsample)
   *
   * Each is dispatched only when (BGMode == 7 && !sub) and the
   * appropriate (Mode7Hires, Mode7HiresBilinear) settings are on;
   * see S9xSelectTileRenderers.
   *
   * Sub-pixel sample positions for 2x/4x: stepping AA, CC by
   * (matrixA / factor), (matrixC / factor) per output column,
   * with the rounding leftover deposited on the final sub-sample
   * to keep the sum-per-native-pixel exactly equal to the
   * matrixA/matrixC the SNES would have advanced.
   *
   * For mid-frame promotion semantics, see ppu.c::S9xUpdateScreen.
   */

Both shapes get added at every conversion commit. If the
explanation is essentially the same as one already nearby (e.g.
BG1 vs BG2 just differ in the priority bit), the second function
can reference the first by name instead of repeating ("Same as
DrawMode7BG1HR4X_AddF1_2_Normal1x1 except Z1/Z2 reflect the
EXTBG per-pixel priority bit; see that function for details").

The whole-file goal at the end of Stage 3: a contributor who has
never read tile.c before should be able to read it top-to-bottom
and understand the dispatch model, the buffer-width tier system,
the math variants, and the Mode 7 sub-pixel arithmetic without
needing this state file or any external doc.

## Stage plan

### Stage 1 — Inventory (no code changes)

Goal: produce a complete, accurate map of the templating in
`src/tile.c` so subsequent stages have a target to convert against.

Single doc commit. The doc lives in `docs/tile-internals.md` (new
directory, new file). Covers:

- The three-level `#include "tile.c"` re-entry scheme.
- Every NAME1 family and what it produces.
- Every NAME2 expansion and what it produces.
- The 7 math variants and what they produce.
- The full table of generated symbols (NAME1 × NAME2 × MATH
  combinations actually produced, after `M7HIRES_ONLY` /
  `NO_INTERLACE` skips).
- The dispatch path: how `S9xSelectTileRenderers` picks an array,
  how the per-tile call site picks an entry.
- The math primitives (`NOMATH`, `REGMATH`, `MATHF1_2`,
  `MATHS1_2`) and `COLOR_ADD` / `COLOR_SUB` / `_1_2` cousins.
- A worked example: trace what
  `DrawMode7BG1BLAddF1_2_Normal1x1` actually expands to.

Stage 1 is the **external** doc. The in-source C comments start
at Stage 2 (per the Documentation rule above). Once Stage 3 is
done and the templating is gone, the external doc can be
shortened to just dispatch / module overview, with the per-renderer
explanations living in the source itself where future contributors
will actually read them.

Status: **TODO**

### Stage 2 — De-macroing renderer bodies

Goal: replace the templating-driven instantiation of each NAME1
family with explicit static (inline) functions.

Order (small/safe first, large/risky last):

1. ~~`DrawMode7BG1HR4X` and `DrawMode7BG2HR4X`~~ — **done** (0cbca9e).
2. ~~`DrawMode7BG1BL4X` and `DrawMode7BG2BL4X`~~ — **done**
   (Stage 2.2a `54c9153` parameterize helpers; Stage 2.2b `69f75a1`
   de-template BL4X). The parameterization commit is shared with
   subsequent BL stages.
3. ~~`DrawMode7BG1BL1X` and `DrawMode7BG2BL1X`~~ — **done**
   (Stage 2.3, `0e147ff`). Same fan-out pattern as BL4X but
   without sub-pixel division. Single commit since helpers were
   already parameterized.
4. ~~`DrawMode7BG1BL` and `DrawMode7BG2BL` (the existing 2x BL)~~
   — **done** (Stage 2.4, `6fde805`). Same fan-out pattern as
   BL4X but with half-step (aa_h = aa/2) sub-pixel division.
   Removed the `DRAW_TILE_NORMAL_M7HIRES_BILINEAR` macro (no
   longer referenced) and the `ARGS / M7HIRES_ONLY` scope
   scaffolding for the BL block (BL was the last user).
5. ~~`DrawMode7BG1HR` and `DrawMode7BG2HR` (the existing 2x HR)~~
   — **done** (Stage 2.5, `624492c`). Same fan-out pattern as
   BL but with inline nearest-neighbour DRAW_PIXEL writes
   (assignment-and-test in single expression matching the
   original macro shape) instead of bilinear blending. Removed
   the `DRAW_TILE_NORMAL_M7HIRES` macro (no longer referenced).
   This was the last user of the level-1 Mode 7 hires templating
   scaffold.
6. ~~`DrawMode7BG1` and `DrawMode7BG2` (native Mode 7)~~ — **done**
   (Stage 2.6a, `f5894be`). Full NAME2 set emitted in 2.6a; dead
   variants dropped in 2.6b/2.7 below.

   Stage 2.6b folded into Stage 2.7 (`3c7ad4f`): drop the
   never-referenced NAME2 variants (Normal4x1, Interlace,
   HiresInterlace) for the de-templated families. Cleans up all
   the unused-Renderers warnings.

7. ~~`DrawMode7MosaicBG1` and `DrawMode7MosaicBG2` (Stage 2.7)~~ —
   **done** (`3c7ad4f`). Mosaic Mode 7 de-templated using a
   `TILE_BODY_MOSAIC_M7` body macro plus `DEFINE_M7_MOSAIC_*`
   fan-out, mirroring the native machinery. Combined with 2.6b
   (drop dead NAME2 variants for both native and mosaic) into
   one commit. The `DRAW_TILE_MOSAIC` macro is gone, and the
   file-scope `Z1`/`Z2`/`MASK`/`DCMODE`/`BG`/`NO_INTERLACE`
   `#define`s for Mode 7 are gone.
8. ~~`DrawBackdrop16` — full NAME2 set.~~ — **done** (Stage 2.8,
   `9ac00cb`). Four NAME2 variants emitted (Normal1x1, Normal2x1,
   Normal4x1, Hires) -- the Interlace and HiresInterlace variants
   the level-2 dispatch normally generates were already excluded
   by the templated form's `NO_INTERLACE` define and the
   de-templated form simply doesn't emit them. Backdrop-specific
   plotters (`BACKDROP_PIXEL_*`) bake in `Z = 1`, `M = 1`,
   `Pix = 0` as literals to match the original DRAW_PIXEL
   expansion exactly.
9. ~~`DrawMosaicPixel16` — full NAME2 set.~~ — **done** (Stage 2.9,
   `bb3cacf`). All six NAME2 variants emitted (Normal1x1,
   Normal2x1, Normal4x1, Hires, Interlace, HiresInterlace) -- this
   was the first family to need the interlace variants and
   therefore the first to bake the per-NAME2 BPSTART expression
   (`StartLine` for non-interlace, `StartLine * 2 + BG.InterlaceLine`
   for interlace) directly into the fan-out. Section-local
   `MP_PIXEL_*` plotters mirror the original DRAW_PIXEL_* shapes
   with `Z = GFX.Z1`/`GFX.Z2` and runtime `Pix`. PITCH is not
   used by mosaic-pixel and therefore not threaded through.
10. ~~`DrawClippedTile16` — full NAME2 set.~~ — **done** (Stage 2.10,
    `8906302`). All six NAME2 variants emitted. First family to
    thread both BPSTART and PITCH (as `BP_STEP_EXPR` = `8 * PITCH`,
    so `8` non-interlace, `16` interlace). Body has 4 flip-case
    branches each with an 8-case fall-through switch -- preserved
    verbatim in `TILE_BODY_CLIPPED`. Section-local `CT_PIXEL_*`
    plotters with the same shape as `MP_PIXEL_*`.
11. ~~`DrawTile16` — full NAME2 set.~~ — **done** (Stage 2.11,
    `a40fa37`). The hot-path renderer for unclipped 8-pixel
    BG tiles. All six NAME2 variants emitted. Same plotter shape
    as DrawMosaicPixel16 / DrawClippedTile16 (`DT_PIXEL_*`,
    Z = GFX.Z1/Z2, runtime Pix). Body simpler than ClippedTile
    (no per-pixel switch over StartPixel, no Width clamp): just
    4 flip cases each with a plain `for (n = 0; n < 8; n++)`
    loop. ARGS shape is the smallest of any tile family
    (Tile, Offset, StartLine, LineCount). Threads BPSTART and
    BP_STEP per NAME2 same as ClippedTile.

    Bundled with Stage 3 (`a40fa37`): once DrawTile16 was
    de-templated, no `#include "tile.c"` self-recursion remained
    inside the `#ifndef NAME1` branch. The level-2/3 dispatch
    scaffold became dead code and was removed.

Each step is one commit. Each commit:

- Touches only the files for that family.
- Builds clean.
- Passes invariants.
- objdump -d shows functionally-equivalent code for the converted
  symbols (verified by Lib externally).
- The dispatch arrays (`Renderers_*`) still resolve correctly.

Status: **TODO** (waiting on Stage 1)

### Stage 3 — Remove the templating scaffold

Goal: delete the `#include "tile.c"` self-recursion, the level
dispatchers, and all the `#define NAME1` / `#define NAME2` /
`#define DRAW_TILE` machinery. With Stage 2 complete nothing uses
them.

**Done** (bundled into the Stage 2.11 commit). The level-2 dispatch
(`#ifndef NAME2`, with per-NAME2 BPSTART / PITCH / DRAW_PIXEL
macros) and the level-3 dispatch (`MAKENAME` 7-fold math fan-out)
have both been deleted. No `#include "tile.c"` self-recursion
remains inside the `#ifndef NAME1` branch. The outer
`#ifndef _NEWTILE_CPP / #else / #ifndef NAME1` wrapper plus the
line-660 self-include remains -- still load-bearing for compile
ordering, but `NAME1` is now never defined and the `#else` branch
of `#ifndef _NEWTILE_CPP` is only reached during the recursive
include. Removing the wrapper is a separate structural cleanup
not bundled here.

Status: **DONE** (`a40fa37`, bundled with Stage 2.11).

### Stage 3.5 — Structural cleanup

Goal: remove the now-dead `#ifndef NAME1` wrapper at `src/tile.c`
line 996. With Stage 3 complete, NAME1 is never defined; the
wrapper's body always runs and the `#else` branch is dead code.

**Done** (`7af3f84`). The `#ifndef NAME1 / #endif` pair has been
collapsed; the family-code block now runs unconditionally on the
recursive include. Replaced with an explanatory banner comment
describing the recursive-include landing-zone purpose. The outer
`#ifndef _NEWTILE_CPP / #else` split and the line-660 self-include
remain; they're still load-bearing for separating "first include"
code (helpers, ConvertTile_*, etc.) from "recursive include"
code (family bodies).

Bundled with Stage 4.

Status: **DONE** (`7af3f84`, bundled with Stage 4).

### Stage 4 — Unroll function-emitting fan-out macros

Goal: convert all `DEFINE_*_FN` / `DEFINE_*_NAME2` / `DEFINE_*_BG`
/ `DEFINE_*_FAMILY` macros (which emit function definitions through
token-pasting) into explicit `static void NAME(args) TILE_BODY_*(...)`
forms. The user's stated motivation: "It will allow us to optimize
the code later better for all the various modes" -- per-function
optimization opportunities (different inlining hints, different
attributes on hot vs cold variants, function-level diff diagnostics)
are easier to apply when each function is a separate textual unit
instead of one row in a fan-out macro.

**Done** (`7af3f84`). All 313 renderer functions are now written
explicitly. No macros emit function definitions. The 11 fan-out
macros that previously did so are gone:

  - `DEFINE_DT_FN` / `DEFINE_DT_NAME2` (DrawTile16)
  - `DEFINE_CT_FN` / `DEFINE_CT_NAME2` (DrawClippedTile16)
  - `DEFINE_MP_FN` / `DEFINE_MP_NAME2` (DrawMosaicPixel16)
  - `DEFINE_BACKDROP_FN` / `DEFINE_BACKDROP_NAME2` (DrawBackdrop16)
  - `DEFINE_M7_FN` / `DEFINE_M7_NAME2` / `DEFINE_M7_BG` (native M7)
  - `DEFINE_M7_MOSAIC_FN` / `DEFINE_M7_MOSAIC_NAME2`
    / `DEFINE_M7_MOSAIC_BG` (mosaic M7)
  - `DEFINE_M7_HR_FAMILY` (M7 HR2X)
  - `DEFINE_M7_HR4X_FAMILY` (M7 HR4X)
  - `DEFINE_M7_BL_FAMILY` (M7 BL2X)
  - `DEFINE_M7_BL4X_FAMILY` (M7 BL4X)
  - `DEFINE_M7_BL1X_FAMILY` (M7 BL1X)

The 5 M7 hires families (HR/HR4X/BL/BL4X/BL1X) had the function
definition inside their `DEFINE_*_FAMILY` macro body. For each
of those, the macro was converted from a function-emitter to a
body-only `TILE_BODY_M7_<X>` macro: the `static void
BG_NAME##suffix##Normal1x1 (...)` line was removed from the macro
body, the macro's parameter list was reduced (suffix and BG_NAME
moved to the call site), and each invocation was rewritten as
explicit `static void NAME(args) TILE_BODY_M7_<X>(...)`.

What stays as macros (per the user's instruction):
  - All body macros: `TILE_BODY_NORMAL_M7`, `TILE_BODY_MOSAIC_M7`,
    `TILE_BODY_CLIPPED`, `TILE_BODY_UNCLIPPED`, `TILE_BODY_BACKDROP`,
    `TILE_BODY_MOSAIC_PIXEL`, `TILE_BODY_M7_HR`, `TILE_BODY_M7_HR4X`,
    `TILE_BODY_M7_BL`, `TILE_BODY_M7_BL4X`, `TILE_BODY_M7_BL1X`.
  - All pixel plotters: `DT_PIXEL_*`, `CT_PIXEL_*`, `MP_PIXEL_*`,
    `BACKDROP_PIXEL_*`, `M7N_PIXEL_*`, `M7HR_LOOKUP_4`,
    `M7HR_BLEND_AND_WRITE`, `M7HR_SAMPLE_BILINEAR`.
  - All color-math: `NOMATH`, `REGMATH`, `MATHF1_2`, `MATHS1_2`,
    `COLOR_ADD*`, `COLOR_SUB*`.
  - All file-scope helpers: `GET_CACHED_TILE`, `IS_BLANK_TILE`,
    `SELECT_PALETTE`, `H_FLIP`, `V_FLIP`, `CLIP_10_BIT_SIGNED`.

Symbol verification (vs pristine `a40fa37`):
  - 313 Draw* functions; full per-function size diff is empty
    across all 313 (verified). True pure refactor.
  - 44 Renderers_ arrays; full diff empty.
  - Build clean, zero warnings.

Files changed: `src/tile.c` +1383 / -400 (+983 net). The unrolled
form is significantly larger than the macro-driven form -- 313
function definitions written by hand take more text than 11 fan-out
macros. The trade-off the user accepted: more lines now, but
per-function flexibility for future optimization.

Comments: each section has a banner comment describing what it
does, and each NAME2 group within a section has a banner like
`/* DrawTile16 NAME2 = Normal1x1: 7 math variants. */` to make
it easy to find the function block for a given variant. Section
banners and section-end markers preserve the original structural
documentation.

Status: **DONE** (`7af3f84`).

### Stage 5 — Folding optimizations (in progress)

Goal: with all functions explicit, look for per-mode optimizations
that the macro abstraction was hiding. Candidates (each its own
commit with profile data):

**Done:**

- ✅ De-unroll `switch (StartPixel)` in `DrawClippedTile16` renderers
  (`58f0bd4`) — −80% on ClippedTile family, −32% on heavy variants.
- ✅ Hoist `Memory.VRAM` to function-entry local in M7 renderers
  (`5acc09d`) — alias-pessimism break, x86 noise-floor on bench
  but cleaner asm; expected non-trivial on weak hardware.
- ✅ Consolidate `DT/CT/MP_PIXEL_*` plotter macros into `DRAW_PIXEL_*`
  (`d56a8eb`) — byte-identical compiled output, source-level cleanup
  only, removes drift risk for future plotter edits.
- ✅ Extract `m7hr_blend()` helper from `M7HR_BLEND_AND_WRITE` macro
  (`4ff591a`) — −20% on M7-BL family (78 KB), bench within noise.

**Open (from latest tile.c audit; numbered by descending impact):**

1. **`MATHF1_2` / `MATHS1_2` re-test `GFX.ClipColors` per pixel.** Could
   specialise at function entry (one-time branch on `ClipColors`
   inside each affected math variant). Estimate ~30-40% smaller per
   function for the 4 affected math families. Modest runtime gain.
   Bounded scope (~50 functions × 2 paths each).

2. **GFX field reload pattern** (`Z1`, `DB`, `S`, `ScreenColors` reloaded
   per pixel due to alias pessimism with writes through `GFX.S`).
   Same class as the M7-VRAM hoist that already showed x86-noise.
   Could be addressed surgically per-renderer or via `restrict`
   qualifiers on the GFX struct fields themselves. NOT recommended
   without weak-hardware testing path — bench impact on x86 is
   below detectable.

3. **GFX struct cache layout reorder.** `SubScreen` lives at offset
   0x3ff0 while the rest of the hot pointer fields are 0x4000+; per
   pixel, math paths touch two cache lines instead of one. One-line
   ppu.h change, no API break, but needs save-state binary-compat
   audit if SGFX is ever serialised.

4. **VRAM-write tile-cache invalidation in ppu.c.** 11 invalidation
   writes per VRAM byte across scattered `IPPU.TileCached[*]` arrays.
   Could be tightened. **Out of scope for tile.c** — flagged here
   for visibility.

Status: **In progress** — items 1–4 still open.

### Stage 5-pre — Earlier "future work" (now mostly done or
                    superseded)

Original ideas from before the latest audit, kept here for history:

- Hoist `Settings.Mode7HiresBilinear == 2` from per-pixel to per-frame
  in M7BL renderers — **done implicitly** as part of the BL refactor;
  `smooth` is now a function-entry local in each BL function (was
  already hoisted but the relationship to the helper is clearer).
- Hoist Mode 7 matrix recompute that's invariant within a tile —
  not yet evaluated; matrix recompute is per-line not per-tile in
  the current code, so the hoist target isn't well-defined.
- Eliminate redundant Z-tests across sub-pixel positions in 4x
  / 2x plotters — not yet evaluated.
- Mosaic per-block hoist of pixel-replicate — not yet evaluated.
- Consolidate `MP_PIXEL_*` / `CT_PIXEL_*` / `DT_PIXEL_*` plotter
  shapes — **done** (`d56a8eb`, consolidated into `DRAW_PIXEL_*`).

## What to do at the start of each session

1. Read this file top-to-bottom.
2. `git status` and `git log --oneline -5` on tile-untangle to
   confirm where the branch actually is.
3. If the branch is ahead of what this file says, **update this
   file** to reflect the actual state before doing any new work.
   The file lags reality if Lib pushed commits between sessions.
4. If a stage was in flight, look at the in-flight notes in this
   file and resume from there.
5. If no work is in flight, pick the next TODO from the stage
   plan above.

## What to do at the end of each session

1. Update **Current state** at the top of this file.
2. If a commit was made: add an entry to **Commit log** below
   with the SHA, the family/feature, and the invariant
   verification status.
3. If work is in flight (e.g. a partial conversion): add an
   **In-flight notes** section describing exactly what was
   done, what file is in what state, and what comes next.
4. Save the file via the create_file or str_replace tool.

## Commit log (newest first)

### `3752ba0` — tile.c: hoist endpix decl in DrawClippedTile16 for C89/MSVC 2010 compat

The de-unroll commit (`58f0bd4`) introduced

```c
SELECT_PALETTE();
uint32 endpix = StartPixel + Width;     /* <-- mid-block decl */
if (endpix > 8) endpix = 8;
```

in each of the 42 `DrawClippedTile16` functions. C89 (which MSVC 2010
enforces) requires all declarations at the top of a block, before any
executable statements. `SELECT_PALETTE()` expands to executable
statements, so `uint32 endpix = ...;` after it is a C99-only construct
and triggers MSVC error C2275 "illegal use of this type as an
expression."

GCC accepts it without warning by default, which is why this slipped
past the Linux x86-64 build; `gcc -Wdeclaration-after-statement` flags
it. Reported via libretro CI's MSVC 2010 build (errors C2275, C2146,
C2065 cascading from line 3393, 3447, 3501, … — one per ClippedTile
function).

Fixed by hoisting the declaration into the function-top decl block;
the assignment stays where it was. Purely syntactic move, semantic
behaviour unchanged.

42 functions touched. Linux x86-64 `.so` size 2,239,424 (unchanged
from `4ff591a`). Bench-harness checksum 3447889920 unchanged.

Standalone variant of this fix (applies directly against
`e3a1550`/`5acc09d` without requiring the consolidation/refactor
commits) saved as `0001-tile-c89-fix-standalone.patch`. Useful if the
fix needs to ship to upstream urgently before the other commits are
ready.

### `4ff591a` — tile.c: extract m7hr_blend helper from M7HR_BLEND_AND_WRITE macro

The 118-line `M7HR_BLEND_AND_WRITE` macro was inline-expanded ~196
times across the BL renderer family (42 plain BL + 28 BL1X + 28 BL4X
functions, 4–6 expansions each). M7-BL was the largest single source
of compiled-text bulk in tile.c at 389 KB / 46% of all `Draw*` code.

The macro's compute work — `op_mask` handling, same-index early-outs,
weight renormalisation, smooth/stable filter selection, final RGB
blend — doesn't depend on the math/Z/write parameters. Only the
trailing `if (z1 > DB) { S = math(...); DB = z2; }` tail does.
Extracted the compute into a `static inline` helper `m7hr_blend()`
that takes `(p_tl, p_tr, p_bl, p_br, Xf, Yf, smooth)` and returns
`1` + `*out = blended` or `0` (skip pixel). The macro reduces to a
~10-line wrapper around the helper plus the Z-test/math/write tail.

Helper marked `__attribute__((always_inline))`. Without it, GCC's
heuristics produce an out-of-line function and a per-pixel `call`
per BL invocation — measurable bench regression (~+10% on BL fast
paths) since the call overhead doesn't amortize across the relatively
cheap per-pixel work. With `always_inline`, codegen is equivalent to
the prior inline form but the helper's clean by-value parameter
binding gives the compiler better register allocation and visibly
shorter compiled bodies.

**Results:**

| Metric          | Before    | After     | Delta              |
|-----------------|-----------|-----------|--------------------|
| `.so` total     | 2,317,152 | 2,239,424 | −77,728 / −3.4%    |
| M7-BL family    |   388,921 |   310,628 | −78,293 / −20.1%   |
| Largest BL fn   |    15,848 |    11,936 | −3,912 / −24.7%    |

Only the 42 BL functions changed size; no other function affected.

**Verification:**

- 313 `Draw*` functions and 44 `Renderers_*` dispatch arrays preserved.
- Build clean, zero warnings.
- Extended `bench.c` to cover BL stable / smooth / S1_2 paths.
- Bench-harness checksum **3447889920** unchanged across all ten
  exercised cases. Behaviour bit-identical for tested inputs.
- Bench timings on Ryzen 9 9950X3D: stable ~0% (noise), smooth +1.6%,
  S1_2 smooth +0.7%. All within bench noise floor.

**Caveat:** The icache pressure relief from a 78 KB smaller M7-BL
family is the main motivation. On weak hardware (embedded ARM,
low-power x86, retro handhelds), L1i is much tighter than on x86
dev boxes, so the win there should exceed the bench numbers above.
Unmeasured.

src/tile.c −19 lines net (107 ins / 126 del); helper occupies ~110
lines of new code, displacing ~130 lines of macro body.

### `d56a8eb` — tile.c: consolidate DT/CT/MP_PIXEL pixel-plotter macros into DRAW_PIXEL_*

Three macro families had byte-identical bodies:

```
DT_PIXEL_{N1x1,N2x1,N4x1,H2x1}   -- DrawTile16 family
CT_PIXEL_{N1x1,N2x1,N4x1,H2x1}   -- DrawClippedTile16 family
MP_PIXEL_{N1x1,N2x1,N4x1,H2x1}   -- DrawMosaicPixel family
```

These were section-local renames introduced during the de-templating
work, when each section wanted its own macro storage and the locality
invariant was that no macro crossed section boundaries. The locality
justification has lapsed now that all three sections share a single
`.c` file — three byte-identical copies just means future plotter
edits have to be replicated three times, with drift risk.

Consolidated into a single `DRAW_PIXEL_*` family used file-wide:

- 8 redundant macro definitions removed (`CT_PIXEL_*`, `MP_PIXEL_*`).
- 4 surviving `DT_PIXEL_*` definitions renamed to `DRAW_PIXEL_*`.
- 378 invocation sites mechanically updated.
- 12 stale `#undef` lines removed (they enforced the section-local
  naming and would now actively break the later sections).
- 3 stale comment-block headers updated.

**Verification:**

- 313 `Draw*` functions and 44 `Renderers_*` arrays preserved.
- Build clean, zero warnings.
- `.so` size **unchanged**: 2,317,152 bytes. All 313 `Draw*` functions
  have identical compiled sizes pre and post.
- Disassembly diff: 4 lines (just the binary path in the objdump
  header). **Code is BYTE-IDENTICAL to baseline** — LTO was already
  inlining all three macro families to the same code, so the
  consolidation is purely a source-level cleanup.
- Bench-harness checksum 3447021568 unchanged across all seven
  exercised cases (pre BL-bench-extension).

src/tile.c −105 lines net (386 ins / 491 del).

### Hoist `Memory.VRAM` to a function-entry local in all M7 renderers (this commit)

Each of the 154 `DrawMode7*` renderer functions previously read
`Memory.VRAM` directly inside the per-pixel inner loop:

```c
uint8 *VRAM1 = Memory.VRAM + 1;  /* hoisted */
...
for (x = Left; x < Right; x++) {
    ...
    /* But this still re-reads Memory.VRAM every pixel: */
    TileData = VRAM1 + (Memory.VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7);
    ...
}
```

`Memory.VRAM` is a `uint8 *` field inside the global `Memory` struct.
The compiler can't prove that writes through `GFX.S[i] = ...`,
`GFX.DB[i] = ...`, etc. don't modify the storage of that pointer
field itself (alias pessimism), so it reloads `Memory.VRAM` every
iteration. The pre-existing `VRAM1 = Memory.VRAM + 1` hoist had no
effect on this -- the compiler still treats the bare `Memory.VRAM[...]`
read as a fresh global access.

Hoisting to a stack local breaks the alias chain -- locals can't be
modified by global writes -- so the compiler keeps `VRAM` in a
register for the whole function:

```c
uint8 *VRAM  = Memory.VRAM;
uint8 *VRAM1 = VRAM + 1;
...
for (x = Left; x < Right; x++) {
    TileData = VRAM1 + (VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7);
    ...
}
```

In `DrawMode7BG2_Normal1x1`, this drops Memory.VRAM RIP-relative
loads from 3 (one in prelude, two in inner loops) to 1 (prelude
only). Same pattern across all 154 M7 functions.

The 3 remaining `Memory.VRAM[...]` references in the M7HR_LOOKUP_*
macros (used by BL renderers) are NOT touched by this commit; the
macros take `vram1` as a parameter, so addressing those would
require adding a `vram` parameter alongside it and updating all
~120 macro call sites. BL renderers are an off-by-default cosmetic
path so deferring that is fine.

Conversion was scripted (out-of-tree Python). The 154 declarations
of `uint8 *VRAM1 = Memory.VRAM + 1;` were each expanded to:

    uint8 *VRAM  = Memory.VRAM;
    uint8 *VRAM1 = VRAM + 1;

Then 252 occurrences of `Memory.VRAM[` outside the macro region
(7478..7615) were replaced with `VRAM[`. The 7 `&Memory.VRAM[...]`
references in tile-conversion functions at the top of the file are
unaffected.

Verification:
  - 313 Draw* functions, 44 Renderers_ arrays preserved.
  - Build clean, zero warnings.
  - Bench harness checksum `3447021568` unchanged after extending
    the harness to cover three M7 cases (DrawMode7BG2_Normal1x1,
    DrawMode7BG2Add_Normal1x1, DrawMode7BG2AddS1_2_Normal1x1).
    Behaviour bit-identical for tested inputs.
  - Total M7 compiled bytes: 603,818 -> 603,939 (+121, +0.02%).
    The .so size is unchanged (alignment padding absorbs the
    per-function +0..16 byte deltas).
  - Per-function inner-loop Memory.VRAM RIP-loads in
    DrawMode7BG2_Normal1x1: 3 -> 1.

Runtime impact (x86, alternating-binary 20-trial bench):

    DrawMode7BG2_Normal1x1            -0.8%   (pristine 0.4030 -> 0.3997)
    DrawMode7BG2Add_Normal1x1         +0.4%   (within noise)
    DrawMode7BG2AddS1_2_Normal1x1     -1.3%   (pristine 0.4141 -> 0.4085)
    DrawTile16* / DrawClippedTile16*  ~unchanged (not modified)

The runtime delta on x86 is within bench noise (±1%). Modern x86
handles repeated RIP-relative loads of stable globals very cheaply
-- the line gets pinned in L1d, the load latency overlaps with
surrounding ALU work via out-of-order execution, the branch
predictor doesn't care about the addressing mode. So eliminating
the reload doesn't show up as a measurable speedup on a fast x86
bench machine.

The change is committed anyway because:
  1. The asm IS measurably cleaner (3 -> 1 reloads per call), and
     the standard "hoist invariant pointer loads out of inner loops"
     pattern is one the compiler can't apply itself due to alias
     rules.
  2. RetroArch's actual deployment targets (embedded ARM, low-power
     x86, retro handhelds) have smaller L1d, weaker branch
     predictors, and less aggressive out-of-order execution. The
     reload that's free on a dev box may cost cycles there. (This
     is unmeasured -- caveat noted explicitly.)
  3. The change is mechanical, narrow, and behaviour-preserving;
     bench checksum is bit-identical. Cost is +121 bytes total, no
     .so size change.
  4. Future optimization passes (e.g. adding `restrict` qualifiers,
     attempting auto-vectorization) work better against code that
     doesn't fight the alias analyser.

Behaviour invariants -- to be verified externally before push:
audio bit-identical, savestate compat, valgrind clean. The
transformation is a pure local-binding rewrite of equivalent
semantics, so all three should hold.

Files changed: `src/tile.c` +154 lines / -154 lines (declaration
expansion is +1 line, substitution is in-place); approximately a
no-op in line count.

### `58f0bd4` -- tile.c: de-unroll switch (StartPixel) in DrawClippedTile16 renderers

The 42 `DrawClippedTile16*` functions each had four H_FLIP/V_FLIP
branches, each containing an unrolled 8-case switch with fall-through:

```c
w = Width;
switch (StartPixel)
{
    case 0: CT_PIXEL_FAM(0, Pix = bp[0], MATH_SEL, MATH_OP) if (!--w) break;
    case 1: CT_PIXEL_FAM(1, Pix = bp[1], MATH_SEL, MATH_OP) if (!--w) break;
    ...
    case 7: CT_PIXEL_FAM(7, Pix = bp[7], MATH_SEL, MATH_OP) break;
}
```

`CT_PIXEL_FAM` is the per-family plotter macro (CT_PIXEL_N1x1,
N2x1, N4x1, or H2x1). Each macro expansion is large -- per-pixel
Z-test + math + write, sometimes two writes for H2x1 -- and inlining
it 8 times in 4 branches × 42 functions accounted for most of
ClippedTile's compiled bulk (~500 KB).

Replaced with a compact for-loop:

```c
{
    uint32 i;
    for (i = StartPixel; i < endpix; i++)
        CT_PIXEL_FAM(i, Pix = bp[i], MATH_SEL, MATH_OP)
}
```

(or `bp[7 - i]` for the V_FLIP variants). `endpix = StartPixel + Width`
is hoisted to a single function-level local with a defensive clamp
`if (endpix > 8) endpix = 8;` -- the original switch had an implicit
hard stop at case 7 even if Width was nominally larger; the explicit
clamp preserves that behavior. All actual call sites in ppu.c bound
`StartPixel + Width <= 8` already, so the clamp is dead code in
practice but cheap (~17 bytes per function) and protects against
future call sites.

Conversion was scripted (out-of-tree Python) handling 168 switch
blocks (4 branches × 42 functions) and 42 function preludes in one
pass.

Verification:
  - 313 Draw* functions, 44 Renderers_ arrays preserved.
  - Build clean, zero warnings.
  - Bench harness (`/home/claude/bench/bench.c`) checksum
    `3446104064` matches across all four exercised cases. Behaviour
    bit-identical for the tested inputs.
  - ClippedTile total compiled size: 498,259 -> 97,553 bytes
    (-80.4%). Largest single ClippedTile function: 23,859 bytes ->
    ~2,300 bytes (-90%).
  - Bench timings:
        DrawClippedTile16AddS1_2_Normal1x1: 0.191 -> 0.129 (-32%)
        DrawClippedTile16Add_Normal1x1:     ~unchanged
    The big-function speedup is dominated by L1i cache pressure
    relief; the small ones see no change because they fit in icache
    in either form.

Behaviour invariants -- to be verified externally before push:
audio bit-identical, savestate compat, valgrind clean. The
transformation is purely a control-flow rewrite of equivalent
semantics, so all three should hold.

Files changed: `src/tile.c` ~-1500 lines net (the unrolled cases
collapse heavily).

### `4518a6a` -- tile.c: fold first-assignments into variable decls (merged upstream as PR follow-up)

Cleanup pass that converts patterns of the form

```c
T name1, name2, name3;
name1 = expr;
name2 = expr;
```

into

```c
T name1 = expr;
T name2 = expr;
T name3;
```

i.e. fold the first assignment of each variable up into its declaration
and leave names that are conditionally set (in branches) or set later
in the function as bare decls.

Sites folded:
  - `COLOR_SUB`: `mC1`, `mC2` → init-decls. `v` was already
    init-declared; its decl line gets split out alongside the new
    `mC1`/`mC2` init-decls.
  - `S9xSelectTileRenderers`: `M7M1`, `M7M2`, `interlace`, `hires`
    → init-decls. `int i;` stays bare (set inside branches).
  - 70 native + mosaic Mode 7 functions: `int32 HOffset, VOffset,
    CentreX, CentreY;` decl block → 4 separate `int32 X = ...;`
    init-decls. `uint8 Pix, starty;` decl → `uint8 Pix; uint8
    starty = Line + 1;`. `Pix` stays bare since it's set inside
    inner-loop pixel writes.
  - 42 Mode 7 hires functions (HR / HR4X / BL / BL4X / BL1X): same
    int32 fold + `uint8 starty;` solo → `uint8 starty = Line + 1;`.

Sites intentionally NOT folded:
  - 42 mosaic Mode 7 functions. Their preamble has an extra
    `if (Line + VMosaic > GFX.EndY) VMosaic = ...;` clamp between
    the decl block and the four `HOffset = ...;` assignments.
    Folding the assignments into the decls reorders the code
    textually -- semantically equivalent (the clamp doesn't read
    HOffset and the int32 inits don't read VMosaic), but the
    compiler ends up laying out stack frames slightly differently
    and emits ~+563 bytes of mov-instruction drift across the 42
    affected functions. Since this is meant to be a pure cleanup
    commit, we leave these alone. They could be folded in a
    follow-up if/when the codegen drift is acceptable.
  - The renderer-function preambles `uint8 *pCache, *bp, Pix, n;`.
    `bp` is conditionally set in 4 H_FLIP/V_FLIP branches; folding
    one branch's assignment up to the decl would muddle the
    structure. `Pix` is set inside plotter macros, `n` is a for-
    loop counter, `pCache` is set inside a `GET_CACHED_TILE()`
    macro. None are foldable in a clean way.
  - The `int AA, BB, CC, DD, xx, yy;` decl in M7 functions. `BB`,
    `DD`, `xx`, `yy` are foldable in linear flow, but `AA` and
    `CC` are set in HFlip branches. Splitting the decl into
    `int AA, CC;` plus separate init-decls for the others would
    work but is a more invasive rewrite. Left for a follow-up.

How:
  - Two manual edits for `COLOR_SUB` and `S9xSelectTileRenderers`.
  - A Python script (`/home/claude/fold_m7_preamble.py`) handles the
    M7 preamble patterns: 6 named patterns total, 4 of which fire
    (Patterns 1, 2, 5, 6); 2 are intentionally skipped (Patterns 3,
    4 -- the mosaic variants).
  - A trailing cleanup pass collapses runs of 3+ consecutive blank
    lines to 2, and double-blanks left behind by the BL-family fold
    to single blanks.

Verification (vs pristine `66a907a`):
  - 313 Draw* functions: full per-function size diff is empty across
    all 313. Pure refactor.
  - 44 Renderers_ arrays: full diff empty.
  - Build clean, zero warnings.

Files changed: `src/tile.c` -228 lines net (638 insertions, 866
deletions in git's view, but most of the delta is line-shape
rather than removal -- multi-decl + assignment-line pairs become
single init-decl lines, so each fold removes ~1 line).

Behaviour invariants -- to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean.
This commit is a pure refactor with byte-identical codegen, so
all three should hold trivially.

### `66a907a` — Merge pull request #167 from libretro/tile-untangle

### `af28f95` — tile.c: remove self-include and reorder helpers/renderers/dispatch linearly

Removes the `#include "tile.c"` self-recursion at line 660 of the
previous file structure, and reorders the file so that helpers,
renderers, and dispatch functions are laid out linearly.

Old structure used the self-include as a mechanism to split the file
into two halves with different scope:
```
#ifndef _NEWTILE_CPP            <-- wraps whole file
#define _NEWTILE_CPP
[lines 191-659: helpers + tile-cache conversion routines]
#include "tile.c"                <-- recursive: enters #else arm
[lines 661-993: dispatch funcs S9xSelectTileRenderers et al]
#else                            <-- second-include arm
[lines 996-28170: 313 renderer functions + 44 dispatch arrays]
#endif
```
The dispatch functions reference `Renderers_*` arrays defined in the
renderer section, so they had to come after the recursive include.
The recursive include was the mechanism to inject the renderer
section ahead of the dispatch functions in spite of textual order.

New structure is straightforward:
```
[license, comments, #includes]
[helpers + tile-cache conversion routines]
[313 renderer functions + 44 dispatch arrays]
[dispatch funcs S9xSelectTileRenderers et al]
```
Same code, same dependency order, just laid out top-to-bottom without
the self-recursion. The `#ifndef _NEWTILE_CPP / #define / #else /
#endif` wrapper directives are removed (they only existed to make the
self-include work).

How:
  - A Python script (`/home/claude/snes9x2010/de_self_include.py`)
    parses the file boundaries by content match, slices the file into
    four pieces (prologue, helpers, dispatch, renderers), and
    re-glues them as: prologue + helpers + renderers + dispatch.
  - The four wrapper directive lines and the `#include "tile.c"`
    line are dropped.
  - The "Top-level compilation." comment in the prologue is replaced
    with a short note about the linear structure.
  - The "First-level include: Get all the renderers." preamble that
    introduced the recursive include is dropped.
  - The "Recursive-include landing zone" banner that introduced the
    `#else` arm is replaced with a brief banner about the renderer
    family code.
  - A new "Dispatch helpers" banner is inserted between the renderers
    and the dispatch functions to mark the transition.

What stays:
  - The inner `#ifdef MSB_FIRST` at ~line 253 selecting between two
    `pixbit[][]` lookup tables for big- vs little-endian compiles.
    This is a genuine endianness conditional, not templating.
  - All function definitions, dispatch arrays, plotter macros,
    color-math macros, and file-scope helpers are untouched in their
    bodies. The change is purely textual rearrangement of top-level
    blocks plus removal of the wrapper directives.

Effect:
  - File is laid out linearly. `tile.c` can be sampled with `perf
    record`, walked top-to-bottom in an editor, and indexed by tools
    without the preprocessor playing tricks.
  - Synthetic benchmarks via `bench.c` that include `src/tile.c`
    directly now work correctly. The previous structure caused the
    self-include to resolve back to the original file via -I path,
    breaking any attempt to A/B-test modifications.
  - tile.c line count: 28,171 -> 28,161 (-10 lines net). Most of the
    delta is the four wrapper directives + a trimmed comment block;
    the bulk of the file is unchanged content reordered.

Compile-time impact (measured locally, 5 runs each, single-thread):
  - Pristine `9c27ab2`: 10.33 +/- 0.10 s (mean +/- std)
  - This commit:        10.10 +/- 0.15 s
  - Net: -0.2 s (~-2.2%). Real but modest. The preprocessor was
    already fast at scanning the skipped #ifndef block; most of the
    compile cost is parsing+codegen of the renderer body, which
    happens once either way. Bigger structural value than perf value.

Symbol verification (vs pristine `9c27ab2`):
  - 313 Draw* functions; full per-function size diff is empty across
    all 313 (verified). True pure refactor.
  - 44 Renderers_ arrays; full diff empty.
  - Build clean, zero warnings.

Files changed: `src/tile.c` -10 lines net (1540 lines moved due to
the cut-and-paste reorder; +765 / -775 in git's view).

Behaviour invariants -- to be verified externally by Lib before push:
audio bit-identical (should hold; this is a textual reorder, no
codegen change), savestate compat, valgrind clean.

### `9c27ab2` — tile.c: delete BG2 if(0) dead branches; hoist Mode7HiresBilinear from per-pixel to per-call

Two related cleanups from the post-commit-A optimization list, bundled
because they're independently small and both target Mode 7 hot paths.

**Item 3 (BG2 if(0) deletion):** the body-inlining in commit A
(`f5720b6`) substituted `DC_EXPR = 0` into BG2 Mode 7 functions,
producing 77 instances of dead `if (0) { ... }` blocks (one per BG2
native + mosaic + HR/HR4X/BL/BL4X/BL1X function). The compiler was
already deleting these blocks; `9c27ab2` deletes them at the
source level. -462 lines, byte-identical machine code.

The deleted block in each BG2 function:
```
    if (0)
    {
        if (IPPU.DirectColourMapsNeedRebuild)
            S9xBuildDirectColourMaps();
        GFX.RealScreenColors = DirectColourMaps[0];
    }
```
This block is the inlined Direct Colour Mode handling for BG1 (which
has `DC_EXPR = (Memory.FillRAM[0x2130] & 1)`), but for BG2 the
`DC_EXPR` is the literal `0` so the conditional and its body are
dead.

**Item 2 (Mode7HiresBilinear hoist):** `Settings.Mode7HiresBilinear`
is a frame-constant setting (set from the libretro environment-variable
callback on init / option-change), but the previous code read it
once per pixel inside `M7HR_BLEND_AND_WRITE`. This commit threads it
through the macro chain as a parameter, computed once per renderer
function call (which itself is once per scanline group via a
function-pointer dispatch).

Implementation (Strategy C of three considered):
  - `M7HR_BLEND_AND_WRITE` macro: added `smooth_arg` as the last
    parameter. Internal `uint8 smooth = (Settings.Mode7HiresBilinear
    == 2);` line replaced with `uint8 smooth_local = (smooth_arg);`
    (renamed to avoid shadowing the outer `smooth` declared in the
    enclosing function).
  - `M7HR_SAMPLE_BILINEAR` macro: added `smooth_arg` parameter and
    threads it through to the wrapped `M7HR_BLEND_AND_WRITE` call.
  - All 168 external call sites updated: 56 of `M7HR_SAMPLE_BILINEAR`
    and 112 of `M7HR_BLEND_AND_WRITE`, all in BL/BL4X/BL1X family
    functions (HR/HR4X are nearest-neighbour and don't blend).
  - Each of the 42 BL family functions (3 family x 2 BG x 7 math)
    declares `uint8 smooth = (Settings.Mode7HiresBilinear == 2);`
    once at the top, before the per-line loop.

Not byte-identical to pristine (`f5720b6`): BL family function
sizes shifted in both directions, total -140 bytes (-0.04% of BL
family code). All non-BL functions byte-identical. Behavior is
equivalent: `smooth` is computed identically, just hoisted.

Other strategies considered:
  - Strategy A: have the macro reference an outer-scope `smooth`
    directly. Rejected: the macro becomes dependent on caller scope,
    which is exactly the macro-coupling we've been reducing.
  - Strategy B: specialize each BL function into two (smooth /
    not-smooth) variants and branch at function entry. Rejected for
    now: doubles BL family function count, would need profile data
    to justify. Could be a future commit if the per-pixel branch on
    `smooth` shows up in profiles.

Symbol verification (vs pristine `f5720b6`):
  - 313 Draw* functions present (verified).
  - 44 Renderers_ arrays present (verified).
  - Function size diff: 88 lines, all in BL family. All non-BL
    functions byte-identical.
  - Build clean, zero warnings.

Files changed: `src/tile.c` -420 net (item 3 -462, item 2 +42).

Behaviour invariants -- to be verified externally by Lib before
push: audio bit-identical (this is the crucial check since codegen
drifted), savestate compat, valgrind clean. Audio hashes should
match if the hoist is truly equivalent; if they don't, I need to
investigate.

### `f5720b6` — tile.c: inline body macros into per-function bodies (commit A)

Inlines all 11 `TILE_BODY_*` macros into the bodies of the 313
renderer functions. Per the user's request: "tile.c is still macro
soup, not what I expected. these macros add considerably to
compilation time and make the code completely incomprehensible."
The user's specific direction: "you should be able to restructure
this code so that we don't need all this macro soup", with
"static inline" rejected ("we cannot guarantee the compiler will
inline every single invocation, best to keep that as macros") and
the small macros (plotters, color math, helpers) explicitly kept
as-is.

What's gone:
  - `TILE_BODY_UNCLIPPED` (was DrawTile16's body)
  - `TILE_BODY_CLIPPED` (was DrawClippedTile16's body)
  - `TILE_BODY_MOSAIC_PIXEL` (was DrawMosaicPixel16's body)
  - `TILE_BODY_BACKDROP` (was DrawBackdrop16's body)
  - `TILE_BODY_NORMAL_M7` (was DrawMode7BG{1,2}'s body)
  - `TILE_BODY_MOSAIC_M7` (was DrawMode7MosaicBG{1,2}'s body)
  - `TILE_BODY_M7_HR` (was DrawMode7BG{1,2}HR's body)
  - `TILE_BODY_M7_HR4X` (was DrawMode7BG{1,2}HR4X's body)
  - `TILE_BODY_M7_BL` (was DrawMode7BG{1,2}BL's body)
  - `TILE_BODY_M7_BL4X` (was DrawMode7BG{1,2}BL4X's body)
  - `TILE_BODY_M7_BL1X` (was DrawMode7BG{1,2}BL1X's body)

How:
  - Each `#define TILE_BODY_X(...)` macro was parsed by a Python
    script (`/home/claude/inline.py`).
  - Each call site `static void NAME(args) TILE_BODY_X(arg1, ...)`
    was replaced with the full inlined function body via
    token-aware parameter substitution (regex with word
    boundaries to avoid partial-name matches).
  - The substituted body has all macro parameters resolved
    (`MATH_SELECTOR` -> `REGMATH`, `MATH_OP` -> `ADD`, `Z_EXPR` ->
    `(D + 7)`, etc.). Plotter macros that the body invokes
    (`DT_PIXEL_*`, `M7N_PIXEL_*`, etc.) stay as macros and expand
    at compile time as before.
  - All `#define TILE_BODY_*` and `#undef TILE_BODY_*` lines
    deleted.
  - Stale comments referencing the now-deleted body macros were
    rewritten to reflect the inlined state.

What stays as macros (per the user's instruction):
  - Pixel plotters: `DT_PIXEL_*`, `CT_PIXEL_*`, `MP_PIXEL_*`,
    `BACKDROP_PIXEL_*`, `M7N_PIXEL_*`.
  - Color-math: `NOMATH`, `REGMATH`, `MATHF1_2`, `MATHS1_2`,
    `COLOR_ADD*`, `COLOR_SUB*`.
  - File-scope helpers: `GET_CACHED_TILE`, `IS_BLANK_TILE`,
    `SELECT_PALETTE`, `H_FLIP`, `V_FLIP`, `CLIP_10_BIT_SIGNED`.
  - Mode 7 bilinear helpers: `M7HR_LOOKUP_4`,
    `M7HR_BLEND_AND_WRITE`, `M7HR_SAMPLE_BILINEAR`,
    `M7HR_LOOKUP_4_FILL`.

Effect:
  - Each of the 313 renderer functions can now be read top-to-bottom
    without macro chasing for the body. Only small leaf macros
    (plotters / color-math / helpers) need expansion to follow the
    code, and those are localized: a plotter macro is small enough
    to mentally inline at a glance.
  - File grew from 4,903 lines (`7af3f84`) to 28,591 lines (this
    commit). +23,688 lines net. The volume is what it costs to
    write each function independently.
  - Compile-time impact is expected to drop because the preprocessor
    no longer expands gigantic body macros 7-14 times per family;
    the resulting token stream is the same but fewer intermediate
    expansions are needed. Not measured here -- that's a `make`
    benchmark to run separately.

Symbol verification (vs pristine `7af3f84`):
  - 313 Draw* functions; full per-function size diff is empty
    across all 313 (verified). True pure refactor.
  - 44 Renderers_ arrays; full diff empty.
  - Build clean, zero warnings.

Files changed: `src/tile.c` +25,XXX / -1,5XX (-1 net body macro
definitions; +23,688 net lines for inlined bodies).

This is "commit A" of the macro-removal sub-effort. "Commit B"
(adding inline documentation explaining what each body does, e.g.
the Mode 7 matrix-rotation arithmetic and the bilinear blend
math) is a separate future effort: it requires actually
understanding the SNES PPU semantics rather than just transcribing
code, and Claude should treat its draft comments as guesses to
verify rather than authoritative documentation.

Behaviour invariants -- to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean.

### `7af3f84` — tile.c: unroll fan-out macros + structural cleanup (Stage 3.5 + Stage 4)

Removes the now-dead `#ifndef NAME1` wrapper (Stage 3.5) and unrolls
all 11 function-emitting fan-out macros into explicit `static void
NAME(args) TILE_BODY_*(args)` definitions (Stage 4).

User-stated motivation for Stage 4: "It will allow us to optimize
the code later better for all the various modes." Per-function
optimization opportunities (different inlining hints, different
attributes on hot vs cold variants, function-level diff diagnostics)
are easier to apply when each function is a separate textual unit
instead of one row in a fan-out macro.

**Stage 3.5 part:** removes the `#ifndef NAME1 / #endif` wrapper
at `src/tile.c` line 996. With Stage 3 complete, NAME1 is never
defined; the wrapper's body always runs. The `#endif` at end-of-file
that closed it is also removed. Replaced with an explanatory banner
comment describing the recursive-include landing-zone.

**Stage 4 part:** all 313 renderer function definitions are now
written explicitly. The 11 fan-out macros are gone:
  - DrawTile16: `DEFINE_DT_FN`, `DEFINE_DT_NAME2`
  - DrawClippedTile16: `DEFINE_CT_FN`, `DEFINE_CT_NAME2`
  - DrawMosaicPixel16: `DEFINE_MP_FN`, `DEFINE_MP_NAME2`
  - DrawBackdrop16: `DEFINE_BACKDROP_FN`, `DEFINE_BACKDROP_NAME2`
  - native Mode 7: `DEFINE_M7_FN`, `DEFINE_M7_NAME2`, `DEFINE_M7_BG`
  - mosaic Mode 7: `DEFINE_M7_MOSAIC_FN`, `DEFINE_M7_MOSAIC_NAME2`,
    `DEFINE_M7_MOSAIC_BG`
  - M7 HR2X: `DEFINE_M7_HR_FAMILY`
  - M7 HR4X: `DEFINE_M7_HR4X_FAMILY`
  - M7 BL2X: `DEFINE_M7_BL_FAMILY`
  - M7 BL4X: `DEFINE_M7_BL4X_FAMILY`
  - M7 BL1X: `DEFINE_M7_BL1X_FAMILY`

The 5 M7 hires `DEFINE_M7_*_FAMILY` macros previously embedded the
function definition inside the macro body. For each of those, the
macro was converted from function-emitter to body-only `TILE_BODY_M7_<X>`:
the `static void BG_NAME##suffix##Normal1x1 (...)` line was removed,
the parameter list was reduced from 7 params to 5 (suffix and
BG_NAME moved to the call site), and each of the 14 invocations
(7 BG1 + 7 BG2) was rewritten as explicit
`static void NAME(args) TILE_BODY_M7_<X>(sel, op, z_expr, mask, dc)`.

Comments: per-NAME2 banner comments (e.g.
`/* DrawTile16 NAME2 = Normal1x1: 7 math variants. */`) added for
each NAME2 group across all sections. Section banners and
section-end markers preserve the original structural documentation.
Each unrolled section also has an "Outer fan-out: explicit functions
and dispatch arrays" preamble describing what changed.

What stays as macros (per the user's instruction):
  - All body macros: `TILE_BODY_NORMAL_M7`, `TILE_BODY_MOSAIC_M7`,
    `TILE_BODY_CLIPPED`, `TILE_BODY_UNCLIPPED`, `TILE_BODY_BACKDROP`,
    `TILE_BODY_MOSAIC_PIXEL`, `TILE_BODY_M7_HR`, `TILE_BODY_M7_HR4X`,
    `TILE_BODY_M7_BL`, `TILE_BODY_M7_BL4X`, `TILE_BODY_M7_BL1X`.
  - All pixel plotters: `DT_PIXEL_*`, `CT_PIXEL_*`, `MP_PIXEL_*`,
    `BACKDROP_PIXEL_*`, `M7N_PIXEL_*`, `M7HR_LOOKUP_4`,
    `M7HR_BLEND_AND_WRITE`, `M7HR_SAMPLE_BILINEAR`.
  - All color-math: `NOMATH`, `REGMATH`, `MATHF1_2`, `MATHS1_2`,
    `COLOR_ADD*`, `COLOR_SUB*`.
  - All file-scope helpers: `GET_CACHED_TILE`, `IS_BLANK_TILE`,
    `SELECT_PALETTE`, `H_FLIP`, `V_FLIP`, `CLIP_10_BIT_SIGNED`.

Symbol verification (vs pristine `a40fa37`):
  - 313 Draw* functions; full per-function size diff is empty
    across all 313 (verified). True pure refactor.
  - 44 Renderers_ arrays; full diff empty.
  - Build clean, zero warnings.

Files changed: `src/tile.c` +1383 / -400 (+983 net). The unrolled
form is significantly larger than the macro-driven form -- 313
function definitions written by hand take more text than 11 fan-out
macros. The trade-off the user accepted: more lines now, but
per-function flexibility for future optimization.

Behaviour invariants -- to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean.

### `a40fa37` — tile.c: de-template DrawTile16 + remove dispatch scaffold (Stage 2.11 + Stage 3)

**Final commit of the tile-untangle effort.** De-templates the
last templated family (DrawTile16) and removes the now-dead
level-2/level-3 dispatch scaffold from `tile.c`.

**Stage 2.11 part:** de-templates DrawTile16, the renderer for
unclipped 8-pixel-wide BG tiles. The hot path of the BG-tile
renderer.

Body shape: simpler than DrawClippedTile16. No per-pixel switch
over StartPixel, no Width clamp -- just 4 flip-case branches each
running `for (n = 0; n < 8; n++) DRAW_PIXEL(n, Pix = bp[idx])`.

ARGS shape: (uint32 Tile, uint32 Offset, uint32 StartLine,
uint32 LineCount). Four parameters -- the smallest of any tile
family.

Same plotter shape as DrawMosaicPixel16 / DrawClippedTile16
(Z = GFX.Z1/Z2, runtime Pix). Section-local DT_PIXEL_*
plotters. New TILE_BODY_UNCLIPPED body macro mirrors the
previously-templated DRAW_TILE() expansion -- 4 flip-case
branches, 8-pixel inner for-loop. Threads BPSTART_EXPR and
BP_STEP_EXPR per NAME2 same as ClippedTile.

42 functions emitted (6 NAME2 x 7 math) plus 6 dispatch arrays.

**Stage 3 part:** removes the level-2 and level-3 dispatch
scaffold from `tile.c`. With Stage 2.11 landed, no `#include
"tile.c"` self-recursion remains inside the `#ifndef NAME1`
branch -- so the entire `#else` block (lines that used to handle
NAME1-defined recursive includes for level-2 dispatch, and
NAME1+NAME2-defined recursive includes for level-3 dispatch)
is dead code.

Deleted:
  - `#define BPSTART StartLine` / `#define PITCH 1` and matching
    interlace redefines
  - per-NAME2 `DRAW_PIXEL_*` macro definitions (DRAW_PIXEL,
    DRAW_PIXEL_N2x1, DRAW_PIXEL_N4x1, DRAW_PIXEL_H2x1)
  - per-NAME2 `#define NAME2 / #include "tile.c" / #undef NAME2`
    machinery for Normal1x1, Normal2x1, Normal4x1, Hires,
    Interlace, HiresInterlace
  - `#ifndef M7HIRES_ONLY` and `#ifndef NO_INTERLACE` guards
  - level-3 `MAKENAME` / `CONCAT3` machinery
  - 7 `static void MAKENAME(NAME1, suffix, NAME2) (ARGS)`
    instantiations and the dispatcher array template

What stays:
  - line-660 outer `#include "tile.c"` -- load-bearing, triggers
    the recursive include that places family code in the
    compilation
  - `#ifndef _NEWTILE_CPP / #else` outer wrapper at the top of
    the file -- still load-bearing for separating "first include"
    code (helpers, ConvertTile_*, etc.) from "recursive include"
    code (family bodies)
  - `#ifndef NAME1` wrapper at line 996 -- structurally dead
    (NAME1 is never defined now), but kept for now; removing it
    is a separate structural cleanup

Removed:
  - templated DrawTile16 instantiation block
  - DRAW_TILE() macro definition for unclipped tile
  - `#define Z1 GFX.Z1` / `#define Z2 GFX.Z2` and matching
    `#undef`s
  - 264 lines of dispatch scaffold (level-2 plus level-3)

Added:
  - de-templated DrawTile16 section with banner comment
  - `DT_PIXEL_*` plotters (4 plotter shapes)
  - `TILE_BODY_UNCLIPPED` body macro
  - `DEFINE_DT_FN` / `DEFINE_DT_NAME2` fan-out
  - 42 explicit `DrawTile16*_*` functions
  - 6 explicit `Renderers_DrawTile16*` arrays

Symbol verification (vs pristine `8906302`):
  - 313 Draw* functions; full per-function size diff is empty
    across all 313 (verified). True pure refactor.
  - 44 Renderers_ arrays; full diff empty.
  - Build clean, zero warnings.

Files changed: `src/tile.c` +200/-336 (-136 net). The DrawTile16
de-templated section adds ~200 lines, but Stage 3 deletes ~264
lines of scaffold, for a net source-size reduction.

**The tile-untangle effort proper is done.** All 11 templated
families are de-templated, the level-2/3 dispatch scaffold is
gone, and the Mode 7 file-scope macros are gone. Optional Stage
4 (folding optimizations) and the structural cleanup of the
now-dead `#ifndef NAME1` wrapper are separate, profile-driven /
opportunistic concerns.

Lessons from earlier stages applied without trouble:
  - N parameter unparenthesized in plotter bodies (Stage 2.6b/2.7)
  - `&& (M)` clause preserved verbatim (Stage 2.5)
  - Per-NAME2 BPSTART threading (Stage 2.9)
  - PITCH as BP_STEP literal (Stage 2.10)

This commit also updates docs/tile-untangle-state.md to reflect
Stages 2.11 + 3 completion. Bundled SHA refresh from the previous
rebase.

Behaviour invariants -- to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean,
7-distinct-math-variants on the 42 new DrawTile16 functions.

### `8906302` — tile.c: de-template DrawClippedTile16 (Stage 2.10)

Stage 2.10. De-templates DrawClippedTile16, the renderer that
draws a tile clipped to an arbitrary [StartPixel, StartPixel +
Width) horizontal range. Used by the BG-tile path when only part
of a tile is visible (clipped against a window or screen edge).

Body shape: 4 flip-case branches (no flip / H_FLIP only / V_FLIP
only / both flips), each running an 8-case switch on StartPixel
with fall-through and a per-case `--w` decrement to honour the
Width clamp. Each case calls DRAW_PIXEL with the assignment-and-
test `Pix = bp[idx]` -- if the source pixel is transparent (0),
the AND short-circuits and no write happens.

First family to thread both BPSTART and PITCH:
  - Non-interlace (Normal1x1, Normal2x1, Normal4x1, Hires):
    BPSTART = StartLine,                    BP_STEP = 8
  - Interlace (Interlace, HiresInterlace):
    BPSTART = (StartLine * 2 + BG.InterlaceLine), BP_STEP = 16

The original templated form used `8 * PITCH` for the per-line
cache pointer advance; we substitute the product directly into
the BP_STEP_EXPR parameter so the call site sees a literal (8 or
16). This keeps the pointer-arithmetic compiler-visible and
matches the templated codegen byte-for-byte.

ARGS shape: (uint32 Tile, uint32 Offset, uint32 StartPixel,
uint32 Width, uint32 StartLine, uint32 LineCount). Six parameters.
Note this differs from DrawMosaicPixel16's order
(StartLine/StartPixel/Width vs StartPixel/Width/StartLine).

Z values are runtime GFX.Z1 / GFX.Z2 (file-scope GFX struct
fields, set by the caller per BG/priority pass). Same as
DrawMosaicPixel16.

Section-local plotters CT_PIXEL_N1x1 / N2x1 / N4x1 / H2x1 mirror
the MP_PIXEL_* shapes from Stage 2.9 with section-local naming.
New TILE_BODY_CLIPPED body macro mirrors the previously-templated
DRAW_TILE() expansion -- 4 flip-case branches, 8-case switches
with fall-through, all 64 PIXEL_PLOT call sites preserved.

Removed:
  - templated DrawClippedTile16 instantiation block
  - #define Z1 GFX.Z1 / #define Z2 GFX.Z2 and matching #undefs
  - DRAW_TILE() macro definition for clipped-tile (~85 lines)

Added:
  - de-templated section with banner comment
  - CT_PIXEL_* plotters (4 plotter shapes)
  - TILE_BODY_CLIPPED body macro (~75 lines, mirrors original)
  - DEFINE_CT_FN / DEFINE_CT_NAME2 fan-out
  - 42 explicit DrawClippedTile16*_* functions (6 NAME2 x 7 math)
  - 6 explicit Renderers_DrawClippedTile16* arrays

Symbol verification (vs pristine `bb3cacf`):
  - 313 Draw* functions; full per-function size diff is empty
    across all 313 (verified). True pure refactor.
  - 44 Renderers_ arrays; full diff empty.
  - Build clean, zero warnings.

Files changed: `src/tile.c` +228/-105 (+123 net).

One non-Mode-7 family remains: DrawTile16 (Stage 2.11), the
hot-path renderer for unclipped 8-pixel tiles. After that, Stage
3 removes the level-2/level-3 templating scaffold (the
`#ifndef NAME1` / `#ifndef NAME2` recursion and the per-NAME2
BPSTART/PITCH/DRAW_PIXEL machinery in the `#else` branches).
The plan is to bundle the Stage 3 scaffold removal with the
Stage 2.11 commit, since Stage 2.11 makes the scaffold dead code
and removing dead code in the same commit that creates the
deadness follows the precedent set by earlier stages
(DRAW_TILE_NORMAL / DRAW_TILE_MOSAIC removal in Stage 2.6a /
Stage 2.6b/2.7).

Lessons from earlier stages applied without trouble:
  - N parameter unparenthesized in plotter bodies (Stage 2.6b/2.7)
  - `&& (M)` clause preserved verbatim (Stage 2.5)
  - Per-NAME2 BPSTART threading (Stage 2.9)

This commit also updates docs/tile-untangle-state.md to reflect
Stage 2.10 completion. Bundled SHA refresh from the previous
rebase.

Behaviour invariants -- to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean,
7-distinct-math-variants on the 42 new DrawClippedTile16 functions.

### `bb3cacf` — tile.c: de-template DrawMosaicPixel16 (Stage 2.9)

Stage 2.9. De-templates DrawMosaicPixel16, the renderer that
replicates a single mosaic pixel (sourced from one cached tile)
across an HMosaic-by-LineCount block. Used by the BG-mosaic path
(non-Mode-7 mosaic).

First family in the de-templating effort to need **all six
NAME2 variants**, including Interlace and HiresInterlace -- and
therefore the first to thread per-NAME2 BPSTART expressions
through the fan-out:

  - Non-interlace (Normal1x1, Normal2x1, Normal4x1, Hires):
    BPSTART = StartLine
  - Interlace (Interlace, HiresInterlace):
    BPSTART = (StartLine * 2 + BG.InterlaceLine)

PITCH is not used by mosaic-pixel (per the original "PITCH is
not used" comment) so it isn't threaded. The Interlace variants
differ from Normal2x1 / Hires only in which row of the cached
tile gets sampled.

ARGS shape: `(uint32 Tile, uint32 Offset, uint32 StartLine,
uint32 StartPixel, uint32 Width, uint32 LineCount)` -- six
parameters.

Z values are runtime `GFX.Z1` / `GFX.Z2` (file-scope GFX struct
fields, set by the caller per BG/priority pass). The plotter's
"M" parameter is the literal 1 -- the source-pixel transparency
check (`if (Pix)`) happens once outside the replication loop.

Section-local plotters `MP_PIXEL_N1x1` / `N2x1` / `N4x1` / `H2x1`
mirror the original DRAW_PIXEL_* shapes with these conventions
baked in. New `TILE_BODY_MOSAIC_PIXEL` body macro mirrors the
previously-templated DRAW_TILE() expansion.

Removed:
  - templated DrawMosaicPixel16 instantiation block
  - `#define Z1 GFX.Z1` / `#define Z2 GFX.Z2` and matching `#undef`s
  - DRAW_TILE() macro definition for mosaic-pixel

Added:
  - de-templated section with banner comment
  - `MP_PIXEL_*` plotters (4 plotter shapes)
  - `TILE_BODY_MOSAIC_PIXEL` body macro
  - `DEFINE_MP_FN` / `DEFINE_MP_NAME2` fan-out
  - 42 explicit `DrawMosaicPixel16*_*` functions (6 NAME2 x 7 math)
  - 6 explicit `Renderers_DrawMosaicPixel16*` arrays

Symbol verification (vs pristine `9ac00cb`):
  - 313 Draw* functions; full per-function size diff is empty
    (verified). True pure refactor.
  - 44 Renderers_ arrays; full diff empty.
  - Build clean, zero warnings.

Files changed: `src/tile.c` +160/-43 (+117 net).

Two non-Mode-7 families remain: DrawClippedTile16 (Stage 2.10),
DrawTile16 (Stage 2.11). After those, Stage 3 removes the
level-2/level-3 templating scaffold (the `#ifndef NAME1` /
`#ifndef NAME2` recursion and the per-NAME2 BPSTART/PITCH/
DRAW_PIXEL machinery in the `#else` branches). The shared
helpers `GET_CACHED_TILE`, `IS_BLANK_TILE`, `SELECT_PALETTE`,
`H_FLIP`, `V_FLIP` stay -- they're used by all the tile-rendering
families and live at file scope.

Lessons from earlier stages applied without trouble:
  - N parameter unparenthesized in plotter bodies (Stage 2.6b/2.7)
  - `&& (M)` clause preserved verbatim (Stage 2.5)
  - 7-fold math fan-out matches the existing pattern

This commit also updates docs/tile-untangle-state.md to reflect
Stage 2.9 completion. Bundled SHA refresh from the previous
rebase.

Behaviour invariants -- to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean,
7-distinct-math-variants on the 42 new DrawMosaicPixel16 (`bb3cacf`)
functions.

### `9ac00cb` — tile.c: de-template DrawBackdrop16 (Stage 2.8)

Stage 2.8. First non-Mode-7 family de-templated. DrawBackdrop16
is the renderer that fills a horizontal range of the scanline
buffer with the backdrop colour (palette index 0) at depth 1,
drawn after all BG/OBJ layers.

Structurally simpler than Mode 7:
  - No tile lookup, no per-pixel data
  - Z is constant 1 (depth 1, drawn last)
  - Pix is constant 0 (always palette index 0)
  - The DRAW_PIXEL "M" parameter (test) is constant 1
  - ARGS shape is `(uint32 Offset, uint32 Left, uint32 Right)` --
    Offset is a parameter, not a body-local

Different ARGS from Mode 7 (which uses `(uint32 Left, uint32 Right,
int D)`) means the body and fan-out can't share with Mode 7's
machinery. The de-templated form uses a separate set of
`BACKDROP_PIXEL_*` plotter macros and a `TILE_BODY_BACKDROP` body
macro.

The original templated form baked Z = 1, Pix = 0 as preprocessor
#defines and used NO_INTERLACE to skip Interlace/HiresInterlace
variants. The de-templated form bakes the same constants directly
into the plotter macros and simply doesn't emit Interlace
variants.

Per-NAME2 plotters mirror the original DRAW_PIXEL_* expansion
exactly, including the `&& (1)` clause from the templated `M = 1`
substitution. This preserves the expression tree shape the
compiler sees and gives byte-identical codegen.

Files changed: `src/tile.c`, +136/-39, +97 net. The de-templated
form is bigger than the templated source because backdrop's
templated form was tiny (40 lines, all the work was in the
level-2/3 dispatch); explicit form unrolls 4 plotter macros plus
body plus 4 fan-out invocations.

Removed:
  - templated DrawBackdrop16 instantiation block
  - `#define NO_INTERLACE` / `Z1` / `Z2` / `Pix` and matching
    `#undef`s
  - DRAW_TILE() macro definition for backdrop

Added:
  - de-templated section with banner comment
  - `BACKDROP_PIXEL_N1x1` / `N2x1` / `N4x1` / `H2x1` plotters
  - `TILE_BODY_BACKDROP` body macro
  - `DEFINE_BACKDROP_FN` / `DEFINE_BACKDROP_NAME2` fan-out
  - 28 explicit `DrawBackdrop16*_*` functions
  - 4 explicit `Renderers_DrawBackdrop16*` arrays

Symbol verification (vs pristine `3c7ad4f`):
  - 313 Draw* functions; full per-function size diff vs pristine
    is empty (verified). True pure refactor.
  - 44 Renderers_ arrays; full diff empty.
  - Build clean, zero warnings.

Lessons from earlier stages applied (no codegen surprises this
time):
  - N parameter unparenthesized in plotter bodies (Stage 2.6b/2.7
    lesson)
  - `&& (M)` clause preserved verbatim in plotter, even though
    M is the literal `1` (Stage 2.5 lesson about preserving
    expression-tree shape)

The non-Mode-7 families now have one done (DrawBackdrop16) and
three remaining (DrawMosaicPixel16, DrawClippedTile16,
DrawTile16). After those, Stage 3 removes the level-2/3
templating scaffold.

This commit also updates docs/tile-untangle-state.md to reflect
Stage 2.8 completion. Bundled SHA refresh from the previous
rebase.

Behaviour invariants -- to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean,
7-distinct-math-variants on the 28 new backdrop functions.

### `3c7ad4f` — tile.c: de-template mosaic Mode 7 + drop dead NAME2 variants (Stage 2.6b/2.7)

Combines Stage 2.6b (drop unused NAME2 variants for de-templated
native Mode 7 BG families) and Stage 2.7 (de-template the mosaic
Mode 7 NAME1 families) into one commit.

Stage 2.7 part: de-templates DrawMode7MosaicBG1 and DrawMode7MosaicBG2.
Same approach as native: a `TILE_BODY_MOSAIC_M7` body macro
mirrors the structure of the previously-templated DRAW_TILE_MOSAIC,
parameterized for explicit MATH_SELECTOR/MATH_OP/Z_EXPR/MASK_VAL/
DC_EXPR/BG_INDEX/PIXEL_PLOT. The `BG_INDEX` parameter (0 or 1)
selects which `PPU.BGMosaic[]` entry to read for the per-BG
mosaic-enable bit; this is the only structural difference from
the native body. New fan-out macros `DEFINE_M7_MOSAIC_FN`,
`DEFINE_M7_MOSAIC_NAME2`, `DEFINE_M7_MOSAIC_BG` mirror the native
ones. 42 functions emitted (2 BGs x 3 NAME2 x 7 math) plus 6
arrays.

Stage 2.6b part: Drop Normal4x1, Interlace, HiresInterlace from
both native and mosaic Mode 7 BG families. The dispatcher only
selects Normal1x1, Normal2x1, and Hires (LTO had been stripping
the others all along). The `M7N_PIXEL_N4x1` macro is also
dropped since no surviving variant uses it.

Together: 84 native + 42 mosaic = 126 native+mosaic Mode 7
functions emitted explicitly. Down from 168+168 = 336 in the
templated form before LTO (LTO stripped half on both sides).

Pure refactor confirmed via byte-identical machine code:
  - 313 Draw* functions, full per-function size diff vs pristine
    `f5894be` is empty across all 313.
  - 44 Renderers_ arrays, full diff empty.

Codegen-correctness lesson learned: my first draft of
`M7N_PIXEL_*` plotter macros parenthesized the N parameter
(`Offset + 2 * (N)`). For native Mode 7 this was byte-identical
because the call site passes `N = x` (a simple loop variable)
and `2 * x` == `2 * (x)`. For mosaic the call site passes
`N = w + h * GFX.PPL` (a compound expression), and the original
templated `DRAW_PIXEL_*` macros used unparenthesized N, so the
substitution was `2 * w + h * GFX.PPL` not `2 * (w + h * GFX.PPL)`
-- semantically different memory locations. I changed the
plotter macros to use unparenthesized N to match the original;
this restored byte-identical mosaic codegen and (incidentally)
preserves whatever buffer-offset semantics the original assumed.

Bundled cleanups:
  - `DRAW_TILE_MOSAIC` macro definition deleted (~135 lines)
  - file-scope `Z1`/`Z2`/`MASK`/`DCMODE`/`BG`/`NO_INTERLACE`
    `#define`s for Mode 7 deleted
  - `M7N_PIXEL_N4x1` definition deleted
  - dangling `#undef DRAW_TILE_MOSAIC` and `#undef NO_INTERLACE`
    deleted

Removed:
  - templated DrawMode7MosaicBG{1,2} instantiation block
  - DRAW_TILE_MOSAIC macro definition
  - file-scope Mode 7 #defines (Z1, Z2, MASK, DCMODE, BG,
    NO_INTERLACE)
  - M7N_PIXEL_N4x1 macro
  - 3 dead NAME2 variants per BG (Normal4x1, Interlace,
    HiresInterlace) -- previously LTO-stripped but visible in
    the source

Added:
  - TILE_BODY_MOSAIC_M7 body macro
  - DEFINE_M7_MOSAIC_FN / DEFINE_M7_MOSAIC_NAME2 /
    DEFINE_M7_MOSAIC_BG fan-out macros
  - 42 explicit DrawMode7MosaicBG{1,2}* functions
  - 6 explicit Renderers_DrawMode7MosaicBG{1,2}* arrays

Files changed: `src/tile.c`, +251/-269, -18 net (the ~135-line
DRAW_TILE_MOSAIC definition and the dead-NAME2 boilerplate
together outweigh the new mosaic body and fan-out).

The Mode 7 group is now complete. Stage 2.8 (DrawBackdrop16)
starts the non-Mode-7 families.

This commit also updates docs/tile-untangle-state.md to reflect
Stage 2.6b/2.7 completion. Bundled SHA refresh from the previous
rebase (Stage 2.6a entry switching from "(this commit)"
placeholder to its real upstream SHA `f5894be`).

Behaviour invariants -- to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean,
7-distinct-math-variants on the new mosaic functions.

### `f5894be` — tile.c: de-template native Mode 7 BG renderers (Stage 2.6a)

Stage 2.6a. De-templates the two non-mosaic native Mode 7 NAME1
families: `DrawMode7BG1` and `DrawMode7BG2`. Their mosaic
counterparts (`DrawMode7MosaicBG1`, `DrawMode7MosaicBG2`) remain
templated and are de-templated separately in Stage 2.7.

Native Mode 7 is the first family to use the **full NAME2 set**
in level-2 dispatch (Normal1x1, Normal2x1, Normal4x1, Hires,
Interlace, HiresInterlace), so the fan-out structure is two
levels deeper than the M7 hires families:

- `M7N_PIXEL_*` macros: per-NAME2 inline pixel plotters that
  mirror the original DRAW_PIXEL_N1x1 / N2x1 / N4x1 / H2x1 macro
  shapes. Take MATH_SELECTOR / MATH_OP / Z_EXPR as parameters
  instead of reading from enclosing scope. Preserve the
  assignment-and-test expression form
  `(Pix = (b & MASK_VAL))` so the compiler can short-circuit
  the AND when the Z-test fails (lesson from Stage 2.5).
- `TILE_BODY_NORMAL_M7`: per-line Mode 7 body, takes PIXEL_PLOT
  as a parameter. Identical structure and arithmetic to the
  former DRAW_TILE_NORMAL macro.
- `DEFINE_M7_FN`: emits one function with the chosen math op
  and pixel plotter.
- `DEFINE_M7_NAME2`: 7-fold math fan-out plus dispatch array.
- `DEFINE_M7_BG`: 6-fold NAME2 fan-out, used twice (BG1, BG2).

84 functions emitted (2 BGs x 6 NAME2 x 7 math) plus 12 dispatch
arrays. Of those, 36 functions and 6 arrays are LTO-stripped
because the dispatcher only references the Normal1x1, Normal2x1,
and Hires variants. The Normal4x1 / Interlace / HiresInterlace
variants are emitted to match the templated form byte-for-byte;
they're removed in Stage 2.6b.

Removed:
  - templated `DrawMode7BG1` / `DrawMode7BG2` instantiation blocks
  - `DRAW_TILE_NORMAL` macro definition (no longer referenced;
    last user was the templated BG1/BG2 instantiations -- the
    Mosaic blocks below this section use DRAW_TILE_MOSAIC, not
    DRAW_TILE_NORMAL)

Added:
  - banner section comment explaining the family
  - `M7N_PIXEL_N1x1` / `N2x1` / `N4x1` / `H2x1` per-NAME2 plotters
  - `TILE_BODY_NORMAL_M7` body macro
  - `DEFINE_M7_FN` / `DEFINE_M7_NAME2` / `DEFINE_M7_BG` fan-out
  - 84 explicit `DrawMode7BG{1,2}*` functions
  - 12 explicit `Renderers_DrawMode7BG{1,2}*` arrays

The mosaic variants (DrawMode7MosaicBG1, DrawMode7MosaicBG2) are
left templated for Stage 2.7. They still use the file-scope
Z1/Z2/MASK/DCMODE/BG/NO_INTERLACE #defines which therefore stay
alive across the Mosaic block.

Symbol verification (vs pristine `624492c`):
  - 313 Draw* functions; full per-function size diff vs pristine
    is empty across all 313 (verified, not just the 84 native
    Mode 7 ones). True pure refactor.
  - 44 Renderers_ arrays; full diff empty.
  - Build clean. 8 unused-Renderers warnings up from 4 -- the
    extra 4 are the BG1/BG2 Normal4x1/Interlace/HiresInterlace
    variants that were previously hidden inside the templated
    expansion and are now visible at file scope. Stage 2.6b
    removes them and cleans up the warnings.

Behaviour invariants -- to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean,
7-distinct-math-variants on the new functions.

This commit also updates docs/tile-untangle-state.md (in-tree)
to reflect Stage 2.6a completion. Bundled SHA refresh from the
previous rebase (Stage 2.5 entry switching from "(this commit)"
placeholder to its real upstream SHA `624492c`).

### `624492c` — tile.c: de-template Mode 7 HR renderers (Stage 2.5)

Stage 2.5. Same fan-out pattern as BL (Stage 2.4) but with inline
nearest-neighbour DRAW_PIXEL writes instead of bilinear blending.
A local `DEFINE_M7_HR_FAMILY` macro materializes 14 functions (7
math variants x 2 BGs).

HR is the original 2x nearest-neighbour Mode 7 path: two samples
per native pixel using half-step (aa_h = aa/2), each written
through the standard Z-test + MATH pixel write. Same overall
structure as BL but inline of the per-pixel write instead of
helper invocation.

The per-pixel write inlines the original Normal1x1 DRAW_PIXEL
shape exactly: `if (Z_EXPR > GFX.DB[...] && (Pix = (b & MASK)))`
-- single assignment-and-test expression, no separate Pix
assignment, no idx local. This matches the original templated
codegen byte-for-byte. (An earlier draft separated the assignment
and used an idx local; that produced ~+58 bytes per BG2 function
and ~18 extra instructions because the compiler couldn't exploit
the Z-test short-circuit to skip the AND-with-MASK on
non-drawing pixels. Restoring the original shape brought the
codegen back to byte-identical.)

Files changed: `src/tile.c`, +280/-195, +85 net.

Removed:
  - templated `DrawMode7BG{1,2}HR` instantiation block
  - `DRAW_TILE_NORMAL_M7HIRES` macro definition (~140 lines, no
    longer referenced -- HR was the only user)
  - `ARGS / M7HIRES_ONLY` scope `#define`s and `#undef`s (HR was
    the last user inside that scope)
  - dangling `#undef DRAW_TILE_NORMAL_M7HIRES`

Added:
  - de-templated section with banner comment explaining the family
  - `DEFINE_M7_HR_FAMILY` fan-out macro with parameter docs
  - 14 explicit `DrawMode7BG{1,2}HR*_Normal1x1` functions
  - 2 explicit `Renderers_DrawMode7BG{1,2}HRNormal1x1` arrays

Bundled stale-comment fixes (replaced as part of the section
banner rewrite):
  - "These functions are scaffolded but not yet referenced by any
    dispatcher. The wiring lands in a follow-up patch." This was
    true at one point, but the dispatcher in `S9xSelectTileRenderers`
    references `Renderers_DrawMode7BG{1,2}HRNormal1x1` already.
    Banner now describes the actual wiring.
  - The cross-reference in the BL helper section header
    ("same output rate as DRAW_TILE_NORMAL_M7HIRES") was updated
    to point to the de-templated HR family below, since the
    macro it referenced no longer exists.

Symbol verification (vs pristine `6fde805`):
  - 14 HR functions present, byte-identical machine code.
    Per-function sizes verified for all 14 -- diff of size lists
    is empty:

      DrawMode7BG1HR_Normal1x1        0x5a9
      DrawMode7BG1HRAdd_Normal1x1     0x8a1
      DrawMode7BG1HRAddF1_2_Normal1x1 0x7b1
      DrawMode7BG2HR_Normal1x1        0x607
      DrawMode7BG2HRSubS1_2_Normal1x1 0xb60
      (all match pristine exactly)

  - 2 Renderers_ arrays present
  - Total Draw* count unchanged: 313
  - Total Renderers_* count unchanged: 45
  - No new warnings

Five M7 hires families are now fully de-templated: HR4X
(0cbca9e), BL4X (69f75a1), BL1X (0e147ff), BL (6fde805), HR
(`624492c`). The Mode 7 hires templating scaffold is gone:
the second `M7HIRES_ONLY` scope was the last consumer, and it's
now removed. Stage 2.6 (native Mode 7) is next.

This commit also updates docs/tile-untangle-state.md (in-tree)
to reflect Stage 2.5 completion.

### `6fde805` — tile.c: de-template Mode 7 BL renderers (Stage 2.4)

Stage 2.4. Same fan-out pattern as BL4X / BL1X: a local
`DEFINE_M7_BL_FAMILY` macro materializes 14 functions (7 math
variants x 2 BGs) using the parameterized helpers from `54c9153`.

BL is the original 2x bilinear-filtered Mode 7 path: two samples
per native pixel using half-step (aa_h = aa/2) sub-pixel division,
each blended through M7HR_SAMPLE_BILINEAR / M7HR_BLEND_AND_WRITE.
Differs from BL4X in step factor (1/2 instead of 1/4) and output
indexing (2*x and 2*x+1 instead of 4*x..4*x+3).

Files changed: `src/tile.c`, +250/-180, +70 net.

Removed:
  - templated `DrawMode7BG{1,2}BL` instantiation block
  - `DRAW_TILE_NORMAL_M7HIRES_BILINEAR` macro definition (~110
    lines, no longer referenced)
  - `ARGS / M7HIRES_ONLY` scope `#define`s (the BL block was the
    last user; HR4X / BL4X / BL1X / BL2X are now all de-templated)
  - dangling `#undef ARGS / #undef M7HIRES_ONLY / #undef
    DRAW_TILE_NORMAL_M7HIRES_BILINEAR`

Added:
  - de-templated section with banner comment explaining the family
  - `DEFINE_M7_BL_FAMILY` fan-out macro with parameter docs
  - 14 explicit `DrawMode7BG{1,2}BL*_Normal1x1` functions
  - 2 explicit `Renderers_DrawMode7BG{1,2}BLNormal1x1` arrays

Bundled stale-comment fixes (replaced as part of the section
banner rewrite):
  - The "BG2BL pins to low priority tier" claim at the head of
    the BL block was wrong post-`51b05ee` (the actual code uses
    the EXTBG per-pixel priority bit just like the other BG2
    paths). Banner now accurately describes the priority handling.
  - The "colour math is bypassed" claim was a duplicate of the
    one fixed in `54c9153`; deleted with the rest of the old
    header.

Symbol verification (vs pristine `0e147ff`):
  - 14 BL functions present, byte-identical machine code:

      DrawMode7BG1BLAdd_Normal1x1     0x32d1
      DrawMode7BG1BLAddF1_2_Normal1x1 0x2f81
      DrawMode7BG2BL_Normal1x1        0x2d0a
      DrawMode7BG2BLSubS1_2_Normal1x1 0x3db2
      (all match pristine exactly; same offsets too)

  - 2 Renderers_ arrays present
  - Total Draw* count unchanged: 313
  - Total Renderers_* count unchanged: 45
  - No new warnings

Behaviour invariants — to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean,
7-distinct-math-variants on BL.

This commit also updates docs/tile-untangle-state.md (in-tree)
to reflect Stage 2.4 completion.

### `0e147ff` — tile.c: de-template Mode 7 BL1X renderers (Stage 2.3)

Stage 2.3. Same fan-out pattern as HR4X / BL4X: a local
`DEFINE_M7_BL1X_FAMILY` macro materializes 14 functions (7 math
variants x 2 BGs) using the parameterized helpers from `54c9153`.

BL1X is the bilinear-filtered Mode 7 path at native (1x) width --
one sample per native pixel, but each sample goes through bilinear
blending. Used when the user enables bilinear without enabling
hires.

Differs from BL2X / BL4X in that there's no sub-pixel division of
aa/cc (no aa_q/aa_h locals) and output goes to GFX.S[Offset + x]
(no 2*x or 4*x sub-pixel index). The wrap path uses
M7HR_SAMPLE_BILINEAR; the repeat path inlines the LOOKUP_4 /
LOOKUP_4_FILL split for per-sample range checking.

Files changed: `src/tile.c`, +211/-133, +78 net.

Removed:
  - templated `DrawMode7BG{1,2}BL1X` instantiation block (~38 lines)
  - `DRAW_TILE_NORMAL_M7BL_1X` macro definition (~90 lines, no
    longer referenced)
  - dangling `#undef DRAW_TILE_NORMAL_M7BL_1X`

Added:
  - de-templated section with banner + fan-out macro doc
  - 14 explicit `DrawMode7BG{1,2}BL1X*_Normal1x1` functions
  - 2 explicit `Renderers_DrawMode7BG{1,2}BL1XNormal1x1` arrays

Symbol verification (vs pristine `bc47509`):
  - 14 BL1X functions present, byte-identical machine code
    (verified by comparing function sizes via nm --print-size)
  - 2 Renderers_ arrays present
  - Total Draw* count unchanged: 313
  - Total Renderers_* count unchanged: 45
  - No new warnings

Behaviour invariants — to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean,
7-distinct-math-variants on BL1X.

This commit also updates docs/tile-untangle-state.md (in-tree)
to reflect Stage 2.3 completion. Per the new convention, state
file updates ride along in the relevant commit instead of being
batched into separate "Update doc" commits.

### `69f75a1` — tile.c: de-template Mode 7 BL4X renderers (Stage 2.2)

Stage 2.2b. Same fan-out pattern as HR4X: a local
`DEFINE_M7_BL4X_FAMILY` macro materializes 14 functions (7 math
variants x 2 BGs) using the parameterized helpers from `54c9153`.

Files changed: `src/tile.c`, +217/-135, +82 net.

Removed:
  - templated `DrawMode7BG{1,2}BL4X` instantiation block
  - `DRAW_TILE_NORMAL_M7HIRES_4X_BILINEAR` macro definition (~100
    lines, no longer referenced)
  - dangling `#undef DRAW_TILE_NORMAL_M7HIRES_4X_BILINEAR`

Added:
  - de-templated section with banner + fan-out macro doc
  - 14 explicit `DrawMode7BG{1,2}BL4X*_Normal1x1` functions
  - 2 explicit `Renderers_DrawMode7BG{1,2}BL4XNormal1x1` arrays

Symbol verification (vs pristine `54c9153`):
  - 14 BL4X functions present, byte-identical machine code
  - 2 Renderers_ arrays present
  - Total Draw* count unchanged: 313
  - Total Renderers_* count unchanged: 45
  - No new warnings

The byte-identical machine code is the strongest possible signal
this is a pure refactor: the compiler produces the same .text
bytes from the explicit form as from the templated form because
the textual substitution chain produces the same expression tree.

Behaviour invariants — to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean,
7-distinct-math-variants on BL4X.

### `54c9153` — tile.c: parameterize Mode 7 bilinear helper macros (Stage 2.2 prep)

Stage 2.2a. Pure refactor in preparation for BL4X de-templating.
Six helper macros that the bilinear renderer paths share
(M7HR_LOOKUP_PIX, M7HR_LOOKUP_PIX_RAW, M7HR_LOOKUP_4,
M7HR_LOOKUP_4_FILL, M7HR_BLEND_AND_WRITE, M7HR_SAMPLE_BILINEAR)
previously read VRAM1, MASK, Z1, Z2, Offset, and MATH(...) from
enclosing scope via the level-1 templating's #define machinery.
They now take those values as explicit parameters.

Files changed: `src/tile.c`, +187/-74, +113 net.

Why: de-templated renderers (HR4X done; BL4X next) cannot rely on
enclosing-scope values because they're not inside the level-1
#define / #include / #undef scaffold. Parameterizing the helpers
lets the same macros serve both templated and de-templated
callers.

Updated:
  - Helper macros take new params: vram1, mask (LOOKUP_*),
    math_selector, math_op, z1_expr, z2_expr, offset (BLEND_AND_WRITE
    and SAMPLE_BILINEAR forwards everything).
  - Level-3 expansion now also defines `MATH_SELECTOR` and
    `MATH_OP` alongside `MATH` for each math variant.
  - 24 templated call sites in BL2X / BL4X / BL1X DRAW_TILE
    macros forward `VRAM1, MASK, MATH_SELECTOR, MATH_OP, Z1, Z2,
    Offset` from their enclosing scope.

Bundled fix: stale doc comment that claimed BL bypasses MATH
and the 7 variants are functionally identical (true before
`6d214cf` color-math fix; false now). Bundled because the
refactor changed the truth-value of the comment.

Verified byte-identical machine code for all BL functions vs
pristine `0cbca9e`. Symbol set unchanged (313 functions, 45
arrays).

### `0cbca9e` — tile.c: de-template Mode 7 HR4X renderers

Stage 2.1 deliverable. Converts `DrawMode7BG{1,2}HR4X` from the
3-level `#include "tile.c"` templating to explicit hand-written
code with C-comment documentation per the documentation rule.

Approach: a local `DEFINE_M7_HR4X_FAMILY(...)` macro fans out the
7 MATH variants per BG. Defined and undef'd within the section;
no new global macros. The fan-out preserves compile-time
specialization — each of the 14 functions gets the math op
resolved at compile time, no per-pixel runtime branch.

C comments: section banner explains the family at module level;
fan-out macro has a comment block explaining each parameter and
the body structure; inline comments call out non-obvious
arithmetic (quarter-step rounding leftover, BG2 priority bit
handling, wrap vs repeat-mode paths).

Files changed: `src/tile.c`, +294/-156, +138 net.

Removed:
  - templated `DrawMode7BG{1,2}HR4X` instantiation block (#define
    NAME1 / #include "tile.c" / #undef NAME1 pattern, x2)
  - `DRAW_TILE_NORMAL_M7HIRES_4X` macro definition (~110 lines,
    no longer referenced)
  - dangling `#undef DRAW_TILE_NORMAL_M7HIRES_4X` at level-1 close

Added:
  - new de-templated section (~280 lines incl. comments)
  - 14 explicit `static void DrawMode7BG{1,2}HR4X*_Normal1x1`
    functions
  - 2 explicit `static void (*Renderers_DrawMode7BG{1,2}HR4X
    Normal1x1[7])(...)` dispatch arrays

Symbol verification (build comparison vs pristine `b388159`):
  - 14 HR4X functions present, same names, same arity
  - 2 Renderers arrays present, same names, same 7 entries each
  - Total Draw* function count unchanged: 313 in both builds
  - Total Renderers_* array count unchanged: 45 in both builds
  - No new warnings; pre-existing 4 'defined but not used'
    warnings on `DrawMode7BG{1,2}Normal4x1` /
    `DrawMode7MosaicBG{1,2}Normal4x1` are still there
    (LTO-eliminated dead arrays, pre-existing condition).

Behaviour invariants — to be verified externally by Lib before
push: audio bit-identical, savestate compat, valgrind clean,
7-distinct-math-variants on HR4X.

### `b388159` — Document tile.c templating model and dispatch path

Stage 1 deliverable. Adds `docs/tile-internals.md` (531 lines).
Descriptive walkthrough of the 3-level `#include "tile.c"` scheme,
every NAME1 family, every NAME2 expansion, the 7 MATH ops, the
runtime `i`-index dispatch, the full symbol-count table including
LTO-eliminated arrays, and a worked expansion of
`DrawMode7BG1BLAddF1_2_Normal1x1`.

No code changes. Build verified clean. Audio/savestate/valgrind
invariants unaffected by definition.

### `5741db1` — tile.c: M7HR snap branches use floor (<=) at fractional 128 boundary

Fixes the Tiny Toons "Buster Busts Loose" title-screen 1-scanline seam
in smooth bilinear mode (Mode7HiresBilinear == 2). 4-character code
change in `m7hr_blend()`'s HEDGE/VEDGE/NEAREST branches: strict
less-than (`<`) flipped to floor (`<=`), with `>=` flipped to `>` to
keep the partition disjoint.

**Mechanism.** When MatrixD scales to ~0.5 source-rows-per-output-line
(common for 1x SNES content rendered at 4x output), every other output
line samples Yf exactly 128. The disconaware classifier's HEDGE/VEDGE/
NEAREST branches snapped to ceiling at this midpoint via strict-less-
than. On the Tiny Toons rainbow ring, the ceiling row at y=156 contains
artistically-unrelated palette content (one-row band of cyan/white
mid-rainbow), so the entire output line jumped to a wrong-colour row.
Floor-snap matches what stable mode does (always uses floor-Y) and
removes the seam without affecting non-boundary pixels.

**Verification.** Per-pixel ground-truth dumps from BG1BL4X via the
debug-dump infrastructure (separate, never committed). At y=156 (357
M7-only pixels), L1 vs stable dropped from 37,961 (pre-fix) to 4,776
(post-fix) — equivalent to "no seam visible". Benign-row L1 cost ~2
per pixel-channel summed over non-seam rows, below visual
discrimination. Lib visually verified on real hardware.

**Diagnostic infrastructure produced during this investigation,
NOT committed:**
  * `m7-debug-dump.patch` — gated by `-DM7HR_DEBUG_DUMP`, captures
    per-output-pixel records (line, out_x, four palette indices, four
    resolved RGB565 colours, Xf, Yf, classifier branch ID) from the
    BG1BL4X wrap-path renderer. Triggered by creating `m7-dump-now`
    in `M7HR_DUMP_DIR` (env, defaults `/tmp` POSIX or `.` Windows).
    Portable, MinGW-clean. Debug release `.o` byte-identical to
    pristine.
  * `dump_reader.py` decodes the binary format. Reusable for any
    future M7 BL bug.

Both kept on Lib's side as tools, not in-tree.

### `ee7ebac` — tile.c: replace M7HR smooth classifier with vertical-row-contrast guard

(The actual upstream commit subject is the body's first paragraph
because the `git commit` invocation pasted the entire message as one
block. Header above is the suggested-but-not-applied subject.)

Replaces the discontinuity-aware classifier (4 SAD evaluations + 4-way
HEDGE/VEDGE/NEAREST/4C decision tree per opaque pixel) with plain
4-corner bilinear plus a single vertical-row-contrast guard.

**Mechanism.** When the sum of two vertical SAD pairs (TL<->BL,
TR<->BR) exceeds `M7HR_VROW_THRESHOLD=24`, fall back to stable's
X-only blend on the top row instead of producing a muddy averaged
scanline. Otherwise full 4C bilinear.

**Why this and not the classifier.** Real-hardware A/B testing
(Contra III "Choose Your Starting Point" and gameplay; Tiny Toons
title) showed the classifier's output on busy pixel-art was nearly
indistinguishable from nearest-neighbour while costing +2.25 ms/frame
over stable. Plan B (unguarded plain 4C, rejected) confirmed plain 4C
was visually equivalent on Contra at 4.06ms but reintroduced the
muddy-average Tiny Toons seam at y=156. Plan C (this commit) keeps
plain 4C on the common case but guards specifically against the
high-contrast-row-pair situation that produces the muddy seam.

**Real-hardware results:**
  * Contra III Choose Your Starting Point smooth: 5.75ms → 5.03ms
    (-0.72ms, -12%).
  * Tiny Toons title screen smooth: rainbow rings continuous across
    y=156, no muddy seam, frame-rate locked at 60Hz.
  * Stable mode unchanged within run-to-run variance.

**Offline rig prediction matched real hardware.** Threshold=24 routed
96.7% of seam-row pixels to stable fallback while leaving 85-92% of
benign-row pixels on plain 4C.

**Cleanup bundled in same commit:** `M7HR_NEAR` /
`M7HR_NEAR_THRESHOLD` macros removed; stale `always_inline` claim on
`m7hr_blend()` preamble updated; user-facing libretro option
description, `snes9x.h` `Mode7HiresBilinear` doc, `M7HR_BLEND_RGB_4C`
macro doc, and HR4X dispatch comment updated to describe the guarded
smooth path. Net -75 lines across `tile.c` / `snes9x.h` /
`libretro_core_options.h`; tile.c .o is -520 bytes at -O2.

**Investigation artifacts (kept on Lib's side, not in-tree):**
`m7-debug-dump.patch`, `dump_reader.py`, the offline render rig,
the four-candidate threshold sensitivity sweep that informed
picking 24.

### `da587d9` — tile.c: M7HR smooth fast-path on palette-row-equal pixels

Adds a palette-row-equality short-circuit at the top of the smooth
all-opaque path. When `p_tl == p_bl && p_tr == p_br`, the source
rows are byte-identical in palette indices (so necessarily
byte-identical in resolved RGB). 4C blend reduces algebraically to
X-only blend on the top row in this case — detect with a 2-byte
palette comparison and route directly there, skipping two
`GFX.ScreenColors` lookups, the row-contrast SAD math, and six of
eight multiplies in the 4C blend.

**Subsumes** the prior all-four-equal early-out (every all-equal
pixel is row-equal; the inner `p_tl==p_tr` check in the row-equal
branch collapses those cases to a single palette lookup with no
blend math).

**Correctness-equivalent** when the precondition holds: X-only blend
produces byte-identical RGB565 output to 4C. Verified offline across
1000 random color/weight cases.

**Real-hardware ms on the test scenes:**
  * Contra III Choose Your Starting Point smooth: 5.03ms → 4.86ms
    (-0.17ms).
  * Tiny Toons title screen smooth: 3.75ms → 3.49ms (-0.26ms).

Hit rate is content-dependent: ~26% of opaque pixels on the Tiny
Toons title dump; higher on flat-content scenes (M7 sky, large flat
regions). Mode 7 visuals unchanged on all scenes.

Patch: +28/-5 lines, `m7hr_blend()` smooth path only. tile.c .o is
byte-identical to `ee7ebac` master at -O2 (m7hr_blend symbol -21
bytes from removing the redundant all-equal early-out).

### `9f5ecb7` — tile.c: split m7hr_blend into smooth/stable specialized helpers

Splits the combined `m7hr_blend()` into two specialized helpers
selected at the macro level:

  * `m7hr_blend_smooth(p_tl, p_tr, p_bl, p_br, Xf, Yf, *out)` —
    4-corner bilinear with row-contrast guard + palette-row-equality
    short-circuit + 4-corner alpha-aware blend.
  * `m7hr_blend_stable(p_tl, p_tr, p_bl, p_br, Xf, *out)` — X-only
    blend on top row + horizontal alpha-aware blend. Yf dropped from
    signature (unused there).

`M7HR_BLEND_AND_WRITE` macro now contains a single
`if (smooth_arg) call smooth else call stable` dispatch per pixel.

**.o-level measurements (-O2, no LTO):**
  * Combined helper bytes: smooth 1214 + stable 441 = 1655 (vs 1457
    in `da587d9` for combined). +198 bytes from duplicated op_mask
    and early-out logic across the two helpers.
  * tile.o: +1528 bytes total.
  * Per-renderer (`DrawMode7BG1BL4X_Normal1x1`): +93 bytes; `cmpl
    $0x2` count 5 (split) vs 1 (combined). The compiler did NOT
    hoist the loop-invariant `smooth` check out of the inner pixel
    loop — dispatch branch is emitted at each macro expansion site.

**Real-hardware ms on the test scenes:**
  * Contra III Choose Your Starting Point stable: 3.50ms → 3.39ms
    (-0.11ms, only clearly-out-of-noise signal).
  * Contra III Choose Your Starting Point smooth: 4.86ms → 4.81ms
    (within noise).
  * Tiny Toons title smooth: 3.49ms → 3.46ms (within noise).

Stable mode benefits most because its helper body is 3.3× smaller
than the combined helper. Smooth path is essentially flat — the
4C blend math which dominates wasn't touched. Visual output
unchanged.

The "compiler will specialize the inner loop on the loop-invariant
`smooth` value once we split" thesis didn't pan out — to force a
true hoist, dispatch would need to move outside the inner pixel
loop (42 renderer functions worth of duplicated loop bodies).
That bigger surgery is parked unless a further perf gap
warrants it.

### `145be5a` — Mode 7 hires vertical doubling (2x_hv / 4x_hv option values)

Adds vertical doubling to the M7 hires render path. New libretro
option values `"2x_hv"` (512×448) and `"4x_hv"` (1024×448) extend
the existing `"2x"` / `"4x"` horizontal-only modes. New
`Settings.Mode7HiresVertical` field stores the flag.

**Approach: end-of-frame post-pass.** All renderers (M7 BG, sprites,
HUD, BG2, color math) keep running normally at 224 rows. After the
final FLUSH_REDRAW, `S9xMode7VertResample` walks the buffer
bottom-up and expands it in place to 448 rows. Two regions:

  * Rows `[0, M7VertStartY)`: HUD / Mode 1 area. Each input row is
    duplicated to two output rows (chunky 2x upscale of HUD text).
  * Rows `[M7VertStartY, ScreenHeight)`: M7 plane area. Output row
    `2y` copies input row `y` verbatim; output row `2y+1` is the
    per-channel average of input rows `y` and `y+1` (RGB565 packed
    blend via `((a & 0xF7DE) >> 1) + ((b & 0xF7DE) >> 1)`).

Bottom-up walk ensures source rows aren't overwritten before they're
read: for source row `y`, output addresses `2y` and `2y+1` are both
≥ `y`; source `y+1` (read for the blend) sits at offset `y+1` which
is < `2y+2` for all `y >= 0`, so earlier iterations don't touch it.

`M7VertStartY` is set by two hooks: cpuexec.c frame-start setup
(when frame begins in BG mode 7 with hires) sets it to 0; ppu.c
mid-frame mode-switch hook (the existing width-promotion path) sets
it to `GFX.StartY` for split-screen frames where frame starts in
Mode 1 and switches to 7 mid-frame. Reset to -1 every frame so the
post-pass is a no-op when not armed.

**Why a post-pass instead of in-renderer vert-2x.** The earlier
attempt modified the 28 BL/BL4X renderers to write two output rows
per source line. This worked for the M7 plane but broke every other
renderer in the pipeline: sprites, HUD, BG2, OBJ, and color math
all wrote to 224-row Y coordinates while the buffer was 448 rows
tall. Sprites landed at half their expected height; HUD got covered
by stale buffer content as the depth test failed against
uninitialized Z values in rows 224..447. The post-pass approach
keeps the entire pipeline at 224 rows and only expands at the very
end, so nothing else needs to know about the doubling.

**Visual cost vs. in-renderer interpolation.** A "real" vertical
upscale (e.g. bsnes HD) re-runs the M7 perspective transform per
output line at sub-source-Y positions, producing per-line samples
that are geometrically correct and have new detail between source
rows. The post-pass instead averages two adjacent rendered rows
(each produced by integer-Y sampling). Result: row-to-row
transitions get smoothed (finish-line stripes, track-edge tiles),
but the M7 plane still samples at 224 source rows so per-line
texture detail isn't added. Visibly lower quality than a true
re-render approach, but ships correctly and doesn't disturb the
rest of the render pipeline. The first shipped attempt did go in
the in-renderer direction (28 BL/BL4X renderers modified to write
two output rows per source line) and broke when the non-M7 layers
(sprites, HUD, BG2, OBJ, color math) wrote to 224-row Y
coordinates while the buffer was 448 rows tall — sprites at half
their expected height, HUD covered by stale buffer content, etc.
Resolving that properly would require deferring the M7 BG to a
separate post-FLUSH_REDRAW pass with its own depth handling,
which is a larger restructuring than this commit attempts.

**Effective regardless of bilinear setting.** Stable, smooth, or HR
(no-bilinear) all benefit: the post-pass interpolates whatever the
renderer wrote. Smoothing along the Y axis applies on top of
whatever X-axis treatment the bilinear setting selects.

**.o-level cost (-O2):**
  * ppu.o: +992 bytes (S9xMode7VertResample is ~373 bytes; the rest
    is the new IPPU.M7VertStartY field touches and the mid-frame
    arm hook).
  * cpuexec.o: +408 bytes (frame-start arm + post-pass call).
  * tile.o: unchanged (no renderer modifications in this approach).

Total binary cost ~+1.4KB. About 7× smaller than the earlier
in-renderer approach (~+10KB).

**Per-frame perf cost.** ~225K pixel writes for the bilinear region
(224 rows × 1024 cols, RGB565 averaging) plus ~450K for the row-
replication regions. On weak hardware, under 1ms per frame. The
in-renderer approach roughly doubled the M7 renderer's per-frame
work, so this is roughly 3-5× cheaper.

## In-flight notes

### Performance claw-back — followups, prioritized

The originally-scoped options 1 and 2 are both shipped. Remaining:

**1. Pack RGB565 for SIMD-style SAD (real work, harder to verify).**
   `M7HR_COLOR_DIST` does 3 IABS+sums on 5-bit-extracted channels.
   On x86 with SSE2, packed-byte saturated subtraction can compute
   absolute differences in parallel. Worth ~2-3x speedup on the SAD
   itself. But snes9x2010 targets weak hardware where this kind of
   intrinsic isn't always available, and getting it right portably
   is fiddly. Defer until threshold of pain warrants it.

**2. Tune `M7HR_VROW_THRESHOLD` against multi-game dumps (real work,
   real payoff).** Lower threshold = more pixels route to cheap
   stable fallback, fewer pay 4C math cost (8 multiplies → 2
   multiplies). Higher threshold = smoother look on borderline
   content. Currently 24, set against one Tiny Toons dump. With
   dumps from 2-3 additional M7 scenes the threshold could be tuned
   across content types. Expected: tuned threshold could land smooth
   perf within ~0.3ms of stable on most content.

**3. Move smooth/stable dispatch outside the inner pixel loop.**
   Big surgery -- 42 renderer functions, either by hand or via a
   wrapping macro that duplicates the loop body. Would force the
   compiler hoist that the macro-level dispatch in `9f5ecb7` failed
   to produce. Hold off unless there's a compelling reason to claw
   back further perf.

## Open questions for Lib

  - **Multi-game dump capture:** which 2-3 M7 games would you pick
    as representative? F-Zero / Pilotwings / Mario Kart are
    suggestions but you know your test rotation better.

## Open followups (deferred from in-flight work, not bundled)

**Combined Mode 7 option-string refactor.** The current setup has
two independent options — `snes9x_2010_mode7_hires` (off/2x/4x/
2x_hv/4x_hv) and `snes9x_2010_mode7_hires_bilinear` (off/stable/
smooth) — that the user has to set in tandem. Some combinations
are more meaningful than others (e.g. `4x_hv + smooth` vs.
`disabled + smooth`), and the implicit "this combo does X, that
combo does Y" relationships aren't visible in the menu. Lib
proposed collapsing them into one option with explicit combined
values: `off`, `2x`, `2x + bilinear stable`, `2x + bilinear smooth`,
`4x + bilinear smooth`, `2x_hv + bilinear smooth`, `4x_hv +
bilinear smooth`, etc. Roughly 9-12 distinct values. Needs back-
compat migration for users with existing `.opt` files (read both
old keys on startup, combine into new value, write new key once).
Deferred until someone has the appetite for the migration code.

**Higher-quality in-renderer vert-2x.** The post-pass approach in
`145be5a` averages two adjacent rendered rows, which smooths
row-to-row transitions but does not add per-line texture detail.
A bsnes-HD-style approach — re-running the M7 perspective
transform per output line at sub-source-Y positions — would
produce per-line samples with new detail between source rows. The
first attempt at this broke when non-M7 layers (sprites, HUD, BG2,
OBJ, color math) wrote to 224-row Y coordinates while the M7 plane
was writing into a 448-row buffer. Resolving it cleanly requires
deferring the M7 BG renderer to a separate post-FLUSH_REDRAW pass
with its own depth buffer and layered-composite step; substantial
restructuring. Parked unless a user case warrants the visual
upgrade vs. the implementation cost.

(Earlier note: the BL color-math stale comment was fixed as part
of `54c9153` since the macro refactor in that commit changed the
truth-value of the comment.)

---

*Last updated: rebase converting the `<TBD>` entry for `145be5a` (Mode 7 hires vertical doubling, 2x_hv / 4x_hv option values) into a finalized history entry, and adding two Open followups: the combined-option-string refactor (collapsing `mode7_hires` + `mode7_hires_bilinear` into one option with explicit combined values) and the higher-quality in-renderer vert-2x (deferred until a user case warrants the substantial restructuring it would need).*