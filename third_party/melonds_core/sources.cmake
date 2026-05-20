# =========================================================
# melonds_core sources.cmake
#
# 维护 melonDS 核心静态库的源文件列表。
# 只包含模拟器核心（CPU/GPU/SPU/NDS/ARM/NDSCart等），
# 排除所有 frontend（Qt/SDL/Switch/Windows/macOS）。
# =========================================================

set(MELONDS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../melonDS/src)

set(MELONDS_CORE_SOURCES
    # --- NDS Core ---
    ${MELONDS_SRC_DIR}/NDS.cpp
    ${MELONDS_SRC_DIR}/NDSCart.cpp
    ${MELONDS_SRC_DIR}/NDSCart_SRAMManager.cpp

    # --- CPU ---
    ${MELONDS_SRC_DIR}/ARM.cpp
    ${MELONDS_SRC_DIR}/ARMInterpreter.cpp
    ${MELONDS_SRC_DIR}/ARMInterpreter_ALU.cpp
    ${MELONDS_SRC_DIR}/ARMInterpreter_Branch.cpp
    ${MELONDS_SRC_DIR}/ARMInterpreter_LoadStore.cpp
    ${MELONDS_SRC_DIR}/CP15.cpp

    # --- GPU ---
    ${MELONDS_SRC_DIR}/GPU.cpp
    ${MELONDS_SRC_DIR}/GPU2D.cpp
    ${MELONDS_SRC_DIR}/GPU2D_Soft.cpp
    ${MELONDS_SRC_DIR}/GPU3D.cpp
    ${MELONDS_SRC_DIR}/GPU3D_Transform.cpp
    ${MELONDS_SRC_DIR}/GPU3D_Soft.cpp

    # --- Audio ---
    ${MELONDS_SRC_DIR}/SPU.cpp

    # --- Peripherals ---
    ${MELONDS_SRC_DIR}/RTC.cpp
    ${MELONDS_SRC_DIR}/SPI.cpp
    ${MELONDS_SRC_DIR}/DMA.cpp
    ${MELONDS_SRC_DIR}/FIFO.h

    # --- Save/Config ---
    ${MELONDS_SRC_DIR}/Savestate.cpp
    ${MELONDS_SRC_DIR}/Config.cpp

    # --- GBA Slot ---
    ${MELONDS_SRC_DIR}/GBACart.cpp

    # --- WiFi ---
    ${MELONDS_SRC_DIR}/Wifi.cpp
    ${MELONDS_SRC_DIR}/WifiAP.cpp

    # --- Action Replay ---
    ${MELONDS_SRC_DIR}/AREngine.cpp
    ${MELONDS_SRC_DIR}/ARCodeFile.cpp

    # --- Utilities ---
    ${MELONDS_SRC_DIR}/CRC32.cpp

    # --- Crypto/Compression ---
    ${MELONDS_SRC_DIR}/tiny-AES-c/aes.c
    ${MELONDS_SRC_DIR}/xxhash/xxhash.c

    # --- DSi (conditionally excluded below) ---
    ${MELONDS_SRC_DIR}/DSi.cpp
    ${MELONDS_SRC_DIR}/DSi_AES.cpp
    ${MELONDS_SRC_DIR}/DSi_Camera.cpp
    ${MELONDS_SRC_DIR}/DSi_DSP.cpp
    ${MELONDS_SRC_DIR}/DSi_I2C.cpp
    ${MELONDS_SRC_DIR}/DSi_NDMA.cpp
    ${MELONDS_SRC_DIR}/DSi_NWifi.cpp
    ${MELONDS_SRC_DIR}/DSi_SD.cpp
    ${MELONDS_SRC_DIR}/DSi_SPI_TSC.cpp

    # --- Header-only sources (listed for IDE awareness) ---
    ${MELONDS_SRC_DIR}/types.h
    ${MELONDS_SRC_DIR}/version.h
    ${MELONDS_SRC_DIR}/Platform.h
    ${MELONDS_SRC_DIR}/ARM_InstrTable.h
    ${MELONDS_SRC_DIR}/melonDLDI.h
    ${MELONDS_SRC_DIR}/ROMList.h
)

set(MELONDS_CORE_HEADERS
    ${MELONDS_SRC_DIR}/NDS.h
    ${MELONDS_SRC_DIR}/NDS_Header.h
    ${MELONDS_SRC_DIR}/NDSCart.h
    ${MELONDS_SRC_DIR}/NDSCart_SRAMManager.h
    ${MELONDS_SRC_DIR}/ARM.h
    ${MELONDS_SRC_DIR}/ARMInterpreter.h
    ${MELONDS_SRC_DIR}/ARMInterpreter_ALU.h
    ${MELONDS_SRC_DIR}/ARMInterpreter_Branch.h
    ${MELONDS_SRC_DIR}/ARMInterpreter_LoadStore.h
    ${MELONDS_SRC_DIR}/GPU.h
    ${MELONDS_SRC_DIR}/GPU2D.h
    ${MELONDS_SRC_DIR}/GPU2D_Soft.h
    ${MELONDS_SRC_DIR}/GPU3D.h
    ${MELONDS_SRC_DIR}/GPU3D_Soft.h
    ${MELONDS_SRC_DIR}/SPU.h
    ${MELONDS_SRC_DIR}/RTC.h
    ${MELONDS_SRC_DIR}/SPI.h
    ${MELONDS_SRC_DIR}/DMA.h
    ${MELONDS_SRC_DIR}/FIFO.h
    ${MELONDS_SRC_DIR}/Savestate.h
    ${MELONDS_SRC_DIR}/Config.h
    ${MELONDS_SRC_DIR}/GBACart.h
    ${MELONDS_SRC_DIR}/Wifi.h
    ${MELONDS_SRC_DIR}/WifiAP.h
    ${MELONDS_SRC_DIR}/AREngine.h
    ${MELONDS_SRC_DIR}/ARCodeFile.h
    ${MELONDS_SRC_DIR}/CRC32.h
    ${MELONDS_SRC_DIR}/types.h
    ${MELONDS_SRC_DIR}/version.h
    ${MELONDS_SRC_DIR}/Platform.h
    ${MELONDS_SRC_DIR}/DSi.h
    ${MELONDS_SRC_DIR}/DSi_AES.h
    ${MELONDS_SRC_DIR}/DSi_Camera.h
    ${MELONDS_SRC_DIR}/DSi_DSP.h
    ${MELONDS_SRC_DIR}/DSi_I2C.h
    ${MELONDS_SRC_DIR}/DSi_NDMA.h
    ${MELONDS_SRC_DIR}/DSi_NWifi.h
    ${MELONDS_SRC_DIR}/DSi_SD.h
    ${MELONDS_SRC_DIR}/DSi_SPI_TSC.h
    ${MELONDS_SRC_DIR}/DSiCrypto.h
    ${MELONDS_SRC_DIR}/FreeBIOS.h
    ${MELONDS_SRC_DIR}/SPI_Firmware.h
    ${MELONDS_SRC_DIR}/ROMList.h
    ${MELONDS_SRC_DIR}/NonStupidBitfield.h
    ${MELONDS_SRC_DIR}/GPU2D_Deko.h
    ${MELONDS_SRC_DIR}/GPU3D_Deko.h
    ${MELONDS_SRC_DIR}/MemConstants.h
    ${MELONDS_SRC_DIR}/OpenGLSupport.h
    ${MELONDS_SRC_DIR}/GPU_OpenGL.h
    ${MELONDS_SRC_DIR}/GPU_OpenGL_shaders.h
    ${MELONDS_SRC_DIR}/GPU3D_OpenGL.h
    ${MELONDS_SRC_DIR}/GPU3D_OpenGL_shaders.h
    ${MELONDS_SRC_DIR}/melonDLDI.h
)

# --- JIT sources (x86_64) ---
set(MELONDS_CORE_JIT_X64_SOURCES
    ${MELONDS_SRC_DIR}/ARM_InstrInfo.cpp
    ${MELONDS_SRC_DIR}/ARMJIT.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_Memory.cpp
    ${MELONDS_SRC_DIR}/dolphin/CommonFuncs.cpp
    ${MELONDS_SRC_DIR}/dolphin/x64ABI.cpp
    ${MELONDS_SRC_DIR}/dolphin/x64CPUDetect.cpp
    ${MELONDS_SRC_DIR}/dolphin/x64Emitter.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_x64/ARMJIT_Compiler.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_x64/ARMJIT_ALU.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_x64/ARMJIT_LoadStore.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_x64/ARMJIT_Branch.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_x64/ARMJIT_Linkage.S
)

set(MELONDS_CORE_JIT_X64_HEADERS
    ${MELONDS_SRC_DIR}/ARM_InstrInfo.h
    ${MELONDS_SRC_DIR}/ARMJIT.h
    ${MELONDS_SRC_DIR}/ARMJIT_Memory.h
    ${MELONDS_SRC_DIR}/ARMJIT_Compiler.h
    ${MELONDS_SRC_DIR}/ARMJIT_Internal.h
    ${MELONDS_SRC_DIR}/ARMJIT_RegisterCache.h
    ${MELONDS_SRC_DIR}/dolphin/BitUtils.h
    ${MELONDS_SRC_DIR}/dolphin/BitSet.h
    ${MELONDS_SRC_DIR}/dolphin/CommonFuncs.h
    ${MELONDS_SRC_DIR}/dolphin/Compat.h
    ${MELONDS_SRC_DIR}/dolphin/CPUDetect.h
    ${MELONDS_SRC_DIR}/dolphin/MathUtil.h
    ${MELONDS_SRC_DIR}/dolphin/x64ABI.h
    ${MELONDS_SRC_DIR}/dolphin/x64Emitter.h
    ${MELONDS_SRC_DIR}/dolphin/x64Reg.h
    ${MELONDS_SRC_DIR}/dolphin/Align.h
    ${MELONDS_SRC_DIR}/ARMJIT_x64/ARMJIT_Compiler.h
    ${MELONDS_SRC_DIR}/ARMJIT_x64/ARMJIT_Offsets.h
)

# --- JIT sources (ARM64) ---
set(MELONDS_CORE_JIT_A64_SOURCES
    ${MELONDS_SRC_DIR}/ARM_InstrInfo.cpp
    ${MELONDS_SRC_DIR}/ARMJIT.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_Memory.cpp
    ${MELONDS_SRC_DIR}/dolphin/CommonFuncs.cpp
    ${MELONDS_SRC_DIR}/dolphin/Arm64Emitter.cpp
    ${MELONDS_SRC_DIR}/dolphin/MathUtil.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_Compiler.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_ALU.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_LoadStore.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_Branch.cpp
    ${MELONDS_SRC_DIR}/ARMJIT_A64/ARMJIT_Linkage.S
)

# --- OpenGL Renderer sources ---
set(MELONDS_CORE_OGL_SOURCES
    ${MELONDS_SRC_DIR}/GPU_OpenGL.cpp
    ${MELONDS_SRC_DIR}/GPU3D_OpenGL.cpp
    ${MELONDS_SRC_DIR}/OpenGLSupport.cpp
)

# --- Deko3D GPU sources ---
set(MELONDS_CORE_DEKO_SOURCES
    ${MELONDS_SRC_DIR}/GPU2D_Deko.cpp
    ${MELONDS_SRC_DIR}/GPU3D_Deko.cpp
)
