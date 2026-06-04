//! `SwitchLogBackend` — route AVM trace + warning to nxlink stdout.
//!
//! The default `NullLogBackend` in ruffle_core uses `tracing::info!` which
//! goes nowhere here (we haven't wired a tracing subscriber). Route to our
//! existing `ruffle_log_cstr` C bridge so SWF-side `trace("hello")` shows
//! up in the developer's nxlink terminal.

use core::ffi::c_char;

use ruffle_core::backend::log::LogBackend;

extern "C" {
    fn ruffle_log_cstr(msg: *const c_char);
}

pub struct SwitchLogBackend;

impl SwitchLogBackend {
    pub fn new() -> Self {
        Self
    }
}

impl LogBackend for SwitchLogBackend {
    fn avm_trace(&self, message: &str) {
        emit("[trace] ", message);
    }

    fn avm_warning(&self, message: &str) {
        emit("[warn] ", message);
    }
}

fn emit(prefix: &str, message: &str) {
    let mut bytes: std::vec::Vec<u8> =
        std::vec::Vec::with_capacity(prefix.len() + message.len() + 2);
    bytes.extend_from_slice(prefix.as_bytes());
    bytes.extend_from_slice(message.as_bytes());
    if !bytes.ends_with(b"\n") {
        bytes.push(b'\n');
    }
    bytes.push(0);
    unsafe { ruffle_log_cstr(bytes.as_ptr() as *const c_char) };
}
