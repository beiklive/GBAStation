//! Phase 3.7 — archive.org import layer (HTTPS via C++ libcurl).
//!
//! Splits the work between Rust and C++ along the natural boundary:
//!   - C++ does the curl + TLS + file I/O work (see `cpp/src/net.cpp`).
//!   - Rust does URL parsing, JSON parsing, and state transitions.
//!
//! The CA bundle (Mozilla's ca-bundle.crt from msys2, ~228 KB) is embedded
//! here via `include_bytes!` and written to SD at boot. We pay 228 KB in
//! the .nro but skip a runtime download (the user couldn't bootstrap
//! HTTPS without already having HTTPS).

use core::ffi::{c_char, c_int};

extern "C" {
    fn write_cacert_to_sd(data: *const c_char, len: c_int) -> c_int;
    fn https_get_into_buf(url: *const c_char, buf: *mut c_char, cap: c_int) -> c_int;
    fn https_download_start(url: *const c_char, out_path: *const c_char) -> c_int;
    fn https_download_tick() -> c_int;
    fn https_download_progress(done_out: *mut u64, total_out: *mut u64);
    fn https_download_cancel();
    fn swkbd_prompt_url(initial: *const c_char, out: *mut c_char, cap: c_int) -> c_int;
    fn swkbd_prompt_rename(initial: *const c_char, out: *mut c_char, cap: c_int) -> c_int;
    fn swkbd_prompt_search(initial: *const c_char, out: *mut c_char, cap: c_int) -> c_int;
    fn ruffle_log_cstr(msg: *const c_char);
}

fn log(s: &str) {
    let mut bytes = s.as_bytes().to_vec();
    bytes.push(0);
    unsafe { ruffle_log_cstr(bytes.as_ptr() as *const _) };
}

/// Mozilla CA bundle (ca-bundle.crt copy from /c/devkitPro/msys2/usr/ssl/
/// certs/). 152 root certs as of May 2026. Mosts modern HTTPS servers
/// (archive.org / cloudflare / let's encrypt) verify against this.
const CACERT_PEM: &[u8] = include_bytes!("../../assets/cacert.pem");

/// Called once from `ruffle_library_init`. Writes the embedded CA bundle
/// to `sdmc:/switch/FlashNX/cacert.pem` (idempotent — skips if
/// already present at the right size). libcurl reads from that path via
/// CURLOPT_CAINFO. We can't use CURLOPT_CAINFO_BLOB (added in curl 7.77)
/// because switch-curl is 7.69.
pub fn boot_init() {
    let len = CACERT_PEM.len();
    let rc = unsafe {
        write_cacert_to_sd(CACERT_PEM.as_ptr() as *const c_char, len as c_int)
    };
    if rc != 0 {
        log(&std::format!("net: write_cacert_to_sd rc={} (CA verify may fail)\n", rc));
    }
}

// ── archive.org metadata ───────────────────────────────────────────────

/// One file inside an archive.org item. Only fields we actually use.
#[derive(Debug, Clone)]
pub struct RemoteFile {
    pub name: std::string::String,
    pub size_bytes: u64,
    pub download_url: std::string::String,
}

/// Extract the item-id from an archive.org URL (or treat the input as a
/// bare item-id). Accepts:
///   - https://archive.org/details/<id>
///   - https://archive.org/download/<id>[/<filename>]
///   - <id>                                           (bare)
pub fn extract_item_id(url_or_id: &str) -> Option<std::string::String> {
    let trimmed = url_or_id.trim();
    if !trimmed.contains("://") && !trimmed.contains('/') {
        // Bare item-id.
        if trimmed.is_empty() {
            return None;
        }
        return Some(trimmed.to_string());
    }
    let parts: std::vec::Vec<&str> = trimmed.split('/').collect();
    for (i, p) in parts.iter().enumerate() {
        if (*p == "details" || *p == "download") && i + 1 < parts.len() {
            let id = parts[i + 1];
            if !id.is_empty() {
                return Some(id.to_string());
            }
        }
    }
    None
}

/// Fetch `https://archive.org/metadata/<item_id>` and parse the JSON.
/// Returns the list of `.swf` files in the item, or an error string for
/// the UI to display. **Synchronous**: blocks the worker thread for
/// 1-3 s typical. Metadata responses are small (<64 KB).
pub fn fetch_archive_metadata(
    item_id: &str,
) -> Result<std::vec::Vec<RemoteFile>, std::string::String> {
    let url = std::format!("https://archive.org/metadata/{}", item_id);
    // 4 MB cap for metadata JSON. Big Flash dumps (armorgames ~500 KB,
    // 1100+ files) blow through smaller caps and the C++ side returns -3
    // overflow. 4 MB covers anything realistic and is a transient alloc.
    let mut buf = std::vec![0u8; 4 * 1024 * 1024];
    let mut url_c = url.clone().into_bytes();
    url_c.push(0);
    let n = unsafe {
        https_get_into_buf(
            url_c.as_ptr() as *const c_char,
            buf.as_mut_ptr() as *mut c_char,
            buf.len() as c_int,
        )
    };
    if n == -3 {
        return Err(std::string::String::from(
            "Reponse archive.org trop volumineuse (>4 MB). Item trop massif pour cette version.",
        ));
    }
    if n < 0 {
        return Err(std::format!(
            "Echec HTTPS (code {}). Verifiez le WiFi + l'URL.",
            n
        ));
    }
    buf.truncate(n as usize);
    let json: serde_json::Value = serde_json::from_slice(&buf).map_err(|e| {
        log(&std::format!("net: JSON parse failed: {}\n", e));
        std::format!("JSON archive.org illisible : {}", e)
    })?;

    // archive.org returns `{server: "...", dir: "...", files: [...]}`.
    // Build the per-file download URL as `https://archive.org/download/
    // <item_id>/<filename URL-encoded>` — archive.org redirects to the
    // CDN server. Going through archive.org keeps us URL-stable even if
    // the file moves between mirrors.
    let files_json = json
        .get("files")
        .and_then(|v| v.as_array())
        .ok_or_else(|| std::string::String::from("JSON sans champ \"files\""))?;
    let mut out: std::vec::Vec<RemoteFile> = std::vec::Vec::new();
    for f in files_json {
        let format = f
            .get("format")
            .and_then(|v| v.as_str())
            .unwrap_or("");
        if format != "Shockwave Flash" {
            // Filter early — items can have dozens of files of which only
            // a few are SWFs (thumbnails, torrents, metadata XMLs, ...).
            continue;
        }
        let name = f
            .get("name")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string();
        if name.is_empty() {
            continue;
        }
        let size_bytes = f
            .get("size")
            .and_then(|v| v.as_str())
            .and_then(|s| s.parse::<u64>().ok())
            .unwrap_or(0);
        let download_url = std::format!(
            "https://archive.org/download/{}/{}",
            item_id,
            url_encode_path(&name),
        );
        out.push(RemoteFile {
            name,
            size_bytes,
            download_url,
        });
    }
    log(&std::format!(
        "net: archive.org/{} -> {} .swf file(s)\n",
        item_id,
        out.len()
    ));
    Ok(out)
}

/// Percent-encode characters that aren't URL-safe in a path segment.
/// Keeps ASCII alphanumerics, `.`, `-`, `_`, `~`; everything else
/// becomes %XX (UTF-8 byte-per-byte). Spaces → %20.
fn url_encode_path(s: &str) -> std::string::String {
    let mut out = std::string::String::with_capacity(s.len());
    for &b in s.as_bytes() {
        let safe = matches!(b,
            b'A'..=b'Z' | b'a'..=b'z' | b'0'..=b'9' | b'.' | b'-' | b'_' | b'~');
        if safe {
            out.push(b as char);
        } else {
            out.push_str(&std::format!("%{:02X}", b));
        }
    }
    out
}

// ── Download lifecycle ─────────────────────────────────────────────────

/// Start an async download. The C++ side sets up a curl multi handle;
/// subsequent calls to `download_tick` pump it without blocking.
pub fn start_download(url: &str, out_path: &str) -> Result<(), std::string::String> {
    let mut url_c = url.as_bytes().to_vec();
    url_c.push(0);
    let mut path_c = out_path.as_bytes().to_vec();
    path_c.push(0);
    let rc = unsafe {
        https_download_start(url_c.as_ptr() as *const c_char, path_c.as_ptr() as *const c_char)
    };
    if rc != 0 {
        return Err(std::format!("Impossible de lancer le telechargement (code {}).", rc));
    }
    Ok(())
}

/// Returns:
///   - `Ok(false)` → still in progress
///   - `Ok(true)`  → finished successfully
///   - `Err(msg)`  → failed; the partial output file has been removed
pub fn tick_download() -> Result<bool, std::string::String> {
    let rc = unsafe { https_download_tick() };
    if rc == 0 {
        return Ok(false);
    }
    if rc == 1 {
        return Ok(true);
    }
    Err(std::format!("Telechargement echoue (code {})", rc))
}

/// Current bytes downloaded / total bytes (0 until the Content-Length
/// header arrives).
pub fn download_progress() -> (u64, u64) {
    let mut done = 0u64;
    let mut total = 0u64;
    unsafe { https_download_progress(&mut done as *mut _, &mut total as *mut _) };
    (done, total)
}

pub fn cancel_download() {
    unsafe { https_download_cancel() };
}

// ── swkbd URL input ────────────────────────────────────────────────────

/// Prompt the user for an archive.org URL via libnx's software keyboard.
/// `initial` pre-fills the input field — pass the most recent history
/// URL so the user can edit a neighbouring item-id with a few keystrokes
/// instead of retyping the whole URL. Pass `None` for a default prefix.
/// Synchronous — the keyboard applet takes over the whole screen until
/// the user submits or cancels. Returns None if cancelled.
pub fn prompt_url_with_initial(initial: Option<&str>) -> Option<std::string::String> {
    let mut buf = std::vec![0u8; 1024];
    // Build NUL-terminated initial string (or NULL ptr if None).
    let initial_owned: Option<std::vec::Vec<u8>> = initial.map(|s| {
        let mut v = s.as_bytes().to_vec();
        v.push(0);
        v
    });
    let initial_ptr = initial_owned
        .as_ref()
        .map(|v| v.as_ptr() as *const c_char)
        .unwrap_or(core::ptr::null());
    let rc = unsafe {
        swkbd_prompt_url(initial_ptr, buf.as_mut_ptr() as *mut c_char, buf.len() as c_int)
    };
    if rc != 0 {
        return None;
    }
    let nul = buf.iter().position(|&b| b == 0).unwrap_or(buf.len());
    buf.truncate(nul);
    std::string::String::from_utf8(buf).ok().filter(|s| !s.is_empty())
}

/// Prompt for a new display name (Phase 3.4.bis RENOMMER). `initial` is
/// the current display name pre-filled so a small edit is one keystroke
/// away. Returns the typed text (which may be empty — meaning "revert
/// to basename") on commit, None on cancel.
pub fn prompt_rename(initial: &str) -> Option<std::string::String> {
    let mut buf = std::vec![0u8; 512];
    let mut initial_owned = initial.as_bytes().to_vec();
    initial_owned.push(0);
    let rc = unsafe {
        swkbd_prompt_rename(
            initial_owned.as_ptr() as *const c_char,
            buf.as_mut_ptr() as *mut c_char,
            buf.len() as c_int,
        )
    };
    if rc != 0 {
        return None;
    }
    let nul = buf.iter().position(|&b| b == 0).unwrap_or(buf.len());
    buf.truncate(nul);
    // We allow returning an empty string (caller interprets as "revert
    // to basename"), so don't filter empties here.
    std::string::String::from_utf8(buf).ok()
}

/// Search prompt for filtering the DistantFiles list. `initial` pre-fills
/// the input with the currently-active filter so the user can refine it.
/// Returns the typed text on commit (empty string = clear filter), None
/// on cancel.
pub fn prompt_search(initial: &str) -> Option<std::string::String> {
    let mut buf = std::vec![0u8; 256];
    let mut initial_owned = initial.as_bytes().to_vec();
    initial_owned.push(0);
    let rc = unsafe {
        swkbd_prompt_search(
            initial_owned.as_ptr() as *const c_char,
            buf.as_mut_ptr() as *mut c_char,
            buf.len() as c_int,
        )
    };
    if rc != 0 {
        return None;
    }
    let nul = buf.iter().position(|&b| b == 0).unwrap_or(buf.len());
    buf.truncate(nul);
    std::string::String::from_utf8(buf).ok()
}
