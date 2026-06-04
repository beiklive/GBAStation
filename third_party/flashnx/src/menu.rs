//! In-game keymap editor screen ("TOUCHES" sub-modal of the pause menu).
//!
//! Two screens, both rendered on top of the frozen-game backdrop already
//! produced by `ruffle_redraw_paused`:
//!
//!   - **List** — scrollable list of Switch buttons (`keymap::EDITABLE_BUTTONS`)
//!     with their current Flash-key binding shown in `[brackets]`. Up/down
//!     navigate, A opens the dropdown for the focused row, B / Minus close.
//!
//!   - **Dropdown** — list of available Flash keys (`keymap::ALL_FLASH_KEYS`,
//!     with index 0 = "(aucune)" to unbind). Up/down navigate, A commits
//!     (saves sidecar immediately, sets `dirty` flag so C++ repopulates its
//!     BINDINGS table on the next frame), B cancels back to the list.
//!
//! All state lives behind a single Mutex so C++ can drive the screen via
//! a thin FFI surface: `is_active`, `open`, `input`, `draw`, `consume_dirty`.
//! Keeping the state machine on the Rust side avoids fattening the FFI with
//! one getter/setter per screen variable.

use std::sync::Mutex;

use crate::backend::render::SwitchRenderBackend;
use crate::keymap;

#[derive(Debug, Clone, Copy)]
enum Screen {
    Inactive,
    /// `selection` indexes `EDITABLE_BUTTONS`, `scroll_offset` is the
    /// index of the topmost visible row (for the 8-rows-at-a-time window).
    List { selection: usize, scroll_offset: usize },
    /// `button_idx` indexes `EDITABLE_BUTTONS` (the row we're editing),
    /// `selection` indexes `ALL_FLASH_KEYS`, `scroll_offset` is the
    /// topmost visible row in the dropdown's scrollable window.
    Dropdown { button_idx: usize, selection: usize, scroll_offset: usize },
}

struct State {
    screen: Screen,
    /// Set true after a successful `set_binding` so the C++ input layer
    /// knows to call its `populate_bindings_from_keymap()` helper next
    /// frame. Cleared by `consume_dirty`.
    dirty: bool,
}

static TOUCHES: Mutex<State> = Mutex::new(State {
    screen: Screen::Inactive,
    dirty: false,
});

/// Maximum rows shown at once in the list screen. Keep in sync with the
/// vertical space `draw_touches_list` allocates in render.rs.
pub const LIST_VISIBLE_ROWS: usize = 8;
/// Maximum rows shown at once in the Flash-key dropdown. Bumped from
/// the old "fits all 12 entries" implicit cap to a fixed 10 — needed
/// after Phase 3.3.bis expanded ALL_FLASH_KEYS to 48 entries (A-Z,
/// 0-9, mods). 10 rows × 40 px = 400 px panel, fits 720-px tall screen
/// with header + footer + 80 px breathing room.
pub const DROPDOWN_VISIBLE_ROWS: usize = 10;

pub fn is_active() -> bool {
    matches!(
        TOUCHES.lock().map(|s| s.screen),
        Ok(Screen::List { .. }) | Ok(Screen::Dropdown { .. })
    )
}

pub fn open() {
    if let Ok(mut s) = TOUCHES.lock() {
        s.screen = Screen::List { selection: 0, scroll_offset: 0 };
    }
}

pub fn close() {
    if let Ok(mut s) = TOUCHES.lock() {
        s.screen = Screen::Inactive;
    }
}

/// Returns true and clears the flag if the C++ caller should refresh its
/// runtime BINDINGS table (a binding was changed since last call).
pub fn consume_dirty() -> bool {
    if let Ok(mut s) = TOUCHES.lock() {
        let d = s.dirty;
        s.dirty = false;
        d
    } else {
        false
    }
}

/// Forward a Switch-button **down-edge** event from C++. `button` is a
/// short name like "A", "Up", "StickLDown". Returns true if the event was
/// consumed (input shouldn't fall through to the game / pause-main menu).
pub fn input(button: &str) -> bool {
    let mut s = match TOUCHES.lock() {
        Ok(g) => g,
        Err(_) => return false,
    };
    match s.screen {
        Screen::Inactive => false,
        Screen::List { selection, scroll_offset } => {
            handle_list_input(&mut s, button, selection, scroll_offset);
            true
        }
        Screen::Dropdown { button_idx, selection, scroll_offset } => {
            handle_dropdown_input(&mut s, button, button_idx, selection, scroll_offset);
            true
        }
    }
}

fn handle_list_input(s: &mut State, button: &str, mut selection: usize, mut scroll: usize) {
    let last = keymap::EDITABLE_BUTTONS.len().saturating_sub(1);
    match button {
        "Up" | "StickLUp" => {
            selection = if selection == 0 { last } else { selection - 1 };
            scroll = clamp_scroll(scroll, selection);
        }
        "Down" | "StickLDown" => {
            selection = if selection >= last { 0 } else { selection + 1 };
            scroll = clamp_scroll(scroll, selection);
        }
        "A" => {
            // Open dropdown for this row. Pre-select the current binding
            // so the cursor lands on the active value, and auto-scroll
            // the dropdown window so the cursor is visible (especially
            // for late-alphabet bindings like "W" that would otherwise
            // be off-screen).
            let btn = keymap::EDITABLE_BUTTONS[selection];
            let current = keymap::current_binding(btn);
            let dropdown_sel = current
                .as_deref()
                .and_then(|k| keymap::ALL_FLASH_KEYS.iter().position(|x| *x == k))
                .unwrap_or(0); // index 0 = "(aucune)"
            let dropdown_scroll = clamp_dropdown_scroll(0, dropdown_sel);
            s.screen = Screen::Dropdown {
                button_idx: selection,
                selection: dropdown_sel,
                scroll_offset: dropdown_scroll,
            };
            return;
        }
        "B" | "Minus" => {
            // Back to pause main — C++ owns that screen, so we just close.
            s.screen = Screen::Inactive;
            return;
        }
        _ => {}
    }
    s.screen = Screen::List { selection, scroll_offset: scroll };
}

fn handle_dropdown_input(
    s: &mut State,
    button: &str,
    button_idx: usize,
    mut selection: usize,
    mut scroll: usize,
) {
    let last = keymap::ALL_FLASH_KEYS.len().saturating_sub(1);
    match button {
        "Up" | "StickLUp" => {
            selection = if selection == 0 { last } else { selection - 1 };
            scroll = clamp_dropdown_scroll(scroll, selection);
        }
        "Down" | "StickLDown" => {
            selection = if selection >= last { 0 } else { selection + 1 };
            scroll = clamp_dropdown_scroll(scroll, selection);
        }
        "A" => {
            // Commit: 0 = unbind, otherwise the chosen Flash key name.
            let btn = keymap::EDITABLE_BUTTONS[button_idx];
            let target = if selection == 0 {
                None
            } else {
                Some(keymap::ALL_FLASH_KEYS[selection])
            };
            let ok = keymap::set_binding(btn, target);
            if ok {
                s.dirty = true;
            }
            // Return to list, restore the same scroll/selection.
            let scroll = clamp_scroll(0, button_idx);
            s.screen = Screen::List { selection: button_idx, scroll_offset: scroll };
            return;
        }
        "B" | "Minus" => {
            // Cancel — back to list without changes.
            let scroll = clamp_scroll(0, button_idx);
            s.screen = Screen::List { selection: button_idx, scroll_offset: scroll };
            return;
        }
        _ => {}
    }
    s.screen = Screen::Dropdown { button_idx, selection, scroll_offset: scroll };
}

/// Adjust `scroll` so the row at `selection` is visible. Window size =
/// `LIST_VISIBLE_ROWS`.
fn clamp_scroll(mut scroll: usize, selection: usize) -> usize {
    if selection < scroll {
        scroll = selection;
    } else if selection >= scroll + LIST_VISIBLE_ROWS {
        scroll = selection + 1 - LIST_VISIBLE_ROWS;
    }
    scroll
}

/// Same as `clamp_scroll` but for the Flash-key dropdown window (10 rows).
fn clamp_dropdown_scroll(mut scroll: usize, selection: usize) -> usize {
    if selection < scroll {
        scroll = selection;
    } else if selection >= scroll + DROPDOWN_VISIBLE_ROWS {
        scroll = selection + 1 - DROPDOWN_VISIBLE_ROWS;
    }
    scroll
}

/// Draw the current TOUCHES screen using the given backend. No-op when
/// inactive. Borrows the keymap briefly to format the bindings snapshot.
pub fn draw(backend: &mut SwitchRenderBackend) {
    let screen = match TOUCHES.lock() {
        Ok(g) => g.screen,
        Err(_) => return,
    };
    match screen {
        Screen::Inactive => {}
        Screen::List { selection, scroll_offset } => {
            // Snapshot bindings for the visible window so we don't hold the
            // keymap lock while rendering (which itself calls draw_rect →
            // GL FFI, slow).
            let bindings: std::vec::Vec<(&'static str, Option<std::string::String>)> = keymap::EDITABLE_BUTTONS
                .iter()
                .map(|btn| (*btn, keymap::current_binding(btn)))
                .collect();
            backend.draw_touches_list(selection, scroll_offset, &bindings, LIST_VISIBLE_ROWS);
        }
        Screen::Dropdown { button_idx, selection, scroll_offset } => {
            let btn = keymap::EDITABLE_BUTTONS[button_idx];
            backend.draw_touches_dropdown(
                btn,
                selection,
                scroll_offset,
                keymap::ALL_FLASH_KEYS,
                DROPDOWN_VISIBLE_ROWS,
            );
        }
    }
}
