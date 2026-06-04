//! Raw FFI to libnx (devkitPro Switch SDK).
//!
//! The pure-Rust `nx` crate reimplements Horizon IPC but is no_std and does NOT
//! expose audren, so for this project we bind libnx headers via bindgen.
//!
//! Phase 1: `build.rs` generates `libnx_sys.rs` into OUT_DIR; include it here:
//!     include!(concat!(env!("OUT_DIR"), "/libnx_sys.rs"));
//!
//! Symbols expected from the C side:
//!   HID    : hidInitialize, padConfigureInput, padInitializeDefault,
//!            padUpdate, padGetButtons, padGetStickPos
//!   Audio  : audrenInitialize, audrenStartAudioRenderer, audrvCreate,
//!            audrvMemPoolAdd, audrvMemPoolAttach, audrvVoiceInit,
//!            audrvVoiceAddWaveBuf, audrvUpdate
//!   Applet : appletMainLoop, appletGetCurrentFocusState, appletHook
//!   Socket : socketInitializeDefault, nxlinkStdio
//!
//! `sdmc:/` is mounted automatically by libnx crt0 — no FS bindings needed.
