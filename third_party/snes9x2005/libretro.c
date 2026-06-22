#include "copyright"

#include "snes9x.h"
#include "soundux.h"
#include "memmap.h"
#include "apu.h"
#include "cheats.h"
#include "display.h"
#include "gfx.h"
#include "cpuexec.h"
#include "spc7110.h"
#include "srtc.h"
#include "sa1.h"

#ifdef PSP
#include <pspkernel.h>
#include <pspgu.h>
#endif

#include <libretro.h>
#include <retro_miscellaneous.h>

#ifndef LOAD_FROM_MEMORY
#include <streams/file_stream.h>
#endif

#include "libretro_core_options.h"

#ifdef _3DS
void* linearMemAlign(size_t size, size_t alignment);
void linearFree(void* mem);
#endif

static retro_log_printf_t log_cb = NULL;
static retro_video_refresh_t video_cb = NULL;
static retro_input_poll_t poll_cb = NULL;
static retro_input_state_t input_cb = NULL;
static retro_audio_sample_batch_t audio_batch_cb = NULL;
static retro_environment_t environ_cb = NULL;
struct retro_perf_callback perf_cb;

static bool libretro_supports_option_categories = false;
static bool libretro_supports_bitmasks = false;

char retro_save_directory[PATH_MAX_LENGTH];
char retro_base_name[PATH_MAX_LENGTH];
bool overclock_cycles = false;
bool reduce_sprite_flicker = false;
int one_c, slow_one_c, two_c;

#define VIDEO_REFRESH_RATE_PAL  (SNES_CLOCK_SPEED * 6.0 / (SNES_CYCLES_PER_SCANLINE * SNES_MAX_PAL_VCOUNTER))
#define VIDEO_REFRESH_RATE_NTSC (SNES_CLOCK_SPEED * 6.0 / (SNES_CYCLES_PER_SCANLINE * SNES_MAX_NTSC_VCOUNTER))
#define AUDIO_SAMPLE_RATE       32040

static int16_t *audio_out_buffer       = NULL;
#ifdef USE_BLARGG_APU
static size_t audio_out_buffer_size    = 0;
static size_t audio_out_buffer_pos     = 0;
static size_t audio_batch_frames_max   = (1 << 16);
#else
static float audio_samples_per_frame   = 0.0f;
static float audio_samples_accumulator = 0.0f;
#endif

#ifndef USE_BLARGG_APU
static bool audio_low_pass_enabled  = false;
static int32_t audio_low_pass_range = (60 * 65536) / 100;
#endif

static unsigned frameskip_type             = 0;
static unsigned frameskip_threshold        = 0;
static uint16_t frameskip_counter          = 0;

static bool retro_audio_buff_active        = false;
static unsigned retro_audio_buff_occupancy = 0;
static bool retro_audio_buff_underrun      = false;
/* Maximum number of consecutive frames that
 * can be skipped */
#define FRAMESKIP_MAX 30

static unsigned retro_audio_latency        = 0;
static bool update_audio_latency           = false;

#ifdef PERF_TEST
#define RETRO_PERFORMANCE_INIT(name) \
    retro_perf_tick_t current_ticks; \
    static struct retro_perf_counter name = {#name}; \
    if (!name.registered) \
       perf_cb.perf_register(&(name)); \
    current_ticks = name.total

#define RETRO_PERFORMANCE_START(name) \
    perf_cb.perf_start(&(name))

#define RETRO_PERFORMANCE_STOP(name) \
    perf_cb.perf_stop(&(name)); \
    current_ticks = name.total - current_ticks;
#else
#define RETRO_PERFORMANCE_INIT(name)
#define RETRO_PERFORMANCE_START(name)
#define RETRO_PERFORMANCE_STOP(name)
#endif

void retro_set_environment(retro_environment_t cb)
{
   struct retro_log_callback log;
#ifndef LOAD_FROM_MEMORY
   struct retro_vfs_interface_info vfs_iface_info;
#endif
   environ_cb = cb;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   libretro_supports_option_categories = false;
   libretro_set_core_options(environ_cb,
		   &libretro_supports_option_categories);
   environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb);

#ifndef LOAD_FROM_MEMORY
   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
      filestream_vfs_init(&vfs_iface_info);
#endif
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   (void) cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_cb = cb;
}

void retro_set_controller_port_device(unsigned in_port, unsigned device)
{
   (void) in_port;
   (void) device;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

static void retro_audio_buff_status_cb(
      bool active, unsigned occupancy, bool underrun_likely)
{
   retro_audio_buff_active    = active;
   retro_audio_buff_occupancy = occupancy;
   retro_audio_buff_underrun  = underrun_likely;
}

static void retro_set_audio_buff_status_cb(void)
{
   if (frameskip_type > 0)
   {
      struct retro_audio_buffer_status_callback buf_status_cb;

      buf_status_cb.callback = retro_audio_buff_status_cb;
      if (!environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK,
            &buf_status_cb))
      {
         if (log_cb)
            log_cb(RETRO_LOG_WARN, "Frameskip disabled - frontend does not support audio buffer status monitoring.\n");

         retro_audio_buff_active    = false;
         retro_audio_buff_occupancy = 0;
         retro_audio_buff_underrun  = false;
         retro_audio_latency        = 0;
      }
      else
      {
         /* Frameskip is enabled - increase frontend
          * audio latency to minimise potential
          * buffer underruns */
         uint32_t frame_time_usec = Settings.FrameTime;

         if (Settings.ForceNTSC)
            frame_time_usec = Settings.FrameTimeNTSC;
         if (Settings.ForcePAL)
            frame_time_usec = Settings.FrameTimePAL;

         /* Set latency to 6x current frame time... */
         retro_audio_latency = (unsigned)(6 * frame_time_usec / 1000);

         /* ...then round up to nearest multiple of 32 */
         retro_audio_latency = (retro_audio_latency + 0x1F) & ~0x1F;
      }
   }
   else
   {
      environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK, NULL);
      retro_audio_latency = 0;
   }

   update_audio_latency = true;
}

void S9xDeinitDisplay(void)
{
#ifdef DS2_DMA
   if (GFX.Screen_buffer)
      AlignedFree(GFX.Screen, PtrAdj.GFXScreen);
#elif defined(_3DS)
   if (GFX.Screen_buffer)
      linearFree(GFX.Screen_buffer);
#else
   if (GFX.Screen_buffer)
      free(GFX.Screen_buffer);
#endif
   if (GFX.SubScreen_buffer)
      free(GFX.SubScreen_buffer);
   if (GFX.ZBuffer_buffer)
      free(GFX.ZBuffer_buffer);
   if (GFX.SubZBuffer_buffer)
      free(GFX.SubZBuffer_buffer);

   GFX.Screen = NULL;
   GFX.Screen_buffer = NULL;
   GFX.SubScreen = NULL;
   GFX.SubScreen_buffer = NULL;
   GFX.ZBuffer = NULL;
   GFX.ZBuffer_buffer = NULL;
   GFX.SubZBuffer = NULL;
   GFX.SubZBuffer_buffer = NULL;
}

void S9xInitDisplay(void)
{
   int32_t h = IMAGE_HEIGHT;
   int32_t safety = 32;

   GFX.Pitch = IMAGE_WIDTH * 2;
#ifdef DS2_DMA
   GFX.Screen_buffer = (uint8_t *) AlignedMalloc(GFX.Pitch * h + safety, 32, &PtrAdj.GFXScreen);
#elif defined(_3DS)
   safety = 0x80;
   GFX.Screen_buffer = (uint8_t *) linearMemAlign(GFX.Pitch * h + safety, 0x80);
#else
   GFX.Screen_buffer = (uint8_t *) malloc(GFX.Pitch * h + safety);
#endif
   GFX.SubScreen_buffer = (uint8_t *) malloc(GFX.Pitch * h + safety);
   GFX.ZBuffer_buffer = (uint8_t *) malloc((GFX.Pitch >> 1) * h + safety);
   GFX.SubZBuffer_buffer = (uint8_t *) malloc((GFX.Pitch >> 1) * h + safety);

   GFX.Screen = GFX.Screen_buffer + safety;
   GFX.SubScreen = GFX.SubScreen_buffer + safety;
   GFX.ZBuffer = GFX.ZBuffer_buffer + safety;
   GFX.SubZBuffer = GFX.SubZBuffer_buffer + safety;

   GFX.Delta = (GFX.SubScreen - GFX.Screen) >> 1;
}

static void init_sfc_setting(void)
{
   memset(&Settings, 0, sizeof(Settings));
   Settings.JoystickEnabled = false;
   Settings.SoundPlaybackRate = AUDIO_SAMPLE_RATE;
#ifdef USE_BLARGG_APU
   Settings.SoundInputRate = AUDIO_SAMPLE_RATE;
#endif
   Settings.CyclesPercentage = 100;

   Settings.DisableSoundEcho = false;
   Settings.InterpolatedSound = true;
   Settings.APUEnabled = true;

   Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
   Settings.FrameTimePAL = 20000;
   Settings.FrameTimeNTSC = 16667;
   Settings.DisableMasterVolume = false;
   Settings.Mouse = true;
   Settings.SuperScope = true;
   Settings.MultiPlayer5 = true;
   Settings.ControllerOption = SNES_JOYPAD;
#ifdef USE_BLARGG_APU
   Settings.SoundSync = false;
#endif
   Settings.ApplyCheats = true;
   Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;
}

static void audio_out_buffer_init(void)
{
   float refresh_rate        = (float)((Settings.PAL) ?
         VIDEO_REFRESH_RATE_PAL : VIDEO_REFRESH_RATE_NTSC);
   float samples_per_frame   = (float)AUDIO_SAMPLE_RATE / refresh_rate;
   size_t buffer_size        = ((size_t)samples_per_frame + 1) << 1;

   audio_out_buffer          = (int16_t *)malloc(buffer_size * sizeof(int16_t));
#ifdef USE_BLARGG_APU
   audio_out_buffer_size     = buffer_size;
   audio_out_buffer_pos      = 0;
   audio_batch_frames_max    = (1 << 16);
#else
   audio_samples_per_frame   = samples_per_frame;
   audio_samples_accumulator = 0.0f;
#endif
}

static void audio_out_buffer_deinit(void)
{
   if (audio_out_buffer)
      free(audio_out_buffer);
   audio_out_buffer = NULL;

#ifdef USE_BLARGG_APU
   audio_out_buffer_size     = 0;
   audio_out_buffer_pos      = 0;
   audio_batch_frames_max    = (1 << 16);
#else
   audio_samples_per_frame   = 0.0f;
   audio_samples_accumulator = 0.0f;
#endif
}

#ifdef USE_BLARGG_APU
static void S9xAudioCallback(void)
{
   size_t available_samples;
   size_t buffer_capacity = audio_out_buffer_size -
         audio_out_buffer_pos;

   S9xFinalizeSamples();
   available_samples = S9xGetSampleCount();

   if (buffer_capacity < available_samples)
   {
      int16_t *tmp_buffer = NULL;
      size_t tmp_buffer_size;
      size_t i;

      tmp_buffer_size = audio_out_buffer_size + (available_samples - buffer_capacity);
      tmp_buffer_size = (tmp_buffer_size << 1) - (tmp_buffer_size >> 1);
      tmp_buffer      = (int16_t *)malloc(tmp_buffer_size * sizeof(int16_t));

      for (i = 0; i < audio_out_buffer_pos; i++)
         tmp_buffer[i] = audio_out_buffer[i];

      free(audio_out_buffer);

      audio_out_buffer      = tmp_buffer;
      audio_out_buffer_size = tmp_buffer_size;
   }

   S9xMixSamples(audio_out_buffer + audio_out_buffer_pos,
         available_samples);
   audio_out_buffer_pos += available_samples;
}
#endif

static void audio_upload_samples(void)
{
   size_t available_frames;
#ifdef USE_BLARGG_APU
   int16_t *audio_out_buffer_ptr;

   S9xAudioCallback();

   audio_out_buffer_ptr = audio_out_buffer;
   available_frames     = audio_out_buffer_pos >> 1;

   /* Since the audio output buffer can
    * (theoretically) have an arbitrarily size,
    * we must write it in chunks of the largest
    * size supported by the frontend
    * (this is not required for the non-blargg
    * code path, since the buffer size has well
    * defined limits in that case) */
   while (available_frames > 0)
   {
      size_t frames_to_write = (available_frames >
            audio_batch_frames_max) ?
                  audio_batch_frames_max :
                  available_frames;
      size_t frames_written = audio_batch_cb(
            audio_out_buffer_ptr, frames_to_write);

      if ((frames_written < frames_to_write) &&
          (frames_written > 0))
         audio_batch_frames_max = frames_written;

      available_frames     -= frames_to_write;
      audio_out_buffer_ptr += frames_to_write << 1;
   }

   audio_out_buffer_pos = 0;
#else
   available_frames           = (size_t)audio_samples_per_frame;
   audio_samples_accumulator += audio_samples_per_frame -
         (float)available_frames;

   if (audio_samples_accumulator > 1.0f)
   {
      available_frames          += 1;
      audio_samples_accumulator -= 1.0f;
   }

   if (audio_low_pass_enabled)
      S9xMixSamplesLowPass(audio_out_buffer,
            available_frames << 1, audio_low_pass_range);
   else
      S9xMixSamples(audio_out_buffer, available_frames << 1);

   audio_batch_cb(audio_out_buffer, available_frames);
#endif
}

void retro_init(void)
{
   struct retro_log_callback log;
   enum retro_pixel_format rgb565;
   bool achievements = true;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   /* State that the core supports achievements. */
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &achievements);

   rgb565 = RETRO_PIXEL_FORMAT_RGB565;
   if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565) && log_cb)
      log_cb(RETRO_LOG_INFO, "Frontend supports RGB565 - will use that instead of XRGB1555.\n");

   init_sfc_setting();
   S9xInitMemory();
   S9xInitAPU();
   S9xInitDisplay();
   S9xInitGFX();
#ifdef USE_BLARGG_APU
   S9xInitSound(0, 0); /* Use default values */
   S9xSetSamplesAvailableCallback(S9xAudioCallback);
#else
   S9xInitSound();
#endif
   CPU.SaveStateVersion = 0;

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;
}

void retro_deinit(void)
{
   if (Settings.SPC7110)
      Del7110Gfx();

   S9xDeinitGFX();
   S9xDeinitDisplay();
   S9xDeinitAPU();
   S9xDeinitMemory();

#ifdef PERF_TEST
   perf_cb.perf_log();
#endif

   audio_out_buffer_deinit();

   /* Reset globals (required for static builds) */
   libretro_supports_option_categories = false;
   libretro_supports_bitmasks = false;
   frameskip_type             = 0;
   frameskip_threshold        = 0;
   frameskip_counter          = 0;
#ifndef USE_BLARGG_APU
   audio_low_pass_enabled     = false;
   audio_low_pass_range       = (60 * 65536) / 100;
#endif
   retro_audio_buff_active    = false;
   retro_audio_buff_occupancy = 0;
   retro_audio_buff_underrun  = false;
   retro_audio_latency        = 0;
   update_audio_latency       = false;
}

uint32_t S9xReadJoypad(int32_t port)
{
   static const uint32_t snes_lut[] =
   {
      SNES_B_MASK,
      SNES_Y_MASK,
      SNES_SELECT_MASK,
      SNES_START_MASK,
      SNES_UP_MASK,
      SNES_DOWN_MASK,
      SNES_LEFT_MASK,
      SNES_RIGHT_MASK,
      SNES_A_MASK,
      SNES_X_MASK,
      SNES_TL_MASK,
      SNES_TR_MASK
   };

   int32_t i;
   uint32_t joypad = 0;
   uint32_t joy_bits = 0;

   if (libretro_supports_bitmasks)
      joy_bits = input_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
   else
   {
      for (i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3+1); i++)
         joy_bits |= input_cb(port, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
   }

   for (i = RETRO_DEVICE_ID_JOYPAD_B; i <= RETRO_DEVICE_ID_JOYPAD_R; i++)
      if (joy_bits & (1 << i))
         joypad |= snes_lut[i];

   return joypad;
}

static void check_variables(bool first_run)
{
   struct retro_variable var;
   bool prev_frameskip_type;

   if (first_run)
   {
      var.key = "snes9x_2005_region";
      var.value = NULL;

      Settings.ForceNTSC = 0;
      Settings.ForcePAL  = 0;

      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      {
         Settings.ForceNTSC = !strcmp(var.value, "NTSC");
         Settings.ForcePAL  = !strcmp(var.value, "PAL");
      }
   }

   var.key = "snes9x_2005_frameskip";
   var.value = NULL;

   prev_frameskip_type = frameskip_type;
   frameskip_type      = 0;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "auto") == 0)
         frameskip_type = 1;
      else if (strcmp(var.value, "manual") == 0)
         frameskip_type = 2;
   }

   var.key = "snes9x_2005_frameskip_threshold";
   var.value = NULL;

   frameskip_threshold = 33;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      frameskip_threshold = strtol(var.value, NULL, 10);

#ifndef USE_BLARGG_APU
   var.key = "snes9x_2005_low_pass_filter";
   var.value = NULL;

   audio_low_pass_enabled = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      if (strcmp(var.value, "enabled") == 0)
         audio_low_pass_enabled = true;

   var.key = "snes9x_2005_low_pass_range";
   var.value = NULL;

   audio_low_pass_range = (60 * 0x10000) / 100;

	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      audio_low_pass_range = (strtol(var.value, NULL, 10) * 0x10000) / 100;
#endif

   var.key = "snes9x_2005_overclock_cycles";
   var.value = NULL;

   overclock_cycles = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "compatible") == 0)
      {
         overclock_cycles = true;
         one_c = 4;
         slow_one_c = 5;
         two_c = 6;
      }
      else if (strcmp(var.value, "max") == 0)
      {
         overclock_cycles = true;
         one_c = 3;
         slow_one_c = 3;
         two_c = 3;
      }
   }

   var.key = "snes9x_2005_reduce_sprite_flicker";
   var.value = NULL;

   reduce_sprite_flicker = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      if (strcmp(var.value, "enabled") == 0)
         reduce_sprite_flicker = true;

   /* Reinitialise frameskipping, if required */
   if (!first_run &&
       (frameskip_type != prev_frameskip_type))
      retro_set_audio_buff_status_cb();
}

void retro_run(void)
{
   bool updated = false;
   int result;
   bool okay;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables(false);

#ifdef NO_VIDEO_OUTPUT
   video_cb(NULL, IPPU.RenderedScreenWidth, IPPU.RenderedScreenHeight, GFX.Pitch);
   IPPU.RenderThisFrame = false;
#endif

   result = -1;
   okay = environ_cb(RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE, &result);
   if (okay)
   {
      bool audioEnabled = 0 != (result & 2);
      bool videoEnabled = 0 != (result & 1);
      bool hardDisableAudio = 0 != (result & 8);
      IPPU.RenderThisFrame = videoEnabled;
#ifdef USE_BLARGG_APU
      S9xSetSoundMute(!audioEnabled || hardDisableAudio);
#endif
      Settings.HardDisableAudio = hardDisableAudio;
   }
   else
   {
      IPPU.RenderThisFrame = true;
#ifdef USE_BLARGG_APU
      S9xSetSoundMute(false);
#endif
      Settings.HardDisableAudio = false;
   }

   /* Check whether current frame should
    * be skipped */
   if ((frameskip_type > 0) &&
       retro_audio_buff_active &&
       IPPU.RenderThisFrame)
   {
      bool skip_frame;

      switch (frameskip_type)
      {
         case 1: /* auto */
            skip_frame = retro_audio_buff_underrun;
            break;
         case 2: /* manual */
            skip_frame = (retro_audio_buff_occupancy < frameskip_threshold);
            break;
         default:
            skip_frame = false;
            break;
      }

      if (skip_frame)
      {
         if(frameskip_counter < FRAMESKIP_MAX)
         {
            IPPU.RenderThisFrame = false;
            frameskip_counter++;
         }
         else
            frameskip_counter = 0;
      }
      else
         frameskip_counter = 0;
   }

   /* If frameskip/timing settings have changed,
    * update frontend audio latency
    * > Can do this before or after the frameskip
    *   check, but doing it after means we at least
    *   retain the current frame's audio output */
   if (update_audio_latency)
   {
      environ_cb(RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY,
            &retro_audio_latency);
      update_audio_latency = false;
   }

   poll_cb();

   RETRO_PERFORMANCE_INIT(S9xMainLoop_func);
   RETRO_PERFORMANCE_START(S9xMainLoop_func);
   S9xMainLoop();
   RETRO_PERFORMANCE_STOP(S9xMainLoop_func);

#ifdef NO_VIDEO_OUTPUT
   audio_upload_samples();
   return;
#endif

   if (IPPU.RenderThisFrame)
   {
#ifdef PSP
      static unsigned int __attribute__((aligned(16))) d_list[32];
      void* const texture_vram_p = (void*)(0x44200000 - (512 * 512)); /* max VRAM address - frame size */
      sceKernelDcacheWritebackRange(GFX.Screen, GFX.Pitch * IPPU.RenderedScreenHeight);
      sceGuStart(GU_DIRECT, d_list);
      sceGuCopyImage(GU_PSM_4444, 0, 0, IPPU.RenderedScreenWidth, IPPU.RenderedScreenHeight, GFX.Pitch >> 1, GFX.Screen, 0, 0, 512, texture_vram_p);
      sceGuTexSync();
      sceGuTexImage(0, 512, 512, 512, texture_vram_p);
      sceGuTexMode(GU_PSM_5551, 0, 0, GU_FALSE);
      sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
      sceGuDisable(GU_BLEND);
      sceGuFinish();
      video_cb(texture_vram_p, IPPU.RenderedScreenWidth, IPPU.RenderedScreenHeight, GFX.Pitch);
#else
      video_cb(GFX.Screen, IPPU.RenderedScreenWidth, IPPU.RenderedScreenHeight, GFX.Pitch);
#endif
   }
   else
      video_cb(NULL, IPPU.RenderedScreenWidth, IPPU.RenderedScreenHeight, GFX.Pitch);

   audio_upload_samples();
}

bool S9xReadMousePosition(int32_t which1, int32_t* x, int32_t* y, uint32_t* buttons)
{
   (void) which1;
   (void) x;
   (void) y;
   (void) buttons;
   return false;
}

bool S9xReadSuperScopePosition(int32_t* x, int32_t* y, uint32_t* buttons)
{
   (void) x;
   (void) y;
   (void) buttons;
   return true;
}

bool JustifierOffscreen(void)
{
   return false;
}

void JustifierButtons(uint32_t* justifiers)
{
   (void) justifiers;
}

unsigned retro_get_region(void)
{
   return Settings.PAL ? RETRO_REGION_PAL : RETRO_REGION_NTSC;
}

void retro_get_system_info(struct retro_system_info* info)
{
#ifdef LOAD_FROM_MEMORY
   info->need_fullpath    = false;
#else
   info->need_fullpath    = true;
#endif
   info->valid_extensions = "smc|fig|sfc|gd3|gd7|dx2|bsx|swc";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = "v1.36" GIT_VERSION;
#ifdef USE_BLARGG_APU
   info->library_name     = "Snes9x 2005 Plus";
#else
   info->library_name     = "Snes9x 2005";
#endif
   info->block_extract    = false;
}

void retro_get_system_av_info(struct retro_system_av_info* info)
{
   info->geometry.base_width   = 256;
   info->geometry.base_height  = 224;
   info->geometry.max_width    = 512;
   info->geometry.max_height   = 512;
   info->geometry.aspect_ratio = 4.0 / 3.0;

   info->timing.sample_rate    = AUDIO_SAMPLE_RATE;
   info->timing.fps            = (Settings.PAL) ?
         VIDEO_REFRESH_RATE_PAL : VIDEO_REFRESH_RATE_NTSC;
}

void retro_reset(void)
{
   CPU.Flags = 0;
   S9xSoftReset();
}

size_t retro_serialize_size(void)
{
   return sizeof(CPU) + sizeof(ICPU) + sizeof(PPU) + sizeof(DMA) +
          0x10000 + 0x20000 + 0x20000 + 0x8000 +
#ifndef USE_BLARGG_APU
          sizeof(APU) + sizeof(IAPU) + 0x10000 + sizeof(SoundData) +
#else
          SPC_SAVE_STATE_BLOCK_SIZE +
#endif
          sizeof(SA1) + sizeof(s7r) + sizeof(rtc_f9);
}

bool retro_serialize(void* data, size_t size)
{
   int32_t i;
   uint8_t* buffer = data;
   (void) size;
#ifdef LAGFIX
   S9xPackStatus();
#ifndef USE_BLARGG_APU
   S9xAPUPackStatus();
#endif
#endif
   S9xUpdateRTC();
   S9xSRTCPreSaveState();
   memcpy(buffer, &CPU, sizeof(CPU));
   buffer += sizeof(CPU);
   memcpy(buffer, &ICPU, sizeof(ICPU));
   buffer += sizeof(ICPU);
   memcpy(buffer, &PPU, sizeof(PPU));
   buffer += sizeof(PPU);
   memcpy(buffer, &DMA, sizeof(DMA));
   buffer += sizeof(DMA);
   memcpy(buffer, Memory.VRAM, 0x10000);
   buffer += 0x10000;
   memcpy(buffer, Memory.RAM, 0x20000);
   buffer += 0x20000;
   memcpy(buffer, Memory.SRAM, 0x20000);
   buffer += 0x20000;
   memcpy(buffer, Memory.FillRAM, 0x8000);
   buffer += 0x8000;
#ifndef USE_BLARGG_APU
   memcpy(buffer, &APU, sizeof(APU));
   buffer += sizeof(APU);
   memcpy(buffer, &IAPU, sizeof(IAPU));
   buffer += sizeof(IAPU);
   memcpy(buffer, IAPU.RAM, 0x10000);
   buffer += 0x10000;
   memcpy(buffer, &SoundData, sizeof(SoundData));
   buffer += sizeof(SoundData);
#else
   S9xAPUSaveState(buffer);
   buffer += SPC_SAVE_STATE_BLOCK_SIZE;
#endif

   SA1.Registers.PC = SA1.PC - SA1.PCBase;
   S9xSA1PackStatus();

   memcpy(buffer, &SA1, sizeof(SA1));
   buffer += sizeof(SA1);
   memcpy(buffer, &s7r, sizeof(s7r));
   buffer += sizeof(s7r);
   memcpy(buffer, &rtc_f9, sizeof(rtc_f9));

   return true;
}

bool retro_unserialize(const void* data, size_t size)
{
   const uint8_t* buffer = data;
#ifndef USE_BLARGG_APU
   uint8_t* IAPU_RAM_current = IAPU.RAM;
   uintptr_t IAPU_RAM_offset;
#endif
   uint32_t sa1_old_flags = SA1.Flags;
   SSA1 sa1_state;

   if (size != retro_serialize_size())
      return false;

   S9xReset();
   memcpy(&CPU, buffer, sizeof(CPU));
   buffer += sizeof(CPU);
   memcpy(&ICPU, buffer, sizeof(ICPU));
   buffer += sizeof(ICPU);
   memcpy(&PPU, buffer, sizeof(PPU));
   buffer += sizeof(PPU);
   memcpy(&DMA, buffer, sizeof(DMA));
   buffer += sizeof(DMA);
   memcpy(Memory.VRAM, buffer, 0x10000);
   buffer += 0x10000;
   memcpy(Memory.RAM, buffer, 0x20000);
   buffer += 0x20000;
   memcpy(Memory.SRAM, buffer, 0x20000);
   buffer += 0x20000;
   memcpy(Memory.FillRAM, buffer, 0x8000);
   buffer += 0x8000;
#ifndef USE_BLARGG_APU
   memcpy(&APU, buffer, sizeof(APU));
   buffer += sizeof(APU);
   memcpy(&IAPU, buffer, sizeof(IAPU));
   buffer += sizeof(IAPU);
   IAPU_RAM_offset = IAPU_RAM_current - IAPU.RAM;
   IAPU.PC += IAPU_RAM_offset;
   IAPU.DirectPage += IAPU_RAM_offset;
   IAPU.WaitAddress1 += IAPU_RAM_offset;
   IAPU.WaitAddress2 += IAPU_RAM_offset;
   IAPU.RAM = IAPU_RAM_current;
   memcpy(IAPU.RAM, buffer, 0x10000);
   buffer += 0x10000;
   memcpy(&SoundData, buffer, sizeof(SoundData));
   buffer += sizeof(SoundData);
#else
   S9xAPULoadState(buffer);
   buffer += SPC_SAVE_STATE_BLOCK_SIZE;
#endif

   memcpy(&sa1_state, buffer, sizeof(sa1_state));
   buffer += sizeof(sa1_state);

   /* SA1 state must be restored 'by hand' */
   SA1.Flags               = sa1_state.Flags;
   SA1.NMIActive           = sa1_state.NMIActive;
   SA1.IRQActive           = sa1_state.IRQActive;
   SA1.WaitingForInterrupt = sa1_state.WaitingForInterrupt;
   SA1.op1                 = sa1_state.op1;
   SA1.op2                 = sa1_state.op2;
   SA1.arithmetic_op       = sa1_state.arithmetic_op;
   SA1.sum                 = sa1_state.sum;
   SA1.overflow            = sa1_state.overflow;
   memcpy(&SA1.Registers, &sa1_state.Registers, sizeof(SA1.Registers));

   memcpy(&s7r, buffer, sizeof(s7r));
   buffer += sizeof(s7r);
   memcpy(&rtc_f9, buffer, sizeof(rtc_f9));

   S9xFixSA1AfterSnapshotLoad();
   SA1.Flags |= sa1_old_flags & (TRACE_FLAG);

   FixROMSpeed();
   IPPU.ColorsChanged = true;
   IPPU.OBJChanged = true;
   CPU.InDMA = false;
   S9xFixColourBrightness();
#ifndef USE_BLARGG_APU
   S9xAPUUnpackStatus();
   S9xFixSoundAfterSnapshotLoad();
#endif
   ICPU.ShiftedPB = ICPU.Registers.PB << 16;
   ICPU.ShiftedDB = ICPU.Registers.DB << 16;
   S9xSetPCBase(ICPU.ShiftedPB + ICPU.Registers.PC);
   S9xUnpackStatus();
   S9xFixCycles();
   S9xReschedule();
   return true;
}

void retro_cheat_reset(void)
{
   S9xDeleteCheats();
   S9xApplyCheats();
}

extern SCheatData Cheat;

void retro_cheat_set(unsigned index, bool enabled, const char* code)
{
   uint32_t address;
   uint8_t val;

   bool sram;
   uint8_t bytes[3];/* used only by GoldFinger, ignored for now */

   if (S9xGameGenieToRaw(code, &address, &val) && S9xProActionReplayToRaw(code, &address, &val) && S9xGoldFingerToRaw(code, &address, &sram, &val, bytes))
      return; /* bad code, ignore */
   if (index > Cheat.num_cheats)
      return; /* cheat added in weird order, ignore */
   if (index == Cheat.num_cheats)
      Cheat.num_cheats++;

   Cheat.c[index].address = address;
   Cheat.c[index].byte = val;
   Cheat.c[index].enabled = enabled;
   Cheat.c[index].saved = false; /* it'll be saved next time cheats run anyways */

   Settings.ApplyCheats = true;
   S9xApplyCheats();
}

static void init_descriptors(void)
{
   struct retro_input_descriptor desc[] =
   {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },

      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },

      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },

      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },

      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },

      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
}

bool retro_load_game(const struct retro_game_info* game)
{
   if (!game)
      return false;

   CPU.Flags = 0;
   init_descriptors();
   check_variables(true);

#ifdef LOAD_FROM_MEMORY
   if (!LoadROM(game))
#else
   if (!LoadROM(game->path))
#endif
      return false;

   Settings.FrameTime = (Settings.PAL ? Settings.FrameTimePAL : Settings.FrameTimeNTSC);

   retro_set_audio_buff_status_cb();
   audio_out_buffer_init();
#ifndef USE_BLARGG_APU
   S9xSetPlaybackRate(Settings.SoundPlaybackRate);
#endif

   return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info, size_t num_info)
{
   (void) game_type;
   (void) info;
   (void) num_info;
   return false;
}

void retro_unload_game(void)
{
}

void* retro_get_memory_data(unsigned type)
{
   uint8_t* data;

   switch(type)
   {
      case RETRO_MEMORY_SAVE_RAM:
         data = Memory.SRAM;
         break;
      case RETRO_MEMORY_SYSTEM_RAM:
         data = Memory.RAM;
         break;
      case RETRO_MEMORY_VIDEO_RAM:
         data = Memory.VRAM;
         break;
      default:
         data = NULL;
         break;
   }

   return data;
}

size_t retro_get_memory_size(unsigned type)
{
   uint32_t size;

   switch(type)
   {
      case RETRO_MEMORY_SAVE_RAM:
         size = (uint32_t) (Memory.SRAMSize ? (1 << (Memory.SRAMSize + 3)) * 128 : 0);
         if (size > 0x20000)
            size = 0x20000;
         break;
      case RETRO_MEMORY_RTC:
         size = (Settings.SRTC || Settings.SPC7110RTC) ? 20 : 0;
         break;
      case RETRO_MEMORY_SYSTEM_RAM:
         size = 128 * 1024;
         break;
      case RETRO_MEMORY_VIDEO_RAM:
         size = 64 * 1024;
         break;
      default:
         size = 0;
         break;
   }

   return size;
}
