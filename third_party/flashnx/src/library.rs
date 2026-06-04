//! Library UI — the "FlashNX launcher" shown at boot before Ruffle starts.
//!
//! Phase 3.4 — replaces the silent `swf_picker_run` first-hit logic with a
//! full game list. C++ enumerates every `.swf` in `sdmc:/ruffle/` and
//! `sdmc:/switch/ruffle/` and forwards each path here via
//! `ruffle_library_add_path`. We parse each file's SWF header lazily on the
//! way in (version + size + dims) so the metadata panel doesn't need to
//! re-open files on every cursor move.
//!
//! Screens (state machine, like `menu::Screen`):
//!   - **Empty**:        no SWF found, instructions where to drop files.
//!   - **List**:         scrollable game list, A=JOUER, X=OPTIONS, -=QUITTER.
//!   - **OptionsModal**: per-game options (TOUCHES + RETOUR for v1).
//!   - **Picked**:       user picked a game; `selected_path` is set; C++
//!                       polls `is_active()` and exits the library loop.
//!   - **Quit**:         user pressed - on the empty/list; C++ exits the
//!                       worker thread (and the `.nro`).
//!
//! Rendering lives in `backend::render::SwitchRenderBackend::draw_library_*`
//! (mirroring the TOUCHES list pattern) so this module stays focused on
//! state + input.
//!
//! When the user selects TOUCHES from the options modal, we delegate to the
//! existing `menu` module's editor — same `menu::open` / `menu::input` /
//! `menu::draw` we use mid-game. To make that work pre-launch, we re-init
//! the keymap module for the chosen game's basename via
//! `keymap::init_for_swf` here too (the keymap module is `OnceLock` so the
//! first init wins — fine because the library always picks BEFORE Ruffle).

use std::fs::File;
use std::io::Read;
use std::sync::Mutex;

use crate::backend::render::SwitchRenderBackend;
use crate::net::{self, RemoteFile};
use crate::{keymap, menu};

/// Currently displayed library screen. `Inactive` is set before
/// `ruffle_library_init` runs and after the user has picked a game.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum Screen {
    Inactive,
    /// No SWF on SD. Shows a help message ("drop .swf in sdmc:/ruffle/").
    Empty,
    /// Main local game list. `selection` indexes `State::entries`,
    /// `scroll_offset` is the topmost visible row.
    List { selection: usize, scroll_offset: usize },
    /// OPTIONS modal for the game at `game_idx`. `selection` indexes
    /// `OPTIONS_ENTRIES`.
    OptionsModal { game_idx: usize, selection: usize },
    /// User pressed A on a game; main loop reads `selected_path` and exits.
    Picked,
    /// User pressed - or chose to quit; main loop exits the `.nro`.
    Quit,
    /// User pressed A on OPTIONS > TOUCHES — control delegated to
    /// `menu::*`. When `menu::is_active()` returns false we return to the
    /// OptionsModal screen.
    TouchesEditor { game_idx: usize },
    /// Confirm screen for OPTIONS > SUPPRIMER. A = delete .swf + all
    /// sidecars / saves matching the basename, then back to List. B =
    /// back to OptionsModal. Destructive, hence the explicit step.
    DeleteConfirm { game_idx: usize },
    // ── Phase 3.7: DISTANT mode (archive.org import) ───────────────────
    /// Entry screen for DISTANT mode. Shows the FlashNX banner + a prompt
    /// "A: SAISIR URL  Y: RETOUR LOCAL  -: QUITTER".
    DistantIdle,
    /// After a successful archive.org metadata fetch. Lists `RemoteFile`s
    /// stored in `State::remote_files`; `selection` indexes that vec.
    DistantFiles { selection: usize, scroll_offset: usize },
    /// Download in flight. The filename and target path live in
    /// `State::download_file_name` / `download_out_path`. Progress polled
    /// every frame via `net::download_progress`.
    DistantDownloading,
    /// Error from URL parse / metadata fetch / download. Message in
    /// `State::distant_error`. B or A dismisses back to DistantIdle.
    DistantError,
}

pub(crate) const OPTIONS_ENTRIES: &[&str] = &["TOUCHES", "RENOMMER", "SUPPRIMER", "RETOUR"];

/// Sidecar JSON written next to the SWF — gives a display name override
/// without touching the physical filename. Per the README 3.4.bis design:
/// **never** rename the .swf file itself (saves/keymap/etc. all key off
/// basename). Sidecar lives at `sdmc:/ruffle/<basename>.meta.json`.
#[derive(Debug, Clone, Default, serde::Serialize, serde::Deserialize)]
pub(crate) struct MetaSidecar {
    pub display_name: Option<std::string::String>,
}

/// User-facing SD roots. Order = priority for lookup (read). Writes
/// always go to entry 0 (the new `sdmc:/flashnx/`). The legacy
/// `sdmc:/ruffle/` is kept for backward compat — users coming from
/// pre-rename builds still see their saves/sidecars without manual
/// migration.
const USER_SD_ROOTS: &[&str] = &["sdmc:/flashnx", "sdmc:/ruffle"];

/// Find a user-facing sidecar / config file by suffix (e.g.
/// "Super_Mario_63_2010.swf.meta.json") under one of the known SD
/// roots. Returns the first existing path, or None.
fn find_user_file(suffix: &str) -> Option<std::string::String> {
    for root in USER_SD_ROOTS {
        let p = std::format!("{}/{}", root, suffix);
        if std::path::Path::new(&p).exists() {
            return Some(p);
        }
    }
    None
}

/// Where to WRITE a user-facing sidecar / config. Always the primary
/// root (entry 0 of `USER_SD_ROOTS`) — new state goes to `flashnx/`.
fn primary_user_path(suffix: &str) -> std::string::String {
    std::format!("{}/{}", USER_SD_ROOTS[0], suffix)
}

fn read_meta_sidecar(basename: &str) -> Option<MetaSidecar> {
    let suffix = std::format!("{}.meta.json", basename);
    let path = find_user_file(&suffix)?;
    let txt = std::fs::read_to_string(&path).ok()?;
    serde_json::from_str(&txt).ok()
}

/// Persist a display-name override. Empty string removes the sidecar so
/// the library reverts to showing the basename. Always writes to the
/// primary root (`sdmc:/flashnx/`); legacy `sdmc:/ruffle/<...>.meta.json`
/// is left untouched (would orphan, but harmless).
fn write_meta_sidecar(basename: &str, display_name: &str) -> bool {
    let path = primary_user_path(&std::format!("{}.meta.json", basename));
    if display_name.trim().is_empty() {
        let _ = std::fs::remove_file(&path);
        // Also try to clean up any legacy copy so the next library
        // boot doesn't resurrect a stale display name.
        if let Some(legacy) = find_user_file(&std::format!("{}.meta.json", basename)) {
            let _ = std::fs::remove_file(&legacy);
        }
        return true;
    }
    let meta = MetaSidecar {
        display_name: Some(display_name.to_string()),
    };
    match serde_json::to_string_pretty(&meta) {
        Ok(json) => std::fs::write(&path, json.as_bytes()).is_ok(),
        Err(_) => false,
    }
}

/// Cached SWF header data parsed once at scan time.
#[derive(Debug, Clone)]
pub(crate) struct Entry {
    pub path: std::string::String,
    pub basename: std::string::String,
    pub display_name: std::string::String,
    pub size_bytes: u64,
    pub swf_version: u8,
    /// 0 = uncompressed FWS, 1 = zlib CWS, 2 = lzma ZWS.
    pub compression_label: &'static str,
    /// True if the movie is ActionScript 3 (AVM2). Surfaced as a neutral "AS3"
    /// tag in the library — Ruffle's AVM2 is less complete than AVM1, so it's
    /// the riskier engine, but it's informational only: many AS3 games run fine
    /// (Mario Forever) while others don't (Pursuit of Hat), so we flag the
    /// engine rather than claim a game is broken.
    pub is_as3: bool,
    /// 0xRRGGBB derived from a hash of the basename — drives the per-game
    /// color chip in the list. Same hash always produces the same color
    /// across reboots (no persistence needed) because the input is stable.
    pub color_chip: u32,
}

pub(crate) struct State {
    pub(crate) screen: Screen,
    pub(crate) entries: std::vec::Vec<Entry>,
    /// Set when the user presses A on a game. Read by C++ via
    /// `ruffle_library_selected_path` after the loop exits.
    selected_path: Option<std::string::String>,
    /// GL texture id for the FlashNX banner image (assets/banner.png decoded
    /// at init). 0 = not loaded (decode failed or init not called yet).
    pub(crate) banner_tex: u32,
    pub(crate) banner_w: u32,
    pub(crate) banner_h: u32,
    /// Monotonic wall-clock ticks captured at init; render() subtracts this
    /// to feed a stable phase into sin() animations (cursor pulse, selection
    /// pulse).
    pub(crate) anim_origin_ticks: u64,
    // ── Phase 3.7 DISTANT mode auxiliary state ─────────────────────────
    /// Files listed by the last archive.org metadata fetch. Populated in
    /// `enter_distant_via_url`, cleared on every successful navigation
    /// back to LOCAL. Indexed by `Screen::DistantFiles::selection`.
    pub(crate) remote_files: std::vec::Vec<RemoteFile>,
    /// Filename + target SD path of the file currently downloading. Saved
    /// so `Screen::DistantDownloading` can show them and post-download
    /// can `add_path` to the local list without re-deriving.
    pub(crate) download_file_name: std::string::String,
    pub(crate) download_out_path: std::string::String,
    /// Last error message — shown in `Screen::DistantError` until user
    /// dismisses with A/B.
    pub(crate) distant_error: std::string::String,
    /// Set of basenames already downloaded this session. Used to draw a
    /// `✓` next to entries in `Screen::DistantFiles` so the user knows
    /// what's been pulled. Cleared by `reset` (back-to-library).
    pub(crate) downloaded_basenames: std::vec::Vec<std::string::String>,
    /// URL history persisted across boots (see `distant_history.json`).
    /// `history_idx` indexes this; 0 = oldest, len-1 = most recent.
    /// Loaded lazily by `load_history_from_sd` on first DistantIdle entry.
    pub(crate) url_history: std::vec::Vec<std::string::String>,
    /// Currently-displayed history index in `Screen::DistantIdle`. Cycled
    /// with L/R. None when history is empty.
    pub(crate) history_idx: Option<usize>,
    /// Active substring filter on the DistantFiles list (X = open swkbd
    /// to set/edit; empty input = clear). Lowercase, substring match
    /// against the lowercased filename. `None` = no filter (show all).
    pub(crate) distant_filter: Option<std::string::String>,
    /// (selection, scroll_offset) snapshot taken when the user pressed A
    /// on a file in DistantFiles, so `on_download_finished` can put them
    /// back on the same row instead of jumping to the top of the list.
    /// Stored as filtered-list indices (matches the screen state).
    pub(crate) download_resume_pos: Option<(usize, usize)>,
}

static LIBRARY: Mutex<State> = Mutex::new(State {
    screen: Screen::Inactive,
    entries: std::vec::Vec::new(),
    selected_path: None,
    banner_tex: 0,
    banner_w: 0,
    banner_h: 0,
    anim_origin_ticks: 0,
    remote_files: std::vec::Vec::new(),
    download_file_name: std::string::String::new(),
    download_out_path: std::string::String::new(),
    distant_error: std::string::String::new(),
    downloaded_basenames: std::vec::Vec::new(),
    url_history: std::vec::Vec::new(),
    history_idx: None,
    distant_filter: None,
    download_resume_pos: None,
});

/// Where the URL history persists across boots. Format: JSON array of
/// strings, max 20 entries (LRU-style — newest at end, oldest dropped
/// when we exceed). Lives in the same dir as `cacert.pem` so the user's
/// `sdmc:/ruffle/` stays SWF-only.
const HISTORY_PATH: &str = "sdmc:/switch/FlashNX/distant_history.json";
const HISTORY_MAX: usize = 20;

/// `visible_rows` on the LOCAL list screen — keep in sync with the slot
/// count drawn in `draw_library_list`. Picked so 1280×720 fits header +
/// banner + rows + footer with margin.
pub const LIST_VISIBLE_ROWS: usize = 6;

/// `visible_rows` on the DISTANT (archive.org) files screen. Larger than
/// LOCAL because typical archive.org dumps run 80-3600+ entries — 10 is
/// the most that fits between the header at y≈150 and footer at y≈680
/// with `ROW_SPACING=50`.
pub const DISTANT_VISIBLE_ROWS: usize = 10;

// ── Setup / FFI helpers ───────────────────────────────────────────────────

extern "C" {
    fn ruffle_tick_now() -> u64;
    fn ruffle_log_cstr(msg: *const core::ffi::c_char);
}

fn log(s: &str) {
    let mut bytes = s.as_bytes().to_vec();
    bytes.push(0);
    unsafe { ruffle_log_cstr(bytes.as_ptr() as *const _) };
}

/// Called from C++ once per `.swf` found during the SD scan. Parses the
/// header inline so the library list has full metadata to display.
pub fn add_path(path: &str) -> bool {
    let basename = path
        .rsplit(['/', '\\'])
        .next()
        .unwrap_or(path)
        .to_string();
    let (size_bytes, swf_version, compression_label, is_as3) = match read_swf_header(path) {
        Some(h) => (h.size_bytes, h.version, h.compression_label, h.is_as3),
        None => {
            log(&std::format!(
                "library: failed to parse SWF header for {}, skipping\n",
                path,
            ));
            return false;
        }
    };
    let color_chip = color_from_basename(&basename);
    // Honour a per-game display-name override from the .meta.json sidecar
    // (Phase 3.4.bis RENOMMER). Sidecar absent / unparseable / empty
    // display_name → fall back to basename.
    let display_name = read_meta_sidecar(&basename)
        .and_then(|m| m.display_name)
        .filter(|s| !s.trim().is_empty())
        .unwrap_or_else(|| basename.clone());
    let entry = Entry {
        path: path.to_string(),
        display_name,
        basename,
        size_bytes,
        swf_version,
        compression_label,
        is_as3,
        color_chip,
    };
    log(&std::format!(
        "library: added {} (SWF v{} {}, {})\n",
        path, swf_version, compression_label,
        if is_as3 { "AS3/AVM2" } else { "AS2/AVM1" },
    ));
    if let Ok(mut s) = LIBRARY.lock() {
        s.entries.push(entry);
    }
    true
}

/// Transition from Inactive → List (or Empty if `entries` is empty). Called
/// after C++ has finished scanning and the GL renderer is up.
pub fn open() {
    // Reload URL history from SD on each open so changes from a previous
    // .nro boot are visible. Cheap (file is <2 KB typical).
    load_history_from_sd();
    if let Ok(mut s) = LIBRARY.lock() {
        s.anim_origin_ticks = unsafe { ruffle_tick_now() };
        s.screen = if s.entries.is_empty() {
            Screen::Empty
        } else {
            Screen::List { selection: 0, scroll_offset: 0 }
        };
    }
}

fn load_history_from_sd() {
    let txt = match std::fs::read_to_string(HISTORY_PATH) {
        Ok(s) => s,
        Err(_) => return, // file absent on first ever boot, normal
    };
    let list: std::vec::Vec<std::string::String> = match serde_json::from_str(&txt) {
        Ok(v) => v,
        Err(e) => {
            log(&std::format!("library: history JSON parse failed: {}\n", e));
            return;
        }
    };
    if let Ok(mut s) = LIBRARY.lock() {
        s.url_history = list;
        if !s.url_history.is_empty() {
            s.history_idx = Some(s.url_history.len() - 1);
        } else {
            s.history_idx = None;
        }
    }
}

fn save_history_to_sd(history: &[std::string::String]) {
    let json = match serde_json::to_string_pretty(&history) {
        Ok(s) => s,
        Err(e) => {
            log(&std::format!("library: history serialise failed: {}\n", e));
            return;
        }
    };
    if let Err(e) = std::fs::write(HISTORY_PATH, json.as_bytes()) {
        log(&std::format!("library: history write failed: {}\n", e));
    }
}

/// Push `url` onto the history. De-dups (if already present, moves to
/// most-recent end). Truncates to `HISTORY_MAX`. Saves to SD.
fn push_history(url: &str) {
    let url = url.trim().to_string();
    if url.is_empty() {
        return;
    }
    if let Ok(mut s) = LIBRARY.lock() {
        if let Some(pos) = s.url_history.iter().position(|u| u == &url) {
            s.url_history.remove(pos);
        }
        s.url_history.push(url);
        while s.url_history.len() > HISTORY_MAX {
            s.url_history.remove(0);
        }
        s.history_idx = Some(s.url_history.len() - 1);
        let snapshot = s.url_history.clone();
        drop(s);
        save_history_to_sd(&snapshot);
    }
}

/// True while the library should keep getting input + render frames. C++
/// loops on this. Returns false once the user picks a game (Picked) or
/// asks to quit (Quit).
pub fn is_active() -> bool {
    match LIBRARY.lock().map(|s| s.screen) {
        Ok(Screen::Inactive) | Ok(Screen::Picked) | Ok(Screen::Quit) => false,
        Ok(_) => true,
        Err(_) => false,
    }
}

/// True if the user picked a game (vs quit). Lets C++ distinguish "load
/// SWF" from "exit `.nro`" after the library loop ends.
pub fn picked() -> bool {
    matches!(LIBRARY.lock().map(|s| s.screen), Ok(Screen::Picked))
}

/// Owned copy of the chosen path. None until the user presses A on a game.
pub fn selected_path() -> Option<std::string::String> {
    LIBRARY.lock().ok().and_then(|s| s.selected_path.clone())
}

/// Reset the library back to Inactive — clears entries, the picked-path
/// slot, and the menu/touches editor sub-screens. Banner texture, banner
/// dims, and anim origin are deliberately kept so we don't re-decode +
/// re-upload the banner PNG every back-to-library cycle. Caller (FFI
/// `ruffle_library_reset`) MUST shutdown and re-init the renderer between
/// game sessions because dropping the SwitchRenderBackend invalidates the
/// banner texture handle that lives on it — we re-decode when the
/// renderer reappears via `ruffle_library_init`.
pub fn reset() {
    // Cancel any download in flight before we clear state — avoids the
    // C++ multi handle leaking + the partial file staying on SD.
    net::cancel_download();
    if let Ok(mut s) = LIBRARY.lock() {
        s.entries.clear();
        s.selected_path = None;
        s.screen = Screen::Inactive;
        s.banner_tex = 0;
        s.banner_w = 0;
        s.banner_h = 0;
        s.remote_files.clear();
        s.download_file_name.clear();
        s.download_out_path.clear();
        s.distant_error.clear();
        s.downloaded_basenames.clear();
        s.distant_filter = None;
        s.download_resume_pos = None;
        // url_history + history_idx are deliberately NOT cleared — they
        // persist across back-to-library cycles AND across .nro reboots
        // (loaded fresh from SD by load_history_from_sd at every library
        // re-open).
    }
    // Make sure any open keymap editor sub-screen closes too — defensive,
    // in practice menu::close() is called by the library state machine on
    // every exit-from-touches transition, but a back-to-library from
    // mid-game means the user never left the in-game menu cleanly.
    menu::close();
}

/// Forward a Switch-button down-edge from C++. Returns true if consumed.
pub fn input(button: &str) -> bool {
    // Sub-screen: TOUCHES editor owns input while active.
    if menu::is_active() {
        let consumed = menu::input(button);
        // If menu just closed itself, fall back to the OPTIONS modal.
        if !menu::is_active() {
            if let Ok(mut s) = LIBRARY.lock() {
                if let Screen::TouchesEditor { game_idx } = s.screen {
                    s.screen = Screen::OptionsModal { game_idx, selection: 0 };
                }
            }
        }
        return consumed;
    }
    // Special case: A / ZR on DistantIdle triggers swkbd or HTTPS metadata
    // fetch, both synchronous + several seconds long. We MUST release the
    // LIBRARY lock for the duration so render() can keep redrawing the
    // last-known screen state and any other callers don't deadlock.
    //
    // Same for A on OPTIONS > RENOMMER — opens swkbd to type the new
    // display name. Hoisted here for the same reason.
    {
        let screen_snap = match LIBRARY.lock() {
            Ok(g) => g.screen,
            Err(_) => return false,
        };
        if screen_snap == Screen::DistantIdle {
            match button {
                "A" => {
                    // Swkbd → fetch flow. Pre-fill from history if available.
                    run_url_fetch_flow();
                    return true;
                }
                "ZR" => {
                    // Re-fetch the currently-displayed history URL without
                    // re-opening the keyboard. Quick "I want this same item
                    // again" path.
                    let url = LIBRARY
                        .lock()
                        .ok()
                        .and_then(|s| s.history_idx.and_then(|i| s.url_history.get(i).cloned()));
                    if let Some(url) = url {
                        run_fetch_for_url(&url);
                    }
                    return true;
                }
                _ => {}
            }
        }
        if let Screen::OptionsModal { game_idx, selection } = screen_snap {
            if button == "A" && OPTIONS_ENTRIES.get(selection).copied() == Some("RENOMMER") {
                run_rename_flow(game_idx);
                return true;
            }
        }
        if matches!(screen_snap, Screen::DistantFiles { .. }) && button == "X" {
            run_distant_search_flow();
            return true;
        }
    }
    let mut s = match LIBRARY.lock() {
        Ok(g) => g,
        Err(_) => return false,
    };
    let screen_copy = s.screen;
    match screen_copy {
        Screen::Inactive | Screen::Picked | Screen::Quit => false,
        Screen::Empty => {
            match button {
                "Minus" => { s.screen = Screen::Quit; }
                "Y" => { s.screen = Screen::DistantIdle; }
                _ => {}
            }
            true
        }
        Screen::List { selection, scroll_offset } => {
            handle_list_input(&mut s, button, selection, scroll_offset);
            true
        }
        Screen::OptionsModal { game_idx, selection } => {
            handle_options_input(&mut s, button, game_idx, selection);
            true
        }
        Screen::TouchesEditor { .. } => false,
        Screen::DeleteConfirm { game_idx } => {
            handle_delete_confirm_input(&mut s, button, game_idx);
            true
        }
        Screen::DistantIdle => {
            handle_distant_idle_input(&mut s, button);
            true
        }
        Screen::DistantFiles { selection, scroll_offset } => {
            handle_distant_files_input(&mut s, button, selection, scroll_offset);
            true
        }
        Screen::DistantDownloading => {
            // B = cancel download. Any other button is ignored during DL.
            if matches!(button, "B") {
                net::cancel_download();
                s.distant_error = std::string::String::from(
                    "Telechargement annule par l'utilisateur."
                );
                s.screen = Screen::DistantError;
            }
            true
        }
        Screen::DistantError => {
            if matches!(button, "A" | "B" | "Minus") {
                s.distant_error.clear();
                s.screen = Screen::DistantIdle;
            }
            true
        }
    }
}

fn handle_list_input(s: &mut State, button: &str, mut selection: usize, mut scroll: usize) {
    let last = s.entries.len().saturating_sub(1);
    match button {
        "Up" | "StickLUp" => {
            selection = if selection == 0 { last } else { selection - 1 };
            scroll = clamp_scroll(scroll, selection, LIST_VISIBLE_ROWS);
        }
        "Down" | "StickLDown" => {
            selection = if selection >= last { 0 } else { selection + 1 };
            scroll = clamp_scroll(scroll, selection, LIST_VISIBLE_ROWS);
        }
        "A" => {
            if let Some(entry) = s.entries.get(selection) {
                s.selected_path = Some(entry.path.clone());
                log(&std::format!(
                    "library: JOUER -> {} ({})\n",
                    entry.display_name, entry.path,
                ));
                s.screen = Screen::Picked;
            }
            return;
        }
        "X" => {
            if !s.entries.is_empty() {
                s.screen = Screen::OptionsModal { game_idx: selection, selection: 0 };
            }
            return;
        }
        "Y" => {
            // Switch to DISTANT (archive.org import) mode.
            s.screen = Screen::DistantIdle;
            return;
        }
        "Minus" => {
            s.screen = Screen::Quit;
            return;
        }
        _ => {}
    }
    s.screen = Screen::List { selection, scroll_offset: scroll };
}

// ── Phase 3.7: DISTANT mode input handlers ────────────────────────────

fn handle_distant_idle_input(s: &mut State, button: &str) {
    // Note: A / ZR are handled at the top of `input()` (hoisted out
    // because they call into swkbd + HTTPS, which we can't run while
    // holding the LIBRARY lock).
    match button {
        "L" => {
            // Older entry in history (idx -= 1, wrap).
            if !s.url_history.is_empty() {
                let len = s.url_history.len();
                let cur = s.history_idx.unwrap_or(len - 1);
                let new_idx = if cur == 0 { len - 1 } else { cur - 1 };
                s.history_idx = Some(new_idx);
            }
        }
        "R" => {
            // Newer entry in history (idx += 1, wrap).
            if !s.url_history.is_empty() {
                let len = s.url_history.len();
                let cur = s.history_idx.unwrap_or(0);
                let new_idx = if cur + 1 >= len { 0 } else { cur + 1 };
                s.history_idx = Some(new_idx);
            }
        }
        "Y" | "B" => {
            s.screen = if s.entries.is_empty() {
                Screen::Empty
            } else {
                Screen::List { selection: 0, scroll_offset: 0 }
            };
        }
        "Minus" => {
            s.screen = Screen::Quit;
        }
        _ => {}
    }
}

/// Drive the full "user typed a URL → fetch metadata → show files" flow.
/// Called from `input()` only — never holds the LIBRARY lock during the
/// swkbd or HTTPS calls (both are seconds-long blocking). The swkbd
/// pre-fills with the currently-displayed history URL so editing a
/// neighbouring item is one-character-change away.
fn run_url_fetch_flow() {
    // Snapshot current history URL to prefill swkbd. Drop the lock
    // before calling out to libnx.
    let prefill = LIBRARY
        .lock()
        .ok()
        .and_then(|s| s.history_idx.and_then(|i| s.url_history.get(i).cloned()));
    let Some(url) = net::prompt_url_with_initial(prefill.as_deref()) else {
        return; // user cancelled
    };
    run_fetch_for_url(&url);
}

/// Fetch the given URL and transition state. Used both by the post-swkbd
/// path (`run_url_fetch_flow`) and by the ZR re-fetch-without-swkbd path.
fn run_fetch_for_url(url: &str) {
    let Some(item_id) = net::extract_item_id(url) else {
        set_distant_error("URL invalide. Attendu une URL archive.org type https://archive.org/details/<id> ou simplement <id>.");
        return;
    };
    log(&std::format!("library: fetching archive.org metadata for {}\n", item_id));
    match net::fetch_archive_metadata(&item_id) {
        Ok(files) if files.is_empty() => {
            set_distant_error("Aucun fichier .SWF trouve dans cet item archive.org.");
        }
        Ok(files) => {
            // Successful fetch — remember the URL for future sessions.
            push_history(url);
            if let Ok(mut s) = LIBRARY.lock() {
                s.remote_files = files;
                s.screen = Screen::DistantFiles { selection: 0, scroll_offset: 0 };
            }
        }
        Err(e) => {
            set_distant_error(&e);
        }
    }
}

fn handle_distant_files_input(
    s: &mut State,
    button: &str,
    mut selection: usize,
    mut scroll: usize,
) {
    // `selection` indexes the FILTERED view, not the raw `remote_files`.
    // When the user presses A we map back to the absolute file via the
    // current filter snapshot. Same applies for L/R page nav + Up/Down.
    let filtered: std::vec::Vec<usize> = filtered_indices(&s.remote_files, &s.distant_filter);
    let total = filtered.len();
    let last = total.saturating_sub(1);
    match button {
        "Up" | "StickLUp" => {
            if total == 0 { return; }
            selection = if selection == 0 { last } else { selection - 1 };
            scroll = clamp_scroll(scroll, selection, DISTANT_VISIBLE_ROWS);
        }
        "Down" | "StickLDown" => {
            if total == 0 { return; }
            selection = if selection >= last { 0 } else { selection + 1 };
            scroll = clamp_scroll(scroll, selection, DISTANT_VISIBLE_ROWS);
        }
        "L" => {
            // Page up — jump by visible_rows, saturate at 0 (no wrap;
            // page nav is for fast traversal, wrapping would be confusing).
            if total == 0 { return; }
            selection = selection.saturating_sub(DISTANT_VISIBLE_ROWS);
            scroll = clamp_scroll(scroll, selection, DISTANT_VISIBLE_ROWS);
        }
        "R" => {
            // Page down — jump by visible_rows, saturate at last.
            if total == 0 { return; }
            selection = (selection + DISTANT_VISIBLE_ROWS).min(last);
            scroll = clamp_scroll(scroll, selection, DISTANT_VISIBLE_ROWS);
        }
        "A" => {
            let Some(abs_idx) = filtered.get(selection).copied() else {
                return;
            };
            let Some(file) = s.remote_files.get(abs_idx).cloned() else {
                return;
            };
            let safe_name: std::string::String = file
                .name
                .chars()
                .map(|c| if matches!(c, '/' | '\\') { '_' } else { c })
                .collect();
            let out_path = std::format!("{}/{}", USER_SD_ROOTS[0], safe_name);
            // If the file is already on SD (entry exists from boot scan),
            // block the download entirely — the OK badge is the signal,
            // re-downloading would just waste bandwidth + overwrite the
            // file. To play it, the user backs out to LOCAL (Y) and hits
            // A on the same file from there. Silent no-op = no popup
            // noise during list navigation.
            if s.entries.iter().any(|e| e.basename == safe_name) {
                log(&std::format!(
                    "library: A ignore — {} deja sur SD (bascule en LOCAL pour jouer)\n",
                    safe_name,
                ));
                return;
            }
            match net::start_download(&file.download_url, &out_path) {
                Ok(()) => {
                    s.download_file_name = file.name.clone();
                    s.download_out_path = out_path;
                    // Remember where the cursor was so we can put it back
                    // after the download finishes (otherwise the user has
                    // to rescroll through 1000s of entries to find their
                    // place).
                    s.download_resume_pos = Some((selection, scroll));
                    s.screen = Screen::DistantDownloading;
                }
                Err(e) => {
                    s.distant_error = e;
                    s.screen = Screen::DistantError;
                }
            }
            return;
        }
        "B" | "Y" => {
            s.remote_files.clear();
            s.distant_filter = None;
            s.screen = Screen::DistantIdle;
            return;
        }
        // X = handled at the top of `input()` (hoisted because swkbd
        // is a synchronous fullscreen applet that mustn't run under
        // the LIBRARY lock).
        _ => {}
    }
    s.screen = Screen::DistantFiles { selection, scroll_offset: scroll };
}

/// Compute the list of absolute indices into `files` that match the
/// active filter. `None` filter or empty string = all indices. Match is
/// substring, case-insensitive, on the filename.
pub(crate) fn filtered_indices(
    files: &[crate::net::RemoteFile],
    filter: &Option<std::string::String>,
) -> std::vec::Vec<usize> {
    let needle = filter
        .as_deref()
        .map(|s| s.trim().to_lowercase())
        .filter(|s| !s.is_empty());
    match needle {
        None => (0..files.len()).collect(),
        Some(q) => files
            .iter()
            .enumerate()
            .filter(|(_, f)| f.name.to_lowercase().contains(&q))
            .map(|(i, _)| i)
            .collect(),
    }
}

/// Set the DistantError state + message from anywhere (used after FFI
/// returns where we don't already hold the lock).
fn set_distant_error(msg: &str) {
    if let Ok(mut s) = LIBRARY.lock() {
        s.distant_error = msg.to_string();
        s.screen = Screen::DistantError;
    }
}

fn handle_options_input(s: &mut State, button: &str, game_idx: usize, mut selection: usize) {
    let last = OPTIONS_ENTRIES.len().saturating_sub(1);
    match button {
        "Up" | "StickLUp" => {
            selection = if selection == 0 { last } else { selection - 1 };
        }
        "Down" | "StickLDown" => {
            selection = if selection >= last { 0 } else { selection + 1 };
        }
        "A" => {
            match OPTIONS_ENTRIES[selection] {
                "TOUCHES" => {
                    // Initialise the keymap for THIS game so the editor's
                    // current_binding/set_binding land in the right
                    // sidecar file. Re-init is a no-op if the basename
                    // matches the last init.
                    if let Some(entry) = s.entries.get(game_idx) {
                        keymap::init_for_swf(&entry.basename);
                    }
                    menu::open();
                    s.screen = Screen::TouchesEditor { game_idx };
                    return;
                }
                "RENOMMER" => {
                    // Handled at the top of `input()` (hoisted out
                    // because it calls into swkbd). No-op here.
                    return;
                }
                "SUPPRIMER" => {
                    s.screen = Screen::DeleteConfirm { game_idx };
                    return;
                }
                "RETOUR" => {
                    let scroll = clamp_scroll(0, game_idx, LIST_VISIBLE_ROWS);
                    s.screen = Screen::List { selection: game_idx, scroll_offset: scroll };
                    return;
                }
                _ => {}
            }
        }
        "B" | "Minus" => {
            let scroll = clamp_scroll(0, game_idx, LIST_VISIBLE_ROWS);
            s.screen = Screen::List { selection: game_idx, scroll_offset: scroll };
            return;
        }
        _ => {}
    }
    s.screen = Screen::OptionsModal { game_idx, selection };
}

/// RENOMMER flow: open swkbd with the current display_name pre-filled,
/// write the .meta.json sidecar with the result, update the in-memory
/// entry. Empty input removes the sidecar (revert to basename). Called
/// from `input()` only — must NOT hold the LIBRARY lock during swkbd.
fn run_rename_flow(game_idx: usize) {
    let (basename, current_display) = match LIBRARY.lock() {
        Ok(g) => match g.entries.get(game_idx) {
            Some(e) => (e.basename.clone(), e.display_name.clone()),
            None => return,
        },
        Err(_) => return,
    };
    let Some(new_name) = net::prompt_rename(&current_display) else {
        return; // cancelled
    };
    let new_name_trimmed = new_name.trim().to_string();
    let persisted = write_meta_sidecar(&basename, &new_name_trimmed);
    if !persisted {
        log("library: write_meta_sidecar failed (in-memory rename only)\n");
    }
    // Update the in-memory entry.
    if let Ok(mut s) = LIBRARY.lock() {
        if let Some(entry) = s.entries.get_mut(game_idx) {
            entry.display_name = if new_name_trimmed.is_empty() {
                entry.basename.clone()
            } else {
                new_name_trimmed
            };
        }
    }
}

/// Handles the destructive SUPPRIMER confirmation screen. A = run the
/// delete + back to List (with selection clamped to the now-shorter list).
/// B / Minus = cancel + back to OptionsModal.
fn handle_delete_confirm_input(s: &mut State, button: &str, game_idx: usize) {
    match button {
        "A" => {
            delete_game(s, game_idx);
            // After deletion the list is shorter — clamp the selection
            // so we don't land out of bounds. If the list is now empty
            // we drop the user back on the Empty screen.
            if s.entries.is_empty() {
                s.screen = Screen::Empty;
                return;
            }
            let new_sel = game_idx.min(s.entries.len() - 1);
            let scroll = clamp_scroll(0, new_sel, LIST_VISIBLE_ROWS);
            s.screen = Screen::List { selection: new_sel, scroll_offset: scroll };
        }
        "B" | "Minus" => {
            s.screen = Screen::OptionsModal { game_idx, selection: 0 };
        }
        _ => {}
    }
}

/// Wipe the `.swf` + all matching sidecars / saves for a game, then
/// remove the in-memory entry. Idempotent re-runs are harmless (delete
/// of missing files is silently ignored by C++). The actual unlink work
/// is done in C++ via `swf_picker_delete_game` (uses opendir/readdir
/// which sidesteps the Horizon `read_dir` truncation bug). We only need
/// to pass the .swf path — C++ derives the basename and scans the parent
/// dir for `<basename>.*` matches.
fn delete_game(s: &mut State, game_idx: usize) {
    let Some(entry) = s.entries.get(game_idx).cloned() else {
        return;
    };
    let mut path_c = entry.path.as_bytes().to_vec();
    path_c.push(0);
    let rc = unsafe {
        swf_picker_delete_game(path_c.as_ptr() as *const core::ffi::c_char)
    };
    log(&std::format!(
        "library: SUPPRIMER {} -> rc={} (count of files removed)\n",
        entry.path, rc,
    ));
    s.entries.remove(game_idx);
}

extern "C" {
    fn swf_picker_delete_game(swf_path: *const core::ffi::c_char) -> core::ffi::c_int;
}

/// Search flow: open swkbd pre-filled with the current filter, submit
/// becomes the new filter. Empty input clears the filter. Selection +
/// scroll reset to 0 so the user starts at the top of the filtered view.
fn run_distant_search_flow() {
    let current = LIBRARY
        .lock()
        .ok()
        .and_then(|s| s.distant_filter.clone())
        .unwrap_or_default();
    let Some(typed) = net::prompt_search(&current) else {
        return; // cancelled
    };
    let trimmed = typed.trim().to_string();
    if let Ok(mut s) = LIBRARY.lock() {
        s.distant_filter = if trimmed.is_empty() { None } else { Some(trimmed) };
        s.screen = Screen::DistantFiles { selection: 0, scroll_offset: 0 };
    }
}

fn clamp_scroll(mut scroll: usize, selection: usize, visible_rows: usize) -> usize {
    if selection < scroll {
        scroll = selection;
    } else if selection >= scroll + visible_rows {
        scroll = selection + 1 - visible_rows;
    }
    scroll
}

/// Render the current screen using the backend. C++ calls this each frame
/// while the library is active, AFTER `glClear` (we own the entire
/// framebuffer — no Ruffle behind us at this stage).
pub fn render(backend: &mut SwitchRenderBackend) {
    let s = match LIBRARY.lock() {
        Ok(g) => g,
        Err(_) => return,
    };
    let screen = s.screen;
    let anim_origin = s.anim_origin_ticks;
    drop(s);

    // Phase = current tick - origin. Drives sin() animations. ms-resolution
    // is enough; we compute it lazily below to avoid an FFI call when the
    // current screen has no animation.
    match screen {
        Screen::Inactive | Screen::Picked | Screen::Quit => {}
        Screen::Empty => {
            backend.draw_library_empty();
        }
        Screen::List { selection, scroll_offset } => {
            // Snapshot entries + banner state so we don't hold the lock
            // across the GL FFI calls in draw_library_list.
            let snapshot = LIBRARY.lock().ok().map(|s| {
                LibraryListSnapshot {
                    entries: s.entries.clone(),
                    banner_tex: s.banner_tex,
                    banner_w: s.banner_w,
                    banner_h: s.banner_h,
                }
            });
            if let Some(snap) = snapshot {
                let now = unsafe { ruffle_tick_now() };
                let phase_ticks = now.saturating_sub(anim_origin);
                backend.draw_library_list(
                    selection,
                    scroll_offset,
                    &snap.entries,
                    LIST_VISIBLE_ROWS,
                    snap.banner_tex,
                    snap.banner_w,
                    snap.banner_h,
                    phase_ticks,
                );
            }
        }
        Screen::OptionsModal { game_idx, selection } => {
            let entry_snapshot = LIBRARY
                .lock()
                .ok()
                .and_then(|s| s.entries.get(game_idx).cloned());
            if let Some(entry) = entry_snapshot {
                backend.draw_library_options(
                    &entry.display_name,
                    selection,
                    OPTIONS_ENTRIES,
                );
            }
        }
        Screen::TouchesEditor { .. } => {
            // Backdrop = Library list frozen behind. Cheapest path: redraw
            // a dim panel + delegate to menu::draw.
            backend.draw_library_dim_backdrop();
            menu::draw(backend);
        }
        Screen::DeleteConfirm { game_idx } => {
            let snap = LIBRARY
                .lock()
                .ok()
                .and_then(|s| s.entries.get(game_idx).map(|e| (e.display_name.clone(), e.basename.clone())));
            if let Some((display_name, basename)) = snap {
                backend.draw_library_delete_confirm(&display_name, &basename);
            }
        }
        // ── Phase 3.7 DISTANT mode ─────────────────────────────────────
        Screen::DistantIdle => {
            // Snapshot history + current pointer so render doesn't hold
            // the lock across GL FFI.
            let (hist_url, hist_pos) = LIBRARY
                .lock()
                .ok()
                .map(|s| {
                    let url = s.history_idx.and_then(|i| s.url_history.get(i).cloned());
                    let pos = s.history_idx.map(|i| (i + 1, s.url_history.len()));
                    (url, pos)
                })
                .unwrap_or((None, None));
            backend.draw_library_distant_idle(hist_url.as_deref(), hist_pos);
        }
        Screen::DistantFiles { selection, scroll_offset } => {
            // Union of session-downloaded basenames (filled by
            // `on_download_finished`) and basenames already scanned
            // from SD into `entries`. The latter catches files that
            // were on SD before this .nro boot — fixes the "OK badge
            // missed across sessions" report.
            let (files, marked, filter, total) = LIBRARY
                .lock()
                .ok()
                .map(|s| {
                    let mut marked = s.downloaded_basenames.clone();
                    for e in &s.entries {
                        if !marked.iter().any(|n| n == &e.basename) {
                            marked.push(e.basename.clone());
                        }
                    }
                    let idx = filtered_indices(&s.remote_files, &s.distant_filter);
                    let total = s.remote_files.len();
                    let filtered: std::vec::Vec<crate::net::RemoteFile> =
                        idx.iter().map(|&i| s.remote_files[i].clone()).collect();
                    (filtered, marked, s.distant_filter.clone(), total)
                })
                .unwrap_or_default();
            backend.draw_library_distant_files(
                selection,
                scroll_offset,
                &files,
                DISTANT_VISIBLE_ROWS,
                &marked,
                filter.as_deref(),
                total,
            );
        }
        Screen::DistantDownloading => {
            // Pump the curl multi handle once per frame and check
            // completion. The progress snapshot reflects whatever the
            // last tick updated.
            let (done, total) = net::download_progress();
            // Snapshot the filename for the UI.
            let file_name = LIBRARY
                .lock()
                .ok()
                .map(|s| s.download_file_name.clone())
                .unwrap_or_default();
            backend.draw_library_distant_downloading(&file_name, done, total);
            match net::tick_download() {
                Ok(false) => {}
                Ok(true) => on_download_finished(),
                Err(msg) => set_distant_error(&msg),
            }
        }
        Screen::DistantError => {
            let msg = LIBRARY
                .lock()
                .ok()
                .map(|s| s.distant_error.clone())
                .unwrap_or_default();
            backend.draw_library_distant_error(&msg);
        }
    }
}

/// Called from `render()` after `tick_download` returns Ok(true). Adds
/// the downloaded file to the local entries list (so it's playable when
/// the user goes back to LOCAL) and returns to the DistantFiles screen
/// so the user can keep picking other files from the same archive.org
/// item without re-typing the URL. The just-downloaded basename is
/// tracked in `downloaded_basenames` so the list shows a `✓` next to it.
fn on_download_finished() {
    let (out_path, file_name) = match LIBRARY.lock() {
        Ok(g) => (g.download_out_path.clone(), g.download_file_name.clone()),
        Err(_) => return,
    };
    if out_path.is_empty() {
        return;
    }
    log(&std::format!("library: download finished -> {}\n", out_path));
    // Add to the LOCAL entries list (so when the user backs out of
    // DISTANT mode, the file appears in the LOCAL library).
    let _ = add_or_replace_path(&out_path);
    if let Ok(mut s) = LIBRARY.lock() {
        s.download_file_name.clear();
        s.download_out_path.clear();
        if !file_name.is_empty() && !s.downloaded_basenames.iter().any(|n| n == &file_name) {
            s.downloaded_basenames.push(file_name);
        }
        // Return to DistantFiles, NOT LOCAL — the user almost certainly
        // wants to pick more files from the same item. To go back to
        // LOCAL they hit Y or B from the DistantFiles screen.
        // Restore the (selection, scroll) we snapshotted at A-press so
        // the cursor lands on the same row the user just downloaded
        // (handy when stepping through a long list).
        let (sel, scroll) = s.download_resume_pos.take().unwrap_or((0, 0));
        // Defensive clamp: if filter changed mid-download (it can't via
        // input lock, but if upstream code ever frees the lock) keep the
        // selection in range.
        let filtered_len = filtered_indices(&s.remote_files, &s.distant_filter).len();
        let sel = sel.min(filtered_len.saturating_sub(1));
        let scroll = clamp_scroll(scroll, sel, DISTANT_VISIBLE_ROWS);
        s.screen = Screen::DistantFiles { selection: sel, scroll_offset: scroll };
    }
}

/// `add_path` with dedupe-by-path: if the basename is already known,
/// REPLACE the entry rather than push a duplicate. Used by the download
/// completion path so re-downloading an existing file refreshes metadata
/// instead of growing the list.
fn add_or_replace_path(path: &str) -> bool {
    // We re-use add_path's header-parse via a manual inline. Cheap.
    if !add_path(path) {
        return false;
    }
    // add_path pushed unconditionally. Dedup: if there are two entries
    // with the same path, keep the most recent (last pushed) one.
    if let Ok(mut s) = LIBRARY.lock() {
        // Find duplicates by path. add_path pushed the latest at the end.
        let last_idx = s.entries.len().saturating_sub(1);
        if last_idx == 0 {
            return true;
        }
        let last_path = s.entries[last_idx].path.clone();
        // Look earlier in the list for the same path; if found, remove it.
        if let Some(prev_idx) = s.entries[..last_idx]
            .iter()
            .position(|e| e.path == last_path)
        {
            s.entries.remove(prev_idx);
        }
    }
    true
}

pub(crate) struct LibraryListSnapshot {
    pub entries: std::vec::Vec<Entry>,
    pub banner_tex: u32,
    pub banner_w: u32,
    pub banner_h: u32,
}

// ── Banner PNG decoding ───────────────────────────────────────────────────

const BANNER_PNG: &[u8] = include_bytes!("../../assets/banner.png");

/// Decode the embedded banner PNG into RGBA bytes + dims. Called once from
/// `ruffle_library_init` (after the renderer is up). On success the caller
/// uploads the bytes as a GL texture; on failure (corrupt PNG, OOM) we just
/// fall back to drawing the title via the pixel font (existing path).
pub(crate) fn decode_banner() -> Option<(std::vec::Vec<u8>, u32, u32)> {
    // png 0.18 wants BufRead + Seek — wrap the static byte slice in a
    // Cursor so std::io::Read + Seek are satisfied without extra alloc.
    let cursor = std::io::Cursor::new(BANNER_PNG);
    let decoder = png::Decoder::new(cursor);
    let mut reader = match decoder.read_info() {
        Ok(r) => r,
        Err(e) => {
            log(&std::format!("library: banner PNG decode_info failed: {:?}\n", e));
            return None;
        }
    };
    let info = reader.info().clone();
    let w = info.width;
    let h = info.height;
    let out_size = reader.output_buffer_size()?;
    let mut buf = std::vec![0u8; out_size];
    if let Err(e) = reader.next_frame(&mut buf) {
        log(&std::format!("library: banner PNG decode failed: {:?}\n", e));
        return None;
    }
    // Promote to RGBA8 if needed. assets/banner.png is RGBA per the README
    // spec, but the user might re-export as RGB or palette down the line.
    let rgba = match info.color_type {
        png::ColorType::Rgba => buf,
        png::ColorType::Rgb => {
            let mut out = std::vec::Vec::with_capacity(buf.len() / 3 * 4);
            for px in buf.chunks_exact(3) {
                out.extend_from_slice(&[px[0], px[1], px[2], 0xFF]);
            }
            out
        }
        png::ColorType::GrayscaleAlpha => {
            let mut out = std::vec::Vec::with_capacity(buf.len() * 2);
            for px in buf.chunks_exact(2) {
                out.extend_from_slice(&[px[0], px[0], px[0], px[1]]);
            }
            out
        }
        png::ColorType::Grayscale => {
            let mut out = std::vec::Vec::with_capacity(buf.len() * 4);
            for &px in &buf {
                out.extend_from_slice(&[px, px, px, 0xFF]);
            }
            out
        }
        png::ColorType::Indexed => {
            log("library: indexed-color PNG banner not supported, ignoring\n");
            return None;
        }
    };
    log(&std::format!(
        "library: banner PNG decoded {}x{} ({} bytes RGBA)\n",
        w, h, rgba.len(),
    ));
    Some((rgba, w, h))
}

/// Called by `ruffle_library_init` after `decode_banner` returned ok. Stores
/// the GL texture id + dims into the library state so `render` can pass
/// them to `draw_library_list`.
pub(crate) fn set_banner_texture(tex: u32, w: u32, h: u32) {
    if let Ok(mut s) = LIBRARY.lock() {
        s.banner_tex = tex;
        s.banner_w = w;
        s.banner_h = h;
    }
}

// ── SWF header parsing ────────────────────────────────────────────────────

struct ParsedSwfHeader {
    size_bytes: u64,
    version: u8,
    compression_label: &'static str,
    is_as3: bool,
}

/// Best-effort ActionScript-3 (AVM2) detection. The authoritative signal is the
/// `FileAttributes` tag's `ActionScript3` flag; that tag is mandatory as the
/// FIRST tag of any SWF >= 8, so we only need the first few dozen bytes of the
/// (decompressed) body — not the whole movie. `file` must be positioned right
/// after the 8-byte SWF header.
fn detect_as3(file: &mut File, version: u8, compression_label: &str) -> bool {
    // SWF < 8 predates FileAttributes → always AVM1.
    if version < 8 {
        return false;
    }
    let mut prefix = [0u8; 64];
    let got = match compression_label {
        "FWS" => fill_read(file, &mut prefix),
        "CWS" => {
            let mut z = flate2::read::ZlibDecoder::new(file);
            fill_read(&mut z, &mut prefix)
        }
        // ZWS = LZMA, only ever emitted for SWF >= 13 (the AS3 era). We don't
        // wire the LZMA prefix reader, so treat it as AS3 by version.
        _ => return true,
    };
    parse_as3_flag(&prefix[..got]).unwrap_or(version >= 9)
}

/// Read repeatedly until `buf` is full or the stream ends. Returns bytes read.
fn fill_read<R: Read>(r: &mut R, buf: &mut [u8]) -> usize {
    let mut n = 0;
    while n < buf.len() {
        match r.read(&mut buf[n..]) {
            Ok(0) => break,
            Ok(k) => n += k,
            Err(_) => break,
        }
    }
    n
}

/// Parse the decompressed SWF body prefix: skip the stage RECT + frame
/// rate/count, then read the first tag. For SWF >= 8 that's `FileAttributes`
/// (tag code 69), whose first flag byte carries `ActionScript3` at bit 3.
fn parse_as3_flag(buf: &[u8]) -> Option<bool> {
    let first = *buf.first()?;
    // RECT: top 5 bits of byte 0 = nbits, then 4 fields of nbits each.
    let nbits = (first >> 3) as usize;
    let rect_bytes = (5 + nbits * 4 + 7) / 8;
    // RECT, then frame rate (u16) + frame count (u16), then the first tag.
    let p = rect_bytes + 4;
    let (lo, hi) = (*buf.get(p)?, *buf.get(p + 1)?);
    let tag_code = u16::from_le_bytes([lo, hi]) >> 6;
    if tag_code != 69 {
        // No FileAttributes as the first tag → not AS3-flagged.
        return Some(false);
    }
    let flags = *buf.get(p + 2)?;
    Some(flags & 0x08 != 0) // ActionScript3 = bit 3.
}

fn read_swf_header(path: &str) -> Option<ParsedSwfHeader> {
    // Take size from the SWF header's `file_length` (u32 LE at bytes 4..8)
    // rather than `fs::metadata().len()` — on Horizon/newlib the latter
    // returned a bogus value (~1.6 GB for every file) that hosed the
    // library metadata panel. The SWF field is canonical anyway: for
    // compressed (CWS/ZWS) movies it's the uncompressed size.
    let mut file = File::open(path).ok()?;
    let mut buf = [0u8; 8];
    let n = file.read(&mut buf).ok()?;
    if n < 8 {
        return None;
    }
    let compression_label = match &buf[0..3] {
        b"FWS" => "FWS",
        b"CWS" => "CWS",
        b"ZWS" => "ZWS",
        _ => return None,
    };
    let version = buf[3];
    let size_bytes = u32::from_le_bytes([buf[4], buf[5], buf[6], buf[7]]) as u64;
    // `file` is now positioned just after the 8-byte header — exactly where
    // detect_as3 expects to start the (possibly compressed) body.
    let is_as3 = detect_as3(&mut file, version, compression_label);
    Some(ParsedSwfHeader {
        size_bytes,
        version,
        compression_label,
        is_as3,
    })
}

// ── Color chip from basename ──────────────────────────────────────────────

/// FNV-1a-style 32-bit hash, folded into HSV (H from hash, S/V fixed) so
/// every basename maps to a distinct vivid color. Tied to the basename
/// (not display_name) so renaming a game in 3.4.bis sidecar doesn't change
/// the chip — same physical file, same chip, less visual jank.
fn color_from_basename(basename: &str) -> u32 {
    let mut h: u32 = 2166136261;
    for &b in basename.as_bytes() {
        h ^= b as u32;
        h = h.wrapping_mul(16777619);
    }
    // Map hash → hue [0, 360), then HSV(H, 0.65, 0.95) → RGB.
    let hue = (h % 360) as f32;
    hsv_to_rgb_u32(hue, 0.65, 0.95)
}

fn hsv_to_rgb_u32(h: f32, s: f32, v: f32) -> u32 {
    let c = v * s;
    let h6 = h / 60.0;
    let x = c * (1.0 - ((h6 % 2.0) - 1.0).abs());
    let (r1, g1, b1) = match h6 as i32 {
        0 => (c, x, 0.0),
        1 => (x, c, 0.0),
        2 => (0.0, c, x),
        3 => (0.0, x, c),
        4 => (x, 0.0, c),
        _ => (c, 0.0, x),
    };
    let m = v - c;
    let r = ((r1 + m) * 255.0) as u32 & 0xFF;
    let g = ((g1 + m) * 255.0) as u32 & 0xFF;
    let b = ((b1 + m) * 255.0) as u32 & 0xFF;
    (r << 16) | (g << 8) | b
}
