fn main() {
    // Phase 0: nothing to generate yet.
    //
    // Phase 1 will:
    //   1. Read $DEVKITPRO / $LIBNX to find libnx headers.
    //   2. Invoke bindgen with `.use_core()` + `.ctypes_prefix("core::ffi")`
    //      against headers for: hid.h, audio/audren.h, audio/audrv.h,
    //      applet.h, socket.h, runtime/nxlink.h.
    //   3. Emit bindings to OUT_DIR/libnx_sys.rs, included by src/ffi/libnx.rs.
    //
    // Clang must find the devkitA64 sysroot:
    //   -target aarch64-none-elf
    //   --sysroot $DEVKITPRO/devkitA64/aarch64-none-elf
    //   -isystem $DEVKITPRO/libnx/include
    //
    // On Windows, $LIBCLANG_PATH must point at LLVM's bin/ for libclang.dll.
}
