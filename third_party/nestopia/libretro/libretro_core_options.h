#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <retro_inline.h>

#ifndef HAVE_NO_LANGEXTRA
#include "libretro_core_options_intl.h"
#endif

/*
 ********************************
 * VERSION: 2.0
 ********************************
 *
 * - 2.0: Add support for core options v2 interface
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition */


struct retro_core_option_v2_category option_cats_us[] = {
   {
      "system",
      "System",
      "Change system region and other hardware-related settings."
   },
   {
      "video",
      "Video",
      "Change aspect ratio, display cropping, color palette and video filter settings."
   },
   {
      "audio",
      "Audio",
      "Change audio effects and channel volumes."
   },
   {
      "input",
      "Input",
      "Change input devices and other input related settings."
   },
   {
      "hacks",
      "Emulation Hacks",
      "Change processor overclocking and emulation accuracy settings affecting low-level performance and compatibility."
   },
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {

/* System */

   {
      "nestopia_favored_system",
      "System Region",
      NULL,
      "'Auto' will use the NstDatabase.xml database file for region autodetection. If there is no database present, it will default to NTSC.",
      NULL,
      "system",
      {
         { "auto",    "Auto" },
         { "ntsc",    "NTSC" },
         { "pal",     "PAL" },
         { "famicom", "Famicom" },
         { "dendy",   "Dendy" },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      "FDS Auto Insert",
      NULL,
      "Automatically insert first FDS disk on reset.",
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_fds_savefile_format",
      "FDS Savefile Format",
      NULL,
      "Define which format will be used by savefiles generated for FDS games. Based on the format chosen, FDS savefiles might be used interchangeably with other cores and standalone emulators. It is recommended to use with caution, as improper handling may cause undesired overwrites, or deletion of the existing saves.",
      NULL,
      "system",
      {
         { "sav_ups", "SAV + UPS (Default)" },
         { "ups",     "UPS (Standalone Nestopia)" },
         { "ips",     "IPS (Mesen)" },
         { NULL, NULL },
      },
      "sav_ups"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      "Blargg NTSC Filter",
      NULL,
      "Enable Blargg NTSC filters.",
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  "Composite Video" },
         { "svideo",     "S-Video" },
         { "rgb",        "RGB SCART" },
         { "monochrome", "Monochrome" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      "Palette",
      NULL,
      "Color palette to be used. If 'Custom' is selected, the palette used will be taken from the 'custom.pal' file placed in the RetroArch System/BIOS directory.",
      NULL,
      "video",
      {
         { "cxa2025as",            "CXA2025AS (US)" },
         { "cxa2025as_jp",         "CXA2025AS (JP)" },
         { "royaltea",             "Royaltea" },
         { "consumer",             "Consumer" },
         { "canonical",            "Canonical" },
         { "alternative",          "Alternative" },
         { "rgb",                  "RGB" },
         { "pal",                  "PAL" },
         { "digital-prime-fbx",    "Digital Prime FBX" },
         { "magnum-fbx",           "Magnum FBX" },
         { "smoothv2-fbx",         "Smooth V2 FBX" },
         { "composite-direct-fbx", "Composite Direct FBX" },
         { "pvm-style-d93-fbx",    "PVM-style D93 FBX" },
         { "ntsc-hardware-fbx",    "NTSC hardware FBX" },
         { "nes-classic-fbx-fs",   "NES Classic FBx (fixed)" },
         { "restored-wii-vc",      "Restored Wii VC" },
         { "wii-vc",               "Wii Virtual Console" },
         { "raw",                  "Raw" },
         { "custom",               "Custom" },
         { NULL, NULL },
      },
      "cxa2025as"
   },
   {
      "nestopia_overscan_v_top",
      "Mask Overscan (Top Vertical)",
      NULL,
      "Mask out (vertically) the potentially random glitchy video output that would have been hidden by the bezel around the edge of a standard-definition television screen.",
      NULL,
      "video",
      {
         { "0", NULL },
         { "1",  NULL },
         { "2",  NULL },
         { "3",  NULL },
         { "4",  NULL },
         { "5",  NULL },
         { "6",  NULL },
         { "7",  NULL },
         { "8",  NULL },
         { "9",  NULL },
         { "10",  NULL },
         { "11",  NULL },
         { "12",  NULL },
         { "13",  NULL },
         { "14",  NULL },
         { "15",  NULL },
         { "16",  NULL },
         { "17",  NULL },
         { "18",  NULL },
         { "19",  NULL },
         { "20",  NULL },
         { "21",  NULL },
         { "22",  NULL },
         { "23",  NULL },
         { "24",  NULL },
         { NULL, NULL },
      },
      "8"
   },
   {
      "nestopia_overscan_v_bottom",
      "Mask Overscan (Bottom Vertical)",
      NULL,
      "Mask out (vertically) the potentially random glitchy video output that would have been hidden by the bezel around the edge of a standard-definition television screen.",
      NULL,
      "video",
      {
         { "0", NULL },
         { "1",  NULL },
         { "2",  NULL },
         { "3",  NULL },
         { "4",  NULL },
         { "5",  NULL },
         { "6",  NULL },
         { "7",  NULL },
         { "8",  NULL },
         { "9",  NULL },
         { "10",  NULL },
         { "11",  NULL },
         { "12",  NULL },
         { "13",  NULL },
         { "14",  NULL },
         { "15",  NULL },
         { "16",  NULL },
         { "17",  NULL },
         { "18",  NULL },
         { "19",  NULL },
         { "20",  NULL },
         { "21",  NULL },
         { "22",  NULL },
         { "23",  NULL },
         { "24",  NULL },
         { NULL, NULL },
      },
      "8"
   },
   {
      "nestopia_overscan_h_left",
      "Mask Overscan (Left Horizontal)",
      NULL,
      "Mask out (horizontally) the potentially random glitchy video output that would have been hidden by the bezel around the edge of a standard-definition television screen.",
      NULL,
      "video",
      {
         { "0", NULL },
         { "1",  NULL },
         { "2",  NULL },
         { "3",  NULL },
         { "4",  NULL },
         { "5",  NULL },
         { "6",  NULL },
         { "7",  NULL },
         { "8",  NULL },
         { "9",  NULL },
         { "10",  NULL },
         { "11",  NULL },
         { "12",  NULL },
         { "13",  NULL },
         { "14",  NULL },
         { "15",  NULL },
         { "16",  NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "nestopia_overscan_h_right",
      "Mask Overscan (Right Horizontal)",
      NULL,
      "Mask out (horizontally) the potentially random glitchy video output that would have been hidden by the bezel around the edge of a standard-definition television screen.",
      NULL,
      "video",
      {
         { "0", NULL },
         { "1",  NULL },
         { "2",  NULL },
         { "3",  NULL },
         { "4",  NULL },
         { "5",  NULL },
         { "6",  NULL },
         { "7",  NULL },
         { "8",  NULL },
         { "9",  NULL },
         { "10",  NULL },
         { "11",  NULL },
         { "12",  NULL },
         { "13",  NULL },
         { "14",  NULL },
         { "15",  NULL },
         { "16",  NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "nestopia_aspect",
      "Preferred Aspect Ratio",
      NULL,
      "RetroArch's aspect ratio must be set to 'Core Provided' in the Video settings. 'Auto' will use the NstDatabase.xml database file for aspect ratio autodetection. If there is no database present, it will default to NTSC.",
      NULL,
      "video",
      {
         { "auto",        "Auto" },
         { "ntsc",        "NTSC" },
         { "pal",         "PAL" },
         { "4:3",         "4:3" },
         { "uncorrected", "Uncorrected" },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      "Game Genie Sound Distortion",
      NULL,
      "The Game Genie cheat device could inadvertently introduce sound distortion in games. By enabling this, you can simulate the distortion it would add to a game's sound.",
      NULL,
      "audio",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_show_advanced_av_settings",
      "Show Advanced Audio Settings (Reopen menu)",
      NULL,
      "Enable configuration of low-level audio channel parameters.",
      NULL,
      "audio",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "nestopia_audio_vol_sq1",
      "Square 1 Channel Volume %",
      NULL,
      "Modify Square 1 Channel Volume %.",
      NULL,
      "audio",
      {
         { "0", NULL },
         { "10", NULL },
         { "20", NULL },
         { "30", NULL },
         { "40", NULL },
         { "50", NULL },
         { "60", NULL },
         { "70", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "nestopia_audio_vol_sq2",
      "Square 2 Channel Volume %",
      NULL,
      "Modify Square 2 Channel Volume %.",
      NULL,
      "audio",
      {
         { "0", NULL },
         { "10", NULL },
         { "20", NULL },
         { "30", NULL },
         { "40", NULL },
         { "50", NULL },
         { "60", NULL },
         { "70", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "nestopia_audio_vol_tri",
      "Triangle Channel Volume %",
      NULL,
      "Modify Triangle Channel Volume %.",
      NULL,
      "audio",
      {
         { "0", NULL },
         { "10", NULL },
         { "20", NULL },
         { "30", NULL },
         { "40", NULL },
         { "50", NULL },
         { "60", NULL },
         { "70", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "nestopia_audio_vol_noise",
      "Noise Channel Volume %",
      NULL,
      "Modify Noise Channel Volume %.",
      NULL,
      "audio",
      {
         { "0", NULL },
         { "10", NULL },
         { "20", NULL },
         { "30", NULL },
         { "40", NULL },
         { "50", NULL },
         { "60", NULL },
         { "70", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "nestopia_audio_vol_dpcm",
      "DPCM Channel Volume %",
      NULL,
      "Modify DPCM Channel Volume %.",
      NULL,
      "audio",
      {
         { "0", NULL },
         { "10", NULL },
         { "20", NULL },
         { "30", NULL },
         { "40", NULL },
         { "50", NULL },
         { "60", NULL },
         { "70", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "nestopia_audio_vol_fds",
      "FDS Channel Volume %",
      NULL,
      "Modify FDS Channel Volume %.",
      NULL,
      "audio",
      {
         { "0", NULL },
         { "10", NULL },
         { "20", NULL },
         { "30", NULL },
         { "40", NULL },
         { "50", NULL },
         { "60", NULL },
         { "70", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "nestopia_audio_vol_mmc5",
      "MMC5 Channel Volume %",
      NULL,
      "Modify MMC5 Channel Volume %.",
      NULL,
      "audio",
      {
         { "0", NULL },
         { "10", NULL },
         { "20", NULL },
         { "30", NULL },
         { "40", NULL },
         { "50", NULL },
         { "60", NULL },
         { "70", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "nestopia_audio_vol_vrc6",
      "VRC6 Channel Volume %",
      NULL,
      "Modify VRC6 Channel Volume %.",
      NULL,
      "audio",
      {
         { "0", NULL },
         { "10", NULL },
         { "20", NULL },
         { "30", NULL },
         { "40", NULL },
         { "50", NULL },
         { "60", NULL },
         { "70", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "nestopia_audio_vol_vrc7",
      "VRC7 Channel Volume %",
      NULL,
      "Modify VRC7 Channel Volume %.",
      NULL,
      "audio",
      {
         { "0", NULL },
         { "10", NULL },
         { "20", NULL },
         { "30", NULL },
         { "40", NULL },
         { "50", NULL },
         { "60", NULL },
         { "70", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "nestopia_audio_vol_n163",
      "N163 Channel Volume %",
      NULL,
      "Modify N163 Channel Volume %.",
      NULL,
      "audio",
      {
         { "0", NULL },
         { "10", NULL },
         { "20", NULL },
         { "30", NULL },
         { "40", NULL },
         { "50", NULL },
         { "60", NULL },
         { "70", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "nestopia_audio_vol_s5b",
      "S5B Channel Volume %",
      NULL,
      "Modify S5B Channel Volume %.",
      NULL,
      "audio",
      {
         { "0", NULL },
         { "10", NULL },
         { "20", NULL },
         { "30", NULL },
         { "40", NULL },
         { "50", NULL },
         { "60", NULL },
         { "70", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "nestopia_audio_type",
      "Audio output",
      NULL,
      "Specify whether the audio output should be mono or stereo.",
      NULL,
      "audio",
      {
         { "mono", NULL },
         { "stereo", NULL },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      "4 Player Adapter",
      NULL,
      "Manually select a 4 Player Adapter if needed. Some games will not recognize the adapter correctly through the NstDatabase.xml database, this option should help fix that.",
      NULL,
      "input",
      {
         { "auto",    "Auto" },
         { "ntsc",    "NTSC" },
         { "famicom", "Famicom" },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      "Shift Buttons Clockwise",
      NULL,
      "Rotate the A/B/X/Y buttons clockwise.", /* is this right? @gadsby */
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_arkanoid_device",
      "Arkanoid device",
      NULL,
      "Select the device you wish to use for the Arkanoid paddle.",
      NULL,
      "input",
      {
         { "mouse", "Mouse" },
         { "pointer", "Pointer" },
      },
      "mouse"
   },
   {
      "nestopia_arkanoid_paddle_range",
      "Arkanoid Paddle Range",
      NULL,
      "Set the range for the Arkanoid paddle.",
      NULL,
      "input",
      {
         { "combined", "Combined range of both controllers (32-166)"},
         { "arkanoidI", "Arkanoid I controller range (46-166)"},
         { "arkanoidII", "Arkanoid II controller range (32-153)"}
      },
      "combined"
   },
   {
      "nestopia_zapper_device",
      "Zapper device",
      NULL,
      "Select the device you wish to use for the Zapper.",
      NULL,
      "input",
      {
         { "lightgun", "Light gun" },
         { "mouse", "Mouse" },
         { "pointer", "Pointer" },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      "Show Crosshair",
      NULL,
      "Set whether to show the crosshair when the Zapper is used.",
      NULL,
      "input",
      {
         { "disabled", NULL},
         { "enabled", NULL },
      },
      "enabled"
   },
   {
      "nestopia_turbo_pulse",
      "Turbo Pulse Speed",
      NULL,
      "Set the turbo pulse speed for the Turbo B and Turbo A buttons.",
      NULL,
      "input",
      {
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "2"
   },

/* Emulation Hacks */

   {
      "nestopia_nospritelimit",
      "Remove Sprite Limit",
      NULL,
      "Remove 8-sprites-per-scanline hardware limit.",
      NULL,
      "hacks",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_overclock",
      "CPU Speed (Overclock)",
      NULL,
      "Overclock the emulated CPU.",
      NULL,
      "hacks",
      {
         { "1x", NULL },
         { "2x", NULL },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      "RAM Power-on State",
      NULL,
      "RAM values on power up. Some games rely on initial RAM values for random number generation as an example.",
      NULL,
      "hacks",
      {
         { "0x00",   NULL },
         { "0xFF",   NULL },
         { "random", "Random" },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

struct retro_core_options_v2 options_us = {
   option_cats_us,
   option_defs_us
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_options_v2 *options_intl[RETRO_LANGUAGE_LAST] = {
   &options_us,      /* RETRO_LANGUAGE_ENGLISH */
   &options_ja,      /* RETRO_LANGUAGE_JAPANESE */
   &options_fr,      /* RETRO_LANGUAGE_FRENCH */
   &options_es,      /* RETRO_LANGUAGE_SPANISH */
   &options_de,      /* RETRO_LANGUAGE_GERMAN */
   &options_it,      /* RETRO_LANGUAGE_ITALIAN */
   &options_nl,      /* RETRO_LANGUAGE_DUTCH */
   &options_pt_br,   /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   &options_pt_pt,   /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   &options_ru,      /* RETRO_LANGUAGE_RUSSIAN */
   &options_ko,      /* RETRO_LANGUAGE_KOREAN */
   &options_cht,     /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   &options_chs,     /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   &options_eo,      /* RETRO_LANGUAGE_ESPERANTO */
   &options_pl,      /* RETRO_LANGUAGE_POLISH */
   &options_vn,      /* RETRO_LANGUAGE_VIETNAMESE */
   &options_ar,      /* RETRO_LANGUAGE_ARABIC */
   &options_el,      /* RETRO_LANGUAGE_GREEK */
   &options_tr,      /* RETRO_LANGUAGE_TURKISH */
   &options_sk,      /* RETRO_LANGUAGE_SLOVAK */
   &options_fa,      /* RETRO_LANGUAGE_PERSIAN */
   &options_he,      /* RETRO_LANGUAGE_HEBREW */
   &options_ast,     /* RETRO_LANGUAGE_ASTURIAN */
   &options_fi,      /* RETRO_LANGUAGE_FINNISH */
   &options_id,      /* RETRO_LANGUAGE_INDONESIAN */
   &options_sv,      /* RETRO_LANGUAGE_SWEDISH */
   &options_uk,      /* RETRO_LANGUAGE_UKRAINIAN */
};
#endif

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should be called as early as possible - ideally inside
 * retro_set_environment(), and no later than retro_load_game()
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static INLINE void libretro_set_core_options(retro_environment_t environ_cb,
      bool *categories_supported)
{
   unsigned version  = 0;
#ifndef HAVE_NO_LANGEXTRA
   unsigned language = 0;
#endif

   if (!environ_cb || !categories_supported)
      return;

   *categories_supported = false;

   if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
      version = 0;

   if (version >= 2)
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_v2_intl core_options_intl;

      core_options_intl.us    = &options_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = options_intl[language];

      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL,
            &core_options_intl);
#else
      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
            &options_us);
#endif
   }
   else
   {
      size_t i, j;
      size_t option_index              = 0;
      size_t num_options               = 0;
      struct retro_core_option_definition
            *option_v1_defs_us         = NULL;
#ifndef HAVE_NO_LANGEXTRA
      size_t num_options_intl          = 0;
      struct retro_core_option_v2_definition
            *option_defs_intl          = NULL;
      struct retro_core_option_definition
            *option_v1_defs_intl       = NULL;
      struct retro_core_options_intl
            core_options_v1_intl;
#endif
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine total number of options */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      if (version >= 1)
      {
         /* Allocate US array */
         option_v1_defs_us = (struct retro_core_option_definition *)
               calloc(num_options + 1, sizeof(struct retro_core_option_definition));

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            struct retro_core_option_v2_definition *option_def_us = &option_defs_us[i];
            struct retro_core_option_value *option_values         = option_def_us->values;
            struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
            struct retro_core_option_value *option_v1_values      = option_v1_def_us->values;

            option_v1_def_us->key           = option_def_us->key;
            option_v1_def_us->desc          = option_def_us->desc;
            option_v1_def_us->info          = option_def_us->info;
            option_v1_def_us->default_value = option_def_us->default_value;

            /* Values must be copied individually... */
            while (option_values->value)
            {
               option_v1_values->value = option_values->value;
               option_v1_values->label = option_values->label;

               option_values++;
               option_v1_values++;
            }
         }

#ifndef HAVE_NO_LANGEXTRA
         if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
             (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH) &&
             options_intl[language])
            option_defs_intl = options_intl[language]->definitions;

         if (option_defs_intl)
         {
            /* Determine number of intl options */
            while (true)
            {
               if (option_defs_intl[num_options_intl].key)
                  num_options_intl++;
               else
                  break;
            }

            /* Allocate intl array */
            option_v1_defs_intl = (struct retro_core_option_definition *)
                  calloc(num_options_intl + 1, sizeof(struct retro_core_option_definition));

            /* Copy parameters from option_defs_intl array */
            for (i = 0; i < num_options_intl; i++)
            {
               struct retro_core_option_v2_definition *option_def_intl = &option_defs_intl[i];
               struct retro_core_option_value *option_values           = option_def_intl->values;
               struct retro_core_option_definition *option_v1_def_intl = &option_v1_defs_intl[i];
               struct retro_core_option_value *option_v1_values        = option_v1_def_intl->values;

               option_v1_def_intl->key           = option_def_intl->key;
               option_v1_def_intl->desc          = option_def_intl->desc;
               option_v1_def_intl->info          = option_def_intl->info;
               option_v1_def_intl->default_value = option_def_intl->default_value;

               /* Values must be copied individually... */
               while (option_values->value)
               {
                  option_v1_values->value = option_values->value;
                  option_v1_values->label = option_values->label;

                  option_values++;
                  option_v1_values++;
               }
            }
         }

         core_options_v1_intl.us    = option_v1_defs_us;
         core_options_v1_intl.local = option_v1_defs_intl;

         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_v1_intl);
#else
         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
#endif
      }
      else
      {
         /* Allocate arrays */
         variables  = (struct retro_variable *)calloc(num_options + 1,
               sizeof(struct retro_variable));
         values_buf = (char **)calloc(num_options, sizeof(char *));

         if (!variables || !values_buf)
            goto error;

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            const char *key                        = option_defs_us[i].key;
            const char *desc                       = option_defs_us[i].desc;
            const char *default_value              = option_defs_us[i].default_value;
            struct retro_core_option_value *values = option_defs_us[i].values;
            size_t buf_len                         = 3;
            size_t default_index                   = 0;

            values_buf[i] = NULL;

            if (desc)
            {
               size_t num_values = 0;

               /* Determine number of values */
               while (true)
               {
                  if (values[num_values].value)
                  {
                     /* Check if this is the default value */
                     if (default_value)
                        if (strcmp(values[num_values].value, default_value) == 0)
                           default_index = num_values;

                     buf_len += strlen(values[num_values].value);
                     num_values++;
                  }
                  else
                     break;
               }

               /* Build values string */
               if (num_values > 0)
               {
                  buf_len += num_values - 1;
                  buf_len += strlen(desc);

                  values_buf[i] = (char *)calloc(buf_len, sizeof(char));
                  if (!values_buf[i])
                     goto error;

                  strcpy(values_buf[i], desc);
                  strcat(values_buf[i], "; ");

                  /* Default value goes first */
                  strcat(values_buf[i], values[default_index].value);

                  /* Add remaining values */
                  for (j = 0; j < num_values; j++)
                  {
                     if (j != default_index)
                     {
                        strcat(values_buf[i], "|");
                        strcat(values_buf[i], values[j].value);
                     }
                  }
               }
            }

            variables[option_index].key   = key;
            variables[option_index].value = values_buf[i];
            option_index++;
         }

         /* Set variables */
         environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
      }

error:
      /* Clean up */

      if (option_v1_defs_us)
      {
         free(option_v1_defs_us);
         option_v1_defs_us = NULL;
      }

#ifndef HAVE_NO_LANGEXTRA
      if (option_v1_defs_intl)
      {
         free(option_v1_defs_intl);
         option_v1_defs_intl = NULL;
      }
#endif

      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
