//! `SwitchRenderBackend` — Ruffle `RenderBackend` impl backed by switch-mesa GL.
//!
//! Phase 1.3 complete (2026-05-23). Three shader programs cover the bulk of
//! Flash's 2D rendering needs:
//!
//!   - **solid**:    per-vertex (pos.xy, rgba) + uniform `u_world` + color
//!                   transform (`u_mult`, `u_add`). Drives `RenderShape`
//!                   solid fills, `DrawRect`, and `DrawLine`/`DrawLineRect`.
//!   - **bitmap**:   per-vertex (pos.xy, uv.xy), samples `u_tex` and applies
//!                   color transform. Drives `RenderBitmap` (and bitmap
//!                   fills inside `RenderShape` once 1.3.6.b lands).
//!   - **gradient**: per-vertex (pos.xy), samples a 256x1 gradient ramp
//!                   texture indexed by `t` computed from a per-draw
//!                   `u_grad_local` matrix; supports linear/radial/focal
//!                   (focal currently approximated as radial) and the three
//!                   spread modes (pad, reflect, repeat).
//!
//! Masking uses the framebuffer stencil buffer (`EGL_STENCIL_SIZE=8` in
//! `gl_context.cpp`). The four mask commands (`push_mask`, `activate_mask`,
//! `deactivate_mask`, `pop_mask`) track a depth counter and toggle
//! `glColorMask`/`glStencilFunc`/`glStencilOp` accordingly.
//!
//! Coordinate convention:
//!   - Tessellator outputs vertex positions in *pixels* (lyon point2 of
//!     twips_to_pixels). Flash `Transform.matrix.tx/ty` are also converted
//!     to pixels before being placed in the world matrix. Then a final
//!     pixels → NDC step maps screen pixels (origin top-left, Y down) to
//!     OpenGL clip space (-1..1, Y up).

use std::any::Any;
use std::borrow::Cow;
use std::cell::Cell;
use std::num::NonZeroU32;
use std::sync::Arc;

use ruffle_render::backend::{
    BitmapCacheEntry, Context3D, Context3DProfile, PixelBenderOutput, PixelBenderTarget,
    RenderBackend, ShapeHandle, ShapeHandleImpl, ViewportDimensions,
};
use ruffle_render::bitmap::{
    Bitmap, BitmapFormat, BitmapHandle, BitmapHandleImpl, BitmapSource, PixelRegion, PixelSnapping,
    RgbaBufRead, SyncHandle,
};
use ruffle_render::commands::{CommandHandler, CommandList, RenderBlendMode};
use ruffle_render::error::Error;
use ruffle_render::filters::Filter;
use ruffle_render::matrix::Matrix;
use ruffle_render::pixel_bender::{PixelBenderShader, PixelBenderShaderHandle};
use ruffle_render::pixel_bender_support::PixelBenderShaderArgument;
use ruffle_render::quality::StageQuality;
use ruffle_render::shape_utils::{DistilledShape, GradientType};
use ruffle_render::tessellator::{DrawType, Gradient, ShapeTessellator};
use ruffle_render::transform::Transform;
use std::sync::atomic::{AtomicU16, AtomicUsize, Ordering};
use std::sync::Mutex;

/// Pause-menu labels rendered by `draw_menu_overlay`. C++ maps the selected
/// index from this slice to an action (Resume / Touches / Restart / Quit).
/// Keep the order in sync with the `MENU_*` constants in `cpp/src/main.cpp`.
pub const MENU_ITEMS: &[&str] = &["REPRENDRE", "TOUCHES", "REDEMARRER", "QUITTER"];

/// 5×7 pixel glyphs for the pause menu. ASCII art keeps the data
/// hand-editable: each row is exactly 5 chars wide, ' ' = off, anything
/// else = on. `draw_text` upper-cases input before lookup, so we only
/// carry one case. Unknown chars render as blank (the cursor still
/// advances). Add more entries here if a future label needs new
/// characters.
type Glyph = [&'static str; 7];
static GLYPHS: &[(char, Glyph)] = &[
    (' ', ["     ", "     ", "     ", "     ", "     ", "     ", "     "]),
    ('A', [" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"]),
    ('B', ["#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "]),
    ('C', [" ####", "#    ", "#    ", "#    ", "#    ", "#    ", " ####"]),
    ('D', ["#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### "]),
    ('E', ["#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"]),
    ('F', ["#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "]),
    ('G', [" ####", "#    ", "#    ", "#  ##", "#   #", "#   #", " ####"]),
    ('H', ["#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"]),
    ('I', [" ### ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "]),
    ('J', ["#####", "    #", "    #", "    #", "    #", "#   #", " ### "]),
    ('K', ["#   #", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "#   #"]),
    ('L', ["#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"]),
    ('M', ["#   #", "## ##", "# # #", "#   #", "#   #", "#   #", "#   #"]),
    ('N', ["#   #", "##  #", "# # #", "#  ##", "#   #", "#   #", "#   #"]),
    ('O', [" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "]),
    ('P', ["#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "]),
    ('Q', [" ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #"]),
    ('R', ["#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"]),
    ('S', [" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "]),
    ('T', ["#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "]),
    ('U', ["#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "]),
    ('V', ["#   #", "#   #", "#   #", "#   #", "#   #", " # # ", "  #  "]),
    ('W', ["#   #", "#   #", "#   #", "#   #", "# # #", "## ##", "#   #"]),
    ('X', ["#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"]),
    ('Y', ["#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  "]),
    ('Z', ["#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####"]),
    ('0', [" ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### "]),
    ('1', ["  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "]),
    ('2', [" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"]),
    ('3', [" ### ", "#   #", "    #", "  ## ", "    #", "#   #", " ### "]),
    ('4', ["#  # ", "#  # ", "#  # ", "#####", "   # ", "   # ", "   # "]),
    ('5', ["#####", "#    ", "#### ", "    #", "    #", "#   #", " ### "]),
    ('6', [" ### ", "#    ", "#    ", "#### ", "#   #", "#   #", " ### "]),
    ('7', ["#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   "]),
    ('8', [" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "]),
    ('9', [" ### ", "#   #", "#   #", " ####", "    #", "    #", " ### "]),
    ('-', ["     ", "     ", "     ", "#####", "     ", "     ", "     "]),
    ('=', ["     ", "     ", "#####", "     ", "#####", "     ", "     "]),
    ('>', ["#    ", " #   ", "  #  ", "   # ", "  #  ", " #   ", "#    "]),
    (':', ["     ", "  #  ", "  #  ", "     ", "  #  ", "  #  ", "     "]),
    ('.', ["     ", "     ", "     ", "     ", "     ", " ##  ", " ##  "]),
    ('/', ["    #", "    #", "   # ", "  #  ", " #   ", "#    ", "#    "]),
];

/// Count of `GpuDraw`s currently alive (created minus dropped). Used to
/// detect leaks: if this monotonically grows (and matches `shapes_registered`
/// minus shape Drops), Ruffle is retaining shape handles forever and our
/// VBO/VAO/IBO pool fills up — exactly the suspected cause of the jetpack
/// crash (rocket nozzle particle system emits a new shape per frame, never
/// freed, until Mesa's bind table walks off the end and faults).
static LIVE_GPU_DRAWS: AtomicUsize = AtomicUsize::new(0);
/// Count of `GpuShape`s currently alive (created minus dropped). Should
/// roughly track `register_shape` calls if Ruffle never drops handles.
static LIVE_GPU_SHAPES: AtomicUsize = AtomicUsize::new(0);

// ─── Mega-buffer arena ─────────────────────────────────────────────────────
//
// Mario 63 + rocket nozzle = ~3 new shapes per frame, never freed by Ruffle
// for several seconds. Each shape used to create its own VBO + IBO + VAO,
// and Mesa-NVK on Tegra X1 segfaults inside `glBindBuffer` once we exceed
// ~27 000 simultaneously-live GL buffer handles (we caught it twice:
// x24=GL_ARRAY_BUFFER, FAR a poisoned slot pointer at offset 0x50 of a
// table, then a small index 0x1011 — Mesa's internal buffer slot table
// has a finite size which we walked off the end of).
//
// The fix is to stop creating GL objects per shape entirely: allocate one
// huge VBO and one huge IBO at boot, then suballocate ranges out of those
// for each shape via a freelist. From Mesa's point of view there are only
// ~5 GL handles total, no matter how many Ruffle shapes pile up.
//
// `glDrawElementsBaseVertex` lets us pack many shapes into a single VBO
// while letting each shape keep its own local 0..N index numbering: the
// driver shifts every fetched index by `base_vertex` before reading.
//
// Sizing: at the crash we had ~14 MB of vertex data + ~3 MB of indices
// in flight. We size for ~4x headroom so a long Mario 63 session has
// plenty of slack.
const ARENA_VBO_SIZE: GLsizeiptr = 64 * 1024 * 1024;  // 64 MB
// IBO bumped to 32 MB after the first arena test (jetpack run 2026-05-25):
// after ~5 minutes of Mario 63 the index arena peaked at 13 MB (81 %) —
// uncomfortably close to OOM. 32 MB gives us 2× headroom for longer
// sessions and more index-heavy levels.
const ARENA_IBO_SIZE: GLsizeiptr = 32 * 1024 * 1024;  // 32 MB
/// VBO alignment = one full vertex (pos.xy + rgba = 6 × f32 = 24 bytes).
/// MUST match the vertex stride so `glDrawElementsBaseVertex(base_vertex)`
/// can use `vbo_offset / 24` and land exactly on a vertex boundary.
/// First mega-arena attempt used 16-byte alignment (a power of two for
/// `&!(align-1)` rounding) — that produced offsets like 48, 64, 80 which
/// are NOT multiples of 24, so base_vertex was off by fractional vertices
/// and Mario 63 rendered as a corrupted mess. Round-up logic switched to
/// the generic `((x + a - 1) / a) * a` to allow non-power-of-2 alignments.
const ARENA_VBO_ALIGN: GLsizeiptr = 24;
/// IBO alignment = sizeof(u32). `glDrawElementsBaseVertex`'s `indices` byte
/// offset must be aligned to the index type (4 bytes for GL_UNSIGNED_INT).
const ARENA_IBO_ALIGN: GLsizeiptr = 4;

struct BufferArena {
    gl_id: GLuint,
    target: GLenum,
    capacity: GLsizeiptr,
    /// Alignment for allocations in this arena (24 for vertex, 4 for index).
    align: GLsizeiptr,
    /// Free segments, sorted by offset, adjacent ones coalesced.
    free: Vec<(GLintptr, GLsizeiptr)>,
    /// High-water diagnostic: max bytes ever in use simultaneously.
    peak_in_use: GLsizeiptr,
    /// Failed-allocation diagnostic: when we ran out, log it once.
    oom_warned: bool,
}

impl BufferArena {
    fn new(target: GLenum, capacity: GLsizeiptr, align: GLsizeiptr) -> Self {
        let mut gl_id: GLuint = 0;
        unsafe {
            glGenBuffers(1, &mut gl_id);
            glBindBuffer(target, gl_id);
            glBufferData(target, capacity, core::ptr::null(), GL_DYNAMIC_DRAW);
            glBindBuffer(target, 0);
        }
        Self {
            gl_id,
            target,
            capacity,
            align,
            free: std::vec![(0 as GLintptr, capacity)],
            peak_in_use: 0,
            oom_warned: false,
        }
    }

    /// Allocate `size` bytes (rounded up to `self.align`). First-fit. Returns
    /// the byte offset, or `None` if the arena is full.
    fn alloc(&mut self, size: GLsizeiptr) -> Option<GLintptr> {
        let size = ((size + self.align - 1) / self.align) * self.align;
        for i in 0..self.free.len() {
            let (off, sz) = self.free[i];
            if sz >= size {
                let alloc_off = off;
                if sz == size {
                    self.free.remove(i);
                } else {
                    self.free[i] = (off + size, sz - size);
                }
                let in_use = self.capacity - self.free_bytes();
                if in_use > self.peak_in_use {
                    self.peak_in_use = in_use;
                }
                return Some(alloc_off);
            }
        }
        if !self.oom_warned {
            self.oom_warned = true;
            let msg = std::format!(
                "ARENA OOM: target=0x{:04X} capacity={} requested={} peak_in_use={}\n",
                self.target, self.capacity, size, self.peak_in_use,
            );
            let mut bytes = msg.into_bytes();
            bytes.push(0);
            unsafe { ruffle_log_cstr(bytes.as_ptr() as *const _) };
        }
        None
    }

    /// Free a previously-allocated region. Size MUST match the alloc size
    /// (after alignment rounding) — caller is responsible.
    fn free_region(&mut self, offset: GLintptr, size: GLsizeiptr) {
        let size = ((size + self.align - 1) / self.align) * self.align;
        let insert_idx = self.free.partition_point(|(off, _)| *off < offset);
        self.free.insert(insert_idx, (offset, size));
        // Coalesce with next.
        if insert_idx + 1 < self.free.len() {
            let (off, sz) = self.free[insert_idx];
            let (next_off, next_sz) = self.free[insert_idx + 1];
            if off + sz == next_off {
                self.free[insert_idx] = (off, sz + next_sz);
                self.free.remove(insert_idx + 1);
            }
        }
        // Coalesce with previous.
        if insert_idx > 0 {
            let (prev_off, prev_sz) = self.free[insert_idx - 1];
            let (off, sz) = self.free[insert_idx];
            if prev_off + prev_sz == off {
                self.free[insert_idx - 1] = (prev_off, prev_sz + sz);
                self.free.remove(insert_idx);
            }
        }
    }

    fn upload(&self, offset: GLintptr, data: &[u8]) {
        unsafe {
            glBindBuffer(self.target, self.gl_id);
            glBufferSubData(
                self.target,
                offset,
                data.len() as GLsizeiptr,
                data.as_ptr() as *const _,
            );
        }
    }

    fn free_bytes(&self) -> GLsizeiptr {
        self.free.iter().map(|(_, sz)| *sz).sum()
    }

    fn in_use_bytes(&self) -> GLsizeiptr {
        self.capacity - self.free_bytes()
    }
}

impl Drop for BufferArena {
    fn drop(&mut self) {
        unsafe { glDeleteBuffers(1, &self.gl_id) };
    }
}

// ─── Pending frees queue ────────────────────────────────────────────────────
//
// `GpuDraw::drop` runs without access to the SwitchRenderBackend (it's just
// triggered by Arc reference count going to zero, anywhere Ruffle decides
// to release a ShapeHandle). We can't free arena regions directly from the
// Drop — they'd need &mut to the arena. Instead, Drop enqueues
// (offset, size) tuples here, and submit_frame drains them at the top of
// each frame, calling `BufferArena::free_region`.
struct PendingFree {
    vbo_offset: GLintptr,
    vbo_size: GLsizeiptr,
    ibo_offset: GLintptr,
    ibo_size: GLsizeiptr,
}
static PENDING_FREES: Mutex<Vec<PendingFree>> = Mutex::new(Vec::new());
use swf::{BlendMode, Color, GradientSpread};

use crate::ffi::gl::*;
use crate::query_ram;

extern "C" {
    fn ruffle_log_cstr(msg: *const core::ffi::c_char);
    /// Monotonic tick counter (armGetSystemTick). Used for FPS heartbeat.
    fn ruffle_tick_now() -> u64;
    /// Tick frequency in Hz (~19.2 MHz on Switch). Constant after boot.
    fn ruffle_tick_freq() -> u64;
    /// Actual current CPU clock in Hz (clkrst). 0 if unavailable. Lets the
    /// heartbeat confirm whether CpuBoostMode is holding the A57 at 1785 MHz.
    fn ruffle_cpu_clock_hz() -> u32;
    /// 1 when docked, 0 handheld.
    fn ruffle_is_docked() -> core::ffi::c_int;
}

fn log(nul_terminated: &[u8]) {
    unsafe { ruffle_log_cstr(nul_terminated.as_ptr() as *const _) };
}

// ─── Per-frame backend-primitive timing (FPS-spike attribution) ──────────────
//
// Question we want answered: when `tick` spikes to ~1.3 s on one frame, is it
// OUR backend (a readback/upload/blit stalling the GPU) or pure AVM2 bytecode
// execution (upstream Ruffle)? We time the primitives Ruffle calls DURING
// player.tick() — render_offscreen (incl. the draw() repatriation), bitmap
// register/upload, copyPixels resolve — and surface them in the slow-frame line.
// A slow frame with huge `tick` but ~0 primN_us is pure AVM2; one where a primN
// dominates is a backend culprit we can fix.
//
// CUR_* accumulate within a frame via `PrimTimer` guards. submit_frame (which
// runs right after player.tick) snapshots CUR into LAST and zeroes CUR; the
// slow-frame logger then reads LAST. Raw ticks (~19.2 MHz), µs at display.
static PRIM_OFFSCREEN_CUR: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFFSCREEN_LAST: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_BMPUP_CUR: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_BMPUP_LAST: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_RESOLVE_CUR: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_RESOLVE_LAST: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
// DIAG (2026-06-03, catmario perf): sub-phase breakdown of render_offscreen,
// which dominates frame time at ~330ms when cacheAsBitmap-heavy AS3 games run.
// ALLOC=make_standalone_texture, RENDER=render_commands_to_texture,
// READBACK=glReadPixels (atlas-slot repatriate), UPLOAD=atlas.upload_region.
// N=call count this frame, PIX=sum of readback-region pixels (glReadPixels cost).
static PRIM_OFF_ALLOC_CUR: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFF_ALLOC_LAST: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFF_RENDER_CUR: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFF_RENDER_LAST: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFF_READBACK_CUR: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFF_READBACK_LAST: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFF_UPLOAD_CUR: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFF_UPLOAD_LAST: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFF_N_CUR: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFF_N_LAST: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFF_PIX_CUR: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
static PRIM_OFF_PIX_LAST: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);

/// RAII guard: adds elapsed ticks to a static on drop, covering every
/// early-return path of the timed function automatically.
struct PrimTimer {
    start: u64,
    acc: &'static std::sync::atomic::AtomicU64,
}
impl PrimTimer {
    fn new(acc: &'static std::sync::atomic::AtomicU64) -> Self {
        PrimTimer { start: unsafe { ruffle_tick_now() }, acc }
    }
}
impl Drop for PrimTimer {
    fn drop(&mut self) {
        let elapsed = unsafe { ruffle_tick_now() }.saturating_sub(self.start);
        self.acc.fetch_add(elapsed, std::sync::atomic::Ordering::Relaxed);
    }
}

// ─── GPU resources ────────────────────────────────────────────────────────────

// ─── Texture atlas ─────────────────────────────────────────────────────────
//
// Mario 63 (and likely many other Flash games) register hundreds of small
// bitmaps. One GL texture per bitmap exhausts driver resources on Tegra X1
// — a deterministic crash at ~600 textures was bisected on 2026-05-24.
//
// Atlas: a single 2048x2048 RGBA texture (16 MB) packed with a shelf-based
// allocator. New atlases are added when the current one fills up. Each
// bitmap becomes a sub-rectangle (u0,v0)–(u1,v1) of one atlas.

const ATLAS_SIZE: u32 = 2048;
const ATLAS_PAD: u32 = 1; // 1 px padding around each bitmap to avoid bleed

struct Shelf {
    y: u32,
    height: u32,
    used_w: u32,
}

struct Atlas {
    texture: GLuint,
    width: u32,
    height: u32,
    shelves: Vec<Shelf>,
}

impl Atlas {
    fn new(size: u32) -> Self {
        let mut tex: GLuint = 0;
        unsafe {
            glGenTextures(1, &mut tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA8 as GLint,
                size as GLsizei,
                size as GLsizei,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                core::ptr::null(),
            );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR as GLint);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR as GLint);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE as GLint);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE as GLint);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        Self {
            texture: tex,
            width: size,
            height: size,
            shelves: Vec::new(),
        }
    }

    /// Try to allocate a `w×h` region (plus padding). Returns the content
    /// origin (without padding).
    fn pack(&mut self, w: u32, h: u32) -> Option<(u32, u32)> {
        let w_full = w + 2 * ATLAS_PAD;
        let h_full = h + 2 * ATLAS_PAD;
        if w_full > self.width || h_full > self.height {
            return None;
        }
        for shelf in &mut self.shelves {
            if shelf.height >= h_full && shelf.used_w + w_full <= self.width {
                let x = shelf.used_w + ATLAS_PAD;
                let y = shelf.y + ATLAS_PAD;
                shelf.used_w += w_full;
                return Some((x, y));
            }
        }
        let next_y = self.shelves.last().map(|s| s.y + s.height).unwrap_or(0);
        if next_y + h_full > self.height {
            return None;
        }
        self.shelves.push(Shelf {
            y: next_y,
            height: h_full,
            used_w: w_full,
        });
        Some((ATLAS_PAD, next_y + ATLAS_PAD))
    }

    fn upload_region(&self, x: u32, y: u32, w: u32, h: u32, pixels: &[u8]) {
        unsafe {
            glBindTexture(GL_TEXTURE_2D, self.texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                x as GLint,
                y as GLint,
                w as GLsizei,
                h as GLsizei,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                pixels.as_ptr() as *const _,
            );
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    /// Like `upload_region`, but also replicates the 1-pixel border into
    /// the surrounding pad area. Required for atlased rendering with
    /// LINEAR filtering: without edge bleed, sampling at the bitmap edge
    /// blends 50% transparent-black-pad → visible black grid between
    /// sprites in Mario 63. Caller must guarantee that (x, y) is at least
    /// ATLAS_PAD pixels away from the atlas borders (always true for our
    /// packer).
    fn upload_region_padded(&self, x: u32, y: u32, w: u32, h: u32, pixels: &[u8]) {
        if w == 0 || h == 0 {
            return;
        }
        // Build a (w+2) × (h+2) buffer with edge replication.
        let pw = (w + 2) as usize;
        let ph = (h + 2) as usize;
        let mut buf = vec![0u8; pw * ph * 4];
        let row_bytes = w as usize * 4;
        // Center rows: copy each source row into the buffer with 1 px
        // of horizontal replication on each side.
        for src_row in 0..h as usize {
            let src_off = src_row * row_bytes;
            let dst_row = src_row + 1;
            let dst_off = dst_row * pw * 4 + 4; // skip the left pad pixel
            buf[dst_off..dst_off + row_bytes]
                .copy_from_slice(&pixels[src_off..src_off + row_bytes]);
            // Left pad pixel = first source pixel of this row.
            let lpad_off = dst_row * pw * 4;
            buf[lpad_off..lpad_off + 4].copy_from_slice(&pixels[src_off..src_off + 4]);
            // Right pad pixel = last source pixel of this row.
            let rpad_off = dst_row * pw * 4 + (pw - 1) * 4;
            let last_pix_off = src_off + (w as usize - 1) * 4;
            buf[rpad_off..rpad_off + 4]
                .copy_from_slice(&pixels[last_pix_off..last_pix_off + 4]);
        }
        // Top pad row (row 0) = duplicate of first content row (row 1,
        // already has horizontal replication baked in).
        let row_stride = pw * 4;
        buf.copy_within(row_stride..2 * row_stride, 0);
        // Bottom pad row (row h+1) = duplicate of last content row (row h).
        let last_content = h as usize * row_stride;
        let last_pad = (h as usize + 1) * row_stride;
        buf.copy_within(last_content..last_content + row_stride, last_pad);

        unsafe {
            glBindTexture(GL_TEXTURE_2D, self.texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                (x as i32) - 1,
                (y as i32) - 1,
                (w + 2) as GLsizei,
                (h + 2) as GLsizei,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                buf.as_ptr() as *const _,
            );
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

impl Drop for Atlas {
    fn drop(&mut self) {
        unsafe { glDeleteTextures(1, &self.texture) };
    }
}

#[derive(Clone, Debug)]
struct SwitchBitmapHandle {
    atlas_index: usize,
    /// Atlas-space UV bounds in [0, 1].
    u0: f32,
    v0: f32,
    u1: f32,
    v1: f32,
    width: u32,
    height: u32,
}
impl BitmapHandleImpl for SwitchBitmapHandle {}

// ─── Standalone (FBO-attachable) textures ─────────────────────────────────────
//
// The atlas system above packs many bitmaps into shared GL textures, which is
// great for the common case but cannot be used as an FBO color attachment
// (you'd render over neighbours). cacheAsBitmap / filtered display objects need
// their own texture to render into and sample from, so Ruffle hands us a
// dedicated handle via `create_empty_texture`. This is the second BitmapHandle
// variant — code paths taking a BitmapHandle try `as_standalone_bitmap` before
// falling back to `as_switch_bitmap`. Mirrors the wgpu backend where EVERY
// bitmap is a standalone `Texture`.

/// A GL texture that owns its storage (not atlas-packed), suitable as an FBO
/// color attachment and as a sampling source. Owns the GL texture; the Drop
/// frees it.
struct StandaloneTexture {
    texture: GLuint,
    width: u32,
    height: u32,
}

impl Drop for StandaloneTexture {
    fn drop(&mut self) {
        unsafe { glDeleteTextures(1, &self.texture) };
    }
}

impl std::fmt::Debug for StandaloneTexture {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "StandaloneTexture(id={}, {}x{})", self.texture, self.width, self.height)
    }
}

/// BitmapHandle payload for a standalone texture. Cheap to clone (Arc); the
/// GL texture dies when the last Arc drops.
#[derive(Clone, Debug)]
struct StandaloneBitmap(Arc<StandaloneTexture>);
impl BitmapHandleImpl for StandaloneBitmap {}

/// Minimal SyncHandle. Ruffle wants something back from `render_offscreen` to
/// confirm the work was scheduled; `apply_filter` returns this since its
/// result is read straight back via `render_bitmap`, never via a sync.
#[derive(Debug)]
struct NoOpSyncHandle;
impl SyncHandle for NoOpSyncHandle {}

/// SyncHandle for `BitmapData.draw()`. Holds a NON-owning GL texture id (the
/// temp the draw commands were rendered into) plus the dirty region to read
/// back. Ruffle stores this in the BitmapData's `GpuModified` state and calls
/// `resolve_sync_handle` on the next CPU access (e.g. `copyPixels`), which
/// reads the pixels back into the BitmapData's CPU buffer. The texture itself
/// lives in the backend's `offscreen_temp_retired`/`offscreen_temp_pool`
/// (recycled one frame later, after Ruffle has resolved/dropped this handle in
/// the same tick), so this struct does NOT free it on drop — avoiding a
/// per-call texture alloc that cost ~90ms/frame on cacheAsBitmap-heavy games.
struct BitmapDataSyncHandle {
    texture: GLuint,
    /// Dirty region (BitmapData/top-left coords) — must match the `bounds`
    /// Ruffle passed to `render_offscreen`, since the readback closure indexes
    /// the buffer relative to this region's origin.
    x: u32,
    y: u32,
    w: u32,
    h: u32,
}
impl std::fmt::Debug for BitmapDataSyncHandle {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "BitmapDataSyncHandle(tex={}, region={}x{}@{},{})",
            self.texture, self.w, self.h, self.x, self.y
        )
    }
}
impl SyncHandle for BitmapDataSyncHandle {}

/// Allocate a fresh transparent RGBA8 texture (linear + clamp-to-edge),
/// suitable as an FBO color attachment. Returns None for a zero dimension.
fn make_standalone_texture(width: u32, height: u32) -> Option<StandaloneTexture> {
    if width == 0 || height == 0 {
        return None;
    }
    let mut tex: GLuint = 0;
    unsafe {
        glGenTextures(1, &mut tex);
        // glGenTextures returns 0 on failure (e.g. GL out of memory / too many
        // live textures). Using a 0 texture as an FBO color attachment or
        // sampler source crashes Mesa with a NULL deref (Data Abort, FAR≈0x0e).
        // Bail so callers (the filter pool) skip the pass instead of crashing.
        if tex == 0 {
            ruffle_log_cstr(b"make_standalone_texture: glGenTextures returned 0 (OOM?)\n\0".as_ptr() as *const _);
            return None;
        }
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        // Allocate storage with NULL data: every consumer fully overwrites the
        // texture before sampling it (render_commands_to_texture glClears it;
        // filter passes draw the whole region). The old `vec![0u8; w*h*4]`
        // CPU-side zero-fill was pure overhead — and dominated frame time when
        // the (now bounded) filter pool had to re-allocate on a cache miss.
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA8 as GLint,
            width as GLsizei, height as GLsizei, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, core::ptr::null(),
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR as GLint);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR as GLint);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE as GLint);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE as GLint);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    Some(StandaloneTexture { texture: tex, width, height })
}

/// What kind of draw call this is (chooses the shader program).
enum DrawKind {
    Solid,
    Gradient {
        /// Index into `GpuShape::gradient_textures`.
        texture_index: usize,
        /// 3x3 column-major matrix that maps `a_pos` (shape pixels) to
        /// gradient-local coords. Pre-inverted on CPU.
        local_matrix: [GLfloat; 9],
        gradient_kind: i32, // 0=linear, 1=radial, 2=focal
        spread: i32,        // 0=pad, 1=reflect, 2=repeat
        focal: f32,
    },
    Bitmap {
        /// Index into `SwitchRenderBackend::atlases` — the GL texture is
        /// owned by the atlas, not per-draw.
        atlas_index: usize,
        /// Atlas-space UV remap (origin.x, origin.y, scale.x, scale.y).
        uv_remap: [f32; 4],
        /// 3x3 column-major matrix mapping `a_pos` (shape pixels) to UV
        /// in [0, 1] of the source bitmap. Pre-inverted by
        /// `swf_bitmap_to_gl_matrix`.
        local_matrix: [GLfloat; 9],
        #[allow(dead_code)]
        is_smoothed: bool,
        #[allow(dead_code)]
        is_repeating: bool,
    },
}

struct GpuDraw {
    /// Byte offset of this draw's vertices inside the global vertex arena.
    vbo_offset: GLintptr,
    /// Allocated vertex bytes (multiple of `ARENA_ALIGN`).
    vbo_size: GLsizeiptr,
    /// Byte offset of this draw's indices inside the global index arena.
    ibo_offset: GLintptr,
    /// Allocated index bytes (multiple of `ARENA_ALIGN`).
    ibo_size: GLsizeiptr,
    num_indices: GLsizei,
    kind: DrawKind,
}

impl Drop for GpuDraw {
    fn drop(&mut self) {
        LIVE_GPU_DRAWS.fetch_sub(1, Ordering::Relaxed);
        // Can't free arena regions from here — no &mut to the backend.
        // Enqueue; submit_frame drains at the top of each frame.
        PENDING_FREES.lock().unwrap().push(PendingFree {
            vbo_offset: self.vbo_offset,
            vbo_size: self.vbo_size,
            ibo_offset: self.ibo_offset,
            ibo_size: self.ibo_size,
        });
    }
}

struct GpuShape {
    draws: Vec<GpuDraw>,
    gradient_textures: Vec<GLuint>,
}

impl Drop for GpuShape {
    fn drop(&mut self) {
        LIVE_GPU_SHAPES.fetch_sub(1, Ordering::Relaxed);
        if !self.gradient_textures.is_empty() {
            unsafe {
                glDeleteTextures(
                    self.gradient_textures.len() as GLsizei,
                    self.gradient_textures.as_ptr(),
                );
            }
        }
    }
}

impl std::fmt::Debug for GpuShape {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "GpuShape({} draws, {} gradients)", self.draws.len(), self.gradient_textures.len())
    }
}

#[derive(Debug)]
struct SwitchShapeHandle(Arc<GpuShape>);
impl ShapeHandleImpl for SwitchShapeHandle {}

// ─── Shader programs ──────────────────────────────────────────────────────────

struct SolidProgram {
    program: GLuint,
    u_world: GLint,
    u_mult: GLint,
    u_add: GLint,
}

struct BitmapProgram {
    program: GLuint,
    u_world: GLint,
    u_mult: GLint,
    u_add: GLint,
    u_tex: GLint,
    u_uv_remap: GLint,
}

struct GradientProgram {
    program: GLuint,
    u_world: GLint,
    u_mult: GLint,
    u_add: GLint,
    u_tex: GLint,
    u_grad_local: GLint,
    u_grad_kind: GLint,
    u_grad_spread: GLint,
    u_grad_focal: GLint,
}

/// Shader for "bitmap fill inside a shape": vertex computes UV from
/// `a_pos` via a per-draw 3×3 matrix (no per-vertex UV attribute), then
/// remaps from [0,1] to the atlas sub-rectangle. Fragment samples the
/// bound texture and applies color transform.
struct ShapeBitmapProgram {
    program: GLuint,
    u_world: GLint,
    u_mult: GLint,
    u_add: GLint,
    u_tex: GLint,
    u_uv: GLint,
    u_uv_remap: GLint,
    u_wrap_mode: GLint,
}

impl Drop for SolidProgram {
    fn drop(&mut self) {
        unsafe { glDeleteProgram(self.program) };
    }
}
impl Drop for BitmapProgram {
    fn drop(&mut self) {
        unsafe { glDeleteProgram(self.program) };
    }
}
impl Drop for GradientProgram {
    fn drop(&mut self) {
        unsafe { glDeleteProgram(self.program) };
    }
}
impl Drop for ShapeBitmapProgram {
    fn drop(&mut self) {
        unsafe { glDeleteProgram(self.program) };
    }
}

// ─── Filter programs ──────────────────────────────────────────────────────────

struct ColorMatrixFilterProgram {
    program: GLuint,
    u_src_uv: GLint,
    u_color_mat: GLint,
    u_color_extra: GLint,
}
struct BlurFilterProgram {
    program: GLuint,
    u_src_uv: GLint,
    u_blur_dir: GLint,
    u_blur_m: GLint,
    u_blur_m2: GLint,
    u_blur_full_size: GLint,
    u_blur_first_weight: GLint,
    u_blur_last_offset: GLint,
    u_blur_last_weight: GLint,
}
struct GlowFilterProgram {
    program: GLuint,
    u_src_uv: GLint,
    u_blur_uv: GLint,
    u_color: GLint,
    u_strength: GLint,
    u_inner: GLint,
    u_knockout: GLint,
    u_composite_source: GLint,
}
impl Drop for ColorMatrixFilterProgram {
    fn drop(&mut self) { unsafe { glDeleteProgram(self.program) }; }
}
impl Drop for BlurFilterProgram {
    fn drop(&mut self) { unsafe { glDeleteProgram(self.program) }; }
}
impl Drop for GlowFilterProgram {
    fn drop(&mut self) { unsafe { glDeleteProgram(self.program) }; }
}

struct BevelFilterProgram {
    program: GLuint,
    u_src_uv: GLint,
    u_blur_uv_l: GLint,
    u_blur_uv_r: GLint,
    u_highlight: GLint,
    u_shadow: GLint,
    u_strength: GLint,
    u_bevel_type: GLint,
    u_knockout: GLint,
}
impl Drop for BevelFilterProgram {
    fn drop(&mut self) { unsafe { glDeleteProgram(self.program) }; }
}

/// Two-texture programs for `render_alpha_mask` and complex `blend` modes.
/// Both reuse FILTER_VERT (a full-quad pass with `u_src_uv`), and sample a
/// second texture at unit 1 in addition to `u_tex` at unit 0.
struct AlphaMaskProgram {
    program: GLuint,
    u_src_uv: GLint,
}
/// Single-texture full-quad blit program (FILTER_VERT + a chosen fragment
/// shader). Used for the premultiplied<->straight conversions that move
/// render_offscreen results between premultiplied temps and straight atlas
/// slots without a CPU readback.
struct BlitProgram {
    program: GLuint,
    u_src_uv: GLint,
}
impl Drop for BlitProgram {
    fn drop(&mut self) { unsafe { glDeleteProgram(self.program) }; }
}
struct ComplexBlendProgram {
    program: GLuint,
    u_src_uv: GLint,
    u_blend_mode: GLint,
    u_current_flip: GLint,
}
impl Drop for AlphaMaskProgram {
    fn drop(&mut self) { unsafe { glDeleteProgram(self.program) }; }
}
impl Drop for ComplexBlendProgram {
    fn drop(&mut self) { unsafe { glDeleteProgram(self.program) }; }
}

// ─── Stencil mask state ───────────────────────────────────────────────────────

/// DIAGNOSTIC (2026-06-03, catmario invisible world): when true, maskees draw
/// unconditionally (GL_ALWAYS) instead of being gated by the stencil coverage
/// count. If the world (platforms/ground/enemies) appears with this on, the
/// invisible-world bug is in our stencil masking; if it stays invisible, the
/// cached content itself is empty/not composited. Set back to false after.
/// Result (2026-06-03): world stayed invisible with gating off → NOT masking;
/// the cached content path is the culprit. Left at false.
const DISABLE_MASK_GATING: bool = false;

/// DIAGNOSTIC TOGGLE: when true, mask shapes stay invisible but the stencil
/// gating is skipped so maskees draw unconditionally. Used to confirm whether
/// the SMWF overworld blank screen is caused by our stencil masking. Set back
/// to false once the masking bug is understood/fixed.
#[derive(Default, Clone, Copy)]
struct MaskState {
    /// Nesting depth: 0 = no mask, N = drawing the Nth maskee. Doubles as the
    /// stencil coverage count a maskee at this depth must equal.
    depth: u32,
    /// True while we are drawing a MASK shape into the stencil (between
    /// push_mask/deactivate_mask and the following activate_mask/pop). Draws
    /// issued in this phase write the stencil region; if none happen, the
    /// maskee is gated against an empty stencil → invisible.
    writing: bool,
}

// ─── The backend ──────────────────────────────────────────────────────────────

/// Cached GL state to avoid redundant calls. On Mesa-Switch each call goes
/// through the driver's command-buffer encoder; even if value-unchanged
/// dispatches are cheap on PC, they're measurable on Tegra X1. Mario 63 in
/// the worst frame (FLUDD rocket) issues ~3 shapes/frame × 5 draws each =
/// ~15 draws/frame all on `shape_bitmap_prog` with the same atlas texture
/// and the same wrap_mode. With this cache, only one glUseProgram +
/// one glBindTexture per such run reaches the driver.
///
/// Interior mutability via `Cell` so the `use_*` helpers can keep `&self`
/// without bubbling `&mut self` through every render path.
#[derive(Default)]
struct GlStateCache {
    last_program: Cell<GLuint>,
    last_texture: Cell<GLuint>,
    last_wrap_mode: Cell<i32>,
    last_vao: Cell<GLuint>,
}

impl GlStateCache {
    /// Forget what we cached. Call at submit_frame start (after we know
    /// any external glXxx calls have potentially mutated state) and at end
    /// (where we reset GL to zero anyway).
    fn invalidate(&self) {
        self.last_program.set(0);
        self.last_texture.set(0);
        self.last_wrap_mode.set(-1);
        self.last_vao.set(0);
    }

    fn use_program(&self, prog: GLuint) {
        if self.last_program.get() != prog {
            unsafe { glUseProgram(prog) };
            self.last_program.set(prog);
        }
    }

    fn bind_texture_unit0(&self, tex: GLuint) {
        if self.last_texture.get() != tex {
            unsafe {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex);
            }
            self.last_texture.set(tex);
        }
    }

    fn set_wrap_mode(&self, location: GLint, mode: i32) {
        if self.last_wrap_mode.get() != mode {
            unsafe { glUniform1i(location, mode) };
            self.last_wrap_mode.set(mode);
        }
    }

    fn bind_vao(&self, vao: GLuint) {
        if self.last_vao.get() != vao {
            unsafe { glBindVertexArray(vao) };
            self.last_vao.set(vao);
        }
    }
}

pub struct SwitchRenderBackend {
    dimensions: ViewportDimensions,
    tessellator: ShapeTessellator,

    solid: SolidProgram,
    bitmap_prog: BitmapProgram,
    shape_bitmap_prog: ShapeBitmapProgram,
    gradient_prog: GradientProgram,

    /// Mesa-Switch GL state cache. See `GlStateCache` docs above.
    gl_state: GlStateCache,

    /// Solid unit quad (pos+rgba, 6 vertices). Used by `draw_rect`.
    rect_vao: GLuint,
    rect_vbo: GLuint,

    /// Bitmap unit quad (pos+uv, 6 vertices). Used by `render_bitmap`.
    bitmap_vao: GLuint,
    bitmap_vbo: GLuint,

    /// Unit line (pos+rgba, 2 vertices). Used by `draw_line`.
    line_vao: GLuint,
    line_vbo: GLuint,

    /// Unit line-rect (pos+rgba, 5 vertices using GL_LINE_LOOP-equivalent
    /// via two segments × 4 — simpler: 4 separate GL_LINES, 8 verts).
    line_rect_vao: GLuint,
    line_rect_vbo: GLuint,

    mask: MaskState,
    warned_unsupported: u32,
    /// Frame counter for periodic `glGetError` polling.
    frame_count: u32,
    /// Diagnostic counters: how many shapes/bitmaps registered so far.
    shapes_registered: u32,
    bitmaps_registered: u32,
    bitmap_draws_emitted: u32,
    bitmap_render_count: u32,
    /// System tick at the start of the current heartbeat window (60 frames).
    /// Set to 0 on first heartbeat; FPS measurement skipped until we have
    /// two samples to subtract. Uses `armGetSystemTick` for high resolution
    /// — at ~19.2 MHz a 60-frame window resolves ~50 ns granularity, way
    /// better than what FPS measurement needs.
    heartbeat_tick: u64,
    /// Number of GL draw calls (glDrawElements*/glDrawArrays) emitted since
    /// the last heartbeat. Helps correlate FPS drops with draw-call count
    /// — if it spikes from ~30 to ~300, the next perf step is batching.
    draw_calls_this_window: u32,
    /// Mask diagnostics, reset per heartbeat window. `push_mask_window` counts
    /// `push_mask` calls; `alpha_mask_window` counts `render_alpha_mask` (which
    /// we currently SKIP — non-zero on a blank screen would explain it);
    /// `masked_draw_window` counts shape/bitmap draws issued while a stencil
    /// mask is active (gated on stencil EQUAL). If a screen draws thousands of
    /// masked things but shows nothing, the mask shape isn't writing stencil.
    push_mask_window: u32,
    alpha_mask_window: u32,
    masked_draw_window: u32,
    /// Draws issued while writing a mask shape into the stencil (writing=true).
    /// If this is ~0 while `masked_draw_window` is large, mask shapes aren't
    /// producing stencil → maskee gated empty → everything masked is invisible.
    mask_shape_draw_window: u32,
    /// Max `cache_entries` count in any frame of the current window. A periodic
    /// spike (e.g. once/sec) means an HUD/text element is re-caching + (with
    /// filters on) re-filtering on a timer — the idle-stutter suspect.
    cache_entries_max_window: u32,
    /// How many times Ruffle has called `render_offscreen` since boot —
    /// non-zero means something on stage uses `cacheAsBitmap` or a filter.
    /// Logged every heartbeat so we can correlate spikes with crashes.
    render_offscreen_calls: u32,
    /// How many times Ruffle has called `apply_filter` since boot.
    apply_filter_calls: u32,
    /// How many times we've read a BitmapData.draw() result back to the CPU
    /// (`resolve_sync_handle`). Non-zero confirms the tile-engine readback path
    /// (SMWF terrain) is firing.
    resolve_sync_calls: u32,
    /// One bit per `Filter` variant we've seen via `is_filter_supported`,
    /// so each variant is logged the first time only. Variant ordinals
    /// match `filter_variant_ordinal()`. `Cell` would be simpler but
    /// `is_filter_supported` takes `&self`, so we use an atomic.
    filters_seen_mask: AtomicU16,
    /// Pool of texture atlases. New atlases get appended when current is
    /// full. Bitmaps are packed into these instead of getting individual
    /// GL textures.
    atlases: Vec<Atlas>,

    /// Single global VBO for all shape draws (suballocated via freelist).
    /// All `GpuDraw::vbo_offset` are byte offsets into this buffer.
    vertex_arena: BufferArena,
    /// Single global IBO for all shape draws.
    index_arena: BufferArena,
    /// Single VAO used for every shape draw. Pre-configured at boot to
    /// read (pos.xy, rgba) from `vertex_arena` with stride 24, and to use
    /// `index_arena` as the element buffer. Each draw shifts the read
    /// origin via `glDrawElementsBaseVertex(base_vertex)`.
    shape_vao: GLuint,

    /// When `Some((w, h))`, `world_matrix` targets an offscreen FBO of that
    /// size (no Y-flip) instead of the main framebuffer. Set while replaying
    /// commands into a cache texture. Commands are pre-shifted by Ruffle to
    /// target-local coords, so no origin offset is needed.
    offscreen_dims: Option<(u32, u32)>,
    /// Reusable FBO object (lazy; 0 = not created). Color attachment is
    /// rebound per offscreen render.
    offscreen_fbo: GLuint,
    /// Shared depth+stencil renderbuffer attached to `offscreen_fbo`, so
    /// stencil masks pushed by `commands.execute()` work inside the FBO.
    /// Grows monotonically; attached once.
    offscreen_depth_stencil: GLuint,
    offscreen_depth_stencil_dims: (u32, u32),

    color_matrix_filter: ColorMatrixFilterProgram,
    unpremult_blit: BlitProgram,
    premult_blit: BlitProgram,
    blur_filter: BlurFilterProgram,
    glow_filter: GlowFilterProgram,
    bevel_filter: BevelFilterProgram,
    /// Two-texture composite programs for soft alpha masks + complex blends.
    alpha_mask_prog: AlphaMaskProgram,
    complex_blend_prog: ComplexBlendProgram,
    /// How many times `blend` ran a real (non-Normal) composite this window,
    /// and `render_alpha_mask` ran a soft-mask composite. Surfaced in the
    /// heartbeat so a blank/wrong screen can be correlated with these paths.
    blend_window: u32,
    /// Pool of standalone textures reused across filter passes within a
    /// single submit_frame, keyed by `(width, height)`. Avoids paying
    /// glGenTextures + glTexImage2D + glDeleteTextures per filter per
    /// frame, which was the main fps killer in Phase 2.3's first try.
    filter_tex_pool: FilterTexturePool,

    /// Reusable temp textures for `render_offscreen` (BitmapData.draw /
    /// cacheAsBitmap). `_pool` holds textures free for reuse; `_retired` holds
    /// the ones handed to this frame's SyncHandles. Ruffle resolves/drops each
    /// SyncHandle within the same tick, so `submit_frame` moves `_retired` back
    /// into `_pool` for the next frame. This avoids a per-call glGenTextures +
    /// glTexImage2D + glDeleteTextures — which became the dominant cost
    /// (~90ms/frame, 48 allocs) once the readback was moved onto the GPU.
    offscreen_temp_pool: Vec<StandaloneTexture>,
    offscreen_temp_retired: Vec<StandaloneTexture>,

    /// Per-frame perf attribution for the slow-frame detector. `frame_snapshot`
    /// is the raw counter state captured at the top of `submit_frame`;
    /// `last_frame` is the delta of the frame that just finished. lib.rs reads
    /// `last_frame` whenever a frame blows the FPS budget, so an FPS spike can
    /// be pinned on what the frame actually did (offscreen filter passes,
    /// bitmap uploads, shape tessellation, draw-call count, …). Cumulative
    /// counters (offscreen/filter/resolve/bmp/shape) are exact; the window
    /// counters (dc/blend/pmask/mdraw) under-report on the 1-in-60 heartbeat
    /// frame because the heartbeat zeroes them mid-`submit_frame`.
    frame_snapshot: FrameBreakdown,
    last_frame: FrameBreakdown,
}

/// One frame's worth of per-counter activity (or the raw snapshot used to
/// derive it). All fields are deltas in `last_frame`. Logged by the slow-frame
/// detector — see `SwitchRenderBackend::log_slow_frame`.
#[derive(Clone, Copy, Default)]
struct FrameBreakdown {
    /// GL draw calls (glDrawElements*/glDrawArrays) emitted this frame.
    draw_calls: u32,
    /// `render_offscreen` calls — cacheAsBitmap / filter source renders.
    offscreen: u32,
    /// `apply_filter` calls — individual blur/glow/bevel/color-matrix passes.
    filter: u32,
    /// `resolve_sync_handle` readbacks (BitmapData.draw() → CPU).
    resolve: u32,
    /// Bitmaps registered (texture uploads) this frame.
    bmp_uploads: u32,
    /// Shapes registered (tessellation) this frame.
    shape_regs: u32,
    /// Non-Normal blend composites run this frame.
    blend: u32,
    /// `push_mask` calls this frame.
    pushmask: u32,
    /// Draws issued under an active stencil mask this frame.
    masked_draw: u32,
    /// cacheAsBitmap entries processed by `submit_frame` this frame.
    cache_entries: u32,
    /// Filter chains actually run this frame (bounded by the per-frame budget).
    filter_chains: u32,
}

/// Pool of `StandaloneTexture` keyed by `(width, height)`. Acquire pulls an
/// existing entry of the right size or makes a fresh one; release pushes it
/// back for the next caller. Each entry is RGBA8 with linear sampling and
/// clamp-to-edge wrap — same setup as `make_standalone_texture`.
///
/// Reusing entries across filter passes prevents the per-frame texture
/// alloc/free thrash that brought Mario 63 down to 5 fps in the prior patch.
/// How many frames a pooled texture survives without being reused before the
/// pool frees it. 2 = "used this frame or last frame stays". This bounds the
/// pool to the recent working set: a stable filtered scene reuses every
/// texture each frame (0 reallocations after the first frame), while sizes
/// that stop appearing are reclaimed within 2 frames — preventing the
/// unbounded session-long growth that exhausted GL textures (→ glGenTextures
/// 0 → Mesa NULL-deref crash). A fixed COUNT cap was worse: once full of stale
/// sizes it blocked new ones, thrashing alloc/free every frame.
const FILTER_POOL_TTL_FRAMES: u64 = 2;

struct FilterTexturePool {
    /// Each entry carries the frame it was last released, for TTL eviction.
    buckets: std::collections::HashMap<(u32, u32), Vec<(StandaloneTexture, u64)>>,
    /// Total retained (for the heartbeat).
    pooled: usize,
    /// Set by `begin_frame`; `release` stamps freed textures with it.
    current_frame: u64,
}

impl FilterTexturePool {
    fn new() -> Self {
        Self { buckets: std::collections::HashMap::new(), pooled: 0, current_frame: 0 }
    }
    /// Reclaim textures not reused within `FILTER_POOL_TTL_FRAMES`. Called once
    /// per `submit_frame` before the cache_entries filter chain runs.
    fn begin_frame(&mut self, frame: u64) {
        self.current_frame = frame;
        let keep_from = frame.saturating_sub(FILTER_POOL_TTL_FRAMES - 1);
        for bucket in self.buckets.values_mut() {
            let before = bucket.len();
            bucket.retain(|(_, f)| *f >= keep_from); // dropped entries free their GL texture
            self.pooled -= before - bucket.len();
        }
        self.buckets.retain(|_, v| !v.is_empty());
    }
    fn acquire(&mut self, w: u32, h: u32) -> Option<StandaloneTexture> {
        if let Some(bucket) = self.buckets.get_mut(&(w, h)) {
            if let Some((tex, _)) = bucket.pop() {
                self.pooled = self.pooled.saturating_sub(1);
                return Some(tex);
            }
        }
        make_standalone_texture(w, h)
    }
    fn release(&mut self, tex: StandaloneTexture) {
        let key = (tex.width, tex.height);
        let f = self.current_frame;
        self.buckets.entry(key).or_default().push((tex, f));
        self.pooled += 1;
    }
    fn len(&self) -> usize { self.pooled }
}

/// Returns a stable 0..=9 ordinal + short name for a `Filter` variant so we
/// can dedupe `is_filter_supported` logs without allocating a HashSet.
fn filter_variant_ordinal(f: &Filter) -> (u8, &'static str) {
    match f {
        Filter::BevelFilter(_) => (0, "Bevel"),
        Filter::BlurFilter(_) => (1, "Blur"),
        Filter::ColorMatrixFilter(_) => (2, "ColorMatrix"),
        Filter::ConvolutionFilter(_) => (3, "Convolution"),
        Filter::DisplacementMapFilter(_) => (4, "DisplacementMap"),
        Filter::DropShadowFilter(_) => (5, "DropShadow"),
        Filter::GlowFilter(_) => (6, "Glow"),
        Filter::GradientBevelFilter(_) => (7, "GradientBevel"),
        Filter::GradientGlowFilter(_) => (8, "GradientGlow"),
        Filter::ShaderFilter(_) => (9, "Shader"),
    }
}

// ─── Shaders source ───────────────────────────────────────────────────────────

const SOLID_VERT: &[u8] = b"#version 330 core\n\
layout(location = 0) in vec2 a_pos;\n\
layout(location = 1) in vec4 a_col;\n\
uniform mat3 u_world;\n\
out vec4 v_col;\n\
void main() {\n\
    vec3 p = u_world * vec3(a_pos, 1.0);\n\
    gl_Position = vec4(p.xy, 0.0, 1.0);\n\
    v_col = a_col;\n\
}\n\0";

const SOLID_FRAG: &[u8] = b"#version 330 core\n\
in vec4 v_col;\n\
out vec4 frag_color;\n\
uniform vec4 u_mult;\n\
uniform vec4 u_add;\n\
void main() {\n\
    frag_color = clamp(v_col * u_mult + u_add, 0.0, 1.0);\n\
}\n\0";

const BITMAP_VERT: &[u8] = b"#version 330 core\n\
layout(location = 0) in vec2 a_pos;\n\
layout(location = 1) in vec2 a_uv;\n\
uniform mat3 u_world;\n\
uniform vec4 u_uv_remap;\n\
out vec2 v_uv;\n\
void main() {\n\
    vec3 p = u_world * vec3(a_pos, 1.0);\n\
    gl_Position = vec4(p.xy, 0.0, 1.0);\n\
    v_uv = u_uv_remap.xy + a_uv * u_uv_remap.zw;\n\
}\n\0";

const BITMAP_FRAG: &[u8] = b"#version 330 core\n\
in vec2 v_uv;\n\
out vec4 frag_color;\n\
uniform sampler2D u_tex;\n\
uniform vec4 u_mult;\n\
uniform vec4 u_add;\n\
void main() {\n\
    vec4 c = texture(u_tex, v_uv);\n\
    frag_color = clamp(c * u_mult + u_add, 0.0, 1.0);\n\
}\n\0";

/// Vertex shader for gradient draws: just like solid except we forward the
/// pre-projection position (`a_pos`) so the frag can compute gradient-local
/// coords from it.
const GRADIENT_VERT: &[u8] = b"#version 330 core\n\
layout(location = 0) in vec2 a_pos;\n\
uniform mat3 u_world;\n\
out vec2 v_pos;\n\
void main() {\n\
    vec3 p = u_world * vec3(a_pos, 1.0);\n\
    gl_Position = vec4(p.xy, 0.0, 1.0);\n\
    v_pos = a_pos;\n\
}\n\0";

/// Vertex shader for bitmap fills inside shapes: computes the per-bitmap UV
/// from `u_uv * a_pos` (matrix already pre-inverted by `swf_bitmap_to_gl_matrix`)
/// and passes it through unmodified. The fragment shader handles wrap mode
/// (fract for repeating fills, clamp otherwise) BEFORE remapping into the
/// atlas sub-rect — doing fract/clamp before remap is critical, since the
/// atlas places multiple bitmaps in one texture and remapping an out-of-
/// range UV would index into a neighbour bitmap (visible bug: Mario 63's
/// ground tile showed Mario's hat sprite).
const SHAPE_BITMAP_VERT: &[u8] = b"#version 330 core\n\
layout(location = 0) in vec2 a_pos;\n\
uniform mat3 u_world;\n\
uniform mat3 u_uv;\n\
out vec2 v_uv_local;\n\
void main() {\n\
    vec3 p = u_world * vec3(a_pos, 1.0);\n\
    gl_Position = vec4(p.xy, 0.0, 1.0);\n\
    vec3 uv = u_uv * vec3(a_pos, 1.0);\n\
    v_uv_local = uv.xy;\n\
}\n\0";

const SHAPE_BITMAP_FRAG: &[u8] = b"#version 330 core\n\
in vec2 v_uv_local;\n\
out vec4 frag_color;\n\
uniform sampler2D u_tex;\n\
uniform vec4 u_mult;\n\
uniform vec4 u_add;\n\
uniform vec4 u_uv_remap;\n\
uniform int u_wrap_mode;\n\
void main() {\n\
    vec2 local = (u_wrap_mode == 1) ? fract(v_uv_local) : clamp(v_uv_local, 0.0, 1.0);\n\
    vec2 atlas_uv = u_uv_remap.xy + local * u_uv_remap.zw;\n\
    vec4 c = texture(u_tex, atlas_uv);\n\
    frag_color = clamp(c * u_mult + u_add, 0.0, 1.0);\n\
}\n\0";

// `u_grad_local` here is the matrix produced by ruffle's `swf_to_gl_matrix`
// — already inverted *and* normalised so that `lp.x` is the linear gradient
// parameter in [0, 1], and `(lp.xy - 0.5)` is the radial offset (the
// gradient circle has radius 0.5, centred at (0.5, 0.5)).
const GRADIENT_FRAG: &[u8] = b"#version 330 core\n\
in vec2 v_pos;\n\
out vec4 frag_color;\n\
uniform sampler2D u_tex;\n\
uniform mat3 u_grad_local;\n\
uniform int u_grad_kind;\n\
uniform int u_grad_spread;\n\
uniform float u_grad_focal;\n\
uniform vec4 u_mult;\n\
uniform vec4 u_add;\n\
\n\
float apply_spread(float t, int mode) {\n\
    if (mode == 0) return clamp(t, 0.0, 1.0);\n\
    if (mode == 2) return fract(t);\n\
    float f = fract(t * 0.5) * 2.0;\n\
    return f > 1.0 ? 2.0 - f : f;\n\
}\n\
\n\
void main() {\n\
    vec3 lp = u_grad_local * vec3(v_pos, 1.0);\n\
    float t;\n\
    if (u_grad_kind == 0) {\n\
        // Linear: lp.x is already the gradient parameter.\n\
        t = lp.x;\n\
    } else {\n\
        // Radial / focal: centre at (0.5, 0.5), radius 0.5 -> multiply by 2.\n\
        vec2 d = lp.xy - vec2(0.5);\n\
        t = length(d) * 2.0;\n\
        if (u_grad_kind == 2) {\n\
            // Focal: very rough offset, good enough as a placeholder.\n\
            t = clamp(t + u_grad_focal * d.x * 2.0, 0.0, 1.0);\n\
        }\n\
    }\n\
    t = apply_spread(t, u_grad_spread);\n\
    vec4 c = texture(u_tex, vec2(t, 0.5));\n\
    frag_color = clamp(c * u_mult + u_add, 0.0, 1.0);\n\
}\n\0";

// ─── Filter shaders ───────────────────────────────────────────────────────────
//
// Ported from `third_party/ruffle/render/wgpu/shaders/filter/{blur,glow,color_matrix}.wgsl`
// with one convention difference: no Y-flip in the vertex stage. wgpu's filter
// vertex shader does `vec4(pos.x*2-1, 1-pos.y*2, ...)` to compensate for its
// top-left texture origin; GL stores texel(0,0) at bottom-left so the no-flip
// version is correct here.
//
// All filter passes share: unit quad input (pos.xy in [0,1]², matching
// `build_bitmap_quad`), `u_src_uv` re-mapping the [0,1] UV into a sub-rect of
// the source texture, and `u_tex` sampler bound at unit 0 (set at link time).

const FILTER_VERT: &[u8] = b"#version 330 core\n\
layout(location = 0) in vec2 a_pos;\n\
layout(location = 1) in vec2 a_uv;\n\
uniform vec4 u_src_uv;\n\
out vec2 v_uv;\n\
void main() {\n\
    gl_Position = vec4(a_pos.x * 2.0 - 1.0, a_pos.y * 2.0 - 1.0, 0.0, 1.0);\n\
    v_uv = u_src_uv.xy + a_uv * u_src_uv.zw;\n\
}\n\0";

// Faithful port of `color_matrix.wgsl`. 20-float ColorMatrix as a 4×4 mat plus
// a vec4 of "+" terms; un-premultiply rgb before the multiply, re-premultiply
// after, to match the Flash convention.
const COLOR_MATRIX_FRAG: &[u8] = b"#version 330 core\n\
in vec2 v_uv;\n\
out vec4 frag_color;\n\
uniform sampler2D u_tex;\n\
uniform mat4 u_color_mat;\n\
uniform vec4 u_color_extra;\n\
void main() {\n\
    vec4 src = texture(u_tex, v_uv);\n\
    vec3 rgb_un = src.a > 0.0 ? src.rgb / src.a : vec3(0.0);\n\
    vec4 in_vec = vec4(rgb_un, src.a);\n\
    vec4 out_vec = u_color_mat * in_vec + u_color_extra;\n\
    vec4 c = clamp(out_vec, 0.0, 1.0);\n\
    frag_color = vec4(c.rgb * c.a, c.a);\n\
}\n\0";

// Premultiplied -> straight-alpha copy. `render_offscreen` renders draw()
// commands into a PREMULTIPLIED temp texture; atlas slots store STRAIGHT
// alpha, so repatriating the result into an atlas slot needs an
// un-premultiply. Doing it on the GPU (this shader, into the atlas FBO)
// replaces a per-call `glReadPixels` + CPU divide + re-upload — that readback
// was ~78% of frame time on cacheAsBitmap-heavy AS3 games (catmario:
// ~260ms/frame across 48 draw() repatriations). Reuses FILTER_VERT.
const UNPREMULT_FRAG: &[u8] = b"#version 330 core\n\
in vec2 v_uv;\n\
out vec4 frag_color;\n\
uniform sampler2D u_tex;\n\
void main() {\n\
    vec4 src = texture(u_tex, v_uv);\n\
    frag_color = src.a > 0.0 ? vec4(src.rgb / src.a, src.a) : vec4(0.0);\n\
}\n\0";

// Straight-alpha -> premultiplied copy (inverse of UNPREMULT_FRAG). Used to
// SEED a render_offscreen temp with the BitmapData's existing (straight, atlas)
// content before compositing new draw() commands on top — Ruffle's
// `render_offscreen` must blend onto the previous contents (its wgpu backend
// uses `RenderTargetMode::FreshWithTexture`). Without this seed, a game that
// builds its frame by accumulating many draw()s into one BitmapData (a software
// blitter, e.g. catmario's `stageBitmapdata`) loses all but the last draw.
const PREMULT_FRAG: &[u8] = b"#version 330 core\n\
in vec2 v_uv;\n\
out vec4 frag_color;\n\
uniform sampler2D u_tex;\n\
void main() {\n\
    vec4 src = texture(u_tex, v_uv);\n\
    frag_color = vec4(src.rgb * src.a, src.a);\n\
}\n\0";

// Separable Gaussian-approximating blur, faithful port of `blur.wgsl`. The
// vertex stage pre-shifts UV so the fragment loop starts at the right offset
// (`u_blur_m` half-distance, `u_blur_m2 = m*2` outer bound). The last sample
// is fused with a fractional weight to handle non-integer kernel radii.
// See <https://fgiesen.wordpress.com/2012/08/01/fast-blurs-2/>.
const BLUR_VERT: &[u8] = b"#version 330 core\n\
layout(location = 0) in vec2 a_pos;\n\
layout(location = 1) in vec2 a_uv;\n\
uniform vec4 u_src_uv;\n\
uniform vec2 u_blur_dir;\n\
uniform float u_blur_m;\n\
out vec2 v_uv;\n\
void main() {\n\
    gl_Position = vec4(a_pos.x * 2.0 - 1.0, a_pos.y * 2.0 - 1.0, 0.0, 1.0);\n\
    vec2 raw = u_src_uv.xy + a_uv * u_src_uv.zw;\n\
    v_uv = raw - u_blur_dir * u_blur_m;\n\
}\n\0";

const BLUR_FRAG: &[u8] = b"#version 330 core\n\
in vec2 v_uv;\n\
out vec4 frag_color;\n\
uniform sampler2D u_tex;\n\
uniform vec2 u_blur_dir;\n\
uniform float u_blur_m2;\n\
uniform float u_blur_full_size;\n\
uniform float u_blur_first_weight;\n\
uniform float u_blur_last_offset;\n\
uniform float u_blur_last_weight;\n\
void main() {\n\
    vec2 direction = u_blur_dir;\n\
    vec4 total = vec4(0.0);\n\
    total += texture(u_tex, v_uv - direction) * u_blur_first_weight;\n\
    vec4 center = vec4(0.0);\n\
    for (float i = 0.5; i < u_blur_m2; i += 2.0) {\n\
        center += texture(u_tex, v_uv + direction * i);\n\
    }\n\
    total += center * 2.0;\n\
    vec2 last_loc = v_uv + direction * (u_blur_m2 + u_blur_last_offset);\n\
    total += texture(u_tex, last_loc) * u_blur_last_weight;\n\
    vec4 result = total / u_blur_full_size;\n\
    frag_color = floor(result * 255.0) / 255.0;\n\
}\n\0";

// Glow composite + DropShadow: faithful port of `glow.wgsl`. Reads the source
// texture (unit 0) and a pre-blurred version of it (unit 1), composites with
// a uniform colour + strength + inner/knockout/composite_source flags. The
// blur_uv is offset per-vertex by `u_blur_uv.xy` (DropShadow distance), so
// the blur effectively shifts on the destination.
const GLOW_VERT: &[u8] = b"#version 330 core\n\
layout(location = 0) in vec2 a_pos;\n\
layout(location = 1) in vec2 a_uv;\n\
uniform vec4 u_src_uv;\n\
uniform vec4 u_blur_uv;\n\
out vec2 v_src_uv;\n\
out vec2 v_blur_uv;\n\
void main() {\n\
    gl_Position = vec4(a_pos.x * 2.0 - 1.0, a_pos.y * 2.0 - 1.0, 0.0, 1.0);\n\
    v_src_uv = u_src_uv.xy + a_uv * u_src_uv.zw;\n\
    v_blur_uv = u_blur_uv.xy + a_uv * u_blur_uv.zw;\n\
}\n\0";

const GLOW_FRAG: &[u8] = b"#version 330 core\n\
in vec2 v_src_uv;\n\
in vec2 v_blur_uv;\n\
out vec4 frag_color;\n\
uniform sampler2D u_tex;\n\
uniform sampler2D u_blur_tex;\n\
uniform vec4 u_color;\n\
uniform float u_strength;\n\
uniform int u_inner;\n\
uniform int u_knockout;\n\
uniform int u_composite_source;\n\
void main() {\n\
    bool inner = u_inner != 0;\n\
    bool knockout = u_knockout != 0;\n\
    bool composite_source = u_composite_source != 0;\n\
    float blur_a = texture(u_blur_tex, v_blur_uv).a;\n\
    vec4 dst = texture(u_tex, v_src_uv);\n\
    if (v_blur_uv.x < 0.0 || v_blur_uv.x > 1.0 || v_blur_uv.y < 0.0 || v_blur_uv.y > 1.0) {\n\
        blur_a = 0.0;\n\
    }\n\
    vec4 color = vec4(u_color.r, u_color.g, u_color.b, 1.0);\n\
    if (inner) {\n\
        float alpha = u_color.a * clamp((1.0 - blur_a) * u_strength, 0.0, 1.0);\n\
        if (knockout) {\n\
            color = color * alpha * dst.a;\n\
        } else if (composite_source) {\n\
            color = color * alpha * dst.a + dst * (1.0 - alpha);\n\
        } else {\n\
            color = color * alpha * dst.a;\n\
        }\n\
    } else {\n\
        float alpha = u_color.a * clamp(blur_a * u_strength, 0.0, 1.0);\n\
        if (knockout) {\n\
            color = color * alpha * (1.0 - dst.a);\n\
        } else if (composite_source) {\n\
            color = color * alpha * (1.0 - dst.a) + dst;\n\
        } else {\n\
            color = color * alpha;\n\
        }\n\
    }\n\
    frag_color = color;\n\
}\n\0";

// Bevel: like Glow, but samples the blurred alpha at TWO opposite offsets
// (±blur_offset along the filter angle) to derive a highlight side and a
// shadow side. Faithful port of wgpu's `bevel.wgsl`.
const BEVEL_VERT: &[u8] = b"#version 330 core\n\
layout(location = 0) in vec2 a_pos;\n\
layout(location = 1) in vec2 a_uv;\n\
uniform vec4 u_src_uv;\n\
uniform vec4 u_blur_uv_l;\n\
uniform vec4 u_blur_uv_r;\n\
out vec2 v_src_uv;\n\
out vec2 v_blur_l;\n\
out vec2 v_blur_r;\n\
void main() {\n\
    gl_Position = vec4(a_pos.x * 2.0 - 1.0, a_pos.y * 2.0 - 1.0, 0.0, 1.0);\n\
    v_src_uv = u_src_uv.xy + a_uv * u_src_uv.zw;\n\
    v_blur_l = u_blur_uv_l.xy + a_uv * u_blur_uv_l.zw;\n\
    v_blur_r = u_blur_uv_r.xy + a_uv * u_blur_uv_r.zw;\n\
}\n\0";

const BEVEL_FRAG: &[u8] = b"#version 330 core\n\
in vec2 v_src_uv;\n\
in vec2 v_blur_l;\n\
in vec2 v_blur_r;\n\
out vec4 frag_color;\n\
uniform sampler2D u_tex;\n\
uniform sampler2D u_blur_tex;\n\
uniform vec4 u_highlight;\n\
uniform vec4 u_shadow;\n\
uniform float u_strength;\n\
uniform int u_bevel_type;\n\
uniform int u_knockout;\n\
void main() {\n\
    bool knockout = u_knockout != 0;\n\
    bool outer = (u_bevel_type == 0 || u_bevel_type == 2);\n\
    bool inner = (u_bevel_type == 1 || u_bevel_type == 2);\n\
    float bl = texture(u_blur_tex, v_blur_l).a;\n\
    float br = texture(u_blur_tex, v_blur_r).a;\n\
    vec4 dst = texture(u_tex, v_src_uv);\n\
    if (v_blur_l.x < 0.0 || v_blur_l.x > 1.0 || v_blur_l.y < 0.0 || v_blur_l.y > 1.0) bl = 0.0;\n\
    if (v_blur_r.x < 0.0 || v_blur_r.x > 1.0 || v_blur_r.y < 0.0 || v_blur_r.y > 1.0) br = 0.0;\n\
    float ha = clamp((bl - br) * u_strength, 0.0, 1.0);\n\
    float sa = clamp((br - bl) * u_strength, 0.0, 1.0);\n\
    vec4 glow = u_highlight * ha + u_shadow * sa;\n\
    vec4 outc;\n\
    if (inner && outer) {\n\
        outc = knockout ? glow : (dst - dst * glow.a + glow);\n\
    } else if (inner) {\n\
        outc = knockout ? (glow * dst.a) : (glow * dst.a + dst * (1.0 - glow.a));\n\
    } else {\n\
        outc = knockout ? (glow - glow * dst.a) : (dst + glow - glow * dst.a);\n\
    }\n\
    frag_color = outc;\n\
}\n\0";

// Alpha-mask composite, faithful port of `alpha_mask.wgsl`. Samples the
// pre-rendered maskee (unit 0) and mask (unit 1) textures at the same UV and
// outputs the maskee scaled by the mask's alpha — a soft/luminance mask that
// the stencil masking path can't express. Reuses FILTER_VERT (u_src_uv set to
// the full [0,1]² so v_uv == a_uv). Output is premultiplied; the caller draws
// the result back over the stage with premultiplied "over" blending.
const ALPHA_MASK_FRAG: &[u8] = b"#version 330 core\n\
in vec2 v_uv;\n\
out vec4 frag_color;\n\
uniform sampler2D u_tex;\n\
uniform sampler2D u_mask_tex;\n\
void main() {\n\
    vec4 dst = texture(u_tex, v_uv);\n\
    vec4 src = texture(u_mask_tex, v_uv);\n\
    frag_color = vec4(dst.rgb * src.a, dst.a * src.a);\n\
}\n\0";

// Complex (non-trivial) blend modes, faithful port of the wgpu `blend/*.wgsl`
// family (multiply/lighten/darken/difference/invert/overlay/hardlight). Samples
// the backdrop "parent" (unit 0, a glCopyTexSubImage2D snapshot of the current
// render target) and the freshly-rendered blend group "current" (unit 1), and
// writes the full composited pixel (premultiplied) so the caller can overwrite
// the target region with blending DISABLED. `u_current_flip` flips the current
// sampler's V when the target is the main framebuffer (which renders Y-flipped,
// unlike offscreen textures whose row 0 is the Flash top); the parent snapshot
// is always sampled straight since it's a 1:1 copy of the target. All the inner
// blend funcs operate on un-premultiplied colour, guarding the divide so a
// transparent backdrop (dst.a == 0) collapses the formula back to `src`.
const COMPLEX_BLEND_FRAG: &[u8] = b"#version 330 core\n\
in vec2 v_uv;\n\
out vec4 frag_color;\n\
uniform sampler2D u_tex;\n\
uniform sampler2D u_current_tex;\n\
uniform int u_blend_mode;\n\
uniform float u_current_flip;\n\
vec3 blend_func(vec3 s, vec3 d) {\n\
    if (u_blend_mode == 0) { return s * d; }\n\
    if (u_blend_mode == 1) { return max(s, d); }\n\
    if (u_blend_mode == 2) { return min(s, d); }\n\
    if (u_blend_mode == 3) { return abs(d - s); }\n\
    if (u_blend_mode == 4) { return 1.0 - d; }\n\
    if (u_blend_mode == 5) {\n\
        vec3 o = s;\n\
        o.r = (d.r <= 0.5) ? (2.0 * s.r * d.r) : (1.0 - 2.0 * (1.0 - d.r) * (1.0 - s.r));\n\
        o.g = (d.g <= 0.5) ? (2.0 * s.g * d.g) : (1.0 - 2.0 * (1.0 - d.g) * (1.0 - s.g));\n\
        o.b = (d.b <= 0.5) ? (2.0 * s.b * d.b) : (1.0 - 2.0 * (1.0 - d.b) * (1.0 - s.b));\n\
        return o;\n\
    }\n\
    if (u_blend_mode == 6) {\n\
        vec3 o = s;\n\
        o.r = (s.r <= 0.5) ? (2.0 * s.r * d.r) : (1.0 - 2.0 * (1.0 - d.r) * (1.0 - s.r));\n\
        o.g = (s.g <= 0.5) ? (2.0 * s.g * d.g) : (1.0 - 2.0 * (1.0 - d.g) * (1.0 - s.g));\n\
        o.b = (s.b <= 0.5) ? (2.0 * s.b * d.b) : (1.0 - 2.0 * (1.0 - d.b) * (1.0 - s.b));\n\
        return o;\n\
    }\n\
    return s;\n\
}\n\
void main() {\n\
    vec2 cuv = vec2(v_uv.x, mix(v_uv.y, 1.0 - v_uv.y, u_current_flip));\n\
    vec4 dst = texture(u_tex, v_uv);\n\
    vec4 src = texture(u_current_tex, cuv);\n\
    if (src.a <= 0.0) { frag_color = dst; return; }\n\
    vec3 s_un = src.rgb / src.a;\n\
    vec3 d_un = (dst.a > 0.0) ? (dst.rgb / dst.a) : vec3(0.0);\n\
    vec3 bf = blend_func(s_un, d_un);\n\
    vec3 rgb = src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a) + src.a * dst.a * bf;\n\
    float a = src.a + dst.a * (1.0 - src.a);\n\
    frag_color = vec4(rgb, a);\n\
}\n\0";

// ─── Shader build helpers ─────────────────────────────────────────────────────

fn compile_shader(kind: GLenum, src_nul: &[u8]) -> Option<GLuint> {
    unsafe {
        let shader = glCreateShader(kind);
        let src_ptr = src_nul.as_ptr() as *const GLchar;
        glShaderSource(shader, 1, &src_ptr, core::ptr::null());
        glCompileShader(shader);
        let mut status: GLint = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &mut status);
        if status == 0 {
            log(b"backend shader compile failed:\n\0");
            log_info_log(shader, false);
            glDeleteShader(shader);
            return None;
        }
        Some(shader)
    }
}

fn link_program(vert_src: &[u8], frag_src: &[u8]) -> Option<GLuint> {
    let vs = compile_shader(GL_VERTEX_SHADER, vert_src)?;
    let fs = compile_shader(GL_FRAGMENT_SHADER, frag_src)?;
    unsafe {
        let program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        let mut status: GLint = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &mut status);
        if status == 0 {
            log_info_log(program, true);
            glDeleteProgram(program);
            return None;
        }
        Some(program)
    }
}

fn log_info_log(handle: GLuint, is_program: bool) {
    unsafe {
        let mut buf: [u8; 1024] = [0; 1024];
        let mut written: GLsizei = 0;
        if is_program {
            glGetProgramInfoLog(handle, buf.len() as GLsizei, &mut written, buf.as_mut_ptr() as *mut GLchar);
        } else {
            glGetShaderInfoLog(handle, buf.len() as GLsizei, &mut written, buf.as_mut_ptr() as *mut GLchar);
        }
        buf[buf.len() - 1] = 0;
        ruffle_log_cstr(buf.as_ptr() as *const _);
    }
}

fn loc(program: GLuint, name: &[u8]) -> GLint {
    unsafe { glGetUniformLocation(program, name.as_ptr() as *const _) }
}

fn build_solid_program() -> Option<SolidProgram> {
    let program = link_program(SOLID_VERT, SOLID_FRAG)?;
    Some(SolidProgram {
        u_world: loc(program, b"u_world\0"),
        u_mult: loc(program, b"u_mult\0"),
        u_add: loc(program, b"u_add\0"),
        program,
    })
}

fn build_bitmap_program() -> Option<BitmapProgram> {
    let program = link_program(BITMAP_VERT, BITMAP_FRAG)?;
    Some(BitmapProgram {
        u_world: loc(program, b"u_world\0"),
        u_mult: loc(program, b"u_mult\0"),
        u_add: loc(program, b"u_add\0"),
        u_tex: loc(program, b"u_tex\0"),
        u_uv_remap: loc(program, b"u_uv_remap\0"),
        program,
    })
}

fn build_shape_bitmap_program() -> Option<ShapeBitmapProgram> {
    let program = link_program(SHAPE_BITMAP_VERT, SHAPE_BITMAP_FRAG)?;
    Some(ShapeBitmapProgram {
        u_world: loc(program, b"u_world\0"),
        u_mult: loc(program, b"u_mult\0"),
        u_add: loc(program, b"u_add\0"),
        u_tex: loc(program, b"u_tex\0"),
        u_uv: loc(program, b"u_uv\0"),
        u_uv_remap: loc(program, b"u_uv_remap\0"),
        u_wrap_mode: loc(program, b"u_wrap_mode\0"),
        program,
    })
}

fn build_gradient_program() -> Option<GradientProgram> {
    let program = link_program(GRADIENT_VERT, GRADIENT_FRAG)?;
    Some(GradientProgram {
        u_world: loc(program, b"u_world\0"),
        u_mult: loc(program, b"u_mult\0"),
        u_add: loc(program, b"u_add\0"),
        u_tex: loc(program, b"u_tex\0"),
        u_grad_local: loc(program, b"u_grad_local\0"),
        u_grad_kind: loc(program, b"u_grad_kind\0"),
        u_grad_spread: loc(program, b"u_grad_spread\0"),
        u_grad_focal: loc(program, b"u_grad_focal\0"),
        program,
    })
}

fn build_color_matrix_filter_program() -> Option<ColorMatrixFilterProgram> {
    let program = link_program(FILTER_VERT, COLOR_MATRIX_FRAG)?;
    Some(ColorMatrixFilterProgram {
        u_src_uv: loc(program, b"u_src_uv\0"),
        u_color_mat: loc(program, b"u_color_mat\0"),
        u_color_extra: loc(program, b"u_color_extra\0"),
        program,
    })
}

fn build_unpremult_blit_program() -> Option<BlitProgram> {
    let program = link_program(FILTER_VERT, UNPREMULT_FRAG)?;
    Some(BlitProgram {
        u_src_uv: loc(program, b"u_src_uv\0"),
        program,
    })
}

fn build_premult_blit_program() -> Option<BlitProgram> {
    let program = link_program(FILTER_VERT, PREMULT_FRAG)?;
    Some(BlitProgram {
        u_src_uv: loc(program, b"u_src_uv\0"),
        program,
    })
}

fn build_blur_filter_program() -> Option<BlurFilterProgram> {
    let program = link_program(BLUR_VERT, BLUR_FRAG)?;
    Some(BlurFilterProgram {
        u_src_uv: loc(program, b"u_src_uv\0"),
        u_blur_dir: loc(program, b"u_blur_dir\0"),
        u_blur_m: loc(program, b"u_blur_m\0"),
        u_blur_m2: loc(program, b"u_blur_m2\0"),
        u_blur_full_size: loc(program, b"u_blur_full_size\0"),
        u_blur_first_weight: loc(program, b"u_blur_first_weight\0"),
        u_blur_last_offset: loc(program, b"u_blur_last_offset\0"),
        u_blur_last_weight: loc(program, b"u_blur_last_weight\0"),
        program,
    })
}

fn build_glow_filter_program() -> Option<GlowFilterProgram> {
    let program = link_program(GLOW_VERT, GLOW_FRAG)?;
    Some(GlowFilterProgram {
        u_src_uv: loc(program, b"u_src_uv\0"),
        u_blur_uv: loc(program, b"u_blur_uv\0"),
        u_color: loc(program, b"u_color\0"),
        u_strength: loc(program, b"u_strength\0"),
        u_inner: loc(program, b"u_inner\0"),
        u_knockout: loc(program, b"u_knockout\0"),
        u_composite_source: loc(program, b"u_composite_source\0"),
        program,
    })
}

fn build_bevel_filter_program() -> Option<BevelFilterProgram> {
    let program = link_program(BEVEL_VERT, BEVEL_FRAG)?;
    Some(BevelFilterProgram {
        u_src_uv: loc(program, b"u_src_uv\0"),
        u_blur_uv_l: loc(program, b"u_blur_uv_l\0"),
        u_blur_uv_r: loc(program, b"u_blur_uv_r\0"),
        u_highlight: loc(program, b"u_highlight\0"),
        u_shadow: loc(program, b"u_shadow\0"),
        u_strength: loc(program, b"u_strength\0"),
        u_bevel_type: loc(program, b"u_bevel_type\0"),
        u_knockout: loc(program, b"u_knockout\0"),
        program,
    })
}

fn build_alpha_mask_program() -> Option<AlphaMaskProgram> {
    let program = link_program(FILTER_VERT, ALPHA_MASK_FRAG)?;
    Some(AlphaMaskProgram {
        u_src_uv: loc(program, b"u_src_uv\0"),
        program,
    })
}

fn build_complex_blend_program() -> Option<ComplexBlendProgram> {
    let program = link_program(FILTER_VERT, COMPLEX_BLEND_FRAG)?;
    Some(ComplexBlendProgram {
        u_src_uv: loc(program, b"u_src_uv\0"),
        u_blend_mode: loc(program, b"u_blend_mode\0"),
        u_current_flip: loc(program, b"u_current_flip\0"),
        program,
    })
}

// ─── Geometry helpers ─────────────────────────────────────────────────────────

fn build_solid_quad() -> (GLuint, GLuint) {
    #[rustfmt::skip]
    const QUAD: [f32; 36] = [
        0.0, 0.0, 1.0, 1.0, 1.0, 1.0,
        1.0, 0.0, 1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
        0.0, 0.0, 1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
        0.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    ];
    upload_static_vbo_pos_rgba(&QUAD)
}

fn build_bitmap_quad() -> (GLuint, GLuint) {
    // (pos.xy, uv.xy) — 4 floats per vertex × 6 vertices = 24 floats.
    #[rustfmt::skip]
    const QUAD: [f32; 24] = [
        0.0, 0.0, 0.0, 0.0,
        1.0, 0.0, 1.0, 0.0,
        1.0, 1.0, 1.0, 1.0,
        0.0, 0.0, 0.0, 0.0,
        1.0, 1.0, 1.0, 1.0,
        0.0, 1.0, 0.0, 1.0,
    ];
    let mut vao: GLuint = 0;
    let mut vbo: GLuint = 0;
    unsafe {
        glGenVertexArrays(1, &mut vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &mut vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            core::mem::size_of_val(&QUAD) as GLsizeiptr,
            QUAD.as_ptr() as *const _,
            GL_STATIC_DRAW,
        );
        let stride = (4 * core::mem::size_of::<f32>()) as GLsizei;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, core::ptr::null());
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            stride,
            (2 * core::mem::size_of::<f32>()) as *const _,
        );
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    (vao, vbo)
}

fn build_line_segment() -> (GLuint, GLuint) {
    // Unit horizontal line: (0,0) to (1,0) with per-vertex white. Tinted by
    // a per-call DYNAMIC upload before drawing.
    #[rustfmt::skip]
    const LINE: [f32; 12] = [
        0.0, 0.0, 1.0, 1.0, 1.0, 1.0,
        1.0, 0.0, 1.0, 1.0, 1.0, 1.0,
    ];
    upload_static_vbo_pos_rgba(&LINE)
}

fn build_line_rect() -> (GLuint, GLuint) {
    // Four edges of a unit rect as 4 GL_LINES segments (8 vertices).
    #[rustfmt::skip]
    const LINES: [f32; 48] = [
        0.0, 0.0, 1.0, 1.0, 1.0, 1.0,  1.0, 0.0, 1.0, 1.0, 1.0, 1.0,
        1.0, 0.0, 1.0, 1.0, 1.0, 1.0,  1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  0.0, 1.0, 1.0, 1.0, 1.0, 1.0,
        0.0, 1.0, 1.0, 1.0, 1.0, 1.0,  0.0, 0.0, 1.0, 1.0, 1.0, 1.0,
    ];
    upload_static_vbo_pos_rgba(&LINES)
}

/// Build the single VAO used by every shape draw. Bound once per frame
/// during `submit_frame`, then each draw call uses
/// `glDrawElementsBaseVertex` to point at its own slice of the arenas.
///
/// The arena VBO is recorded as the source for attribs 0 (pos.xy) and 1
/// (rgba). The arena IBO is recorded as the VAO's element buffer. These
/// bindings persist for the lifetime of the VAO — `glBufferSubData` calls
/// to upload new shape data later don't disturb them.
fn build_shape_arena_vao(arena_vbo: GLuint, arena_ibo: GLuint) -> GLuint {
    let mut vao: GLuint = 0;
    unsafe {
        glGenVertexArrays(1, &mut vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, arena_vbo);
        let stride = (6 * core::mem::size_of::<f32>()) as GLsizei;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, core::ptr::null());
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            4,
            GL_FLOAT,
            GL_FALSE,
            stride,
            (2 * core::mem::size_of::<f32>()) as *const _,
        );
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arena_ibo);
        glBindVertexArray(0);
        // Unbind GL_ARRAY_BUFFER without disturbing the VAO's recorded
        // attrib bindings (VAO already captured them above). The IBO bind
        // is part of VAO state in core profile, so we don't unbind that.
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    vao
}

/// Upload a (pos+rgba) interleaved f32 buffer to a fresh VAO/VBO with the
/// standard 6-float stride and attribute layout (loc 0 = vec2 pos, loc 1 =
/// vec4 col). Returns (vao, vbo).
fn upload_static_vbo_pos_rgba(verts: &[f32]) -> (GLuint, GLuint) {
    let mut vao: GLuint = 0;
    let mut vbo: GLuint = 0;
    unsafe {
        glGenVertexArrays(1, &mut vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &mut vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            (verts.len() * core::mem::size_of::<f32>()) as GLsizeiptr,
            verts.as_ptr() as *const _,
            GL_STATIC_DRAW,
        );
        let stride = (6 * core::mem::size_of::<f32>()) as GLsizei;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, core::ptr::null());
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            4,
            GL_FLOAT,
            GL_FALSE,
            stride,
            (2 * core::mem::size_of::<f32>()) as *const _,
        );
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    (vao, vbo)
}

/// Upload one tessellated `Draw` and store its draw kind. Returns None for
/// degenerate draws (empty). `bitmap_meta` is `Some` only when the draw is
/// `DrawType::Bitmap` and the source bitmap was successfully resolved.
fn upload_draw(
    draw: &ruffle_render::tessellator::Draw,
    gradient_textures: &[GLuint],
    bitmap_meta: Option<&SwitchBitmapHandle>,
    vertex_arena: &mut BufferArena,
    index_arena: &mut BufferArena,
) -> Option<GpuDraw> {
    if draw.vertices.is_empty() || draw.indices.is_empty() {
        return None;
    }

    // (pos.xy, rgba) interleaved.
    let mut verts: Vec<f32> = Vec::with_capacity(draw.vertices.len() * 6);
    for v in &draw.vertices {
        verts.push(v.x);
        verts.push(v.y);
        verts.push(v.color.r as f32 / 255.0);
        verts.push(v.color.g as f32 / 255.0);
        verts.push(v.color.b as f32 / 255.0);
        verts.push(v.color.a as f32 / 255.0);
    }

    // Allocate space in the global arenas. We pay no glGen* per draw —
    // the data lands inside the single mega-VBO and mega-IBO. The arenas'
    // freelists coalesce frees so long sessions don't fragment too badly.
    let vbo_bytes = (verts.len() * core::mem::size_of::<f32>()) as GLsizeiptr;
    let ibo_bytes = (draw.indices.len() * core::mem::size_of::<u32>()) as GLsizeiptr;
    let vbo_offset = vertex_arena.alloc(vbo_bytes)?;
    let ibo_offset = match index_arena.alloc(ibo_bytes) {
        Some(o) => o,
        None => {
            // Roll back the vertex alloc so we don't leak.
            vertex_arena.free_region(vbo_offset, vbo_bytes);
            return None;
        }
    };
    let verts_bytes: &[u8] = unsafe {
        core::slice::from_raw_parts(
            verts.as_ptr() as *const u8,
            verts.len() * core::mem::size_of::<f32>(),
        )
    };
    let indices_bytes: &[u8] = unsafe {
        core::slice::from_raw_parts(
            draw.indices.as_ptr() as *const u8,
            draw.indices.len() * core::mem::size_of::<u32>(),
        )
    };
    vertex_arena.upload(vbo_offset, verts_bytes);
    index_arena.upload(ibo_offset, indices_bytes);
    // Aligned sizes (what the arenas actually consumed) — needed at free
    // time. Mirror the rounding `alloc` does.
    let vbo_size = ((vbo_bytes + ARENA_VBO_ALIGN - 1) / ARENA_VBO_ALIGN) * ARENA_VBO_ALIGN;
    let ibo_size = ((ibo_bytes + ARENA_IBO_ALIGN - 1) / ARENA_IBO_ALIGN) * ARENA_IBO_ALIGN;

    let kind = match &draw.draw_type {
        DrawType::Color => DrawKind::Solid,
        DrawType::Gradient { matrix, gradient } => {
            // The tessellator's `matrix` is already inverted and normalised
            // by `swf_to_gl_matrix` so that `mat * vec3(vert_pixels, 1)` ∈
            // [0, 1] for linear gradients. Just flatten the [[f32; 3]; 3]
            // (column-major) into the 9-float layout `glUniformMatrix3fv`
            // expects.
            let local_matrix = [
                matrix[0][0], matrix[0][1], matrix[0][2],
                matrix[1][0], matrix[1][1], matrix[1][2],
                matrix[2][0], matrix[2][1], matrix[2][2],
            ];
            let texture_index = *gradient;
            if texture_index >= gradient_textures.len() {
                DrawKind::Solid
            } else {
                DrawKind::Gradient {
                    texture_index,
                    local_matrix,
                    gradient_kind: 0, // refined below by caller
                    spread: 0,
                    focal: 0.0,
                }
            }
        }
        DrawType::Bitmap(b) => {
            let Some(meta) = bitmap_meta else {
                return Some(GpuDraw {
                    vbo_offset,
                    vbo_size,
                    ibo_offset,
                    ibo_size,
                    num_indices: draw.indices.len() as GLsizei,
                    kind: DrawKind::Solid,
                });
            };
            // `b.matrix` maps `a_pos` (shape pixels) to UV in [0,1] of the
            // source bitmap. The shader composes with `u_uv_remap` to land
            // in the atlas sub-rect.
            let local_matrix = [
                b.matrix[0][0], b.matrix[0][1], b.matrix[0][2],
                b.matrix[1][0], b.matrix[1][1], b.matrix[1][2],
                b.matrix[2][0], b.matrix[2][1], b.matrix[2][2],
            ];
            DrawKind::Bitmap {
                atlas_index: meta.atlas_index,
                uv_remap: [meta.u0, meta.v0, meta.u1 - meta.u0, meta.v1 - meta.v0],
                local_matrix,
                is_smoothed: b.is_smoothed,
                is_repeating: b.is_repeating,
            }
        }
    };

    Some(GpuDraw {
        vbo_offset,
        vbo_size,
        ibo_offset,
        ibo_size,
        num_indices: draw.indices.len() as GLsizei,
        kind,
    })
}

/// Bake the gradient stops into a 256x1 RGBA texture. Linear interpolation
/// in sRGB regardless of `interpolation` mode — close enough for 1.3.6 iter 1.
fn build_gradient_texture(g: &Gradient) -> GLuint {
    let mut pixels = [0u8; 256 * 4];
    if g.records.is_empty() {
        // Empty: opaque white. Avoids div-by-zero in the loop below.
        for i in 0..256 {
            pixels[i * 4] = 255;
            pixels[i * 4 + 1] = 255;
            pixels[i * 4 + 2] = 255;
            pixels[i * 4 + 3] = 255;
        }
    } else {
        for i in 0..256 {
            // Find the two records bracketing this position.
            let pos = i as f32 / 255.0;
            let target = (pos * 255.0).round() as u8;
            let (lo, hi) = bracket(g, target);
            let color = if lo.ratio == hi.ratio {
                lo.color.clone()
            } else {
                let t = (target as f32 - lo.ratio as f32) / (hi.ratio as f32 - lo.ratio as f32);
                lerp_color(&lo.color, &hi.color, t)
            };
            pixels[i * 4] = color.r;
            pixels[i * 4 + 1] = color.g;
            pixels[i * 4 + 2] = color.b;
            pixels[i * 4 + 3] = color.a;
        }
    }

    let mut tex: GLuint = 0;
    unsafe {
        glGenTextures(1, &mut tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8 as GLint,
            256,
            1,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels.as_ptr() as *const _,
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR as GLint);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR as GLint);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE as GLint);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE as GLint);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    tex
}

fn bracket(g: &Gradient, target: u8) -> (swf::GradientRecord, swf::GradientRecord) {
    let mut lo = g.records.first().cloned().unwrap();
    let mut hi = g.records.last().cloned().unwrap();
    for r in &g.records {
        if r.ratio <= target {
            lo = r.clone();
        }
        if r.ratio >= target {
            hi = r.clone();
            break;
        }
    }
    (lo, hi)
}

fn lerp_color(a: &Color, b: &Color, t: f32) -> Color {
    let t = t.clamp(0.0, 1.0);
    let mix = |x: u8, y: u8| (x as f32 + (y as f32 - x as f32) * t).round() as u8;
    Color {
        r: mix(a.r, b.r),
        g: mix(a.g, b.g),
        b: mix(a.b, b.b),
        a: mix(a.a, b.a),
    }
}

/// Convert a Ruffle `Bitmap` into an RGBA byte buffer + dims. Returns None
/// for empty or unrecognised formats.
fn bitmap_to_rgba_bytes(bitmap: &Bitmap<'_>) -> Option<(Vec<u8>, u32, u32)> {
    let rgba = bitmap.clone().to_rgba();
    let w = rgba.width();
    let h = rgba.height();
    if w == 0 || h == 0 || rgba.format() != BitmapFormat::Rgba {
        return None;
    }
    Some((rgba.data().to_vec(), w, h))
}

// ─── Downcast helpers ─────────────────────────────────────────────────────────

fn as_switch_shape(handle: &ShapeHandle) -> Option<&SwitchShapeHandle> {
    <dyn Any>::downcast_ref(&*handle.0)
}

fn as_switch_bitmap(handle: &BitmapHandle) -> Option<&SwitchBitmapHandle> {
    <dyn Any>::downcast_ref(&*handle.0)
}

fn as_standalone_bitmap(handle: &BitmapHandle) -> Option<&StandaloneBitmap> {
    <dyn Any>::downcast_ref(&*handle.0)
}

// ─── Backend implementation ───────────────────────────────────────────────────

impl SwitchRenderBackend {
    pub fn new(width: u32, height: u32) -> Option<Self> {
        // Reset cross-instance statics so the diagnostic counters and the
        // pending-frees queue match THIS backend, not whatever the previous
        // one left behind. Without this clear, restarting the Player (e.g.
        // pause-menu REDEMARRER) makes:
        //   - LIVE_GPU_DRAWS/SHAPES briefly noisy if old Drops race with
        //     new register_shape calls (they don't in practice — drops are
        //     synchronous on Arc=0 — but defensive cost is one atomic store).
        //   - PENDING_FREES the actual bug: stale (offset, size) tuples from
        //     the old arena get applied to the NEW fresh arena's freelist
        //     on first submit_frame drain, marking already-free regions as
        //     "double-free" and producing the `arena_v=-2MB(frag 18)`
        //     nonsense in heartbeat logs. Worse, the bogus free regions
        //     would alias with future allocs and silently corrupt draws.
        PENDING_FREES.lock().unwrap().clear();
        LIVE_GPU_DRAWS.store(0, Ordering::Relaxed);
        LIVE_GPU_SHAPES.store(0, Ordering::Relaxed);

        let solid = build_solid_program()?;
        let bitmap_prog = build_bitmap_program()?;
        let shape_bitmap_prog = build_shape_bitmap_program()?;
        let gradient_prog = build_gradient_program()?;
        let color_matrix_filter = build_color_matrix_filter_program()?;
        let unpremult_blit = build_unpremult_blit_program()?;
        let premult_blit = build_premult_blit_program()?;
        let blur_filter = build_blur_filter_program()?;
        let glow_filter = build_glow_filter_program()?;
        let bevel_filter = build_bevel_filter_program()?;
        let alpha_mask_prog = build_alpha_mask_program()?;
        let complex_blend_prog = build_complex_blend_program()?;

        let (rect_vao, rect_vbo) = build_solid_quad();
        let (bitmap_vao, bitmap_vbo) = build_bitmap_quad();
        let (line_vao, line_vbo) = build_line_segment();
        let (line_rect_vao, line_rect_vbo) = build_line_rect();

        // Mega-buffer arena for all shape draws — see the BufferArena
        // comment block at the top of this file for the rationale.
        let vertex_arena = BufferArena::new(
            GL_ARRAY_BUFFER,
            ARENA_VBO_SIZE,
            ARENA_VBO_ALIGN,
        );
        let index_arena = BufferArena::new(
            GL_ELEMENT_ARRAY_BUFFER,
            ARENA_IBO_SIZE,
            ARENA_IBO_ALIGN,
        );
        let shape_vao = build_shape_arena_vao(vertex_arena.gl_id, index_arena.gl_id);

        // The texture samplers `u_tex` in every program are always bound to
        // texture unit 0. Set them once at link time so we don't have to
        // `glUniform1i(u_tex, 0)` on every draw. Mesa caches sampler bindings
        // per-program across glUseProgram switches, so this is permanent.
        unsafe {
            glUseProgram(bitmap_prog.program);
            glUniform1i(bitmap_prog.u_tex, 0);
            glUseProgram(shape_bitmap_prog.program);
            glUniform1i(shape_bitmap_prog.u_tex, 0);
            glUseProgram(gradient_prog.program);
            glUniform1i(gradient_prog.u_tex, 0);
            // Filter programs sample at unit 0; glow additionally samples a
            // pre-blurred source at unit 1.
            glUseProgram(color_matrix_filter.program);
            glUniform1i(loc(color_matrix_filter.program, b"u_tex\0"), 0);
            glUseProgram(unpremult_blit.program);
            glUniform1i(loc(unpremult_blit.program, b"u_tex\0"), 0);
            glUseProgram(premult_blit.program);
            glUniform1i(loc(premult_blit.program, b"u_tex\0"), 0);
            glUseProgram(blur_filter.program);
            glUniform1i(loc(blur_filter.program, b"u_tex\0"), 0);
            glUseProgram(glow_filter.program);
            glUniform1i(loc(glow_filter.program, b"u_tex\0"), 0);
            glUniform1i(loc(glow_filter.program, b"u_blur_tex\0"), 1);
            glUseProgram(bevel_filter.program);
            glUniform1i(loc(bevel_filter.program, b"u_tex\0"), 0);
            glUniform1i(loc(bevel_filter.program, b"u_blur_tex\0"), 1);
            // Two-texture composites: backdrop/maskee at unit 0, mask/current
            // at unit 1.
            glUseProgram(alpha_mask_prog.program);
            glUniform1i(loc(alpha_mask_prog.program, b"u_tex\0"), 0);
            glUniform1i(loc(alpha_mask_prog.program, b"u_mask_tex\0"), 1);
            glUseProgram(complex_blend_prog.program);
            glUniform1i(loc(complex_blend_prog.program, b"u_tex\0"), 0);
            glUniform1i(loc(complex_blend_prog.program, b"u_current_tex\0"), 1);
            glUseProgram(0);
        }

        Some(Self {
            dimensions: ViewportDimensions {
                width,
                height,
                scale_factor: 1.0,
            },
            tessellator: ShapeTessellator::new(),
            solid,
            bitmap_prog,
            shape_bitmap_prog,
            gradient_prog,
            color_matrix_filter,
            unpremult_blit,
            premult_blit,
            blur_filter,
            glow_filter,
            bevel_filter,
            alpha_mask_prog,
            complex_blend_prog,
            blend_window: 0,
            filter_tex_pool: FilterTexturePool::new(),
            offscreen_temp_pool: Vec::new(),
            offscreen_temp_retired: Vec::new(),
            gl_state: GlStateCache::default(),
            rect_vao,
            rect_vbo,
            bitmap_vao,
            bitmap_vbo,
            line_vao,
            line_vbo,
            line_rect_vao,
            line_rect_vbo,
            mask: MaskState::default(),
            warned_unsupported: 0,
            frame_count: 0,
            shapes_registered: 0,
            bitmaps_registered: 0,
            bitmap_draws_emitted: 0,
            heartbeat_tick: 0,
            draw_calls_this_window: 0,
            push_mask_window: 0,
            alpha_mask_window: 0,
            masked_draw_window: 0,
            mask_shape_draw_window: 0,
            cache_entries_max_window: 0,
            render_offscreen_calls: 0,
            apply_filter_calls: 0,
            resolve_sync_calls: 0,
            filters_seen_mask: AtomicU16::new(0),
            bitmap_render_count: 0,
            atlases: Vec::new(),
            vertex_arena,
            index_arena,
            shape_vao,
            offscreen_dims: None,
            offscreen_fbo: 0,
            offscreen_depth_stencil: 0,
            offscreen_depth_stencil_dims: (0, 0),
            frame_snapshot: FrameBreakdown::default(),
            last_frame: FrameBreakdown::default(),
        })
    }

    /// Build the 3x3 column-major matrix that combines (Flash 2x3 affine)
    /// with (pixels → NDC). Sent as the `u_world` uniform.
    ///
    /// Main framebuffer: target = viewport, Y flipped (Flash top → NDC y=+1).
    /// Offscreen FBO (`offscreen_dims`): target = FBO size, NO Y flip so that
    /// Flash top maps to texel y=0 of the result — matching the convention of
    /// CPU-uploaded bitmaps (glTexImage2D row 0 = top = texel y=0), so a later
    /// `render_bitmap` of this texture samples it the same way as any bitmap.
    /// Commands are pre-shifted by Ruffle to target-local coords, so no origin
    /// offset is applied here.
    fn world_matrix(&self, m: &Matrix) -> [GLfloat; 9] {
        let (w, h, flip_y) = match self.offscreen_dims {
            Some((ow, oh)) => (ow.max(1) as f32, oh.max(1) as f32, false),
            None => (
                self.dimensions.width.max(1) as f32,
                self.dimensions.height.max(1) as f32,
                true,
            ),
        };
        let a = m.a;
        let b = m.b;
        let c = m.c;
        let d = m.d;
        let tx = m.tx.to_pixels() as f32;
        let ty = m.ty.to_pixels() as f32;
        let sx = 2.0 / w;
        let sy = if flip_y { -2.0 / h } else { 2.0 / h };
        let ty_off = if flip_y { 1.0 } else { -1.0 };
        [
            a * sx,
            b * sy,
            0.0,
            c * sx,
            d * sy,
            0.0,
            tx * sx - 1.0,
            ty * sy + ty_off,
            1.0,
        ]
    }

    /// Lazy-create the reusable FBO + a shared depth-stencil renderbuffer
    /// sized to cover at least `(w, h)`. The renderbuffer is required so that
    /// stencil masks pushed by `commands.execute()` inside the FBO actually
    /// work (without it the stencil ops no-op and masked sub-trees vanish).
    /// Grows monotonically; attachment persists. Must be called with the FBO
    /// already bound.
    fn ensure_offscreen_depth_stencil(&mut self, w: u32, h: u32) {
        let need_create = self.offscreen_depth_stencil == 0;
        if need_create {
            unsafe {
                let mut rbo: GLuint = 0;
                glGenRenderbuffers(1, &mut rbo);
                self.offscreen_depth_stencil = rbo;
            }
        }
        let (cw, ch) = self.offscreen_depth_stencil_dims;
        let nw = cw.max(w).max(1);
        let nh = ch.max(h).max(1);
        if need_create || nw > cw || nh > ch {
            unsafe {
                glBindRenderbuffer(GL_RENDERBUFFER, self.offscreen_depth_stencil);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, nw as GLsizei, nh as GLsizei);
                glBindRenderbuffer(GL_RENDERBUFFER, 0);
            }
            self.offscreen_depth_stencil_dims = (nw, nh);
        }
        if need_create {
            unsafe {
                glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                    self.offscreen_depth_stencil,
                );
            }
        }
    }

    /// Bind `tex` as the FBO color attachment and replay `commands` into it.
    /// Restores the previous render target + viewport. Returns false if the
    /// FBO is incomplete.
    fn render_commands_to_texture(
        &mut self,
        tex: GLuint,
        tex_w: u32,
        tex_h: u32,
        commands: CommandList,
        clear: Option<Color>,
    ) -> bool {
        if self.offscreen_fbo == 0 {
            unsafe {
                let mut fbo: GLuint = 0;
                glGenFramebuffers(1, &mut fbo);
                self.offscreen_fbo = fbo;
            }
        }
        let mut prev_fbo: GLint = 0;
        let mut prev_vp: [GLint; 4] = [0; 4];
        unsafe {
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mut prev_fbo);
            glGetIntegerv(GL_VIEWPORT, prev_vp.as_mut_ptr());
            glBindFramebuffer(GL_FRAMEBUFFER, self.offscreen_fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        }
        self.ensure_offscreen_depth_stencil(tex_w, tex_h);
        unsafe {
            let status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if status != GL_FRAMEBUFFER_COMPLETE {
                let msg = std::format!("offscreen: FBO incomplete 0x{:04X} ({}x{})\n", status, tex_w, tex_h);
                let mut b = msg.into_bytes();
                b.push(0);
                ruffle_log_cstr(b.as_ptr() as *const _);
                glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo as GLuint);
                return false;
            }
            glViewport(0, 0, tex_w as GLsizei, tex_h as GLsizei);
            glClearStencil(0);
            // `Some(color)` = fresh target (clear to color). `None` = composite
            // mode: the temp was pre-seeded with the BitmapData's existing
            // content (render_offscreen's FreshWithTexture semantics), so keep
            // the colour and only reset the stencil for this pass's masks.
            if let Some(c) = clear {
                glClearColor(
                    c.r as GLfloat / 255.0,
                    c.g as GLfloat / 255.0,
                    c.b as GLfloat / 255.0,
                    c.a as GLfloat / 255.0,
                );
                glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            } else {
                glClear(GL_STENCIL_BUFFER_BIT);
            }
            // Premultiplied-alpha-correct accumulation: standard blend for RGB
            // but accumulate the alpha channel additively, otherwise a cache
            // texture's alpha ends up as `a²` and is too faint when sampled.
            glEnable(GL_BLEND);
            glBlendFuncSeparate(
                GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
            );
            glDisable(GL_STENCIL_TEST);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glStencilMask(0xFF);
        }

        let prev_mask = self.mask;
        self.mask = MaskState::default();
        let prev_offscreen = self.offscreen_dims;
        self.offscreen_dims = Some((tex_w, tex_h));
        self.gl_state.invalidate();

        commands.execute(self);

        self.offscreen_dims = prev_offscreen;
        self.mask = prev_mask;
        unsafe {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo as GLuint);
            glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
            // Restore the main-framebuffer blend (non-separate is fine there).
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        self.gl_state.invalidate();
        true
    }

    /// Generic single-pass filter blit. Binds the reusable FBO to `dst_tex`,
    /// runs `program` over the unit quad sampling `src_tex` (with `src_pt` /
    /// `src_size` defining the sub-rect in source coords), and writes into
    /// `(dst_x, dst_y, dst_w, dst_h)` in destination viewport coords.
    /// `setup_uniforms` is called once the program is bound, before the draw,
    /// to push filter-specific uniforms. Blend is DISABLED (filter passes
    /// overwrite rather than composite) and stencil is OFF. Restores the
    /// previous FBO/viewport. Returns false on FBO incompleteness.
    #[allow(clippy::too_many_arguments)]
    fn draw_filter_pass(
        &mut self,
        program: GLuint,
        u_src_uv_loc: GLint,
        src_tex: GLuint,
        src_w: u32,
        src_h: u32,
        src_pt: (u32, u32),
        src_size: (u32, u32),
        dst_tex: GLuint,
        dst_x: i32,
        dst_y: i32,
        dst_w: u32,
        dst_h: u32,
        setup_uniforms: impl FnOnce(),
    ) -> bool {
        if self.offscreen_fbo == 0 {
            unsafe {
                let mut fbo: GLuint = 0;
                glGenFramebuffers(1, &mut fbo);
                self.offscreen_fbo = fbo;
            }
        }
        let mut prev_fbo: GLint = 0;
        let mut prev_vp: [GLint; 4] = [0; 4];
        unsafe {
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mut prev_fbo);
            glGetIntegerv(GL_VIEWPORT, prev_vp.as_mut_ptr());
            glBindFramebuffer(GL_FRAMEBUFFER, self.offscreen_fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst_tex, 0);
        }
        let need_w = (dst_x.max(0) as u32).saturating_add(dst_w);
        let need_h = (dst_y.max(0) as u32).saturating_add(dst_h);
        self.ensure_offscreen_depth_stencil(need_w, need_h);
        unsafe {
            let status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if status != GL_FRAMEBUFFER_COMPLETE {
                let msg = std::format!("filter pass: FBO incomplete 0x{:04X}\n", status);
                let mut b = msg.into_bytes();
                b.push(0);
                ruffle_log_cstr(b.as_ptr() as *const _);
                glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo as GLuint);
                return false;
            }
            glViewport(dst_x, dst_y, dst_w as GLsizei, dst_h as GLsizei);
            glDisable(GL_BLEND);
            glDisable(GL_STENCIL_TEST);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glUseProgram(program);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, src_tex);
            // Source UV sub-rect: which region of src_tex to sample.
            let su = src_pt.0 as f32 / src_w.max(1) as f32;
            let sv = src_pt.1 as f32 / src_h.max(1) as f32;
            let sw = src_size.0 as f32 / src_w.max(1) as f32;
            let sh = src_size.1 as f32 / src_h.max(1) as f32;
            glUniform4f(u_src_uv_loc, su, sv, sw, sh);
        }
        setup_uniforms();
        unsafe {
            glBindVertexArray(self.bitmap_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glEnable(GL_BLEND);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo as GLuint);
            glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
        }
        self.gl_state.invalidate();
        true
    }

    /// Reorder a 20-float SWF ColorMatrixFilter into the `(mat4, vec4)` pair
    /// the GLSL `color_matrix.wgsl` expects (column-major mat4).
    fn color_matrix_uniforms(matrix: &[f32; 20]) -> ([f32; 16], [f32; 4]) {
        let mat4 = [
            matrix[0], matrix[5], matrix[10], matrix[15],  // col 0 = input r
            matrix[1], matrix[6], matrix[11], matrix[16],  // col 1 = input g
            matrix[2], matrix[7], matrix[12], matrix[17],  // col 2 = input b
            matrix[3], matrix[8], matrix[13], matrix[18],  // col 3 = input a
        ];
        let extras = [matrix[4] / 255.0, matrix[9] / 255.0, matrix[14] / 255.0, matrix[19] / 255.0];
        (mat4, extras)
    }

    /// Identity-blit `(src_tex, src_pt, src_size)` to `(dst_tex, dst_pt, dst_w, dst_h)`
    /// via the ColorMatrix shader with an identity matrix. Used to copy the
    /// final filter target back to a cache entry's destination texture.
    #[allow(clippy::too_many_arguments)]
    fn blit_identity(
        &mut self,
        src_tex: GLuint,
        src_w: u32,
        src_h: u32,
        src_pt: (u32, u32),
        src_size: (u32, u32),
        dst_tex: GLuint,
        dst_pt: (i32, i32),
        dst_w: u32,
        dst_h: u32,
    ) -> bool {
        let prog = self.color_matrix_filter.program;
        let u_src_uv = self.color_matrix_filter.u_src_uv;
        let u_mat = self.color_matrix_filter.u_color_mat;
        let u_extra = self.color_matrix_filter.u_color_extra;
        #[rustfmt::skip]
        let id: [f32; 16] = [
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0,
        ];
        let zero = [0.0_f32; 4];
        self.draw_filter_pass(
            prog, u_src_uv,
            src_tex, src_w, src_h, src_pt, src_size,
            dst_tex, dst_pt.0, dst_pt.1, dst_w, dst_h,
            move || unsafe {
                glUniformMatrix4fv(u_mat, 1, GL_FALSE, id.as_ptr());
                glUniform4f(u_extra, zero[0], zero[1], zero[2], zero[3]);
            },
        )
    }

    /// GPU premultiplied->straight blit of the `(src_pt, src_size)` sub-rect of
    /// `src_tex` into the `(dst_pt, src_size)` sub-rect of `dst_tex`, via
    /// UNPREMULT_FRAG. Used to repatriate a draw() render into an atlas slot
    /// without the per-call `glReadPixels` + CPU un-premultiply + re-upload that
    /// dominated frame time on cacheAsBitmap-heavy AS3 games.
    #[allow(clippy::too_many_arguments)]
    fn blit_unpremult(
        &mut self,
        src_tex: GLuint,
        src_w: u32,
        src_h: u32,
        src_pt: (u32, u32),
        src_size: (u32, u32),
        dst_tex: GLuint,
        dst_pt: (i32, i32),
        dst_w: u32,
        dst_h: u32,
    ) -> bool {
        let prog = self.unpremult_blit.program;
        let u_src_uv = self.unpremult_blit.u_src_uv;
        self.draw_filter_pass(
            prog, u_src_uv,
            src_tex, src_w, src_h, src_pt, src_size,
            dst_tex, dst_pt.0, dst_pt.1, dst_w, dst_h,
            || {},
        )
    }

    /// GPU straight->premultiplied blit (inverse of `blit_unpremult`), via
    /// PREMULT_FRAG. Seeds a render_offscreen temp with a BitmapData's existing
    /// (straight, atlas-stored) content so the new draw() commands composite
    /// onto it instead of replacing it.
    #[allow(clippy::too_many_arguments)]
    fn blit_premult(
        &mut self,
        src_tex: GLuint,
        src_w: u32,
        src_h: u32,
        src_pt: (u32, u32),
        src_size: (u32, u32),
        dst_tex: GLuint,
        dst_pt: (i32, i32),
        dst_w: u32,
        dst_h: u32,
    ) -> bool {
        let prog = self.premult_blit.program;
        let u_src_uv = self.premult_blit.u_src_uv;
        self.draw_filter_pass(
            prog, u_src_uv,
            src_tex, src_w, src_h, src_pt, src_size,
            dst_tex, dst_pt.0, dst_pt.1, dst_w, dst_h,
            || {},
        )
    }

    /// Acquire a temp texture for a `render_offscreen` pass: reuse a pooled one
    /// of the exact size if available (the steady-state case — BitmapData sizes
    /// are stable across frames), else allocate a fresh one.
    /// `render_commands_to_texture` clears it, so stale pooled content is fine.
    fn acquire_offscreen_temp(&mut self, w: u32, h: u32) -> Option<StandaloneTexture> {
        if let Some(i) = self
            .offscreen_temp_pool
            .iter()
            .position(|t| t.width == w && t.height == h)
        {
            return Some(self.offscreen_temp_pool.swap_remove(i));
        }
        make_standalone_texture(w, h)
    }

    /// Read an (x, y, w, h) sub-rect of `tex` back into a CPU RGBA buffer with
    /// STRAIGHT alpha. `tex` is one of our offscreen renders (premultiplied,
    /// texel row 0 = Flash top): attach it to the shared offscreen FBO,
    /// `glReadPixels` (row 0 = y=0 = texel row 0 = top — no Y-flip, exactly what
    /// BitmapData CPU pixels and atlas uploads both expect), then un-premultiply.
    /// Saves/restores the bound FBO since this runs during AS execution, not
    /// inside our frame render. Buffer stride = w*4, row 0 = the region's y_min.
    /// Shared by `resolve_sync_handle` (copyPixels readback) and
    /// `render_offscreen` (repatriating draw() into an atlas-backed handle).
    fn readback_region_straight(&mut self, tex: GLuint, x: u32, y: u32, w: u32, h: u32) -> Vec<u8> {
        let mut buf = vec![0u8; (w as usize) * (h as usize) * 4];
        if w == 0 || h == 0 {
            return buf;
        }
        if self.offscreen_fbo == 0 {
            unsafe {
                let mut fbo: GLuint = 0;
                glGenFramebuffers(1, &mut fbo);
                self.offscreen_fbo = fbo;
            }
        }
        let mut prev_fbo: GLint = 0;
        unsafe {
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mut prev_fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, self.offscreen_fbo);
            glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0,
            );
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(
                x as GLint, y as GLint, w as GLsizei, h as GLsizei,
                GL_RGBA, GL_UNSIGNED_BYTE, buf.as_mut_ptr() as *mut _,
            );
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo as GLuint);
        }
        // The offscreen blend accumulates PREMULTIPLIED alpha; straight-alpha
        // consumers (BitmapData CPU pixels, atlas slots) need un-premultiply —
        // a no-op for opaque pixels (a=255), the common tile-engine case.
        for px in buf.chunks_exact_mut(4) {
            let a = px[3] as u32;
            if a != 0 && a != 255 {
                px[0] = ((px[0] as u32 * 255) / a).min(255) as u8;
                px[1] = ((px[1] as u32 * 255) / a).min(255) as u8;
                px[2] = ((px[2] as u32 * 255) / a).min(255) as u8;
            }
        }
        buf
    }

    /// Apply a ColorMatrixFilter from `source` (full standalone) to
    /// `destination`. Handles source==dest via a pool temp.
    #[allow(clippy::too_many_arguments)]
    fn apply_color_matrix_raw(
        &mut self,
        src_tex: GLuint,
        src_w: u32,
        src_h: u32,
        source_point: (u32, u32),
        source_size: (u32, u32),
        dst_tex: GLuint,
        dest_point: (i32, i32),
        filter: &swf::ColorMatrixFilter,
    ) -> bool {
        let prog = self.color_matrix_filter.program;
        let u_src_uv = self.color_matrix_filter.u_src_uv;
        let u_mat = self.color_matrix_filter.u_color_mat;
        let u_extra = self.color_matrix_filter.u_color_extra;
        let (mat, extras) = Self::color_matrix_uniforms(&filter.matrix);

        if src_tex != dst_tex {
            return self.draw_filter_pass(
                prog, u_src_uv,
                src_tex, src_w, src_h, source_point, source_size,
                dst_tex, dest_point.0, dest_point.1, source_size.0, source_size.1,
                move || unsafe {
                    glUniformMatrix4fv(u_mat, 1, GL_FALSE, mat.as_ptr());
                    glUniform4f(u_extra, extras[0], extras[1], extras[2], extras[3]);
                },
            );
        }
        // In-place: filter into a temp, then identity-blit back.
        let Some(temp) = self.filter_tex_pool.acquire(source_size.0, source_size.1) else { return false };
        let temp_tex = temp.texture;
        let temp_w = temp.width;
        let temp_h = temp.height;
        let ok1 = self.draw_filter_pass(
            prog, u_src_uv,
            src_tex, src_w, src_h, source_point, source_size,
            temp_tex, 0, 0, source_size.0, source_size.1,
            move || unsafe {
                glUniformMatrix4fv(u_mat, 1, GL_FALSE, mat.as_ptr());
                glUniform4f(u_extra, extras[0], extras[1], extras[2], extras[3]);
            },
        );
        if !ok1 {
            self.filter_tex_pool.release(temp);
            return false;
        }
        let ok2 = self.blit_identity(
            temp_tex, temp_w, temp_h, (0, 0), (temp_w, temp_h),
            dst_tex, dest_point, source_size.0, source_size.1,
        );
        self.filter_tex_pool.release(temp);
        ok2
    }

    /// Run the H+V ping-pong loop of a separable blur. Returns the temp
    /// texture holding the blurred result, or None if the blur was impotent
    /// (no axis above 1.0). Caller releases the returned texture to the pool.
    fn run_blur_to_temp(
        &mut self,
        src_tex: GLuint,
        src_w: u32,
        src_h: u32,
        source_point: (u32, u32),
        source_size: (u32, u32),
        filter: &swf::BlurFilter,
    ) -> Option<StandaloneTexture> {
        // Cap blur quality passes at 1. Flash defaults to 3 (a box blur
        // iterated 3× ≈ Gaussian), but each pass is 2 extra FBO draws (H+V) per
        // filtered element — and Mario 63's menu filters dozens of cached text
        // elements per frame, so 3 passes tripled the offscreen draw load and
        // spiked render time. One pass is visually fine for thin glow/shadow
        // outlines and roughly thirds the blur cost.
        let num_passes = (filter.num_passes() as usize).min(1);
        let blur_x = filter.blur_x.to_f32().min(255.0);
        let blur_y = filter.blur_y.to_f32().min(255.0);

        // Neither axis blurs → keep the None contract (glow/bevel synthesise a
        // transparent halo; plain blur passes the source through). Checked up
        // front so the half-res seed below never runs for an impotent blur.
        if blur_x <= 1.0 && blur_y <= 1.0 {
            return None;
        }

        // HALF-RESOLUTION blur. Blur is low-frequency, so we downsample the
        // source, blur at ¼ the fill, and let callers upsample the result via
        // normalised-uv sampling / size-aware blit — visually identical for
        // glow/shadow/bevel halos. This was the dominant per-frame cost on
        // Mario 63's lit scenes: each filter chain ran ~8-11 ms and 9-26 chains
        // fire per frame, so `render` hit 90-260 ms (fps 9-26). The result temp
        // is half-size, which is transparent to callers ONLY because their
        // blur-offset uv divides by the SOURCE size, not the temp pixel size
        // (see apply_glow_or_drop_shadow_raw / apply_bevel_raw). Engage only
        // above a min size so thin outlines stay crisp and tiny surfaces don't
        // pay for the extra downsample.
        let downscale = source_size.0 >= 64 && source_size.1 >= 64;
        let (work_w, work_h, scale) = if downscale {
            ((source_size.0 / 2).max(1), (source_size.1 / 2).max(1), 0.5_f32)
        } else {
            (source_size.0, source_size.1, 1.0_f32)
        };

        let mut flip = self.filter_tex_pool.acquire(work_w, work_h)?;
        let Some(mut flop) = self.filter_tex_pool.acquire(work_w, work_h) else {
            self.filter_tex_pool.release(flip);
            return None;
        };

        // Seed `flip` with the source at work resolution — blit_identity scales
        // full-res src → work-res via linear filtering, which IS the downsample.
        if !self.blit_identity(
            src_tex, src_w, src_h, source_point, source_size,
            flip.texture, (0, 0), work_w, work_h,
        ) {
            self.filter_tex_pool.release(flip);
            self.filter_tex_pool.release(flop);
            return None;
        }

        let prog = self.blur_filter.program;
        let u_src_uv = self.blur_filter.u_src_uv;
        let u_dir = self.blur_filter.u_blur_dir;
        let u_m = self.blur_filter.u_blur_m;
        let u_m2 = self.blur_filter.u_blur_m2;
        let u_full = self.blur_filter.u_blur_full_size;
        let u_first = self.blur_filter.u_blur_first_weight;
        let u_last_off = self.blur_filter.u_blur_last_offset;
        let u_last_wt = self.blur_filter.u_blur_last_weight;

        let mut any_pass = false;
        for _ in 0..num_passes {
            for i in 0..2 {
                let horizontal = i % 2 == 0;
                // Strength is in source pixels; at work resolution each texel
                // spans 1/scale source px, so the kernel radius scales with
                // `scale` to keep the spatial blur the same.
                let strength = if horizontal { blur_x } else { blur_y } * scale;
                let full_size = strength.min(255.0);
                if full_size <= 1.0 { continue; }

                // `flip` is already seeded with the (downsampled) source, so we
                // always ping-pong on the work-res temps.
                let (sample_tex, sample_w, sample_h, sample_pt, sample_sz) =
                    (flip.texture, flip.width, flip.height, (0, 0), (flip.width, flip.height));
                // Fractional-radius fast blur (cf. fgiesen blog post).
                let radius = (full_size - 1.0) / 2.0;
                let m = radius.ceil() - 1.0;
                let alpha = ((radius - m) * 255.0).floor() / 255.0;
                let last_offset = 1.0 / ((1.0 / alpha) + 1.0);
                let last_weight = alpha + 1.0;
                let dir = if horizontal {
                    (1.0_f32 / sample_w.max(1) as f32, 0.0_f32)
                } else {
                    (0.0_f32, 1.0_f32 / sample_h.max(1) as f32)
                };
                let m_val = m;
                let m2_val = m * 2.0;
                let flop_tex = flop.texture;
                let flop_w = flop.width;
                let flop_h = flop.height;
                let ok = self.draw_filter_pass(
                    prog, u_src_uv,
                    sample_tex, sample_w, sample_h, sample_pt, sample_sz,
                    flop_tex, 0, 0, flop_w, flop_h,
                    move || unsafe {
                        glUniform2f(u_dir, dir.0, dir.1);
                        glUniform1f(u_m, m_val);
                        glUniform1f(u_m2, m2_val);
                        glUniform1f(u_full, full_size);
                        glUniform1f(u_first, alpha);
                        glUniform1f(u_last_off, last_offset);
                        glUniform1f(u_last_wt, last_weight);
                    },
                );
                if !ok {
                    self.filter_tex_pool.release(flip);
                    self.filter_tex_pool.release(flop);
                    return None;
                }
                any_pass = true;
                std::mem::swap(&mut flip, &mut flop);
            }
        }
        self.filter_tex_pool.release(flop);
        // `flip` holds the blurred source — or, if both scaled strengths fell
        // below 1 px (a sub-pixel blur on a large surface), merely the
        // downsampled seed, which is itself a valid mild low-freq halo. Either
        // way it's a usable result, so we never fall back to the None path here
        // (we already returned None up front for a truly impotent blur).
        let _ = any_pass;
        Some(flip)
    }

    /// Apply a Blur filter `source` → `destination`.
    #[allow(clippy::too_many_arguments)]
    fn apply_blur_raw(
        &mut self,
        src_tex: GLuint,
        src_w: u32,
        src_h: u32,
        source_point: (u32, u32),
        source_size: (u32, u32),
        dst_tex: GLuint,
        dest_point: (i32, i32),
        filter: &swf::BlurFilter,
    ) -> bool {
        match self.run_blur_to_temp(src_tex, src_w, src_h, source_point, source_size, filter) {
            Some(result) => {
                let rt = result.texture;
                let rw = result.width;
                let rh = result.height;
                let ok = self.blit_identity(
                    rt, rw, rh, (0, 0), (rw, rh),
                    dst_tex, dest_point, source_size.0, source_size.1,
                );
                self.filter_tex_pool.release(result);
                ok
            }
            None => self.blit_identity(
                src_tex, src_w, src_h, source_point, source_size,
                dst_tex, dest_point, source_size.0, source_size.1,
            ),
        }
    }

    /// Apply a Glow (`blur_offset = (0, 0)`) or DropShadow (non-zero offset).
    /// `blur_offset` is in source pixels. Faithful to wgpu's
    /// `vertices_with_blur_offset`: `blur_uv = (source_left + blur_offset) /
    /// source_width`. DropShadow callers pass `(-x, -y)` so the blur sample
    /// at quad top-left lies above-left of source, visible shadow ends up
    /// down-right (the angle=0 convention).
    #[allow(clippy::too_many_arguments)]
    fn apply_glow_or_drop_shadow_raw(
        &mut self,
        src_tex: GLuint,
        src_w: u32,
        src_h: u32,
        source_point: (u32, u32),
        source_size: (u32, u32),
        dst_tex: GLuint,
        dest_point: (i32, i32),
        filter: &swf::GlowFilter,
        blur_offset: (f32, f32),
    ) -> bool {
        let blur_args = filter.inner_blur_filter();
        let blur_temp_opt = self.run_blur_to_temp(
            src_tex, src_w, src_h, source_point, source_size, &blur_args,
        );

        // If blur was impotent, synthesise a fully-transparent temp so the
        // glow shader reads blur_a=0 and outputs the "no glow" tint cleanly.
        // We don't bind the temp's pixel size: the blur temp may be half-res
        // (see run_blur_to_temp), and the blur-offset uv below is computed from
        // the SOURCE size so it's resolution-independent.
        let (blur_tex, blur_temp_to_release) = match blur_temp_opt {
            Some(t) => (t.texture, Some(t)),
            None => {
                let Some(empty) = self.filter_tex_pool.acquire(source_size.0, source_size.1) else {
                    return false;
                };
                // Pool entries may hold stale data — clear to transparent.
                if self.offscreen_fbo == 0 {
                    unsafe {
                        let mut fbo: GLuint = 0;
                        glGenFramebuffers(1, &mut fbo);
                        self.offscreen_fbo = fbo;
                    }
                }
                unsafe {
                    let mut prev_fbo: GLint = 0;
                    let mut prev_vp: [GLint; 4] = [0; 4];
                    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mut prev_fbo);
                    glGetIntegerv(GL_VIEWPORT, prev_vp.as_mut_ptr());
                    glBindFramebuffer(GL_FRAMEBUFFER, self.offscreen_fbo);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, empty.texture, 0);
                    glClearColor(0.0, 0.0, 0.0, 0.0);
                    glClear(GL_COLOR_BUFFER_BIT);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
                    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo as GLuint);
                    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
                }
                (empty.texture, Some(empty))
            }
        };

        let prog = self.glow_filter.program;
        let u_src_uv = self.glow_filter.u_src_uv;
        let u_blur_uv = self.glow_filter.u_blur_uv;
        let u_color = self.glow_filter.u_color;
        let u_strength = self.glow_filter.u_strength;
        let u_inner = self.glow_filter.u_inner;
        let u_knockout = self.glow_filter.u_knockout;
        let u_composite_source = self.glow_filter.u_composite_source;

        // Blur UV remap matches wgpu: at quad (0,0), uv = blur_offset / W; at
        // quad (1,1), uv = 1 + blur_offset / W. Sign is direct (no negation).
        // Divide by the SOURCE size (not the blur temp's pixel size) so the
        // offset stays correct when the blur temp is half-res — the temp spans
        // the same [0,1] spatial region regardless of its resolution.
        let bu0 = blur_offset.0 / source_size.0.max(1) as f32;
        let bv0 = blur_offset.1 / source_size.1.max(1) as f32;
        let color_f = [
            filter.color.r as f32 / 255.0,
            filter.color.g as f32 / 255.0,
            filter.color.b as f32 / 255.0,
            filter.color.a as f32 / 255.0,
        ];
        let strength = filter.strength.to_f32();
        let inner_i: GLint = if filter.is_inner() { 1 } else { 0 };
        let knockout_i: GLint = if filter.is_knockout() { 1 } else { 0 };
        let composite_i: GLint = if filter.composite_source() { 1 } else { 0 };

        unsafe {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, blur_tex);
            glActiveTexture(GL_TEXTURE0);
        }
        let ok = self.draw_filter_pass(
            prog, u_src_uv,
            src_tex, src_w, src_h, source_point, source_size,
            dst_tex, dest_point.0, dest_point.1, source_size.0, source_size.1,
            move || unsafe {
                glUniform4f(u_blur_uv, bu0, bv0, 1.0, 1.0);
                glUniform4f(u_color, color_f[0], color_f[1], color_f[2], color_f[3]);
                glUniform1f(u_strength, strength);
                glUniform1i(u_inner, inner_i);
                glUniform1i(u_knockout, knockout_i);
                glUniform1i(u_composite_source, composite_i);
            },
        );
        unsafe {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }
        if let Some(t) = blur_temp_to_release { self.filter_tex_pool.release(t); }
        ok
    }

    /// Bevel: blur the source alpha, then a composite pass samples that blur at
    /// two opposite offsets (±angle·distance) to make a highlight side and a
    /// shadow side. Faithful port of wgpu's bevel. Mirrors the glow path.
    fn apply_bevel_raw(
        &mut self,
        src_tex: GLuint,
        src_w: u32,
        src_h: u32,
        source_point: (u32, u32),
        source_size: (u32, u32),
        dst_tex: GLuint,
        dest_point: (i32, i32),
        filter: &swf::BevelFilter,
    ) -> bool {
        let blur_args = filter.inner_blur_filter();
        let blur_temp_opt = self.run_blur_to_temp(
            src_tex, src_w, src_h, source_point, source_size, &blur_args,
        );
        let (blur_tex, blur_temp_to_release) = match blur_temp_opt {
            Some(t) => (t.texture, Some(t)),
            None => {
                // Impotent blur → synthesise a transparent temp so both
                // samples read 0 (no highlight/shadow, source passes through).
                let Some(empty) = self.filter_tex_pool.acquire(source_size.0, source_size.1) else {
                    return false;
                };
                if self.offscreen_fbo == 0 {
                    unsafe {
                        let mut fbo: GLuint = 0;
                        glGenFramebuffers(1, &mut fbo);
                        self.offscreen_fbo = fbo;
                    }
                }
                unsafe {
                    let mut prev_fbo: GLint = 0;
                    let mut prev_vp: [GLint; 4] = [0; 4];
                    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mut prev_fbo);
                    glGetIntegerv(GL_VIEWPORT, prev_vp.as_mut_ptr());
                    glBindFramebuffer(GL_FRAMEBUFFER, self.offscreen_fbo);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, empty.texture, 0);
                    glClearColor(0.0, 0.0, 0.0, 0.0);
                    glClear(GL_COLOR_BUFFER_BIT);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
                    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo as GLuint);
                    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
                }
                (empty.texture, Some(empty))
            }
        };

        // ±blur_offset along the filter angle, normalised to the SOURCE size
        // (not the blur temp's pixel size) so the highlight/shadow offset stays
        // correct when the blur temp is half-res — see run_blur_to_temp.
        let distance = filter.distance.to_f32();
        let angle = filter.angle.to_f32();
        let off = (angle.cos() * distance, angle.sin() * distance);
        let bw = source_size.0.max(1) as f32;
        let bh = source_size.1.max(1) as f32;
        let (lu, lv) = (off.0 / bw, off.1 / bh);
        let (ru, rv) = (-off.0 / bw, -off.1 / bh);

        // Premultiplied colors (matches wgpu) — the cache texture is later
        // drawn back with premultiplied "over".
        let prem = |c: swf::Color| {
            let a = c.a as f32 / 255.0;
            [c.r as f32 / 255.0 * a, c.g as f32 / 255.0 * a, c.b as f32 / 255.0 * a, a]
        };
        let hi = prem(filter.highlight_color);
        let sh = prem(filter.shadow_color);
        let strength = filter.strength.to_f32();
        let bevel_type: GLint = if filter.is_on_top() { 2 } else if filter.is_inner() { 1 } else { 0 };
        let knockout_i: GLint = if filter.is_knockout() { 1 } else { 0 };

        let prog = self.bevel_filter.program;
        let u_src_uv = self.bevel_filter.u_src_uv;
        let u_blur_uv_l = self.bevel_filter.u_blur_uv_l;
        let u_blur_uv_r = self.bevel_filter.u_blur_uv_r;
        let u_highlight = self.bevel_filter.u_highlight;
        let u_shadow = self.bevel_filter.u_shadow;
        let u_strength = self.bevel_filter.u_strength;
        let u_bevel_type = self.bevel_filter.u_bevel_type;
        let u_knockout = self.bevel_filter.u_knockout;

        unsafe {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, blur_tex);
            glActiveTexture(GL_TEXTURE0);
        }
        let ok = self.draw_filter_pass(
            prog, u_src_uv,
            src_tex, src_w, src_h, source_point, source_size,
            dst_tex, dest_point.0, dest_point.1, source_size.0, source_size.1,
            move || unsafe {
                glUniform4f(u_blur_uv_l, lu, lv, 1.0, 1.0);
                glUniform4f(u_blur_uv_r, ru, rv, 1.0, 1.0);
                glUniform4f(u_highlight, hi[0], hi[1], hi[2], hi[3]);
                glUniform4f(u_shadow, sh[0], sh[1], sh[2], sh[3]);
                glUniform1f(u_strength, strength);
                glUniform1i(u_bevel_type, bevel_type);
                glUniform1i(u_knockout, knockout_i);
            },
        );
        unsafe {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }
        if let Some(t) = blur_temp_to_release { self.filter_tex_pool.release(t); }
        ok
    }

    /// Filter dispatcher used by both the trait `apply_filter` (for
    /// BitmapData operations Ruffle drives directly) and the
    /// `cache_entries` chain in `submit_frame`. Takes raw texture IDs so the
    /// cache_entries loop can use `FilterTexturePool` temps without wrapping
    /// each one in a `BitmapHandle` Arc (which would tie its lifetime to
    /// the Arc rather than the pool — the perf blocker for filtered scenes).
    #[allow(clippy::too_many_arguments)]
    fn apply_filter_raw(
        &mut self,
        src_tex: GLuint,
        src_w: u32,
        src_h: u32,
        source_point: (u32, u32),
        source_size: (u32, u32),
        dst_tex: GLuint,
        dest_point: (i32, i32),
        filter: &Filter,
    ) -> bool {
        match filter {
            Filter::ColorMatrixFilter(args) => self.apply_color_matrix_raw(
                src_tex, src_w, src_h, source_point, source_size,
                dst_tex, dest_point, args,
            ),
            Filter::BlurFilter(args) => self.apply_blur_raw(
                src_tex, src_w, src_h, source_point, source_size,
                dst_tex, dest_point, args,
            ),
            Filter::GlowFilter(args) => self.apply_glow_or_drop_shadow_raw(
                src_tex, src_w, src_h, source_point, source_size,
                dst_tex, dest_point, args, (0.0, 0.0),
            ),
            Filter::DropShadowFilter(args) => {
                let inner = args.inner_glow_filter();
                let dist = args.distance.to_f32();
                let angle = args.angle.to_f32();
                let x = angle.cos() * dist;
                let y = angle.sin() * dist;
                self.apply_glow_or_drop_shadow_raw(
                    src_tex, src_w, src_h, source_point, source_size,
                    dst_tex, dest_point, &inner, (-x, -y),
                )
            }
            Filter::BevelFilter(args) => self.apply_bevel_raw(
                src_tex, src_w, src_h, source_point, source_size,
                dst_tex, dest_point, args,
            ),
            _ => false,
        }
    }

    /// Pixel dimensions of whatever we're currently rendering into: the main
    /// framebuffer normally, or the active offscreen FBO when replaying
    /// commands into a cache/blend/mask texture.
    fn current_target_dims(&self) -> (u32, u32) {
        match self.offscreen_dims {
            Some((w, h)) => (w, h),
            None => (self.dimensions.width, self.dimensions.height),
        }
    }

    /// Draw a standalone texture covering the whole current target (full-screen
    /// quad), reusing the proven standalone-`render_bitmap` path (bitmap shader
    /// + bitmap_vao + Y-flip-aware `world_matrix`), but with a caller-chosen GL
    /// blend state set just before the draw. Always restores the default
    /// premultiplied-over blend afterwards. `tex` is assumed premultiplied with
    /// texel row 0 = Flash top (every offscreen render we produce is).
    fn draw_fullscreen_texture(&mut self, tex: GLuint, tw: u32, th: u32, set_blend: impl FnOnce()) {
        let scaled = Matrix::scale(tw as f32, th as f32);
        let world = self.world_matrix(&scaled);
        const IDENT_MULT: [f32; 4] = [1.0, 1.0, 1.0, 1.0];
        const IDENT_ADD: [f32; 4] = [0.0, 0.0, 0.0, 0.0];
        let uv_remap = [0.0, 0.0, 1.0, 1.0];
        self.use_bitmap(&world, &IDENT_MULT, &IDENT_ADD, tex, &uv_remap);
        self.gl_state.bind_vao(self.bitmap_vao);
        self.draw_calls_this_window = self.draw_calls_this_window.saturating_add(1);
        set_blend();
        unsafe {
            glDrawArrays(GL_TRIANGLES, 0, 6);
            // Restore the main-pass blend so following draws composite normally.
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    }

    /// Composite a soft alpha mask: `result_tex` ← maskee × mask.alpha. Both
    /// inputs and the output share the offscreen "row 0 = Flash top" layout, so
    /// the combine FBO pass samples them straight. Returns false on FBO failure.
    fn composite_alpha_mask(
        &mut self,
        maskee_tex: GLuint,
        mask_tex: GLuint,
        result_tex: GLuint,
        w: u32,
        h: u32,
    ) -> bool {
        unsafe {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mask_tex);
            glActiveTexture(GL_TEXTURE0);
        }
        let ok = self.draw_filter_pass(
            self.alpha_mask_prog.program,
            self.alpha_mask_prog.u_src_uv,
            maskee_tex, w, h, (0, 0), (w, h),
            result_tex, 0, 0, w, h,
            || {},
        );
        unsafe {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }
        ok
    }

    /// Run a complex blend (multiply/overlay/…) straight onto the current
    /// target: a full-screen quad samples the backdrop snapshot (`parent_tex`,
    /// unit 0) and the freshly-rendered blend group (`current_tex`, unit 1),
    /// outputs the full composite, and overwrites the target with blending
    /// DISABLED. `flip` (0/1) flips the current sampler's V on the main
    /// framebuffer (Y-flipped) vs an offscreen target (not flipped).
    fn composite_complex_to_current(
        &mut self,
        parent_tex: GLuint,
        current_tex: GLuint,
        w: u32,
        h: u32,
        mode: i32,
        flip: f32,
    ) {
        let prog = self.complex_blend_prog.program;
        let u_src_uv = self.complex_blend_prog.u_src_uv;
        let u_blend_mode = self.complex_blend_prog.u_blend_mode;
        let u_current_flip = self.complex_blend_prog.u_current_flip;
        unsafe {
            glViewport(0, 0, w as GLsizei, h as GLsizei);
            glDisable(GL_BLEND);
            glDisable(GL_STENCIL_TEST);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glStencilMask(0xFF);
            glUseProgram(prog);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, parent_tex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, current_tex);
            glUniform4f(u_src_uv, 0.0, 0.0, 1.0, 1.0);
            glUniform1i(u_blend_mode, mode);
            glUniform1f(u_current_flip, flip);
            glBindVertexArray(self.bitmap_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        self.draw_calls_this_window = self.draw_calls_this_window.saturating_add(1);
        // The direct glUseProgram/bind above bypassed the state cache.
        self.gl_state.invalidate();
    }

    fn use_solid(&self, world: &[GLfloat; 9], mult: &[f32; 4], add: &[f32; 4]) {
        self.gl_state.use_program(self.solid.program);
        unsafe {
            glUniformMatrix3fv(self.solid.u_world, 1, GL_FALSE, world.as_ptr());
            glUniform4f(self.solid.u_mult, mult[0], mult[1], mult[2], mult[3]);
            glUniform4f(self.solid.u_add, add[0], add[1], add[2], add[3]);
        }
    }

    fn use_bitmap(
        &self,
        world: &[GLfloat; 9],
        mult: &[f32; 4],
        add: &[f32; 4],
        tex: GLuint,
        uv_remap: &[f32; 4],
    ) {
        // Sampler binding (u_tex = 0) set once at program link; no per-draw
        // glUniform1i(u_tex) needed here.
        self.gl_state.use_program(self.bitmap_prog.program);
        self.gl_state.bind_texture_unit0(tex);
        unsafe {
            glUniformMatrix3fv(self.bitmap_prog.u_world, 1, GL_FALSE, world.as_ptr());
            glUniform4f(self.bitmap_prog.u_mult, mult[0], mult[1], mult[2], mult[3]);
            glUniform4f(self.bitmap_prog.u_add, add[0], add[1], add[2], add[3]);
            glUniform4f(
                self.bitmap_prog.u_uv_remap,
                uv_remap[0], uv_remap[1], uv_remap[2], uv_remap[3],
            );
        }
    }

    fn use_shape_bitmap(
        &self,
        world: &[GLfloat; 9],
        mult: &[f32; 4],
        add: &[f32; 4],
        tex: GLuint,
        uv_matrix: &[GLfloat; 9],
        uv_remap: &[f32; 4],
        is_repeating: bool,
    ) {
        // Atlas texture parameters are set once at atlas creation; no per-
        // draw glTexParameteri (avoids per-frame state churn that bisection
        // on 2026-05-24 implicated in a Mario 63 driver-side issue).
        // u_wrap_mode and u_tex sampler are routed through the GL state
        // cache so identical-state runs of draws (very common for atlas
        // bitmap fills) only hit the driver once.
        self.gl_state.use_program(self.shape_bitmap_prog.program);
        self.gl_state.bind_texture_unit0(tex);
        // u_wrap_mode: 0 = clamp (default for non-repeating fills),
        // 1 = fract (for tile/repeat fills like Mario 63 ground).
        self.gl_state.set_wrap_mode(
            self.shape_bitmap_prog.u_wrap_mode,
            if is_repeating { 1 } else { 0 },
        );
        unsafe {
            glUniformMatrix3fv(self.shape_bitmap_prog.u_world, 1, GL_FALSE, world.as_ptr());
            glUniform4f(self.shape_bitmap_prog.u_mult, mult[0], mult[1], mult[2], mult[3]);
            glUniform4f(self.shape_bitmap_prog.u_add, add[0], add[1], add[2], add[3]);
            glUniformMatrix3fv(self.shape_bitmap_prog.u_uv, 1, GL_FALSE, uv_matrix.as_ptr());
            glUniform4f(
                self.shape_bitmap_prog.u_uv_remap,
                uv_remap[0], uv_remap[1], uv_remap[2], uv_remap[3],
            );
        }
    }

    fn use_gradient(
        &self,
        world: &[GLfloat; 9],
        mult: &[f32; 4],
        add: &[f32; 4],
        tex: GLuint,
        local_matrix: &[GLfloat; 9],
        kind: i32,
        spread: i32,
        focal: f32,
    ) {
        self.gl_state.use_program(self.gradient_prog.program);
        self.gl_state.bind_texture_unit0(tex);
        unsafe {
            glUniformMatrix3fv(self.gradient_prog.u_world, 1, GL_FALSE, world.as_ptr());
            glUniform4f(self.gradient_prog.u_mult, mult[0], mult[1], mult[2], mult[3]);
            glUniform4f(self.gradient_prog.u_add, add[0], add[1], add[2], add[3]);
            glUniformMatrix3fv(self.gradient_prog.u_grad_local, 1, GL_FALSE, local_matrix.as_ptr());
            glUniform1i(self.gradient_prog.u_grad_kind, kind);
            glUniform1i(self.gradient_prog.u_grad_spread, spread);
            glUniform1f(self.gradient_prog.u_grad_focal, focal);
        }
    }

    /// Pack a bitmap's RGBA pixels into one of our atlases. Returns the
    /// SwitchBitmapHandle metadata, or None if the bitmap is too big for
    /// the atlas size.
    fn pack_into_atlas(
        &mut self,
        pixels: &[u8],
        width: u32,
        height: u32,
    ) -> Option<SwitchBitmapHandle> {
        // A bitmap bigger than the atlas in either axis can NEVER be packed.
        // Bail out here — before the new-atlas path below would allocate a
        // doomed 16 MB GPU texture (`Atlas::new` → glGenTextures + glTexImage2D)
        // only for `pack` to then fail. That wasted allocation, under GPU memory
        // pressure, is what preceded a Mesa NULL-deref native crash (DataAbort,
        // FAR=0x98) on haunt-the-house's 3400x1600 background (atlas is 2048²).
        // Returning None makes register_bitmap report TooLarge cleanly, so
        // Ruffle no-ops that one oversized bitmap instead of taking down the app.
        if width > ATLAS_SIZE || height > ATLAS_SIZE {
            self.warn_once(b"pack_into_atlas: bitmap exceeds 2048 atlas, skipped (no crash)\n\0");
            return None;
        }
        for (idx, atlas) in self.atlases.iter_mut().enumerate() {
            if let Some((x, y)) = atlas.pack(width, height) {
                atlas.upload_region_padded(x, y, width, height, pixels);
                let inv = 1.0 / ATLAS_SIZE as f32;
                return Some(SwitchBitmapHandle {
                    atlas_index: idx,
                    u0: x as f32 * inv,
                    v0: y as f32 * inv,
                    u1: (x + width) as f32 * inv,
                    v1: (y + height) as f32 * inv,
                    width,
                    height,
                });
            }
        }
        // No room — allocate a new atlas (16 MB GPU texture).
        let new_atlas_index = self.atlases.len();
        let msg = std::format!(
            "atlas: allocating #{} (16 MB) for {}x{}\n",
            new_atlas_index, width, height,
        );
        let mut bytes = msg.into_bytes();
        bytes.push(0);
        unsafe { ruffle_log_cstr(bytes.as_ptr() as *const _) };
        let mut atlas = Atlas::new(ATLAS_SIZE);
        let Some((x, y)) = atlas.pack(width, height) else {
            return None;
        };
        atlas.upload_region_padded(x, y, width, height, pixels);
        self.atlases.push(atlas);
        let inv = 1.0 / ATLAS_SIZE as f32;
        Some(SwitchBitmapHandle {
            atlas_index: new_atlas_index,
            u0: x as f32 * inv,
            v0: y as f32 * inv,
            u1: (x + width) as f32 * inv,
            v1: (y + height) as f32 * inv,
            width,
            height,
        })
    }

    fn warn_once(&mut self, msg: &[u8]) {
        if self.warned_unsupported < 8 {
            self.warned_unsupported += 1;
            log(msg);
        }
    }

    /// Snapshot the raw counters that feed `FrameBreakdown`, so a per-frame
    /// delta can be taken across `submit_frame`. The window counters
    /// (draw_calls/blend/pushmask/masked_draw) only grow within a single frame
    /// except on the heartbeat frame, where the heartbeat zeroes them — see the
    /// caveat on `frame_snapshot`.
    fn frame_counters(&self) -> FrameBreakdown {
        FrameBreakdown {
            draw_calls: self.draw_calls_this_window,
            offscreen: self.render_offscreen_calls,
            filter: self.apply_filter_calls,
            resolve: self.resolve_sync_calls,
            bmp_uploads: self.bitmaps_registered,
            shape_regs: self.shapes_registered,
            blend: self.blend_window,
            pushmask: self.push_mask_window,
            masked_draw: self.masked_draw_window,
            cache_entries: 0,
            filter_chains: 0,
        }
    }

    /// Emit a one-line breakdown for a frame that blew the FPS budget. Called
    /// from lib.rs's `render_frame_with_dt` once it knows the frame's wall time
    /// (tick + render). `last_frame` was filled at the end of `submit_frame`.
    /// Timings are microseconds. This fires only on slow frames, so it never
    /// floods nxlink during smooth play but captures every spike with the
    /// activity that caused it.
    pub fn log_slow_frame(&self, total_us: u64, tick_us: u64, render_us: u64) {
        let fb = self.last_frame;
        // Backend-primitive time during this frame's tick (LAST_* snapshotted at
        // submit_frame). primOffs = render_offscreen incl. draw() repatriation;
        // primBmp = bitmap register/upload; primRes = copyPixels resolve. tick
        // huge + these ~0 ⇒ pure AVM2 (upstream); one dominating ⇒ our backend.
        let tick_freq = unsafe { ruffle_tick_freq() };
        let to_us = |t: u64| if tick_freq > 0 { t.saturating_mul(1_000_000) / tick_freq } else { 0 };
        let prim_offs = to_us(PRIM_OFFSCREEN_LAST.load(std::sync::atomic::Ordering::Relaxed));
        let prim_bmp = to_us(PRIM_BMPUP_LAST.load(std::sync::atomic::Ordering::Relaxed));
        let prim_res = to_us(PRIM_RESOLVE_LAST.load(std::sync::atomic::Ordering::Relaxed));
        let off_alloc = to_us(PRIM_OFF_ALLOC_LAST.load(std::sync::atomic::Ordering::Relaxed));
        let off_render = to_us(PRIM_OFF_RENDER_LAST.load(std::sync::atomic::Ordering::Relaxed));
        let off_readback = to_us(PRIM_OFF_READBACK_LAST.load(std::sync::atomic::Ordering::Relaxed));
        let off_upload = to_us(PRIM_OFF_UPLOAD_LAST.load(std::sync::atomic::Ordering::Relaxed));
        let off_n = PRIM_OFF_N_LAST.load(std::sync::atomic::Ordering::Relaxed);
        let off_pix = PRIM_OFF_PIX_LAST.load(std::sync::atomic::Ordering::Relaxed);
        let msg = std::format!(
            "SLOW f{} {}us (tick {}us render {}us) primOffs={}us primBmp={}us primRes={}us dc={} offs={} filt={}({}chains) resolve={} bmpUp={} shpReg={} blend={} pmask={} mdraw={} cacheEnt={} | offN={} offPix={} alloc={}us render={}us readback={}us upload={}us\n",
            self.frame_count,
            total_us, tick_us, render_us,
            prim_offs, prim_bmp, prim_res,
            fb.draw_calls, fb.offscreen, fb.filter, fb.filter_chains,
            fb.resolve, fb.bmp_uploads, fb.shape_regs,
            fb.blend, fb.pushmask, fb.masked_draw, fb.cache_entries,
            off_n, off_pix, off_alloc, off_render, off_readback, off_upload,
        );
        let mut bytes = msg.into_bytes();
        bytes.push(0);
        unsafe { ruffle_log_cstr(bytes.as_ptr() as *const _) };
    }

    /// Draw a small white crosshair at the given screen pixel position.
    /// Intended to be called *after* `submit_frame` has returned so it
    /// overlays the player's rendering rather than getting cleared away.
    /// Re-binds the GL state we'd left in a fresh state at end of submit.
    pub fn draw_cursor_overlay(&mut self, x: f32, y: f32, clicked: bool) {
        const BAR_W: f32 = 24.0;
        const BAR_H: f32 = 4.0;
        // Red when clicked, white otherwise. Helps confirm clicks register.
        let color = if clicked {
            swf::Color::from_rgb(0xFF1744, 255)
        } else {
            swf::Color::from_rgb(0xFFFFFF, 255)
        };
        // Horizontal bar centred on (x, y).
        let h_mat = Matrix {
            a: BAR_W,
            b: 0.0,
            c: 0.0,
            d: BAR_H,
            tx: swf::Twips::from_pixels((x - BAR_W * 0.5) as f64),
            ty: swf::Twips::from_pixels((y - BAR_H * 0.5) as f64),
        };
        // Vertical bar.
        let v_mat = Matrix {
            a: BAR_H,
            b: 0.0,
            c: 0.0,
            d: BAR_W,
            tx: swf::Twips::from_pixels((x - BAR_H * 0.5) as f64),
            ty: swf::Twips::from_pixels((y - BAR_W * 0.5) as f64),
        };
        unsafe {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_STENCIL_TEST);
        }
        // Reuse CommandHandler's draw_rect path. It binds program + VAO and
        // uploads a fresh dynamic quad each call.
        <Self as CommandHandler>::draw_rect(self, color, h_mat);
        <Self as CommandHandler>::draw_rect(self, color, v_mat);
        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        // We just zeroed program + VAO, but the cache thinks they are still
        // bound. Invalidate so the next frame's first draw re-binds.
        self.gl_state.invalidate();
    }

    /// Draw an ASCII string with the embedded 5x7 pixel font (see `GLYPHS`).
    /// `x`, `y` are top-left in screen pixels. Each lit glyph pixel becomes
    /// a `scale × scale` solid rect drawn via the same path as `draw_rect`.
    /// Unknown chars render as blank space.
    pub fn draw_text(&mut self, x: f32, y: f32, scale: f32, text: &str, color: swf::Color) {
        let mut cur_x = x;
        for ch in text.chars() {
            // Uppercase fold — our font only carries A-Z.
            let lookup = ch.to_ascii_uppercase();
            if let Some((_, pattern)) = GLYPHS.iter().find(|(c, _)| *c == lookup) {
                for (row_idx, row_str) in pattern.iter().enumerate() {
                    for (col_idx, pixel) in row_str.chars().enumerate() {
                        if pixel != ' ' {
                            let px = cur_x + col_idx as f32 * scale;
                            let py = y + row_idx as f32 * scale;
                            let mat = Matrix {
                                a: scale,
                                b: 0.0,
                                c: 0.0,
                                d: scale,
                                tx: swf::Twips::from_pixels(px as f64),
                                ty: swf::Twips::from_pixels(py as f64),
                            };
                            <Self as CommandHandler>::draw_rect(self, color, mat);
                        }
                    }
                }
            }
            // Advance by 6 px (5-wide glyph + 1-px gap), scaled.
            cur_x += 6.0 * scale;
        }
    }

    /// Measure rendered width of `text` in pixels at the given scale.
    /// Lets the menu centre items horizontally.
    pub fn measure_text(&self, text: &str, scale: f32) -> f32 {
        text.chars().count() as f32 * 6.0 * scale
    }

    /// Draw the pause-modal overlay on top of whatever's already in the
    /// framebuffer. The caller is expected to have re-rendered the paused
    /// game state (via `Player::render`) so this overlay sits over a frozen
    /// snapshot of the game, not a blank screen.
    ///
    /// `selected` indexes `MENU_ITEMS`. The cursor `>` is drawn on the
    /// selected row; the selected label is rendered in yellow, others in
    /// white. Help line at the bottom describes the buttons.
    pub fn draw_menu_overlay(&mut self, selected: usize) {
        // Re-bind blend / disable stencil, same as the cursor overlay.
        unsafe {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_STENCIL_TEST);
        }
        let vw = self.dimensions.width as f32;
        let vh = self.dimensions.height as f32;

        // Full-screen dim backdrop (50 % black). Hides the game so the
        // user's eye snaps to the menu.
        let backdrop = Matrix {
            a: vw,
            b: 0.0,
            c: 0.0,
            d: vh,
            tx: swf::Twips::from_pixels(0.0),
            ty: swf::Twips::from_pixels(0.0),
        };
        // 50 % black backdrop (AA=0x80).
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0x80_00_00_00), backdrop);

        // Centred panel — sized so 1280x720 fits 3 items + title comfortably.
        const PANEL_W: f32 = 520.0;
        const PANEL_H: f32 = 380.0;
        let panel_x = (vw - PANEL_W) * 0.5;
        let panel_y = (vh - PANEL_H) * 0.5;
        let panel = Matrix {
            a: PANEL_W,
            b: 0.0,
            c: 0.0,
            d: PANEL_H,
            tx: swf::Twips::from_pixels(panel_x as f64),
            ty: swf::Twips::from_pixels(panel_y as f64),
        };
        // Dark blue-ish panel background (alpha 240/255 → mostly opaque).
        // Dark navy panel at ~94% alpha so a hint of the paused frame shows
        // through. AARRGGBB = F0 14 20 38.
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0xF0_14_20_38), panel);
        // 1-px-style border via line rect at panel scale.
        let border = Matrix {
            a: PANEL_W,
            b: 0.0,
            c: 0.0,
            d: PANEL_H,
            tx: swf::Twips::from_pixels(panel_x as f64),
            ty: swf::Twips::from_pixels(panel_y as f64),
        };
        <Self as CommandHandler>::draw_line_rect(
            self,
            swf::Color::from_rgb(0xFFFFFF, 255),
            border,
        );

        // Title.
        const TITLE_SCALE: f32 = 5.0;
        let title = "PAUSE";
        let title_w = self.measure_text(title, TITLE_SCALE);
        let title_x = panel_x + (PANEL_W - title_w) * 0.5;
        let title_y = panel_y + 30.0;
        self.draw_text(
            title_x,
            title_y,
            TITLE_SCALE,
            title,
            swf::Color::from_rgb(0xFFFFFF, 255),
        );

        // Menu items.
        const ITEM_SCALE: f32 = 3.0;
        const ITEM_SPACING: f32 = 50.0;
        let items_y = panel_y + 130.0;
        let item_color_selected = swf::Color::from_rgb(0xFFD740, 255); // amber
        let item_color_normal = swf::Color::from_rgb(0xCCCCCC, 255);
        // Pre-measure the longest item so all rows share a left margin.
        let longest = MENU_ITEMS
            .iter()
            .map(|s| s.chars().count())
            .max()
            .unwrap_or(0) as f32;
        let block_w = (longest + 2.0) * 6.0 * ITEM_SCALE; // 2 chars left padding for ">  "
        let items_x = panel_x + (PANEL_W - block_w) * 0.5;
        for (i, item) in MENU_ITEMS.iter().enumerate() {
            let y = items_y + i as f32 * ITEM_SPACING;
            let color = if i == selected {
                item_color_selected
            } else {
                item_color_normal
            };
            if i == selected {
                self.draw_text(items_x, y, ITEM_SCALE, ">", color);
            }
            // Always render label at the same x (cursor occupies first slot).
            let label_x = items_x + 2.0 * 6.0 * ITEM_SCALE;
            self.draw_text(label_x, y, ITEM_SCALE, item, color);
        }

        // Footer help line.
        const HELP_SCALE: f32 = 2.0;
        let help = "A:OK   B:ANNULER   HAUT/BAS:NAV";
        let help_w = self.measure_text(help, HELP_SCALE);
        let help_x = panel_x + (PANEL_W - help_w) * 0.5;
        let help_y = panel_y + PANEL_H - 40.0;
        self.draw_text(
            help_x,
            help_y,
            HELP_SCALE,
            help,
            swf::Color::from_rgb(0x99AABB, 255),
        );

        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        self.gl_state.invalidate();
    }

    /// TOUCHES list screen — the keymap editor. Shows up to `visible_rows`
    /// entries of `bindings` (Switch-button name + current Flash-key
    /// binding) starting at `scroll_offset`. The row at `selection` is
    /// highlighted in amber with a `>` cursor.
    pub fn draw_touches_list(
        &mut self,
        selection: usize,
        scroll_offset: usize,
        bindings: &[(&str, Option<std::string::String>)],
        visible_rows: usize,
    ) {
        unsafe {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_STENCIL_TEST);
        }
        let vw = self.dimensions.width as f32;
        let vh = self.dimensions.height as f32;

        // Same backdrop + panel framing as the pause menu — visually links
        // the two screens.
        let backdrop = Matrix {
            a: vw, b: 0.0, c: 0.0, d: vh,
            tx: swf::Twips::from_pixels(0.0),
            ty: swf::Twips::from_pixels(0.0),
        };
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0x80_00_00_00), backdrop);

        const PANEL_W: f32 = 720.0;
        const PANEL_H: f32 = 600.0;
        let panel_x = (vw - PANEL_W) * 0.5;
        let panel_y = (vh - PANEL_H) * 0.5;
        let panel = Matrix {
            a: PANEL_W, b: 0.0, c: 0.0, d: PANEL_H,
            tx: swf::Twips::from_pixels(panel_x as f64),
            ty: swf::Twips::from_pixels(panel_y as f64),
        };
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0xF0_14_20_38), panel);
        <Self as CommandHandler>::draw_line_rect(
            self,
            swf::Color::from_rgb(0xFFFFFF, 255),
            panel,
        );

        // Title.
        const TITLE_SCALE: f32 = 4.0;
        let title = "TOUCHES";
        let title_w = self.measure_text(title, TITLE_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - title_w) * 0.5,
            panel_y + 25.0,
            TITLE_SCALE,
            title,
            swf::Color::from_rgb(0xFFFFFF, 255),
        );

        // Rows.
        const ROW_SCALE: f32 = 3.0;
        const ROW_SPACING: f32 = 50.0;
        let rows_top_y = panel_y + 130.0;
        let rows_left_x = panel_x + 80.0;
        // Right column = binding value, aligned to a fixed x.
        let value_col_x = panel_x + 360.0;

        let total = bindings.len();
        let end = (scroll_offset + visible_rows).min(total);
        for (visible_idx, abs_idx) in (scroll_offset..end).enumerate() {
            let (btn, binding) = &bindings[abs_idx];
            let y = rows_top_y + visible_idx as f32 * ROW_SPACING;
            let is_sel = abs_idx == selection;
            let color = if is_sel {
                swf::Color::from_rgb(0xFFD740, 255) // amber
            } else {
                swf::Color::from_rgb(0xCCCCCC, 255)
            };
            if is_sel {
                self.draw_text(rows_left_x - 30.0, y, ROW_SCALE, ">", color);
            }
            self.draw_text(rows_left_x, y, ROW_SCALE, btn, color);
            let value_str = binding
                .as_deref()
                .unwrap_or("(aucune)");
            // Brackets around the value to suggest "editable field".
            let bracketed = std::format!("[ {} ]", value_str);
            self.draw_text(value_col_x, y, ROW_SCALE, &bracketed, color);
        }

        // Scroll indicator on the right edge if the list is longer than
        // what's visible.
        if total > visible_rows {
            let bar_x = panel_x + PANEL_W - 30.0;
            let bar_top_y = rows_top_y;
            let bar_h_total = visible_rows as f32 * ROW_SPACING;
            let bar_h_thumb = (bar_h_total * visible_rows as f32 / total as f32).max(20.0);
            let progress = scroll_offset as f32 / (total - visible_rows) as f32;
            let thumb_y = bar_top_y + (bar_h_total - bar_h_thumb) * progress;
            // Bar track (faint).
            let track = Matrix {
                a: 4.0, b: 0.0, c: 0.0, d: bar_h_total,
                tx: swf::Twips::from_pixels(bar_x as f64),
                ty: swf::Twips::from_pixels(bar_top_y as f64),
            };
            <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0x40_99AABB), track);
            // Thumb.
            let thumb = Matrix {
                a: 4.0, b: 0.0, c: 0.0, d: bar_h_thumb,
                tx: swf::Twips::from_pixels(bar_x as f64),
                ty: swf::Twips::from_pixels(thumb_y as f64),
            };
            <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgb(0xFFD740, 255), thumb);
        }

        // Footer.
        const HELP_SCALE: f32 = 2.0;
        let help = "A:EDITER   B:RETOUR   HAUT/BAS:NAV";
        let help_w = self.measure_text(help, HELP_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - help_w) * 0.5,
            panel_y + PANEL_H - 40.0,
            HELP_SCALE,
            help,
            swf::Color::from_rgb(0x99AABB, 255),
        );

        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        self.gl_state.invalidate();
    }

    /// TOUCHES dropdown — shown when the user presses A on a list row.
    /// Scrollable window so we can fit the full 48-key Flash keyboard
    /// without overflowing the 720-px viewport. `visible_rows` rows are
    /// drawn at a time starting from `scroll_offset`; selection is
    /// always within this window (caller maintains via clamp_scroll).
    pub fn draw_touches_dropdown(
        &mut self,
        button_name: &str,
        selection: usize,
        scroll_offset: usize,
        options: &[&str],
        visible_rows: usize,
    ) {
        unsafe {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_STENCIL_TEST);
        }
        let vw = self.dimensions.width as f32;
        let vh = self.dimensions.height as f32;

        // Full-screen dim (slightly deeper than the list backdrop so the
        // dropdown reads as a modal-over-modal).
        let backdrop = Matrix {
            a: vw, b: 0.0, c: 0.0, d: vh,
            tx: swf::Twips::from_pixels(0.0),
            ty: swf::Twips::from_pixels(0.0),
        };
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0xB0_00_00_00), backdrop);

        const PANEL_W: f32 = 480.0;
        let row_h: f32 = 40.0;
        // Panel sized for at most `visible_rows` rows + header + footer.
        // No longer grows with total options count — that was the bug
        // when ALL_FLASH_KEYS jumped from 12 to 48 entries.
        let panel_h = 130.0 + visible_rows as f32 * row_h + 60.0;
        let panel_x = (vw - PANEL_W) * 0.5;
        let panel_y = (vh - panel_h) * 0.5;
        let panel = Matrix {
            a: PANEL_W, b: 0.0, c: 0.0, d: panel_h,
            tx: swf::Twips::from_pixels(panel_x as f64),
            ty: swf::Twips::from_pixels(panel_y as f64),
        };
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0xF0_14_20_38), panel);
        <Self as CommandHandler>::draw_line_rect(
            self,
            swf::Color::from_rgb(0xFFFFFF, 255),
            panel,
        );

        // Title.
        const TITLE_SCALE: f32 = 3.0;
        let title = std::format!("{} ->", button_name);
        let title_w = self.measure_text(&title, TITLE_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - title_w) * 0.5,
            panel_y + 25.0,
            TITLE_SCALE,
            &title,
            swf::Color::from_rgb(0xFFFFFF, 255),
        );

        // Options (windowed). Slice the list to scroll_offset..end and
        // index back to absolute selection for the highlight check.
        const OPT_SCALE: f32 = 2.5;
        let opts_top_y = panel_y + 110.0;
        let opts_left_x = panel_x + 100.0;
        let total = options.len();
        let end = (scroll_offset + visible_rows).min(total);
        for (visible_idx, abs_idx) in (scroll_offset..end).enumerate() {
            let y = opts_top_y + visible_idx as f32 * row_h;
            let is_sel = abs_idx == selection;
            let color = if is_sel {
                swf::Color::from_rgb(0xFFD740, 255)
            } else {
                swf::Color::from_rgb(0xCCCCCC, 255)
            };
            if is_sel {
                self.draw_text(opts_left_x - 30.0, y, OPT_SCALE, ">", color);
            }
            self.draw_text(opts_left_x, y, OPT_SCALE, options[abs_idx], color);
        }

        // Scrollbar (matches the TOUCHES list scrollbar style).
        if total > visible_rows {
            let bar_x = panel_x + PANEL_W - 30.0;
            let bar_top_y = opts_top_y;
            let bar_h_total = visible_rows as f32 * row_h;
            let bar_h_thumb = (bar_h_total * visible_rows as f32 / total as f32).max(20.0);
            let progress = scroll_offset as f32 / (total - visible_rows) as f32;
            let thumb_y = bar_top_y + (bar_h_total - bar_h_thumb) * progress;
            let track = Matrix {
                a: 4.0, b: 0.0, c: 0.0, d: bar_h_total,
                tx: swf::Twips::from_pixels(bar_x as f64),
                ty: swf::Twips::from_pixels(bar_top_y as f64),
            };
            <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0x40_99AABB), track);
            let thumb = Matrix {
                a: 4.0, b: 0.0, c: 0.0, d: bar_h_thumb,
                tx: swf::Twips::from_pixels(bar_x as f64),
                ty: swf::Twips::from_pixels(thumb_y as f64),
            };
            <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgb(0xFFD740, 255), thumb);
        }

        // Footer.
        const HELP_SCALE: f32 = 2.0;
        let help = "A:OK   B:ANNULER   HAUT/BAS:NAV";
        let help_w = self.measure_text(help, HELP_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - help_w) * 0.5,
            panel_y + panel_h - 35.0,
            HELP_SCALE,
            help,
            swf::Color::from_rgb(0x99AABB, 255),
        );

        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        self.gl_state.invalidate();
    }

    // ── Library UI (Phase 3.4) ──────────────────────────────────────────

    /// Upload an RGBA8 byte buffer as a standalone GL texture (not packed
    /// into any atlas). Used by the library boot path to upload
    /// `assets/banner.png` as a single texture that survives until the
    /// library renderer is dropped. Returns the GL id, or 0 on failure.
    pub fn upload_rgba_texture(&mut self, rgba: &[u8], width: u32, height: u32) -> GLuint {
        if width == 0 || height == 0 || rgba.len() < (width as usize) * (height as usize) * 4 {
            return 0;
        }
        let mut tex: GLuint = 0;
        unsafe {
            glGenTextures(1, &mut tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA8 as GLint,
                width as GLsizei,
                height as GLsizei,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                rgba.as_ptr() as *const _,
            );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR as GLint);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR as GLint);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE as GLint);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE as GLint);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        // The cache thinks unit 0 is bound to whatever was there before. We
        // just clobbered it via the upload binds + unbind — invalidate so
        // the next draw re-binds correctly.
        self.gl_state.invalidate();
        tex
    }

    /// Draw a screen-aligned axis-aligned textured rectangle. Uses the
    /// existing `bitmap_prog` + unit-quad VAO; no per-call buffer upload.
    /// Identity color transform (mult=1, add=0).
    pub fn draw_textured_rect(
        &mut self,
        x: f32,
        y: f32,
        w: f32,
        h: f32,
        tex: GLuint,
    ) {
        if tex == 0 || w <= 0.0 || h <= 0.0 {
            return;
        }
        let mat = Matrix {
            a: w,
            b: 0.0,
            c: 0.0,
            d: h,
            tx: swf::Twips::from_pixels(x as f64),
            ty: swf::Twips::from_pixels(y as f64),
        };
        let world = self.world_matrix(&mat);
        let mult = [1.0, 1.0, 1.0, 1.0];
        let add = [0.0, 0.0, 0.0, 0.0];
        let uv_remap = [0.0, 0.0, 1.0, 1.0];
        self.use_bitmap(&world, &mult, &add, tex, &uv_remap);
        self.gl_state.bind_vao(self.bitmap_vao);
        unsafe {
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    /// Full-screen black clear used at the top of each library render. We
    /// own the framebuffer here (no Ruffle behind us pre-init).
    pub fn library_clear(&mut self) {
        unsafe {
            glDisable(GL_STENCIL_TEST);
            glDisable(GL_BLEND);
            glClearColor(0.078, 0.125, 0.219, 1.0); // dark navy, matches panels
            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        self.gl_state.invalidate();
    }

    /// Empty-state screen — no `.swf` found on SD. Shows where to drop files.
    pub fn draw_library_empty(&mut self) {
        self.library_clear();
        let vw = self.dimensions.width as f32;
        let vh = self.dimensions.height as f32;

        let title = "AUCUN JEU";
        let scale_title = 6.0;
        let title_w = self.measure_text(title, scale_title);
        // Drop shadow on the title — dark navy offset (4, 4) under the white.
        self.draw_text(
            (vw - title_w) * 0.5 + 4.0,
            vh * 0.30 + 4.0,
            scale_title,
            title,
            swf::Color::from_rgb(0x000000, 255),
        );
        self.draw_text(
            (vw - title_w) * 0.5,
            vh * 0.30,
            scale_title,
            title,
            swf::Color::from_rgb(0xFFD740, 255),
        );

        let lines = [
            "DEPOSEZ DES FICHIERS .SWF DANS",
            "SDMC:/RUFFLE/   OU   SDMC:/SWITCH/RUFFLE/",
            "PUIS REDEMARREZ FLASHNX.",
        ];
        let scale_msg = 2.5;
        let mut y = vh * 0.48;
        for line in &lines {
            let w = self.measure_text(line, scale_msg);
            self.draw_text(
                (vw - w) * 0.5,
                y,
                scale_msg,
                line,
                swf::Color::from_rgb(0xCCCCCC, 255),
            );
            y += 40.0;
        }

        // Footer: Y opens DISTANT mode (so a user with empty SD can
        // still import via archive.org without needing to drop files
        // on SD first); - exits .nro.
        const HELP_SCALE: f32 = 2.0;
        let help = "Y:IMPORT DISTANT   -:QUITTER";
        let help_w = self.measure_text(help, HELP_SCALE);
        self.draw_text(
            (vw - help_w) * 0.5,
            vh - 60.0,
            HELP_SCALE,
            help,
            swf::Color::from_rgb(0x99AABB, 255),
        );
        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        self.gl_state.invalidate();
    }

    /// Main library list. Reads `entries` (snapshot — caller drops the
    /// Mutex before this call), draws the title banner + N visible rows +
    /// metadata panel + footer help. Animations (cursor pulse, selection
    /// pulse) driven by `phase_ticks` (system ticks since the library
    /// opened — see `library::State::anim_origin_ticks`).
    #[allow(clippy::too_many_arguments)]
    pub fn draw_library_list(
        &mut self,
        selection: usize,
        scroll_offset: usize,
        entries: &[crate::library::Entry],
        visible_rows: usize,
        banner_tex: GLuint,
        banner_w: u32,
        banner_h: u32,
        phase_ticks: u64,
    ) {
        self.library_clear();
        let vw = self.dimensions.width as f32;
        let vh = self.dimensions.height as f32;

        // Phase in seconds (tick_freq ≈ 19.2 MHz on Switch — same as Ruffle's
        // pacing clock). We pass it through `sinf`-style math without
        // pulling in libm; (phase % period) is enough for visual pulses.
        let phase_s = (phase_ticks as f64) / (unsafe { ruffle_tick_freq() } as f64);
        // Sine via Taylor — pulls in no extra deps, accurate enough for
        // ±5 % amplitude visual pulses. Period 1.6 s reads as "subtle
        // breathing" rather than "annoying flash".
        let pulse = approx_sin(phase_s as f32 * (2.0 * core::f32::consts::PI / 1.6));

        // ── Banner (or ASCII title fallback) ─────────────────────────
        let banner_y = 24.0;
        if banner_tex != 0 && banner_w > 0 && banner_h > 0 {
            // Centre at 720×144 (asset spec). If banner is larger we down-
            // scale to fit the viewport with a 32-px side margin.
            let max_w = vw - 64.0;
            let scale = (max_w / banner_w as f32).min(1.0);
            let draw_w = banner_w as f32 * scale;
            let draw_h = banner_h as f32 * scale;
            let draw_x = (vw - draw_w) * 0.5;
            self.draw_textured_rect(draw_x, banner_y, draw_w, draw_h, banner_tex);
        } else {
            // ASCII fallback — same look as the empty-state title but
            // smaller so it still leaves room for the list.
            let title = "FLASHNX";
            let scale_title = 5.0;
            let title_w = self.measure_text(title, scale_title);
            // Drop shadow.
            self.draw_text(
                (vw - title_w) * 0.5 + 3.0,
                banner_y + 30.0 + 3.0,
                scale_title,
                title,
                swf::Color::from_rgb(0x000000, 255),
            );
            self.draw_text(
                (vw - title_w) * 0.5,
                banner_y + 30.0,
                scale_title,
                title,
                swf::Color::from_rgb(0xFFD740, 255),
            );
        }

        // ── Game list ───────────────────────────────────────────────────
        const ROW_SCALE: f32 = 3.0;
        const ROW_SPACING: f32 = 50.0;
        const CHIP_PAD: f32 = 12.0;
        const CHIP_SIZE: f32 = 18.0;
        let rows_top_y = banner_y + 170.0;
        let rows_left_x = 80.0;
        let chip_x = rows_left_x;
        let label_x = rows_left_x + CHIP_SIZE + CHIP_PAD * 2.0;

        let total = entries.len();
        let end = (scroll_offset + visible_rows).min(total);
        for (visible_idx, abs_idx) in (scroll_offset..end).enumerate() {
            let entry = &entries[abs_idx];
            let y = rows_top_y + visible_idx as f32 * ROW_SPACING;
            let is_sel = abs_idx == selection;

            // Color chip — small square in the per-game hash color.
            let chip_color = swf::Color::from_rgb(entry.color_chip, 255);
            let chip_mat = Matrix {
                a: CHIP_SIZE, b: 0.0, c: 0.0, d: CHIP_SIZE,
                tx: swf::Twips::from_pixels(chip_x as f64),
                ty: swf::Twips::from_pixels((y + 4.0) as f64),
            };
            <Self as CommandHandler>::draw_rect(self, chip_color, chip_mat);

            // Label color: amber for selection (pulsing), light grey otherwise.
            let color = if is_sel {
                // Pulse amber [0xFFD740] ↔ [0xFFEC8B]. Linear blend on each
                // channel; 0.5 (pulse+1)/2 stays in [0,1].
                let p = (pulse * 0.5) + 0.5;
                let r = (0xFF as f32 + (0xFF - 0xFF) as f32 * p) as u32;
                let g = (0xD7 as f32 + (0xEC - 0xD7) as f32 * p) as u32;
                let b = (0x40 as f32 + (0x8B - 0x40) as f32 * p) as u32;
                swf::Color::from_rgb((r << 16) | (g << 8) | b, 255)
            } else {
                swf::Color::from_rgb(0xCCCCCC, 255)
            };
            if is_sel {
                // Animated cursor — `►` shape, x position breathes by a few
                // pixels each cycle.
                let cursor_dx = pulse * 4.0;
                self.draw_text(label_x - 36.0 + cursor_dx, y, ROW_SCALE, ">", color);
            }
            // Truncate display name if it would overflow the visible area.
            // Scale 3 ≈ 18 px/char ; label_x ≈ 122, scrollbar ~vw-30=1250,
            // → ~60 chars fit. 40 keeps comfortable visual margin before
            // hitting the scrollbar / metadata panel edges.
            let max_chars = 40usize;
            let display = if entry.display_name.chars().count() > max_chars {
                let mut t: std::string::String = entry.display_name.chars().take(max_chars - 1).collect();
                t.push('…');
                t
            } else {
                entry.display_name.clone()
            };
            self.draw_text(label_x, y, ROW_SCALE, &display, color);
        }

        // Scrollbar on the right edge if needed.
        if total > visible_rows {
            let bar_x = vw - 30.0;
            let bar_top_y = rows_top_y;
            let bar_h_total = visible_rows as f32 * ROW_SPACING;
            let bar_h_thumb = (bar_h_total * visible_rows as f32 / total as f32).max(20.0);
            let progress = scroll_offset as f32 / (total - visible_rows) as f32;
            let thumb_y = bar_top_y + (bar_h_total - bar_h_thumb) * progress;
            let track = Matrix {
                a: 4.0, b: 0.0, c: 0.0, d: bar_h_total,
                tx: swf::Twips::from_pixels(bar_x as f64),
                ty: swf::Twips::from_pixels(bar_top_y as f64),
            };
            <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0x40_99AABB), track);
            let thumb = Matrix {
                a: 4.0, b: 0.0, c: 0.0, d: bar_h_thumb,
                tx: swf::Twips::from_pixels(bar_x as f64),
                ty: swf::Twips::from_pixels(thumb_y as f64),
            };
            <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgb(0xFFD740, 255), thumb);
        }

        // ── Metadata panel (bottom strip) ──────────────────────────────
        if let Some(entry) = entries.get(selection) {
            let panel_y = vh - 130.0;
            let panel_x = 40.0;
            let panel_w = vw - 80.0;
            let panel_h = 70.0;
            let panel_mat = Matrix {
                a: panel_w, b: 0.0, c: 0.0, d: panel_h,
                tx: swf::Twips::from_pixels(panel_x as f64),
                ty: swf::Twips::from_pixels(panel_y as f64),
            };
            <Self as CommandHandler>::draw_rect(
                self,
                swf::Color::from_rgba(0xC0_20_2C_40),
                panel_mat,
            );

            // Display name (larger).
            self.draw_text(
                panel_x + 20.0,
                panel_y + 10.0,
                3.0,
                &entry.display_name,
                swf::Color::from_rgb(0xFFFFFF, 255),
            );
            // Sub-line: size · compression · version, plus a neutral engine
            // tag. Stage dims dropped — 3.8 forces ShowAll + letterbox so native
            // dims are no longer user-facing info. AS3 (AVM2) games get a soft
            // amber "AS3" marker — purely informational, NOT a verdict: Ruffle's
            // AVM2 is less complete so AS3 is the "here be dragons" engine, yet
            // plenty of AS3 games (Mario Forever, Flappy Bird, Tetris'd) run
            // perfectly while others (Pursuit of Hat) don't. So we flag the
            // engine, not "broken". AS2 is the norm and needs no tag.
            let size_str = format_size_pretty(entry.size_bytes);
            let (meta, meta_color) = if entry.is_as3 {
                (
                    std::format!(
                        "{} // SWF V{} {} // AS3",
                        size_str, entry.swf_version, entry.compression_label,
                    ),
                    0xE0B24D,
                )
            } else {
                (
                    std::format!(
                        "{} // SWF V{} {}",
                        size_str, entry.swf_version, entry.compression_label,
                    ),
                    0xAABFD8,
                )
            };
            self.draw_text(
                panel_x + 20.0,
                panel_y + 42.0,
                2.0,
                &meta,
                swf::Color::from_rgb(meta_color, 255),
            );

            // Tiny basename in lower-right (so the user always sees the
            // physical filename — matters when display_name diverges in
            // Phase 3.4.bis RENOMMER).
            let bn_str = std::format!("[{}]", entry.basename);
            let bn_w = self.measure_text(&bn_str, 1.5);
            self.draw_text(
                panel_x + panel_w - bn_w - 20.0,
                panel_y + panel_h - 22.0,
                1.5,
                &bn_str,
                swf::Color::from_rgb(0x7A8A9C, 255),
            );
        }

        // Footer.
        const HELP_SCALE: f32 = 2.0;
        let help = "A:JOUER   X:OPTIONS   Y:IMPORT   -:QUITTER";
        let help_w = self.measure_text(help, HELP_SCALE);
        self.draw_text(
            (vw - help_w) * 0.5,
            vh - 42.0,
            HELP_SCALE,
            help,
            swf::Color::from_rgb(0x99AABB, 255),
        );

        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        self.gl_state.invalidate();
    }

    /// OPTIONS modal — small panel showing the game name + per-game options.
    /// v1: only TOUCHES + RETOUR.
    pub fn draw_library_options(
        &mut self,
        game_display_name: &str,
        selection: usize,
        options: &[&str],
    ) {
        unsafe {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_STENCIL_TEST);
        }
        let vw = self.dimensions.width as f32;
        let vh = self.dimensions.height as f32;

        let backdrop = Matrix {
            a: vw, b: 0.0, c: 0.0, d: vh,
            tx: swf::Twips::from_pixels(0.0),
            ty: swf::Twips::from_pixels(0.0),
        };
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0xB0_00_00_00), backdrop);

        const PANEL_W: f32 = 520.0;
        let row_h: f32 = 50.0;
        let panel_h = 180.0 + options.len() as f32 * row_h + 60.0;
        let panel_x = (vw - PANEL_W) * 0.5;
        let panel_y = (vh - panel_h) * 0.5;
        let panel = Matrix {
            a: PANEL_W, b: 0.0, c: 0.0, d: panel_h,
            tx: swf::Twips::from_pixels(panel_x as f64),
            ty: swf::Twips::from_pixels(panel_y as f64),
        };
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0xF0_14_20_38), panel);
        <Self as CommandHandler>::draw_line_rect(self, swf::Color::from_rgb(0xFFFFFF, 255), panel);

        // Header.
        const TITLE_SCALE: f32 = 3.0;
        let header = "OPTIONS";
        let header_w = self.measure_text(header, TITLE_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - header_w) * 0.5,
            panel_y + 25.0,
            TITLE_SCALE,
            header,
            swf::Color::from_rgb(0xFFFFFF, 255),
        );
        // Game name (sub-title).
        const SUB_SCALE: f32 = 2.0;
        // Truncate game name to fit the panel.
        let max_chars = 28usize;
        let sub = if game_display_name.chars().count() > max_chars {
            let mut t: std::string::String = game_display_name.chars().take(max_chars - 1).collect();
            t.push('…');
            t
        } else {
            game_display_name.to_string()
        };
        let sub_w = self.measure_text(&sub, SUB_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - sub_w) * 0.5,
            panel_y + 75.0,
            SUB_SCALE,
            &sub,
            swf::Color::from_rgb(0xAABFD8, 255),
        );

        // Options list.
        const OPT_SCALE: f32 = 2.5;
        let opts_top_y = panel_y + 140.0;
        let opts_left_x = panel_x + 120.0;
        for (i, opt) in options.iter().enumerate() {
            let y = opts_top_y + i as f32 * row_h;
            let is_sel = i == selection;
            let color = if is_sel {
                swf::Color::from_rgb(0xFFD740, 255)
            } else {
                swf::Color::from_rgb(0xCCCCCC, 255)
            };
            if is_sel {
                self.draw_text(opts_left_x - 30.0, y, OPT_SCALE, ">", color);
            }
            self.draw_text(opts_left_x, y, OPT_SCALE, opt, color);
        }

        // Footer.
        const HELP_SCALE: f32 = 2.0;
        let help = "A:OK   B:RETOUR";
        let help_w = self.measure_text(help, HELP_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - help_w) * 0.5,
            panel_y + panel_h - 38.0,
            HELP_SCALE,
            help,
            swf::Color::from_rgb(0x99AABB, 255),
        );

        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        self.gl_state.invalidate();
    }

    /// Destructive-confirm modal for OPTIONS > SUPPRIMER. Bigger / redder
    /// than `draw_library_options` because the action is irreversible.
    pub fn draw_library_delete_confirm(
        &mut self,
        game_display_name: &str,
        basename: &str,
    ) {
        unsafe {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_STENCIL_TEST);
        }
        let vw = self.dimensions.width as f32;
        let vh = self.dimensions.height as f32;

        let backdrop = Matrix {
            a: vw, b: 0.0, c: 0.0, d: vh,
            tx: swf::Twips::from_pixels(0.0),
            ty: swf::Twips::from_pixels(0.0),
        };
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0xCC_00_00_00), backdrop);

        const PANEL_W: f32 = 720.0;
        const PANEL_H: f32 = 360.0;
        let panel_x = (vw - PANEL_W) * 0.5;
        let panel_y = (vh - PANEL_H) * 0.5;
        let panel = Matrix {
            a: PANEL_W, b: 0.0, c: 0.0, d: PANEL_H,
            tx: swf::Twips::from_pixels(panel_x as f64),
            ty: swf::Twips::from_pixels(panel_y as f64),
        };
        // Dark red panel to signal danger.
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0xF0_40_10_18), panel);
        <Self as CommandHandler>::draw_line_rect(self, swf::Color::from_rgb(0xFF6060, 255), panel);

        const TITLE_SCALE: f32 = 4.0;
        let header = "SUPPRIMER ?";
        let header_w = self.measure_text(header, TITLE_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - header_w) * 0.5,
            panel_y + 30.0,
            TITLE_SCALE,
            header,
            swf::Color::from_rgb(0xFFD740, 255),
        );

        const NAME_SCALE: f32 = 2.5;
        let max_chars = 36usize;
        let display = if game_display_name.chars().count() > max_chars {
            let mut t: std::string::String = game_display_name.chars().take(max_chars - 1).collect();
            t.push('…');
            t
        } else {
            game_display_name.to_string()
        };
        let dw = self.measure_text(&display, NAME_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - dw) * 0.5,
            panel_y + 105.0,
            NAME_SCALE,
            &display,
            swf::Color::from_rgb(0xFFFFFF, 255),
        );

        const SUB_SCALE: f32 = 1.5;
        let bn = std::format!("[{}]", basename);
        let bnw = self.measure_text(&bn, SUB_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - bnw) * 0.5,
            panel_y + 145.0,
            SUB_SCALE,
            &bn,
            swf::Color::from_rgb(0xCCAAAA, 255),
        );

        const WARN_SCALE: f32 = 2.0;
        let warn1 = "Le fichier .swf, les sauvegardes (.sol),";
        let warn2 = "les touches et l'alias seront effaces.";
        let w1w = self.measure_text(warn1, WARN_SCALE);
        let w2w = self.measure_text(warn2, WARN_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - w1w) * 0.5,
            panel_y + 195.0,
            WARN_SCALE,
            warn1,
            swf::Color::from_rgb(0xFFEEDD, 255),
        );
        self.draw_text(
            panel_x + (PANEL_W - w2w) * 0.5,
            panel_y + 225.0,
            WARN_SCALE,
            warn2,
            swf::Color::from_rgb(0xFFEEDD, 255),
        );
        let irrev = "Action irreversible.";
        let iw = self.measure_text(irrev, WARN_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - iw) * 0.5,
            panel_y + 260.0,
            WARN_SCALE,
            irrev,
            swf::Color::from_rgb(0xFF9090, 255),
        );

        const HELP_SCALE: f32 = 2.5;
        let help = "A: SUPPRIMER     B: ANNULER";
        let help_w = self.measure_text(help, HELP_SCALE);
        self.draw_text(
            panel_x + (PANEL_W - help_w) * 0.5,
            panel_y + PANEL_H - 50.0,
            HELP_SCALE,
            help,
            swf::Color::from_rgb(0xFFFFFF, 255),
        );

        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        self.gl_state.invalidate();
    }

    // ── Phase 3.7: DISTANT mode screens ────────────────────────────────

    /// Splash for `Screen::DistantIdle`. `recent_url` is the
    /// currently-displayed history entry (if any); `hist_pos` is
    /// `(current, total)` for the "[3 / 12]" badge. When history is
    /// empty we show the static "press A to type URL" CTA.
    pub fn draw_library_distant_idle(
        &mut self,
        recent_url: Option<&str>,
        hist_pos: Option<(usize, usize)>,
    ) {
        self.library_clear();
        let vw = self.dimensions.width as f32;
        let vh = self.dimensions.height as f32;

        // Title with drop shadow.
        let title = "IMPORT DISTANT";
        let scale_t = 5.0;
        let tw = self.measure_text(title, scale_t);
        self.draw_text(
            (vw - tw) * 0.5 + 4.0,
            vh * 0.13 + 4.0,
            scale_t,
            title,
            swf::Color::from_rgb(0x000000, 255),
        );
        self.draw_text(
            (vw - tw) * 0.5,
            vh * 0.13,
            scale_t,
            title,
            swf::Color::from_rgb(0xFFD740, 255),
        );

        // Subtitle.
        let sub = "TELECHARGEMENT DE SWF DEPUIS ARCHIVE.ORG";
        let scale_s = 2.0;
        let sw = self.measure_text(sub, scale_s);
        self.draw_text(
            (vw - sw) * 0.5,
            vh * 0.24,
            scale_s,
            sub,
            swf::Color::from_rgb(0xAABFD8, 255),
        );

        // Big CTA / history panel.
        const PANEL_W: f32 = 1120.0;
        const PANEL_H: f32 = 280.0;
        let panel_x = (vw - PANEL_W) * 0.5;
        let panel_y = vh * 0.34;
        let panel = Matrix {
            a: PANEL_W, b: 0.0, c: 0.0, d: PANEL_H,
            tx: swf::Twips::from_pixels(panel_x as f64),
            ty: swf::Twips::from_pixels(panel_y as f64),
        };
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0xC0_14_20_38), panel);
        <Self as CommandHandler>::draw_line_rect(self, swf::Color::from_rgb(0xFFD740, 255), panel);

        match recent_url {
            None => {
                // No history yet — just the "press A" CTA.
                let cta = "APPUYEZ SUR A POUR SAISIR UNE URL";
                let scale_c = 3.0;
                let cw = self.measure_text(cta, scale_c);
                self.draw_text(
                    panel_x + (PANEL_W - cw) * 0.5,
                    panel_y + 80.0,
                    scale_c,
                    cta,
                    swf::Color::from_rgb(0xFFFFFF, 255),
                );
                let hint = "EXEMPLE: HTTPS://ARCHIVE.ORG/DETAILS/<ITEM-ID>";
                let scale_h = 1.8;
                let hw = self.measure_text(hint, scale_h);
                self.draw_text(
                    panel_x + (PANEL_W - hw) * 0.5,
                    panel_y + 150.0,
                    scale_h,
                    hint,
                    swf::Color::from_rgb(0x99AABB, 255),
                );
                let hint2 = "OU SIMPLEMENT <ITEM-ID>";
                let hw2 = self.measure_text(hint2, scale_h);
                self.draw_text(
                    panel_x + (PANEL_W - hw2) * 0.5,
                    panel_y + 185.0,
                    scale_h,
                    hint2,
                    swf::Color::from_rgb(0x99AABB, 255),
                );
            }
            Some(url) => {
                // Show "HISTORIQUE [n / total]" header.
                let scale_lbl = 2.0;
                let label = if let Some((cur, total)) = hist_pos {
                    std::format!("HISTORIQUE [{} / {}]", cur, total)
                } else {
                    std::string::String::from("HISTORIQUE")
                };
                let lw = self.measure_text(&label, scale_lbl);
                self.draw_text(
                    panel_x + (PANEL_W - lw) * 0.5,
                    panel_y + 30.0,
                    scale_lbl,
                    &label,
                    swf::Color::from_rgb(0xFFD740, 255),
                );

                // URL truncated if too wide. Scale 2 → 12 px per char.
                let scale_u = 2.0;
                let char_w = 6.0 * scale_u;
                let max_chars = ((PANEL_W - 60.0) / char_w) as usize;
                let mut display = url.to_string();
                if display.chars().count() > max_chars && max_chars > 1 {
                    display = display.chars().take(max_chars - 1).collect();
                    display.push('…');
                }
                let uw = self.measure_text(&display, scale_u);
                self.draw_text(
                    panel_x + (PANEL_W - uw) * 0.5,
                    panel_y + 90.0,
                    scale_u,
                    &display,
                    swf::Color::from_rgb(0xFFFFFF, 255),
                );

                // Action hints stacked inside the panel.
                let scale_h = 1.8;
                let lines = [
                    "ZR : CHARGER CETTE URL DIRECTEMENT",
                    "A  : SAISIR / EDITER URL (CLAVIER)",
                    "L / R : URL PRECEDENTE / SUIVANTE",
                ];
                for (i, line) in lines.iter().enumerate() {
                    let w = self.measure_text(line, scale_h);
                    self.draw_text(
                        panel_x + (PANEL_W - w) * 0.5,
                        panel_y + 150.0 + i as f32 * 32.0,
                        scale_h,
                        line,
                        swf::Color::from_rgb(0x99AABB, 255),
                    );
                }
            }
        }

        // Footer.
        const HELP_SCALE: f32 = 2.0;
        let help = if recent_url.is_some() {
            "ZR:OUVRIR   A:EDITER   L/R:NAV   Y:RETOUR LOCAL   -:QUITTER"
        } else {
            "A:SAISIR URL   Y:RETOUR LOCAL   -:QUITTER"
        };
        let help_w = self.measure_text(help, HELP_SCALE);
        self.draw_text(
            (vw - help_w) * 0.5,
            vh - 42.0,
            HELP_SCALE,
            help,
            swf::Color::from_rgb(0x99AABB, 255),
        );
        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        self.gl_state.invalidate();
    }

    /// List of remote files (one row per `RemoteFile`). Mirrors the local
    /// `draw_library_list` layout but skips the per-file color chip /
    /// metadata panel — remote files only have name + size to show.
    /// `downloaded` is the set of basenames already pulled this session
    /// (drawn with a green `OK` prefix so the user knows what's done).
    pub fn draw_library_distant_files(
        &mut self,
        selection: usize,
        scroll_offset: usize,
        files: &[crate::net::RemoteFile],
        visible_rows: usize,
        downloaded: &[std::string::String],
        filter: Option<&str>,
        total_unfiltered: usize,
    ) {
        self.library_clear();
        let vw = self.dimensions.width as f32;
        let vh = self.dimensions.height as f32;

        // Header.
        let title = "FICHIERS DISTANTS";
        let scale_t = 4.0;
        let tw = self.measure_text(title, scale_t);
        self.draw_text(
            (vw - tw) * 0.5 + 3.0,
            30.0 + 3.0,
            scale_t,
            title,
            swf::Color::from_rgb(0x000000, 255),
        );
        self.draw_text(
            (vw - tw) * 0.5,
            30.0,
            scale_t,
            title,
            swf::Color::from_rgb(0xFFD740, 255),
        );

        // Sub-line shows filter status: "23/3633 — FILTRE: mario" when
        // filter is active, "3633 FICHIER(S) .SWF TROUVE(S)" otherwise.
        let sub = match filter {
            Some(f) if !f.is_empty() => {
                std::format!("{} / {} — FILTRE: {}", files.len(), total_unfiltered, f)
            }
            _ => {
                // Vrai pluriel conditionnel. L'ancien "FICHIER(S) .SWF TROUVE(S)"
                // s'affichait "FICHIER S ... TROUVE S" car la police pixel-art ne
                // rend pas les parenthèses (rendues comme des espaces). 0 et 1 =
                // singulier en français, >1 = pluriel.
                let n = files.len();
                let p = if n > 1 { "S" } else { "" };
                std::format!("{} FICHIER{} .SWF TROUVE{}", n, p, p)
            }
        };
        let scale_s = 2.0;
        let sw = self.measure_text(&sub, scale_s);
        self.draw_text(
            (vw - sw) * 0.5,
            85.0,
            scale_s,
            &sub,
            swf::Color::from_rgb(0xAABFD8, 255),
        );

        // Rows.
        const ROW_SCALE: f32 = 2.5;
        const ROW_SPACING: f32 = 50.0;
        let rows_top_y = 150.0;
        let rows_left_x = 80.0;
        let total = files.len();
        let end = (scroll_offset + visible_rows).min(total);
        for (visible_idx, abs_idx) in (scroll_offset..end).enumerate() {
            let f = &files[abs_idx];
            let y = rows_top_y + visible_idx as f32 * ROW_SPACING;
            let is_sel = abs_idx == selection;
            let color = if is_sel {
                swf::Color::from_rgb(0xFFD740, 255)
            } else {
                swf::Color::from_rgb(0xCCCCCC, 255)
            };
            if is_sel {
                self.draw_text(rows_left_x - 30.0, y, ROW_SCALE, ">", color);
            }
            // OK badge for files already downloaded this session.
            let is_downloaded = downloaded.iter().any(|n| n == &f.name);
            let badge_w = if is_downloaded {
                let badge = "OK";
                let bw = self.measure_text(badge, 2.0);
                // Bright green tint so it pops over the amber/grey rows.
                self.draw_text(rows_left_x, y + 4.0, 2.0, badge, swf::Color::from_rgb(0x66DD66, 255));
                bw + 12.0
            } else {
                0.0
            };
            let name_x = rows_left_x + badge_w;
            // Truncate filename to fit. Each row = filename + size on
            // the right edge.
            let size_str = format_size_pretty(f.size_bytes);
            let size_w = self.measure_text(&size_str, ROW_SCALE);
            let size_x = vw - 80.0 - size_w;
            let max_name_w = size_x - name_x - 20.0;
            let mut display = f.name.clone();
            // ~6 px per char at ROW_SCALE * 6 (5+1 spacing).
            let char_w = 6.0 * ROW_SCALE;
            let max_chars = (max_name_w / char_w) as usize;
            if display.chars().count() > max_chars && max_chars > 1 {
                display = display.chars().take(max_chars - 1).collect();
                display.push('…');
            }
            self.draw_text(name_x, y, ROW_SCALE, &display, color);
            self.draw_text(size_x, y, ROW_SCALE, &size_str, color);
        }

        // Scrollbar if needed.
        if total > visible_rows {
            let bar_x = vw - 30.0;
            let bar_top_y = rows_top_y;
            let bar_h_total = visible_rows as f32 * ROW_SPACING;
            let bar_h_thumb = (bar_h_total * visible_rows as f32 / total as f32).max(20.0);
            let progress = scroll_offset as f32 / (total - visible_rows) as f32;
            let thumb_y = bar_top_y + (bar_h_total - bar_h_thumb) * progress;
            let track = Matrix {
                a: 4.0, b: 0.0, c: 0.0, d: bar_h_total,
                tx: swf::Twips::from_pixels(bar_x as f64),
                ty: swf::Twips::from_pixels(bar_top_y as f64),
            };
            <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgba(0x40_99AABB), track);
            let thumb = Matrix {
                a: 4.0, b: 0.0, c: 0.0, d: bar_h_thumb,
                tx: swf::Twips::from_pixels(bar_x as f64),
                ty: swf::Twips::from_pixels(thumb_y as f64),
            };
            <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgb(0xFFD740, 255), thumb);
        }

        // Footer.
        const HELP_SCALE: f32 = 2.0;
        let help = "A:TELECHARGER   B:RETOUR   X:RECHERCHE   L/R:PAGE   HAUT/BAS:NAV";
        let help_w = self.measure_text(help, HELP_SCALE);
        self.draw_text(
            (vw - help_w) * 0.5,
            vh - 42.0,
            HELP_SCALE,
            help,
            swf::Color::from_rgb(0x99AABB, 255),
        );
        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        self.gl_state.invalidate();
    }

    /// Download in flight — big title, filename, progress bar, footer.
    /// `bytes_total = 0` means Content-Length wasn't known at the start;
    /// show an indeterminate bar in that case (just a slim animated
    /// marker; for v1 we just show "??.?? / ??" until total arrives).
    pub fn draw_library_distant_downloading(
        &mut self,
        file_name: &str,
        bytes_done: u64,
        bytes_total: u64,
    ) {
        self.library_clear();
        let vw = self.dimensions.width as f32;
        let vh = self.dimensions.height as f32;

        let title = "TELECHARGEMENT";
        let scale_t = 5.0;
        let tw = self.measure_text(title, scale_t);
        self.draw_text(
            (vw - tw) * 0.5 + 4.0,
            vh * 0.18 + 4.0,
            scale_t,
            title,
            swf::Color::from_rgb(0x000000, 255),
        );
        self.draw_text(
            (vw - tw) * 0.5,
            vh * 0.18,
            scale_t,
            title,
            swf::Color::from_rgb(0xFFD740, 255),
        );

        // Filename (truncated if needed).
        let scale_n = 2.0;
        let mut display = file_name.to_string();
        let max_chars = 56usize;
        if display.chars().count() > max_chars && max_chars > 1 {
            display = display.chars().take(max_chars - 1).collect();
            display.push('…');
        }
        let nw = self.measure_text(&display, scale_n);
        self.draw_text(
            (vw - nw) * 0.5,
            vh * 0.34,
            scale_n,
            &display,
            swf::Color::from_rgb(0xCCCCCC, 255),
        );

        // Progress bar (centred 800x40, fill amber, track navy).
        const BAR_W: f32 = 800.0;
        const BAR_H: f32 = 40.0;
        let bar_x = (vw - BAR_W) * 0.5;
        let bar_y = vh * 0.50;
        let track = Matrix {
            a: BAR_W, b: 0.0, c: 0.0, d: BAR_H,
            tx: swf::Twips::from_pixels(bar_x as f64),
            ty: swf::Twips::from_pixels(bar_y as f64),
        };
        <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgb(0x142038, 255), track);
        <Self as CommandHandler>::draw_line_rect(self, swf::Color::from_rgb(0xFFFFFF, 255), track);

        let frac = if bytes_total > 0 {
            (bytes_done as f32 / bytes_total as f32).clamp(0.0, 1.0)
        } else {
            0.0
        };
        if frac > 0.0 {
            let fill = Matrix {
                a: BAR_W * frac, b: 0.0, c: 0.0, d: BAR_H,
                tx: swf::Twips::from_pixels(bar_x as f64),
                ty: swf::Twips::from_pixels(bar_y as f64),
            };
            <Self as CommandHandler>::draw_rect(self, swf::Color::from_rgb(0xFFD740, 255), fill);
        }

        // % + bytes label below the bar.
        let scale_p = 2.5;
        let label = if bytes_total > 0 {
            std::format!(
                "{}%   {} / {}",
                (frac * 100.0) as u32,
                format_size_pretty(bytes_done),
                format_size_pretty(bytes_total),
            )
        } else {
            std::format!("{} ...", format_size_pretty(bytes_done))
        };
        let pw = self.measure_text(&label, scale_p);
        self.draw_text(
            (vw - pw) * 0.5,
            bar_y + BAR_H + 20.0,
            scale_p,
            &label,
            swf::Color::from_rgb(0xFFFFFF, 255),
        );

        // Footer.
        const HELP_SCALE: f32 = 2.0;
        let help = "B:ANNULER";
        let help_w = self.measure_text(help, HELP_SCALE);
        self.draw_text(
            (vw - help_w) * 0.5,
            vh - 42.0,
            HELP_SCALE,
            help,
            swf::Color::from_rgb(0x99AABB, 255),
        );
        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        self.gl_state.invalidate();
    }

    /// Error toast for DISTANT mode (URL parse / metadata fetch / DL fail).
    pub fn draw_library_distant_error(&mut self, msg: &str) {
        self.library_clear();
        let vw = self.dimensions.width as f32;
        let vh = self.dimensions.height as f32;

        let title = "ERREUR";
        let scale_t = 5.0;
        let tw = self.measure_text(title, scale_t);
        self.draw_text(
            (vw - tw) * 0.5 + 4.0,
            vh * 0.22 + 4.0,
            scale_t,
            title,
            swf::Color::from_rgb(0x000000, 255),
        );
        self.draw_text(
            (vw - tw) * 0.5,
            vh * 0.22,
            scale_t,
            title,
            swf::Color::from_rgb(0xFF5040, 255),
        );

        // Word-wrap the message into ~70-char lines (rough heuristic at
        // scale 2.0). We use the simple split-on-space algorithm. Lines
        // are centred horizontally.
        let scale_m = 2.0;
        const WRAP_AT: usize = 60;
        let mut lines: std::vec::Vec<std::string::String> = std::vec::Vec::new();
        let mut current = std::string::String::new();
        for word in msg.split(' ') {
            if current.is_empty() {
                current.push_str(word);
            } else if current.len() + 1 + word.len() <= WRAP_AT {
                current.push(' ');
                current.push_str(word);
            } else {
                lines.push(current.clone());
                current.clear();
                current.push_str(word);
            }
        }
        if !current.is_empty() {
            lines.push(current);
        }
        let mut y = vh * 0.42;
        for line in &lines {
            let w = self.measure_text(line, scale_m);
            self.draw_text(
                (vw - w) * 0.5,
                y,
                scale_m,
                line,
                swf::Color::from_rgb(0xCCCCCC, 255),
            );
            y += 30.0;
        }

        const HELP_SCALE: f32 = 2.0;
        let help = "A/B:OK";
        let help_w = self.measure_text(help, HELP_SCALE);
        self.draw_text(
            (vw - help_w) * 0.5,
            vh - 42.0,
            HELP_SCALE,
            help,
            swf::Color::from_rgb(0x99AABB, 255),
        );
        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
        }
        self.gl_state.invalidate();
    }

    /// Dim backdrop used when the menu module's TOUCHES editor is on top of
    /// the library (pre-launch keymap edit). Quick black fill — no library
    /// content underneath, no Ruffle render — just a flat backdrop so
    /// `menu::draw` sits on something solid.
    pub fn draw_library_dim_backdrop(&mut self) {
        unsafe {
            glDisable(GL_STENCIL_TEST);
            glClearColor(0.04, 0.06, 0.10, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        self.gl_state.invalidate();
    }

    // ── Mask state machine ──
    //
    // Flash mask sequence for one mask:
    //   1. push_mask     → begin drawing the mask shape
    //   2. (draw mask shape commands)
    //   3. activate_mask → mask done, begin drawing the maskee
    //   4. (draw maskee shape commands)
    //   5. deactivate_mask → maskee done
    //   6. pop_mask      → undo the stencil ref
    //
    // Scheme: INCR/DECR coverage counting. The frame starts with stencil
    // cleared to 0 (submit_frame). A maskee at nesting depth N is drawn where
    // the stencil count equals N — i.e. it was covered by all N enclosing mask
    // shapes (their intersection). Sequential masks each INCR from 0 then DECR
    // back, so no per-push full-buffer clear is needed. This replaced an
    // earlier bit-OR + REPLACE scheme whose written value didn't match the
    // EQUAL gate, leaving every maskee rejected (SMWF overworld was blank).
    fn mask_push(&mut self) {
        self.push_mask_window = self.push_mask_window.saturating_add(1);
        self.mask.writing = true;
        self.mask.depth = self.mask.depth.saturating_add(1);
        unsafe {
            glEnable(GL_STENCIL_TEST);
            // Mask shape writes stencil only (no color): increment coverage.
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        }
    }

    fn mask_activate(&mut self) {
        // Mask shape done. Draw the maskee where coverage == nesting depth.
        self.mask.writing = false;
        unsafe {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glStencilMask(0);
            let func = if DISABLE_MASK_GATING { GL_ALWAYS } else { GL_EQUAL };
            glStencilFunc(func, self.mask.depth as GLint, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        }
    }

    fn mask_deactivate(&mut self) {
        // Maskee done. Redraw the mask shape decrementing coverage back, so
        // sibling/outer masks see a clean stencil without a full clear.
        self.mask.writing = true;
        unsafe {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
        }
    }

    fn mask_pop(&mut self) {
        self.mask.writing = false;
        if self.mask.depth > 0 {
            self.mask.depth -= 1;
        }
        unsafe {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            if self.mask.depth == 0 {
                glDisable(GL_STENCIL_TEST);
            } else {
                // Resume gating the enclosing maskee at the outer depth.
                glStencilMask(0);
                let func = if DISABLE_MASK_GATING { GL_ALWAYS } else { GL_EQUAL };
                glStencilFunc(func, self.mask.depth as GLint, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            }
        }
    }
}

/// Format a byte count as a short pretty string ("3 KB", "15 MB"). Picks
/// the largest unit that keeps the integer part ≤ 999. KiB-style (1024)
/// instead of decimal because that's what hbmenu / fsadm show for files.
fn format_size_pretty(bytes: u64) -> std::string::String {
    const KB: u64 = 1024;
    const MB: u64 = 1024 * 1024;
    const GB: u64 = 1024 * 1024 * 1024;
    if bytes >= GB {
        std::format!("{}.{} GB", bytes / GB, (bytes % GB) / (GB / 10))
    } else if bytes >= MB {
        std::format!("{} MB", bytes / MB)
    } else if bytes >= KB {
        std::format!("{} KB", bytes / KB)
    } else {
        std::format!("{} B", bytes)
    }
}

/// Cheap sin approximation for UI animations. Bhaskara-I-style polynomial
/// — accurate to ~3 decimal places, no libm dependency, branch-free except
/// for the period fold. Plenty for visual pulses (we only use it to
/// modulate amber → bright-amber and a 4-pixel cursor offset).
fn approx_sin(x: f32) -> f32 {
    // Fold to [-π, π].
    let two_pi = 2.0 * core::f32::consts::PI;
    let mut t = x % two_pi;
    if t > core::f32::consts::PI { t -= two_pi; }
    if t < -core::f32::consts::PI { t += two_pi; }
    // Bhaskara I: sin(x) ≈ 16x(π − x) / (5π² − 4x(π − x)) for x ∈ [0, π].
    // Use sign symmetry for negative x.
    let sign = if t < 0.0 { -1.0 } else { 1.0 };
    let t = t.abs();
    let pi = core::f32::consts::PI;
    let num = 16.0 * t * (pi - t);
    let den = 5.0 * pi * pi - 4.0 * t * (pi - t);
    sign * (num / den)
}

impl RenderBackend for SwitchRenderBackend {
    fn viewport_dimensions(&self) -> ViewportDimensions {
        self.dimensions
    }

    fn set_viewport_dimensions(&mut self, dimensions: ViewportDimensions) {
        self.dimensions = dimensions;
        unsafe {
            glViewport(0, 0, dimensions.width as GLsizei, dimensions.height as GLsizei);
        }
    }

    fn register_shape(
        &mut self,
        shape: DistilledShape,
        bitmap_source: &dyn BitmapSource,
    ) -> ShapeHandle {
        let mesh = self.tessellator.tessellate_shape(shape, bitmap_source);

        // Bake gradient textures first so per-draw can reference them.
        let mut gradient_textures: Vec<GLuint> = Vec::with_capacity(mesh.gradients.len());
        for g in &mesh.gradients {
            gradient_textures.push(build_gradient_texture(g));
        }

        // Baseline: budget=0 → bitmap fills render as solid white (the
        // "blocs blancs" state of commit 6a2b858, README "phase 1.5"). With
        // ANY budget > 0 Mario 63 deterministically crashes at host frame
        // ~40 during render_shape's DrawKind::Bitmap path inside
        // submit_frame — bitmap registration is fine (650+ regs at flat
        // RAM), the bug is in the GL draw side. Restore =0 while we
        // instrument that exact path.
        // Crash fixed (2026-05-24): jpeg_decoder's std::thread::spawn for
        // JPEGs > 128*128 used to crash Switch newlib pthread. Forked the
        // crate to always use Immediate worker → no spawn → no crash.
        // We can now resolve every bitmap fill (full sprites for Mario 63).
        const PER_SHAPE_BITMAP_BUDGET: usize = usize::MAX;
        let mut bitmap_metas: Vec<Option<SwitchBitmapHandle>> =
            Vec::with_capacity(mesh.draws.len());
        let bitmap_fill_count = mesh
            .draws
            .iter()
            .filter(|d| matches!(d.draw_type, DrawType::Bitmap(_)))
            .count();
        let resolve_bitmaps = bitmap_fill_count <= PER_SHAPE_BITMAP_BUDGET;
        for draw in &mesh.draws {
            let meta = if resolve_bitmaps {
                if let DrawType::Bitmap(b) = &draw.draw_type {
                    bitmap_source
                        .bitmap_handle(b.bitmap_id, self)
                        .and_then(|h| as_switch_bitmap(&h).cloned())
                } else {
                    None
                }
            } else {
                None
            };
            bitmap_metas.push(meta);
        }

        let mut draws: Vec<GpuDraw> = Vec::with_capacity(mesh.draws.len());
        for (idx, draw) in mesh.draws.iter().enumerate() {
            let meta_ref = bitmap_metas[idx].as_ref();
            if let Some(mut gpu) = upload_draw(
                draw,
                &gradient_textures,
                meta_ref,
                &mut self.vertex_arena,
                &mut self.index_arena,
            ) {
                // Refine gradient parameters now that we have the Gradient.
                if let DrawKind::Gradient {
                    texture_index,
                    gradient_kind,
                    spread,
                    focal,
                    ..
                } = &mut gpu.kind
                {
                    let g = &mesh.gradients[*texture_index];
                    *gradient_kind = match g.gradient_type {
                        GradientType::Linear => 0,
                        GradientType::Radial => 1,
                        GradientType::Focal => 2,
                    };
                    *spread = match g.repeat_mode {
                        GradientSpread::Pad => 0,
                        GradientSpread::Reflect => 1,
                        GradientSpread::Repeat => 2,
                    };
                    *focal = f32::from(g.focal_point);
                }
                LIVE_GPU_DRAWS.fetch_add(1, Ordering::Relaxed);
                draws.push(gpu);
            }
        }

        self.shapes_registered = self.shapes_registered.wrapping_add(1);
        LIVE_GPU_SHAPES.fetch_add(1, Ordering::Relaxed);

        // Periodic visibility into shape pressure. With Mario 63's rocket
        // nozzle particle system pumping ~3 shapes/frame, this lets us see
        // whether Ruffle is dropping old handles (live stays bounded) or
        // not (live grows linearly with `shapes_registered`).
        if self.shapes_registered % 500 == 0 {
            let live_s = LIVE_GPU_SHAPES.load(Ordering::Relaxed);
            let live_d = LIVE_GPU_DRAWS.load(Ordering::Relaxed);
            let msg = std::format!(
                "register_shape: total={} live_shapes={} live_draws={}\n",
                self.shapes_registered, live_s, live_d,
            );
            let mut bytes = msg.into_bytes();
            bytes.push(0);
            unsafe { ruffle_log_cstr(bytes.as_ptr() as *const _) };
        }

        ShapeHandle(Arc::new(SwitchShapeHandle(Arc::new(GpuShape {
            draws,
            gradient_textures,
        }))))
    }

    fn render_offscreen(
        &mut self,
        handle: BitmapHandle,
        commands: CommandList,
        _quality: StageQuality,
        bounds: PixelRegion,
    ) -> Option<Box<dyn SyncHandle>> {
        let _pt = PrimTimer::new(&PRIM_OFFSCREEN_CUR);
        self.render_offscreen_calls = self.render_offscreen_calls.wrapping_add(1);
        // Where the BitmapData's pixels live + its dimensions. BitmapData backs
        // its handle via `register_bitmap` (atlas) in the common
        // `new BitmapData()` case; large ones fall back to a standalone texture.
        #[derive(Clone, Copy)]
        enum Backing {
            Standalone(GLuint),
            Atlas { tex: GLuint, base_x: u32, base_y: u32 },
        }
        let (tex_w, tex_h, backing) = if let Some(s) = as_standalone_bitmap(&handle) {
            (s.0.width, s.0.height, Backing::Standalone(s.0.texture))
        } else if let Some(b) = as_switch_bitmap(&handle) {
            let base_x = (b.u0 * ATLAS_SIZE as f32).round() as u32;
            let base_y = (b.v0 * ATLAS_SIZE as f32).round() as u32;
            let Some(a) = self.atlases.get(b.atlas_index) else { return None };
            (b.width, b.height, Backing::Atlas { tex: a.texture, base_x, base_y })
        } else {
            self.warn_once(b"render_offscreen: unknown handle\n\0");
            return None;
        };
        if tex_w == 0 || tex_h == 0 {
            return None;
        }
        // render_offscreen must COMPOSITE the draw() commands onto the
        // BitmapData's existing content (Ruffle's wgpu backend uses
        // `RenderTargetMode::FreshWithTexture`), not replace it. We render into a
        // pooled temp (atlas slots can't be FBO targets) in three steps:
        //   1. SEED temp with the BitmapData's current pixels (premultiplied).
        //   2. COMPOSITE the new draw() commands on top (no colour clear).
        //   3. WRITE the result back into the BitmapData's storage.
        // Without the seed, a software-blitter game that accumulates many
        // draw()s into one BitmapData per frame (catmario's `stageBitmapdata`:
        // ~48 tile draws/frame) keeps only the last draw → invisible world.
        // `temp` is also returned as the SyncHandle, so copyPixels/getPixel
        // (SMWF's tile-engine readback) still resolve from the full result.
        PRIM_OFF_N_CUR.fetch_add(1, std::sync::atomic::Ordering::Relaxed);
        PRIM_OFF_PIX_CUR.fetch_add(
            (tex_w as u64) * (tex_h as u64),
            std::sync::atomic::Ordering::Relaxed,
        );
        let temp = {
            let _t = PrimTimer::new(&PRIM_OFF_ALLOC_CUR);
            self.acquire_offscreen_temp(tex_w, tex_h)?
        };
        let temp_id = temp.texture;

        // 1. Seed temp with the BitmapData's current content (premultiplied).
        {
            let _t = PrimTimer::new(&PRIM_OFF_READBACK_CUR);
            match backing {
                Backing::Standalone(s_tex) => {
                    // Standalone already stores premultiplied — straight copy.
                    self.blit_identity(
                        s_tex, tex_w, tex_h, (0, 0), (tex_w, tex_h),
                        temp_id, (0, 0), tex_w, tex_h,
                    );
                }
                Backing::Atlas { tex, base_x, base_y } => {
                    // Atlas stores STRAIGHT alpha — premultiply it into temp.
                    self.blit_premult(
                        tex, ATLAS_SIZE, ATLAS_SIZE, (base_x, base_y), (tex_w, tex_h),
                        temp_id, (0, 0), tex_w, tex_h,
                    );
                }
            }
        }

        // 2. Composite the new draw() commands on top (no colour clear).
        let rendered = {
            let _t = PrimTimer::new(&PRIM_OFF_RENDER_CUR);
            self.render_commands_to_texture(temp_id, tex_w, tex_h, commands, None)
        };
        if !rendered {
            self.offscreen_temp_retired.push(temp);
            return None;
        }

        // 3. Write the composited result back into the BitmapData's storage.
        {
            let _t = PrimTimer::new(&PRIM_OFF_UPLOAD_CUR);
            match backing {
                Backing::Standalone(s_tex) => {
                    self.blit_identity(
                        temp_id, tex_w, tex_h, (0, 0), (tex_w, tex_h),
                        s_tex, (0, 0), tex_w, tex_h,
                    );
                }
                Backing::Atlas { tex, base_x, base_y } => {
                    self.blit_unpremult(
                        temp_id, tex_w, tex_h, (0, 0), (tex_w, tex_h),
                        tex, (base_x as i32, base_y as i32), tex_w, tex_h,
                    );
                }
            }
        }
        self.warn_once(b"render_offscreen: composite draw() -> handle\n\0");
        // Retire temp for reuse next frame (submit_frame recycles it into the
        // pool) instead of freeing it; the SyncHandle references it by raw id.
        self.offscreen_temp_retired.push(temp);
        // Read back exactly `bounds`: the resolve closure indexes its buffer
        // relative to this region's origin with stride = bounds.width().
        Some(Box::new(BitmapDataSyncHandle {
            texture: temp_id,
            x: bounds.x_min,
            y: bounds.y_min,
            w: bounds.width(),
            h: bounds.height(),
        }))
    }

    fn apply_filter(
        &mut self,
        source: BitmapHandle,
        source_point: (u32, u32),
        source_size: (u32, u32),
        destination: BitmapHandle,
        dest_point: (i32, i32),
        filter: Filter,
    ) -> Option<Box<dyn SyncHandle>> {
        self.apply_filter_calls = self.apply_filter_calls.wrapping_add(1);
        let Some(src) = as_standalone_bitmap(&source) else { return None };
        let Some(dst) = as_standalone_bitmap(&destination) else { return None };
        let (src_tex, src_w, src_h) = (src.0.texture, src.0.width, src.0.height);
        let dst_tex = dst.0.texture;
        let ok = self.apply_filter_raw(
            src_tex, src_w, src_h, source_point, source_size,
            dst_tex, dest_point, &filter,
        );
        if ok { Some(Box::new(NoOpSyncHandle)) } else { None }
    }

    fn is_filter_supported(&self, filter: &Filter) -> bool {
        let (ord, name) = filter_variant_ordinal(filter);
        let bit = 1u16 << ord;
        let prev = self.filters_seen_mask.fetch_or(bit, Ordering::Relaxed);
        if prev & bit == 0 {
            let msg = std::format!("is_filter_supported: {} (first sighting)\n", name);
            let mut bytes = msg.into_bytes();
            bytes.push(0);
            unsafe { ruffle_log_cstr(bytes.as_ptr() as *const _) };
        }
        // Re-enabled 2026-05-29 after fixing the crash root cause: the filter
        // shader chain itself was fine, but the FilterTexturePool grew
        // unbounded (one texture per distinct size, never freed) → texture
        // exhaustion → glGenTextures returned 0 → Mesa NULL-deref. Fixed by
        // bounding the pool (MAX_POOLED_FILTER_TEXTURES) and returning None
        // from make_standalone_texture on failure (filters then skip cleanly
        // instead of using a 0 texture). Restores glow/drop-shadow (e.g. the
        // outlined-letter borders Mario 63 draws on its menu text).
        matches!(
            filter,
            Filter::ColorMatrixFilter(_)
                | Filter::BlurFilter(_)
                | Filter::GlowFilter(_)
                | Filter::DropShadowFilter(_)
                | Filter::BevelFilter(_)
        )
    }

    fn is_offscreen_supported(&self) -> bool {
        // Enabled with the minimal cache path (no filter shaders yet). Ruffle
        // will cacheAsBitmap filtered/cached display objects, render their
        // commands into our standalone textures, and draw them back. Filters
        // in cache_entries are ignored for now (is_filter_supported = false),
        // so we render the unfiltered source content — visible content shows,
        // alpha~0+filter-only content (platforms) stays invisible until the
        // filter pipeline lands.
        true
    }

    fn submit_frame(
        &mut self,
        clear: Color,
        commands: CommandList,
        cache_entries: Vec<BitmapCacheEntry>,
    ) {
        // Drain any pending arena frees enqueued by `GpuDraw::drop`. Doing
        // this at frame boundaries (not from Drop itself) keeps us off the
        // hook for &mut access during arbitrary Ruffle drops, and keeps
        // arena bookkeeping localised to the GL thread.
        {
            let mut pending = PENDING_FREES.lock().unwrap();
            for f in pending.drain(..) {
                self.vertex_arena.free_region(f.vbo_offset, f.vbo_size);
                self.index_arena.free_region(f.ibo_offset, f.ibo_size);
            }
        }

        // Snapshot+reset the per-frame backend-primitive timers. We're at the
        // start of submit_frame — right after player.tick() ran the AVM frame and
        // any render_offscreen/upload/resolve it triggered — so CUR holds exactly
        // this frame's tick-side primitive time. Move it to LAST (read by
        // log_slow_frame, which runs just after) and zero CUR for the next frame.
        PRIM_OFFSCREEN_LAST.store(
            PRIM_OFFSCREEN_CUR.swap(0, std::sync::atomic::Ordering::Relaxed),
            std::sync::atomic::Ordering::Relaxed,
        );
        PRIM_BMPUP_LAST.store(
            PRIM_BMPUP_CUR.swap(0, std::sync::atomic::Ordering::Relaxed),
            std::sync::atomic::Ordering::Relaxed,
        );
        PRIM_RESOLVE_LAST.store(
            PRIM_RESOLVE_CUR.swap(0, std::sync::atomic::Ordering::Relaxed),
            std::sync::atomic::Ordering::Relaxed,
        );
        // DIAG: render_offscreen sub-phase timers (see statics near top).
        for (cur, last) in [
            (&PRIM_OFF_ALLOC_CUR, &PRIM_OFF_ALLOC_LAST),
            (&PRIM_OFF_RENDER_CUR, &PRIM_OFF_RENDER_LAST),
            (&PRIM_OFF_READBACK_CUR, &PRIM_OFF_READBACK_LAST),
            (&PRIM_OFF_UPLOAD_CUR, &PRIM_OFF_UPLOAD_LAST),
            (&PRIM_OFF_N_CUR, &PRIM_OFF_N_LAST),
            (&PRIM_OFF_PIX_CUR, &PRIM_OFF_PIX_LAST),
        ] {
            last.store(
                cur.swap(0, std::sync::atomic::Ordering::Relaxed),
                std::sync::atomic::Ordering::Relaxed,
            );
        }

        // Recycle this frame's render_offscreen temps into the reuse pool. Their
        // SyncHandles were resolved/dropped during this frame's tick (Ruffle
        // reads a BitmapData.draw() result back on the next CPU access, which
        // happens in the same AS frame), so the textures are safe to reuse next
        // frame. Cap the pool so churning BitmapData sizes can't grow it without
        // bound (excess textures drop here → glDeleteTextures).
        const OFFSCREEN_TEMP_POOL_CAP: usize = 128;
        self.offscreen_temp_pool
            .append(&mut self.offscreen_temp_retired);
        if self.offscreen_temp_pool.len() > OFFSCREEN_TEMP_POOL_CAP {
            self.offscreen_temp_pool.truncate(OFFSCREEN_TEMP_POOL_CAP);
        }
        // Snapshot counters for the per-frame slow-frame breakdown (consumed
        // right after `commands.execute` below). `cache_entries` is moved by the
        // filter loop, so grab its length up front.
        self.frame_snapshot = self.frame_counters();
        let frame_cache_entries = cache_entries.len() as u32;

        // Render cacheAsBitmap entries: each has a standalone destination
        // texture, a command list, a clear color, and (ignored for now) a
        // filter list. Minimal path — render the source commands directly
        // into the cache texture. Ruffle later draws it back via
        // `render_bitmap`. Filters are NOT applied yet (see is_offscreen).
        // Faithful port of wgpu's submit_frame cache_entries flow
        // (`render/wgpu/src/backend.rs:512`):
        //   1. Render commands directly into entry.handle.texture — this is
        //      the first filter source.
        //   2. Chain filters: each apply() reads `current` and writes into a
        //      fresh pool texture. On unsupported filter (returns None) we
        //      passthrough — keep current_handle. wgpu uses an identity-blit
        //      fallback that allocates a fresh texture; our passthrough is
        //      functionally equivalent and saves one copy.
        //   3. If filters moved current off entry.handle, identity-blit the
        //      final filter texture back into entry.handle (so the cache
        //      texture sees the filtered result).
        self.cache_entries_max_window = self.cache_entries_max_window.max(cache_entries.len() as u32);
        // Age out filter-pool textures not reused recently (TTL eviction).
        self.filter_tex_pool.begin_frame(self.frame_count as u64);
        // Per-frame filter budget. Each filtered cache entry costs ~3-5
        // offscreen passes; a menu *transition* can re-filter dozens of
        // animated elements in one frame, spiking render time. Cap how many
        // filter CHAINS we run per frame — entries past the budget keep the
        // content from step 1 (text/shape) but skip their bevel/glow border for
        // that frame.
        //
        // IMPORTANT: step 1 (render the content into entry.handle) must run for
        // EVERY entry, every frame. `entry.handle` is NOT a persistent cache we
        // can leave stale — Ruffle re-uses/clears it, so skipping step 1 blanks
        // the whole element (the "tous les boutons clignotent / plus de texte"
        // regression). Only the *filter pass* is budgeted, never the content.
        //
        // Budget set high (was 6) so the bevel/glow borders stay present on
        // Mario 63's menus, where many text fields re-cache each frame
        // (cacheMax peaks ~40). Raising it trades a little render time on busy
        // transitions for the reflections no longer dropping in and out. Tune
        // down if a heavy menu hitches.
        const FILTER_CHAINS_PER_FRAME_BUDGET: usize = 48;
        let mut filter_chains_run: usize = 0;
        for entry in cache_entries {
            let Some(standalone) = as_standalone_bitmap(&entry.handle) else {
                self.warn_once(b"cache_entry: non-standalone handle (skipped)\n\0");
                continue;
            };
            let dst_tex = standalone.0.texture;
            let w = standalone.0.width;
            let h = standalone.0.height;

            // Step 1: render the content into entry.handle (ALWAYS — see above).
            self.render_commands_to_texture(dst_tex, w, h, entry.commands, Some(entry.clear));
            if entry.filters.is_empty() {
                continue;
            }
            // Over the per-frame filter budget → leave this entry unfiltered for
            // this frame: text/shape is still present (step 1), just without the
            // bevel/glow border this frame.
            if filter_chains_run >= FILTER_CHAINS_PER_FRAME_BUDGET {
                continue;
            }
            filter_chains_run += 1;

            // Step 2: filter chain using the (now bounded) FilterTexturePool.
            // The first source is entry.handle.texture itself; each successful
            // filter writes into a fresh pool temp and the previous owned temp
            // is released. acquire() can return None (pool/​GL exhaustion guard)
            // — we break and keep whatever we have, so we never feed a 0 texture
            // to the shaders (the old crash).
            let mut current_tex = dst_tex;
            let mut current_owned: Option<StandaloneTexture> = None;
            for filter in entry.filters {
                let Some(next) = self.filter_tex_pool.acquire(w, h) else { break };
                let next_tex = next.texture;
                let ok = self.apply_filter_raw(
                    current_tex, w, h, (0, 0), (w, h),
                    next_tex, (0, 0), &filter,
                );
                if ok {
                    if let Some(prev) = current_owned.take() {
                        self.filter_tex_pool.release(prev);
                    }
                    current_tex = next_tex;
                    current_owned = Some(next);
                } else {
                    // Unsupported/failed filter — passthrough, return to pool.
                    self.filter_tex_pool.release(next);
                }
            }

            // Step 3: if the chain moved current off entry.handle, blit the
            // final temp back into entry.handle and return the temp to pool.
            if let Some(final_owned) = current_owned {
                let ft = final_owned.texture;
                self.blit_identity(ft, w, h, (0, 0), (w, h), dst_tex, (0, 0), w, h);
                self.filter_tex_pool.release(final_owned);
            }
        }

        // Drain GL errors once per second, plus a one-line heartbeat with
        // running counters every 2 seconds. Quiet otherwise.
        self.frame_count = self.frame_count.wrapping_add(1);
        // Diagnostic heartbeat: full counters every 60 frames (~1 s), plus a
        // 1-byte-cheap per-frame tick so the LAST frame before a crash is
        // visible in the log. The previous 120-frame cadence left a ~2 s
        // window of total silence around the jetpack crash.
        //
        // Note about RAM: the previous "WARN low ram" alert was misleading.
        // `svcGetInfo(UsedMemorySize)` returns the heap RESERVED by the
        // applet (set once at crt0), not the heap actually consumed by
        // malloc. It barely moves, so a 99% ratio at boot is normal and the
        // warning fired every 30 frames for nothing. Removed.
        if self.frame_count % 60 == 0 {
            // Wall-clock FPS over the last 60-frame window. armGetSystemTick
            // runs at ~19.2 MHz so the resolution is ~50 ns — way more than
            // FPS needs. We log "—" on the very first heartbeat since we
            // don't have a previous tick to subtract from.
            let now_tick = unsafe { ruffle_tick_now() };
            let tick_freq = unsafe { ruffle_tick_freq() };
            let fps_str = if self.heartbeat_tick != 0 && tick_freq > 0 {
                let dt_ticks = now_tick.saturating_sub(self.heartbeat_tick);
                if dt_ticks > 0 {
                    // 60 frames over `dt_ticks` ticks at `tick_freq` Hz =
                    // 60 * tick_freq / dt_ticks frames per second. Multiply
                    // by 10 then format as "X.Y" to get one decimal place
                    // without pulling in float formatting.
                    let fps_x10 = (60u64 * tick_freq * 10) / dt_ticks;
                    std::format!("{}.{}", fps_x10 / 10, fps_x10 % 10)
                } else {
                    std::string::String::from("inf")
                }
            } else {
                std::string::String::from("—")
            };
            self.heartbeat_tick = now_tick;
            // Read + clear the tick/render time accumulators populated by
            // render_frame_with_dt in lib.rs. Convert from system ticks
            // (~19.2 MHz) to milliseconds across the 60-frame window. Mean
            // per-frame time = total_ms / 60. Helps localise the bottleneck:
            //   tick=high render=low → AVM1/game-logic CPU bound
            //   tick=low  render=high → GL/draw-call bound
            //   tick=high render=high → both contribute (shape register etc)
            let tick_total_ticks = crate::TICK_TICKS_ACCUM.swap(0, Ordering::Relaxed);
            let render_total_ticks = crate::RENDER_TICKS_ACCUM.swap(0, Ordering::Relaxed);
            let tick_max_ticks = crate::TICK_TICKS_MAX.swap(0, Ordering::Relaxed);
            let render_max_ticks = crate::RENDER_TICKS_MAX.swap(0, Ordering::Relaxed);
            let (tick_ms, render_ms, tick_max_ms, render_max_ms) = if tick_freq > 0 {
                (
                    (tick_total_ticks * 1000) / tick_freq,
                    (render_total_ticks * 1000) / tick_freq,
                    (tick_max_ticks * 1000) / tick_freq,
                    (render_max_ticks * 1000) / tick_freq,
                )
            } else {
                (0, 0, 0, 0)
            };
            let cache_max = self.cache_entries_max_window;
            self.cache_entries_max_window = 0;
            let draw_calls = self.draw_calls_this_window;
            self.draw_calls_this_window = 0;
            let (pushmask, amask, maskeddraw, maskshape) = (
                self.push_mask_window, self.alpha_mask_window,
                self.masked_draw_window, self.mask_shape_draw_window,
            );
            let blend = self.blend_window;
            self.push_mask_window = 0;
            self.alpha_mask_window = 0;
            self.masked_draw_window = 0;
            self.mask_shape_draw_window = 0;
            self.blend_window = 0;
            let (ram_used, ram_total) = query_ram();
            let live_s = LIVE_GPU_SHAPES.load(Ordering::Relaxed);
            let live_d = LIVE_GPU_DRAWS.load(Ordering::Relaxed);
            let v_used_mb = self.vertex_arena.in_use_bytes() / (1024 * 1024);
            let v_peak_mb = self.vertex_arena.peak_in_use / (1024 * 1024);
            let i_used_mb = self.index_arena.in_use_bytes() / (1024 * 1024);
            let i_peak_mb = self.index_arena.peak_in_use / (1024 * 1024);
            let v_frag = self.vertex_arena.free.len();
            let i_frag = self.index_arena.free.len();
            // Actual CPU clock (MHz) + dock state, so we can read whether
            // CpuBoostMode is holding the A57 at 1785 MHz during heavy AVM1
            // scenes (the water lake) — confirming if any CPU headroom remains.
            let cpu_mhz = unsafe { ruffle_cpu_clock_hz() } / 1_000_000;
            let docked = unsafe { ruffle_is_docked() } != 0;
            let msg = std::format!(
                "f{}: fps={} cpu={}MHz dock={} tick={}ms render={}ms dc/win={} shapes={}(live {}) draws_live={} arena_v={}MB/peak{}MB(frag {}) arena_i={}MB/peak{}MB(frag {}) bitmaps={} atlases={} bitmap_draws={} offscreen={} sync={} filter={} fpool={} pushmask={} amask={} blend={} maskeddraw={} maskshape={} tickMax={}ms rndMax={}ms cacheMax={} ram={}MB/{}MB\n",
                self.frame_count,
                fps_str,
                cpu_mhz,
                docked,
                tick_ms,
                render_ms,
                draw_calls,
                self.shapes_registered,
                live_s,
                live_d,
                v_used_mb, v_peak_mb, v_frag,
                i_used_mb, i_peak_mb, i_frag,
                self.bitmaps_registered,
                self.atlases.len(),
                self.bitmap_draws_emitted,
                self.render_offscreen_calls,
                self.resolve_sync_calls,
                self.apply_filter_calls,
                self.filter_tex_pool.len(),
                pushmask,
                amask,
                blend,
                maskeddraw,
                maskshape,
                tick_max_ms,
                render_max_ms,
                cache_max,
                ram_used / (1024 * 1024),
                ram_total / (1024 * 1024),
            );
            let mut bytes = msg.into_bytes();
            bytes.push(0);
            unsafe { ruffle_log_cstr(bytes.as_ptr() as *const _) };
        } else if self.frame_count % 10 == 0 {
            // Tight tick every 10 frames so we know "we made it to f3170"
            // even when the heartbeat hasn't fired. Very short payload.
            let msg = std::format!("·f{}\n", self.frame_count);
            let mut bytes = msg.into_bytes();
            bytes.push(0);
            unsafe { ruffle_log_cstr(bytes.as_ptr() as *const _) };
        }
        if self.frame_count % 60 == 0 {
            unsafe {
                let mut err = glGetError();
                while err != GL_NO_ERROR {
                    let name = match err {
                        GL_INVALID_ENUM => "GL_INVALID_ENUM",
                        GL_INVALID_VALUE => "GL_INVALID_VALUE",
                        GL_INVALID_OPERATION => "GL_INVALID_OPERATION",
                        GL_OUT_OF_MEMORY => "GL_OUT_OF_MEMORY",
                        GL_INVALID_FRAMEBUFFER_OPERATION => "GL_INVALID_FRAMEBUFFER_OPERATION",
                        _ => "GL_UNKNOWN",
                    };
                    let msg = std::format!("gl err: 0x{:04X} ({})\n", err, name);
                    let mut bytes = msg.into_bytes();
                    bytes.push(0);
                    ruffle_log_cstr(bytes.as_ptr() as *const _);
                    err = glGetError();
                }
            }
        }

        unsafe {
            glViewport(
                0,
                0,
                self.dimensions.width as GLsizei,
                self.dimensions.height as GLsizei,
            );
            glClearColor(
                clear.r as GLfloat / 255.0,
                clear.g as GLfloat / 255.0,
                clear.b as GLfloat / 255.0,
                clear.a as GLfloat / 255.0,
            );
            glClearStencil(0);
            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_STENCIL_TEST);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glStencilMask(0xFF);
        }
        self.mask = MaskState::default();
        // Anything outside our render path (Ruffle internals, our own
        // overlay path) may have touched GL state since the last frame's
        // closing reset. Drop the cache so the first use_* below
        // unconditionally re-binds.
        self.gl_state.invalidate();

        commands.execute(self);

        unsafe {
            glUseProgram(0);
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_STENCIL_TEST);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
        // Mirror the actual GL state we just wrote so the cache stays
        // truthful for any post-frame work (e.g. the cursor overlay).
        self.gl_state.invalidate();

        // Close out the per-frame breakdown: delta vs the top-of-frame
        // snapshot. Cumulative counters use wrapping_sub (exact); window
        // counters use saturating_sub so the 1-in-60 heartbeat frame (which
        // zeroed them mid-frame) clamps to 0 instead of printing garbage.
        let s = self.frame_snapshot;
        self.last_frame = FrameBreakdown {
            draw_calls: self.draw_calls_this_window.saturating_sub(s.draw_calls),
            offscreen: self.render_offscreen_calls.wrapping_sub(s.offscreen),
            filter: self.apply_filter_calls.wrapping_sub(s.filter),
            resolve: self.resolve_sync_calls.wrapping_sub(s.resolve),
            bmp_uploads: self.bitmaps_registered.wrapping_sub(s.bmp_uploads),
            shape_regs: self.shapes_registered.wrapping_sub(s.shape_regs),
            blend: self.blend_window.saturating_sub(s.blend),
            pushmask: self.push_mask_window.saturating_sub(s.pushmask),
            masked_draw: self.masked_draw_window.saturating_sub(s.masked_draw),
            cache_entries: frame_cache_entries,
            filter_chains: filter_chains_run as u32,
        };
    }

    fn create_empty_texture(
        &mut self,
        width: NonZeroU32,
        height: NonZeroU32,
    ) -> Result<BitmapHandle, Error> {
        // Standalone (FBO-attachable) texture — Ruffle hands this to
        // `render_offscreen` / cache_entries for cacheAsBitmap + filtered
        // display objects, then draws it back via `render_bitmap`.
        let standalone = make_standalone_texture(width.get(), height.get())
            .ok_or(Error::TooLarge)?;
        self.bitmaps_registered = self.bitmaps_registered.wrapping_add(1);
        Ok(BitmapHandle(Arc::new(StandaloneBitmap(Arc::new(standalone)))))
    }

    fn register_bitmap(&mut self, bitmap: Bitmap<'_>) -> Result<BitmapHandle, Error> {
        let _pt = PrimTimer::new(&PRIM_BMPUP_CUR);
        let Some((bytes, w, h)) = bitmap_to_rgba_bytes(&bitmap) else {
            return Err(Error::UnknownType);
        };
        // Small bitmaps stay atlas-backed: shape bitmap fills (see
        // `DrawKind::Bitmap`) look up by `as_switch_bitmap` which requires the
        // atlas variant, and packing many small bitmaps into one texture is
        // what keeps Tegra's texture count sane. Keeping ALL bitmaps standalone
        // broke the SMWF sky (a shape with a JPEG fill → no atlas variant) — so
        // the atlas path is the default.
        if let Some(meta) = self.pack_into_atlas(&bytes, w, h) {
            self.bitmaps_registered = self.bitmaps_registered.wrapping_add(1);
            return Ok(BitmapHandle(Arc::new(meta)));
        }
        // Too big for the 2048² atlas. Returning Err(TooLarge) here used to make
        // Ruffle's `BitmapRawDataWrapper::bitmap_handle` (which `.expect()`s a
        // handle) PANIC — haunt-the-house's 3400×1600 BitmapData.draw crashed
        // the app (panic → worker-thread TLS fault, see exception.cpp backtrace).
        // Give it a standalone GL texture instead (good up to GL_MAX ≈ 16384,
        // and FBO-attachable — exactly what BitmapData.draw wants), with the
        // pixels uploaded. As a shape FILL it'd fall back to solid (no atlas
        // variant), but it never crashes. Genuine GL OOM / over GL_MAX still
        // returns TooLarge (Ruffle handles a None handle there without us
        // forcing it through the expect path on every frame).
        let Some(standalone) = make_standalone_texture(w, h) else {
            return Err(Error::TooLarge);
        };
        unsafe {
            glBindTexture(GL_TEXTURE_2D, standalone.texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(
                GL_TEXTURE_2D, 0, 0, 0,
                w as GLsizei, h as GLsizei,
                GL_RGBA, GL_UNSIGNED_BYTE, bytes.as_ptr() as *const _,
            );
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        self.warn_once(b"register_bitmap: >2048 bitmap -> standalone texture (no crash)\n\0");
        self.bitmaps_registered = self.bitmaps_registered.wrapping_add(1);
        Ok(BitmapHandle(Arc::new(StandaloneBitmap(Arc::new(standalone)))))
    }

    fn update_texture(
        &mut self,
        handle: &BitmapHandle,
        bitmap: Bitmap<'_>,
        region: PixelRegion,
    ) -> Result<(), Error> {
        let _pt = PrimTimer::new(&PRIM_BMPUP_CUR);
        let rgba = bitmap.to_rgba();
        let w = region.x_max.saturating_sub(region.x_min);
        let h = region.y_max.saturating_sub(region.y_min);
        if w == 0 || h == 0 {
            return Ok(());
        }
        // Standalone texture: upload the sub-region directly to its GL texture.
        if let Some(standalone) = as_standalone_bitmap(handle) {
            unsafe {
                glBindTexture(GL_TEXTURE_2D, standalone.0.texture);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                let stride = rgba.width() as usize * 4;
                let src_offset = (region.y_min as usize) * stride + (region.x_min as usize) * 4;
                glTexSubImage2D(
                    GL_TEXTURE_2D, 0,
                    region.x_min as GLint, region.y_min as GLint,
                    w as GLsizei, h as GLsizei,
                    GL_RGBA, GL_UNSIGNED_BYTE,
                    rgba.data()[src_offset..].as_ptr() as *const _,
                );
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            return Ok(());
        }
        let Some(switch_bitmap) = as_switch_bitmap(handle) else {
            return Err(Error::UnknownHandle(handle.clone()));
        };
        let atlas = match self.atlases.get(switch_bitmap.atlas_index) {
            Some(a) => a,
            None => return Err(Error::UnknownHandle(handle.clone())),
        };
        // Compute the atlas-space pixel offset for the bitmap.
        let base_x = (switch_bitmap.u0 * ATLAS_SIZE as f32).round() as u32;
        let base_y = (switch_bitmap.v0 * ATLAS_SIZE as f32).round() as u32;
        atlas.upload_region(base_x + region.x_min, base_y + region.y_min, w, h, rgba.data());
        Ok(())
    }

    fn create_context3d(
        &mut self,
        _profile: Context3DProfile,
    ) -> Result<Box<dyn Context3D>, Error> {
        Err(Error::Unimplemented("createContext3D".into()))
    }

    fn debug_info(&self) -> Cow<'static, str> {
        Cow::Borrowed(
            "Renderer: SwitchRenderBackend (phase 1.3 — shapes, bitmaps, lines, gradients, masks)",
        )
    }

    fn name(&self) -> &'static str {
        "switch-mesa-gl"
    }

    fn set_quality(&mut self, _quality: StageQuality) {}

    fn compile_pixelbender_shader(
        &mut self,
        _shader: PixelBenderShader,
    ) -> Result<PixelBenderShaderHandle, Error> {
        Err(Error::Unimplemented(
            "Pixel bender shader compilation".into(),
        ))
    }

    fn run_pixelbender_shader(
        &mut self,
        _handle: PixelBenderShaderHandle,
        _arguments: &[PixelBenderShaderArgument],
        _target: &PixelBenderTarget,
    ) -> Result<PixelBenderOutput, Error> {
        Err(Error::Unimplemented("Pixel bender shader".into()))
    }

    fn resolve_sync_handle(
        &mut self,
        handle: Box<dyn SyncHandle>,
        with_rgba: RgbaBufRead,
    ) -> Result<(), Error> {
        let _pt = PrimTimer::new(&PRIM_RESOLVE_CUR);
        // The only sync handles we produce are `BitmapDataSyncHandle` (from
        // BitmapData.draw()). Read the rendered dirty region back from its temp
        // texture (straight alpha) and hand it to Ruffle's copy closure. This is
        // the same readback `render_offscreen` now uses to repatriate the result
        // into an atlas-backed handle for direct-display games.
        let sh = Box::<dyn Any>::downcast::<BitmapDataSyncHandle>(handle)
            .map_err(|_| Error::Unimplemented("resolve_sync_handle: unknown handle".into()))?;
        self.resolve_sync_calls = self.resolve_sync_calls.wrapping_add(1);
        let (rw, rh) = (sh.w, sh.h);
        if rw == 0 || rh == 0 {
            return Ok(());
        }
        let buf = self.readback_region_straight(sh.texture, sh.x, sh.y, rw, rh);
        with_rgba(&buf, rw * 4);
        Ok(())
    }
}

// ─── CommandHandler ───────────────────────────────────────────────────────────

impl CommandHandler for SwitchRenderBackend {
    fn render_bitmap(
        &mut self,
        bitmap: BitmapHandle,
        transform: Transform,
        _smoothing: bool,
        pixel_snapping: PixelSnapping,
    ) {
        if self.mask.writing {
            self.mask_shape_draw_window = self.mask_shape_draw_window.saturating_add(1);
        } else if self.mask.depth > 0 {
            self.masked_draw_window = self.masked_draw_window.saturating_add(1);
        }
        // Standalone (FBO-backed) variant: own GL texture, full [0,1]² UV.
        // Used to draw cacheAsBitmap / filter / BitmapData results back onto
        // the stage.
        if let Some(standalone) = as_standalone_bitmap(&bitmap) {
            let tex = standalone.0.texture;
            let w = standalone.0.width as f32;
            let h = standalone.0.height as f32;
            let mut m = transform.matrix;
            pixel_snapping.apply(&mut m);
            let scaled = Matrix {
                a: m.a * w,
                b: m.b * w,
                c: m.c * h,
                d: m.d * h,
                tx: m.tx,
                ty: m.ty,
            };
            let world = self.world_matrix(&scaled);
            let mult = transform.color_transform.mult_rgba_normalized();
            let add = transform.color_transform.add_rgba_normalized();
            let uv_remap = [0.0, 0.0, 1.0, 1.0];
            self.bitmap_render_count = self.bitmap_render_count.wrapping_add(1);
            self.use_bitmap(&world, &mult, &add, tex, &uv_remap);
            self.gl_state.bind_vao(self.bitmap_vao);
            self.draw_calls_this_window = self.draw_calls_this_window.saturating_add(1);
            // Standalone cache textures store PREMULTIPLIED alpha (the offscreen
            // render uses `glBlendFuncSeparate(ONE, ONE_MINUS_SRC_ALPHA)` for the
            // alpha channel + the glow shader outputs `color * alpha`). The
            // straight-alpha blend used for atlas bitmaps multiplies alpha a
            // second time, producing alpha² output — too faint for filter
            // results like DropShadow. Switch to premultiplied "over" blend
            // for the standalone draw, then restore.
            unsafe {
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            return;
        }
        let Some(switch_bitmap) = as_switch_bitmap(&bitmap) else {
            self.warn_once(b"cmd: render_bitmap with non-Switch BitmapHandle\n\0");
            return;
        };
        let Some(atlas) = self.atlases.get(switch_bitmap.atlas_index) else {
            return;
        };
        let tex = atlas.texture;
        let mut m = transform.matrix;
        pixel_snapping.apply(&mut m);
        let w = switch_bitmap.width as f32;
        let h = switch_bitmap.height as f32;
        let scaled = Matrix {
            a: m.a * w,
            b: m.b * w,
            c: m.c * h,
            d: m.d * h,
            tx: m.tx,
            ty: m.ty,
        };
        let world = self.world_matrix(&scaled);
        let mult = transform.color_transform.mult_rgba_normalized();
        let add = transform.color_transform.add_rgba_normalized();
        let uv_remap = [
            switch_bitmap.u0,
            switch_bitmap.v0,
            switch_bitmap.u1 - switch_bitmap.u0,
            switch_bitmap.v1 - switch_bitmap.v0,
        ];
        self.bitmap_render_count = self.bitmap_render_count.wrapping_add(1);
        self.use_bitmap(&world, &mult, &add, tex, &uv_remap);
        self.gl_state.bind_vao(self.bitmap_vao);
        self.draw_calls_this_window = self.draw_calls_this_window.saturating_add(1);
        unsafe {
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    fn render_stage3d(&mut self, _bitmap: BitmapHandle, _transform: Transform) {
        self.warn_once(b"cmd: render_stage3d (skipped, no Context3D)\n\0");
    }

    fn render_shape(&mut self, shape: ShapeHandle, transform: Transform) {
        let Some(switch_shape) = as_switch_shape(&shape) else {
            self.warn_once(b"cmd: render_shape with non-Switch ShapeHandle\n\0");
            return;
        };
        // Bail out on a non-finite transform: AS code occasionally produces
        // NaN scales/translations that would propagate into the shader and
        // crash the driver mid-sample.
        if !transform.matrix.a.is_finite()
            || !transform.matrix.b.is_finite()
            || !transform.matrix.c.is_finite()
            || !transform.matrix.d.is_finite()
        {
            return;
        }
        let world = self.world_matrix(&transform.matrix);
        if world.iter().any(|v| !v.is_finite()) {
            return;
        }
        let mult = transform.color_transform.mult_rgba_normalized();
        let add = transform.color_transform.add_rgba_normalized();
        // RELIABLE mask counters: only count once we're certain this shape
        // actually issues geometry (past all early-returns). `mask_shape` now
        // means "a mask shape that really draws into the stencil"; if it's ~0
        // while maskee draws are high, mask shapes produce no geometry.
        let ndraws = switch_shape.0.draws.len() as u32;
        if ndraws > 0 {
            if self.mask.writing {
                self.mask_shape_draw_window = self.mask_shape_draw_window.saturating_add(1);
            } else if self.mask.depth > 0 {
                self.masked_draw_window = self.masked_draw_window.saturating_add(1);
            }
        }
        for draw in &switch_shape.0.draws {
            match &draw.kind {
                DrawKind::Solid => {
                    self.use_solid(&world, &mult, &add);
                }
                DrawKind::Gradient {
                    texture_index,
                    local_matrix,
                    gradient_kind,
                    spread,
                    focal,
                } => {
                    let tex = switch_shape.0.gradient_textures[*texture_index];
                    self.use_gradient(
                        &world,
                        &mult,
                        &add,
                        tex,
                        local_matrix,
                        *gradient_kind,
                        *spread,
                        *focal,
                    );
                }
                DrawKind::Bitmap {
                    atlas_index,
                    uv_remap,
                    local_matrix,
                    is_smoothed: _,
                    is_repeating,
                } => {
                    if local_matrix.iter().any(|v| !v.is_finite()) {
                        continue;
                    }
                    let Some(atlas) = self.atlases.get(*atlas_index) else {
                        continue;
                    };
                    let tex = atlas.texture;
                    self.bitmap_draws_emitted = self.bitmap_draws_emitted.wrapping_add(1);
                    self.use_shape_bitmap(
                        &world,
                        &mult,
                        &add,
                        tex,
                        local_matrix,
                        uv_remap,
                        *is_repeating,
                    );
                }
            }
            // Single VAO for all shape draws — it points at the arena VBO
            // and IBO. base_vertex shifts each fetched index by the byte
            // offset of this draw's vertices, expressed as a vertex count
            // (stride 24 bytes = 6 f32 per vertex).
            let stride_bytes = 6 * core::mem::size_of::<f32>() as GLintptr;
            let base_vertex = (draw.vbo_offset / stride_bytes) as GLint;
            self.gl_state.bind_vao(self.shape_vao);
            self.draw_calls_this_window = self.draw_calls_this_window.saturating_add(1);
            unsafe {
                glDrawElementsBaseVertex(
                    GL_TRIANGLES,
                    draw.num_indices,
                    GL_UNSIGNED_INT,
                    draw.ibo_offset as *const _,
                    base_vertex,
                );
            }
        }
    }

    fn render_alpha_mask(
        &mut self,
        maskee_commands: CommandList,
        mask_commands: CommandList,
    ) {
        // Soft alpha/luminance mask (the kind stencil masking can't express).
        // Render maskee + mask into two offscreen textures sized to the current
        // target, composite maskee × mask.alpha into a third, then draw that
        // back over the stage. All three textures share the "row 0 = Flash top"
        // offscreen layout, so the combine pass needs no Y handling; only the
        // final draw-back (proven standalone-bitmap path) flips for the main FB.
        self.alpha_mask_window = self.alpha_mask_window.saturating_add(1);
        // We have a single shared offscreen FBO; recursing into it (when this
        // mask is itself nested inside a cache entry / blend / outer mask)
        // would reset the outer target's color attachment mid-render. Degrade
        // to an inline unmasked draw in that case — the outer render stays
        // correct. The common top-level case (offscreen_dims == None) is fully
        // handled.
        if self.offscreen_dims.is_some() {
            maskee_commands.execute(self);
            return;
        }
        let (w, h) = self.current_target_dims();
        if w == 0 || h == 0 {
            maskee_commands.execute(self);
            return;
        }
        // Acquire all three textures up front so we can fall back to drawing the
        // maskee unmasked (better than vanishing) if the pool/GL is exhausted.
        let acquired = (
            self.filter_tex_pool.acquire(w, h),
            self.filter_tex_pool.acquire(w, h),
            self.filter_tex_pool.acquire(w, h),
        );
        let (maskee, mask, result) = match acquired {
            (Some(a), Some(b), Some(c)) => (a, b, c),
            (a, b, c) => {
                if let Some(t) = a { self.filter_tex_pool.release(t); }
                if let Some(t) = b { self.filter_tex_pool.release(t); }
                if let Some(t) = c { self.filter_tex_pool.release(t); }
                maskee_commands.execute(self);
                return;
            }
        };
        let transparent = Color { r: 0, g: 0, b: 0, a: 0 };
        let mk_ok = self.render_commands_to_texture(maskee.texture, w, h, maskee_commands, Some(transparent));
        let ms_ok = self.render_commands_to_texture(mask.texture, w, h, mask_commands, Some(transparent));
        if mk_ok && ms_ok
            && self.composite_alpha_mask(maskee.texture, mask.texture, result.texture, w, h)
        {
            self.draw_fullscreen_texture(result.texture, w, h, || unsafe {
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            });
        } else if mk_ok {
            // Composite failed but the maskee rendered — show it unmasked.
            self.draw_fullscreen_texture(maskee.texture, w, h, || unsafe {
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            });
        }
        self.filter_tex_pool.release(maskee);
        self.filter_tex_pool.release(mask);
        self.filter_tex_pool.release(result);
    }

    fn draw_rect(&mut self, color: Color, matrix: Matrix) {
        if self.mask.writing {
            self.mask_shape_draw_window = self.mask_shape_draw_window.saturating_add(1);
        }
        let r = color.r as f32 / 255.0;
        let g = color.g as f32 / 255.0;
        let b = color.b as f32 / 255.0;
        let a = color.a as f32 / 255.0;
        #[rustfmt::skip]
        let quad: [f32; 36] = [
            0.0, 0.0, r, g, b, a,
            1.0, 0.0, r, g, b, a,
            1.0, 1.0, r, g, b, a,
            0.0, 0.0, r, g, b, a,
            1.0, 1.0, r, g, b, a,
            0.0, 1.0, r, g, b, a,
        ];
        let world = self.world_matrix(&matrix);
        const IDENT_MULT: [f32; 4] = [1.0, 1.0, 1.0, 1.0];
        const IDENT_ADD: [f32; 4] = [0.0, 0.0, 0.0, 0.0];
        self.use_solid(&world, &IDENT_MULT, &IDENT_ADD);
        self.gl_state.bind_vao(self.rect_vao);
        self.draw_calls_this_window = self.draw_calls_this_window.saturating_add(1);
        unsafe {
            glBindBuffer(GL_ARRAY_BUFFER, self.rect_vbo);
            glBufferData(
                GL_ARRAY_BUFFER,
                core::mem::size_of_val(&quad) as GLsizeiptr,
                quad.as_ptr() as *const _,
                GL_DYNAMIC_DRAW,
            );
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    fn draw_line(&mut self, color: Color, matrix: Matrix) {
        let r = color.r as f32 / 255.0;
        let g = color.g as f32 / 255.0;
        let b = color.b as f32 / 255.0;
        let a = color.a as f32 / 255.0;
        #[rustfmt::skip]
        let line: [f32; 12] = [
            0.0, 0.0, r, g, b, a,
            1.0, 0.0, r, g, b, a,
        ];
        let world = self.world_matrix(&matrix);
        const IDENT_MULT: [f32; 4] = [1.0, 1.0, 1.0, 1.0];
        const IDENT_ADD: [f32; 4] = [0.0, 0.0, 0.0, 0.0];
        self.use_solid(&world, &IDENT_MULT, &IDENT_ADD);
        self.gl_state.bind_vao(self.line_vao);
        self.draw_calls_this_window = self.draw_calls_this_window.saturating_add(1);
        unsafe {
            glLineWidth(1.0);
            glBindBuffer(GL_ARRAY_BUFFER, self.line_vbo);
            glBufferData(
                GL_ARRAY_BUFFER,
                core::mem::size_of_val(&line) as GLsizeiptr,
                line.as_ptr() as *const _,
                GL_DYNAMIC_DRAW,
            );
            glDrawArrays(GL_LINES, 0, 2);
        }
    }

    fn draw_line_rect(&mut self, color: Color, matrix: Matrix) {
        let r = color.r as f32 / 255.0;
        let g = color.g as f32 / 255.0;
        let b = color.b as f32 / 255.0;
        let a = color.a as f32 / 255.0;
        #[rustfmt::skip]
        let lines: [f32; 48] = [
            0.0, 0.0, r, g, b, a,  1.0, 0.0, r, g, b, a,
            1.0, 0.0, r, g, b, a,  1.0, 1.0, r, g, b, a,
            1.0, 1.0, r, g, b, a,  0.0, 1.0, r, g, b, a,
            0.0, 1.0, r, g, b, a,  0.0, 0.0, r, g, b, a,
        ];
        let world = self.world_matrix(&matrix);
        const IDENT_MULT: [f32; 4] = [1.0, 1.0, 1.0, 1.0];
        const IDENT_ADD: [f32; 4] = [0.0, 0.0, 0.0, 0.0];
        self.use_solid(&world, &IDENT_MULT, &IDENT_ADD);
        self.gl_state.bind_vao(self.line_rect_vao);
        self.draw_calls_this_window = self.draw_calls_this_window.saturating_add(1);
        unsafe {
            glLineWidth(1.0);
            glBindBuffer(GL_ARRAY_BUFFER, self.line_rect_vbo);
            glBufferData(
                GL_ARRAY_BUFFER,
                core::mem::size_of_val(&lines) as GLsizeiptr,
                lines.as_ptr() as *const _,
                GL_DYNAMIC_DRAW,
            );
            glDrawArrays(GL_LINES, 0, 8);
        }
    }

    fn push_mask(&mut self) {
        self.mask_push();
    }
    fn activate_mask(&mut self) {
        self.mask_activate();
    }
    fn deactivate_mask(&mut self) {
        self.mask_deactivate();
    }
    fn pop_mask(&mut self) {
        self.mask_pop();
    }

    fn blend(&mut self, commands: CommandList, blend_mode: RenderBlendMode) {
        // Classify per wgpu's `BlendType`:
        //  - Normal/Layer  → just inline the group (source-over is the default
        //    blend, and drawing primitives sequentially is exactly the group's
        //    composite). No extra texture.
        //  - Add/Subtract/Screen ("trivial") → render the group into a temp,
        //    then draw it back with the matching GL blend state, so the group
        //    composites with the backdrop as a unit (no per-primitive double
        //    accumulation).
        //  - Multiply/Lighten/Darken/Difference/Invert/Overlay/HardLight
        //    ("complex") → snapshot the backdrop, render the group into a temp,
        //    then a shader composites the two straight onto the target.
        //  - Alpha/Erase need real layer tracking (group alpha vs the enclosing
        //    layer); Shader is PixelBender (unsupported). Fall back to inline.
        let mode = match blend_mode {
            RenderBlendMode::Builtin(m) => m,
            RenderBlendMode::Shader(_) => {
                commands.execute(self);
                return;
            }
        };

        // Nested inside another offscreen render (cache entry / outer blend /
        // mask)? Our single shared offscreen FBO can't recurse without
        // corrupting the outer target's color attachment, so degrade to a plain
        // inline (Normal) composite. Top-level blends (the common case) run the
        // full path below.
        if self.offscreen_dims.is_some() {
            commands.execute(self);
            return;
        }

        // 0..=6 must match the u_blend_mode switch in COMPLEX_BLEND_FRAG.
        let complex_mode: i32 = match mode {
            BlendMode::Multiply => 0,
            BlendMode::Lighten => 1,
            BlendMode::Darken => 2,
            BlendMode::Difference => 3,
            BlendMode::Invert => 4,
            BlendMode::Overlay => 5,
            BlendMode::HardLight => 6,
            // Non-complex modes handled below.
            BlendMode::Normal | BlendMode::Layer | BlendMode::Alpha | BlendMode::Erase => {
                commands.execute(self);
                return;
            }
            BlendMode::Add | BlendMode::Subtract | BlendMode::Screen => {
                let (w, h) = self.current_target_dims();
                let Some(temp) = (if w == 0 || h == 0 { None } else { self.filter_tex_pool.acquire(w, h) }) else {
                    commands.execute(self);
                    return;
                };
                let transparent = Color { r: 0, g: 0, b: 0, a: 0 };
                if self.render_commands_to_texture(temp.texture, w, h, commands, Some(transparent)) {
                    self.blend_window = self.blend_window.saturating_add(1);
                    let m = mode;
                    self.draw_fullscreen_texture(temp.texture, w, h, move || unsafe {
                        // Premultiplied group temp. Alpha channel always uses
                        // "over"; RGB uses the mode-specific factors/equation.
                        match m {
                            BlendMode::Add => {
                                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                                glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                            }
                            BlendMode::Subtract => {
                                glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
                                glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                            }
                            // Screen: out = src + dst*(1 - src).
                            _ => {
                                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                                glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_COLOR, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                            }
                        }
                    });
                }
                self.filter_tex_pool.release(temp);
                return;
            }
        };

        // Complex path: snapshot the backdrop + render the group, then composite.
        let (w, h) = self.current_target_dims();
        let flip = if self.offscreen_dims.is_some() { 0.0 } else { 1.0 };
        let parent = if w == 0 || h == 0 { None } else { self.filter_tex_pool.acquire(w, h) };
        let current = if w == 0 || h == 0 { None } else { self.filter_tex_pool.acquire(w, h) };
        let (parent, current) = match (parent, current) {
            (Some(p), Some(c)) => (p, c),
            (a, b) => {
                if let Some(t) = a { self.filter_tex_pool.release(t); }
                if let Some(t) = b { self.filter_tex_pool.release(t); }
                commands.execute(self);
                return;
            }
        };
        // Snapshot the current target's colour into `parent` (1:1, so it's
        // sampled straight regardless of target Y orientation). Reads from the
        // currently-bound framebuffer (the main FB here) into the texture bound
        // on the active unit, so pin the active unit to 0 first.
        unsafe {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, parent.texture);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w as GLsizei, h as GLsizei);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        let transparent = Color { r: 0, g: 0, b: 0, a: 0 };
        if self.render_commands_to_texture(current.texture, w, h, commands, Some(transparent)) {
            self.blend_window = self.blend_window.saturating_add(1);
            self.composite_complex_to_current(parent.texture, current.texture, w, h, complex_mode, flip);
        }
        self.filter_tex_pool.release(parent);
        self.filter_tex_pool.release(current);
    }
}

impl Drop for SwitchRenderBackend {
    fn drop(&mut self) {
        unsafe {
            glDeleteBuffers(1, &self.rect_vbo);
            glDeleteVertexArrays(1, &self.rect_vao);
            glDeleteBuffers(1, &self.bitmap_vbo);
            glDeleteVertexArrays(1, &self.bitmap_vao);
            glDeleteBuffers(1, &self.line_vbo);
            glDeleteVertexArrays(1, &self.line_vao);
            glDeleteBuffers(1, &self.line_rect_vbo);
            glDeleteVertexArrays(1, &self.line_rect_vao);
            glDeleteVertexArrays(1, &self.shape_vao);
            if self.offscreen_fbo != 0 {
                glDeleteFramebuffers(1, &self.offscreen_fbo);
            }
            if self.offscreen_depth_stencil != 0 {
                glDeleteRenderbuffers(1, &self.offscreen_depth_stencil);
            }
            // vertex_arena / index_arena released via their Drop impls.
            // Programs freed by their respective Drop impls.
        }
    }
}
