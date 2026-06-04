//! Phase 1: assemble a `ruffle_core::PlayerBuilder` with our Switch backends
//! and hand the resulting `Player` back to C++ as an opaque handle pointer.
//!
//! Skeleton sketch:
//!   pub struct PlayerHandle(Box<ruffle_core::Player>);
//!   #[no_mangle] pub extern "C" fn ruffle_player_new(swf_path: *const c_char) -> *mut PlayerHandle;
//!   #[no_mangle] pub extern "C" fn ruffle_player_tick(handle: *mut PlayerHandle, dt_ms: f64);
//!   #[no_mangle] pub extern "C" fn ruffle_player_free(handle: *mut PlayerHandle);
