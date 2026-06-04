#pragma once

#include <cstdint>

static constexpr int VIEWPORT_W = 1280;
static constexpr int VIEWPORT_H = 720;

static constexpr int SK_NONE      = 0;
static constexpr int SK_SPACE     = 1;
static constexpr int SK_ENTER     = 2;
static constexpr int SK_ESCAPE    = 3;
static constexpr int SK_LEFT      = 4;
static constexpr int SK_RIGHT     = 5;
static constexpr int SK_UP        = 6;
static constexpr int SK_DOWN      = 7;
static constexpr int SK_Z         = 8;
static constexpr int SK_X         = 9;
static constexpr int SK_SHIFT     = 10;
static constexpr int SK_P         = 11;
static constexpr int SK_A         = 12;
static constexpr int SK_B         = 13;
static constexpr int SK_C         = 14;
static constexpr int SK_D         = 15;
static constexpr int SK_E         = 16;
static constexpr int SK_F         = 17;
static constexpr int SK_G         = 18;
static constexpr int SK_H         = 19;
static constexpr int SK_I         = 20;
static constexpr int SK_J         = 21;
static constexpr int SK_K         = 22;
static constexpr int SK_L         = 23;
static constexpr int SK_M         = 24;
static constexpr int SK_N         = 25;
static constexpr int SK_O         = 26;
static constexpr int SK_Q         = 27;
static constexpr int SK_R         = 28;
static constexpr int SK_S         = 29;
static constexpr int SK_T         = 30;
static constexpr int SK_U         = 31;
static constexpr int SK_V         = 32;
static constexpr int SK_W         = 33;
static constexpr int SK_Y         = 34;
static constexpr int SK_0         = 35;
static constexpr int SK_1         = 36;
static constexpr int SK_2         = 37;
static constexpr int SK_3         = 38;
static constexpr int SK_4         = 39;
static constexpr int SK_5         = 40;
static constexpr int SK_6         = 41;
static constexpr int SK_7         = 42;
static constexpr int SK_8         = 43;
static constexpr int SK_9         = 44;
static constexpr int SK_TAB       = 45;
static constexpr int SK_BACKSPACE = 46;
static constexpr int SK_CONTROL   = 47;
static constexpr int SK_ALT       = 48;

extern "C" {

int ruffle_set_swf_path(const char* path);

int ruffle_init(void);

void ruffle_render_frame(void);

void ruffle_render_frame_dt(uint64_t dt_us);

void ruffle_redraw_paused(void);

void ruffle_draw_menu(int selected);

int ruffle_restart(void);

void ruffle_handle_key(int code, bool down);

void ruffle_handle_mouse_move(int x, int y);

void ruffle_handle_mouse_button(bool down);

void ruffle_shutdown(void);

void ruffle_library_init(void);
void ruffle_library_add_path(const char* path);
void ruffle_library_open(void);
int  ruffle_library_active(void);
int  ruffle_library_picked(void);
int  ruffle_library_input(const char* button_name);
void ruffle_library_render(void);
int  ruffle_library_selected_path(char* out, int cap);
void ruffle_library_shutdown(void);
void ruffle_library_reset(void);

int ruffle_keymap_lookup(const char* name);

void ruffle_audio_fill_buffer(int16_t* out, unsigned long long len);

int  ruffle_query_ram(uint64_t* used_out, uint64_t* total_out);
uint64_t ruffle_tick_now(void);
uint64_t ruffle_tick_freq(void);
void ruffle_crash_dump(const char* msg);

}
