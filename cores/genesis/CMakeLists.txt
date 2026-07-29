set(GENESIS_PLUS_GX_DIR ${CMAKE_SOURCE_DIR}/third_party/Genesis-Plus-GX)

set(GENESIS_PLUS_GX_SOURCES
    ${GENESIS_PLUS_GX_DIR}/core/genesis.c
    ${GENESIS_PLUS_GX_DIR}/core/io_ctrl.c
    ${GENESIS_PLUS_GX_DIR}/core/loadrom.c
    ${GENESIS_PLUS_GX_DIR}/core/mem68k.c
    ${GENESIS_PLUS_GX_DIR}/core/membnk.c
    ${GENESIS_PLUS_GX_DIR}/core/memz80.c
    ${GENESIS_PLUS_GX_DIR}/core/state.c
    ${GENESIS_PLUS_GX_DIR}/core/system.c
    ${GENESIS_PLUS_GX_DIR}/core/vdp_ctrl.c
    ${GENESIS_PLUS_GX_DIR}/core/vdp_render.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/areplay.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/eeprom_93c.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/eeprom_i2c.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/eeprom_spi.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/flash_cfi.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/ggenie.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/md_cart.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/megasd.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/sms_cart.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/sram.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/yx5200.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/svp/ssp16.c
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/svp/svp.c
    ${GENESIS_PLUS_GX_DIR}/core/cd_hw/cd_cart.c
    ${GENESIS_PLUS_GX_DIR}/core/cd_hw/cdc.c
    ${GENESIS_PLUS_GX_DIR}/core/cd_hw/cdd.c
    ${GENESIS_PLUS_GX_DIR}/core/cd_hw/gfx.c
    ${GENESIS_PLUS_GX_DIR}/core/cd_hw/pcm.c
    ${GENESIS_PLUS_GX_DIR}/core/cd_hw/scd.c
    ${GENESIS_PLUS_GX_DIR}/core/input_hw/activator.c
    ${GENESIS_PLUS_GX_DIR}/core/input_hw/gamepad.c
    ${GENESIS_PLUS_GX_DIR}/core/input_hw/graphic_board.c
    ${GENESIS_PLUS_GX_DIR}/core/input_hw/input.c
    ${GENESIS_PLUS_GX_DIR}/core/input_hw/lightgun.c
    ${GENESIS_PLUS_GX_DIR}/core/input_hw/mouse.c
    ${GENESIS_PLUS_GX_DIR}/core/input_hw/paddle.c
    ${GENESIS_PLUS_GX_DIR}/core/input_hw/sportspad.c
    ${GENESIS_PLUS_GX_DIR}/core/input_hw/teamplayer.c
    ${GENESIS_PLUS_GX_DIR}/core/input_hw/terebi_oekaki.c
    ${GENESIS_PLUS_GX_DIR}/core/input_hw/xe_1ap.c
    ${GENESIS_PLUS_GX_DIR}/core/m68k/m68kcpu.c
    ${GENESIS_PLUS_GX_DIR}/core/m68k/s68kcpu.c
    ${GENESIS_PLUS_GX_DIR}/core/ntsc/md_ntsc.c
    ${GENESIS_PLUS_GX_DIR}/core/ntsc/sms_ntsc.c
    ${GENESIS_PLUS_GX_DIR}/core/sound/blip_buf.c
    ${GENESIS_PLUS_GX_DIR}/core/sound/eq.c
    ${GENESIS_PLUS_GX_DIR}/core/sound/opll.c
    ${GENESIS_PLUS_GX_DIR}/core/sound/psg.c
    ${GENESIS_PLUS_GX_DIR}/core/sound/sound.c
    ${GENESIS_PLUS_GX_DIR}/core/sound/ym2413.c
    ${GENESIS_PLUS_GX_DIR}/core/sound/ym2612.c
    ${GENESIS_PLUS_GX_DIR}/core/sound/ym3438.c
    ${GENESIS_PLUS_GX_DIR}/core/z80/z80.c
)

add_library(genesis_plus_gx_core STATIC ${GENESIS_PLUS_GX_SOURCES})
target_include_directories(genesis_plus_gx_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GENESIS_PLUS_GX_DIR}/core
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/svp
    ${GENESIS_PLUS_GX_DIR}/core/cd_hw
    ${GENESIS_PLUS_GX_DIR}/core/input_hw
    ${GENESIS_PLUS_GX_DIR}/core/m68k
    ${GENESIS_PLUS_GX_DIR}/core/ntsc
    ${GENESIS_PLUS_GX_DIR}/core/sound
    ${GENESIS_PLUS_GX_DIR}/core/sound/minimp3
    ${GENESIS_PLUS_GX_DIR}/core/z80
)
target_compile_definitions(genesis_plus_gx_core PRIVATE
    USE_16BPP_RENDERING
    LSB_FIRST
    blip_new=gpgx_blip_new
    blip_set_rates=gpgx_blip_set_rates
    blip_clear=gpgx_blip_clear
    blip_add_delta=gpgx_blip_add_delta
    blip_add_delta_fast=gpgx_blip_add_delta_fast
    blip_clocks_needed=gpgx_blip_clocks_needed
    blip_end_frame=gpgx_blip_end_frame
    blip_samples_avail=gpgx_blip_samples_avail
    blip_discard_samples_dirty=gpgx_blip_discard_samples_dirty
    blip_read_samples=gpgx_blip_read_samples
    blip_mix_samples=gpgx_blip_mix_samples
    blip_delete=gpgx_blip_delete
    blip_save_buffer_state=gpgx_blip_save_buffer_state
    blip_load_buffer_state=gpgx_blip_load_buffer_state
    blip_new_buffer_state=gpgx_blip_new_buffer_state
    blip_delete_buffer_state=gpgx_blip_delete_buffer_state
)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(genesis_plus_gx_core PRIVATE -fgnu89-inline)
endif()
set_target_properties(genesis_plus_gx_core PROPERTIES C_STANDARD 11)

add_library(genesis_core STATIC
    GenesisCore.cpp
    GenesisBridge.c
)
target_include_directories(genesis_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)
target_include_directories(genesis_core PRIVATE
    ${GENESIS_PLUS_GX_DIR}/core
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw
    ${GENESIS_PLUS_GX_DIR}/core/cart_hw/svp
    ${GENESIS_PLUS_GX_DIR}/core/cd_hw
    ${GENESIS_PLUS_GX_DIR}/core/input_hw
    ${GENESIS_PLUS_GX_DIR}/core/m68k
    ${GENESIS_PLUS_GX_DIR}/core/ntsc
    ${GENESIS_PLUS_GX_DIR}/core/sound
    ${GENESIS_PLUS_GX_DIR}/core/sound/minimp3
    ${GENESIS_PLUS_GX_DIR}/core/z80
)
target_compile_definitions(genesis_core PRIVATE
    USE_16BPP_RENDERING
    LSB_FIRST
)
target_link_libraries(genesis_core PUBLIC genesis_plus_gx_core borealis)
set_target_properties(genesis_core PROPERTIES C_STANDARD 11 CXX_STANDARD 17)
