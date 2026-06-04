//! NavigatorBackend impl — minimal no-op for local .swf playback.
//!
//! Mario 63 calls `loadPolicyFile("http://...")` but tolerates failure, so
//! the simplest viable impl rejects all network requests and forwards local
//! file: URLs to StorageBackend.
