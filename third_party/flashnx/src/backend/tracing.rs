//! Minimal `tracing::Subscriber` that pipes events into the existing
//! `ruffle_log_cstr` C bridge, so Ruffle's internal `error!`/`warn!`/`info!`
//! calls reach nxlink. Without this they hit Ruffle's default noop dispatch
//! and disappear — making any internal failure (notably the frame-40 crash
//! during Mario 63's preload) silent.
//!
//! We only forward events at INFO level and above (info, warn, error),
//! skipping debug/trace to keep the log stream readable. Spans are kept
//! minimal: a single dummy ID is reused so we don't pay any allocation cost.

use core::ffi::c_char;
use core::sync::atomic::{AtomicU64, Ordering};
use std::fmt::Write;

use tracing::field::{Field, Visit};
use tracing::{span, Event, Metadata, Subscriber};

extern "C" {
    fn ruffle_log_cstr(msg: *const c_char);
}

pub struct SwitchTracingSubscriber {
    next_span: AtomicU64,
}

impl SwitchTracingSubscriber {
    pub const fn new() -> Self {
        Self {
            next_span: AtomicU64::new(1),
        }
    }
}

impl Subscriber for SwitchTracingSubscriber {
    fn enabled(&self, meta: &Metadata<'_>) -> bool {
        // INFO and above (info, warn, error). Trace + debug filtered out
        // to avoid spamming nxlink during normal play.
        *meta.level() <= tracing::Level::INFO
    }

    fn new_span(&self, _attrs: &span::Attributes<'_>) -> span::Id {
        // Generate a unique ID per span so any code that compares IDs (rare)
        // doesn't false-equate two unrelated spans.
        let id = self.next_span.fetch_add(1, Ordering::Relaxed);
        // Span IDs must be non-zero.
        span::Id::from_u64(id.max(1))
    }

    fn record(&self, _span: &span::Id, _values: &span::Record<'_>) {}
    fn record_follows_from(&self, _span: &span::Id, _follows: &span::Id) {}
    fn enter(&self, _span: &span::Id) {}
    fn exit(&self, _span: &span::Id) {}

    fn event(&self, event: &Event<'_>) {
        let meta = event.metadata();
        let level = meta.level();
        let mut visitor = MsgVisitor::default();
        event.record(&mut visitor);
        let mut line = std::format!(
            "[tr/{}] {}: {}\n",
            level,
            meta.target(),
            visitor.message,
        );
        if line.len() > 800 {
            line.truncate(800);
            line.push('\n');
        }
        let mut bytes = line.into_bytes();
        bytes.push(0);
        unsafe { ruffle_log_cstr(bytes.as_ptr() as *const c_char) };
    }
}

#[derive(Default)]
struct MsgVisitor {
    message: std::string::String,
}

impl Visit for MsgVisitor {
    fn record_debug(&mut self, field: &Field, value: &dyn core::fmt::Debug) {
        if field.name() == "message" {
            let _ = write!(&mut self.message, "{:?}", value);
        } else {
            let _ = write!(&mut self.message, " {}={:?}", field.name(), value);
        }
    }

    fn record_str(&mut self, field: &Field, value: &str) {
        if field.name() == "message" {
            self.message.push_str(value);
        } else {
            let _ = write!(&mut self.message, " {}={}", field.name(), value);
        }
    }
}
