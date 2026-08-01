#include "libretro.h"
#include "libretro_core_options.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>

#include <streams/file_stream.h>
#include <file/file_path.h>
#include <compat/strl.h>
#include <retro_miscellaneous.h>

#include "../source/core/api/NstApiMachine.hpp"
#include "../source/core/api/NstApiEmulator.hpp"
#include "../source/core/api/NstApiVideo.hpp"
#include "../source/core/api/NstApiCheats.hpp"
#include "../source/core/api/NstApiSound.hpp"
#include "../source/core/api/NstApiInput.hpp"
#include "../source/core/api/NstApiCartridge.hpp"
#include "../source/core/api/NstApiUser.hpp"
#include "../source/core/api/NstApiFds.hpp"

#include "../source/core/NstMachine.hpp"

#include "nstdatabase.hpp"
#include "palettes.hpp"

#define NST_VERSION "1.53.2"

#define NES_NTSC_PAR ((Api::Video::Output::WIDTH - (overscan_h_left + overscan_h_right)) * (8.0 / 7.0)) / (Api::Video::Output::HEIGHT - (overscan_v_top + overscan_v_bottom))
#define NES_PAL_PAR ((Api::Video::Output::WIDTH - (overscan_h_left + overscan_h_right)) * (2950000.0 / 2128137.0)) / (Api::Video::Output::HEIGHT - (overscan_v_top + overscan_v_bottom))
#define NES_4_3_DAR (4.0 / 3.0);
#define SAMPLERATE 48000

#define RETRO_DEVICE_AUTO RETRO_DEVICE_JOYPAD
#define RETRO_DEVICE_GAMEPAD RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)
#define RETRO_DEVICE_ARKANOID RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_MOUSE, 0)
#define RETRO_DEVICE_ZAPPER RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_POINTER, 0)

using namespace Nes;

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

#ifdef _3DS
extern "C" void* linearMemAlign(size_t size, size_t alignment);
extern "C" void linearFree(void* mem);
#endif
static uint32_t* video_buffer = NULL;

static int16_t audio_buffer[(SAMPLERATE / 50) << 1];
static int16_t audio_stereo_buffer[(SAMPLERATE / 50) << 1];
static Api::Emulator emulator;
static Api::Machine *machine;
static Api::Fds *fds;
static char g_basename[256];
static char g_rom_dir[256];
static char *g_save_dir;
static char samp_dir[PATH_MAX_LENGTH];
static unsigned blargg_ntsc;
static bool fds_auto_insert;
static int arkanoid_paddle_min = 0;
static int arkanoid_paddle_max = 255;
static int overscan_v_top, overscan_v_bottom;
static int overscan_h_left, overscan_h_right;
static bool libretro_supports_option_categories = false;
static unsigned aspect_ratio_mode;
static unsigned tpulse; // A/B Button turbo pulse width in frames

static unsigned char tstate[4] = { 2, 2, 2, 2 }; // A/B Button turbo pulse width counter 0 => lo, !0 => hi, in range [0, tpulse]
static int cur_x = 0; // Absolute x coordinate of zapper/arkanoid in pixels
static int cur_y = 0; // Absolute y coordinate of zapper          in pixels
static unsigned char prevL = false; // => L Button is held; controls famicon disc drive
static unsigned char prevR = false; // => R Button is held; controls famicon disc drive
static const int tracked_input_state_size_bytes = 12; // 8 tracked-input bytes + 4 bytes of audio pacing accumulator
static size_t state_size = 0;

static enum {
   SHOW_CROSSHAIR_DISABLED,
   SHOW_CROSSHAIR_OFF,
   SHOW_CROSSHAIR_ON,
} show_crosshair;

static unsigned libretro_msg_interface_version = 0;
static bool libretro_supports_bitmasks = false;
static bool show_advanced_av_settings = true;

int16_t video_width = Api::Video::Output::WIDTH;
size_t pitch;

static Api::Video::Output *video;
static Api::Sound::Output *audio;
static Api::Input::Controllers *input;
static Api::Machine::FavoredSystem favsystem;

static void *sram;
static unsigned long sram_size;
static bool is_pal;
static byte custpal[64*3];

/* Exact audio pacing.  The APU synthesizes SAMPLERATE samples per
 * emulated second of master-clock time, so the true number of samples
 * per video frame is
 *
 *    SAMPLERATE * master_ticks_per_frame / master_clock
 *
 * (798.68... for NTSC, 959.87... for PAL/Dendy), not SAMPLERATE/60 or
 * SAMPLERATE/50.  Requesting the rounded-up integer every frame makes
 * Apu::FlushSound() pad the difference by repeating the instantaneous
 * sample without advancing synthesis time (measured with an
 * instrumented build: 79 padded samples per 60 NTSC frames, 8 per 60
 * PAL frames).  Track the exact rational with a remainder accumulator
 * instead and request 798/799 (959/960) so generation and consumption
 * stay in lock step. */
static unsigned audio_spf_base;  /* whole samples per frame            */
static unsigned long audio_spf_rem;  /* numerator of fractional part   */
static unsigned long audio_spf_den;  /* denominator                    */
static unsigned long audio_frac;     /* running remainder accumulator  */

static unsigned long gcd_ul(unsigned long a, unsigned long b)
{
   while (b)
   {
      unsigned long t = a % b;
      a = b;
      b = t;
   }
   return a;
}

static void update_audio_timing(void)
{
   /* PAL and Dendy share the frame clock (PPU_DENDY_HVSYNC equals
    * PPU_RP2C07_HVSYNC). */
   unsigned long clk  = is_pal ? (unsigned long)Core::CLK_PAL
                               : (unsigned long)Core::CLK_NTSC;
   unsigned long tick = is_pal
      ? Core::CLK_PAL_DIV  * (unsigned long)Core::PPU_RP2C07_HVSYNC
      : Core::CLK_NTSC_DIV * (unsigned long)Core::PPU_RP2C02_HVSYNC;
   /* Reduce SAMPLERATE/clk before multiplying so everything fits in
    * 32 bits: 48000 and both master clocks share a large factor
    * (worst case after reduction is 160 * 4255680 < 2^31). */
   unsigned long g    = gcd_ul(SAMPLERATE, clk);
   unsigned long rn   = SAMPLERATE / g;
   unsigned long rd   = clk / g;

   audio_spf_base = (unsigned)(rn * tick / rd);
   audio_spf_rem  = rn * tick % rd;
   audio_spf_den  = rd;
   audio_frac     = 0;
}


static enum {
   FDS_SAVEFILE_SAV_UPS = 0,
   FDS_SAVEFILE_UPS,
   FDS_SAVEFILE_IPS,
} fds_savefile_format;
static bool fds_sav_extension;
static bool fds_ups_extension;
static bool fds_ips_extension;
static bool fds_patch_format_ups;
static bool fds_patch_format_ips;

int crossx = 0;
int crossy = 0;

#define CROSSHAIR_SIZE 3

void draw_crosshair(int x, int y)
{
   uint32_t w = 0xFFFFFFFF;
   uint32_t b = 0x00000000;
   int current_width = 256;
   
   if (blargg_ntsc){
      x *= 2.36;
      current_width = 602;
   }

   for (int i = MAX(-CROSSHAIR_SIZE, -x); i <= MIN(CROSSHAIR_SIZE, current_width - x); i++) {
     video_buffer[current_width * y + x + i] = i % 2 == 0 ? w : b;
   }

   for (int i = MAX(-CROSSHAIR_SIZE, -y); i <= MIN(CROSSHAIR_SIZE, 239 - y); i++) {
     video_buffer[current_width * (y + i) + x] = i % 2 == 0 ? w : b;
   }
}

static void load_wav(const char* sampgame, Api::User::File& file)
{
   char game_dir[PATH_MAX_LENGTH];
   char samp_path[PATH_MAX_LENGTH];
   char samp_name[16];
   int64_t length = 0;
   int blockalign = 0;
   int numchannels = 0;
   int bitspersample = 0;
   char fmt[4] = { 0x66, 0x6d, 0x74, 0x20};
   char subchunk2id[4] = { 0x64, 0x61, 0x74, 0x61};
   char *wavfile = NULL;
   char *dataptr;

   fill_pathname_join_special(game_dir, samp_dir, sampgame, sizeof(game_dir));
   /* sprintf rather than snprintf: newlib hides the C99 snprintf
    * declaration under -std=c++98 (psl1ght).  Bounded by construction:
    * "%02u.wav" of a 32-bit unsigned is at most 10 + 4 characters,
    * 15 bytes with the terminator, and samp_name holds 16. */
   sprintf(samp_name, "%02u.wav", file.GetId());
   fill_pathname_join_special(samp_path, game_dir, samp_name, sizeof(samp_path));
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "samp_path: %s\n", samp_path);

   if (!filestream_read_file(samp_path, (void**)&wavfile, &length))
      return;

   /* Smallest valid file: 44 byte canonical header plus sample data.
    * Check to see if it has a valid header */
   if (length > 44 &&
         memcmp(&wavfile[0x00], "RIFF", 4) == 0 &&
         memcmp(&wavfile[0x08], "WAVE", 4) == 0 &&
         memcmp(&wavfile[0x0c], &fmt, 4) == 0 &&
         memcmp(&wavfile[0x24], &subchunk2id, 4) == 0)
   {
      /* Load the sample into the emulator */
      dataptr = &wavfile[0x2c];
      blockalign = wavfile[0x21] << 8 | wavfile[0x20];
      numchannels = wavfile[0x17] << 8 | wavfile[0x16];
      bitspersample = wavfile[0x23] << 8 | wavfile[0x22];
      if (blockalign > 0)
         file.SetSampleContent(dataptr, (length - 44) / blockalign, 0, bitspersample, 44100);
   }

   free(wavfile);
}

static void display_msg(enum retro_log_level level, unsigned duration, const char *str)
{
   if (!environ_cb)
      return;

   if (libretro_msg_interface_version >= 1)
   {
      struct retro_message_ext msg;
      unsigned priority;

      switch (level)
      {
         case RETRO_LOG_ERROR:
            priority = 5;
            break;
         case RETRO_LOG_WARN:
            priority = 4;
            break;
         case RETRO_LOG_INFO:
            priority = 3;
            break;
         case RETRO_LOG_DEBUG:
         default:
            priority = 1;
            break;
      }

      msg.msg      = str;
      msg.duration = duration;
      msg.priority = priority;
      msg.level    = level;
      msg.target   = RETRO_MESSAGE_TARGET_OSD;
      msg.type     = RETRO_MESSAGE_TYPE_NOTIFICATION_ALT;
      msg.progress = -1;

      environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
   }
   else
   {
      float fps       = is_pal ? 50 : 60;
      unsigned frames = (unsigned)(((float)duration * fps / 1000.0f) + 0.5f);
      struct retro_message msg;

      msg.msg    = str;
      msg.frames = frames;

      environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
   }
}

static void NST_CALLBACK file_io_callback(void*, Api::User::File &file)
{
   const void *addr;
   unsigned long addr_size;

   switch (file.GetAction())
   {
      case Api::User::File::LOAD_SAMPLE_MOERO_PRO_YAKYUU:
         load_wav("moepro", file); break;
      case Api::User::File::LOAD_SAMPLE_MOERO_PRO_YAKYUU_88:
         load_wav("moepro88", file); break;
      case Api::User::File::LOAD_SAMPLE_MOERO_PRO_TENNIS:
         load_wav("mptennis", file); break;
      case Api::User::File::LOAD_SAMPLE_TERAO_NO_DOSUKOI_OOZUMOU:
         load_wav("terao", file); break;
      case Api::User::File::LOAD_SAMPLE_AEROBICS_STUDIO:
         load_wav("ftaerobi", file); break;

      case Api::User::File::LOAD_BATTERY:
      case Api::User::File::LOAD_EEPROM:
      case Api::User::File::LOAD_TAPE:
      case Api::User::File::LOAD_TURBOFILE:
         file.GetRawStorage(sram, sram_size);
         break;

      case Api::User::File::SAVE_BATTERY:
      case Api::User::File::SAVE_EEPROM:
      case Api::User::File::SAVE_TAPE:
      case Api::User::File::SAVE_TURBOFILE:
         file.GetContent(addr, addr_size);
         if (addr != sram || sram_size != addr_size)
            if (log_cb)
               log_cb(RETRO_LOG_INFO, "[Nestopia]: SRAM changed place in RAM!\n");
         break;
      case Api::User::File::LOAD_FDS:
         {
            char base[PATH_MAX_LENGTH];
            const char *ext      = "";
            const char *save_dir = (g_save_dir && *g_save_dir)
                  ? g_save_dir : g_rom_dir;
            char *patch_data     = NULL;
            int64_t patch_size   = 0;
            if (fds_sav_extension)
               ext = ".sav";
            else if (fds_ups_extension)
               ext = ".ups";
            else if (fds_ips_extension)
               ext = ".ips";
            fill_pathname_join_special(base, save_dir, g_basename, sizeof(base));
            strlcat(base, ext, sizeof(base));
            if (log_cb)
               log_cb(RETRO_LOG_INFO, "Want to load FDS savefile using %s extension from: %s\n", ext, base);

            if (!filestream_read_file(base, (void**)&patch_data, &patch_size))
               return;

            {
               std::istringstream in_tmp(
                     std::string(patch_data, (size_t)patch_size),
                     std::istringstream::in | std::istringstream::binary);
               file.SetPatchContent(in_tmp);
            }
            free(patch_data);
         }
         break;
      case Api::User::File::SAVE_FDS:
         {
            char base[PATH_MAX_LENGTH];
            const char *ext      = "";
            const char *save_dir = (g_save_dir && *g_save_dir)
                  ? g_save_dir : g_rom_dir;
            Result result = RESULT_ERR_GENERIC;
            std::ostringstream out_tmp(
                  std::ostringstream::out | std::ostringstream::binary);
            if (fds_sav_extension)
               ext = ".sav";
            else if (fds_ups_extension)
               ext = ".ups";
            else if (fds_ips_extension)
               ext = ".ips";
            fill_pathname_join_special(base, save_dir, g_basename, sizeof(base));
            strlcat(base, ext, sizeof(base));
            if (log_cb)
               log_cb(RETRO_LOG_INFO, "Want to save FDS savefile using %s extension to: %s\n", ext, base);

            if (fds_patch_format_ups)
               result = file.GetPatchContent(Api::User::File::PATCH_UPS, out_tmp);
            else if (fds_patch_format_ips)
               result = file.GetPatchContent(Api::User::File::PATCH_IPS, out_tmp);

            if (NES_SUCCEEDED(result))
            {
               const std::string patch = out_tmp.str();
               filestream_write_file(base, patch.data(), (int64_t)patch.size());
            }
         }
         break;
      default:
         break;
   }
}

static void check_system_specs(void)
{
   unsigned level = 6;
   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

void retro_init(void)
{
   struct retro_log_callback log;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

   environ_cb(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION,
      &libretro_msg_interface_version);

   check_system_specs();
}

void retro_deinit(void)
{
   libretro_supports_bitmasks = false;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   switch (device)
   {
      case RETRO_DEVICE_AUTO:
         Api::Input(emulator).AutoSelectController(port);
         break;
      case RETRO_DEVICE_NONE:
         Api::Input(emulator).ConnectController(port, Api::Input::UNCONNECTED);
         break;
      case RETRO_DEVICE_GAMEPAD:
         switch (port)
         {
            case 0:
               Api::Input(emulator).ConnectController(port, Api::Input::PAD1);
               break;
            case 1:
               Api::Input(emulator).ConnectController(port, Api::Input::PAD2);
               break;
            case 2:
               Api::Input(emulator).ConnectController(port, Api::Input::PAD3);
               break;
            case 3:
               Api::Input(emulator).ConnectController(port, Api::Input::PAD4);
               break;
         }
         break;
      case RETRO_DEVICE_ARKANOID:
         Api::Input(emulator).ConnectController(port, Api::Input::PADDLE);
         break;
      case RETRO_DEVICE_ZAPPER:
         Api::Input(emulator).ConnectController(port, Api::Input::ZAPPER);
         break;
   }
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "Nestopia";
#ifdef GIT_VERSION
   info->library_version  = NST_VERSION GIT_VERSION;
#else
   info->library_version  = NST_VERSION;
#endif
   info->need_fullpath    = false;
   info->valid_extensions = "nes|fds|unf|unif";
}

double get_aspect_ratio(void)
{
  double aspect_ratio = is_pal ? NES_PAL_PAR : NES_NTSC_PAR;

  if (aspect_ratio_mode == 1)
  {
    aspect_ratio = NES_NTSC_PAR;
  }
  else if (aspect_ratio_mode == 2)
  {
    aspect_ratio = NES_PAL_PAR;
  }
  else if (aspect_ratio_mode == 3)
  {
    aspect_ratio = NES_4_3_DAR;
  }
  else if (aspect_ratio_mode == 4)
  {
    aspect_ratio = 0;
  }
    
  return aspect_ratio;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   const retro_system_timing timing = {
      is_pal ? (double)Core::CLK_PAL  / (Core::CLK_PAL_DIV  * (double)Core::PPU_RP2C07_HVSYNC)
             : (double)Core::CLK_NTSC / (Core::CLK_NTSC_DIV * (double)Core::PPU_RP2C02_HVSYNC),
      SAMPLERATE };
   info->timing = timing;

   // It's better if the size is based on NTSC_WIDTH if the filter is on
   const retro_game_geometry geom = {
      Api::Video::Output::WIDTH - (overscan_h_left + overscan_h_right),
      Api::Video::Output::HEIGHT - (overscan_v_top + overscan_v_bottom),
      Api::Video::Output::NTSC_WIDTH,
      Api::Video::Output::HEIGHT,
      get_aspect_ratio(),
   };
   info->geometry = geom;
}


void retro_set_environment(retro_environment_t cb)
{
   struct retro_vfs_interface_info vfs_iface_info;

   environ_cb = cb;
   libretro_set_core_options(environ_cb,
         &libretro_supports_option_categories);

   vfs_iface_info.required_interface_version = FILESTREAM_REQUIRED_VFS_VERSION;
   vfs_iface_info.iface                      = NULL;
   if (cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
      filestream_vfs_init(&vfs_iface_info);

   static const struct retro_controller_description port1[] = {
      { "Auto", RETRO_DEVICE_AUTO },
      { "Gamepad", RETRO_DEVICE_GAMEPAD },
      { NULL, 0 },
   };
   static const struct retro_controller_description port2[] = {
      { "Auto", RETRO_DEVICE_AUTO },
      { "Gamepad", RETRO_DEVICE_GAMEPAD },
      { "Arkanoid", RETRO_DEVICE_ARKANOID },
      { "Zapper", RETRO_DEVICE_ZAPPER },
      { NULL, 0 },
   };
   static const struct retro_controller_description port3[] = {
      { "Auto", RETRO_DEVICE_AUTO },
      { "Gamepad", RETRO_DEVICE_GAMEPAD },
      { NULL, 0 },
   };
   static const struct retro_controller_description port4[] = {
      { "Auto", RETRO_DEVICE_AUTO },
      { "Gamepad", RETRO_DEVICE_GAMEPAD },
      { NULL, 0 },
   };

   static const struct retro_controller_info ports[] = {
      { port1, 2 },
      { port2, 4 },
      { port3, 2 },
      { port4, 2 },
      { NULL, 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void *)ports);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{
   machine->Reset(false);

   if (machine->Is(Nes::Api::Machine::DISK))
   {
      fds->EjectDisk();
      if (fds_auto_insert)
         fds->InsertDisk(0, 0);
   }
}

typedef struct
{
   unsigned retro;
   unsigned nes;
} keymap;

static enum {
   ARKANOID_DEVICE_MOUSE,
   ARKANOID_DEVICE_POINTER
} arkanoid_device;

static enum {
    ZAPPER_DEVICE_LIGHTGUN,
    ZAPPER_DEVICE_MOUSE,
    ZAPPER_DEVICE_POINTER
} zapper_device;

static keymap bindmap_default[] = {
   { RETRO_DEVICE_ID_JOYPAD_A, Core::Input::Controllers::Pad::A },
   { RETRO_DEVICE_ID_JOYPAD_B, Core::Input::Controllers::Pad::B },
   { RETRO_DEVICE_ID_JOYPAD_X, Core::Input::Controllers::Pad::A },
   { RETRO_DEVICE_ID_JOYPAD_Y, Core::Input::Controllers::Pad::B },
   { RETRO_DEVICE_ID_JOYPAD_SELECT, Core::Input::Controllers::Pad::SELECT },
   { RETRO_DEVICE_ID_JOYPAD_START, Core::Input::Controllers::Pad::START },
   { RETRO_DEVICE_ID_JOYPAD_UP, Core::Input::Controllers::Pad::UP },
   { RETRO_DEVICE_ID_JOYPAD_DOWN, Core::Input::Controllers::Pad::DOWN },
   { RETRO_DEVICE_ID_JOYPAD_LEFT, Core::Input::Controllers::Pad::LEFT },
   { RETRO_DEVICE_ID_JOYPAD_RIGHT, Core::Input::Controllers::Pad::RIGHT },
};

static keymap bindmap_shifted[] = {
   { RETRO_DEVICE_ID_JOYPAD_B, Core::Input::Controllers::Pad::A },
   { RETRO_DEVICE_ID_JOYPAD_Y, Core::Input::Controllers::Pad::B },
   { RETRO_DEVICE_ID_JOYPAD_A, Core::Input::Controllers::Pad::A },
   { RETRO_DEVICE_ID_JOYPAD_X, Core::Input::Controllers::Pad::B },
   { RETRO_DEVICE_ID_JOYPAD_SELECT, Core::Input::Controllers::Pad::SELECT },
   { RETRO_DEVICE_ID_JOYPAD_START, Core::Input::Controllers::Pad::START },
   { RETRO_DEVICE_ID_JOYPAD_UP, Core::Input::Controllers::Pad::UP },
   { RETRO_DEVICE_ID_JOYPAD_DOWN, Core::Input::Controllers::Pad::DOWN },
   { RETRO_DEVICE_ID_JOYPAD_LEFT, Core::Input::Controllers::Pad::LEFT },
   { RETRO_DEVICE_ID_JOYPAD_RIGHT, Core::Input::Controllers::Pad::RIGHT },
};

static keymap *bindmap = bindmap_default;

/* Input snapshot, captured exactly once per retro_run.
 *
 * Nestopia invokes the controller callbacks below from inside
 * emulator.Execute(), once per hardware controller strobe.  A game may
 * strobe zero or several times per frame, so polling and reading the
 * frontend from those callbacks calls retro_input_poll_t an arbitrary
 * number of times per frame and can observe several different input
 * states within a single video frame.  Everything is sampled here
 * instead; the callbacks only consume the snapshot. */
static int16_t pad_state[4];
static int16_t aux_mouse_dx, aux_mouse_dy;
static int16_t aux_mouse_left;
static int16_t aux_pointer_x, aux_pointer_y, aux_pointer_pressed;
static int16_t aux_gun_x, aux_gun_y;
static int16_t aux_gun_trigger, aux_gun_reload, aux_gun_offscreen;

static void update_input_snapshot(void)
{
   unsigned p;

   input_poll_cb();

   for (p = 0; p < 4; p++)
   {
      if (libretro_supports_bitmasks)
         pad_state[p] = input_state_cb(p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
      else
      {
         int16_t ret = 0;
         for (unsigned i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3 + 1); i++)
            ret |= input_state_cb(p, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
         pad_state[p] = ret;
      }

      /* A/B turbo cadence advances once per video frame, not once per
       * controller strobe. */
      if (tstate[p]) tstate[p]--; else tstate[p] = tpulse;
   }

   aux_mouse_dx        = input_state_cb(1, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
   aux_mouse_dy        = input_state_cb(1, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
   aux_mouse_left      = input_state_cb(1, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
   aux_pointer_x       = input_state_cb(1, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
   aux_pointer_y       = input_state_cb(1, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
   aux_pointer_pressed = input_state_cb(1, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
   aux_gun_offscreen   = input_state_cb(1, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN);
   aux_gun_x           = input_state_cb(1, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X);
   aux_gun_y           = input_state_cb(1, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y);
   aux_gun_trigger     = input_state_cb(1, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER);
   aux_gun_reload      = input_state_cb(1, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD);
}

/* Consume the frame's relative mouse motion exactly once, however many
 * times the game strobes a mouse-driven controller this frame. */
static void take_mouse_delta(int *dx, int *dy)
{
   if (dx) *dx = aux_mouse_dx;
   if (dy) *dy = aux_mouse_dy;
   aux_mouse_dx = 0;
   aux_mouse_dy = 0;
}

static void NST_CALLBACK nst_cb_event(void *userdata, Api::User::Event event, const void *data) {
   // Handle special events
   switch (event) {
      case Api::User::EVENT_CPU_JAM:
         log_cb(RETRO_LOG_WARN, "Cpu: Jammed.");
         break;
      case Api::User::EVENT_CPU_UNOFFICIAL_OPCODE:
         log_cb(RETRO_LOG_DEBUG, "Cpu: Unofficial Opcode %s\n", (const char*)data);
         break;
      case Api::User::EVENT_DISPLAY_TIMER:
         display_msg(RETRO_LOG_INFO, 1000, (const char*)data);
         break;
      default: break;
   }
}

static bool NST_CALLBACK gamepad_callback(Api::Base::UserData data, Core::Input::Controllers::Pad& pad, unsigned int port)
{
   bool pressed_l3        = false;

   uint buttons = 0;
   int16_t ret = pad_state[port];

   for (unsigned bind = 0; bind < sizeof(bindmap_default) / sizeof(bindmap[0]); bind++)
      buttons |= (ret & (1 << bindmap[bind].retro)) ? bindmap[bind].nes : 0;
   if (ret & (1 << bindmap[2].retro))
      tstate[port] ? buttons &= ~Core::Input::Controllers::Pad::A : buttons |= Core::Input::Controllers::Pad::A;
   if (ret & (1 << bindmap[3].retro))
      tstate[port] ? buttons &= ~Core::Input::Controllers::Pad::B : buttons |= Core::Input::Controllers::Pad::B;

   pad.buttons = buttons;
   buttons = 0;

   /* Player 0 needs some extra checks */
   if (port == 0)
   {
      pressed_l3       = ret & (1 << RETRO_DEVICE_ID_JOYPAD_L3);
   }

   if (pressed_l3)
      buttons = pad.mic | 0x04;
   pad.mic = buttons;

   return true;
}

static bool NST_CALLBACK arkanoid_callback(Api::Base::UserData data, Core::Input::Controllers::Paddle& paddle)
{
   int min_x = overscan_h_left;
   int max_x = 255 - overscan_h_right;

   unsigned int button = 0;

   switch (arkanoid_device)
   {
      case ARKANOID_DEVICE_MOUSE:
      {
         int dx;
         min_x = arkanoid_paddle_min;
         max_x = arkanoid_paddle_max;
         take_mouse_delta(&dx, NULL);
         cur_x += dx;
         button = aux_mouse_left;
         break;
      }
      case ARKANOID_DEVICE_POINTER:
         cur_x = aux_pointer_x;
         cur_x = (cur_x + 0x7FFF) * max_x / (0x7FFF * 2);
         button = aux_pointer_pressed;
         break;
   }

   if (cur_x < min_x)
      cur_x = min_x;
   else if (cur_x > max_x)
      cur_x = max_x;
   paddle.x = cur_x;
   paddle.button = button;

   return true;
}

static bool NST_CALLBACK vssystem_callback(Api::Base::UserData data, Core::Input::Controllers::VsSystem& vsSystem)
{
   uint buttons = 0;
   int16_t ret = pad_state[0];

   if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_L2))
      buttons |= Core::Input::Controllers::VsSystem::COIN_1;

   if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_R2))
      buttons |= Core::Input::Controllers::VsSystem::COIN_2;

   vsSystem.insertCoin = buttons;

   return true;
}

static bool NST_CALLBACK zapper_callback(Api::Base::UserData data, Core::Input::Controllers::Zapper& zapper)
{
   int min_x = overscan_h_left;
   int max_x = 255 - overscan_h_right;
   int min_y = overscan_v_top;
   int max_y = 239 - overscan_v_bottom;

   zapper.fire = 0;

   if (show_crosshair)
      show_crosshair = SHOW_CROSSHAIR_ON;

   switch (zapper_device)
   {
      case ZAPPER_DEVICE_LIGHTGUN:
         if (!aux_gun_offscreen)
         {
            cur_x = aux_gun_x;
            cur_y = aux_gun_y;

            cur_x = cur_x != 0 ? (cur_x + 0x7FFF) * max_x / (0x7FFF * 2) : crossx;
            cur_y = cur_y != 0 ? (cur_y + 0x7FFF) * max_y / (0x7FFF * 2) : crossy;
         }
         else
         {
            cur_x = min_x;
            cur_y = min_y;
         }

         if (aux_gun_trigger) {
            zapper.x = cur_x;
            zapper.y = cur_y;
            zapper.fire = 1;
         }

         if (aux_gun_reload) {
            zapper.x = ~1U;
            zapper.fire = 1;
         }
         break;
      case ZAPPER_DEVICE_MOUSE:
      {
         int dx, dy;
         take_mouse_delta(&dx, &dy);
         cur_x += dx;
         cur_y += dy;

         if (cur_x < min_x)
            cur_x = min_x;
         else if (cur_x > max_x)
            cur_x = max_x;

         if (cur_y < min_y)
            cur_y = min_y;
         else if (cur_y > max_y)
            cur_y = max_y;

         if (aux_mouse_left)
         {
            zapper.x = cur_x;
            zapper.y = cur_y;
            zapper.fire = 1;
         }
         break;
      }
      case ZAPPER_DEVICE_POINTER:
         cur_x = aux_pointer_x;
         cur_y = aux_pointer_y;

         cur_x = cur_x != 0 ? (cur_x + 0x7FFF) * max_x / (0x7FFF * 2) : crossx;
         cur_y = cur_y != 0 ? (cur_y + 0x7FFF) * max_y / (0x7FFF * 2) : crossy;

         if (aux_pointer_pressed)
         {
            zapper.x = cur_x;
            zapper.y = cur_y;
            zapper.fire = 1;
         }
         break;
      default:
         break;
   }

   if (cur_x > max_x) { crossx = max_x; }
   else if (cur_x < min_x) { crossx = min_x; }
   else { crossx = cur_x; }

   if (cur_y > max_y) { crossy = max_y; }
   else if (cur_y < min_y) { crossy = min_y; }
   else { crossy = cur_y; }

   return true;
}

static void poll_fds_buttons()
{
   if (machine->Is(Nes::Api::Machine::DISK))
   {
      bool pressed_l = pad_state[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L);
      bool pressed_r = pad_state[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R);

      bool curL         = pressed_l;

      if (curL && !prevL)
      {
         if (!fds->IsAnyDiskInserted()) {
            fds->InsertDisk(0, 0);
            display_msg(RETRO_LOG_INFO, 2000, "Disk Inserted");
         }
         else if (fds->CanChangeDiskSide()) {
            fds->ChangeSide();
            std::string msg = std::string("Switched to Disk ") +
                (fds->GetCurrentDisk() == 0 ? "1" : "2") +
                " Side " + (fds->GetCurrentDiskSide() == 0 ? "A" : "B");
            display_msg(RETRO_LOG_INFO, 2000, msg.c_str());
         }
      }
      prevL = curL;

      bool curR         = pressed_r;

      if (curR && !prevR && (fds->GetNumDisks() > 1))
      {
         int currdisk = fds->GetCurrentDisk();
         fds->EjectDisk();
         fds->InsertDisk(!currdisk, 0);

         std::string msg = std::string("Disk ") + (fds->GetCurrentDisk() ? "2" : "1");

         if (fds->IsAnyDiskInserted())
            msg += " Inserted";
         else
            msg += " Ejected";

         display_msg(RETRO_LOG_INFO, 2000, msg.c_str());
      }
      prevR = curR;
   }
}

static void check_variables(void)
{
   static bool last_ntsc_val_same;
   struct retro_variable var = {0};
   struct retro_system_av_info av_info;
   struct retro_core_option_display option_display;

   Api::Sound sound(emulator);
   Api::Video video(emulator);
   Api::Video::RenderState renderState;
   Api::Machine machine(emulator);
   Api::Video::RenderState::Filter filter;

   /* System */

   var.key = "nestopia_favored_system"; // System Region
   is_pal = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "auto") == 0)
      {
         machine.SetMode(machine.GetDesiredMode());
         if (machine.GetMode() == Api::Machine::PAL)
         {
            is_pal = true;
            favsystem = Api::Machine::FAVORED_NES_PAL;
            machine.SetMode(Api::Machine::PAL);
         }
         else
         {
            favsystem = Api::Machine::FAVORED_NES_NTSC;
            machine.SetMode(Api::Machine::NTSC);
         }
      }
      else if (strcmp(var.value, "ntsc") == 0)
      {
         favsystem = Api::Machine::FAVORED_NES_NTSC;
         machine.SetMode(Api::Machine::NTSC);
      }
      else if (strcmp(var.value, "pal") == 0)
      {
         favsystem = Api::Machine::FAVORED_NES_PAL;
         machine.SetMode(Api::Machine::PAL);
         is_pal = true;
      }
      else if (strcmp(var.value, "famicom") == 0)
      {
         favsystem = Api::Machine::FAVORED_FAMICOM;
         machine.SetMode(Api::Machine::NTSC);
      }
      else if (strcmp(var.value, "dendy") == 0)
      {
         favsystem = Api::Machine::FAVORED_DENDY;
         machine.SetMode(Api::Machine::PAL);
         is_pal = true;
      }
      else
      {
         favsystem = Api::Machine::FAVORED_NES_NTSC;
         machine.SetMode(Api::Machine::NTSC);
      }
   }
   if (audio) delete audio;
   update_audio_timing();
   audio = new Api::Sound::Output(audio_buffer, audio_spf_base);

   var.key = "nestopia_fds_auto_insert"; // FDS Auto Insert
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
      fds_auto_insert = (strcmp(var.value, "enabled") == 0);

   var.key = "nestopia_fds_savefile_format"; // FDS Savefile Format
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "sav_ups") == 0)
         fds_savefile_format = FDS_SAVEFILE_SAV_UPS;
      else if (strcmp(var.value, "ups") == 0)
         fds_savefile_format = FDS_SAVEFILE_UPS;
      else if (strcmp(var.value, "ips") == 0)
         fds_savefile_format = FDS_SAVEFILE_IPS;

      /* FDS savefile format checks are positioned here to allow changes at runtime.
       * This makes it possible to convert any savefile currently in use to the preferred format.*/
      fds_sav_extension = (fds_savefile_format == FDS_SAVEFILE_SAV_UPS);
      fds_ups_extension = (fds_savefile_format == FDS_SAVEFILE_UPS);
      fds_ips_extension = (fds_savefile_format == FDS_SAVEFILE_IPS);
      fds_patch_format_ups = ((fds_savefile_format == FDS_SAVEFILE_SAV_UPS) ||
                              (fds_savefile_format == FDS_SAVEFILE_UPS));
      fds_patch_format_ips = (fds_savefile_format == FDS_SAVEFILE_IPS);
   }

   /* Video */

   var.key = "nestopia_blargg_ntsc_filter"; // Blargg NTSC Filter
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "disabled") == 0)
         blargg_ntsc = 0;
      else if (strcmp(var.value, "composite") == 0)
         blargg_ntsc = 2;
      else if (strcmp(var.value, "svideo") == 0)
         blargg_ntsc = 3;
      else if (strcmp(var.value, "rgb") == 0)
         blargg_ntsc = 4;
      else if (strcmp(var.value, "monochrome") == 0)
         blargg_ntsc = 5;
   }

   switch(blargg_ntsc)
   {
      case 0: // Disabled
         filter = Api::Video::RenderState::FILTER_NONE;
         video_width = Api::Video::Output::WIDTH;
         video.SetSaturation(Api::Video::DEFAULT_SATURATION);
         break;
      case 2: // Composite Video
         filter = Api::Video::RenderState::FILTER_NTSC;
         video.SetSharpness(Api::Video::DEFAULT_SHARPNESS_COMP);
         video.SetColorResolution(Api::Video::DEFAULT_COLOR_RESOLUTION_COMP);
         video.SetColorBleed(Api::Video::DEFAULT_COLOR_BLEED_COMP);
         video.SetColorArtifacts(Api::Video::DEFAULT_COLOR_ARTIFACTS_COMP);
         video.SetColorFringing(Api::Video::DEFAULT_COLOR_FRINGING_COMP);
         video.SetSaturation(Api::Video::DEFAULT_SATURATION_COMP);
         video_width = Api::Video::Output::NTSC_WIDTH;
         break;
      case 3: // S-Video
         filter = Api::Video::RenderState::FILTER_NTSC;
         video.SetSharpness(Api::Video::DEFAULT_SHARPNESS_SVIDEO);
         video.SetColorResolution(Api::Video::DEFAULT_COLOR_RESOLUTION_SVIDEO);
         video.SetColorBleed(Api::Video::DEFAULT_COLOR_BLEED_SVIDEO);
         video.SetColorArtifacts(Api::Video::DEFAULT_COLOR_ARTIFACTS_SVIDEO);
         video.SetColorFringing(Api::Video::DEFAULT_COLOR_FRINGING_SVIDEO);
         video.SetSaturation(Api::Video::DEFAULT_SATURATION_SVIDEO);
         video_width = Api::Video::Output::NTSC_WIDTH;
         break;
      case 4: // RGB Scart
         filter = Api::Video::RenderState::FILTER_NTSC;
         video.SetSharpness(Api::Video::DEFAULT_SHARPNESS_RGB);
         video.SetColorResolution(Api::Video::DEFAULT_COLOR_RESOLUTION_RGB);
         video.SetColorBleed(Api::Video::DEFAULT_COLOR_BLEED_RGB);
         video.SetColorArtifacts(Api::Video::DEFAULT_COLOR_ARTIFACTS_RGB);
         video.SetColorFringing(Api::Video::DEFAULT_COLOR_FRINGING_RGB);
         video.SetSaturation(Api::Video::DEFAULT_SATURATION_RGB);
         video_width = Api::Video::Output::NTSC_WIDTH;
         break;
     case 5: // Monochrome
         filter = Api::Video::RenderState::FILTER_NTSC;
         video.SetSharpness(Api::Video::DEFAULT_SHARPNESS_MONO);
         video.SetColorResolution(Api::Video::DEFAULT_COLOR_RESOLUTION_MONO);
         video.SetColorBleed(Api::Video::DEFAULT_COLOR_BLEED_MONO);
         video.SetColorArtifacts(Api::Video::DEFAULT_COLOR_ARTIFACTS_MONO);
         video.SetColorFringing(Api::Video::DEFAULT_COLOR_FRINGING_MONO);
         video.SetSaturation(Api::Video::DEFAULT_SATURATION_MONO);
         video_width = Api::Video::Output::NTSC_WIDTH;
         break;
   }

   var.key = "nestopia_palette"; // Palette
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      bool decoder = false;
      if (strcmp(var.value, "consumer") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_YUV);
         video.SetDecoder(Api::Video::DECODER_CONSUMER);
         decoder = true;
      }
      else if (strcmp(var.value, "canonical") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_YUV);
         video.SetDecoder(Api::Video::DECODER_CANONICAL);
         decoder = true;
      }
      else if (strcmp(var.value, "alternative") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_YUV);
         video.SetDecoder(Api::Video::DECODER_ALTERNATIVE);
         decoder = true;
      }
      else if (strcmp(var.value, "cxa2025as") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_YUV);
         video.SetDecoder(Api::Video::DECODER_CXA2025AS_US);
         decoder = true;
      }
      else if (strcmp(var.value, "cxa2025as_jp") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_YUV);
         video.SetDecoder(Api::Video::DECODER_CXA2025AS_JP);
         decoder = true;
      }
      else if (strcmp(var.value, "rgb") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_RGB);
      }
      else if (strcmp(var.value, "royaltea") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(royaltea_palette, Api::Video::Palette::STD_PALETTE);
      }
      else if (strcmp(var.value, "pal") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(pal_palette, Api::Video::Palette::STD_PALETTE);
      }
      else if (strcmp(var.value, "digital-prime-fbx") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(digital_prime_fbx_palette, Api::Video::Palette::STD_PALETTE);
      }
      else if (strcmp(var.value, "magnum-fbx") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(magnum_fbx_palette, Api::Video::Palette::STD_PALETTE);
      }
      else if (strcmp(var.value, "smoothv2-fbx") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(smoothv2_fbx_palette, Api::Video::Palette::STD_PALETTE);
      }
      else if (strcmp(var.value, "composite-direct-fbx") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(composite_direct_fbx_palette, Api::Video::Palette::STD_PALETTE);
      }
      else if (strcmp(var.value, "pvm-style-d93-fbx") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(pvm_style_d93_fbx_palette, Api::Video::Palette::STD_PALETTE);
      }
      else if (strcmp(var.value, "ntsc-hardware-fbx") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(ntsc_hardware_fbx_palette, Api::Video::Palette::STD_PALETTE);
      }
      else if (strcmp(var.value, "nes-classic-fbx-fs") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(nes_classic_fbx_fs_palette, Api::Video::Palette::STD_PALETTE);
      }
      else if (strcmp(var.value, "restored-wii-vc") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(restored_wii_vc_palette, Api::Video::Palette::STD_PALETTE);
      }
      else if (strcmp(var.value, "wii-vc") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(wii_vc_palette, Api::Video::Palette::STD_PALETTE);
      }
      else if (strcmp(var.value, "raw") == 0) {
         /* outputs raw chroma/level/emphasis in the R/G/B channels
          * that can be decoded by the frontend (using shaders for example)
          * the following formulas can be used to extract the
          * values back from a normalized R/G/B triplet
          * chroma   = floor((R * 15.0) + 0.5)
          * level    = floor((G *  3.0) + 0.5)
          * emphasis = floor((B *  7.0) + 0.5) */
         unsigned char raw_palette[512][3];
         int i;
         for (i = 0; i < 512; i++)
         {
            raw_palette[i][0] = (((i >> 0) & 0xF) * 255) / 15;
            raw_palette[i][1] = (((i >> 4) & 0x3) * 255) / 3;
            raw_palette[i][2] = (((i >> 6) & 0x7) * 255) / 7;
         }
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom(raw_palette, Api::Video::Palette::EXT_PALETTE);
      }
      else if (strcmp(var.value, "custom") == 0) {
         video.GetPalette().SetMode(Api::Video::Palette::MODE_CUSTOM);
         video.GetPalette().SetCustom((const byte(*)[3])custpal, Api::Video::Palette::STD_PALETTE);
      }

      // Fix up palette for VS. System
      Api::Cartridge cart(emulator);
      if (!machine.Is(Api::Machine::DISK) && cart.GetProfile()) {
         switch (cart.GetProfile()->system.type) {
            case Api::Cartridge::Profile::System::VS_UNISYSTEM:
            case Api::Cartridge::Profile::System::VS_DUALSYSTEM: {
               if (!decoder) { // If we're using an internal decoder, no override
                  video.GetPalette().SetMode(Api::Video::Palette::MODE_YUV);
                  video.SetDecoder(Api::Video::DECODER_CONSUMER);
               }
            }
            default: break;
         }
      }
   }

   var.key = "nestopia_overscan_v_top"; // Mask Overscan (Top Vertical)
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
      overscan_v_top = atoi(var.value);

   var.key = "nestopia_overscan_v_bottom"; // Mask Overscan (Bottom Vertical)
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
      overscan_v_bottom = atoi(var.value);

   var.key = "nestopia_overscan_h_left"; // Mask Overscan (Left Horizontal)
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
      overscan_h_left = atoi(var.value);

   var.key = "nestopia_overscan_h_right"; // Mask Overscan (Right Horizontal)
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
      overscan_h_right = atoi(var.value);

   var.key = "nestopia_aspect"; // Preferred Aspect Ratio
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "ntsc"))
         aspect_ratio_mode = 1;
      else if (!strcmp(var.value, "pal"))
         aspect_ratio_mode = 2;
      else if (!strcmp(var.value, "4:3"))
         aspect_ratio_mode = 3;
      else if (!strcmp(var.value, "uncorrected"))
         aspect_ratio_mode = 4;
      else
         aspect_ratio_mode = 0;
   }

   /* Audio */

   var.key = "nestopia_genie_distortion"; // Game Genie Sound Distortion
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "disabled") == 0)
         sound.SetGenie(0);
      else if (strcmp(var.value, "enabled") == 0)
         sound.SetGenie(1);
   }

   /* "Show settings" are not required if categories are supported */
   option_display.visible = !libretro_supports_option_categories;
   option_display.key = "nestopia_show_advanced_av_settings";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

   var.key = "nestopia_show_advanced_av_settings"; // Show Advanced Audio Settings (Reopen Menu)
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      bool show_advanced_av_settings_prev = show_advanced_av_settings;

      show_advanced_av_settings = true;
      if (strcmp(var.value, "disabled") == 0)
         show_advanced_av_settings = false;

      if (show_advanced_av_settings != show_advanced_av_settings_prev)
      {
         size_t i;
         char av_keys[11][40] = {
         "nestopia_audio_vol_sq1",
         "nestopia_audio_vol_sq2",
         "nestopia_audio_vol_tri",
         "nestopia_audio_vol_noise",
         "nestopia_audio_vol_dpcm",
         "nestopia_audio_vol_fds",
         "nestopia_audio_vol_mmc5",
         "nestopia_audio_vol_vrc6",
         "nestopia_audio_vol_vrc7",
         "nestopia_audio_vol_n163",
         "nestopia_audio_vol_s5b"
         };

         option_display.visible = show_advanced_av_settings
                                 || libretro_supports_option_categories;

         for (i = 0; i < 11; i++)
         {
         option_display.key = av_keys[i];
         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
         }
      }
   }

   var.key = "nestopia_audio_vol_sq1"; // Square 1 Channel Volume %
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sound.SetVolume(Api::Sound::CHANNEL_SQUARE1, atoi(var.value));

   var.key = "nestopia_audio_vol_sq2"; // Square 2 Channel Volume %
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sound.SetVolume(Api::Sound::CHANNEL_SQUARE2, atoi(var.value));

   var.key = "nestopia_audio_vol_tri"; // Triangle Channel Volume %
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sound.SetVolume(Api::Sound::CHANNEL_TRIANGLE, atoi(var.value));

   var.key = "nestopia_audio_vol_noise"; // Noise Channel Volume %
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sound.SetVolume(Api::Sound::CHANNEL_NOISE, atoi(var.value));

   var.key = "nestopia_audio_vol_dpcm"; // DPCM Channel Volume %
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sound.SetVolume(Api::Sound::CHANNEL_DPCM, atoi(var.value));

   var.key = "nestopia_audio_vol_fds"; // FDS Channel Volume %
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sound.SetVolume(Api::Sound::CHANNEL_FDS, atoi(var.value));

   var.key = "nestopia_audio_vol_mmc5"; // MMC5 Channel Volume %
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sound.SetVolume(Api::Sound::CHANNEL_MMC5, atoi(var.value));

   var.key = "nestopia_audio_vol_vrc6"; // VRC6 Channel Volume %
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sound.SetVolume(Api::Sound::CHANNEL_VRC6, atoi(var.value));

   var.key = "nestopia_audio_vol_vrc7"; // VRC7 Channel Volume %
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sound.SetVolume(Api::Sound::CHANNEL_VRC7, atoi(var.value));

   var.key = "nestopia_audio_vol_n163"; // N163 Channel Volume %
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sound.SetVolume(Api::Sound::CHANNEL_N163, atoi(var.value));

   var.key = "nestopia_audio_vol_s5b"; // S5B Channel Volume %
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sound.SetVolume(Api::Sound::CHANNEL_S5B, atoi(var.value));

   var.key = "nestopia_audio_type"; // Audio output
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "mono") == 0)
         sound.SetSpeaker(Api::Sound::SPEAKER_MONO);
      else
         sound.SetSpeaker(Api::Sound::SPEAKER_STEREO);
   }

   /* Input */

   var.key = "nestopia_select_adapter"; // 4 Player Adapter
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
       if (!strcmp(var.value, "auto")) {
           Api::Input(emulator).AutoSelectAdapter();
       }
       else if (!strcmp(var.value, "ntsc")) {
           Api::Input(emulator).ConnectAdapter(Api::Input::ADAPTER_NES);
        }
       else if (!strcmp(var.value, "famicom")) {
           Api::Input(emulator).ConnectAdapter(Api::Input::ADAPTER_FAMICOM);
        }
   }

   var.key = "nestopia_button_shift"; // Shift Buttons Clockwise
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "disabled") == 0)
         bindmap = bindmap_default;
      else if (strcmp(var.value, "enabled") == 0)
         bindmap = bindmap_shifted;
   }

   var.key = "nestopia_arkanoid_device"; // Arkanoid Device
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "mouse") == 0)
         arkanoid_device = ARKANOID_DEVICE_MOUSE;
      if (strcmp(var.value, "pointer") == 0)
         arkanoid_device = ARKANOID_DEVICE_POINTER;
   }

   // https://www.nesdev.org/wiki/Arkanoid_controller
   // There are two different Arkanoid (or Vaus) controllers.
   // And each controller has a slightly different range of values.
   var.key = "nestopia_arkanoid_paddle_range"; // Arkanoid Paddle Range
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      // Default full range that will work for both arkanoidI and arkanoidII
      arkanoid_paddle_min = 32;
      arkanoid_paddle_max = 166;
      if (strcmp(var.value, "arkanoidI") == 0)
      {
         arkanoid_paddle_min = 46;
         arkanoid_paddle_max = 166;
      }
      else if (strcmp(var.value, "arkanoidII") == 0)
      {
         arkanoid_paddle_min = 32;
         arkanoid_paddle_max = 153;
      }
   }

   var.key = "nestopia_zapper_device"; // Zapper Device
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "lightgun") == 0)
         zapper_device = ZAPPER_DEVICE_LIGHTGUN;
      else if (strcmp(var.value, "mouse") == 0)
         zapper_device = ZAPPER_DEVICE_MOUSE;
      else if (strcmp(var.value, "pointer") == 0)
         zapper_device = ZAPPER_DEVICE_POINTER;
   }

   var.key = "nestopia_show_crosshair"; // Show Crosshair
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "disabled") == 0)
         show_crosshair = SHOW_CROSSHAIR_DISABLED;
      else
         show_crosshair = SHOW_CROSSHAIR_OFF;
   }

   var.key = "nestopia_turbo_pulse"; // Turbo Pulse Speed
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
      tpulse = atoi(var.value);

   pitch = video_width * 4;

   renderState.filter = filter;
   renderState.width = video_width;
   renderState.height = Api::Video::Output::HEIGHT;
   renderState.bits.count = 32;
   renderState.bits.mask.r = 0x00ff0000;
   renderState.bits.mask.g = 0x0000ff00;
   renderState.bits.mask.b = 0x000000ff;
   if (NES_FAILED(video.SetRenderState( renderState )) && log_cb)
      log_cb(RETRO_LOG_WARN, "Nestopia core rejected render state\n");;

   retro_get_system_av_info(&av_info);
   environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &av_info);

   /* Emulation Hacks */

   var.key = "nestopia_nospritelimit"; // Remove Sprite Limit
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "disabled") == 0)
         video.EnableUnlimSprites(false);
      else if (strcmp(var.value, "enabled") == 0)
         video.EnableUnlimSprites(true);
   }

   var.key = "nestopia_overclock"; // CPU Speed (Overclock)
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "1x") == 0)
         video.EnableOverclocking(false);
      else if (strcmp(var.value, "2x") == 0)
         video.EnableOverclocking(true);
   }

   var.key = "nestopia_ram_power_state"; // RAM Power-on State
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
   {
      if (strcmp(var.value, "0x00") == 0)
         machine.SetRamPowerState(0);
      else if (strcmp(var.value, "0xFF") == 0)
         machine.SetRamPowerState(1);
      else if (strcmp(var.value, "random") == 0)
         machine.SetRamPowerState(2);
   }

}

void retro_run(void)
{
   /* Exact per-frame sample count via remainder carry; must be set
    * before Execute() since the APU reads the requested length when
    * it flushes the frame's audio. */
   unsigned frames = audio_spf_base;
   audio_frac += audio_spf_rem;
   if (audio_frac >= audio_spf_den)
   {
      audio_frac -= audio_spf_den;
      frames++;
   }
   audio->length[0] = frames;

   update_input_snapshot();
   poll_fds_buttons();
   emulator.Execute(video, audio, input);

   if (show_crosshair == SHOW_CROSSHAIR_ON)
      draw_crosshair(crossx, crossy);

   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
   {
      check_variables();
      delete video;
      video = 0;
      video = new Api::Video::Output(video_buffer, video_width * sizeof(uint32_t));
   }

   video->pixels = video_buffer;
   int dif = blargg_ntsc ? 9 : 4;

   size_t vboffset = ((blargg_ntsc ? Api::Video::Output::NTSC_WIDTH : Api::Video::Output::WIDTH) * overscan_v_top) +
      ((overscan_h_left * dif) / 4);

   video_cb(video_buffer + vboffset,
      video_width - (((overscan_h_left + overscan_h_right) * dif) / 4),
      Api::Video::Output::HEIGHT - (overscan_v_top + overscan_v_bottom),
      pitch);

   // Use audio buffer untouched for stereo, duplicate samples for mono
   if (Api::Sound(emulator).GetSpeaker() == Api::Sound::SPEAKER_MONO) 
   {
      for (unsigned i = 0; i < frames; i++)
         audio_stereo_buffer[i << 1] = audio_stereo_buffer[(i << 1) + 1] = audio_buffer[i];
      audio_batch_cb(audio_stereo_buffer, frames);
   }
   else
      audio_batch_cb(audio_buffer, frames);
}

static void extract_basename(char *buf, const char *path, size_t size)
{
   const char *base = strrchr(path, '/');
   if (!base)
      base = strrchr(path, '\\');
   if (!base)
      base = path;

   if (*base == '\\' || *base == '/')
      base++;

   strncpy(buf, base, size - 1);
   buf[size - 1] = '\0';

   char *ext = strrchr(buf, '.');
   if (ext)
      *ext = '\0';
}

static void extract_directory(char *buf, const char *path, size_t size)
{
   strncpy(buf, path, size - 1);
   buf[size - 1] = '\0';

   char *base = strrchr(buf, '/');
   if (!base)
      base = strrchr(buf, '\\');

   if (base)
      *base = '\0';
   else
      buf[0] = '\0';
}


bool retro_load_game(const struct retro_game_info *info)
{
   const char *dir;
   char nestopia_dir[PATH_MAX_LENGTH];
   char db_path[PATH_MAX_LENGTH];
   char palette_path[PATH_MAX_LENGTH];

   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Turbo A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Turbo B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "(FDS) Disk Side Change" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "(FDS) Eject Disk" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "(VSSystem) Coin 1" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "(VSSystem) Coin 2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "(Famicom) Microphone" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,   "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,    "Start" },

      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "B" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "A" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Turbo A" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Turbo B" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "(FDS) Disk Side Change" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "(FDS) Eject Disk" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,   "Select" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,    "Start" },

      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "B" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "A" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Turbo A" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Turbo B" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "(FDS) Disk Side Change" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "(FDS) Eject Disk" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,   "Select" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,    "Start" },

      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "B" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "A" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Turbo A" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Turbo B" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "(FDS) Disk Side Change" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "(FDS) Eject Disk" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,   "Select" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,    "Start" },

      { 0 },
   };

#ifdef _3DS
   video_buffer = (uint32_t*)linearMemAlign(Api::Video::Output::NTSC_WIDTH * Api::Video::Output::HEIGHT * sizeof(uint32_t), 0x80);
#else
   video_buffer = (uint32_t*)malloc(Api::Video::Output::NTSC_WIDTH * Api::Video::Output::HEIGHT * sizeof(uint32_t));
#endif

   machine = new Api::Machine(emulator);
   input = new Api::Input::Controllers;
   Api::User::fileIoCallback.Set(file_io_callback, 0);

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   if (!environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) || !dir)
      return false;

   fill_pathname_join_special(nestopia_dir, dir, "nestopia", sizeof(nestopia_dir));
   fill_pathname_join_special(samp_dir, nestopia_dir, "samples", sizeof(samp_dir));

   fill_pathname_join_special(palette_path, dir, "custom.pal", sizeof(palette_path));

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "Custom palette path: %s\n", palette_path);
   
   RFILE *custompalette = filestream_open(palette_path,
         RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);

   if (custompalette)
   {
      filestream_read(custompalette, custpal, sizeof(custpal));
      filestream_close(custompalette);
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "custom.pal loaded from system directory.\n");
   }
   else
   {
      memcpy(custpal, royaltea_palette, sizeof(custpal));
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "custom.pal not found in system directory.\n");
   }

   fill_pathname_join_special(db_path, dir, "NstDatabase.xml", sizeof(db_path));

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "NstDatabase.xml path: %s\n", db_path);
   
   Api::Cartridge::Database database(emulator);

   char *db_data       = NULL;
   int64_t db_file_len = 0;

   if (filestream_read_file(db_path, (void**)&db_data, &db_file_len))
   {
      std::istringstream db_external(
            std::string(db_data, (size_t)db_file_len),
            std::istringstream::in | std::istringstream::binary);
      database.Load(db_external);
      free(db_data);
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Using external XML database\n");
   }
   else
   {
      size_t db_size = sizeof(nst_db_xml)/sizeof(unsigned char);
      std::string db_buf((const char*)nst_db_xml, db_size);
      std::istringstream db_baked(db_buf);
      database.Load(db_baked);
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Using baked in XML database\n");
   }

   database.Enable(true);
   if (info->path != NULL)
   {
      extract_basename(g_basename, info->path, sizeof(g_basename));
      extract_directory(g_rom_dir, info->path, sizeof(g_rom_dir));
   }
   
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "XRGB8888 is not supported.\n");
      return false;
   }
   
   std::stringstream ss(std::string(reinterpret_cast<const char*>(info->data),
            reinterpret_cast<const char*>(info->data) + info->size));

   if (info->path && (strstr(info->path, ".fds") || strstr(info->path, ".FDS")))
   {
      fds = new Api::Fds(emulator);

      if (fds)
      {
         char fds_bios_path[PATH_MAX_LENGTH];
         char *bios_data     = NULL;
         int64_t bios_size   = 0;

         /* search for BIOS in system directory */
         fill_pathname_join_special(fds_bios_path, dir, "disksys.rom", sizeof(fds_bios_path));
         if (log_cb)
            log_cb(RETRO_LOG_INFO, "FDS BIOS path: %s\n", fds_bios_path);

         if (!filestream_read_file(fds_bios_path, (void**)&bios_data, &bios_size))
            return false;

         {
            std::istringstream fds_bios_stream(
                  std::string(bios_data, (size_t)bios_size),
                  std::istringstream::in | std::istringstream::binary);
            fds->SetBIOS(&fds_bios_stream);
         }
         free(bios_data);
      }
      else
         return false;
   }
   
   if (!environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &g_save_dir))
   {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "Could not find save directory.\n");
   }

   is_pal = false;
   check_variables();

   if (machine->Load(ss, favsystem))
      return false;

   Api::Video ivideo(emulator);
   ivideo.SetSharpness(Api::Video::DEFAULT_SHARPNESS_RGB);
   ivideo.SetColorResolution(Api::Video::DEFAULT_COLOR_RESOLUTION_RGB);
   ivideo.SetColorBleed(Api::Video::DEFAULT_COLOR_BLEED_RGB);
   ivideo.SetColorArtifacts(Api::Video::DEFAULT_COLOR_ARTIFACTS_RGB);
   ivideo.SetColorFringing(Api::Video::DEFAULT_COLOR_FRINGING_RGB);

   Api::Video::RenderState state;
   state.filter = Api::Video::RenderState::FILTER_NONE;
   state.width = 256;
   state.height = 240;
   state.bits.count = 32;
   state.bits.mask.r = 0x00ff0000;
   state.bits.mask.g = 0x0000ff00;
   state.bits.mask.b = 0x000000ff;
   ivideo.SetRenderState(state);

   Api::Sound isound(emulator);
   isound.SetSampleRate(SAMPLERATE);
   isound.SetSpeaker(Api::Sound::SPEAKER_MONO);

   Api::Input(emulator).AutoSelectControllers();
   Api::Input::Controllers::Pad::callback.Set(&gamepad_callback, NULL);
   Api::Input::Controllers::Paddle::callback.Set(&arkanoid_callback, NULL);
   Api::Input::Controllers::VsSystem::callback.Set(&vssystem_callback, NULL);
   Api::Input::Controllers::Zapper::callback.Set(&zapper_callback, NULL);

   Api::User::eventCallback.Set(nst_cb_event, 0);

   machine->Power(true);

   check_variables();

   if (fds_auto_insert && machine->Is(Nes::Api::Machine::DISK))
      fds->InsertDisk(0, 0);
   
   video = new Api::Video::Output(video_buffer, video_width * sizeof(uint32_t));
   
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[Nestopia]: Machine is %s.\n", is_pal ? "PAL" : "NTSC");

   return true;
}

void retro_unload_game(void)
{
   Api::Input::Controllers::Pad::callback.Unset();
   Api::Input::Controllers::Paddle::callback.Unset();
   Api::Input::Controllers::VsSystem::callback.Unset();
   Api::Input::Controllers::Zapper::callback.Unset();

   if (machine)
   {
      machine->Unload();

      if (machine->Is(Nes::Api::Machine::DISK))
      {
         if (fds)
            delete fds;
         fds = 0;
      }

      delete machine;
   }

   if (video)
      delete video;
   if (audio)
      delete audio;
   if (input)
      delete input;

   machine = 0;
   video   = 0;
   audio   = 0;
   input   = 0;

   sram = 0;
   sram_size = 0;
   state_size = 0;

#ifdef _3DS
   linearFree(video_buffer);
#else
   free(video_buffer);
#endif
   video_buffer = NULL;
}

unsigned retro_get_region(void)
{
   return is_pal ? RETRO_REGION_PAL : RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned, const struct retro_game_info *, size_t)
{
   return false;
}

size_t retro_serialize_size(void)
{
   if (!state_size) {
      std::stringstream ss;
      if (machine->SaveState(ss, Api::Machine::NO_COMPRESSION))
         return 0;
      state_size = ss.str().size() + tracked_input_state_size_bytes;
   }

   return state_size;
}

bool retro_serialize(void *data, size_t size)
{
   std::stringstream ss;
   if (machine->SaveState(ss, Api::Machine::NO_COMPRESSION))
      return false;

   std::string state = ss.str();
   if (state.size() + tracked_input_state_size_bytes > size)
      return false;

   std::copy(state.begin(), state.end(), reinterpret_cast<char*>(data));

   unsigned char *tracked_input_state_ptr = reinterpret_cast<unsigned char*>(data) + state.size();

   *tracked_input_state_ptr++ = tstate[0];
   *tracked_input_state_ptr++ = tstate[1];
   *tracked_input_state_ptr++ = tstate[2];
   *tracked_input_state_ptr++ = tstate[3];
   *tracked_input_state_ptr++ = (unsigned char) cur_x;
   *tracked_input_state_ptr++ = (unsigned char) cur_y;
   *tracked_input_state_ptr++ = prevL;
   *tracked_input_state_ptr++ = prevR;
   *tracked_input_state_ptr++ = (unsigned char)(audio_frac       & 0xff);
   *tracked_input_state_ptr++ = (unsigned char)((audio_frac >> 8) & 0xff);
   *tracked_input_state_ptr++ = (unsigned char)((audio_frac >> 16) & 0xff);
   *tracked_input_state_ptr++ = (unsigned char)((audio_frac >> 24) & 0xff);

   return true;
}

bool retro_unserialize(const void *data, size_t size)
{
   // Footer size detection: current states carry the full footer,
   // states from the 8-byte-footer era carry 4 bytes less, and legacy
   // states carry no footer at all.
   size_t expected = retro_serialize_size();
   size_t footer   = 0;

   if (size >= expected)
      footer = tracked_input_state_size_bytes;
   else if (size + 4 >= expected)
      footer = tracked_input_state_size_bytes - 4;

   size_t nestopia_savestate_size = size - footer;

   std::stringstream ss(std::string(reinterpret_cast<const char*>(data),
      reinterpret_cast<const char*>(data) + nestopia_savestate_size));

   if (footer >= 8) {
      unsigned char const *tracked_input_state_ptr =
         reinterpret_cast<unsigned char const*>(data) + nestopia_savestate_size;
      tstate[0] = *tracked_input_state_ptr++;
      tstate[1] = *tracked_input_state_ptr++;
      tstate[2] = *tracked_input_state_ptr++;
      tstate[3] = *tracked_input_state_ptr++;
      cur_x  = (int) *tracked_input_state_ptr++;
      cur_y  = (int) *tracked_input_state_ptr++;
      prevL  = *tracked_input_state_ptr++;
      prevR  = *tracked_input_state_ptr++;

      if (footer >= 12) {
         audio_frac  = (unsigned long)*tracked_input_state_ptr++;
         audio_frac |= (unsigned long)*tracked_input_state_ptr++ << 8;
         audio_frac |= (unsigned long)*tracked_input_state_ptr++ << 16;
         audio_frac |= (unsigned long)*tracked_input_state_ptr++ << 24;
      }
      else
         audio_frac = 0;

      /* Guard against a state saved under the other region's
       * denominator. */
      if (audio_spf_den && audio_frac >= audio_spf_den)
         audio_frac %= audio_spf_den;
   }

   return !machine->LoadState(ss);
}

void *retro_get_memory_data(unsigned id)
{
   Core::Machine& machineGet = emulator;
   switch(id)
   {
      case RETRO_MEMORY_SAVE_RAM:
      return sram;
       
      case RETRO_MEMORY_SYSTEM_RAM:
      return (void*)&machineGet.cpu.GetRam()[0];
       
   }

   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   Core::Machine& machineGet = emulator;
   switch(id)
   {
      case RETRO_MEMORY_SAVE_RAM:
         return sram_size;
       
      case RETRO_MEMORY_SYSTEM_RAM:
         return machineGet.cpu.RAM_SIZE;
   }

   return 0;
}

void retro_cheat_reset(void)
{
   Nes::Api::Cheats cheater(emulator);
   cheater.ClearCodes();
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   Nes::Api::Cheats cheater(emulator);
   Nes::Api::Cheats::Code ggCode;
   char codeCopy[256];
   char *part;

   if (code == NULL) return;
   strcpy(codeCopy,code);
   part = strtok(codeCopy,"+,;._ ");

   while (part)
   {
      if ((strlen(part) == 7) && (part[4]==':'))
      {
         part[4]='\0';
         ggCode.address=strtoul(part,NULL,16);
         ggCode.value=strtoul(part+5,NULL,16);
         cheater.SetCode(ggCode);
      }
      else if ((strlen(part)==10) && (part[4]=='?') && (part[7]==':'))
      {
         part[4]='\0';
         part[7]='\0';
         ggCode.address=strtoul(part,NULL,16);
         ggCode.compare=strtoul(part+5,NULL,16);
         ggCode.useCompare=true;
         ggCode.value=strtoul(part+8,NULL,16);
         cheater.SetCode(ggCode);
      }
      else if (Nes::Api::Cheats::GameGenieDecode(part, ggCode) == RESULT_OK)
         cheater.SetCode(ggCode);
      else if (Nes::Api::Cheats::ProActionRockyDecode(part, ggCode) == RESULT_OK)
         cheater.SetCode(ggCode);
      part = strtok(NULL,"+,;._ ");
   }
}
