//! `SwitchAudioBackend` — port of `CpalAudioBackend` from
//! `frontend-utils/src/backends/audio.rs`, wired to libnx's `audren` via the
//! C++ side in `cpp/src/audio.cpp`.
//!
//! The pattern is identical to cpal:
//!   1. `AudioMixer` does all the SWF audio work (decoding ADPCM/PCM, mixing
//!      sound instances, applying volume).
//!   2. A `mixer.proxy()` is stashed in a process-global slot so the C++
//!      audio worker thread can pull samples on its own cadence (audren
//!      frame events fire ~every 5 ms).
//!   3. The C++ thread calls back into Rust through `ruffle_audio_fill_buffer`,
//!      which invokes `proxy.mix::<i16>(buf)`. `audrvVoiceAddWaveBuf` then
//!      hands the filled PCM to the renderer.
//!
//! Format chosen: 48 kHz stereo i16 — Switch native audio rate, matches what
//! libnx `audrenSoftwareAudioRendererConfig` documents as the standard.

use core::ffi::{c_int, c_uint};
use std::sync::{Mutex, OnceLock};

use ruffle_core::backend::audio::{
    swf, AudioBackend, AudioMixer, AudioMixerProxy, DecodeError, RegisterError, SoundHandle,
    SoundInstanceHandle, SoundStreamInfo, SoundTransform,
};
use ruffle_core::impl_audio_mixer_backend;

/// Process-global slot for the mixer proxy. The C++ audio worker thread
/// pulls samples through here independently of any Player lock. Wrapped in
/// `Mutex<Option<…>>` so we can swap it out cleanly during shutdown without
/// risking the C++ side reading a freed proxy.
static AUDIO_PROXY: OnceLock<Mutex<Option<AudioMixerProxy>>> = OnceLock::new();

const OUTPUT_CHANNELS: u8 = 2;
const OUTPUT_SAMPLE_RATE: u32 = 48_000;

extern "C" {
    /// Bring audren up. Returns 0 on success, non-zero on failure. Idempotent;
    /// safe to call multiple times.
    fn ruffle_audio_init(sample_rate: c_uint, channels: c_uint) -> c_int;
    /// Tear audren down. Idempotent.
    fn ruffle_audio_shutdown();
    /// Start / pause the audren voice. The mixer keeps state independently;
    /// these only gate whether samples reach the speakers.
    fn ruffle_audio_play();
    fn ruffle_audio_pause();
}

pub struct SwitchAudioBackend {
    mixer: AudioMixer,
}

impl SwitchAudioBackend {
    pub fn new() -> Self {
        let mut mixer = AudioMixer::new(OUTPUT_CHANNELS, OUTPUT_SAMPLE_RATE);
        // Mix at native level (1.0). The old 0.5 headroom avoided i16-cast
        // saturation when Mario 63's many simultaneous SFX + MP3 summed past
        // [-1, 1] (audible crackle, 2026-05-24) — but it halved the volume of
        // EVERY game for that one worst case (e.g. a quiet game peaked at ~10%
        // of full scale). Instead, `ruffle_audio_fill_buffer` now mixes in f32
        // and applies a make-up gain + soft limiter before the i16 cast: quiet
        // games get louder, dense peaks compress smoothly with no crackle.
        mixer.set_volume(1.0);
        // Stash the proxy in the global slot so the C++ side can pull samples.
        let slot = AUDIO_PROXY.get_or_init(|| Mutex::new(None));
        if let Ok(mut guard) = slot.lock() {
            *guard = Some(mixer.proxy());
        }
        let rc = unsafe {
            ruffle_audio_init(OUTPUT_SAMPLE_RATE as c_uint, OUTPUT_CHANNELS as c_uint)
        };
        if rc != 0 {
            // Audio failed to come up; mixer still works in the background
            // (calls return Ok, sounds just don't reach speakers). Log via
            // tracing so the user sees it in nxlink.
            tracing::warn!("audren init returned {} — audio will be silent", rc);
        }
        Self { mixer }
    }
}

impl Default for SwitchAudioBackend {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for SwitchAudioBackend {
    fn drop(&mut self) {
        // Drop the proxy first so the C++ side stops pulling from a stale
        // mixer, then shut audren down.
        if let Some(slot) = AUDIO_PROXY.get() {
            if let Ok(mut guard) = slot.lock() {
                *guard = None;
            }
        }
        unsafe { ruffle_audio_shutdown() };
    }
}

impl AudioBackend for SwitchAudioBackend {
    impl_audio_mixer_backend!(mixer);

    fn play(&mut self) {
        unsafe { ruffle_audio_play() };
    }

    fn pause(&mut self) {
        unsafe { ruffle_audio_pause() };
    }
}

/// Reusable f32 mix scratch for `ruffle_audio_fill_buffer`. A `static` (not a
/// `thread_local!`) on purpose: the caller is a libnx-created C++ worker thread
/// whose Rust TLS isn't initialised, so a thread-local would fault. The audio
/// worker is single-threaded, so this mutex never actually contends.
static SCRATCH_F32: Mutex<std::vec::Vec<f32>> = Mutex::new(std::vec::Vec::new());

/// Called from the C++ audio worker thread to fill `len` interleaved i16
/// stereo samples (so the buffer holds `len/2` frames). No-op when no
/// SwitchAudioBackend is currently alive (returns leaving the buffer at
/// whatever value the caller initialised it to — typically zero).
#[no_mangle]
pub extern "C" fn ruffle_audio_fill_buffer(out: *mut i16, len: usize) {
    if out.is_null() || len == 0 {
        return;
    }
    // SAFETY: the C++ caller guarantees `out` points to at least `len`
    // contiguous i16s for the duration of the call. Audren wave buffers
    // are pinned in dedicated memory pools so they don't move under us.
    let buf = unsafe { core::slice::from_raw_parts_mut(out, len) };
    let Some(slot) = AUDIO_PROXY.get() else {
        return;
    };
    let Ok(mut guard) = slot.lock() else { return };
    let Some(proxy) = guard.as_mut() else { return };
    // Mix in f32 at native level, then apply a make-up gain + soft limiter
    // before the i16 cast (see `set_volume(1.0)` in `new`). The Reinhard curve
    // x/(1+|x|) maps (-inf,inf) → (-1,1) smoothly: quiet sounds get ~GAIN louder
    // in the near-linear region while loud peaks compress gently instead of
    // hard-saturating the i16 cast (the Mario 63 crackle the old 0.5 worked
    // around). |y| < 1 always, so the cast never clips. GAIN is tunable by ear.
    let Ok(mut scratch) = SCRATCH_F32.lock() else { return };
    scratch.resize(len, 0.0);
    proxy.mix::<f32>(&mut scratch[..]);
    const GAIN: f32 = 3.0;
    for (o, &s) in buf.iter_mut().zip(scratch.iter()) {
        let x = s * GAIN;
        let y = x / (1.0 + x.abs());
        *o = (y * 32767.0) as i16;
    }
}
