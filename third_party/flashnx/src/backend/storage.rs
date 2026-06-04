//! StorageBackend impl — persists Flash SharedObject data on the SD card.
//!
//! **Layout (Phase 3.4 flat / 2026-05-26 nuit revision)**:
//! Saves live FLAT alongside the `.swf` they belong to, keyed by basename:
//!
//!     sdmc:/flashnx/<swf_basename>.<sol_name>.sol
//!
//! Mirrors the `.keymap.json` / `.meta.json` sidecar convention. Easier to
//! manage manually (one folder, no nested host/movie dirs) and matches
//! what a user expects when they open the SD card.
//!
//! **Backward compatibility**: the legacy nested layout
//! `<base>/saves/<host>/<swf_basename>/<sol_name>.sol` is still read as a
//! fallback. Writes go ONLY to the new flat path; the legacy file stays
//! in place until the user manually cleans up. This keeps existing saves
//! working without forcing a one-shot migration that could fail mid-walk.
//!
//! libnx's crt0 mounts `sdmc:/` automatically, so plain `std::fs` calls
//! work. AMF format is cross-platform — users can drop in `.sol` files
//! from desktop Ruffle / Flash Player `%APPDATA%` and they just work.

use std::fs;
use std::fs::File;
use std::io::{Read, Write};
use std::path::{Component, Path, PathBuf};

use ruffle_core::backend::storage::StorageBackend;

pub struct SwitchStorageBackend {
    /// Flat root where new saves are written, e.g. `sdmc:/flashnx/`.
    flat_root: PathBuf,
    /// Legacy nested root checked on `get` only. e.g. `sdmc:/ruffle/saves/`.
    /// `None` if no legacy data exists on this device — skips the
    /// fallback check entirely. Cheap to set up: we just test `exists()`
    /// once at construction.
    legacy_root: Option<PathBuf>,
}

impl SwitchStorageBackend {
    /// `flat_root` = base dir where new saves go (typically
    /// `sdmc:/flashnx/`). `legacy_root` = old nested-tree base
    /// (typically `sdmc:/ruffle/saves/` or `sdmc:/flashnx/saves/`); pass
    /// the path even if it doesn't exist — we test and remember.
    pub fn new(flat_root: PathBuf, legacy_root: PathBuf) -> Self {
        if !flat_root.exists() {
            if let Err(e) = fs::create_dir_all(&flat_root) {
                tracing::warn!(
                    "SwitchStorageBackend: failed to create {}: {}",
                    flat_root.display(),
                    e
                );
            }
        }
        let legacy_root = if legacy_root.exists() {
            Some(legacy_root)
        } else {
            None
        };
        Self {
            flat_root,
            legacy_root,
        }
    }

    fn is_path_allowed(path: &Path) -> bool {
        path.components().all(|c| c != Component::ParentDir)
    }

    /// Split Ruffle's SharedObject `name` (e.g.
    /// "flashforswitch.local/Super_Mario_63_2010.swf/marionowe") into
    /// (basename, sol_name) by `/`. The last component is the SO name;
    /// the penultimate is the SWF basename. If there's only one
    /// component, basename is None (we don't expect this in practice,
    /// but we handle it).
    fn split_name(name: &str) -> (Option<&str>, &str) {
        let parts: std::vec::Vec<&str> = name.split('/').collect();
        match parts.len() {
            0 => (None, name),
            1 => (None, parts[0]),
            _ => (Some(parts[parts.len() - 2]), parts[parts.len() - 1]),
        }
    }

    /// New flat path: `<flat_root>/<basename>.<sol_name>.sol`. If
    /// basename is None we fall back to `<flat_root>/<sol_name>.sol`.
    fn flat_path(&self, name: &str) -> PathBuf {
        let (basename, sol_name) = Self::split_name(name);
        let filename = match basename {
            Some(b) => std::format!("{}.{}.sol", b, sol_name),
            None => std::format!("{}.sol", sol_name),
        };
        self.flat_root.join(filename)
    }

    /// Legacy nested path:
    /// `<legacy_root>/<host>/<basename>/<sol_name>.sol` (mirrors
    /// `<legacy_root>/<full_name>.sol` since `name` already contains
    /// the slashes).
    fn legacy_path(&self, name: &str) -> Option<PathBuf> {
        self.legacy_root
            .as_ref()
            .map(|root| root.join(std::format!("{name}.sol")))
    }

    fn read_chunked(path: &Path) -> Option<std::vec::Vec<u8>> {
        // 4 KB chunked read — avoids the Horizon newlib ENOMEM @ 32+ KB
        // bug that bites `std::fs::read` / `read_to_end`'s default
        // growth step. Same workaround as in keymap.rs.
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
        Some(data)
    }
}

impl StorageBackend for SwitchStorageBackend {
    fn get(&self, name: &str) -> Option<Vec<u8>> {
        // Try the new flat layout first.
        let flat = self.flat_path(name);
        if !Self::is_path_allowed(&flat) {
            tracing::warn!("storage.get({}) path not allowed", name);
            return None;
        }
        if let Some(data) = Self::read_chunked(&flat) {
            tracing::info!(
                "storage.get({}) HIT flat path={} {}B",
                name,
                flat.display(),
                data.len()
            );
            return Some(data);
        }
        // Fall back to the legacy nested layout. Helps users who already
        // have saves from before the flat-layout refactor.
        if let Some(legacy) = self.legacy_path(name) {
            if !Self::is_path_allowed(&legacy) {
                return None;
            }
            if let Some(data) = Self::read_chunked(&legacy) {
                tracing::info!(
                    "storage.get({}) HIT legacy path={} {}B (next put will move to flat)",
                    name,
                    legacy.display(),
                    data.len()
                );
                return Some(data);
            }
        }
        tracing::info!("storage.get({}) MISS flat={}", name, flat.display());
        None
    }

    fn put(&mut self, name: &str, value: &[u8]) -> bool {
        let path = self.flat_path(name);
        if !Self::is_path_allowed(&path) {
            tracing::warn!("storage.put({}) path not allowed", name);
            return false;
        }
        if let Some(parent) = path.parent() {
            if !parent.exists() {
                if let Err(e) = fs::create_dir_all(parent) {
                    tracing::warn!("storage.put: mkdir failed: {}", e);
                    return false;
                }
            }
        }
        match File::create(&path) {
            Ok(mut f) => match f.write_all(value) {
                Ok(()) => {
                    tracing::info!(
                        "storage.put({}) OK path={} {}B",
                        name,
                        path.display(),
                        value.len()
                    );
                    true
                }
                Err(e) => {
                    tracing::warn!("storage.put({}) write failed: {}", name, e);
                    false
                }
            },
            Err(e) => {
                tracing::warn!("storage.put({}) create failed: {}", name, e);
                false
            }
        }
    }

    fn remove_key(&mut self, name: &str) {
        // Remove both the new flat path AND the legacy nested path so a
        // delete really wipes the save (otherwise the legacy read-
        // fallback would resurrect a "removed" save next session).
        let flat = self.flat_path(name);
        if Self::is_path_allowed(&flat) {
            let _ = fs::remove_file(&flat);
        }
        if let Some(legacy) = self.legacy_path(name) {
            if Self::is_path_allowed(&legacy) {
                let _ = fs::remove_file(&legacy);
            }
        }
    }
}
