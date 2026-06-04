//! User-editable Switch-button → Flash-key bindings.
//!
//! Lookup hierarchy (first hit wins):
//!   1. `sdmc:/ruffle/<basename>.keymap.json` — per-game override (basename
//!      derived from the loaded SWF's URL, e.g. `Super_Mario_63_2010.swf`)
//!   2. `sdmc:/ruffle/keymap_default.json`    — global default chosen by the
//!      user
//!   3. Hardcoded fallback (`FALLBACK_BINDINGS`) — Mario-63-biased Flash
//!      platformer baseline, ships in the .nro. Always available so we never
//!      hand back an empty table.
//!
//! On first boot, if `keymap_default.json` is missing, we write the fallback
//! to SD so the user discovers the file's existence + schema in their
//! `sdmc:/ruffle/` folder.
//!
//! Pattern stolen from RetroArch (`.rmp` per-game remaps in `config/remaps/`)
//! and ScummVM (`[gameid]` INI sections). See README "Customisation des
//! touches" for the user-facing doc.
//!
//! No UI wizard yet — power users edit JSON manually. The in-game remap
//! wizard ("REMAPPER" entry in the pause menu) is a planned follow-up that
//! will write to the same sidecar files.

use std::collections::BTreeMap;
use std::fs::File;
use std::io::{Read, Write};
use std::sync::Mutex;

use serde::{Deserialize, Serialize};

/// JSON-serialisable keymap. The field order in `bindings` is preserved
/// (BTreeMap → alphabetical) so the file is diff-stable when the user
/// regenerates / edits it.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Keymap {
    pub version: u32,
    pub bindings: BTreeMap<std::string::String, std::string::String>,
}

/// Hardcoded fallback baked into the .nro. Mirrors what `cpp/src/main.cpp`
/// used to declare as the static `BINDINGS` array — Mario 63's Z/X/Shift
/// platformer convention plus universal Space/Enter/Escape/P/arrows. Buttons
/// reserved by the runtime ("Minus" = pause-menu) are absent on purpose;
/// see `RESERVED_BUTTONS` for the explanation.
pub const FALLBACK_BINDINGS: &[(&str, &str)] = &[
    ("A",            "Space"),  // jump in most Flash games
    ("B",            "Z"),      // alt jump (Mario 63 uses Z)
    ("X",            "X"),      // run / item / dive
    ("Y",            "Shift"),  // alt run
    ("R",            "Enter"),  // "Press Start" prompts
    ("Plus",         "P"),      // standard in-game pause key
    ("L",            "Escape"),
    ("Left",         "Left"),
    ("Right",        "Right"),
    ("Up",           "Up"),
    ("Down",         "Down"),
    ("StickLLeft",   "Left"),
    ("StickLRight",  "Right"),
    ("StickLUp",     "Up"),
    ("StickLDown",   "Down"),
];

/// Buttons the runtime owns — user cannot remap these via JSON. Trying to
/// bind one is silently ignored on load (with a warn log so the user sees
/// why their edit didn't take). Keeps the pause menu always reachable.
pub const RESERVED_BUTTONS: &[&str] = &["Minus"];

/// Buttons exposed in the TOUCHES editor UI (Phase 3.3 suite). Order =
/// display order in the list. Subset of all joycon buttons known to the C++
/// input layer; reserved buttons are absent on purpose.
pub const EDITABLE_BUTTONS: &[&str] = &[
    "A", "B", "X", "Y",
    "L", "R", "ZL", "Plus",
    "Up", "Down", "Left", "Right",
    "StickLUp", "StickLDown", "StickLLeft", "StickLRight",
];

/// Flash-key options shown in the TOUCHES dropdown. Index 0 ("(aucune)")
/// unbinds the button. Must be a superset of what `flash_key_name_to_sk`
/// recognises — additions here without a matching `flash_key_name_to_sk`
/// arm will log "unknown Flash key" at lookup time.
pub const ALL_FLASH_KEYS: &[&str] = &[
    "(aucune)",
    // Common modifier / nav keys first (most used in Flash games).
    "Space",
    "Enter",
    "Escape",
    "Shift",
    "Control",
    "Alt",
    "Tab",
    "Backspace",
    "Up", "Down", "Left", "Right",
    // Full alphabet (A-Z). Sorted so the user can navigate by letter.
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
    "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
    // Digits 0-9.
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
];

/// SWF basename the keymap was loaded for. Used by `save_sidecar` to know
/// where to write. Set by `init_for_swf` — replaced when the user goes
/// back to the library and picks a different game (Phase 3.4 quit-to-
/// library flow). REDEMARRER keeps the same basename and so re-uses the
/// already-loaded keymap.
static ACTIVE_BASENAME: Mutex<Option<std::string::String>> = Mutex::new(None);

/// In-memory keymap. Lock is held briefly across single-key edits + during
/// sidecar write.
static ACTIVE_KEYMAP: Mutex<Option<Keymap>> = Mutex::new(None);

fn fallback_keymap() -> Keymap {
    let mut bindings = BTreeMap::new();
    for (btn, key) in FALLBACK_BINDINGS {
        bindings.insert((*btn).into(), (*key).into());
    }
    Keymap { version: 1, bindings }
}

/// User-visible SD roots, priority order. Reads scan all, first hit
/// wins. Writes always go to entry 0. Mirrors `library::USER_SD_ROOTS`
/// — duplicated here to keep keymap a leaf module (no library dep).
const USER_SD_ROOTS: &[&str] = &["sdmc:/flashnx", "sdmc:/ruffle"];

fn find_user_path(suffix: &str) -> Option<std::string::String> {
    for root in USER_SD_ROOTS {
        let p = std::format!("{}/{}", root, suffix);
        if std::path::Path::new(&p).exists() {
            return Some(p);
        }
    }
    None
}

fn primary_path(suffix: &str) -> std::string::String {
    std::format!("{}/{}", USER_SD_ROOTS[0], suffix)
}

/// Read a small JSON file using chunked 4 KB reads — same workaround as
/// `SwitchStorageBackend::get`. `std::fs::read` on Horizon newlib returns
/// `ENOMEM` once the buffer hits ~32 KB; keymap files are far below that
/// threshold today but we share the safe path defensively.
fn read_json_file(path: &str) -> Option<std::string::String> {
    let mut file = File::open(path).ok()?;
    let mut data = std::vec::Vec::new();
    let mut buf = [0u8; 4096];
    loop {
        match file.read(&mut buf) {
            Ok(0) => break,
            Ok(n) => data.extend_from_slice(&buf[..n]),
            Err(_) => return None,
        }
    }
    std::string::String::from_utf8(data).ok()
}

fn parse_keymap(json: &str, source: &str) -> Option<Keymap> {
    match serde_json::from_str::<Keymap>(json) {
        Ok(mut km) => {
            // Strip reserved buttons silently — they'd break the runtime
            // contract if honoured.
            for btn in RESERVED_BUTTONS {
                if km.bindings.remove(*btn).is_some() {
                    log(&std::format!(
                        "keymap: ignoring reserved button '{}' in {}\n",
                        btn, source,
                    ));
                }
            }
            Some(km)
        }
        Err(e) => {
            log(&std::format!(
                "keymap: failed to parse {} ({}), skipping\n",
                source, e,
            ));
            None
        }
    }
}

fn write_default_to_sd(path: &str, keymap: &Keymap) {
    // Pretty-printed so a user opening it in Notepad sees one binding per
    // line — easy to edit without breaking comma placement.
    let json = match serde_json::to_string_pretty(keymap) {
        Ok(s) => s,
        Err(e) => {
            log(&std::format!("keymap: serialize default failed: {}\n", e));
            return;
        }
    };
    match File::create(path) {
        Ok(mut f) => {
            if let Err(e) = f.write_all(json.as_bytes()) {
                log(&std::format!(
                    "keymap: write default to {} failed: {}\n",
                    path, e,
                ));
            } else {
                log(&std::format!(
                    "keymap: wrote default to {} ({} bytes) — user can now edit it\n",
                    path,
                    json.len(),
                ));
            }
        }
        Err(e) => {
            log(&std::format!(
                "keymap: create {} failed: {} (SD card readonly?)\n",
                path, e,
            ));
        }
    }
}

/// Initialise `ACTIVE_KEYMAP` for the given SWF basename (e.g.
/// "Super_Mario_63_2010.swf"). Called from `ruffle_init` and from the
/// library's pre-launch OPTIONS > TOUCHES path. Idempotent **for the same
/// basename** — REDEMARRER keeps the same keymap, but back-to-library +
/// pick-different-game reloads the per-game sidecar so the user doesn't
/// inherit the previous game's bindings.
pub fn init_for_swf(swf_basename: &str) {
    if let Ok(g) = ACTIVE_BASENAME.lock() {
        if g.as_deref() == Some(swf_basename) {
            // Same basename as last init — no-op.
            return;
        }
    }
    // Lookup order: new `sdmc:/flashnx/` first, then legacy
    // `sdmc:/ruffle/`. Writes (save_sidecar, default bootstrap) go to
    // the primary `flashnx/` location.
    let sidecar_name = std::format!("{}.keymap.json", swf_basename);
    let sidecar = find_user_path(&sidecar_name);
    let default = find_user_path("keymap_default.json");
    let default_write = primary_path("keymap_default.json");

    let km = if let Some(txt) = sidecar.as_deref().and_then(read_json_file) {
        let path_str = sidecar.as_deref().unwrap_or("?");
        log(std::format!("keymap: using per-game sidecar {}\n", path_str));
        parse_keymap(&txt, path_str).unwrap_or_else(|| {
            log("keymap: sidecar invalid, falling back to default\n");
            try_default_or_fallback(default.as_deref())
        })
    } else if let Some(txt) = default.as_deref().and_then(read_json_file) {
        let path_str = default.as_deref().unwrap_or("?");
        log(std::format!("keymap: using global default {}\n", path_str));
        parse_keymap(&txt, path_str).unwrap_or_else(|| {
            log("keymap: default invalid, falling back to hardcoded\n");
            fallback_keymap()
        })
    } else {
        // No JSON on SD at all — write the hardcoded fallback to the new
        // default path so the user discovers the schema in their flashnx
        // dir on next reboot / SD inspection. Only on first ever boot.
        log("keymap: no JSON on SD, bootstrapping global default + using hardcoded fallback\n");
        let km = fallback_keymap();
        write_default_to_sd(&default_write, &km);
        km
    };

    if let Ok(mut g) = ACTIVE_BASENAME.lock() {
        *g = Some(swf_basename.into());
    }
    if let Ok(mut g) = ACTIVE_KEYMAP.lock() {
        *g = Some(km);
    }
}

/// Clear the active keymap so the next `init_for_swf` re-reads from SD.
/// Called by `ruffle_library_reset` when the user quits a game back to
/// the library — the next pick may be a different game with a different
/// per-game sidecar, so we drop the current one to force a fresh load.
pub fn reset() {
    if let Ok(mut g) = ACTIVE_KEYMAP.lock() {
        *g = None;
    }
    if let Ok(mut g) = ACTIVE_BASENAME.lock() {
        *g = None;
    }
}

/// Current binding for `button` (e.g. "A"), or `None` if unbound. Caller
/// gets an owned String to avoid holding the Mutex across UI work.
pub fn current_binding(button: &str) -> Option<std::string::String> {
    let g = ACTIVE_KEYMAP.lock().ok()?;
    g.as_ref()?.bindings.get(button).cloned()
}

/// Set `button` → `flash_key` (e.g. "A" → "Space"). `None` clears the
/// binding. Triggers a write to the per-game sidecar so the change persists
/// across reboots. Returns false on write failure (in-memory change still
/// applied — caller can retry / surface error).
pub fn set_binding(button: &str, flash_key: Option<&str>) -> bool {
    {
        let mut g = match ACTIVE_KEYMAP.lock() {
            Ok(g) => g,
            Err(_) => return false,
        };
        let Some(km) = g.as_mut() else { return false };
        match flash_key {
            Some(k) => {
                km.bindings.insert(button.into(), k.into());
            }
            None => {
                km.bindings.remove(button);
            }
        }
    }
    save_sidecar()
}

/// Persist the active keymap to `sdmc:/ruffle/<basename>.keymap.json`. Auto
/// called by `set_binding`; also callable directly. Returns true on success.
pub fn save_sidecar() -> bool {
    let basename = match ACTIVE_BASENAME.lock() {
        Ok(g) => match g.as_ref() {
            Some(b) => b.clone(),
            None => return false,
        },
        Err(_) => return false,
    };
    let km = match ACTIVE_KEYMAP.lock() {
        Ok(g) => match g.as_ref() {
            Some(k) => k.clone(),
            None => return false,
        },
        Err(_) => return false,
    };
    let path = primary_path(&std::format!("{}.keymap.json", basename));
    let json = match serde_json::to_string_pretty(&km) {
        Ok(s) => s,
        Err(e) => {
            log(std::format!("keymap: serialize for save failed: {}\n", e));
            return false;
        }
    };
    match File::create(&path) {
        Ok(mut f) => match f.write_all(json.as_bytes()) {
            Ok(_) => {
                log(std::format!(
                    "keymap: saved sidecar {} ({} bytes)\n",
                    path,
                    json.len(),
                ));
                true
            }
            Err(e) => {
                log(std::format!("keymap: write to {} failed: {}\n", path, e));
                false
            }
        },
        Err(e) => {
            log(std::format!("keymap: create {} failed: {}\n", path, e));
            false
        }
    }
}

fn try_default_or_fallback(default_path: Option<&str>) -> Keymap {
    let Some(default_path) = default_path else { return fallback_keymap(); };
    if let Some(txt) = read_json_file(default_path) {
        parse_keymap(&txt, default_path).unwrap_or_else(fallback_keymap)
    } else {
        fallback_keymap()
    }
}

/// Look up the Flash key bound to `button_name` (e.g. "A", "StickLLeft").
/// Returns `None` if no binding exists (button is unmapped). Returns the
/// SK_* code from `crate::sk_*` if a binding exists and the target key is
/// recognised. Returns `Some(SK_NONE)` if the JSON binds to a key we don't
/// support yet — caller treats that as "ignored".
pub fn lookup(button_name: &str) -> Option<core::ffi::c_int> {
    let g = ACTIVE_KEYMAP.lock().ok()?;
    let km = g.as_ref()?;
    let key_name = km.bindings.get(button_name)?;
    Some(flash_key_name_to_sk(key_name))
}

/// Map a Flash key NAME (as written in JSON, e.g. "Space", "Z") to one of
/// our `SK_*` integer constants. Add new entries here as we expand keyboard
/// support. Unknown names log a warning and return `SK_NONE`.
fn flash_key_name_to_sk(name: &str) -> core::ffi::c_int {
    match name {
        "Space"     => crate::SK_SPACE,
        "Enter"     => crate::SK_ENTER,
        "Escape"    => crate::SK_ESCAPE,
        "Shift"     => crate::SK_SHIFT,
        "Control"   => crate::SK_CONTROL,
        "Alt"       => crate::SK_ALT,
        "Tab"       => crate::SK_TAB,
        "Backspace" => crate::SK_BACKSPACE,
        "Left"      => crate::SK_LEFT,
        "Right"     => crate::SK_RIGHT,
        "Up"        => crate::SK_UP,
        "Down"      => crate::SK_DOWN,
        // A-Z.
        "A" => crate::SK_A, "B" => crate::SK_B, "C" => crate::SK_C,
        "D" => crate::SK_D, "E" => crate::SK_E, "F" => crate::SK_F,
        "G" => crate::SK_G, "H" => crate::SK_H, "I" => crate::SK_I,
        "J" => crate::SK_J, "K" => crate::SK_K, "L" => crate::SK_L,
        "M" => crate::SK_M, "N" => crate::SK_N, "O" => crate::SK_O,
        "P" => crate::SK_P, "Q" => crate::SK_Q, "R" => crate::SK_R,
        "S" => crate::SK_S, "T" => crate::SK_T, "U" => crate::SK_U,
        "V" => crate::SK_V, "W" => crate::SK_W, "X" => crate::SK_X,
        "Y" => crate::SK_Y, "Z" => crate::SK_Z,
        // 0-9.
        "0" => crate::SK_0, "1" => crate::SK_1, "2" => crate::SK_2,
        "3" => crate::SK_3, "4" => crate::SK_4, "5" => crate::SK_5,
        "6" => crate::SK_6, "7" => crate::SK_7, "8" => crate::SK_8,
        "9" => crate::SK_9,
        other => {
            log(std::format!(
                "keymap: unknown Flash key '{}' in bindings — ignored\n",
                other,
            ));
            crate::SK_NONE
        }
    }
}

// ── Logging helper ────────────────────────────────────────────────────────
// Shadow of crate::log/log_str so this module is self-contained for the
// `b"..."` byte-string case while still routing through the same C
// `ruffle_log_cstr` sink.

extern "C" {
    fn ruffle_log_cstr(msg: *const core::ffi::c_char);
}

trait LogArg {
    fn emit(self);
}

impl LogArg for &str {
    fn emit(self) {
        let mut bytes = self.as_bytes().to_vec();
        bytes.push(0);
        unsafe { ruffle_log_cstr(bytes.as_ptr() as *const _) };
    }
}

impl LogArg for &std::string::String {
    fn emit(self) {
        self.as_str().emit();
    }
}

impl LogArg for std::string::String {
    fn emit(self) {
        self.as_str().emit();
    }
}

impl LogArg for &[u8] {
    fn emit(self) {
        // Always copy + append NUL — relying on the caller to include `\0`
        // in their byte literal is a footgun (forgot one → ruffle_log_cstr
        // reads past the buffer into adjacent .rodata until it finds a
        // stray NUL, dumping unrelated strings into stdout — caught
        // 2026-05-25 nuit during first hardware test of this module).
        let mut v = self.to_vec();
        v.push(0);
        unsafe { ruffle_log_cstr(v.as_ptr() as *const _) };
    }
}

// b"..." literals have type &[u8; N] — coerce to slice so we don't force
// the caller to write `&b"..."[..]` at each site.
impl<const N: usize> LogArg for &[u8; N] {
    fn emit(self) {
        let s: &[u8] = self;
        s.emit();
    }
}

fn log<T: LogArg>(msg: T) {
    msg.emit();
}
