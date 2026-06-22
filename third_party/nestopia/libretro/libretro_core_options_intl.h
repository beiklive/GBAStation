#ifndef LIBRETRO_CORE_OPTIONS_INTL_H__
#define LIBRETRO_CORE_OPTIONS_INTL_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1500 && _MSC_VER < 1900)
/* https://support.microsoft.com/en-us/kb/980263 */
#pragma execution_character_set("utf-8")
#pragma warning(disable:4566)
#endif

#include <libretro.h>

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
/* RETRO_LANGUAGE_AR */

#define CATEGORY_SYSTEM_LABEL_AR NULL
#define CATEGORY_SYSTEM_INFO_0_AR NULL
#define CATEGORY_VIDEO_LABEL_AR "فيديو"
#define CATEGORY_VIDEO_INFO_0_AR NULL
#define CATEGORY_AUDIO_LABEL_AR "نظام تشغيل الصوت"
#define CATEGORY_AUDIO_INFO_0_AR NULL
#define CATEGORY_INPUT_LABEL_AR "الإدخال"
#define CATEGORY_INPUT_INFO_0_AR NULL
#define CATEGORY_HACKS_LABEL_AR NULL
#define CATEGORY_HACKS_INFO_0_AR NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_AR NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_AR NULL
#define OPTION_VAL_AUTO_AR "تلقائي"
#define OPTION_VAL_NTSC_AR NULL
#define OPTION_VAL_PAL_AR NULL
#define OPTION_VAL_FAMICOM_AR NULL
#define OPTION_VAL_DENDY_AR NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_AR NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_AR NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_AR NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_AR NULL
#define OPTION_VAL_COMPOSITE_AR NULL
#define OPTION_VAL_SVIDEO_AR NULL
#define OPTION_VAL_RGB_AR NULL
#define OPTION_VAL_MONOCHROME_AR "أحادي اللون"
#define NESTOPIA_PALETTE_LABEL_AR NULL
#define NESTOPIA_PALETTE_INFO_0_AR NULL
#define OPTION_VAL_CXA2025AS_AR NULL
#define OPTION_VAL_CONSUMER_AR NULL
#define OPTION_VAL_CANONICAL_AR NULL
#define OPTION_VAL_ALTERNATIVE_AR NULL
#define OPTION_VAL_RGB_O3_AR NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_AR NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_AR NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_AR NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_AR NULL
#define OPTION_VAL_RAW_AR NULL
#define OPTION_VAL_CUSTOM_AR "مخصص"
#define NESTOPIA_OVERSCAN_V_LABEL_AR NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_AR NULL
#define NESTOPIA_OVERSCAN_H_LABEL_AR NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_AR NULL
#define NESTOPIA_ASPECT_LABEL_AR NULL
#define NESTOPIA_ASPECT_INFO_0_AR NULL
#define OPTION_VAL_4_3_AR NULL
#define OPTION_VAL_UNCORRECTED_AR NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_AR NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_AR NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_AR NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_AR NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_AR NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_AR NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_AR NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_AR NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_AR NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_AR NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_AR NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_AR NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_AR NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_AR NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_AR NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_AR NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_AR NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_AR NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_AR NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_AR NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_AR NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_AR NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_AR NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_AR NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_AR NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_AR NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_AR NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_AR NULL
#define OPTION_VAL_MONO_AR NULL
#define OPTION_VAL_STEREO_AR NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_AR NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_AR NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_AR NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_AR NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_AR NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_AR NULL
#define OPTION_VAL_MOUSE_AR NULL
#define OPTION_VAL_POINTER_AR NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_AR NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_AR NULL
#define OPTION_VAL_LIGHTGUN_AR NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_AR NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_AR NULL
#define NESTOPIA_TURBO_PULSE_LABEL_AR NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_AR NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_AR NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_AR NULL
#define NESTOPIA_OVERCLOCK_LABEL_AR NULL
#define NESTOPIA_OVERCLOCK_INFO_0_AR NULL
#define OPTION_VAL_1X_AR NULL
#define OPTION_VAL_2X_AR NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_AR NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_AR NULL
#define OPTION_VAL_0X00_AR NULL
#define OPTION_VAL_0XFF_AR NULL
#define OPTION_VAL_RANDOM_AR NULL

struct retro_core_option_v2_category option_cats_ar[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_AR,
      CATEGORY_SYSTEM_INFO_0_AR
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_AR,
      CATEGORY_VIDEO_INFO_0_AR
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_AR,
      CATEGORY_AUDIO_INFO_0_AR
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_AR,
      CATEGORY_INPUT_INFO_0_AR
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_AR,
      CATEGORY_HACKS_INFO_0_AR
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ar[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_AR,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_AR,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_AR },
         { "ntsc",    OPTION_VAL_NTSC_AR },
         { "pal",     OPTION_VAL_PAL_AR },
         { "famicom", OPTION_VAL_FAMICOM_AR },
         { "dendy",   OPTION_VAL_DENDY_AR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_AR,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_AR,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_AR,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_AR,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_AR },
         { "svideo",     OPTION_VAL_SVIDEO_AR },
         { "rgb",        OPTION_VAL_RGB_AR },
         { "monochrome", OPTION_VAL_MONOCHROME_AR },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_AR,
      NULL,
      NESTOPIA_PALETTE_INFO_0_AR,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_AR },
         { "consumer",             OPTION_VAL_CONSUMER_AR },
         { "canonical",            OPTION_VAL_CANONICAL_AR },
         { "alternative",          OPTION_VAL_ALTERNATIVE_AR },
         { "rgb",                  OPTION_VAL_RGB_O3_AR },
         { "pal",                  OPTION_VAL_PAL_AR },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_AR },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_AR },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_AR },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_AR },
         { "raw",                  OPTION_VAL_RAW_AR },
         { "custom",               OPTION_VAL_CUSTOM_AR },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_AR,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_AR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_AR,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_AR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_AR,
      NULL,
      NESTOPIA_ASPECT_INFO_0_AR,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_AR },
         { "ntsc",        OPTION_VAL_NTSC_AR },
         { "pal",         OPTION_VAL_PAL_AR },
         { "4:3",         OPTION_VAL_4_3_AR },
         { "uncorrected", OPTION_VAL_UNCORRECTED_AR },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_AR,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_AR,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_AR,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_AR,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_AR,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_AR,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_AR,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_AR,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_AR,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_AR,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_AR,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_AR,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_AR,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_AR,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_AR,
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
      NESTOPIA_AUDIO_TYPE_LABEL_AR,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_AR,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_AR },
         { "stereo", OPTION_VAL_STEREO_AR },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_AR,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_AR,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_AR },
         { "ntsc",    OPTION_VAL_NTSC_AR },
         { "famicom", OPTION_VAL_FAMICOM_AR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_AR,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_AR, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_AR,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_AR,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_AR },
         { "pointer", OPTION_VAL_POINTER_AR },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_AR,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_AR,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_AR },
         { "mouse", OPTION_VAL_MOUSE_AR },
         { "pointer", OPTION_VAL_POINTER_AR },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_AR,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_AR,
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
      NESTOPIA_TURBO_PULSE_LABEL_AR,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_AR,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_AR,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_AR,
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
      NESTOPIA_OVERCLOCK_LABEL_AR,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_AR,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_AR },
         { "2x", OPTION_VAL_2X_AR },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_AR,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_AR,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_AR },
         { "0xFF",   OPTION_VAL_0XFF_AR },
         { "random", OPTION_VAL_RANDOM_AR },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ar = {
   option_cats_ar,
   option_defs_ar
};

/* RETRO_LANGUAGE_AST */

#define CATEGORY_SYSTEM_LABEL_AST "Sistema"
#define CATEGORY_SYSTEM_INFO_0_AST NULL
#define CATEGORY_VIDEO_LABEL_AST "Videu"
#define CATEGORY_VIDEO_INFO_0_AST NULL
#define CATEGORY_AUDIO_LABEL_AST "Audiu"
#define CATEGORY_AUDIO_INFO_0_AST NULL
#define CATEGORY_INPUT_LABEL_AST "Entrada"
#define CATEGORY_INPUT_INFO_0_AST NULL
#define CATEGORY_HACKS_LABEL_AST "Hacks de la emulación"
#define CATEGORY_HACKS_INFO_0_AST NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_AST "Rexón del sistema"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_AST NULL
#define OPTION_VAL_AUTO_AST NULL
#define OPTION_VAL_NTSC_AST NULL
#define OPTION_VAL_PAL_AST NULL
#define OPTION_VAL_FAMICOM_AST NULL
#define OPTION_VAL_DENDY_AST NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_AST NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_AST NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_AST NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_AST NULL
#define OPTION_VAL_COMPOSITE_AST NULL
#define OPTION_VAL_SVIDEO_AST NULL
#define OPTION_VAL_RGB_AST NULL
#define OPTION_VAL_MONOCHROME_AST NULL
#define NESTOPIA_PALETTE_LABEL_AST "Paleta"
#define NESTOPIA_PALETTE_INFO_0_AST NULL
#define OPTION_VAL_CXA2025AS_AST NULL
#define OPTION_VAL_CONSUMER_AST NULL
#define OPTION_VAL_CANONICAL_AST NULL
#define OPTION_VAL_ALTERNATIVE_AST NULL
#define OPTION_VAL_RGB_O3_AST NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_AST NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_AST NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_AST NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_AST NULL
#define OPTION_VAL_RAW_AST "En bruto"
#define OPTION_VAL_CUSTOM_AST NULL
#define NESTOPIA_OVERSCAN_V_LABEL_AST NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_AST NULL
#define NESTOPIA_OVERSCAN_H_LABEL_AST NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_AST NULL
#define NESTOPIA_ASPECT_LABEL_AST NULL
#define NESTOPIA_ASPECT_INFO_0_AST NULL
#define OPTION_VAL_4_3_AST NULL
#define OPTION_VAL_UNCORRECTED_AST NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_AST NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_AST NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_AST NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_AST NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_AST NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_AST NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_AST NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_AST NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_AST NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_AST NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_AST "Volume de la canal de ruíu (%)"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_AST "Modifica'l volume de la canal de ruíu (%)."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_AST NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_AST NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_AST "Volume de la canal de FDS (%)"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_AST "Modifica'l volume de la canal de FDS (%)."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_AST "Volume de la canal MMC5 (%)"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_AST "Modifica'l volume de la canal MMC5 (%)."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_AST "Volume de la canal VRC6 (%)"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_AST "Modifica'l volume de la canal VCR6 (%)."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_AST "Volume de la canal VRC7 (%)"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_AST "Modifica'l volume de la canal VRC7 (%)."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_AST "Volume de la canal N163 (%)"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_AST "Modifica'l volume de la canal N163 (%)."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_AST "Volume de la canal S5B (%)"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_AST NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_AST NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_AST NULL
#define OPTION_VAL_MONO_AST NULL
#define OPTION_VAL_STEREO_AST NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_AST "Adautador pa 4 xugadores"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_AST NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_AST NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_AST NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_AST NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_AST NULL
#define OPTION_VAL_MOUSE_AST "Mur"
#define OPTION_VAL_POINTER_AST "Punteru"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_AST "Preséu Zapper"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_AST "Seleiciona'l preséu que quies usar pal Zapper."
#define OPTION_VAL_LIGHTGUN_AST NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_AST NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_AST NULL
#define NESTOPIA_TURBO_PULSE_LABEL_AST NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_AST NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_AST NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_AST NULL
#define NESTOPIA_OVERCLOCK_LABEL_AST NULL
#define NESTOPIA_OVERCLOCK_INFO_0_AST NULL
#define OPTION_VAL_1X_AST "x1"
#define OPTION_VAL_2X_AST "x2"
#define NESTOPIA_RAM_POWER_STATE_LABEL_AST NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_AST NULL
#define OPTION_VAL_0X00_AST NULL
#define OPTION_VAL_0XFF_AST NULL
#define OPTION_VAL_RANDOM_AST NULL

struct retro_core_option_v2_category option_cats_ast[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_AST,
      CATEGORY_SYSTEM_INFO_0_AST
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_AST,
      CATEGORY_VIDEO_INFO_0_AST
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_AST,
      CATEGORY_AUDIO_INFO_0_AST
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_AST,
      CATEGORY_INPUT_INFO_0_AST
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_AST,
      CATEGORY_HACKS_INFO_0_AST
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ast[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_AST,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_AST,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_AST },
         { "ntsc",    OPTION_VAL_NTSC_AST },
         { "pal",     OPTION_VAL_PAL_AST },
         { "famicom", OPTION_VAL_FAMICOM_AST },
         { "dendy",   OPTION_VAL_DENDY_AST },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_AST,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_AST,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_AST,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_AST,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_AST },
         { "svideo",     OPTION_VAL_SVIDEO_AST },
         { "rgb",        OPTION_VAL_RGB_AST },
         { "monochrome", OPTION_VAL_MONOCHROME_AST },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_AST,
      NULL,
      NESTOPIA_PALETTE_INFO_0_AST,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_AST },
         { "consumer",             OPTION_VAL_CONSUMER_AST },
         { "canonical",            OPTION_VAL_CANONICAL_AST },
         { "alternative",          OPTION_VAL_ALTERNATIVE_AST },
         { "rgb",                  OPTION_VAL_RGB_O3_AST },
         { "pal",                  OPTION_VAL_PAL_AST },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_AST },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_AST },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_AST },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_AST },
         { "raw",                  OPTION_VAL_RAW_AST },
         { "custom",               OPTION_VAL_CUSTOM_AST },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_AST,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_AST,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_AST,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_AST,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_AST,
      NULL,
      NESTOPIA_ASPECT_INFO_0_AST,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_AST },
         { "ntsc",        OPTION_VAL_NTSC_AST },
         { "pal",         OPTION_VAL_PAL_AST },
         { "4:3",         OPTION_VAL_4_3_AST },
         { "uncorrected", OPTION_VAL_UNCORRECTED_AST },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_AST,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_AST,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_AST,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_AST,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_AST,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_AST,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_AST,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_AST,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_AST,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_AST,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_AST,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_AST,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_AST,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_AST,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_AST,
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
      NESTOPIA_AUDIO_TYPE_LABEL_AST,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_AST,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_AST },
         { "stereo", OPTION_VAL_STEREO_AST },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_AST,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_AST,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_AST },
         { "ntsc",    OPTION_VAL_NTSC_AST },
         { "famicom", OPTION_VAL_FAMICOM_AST },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_AST,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_AST, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_AST,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_AST,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_AST },
         { "pointer", OPTION_VAL_POINTER_AST },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_AST,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_AST,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_AST },
         { "mouse", OPTION_VAL_MOUSE_AST },
         { "pointer", OPTION_VAL_POINTER_AST },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_AST,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_AST,
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
      NESTOPIA_TURBO_PULSE_LABEL_AST,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_AST,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_AST,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_AST,
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
      NESTOPIA_OVERCLOCK_LABEL_AST,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_AST,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_AST },
         { "2x", OPTION_VAL_2X_AST },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_AST,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_AST,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_AST },
         { "0xFF",   OPTION_VAL_0XFF_AST },
         { "random", OPTION_VAL_RANDOM_AST },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ast = {
   option_cats_ast,
   option_defs_ast
};

/* RETRO_LANGUAGE_CA */

#define CATEGORY_SYSTEM_LABEL_CA "Sistema"
#define CATEGORY_SYSTEM_INFO_0_CA NULL
#define CATEGORY_VIDEO_LABEL_CA "Vídeo"
#define CATEGORY_VIDEO_INFO_0_CA NULL
#define CATEGORY_AUDIO_LABEL_CA "Àudio"
#define CATEGORY_AUDIO_INFO_0_CA NULL
#define CATEGORY_INPUT_LABEL_CA "Dispositius de joc"
#define CATEGORY_INPUT_INFO_0_CA NULL
#define CATEGORY_HACKS_LABEL_CA NULL
#define CATEGORY_HACKS_INFO_0_CA NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_CA NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_CA NULL
#define OPTION_VAL_AUTO_CA "Automàtic"
#define OPTION_VAL_NTSC_CA NULL
#define OPTION_VAL_PAL_CA NULL
#define OPTION_VAL_FAMICOM_CA NULL
#define OPTION_VAL_DENDY_CA NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_CA NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_CA NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_CA NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_CA NULL
#define OPTION_VAL_COMPOSITE_CA NULL
#define OPTION_VAL_SVIDEO_CA NULL
#define OPTION_VAL_RGB_CA NULL
#define OPTION_VAL_MONOCHROME_CA "Monocrom"
#define NESTOPIA_PALETTE_LABEL_CA NULL
#define NESTOPIA_PALETTE_INFO_0_CA NULL
#define OPTION_VAL_CXA2025AS_CA NULL
#define OPTION_VAL_CONSUMER_CA NULL
#define OPTION_VAL_CANONICAL_CA NULL
#define OPTION_VAL_ALTERNATIVE_CA NULL
#define OPTION_VAL_RGB_O3_CA NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_CA NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_CA NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_CA NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_CA NULL
#define OPTION_VAL_RAW_CA NULL
#define OPTION_VAL_CUSTOM_CA "Personalitzat"
#define NESTOPIA_OVERSCAN_V_LABEL_CA NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_CA NULL
#define NESTOPIA_OVERSCAN_H_LABEL_CA NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_CA NULL
#define NESTOPIA_ASPECT_LABEL_CA NULL
#define NESTOPIA_ASPECT_INFO_0_CA NULL
#define OPTION_VAL_4_3_CA NULL
#define OPTION_VAL_UNCORRECTED_CA NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_CA NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_CA NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_CA NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_CA NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_CA NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_CA NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_CA NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_CA NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_CA NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_CA NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_CA NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_CA NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_CA NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_CA NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_CA NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_CA NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_CA NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_CA NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_CA NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_CA NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_CA NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_CA NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_CA NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_CA NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_CA NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_CA NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_CA NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_CA NULL
#define OPTION_VAL_MONO_CA NULL
#define OPTION_VAL_STEREO_CA NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_CA NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_CA NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_CA NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_CA NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_CA NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_CA NULL
#define OPTION_VAL_MOUSE_CA NULL
#define OPTION_VAL_POINTER_CA NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_CA NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_CA NULL
#define OPTION_VAL_LIGHTGUN_CA NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_CA NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_CA NULL
#define NESTOPIA_TURBO_PULSE_LABEL_CA NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_CA NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_CA NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_CA NULL
#define NESTOPIA_OVERCLOCK_LABEL_CA NULL
#define NESTOPIA_OVERCLOCK_INFO_0_CA NULL
#define OPTION_VAL_1X_CA NULL
#define OPTION_VAL_2X_CA NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_CA NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_CA NULL
#define OPTION_VAL_0X00_CA NULL
#define OPTION_VAL_0XFF_CA NULL
#define OPTION_VAL_RANDOM_CA NULL

struct retro_core_option_v2_category option_cats_ca[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_CA,
      CATEGORY_SYSTEM_INFO_0_CA
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_CA,
      CATEGORY_VIDEO_INFO_0_CA
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_CA,
      CATEGORY_AUDIO_INFO_0_CA
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_CA,
      CATEGORY_INPUT_INFO_0_CA
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_CA,
      CATEGORY_HACKS_INFO_0_CA
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ca[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_CA,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_CA,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_CA },
         { "ntsc",    OPTION_VAL_NTSC_CA },
         { "pal",     OPTION_VAL_PAL_CA },
         { "famicom", OPTION_VAL_FAMICOM_CA },
         { "dendy",   OPTION_VAL_DENDY_CA },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_CA,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_CA,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_CA,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_CA,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_CA },
         { "svideo",     OPTION_VAL_SVIDEO_CA },
         { "rgb",        OPTION_VAL_RGB_CA },
         { "monochrome", OPTION_VAL_MONOCHROME_CA },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_CA,
      NULL,
      NESTOPIA_PALETTE_INFO_0_CA,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_CA },
         { "consumer",             OPTION_VAL_CONSUMER_CA },
         { "canonical",            OPTION_VAL_CANONICAL_CA },
         { "alternative",          OPTION_VAL_ALTERNATIVE_CA },
         { "rgb",                  OPTION_VAL_RGB_O3_CA },
         { "pal",                  OPTION_VAL_PAL_CA },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_CA },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_CA },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_CA },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_CA },
         { "raw",                  OPTION_VAL_RAW_CA },
         { "custom",               OPTION_VAL_CUSTOM_CA },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_CA,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_CA,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_CA,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_CA,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_CA,
      NULL,
      NESTOPIA_ASPECT_INFO_0_CA,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_CA },
         { "ntsc",        OPTION_VAL_NTSC_CA },
         { "pal",         OPTION_VAL_PAL_CA },
         { "4:3",         OPTION_VAL_4_3_CA },
         { "uncorrected", OPTION_VAL_UNCORRECTED_CA },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_CA,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_CA,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_CA,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_CA,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_CA,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_CA,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_CA,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_CA,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_CA,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_CA,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_CA,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_CA,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_CA,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_CA,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_CA,
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
      NESTOPIA_AUDIO_TYPE_LABEL_CA,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_CA,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_CA },
         { "stereo", OPTION_VAL_STEREO_CA },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_CA,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_CA,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_CA },
         { "ntsc",    OPTION_VAL_NTSC_CA },
         { "famicom", OPTION_VAL_FAMICOM_CA },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_CA,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_CA, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_CA,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_CA,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_CA },
         { "pointer", OPTION_VAL_POINTER_CA },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_CA,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_CA,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_CA },
         { "mouse", OPTION_VAL_MOUSE_CA },
         { "pointer", OPTION_VAL_POINTER_CA },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_CA,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_CA,
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
      NESTOPIA_TURBO_PULSE_LABEL_CA,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_CA,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_CA,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_CA,
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
      NESTOPIA_OVERCLOCK_LABEL_CA,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_CA,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_CA },
         { "2x", OPTION_VAL_2X_CA },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_CA,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_CA,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_CA },
         { "0xFF",   OPTION_VAL_0XFF_CA },
         { "random", OPTION_VAL_RANDOM_CA },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ca = {
   option_cats_ca,
   option_defs_ca
};

/* RETRO_LANGUAGE_CHS */

#define CATEGORY_SYSTEM_LABEL_CHS "系统"
#define CATEGORY_SYSTEM_INFO_0_CHS NULL
#define CATEGORY_VIDEO_LABEL_CHS "视频"
#define CATEGORY_VIDEO_INFO_0_CHS NULL
#define CATEGORY_AUDIO_LABEL_CHS "音频"
#define CATEGORY_AUDIO_INFO_0_CHS NULL
#define CATEGORY_INPUT_LABEL_CHS "输入"
#define CATEGORY_INPUT_INFO_0_CHS NULL
#define CATEGORY_HACKS_LABEL_CHS "模拟修改"
#define CATEGORY_HACKS_INFO_0_CHS NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_CHS "系统区域"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_CHS NULL
#define OPTION_VAL_AUTO_CHS "自动"
#define OPTION_VAL_NTSC_CHS NULL
#define OPTION_VAL_PAL_CHS NULL
#define OPTION_VAL_FAMICOM_CHS NULL
#define OPTION_VAL_DENDY_CHS NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_CHS NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_CHS NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_CHS "Blgg NTSC过滤器"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_CHS NULL
#define OPTION_VAL_COMPOSITE_CHS "合成视频"
#define OPTION_VAL_SVIDEO_CHS "S-视频"
#define OPTION_VAL_RGB_CHS NULL
#define OPTION_VAL_MONOCHROME_CHS "黑白"
#define NESTOPIA_PALETTE_LABEL_CHS "调色板"
#define NESTOPIA_PALETTE_INFO_0_CHS NULL
#define OPTION_VAL_CXA2025AS_CHS NULL
#define OPTION_VAL_CONSUMER_CHS NULL
#define OPTION_VAL_CANONICAL_CHS NULL
#define OPTION_VAL_ALTERNATIVE_CHS "候选窗口"
#define OPTION_VAL_RGB_O3_CHS NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_CHS NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_CHS NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_CHS NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_CHS NULL
#define OPTION_VAL_RAW_CHS NULL
#define OPTION_VAL_CUSTOM_CHS "自定义"
#define NESTOPIA_OVERSCAN_V_LABEL_CHS NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_CHS NULL
#define NESTOPIA_OVERSCAN_H_LABEL_CHS NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_CHS NULL
#define NESTOPIA_ASPECT_LABEL_CHS NULL
#define NESTOPIA_ASPECT_INFO_0_CHS NULL
#define OPTION_VAL_4_3_CHS NULL
#define OPTION_VAL_UNCORRECTED_CHS NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_CHS NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_CHS NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_CHS NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_CHS NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_CHS NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_CHS NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_CHS NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_CHS NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_CHS NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_CHS NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_CHS NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_CHS NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_CHS NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_CHS NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_CHS NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_CHS NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_CHS NULL
#define OPTION_VAL_MONO_CHS NULL
#define OPTION_VAL_STEREO_CHS NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_CHS NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_CHS NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_CHS NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_CHS NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_CHS NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_CHS NULL
#define OPTION_VAL_MOUSE_CHS "鼠标"
#define OPTION_VAL_POINTER_CHS "指针"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_CHS NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_CHS NULL
#define OPTION_VAL_LIGHTGUN_CHS NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_CHS NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_CHS NULL
#define NESTOPIA_TURBO_PULSE_LABEL_CHS NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_CHS NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_CHS NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_CHS NULL
#define NESTOPIA_OVERCLOCK_LABEL_CHS NULL
#define NESTOPIA_OVERCLOCK_INFO_0_CHS NULL
#define OPTION_VAL_1X_CHS NULL
#define OPTION_VAL_2X_CHS "2 倍"
#define NESTOPIA_RAM_POWER_STATE_LABEL_CHS NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_CHS NULL
#define OPTION_VAL_0X00_CHS NULL
#define OPTION_VAL_0XFF_CHS NULL
#define OPTION_VAL_RANDOM_CHS "随机"

struct retro_core_option_v2_category option_cats_chs[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_CHS,
      CATEGORY_SYSTEM_INFO_0_CHS
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_CHS,
      CATEGORY_VIDEO_INFO_0_CHS
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_CHS,
      CATEGORY_AUDIO_INFO_0_CHS
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_CHS,
      CATEGORY_INPUT_INFO_0_CHS
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_CHS,
      CATEGORY_HACKS_INFO_0_CHS
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_chs[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_CHS,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_CHS,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_CHS },
         { "ntsc",    OPTION_VAL_NTSC_CHS },
         { "pal",     OPTION_VAL_PAL_CHS },
         { "famicom", OPTION_VAL_FAMICOM_CHS },
         { "dendy",   OPTION_VAL_DENDY_CHS },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_CHS,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_CHS,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_CHS,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_CHS,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_CHS },
         { "svideo",     OPTION_VAL_SVIDEO_CHS },
         { "rgb",        OPTION_VAL_RGB_CHS },
         { "monochrome", OPTION_VAL_MONOCHROME_CHS },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_CHS,
      NULL,
      NESTOPIA_PALETTE_INFO_0_CHS,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_CHS },
         { "consumer",             OPTION_VAL_CONSUMER_CHS },
         { "canonical",            OPTION_VAL_CANONICAL_CHS },
         { "alternative",          OPTION_VAL_ALTERNATIVE_CHS },
         { "rgb",                  OPTION_VAL_RGB_O3_CHS },
         { "pal",                  OPTION_VAL_PAL_CHS },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_CHS },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_CHS },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_CHS },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_CHS },
         { "raw",                  OPTION_VAL_RAW_CHS },
         { "custom",               OPTION_VAL_CUSTOM_CHS },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_CHS,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_CHS,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_CHS,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_CHS,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_CHS,
      NULL,
      NESTOPIA_ASPECT_INFO_0_CHS,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_CHS },
         { "ntsc",        OPTION_VAL_NTSC_CHS },
         { "pal",         OPTION_VAL_PAL_CHS },
         { "4:3",         OPTION_VAL_4_3_CHS },
         { "uncorrected", OPTION_VAL_UNCORRECTED_CHS },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_CHS,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_CHS,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_CHS,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_CHS,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_CHS,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_CHS,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_CHS,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_CHS,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_CHS,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_CHS,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_CHS,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_CHS,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_CHS,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_CHS,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_CHS,
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
      NESTOPIA_AUDIO_TYPE_LABEL_CHS,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_CHS,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_CHS },
         { "stereo", OPTION_VAL_STEREO_CHS },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_CHS,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_CHS,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_CHS },
         { "ntsc",    OPTION_VAL_NTSC_CHS },
         { "famicom", OPTION_VAL_FAMICOM_CHS },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_CHS,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_CHS, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_CHS,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_CHS,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_CHS },
         { "pointer", OPTION_VAL_POINTER_CHS },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_CHS,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_CHS,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_CHS },
         { "mouse", OPTION_VAL_MOUSE_CHS },
         { "pointer", OPTION_VAL_POINTER_CHS },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_CHS,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_CHS,
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
      NESTOPIA_TURBO_PULSE_LABEL_CHS,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_CHS,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_CHS,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_CHS,
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
      NESTOPIA_OVERCLOCK_LABEL_CHS,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_CHS,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_CHS },
         { "2x", OPTION_VAL_2X_CHS },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_CHS,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_CHS,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_CHS },
         { "0xFF",   OPTION_VAL_0XFF_CHS },
         { "random", OPTION_VAL_RANDOM_CHS },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_chs = {
   option_cats_chs,
   option_defs_chs
};

/* RETRO_LANGUAGE_CHT */

#define CATEGORY_SYSTEM_LABEL_CHT "系統"
#define CATEGORY_SYSTEM_INFO_0_CHT NULL
#define CATEGORY_VIDEO_LABEL_CHT "顯示"
#define CATEGORY_VIDEO_INFO_0_CHT "變更寬高比、畫面裁切、配色和影像濾鏡的設定。"
#define CATEGORY_AUDIO_LABEL_CHT "音效"
#define CATEGORY_AUDIO_INFO_0_CHT NULL
#define CATEGORY_INPUT_LABEL_CHT "輸入"
#define CATEGORY_INPUT_INFO_0_CHT NULL
#define CATEGORY_HACKS_LABEL_CHT "進階"
#define CATEGORY_HACKS_INFO_0_CHT "變更模擬器進階破解的設定。"
#define NESTOPIA_FAVORED_SYSTEM_LABEL_CHT "系統區域"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_CHT NULL
#define OPTION_VAL_AUTO_CHT "自動"
#define OPTION_VAL_NTSC_CHT NULL
#define OPTION_VAL_PAL_CHT NULL
#define OPTION_VAL_FAMICOM_CHT NULL
#define OPTION_VAL_DENDY_CHT NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_CHT NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_CHT NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_CHT NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_CHT NULL
#define OPTION_VAL_COMPOSITE_CHT NULL
#define OPTION_VAL_SVIDEO_CHT NULL
#define OPTION_VAL_RGB_CHT NULL
#define OPTION_VAL_MONOCHROME_CHT "單色白 (Monochrome)"
#define NESTOPIA_PALETTE_LABEL_CHT NULL
#define NESTOPIA_PALETTE_INFO_0_CHT NULL
#define OPTION_VAL_CXA2025AS_CHT NULL
#define OPTION_VAL_CONSUMER_CHT NULL
#define OPTION_VAL_CANONICAL_CHT NULL
#define OPTION_VAL_ALTERNATIVE_CHT NULL
#define OPTION_VAL_RGB_O3_CHT "三原色"
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_CHT NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_CHT NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_CHT NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_CHT NULL
#define OPTION_VAL_RAW_CHT NULL
#define OPTION_VAL_CUSTOM_CHT "自訂"
#define NESTOPIA_OVERSCAN_V_LABEL_CHT NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_CHT NULL
#define NESTOPIA_OVERSCAN_H_LABEL_CHT NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_CHT NULL
#define NESTOPIA_ASPECT_LABEL_CHT NULL
#define NESTOPIA_ASPECT_INFO_0_CHT NULL
#define OPTION_VAL_4_3_CHT NULL
#define OPTION_VAL_UNCORRECTED_CHT NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_CHT NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_CHT NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_CHT NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_CHT NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_CHT NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_CHT NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_CHT NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_CHT NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_CHT NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_CHT NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_CHT NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_CHT NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_CHT NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_CHT NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_CHT NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_CHT NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_CHT NULL
#define OPTION_VAL_MONO_CHT NULL
#define OPTION_VAL_STEREO_CHT NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_CHT "四手把轉接器"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_CHT "部分遊戲無法通過 NstDatabase.xml 資料庫，正確識別轉接器，變更此選項有助於解決這個問題。"
#define NESTOPIA_BUTTON_SHIFT_LABEL_CHT NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_CHT NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_CHT NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_CHT NULL
#define OPTION_VAL_MOUSE_CHT NULL
#define OPTION_VAL_POINTER_CHT NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_CHT NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_CHT NULL
#define OPTION_VAL_LIGHTGUN_CHT NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_CHT NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_CHT NULL
#define NESTOPIA_TURBO_PULSE_LABEL_CHT NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_CHT NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_CHT NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_CHT "刪除每條掃描線8張素材的硬體限制。"
#define NESTOPIA_OVERCLOCK_LABEL_CHT NULL
#define NESTOPIA_OVERCLOCK_INFO_0_CHT NULL
#define OPTION_VAL_1X_CHT "1倍"
#define OPTION_VAL_2X_CHT "2倍"
#define NESTOPIA_RAM_POWER_STATE_LABEL_CHT NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_CHT NULL
#define OPTION_VAL_0X00_CHT NULL
#define OPTION_VAL_0XFF_CHT NULL
#define OPTION_VAL_RANDOM_CHT NULL

struct retro_core_option_v2_category option_cats_cht[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_CHT,
      CATEGORY_SYSTEM_INFO_0_CHT
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_CHT,
      CATEGORY_VIDEO_INFO_0_CHT
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_CHT,
      CATEGORY_AUDIO_INFO_0_CHT
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_CHT,
      CATEGORY_INPUT_INFO_0_CHT
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_CHT,
      CATEGORY_HACKS_INFO_0_CHT
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_cht[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_CHT,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_CHT,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_CHT },
         { "ntsc",    OPTION_VAL_NTSC_CHT },
         { "pal",     OPTION_VAL_PAL_CHT },
         { "famicom", OPTION_VAL_FAMICOM_CHT },
         { "dendy",   OPTION_VAL_DENDY_CHT },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_CHT,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_CHT,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_CHT,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_CHT,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_CHT },
         { "svideo",     OPTION_VAL_SVIDEO_CHT },
         { "rgb",        OPTION_VAL_RGB_CHT },
         { "monochrome", OPTION_VAL_MONOCHROME_CHT },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_CHT,
      NULL,
      NESTOPIA_PALETTE_INFO_0_CHT,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_CHT },
         { "consumer",             OPTION_VAL_CONSUMER_CHT },
         { "canonical",            OPTION_VAL_CANONICAL_CHT },
         { "alternative",          OPTION_VAL_ALTERNATIVE_CHT },
         { "rgb",                  OPTION_VAL_RGB_O3_CHT },
         { "pal",                  OPTION_VAL_PAL_CHT },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_CHT },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_CHT },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_CHT },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_CHT },
         { "raw",                  OPTION_VAL_RAW_CHT },
         { "custom",               OPTION_VAL_CUSTOM_CHT },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_CHT,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_CHT,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_CHT,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_CHT,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_CHT,
      NULL,
      NESTOPIA_ASPECT_INFO_0_CHT,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_CHT },
         { "ntsc",        OPTION_VAL_NTSC_CHT },
         { "pal",         OPTION_VAL_PAL_CHT },
         { "4:3",         OPTION_VAL_4_3_CHT },
         { "uncorrected", OPTION_VAL_UNCORRECTED_CHT },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_CHT,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_CHT,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_CHT,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_CHT,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_CHT,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_CHT,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_CHT,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_CHT,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_CHT,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_CHT,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_CHT,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_CHT,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_CHT,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_CHT,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_CHT,
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
      NESTOPIA_AUDIO_TYPE_LABEL_CHT,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_CHT,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_CHT },
         { "stereo", OPTION_VAL_STEREO_CHT },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_CHT,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_CHT,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_CHT },
         { "ntsc",    OPTION_VAL_NTSC_CHT },
         { "famicom", OPTION_VAL_FAMICOM_CHT },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_CHT,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_CHT, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_CHT,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_CHT,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_CHT },
         { "pointer", OPTION_VAL_POINTER_CHT },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_CHT,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_CHT,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_CHT },
         { "mouse", OPTION_VAL_MOUSE_CHT },
         { "pointer", OPTION_VAL_POINTER_CHT },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_CHT,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_CHT,
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
      NESTOPIA_TURBO_PULSE_LABEL_CHT,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_CHT,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_CHT,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_CHT,
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
      NESTOPIA_OVERCLOCK_LABEL_CHT,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_CHT,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_CHT },
         { "2x", OPTION_VAL_2X_CHT },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_CHT,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_CHT,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_CHT },
         { "0xFF",   OPTION_VAL_0XFF_CHT },
         { "random", OPTION_VAL_RANDOM_CHT },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_cht = {
   option_cats_cht,
   option_defs_cht
};

/* RETRO_LANGUAGE_CS */

#define CATEGORY_SYSTEM_LABEL_CS "Systém"
#define CATEGORY_SYSTEM_INFO_0_CS NULL
#define CATEGORY_VIDEO_LABEL_CS NULL
#define CATEGORY_VIDEO_INFO_0_CS "Změňte poměr stran, oříznutí displeje, paletu barev a nastavení filtru videa."
#define CATEGORY_AUDIO_LABEL_CS "Zvuk"
#define CATEGORY_AUDIO_INFO_0_CS "Změna zvukových efektů a hlasitosti kanálů."
#define CATEGORY_INPUT_LABEL_CS "Vstup"
#define CATEGORY_INPUT_INFO_0_CS "Změna vstupních zařízení a dalších nastavení souvisejících se vstupem."
#define CATEGORY_HACKS_LABEL_CS NULL
#define CATEGORY_HACKS_INFO_0_CS "Změna nastavení přetaktování procesoru a přesnosti emulace ovlivňující výkon a kompatibilitu na nízké úrovni."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_CS "Systémový Region"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_CS "'Auto' použije databázový soubor NstDatabase.xml pro automatickou detekci regionu. Pokud není k dispozici žádná databáze, bude výchozí nastavení NTSC."
#define OPTION_VAL_AUTO_CS NULL
#define OPTION_VAL_NTSC_CS NULL
#define OPTION_VAL_PAL_CS NULL
#define OPTION_VAL_FAMICOM_CS NULL
#define OPTION_VAL_DENDY_CS NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_CS "Automatické Vložení FDS"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_CS "Automatické vložení prvního disku FDS při resetu."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_CS "Blargg NTSC Filtr"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_CS "Povolení Filtrů Blargg NTSC."
#define OPTION_VAL_COMPOSITE_CS "Kompozitní Video"
#define OPTION_VAL_SVIDEO_CS NULL
#define OPTION_VAL_RGB_CS NULL
#define OPTION_VAL_MONOCHROME_CS NULL
#define NESTOPIA_PALETTE_LABEL_CS "Paleta"
#define NESTOPIA_PALETTE_INFO_0_CS NULL
#define OPTION_VAL_CXA2025AS_CS NULL
#define OPTION_VAL_CONSUMER_CS "Spotřebitel"
#define OPTION_VAL_CANONICAL_CS "Kanonické"
#define OPTION_VAL_ALTERNATIVE_CS "Alternativní"
#define OPTION_VAL_RGB_O3_CS NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_CS "Kompozitní Přímý FBx"
#define OPTION_VAL_PVM_STYLE_D93_FBX_CS NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_CS NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_CS "NES Classic FBx (Pevné)"
#define OPTION_VAL_RAW_CS "Nezpracovaná Data"
#define OPTION_VAL_CUSTOM_CS "Vlastní"
#define NESTOPIA_OVERSCAN_V_LABEL_CS "Překrytí Masky (Vertikální)"
#define NESTOPIA_OVERSCAN_V_INFO_0_CS "Zamaskuje (vertikálně) potenciálně náhodný poruchový obrazový výstup, který by byl skrytý za rámečkem na okraji obrazovky televizoru se standardním rozlišením."
#define NESTOPIA_OVERSCAN_H_LABEL_CS "Překrytí Masky (Horizontální)"
#define NESTOPIA_OVERSCAN_H_INFO_0_CS "Maskovat (horizontálně) potenciálně náhodný poruchový obrazový výstup, který by byl skrytý za rámečkem kolem okraje obrazovky televizoru se standardním rozlišením."
#define NESTOPIA_ASPECT_LABEL_CS "Preferovaný Poměr Stran"
#define NESTOPIA_ASPECT_INFO_0_CS NULL
#define OPTION_VAL_4_3_CS NULL
#define OPTION_VAL_UNCORRECTED_CS "Neopraveno"
#define NESTOPIA_GENIE_DISTORTION_LABEL_CS "Game Genie Zkreslení Zvuku"
#define NESTOPIA_GENIE_DISTORTION_INFO_0_CS "Cheatovací zařízení Game Genie by mohlo ve hrách neúmyslně zkreslit zvuk. Povolením této funkce můžete simulovat zkreslení, které by přidalo do zvuku hry."
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_CS "Zobrazit Pokročilá Nastavení Zvuku (Znovu Otevřít Menu)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_CS "Povolení konfigurace parametrů nízkoúrovňového zvukového kanálu."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_CS "Square 1 Hlasitost Kanálu %"
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_CS "Úprava Square 1 Hlasitost Kanálu %."
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_CS "Square 2 Hlasitost Kanálu %"
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_CS "Úprava Square 2 Hlasitost Kanálu %."
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_CS "Triangle Hlasitost Kanálu %"
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_CS "Úprava Triangle Hlasitost Kanálu %."
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_CS "Noise Hlasitost Kanálu %"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_CS "Úprava Noise Hlasitost Kanálu %."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_CS "DPCM Hlasitost Kanálu %"
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_CS "Úprava DPCM Hlasitost Kanálu %."
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_CS "FDS Hlasitost Kanálu %"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_CS "Úprava FDS Hlasitost Kanálu %."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_CS "MMC5 Hlasitost Kanálu %"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_CS "Úprava MMC5 Hlasitost Kanálu %."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_CS "VRC6 Hlasitost Kanálu %"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_CS "Úprava VRC6 Hlasitost Kanálu %."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_CS "VRC7 Hlasitost Kanálu %"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_CS "Úprava VRC7 Hlasitost Kanálu %."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_CS "N163 Hlasitost Kanálu %"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_CS "Úprava N163 Hlasitost Kanálu %."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_CS "S5B Hlasitost Kanálu %"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_CS NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_CS NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_CS NULL
#define OPTION_VAL_MONO_CS NULL
#define OPTION_VAL_STEREO_CS NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_CS "Adaptér pro 4 Hráče"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_CS "V případě potřeby ručně vyberte adaptér pro 4 hráče. Některé hry nerozpoznají adaptér správně prostřednictvím databáze NstDatabase.xml, tato možnost by to měla pomoci napravit."
#define NESTOPIA_BUTTON_SHIFT_LABEL_CS "Tlačítka Posunuté po Směru Hodinových Ručiček"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_CS "Otočit tlačítka A/B/X/Y hodinovým směrem."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_CS "Arkanoid zařízení"
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_CS "Vyberte zařízení, které chcete použít pro pádlo Arkanoid."
#define OPTION_VAL_MOUSE_CS "Myš"
#define OPTION_VAL_POINTER_CS "Ukazatel"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_CS "Zapperové zařízení"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_CS "Vyberte zařízení, které chcete použít pro Zapper."
#define OPTION_VAL_LIGHTGUN_CS "Světelná Pistole"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_CS "Zobrazit Kříž"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_CS "Nastavení zobrazení zaměřovacího kříže při použití Zapperu."
#define NESTOPIA_TURBO_PULSE_LABEL_CS "Rychlost Pulsu Turba"
#define NESTOPIA_TURBO_PULSE_INFO_0_CS "Nastavte rychlost turboimpulzů pro tlačítka Turbo B a Turbo A."
#define NESTOPIA_NOSPRITELIMIT_LABEL_CS "Odebrat Limit Spritu"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_CS "Odstranění hardwarového limitu 8 sprintů na řádek skenování."
#define NESTOPIA_OVERCLOCK_LABEL_CS "Rychlost CPU (Přetaktování)"
#define NESTOPIA_OVERCLOCK_INFO_0_CS "Přetaktování emulovaného CPU."
#define OPTION_VAL_1X_CS NULL
#define OPTION_VAL_2X_CS NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_CS "Stav RAM po Zapnutí"
#define NESTOPIA_RAM_POWER_STATE_INFO_0_CS "Hodnoty paměti RAM při zapnutí. Některé hry se například spoléhají na počáteční hodnoty RAM při generování náhodných čísel."
#define OPTION_VAL_0X00_CS NULL
#define OPTION_VAL_0XFF_CS NULL
#define OPTION_VAL_RANDOM_CS "Náhodná"

struct retro_core_option_v2_category option_cats_cs[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_CS,
      CATEGORY_SYSTEM_INFO_0_CS
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_CS,
      CATEGORY_VIDEO_INFO_0_CS
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_CS,
      CATEGORY_AUDIO_INFO_0_CS
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_CS,
      CATEGORY_INPUT_INFO_0_CS
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_CS,
      CATEGORY_HACKS_INFO_0_CS
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_cs[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_CS,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_CS,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_CS },
         { "ntsc",    OPTION_VAL_NTSC_CS },
         { "pal",     OPTION_VAL_PAL_CS },
         { "famicom", OPTION_VAL_FAMICOM_CS },
         { "dendy",   OPTION_VAL_DENDY_CS },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_CS,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_CS,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_CS,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_CS,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_CS },
         { "svideo",     OPTION_VAL_SVIDEO_CS },
         { "rgb",        OPTION_VAL_RGB_CS },
         { "monochrome", OPTION_VAL_MONOCHROME_CS },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_CS,
      NULL,
      NESTOPIA_PALETTE_INFO_0_CS,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_CS },
         { "consumer",             OPTION_VAL_CONSUMER_CS },
         { "canonical",            OPTION_VAL_CANONICAL_CS },
         { "alternative",          OPTION_VAL_ALTERNATIVE_CS },
         { "rgb",                  OPTION_VAL_RGB_O3_CS },
         { "pal",                  OPTION_VAL_PAL_CS },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_CS },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_CS },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_CS },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_CS },
         { "raw",                  OPTION_VAL_RAW_CS },
         { "custom",               OPTION_VAL_CUSTOM_CS },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_CS,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_CS,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_CS,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_CS,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_CS,
      NULL,
      NESTOPIA_ASPECT_INFO_0_CS,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_CS },
         { "ntsc",        OPTION_VAL_NTSC_CS },
         { "pal",         OPTION_VAL_PAL_CS },
         { "4:3",         OPTION_VAL_4_3_CS },
         { "uncorrected", OPTION_VAL_UNCORRECTED_CS },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_CS,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_CS,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_CS,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_CS,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_CS,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_CS,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_CS,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_CS,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_CS,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_CS,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_CS,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_CS,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_CS,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_CS,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_CS,
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
      NESTOPIA_AUDIO_TYPE_LABEL_CS,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_CS,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_CS },
         { "stereo", OPTION_VAL_STEREO_CS },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_CS,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_CS,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_CS },
         { "ntsc",    OPTION_VAL_NTSC_CS },
         { "famicom", OPTION_VAL_FAMICOM_CS },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_CS,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_CS, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_CS,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_CS,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_CS },
         { "pointer", OPTION_VAL_POINTER_CS },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_CS,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_CS,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_CS },
         { "mouse", OPTION_VAL_MOUSE_CS },
         { "pointer", OPTION_VAL_POINTER_CS },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_CS,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_CS,
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
      NESTOPIA_TURBO_PULSE_LABEL_CS,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_CS,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_CS,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_CS,
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
      NESTOPIA_OVERCLOCK_LABEL_CS,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_CS,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_CS },
         { "2x", OPTION_VAL_2X_CS },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_CS,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_CS,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_CS },
         { "0xFF",   OPTION_VAL_0XFF_CS },
         { "random", OPTION_VAL_RANDOM_CS },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_cs = {
   option_cats_cs,
   option_defs_cs
};

/* RETRO_LANGUAGE_CY */

#define CATEGORY_SYSTEM_LABEL_CY NULL
#define CATEGORY_SYSTEM_INFO_0_CY NULL
#define CATEGORY_VIDEO_LABEL_CY NULL
#define CATEGORY_VIDEO_INFO_0_CY NULL
#define CATEGORY_AUDIO_LABEL_CY NULL
#define CATEGORY_AUDIO_INFO_0_CY NULL
#define CATEGORY_INPUT_LABEL_CY "Mewnbwn"
#define CATEGORY_INPUT_INFO_0_CY NULL
#define CATEGORY_HACKS_LABEL_CY NULL
#define CATEGORY_HACKS_INFO_0_CY NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_CY NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_CY NULL
#define OPTION_VAL_AUTO_CY NULL
#define OPTION_VAL_NTSC_CY NULL
#define OPTION_VAL_PAL_CY NULL
#define OPTION_VAL_FAMICOM_CY NULL
#define OPTION_VAL_DENDY_CY NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_CY NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_CY NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_CY NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_CY NULL
#define OPTION_VAL_COMPOSITE_CY NULL
#define OPTION_VAL_SVIDEO_CY NULL
#define OPTION_VAL_RGB_CY NULL
#define OPTION_VAL_MONOCHROME_CY NULL
#define NESTOPIA_PALETTE_LABEL_CY NULL
#define NESTOPIA_PALETTE_INFO_0_CY NULL
#define OPTION_VAL_CXA2025AS_CY NULL
#define OPTION_VAL_CONSUMER_CY NULL
#define OPTION_VAL_CANONICAL_CY NULL
#define OPTION_VAL_ALTERNATIVE_CY NULL
#define OPTION_VAL_RGB_O3_CY NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_CY NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_CY NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_CY NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_CY NULL
#define OPTION_VAL_RAW_CY NULL
#define OPTION_VAL_CUSTOM_CY NULL
#define NESTOPIA_OVERSCAN_V_LABEL_CY NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_CY NULL
#define NESTOPIA_OVERSCAN_H_LABEL_CY NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_CY NULL
#define NESTOPIA_ASPECT_LABEL_CY NULL
#define NESTOPIA_ASPECT_INFO_0_CY NULL
#define OPTION_VAL_4_3_CY NULL
#define OPTION_VAL_UNCORRECTED_CY NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_CY NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_CY NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_CY NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_CY NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_CY NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_CY NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_CY NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_CY NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_CY NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_CY NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_CY NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_CY NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_CY NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_CY NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_CY NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_CY NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_CY NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_CY NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_CY NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_CY NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_CY NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_CY NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_CY NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_CY NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_CY NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_CY NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_CY NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_CY NULL
#define OPTION_VAL_MONO_CY NULL
#define OPTION_VAL_STEREO_CY NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_CY NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_CY NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_CY NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_CY NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_CY NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_CY NULL
#define OPTION_VAL_MOUSE_CY NULL
#define OPTION_VAL_POINTER_CY NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_CY NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_CY NULL
#define OPTION_VAL_LIGHTGUN_CY NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_CY NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_CY NULL
#define NESTOPIA_TURBO_PULSE_LABEL_CY NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_CY NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_CY NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_CY NULL
#define NESTOPIA_OVERCLOCK_LABEL_CY NULL
#define NESTOPIA_OVERCLOCK_INFO_0_CY NULL
#define OPTION_VAL_1X_CY NULL
#define OPTION_VAL_2X_CY NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_CY NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_CY NULL
#define OPTION_VAL_0X00_CY NULL
#define OPTION_VAL_0XFF_CY NULL
#define OPTION_VAL_RANDOM_CY NULL

struct retro_core_option_v2_category option_cats_cy[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_CY,
      CATEGORY_SYSTEM_INFO_0_CY
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_CY,
      CATEGORY_VIDEO_INFO_0_CY
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_CY,
      CATEGORY_AUDIO_INFO_0_CY
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_CY,
      CATEGORY_INPUT_INFO_0_CY
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_CY,
      CATEGORY_HACKS_INFO_0_CY
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_cy[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_CY,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_CY,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_CY },
         { "ntsc",    OPTION_VAL_NTSC_CY },
         { "pal",     OPTION_VAL_PAL_CY },
         { "famicom", OPTION_VAL_FAMICOM_CY },
         { "dendy",   OPTION_VAL_DENDY_CY },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_CY,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_CY,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_CY,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_CY,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_CY },
         { "svideo",     OPTION_VAL_SVIDEO_CY },
         { "rgb",        OPTION_VAL_RGB_CY },
         { "monochrome", OPTION_VAL_MONOCHROME_CY },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_CY,
      NULL,
      NESTOPIA_PALETTE_INFO_0_CY,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_CY },
         { "consumer",             OPTION_VAL_CONSUMER_CY },
         { "canonical",            OPTION_VAL_CANONICAL_CY },
         { "alternative",          OPTION_VAL_ALTERNATIVE_CY },
         { "rgb",                  OPTION_VAL_RGB_O3_CY },
         { "pal",                  OPTION_VAL_PAL_CY },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_CY },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_CY },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_CY },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_CY },
         { "raw",                  OPTION_VAL_RAW_CY },
         { "custom",               OPTION_VAL_CUSTOM_CY },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_CY,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_CY,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_CY,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_CY,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_CY,
      NULL,
      NESTOPIA_ASPECT_INFO_0_CY,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_CY },
         { "ntsc",        OPTION_VAL_NTSC_CY },
         { "pal",         OPTION_VAL_PAL_CY },
         { "4:3",         OPTION_VAL_4_3_CY },
         { "uncorrected", OPTION_VAL_UNCORRECTED_CY },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_CY,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_CY,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_CY,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_CY,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_CY,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_CY,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_CY,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_CY,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_CY,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_CY,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_CY,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_CY,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_CY,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_CY,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_CY,
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
      NESTOPIA_AUDIO_TYPE_LABEL_CY,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_CY,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_CY },
         { "stereo", OPTION_VAL_STEREO_CY },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_CY,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_CY,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_CY },
         { "ntsc",    OPTION_VAL_NTSC_CY },
         { "famicom", OPTION_VAL_FAMICOM_CY },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_CY,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_CY, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_CY,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_CY,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_CY },
         { "pointer", OPTION_VAL_POINTER_CY },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_CY,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_CY,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_CY },
         { "mouse", OPTION_VAL_MOUSE_CY },
         { "pointer", OPTION_VAL_POINTER_CY },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_CY,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_CY,
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
      NESTOPIA_TURBO_PULSE_LABEL_CY,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_CY,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_CY,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_CY,
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
      NESTOPIA_OVERCLOCK_LABEL_CY,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_CY,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_CY },
         { "2x", OPTION_VAL_2X_CY },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_CY,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_CY,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_CY },
         { "0xFF",   OPTION_VAL_0XFF_CY },
         { "random", OPTION_VAL_RANDOM_CY },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_cy = {
   option_cats_cy,
   option_defs_cy
};

/* RETRO_LANGUAGE_DA */

#define CATEGORY_SYSTEM_LABEL_DA NULL
#define CATEGORY_SYSTEM_INFO_0_DA NULL
#define CATEGORY_VIDEO_LABEL_DA NULL
#define CATEGORY_VIDEO_INFO_0_DA NULL
#define CATEGORY_AUDIO_LABEL_DA "Lyd"
#define CATEGORY_AUDIO_INFO_0_DA NULL
#define CATEGORY_INPUT_LABEL_DA NULL
#define CATEGORY_INPUT_INFO_0_DA NULL
#define CATEGORY_HACKS_LABEL_DA NULL
#define CATEGORY_HACKS_INFO_0_DA NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_DA NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_DA NULL
#define OPTION_VAL_AUTO_DA NULL
#define OPTION_VAL_NTSC_DA NULL
#define OPTION_VAL_PAL_DA NULL
#define OPTION_VAL_FAMICOM_DA NULL
#define OPTION_VAL_DENDY_DA NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_DA NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_DA NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_DA NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_DA NULL
#define OPTION_VAL_COMPOSITE_DA NULL
#define OPTION_VAL_SVIDEO_DA NULL
#define OPTION_VAL_RGB_DA NULL
#define OPTION_VAL_MONOCHROME_DA NULL
#define NESTOPIA_PALETTE_LABEL_DA NULL
#define NESTOPIA_PALETTE_INFO_0_DA NULL
#define OPTION_VAL_CXA2025AS_DA NULL
#define OPTION_VAL_CONSUMER_DA NULL
#define OPTION_VAL_CANONICAL_DA NULL
#define OPTION_VAL_ALTERNATIVE_DA NULL
#define OPTION_VAL_RGB_O3_DA NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_DA NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_DA NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_DA NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_DA NULL
#define OPTION_VAL_RAW_DA NULL
#define OPTION_VAL_CUSTOM_DA NULL
#define NESTOPIA_OVERSCAN_V_LABEL_DA NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_DA NULL
#define NESTOPIA_OVERSCAN_H_LABEL_DA NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_DA NULL
#define NESTOPIA_ASPECT_LABEL_DA NULL
#define NESTOPIA_ASPECT_INFO_0_DA NULL
#define OPTION_VAL_4_3_DA NULL
#define OPTION_VAL_UNCORRECTED_DA NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_DA NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_DA NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_DA NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_DA NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_DA NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_DA NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_DA NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_DA NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_DA NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_DA NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_DA NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_DA NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_DA NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_DA NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_DA NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_DA NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_DA NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_DA NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_DA NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_DA NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_DA NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_DA NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_DA NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_DA NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_DA NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_DA NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_DA NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_DA NULL
#define OPTION_VAL_MONO_DA NULL
#define OPTION_VAL_STEREO_DA NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_DA NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_DA NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_DA NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_DA NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_DA NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_DA NULL
#define OPTION_VAL_MOUSE_DA NULL
#define OPTION_VAL_POINTER_DA NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_DA NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_DA NULL
#define OPTION_VAL_LIGHTGUN_DA NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_DA NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_DA NULL
#define NESTOPIA_TURBO_PULSE_LABEL_DA NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_DA NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_DA NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_DA NULL
#define NESTOPIA_OVERCLOCK_LABEL_DA NULL
#define NESTOPIA_OVERCLOCK_INFO_0_DA NULL
#define OPTION_VAL_1X_DA NULL
#define OPTION_VAL_2X_DA NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_DA NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_DA NULL
#define OPTION_VAL_0X00_DA NULL
#define OPTION_VAL_0XFF_DA NULL
#define OPTION_VAL_RANDOM_DA NULL

struct retro_core_option_v2_category option_cats_da[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_DA,
      CATEGORY_SYSTEM_INFO_0_DA
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_DA,
      CATEGORY_VIDEO_INFO_0_DA
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_DA,
      CATEGORY_AUDIO_INFO_0_DA
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_DA,
      CATEGORY_INPUT_INFO_0_DA
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_DA,
      CATEGORY_HACKS_INFO_0_DA
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_da[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_DA,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_DA,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_DA },
         { "ntsc",    OPTION_VAL_NTSC_DA },
         { "pal",     OPTION_VAL_PAL_DA },
         { "famicom", OPTION_VAL_FAMICOM_DA },
         { "dendy",   OPTION_VAL_DENDY_DA },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_DA,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_DA,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_DA,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_DA,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_DA },
         { "svideo",     OPTION_VAL_SVIDEO_DA },
         { "rgb",        OPTION_VAL_RGB_DA },
         { "monochrome", OPTION_VAL_MONOCHROME_DA },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_DA,
      NULL,
      NESTOPIA_PALETTE_INFO_0_DA,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_DA },
         { "consumer",             OPTION_VAL_CONSUMER_DA },
         { "canonical",            OPTION_VAL_CANONICAL_DA },
         { "alternative",          OPTION_VAL_ALTERNATIVE_DA },
         { "rgb",                  OPTION_VAL_RGB_O3_DA },
         { "pal",                  OPTION_VAL_PAL_DA },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_DA },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_DA },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_DA },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_DA },
         { "raw",                  OPTION_VAL_RAW_DA },
         { "custom",               OPTION_VAL_CUSTOM_DA },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_DA,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_DA,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_DA,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_DA,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_DA,
      NULL,
      NESTOPIA_ASPECT_INFO_0_DA,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_DA },
         { "ntsc",        OPTION_VAL_NTSC_DA },
         { "pal",         OPTION_VAL_PAL_DA },
         { "4:3",         OPTION_VAL_4_3_DA },
         { "uncorrected", OPTION_VAL_UNCORRECTED_DA },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_DA,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_DA,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_DA,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_DA,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_DA,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_DA,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_DA,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_DA,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_DA,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_DA,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_DA,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_DA,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_DA,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_DA,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_DA,
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
      NESTOPIA_AUDIO_TYPE_LABEL_DA,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_DA,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_DA },
         { "stereo", OPTION_VAL_STEREO_DA },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_DA,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_DA,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_DA },
         { "ntsc",    OPTION_VAL_NTSC_DA },
         { "famicom", OPTION_VAL_FAMICOM_DA },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_DA,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_DA, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_DA,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_DA,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_DA },
         { "pointer", OPTION_VAL_POINTER_DA },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_DA,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_DA,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_DA },
         { "mouse", OPTION_VAL_MOUSE_DA },
         { "pointer", OPTION_VAL_POINTER_DA },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_DA,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_DA,
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
      NESTOPIA_TURBO_PULSE_LABEL_DA,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_DA,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_DA,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_DA,
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
      NESTOPIA_OVERCLOCK_LABEL_DA,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_DA,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_DA },
         { "2x", OPTION_VAL_2X_DA },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_DA,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_DA,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_DA },
         { "0xFF",   OPTION_VAL_0XFF_DA },
         { "random", OPTION_VAL_RANDOM_DA },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_da = {
   option_cats_da,
   option_defs_da
};

/* RETRO_LANGUAGE_DE */

#define CATEGORY_SYSTEM_LABEL_DE NULL
#define CATEGORY_SYSTEM_INFO_0_DE NULL
#define CATEGORY_VIDEO_LABEL_DE NULL
#define CATEGORY_VIDEO_INFO_0_DE "Seitenverhältnis ändern, Zuschneiden, Farbpalette und Video-Filtereinstellungen ändern."
#define CATEGORY_AUDIO_LABEL_DE NULL
#define CATEGORY_AUDIO_INFO_0_DE "Audioeffekte und Kanallautstärke ändern."
#define CATEGORY_INPUT_LABEL_DE NULL
#define CATEGORY_INPUT_INFO_0_DE "Eingabegeräte und andere Eingabe-bezogene Einstellungen ändern."
#define CATEGORY_HACKS_LABEL_DE "Emulations-Hacks"
#define CATEGORY_HACKS_INFO_0_DE "Parameter für Prozessorübertaktung und Emulationsgenauigkeit, die sich auf die Low-Level-Leistung und Kompatibilität auswirken, ändern."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_DE "Systemregion"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_DE "„Automatisch“ verwendet die Datenbankdatei NstDatabase.xml für die regionale Autoerkennung verwenden. Wenn keine Datenbank vorhanden ist, wird standardmäßig NTSC verwendet."
#define OPTION_VAL_AUTO_DE "Automatisch"
#define OPTION_VAL_NTSC_DE NULL
#define OPTION_VAL_PAL_DE NULL
#define OPTION_VAL_FAMICOM_DE NULL
#define OPTION_VAL_DENDY_DE NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_DE "FDS automatisch einlegen"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_DE "Beim Zurücksetzen automatisch die erste FDS-Diskette einlegen."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_DE "NTSC-Filter Blargg"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_DE "NTSC-Filter Blargg aktivieren."
#define OPTION_VAL_COMPOSITE_DE NULL
#define OPTION_VAL_SVIDEO_DE NULL
#define OPTION_VAL_RGB_DE "RGB-SCART"
#define OPTION_VAL_MONOCHROME_DE "Monochrom"
#define NESTOPIA_PALETTE_LABEL_DE NULL
#define NESTOPIA_PALETTE_INFO_0_DE NULL
#define OPTION_VAL_CXA2025AS_DE NULL
#define OPTION_VAL_CONSUMER_DE NULL
#define OPTION_VAL_CANONICAL_DE "Kanonisch"
#define OPTION_VAL_ALTERNATIVE_DE "Alternativ"
#define OPTION_VAL_RGB_O3_DE NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_DE NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_DE "PVM-Stil D93 FBx"
#define OPTION_VAL_NTSC_HARDWARE_FBX_DE "NTSC-Hardware FBx"
#define OPTION_VAL_NES_CLASSIC_FBX_FS_DE "NES Classic FBx (fest)"
#define OPTION_VAL_RAW_DE "Roh"
#define OPTION_VAL_CUSTOM_DE "Benutzerdefiniert"
#define NESTOPIA_OVERSCAN_V_LABEL_DE "Overscan verbergen (vertikal)"
#define NESTOPIA_OVERSCAN_V_INFO_0_DE "Blendet (vertikal) die potenziell zufällige, fehlerhafte Videoausgabe aus, die durch den Rand eines Fernsehbildschirms mit Standardauflösung verdeckt wäre."
#define NESTOPIA_OVERSCAN_H_LABEL_DE "Overscan verbergen (horizontal)"
#define NESTOPIA_OVERSCAN_H_INFO_0_DE "Blendet (horizontal) die potenziell zufällige, fehlerhafte Videoausgabe aus, die durch den Rand eines Fernsehbildschirms mit Standardauflösung verdeckt wäre."
#define NESTOPIA_ASPECT_LABEL_DE "Bevorzugtes Seitenverhältnis"
#define NESTOPIA_ASPECT_INFO_0_DE NULL
#define OPTION_VAL_4_3_DE NULL
#define OPTION_VAL_UNCORRECTED_DE "Unkorrigiert"
#define NESTOPIA_GENIE_DISTORTION_LABEL_DE "Game-Genie-Soundverzerrung"
#define NESTOPIA_GENIE_DISTORTION_INFO_0_DE "Das Cheat-Gerät Game Genie kann unbeabsichtigt Klangverzerrungen in Spielen verursachen. Wird diese Funktion aktiviert, kann die Verzerrung simuliert werden, die dem Sound eines Spiels hinzugefügt würde."
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_DE "Erweiterte Audioeinstellungen anzeigen (Menü erneut öffnen)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_DE "Konfiguration der Low-Level-Audiokanalparameter aktivieren."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_DE "Square-1-Kanallautstärke %"
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_DE "Square-1-Kanallautstärke % ändern."
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_DE "Square-2-Kanallautstärke %"
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_DE "Square-2-Kanallautstärke % ändern."
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_DE "Triangle-Kanallautstärke %"
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_DE "Triangle-Kanallautstärke % ändern."
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_DE "Rauschkanallautstärke %"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_DE "Rauschkanallautstärke % ändern."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_DE "DPCM-Kanallautstärke %"
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_DE "DPCM-Kanallautstärke % ändern."
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_DE "FDS-Kanallautstärke %"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_DE "FDS-Kanallautstärke % ändern."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_DE "MMC5-Kanallautstärke %"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_DE "MMC5-Kanallautstärke % ändern."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_DE "VRC6-Kanallautstärke %"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_DE "VRC6-Kanallautstärke % ändern."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_DE "VRC7-Kanallautstärke %"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_DE "VRC7-Kanallautstärke % ändern."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_DE "N163-Kanallautstärke %"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_DE "N163-Kanallautstärke % ändern."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_DE "S5B-Kanallautstärke %"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_DE NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_DE NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_DE NULL
#define OPTION_VAL_MONO_DE NULL
#define OPTION_VAL_STEREO_DE NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_DE "4-Spieler-Adapter"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_DE "Bei Bedarf manuell einen 4-Spieler-Adapter auswählen. Einige Spiele erkennen den Adapter nicht korrekt über die Datenbank NstDatabase.xml; diese Option sollte helfen, dies zu beheben."
#define NESTOPIA_BUTTON_SHIFT_LABEL_DE "Tasten im Uhrzeigersinn drehen"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_DE "Die A/B/X/Y-Tasten im Uhrzeigersinn drehen."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_DE "Arkanoid-Gerät"
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_DE "Das Gerät auswählen, das für das Arkanoid-Paddel verwenden werden soll."
#define OPTION_VAL_MOUSE_DE "Maus"
#define OPTION_VAL_POINTER_DE "Zeiger"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_DE "Zapper-Gerät"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_DE "Das Gerät auswählen, das für den Zapper verwenden werden soll."
#define OPTION_VAL_LIGHTGUN_DE "Lightgun"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_DE "Fadenkreuz anzeigen"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_DE "Festlegen, ob das Fadenkreuz angezeigt werden soll, wenn der Zapper verwendet wird."
#define NESTOPIA_TURBO_PULSE_LABEL_DE "Turbo-Impulsgeschwindigkeit"
#define NESTOPIA_TURBO_PULSE_INFO_0_DE "Die Turbo-Impulsgeschwindigkeit für die Tasten Turbo B und Turbo A einstellen."
#define NESTOPIA_NOSPRITELIMIT_LABEL_DE "Sprite-Limit entfernen"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_DE "Die Hardware-Beschränkung von 8 Sprites pro Scanline entfernen."
#define NESTOPIA_OVERCLOCK_LABEL_DE "CPU-Geschwindigkeit (Übertaktung)"
#define NESTOPIA_OVERCLOCK_INFO_0_DE "Übertaktung der emulierte CPU."
#define OPTION_VAL_1X_DE "1 ×"
#define OPTION_VAL_2X_DE "2 ×"
#define NESTOPIA_RAM_POWER_STATE_LABEL_DE "RAM-Einschaltstatus"
#define NESTOPIA_RAM_POWER_STATE_INFO_0_DE "RAM-Werte beim Einschalten. Einige Spiele verlassen sich beispielsweise bei der Generierung von Zufallszahlen auf die anfänglichen RAM-Werte."
#define OPTION_VAL_0X00_DE NULL
#define OPTION_VAL_0XFF_DE NULL
#define OPTION_VAL_RANDOM_DE "Zufällig"

struct retro_core_option_v2_category option_cats_de[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_DE,
      CATEGORY_SYSTEM_INFO_0_DE
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_DE,
      CATEGORY_VIDEO_INFO_0_DE
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_DE,
      CATEGORY_AUDIO_INFO_0_DE
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_DE,
      CATEGORY_INPUT_INFO_0_DE
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_DE,
      CATEGORY_HACKS_INFO_0_DE
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_de[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_DE,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_DE,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_DE },
         { "ntsc",    OPTION_VAL_NTSC_DE },
         { "pal",     OPTION_VAL_PAL_DE },
         { "famicom", OPTION_VAL_FAMICOM_DE },
         { "dendy",   OPTION_VAL_DENDY_DE },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_DE,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_DE,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_DE,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_DE,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_DE },
         { "svideo",     OPTION_VAL_SVIDEO_DE },
         { "rgb",        OPTION_VAL_RGB_DE },
         { "monochrome", OPTION_VAL_MONOCHROME_DE },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_DE,
      NULL,
      NESTOPIA_PALETTE_INFO_0_DE,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_DE },
         { "consumer",             OPTION_VAL_CONSUMER_DE },
         { "canonical",            OPTION_VAL_CANONICAL_DE },
         { "alternative",          OPTION_VAL_ALTERNATIVE_DE },
         { "rgb",                  OPTION_VAL_RGB_O3_DE },
         { "pal",                  OPTION_VAL_PAL_DE },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_DE },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_DE },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_DE },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_DE },
         { "raw",                  OPTION_VAL_RAW_DE },
         { "custom",               OPTION_VAL_CUSTOM_DE },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_DE,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_DE,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_DE,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_DE,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_DE,
      NULL,
      NESTOPIA_ASPECT_INFO_0_DE,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_DE },
         { "ntsc",        OPTION_VAL_NTSC_DE },
         { "pal",         OPTION_VAL_PAL_DE },
         { "4:3",         OPTION_VAL_4_3_DE },
         { "uncorrected", OPTION_VAL_UNCORRECTED_DE },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_DE,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_DE,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_DE,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_DE,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_DE,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_DE,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_DE,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_DE,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_DE,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_DE,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_DE,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_DE,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_DE,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_DE,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_DE,
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
      NESTOPIA_AUDIO_TYPE_LABEL_DE,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_DE,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_DE },
         { "stereo", OPTION_VAL_STEREO_DE },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_DE,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_DE,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_DE },
         { "ntsc",    OPTION_VAL_NTSC_DE },
         { "famicom", OPTION_VAL_FAMICOM_DE },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_DE,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_DE, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_DE,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_DE,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_DE },
         { "pointer", OPTION_VAL_POINTER_DE },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_DE,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_DE,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_DE },
         { "mouse", OPTION_VAL_MOUSE_DE },
         { "pointer", OPTION_VAL_POINTER_DE },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_DE,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_DE,
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
      NESTOPIA_TURBO_PULSE_LABEL_DE,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_DE,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_DE,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_DE,
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
      NESTOPIA_OVERCLOCK_LABEL_DE,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_DE,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_DE },
         { "2x", OPTION_VAL_2X_DE },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_DE,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_DE,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_DE },
         { "0xFF",   OPTION_VAL_0XFF_DE },
         { "random", OPTION_VAL_RANDOM_DE },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_de = {
   option_cats_de,
   option_defs_de
};

/* RETRO_LANGUAGE_EL */

#define CATEGORY_SYSTEM_LABEL_EL "Σύστημα"
#define CATEGORY_SYSTEM_INFO_0_EL NULL
#define CATEGORY_VIDEO_LABEL_EL "Οδηγός Βίντεο"
#define CATEGORY_VIDEO_INFO_0_EL NULL
#define CATEGORY_AUDIO_LABEL_EL "Οδηγός Ήχου"
#define CATEGORY_AUDIO_INFO_0_EL NULL
#define CATEGORY_INPUT_LABEL_EL "Οδηγός Εισαγωγής"
#define CATEGORY_INPUT_INFO_0_EL NULL
#define CATEGORY_HACKS_LABEL_EL NULL
#define CATEGORY_HACKS_INFO_0_EL NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_EL NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_EL NULL
#define OPTION_VAL_AUTO_EL "Αυτόματο"
#define OPTION_VAL_NTSC_EL NULL
#define OPTION_VAL_PAL_EL NULL
#define OPTION_VAL_FAMICOM_EL NULL
#define OPTION_VAL_DENDY_EL NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_EL NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_EL NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_EL NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_EL NULL
#define OPTION_VAL_COMPOSITE_EL NULL
#define OPTION_VAL_SVIDEO_EL NULL
#define OPTION_VAL_RGB_EL NULL
#define OPTION_VAL_MONOCHROME_EL "Μονόχρωμο"
#define NESTOPIA_PALETTE_LABEL_EL NULL
#define NESTOPIA_PALETTE_INFO_0_EL NULL
#define OPTION_VAL_CXA2025AS_EL NULL
#define OPTION_VAL_CONSUMER_EL NULL
#define OPTION_VAL_CANONICAL_EL NULL
#define OPTION_VAL_ALTERNATIVE_EL NULL
#define OPTION_VAL_RGB_O3_EL NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_EL NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_EL NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_EL NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_EL NULL
#define OPTION_VAL_RAW_EL NULL
#define OPTION_VAL_CUSTOM_EL NULL
#define NESTOPIA_OVERSCAN_V_LABEL_EL NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_EL NULL
#define NESTOPIA_OVERSCAN_H_LABEL_EL NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_EL NULL
#define NESTOPIA_ASPECT_LABEL_EL NULL
#define NESTOPIA_ASPECT_INFO_0_EL NULL
#define OPTION_VAL_4_3_EL NULL
#define OPTION_VAL_UNCORRECTED_EL NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_EL NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_EL NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_EL NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_EL NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_EL NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_EL NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_EL NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_EL NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_EL NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_EL NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_EL NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_EL NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_EL NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_EL NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_EL NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_EL NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_EL NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_EL NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_EL NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_EL NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_EL NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_EL NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_EL NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_EL NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_EL NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_EL NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_EL NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_EL NULL
#define OPTION_VAL_MONO_EL NULL
#define OPTION_VAL_STEREO_EL NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_EL NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_EL NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_EL NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_EL NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_EL NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_EL NULL
#define OPTION_VAL_MOUSE_EL NULL
#define OPTION_VAL_POINTER_EL NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_EL NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_EL NULL
#define OPTION_VAL_LIGHTGUN_EL NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_EL NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_EL NULL
#define NESTOPIA_TURBO_PULSE_LABEL_EL NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_EL NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_EL NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_EL NULL
#define NESTOPIA_OVERCLOCK_LABEL_EL NULL
#define NESTOPIA_OVERCLOCK_INFO_0_EL NULL
#define OPTION_VAL_1X_EL NULL
#define OPTION_VAL_2X_EL NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_EL NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_EL NULL
#define OPTION_VAL_0X00_EL NULL
#define OPTION_VAL_0XFF_EL NULL
#define OPTION_VAL_RANDOM_EL NULL

struct retro_core_option_v2_category option_cats_el[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_EL,
      CATEGORY_SYSTEM_INFO_0_EL
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_EL,
      CATEGORY_VIDEO_INFO_0_EL
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_EL,
      CATEGORY_AUDIO_INFO_0_EL
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_EL,
      CATEGORY_INPUT_INFO_0_EL
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_EL,
      CATEGORY_HACKS_INFO_0_EL
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_el[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_EL,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_EL,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_EL },
         { "ntsc",    OPTION_VAL_NTSC_EL },
         { "pal",     OPTION_VAL_PAL_EL },
         { "famicom", OPTION_VAL_FAMICOM_EL },
         { "dendy",   OPTION_VAL_DENDY_EL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_EL,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_EL,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_EL,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_EL,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_EL },
         { "svideo",     OPTION_VAL_SVIDEO_EL },
         { "rgb",        OPTION_VAL_RGB_EL },
         { "monochrome", OPTION_VAL_MONOCHROME_EL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_EL,
      NULL,
      NESTOPIA_PALETTE_INFO_0_EL,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_EL },
         { "consumer",             OPTION_VAL_CONSUMER_EL },
         { "canonical",            OPTION_VAL_CANONICAL_EL },
         { "alternative",          OPTION_VAL_ALTERNATIVE_EL },
         { "rgb",                  OPTION_VAL_RGB_O3_EL },
         { "pal",                  OPTION_VAL_PAL_EL },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_EL },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_EL },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_EL },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_EL },
         { "raw",                  OPTION_VAL_RAW_EL },
         { "custom",               OPTION_VAL_CUSTOM_EL },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_EL,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_EL,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_EL,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_EL,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_EL,
      NULL,
      NESTOPIA_ASPECT_INFO_0_EL,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_EL },
         { "ntsc",        OPTION_VAL_NTSC_EL },
         { "pal",         OPTION_VAL_PAL_EL },
         { "4:3",         OPTION_VAL_4_3_EL },
         { "uncorrected", OPTION_VAL_UNCORRECTED_EL },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_EL,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_EL,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_EL,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_EL,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_EL,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_EL,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_EL,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_EL,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_EL,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_EL,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_EL,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_EL,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_EL,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_EL,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_EL,
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
      NESTOPIA_AUDIO_TYPE_LABEL_EL,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_EL,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_EL },
         { "stereo", OPTION_VAL_STEREO_EL },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_EL,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_EL,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_EL },
         { "ntsc",    OPTION_VAL_NTSC_EL },
         { "famicom", OPTION_VAL_FAMICOM_EL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_EL,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_EL, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_EL,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_EL,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_EL },
         { "pointer", OPTION_VAL_POINTER_EL },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_EL,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_EL,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_EL },
         { "mouse", OPTION_VAL_MOUSE_EL },
         { "pointer", OPTION_VAL_POINTER_EL },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_EL,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_EL,
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
      NESTOPIA_TURBO_PULSE_LABEL_EL,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_EL,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_EL,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_EL,
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
      NESTOPIA_OVERCLOCK_LABEL_EL,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_EL,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_EL },
         { "2x", OPTION_VAL_2X_EL },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_EL,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_EL,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_EL },
         { "0xFF",   OPTION_VAL_0XFF_EL },
         { "random", OPTION_VAL_RANDOM_EL },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_el = {
   option_cats_el,
   option_defs_el
};

/* RETRO_LANGUAGE_EN */

#define CATEGORY_SYSTEM_LABEL_EN NULL
#define CATEGORY_SYSTEM_INFO_0_EN NULL
#define CATEGORY_VIDEO_LABEL_EN NULL
#define CATEGORY_VIDEO_INFO_0_EN "Change aspect ratio, display cropping, colour palette and video filter settings."
#define CATEGORY_AUDIO_LABEL_EN NULL
#define CATEGORY_AUDIO_INFO_0_EN NULL
#define CATEGORY_INPUT_LABEL_EN NULL
#define CATEGORY_INPUT_INFO_0_EN NULL
#define CATEGORY_HACKS_LABEL_EN NULL
#define CATEGORY_HACKS_INFO_0_EN "Change processor over-clocking and emulation accuracy settings affecting low-level performance and compatibility."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_EN NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_EN NULL
#define OPTION_VAL_AUTO_EN NULL
#define OPTION_VAL_NTSC_EN NULL
#define OPTION_VAL_PAL_EN NULL
#define OPTION_VAL_FAMICOM_EN NULL
#define OPTION_VAL_DENDY_EN NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_EN NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_EN NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_EN NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_EN NULL
#define OPTION_VAL_COMPOSITE_EN NULL
#define OPTION_VAL_SVIDEO_EN NULL
#define OPTION_VAL_RGB_EN NULL
#define OPTION_VAL_MONOCHROME_EN NULL
#define NESTOPIA_PALETTE_LABEL_EN NULL
#define NESTOPIA_PALETTE_INFO_0_EN NULL
#define OPTION_VAL_CXA2025AS_EN NULL
#define OPTION_VAL_CONSUMER_EN NULL
#define OPTION_VAL_CANONICAL_EN NULL
#define OPTION_VAL_ALTERNATIVE_EN NULL
#define OPTION_VAL_RGB_O3_EN NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_EN NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_EN NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_EN NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_EN NULL
#define OPTION_VAL_RAW_EN NULL
#define OPTION_VAL_CUSTOM_EN NULL
#define NESTOPIA_OVERSCAN_V_LABEL_EN NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_EN NULL
#define NESTOPIA_OVERSCAN_H_LABEL_EN NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_EN NULL
#define NESTOPIA_ASPECT_LABEL_EN NULL
#define NESTOPIA_ASPECT_INFO_0_EN NULL
#define OPTION_VAL_4_3_EN NULL
#define OPTION_VAL_UNCORRECTED_EN NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_EN NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_EN NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_EN NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_EN NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_EN NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_EN NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_EN NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_EN NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_EN NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_EN NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_EN NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_EN NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_EN NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_EN NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_EN NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_EN NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_EN NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_EN NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_EN NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_EN NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_EN NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_EN NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_EN NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_EN NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_EN NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_EN NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_EN NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_EN NULL
#define OPTION_VAL_MONO_EN NULL
#define OPTION_VAL_STEREO_EN NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_EN NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_EN "Manually select a 4 Player Adapter if needed. Some games will not recognise the adapter correctly through the NstDatabase.xml database, this option should help fix that."
#define NESTOPIA_BUTTON_SHIFT_LABEL_EN NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_EN NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_EN NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_EN NULL
#define OPTION_VAL_MOUSE_EN NULL
#define OPTION_VAL_POINTER_EN NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_EN NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_EN NULL
#define OPTION_VAL_LIGHTGUN_EN NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_EN "Show Cross-hair"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_EN "Set whether to show the cross-hair when the Zapper is used."
#define NESTOPIA_TURBO_PULSE_LABEL_EN NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_EN NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_EN NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_EN "Remove 8-sprites-per-scan-line hardware limit."
#define NESTOPIA_OVERCLOCK_LABEL_EN "CPU Speed (Over-clock)"
#define NESTOPIA_OVERCLOCK_INFO_0_EN "Over-clock the emulated CPU."
#define OPTION_VAL_1X_EN NULL
#define OPTION_VAL_2X_EN NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_EN NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_EN NULL
#define OPTION_VAL_0X00_EN NULL
#define OPTION_VAL_0XFF_EN NULL
#define OPTION_VAL_RANDOM_EN NULL

struct retro_core_option_v2_category option_cats_en[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_EN,
      CATEGORY_SYSTEM_INFO_0_EN
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_EN,
      CATEGORY_VIDEO_INFO_0_EN
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_EN,
      CATEGORY_AUDIO_INFO_0_EN
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_EN,
      CATEGORY_INPUT_INFO_0_EN
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_EN,
      CATEGORY_HACKS_INFO_0_EN
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_en[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_EN,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_EN,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_EN },
         { "ntsc",    OPTION_VAL_NTSC_EN },
         { "pal",     OPTION_VAL_PAL_EN },
         { "famicom", OPTION_VAL_FAMICOM_EN },
         { "dendy",   OPTION_VAL_DENDY_EN },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_EN,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_EN,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_EN,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_EN,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_EN },
         { "svideo",     OPTION_VAL_SVIDEO_EN },
         { "rgb",        OPTION_VAL_RGB_EN },
         { "monochrome", OPTION_VAL_MONOCHROME_EN },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_EN,
      NULL,
      NESTOPIA_PALETTE_INFO_0_EN,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_EN },
         { "consumer",             OPTION_VAL_CONSUMER_EN },
         { "canonical",            OPTION_VAL_CANONICAL_EN },
         { "alternative",          OPTION_VAL_ALTERNATIVE_EN },
         { "rgb",                  OPTION_VAL_RGB_O3_EN },
         { "pal",                  OPTION_VAL_PAL_EN },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_EN },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_EN },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_EN },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_EN },
         { "raw",                  OPTION_VAL_RAW_EN },
         { "custom",               OPTION_VAL_CUSTOM_EN },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_EN,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_EN,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_EN,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_EN,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_EN,
      NULL,
      NESTOPIA_ASPECT_INFO_0_EN,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_EN },
         { "ntsc",        OPTION_VAL_NTSC_EN },
         { "pal",         OPTION_VAL_PAL_EN },
         { "4:3",         OPTION_VAL_4_3_EN },
         { "uncorrected", OPTION_VAL_UNCORRECTED_EN },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_EN,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_EN,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_EN,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_EN,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_EN,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_EN,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_EN,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_EN,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_EN,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_EN,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_EN,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_EN,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_EN,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_EN,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_EN,
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
      NESTOPIA_AUDIO_TYPE_LABEL_EN,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_EN,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_EN },
         { "stereo", OPTION_VAL_STEREO_EN },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_EN,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_EN,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_EN },
         { "ntsc",    OPTION_VAL_NTSC_EN },
         { "famicom", OPTION_VAL_FAMICOM_EN },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_EN,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_EN, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_EN,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_EN,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_EN },
         { "pointer", OPTION_VAL_POINTER_EN },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_EN,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_EN,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_EN },
         { "mouse", OPTION_VAL_MOUSE_EN },
         { "pointer", OPTION_VAL_POINTER_EN },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_EN,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_EN,
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
      NESTOPIA_TURBO_PULSE_LABEL_EN,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_EN,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_EN,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_EN,
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
      NESTOPIA_OVERCLOCK_LABEL_EN,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_EN,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_EN },
         { "2x", OPTION_VAL_2X_EN },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_EN,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_EN,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_EN },
         { "0xFF",   OPTION_VAL_0XFF_EN },
         { "random", OPTION_VAL_RANDOM_EN },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_en = {
   option_cats_en,
   option_defs_en
};

/* RETRO_LANGUAGE_EO */

#define CATEGORY_SYSTEM_LABEL_EO NULL
#define CATEGORY_SYSTEM_INFO_0_EO NULL
#define CATEGORY_VIDEO_LABEL_EO "Video Driver"
#define CATEGORY_VIDEO_INFO_0_EO NULL
#define CATEGORY_AUDIO_LABEL_EO "Audio Driver"
#define CATEGORY_AUDIO_INFO_0_EO NULL
#define CATEGORY_INPUT_LABEL_EO "Input Driver"
#define CATEGORY_INPUT_INFO_0_EO NULL
#define CATEGORY_HACKS_LABEL_EO NULL
#define CATEGORY_HACKS_INFO_0_EO NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_EO NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_EO NULL
#define OPTION_VAL_AUTO_EO NULL
#define OPTION_VAL_NTSC_EO NULL
#define OPTION_VAL_PAL_EO NULL
#define OPTION_VAL_FAMICOM_EO NULL
#define OPTION_VAL_DENDY_EO NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_EO NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_EO NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_EO NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_EO NULL
#define OPTION_VAL_COMPOSITE_EO NULL
#define OPTION_VAL_SVIDEO_EO NULL
#define OPTION_VAL_RGB_EO NULL
#define OPTION_VAL_MONOCHROME_EO NULL
#define NESTOPIA_PALETTE_LABEL_EO NULL
#define NESTOPIA_PALETTE_INFO_0_EO NULL
#define OPTION_VAL_CXA2025AS_EO NULL
#define OPTION_VAL_CONSUMER_EO NULL
#define OPTION_VAL_CANONICAL_EO NULL
#define OPTION_VAL_ALTERNATIVE_EO NULL
#define OPTION_VAL_RGB_O3_EO NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_EO NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_EO NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_EO NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_EO NULL
#define OPTION_VAL_RAW_EO NULL
#define OPTION_VAL_CUSTOM_EO NULL
#define NESTOPIA_OVERSCAN_V_LABEL_EO NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_EO NULL
#define NESTOPIA_OVERSCAN_H_LABEL_EO NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_EO NULL
#define NESTOPIA_ASPECT_LABEL_EO NULL
#define NESTOPIA_ASPECT_INFO_0_EO NULL
#define OPTION_VAL_4_3_EO NULL
#define OPTION_VAL_UNCORRECTED_EO NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_EO NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_EO NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_EO NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_EO NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_EO NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_EO NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_EO NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_EO NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_EO NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_EO NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_EO NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_EO NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_EO NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_EO NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_EO NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_EO NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_EO NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_EO NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_EO NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_EO NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_EO NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_EO NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_EO NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_EO NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_EO NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_EO NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_EO NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_EO NULL
#define OPTION_VAL_MONO_EO NULL
#define OPTION_VAL_STEREO_EO NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_EO NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_EO NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_EO NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_EO NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_EO NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_EO NULL
#define OPTION_VAL_MOUSE_EO NULL
#define OPTION_VAL_POINTER_EO NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_EO NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_EO NULL
#define OPTION_VAL_LIGHTGUN_EO NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_EO NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_EO NULL
#define NESTOPIA_TURBO_PULSE_LABEL_EO NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_EO NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_EO NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_EO NULL
#define NESTOPIA_OVERCLOCK_LABEL_EO NULL
#define NESTOPIA_OVERCLOCK_INFO_0_EO NULL
#define OPTION_VAL_1X_EO NULL
#define OPTION_VAL_2X_EO NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_EO NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_EO NULL
#define OPTION_VAL_0X00_EO NULL
#define OPTION_VAL_0XFF_EO NULL
#define OPTION_VAL_RANDOM_EO NULL

struct retro_core_option_v2_category option_cats_eo[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_EO,
      CATEGORY_SYSTEM_INFO_0_EO
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_EO,
      CATEGORY_VIDEO_INFO_0_EO
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_EO,
      CATEGORY_AUDIO_INFO_0_EO
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_EO,
      CATEGORY_INPUT_INFO_0_EO
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_EO,
      CATEGORY_HACKS_INFO_0_EO
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_eo[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_EO,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_EO,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_EO },
         { "ntsc",    OPTION_VAL_NTSC_EO },
         { "pal",     OPTION_VAL_PAL_EO },
         { "famicom", OPTION_VAL_FAMICOM_EO },
         { "dendy",   OPTION_VAL_DENDY_EO },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_EO,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_EO,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_EO,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_EO,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_EO },
         { "svideo",     OPTION_VAL_SVIDEO_EO },
         { "rgb",        OPTION_VAL_RGB_EO },
         { "monochrome", OPTION_VAL_MONOCHROME_EO },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_EO,
      NULL,
      NESTOPIA_PALETTE_INFO_0_EO,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_EO },
         { "consumer",             OPTION_VAL_CONSUMER_EO },
         { "canonical",            OPTION_VAL_CANONICAL_EO },
         { "alternative",          OPTION_VAL_ALTERNATIVE_EO },
         { "rgb",                  OPTION_VAL_RGB_O3_EO },
         { "pal",                  OPTION_VAL_PAL_EO },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_EO },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_EO },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_EO },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_EO },
         { "raw",                  OPTION_VAL_RAW_EO },
         { "custom",               OPTION_VAL_CUSTOM_EO },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_EO,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_EO,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_EO,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_EO,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_EO,
      NULL,
      NESTOPIA_ASPECT_INFO_0_EO,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_EO },
         { "ntsc",        OPTION_VAL_NTSC_EO },
         { "pal",         OPTION_VAL_PAL_EO },
         { "4:3",         OPTION_VAL_4_3_EO },
         { "uncorrected", OPTION_VAL_UNCORRECTED_EO },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_EO,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_EO,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_EO,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_EO,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_EO,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_EO,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_EO,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_EO,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_EO,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_EO,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_EO,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_EO,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_EO,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_EO,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_EO,
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
      NESTOPIA_AUDIO_TYPE_LABEL_EO,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_EO,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_EO },
         { "stereo", OPTION_VAL_STEREO_EO },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_EO,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_EO,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_EO },
         { "ntsc",    OPTION_VAL_NTSC_EO },
         { "famicom", OPTION_VAL_FAMICOM_EO },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_EO,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_EO, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_EO,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_EO,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_EO },
         { "pointer", OPTION_VAL_POINTER_EO },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_EO,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_EO,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_EO },
         { "mouse", OPTION_VAL_MOUSE_EO },
         { "pointer", OPTION_VAL_POINTER_EO },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_EO,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_EO,
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
      NESTOPIA_TURBO_PULSE_LABEL_EO,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_EO,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_EO,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_EO,
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
      NESTOPIA_OVERCLOCK_LABEL_EO,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_EO,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_EO },
         { "2x", OPTION_VAL_2X_EO },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_EO,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_EO,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_EO },
         { "0xFF",   OPTION_VAL_0XFF_EO },
         { "random", OPTION_VAL_RANDOM_EO },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_eo = {
   option_cats_eo,
   option_defs_eo
};

/* RETRO_LANGUAGE_ES */

#define CATEGORY_SYSTEM_LABEL_ES "Sistema"
#define CATEGORY_SYSTEM_INFO_0_ES NULL
#define CATEGORY_VIDEO_LABEL_ES "Vídeo"
#define CATEGORY_VIDEO_INFO_0_ES "Cambia los ajustes de relación de aspecto, recorte de pantalla, paleta de colores y los filtros de vídeo."
#define CATEGORY_AUDIO_LABEL_ES NULL
#define CATEGORY_AUDIO_INFO_0_ES "Cambia los efectos de audio y el volumen de los canales."
#define CATEGORY_INPUT_LABEL_ES "Entrada"
#define CATEGORY_INPUT_INFO_0_ES "Cambia los ajustes de los dispositivos de entrada."
#define CATEGORY_HACKS_LABEL_ES "Arreglos de emulación"
#define CATEGORY_HACKS_INFO_0_ES "Cambia las opciones de velocidad del procesador y de precisión de la emulación que afectan al rendimiento a bajo nivel y a la compatibilidad."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_ES "Región del sistema"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_ES "«Selección automática» utilizará el archivo de base de datos NstDatabase.xml para detectar automáticamente la región. En caso de no existir la base de datos, se utilizará la relación NTSC por defecto."
#define OPTION_VAL_AUTO_ES "Selección automática"
#define OPTION_VAL_NTSC_ES NULL
#define OPTION_VAL_PAL_ES NULL
#define OPTION_VAL_FAMICOM_ES NULL
#define OPTION_VAL_DENDY_ES NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_ES "Autoinsertar disquete de FDS"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_ES "Introduce automáticamente el primer disquete del FDS al reiniciar."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_ES "Filtro NTSC de Blargg"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_ES "Activa los filtros NTSC de Blargg."
#define OPTION_VAL_COMPOSITE_ES "Vídeo compuesto"
#define OPTION_VAL_SVIDEO_ES NULL
#define OPTION_VAL_RGB_ES "SCART RGB"
#define OPTION_VAL_MONOCHROME_ES "Monocromo"
#define NESTOPIA_PALETTE_LABEL_ES "Paleta"
#define NESTOPIA_PALETTE_INFO_0_ES NULL
#define OPTION_VAL_CXA2025AS_ES NULL
#define OPTION_VAL_CONSUMER_ES "Para consumidores"
#define OPTION_VAL_CANONICAL_ES "Canónica"
#define OPTION_VAL_ALTERNATIVE_ES "Alternativa"
#define OPTION_VAL_RGB_O3_ES NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_ES "Directa de vídeo compuesto por FBx"
#define OPTION_VAL_PVM_STYLE_D93_FBX_ES "D93 tipo PVM de FBx"
#define OPTION_VAL_NTSC_HARDWARE_FBX_ES "Hardware NTSC de FBx"
#define OPTION_VAL_NES_CLASSIC_FBX_FS_ES "NES Classic de FBx (corregida)"
#define OPTION_VAL_RAW_ES "Datos en bruto"
#define OPTION_VAL_CUSTOM_ES "Personalizar"
#define NESTOPIA_OVERSCAN_V_LABEL_ES "Camuflar sobrebarrido (vertical)"
#define NESTOPIA_OVERSCAN_V_INFO_0_ES "Camufla (la parte vertical de) los posibles defectos visuales aleatorios de vídeo que normalmente ocultaría el marco de un televisor de definición estándar."
#define NESTOPIA_OVERSCAN_H_LABEL_ES "Camuflar sobrebarrido (horizontal)"
#define NESTOPIA_OVERSCAN_H_INFO_0_ES "Camufla (la parte horizontal de) los posibles defectos visuales aleatorios de vídeo que normalmente ocultaría el marco de un televisor de definición estándar."
#define NESTOPIA_ASPECT_LABEL_ES "Relación de aspecto preferida"
#define NESTOPIA_ASPECT_INFO_0_ES NULL
#define OPTION_VAL_4_3_ES NULL
#define OPTION_VAL_UNCORRECTED_ES "Sin corregir"
#define NESTOPIA_GENIE_DISTORTION_LABEL_ES "Distorsión sonora del Game Genie"
#define NESTOPIA_GENIE_DISTORTION_INFO_0_ES "El sistema de trucos Game Genie podía introducir involuntariamente una distorsión en el sonido de los juegos. Al activar esta opción podrás simular esa distorsión."
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_ES "Mostrar ajustes avanzados de audio (es necesario abrir y cerrar el menú)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_ES "Permite configurar los ajustes de bajo nivel de los canales de audio."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_ES "Volumen del canal de ondas cuadradas 1 (%)"
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_ES "Modifica el volumen del canal de ondas cuadradas 1 (en porcentaje)."
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_ES "Volumen del canal de ondas cuadradas 2 (%)"
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_ES "Modifica el volumen del canal de ondas cuadradas 2 (en porcentaje)."
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_ES "Volumen del canal de ondas triangulares (%)"
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_ES "Modifica el volumen del canal de ondas triangulares (en porcentaje)."
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_ES "Volumen del canal de ruido (%)"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_ES "Modifica el volumen del canal de ruido (en porcentaje)."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_ES "Volumen del canal de DPCM (%)"
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_ES "Modifica el volumen del canal de DPCM (en porcentaje)."
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_ES "Volumen del canal de FDS (%)"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_ES "Modifica el volumen del canal de la FDS (en porcentaje)."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_ES "Volumen del canal de MMC5 (%)"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_ES "Modifica el volumen del canal de MMC5 (en porcentaje)."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_ES "Volumen del canal de VRC6 (%)"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_ES "Modifica el volumen del canal de VRC6 (en porcentaje)."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_ES "Volumen del canal de VRC7 (%)"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_ES "Modifica el volumen del canal de VRC7 (en porcentaje)."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_ES "Volumen del canal de N163 (%)"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_ES "Modifica el volumen del canal de N163 (en porcentaje)."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_ES "Volumen del canal de S5B (%)"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_ES NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_ES NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_ES NULL
#define OPTION_VAL_MONO_ES NULL
#define OPTION_VAL_STEREO_ES NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_ES "Adaptador para 4 jugadores"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_ES "Selecciona de forma manual un adaptador para cuatro jugadores si es necesario. Algunos juegos no reconocerán correctamente el adaptador utilizando la base de datos NstDatabase.xml, esta opción sirve para ponerle remedio."
#define NESTOPIA_BUTTON_SHIFT_LABEL_ES "Rotar los botones en sentido horario"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_ES "Cambia el uso de los botones A/B/X/Y girando su utilización en el sentido de las agujas del reloj."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_ES "Dispositivo para Arkanoid"
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_ES "Selecciona el dispositivo que quieres usar para el mando analógico de Arkanoid."
#define OPTION_VAL_MOUSE_ES "Ratón"
#define OPTION_VAL_POINTER_ES "Puntero/Pantalla táctil"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_ES "Dispositivo para Zapper"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_ES "Selecciona el dispositivo que quieres usar para la Zapper."
#define OPTION_VAL_LIGHTGUN_ES "Pistola de luz"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_ES "Mostrar punto de mira"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_ES "Muestra un punto de mira al utilizar la Zapper."
#define NESTOPIA_TURBO_PULSE_LABEL_ES "Velocidad de pulsación del turbo"
#define NESTOPIA_TURBO_PULSE_INFO_0_ES "Establece la velocidad de pulsación de los botones Turbo B y Turbo A."
#define NESTOPIA_NOSPRITELIMIT_LABEL_ES "Desactivar límite de sprites"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_ES "Elimina el límite de 8 sprites por línea de barrido que tenía el hardware original."
#define NESTOPIA_OVERCLOCK_LABEL_ES "Velocidad de la CPU (aceleración)"
#define NESTOPIA_OVERCLOCK_INFO_0_ES "Aumenta la velocidad de la CPU emulada."
#define OPTION_VAL_1X_ES NULL
#define OPTION_VAL_2X_ES NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_ES "Estado de la RAM al encender"
#define NESTOPIA_RAM_POWER_STATE_INFO_0_ES "Indica los valores que tendrá la RAM al arrancar. Por ejemplo, algunos juegos necesitan un valor concreto en la RAM para generar números aleatorios."
#define OPTION_VAL_0X00_ES NULL
#define OPTION_VAL_0XFF_ES NULL
#define OPTION_VAL_RANDOM_ES "Al azar"

struct retro_core_option_v2_category option_cats_es[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_ES,
      CATEGORY_SYSTEM_INFO_0_ES
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_ES,
      CATEGORY_VIDEO_INFO_0_ES
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_ES,
      CATEGORY_AUDIO_INFO_0_ES
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_ES,
      CATEGORY_INPUT_INFO_0_ES
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_ES,
      CATEGORY_HACKS_INFO_0_ES
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_es[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_ES,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_ES,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_ES },
         { "ntsc",    OPTION_VAL_NTSC_ES },
         { "pal",     OPTION_VAL_PAL_ES },
         { "famicom", OPTION_VAL_FAMICOM_ES },
         { "dendy",   OPTION_VAL_DENDY_ES },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_ES,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_ES,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_ES,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_ES,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_ES },
         { "svideo",     OPTION_VAL_SVIDEO_ES },
         { "rgb",        OPTION_VAL_RGB_ES },
         { "monochrome", OPTION_VAL_MONOCHROME_ES },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_ES,
      NULL,
      NESTOPIA_PALETTE_INFO_0_ES,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_ES },
         { "consumer",             OPTION_VAL_CONSUMER_ES },
         { "canonical",            OPTION_VAL_CANONICAL_ES },
         { "alternative",          OPTION_VAL_ALTERNATIVE_ES },
         { "rgb",                  OPTION_VAL_RGB_O3_ES },
         { "pal",                  OPTION_VAL_PAL_ES },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_ES },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_ES },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_ES },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_ES },
         { "raw",                  OPTION_VAL_RAW_ES },
         { "custom",               OPTION_VAL_CUSTOM_ES },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_ES,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_ES,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_ES,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_ES,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_ES,
      NULL,
      NESTOPIA_ASPECT_INFO_0_ES,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_ES },
         { "ntsc",        OPTION_VAL_NTSC_ES },
         { "pal",         OPTION_VAL_PAL_ES },
         { "4:3",         OPTION_VAL_4_3_ES },
         { "uncorrected", OPTION_VAL_UNCORRECTED_ES },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_ES,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_ES,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_ES,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_ES,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_ES,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_ES,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_ES,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_ES,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_ES,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_ES,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_ES,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_ES,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_ES,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_ES,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_ES,
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
      NESTOPIA_AUDIO_TYPE_LABEL_ES,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_ES,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_ES },
         { "stereo", OPTION_VAL_STEREO_ES },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_ES,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_ES,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_ES },
         { "ntsc",    OPTION_VAL_NTSC_ES },
         { "famicom", OPTION_VAL_FAMICOM_ES },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_ES,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_ES, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_ES,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_ES,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_ES },
         { "pointer", OPTION_VAL_POINTER_ES },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_ES,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_ES,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_ES },
         { "mouse", OPTION_VAL_MOUSE_ES },
         { "pointer", OPTION_VAL_POINTER_ES },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_ES,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_ES,
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
      NESTOPIA_TURBO_PULSE_LABEL_ES,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_ES,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_ES,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_ES,
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
      NESTOPIA_OVERCLOCK_LABEL_ES,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_ES,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_ES },
         { "2x", OPTION_VAL_2X_ES },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_ES,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_ES,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_ES },
         { "0xFF",   OPTION_VAL_0XFF_ES },
         { "random", OPTION_VAL_RANDOM_ES },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_es = {
   option_cats_es,
   option_defs_es
};

/* RETRO_LANGUAGE_FA */

#define CATEGORY_SYSTEM_LABEL_FA "سیستم"
#define CATEGORY_SYSTEM_INFO_0_FA NULL
#define CATEGORY_VIDEO_LABEL_FA "ویدیو"
#define CATEGORY_VIDEO_INFO_0_FA NULL
#define CATEGORY_AUDIO_LABEL_FA "صدا"
#define CATEGORY_AUDIO_INFO_0_FA NULL
#define CATEGORY_INPUT_LABEL_FA "داده"
#define CATEGORY_INPUT_INFO_0_FA NULL
#define CATEGORY_HACKS_LABEL_FA NULL
#define CATEGORY_HACKS_INFO_0_FA NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_FA NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_FA NULL
#define OPTION_VAL_AUTO_FA "خودکار"
#define OPTION_VAL_NTSC_FA NULL
#define OPTION_VAL_PAL_FA NULL
#define OPTION_VAL_FAMICOM_FA NULL
#define OPTION_VAL_DENDY_FA NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_FA NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_FA NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_FA NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_FA NULL
#define OPTION_VAL_COMPOSITE_FA NULL
#define OPTION_VAL_SVIDEO_FA NULL
#define OPTION_VAL_RGB_FA NULL
#define OPTION_VAL_MONOCHROME_FA NULL
#define NESTOPIA_PALETTE_LABEL_FA NULL
#define NESTOPIA_PALETTE_INFO_0_FA NULL
#define OPTION_VAL_CXA2025AS_FA NULL
#define OPTION_VAL_CONSUMER_FA NULL
#define OPTION_VAL_CANONICAL_FA NULL
#define OPTION_VAL_ALTERNATIVE_FA NULL
#define OPTION_VAL_RGB_O3_FA NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_FA NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_FA NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_FA NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_FA NULL
#define OPTION_VAL_RAW_FA NULL
#define OPTION_VAL_CUSTOM_FA "سفارشی"
#define NESTOPIA_OVERSCAN_V_LABEL_FA NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_FA NULL
#define NESTOPIA_OVERSCAN_H_LABEL_FA NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_FA NULL
#define NESTOPIA_ASPECT_LABEL_FA NULL
#define NESTOPIA_ASPECT_INFO_0_FA NULL
#define OPTION_VAL_4_3_FA NULL
#define OPTION_VAL_UNCORRECTED_FA NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_FA NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_FA NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_FA NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_FA NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_FA NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_FA NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_FA NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_FA NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_FA NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_FA NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_FA NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_FA NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_FA NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_FA NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_FA NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_FA NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_FA NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_FA NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_FA NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_FA NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_FA NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_FA NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_FA NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_FA NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_FA NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_FA NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_FA NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_FA NULL
#define OPTION_VAL_MONO_FA NULL
#define OPTION_VAL_STEREO_FA NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_FA NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_FA NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_FA NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_FA NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_FA NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_FA NULL
#define OPTION_VAL_MOUSE_FA NULL
#define OPTION_VAL_POINTER_FA NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_FA NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_FA NULL
#define OPTION_VAL_LIGHTGUN_FA NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_FA NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_FA NULL
#define NESTOPIA_TURBO_PULSE_LABEL_FA NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_FA NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_FA NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_FA NULL
#define NESTOPIA_OVERCLOCK_LABEL_FA NULL
#define NESTOPIA_OVERCLOCK_INFO_0_FA NULL
#define OPTION_VAL_1X_FA NULL
#define OPTION_VAL_2X_FA NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_FA NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_FA NULL
#define OPTION_VAL_0X00_FA NULL
#define OPTION_VAL_0XFF_FA NULL
#define OPTION_VAL_RANDOM_FA NULL

struct retro_core_option_v2_category option_cats_fa[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_FA,
      CATEGORY_SYSTEM_INFO_0_FA
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_FA,
      CATEGORY_VIDEO_INFO_0_FA
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_FA,
      CATEGORY_AUDIO_INFO_0_FA
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_FA,
      CATEGORY_INPUT_INFO_0_FA
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_FA,
      CATEGORY_HACKS_INFO_0_FA
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_fa[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_FA,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_FA,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_FA },
         { "ntsc",    OPTION_VAL_NTSC_FA },
         { "pal",     OPTION_VAL_PAL_FA },
         { "famicom", OPTION_VAL_FAMICOM_FA },
         { "dendy",   OPTION_VAL_DENDY_FA },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_FA,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_FA,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_FA,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_FA,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_FA },
         { "svideo",     OPTION_VAL_SVIDEO_FA },
         { "rgb",        OPTION_VAL_RGB_FA },
         { "monochrome", OPTION_VAL_MONOCHROME_FA },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_FA,
      NULL,
      NESTOPIA_PALETTE_INFO_0_FA,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_FA },
         { "consumer",             OPTION_VAL_CONSUMER_FA },
         { "canonical",            OPTION_VAL_CANONICAL_FA },
         { "alternative",          OPTION_VAL_ALTERNATIVE_FA },
         { "rgb",                  OPTION_VAL_RGB_O3_FA },
         { "pal",                  OPTION_VAL_PAL_FA },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_FA },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_FA },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_FA },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_FA },
         { "raw",                  OPTION_VAL_RAW_FA },
         { "custom",               OPTION_VAL_CUSTOM_FA },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_FA,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_FA,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_FA,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_FA,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_FA,
      NULL,
      NESTOPIA_ASPECT_INFO_0_FA,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_FA },
         { "ntsc",        OPTION_VAL_NTSC_FA },
         { "pal",         OPTION_VAL_PAL_FA },
         { "4:3",         OPTION_VAL_4_3_FA },
         { "uncorrected", OPTION_VAL_UNCORRECTED_FA },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_FA,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_FA,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_FA,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_FA,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_FA,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_FA,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_FA,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_FA,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_FA,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_FA,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_FA,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_FA,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_FA,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_FA,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_FA,
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
      NESTOPIA_AUDIO_TYPE_LABEL_FA,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_FA,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_FA },
         { "stereo", OPTION_VAL_STEREO_FA },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_FA,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_FA,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_FA },
         { "ntsc",    OPTION_VAL_NTSC_FA },
         { "famicom", OPTION_VAL_FAMICOM_FA },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_FA,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_FA, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_FA,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_FA,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_FA },
         { "pointer", OPTION_VAL_POINTER_FA },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_FA,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_FA,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_FA },
         { "mouse", OPTION_VAL_MOUSE_FA },
         { "pointer", OPTION_VAL_POINTER_FA },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_FA,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_FA,
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
      NESTOPIA_TURBO_PULSE_LABEL_FA,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_FA,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_FA,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_FA,
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
      NESTOPIA_OVERCLOCK_LABEL_FA,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_FA,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_FA },
         { "2x", OPTION_VAL_2X_FA },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_FA,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_FA,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_FA },
         { "0xFF",   OPTION_VAL_0XFF_FA },
         { "random", OPTION_VAL_RANDOM_FA },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_fa = {
   option_cats_fa,
   option_defs_fa
};

/* RETRO_LANGUAGE_FI */

#define CATEGORY_SYSTEM_LABEL_FI "Järjestelmä"
#define CATEGORY_SYSTEM_INFO_0_FI NULL
#define CATEGORY_VIDEO_LABEL_FI NULL
#define CATEGORY_VIDEO_INFO_0_FI "Muuta kuvasuhteen, näytön rajauksen, väripaletin ja videosuodattimen asetuksia."
#define CATEGORY_AUDIO_LABEL_FI "Ääni"
#define CATEGORY_AUDIO_INFO_0_FI "Muuta äänitehosteita ja äänikanavien äänenvoimakkuuksia."
#define CATEGORY_INPUT_LABEL_FI "Syöte"
#define CATEGORY_INPUT_INFO_0_FI "Muuta syöttölaitteita ja muita syöttöön liittyviä asetuksia."
#define CATEGORY_HACKS_LABEL_FI "Emulointikikat"
#define CATEGORY_HACKS_INFO_0_FI "Muuta prosessorin ylikellotus- ja emulointitarkkuusasetuksia, jotka vaikuttavat matalan tason suorituskykyyn ja yhteensopivuuteen."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_FI "Järjestelmän alue"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_FI "\"Automaattinen\" käyttää NstDatabase.xml tietokannan tiedostoa alueen tunnistamiseen. Jos tietokantaa ei ole saatavilla, oletuksena käytetään NTSC."
#define OPTION_VAL_AUTO_FI "Automaattinen"
#define OPTION_VAL_NTSC_FI NULL
#define OPTION_VAL_PAL_FI NULL
#define OPTION_VAL_FAMICOM_FI NULL
#define OPTION_VAL_DENDY_FI NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_FI "FDS automaattinen syöttö"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_FI "Syötä automaattisesti ensimmäinen FDS-levyke uudelleenkäynnistyksessä."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_FI "Blargg NTSC suodatin"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_FI "Ota käyttöön Blargg NTSC suodattimet."
#define OPTION_VAL_COMPOSITE_FI "Komposiitti video"
#define OPTION_VAL_SVIDEO_FI NULL
#define OPTION_VAL_RGB_FI NULL
#define OPTION_VAL_MONOCHROME_FI "Yksivärinen"
#define NESTOPIA_PALETTE_LABEL_FI "Paletti"
#define NESTOPIA_PALETTE_INFO_0_FI NULL
#define OPTION_VAL_CXA2025AS_FI NULL
#define OPTION_VAL_CONSUMER_FI "Kuluttaja"
#define OPTION_VAL_CANONICAL_FI "Kanoninen"
#define OPTION_VAL_ALTERNATIVE_FI "Vaihtoehtoinen"
#define OPTION_VAL_RGB_O3_FI NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_FI "Komposiitti Direct FBx"
#define OPTION_VAL_PVM_STYLE_D93_FBX_FI "PVM-tyylinen D93 FBx"
#define OPTION_VAL_NTSC_HARDWARE_FBX_FI "NTSC laitteisto FBx"
#define OPTION_VAL_NES_CLASSIC_FBX_FS_FI "NES Classic FBx (korjattu)"
#define OPTION_VAL_RAW_FI "Tavallinen"
#define OPTION_VAL_CUSTOM_FI "Mukautettu"
#define NESTOPIA_OVERSCAN_V_LABEL_FI "Naamioi yliskannaus (Pystysuora)"
#define NESTOPIA_OVERSCAN_V_INFO_0_FI "Naamioi mahdollisesti satunnainen (pystysuora) vääristynyt videon ulostulo, joka olisi jäänyt piiloon kehysten reunan taakse perinteisen tarkkuuden näytöllä."
#define NESTOPIA_OVERSCAN_H_LABEL_FI "Naamioi yliskannaus (Vaakasuora)"
#define NESTOPIA_OVERSCAN_H_INFO_0_FI "Naamioi mahdollisesti satunnainen (vaakasuora) vääristynyt videon ulostulo, joka olisi jäänyt piiloon kehysten reunan taakse perinteisen tarkkuuden näytöllä."
#define NESTOPIA_ASPECT_LABEL_FI "Ensisijainen kuvasuhde"
#define NESTOPIA_ASPECT_INFO_0_FI NULL
#define OPTION_VAL_4_3_FI NULL
#define OPTION_VAL_UNCORRECTED_FI "Ei korjattu"
#define NESTOPIA_GENIE_DISTORTION_LABEL_FI "Game Genien äänen vääristymä"
#define NESTOPIA_GENIE_DISTORTION_INFO_0_FI "Game Genie huijauslaite voi vahingossa aiheuttaa äänen vääristymisen peleissä. Ottamalla tämän käyttöön voit simuloida vääristymää, jonka se lisäisi pelin ääniin."
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_FI "Näytä tarkemmat ääniasetukset (Avaa valikko uudestaan)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_FI "Ota käyttöön alhaisen tason äänikanavan parametrien muokkaus."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_FI "Neliöaalto kanavan 1 äänenvoimakkuus %"
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_FI "Muokkaa neliöaalto kanavan 1 äänenvoimakkuutta %."
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_FI "Neliöaalto kanavan 2 äänenvoimakkuus %"
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_FI "Muokkaa neliöaalto kanavan 2 äänenvoimakkuutta %."
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_FI "Kolmioaalto kanavan äänenvoimakkuus %"
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_FI "Muokkaa kolmioaalto kanavan äänenvoimakkuutta %."
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_FI "Äänikanavan äänenvoimakkuus %"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_FI "Muokkaa äänikanavan äänenvoimakkuutta %."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_FI "DPCM kanavan äänenvoimakkuus %"
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_FI "Muokkaa DPCM-kanavan äänenvoimakkuutta %."
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_FI "FDS-kanavan äänenvoimakkuutta %"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_FI "Muokkaa FDS-kanavan äänenvoimakkuutta %."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_FI "MMC5-kanavan äänenvoimakkuus %"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_FI "Muokkaa MMC5-kanavan äänenvoimakkuus %."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_FI "VRC6-kanavan äänenvoimakkuus %"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_FI "Muokkaa VRC6-kanavan äänenvoimakkuutta %."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_FI "VRC7-kanavan äänenvoimakkuus %"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_FI "Muokkaa VRC7-kanavan äänenvoimakkuutta %."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_FI "N163-kanavan äänenvoimakkuus %"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_FI "Muokkaa N163-kanavan äänenvoimakkuutta %."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_FI "S5B-kanavan äänenvoimakkuus %"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_FI NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_FI NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_FI NULL
#define OPTION_VAL_MONO_FI NULL
#define OPTION_VAL_STEREO_FI NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_FI "4 pelaajan sovitin"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_FI "Valitse tarvittaessa manuaalisesti 4 pelaajan sovitin. Jotkin pelit eivät tunnista adapteria oikein NstDatabasen.xml tietokannan kautta, tämän valinnan pitäisi auttaa korjaamaan se."
#define NESTOPIA_BUTTON_SHIFT_LABEL_FI "Siirrä painikkeita myötäpäivään"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_FI "Kierrä A/B/X/Y painikkeita myötäpäivään."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_FI "Arkanoid-laite"
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_FI "Valitse laite, jota haluat käyttää Arkanoidissa."
#define OPTION_VAL_MOUSE_FI "Hiiri"
#define OPTION_VAL_POINTER_FI "Osoitin"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_FI "Zapper laite"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_FI "Valitse laite, jota haluat käyttää Zapperille."
#define OPTION_VAL_LIGHTGUN_FI "Valoase"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_FI "Näytä tähtäin"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_FI "Valitse näytetäänkö tähtäin kun Zapperia käytetään."
#define NESTOPIA_TURBO_PULSE_LABEL_FI "Turbon pulssin nopeus"
#define NESTOPIA_TURBO_PULSE_INFO_0_FI "Aseta Turbo B ja Turbo A -painikkeiden pulssin nopeus."
#define NESTOPIA_NOSPRITELIMIT_LABEL_FI "Remove sprite-rajoitus"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_FI "Poista 8 spriten vaakasuuntainen laitteisto rajoitus."
#define NESTOPIA_OVERCLOCK_LABEL_FI "Prosessorin nopeus (Ylikellotus)"
#define NESTOPIA_OVERCLOCK_INFO_0_FI "Ylikellota emuloitu prosessori."
#define OPTION_VAL_1X_FI NULL
#define OPTION_VAL_2X_FI NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_FI "RAM-käynnistystila"
#define NESTOPIA_RAM_POWER_STATE_INFO_0_FI "RAM-arvot käynnistyksessä. Jotkin pelit tarvitsevat alustavat RAM-arvot joidenkin satunnaisten numeroiden luomiseen."
#define OPTION_VAL_0X00_FI NULL
#define OPTION_VAL_0XFF_FI NULL
#define OPTION_VAL_RANDOM_FI "Satunnainen"

struct retro_core_option_v2_category option_cats_fi[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_FI,
      CATEGORY_SYSTEM_INFO_0_FI
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_FI,
      CATEGORY_VIDEO_INFO_0_FI
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_FI,
      CATEGORY_AUDIO_INFO_0_FI
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_FI,
      CATEGORY_INPUT_INFO_0_FI
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_FI,
      CATEGORY_HACKS_INFO_0_FI
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_fi[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_FI,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_FI,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_FI },
         { "ntsc",    OPTION_VAL_NTSC_FI },
         { "pal",     OPTION_VAL_PAL_FI },
         { "famicom", OPTION_VAL_FAMICOM_FI },
         { "dendy",   OPTION_VAL_DENDY_FI },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_FI,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_FI,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_FI,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_FI,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_FI },
         { "svideo",     OPTION_VAL_SVIDEO_FI },
         { "rgb",        OPTION_VAL_RGB_FI },
         { "monochrome", OPTION_VAL_MONOCHROME_FI },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_FI,
      NULL,
      NESTOPIA_PALETTE_INFO_0_FI,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_FI },
         { "consumer",             OPTION_VAL_CONSUMER_FI },
         { "canonical",            OPTION_VAL_CANONICAL_FI },
         { "alternative",          OPTION_VAL_ALTERNATIVE_FI },
         { "rgb",                  OPTION_VAL_RGB_O3_FI },
         { "pal",                  OPTION_VAL_PAL_FI },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_FI },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_FI },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_FI },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_FI },
         { "raw",                  OPTION_VAL_RAW_FI },
         { "custom",               OPTION_VAL_CUSTOM_FI },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_FI,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_FI,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_FI,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_FI,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_FI,
      NULL,
      NESTOPIA_ASPECT_INFO_0_FI,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_FI },
         { "ntsc",        OPTION_VAL_NTSC_FI },
         { "pal",         OPTION_VAL_PAL_FI },
         { "4:3",         OPTION_VAL_4_3_FI },
         { "uncorrected", OPTION_VAL_UNCORRECTED_FI },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_FI,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_FI,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_FI,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_FI,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_FI,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_FI,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_FI,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_FI,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_FI,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_FI,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_FI,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_FI,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_FI,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_FI,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_FI,
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
      NESTOPIA_AUDIO_TYPE_LABEL_FI,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_FI,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_FI },
         { "stereo", OPTION_VAL_STEREO_FI },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_FI,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_FI,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_FI },
         { "ntsc",    OPTION_VAL_NTSC_FI },
         { "famicom", OPTION_VAL_FAMICOM_FI },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_FI,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_FI, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_FI,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_FI,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_FI },
         { "pointer", OPTION_VAL_POINTER_FI },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_FI,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_FI,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_FI },
         { "mouse", OPTION_VAL_MOUSE_FI },
         { "pointer", OPTION_VAL_POINTER_FI },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_FI,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_FI,
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
      NESTOPIA_TURBO_PULSE_LABEL_FI,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_FI,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_FI,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_FI,
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
      NESTOPIA_OVERCLOCK_LABEL_FI,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_FI,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_FI },
         { "2x", OPTION_VAL_2X_FI },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_FI,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_FI,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_FI },
         { "0xFF",   OPTION_VAL_0XFF_FI },
         { "random", OPTION_VAL_RANDOM_FI },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_fi = {
   option_cats_fi,
   option_defs_fi
};

/* RETRO_LANGUAGE_FR */

#define CATEGORY_SYSTEM_LABEL_FR "Système"
#define CATEGORY_SYSTEM_INFO_0_FR NULL
#define CATEGORY_VIDEO_LABEL_FR "Vidéo"
#define CATEGORY_VIDEO_INFO_0_FR "Modifier les réglages du rapport d'aspect, du recadrage de l'affichage, de la palette de couleurs et du filtre vidéo."
#define CATEGORY_AUDIO_LABEL_FR NULL
#define CATEGORY_AUDIO_INFO_0_FR "Modifier les effets audio et le volume des canaux."
#define CATEGORY_INPUT_LABEL_FR "Entrées"
#define CATEGORY_INPUT_INFO_0_FR "Modifier les périphériques d'entrée et les autres réglages liés aux entrées."
#define CATEGORY_HACKS_LABEL_FR "Hacks d'émulation"
#define CATEGORY_HACKS_INFO_0_FR "Modifier les réglages d'overclocking du processeur et de la précision d'émulation qui affectent les performances de bas niveau et la compatibilité."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_FR "Région du système"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_FR "'Auto' utilisera le fichier de base de données NstDatabase.xml pour la détection automatique de la région. S'il n'y a pas de base de données présente, elle sera sur NTSC par défaut."
#define OPTION_VAL_AUTO_FR NULL
#define OPTION_VAL_NTSC_FR NULL
#define OPTION_VAL_PAL_FR NULL
#define OPTION_VAL_FAMICOM_FR NULL
#define OPTION_VAL_DENDY_FR NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_FR "Insertion automatique du FDS"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_FR "Insérer automatiquement le premier disque FDS à la réinitialisation."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_FR "Filtre NTSC de Blargg"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_FR "Activer les filtres NTSC de Blargg."
#define OPTION_VAL_COMPOSITE_FR "Vidéo composite"
#define OPTION_VAL_SVIDEO_FR NULL
#define OPTION_VAL_RGB_FR "Péritel RVB (SCART)"
#define OPTION_VAL_MONOCHROME_FR NULL
#define NESTOPIA_PALETTE_LABEL_FR NULL
#define NESTOPIA_PALETTE_INFO_0_FR NULL
#define OPTION_VAL_CXA2025AS_FR NULL
#define OPTION_VAL_CONSUMER_FR "Consommateur"
#define OPTION_VAL_CANONICAL_FR "Canonique"
#define OPTION_VAL_ALTERNATIVE_FR NULL
#define OPTION_VAL_RGB_O3_FR "RVB"
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_FR "FBx composite direct"
#define OPTION_VAL_PVM_STYLE_D93_FBX_FR "FBx style PVM D93"
#define OPTION_VAL_NTSC_HARDWARE_FBX_FR "FBx matériel NTSC"
#define OPTION_VAL_NES_CLASSIC_FBX_FS_FR "FBx NES Classic (corrigée)"
#define OPTION_VAL_RAW_FR "Données brutes"
#define OPTION_VAL_CUSTOM_FR "Personnalisée"
#define NESTOPIA_OVERSCAN_V_LABEL_FR "Masquer le surbalayage (vertical)"
#define NESTOPIA_OVERSCAN_V_INFO_0_FR "Masquer (verticalement) la sortie vidéo potentiellement buggée et aléatoire qui aurait été cachée par le biseau autour du bord d'un écran de télévision définition standard."
#define NESTOPIA_OVERSCAN_H_LABEL_FR "Masquer le surbalayage (horizontal)"
#define NESTOPIA_OVERSCAN_H_INFO_0_FR "Masquer (horizontalement) la sortie vidéo potentiellement buggée et aléatoire qui aurait été cachée par le biseau autour du bord d'un écran de télévision définition standard."
#define NESTOPIA_ASPECT_LABEL_FR "Rapport d'aspect préféré"
#define NESTOPIA_ASPECT_INFO_0_FR NULL
#define OPTION_VAL_4_3_FR NULL
#define OPTION_VAL_UNCORRECTED_FR "Non corrigé"
#define NESTOPIA_GENIE_DISTORTION_LABEL_FR "Distorsion sonore du Game Genie"
#define NESTOPIA_GENIE_DISTORTION_INFO_0_FR "Le module de cheat Game Genie pouvait introduire par inadvertance une distorsion du son dans les jeux. En activant ceci, vous pouvez simuler la distorsion qu'il ajouterait au son d'un jeu."
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_FR "Afficher les paramètres audio avancés (Réouvrir le menu)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_FR "Activer la configuration des paramètres des canaux audio de bas niveau."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_FR "Volume du canal carré 1 (%)"
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_FR "Modifier le volume du canal carré 1 (%)."
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_FR "Volume du canal carré 2 (%)"
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_FR "Modifier le volume du canal carré 2 (%)."
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_FR "Volume du canal triangle (%)"
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_FR "Modifier le volume du canal triangle (%)."
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_FR "Volume du canal de bruit (%)"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_FR "Modifier le volume du canal de bruit (%)."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_FR "Volume du canal DPCM (%)"
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_FR "Modifier le volume du canal DPCM (%)."
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_FR "Volume du canal FDS (%)"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_FR "Modifier le volume du canal FDS (%)."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_FR "Volume du canal MMC5 (%)"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_FR "Modifier le volume du canal MMC5 (%)."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_FR "Volume du canal VRC6 (%)"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_FR "Modifier le volume du canal VRC6 (%)."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_FR "Volume du canal VRC7 (%)"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_FR "Modifier le volume du canal VRC7 (%)."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_FR "Volume du canal N163 (%)"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_FR "Modifier le volume du canal N163 (%)."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_FR "Volume du canal S5B (%)"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_FR NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_FR NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_FR NULL
#define OPTION_VAL_MONO_FR NULL
#define OPTION_VAL_STEREO_FR NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_FR "Adaptateur 4 joueurs"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_FR "Sélectionner manuellement un adaptateur 4 joueurs si nécessaire. Certains jeux ne reconnaîtront pas correctement l'adaptateur à partir de la base de données NstDatabase.xml, cette option devrait aider à corriger cela."
#define NESTOPIA_BUTTON_SHIFT_LABEL_FR "Touches décalées dans le sens horaire"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_FR "Décaler les touches A/B/X/Y dans le sens horaire."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_FR "Appareil Arkanoid"
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_FR "Sélectionnez le périphérique que vous souhaitez utiliser pour la raquette Arkanoid."
#define OPTION_VAL_MOUSE_FR "Souris"
#define OPTION_VAL_POINTER_FR "Pointeur"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_FR "Périphérique Zapper"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_FR "Sélectionnez le périphérique que vous souhaitez utiliser pour le Zapper."
#define OPTION_VAL_LIGHTGUN_FR "Pistolet"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_FR "Afficher le viseur"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_FR "Définir s'il faut afficher ou non le viseur lors de l'utilisation du Zapper."
#define NESTOPIA_TURBO_PULSE_LABEL_FR "Vitesse d'impulsion du turbo"
#define NESTOPIA_TURBO_PULSE_INFO_0_FR "Régler la vitesse d'impulsion du turbo pour les touches Turbo B et Turbo A."
#define NESTOPIA_NOSPRITELIMIT_LABEL_FR "Supprimer la limite de sprites"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_FR "Supprimer la limite matérielle de 8 sprites par ligne de balayage."
#define NESTOPIA_OVERCLOCK_LABEL_FR "Vitesse du processeur (overclock)"
#define NESTOPIA_OVERCLOCK_INFO_0_FR "Overclocker le processeur émulé."
#define OPTION_VAL_1X_FR "x1"
#define OPTION_VAL_2X_FR "x2"
#define NESTOPIA_RAM_POWER_STATE_LABEL_FR "État de la RAM à la mise sous tension"
#define NESTOPIA_RAM_POWER_STATE_INFO_0_FR "Les valeurs dans la RAM lors de la mise sous tension. Certains jeux dépendent des valeurs initiales dans la RAM pour la génération de nombres aléatoires par exemple."
#define OPTION_VAL_0X00_FR NULL
#define OPTION_VAL_0XFF_FR NULL
#define OPTION_VAL_RANDOM_FR "Aléatoire"

struct retro_core_option_v2_category option_cats_fr[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_FR,
      CATEGORY_SYSTEM_INFO_0_FR
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_FR,
      CATEGORY_VIDEO_INFO_0_FR
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_FR,
      CATEGORY_AUDIO_INFO_0_FR
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_FR,
      CATEGORY_INPUT_INFO_0_FR
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_FR,
      CATEGORY_HACKS_INFO_0_FR
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_fr[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_FR,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_FR,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_FR },
         { "ntsc",    OPTION_VAL_NTSC_FR },
         { "pal",     OPTION_VAL_PAL_FR },
         { "famicom", OPTION_VAL_FAMICOM_FR },
         { "dendy",   OPTION_VAL_DENDY_FR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_FR,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_FR,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_FR,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_FR,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_FR },
         { "svideo",     OPTION_VAL_SVIDEO_FR },
         { "rgb",        OPTION_VAL_RGB_FR },
         { "monochrome", OPTION_VAL_MONOCHROME_FR },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_FR,
      NULL,
      NESTOPIA_PALETTE_INFO_0_FR,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_FR },
         { "consumer",             OPTION_VAL_CONSUMER_FR },
         { "canonical",            OPTION_VAL_CANONICAL_FR },
         { "alternative",          OPTION_VAL_ALTERNATIVE_FR },
         { "rgb",                  OPTION_VAL_RGB_O3_FR },
         { "pal",                  OPTION_VAL_PAL_FR },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_FR },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_FR },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_FR },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_FR },
         { "raw",                  OPTION_VAL_RAW_FR },
         { "custom",               OPTION_VAL_CUSTOM_FR },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_FR,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_FR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_FR,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_FR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_FR,
      NULL,
      NESTOPIA_ASPECT_INFO_0_FR,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_FR },
         { "ntsc",        OPTION_VAL_NTSC_FR },
         { "pal",         OPTION_VAL_PAL_FR },
         { "4:3",         OPTION_VAL_4_3_FR },
         { "uncorrected", OPTION_VAL_UNCORRECTED_FR },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_FR,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_FR,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_FR,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_FR,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_FR,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_FR,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_FR,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_FR,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_FR,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_FR,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_FR,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_FR,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_FR,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_FR,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_FR,
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
      NESTOPIA_AUDIO_TYPE_LABEL_FR,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_FR,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_FR },
         { "stereo", OPTION_VAL_STEREO_FR },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_FR,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_FR,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_FR },
         { "ntsc",    OPTION_VAL_NTSC_FR },
         { "famicom", OPTION_VAL_FAMICOM_FR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_FR,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_FR, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_FR,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_FR,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_FR },
         { "pointer", OPTION_VAL_POINTER_FR },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_FR,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_FR,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_FR },
         { "mouse", OPTION_VAL_MOUSE_FR },
         { "pointer", OPTION_VAL_POINTER_FR },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_FR,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_FR,
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
      NESTOPIA_TURBO_PULSE_LABEL_FR,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_FR,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_FR,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_FR,
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
      NESTOPIA_OVERCLOCK_LABEL_FR,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_FR,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_FR },
         { "2x", OPTION_VAL_2X_FR },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_FR,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_FR,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_FR },
         { "0xFF",   OPTION_VAL_0XFF_FR },
         { "random", OPTION_VAL_RANDOM_FR },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_fr = {
   option_cats_fr,
   option_defs_fr
};

/* RETRO_LANGUAGE_GL */

#define CATEGORY_SYSTEM_LABEL_GL NULL
#define CATEGORY_SYSTEM_INFO_0_GL NULL
#define CATEGORY_VIDEO_LABEL_GL "Vídeo"
#define CATEGORY_VIDEO_INFO_0_GL NULL
#define CATEGORY_AUDIO_LABEL_GL "Son"
#define CATEGORY_AUDIO_INFO_0_GL NULL
#define CATEGORY_INPUT_LABEL_GL "Entrada"
#define CATEGORY_INPUT_INFO_0_GL NULL
#define CATEGORY_HACKS_LABEL_GL "Hacks de emulación"
#define CATEGORY_HACKS_INFO_0_GL NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_GL NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_GL NULL
#define OPTION_VAL_AUTO_GL NULL
#define OPTION_VAL_NTSC_GL NULL
#define OPTION_VAL_PAL_GL NULL
#define OPTION_VAL_FAMICOM_GL NULL
#define OPTION_VAL_DENDY_GL NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_GL NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_GL NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_GL NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_GL NULL
#define OPTION_VAL_COMPOSITE_GL NULL
#define OPTION_VAL_SVIDEO_GL NULL
#define OPTION_VAL_RGB_GL NULL
#define OPTION_VAL_MONOCHROME_GL NULL
#define NESTOPIA_PALETTE_LABEL_GL NULL
#define NESTOPIA_PALETTE_INFO_0_GL NULL
#define OPTION_VAL_CXA2025AS_GL NULL
#define OPTION_VAL_CONSUMER_GL NULL
#define OPTION_VAL_CANONICAL_GL NULL
#define OPTION_VAL_ALTERNATIVE_GL NULL
#define OPTION_VAL_RGB_O3_GL NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_GL NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_GL NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_GL NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_GL NULL
#define OPTION_VAL_RAW_GL NULL
#define OPTION_VAL_CUSTOM_GL NULL
#define NESTOPIA_OVERSCAN_V_LABEL_GL NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_GL NULL
#define NESTOPIA_OVERSCAN_H_LABEL_GL NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_GL NULL
#define NESTOPIA_ASPECT_LABEL_GL NULL
#define NESTOPIA_ASPECT_INFO_0_GL NULL
#define OPTION_VAL_4_3_GL NULL
#define OPTION_VAL_UNCORRECTED_GL NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_GL NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_GL NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_GL NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_GL NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_GL NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_GL NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_GL NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_GL NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_GL NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_GL NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_GL NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_GL NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_GL NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_GL NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_GL NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_GL NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_GL NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_GL NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_GL NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_GL NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_GL NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_GL NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_GL NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_GL NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_GL NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_GL NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_GL NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_GL NULL
#define OPTION_VAL_MONO_GL NULL
#define OPTION_VAL_STEREO_GL NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_GL NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_GL NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_GL NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_GL NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_GL NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_GL NULL
#define OPTION_VAL_MOUSE_GL NULL
#define OPTION_VAL_POINTER_GL NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_GL NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_GL NULL
#define OPTION_VAL_LIGHTGUN_GL NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_GL NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_GL NULL
#define NESTOPIA_TURBO_PULSE_LABEL_GL NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_GL NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_GL NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_GL NULL
#define NESTOPIA_OVERCLOCK_LABEL_GL NULL
#define NESTOPIA_OVERCLOCK_INFO_0_GL NULL
#define OPTION_VAL_1X_GL NULL
#define OPTION_VAL_2X_GL NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_GL NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_GL NULL
#define OPTION_VAL_0X00_GL NULL
#define OPTION_VAL_0XFF_GL NULL
#define OPTION_VAL_RANDOM_GL NULL

struct retro_core_option_v2_category option_cats_gl[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_GL,
      CATEGORY_SYSTEM_INFO_0_GL
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_GL,
      CATEGORY_VIDEO_INFO_0_GL
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_GL,
      CATEGORY_AUDIO_INFO_0_GL
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_GL,
      CATEGORY_INPUT_INFO_0_GL
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_GL,
      CATEGORY_HACKS_INFO_0_GL
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_gl[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_GL,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_GL,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_GL },
         { "ntsc",    OPTION_VAL_NTSC_GL },
         { "pal",     OPTION_VAL_PAL_GL },
         { "famicom", OPTION_VAL_FAMICOM_GL },
         { "dendy",   OPTION_VAL_DENDY_GL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_GL,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_GL,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_GL,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_GL,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_GL },
         { "svideo",     OPTION_VAL_SVIDEO_GL },
         { "rgb",        OPTION_VAL_RGB_GL },
         { "monochrome", OPTION_VAL_MONOCHROME_GL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_GL,
      NULL,
      NESTOPIA_PALETTE_INFO_0_GL,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_GL },
         { "consumer",             OPTION_VAL_CONSUMER_GL },
         { "canonical",            OPTION_VAL_CANONICAL_GL },
         { "alternative",          OPTION_VAL_ALTERNATIVE_GL },
         { "rgb",                  OPTION_VAL_RGB_O3_GL },
         { "pal",                  OPTION_VAL_PAL_GL },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_GL },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_GL },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_GL },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_GL },
         { "raw",                  OPTION_VAL_RAW_GL },
         { "custom",               OPTION_VAL_CUSTOM_GL },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_GL,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_GL,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_GL,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_GL,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_GL,
      NULL,
      NESTOPIA_ASPECT_INFO_0_GL,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_GL },
         { "ntsc",        OPTION_VAL_NTSC_GL },
         { "pal",         OPTION_VAL_PAL_GL },
         { "4:3",         OPTION_VAL_4_3_GL },
         { "uncorrected", OPTION_VAL_UNCORRECTED_GL },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_GL,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_GL,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_GL,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_GL,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_GL,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_GL,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_GL,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_GL,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_GL,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_GL,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_GL,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_GL,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_GL,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_GL,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_GL,
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
      NESTOPIA_AUDIO_TYPE_LABEL_GL,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_GL,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_GL },
         { "stereo", OPTION_VAL_STEREO_GL },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_GL,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_GL,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_GL },
         { "ntsc",    OPTION_VAL_NTSC_GL },
         { "famicom", OPTION_VAL_FAMICOM_GL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_GL,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_GL, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_GL,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_GL,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_GL },
         { "pointer", OPTION_VAL_POINTER_GL },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_GL,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_GL,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_GL },
         { "mouse", OPTION_VAL_MOUSE_GL },
         { "pointer", OPTION_VAL_POINTER_GL },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_GL,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_GL,
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
      NESTOPIA_TURBO_PULSE_LABEL_GL,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_GL,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_GL,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_GL,
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
      NESTOPIA_OVERCLOCK_LABEL_GL,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_GL,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_GL },
         { "2x", OPTION_VAL_2X_GL },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_GL,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_GL,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_GL },
         { "0xFF",   OPTION_VAL_0XFF_GL },
         { "random", OPTION_VAL_RANDOM_GL },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_gl = {
   option_cats_gl,
   option_defs_gl
};

/* RETRO_LANGUAGE_HE */

#define CATEGORY_SYSTEM_LABEL_HE NULL
#define CATEGORY_SYSTEM_INFO_0_HE NULL
#define CATEGORY_VIDEO_LABEL_HE "וידאו"
#define CATEGORY_VIDEO_INFO_0_HE NULL
#define CATEGORY_AUDIO_LABEL_HE "שמע"
#define CATEGORY_AUDIO_INFO_0_HE NULL
#define CATEGORY_INPUT_LABEL_HE "קלט"
#define CATEGORY_INPUT_INFO_0_HE NULL
#define CATEGORY_HACKS_LABEL_HE NULL
#define CATEGORY_HACKS_INFO_0_HE NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_HE NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_HE NULL
#define OPTION_VAL_AUTO_HE NULL
#define OPTION_VAL_NTSC_HE NULL
#define OPTION_VAL_PAL_HE NULL
#define OPTION_VAL_FAMICOM_HE NULL
#define OPTION_VAL_DENDY_HE NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_HE NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_HE NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_HE NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_HE NULL
#define OPTION_VAL_COMPOSITE_HE NULL
#define OPTION_VAL_SVIDEO_HE NULL
#define OPTION_VAL_RGB_HE NULL
#define OPTION_VAL_MONOCHROME_HE NULL
#define NESTOPIA_PALETTE_LABEL_HE NULL
#define NESTOPIA_PALETTE_INFO_0_HE NULL
#define OPTION_VAL_CXA2025AS_HE NULL
#define OPTION_VAL_CONSUMER_HE NULL
#define OPTION_VAL_CANONICAL_HE NULL
#define OPTION_VAL_ALTERNATIVE_HE NULL
#define OPTION_VAL_RGB_O3_HE NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_HE NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_HE NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_HE NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_HE NULL
#define OPTION_VAL_RAW_HE NULL
#define OPTION_VAL_CUSTOM_HE NULL
#define NESTOPIA_OVERSCAN_V_LABEL_HE NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_HE NULL
#define NESTOPIA_OVERSCAN_H_LABEL_HE NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_HE NULL
#define NESTOPIA_ASPECT_LABEL_HE NULL
#define NESTOPIA_ASPECT_INFO_0_HE NULL
#define OPTION_VAL_4_3_HE NULL
#define OPTION_VAL_UNCORRECTED_HE NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_HE NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_HE NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_HE NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_HE NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_HE NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_HE NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_HE NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_HE NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_HE NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_HE NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_HE NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_HE NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_HE NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_HE NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_HE NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_HE NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_HE NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_HE NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_HE NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_HE NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_HE NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_HE NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_HE NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_HE NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_HE NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_HE NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_HE NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_HE NULL
#define OPTION_VAL_MONO_HE NULL
#define OPTION_VAL_STEREO_HE NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_HE NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_HE NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_HE NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_HE NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_HE NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_HE NULL
#define OPTION_VAL_MOUSE_HE NULL
#define OPTION_VAL_POINTER_HE NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_HE NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_HE NULL
#define OPTION_VAL_LIGHTGUN_HE NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_HE NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_HE NULL
#define NESTOPIA_TURBO_PULSE_LABEL_HE NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_HE NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_HE NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_HE NULL
#define NESTOPIA_OVERCLOCK_LABEL_HE NULL
#define NESTOPIA_OVERCLOCK_INFO_0_HE NULL
#define OPTION_VAL_1X_HE NULL
#define OPTION_VAL_2X_HE NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_HE NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_HE NULL
#define OPTION_VAL_0X00_HE NULL
#define OPTION_VAL_0XFF_HE NULL
#define OPTION_VAL_RANDOM_HE NULL

struct retro_core_option_v2_category option_cats_he[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_HE,
      CATEGORY_SYSTEM_INFO_0_HE
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_HE,
      CATEGORY_VIDEO_INFO_0_HE
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_HE,
      CATEGORY_AUDIO_INFO_0_HE
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_HE,
      CATEGORY_INPUT_INFO_0_HE
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_HE,
      CATEGORY_HACKS_INFO_0_HE
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_he[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_HE,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_HE,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_HE },
         { "ntsc",    OPTION_VAL_NTSC_HE },
         { "pal",     OPTION_VAL_PAL_HE },
         { "famicom", OPTION_VAL_FAMICOM_HE },
         { "dendy",   OPTION_VAL_DENDY_HE },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_HE,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_HE,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_HE,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_HE,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_HE },
         { "svideo",     OPTION_VAL_SVIDEO_HE },
         { "rgb",        OPTION_VAL_RGB_HE },
         { "monochrome", OPTION_VAL_MONOCHROME_HE },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_HE,
      NULL,
      NESTOPIA_PALETTE_INFO_0_HE,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_HE },
         { "consumer",             OPTION_VAL_CONSUMER_HE },
         { "canonical",            OPTION_VAL_CANONICAL_HE },
         { "alternative",          OPTION_VAL_ALTERNATIVE_HE },
         { "rgb",                  OPTION_VAL_RGB_O3_HE },
         { "pal",                  OPTION_VAL_PAL_HE },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_HE },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_HE },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_HE },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_HE },
         { "raw",                  OPTION_VAL_RAW_HE },
         { "custom",               OPTION_VAL_CUSTOM_HE },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_HE,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_HE,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_HE,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_HE,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_HE,
      NULL,
      NESTOPIA_ASPECT_INFO_0_HE,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_HE },
         { "ntsc",        OPTION_VAL_NTSC_HE },
         { "pal",         OPTION_VAL_PAL_HE },
         { "4:3",         OPTION_VAL_4_3_HE },
         { "uncorrected", OPTION_VAL_UNCORRECTED_HE },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_HE,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_HE,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_HE,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_HE,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_HE,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_HE,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_HE,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_HE,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_HE,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_HE,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_HE,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_HE,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_HE,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_HE,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_HE,
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
      NESTOPIA_AUDIO_TYPE_LABEL_HE,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_HE,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_HE },
         { "stereo", OPTION_VAL_STEREO_HE },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_HE,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_HE,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_HE },
         { "ntsc",    OPTION_VAL_NTSC_HE },
         { "famicom", OPTION_VAL_FAMICOM_HE },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_HE,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_HE, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_HE,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_HE,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_HE },
         { "pointer", OPTION_VAL_POINTER_HE },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_HE,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_HE,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_HE },
         { "mouse", OPTION_VAL_MOUSE_HE },
         { "pointer", OPTION_VAL_POINTER_HE },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_HE,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_HE,
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
      NESTOPIA_TURBO_PULSE_LABEL_HE,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_HE,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_HE,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_HE,
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
      NESTOPIA_OVERCLOCK_LABEL_HE,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_HE,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_HE },
         { "2x", OPTION_VAL_2X_HE },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_HE,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_HE,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_HE },
         { "0xFF",   OPTION_VAL_0XFF_HE },
         { "random", OPTION_VAL_RANDOM_HE },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_he = {
   option_cats_he,
   option_defs_he
};

/* RETRO_LANGUAGE_HR */

#define CATEGORY_SYSTEM_LABEL_HR NULL
#define CATEGORY_SYSTEM_INFO_0_HR NULL
#define CATEGORY_VIDEO_LABEL_HR NULL
#define CATEGORY_VIDEO_INFO_0_HR NULL
#define CATEGORY_AUDIO_LABEL_HR NULL
#define CATEGORY_AUDIO_INFO_0_HR NULL
#define CATEGORY_INPUT_LABEL_HR NULL
#define CATEGORY_INPUT_INFO_0_HR NULL
#define CATEGORY_HACKS_LABEL_HR NULL
#define CATEGORY_HACKS_INFO_0_HR NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_HR NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_HR NULL
#define OPTION_VAL_AUTO_HR NULL
#define OPTION_VAL_NTSC_HR NULL
#define OPTION_VAL_PAL_HR NULL
#define OPTION_VAL_FAMICOM_HR NULL
#define OPTION_VAL_DENDY_HR NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_HR NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_HR NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_HR NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_HR NULL
#define OPTION_VAL_COMPOSITE_HR NULL
#define OPTION_VAL_SVIDEO_HR NULL
#define OPTION_VAL_RGB_HR NULL
#define OPTION_VAL_MONOCHROME_HR NULL
#define NESTOPIA_PALETTE_LABEL_HR NULL
#define NESTOPIA_PALETTE_INFO_0_HR NULL
#define OPTION_VAL_CXA2025AS_HR NULL
#define OPTION_VAL_CONSUMER_HR NULL
#define OPTION_VAL_CANONICAL_HR NULL
#define OPTION_VAL_ALTERNATIVE_HR NULL
#define OPTION_VAL_RGB_O3_HR NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_HR NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_HR NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_HR NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_HR NULL
#define OPTION_VAL_RAW_HR NULL
#define OPTION_VAL_CUSTOM_HR NULL
#define NESTOPIA_OVERSCAN_V_LABEL_HR NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_HR NULL
#define NESTOPIA_OVERSCAN_H_LABEL_HR NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_HR NULL
#define NESTOPIA_ASPECT_LABEL_HR NULL
#define NESTOPIA_ASPECT_INFO_0_HR NULL
#define OPTION_VAL_4_3_HR NULL
#define OPTION_VAL_UNCORRECTED_HR NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_HR NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_HR NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_HR NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_HR NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_HR NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_HR NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_HR NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_HR NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_HR NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_HR NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_HR NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_HR NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_HR NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_HR NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_HR NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_HR NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_HR NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_HR NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_HR NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_HR NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_HR NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_HR NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_HR NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_HR NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_HR NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_HR NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_HR NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_HR NULL
#define OPTION_VAL_MONO_HR NULL
#define OPTION_VAL_STEREO_HR NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_HR NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_HR NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_HR NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_HR NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_HR NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_HR NULL
#define OPTION_VAL_MOUSE_HR NULL
#define OPTION_VAL_POINTER_HR NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_HR NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_HR NULL
#define OPTION_VAL_LIGHTGUN_HR NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_HR NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_HR NULL
#define NESTOPIA_TURBO_PULSE_LABEL_HR NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_HR NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_HR NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_HR NULL
#define NESTOPIA_OVERCLOCK_LABEL_HR NULL
#define NESTOPIA_OVERCLOCK_INFO_0_HR NULL
#define OPTION_VAL_1X_HR NULL
#define OPTION_VAL_2X_HR NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_HR NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_HR NULL
#define OPTION_VAL_0X00_HR NULL
#define OPTION_VAL_0XFF_HR NULL
#define OPTION_VAL_RANDOM_HR NULL

struct retro_core_option_v2_category option_cats_hr[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_HR,
      CATEGORY_SYSTEM_INFO_0_HR
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_HR,
      CATEGORY_VIDEO_INFO_0_HR
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_HR,
      CATEGORY_AUDIO_INFO_0_HR
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_HR,
      CATEGORY_INPUT_INFO_0_HR
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_HR,
      CATEGORY_HACKS_INFO_0_HR
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_hr[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_HR,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_HR,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_HR },
         { "ntsc",    OPTION_VAL_NTSC_HR },
         { "pal",     OPTION_VAL_PAL_HR },
         { "famicom", OPTION_VAL_FAMICOM_HR },
         { "dendy",   OPTION_VAL_DENDY_HR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_HR,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_HR,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_HR,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_HR,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_HR },
         { "svideo",     OPTION_VAL_SVIDEO_HR },
         { "rgb",        OPTION_VAL_RGB_HR },
         { "monochrome", OPTION_VAL_MONOCHROME_HR },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_HR,
      NULL,
      NESTOPIA_PALETTE_INFO_0_HR,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_HR },
         { "consumer",             OPTION_VAL_CONSUMER_HR },
         { "canonical",            OPTION_VAL_CANONICAL_HR },
         { "alternative",          OPTION_VAL_ALTERNATIVE_HR },
         { "rgb",                  OPTION_VAL_RGB_O3_HR },
         { "pal",                  OPTION_VAL_PAL_HR },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_HR },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_HR },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_HR },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_HR },
         { "raw",                  OPTION_VAL_RAW_HR },
         { "custom",               OPTION_VAL_CUSTOM_HR },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_HR,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_HR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_HR,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_HR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_HR,
      NULL,
      NESTOPIA_ASPECT_INFO_0_HR,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_HR },
         { "ntsc",        OPTION_VAL_NTSC_HR },
         { "pal",         OPTION_VAL_PAL_HR },
         { "4:3",         OPTION_VAL_4_3_HR },
         { "uncorrected", OPTION_VAL_UNCORRECTED_HR },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_HR,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_HR,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_HR,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_HR,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_HR,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_HR,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_HR,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_HR,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_HR,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_HR,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_HR,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_HR,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_HR,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_HR,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_HR,
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
      NESTOPIA_AUDIO_TYPE_LABEL_HR,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_HR,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_HR },
         { "stereo", OPTION_VAL_STEREO_HR },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_HR,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_HR,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_HR },
         { "ntsc",    OPTION_VAL_NTSC_HR },
         { "famicom", OPTION_VAL_FAMICOM_HR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_HR,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_HR, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_HR,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_HR,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_HR },
         { "pointer", OPTION_VAL_POINTER_HR },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_HR,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_HR,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_HR },
         { "mouse", OPTION_VAL_MOUSE_HR },
         { "pointer", OPTION_VAL_POINTER_HR },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_HR,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_HR,
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
      NESTOPIA_TURBO_PULSE_LABEL_HR,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_HR,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_HR,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_HR,
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
      NESTOPIA_OVERCLOCK_LABEL_HR,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_HR,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_HR },
         { "2x", OPTION_VAL_2X_HR },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_HR,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_HR,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_HR },
         { "0xFF",   OPTION_VAL_0XFF_HR },
         { "random", OPTION_VAL_RANDOM_HR },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_hr = {
   option_cats_hr,
   option_defs_hr
};

/* RETRO_LANGUAGE_HU */

#define CATEGORY_SYSTEM_LABEL_HU "Rendszer"
#define CATEGORY_SYSTEM_INFO_0_HU NULL
#define CATEGORY_VIDEO_LABEL_HU "Kép"
#define CATEGORY_VIDEO_INFO_0_HU "A képarány, képvágás, színpaletta, és videoszűrő beállításai."
#define CATEGORY_AUDIO_LABEL_HU "Hang"
#define CATEGORY_AUDIO_INFO_0_HU "Hangeffektek és csatornahangerők."
#define CATEGORY_INPUT_LABEL_HU "Bemenet"
#define CATEGORY_INPUT_INFO_0_HU "Bemeneti eszközök és más bemeneti beállítások."
#define CATEGORY_HACKS_LABEL_HU "Emulációs trükkök"
#define CATEGORY_HACKS_INFO_0_HU "A processzor túlhajtás és az emuláció pontosságának beállításai, amelyek befolyásolják az alacsonyszintű teljesítményt és a kompatibilitást."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_HU "Régió"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_HU "Az automatikus beállítás az NstDatabase.xml fájlt használja a régió érzékeléséhez. Ha nincs adatbázis, az NTSC az alapértelmezett."
#define OPTION_VAL_AUTO_HU "Automatikus"
#define OPTION_VAL_NTSC_HU NULL
#define OPTION_VAL_PAL_HU NULL
#define OPTION_VAL_FAMICOM_HU NULL
#define OPTION_VAL_DENDY_HU NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_HU "FDS automatikus behelyezése"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_HU "Az első FDS lemez automatikus behelyezése újraindításkor."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_HU "Blargg NTSC szűrő"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_HU "A Blargg NTSC szűrő engedélyezése."
#define OPTION_VAL_COMPOSITE_HU "Kompozit videojel"
#define OPTION_VAL_SVIDEO_HU NULL
#define OPTION_VAL_RGB_HU NULL
#define OPTION_VAL_MONOCHROME_HU "Egyszínű"
#define NESTOPIA_PALETTE_LABEL_HU "Paletta"
#define NESTOPIA_PALETTE_INFO_0_HU NULL
#define OPTION_VAL_CXA2025AS_HU NULL
#define OPTION_VAL_CONSUMER_HU "Fogyasztói"
#define OPTION_VAL_CANONICAL_HU NULL
#define OPTION_VAL_ALTERNATIVE_HU "Alternatív"
#define OPTION_VAL_RGB_O3_HU NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_HU NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_HU NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_HU NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_HU "NES Classic FBx (javított)"
#define OPTION_VAL_RAW_HU "Nyers"
#define OPTION_VAL_CUSTOM_HU "Egyedi"
#define NESTOPIA_OVERSCAN_V_LABEL_HU "Overscan kitakarása (függőleges)"
#define NESTOPIA_OVERSCAN_V_INFO_0_HU "A kép függőleges széleinek kitakarása, amelyeket egy normál felbontású tévékészülék kávája kitakart és esetleg hibás képkimenetet tartalmaznak."
#define NESTOPIA_OVERSCAN_H_LABEL_HU "Overscan kitakarása (vízszintes)"
#define NESTOPIA_OVERSCAN_H_INFO_0_HU "A kép vízszintes széleinek kitakarása, amelyeket egy normál felbontású tévékészülék kávája kitakart és esetleg hibás képkimenetet tartalmaznak."
#define NESTOPIA_ASPECT_LABEL_HU "Kívánt képarány"
#define NESTOPIA_ASPECT_INFO_0_HU NULL
#define OPTION_VAL_4_3_HU NULL
#define OPTION_VAL_UNCORRECTED_HU "Korrigálatlan"
#define NESTOPIA_GENIE_DISTORTION_LABEL_HU "Game Genie hangtorzítás"
#define NESTOPIA_GENIE_DISTORTION_INFO_0_HU "A Game Genie csalóeszköz akaratlan hangtorzítást okozott a játékokban. Ezzel a beállítással emulálható a torzítás, amit a játékok hangjában idézett elő."
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_HU "A hang haladó beállításai (újra meg kell nyitni a menüt)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_HU "A hangcsatornák alacsony szintű paramétereinek beállítása."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_HU "Négyszögjel 1. csatorna hangerő %"
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_HU "Négyszögjel 1. csatorna hangerő % beállítása."
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_HU "Négyszögjel 2. csatorna hangerő %"
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_HU "Négyszögjel 2. csatorna hangerő % beállítása."
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_HU "Háromszögjel csatorna hangerő %"
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_HU "Háromszögjel csatorna hangerő % beállítása."
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_HU "Zajcsatorna hangerő %"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_HU "Zajcsatorna hangerő % beállítása."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_HU "DPCM csatorna hangerő %"
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_HU "DPCM csatorna hangerő % beállítása."
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_HU "FDS csatorna hangerő %"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_HU "FDS csatorna hangerő % beállítása."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_HU "MMC5 csatorna hangerő %"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_HU "MMC5 csatorna hangerő % beállítása."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_HU "VRC6 csatorna hangerő %"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_HU "VRC6 csatorna hangerő % beállítása."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_HU "VRC7 csatorna hangerő %"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_HU "VRC7 csatorna hangerő % beállítása."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_HU "N163 csatorna hangerő %"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_HU "N163 csatorna hangerő % beállítása."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_HU "S5B csatorna hangerő %"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_HU NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_HU NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_HU NULL
#define OPTION_VAL_MONO_HU NULL
#define OPTION_VAL_STEREO_HU NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_HU "4 játékos adapter"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_HU "A négyjátékosos adapter kézi kiválasztása, ha szükséges. Néhány játék nem detektálja helyesen az adaptert az NstDatabase.xml adatbázison keresztül, ez a beállítás segít ezt kijavítani."
#define NESTOPIA_BUTTON_SHIFT_LABEL_HU "Gombok léptetése az óramutató irányába"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_HU "Az A/B/X/Y gombok forgatása az óramutató járása szerint."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_HU "Arkanoid eszköz"
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_HU "Az Arkanoid ütőjéhez használt eszköz."
#define OPTION_VAL_MOUSE_HU "Egér"
#define OPTION_VAL_POINTER_HU "Mutató"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_HU "Zapper eszköz"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_HU "A Zapperhez használt eszköz."
#define OPTION_VAL_LIGHTGUN_HU "Fénypisztoly"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_HU "Célkereszt megjelenítése"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_HU "A célkereszt megjelenítése Zapper használatakor."
#define NESTOPIA_TURBO_PULSE_LABEL_HU "Turbó pulzus sebessége"
#define NESTOPIA_TURBO_PULSE_INFO_0_HU "A turbó pulzus sebessége a Turbo B és Turbo A gombokhoz."
#define NESTOPIA_NOSPRITELIMIT_LABEL_HU "Sprite korlátozás eltávolítása"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_HU "Eltávolítja a hardver soronként legfeljebb 8 sprite-os korlátozását."
#define NESTOPIA_OVERCLOCK_LABEL_HU "CPU sebesség (túlhajtás)"
#define NESTOPIA_OVERCLOCK_INFO_0_HU "Az emulált CPU túlhajtása."
#define OPTION_VAL_1X_HU NULL
#define OPTION_VAL_2X_HU NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_HU "RAM állapota bekapcsoláskor"
#define NESTOPIA_RAM_POWER_STATE_INFO_0_HU "A RAM értékei bekapcsoláskor. Néhány játék hagyatkozik erre például véletlenszám generáláshoz."
#define OPTION_VAL_0X00_HU NULL
#define OPTION_VAL_0XFF_HU NULL
#define OPTION_VAL_RANDOM_HU "Véletlenszerű"

struct retro_core_option_v2_category option_cats_hu[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_HU,
      CATEGORY_SYSTEM_INFO_0_HU
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_HU,
      CATEGORY_VIDEO_INFO_0_HU
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_HU,
      CATEGORY_AUDIO_INFO_0_HU
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_HU,
      CATEGORY_INPUT_INFO_0_HU
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_HU,
      CATEGORY_HACKS_INFO_0_HU
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_hu[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_HU,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_HU,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_HU },
         { "ntsc",    OPTION_VAL_NTSC_HU },
         { "pal",     OPTION_VAL_PAL_HU },
         { "famicom", OPTION_VAL_FAMICOM_HU },
         { "dendy",   OPTION_VAL_DENDY_HU },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_HU,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_HU,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_HU,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_HU,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_HU },
         { "svideo",     OPTION_VAL_SVIDEO_HU },
         { "rgb",        OPTION_VAL_RGB_HU },
         { "monochrome", OPTION_VAL_MONOCHROME_HU },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_HU,
      NULL,
      NESTOPIA_PALETTE_INFO_0_HU,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_HU },
         { "consumer",             OPTION_VAL_CONSUMER_HU },
         { "canonical",            OPTION_VAL_CANONICAL_HU },
         { "alternative",          OPTION_VAL_ALTERNATIVE_HU },
         { "rgb",                  OPTION_VAL_RGB_O3_HU },
         { "pal",                  OPTION_VAL_PAL_HU },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_HU },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_HU },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_HU },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_HU },
         { "raw",                  OPTION_VAL_RAW_HU },
         { "custom",               OPTION_VAL_CUSTOM_HU },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_HU,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_HU,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_HU,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_HU,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_HU,
      NULL,
      NESTOPIA_ASPECT_INFO_0_HU,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_HU },
         { "ntsc",        OPTION_VAL_NTSC_HU },
         { "pal",         OPTION_VAL_PAL_HU },
         { "4:3",         OPTION_VAL_4_3_HU },
         { "uncorrected", OPTION_VAL_UNCORRECTED_HU },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_HU,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_HU,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_HU,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_HU,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_HU,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_HU,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_HU,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_HU,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_HU,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_HU,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_HU,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_HU,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_HU,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_HU,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_HU,
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
      NESTOPIA_AUDIO_TYPE_LABEL_HU,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_HU,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_HU },
         { "stereo", OPTION_VAL_STEREO_HU },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_HU,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_HU,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_HU },
         { "ntsc",    OPTION_VAL_NTSC_HU },
         { "famicom", OPTION_VAL_FAMICOM_HU },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_HU,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_HU, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_HU,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_HU,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_HU },
         { "pointer", OPTION_VAL_POINTER_HU },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_HU,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_HU,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_HU },
         { "mouse", OPTION_VAL_MOUSE_HU },
         { "pointer", OPTION_VAL_POINTER_HU },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_HU,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_HU,
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
      NESTOPIA_TURBO_PULSE_LABEL_HU,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_HU,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_HU,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_HU,
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
      NESTOPIA_OVERCLOCK_LABEL_HU,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_HU,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_HU },
         { "2x", OPTION_VAL_2X_HU },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_HU,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_HU,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_HU },
         { "0xFF",   OPTION_VAL_0XFF_HU },
         { "random", OPTION_VAL_RANDOM_HU },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_hu = {
   option_cats_hu,
   option_defs_hu
};

/* RETRO_LANGUAGE_ID */

#define CATEGORY_SYSTEM_LABEL_ID "Sistem"
#define CATEGORY_SYSTEM_INFO_0_ID NULL
#define CATEGORY_VIDEO_LABEL_ID NULL
#define CATEGORY_VIDEO_INFO_0_ID NULL
#define CATEGORY_AUDIO_LABEL_ID "Suara"
#define CATEGORY_AUDIO_INFO_0_ID NULL
#define CATEGORY_INPUT_LABEL_ID "Masukan"
#define CATEGORY_INPUT_INFO_0_ID NULL
#define CATEGORY_HACKS_LABEL_ID "Peretasan Emulasi"
#define CATEGORY_HACKS_INFO_0_ID NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_ID NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_ID NULL
#define OPTION_VAL_AUTO_ID "Otomatis"
#define OPTION_VAL_NTSC_ID NULL
#define OPTION_VAL_PAL_ID NULL
#define OPTION_VAL_FAMICOM_ID NULL
#define OPTION_VAL_DENDY_ID NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_ID NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_ID NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_ID NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_ID NULL
#define OPTION_VAL_COMPOSITE_ID NULL
#define OPTION_VAL_SVIDEO_ID NULL
#define OPTION_VAL_RGB_ID NULL
#define OPTION_VAL_MONOCHROME_ID NULL
#define NESTOPIA_PALETTE_LABEL_ID NULL
#define NESTOPIA_PALETTE_INFO_0_ID NULL
#define OPTION_VAL_CXA2025AS_ID NULL
#define OPTION_VAL_CONSUMER_ID NULL
#define OPTION_VAL_CANONICAL_ID NULL
#define OPTION_VAL_ALTERNATIVE_ID NULL
#define OPTION_VAL_RGB_O3_ID NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_ID NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_ID NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_ID NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_ID NULL
#define OPTION_VAL_RAW_ID NULL
#define OPTION_VAL_CUSTOM_ID NULL
#define NESTOPIA_OVERSCAN_V_LABEL_ID NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_ID NULL
#define NESTOPIA_OVERSCAN_H_LABEL_ID NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_ID NULL
#define NESTOPIA_ASPECT_LABEL_ID NULL
#define NESTOPIA_ASPECT_INFO_0_ID NULL
#define OPTION_VAL_4_3_ID NULL
#define OPTION_VAL_UNCORRECTED_ID NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_ID NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_ID NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_ID NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_ID NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_ID NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_ID NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_ID NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_ID NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_ID NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_ID NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_ID NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_ID NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_ID NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_ID NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_ID NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_ID NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_ID NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_ID NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_ID NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_ID NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_ID NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_ID NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_ID NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_ID NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_ID NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_ID NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_ID NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_ID NULL
#define OPTION_VAL_MONO_ID NULL
#define OPTION_VAL_STEREO_ID NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_ID NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_ID NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_ID NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_ID NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_ID NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_ID NULL
#define OPTION_VAL_MOUSE_ID NULL
#define OPTION_VAL_POINTER_ID NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_ID NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_ID NULL
#define OPTION_VAL_LIGHTGUN_ID NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_ID NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_ID NULL
#define NESTOPIA_TURBO_PULSE_LABEL_ID NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_ID NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_ID NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_ID NULL
#define NESTOPIA_OVERCLOCK_LABEL_ID NULL
#define NESTOPIA_OVERCLOCK_INFO_0_ID NULL
#define OPTION_VAL_1X_ID NULL
#define OPTION_VAL_2X_ID NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_ID NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_ID NULL
#define OPTION_VAL_0X00_ID NULL
#define OPTION_VAL_0XFF_ID NULL
#define OPTION_VAL_RANDOM_ID NULL

struct retro_core_option_v2_category option_cats_id[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_ID,
      CATEGORY_SYSTEM_INFO_0_ID
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_ID,
      CATEGORY_VIDEO_INFO_0_ID
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_ID,
      CATEGORY_AUDIO_INFO_0_ID
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_ID,
      CATEGORY_INPUT_INFO_0_ID
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_ID,
      CATEGORY_HACKS_INFO_0_ID
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_id[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_ID,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_ID,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_ID },
         { "ntsc",    OPTION_VAL_NTSC_ID },
         { "pal",     OPTION_VAL_PAL_ID },
         { "famicom", OPTION_VAL_FAMICOM_ID },
         { "dendy",   OPTION_VAL_DENDY_ID },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_ID,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_ID,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_ID,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_ID,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_ID },
         { "svideo",     OPTION_VAL_SVIDEO_ID },
         { "rgb",        OPTION_VAL_RGB_ID },
         { "monochrome", OPTION_VAL_MONOCHROME_ID },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_ID,
      NULL,
      NESTOPIA_PALETTE_INFO_0_ID,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_ID },
         { "consumer",             OPTION_VAL_CONSUMER_ID },
         { "canonical",            OPTION_VAL_CANONICAL_ID },
         { "alternative",          OPTION_VAL_ALTERNATIVE_ID },
         { "rgb",                  OPTION_VAL_RGB_O3_ID },
         { "pal",                  OPTION_VAL_PAL_ID },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_ID },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_ID },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_ID },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_ID },
         { "raw",                  OPTION_VAL_RAW_ID },
         { "custom",               OPTION_VAL_CUSTOM_ID },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_ID,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_ID,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_ID,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_ID,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_ID,
      NULL,
      NESTOPIA_ASPECT_INFO_0_ID,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_ID },
         { "ntsc",        OPTION_VAL_NTSC_ID },
         { "pal",         OPTION_VAL_PAL_ID },
         { "4:3",         OPTION_VAL_4_3_ID },
         { "uncorrected", OPTION_VAL_UNCORRECTED_ID },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_ID,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_ID,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_ID,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_ID,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_ID,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_ID,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_ID,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_ID,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_ID,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_ID,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_ID,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_ID,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_ID,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_ID,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_ID,
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
      NESTOPIA_AUDIO_TYPE_LABEL_ID,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_ID,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_ID },
         { "stereo", OPTION_VAL_STEREO_ID },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_ID,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_ID,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_ID },
         { "ntsc",    OPTION_VAL_NTSC_ID },
         { "famicom", OPTION_VAL_FAMICOM_ID },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_ID,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_ID, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_ID,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_ID,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_ID },
         { "pointer", OPTION_VAL_POINTER_ID },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_ID,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_ID,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_ID },
         { "mouse", OPTION_VAL_MOUSE_ID },
         { "pointer", OPTION_VAL_POINTER_ID },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_ID,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_ID,
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
      NESTOPIA_TURBO_PULSE_LABEL_ID,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_ID,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_ID,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_ID,
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
      NESTOPIA_OVERCLOCK_LABEL_ID,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_ID,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_ID },
         { "2x", OPTION_VAL_2X_ID },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_ID,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_ID,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_ID },
         { "0xFF",   OPTION_VAL_0XFF_ID },
         { "random", OPTION_VAL_RANDOM_ID },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_id = {
   option_cats_id,
   option_defs_id
};

/* RETRO_LANGUAGE_IT */

#define CATEGORY_SYSTEM_LABEL_IT "Sistema"
#define CATEGORY_SYSTEM_INFO_0_IT NULL
#define CATEGORY_VIDEO_LABEL_IT NULL
#define CATEGORY_VIDEO_INFO_0_IT "Cambia le proporzioni, il ritaglio, la tavolozza dei colori e le impostazioni del filtro video."
#define CATEGORY_AUDIO_LABEL_IT NULL
#define CATEGORY_AUDIO_INFO_0_IT "Cambia gli effetti audio e i volumi del canale."
#define CATEGORY_INPUT_LABEL_IT NULL
#define CATEGORY_INPUT_INFO_0_IT "Cambia i dispositivi di input e altre impostazioni relative agli input."
#define CATEGORY_HACKS_LABEL_IT "Hack di Emulazione"
#define CATEGORY_HACKS_INFO_0_IT "Cambia le impostazioni di overclocking e di precisione di emulazione del processore che influenzano prestazioni e compatibilità di basso livello."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_IT "Regione Di Sistema"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_IT "'Auto' userà il file del database NstDatabase.xml per il rilevamento automatico della regione. Se non c'è nessun database, sarà predefinito NTSC."
#define OPTION_VAL_AUTO_IT NULL
#define OPTION_VAL_NTSC_IT NULL
#define OPTION_VAL_PAL_IT NULL
#define OPTION_VAL_FAMICOM_IT NULL
#define OPTION_VAL_DENDY_IT NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_IT "FDS Aggiunta automatica"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_IT "Inserisci automaticamente il primo disco FDS al reset."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_IT "Filtro Blargg NTSC"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_IT "Abilita i filtri Blargg NTSC."
#define OPTION_VAL_COMPOSITE_IT "Video Composito"
#define OPTION_VAL_SVIDEO_IT NULL
#define OPTION_VAL_RGB_IT NULL
#define OPTION_VAL_MONOCHROME_IT "Monocromatico"
#define NESTOPIA_PALETTE_LABEL_IT "Tavolozza"
#define NESTOPIA_PALETTE_INFO_0_IT NULL
#define OPTION_VAL_CXA2025AS_IT NULL
#define OPTION_VAL_CONSUMER_IT "Consumatore"
#define OPTION_VAL_CANONICAL_IT "Canonico"
#define OPTION_VAL_ALTERNATIVE_IT "Alternativo"
#define OPTION_VAL_RGB_O3_IT NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_IT "Composito FBx Diretto"
#define OPTION_VAL_PVM_STYLE_D93_FBX_IT "PVM-stile D93 FBx"
#define OPTION_VAL_NTSC_HARDWARE_FBX_IT "FBx hardware NTSC"
#define OPTION_VAL_NES_CLASSIC_FBX_FS_IT "NES Classic FBx (fisso)"
#define OPTION_VAL_RAW_IT "Grezzo"
#define OPTION_VAL_CUSTOM_IT "Personalizzato"
#define NESTOPIA_OVERSCAN_V_LABEL_IT "Maschera Overscan (Verticale)"
#define NESTOPIA_OVERSCAN_V_INFO_0_IT "Maschera fuori (verticalmente) l'uscita video potenzialmente casuale glitchy che sarebbe stato nascosto dalla lunetta intorno al bordo di uno schermo televisivo standard-definizione."
#define NESTOPIA_OVERSCAN_H_LABEL_IT "Maschera Overscan (Orizzontale)"
#define NESTOPIA_OVERSCAN_H_INFO_0_IT "Maschera fuori (in orizzontale) l'uscita video potenzialmente casuale glitchy che sarebbe stato nascosto dalla lunetta intorno al bordo di uno schermo televisivo standard-definizione."
#define NESTOPIA_ASPECT_LABEL_IT "Rapporto Dimensioni Preferite"
#define NESTOPIA_ASPECT_INFO_0_IT NULL
#define OPTION_VAL_4_3_IT NULL
#define OPTION_VAL_UNCORRECTED_IT "Non Corretto"
#define NESTOPIA_GENIE_DISTORTION_LABEL_IT "Game Genie distorzione del suono"
#define NESTOPIA_GENIE_DISTORTION_INFO_0_IT "Il dispositivo di trucchi Game Genie potrebbe inavvertitamente introdurre distorsione del suono nei giochi. Abilitando questo, è possibile simulare la distorsione che aggiungerebbe al suono di un gioco."
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_IT "Mostra le impostazioni audio avanzate (riapri il menu)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_IT "Abilita la configurazione dei parametri del canale audio a basso livello."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_IT "Canale Quadrato 1 Volume %"
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_IT "Modifica Canale Quadrato 1 Volume %."
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_IT "Canale Quadrato 2 Volume %"
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_IT "Modifica Canale Quadrato 2 Volume %."
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_IT "Canale Triangolo Volume %"
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_IT "Modifica Canale Triangolo Volume %."
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_IT "Canale Rumore Volume %"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_IT "Modifica Canale Rumore Volume %."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_IT "Canale DPCM Volume %"
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_IT "Modifica Canale DPCM Volume %."
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_IT "Canale FDS Volume %"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_IT "Modifica Canale FDS Volume %."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_IT "Canale MMC5 Volume %"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_IT "Modifica Canale MMC5 Volume %."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_IT "Canale VRC6 Volume %"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_IT "Modifica Canale VRC6 Volume %."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_IT "Canale VRC7 Volume %"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_IT "Modifica Canale VRC7 Volume %."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_IT "Canale N163 Volume %"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_IT "Modifica Canale N163 Volume %."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_IT "Canale S5B Volume %"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_IT NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_IT NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_IT NULL
#define OPTION_VAL_MONO_IT NULL
#define OPTION_VAL_STEREO_IT NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_IT "Adattatore Da 4 Giocatori"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_IT "Seleziona manualmente un adattatore da 4 giocatori, se necessario. Alcuni giochi non riconosceranno correttamente l'adattatore attraverso NstDatabase. ml di database, questa opzione dovrebbe aiutare a risolvere il problema."
#define NESTOPIA_BUTTON_SHIFT_LABEL_IT "Sposta tasti in senso orario"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_IT "Ruota i pulsanti A/B/X/Y in senso orario."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_IT "Dispositivo di Arkanoid"
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_IT "Selezionare il dispositivo che si desidera utilizzare per la pagaia Arkanoid."
#define OPTION_VAL_MOUSE_IT NULL
#define OPTION_VAL_POINTER_IT "Puntatore"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_IT "Dispositivo Zapper"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_IT "Selezionare il dispositivo che si desidera utilizzare per lo Zapper."
#define OPTION_VAL_LIGHTGUN_IT "Pistola Ottica"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_IT "Mostra Mirino"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_IT "Imposta se mostrare il mirino quando viene utilizzato lo Zapper."
#define NESTOPIA_TURBO_PULSE_LABEL_IT "Velocità Turbo Pulse"
#define NESTOPIA_TURBO_PULSE_INFO_0_IT "Impostare la velocità di impulso turbo per i pulsanti Turbo B e Turbo A."
#define NESTOPIA_NOSPRITELIMIT_LABEL_IT "Rimuovi Limite Sprite"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_IT "Rimuovere il limite hardware 8-sprites-per-scanline."
#define NESTOPIA_OVERCLOCK_LABEL_IT "Velocità CPU (Overclock)"
#define NESTOPIA_OVERCLOCK_INFO_0_IT "Sovrascrivi la CPU emulata."
#define OPTION_VAL_1X_IT NULL
#define OPTION_VAL_2X_IT NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_IT "Stato Potenziamento RAM"
#define NESTOPIA_RAM_POWER_STATE_INFO_0_IT "Valori RAM sull'alimentazione. Alcuni giochi si basano su valori RAM iniziali per la generazione di numeri casuali come esempio."
#define OPTION_VAL_0X00_IT NULL
#define OPTION_VAL_0XFF_IT NULL
#define OPTION_VAL_RANDOM_IT "Casuale"

struct retro_core_option_v2_category option_cats_it[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_IT,
      CATEGORY_SYSTEM_INFO_0_IT
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_IT,
      CATEGORY_VIDEO_INFO_0_IT
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_IT,
      CATEGORY_AUDIO_INFO_0_IT
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_IT,
      CATEGORY_INPUT_INFO_0_IT
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_IT,
      CATEGORY_HACKS_INFO_0_IT
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_it[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_IT,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_IT,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_IT },
         { "ntsc",    OPTION_VAL_NTSC_IT },
         { "pal",     OPTION_VAL_PAL_IT },
         { "famicom", OPTION_VAL_FAMICOM_IT },
         { "dendy",   OPTION_VAL_DENDY_IT },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_IT,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_IT,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_IT,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_IT,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_IT },
         { "svideo",     OPTION_VAL_SVIDEO_IT },
         { "rgb",        OPTION_VAL_RGB_IT },
         { "monochrome", OPTION_VAL_MONOCHROME_IT },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_IT,
      NULL,
      NESTOPIA_PALETTE_INFO_0_IT,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_IT },
         { "consumer",             OPTION_VAL_CONSUMER_IT },
         { "canonical",            OPTION_VAL_CANONICAL_IT },
         { "alternative",          OPTION_VAL_ALTERNATIVE_IT },
         { "rgb",                  OPTION_VAL_RGB_O3_IT },
         { "pal",                  OPTION_VAL_PAL_IT },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_IT },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_IT },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_IT },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_IT },
         { "raw",                  OPTION_VAL_RAW_IT },
         { "custom",               OPTION_VAL_CUSTOM_IT },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_IT,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_IT,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_IT,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_IT,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_IT,
      NULL,
      NESTOPIA_ASPECT_INFO_0_IT,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_IT },
         { "ntsc",        OPTION_VAL_NTSC_IT },
         { "pal",         OPTION_VAL_PAL_IT },
         { "4:3",         OPTION_VAL_4_3_IT },
         { "uncorrected", OPTION_VAL_UNCORRECTED_IT },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_IT,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_IT,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_IT,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_IT,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_IT,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_IT,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_IT,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_IT,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_IT,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_IT,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_IT,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_IT,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_IT,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_IT,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_IT,
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
      NESTOPIA_AUDIO_TYPE_LABEL_IT,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_IT,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_IT },
         { "stereo", OPTION_VAL_STEREO_IT },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_IT,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_IT,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_IT },
         { "ntsc",    OPTION_VAL_NTSC_IT },
         { "famicom", OPTION_VAL_FAMICOM_IT },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_IT,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_IT, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_IT,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_IT,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_IT },
         { "pointer", OPTION_VAL_POINTER_IT },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_IT,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_IT,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_IT },
         { "mouse", OPTION_VAL_MOUSE_IT },
         { "pointer", OPTION_VAL_POINTER_IT },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_IT,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_IT,
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
      NESTOPIA_TURBO_PULSE_LABEL_IT,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_IT,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_IT,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_IT,
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
      NESTOPIA_OVERCLOCK_LABEL_IT,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_IT,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_IT },
         { "2x", OPTION_VAL_2X_IT },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_IT,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_IT,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_IT },
         { "0xFF",   OPTION_VAL_0XFF_IT },
         { "random", OPTION_VAL_RANDOM_IT },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_it = {
   option_cats_it,
   option_defs_it
};

/* RETRO_LANGUAGE_JA */

#define CATEGORY_SYSTEM_LABEL_JA "システム"
#define CATEGORY_SYSTEM_INFO_0_JA NULL
#define CATEGORY_VIDEO_LABEL_JA "ビデオのドライバ"
#define CATEGORY_VIDEO_INFO_0_JA NULL
#define CATEGORY_AUDIO_LABEL_JA "オーディオのドライバ"
#define CATEGORY_AUDIO_INFO_0_JA NULL
#define CATEGORY_INPUT_LABEL_JA "入力"
#define CATEGORY_INPUT_INFO_0_JA NULL
#define CATEGORY_HACKS_LABEL_JA "エミュレーションハック"
#define CATEGORY_HACKS_INFO_0_JA NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_JA "システム地域"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_JA NULL
#define OPTION_VAL_AUTO_JA "自動"
#define OPTION_VAL_NTSC_JA NULL
#define OPTION_VAL_PAL_JA NULL
#define OPTION_VAL_FAMICOM_JA NULL
#define OPTION_VAL_DENDY_JA NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_JA NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_JA NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_JA NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_JA NULL
#define OPTION_VAL_COMPOSITE_JA NULL
#define OPTION_VAL_SVIDEO_JA NULL
#define OPTION_VAL_RGB_JA NULL
#define OPTION_VAL_MONOCHROME_JA "モノクローム"
#define NESTOPIA_PALETTE_LABEL_JA NULL
#define NESTOPIA_PALETTE_INFO_0_JA NULL
#define OPTION_VAL_CXA2025AS_JA NULL
#define OPTION_VAL_CONSUMER_JA NULL
#define OPTION_VAL_CANONICAL_JA NULL
#define OPTION_VAL_ALTERNATIVE_JA NULL
#define OPTION_VAL_RGB_O3_JA NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_JA NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_JA NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_JA NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_JA NULL
#define OPTION_VAL_RAW_JA NULL
#define OPTION_VAL_CUSTOM_JA "カスタム"
#define NESTOPIA_OVERSCAN_V_LABEL_JA NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_JA NULL
#define NESTOPIA_OVERSCAN_H_LABEL_JA NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_JA NULL
#define NESTOPIA_ASPECT_LABEL_JA "優先アスペクト比"
#define NESTOPIA_ASPECT_INFO_0_JA NULL
#define OPTION_VAL_4_3_JA NULL
#define OPTION_VAL_UNCORRECTED_JA NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_JA NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_JA NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_JA NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_JA NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_JA NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_JA NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_JA NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_JA NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_JA NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_JA NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_JA NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_JA NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_JA NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_JA NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_JA NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_JA NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_JA NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_JA NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_JA NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_JA NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_JA NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_JA NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_JA NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_JA NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_JA NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_JA NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_JA NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_JA NULL
#define OPTION_VAL_MONO_JA NULL
#define OPTION_VAL_STEREO_JA NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_JA NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_JA NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_JA NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_JA NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_JA NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_JA NULL
#define OPTION_VAL_MOUSE_JA NULL
#define OPTION_VAL_POINTER_JA NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_JA NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_JA NULL
#define OPTION_VAL_LIGHTGUN_JA NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_JA NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_JA NULL
#define NESTOPIA_TURBO_PULSE_LABEL_JA NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_JA NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_JA NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_JA NULL
#define NESTOPIA_OVERCLOCK_LABEL_JA NULL
#define NESTOPIA_OVERCLOCK_INFO_0_JA NULL
#define OPTION_VAL_1X_JA NULL
#define OPTION_VAL_2X_JA NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_JA NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_JA NULL
#define OPTION_VAL_0X00_JA NULL
#define OPTION_VAL_0XFF_JA NULL
#define OPTION_VAL_RANDOM_JA NULL

struct retro_core_option_v2_category option_cats_ja[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_JA,
      CATEGORY_SYSTEM_INFO_0_JA
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_JA,
      CATEGORY_VIDEO_INFO_0_JA
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_JA,
      CATEGORY_AUDIO_INFO_0_JA
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_JA,
      CATEGORY_INPUT_INFO_0_JA
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_JA,
      CATEGORY_HACKS_INFO_0_JA
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ja[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_JA,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_JA,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_JA },
         { "ntsc",    OPTION_VAL_NTSC_JA },
         { "pal",     OPTION_VAL_PAL_JA },
         { "famicom", OPTION_VAL_FAMICOM_JA },
         { "dendy",   OPTION_VAL_DENDY_JA },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_JA,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_JA,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_JA,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_JA,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_JA },
         { "svideo",     OPTION_VAL_SVIDEO_JA },
         { "rgb",        OPTION_VAL_RGB_JA },
         { "monochrome", OPTION_VAL_MONOCHROME_JA },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_JA,
      NULL,
      NESTOPIA_PALETTE_INFO_0_JA,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_JA },
         { "consumer",             OPTION_VAL_CONSUMER_JA },
         { "canonical",            OPTION_VAL_CANONICAL_JA },
         { "alternative",          OPTION_VAL_ALTERNATIVE_JA },
         { "rgb",                  OPTION_VAL_RGB_O3_JA },
         { "pal",                  OPTION_VAL_PAL_JA },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_JA },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_JA },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_JA },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_JA },
         { "raw",                  OPTION_VAL_RAW_JA },
         { "custom",               OPTION_VAL_CUSTOM_JA },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_JA,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_JA,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_JA,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_JA,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_JA,
      NULL,
      NESTOPIA_ASPECT_INFO_0_JA,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_JA },
         { "ntsc",        OPTION_VAL_NTSC_JA },
         { "pal",         OPTION_VAL_PAL_JA },
         { "4:3",         OPTION_VAL_4_3_JA },
         { "uncorrected", OPTION_VAL_UNCORRECTED_JA },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_JA,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_JA,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_JA,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_JA,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_JA,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_JA,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_JA,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_JA,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_JA,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_JA,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_JA,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_JA,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_JA,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_JA,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_JA,
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
      NESTOPIA_AUDIO_TYPE_LABEL_JA,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_JA,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_JA },
         { "stereo", OPTION_VAL_STEREO_JA },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_JA,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_JA,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_JA },
         { "ntsc",    OPTION_VAL_NTSC_JA },
         { "famicom", OPTION_VAL_FAMICOM_JA },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_JA,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_JA, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_JA,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_JA,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_JA },
         { "pointer", OPTION_VAL_POINTER_JA },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_JA,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_JA,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_JA },
         { "mouse", OPTION_VAL_MOUSE_JA },
         { "pointer", OPTION_VAL_POINTER_JA },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_JA,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_JA,
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
      NESTOPIA_TURBO_PULSE_LABEL_JA,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_JA,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_JA,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_JA,
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
      NESTOPIA_OVERCLOCK_LABEL_JA,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_JA,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_JA },
         { "2x", OPTION_VAL_2X_JA },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_JA,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_JA,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_JA },
         { "0xFF",   OPTION_VAL_0XFF_JA },
         { "random", OPTION_VAL_RANDOM_JA },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ja = {
   option_cats_ja,
   option_defs_ja
};

/* RETRO_LANGUAGE_KO */

#define CATEGORY_SYSTEM_LABEL_KO "시스템"
#define CATEGORY_SYSTEM_INFO_0_KO NULL
#define CATEGORY_VIDEO_LABEL_KO "비디오"
#define CATEGORY_VIDEO_INFO_0_KO "화면비, 화면 잘라내기, 색상 팔레트, 비디오 필터 등을 설정합니다."
#define CATEGORY_AUDIO_LABEL_KO "오디오"
#define CATEGORY_AUDIO_INFO_0_KO "오디오 효과와 채널 볼륨을 설정합니다."
#define CATEGORY_INPUT_LABEL_KO "입력"
#define CATEGORY_INPUT_INFO_0_KO "입력 장치 및 입력 관련 설정을 변경합니다."
#define CATEGORY_HACKS_LABEL_KO "에뮬레이션 핵"
#define CATEGORY_HACKS_INFO_0_KO "성능과 호환성에 영향을 끼치는 프로세서 오버클러킹 및 에뮬레이션 정확도 설정 등을 변경합니다."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_KO "시스템 지역"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_KO "'자동'은 NstDatabase.xml 데이터베이스에 기록된 내용에 따라 화면비를 자동 감지합니다. 데이터베이스에 자료가 없을 경우, NTSC가 사용됩니다."
#define OPTION_VAL_AUTO_KO "자동"
#define OPTION_VAL_NTSC_KO NULL
#define OPTION_VAL_PAL_KO NULL
#define OPTION_VAL_FAMICOM_KO "패미컴"
#define OPTION_VAL_DENDY_KO NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_KO "FDS 자동 삽입"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_KO "초기화할 때 자동으로 첫 번째 FDS 디스크를 삽입합니다."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_KO "Blargg NTSC 필터"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_KO "Blargg NTSC 필터를 사용합니다."
#define OPTION_VAL_COMPOSITE_KO "컴포지트 비디오"
#define OPTION_VAL_SVIDEO_KO "S-비디오"
#define OPTION_VAL_RGB_KO NULL
#define OPTION_VAL_MONOCHROME_KO "흑백"
#define NESTOPIA_PALETTE_LABEL_KO "팔레트"
#define NESTOPIA_PALETTE_INFO_0_KO NULL
#define OPTION_VAL_CXA2025AS_KO NULL
#define OPTION_VAL_CONSUMER_KO "일반 TV"
#define OPTION_VAL_CANONICAL_KO "표준"
#define OPTION_VAL_ALTERNATIVE_KO "대체 표준"
#define OPTION_VAL_RGB_O3_KO NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_KO NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_KO NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_KO NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_KO "NES Classic FBx (개선판)"
#define OPTION_VAL_RAW_KO "원본"
#define OPTION_VAL_CUSTOM_KO "사용자 정의"
#define NESTOPIA_OVERSCAN_V_LABEL_KO "오버스캔 숨기기 (세로)"
#define NESTOPIA_OVERSCAN_V_INFO_0_KO "일반적으로 TV 화면의 테두리 베젤에 의해 가려지는 비디오 출력의 이상한 부분(세로)을 가립니다."
#define NESTOPIA_OVERSCAN_H_LABEL_KO "오버스캔 숨기기 (가로)"
#define NESTOPIA_OVERSCAN_H_INFO_0_KO "일반적으로 TV 화면의 테두리 베젤에 의해 가려지는 비디오 출력의 이상한 부분(가로)을 가립니다."
#define NESTOPIA_ASPECT_LABEL_KO "선호하는 화면비"
#define NESTOPIA_ASPECT_INFO_0_KO NULL
#define OPTION_VAL_4_3_KO NULL
#define OPTION_VAL_UNCORRECTED_KO "교정 안 됨"
#define NESTOPIA_GENIE_DISTORTION_LABEL_KO "Game Genie 소리 왜곡"
#define NESTOPIA_GENIE_DISTORTION_INFO_0_KO "Game Genie 치트 장치는 의도치 않게 게임 소리를 왜곡시킵니다. 이 옵션을 사용하면 Game Genie가 게임에 추가하는 소리 왜곡을 재현할 수 있습니다."
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_KO "고급 오디오 설정 표시 (메뉴 다시 열기 필요)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_KO "저수준 오디오 채널 변수 설정을 활성화합니다."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_KO "구형파 1 채널 볼륨 %"
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_KO "구형파 1 채널의 볼륨 %를 변경합니다."
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_KO "구형파 2 채널 볼륨 %"
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_KO "구형파 2 채널의 볼륨 %를 변경합니다."
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_KO "삼각파 채널 볼륨 %"
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_KO "삼각파 채널의 볼륨 %를 변경합니다."
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_KO "노이즈 채널 볼륨 %"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_KO "노이즈 채널의 볼륨 %를 변경합니다."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_KO "DPCM 채널 볼륨 %"
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_KO "DPCM 채널의 볼륨 %를 변경합니다."
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_KO "FDS 채널 볼륨 %"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_KO "FDS 채널의 볼륨 %를 변경합니다."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_KO "MMC5 채널 볼륨 %"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_KO "MMC5 채널의 볼륨 %를 변경합니다."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_KO "VRC6 채널 볼륨 %"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_KO "VRC6 채널의 볼륨 %를 변경합니다."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_KO "VRC7 채널 볼륨 %"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_KO "VRC7 채널의 볼륨 %를 변경합니다."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_KO "N163 채널 볼륨 %"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_KO "N163 채널의 볼륨 %를 변경합니다."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_KO "S5B 채널 볼륨 %"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_KO NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_KO NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_KO NULL
#define OPTION_VAL_MONO_KO NULL
#define OPTION_VAL_STEREO_KO NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_KO "4 플레이어 어댑터"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_KO "필요할 경우 4 플레이어 어댑터를 수동으로 선택합니다. NstDatabase.xml에 기록되어 있지 않지만 어댑터를 필요로 하는 게임을 플레이할 때 이 옵션으로 문제를 해결할 수 있습니다."
#define NESTOPIA_BUTTON_SHIFT_LABEL_KO "버튼 시계방향 회전"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_KO "A/B/X/Y 버튼을 시계방향으로 회전합니다."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_KO "Arkanoid 장치"
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_KO "Arkanoid의 패들로 사용할 장치를 선택합니다."
#define OPTION_VAL_MOUSE_KO "마우스"
#define OPTION_VAL_POINTER_KO "포인터"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_KO "Zapper 장치"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_KO "Zapper로 사용할 장치를 선택합니다."
#define OPTION_VAL_LIGHTGUN_KO "라이트 건"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_KO "십자선 표시"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_KO "Zapper를 사용할 때 십자선을 표시할 지 설정합니다."
#define NESTOPIA_TURBO_PULSE_LABEL_KO "터보 반복 속도"
#define NESTOPIA_TURBO_PULSE_INFO_0_KO "터보 B 및 터보 A 버튼을 누를 때 입력을 반복할 속도를 설정합니다."
#define NESTOPIA_NOSPRITELIMIT_LABEL_KO "스프라이트 제한 해제"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_KO "원본 하드웨어의 스캔라인 당 스프라이트 8개 제한을 제거합니다."
#define NESTOPIA_OVERCLOCK_LABEL_KO "CPU 속도 (오버클럭)"
#define NESTOPIA_OVERCLOCK_INFO_0_KO "에뮬레이트하는 CPU를 오버클럭합니다."
#define OPTION_VAL_1X_KO NULL
#define OPTION_VAL_2X_KO NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_KO "기동 시 RAM 상태"
#define NESTOPIA_RAM_POWER_STATE_INFO_0_KO "전원이 들어올 때 RAM의 값입니다. 일부 게임은 초기 RAM 값을 난수 생성에 활용하기도 합니다."
#define OPTION_VAL_0X00_KO NULL
#define OPTION_VAL_0XFF_KO NULL
#define OPTION_VAL_RANDOM_KO "무작위"

struct retro_core_option_v2_category option_cats_ko[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_KO,
      CATEGORY_SYSTEM_INFO_0_KO
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_KO,
      CATEGORY_VIDEO_INFO_0_KO
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_KO,
      CATEGORY_AUDIO_INFO_0_KO
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_KO,
      CATEGORY_INPUT_INFO_0_KO
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_KO,
      CATEGORY_HACKS_INFO_0_KO
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ko[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_KO,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_KO,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_KO },
         { "ntsc",    OPTION_VAL_NTSC_KO },
         { "pal",     OPTION_VAL_PAL_KO },
         { "famicom", OPTION_VAL_FAMICOM_KO },
         { "dendy",   OPTION_VAL_DENDY_KO },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_KO,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_KO,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_KO,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_KO,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_KO },
         { "svideo",     OPTION_VAL_SVIDEO_KO },
         { "rgb",        OPTION_VAL_RGB_KO },
         { "monochrome", OPTION_VAL_MONOCHROME_KO },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_KO,
      NULL,
      NESTOPIA_PALETTE_INFO_0_KO,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_KO },
         { "consumer",             OPTION_VAL_CONSUMER_KO },
         { "canonical",            OPTION_VAL_CANONICAL_KO },
         { "alternative",          OPTION_VAL_ALTERNATIVE_KO },
         { "rgb",                  OPTION_VAL_RGB_O3_KO },
         { "pal",                  OPTION_VAL_PAL_KO },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_KO },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_KO },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_KO },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_KO },
         { "raw",                  OPTION_VAL_RAW_KO },
         { "custom",               OPTION_VAL_CUSTOM_KO },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_KO,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_KO,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_KO,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_KO,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_KO,
      NULL,
      NESTOPIA_ASPECT_INFO_0_KO,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_KO },
         { "ntsc",        OPTION_VAL_NTSC_KO },
         { "pal",         OPTION_VAL_PAL_KO },
         { "4:3",         OPTION_VAL_4_3_KO },
         { "uncorrected", OPTION_VAL_UNCORRECTED_KO },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_KO,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_KO,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_KO,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_KO,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_KO,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_KO,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_KO,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_KO,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_KO,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_KO,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_KO,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_KO,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_KO,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_KO,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_KO,
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
      NESTOPIA_AUDIO_TYPE_LABEL_KO,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_KO,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_KO },
         { "stereo", OPTION_VAL_STEREO_KO },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_KO,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_KO,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_KO },
         { "ntsc",    OPTION_VAL_NTSC_KO },
         { "famicom", OPTION_VAL_FAMICOM_KO },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_KO,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_KO, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_KO,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_KO,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_KO },
         { "pointer", OPTION_VAL_POINTER_KO },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_KO,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_KO,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_KO },
         { "mouse", OPTION_VAL_MOUSE_KO },
         { "pointer", OPTION_VAL_POINTER_KO },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_KO,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_KO,
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
      NESTOPIA_TURBO_PULSE_LABEL_KO,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_KO,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_KO,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_KO,
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
      NESTOPIA_OVERCLOCK_LABEL_KO,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_KO,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_KO },
         { "2x", OPTION_VAL_2X_KO },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_KO,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_KO,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_KO },
         { "0xFF",   OPTION_VAL_0XFF_KO },
         { "random", OPTION_VAL_RANDOM_KO },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ko = {
   option_cats_ko,
   option_defs_ko
};

/* RETRO_LANGUAGE_MT */

#define CATEGORY_SYSTEM_LABEL_MT NULL
#define CATEGORY_SYSTEM_INFO_0_MT NULL
#define CATEGORY_VIDEO_LABEL_MT NULL
#define CATEGORY_VIDEO_INFO_0_MT NULL
#define CATEGORY_AUDIO_LABEL_MT NULL
#define CATEGORY_AUDIO_INFO_0_MT NULL
#define CATEGORY_INPUT_LABEL_MT NULL
#define CATEGORY_INPUT_INFO_0_MT NULL
#define CATEGORY_HACKS_LABEL_MT NULL
#define CATEGORY_HACKS_INFO_0_MT NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_MT NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_MT NULL
#define OPTION_VAL_AUTO_MT NULL
#define OPTION_VAL_NTSC_MT NULL
#define OPTION_VAL_PAL_MT NULL
#define OPTION_VAL_FAMICOM_MT NULL
#define OPTION_VAL_DENDY_MT NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_MT NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_MT NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_MT NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_MT NULL
#define OPTION_VAL_COMPOSITE_MT NULL
#define OPTION_VAL_SVIDEO_MT NULL
#define OPTION_VAL_RGB_MT NULL
#define OPTION_VAL_MONOCHROME_MT NULL
#define NESTOPIA_PALETTE_LABEL_MT NULL
#define NESTOPIA_PALETTE_INFO_0_MT NULL
#define OPTION_VAL_CXA2025AS_MT NULL
#define OPTION_VAL_CONSUMER_MT NULL
#define OPTION_VAL_CANONICAL_MT NULL
#define OPTION_VAL_ALTERNATIVE_MT NULL
#define OPTION_VAL_RGB_O3_MT NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_MT NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_MT NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_MT NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_MT NULL
#define OPTION_VAL_RAW_MT NULL
#define OPTION_VAL_CUSTOM_MT NULL
#define NESTOPIA_OVERSCAN_V_LABEL_MT NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_MT NULL
#define NESTOPIA_OVERSCAN_H_LABEL_MT NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_MT NULL
#define NESTOPIA_ASPECT_LABEL_MT NULL
#define NESTOPIA_ASPECT_INFO_0_MT NULL
#define OPTION_VAL_4_3_MT NULL
#define OPTION_VAL_UNCORRECTED_MT NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_MT NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_MT NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_MT NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_MT NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_MT NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_MT NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_MT NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_MT NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_MT NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_MT NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_MT NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_MT NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_MT NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_MT NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_MT NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_MT NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_MT NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_MT NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_MT NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_MT NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_MT NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_MT NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_MT NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_MT NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_MT NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_MT NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_MT NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_MT NULL
#define OPTION_VAL_MONO_MT NULL
#define OPTION_VAL_STEREO_MT NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_MT NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_MT NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_MT NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_MT NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_MT NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_MT NULL
#define OPTION_VAL_MOUSE_MT NULL
#define OPTION_VAL_POINTER_MT NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_MT NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_MT NULL
#define OPTION_VAL_LIGHTGUN_MT NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_MT NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_MT NULL
#define NESTOPIA_TURBO_PULSE_LABEL_MT NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_MT NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_MT NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_MT NULL
#define NESTOPIA_OVERCLOCK_LABEL_MT NULL
#define NESTOPIA_OVERCLOCK_INFO_0_MT NULL
#define OPTION_VAL_1X_MT NULL
#define OPTION_VAL_2X_MT NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_MT NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_MT NULL
#define OPTION_VAL_0X00_MT NULL
#define OPTION_VAL_0XFF_MT NULL
#define OPTION_VAL_RANDOM_MT NULL

struct retro_core_option_v2_category option_cats_mt[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_MT,
      CATEGORY_SYSTEM_INFO_0_MT
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_MT,
      CATEGORY_VIDEO_INFO_0_MT
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_MT,
      CATEGORY_AUDIO_INFO_0_MT
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_MT,
      CATEGORY_INPUT_INFO_0_MT
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_MT,
      CATEGORY_HACKS_INFO_0_MT
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_mt[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_MT,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_MT,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_MT },
         { "ntsc",    OPTION_VAL_NTSC_MT },
         { "pal",     OPTION_VAL_PAL_MT },
         { "famicom", OPTION_VAL_FAMICOM_MT },
         { "dendy",   OPTION_VAL_DENDY_MT },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_MT,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_MT,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_MT,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_MT,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_MT },
         { "svideo",     OPTION_VAL_SVIDEO_MT },
         { "rgb",        OPTION_VAL_RGB_MT },
         { "monochrome", OPTION_VAL_MONOCHROME_MT },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_MT,
      NULL,
      NESTOPIA_PALETTE_INFO_0_MT,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_MT },
         { "consumer",             OPTION_VAL_CONSUMER_MT },
         { "canonical",            OPTION_VAL_CANONICAL_MT },
         { "alternative",          OPTION_VAL_ALTERNATIVE_MT },
         { "rgb",                  OPTION_VAL_RGB_O3_MT },
         { "pal",                  OPTION_VAL_PAL_MT },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_MT },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_MT },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_MT },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_MT },
         { "raw",                  OPTION_VAL_RAW_MT },
         { "custom",               OPTION_VAL_CUSTOM_MT },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_MT,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_MT,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_MT,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_MT,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_MT,
      NULL,
      NESTOPIA_ASPECT_INFO_0_MT,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_MT },
         { "ntsc",        OPTION_VAL_NTSC_MT },
         { "pal",         OPTION_VAL_PAL_MT },
         { "4:3",         OPTION_VAL_4_3_MT },
         { "uncorrected", OPTION_VAL_UNCORRECTED_MT },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_MT,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_MT,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_MT,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_MT,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_MT,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_MT,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_MT,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_MT,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_MT,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_MT,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_MT,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_MT,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_MT,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_MT,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_MT,
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
      NESTOPIA_AUDIO_TYPE_LABEL_MT,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_MT,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_MT },
         { "stereo", OPTION_VAL_STEREO_MT },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_MT,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_MT,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_MT },
         { "ntsc",    OPTION_VAL_NTSC_MT },
         { "famicom", OPTION_VAL_FAMICOM_MT },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_MT,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_MT, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_MT,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_MT,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_MT },
         { "pointer", OPTION_VAL_POINTER_MT },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_MT,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_MT,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_MT },
         { "mouse", OPTION_VAL_MOUSE_MT },
         { "pointer", OPTION_VAL_POINTER_MT },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_MT,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_MT,
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
      NESTOPIA_TURBO_PULSE_LABEL_MT,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_MT,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_MT,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_MT,
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
      NESTOPIA_OVERCLOCK_LABEL_MT,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_MT,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_MT },
         { "2x", OPTION_VAL_2X_MT },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_MT,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_MT,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_MT },
         { "0xFF",   OPTION_VAL_0XFF_MT },
         { "random", OPTION_VAL_RANDOM_MT },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_mt = {
   option_cats_mt,
   option_defs_mt
};

/* RETRO_LANGUAGE_NL */

#define CATEGORY_SYSTEM_LABEL_NL "Systeem"
#define CATEGORY_SYSTEM_INFO_0_NL NULL
#define CATEGORY_VIDEO_LABEL_NL NULL
#define CATEGORY_VIDEO_INFO_0_NL NULL
#define CATEGORY_AUDIO_LABEL_NL "Geluid"
#define CATEGORY_AUDIO_INFO_0_NL NULL
#define CATEGORY_INPUT_LABEL_NL "Invoer"
#define CATEGORY_INPUT_INFO_0_NL NULL
#define CATEGORY_HACKS_LABEL_NL NULL
#define CATEGORY_HACKS_INFO_0_NL NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_NL NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_NL NULL
#define OPTION_VAL_AUTO_NL "Automatisch"
#define OPTION_VAL_NTSC_NL NULL
#define OPTION_VAL_PAL_NL NULL
#define OPTION_VAL_FAMICOM_NL NULL
#define OPTION_VAL_DENDY_NL NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_NL NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_NL NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_NL NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_NL NULL
#define OPTION_VAL_COMPOSITE_NL NULL
#define OPTION_VAL_SVIDEO_NL NULL
#define OPTION_VAL_RGB_NL NULL
#define OPTION_VAL_MONOCHROME_NL NULL
#define NESTOPIA_PALETTE_LABEL_NL NULL
#define NESTOPIA_PALETTE_INFO_0_NL NULL
#define OPTION_VAL_CXA2025AS_NL NULL
#define OPTION_VAL_CONSUMER_NL NULL
#define OPTION_VAL_CANONICAL_NL NULL
#define OPTION_VAL_ALTERNATIVE_NL NULL
#define OPTION_VAL_RGB_O3_NL NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_NL NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_NL NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_NL NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_NL NULL
#define OPTION_VAL_RAW_NL NULL
#define OPTION_VAL_CUSTOM_NL "Aangepast"
#define NESTOPIA_OVERSCAN_V_LABEL_NL NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_NL NULL
#define NESTOPIA_OVERSCAN_H_LABEL_NL NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_NL NULL
#define NESTOPIA_ASPECT_LABEL_NL NULL
#define NESTOPIA_ASPECT_INFO_0_NL NULL
#define OPTION_VAL_4_3_NL NULL
#define OPTION_VAL_UNCORRECTED_NL NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_NL NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_NL NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_NL NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_NL NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_NL NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_NL NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_NL NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_NL NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_NL NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_NL NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_NL NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_NL NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_NL NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_NL NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_NL NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_NL NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_NL NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_NL NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_NL NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_NL NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_NL NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_NL NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_NL NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_NL NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_NL NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_NL NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_NL NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_NL NULL
#define OPTION_VAL_MONO_NL NULL
#define OPTION_VAL_STEREO_NL NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_NL NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_NL NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_NL NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_NL NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_NL NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_NL NULL
#define OPTION_VAL_MOUSE_NL NULL
#define OPTION_VAL_POINTER_NL NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_NL NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_NL NULL
#define OPTION_VAL_LIGHTGUN_NL NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_NL NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_NL NULL
#define NESTOPIA_TURBO_PULSE_LABEL_NL NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_NL NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_NL NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_NL NULL
#define NESTOPIA_OVERCLOCK_LABEL_NL NULL
#define NESTOPIA_OVERCLOCK_INFO_0_NL NULL
#define OPTION_VAL_1X_NL NULL
#define OPTION_VAL_2X_NL NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_NL NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_NL NULL
#define OPTION_VAL_0X00_NL NULL
#define OPTION_VAL_0XFF_NL NULL
#define OPTION_VAL_RANDOM_NL NULL

struct retro_core_option_v2_category option_cats_nl[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_NL,
      CATEGORY_SYSTEM_INFO_0_NL
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_NL,
      CATEGORY_VIDEO_INFO_0_NL
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_NL,
      CATEGORY_AUDIO_INFO_0_NL
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_NL,
      CATEGORY_INPUT_INFO_0_NL
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_NL,
      CATEGORY_HACKS_INFO_0_NL
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_nl[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_NL,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_NL,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_NL },
         { "ntsc",    OPTION_VAL_NTSC_NL },
         { "pal",     OPTION_VAL_PAL_NL },
         { "famicom", OPTION_VAL_FAMICOM_NL },
         { "dendy",   OPTION_VAL_DENDY_NL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_NL,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_NL,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_NL,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_NL,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_NL },
         { "svideo",     OPTION_VAL_SVIDEO_NL },
         { "rgb",        OPTION_VAL_RGB_NL },
         { "monochrome", OPTION_VAL_MONOCHROME_NL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_NL,
      NULL,
      NESTOPIA_PALETTE_INFO_0_NL,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_NL },
         { "consumer",             OPTION_VAL_CONSUMER_NL },
         { "canonical",            OPTION_VAL_CANONICAL_NL },
         { "alternative",          OPTION_VAL_ALTERNATIVE_NL },
         { "rgb",                  OPTION_VAL_RGB_O3_NL },
         { "pal",                  OPTION_VAL_PAL_NL },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_NL },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_NL },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_NL },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_NL },
         { "raw",                  OPTION_VAL_RAW_NL },
         { "custom",               OPTION_VAL_CUSTOM_NL },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_NL,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_NL,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_NL,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_NL,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_NL,
      NULL,
      NESTOPIA_ASPECT_INFO_0_NL,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_NL },
         { "ntsc",        OPTION_VAL_NTSC_NL },
         { "pal",         OPTION_VAL_PAL_NL },
         { "4:3",         OPTION_VAL_4_3_NL },
         { "uncorrected", OPTION_VAL_UNCORRECTED_NL },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_NL,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_NL,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_NL,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_NL,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_NL,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_NL,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_NL,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_NL,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_NL,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_NL,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_NL,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_NL,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_NL,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_NL,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_NL,
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
      NESTOPIA_AUDIO_TYPE_LABEL_NL,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_NL,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_NL },
         { "stereo", OPTION_VAL_STEREO_NL },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_NL,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_NL,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_NL },
         { "ntsc",    OPTION_VAL_NTSC_NL },
         { "famicom", OPTION_VAL_FAMICOM_NL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_NL,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_NL, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_NL,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_NL,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_NL },
         { "pointer", OPTION_VAL_POINTER_NL },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_NL,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_NL,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_NL },
         { "mouse", OPTION_VAL_MOUSE_NL },
         { "pointer", OPTION_VAL_POINTER_NL },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_NL,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_NL,
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
      NESTOPIA_TURBO_PULSE_LABEL_NL,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_NL,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_NL,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_NL,
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
      NESTOPIA_OVERCLOCK_LABEL_NL,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_NL,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_NL },
         { "2x", OPTION_VAL_2X_NL },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_NL,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_NL,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_NL },
         { "0xFF",   OPTION_VAL_0XFF_NL },
         { "random", OPTION_VAL_RANDOM_NL },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_nl = {
   option_cats_nl,
   option_defs_nl
};

/* RETRO_LANGUAGE_NO */

#define CATEGORY_SYSTEM_LABEL_NO NULL
#define CATEGORY_SYSTEM_INFO_0_NO NULL
#define CATEGORY_VIDEO_LABEL_NO NULL
#define CATEGORY_VIDEO_INFO_0_NO NULL
#define CATEGORY_AUDIO_LABEL_NO "Lyd"
#define CATEGORY_AUDIO_INFO_0_NO NULL
#define CATEGORY_INPUT_LABEL_NO "Inndata"
#define CATEGORY_INPUT_INFO_0_NO NULL
#define CATEGORY_HACKS_LABEL_NO NULL
#define CATEGORY_HACKS_INFO_0_NO NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_NO NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_NO NULL
#define OPTION_VAL_AUTO_NO NULL
#define OPTION_VAL_NTSC_NO NULL
#define OPTION_VAL_PAL_NO NULL
#define OPTION_VAL_FAMICOM_NO NULL
#define OPTION_VAL_DENDY_NO NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_NO NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_NO NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_NO NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_NO NULL
#define OPTION_VAL_COMPOSITE_NO NULL
#define OPTION_VAL_SVIDEO_NO NULL
#define OPTION_VAL_RGB_NO NULL
#define OPTION_VAL_MONOCHROME_NO NULL
#define NESTOPIA_PALETTE_LABEL_NO NULL
#define NESTOPIA_PALETTE_INFO_0_NO NULL
#define OPTION_VAL_CXA2025AS_NO NULL
#define OPTION_VAL_CONSUMER_NO NULL
#define OPTION_VAL_CANONICAL_NO NULL
#define OPTION_VAL_ALTERNATIVE_NO NULL
#define OPTION_VAL_RGB_O3_NO NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_NO NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_NO NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_NO NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_NO NULL
#define OPTION_VAL_RAW_NO NULL
#define OPTION_VAL_CUSTOM_NO NULL
#define NESTOPIA_OVERSCAN_V_LABEL_NO NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_NO NULL
#define NESTOPIA_OVERSCAN_H_LABEL_NO NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_NO NULL
#define NESTOPIA_ASPECT_LABEL_NO NULL
#define NESTOPIA_ASPECT_INFO_0_NO NULL
#define OPTION_VAL_4_3_NO NULL
#define OPTION_VAL_UNCORRECTED_NO NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_NO NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_NO NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_NO NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_NO NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_NO NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_NO NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_NO NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_NO NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_NO NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_NO NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_NO NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_NO NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_NO NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_NO NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_NO NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_NO NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_NO NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_NO NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_NO NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_NO NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_NO NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_NO NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_NO NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_NO NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_NO NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_NO NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_NO NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_NO NULL
#define OPTION_VAL_MONO_NO NULL
#define OPTION_VAL_STEREO_NO NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_NO NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_NO NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_NO NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_NO NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_NO NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_NO NULL
#define OPTION_VAL_MOUSE_NO NULL
#define OPTION_VAL_POINTER_NO NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_NO NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_NO NULL
#define OPTION_VAL_LIGHTGUN_NO NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_NO NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_NO NULL
#define NESTOPIA_TURBO_PULSE_LABEL_NO NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_NO NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_NO NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_NO NULL
#define NESTOPIA_OVERCLOCK_LABEL_NO NULL
#define NESTOPIA_OVERCLOCK_INFO_0_NO NULL
#define OPTION_VAL_1X_NO NULL
#define OPTION_VAL_2X_NO NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_NO NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_NO NULL
#define OPTION_VAL_0X00_NO NULL
#define OPTION_VAL_0XFF_NO NULL
#define OPTION_VAL_RANDOM_NO NULL

struct retro_core_option_v2_category option_cats_no[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_NO,
      CATEGORY_SYSTEM_INFO_0_NO
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_NO,
      CATEGORY_VIDEO_INFO_0_NO
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_NO,
      CATEGORY_AUDIO_INFO_0_NO
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_NO,
      CATEGORY_INPUT_INFO_0_NO
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_NO,
      CATEGORY_HACKS_INFO_0_NO
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_no[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_NO,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_NO,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_NO },
         { "ntsc",    OPTION_VAL_NTSC_NO },
         { "pal",     OPTION_VAL_PAL_NO },
         { "famicom", OPTION_VAL_FAMICOM_NO },
         { "dendy",   OPTION_VAL_DENDY_NO },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_NO,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_NO,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_NO,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_NO,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_NO },
         { "svideo",     OPTION_VAL_SVIDEO_NO },
         { "rgb",        OPTION_VAL_RGB_NO },
         { "monochrome", OPTION_VAL_MONOCHROME_NO },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_NO,
      NULL,
      NESTOPIA_PALETTE_INFO_0_NO,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_NO },
         { "consumer",             OPTION_VAL_CONSUMER_NO },
         { "canonical",            OPTION_VAL_CANONICAL_NO },
         { "alternative",          OPTION_VAL_ALTERNATIVE_NO },
         { "rgb",                  OPTION_VAL_RGB_O3_NO },
         { "pal",                  OPTION_VAL_PAL_NO },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_NO },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_NO },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_NO },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_NO },
         { "raw",                  OPTION_VAL_RAW_NO },
         { "custom",               OPTION_VAL_CUSTOM_NO },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_NO,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_NO,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_NO,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_NO,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_NO,
      NULL,
      NESTOPIA_ASPECT_INFO_0_NO,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_NO },
         { "ntsc",        OPTION_VAL_NTSC_NO },
         { "pal",         OPTION_VAL_PAL_NO },
         { "4:3",         OPTION_VAL_4_3_NO },
         { "uncorrected", OPTION_VAL_UNCORRECTED_NO },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_NO,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_NO,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_NO,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_NO,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_NO,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_NO,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_NO,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_NO,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_NO,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_NO,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_NO,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_NO,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_NO,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_NO,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_NO,
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
      NESTOPIA_AUDIO_TYPE_LABEL_NO,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_NO,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_NO },
         { "stereo", OPTION_VAL_STEREO_NO },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_NO,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_NO,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_NO },
         { "ntsc",    OPTION_VAL_NTSC_NO },
         { "famicom", OPTION_VAL_FAMICOM_NO },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_NO,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_NO, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_NO,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_NO,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_NO },
         { "pointer", OPTION_VAL_POINTER_NO },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_NO,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_NO,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_NO },
         { "mouse", OPTION_VAL_MOUSE_NO },
         { "pointer", OPTION_VAL_POINTER_NO },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_NO,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_NO,
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
      NESTOPIA_TURBO_PULSE_LABEL_NO,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_NO,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_NO,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_NO,
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
      NESTOPIA_OVERCLOCK_LABEL_NO,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_NO,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_NO },
         { "2x", OPTION_VAL_2X_NO },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_NO,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_NO,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_NO },
         { "0xFF",   OPTION_VAL_0XFF_NO },
         { "random", OPTION_VAL_RANDOM_NO },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_no = {
   option_cats_no,
   option_defs_no
};

/* RETRO_LANGUAGE_OC */

#define CATEGORY_SYSTEM_LABEL_OC NULL
#define CATEGORY_SYSTEM_INFO_0_OC NULL
#define CATEGORY_VIDEO_LABEL_OC "Vidèo"
#define CATEGORY_VIDEO_INFO_0_OC NULL
#define CATEGORY_AUDIO_LABEL_OC NULL
#define CATEGORY_AUDIO_INFO_0_OC NULL
#define CATEGORY_INPUT_LABEL_OC NULL
#define CATEGORY_INPUT_INFO_0_OC NULL
#define CATEGORY_HACKS_LABEL_OC NULL
#define CATEGORY_HACKS_INFO_0_OC NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_OC NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_OC NULL
#define OPTION_VAL_AUTO_OC NULL
#define OPTION_VAL_NTSC_OC NULL
#define OPTION_VAL_PAL_OC NULL
#define OPTION_VAL_FAMICOM_OC NULL
#define OPTION_VAL_DENDY_OC NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_OC NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_OC NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_OC NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_OC NULL
#define OPTION_VAL_COMPOSITE_OC NULL
#define OPTION_VAL_SVIDEO_OC NULL
#define OPTION_VAL_RGB_OC NULL
#define OPTION_VAL_MONOCHROME_OC NULL
#define NESTOPIA_PALETTE_LABEL_OC NULL
#define NESTOPIA_PALETTE_INFO_0_OC NULL
#define OPTION_VAL_CXA2025AS_OC NULL
#define OPTION_VAL_CONSUMER_OC NULL
#define OPTION_VAL_CANONICAL_OC NULL
#define OPTION_VAL_ALTERNATIVE_OC NULL
#define OPTION_VAL_RGB_O3_OC NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_OC NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_OC NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_OC NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_OC NULL
#define OPTION_VAL_RAW_OC NULL
#define OPTION_VAL_CUSTOM_OC NULL
#define NESTOPIA_OVERSCAN_V_LABEL_OC NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_OC NULL
#define NESTOPIA_OVERSCAN_H_LABEL_OC NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_OC NULL
#define NESTOPIA_ASPECT_LABEL_OC NULL
#define NESTOPIA_ASPECT_INFO_0_OC NULL
#define OPTION_VAL_4_3_OC NULL
#define OPTION_VAL_UNCORRECTED_OC NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_OC NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_OC NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_OC NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_OC NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_OC NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_OC NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_OC NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_OC NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_OC NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_OC NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_OC NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_OC NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_OC NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_OC NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_OC NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_OC NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_OC NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_OC NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_OC NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_OC NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_OC NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_OC NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_OC NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_OC NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_OC NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_OC NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_OC NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_OC NULL
#define OPTION_VAL_MONO_OC NULL
#define OPTION_VAL_STEREO_OC NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_OC NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_OC NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_OC NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_OC NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_OC NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_OC NULL
#define OPTION_VAL_MOUSE_OC NULL
#define OPTION_VAL_POINTER_OC NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_OC NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_OC NULL
#define OPTION_VAL_LIGHTGUN_OC NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_OC NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_OC NULL
#define NESTOPIA_TURBO_PULSE_LABEL_OC NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_OC NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_OC NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_OC NULL
#define NESTOPIA_OVERCLOCK_LABEL_OC NULL
#define NESTOPIA_OVERCLOCK_INFO_0_OC NULL
#define OPTION_VAL_1X_OC NULL
#define OPTION_VAL_2X_OC NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_OC NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_OC NULL
#define OPTION_VAL_0X00_OC NULL
#define OPTION_VAL_0XFF_OC NULL
#define OPTION_VAL_RANDOM_OC NULL

struct retro_core_option_v2_category option_cats_oc[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_OC,
      CATEGORY_SYSTEM_INFO_0_OC
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_OC,
      CATEGORY_VIDEO_INFO_0_OC
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_OC,
      CATEGORY_AUDIO_INFO_0_OC
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_OC,
      CATEGORY_INPUT_INFO_0_OC
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_OC,
      CATEGORY_HACKS_INFO_0_OC
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_oc[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_OC,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_OC,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_OC },
         { "ntsc",    OPTION_VAL_NTSC_OC },
         { "pal",     OPTION_VAL_PAL_OC },
         { "famicom", OPTION_VAL_FAMICOM_OC },
         { "dendy",   OPTION_VAL_DENDY_OC },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_OC,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_OC,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_OC,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_OC,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_OC },
         { "svideo",     OPTION_VAL_SVIDEO_OC },
         { "rgb",        OPTION_VAL_RGB_OC },
         { "monochrome", OPTION_VAL_MONOCHROME_OC },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_OC,
      NULL,
      NESTOPIA_PALETTE_INFO_0_OC,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_OC },
         { "consumer",             OPTION_VAL_CONSUMER_OC },
         { "canonical",            OPTION_VAL_CANONICAL_OC },
         { "alternative",          OPTION_VAL_ALTERNATIVE_OC },
         { "rgb",                  OPTION_VAL_RGB_O3_OC },
         { "pal",                  OPTION_VAL_PAL_OC },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_OC },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_OC },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_OC },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_OC },
         { "raw",                  OPTION_VAL_RAW_OC },
         { "custom",               OPTION_VAL_CUSTOM_OC },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_OC,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_OC,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_OC,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_OC,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_OC,
      NULL,
      NESTOPIA_ASPECT_INFO_0_OC,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_OC },
         { "ntsc",        OPTION_VAL_NTSC_OC },
         { "pal",         OPTION_VAL_PAL_OC },
         { "4:3",         OPTION_VAL_4_3_OC },
         { "uncorrected", OPTION_VAL_UNCORRECTED_OC },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_OC,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_OC,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_OC,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_OC,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_OC,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_OC,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_OC,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_OC,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_OC,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_OC,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_OC,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_OC,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_OC,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_OC,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_OC,
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
      NESTOPIA_AUDIO_TYPE_LABEL_OC,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_OC,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_OC },
         { "stereo", OPTION_VAL_STEREO_OC },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_OC,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_OC,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_OC },
         { "ntsc",    OPTION_VAL_NTSC_OC },
         { "famicom", OPTION_VAL_FAMICOM_OC },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_OC,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_OC, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_OC,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_OC,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_OC },
         { "pointer", OPTION_VAL_POINTER_OC },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_OC,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_OC,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_OC },
         { "mouse", OPTION_VAL_MOUSE_OC },
         { "pointer", OPTION_VAL_POINTER_OC },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_OC,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_OC,
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
      NESTOPIA_TURBO_PULSE_LABEL_OC,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_OC,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_OC,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_OC,
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
      NESTOPIA_OVERCLOCK_LABEL_OC,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_OC,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_OC },
         { "2x", OPTION_VAL_2X_OC },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_OC,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_OC,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_OC },
         { "0xFF",   OPTION_VAL_0XFF_OC },
         { "random", OPTION_VAL_RANDOM_OC },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_oc = {
   option_cats_oc,
   option_defs_oc
};

/* RETRO_LANGUAGE_PL */

#define CATEGORY_SYSTEM_LABEL_PL NULL
#define CATEGORY_SYSTEM_INFO_0_PL NULL
#define CATEGORY_VIDEO_LABEL_PL "Wideo"
#define CATEGORY_VIDEO_INFO_0_PL "Zmień proporcje aspektów, wyświetlanie przycinania, palety kolorów i ustawienia filtrów wideo."
#define CATEGORY_AUDIO_LABEL_PL NULL
#define CATEGORY_AUDIO_INFO_0_PL "Zmień efekty dźwiękowe i głośność kanałów."
#define CATEGORY_INPUT_LABEL_PL "Wejście"
#define CATEGORY_INPUT_INFO_0_PL "Zmień urządzenia wejściowe i inne ustawienia związane z wprowadzaniem."
#define CATEGORY_HACKS_LABEL_PL "Hacki emulacyjne"
#define CATEGORY_HACKS_INFO_0_PL "Zmiana ustawień przetaktowania procesora i dokładności emulacji wpływająca na niską wydajność i kompatybilność."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_PL "Region systemowy"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_PL NULL
#define OPTION_VAL_AUTO_PL NULL
#define OPTION_VAL_NTSC_PL NULL
#define OPTION_VAL_PAL_PL NULL
#define OPTION_VAL_FAMICOM_PL NULL
#define OPTION_VAL_DENDY_PL NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_PL NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_PL NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_PL "Duży filtr NTSC"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_PL NULL
#define OPTION_VAL_COMPOSITE_PL NULL
#define OPTION_VAL_SVIDEO_PL NULL
#define OPTION_VAL_RGB_PL NULL
#define OPTION_VAL_MONOCHROME_PL "Monochromia"
#define NESTOPIA_PALETTE_LABEL_PL "Paleta"
#define NESTOPIA_PALETTE_INFO_0_PL NULL
#define OPTION_VAL_CXA2025AS_PL NULL
#define OPTION_VAL_CONSUMER_PL NULL
#define OPTION_VAL_CANONICAL_PL NULL
#define OPTION_VAL_ALTERNATIVE_PL "Alternatywa"
#define OPTION_VAL_RGB_O3_PL NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_PL NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_PL NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_PL NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_PL "NES Classic FBx (naprawiony)"
#define OPTION_VAL_RAW_PL NULL
#define OPTION_VAL_CUSTOM_PL "Własny"
#define NESTOPIA_OVERSCAN_V_LABEL_PL NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_PL NULL
#define NESTOPIA_OVERSCAN_H_LABEL_PL NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_PL NULL
#define NESTOPIA_ASPECT_LABEL_PL NULL
#define NESTOPIA_ASPECT_INFO_0_PL NULL
#define OPTION_VAL_4_3_PL NULL
#define OPTION_VAL_UNCORRECTED_PL "Niepoprawione"
#define NESTOPIA_GENIE_DISTORTION_LABEL_PL NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_PL NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_PL NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_PL NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_PL NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_PL NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_PL NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_PL NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_PL NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_PL NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_PL NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_PL NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_PL NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_PL NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_PL NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_PL NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_PL NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_PL NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_PL NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_PL NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_PL NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_PL NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_PL NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_PL NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_PL NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_PL NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_PL NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_PL NULL
#define OPTION_VAL_MONO_PL NULL
#define OPTION_VAL_STEREO_PL NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_PL "Adapter dla 4 graczy"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_PL NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_PL NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_PL "Obróć przyciski A/B/X/Y zgodnie z ruchem wskazówek zegara."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_PL NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_PL NULL
#define OPTION_VAL_MOUSE_PL NULL
#define OPTION_VAL_POINTER_PL NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_PL NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_PL NULL
#define OPTION_VAL_LIGHTGUN_PL NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_PL NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_PL NULL
#define NESTOPIA_TURBO_PULSE_LABEL_PL NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_PL NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_PL "Usuń limit sprite'ów/duchów"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_PL "Usuń limit sprzętowy 8-duszków-na-skanlinie."
#define NESTOPIA_OVERCLOCK_LABEL_PL "Prędkość procesora (Podkręcenie)"
#define NESTOPIA_OVERCLOCK_INFO_0_PL "Podkręcanie emulowanego CPU."
#define OPTION_VAL_1X_PL NULL
#define OPTION_VAL_2X_PL NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_PL NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_PL NULL
#define OPTION_VAL_0X00_PL NULL
#define OPTION_VAL_0XFF_PL NULL
#define OPTION_VAL_RANDOM_PL NULL

struct retro_core_option_v2_category option_cats_pl[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_PL,
      CATEGORY_SYSTEM_INFO_0_PL
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_PL,
      CATEGORY_VIDEO_INFO_0_PL
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_PL,
      CATEGORY_AUDIO_INFO_0_PL
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_PL,
      CATEGORY_INPUT_INFO_0_PL
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_PL,
      CATEGORY_HACKS_INFO_0_PL
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_pl[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_PL,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_PL,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_PL },
         { "ntsc",    OPTION_VAL_NTSC_PL },
         { "pal",     OPTION_VAL_PAL_PL },
         { "famicom", OPTION_VAL_FAMICOM_PL },
         { "dendy",   OPTION_VAL_DENDY_PL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_PL,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_PL,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_PL,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_PL,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_PL },
         { "svideo",     OPTION_VAL_SVIDEO_PL },
         { "rgb",        OPTION_VAL_RGB_PL },
         { "monochrome", OPTION_VAL_MONOCHROME_PL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_PL,
      NULL,
      NESTOPIA_PALETTE_INFO_0_PL,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_PL },
         { "consumer",             OPTION_VAL_CONSUMER_PL },
         { "canonical",            OPTION_VAL_CANONICAL_PL },
         { "alternative",          OPTION_VAL_ALTERNATIVE_PL },
         { "rgb",                  OPTION_VAL_RGB_O3_PL },
         { "pal",                  OPTION_VAL_PAL_PL },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_PL },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_PL },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_PL },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_PL },
         { "raw",                  OPTION_VAL_RAW_PL },
         { "custom",               OPTION_VAL_CUSTOM_PL },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_PL,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_PL,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_PL,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_PL,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_PL,
      NULL,
      NESTOPIA_ASPECT_INFO_0_PL,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_PL },
         { "ntsc",        OPTION_VAL_NTSC_PL },
         { "pal",         OPTION_VAL_PAL_PL },
         { "4:3",         OPTION_VAL_4_3_PL },
         { "uncorrected", OPTION_VAL_UNCORRECTED_PL },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_PL,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_PL,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_PL,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_PL,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_PL,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_PL,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_PL,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_PL,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_PL,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_PL,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_PL,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_PL,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_PL,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_PL,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_PL,
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
      NESTOPIA_AUDIO_TYPE_LABEL_PL,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_PL,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_PL },
         { "stereo", OPTION_VAL_STEREO_PL },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_PL,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_PL,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_PL },
         { "ntsc",    OPTION_VAL_NTSC_PL },
         { "famicom", OPTION_VAL_FAMICOM_PL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_PL,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_PL, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_PL,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_PL,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_PL },
         { "pointer", OPTION_VAL_POINTER_PL },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_PL,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_PL,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_PL },
         { "mouse", OPTION_VAL_MOUSE_PL },
         { "pointer", OPTION_VAL_POINTER_PL },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_PL,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_PL,
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
      NESTOPIA_TURBO_PULSE_LABEL_PL,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_PL,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_PL,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_PL,
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
      NESTOPIA_OVERCLOCK_LABEL_PL,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_PL,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_PL },
         { "2x", OPTION_VAL_2X_PL },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_PL,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_PL,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_PL },
         { "0xFF",   OPTION_VAL_0XFF_PL },
         { "random", OPTION_VAL_RANDOM_PL },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_pl = {
   option_cats_pl,
   option_defs_pl
};

/* RETRO_LANGUAGE_PT_BR */

#define CATEGORY_SYSTEM_LABEL_PT_BR "Sistema"
#define CATEGORY_SYSTEM_INFO_0_PT_BR NULL
#define CATEGORY_VIDEO_LABEL_PT_BR "Vídeo"
#define CATEGORY_VIDEO_INFO_0_PT_BR "Altera as configurações de proporção da tela, corte da tela, paleta de cores e filtro de vídeo."
#define CATEGORY_AUDIO_LABEL_PT_BR "Áudio"
#define CATEGORY_AUDIO_INFO_0_PT_BR "Altera os efeitos de áudio e os volumes do canal."
#define CATEGORY_INPUT_LABEL_PT_BR "Entrada"
#define CATEGORY_INPUT_INFO_0_PT_BR "Altera os dispositivos de entrada e as outras configurações relacionadas com a entrada."
#define CATEGORY_HACKS_LABEL_PT_BR "Hacks de emulação"
#define CATEGORY_HACKS_INFO_0_PT_BR "Seleciona as configurações de precisão do overclock e a emulação do processador que afetam o baixo desempenho e a compatibilidade."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_PT_BR "Região do sistema"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_PT_BR "Auto' usará o banco de dados do arquivo NstDatabase.xml para fazer a detecção da região. Quando não houver um banco de dados presente, o padrão será o NTSC."
#define OPTION_VAL_AUTO_PT_BR "Automático"
#define OPTION_VAL_NTSC_PT_BR NULL
#define OPTION_VAL_PAL_PT_BR NULL
#define OPTION_VAL_FAMICOM_PT_BR NULL
#define OPTION_VAL_DENDY_PT_BR "Pendência"
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_PT_BR "Inserção automática do FDS"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_PT_BR "Insere o primeiro disco FDS automaticamente durante a redefinição."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_PT_BR "Filtro Blargg NTSC"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_PT_BR "Ativa os filtros blargg NTSC."
#define OPTION_VAL_COMPOSITE_PT_BR "Vídeo composto"
#define OPTION_VAL_SVIDEO_PT_BR NULL
#define OPTION_VAL_RGB_PT_BR NULL
#define OPTION_VAL_MONOCHROME_PT_BR "Monocromático"
#define NESTOPIA_PALETTE_LABEL_PT_BR "Paleta de cores"
#define NESTOPIA_PALETTE_INFO_0_PT_BR NULL
#define OPTION_VAL_CXA2025AS_PT_BR NULL
#define OPTION_VAL_CONSUMER_PT_BR "Consumidor"
#define OPTION_VAL_CANONICAL_PT_BR "Canônico"
#define OPTION_VAL_ALTERNATIVE_PT_BR "Alternativo"
#define OPTION_VAL_RGB_O3_PT_BR NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_PT_BR "FBx direto composto"
#define OPTION_VAL_PVM_STYLE_D93_FBX_PT_BR "FBx no estilo PVM D93"
#define OPTION_VAL_NTSC_HARDWARE_FBX_PT_BR NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_PT_BR "NES-Classic FBX (corrigido)"
#define OPTION_VAL_RAW_PT_BR "Bruto"
#define OPTION_VAL_CUSTOM_PT_BR "Personalizado"
#define NESTOPIA_OVERSCAN_V_LABEL_PT_BR "Máscara do transbordamento da varredura (Vertical)"
#define NESTOPIA_OVERSCAN_V_INFO_0_PT_BR "Mascara (verticalmente) a saída potencialmente aleatória do vídeo que seria escondida pelo bezel em torno da borda de uma tela de televisão com definição padrão."
#define NESTOPIA_OVERSCAN_H_LABEL_PT_BR "Máscara do transbordamento da varredura (Horizontal)"
#define NESTOPIA_OVERSCAN_H_INFO_0_PT_BR "Mascara (horizontalmente) a saída potencialmente aleatória do vídeo que seria escondida pelo bezel em torno da borda de uma tela de televisão com definição padrão."
#define NESTOPIA_ASPECT_LABEL_PT_BR "Proporção de tela preferida"
#define NESTOPIA_ASPECT_INFO_0_PT_BR NULL
#define OPTION_VAL_4_3_PT_BR NULL
#define OPTION_VAL_UNCORRECTED_PT_BR "Não corrigida"
#define NESTOPIA_GENIE_DISTORTION_LABEL_PT_BR "Distorção de áudio do Game Genie"
#define NESTOPIA_GENIE_DISTORTION_INFO_0_PT_BR "O dispositivo de trapaça Game Genie poderia inadvertidamente introduzir uma distorção no áudio dos jogos. Ativando esta opção, você pode simular a distorção que ele adicionaria ao áudio do jogo."
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_PT_BR "Mostrar configurações avançadas de áudio (reabrir menu)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_PT_BR "Habilita o ajuste dos parâmetros de baixo nível do canal de áudio."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_PT_BR "Volume do canal de ondas quadradas 1 (%)"
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_PT_BR "Modifica o volume do canal de ondas quadradas 1 (em porcentagem)."
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_PT_BR "Volume do canal de ondas quadradas 2 (%)"
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_PT_BR "Modifica o volume do canal de ondas quadradas 2 (em porcentagem)."
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_PT_BR "Volume do canal de ondas triangulares (%)"
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_PT_BR "Modifica o volume do canal de ondas triangulares (em porcentagem)."
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_PT_BR "Volume do canal de ruído (%)"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_PT_BR "Muda o volume do canal de ruído (em porcentagem)."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_PT_BR "Volume do canal DPCM (%)"
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_PT_BR "Modifica o volume do canal DPCM (em porcentagem)."
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_PT_BR "Volume do canal FDS (%)"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_PT_BR "Modifica o volume do canal FDS (em porcentagem)."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_PT_BR "Volume do canal MMC5 (%)"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_PT_BR "Modifica o volume do canal MMC5 (em porcentagem)."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_PT_BR "Volume do canal VRC6 (%)"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_PT_BR "Modifica o volume do canal VRC6 (em porcentagem)."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_PT_BR "Volume do canal VRC7 (%)"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_PT_BR "Modifica o volume do canal VRC7 (em porcentagem)."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_PT_BR "Volume do canal N163 (%)"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_PT_BR "Modifica o volume do canal N163 (em porcentagem)."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_PT_BR "Volume do canal S5B (%)"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_PT_BR NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_PT_BR NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_PT_BR NULL
#define OPTION_VAL_MONO_PT_BR NULL
#define OPTION_VAL_STEREO_PT_BR NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_PT_BR "Adaptador para 4 jogadores"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_PT_BR "Se necessário, selecione manualmente um adaptador para 4 jogadores. Alguns jogos não reconhecerão o adaptador corretamente através do banco de dados NstDatabase.xml, esta opção deve ajudar a corrigir isso."
#define NESTOPIA_BUTTON_SHIFT_LABEL_PT_BR "Girar os botões no sentido horário"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_PT_BR "Gira os botões A/B/X/Y no sentido horário."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_PT_BR "Dispositivo Arkanoid"
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_PT_BR "Seleciona o dispositivo que você deseja usar para a palheta Arkanoid."
#define OPTION_VAL_MOUSE_PT_BR NULL
#define OPTION_VAL_POINTER_PT_BR "Ponteiro"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_PT_BR "Dispositivo Zapper"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_PT_BR "Selecione o dispositivo que deseja usar para o Zapper."
#define OPTION_VAL_LIGHTGUN_PT_BR "Pistola de luz"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_PT_BR "Mostrar mira"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_PT_BR "Define se deve mostrar a mira quando o Zapper é usado."
#define NESTOPIA_TURBO_PULSE_LABEL_PT_BR "Velocidade dos botões turbo"
#define NESTOPIA_TURBO_PULSE_INFO_0_PT_BR "Define a velocidade do pressionamento dos botões Turbo B e Turbo A."
#define NESTOPIA_NOSPRITELIMIT_LABEL_PT_BR "Remove o limite do sprite"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_PT_BR "Remove o limite do hardware com 8 sprites por linhas de escaneamento."
#define NESTOPIA_OVERCLOCK_LABEL_PT_BR "Velocidade da CPU (Overclock)"
#define NESTOPIA_OVERCLOCK_INFO_0_PT_BR "Faz um overclock na CPU emulada."
#define OPTION_VAL_1X_PT_BR NULL
#define OPTION_VAL_2X_PT_BR NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_PT_BR "Condição da RAM ao ligar"
#define NESTOPIA_RAM_POWER_STATE_INFO_0_PT_BR "Os valores da RAM ao ligar. Alguns jogos dependem de valores iniciais da RAM para a geração de números aleatórios."
#define OPTION_VAL_0X00_PT_BR NULL
#define OPTION_VAL_0XFF_PT_BR NULL
#define OPTION_VAL_RANDOM_PT_BR "Aleatório"

struct retro_core_option_v2_category option_cats_pt_br[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_PT_BR,
      CATEGORY_SYSTEM_INFO_0_PT_BR
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_PT_BR,
      CATEGORY_VIDEO_INFO_0_PT_BR
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_PT_BR,
      CATEGORY_AUDIO_INFO_0_PT_BR
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_PT_BR,
      CATEGORY_INPUT_INFO_0_PT_BR
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_PT_BR,
      CATEGORY_HACKS_INFO_0_PT_BR
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_pt_br[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_PT_BR,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_PT_BR,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_PT_BR },
         { "ntsc",    OPTION_VAL_NTSC_PT_BR },
         { "pal",     OPTION_VAL_PAL_PT_BR },
         { "famicom", OPTION_VAL_FAMICOM_PT_BR },
         { "dendy",   OPTION_VAL_DENDY_PT_BR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_PT_BR,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_PT_BR,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_PT_BR,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_PT_BR,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_PT_BR },
         { "svideo",     OPTION_VAL_SVIDEO_PT_BR },
         { "rgb",        OPTION_VAL_RGB_PT_BR },
         { "monochrome", OPTION_VAL_MONOCHROME_PT_BR },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_PT_BR,
      NULL,
      NESTOPIA_PALETTE_INFO_0_PT_BR,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_PT_BR },
         { "consumer",             OPTION_VAL_CONSUMER_PT_BR },
         { "canonical",            OPTION_VAL_CANONICAL_PT_BR },
         { "alternative",          OPTION_VAL_ALTERNATIVE_PT_BR },
         { "rgb",                  OPTION_VAL_RGB_O3_PT_BR },
         { "pal",                  OPTION_VAL_PAL_PT_BR },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_PT_BR },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_PT_BR },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_PT_BR },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_PT_BR },
         { "raw",                  OPTION_VAL_RAW_PT_BR },
         { "custom",               OPTION_VAL_CUSTOM_PT_BR },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_PT_BR,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_PT_BR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_PT_BR,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_PT_BR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_PT_BR,
      NULL,
      NESTOPIA_ASPECT_INFO_0_PT_BR,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_PT_BR },
         { "ntsc",        OPTION_VAL_NTSC_PT_BR },
         { "pal",         OPTION_VAL_PAL_PT_BR },
         { "4:3",         OPTION_VAL_4_3_PT_BR },
         { "uncorrected", OPTION_VAL_UNCORRECTED_PT_BR },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_PT_BR,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_PT_BR,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_PT_BR,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_PT_BR,
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
      NESTOPIA_AUDIO_TYPE_LABEL_PT_BR,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_PT_BR,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_PT_BR },
         { "stereo", OPTION_VAL_STEREO_PT_BR },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_PT_BR,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_PT_BR,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_PT_BR },
         { "ntsc",    OPTION_VAL_NTSC_PT_BR },
         { "famicom", OPTION_VAL_FAMICOM_PT_BR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_PT_BR,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_PT_BR, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_PT_BR,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_PT_BR,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_PT_BR },
         { "pointer", OPTION_VAL_POINTER_PT_BR },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_PT_BR,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_PT_BR,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_PT_BR },
         { "mouse", OPTION_VAL_MOUSE_PT_BR },
         { "pointer", OPTION_VAL_POINTER_PT_BR },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_PT_BR,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_PT_BR,
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
      NESTOPIA_TURBO_PULSE_LABEL_PT_BR,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_PT_BR,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_PT_BR,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_PT_BR,
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
      NESTOPIA_OVERCLOCK_LABEL_PT_BR,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_PT_BR,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_PT_BR },
         { "2x", OPTION_VAL_2X_PT_BR },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_PT_BR,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_PT_BR,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_PT_BR },
         { "0xFF",   OPTION_VAL_0XFF_PT_BR },
         { "random", OPTION_VAL_RANDOM_PT_BR },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_pt_br = {
   option_cats_pt_br,
   option_defs_pt_br
};

/* RETRO_LANGUAGE_PT_PT */

#define CATEGORY_SYSTEM_LABEL_PT_PT NULL
#define CATEGORY_SYSTEM_INFO_0_PT_PT NULL
#define CATEGORY_VIDEO_LABEL_PT_PT "Vídeo"
#define CATEGORY_VIDEO_INFO_0_PT_PT NULL
#define CATEGORY_AUDIO_LABEL_PT_PT "Áudio"
#define CATEGORY_AUDIO_INFO_0_PT_PT NULL
#define CATEGORY_INPUT_LABEL_PT_PT "Entrada"
#define CATEGORY_INPUT_INFO_0_PT_PT NULL
#define CATEGORY_HACKS_LABEL_PT_PT NULL
#define CATEGORY_HACKS_INFO_0_PT_PT NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_PT_PT NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_PT_PT NULL
#define OPTION_VAL_AUTO_PT_PT "Automático"
#define OPTION_VAL_NTSC_PT_PT NULL
#define OPTION_VAL_PAL_PT_PT NULL
#define OPTION_VAL_FAMICOM_PT_PT NULL
#define OPTION_VAL_DENDY_PT_PT NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_PT_PT NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_PT_PT NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_PT_PT NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_PT_PT NULL
#define OPTION_VAL_COMPOSITE_PT_PT NULL
#define OPTION_VAL_SVIDEO_PT_PT NULL
#define OPTION_VAL_RGB_PT_PT NULL
#define OPTION_VAL_MONOCHROME_PT_PT "Monocromático"
#define NESTOPIA_PALETTE_LABEL_PT_PT NULL
#define NESTOPIA_PALETTE_INFO_0_PT_PT NULL
#define OPTION_VAL_CXA2025AS_PT_PT NULL
#define OPTION_VAL_CONSUMER_PT_PT NULL
#define OPTION_VAL_CANONICAL_PT_PT NULL
#define OPTION_VAL_ALTERNATIVE_PT_PT NULL
#define OPTION_VAL_RGB_O3_PT_PT NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_PT_PT NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_PT_PT NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_PT_PT NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_PT_PT NULL
#define OPTION_VAL_RAW_PT_PT NULL
#define OPTION_VAL_CUSTOM_PT_PT "Personalizar"
#define NESTOPIA_OVERSCAN_V_LABEL_PT_PT NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_PT_PT NULL
#define NESTOPIA_OVERSCAN_H_LABEL_PT_PT NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_PT_PT NULL
#define NESTOPIA_ASPECT_LABEL_PT_PT NULL
#define NESTOPIA_ASPECT_INFO_0_PT_PT NULL
#define OPTION_VAL_4_3_PT_PT NULL
#define OPTION_VAL_UNCORRECTED_PT_PT NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_PT_PT NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_PT_PT NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_PT_PT NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_PT_PT NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_PT_PT NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_PT_PT NULL
#define OPTION_VAL_MONO_PT_PT NULL
#define OPTION_VAL_STEREO_PT_PT NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_PT_PT NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_PT_PT NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_PT_PT NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_PT_PT NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_PT_PT NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_PT_PT NULL
#define OPTION_VAL_MOUSE_PT_PT NULL
#define OPTION_VAL_POINTER_PT_PT NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_PT_PT NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_PT_PT NULL
#define OPTION_VAL_LIGHTGUN_PT_PT NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_PT_PT NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_PT_PT NULL
#define NESTOPIA_TURBO_PULSE_LABEL_PT_PT NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_PT_PT NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_PT_PT NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_PT_PT NULL
#define NESTOPIA_OVERCLOCK_LABEL_PT_PT NULL
#define NESTOPIA_OVERCLOCK_INFO_0_PT_PT NULL
#define OPTION_VAL_1X_PT_PT NULL
#define OPTION_VAL_2X_PT_PT NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_PT_PT NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_PT_PT NULL
#define OPTION_VAL_0X00_PT_PT NULL
#define OPTION_VAL_0XFF_PT_PT NULL
#define OPTION_VAL_RANDOM_PT_PT NULL

struct retro_core_option_v2_category option_cats_pt_pt[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_PT_PT,
      CATEGORY_SYSTEM_INFO_0_PT_PT
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_PT_PT,
      CATEGORY_VIDEO_INFO_0_PT_PT
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_PT_PT,
      CATEGORY_AUDIO_INFO_0_PT_PT
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_PT_PT,
      CATEGORY_INPUT_INFO_0_PT_PT
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_PT_PT,
      CATEGORY_HACKS_INFO_0_PT_PT
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_pt_pt[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_PT_PT,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_PT_PT,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_PT_PT },
         { "ntsc",    OPTION_VAL_NTSC_PT_PT },
         { "pal",     OPTION_VAL_PAL_PT_PT },
         { "famicom", OPTION_VAL_FAMICOM_PT_PT },
         { "dendy",   OPTION_VAL_DENDY_PT_PT },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_PT_PT,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_PT_PT,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_PT_PT,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_PT_PT,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_PT_PT },
         { "svideo",     OPTION_VAL_SVIDEO_PT_PT },
         { "rgb",        OPTION_VAL_RGB_PT_PT },
         { "monochrome", OPTION_VAL_MONOCHROME_PT_PT },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_PT_PT,
      NULL,
      NESTOPIA_PALETTE_INFO_0_PT_PT,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_PT_PT },
         { "consumer",             OPTION_VAL_CONSUMER_PT_PT },
         { "canonical",            OPTION_VAL_CANONICAL_PT_PT },
         { "alternative",          OPTION_VAL_ALTERNATIVE_PT_PT },
         { "rgb",                  OPTION_VAL_RGB_O3_PT_PT },
         { "pal",                  OPTION_VAL_PAL_PT_PT },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_PT_PT },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_PT_PT },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_PT_PT },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_PT_PT },
         { "raw",                  OPTION_VAL_RAW_PT_PT },
         { "custom",               OPTION_VAL_CUSTOM_PT_PT },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_PT_PT,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_PT_PT,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_PT_PT,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_PT_PT,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_PT_PT,
      NULL,
      NESTOPIA_ASPECT_INFO_0_PT_PT,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_PT_PT },
         { "ntsc",        OPTION_VAL_NTSC_PT_PT },
         { "pal",         OPTION_VAL_PAL_PT_PT },
         { "4:3",         OPTION_VAL_4_3_PT_PT },
         { "uncorrected", OPTION_VAL_UNCORRECTED_PT_PT },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_PT_PT,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_PT_PT,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_PT_PT,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_PT_PT,
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
      NESTOPIA_AUDIO_TYPE_LABEL_PT_PT,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_PT_PT,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_PT_PT },
         { "stereo", OPTION_VAL_STEREO_PT_PT },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_PT_PT,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_PT_PT,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_PT_PT },
         { "ntsc",    OPTION_VAL_NTSC_PT_PT },
         { "famicom", OPTION_VAL_FAMICOM_PT_PT },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_PT_PT,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_PT_PT, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_PT_PT,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_PT_PT,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_PT_PT },
         { "pointer", OPTION_VAL_POINTER_PT_PT },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_PT_PT,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_PT_PT,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_PT_PT },
         { "mouse", OPTION_VAL_MOUSE_PT_PT },
         { "pointer", OPTION_VAL_POINTER_PT_PT },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_PT_PT,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_PT_PT,
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
      NESTOPIA_TURBO_PULSE_LABEL_PT_PT,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_PT_PT,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_PT_PT,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_PT_PT,
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
      NESTOPIA_OVERCLOCK_LABEL_PT_PT,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_PT_PT,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_PT_PT },
         { "2x", OPTION_VAL_2X_PT_PT },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_PT_PT,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_PT_PT,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_PT_PT },
         { "0xFF",   OPTION_VAL_0XFF_PT_PT },
         { "random", OPTION_VAL_RANDOM_PT_PT },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_pt_pt = {
   option_cats_pt_pt,
   option_defs_pt_pt
};

/* RETRO_LANGUAGE_RO */

#define CATEGORY_SYSTEM_LABEL_RO NULL
#define CATEGORY_SYSTEM_INFO_0_RO NULL
#define CATEGORY_VIDEO_LABEL_RO NULL
#define CATEGORY_VIDEO_INFO_0_RO NULL
#define CATEGORY_AUDIO_LABEL_RO NULL
#define CATEGORY_AUDIO_INFO_0_RO NULL
#define CATEGORY_INPUT_LABEL_RO NULL
#define CATEGORY_INPUT_INFO_0_RO NULL
#define CATEGORY_HACKS_LABEL_RO NULL
#define CATEGORY_HACKS_INFO_0_RO NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_RO NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_RO NULL
#define OPTION_VAL_AUTO_RO NULL
#define OPTION_VAL_NTSC_RO NULL
#define OPTION_VAL_PAL_RO NULL
#define OPTION_VAL_FAMICOM_RO NULL
#define OPTION_VAL_DENDY_RO NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_RO NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_RO NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_RO NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_RO NULL
#define OPTION_VAL_COMPOSITE_RO NULL
#define OPTION_VAL_SVIDEO_RO NULL
#define OPTION_VAL_RGB_RO NULL
#define OPTION_VAL_MONOCHROME_RO NULL
#define NESTOPIA_PALETTE_LABEL_RO NULL
#define NESTOPIA_PALETTE_INFO_0_RO NULL
#define OPTION_VAL_CXA2025AS_RO NULL
#define OPTION_VAL_CONSUMER_RO NULL
#define OPTION_VAL_CANONICAL_RO NULL
#define OPTION_VAL_ALTERNATIVE_RO NULL
#define OPTION_VAL_RGB_O3_RO NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_RO NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_RO NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_RO NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_RO NULL
#define OPTION_VAL_RAW_RO NULL
#define OPTION_VAL_CUSTOM_RO NULL
#define NESTOPIA_OVERSCAN_V_LABEL_RO NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_RO NULL
#define NESTOPIA_OVERSCAN_H_LABEL_RO NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_RO NULL
#define NESTOPIA_ASPECT_LABEL_RO NULL
#define NESTOPIA_ASPECT_INFO_0_RO NULL
#define OPTION_VAL_4_3_RO NULL
#define OPTION_VAL_UNCORRECTED_RO NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_RO NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_RO NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_RO NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_RO NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_RO NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_RO NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_RO NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_RO NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_RO NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_RO NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_RO NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_RO NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_RO NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_RO NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_RO NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_RO NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_RO NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_RO NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_RO NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_RO NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_RO NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_RO NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_RO NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_RO NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_RO NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_RO NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_RO NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_RO NULL
#define OPTION_VAL_MONO_RO NULL
#define OPTION_VAL_STEREO_RO NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_RO NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_RO NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_RO NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_RO NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_RO NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_RO NULL
#define OPTION_VAL_MOUSE_RO NULL
#define OPTION_VAL_POINTER_RO NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_RO NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_RO NULL
#define OPTION_VAL_LIGHTGUN_RO NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_RO NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_RO NULL
#define NESTOPIA_TURBO_PULSE_LABEL_RO NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_RO NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_RO NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_RO NULL
#define NESTOPIA_OVERCLOCK_LABEL_RO NULL
#define NESTOPIA_OVERCLOCK_INFO_0_RO NULL
#define OPTION_VAL_1X_RO NULL
#define OPTION_VAL_2X_RO NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_RO NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_RO NULL
#define OPTION_VAL_0X00_RO NULL
#define OPTION_VAL_0XFF_RO NULL
#define OPTION_VAL_RANDOM_RO NULL

struct retro_core_option_v2_category option_cats_ro[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_RO,
      CATEGORY_SYSTEM_INFO_0_RO
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_RO,
      CATEGORY_VIDEO_INFO_0_RO
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_RO,
      CATEGORY_AUDIO_INFO_0_RO
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_RO,
      CATEGORY_INPUT_INFO_0_RO
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_RO,
      CATEGORY_HACKS_INFO_0_RO
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ro[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_RO,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_RO,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_RO },
         { "ntsc",    OPTION_VAL_NTSC_RO },
         { "pal",     OPTION_VAL_PAL_RO },
         { "famicom", OPTION_VAL_FAMICOM_RO },
         { "dendy",   OPTION_VAL_DENDY_RO },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_RO,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_RO,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_RO,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_RO,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_RO },
         { "svideo",     OPTION_VAL_SVIDEO_RO },
         { "rgb",        OPTION_VAL_RGB_RO },
         { "monochrome", OPTION_VAL_MONOCHROME_RO },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_RO,
      NULL,
      NESTOPIA_PALETTE_INFO_0_RO,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_RO },
         { "consumer",             OPTION_VAL_CONSUMER_RO },
         { "canonical",            OPTION_VAL_CANONICAL_RO },
         { "alternative",          OPTION_VAL_ALTERNATIVE_RO },
         { "rgb",                  OPTION_VAL_RGB_O3_RO },
         { "pal",                  OPTION_VAL_PAL_RO },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_RO },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_RO },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_RO },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_RO },
         { "raw",                  OPTION_VAL_RAW_RO },
         { "custom",               OPTION_VAL_CUSTOM_RO },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_RO,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_RO,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_RO,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_RO,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_RO,
      NULL,
      NESTOPIA_ASPECT_INFO_0_RO,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_RO },
         { "ntsc",        OPTION_VAL_NTSC_RO },
         { "pal",         OPTION_VAL_PAL_RO },
         { "4:3",         OPTION_VAL_4_3_RO },
         { "uncorrected", OPTION_VAL_UNCORRECTED_RO },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_RO,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_RO,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_RO,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_RO,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_RO,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_RO,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_RO,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_RO,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_RO,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_RO,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_RO,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_RO,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_RO,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_RO,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_RO,
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
      NESTOPIA_AUDIO_TYPE_LABEL_RO,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_RO,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_RO },
         { "stereo", OPTION_VAL_STEREO_RO },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_RO,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_RO,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_RO },
         { "ntsc",    OPTION_VAL_NTSC_RO },
         { "famicom", OPTION_VAL_FAMICOM_RO },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_RO,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_RO, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_RO,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_RO,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_RO },
         { "pointer", OPTION_VAL_POINTER_RO },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_RO,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_RO,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_RO },
         { "mouse", OPTION_VAL_MOUSE_RO },
         { "pointer", OPTION_VAL_POINTER_RO },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_RO,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_RO,
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
      NESTOPIA_TURBO_PULSE_LABEL_RO,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_RO,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_RO,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_RO,
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
      NESTOPIA_OVERCLOCK_LABEL_RO,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_RO,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_RO },
         { "2x", OPTION_VAL_2X_RO },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_RO,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_RO,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_RO },
         { "0xFF",   OPTION_VAL_0XFF_RO },
         { "random", OPTION_VAL_RANDOM_RO },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ro = {
   option_cats_ro,
   option_defs_ro
};

/* RETRO_LANGUAGE_RU */

#define CATEGORY_SYSTEM_LABEL_RU "Системные"
#define CATEGORY_SYSTEM_INFO_0_RU NULL
#define CATEGORY_VIDEO_LABEL_RU "Изображение"
#define CATEGORY_VIDEO_INFO_0_RU "Настройки соотношения сторон, кадрирования, цветовой палитры и фильтрации изображения."
#define CATEGORY_AUDIO_LABEL_RU "Звук"
#define CATEGORY_AUDIO_INFO_0_RU "Настройки звуковых эффектов и громкости каналов."
#define CATEGORY_INPUT_LABEL_RU "Ввод"
#define CATEGORY_INPUT_INFO_0_RU "Изменение устройств ввода и связанных настроек."
#define CATEGORY_HACKS_LABEL_RU "Хаки эмуляции"
#define CATEGORY_HACKS_INFO_0_RU "Настройка разгона процессора и параметров точности эмуляции, влияющих на производительность и совместимость."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_RU "Регион системы"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_RU "'Авто' будет использовать файл базы данных NstDatabase.xml для автоопределения соотношения сторон. При отсутствии базы данных, по умолчанию будет установлен режим NTSC."
#define OPTION_VAL_AUTO_RU "Авто"
#define OPTION_VAL_NTSC_RU NULL
#define OPTION_VAL_PAL_RU NULL
#define OPTION_VAL_FAMICOM_RU NULL
#define OPTION_VAL_DENDY_RU NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_RU "Автовставка FDS"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_RU "Автоматически вставлять первый диск FDS при сбросе."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_RU "NTSC-фильтр Blargg'а"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_RU "Включает NTSC-фильтры Blargg'а."
#define OPTION_VAL_COMPOSITE_RU "Композитное видео"
#define OPTION_VAL_SVIDEO_RU NULL
#define OPTION_VAL_RGB_RU NULL
#define OPTION_VAL_MONOCHROME_RU "Монохромный"
#define NESTOPIA_PALETTE_LABEL_RU "Палитра"
#define NESTOPIA_PALETTE_INFO_0_RU NULL
#define OPTION_VAL_CXA2025AS_RU NULL
#define OPTION_VAL_CONSUMER_RU NULL
#define OPTION_VAL_CANONICAL_RU "Каноничная"
#define OPTION_VAL_ALTERNATIVE_RU "Альтернативная"
#define OPTION_VAL_RGB_O3_RU NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_RU NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_RU NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_RU NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_RU NULL
#define OPTION_VAL_RAW_RU NULL
#define OPTION_VAL_CUSTOM_RU "Кастомная"
#define NESTOPIA_OVERSCAN_V_LABEL_RU "Скрыть вылеты развёртки (по вертикали)"
#define NESTOPIA_OVERSCAN_V_INFO_0_RU "Маскирует (по вертикали) область изображения с возможным искажением графики, которая в телевизорах стандартной чёткости обычно скрыта рамкой по краю экрана."
#define NESTOPIA_OVERSCAN_H_LABEL_RU "Скрыть вылеты развёртки (по горизонтали)"
#define NESTOPIA_OVERSCAN_H_INFO_0_RU "Маскирует (по горизонтали) область изображения с возможным искажением графики, которая в телевизорах стандартной чёткости обычно скрыта рамкой по краю экрана."
#define NESTOPIA_ASPECT_LABEL_RU "Предпочтительное соотношение сторон"
#define NESTOPIA_ASPECT_INFO_0_RU NULL
#define OPTION_VAL_4_3_RU NULL
#define OPTION_VAL_UNCORRECTED_RU "Нескорректированное"
#define NESTOPIA_GENIE_DISTORTION_LABEL_RU "Искажение звука Game Genie"
#define NESTOPIA_GENIE_DISTORTION_INFO_0_RU "Устройство взлома Game Genie могло вызывать в играх произвольное искажение звука. Включение опции позволяет имитировать искажение, добавляемое данным устройством к звуку игр."
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_RU "Показывать расширенные настройки звука (перезайти в меню)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_RU "Включает настройку низкоуровневых параметров звуковых каналов."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_RU "Громкость прямоугольного канала 1 %"
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_RU "Изменение громкости прямоугольного канала 1 %."
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_RU "Громкость прямоугольного канала 2 %"
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_RU "Изменение громкости прямоугольного канала 2 %."
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_RU "Громкость треугольного канала %"
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_RU "Изменение громкости треугольного канала %."
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_RU "Громкость шумового канала %"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_RU "Изменение громкости шумового канала %."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_RU "Громкость канала DPCM %"
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_RU "Изменение громкости канала DPCM %."
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_RU "Громкость канала FDS %"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_RU "Изменение громкости канала FDS %."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_RU "Громкость канала MMC5 %"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_RU "Изменение громкости канала MMC5 %."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_RU "Громкость канала VRC6 %"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_RU "Изменение громкости канала VRC6 %."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_RU "Громкость канала VRC7 %"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_RU "Изменение громкости канала VRC7 %."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_RU "Громкость канала N163 %"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_RU "Изменение громкости канала N163 %."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_RU "Громкость канала S5B %"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_RU NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_RU NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_RU NULL
#define OPTION_VAL_MONO_RU NULL
#define OPTION_VAL_STEREO_RU NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_RU "Адаптер для 4 игроков"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_RU "Ручной выбор адаптера для 4 игроков. Опция помогает в тех случаях, когда игра не определяет адаптер по базе NstDatabase.xml."
#define NESTOPIA_BUTTON_SHIFT_LABEL_RU "Поворот кнопок по часовой стрелке"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_RU "Поворачивает по часовой стрелке кнопки A/B/X/Y."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_RU "Устройство Arkanoid"
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_RU "Выбор устройства, используемого для Arkanoid Paddle."
#define OPTION_VAL_MOUSE_RU "Мышь"
#define OPTION_VAL_POINTER_RU "Указатель"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_RU "Устройство Zapper"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_RU "Выбор устройства, используемого для Zapper."
#define OPTION_VAL_LIGHTGUN_RU "Световой пистолет"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_RU "Показывать прицел"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_RU "Включает отображение прицела при использовании Zapper."
#define NESTOPIA_TURBO_PULSE_LABEL_RU "Скорость пульсации турбо"
#define NESTOPIA_TURBO_PULSE_INFO_0_RU "Установка скорости пульсации турбо для кнопок Тurbo B и Turbo A."
#define NESTOPIA_NOSPRITELIMIT_LABEL_RU "Снять ограничение спрайтов"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_RU "Убирает аппаратное ограничение в 8 спрайтов на линии развёртки."
#define NESTOPIA_OVERCLOCK_LABEL_RU "Скорость CPU (разгон)"
#define NESTOPIA_OVERCLOCK_INFO_0_RU "Разгоняет эмулируемый CPU."
#define OPTION_VAL_1X_RU NULL
#define OPTION_VAL_2X_RU NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_RU "Состояние RAM при включении"
#define NESTOPIA_RAM_POWER_STATE_INFO_0_RU "Значения RAM при включении. Некоторые игры используют начальные значения RAM для генерации случайных чисел."
#define OPTION_VAL_0X00_RU NULL
#define OPTION_VAL_0XFF_RU NULL
#define OPTION_VAL_RANDOM_RU "Случайно"

struct retro_core_option_v2_category option_cats_ru[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_RU,
      CATEGORY_SYSTEM_INFO_0_RU
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_RU,
      CATEGORY_VIDEO_INFO_0_RU
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_RU,
      CATEGORY_AUDIO_INFO_0_RU
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_RU,
      CATEGORY_INPUT_INFO_0_RU
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_RU,
      CATEGORY_HACKS_INFO_0_RU
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ru[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_RU,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_RU,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_RU },
         { "ntsc",    OPTION_VAL_NTSC_RU },
         { "pal",     OPTION_VAL_PAL_RU },
         { "famicom", OPTION_VAL_FAMICOM_RU },
         { "dendy",   OPTION_VAL_DENDY_RU },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_RU,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_RU,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_RU,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_RU,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_RU },
         { "svideo",     OPTION_VAL_SVIDEO_RU },
         { "rgb",        OPTION_VAL_RGB_RU },
         { "monochrome", OPTION_VAL_MONOCHROME_RU },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_RU,
      NULL,
      NESTOPIA_PALETTE_INFO_0_RU,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_RU },
         { "consumer",             OPTION_VAL_CONSUMER_RU },
         { "canonical",            OPTION_VAL_CANONICAL_RU },
         { "alternative",          OPTION_VAL_ALTERNATIVE_RU },
         { "rgb",                  OPTION_VAL_RGB_O3_RU },
         { "pal",                  OPTION_VAL_PAL_RU },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_RU },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_RU },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_RU },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_RU },
         { "raw",                  OPTION_VAL_RAW_RU },
         { "custom",               OPTION_VAL_CUSTOM_RU },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_RU,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_RU,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_RU,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_RU,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_RU,
      NULL,
      NESTOPIA_ASPECT_INFO_0_RU,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_RU },
         { "ntsc",        OPTION_VAL_NTSC_RU },
         { "pal",         OPTION_VAL_PAL_RU },
         { "4:3",         OPTION_VAL_4_3_RU },
         { "uncorrected", OPTION_VAL_UNCORRECTED_RU },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_RU,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_RU,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_RU,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_RU,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_RU,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_RU,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_RU,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_RU,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_RU,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_RU,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_RU,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_RU,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_RU,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_RU,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_RU,
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
      NESTOPIA_AUDIO_TYPE_LABEL_RU,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_RU,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_RU },
         { "stereo", OPTION_VAL_STEREO_RU },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_RU,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_RU,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_RU },
         { "ntsc",    OPTION_VAL_NTSC_RU },
         { "famicom", OPTION_VAL_FAMICOM_RU },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_RU,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_RU, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_RU,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_RU,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_RU },
         { "pointer", OPTION_VAL_POINTER_RU },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_RU,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_RU,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_RU },
         { "mouse", OPTION_VAL_MOUSE_RU },
         { "pointer", OPTION_VAL_POINTER_RU },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_RU,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_RU,
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
      NESTOPIA_TURBO_PULSE_LABEL_RU,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_RU,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_RU,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_RU,
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
      NESTOPIA_OVERCLOCK_LABEL_RU,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_RU,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_RU },
         { "2x", OPTION_VAL_2X_RU },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_RU,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_RU,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_RU },
         { "0xFF",   OPTION_VAL_0XFF_RU },
         { "random", OPTION_VAL_RANDOM_RU },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ru = {
   option_cats_ru,
   option_defs_ru
};

/* RETRO_LANGUAGE_SI */

#define CATEGORY_SYSTEM_LABEL_SI NULL
#define CATEGORY_SYSTEM_INFO_0_SI NULL
#define CATEGORY_VIDEO_LABEL_SI NULL
#define CATEGORY_VIDEO_INFO_0_SI NULL
#define CATEGORY_AUDIO_LABEL_SI NULL
#define CATEGORY_AUDIO_INFO_0_SI NULL
#define CATEGORY_INPUT_LABEL_SI NULL
#define CATEGORY_INPUT_INFO_0_SI NULL
#define CATEGORY_HACKS_LABEL_SI NULL
#define CATEGORY_HACKS_INFO_0_SI NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_SI NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_SI NULL
#define OPTION_VAL_AUTO_SI NULL
#define OPTION_VAL_NTSC_SI NULL
#define OPTION_VAL_PAL_SI NULL
#define OPTION_VAL_FAMICOM_SI NULL
#define OPTION_VAL_DENDY_SI NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_SI NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_SI NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_SI NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_SI NULL
#define OPTION_VAL_COMPOSITE_SI NULL
#define OPTION_VAL_SVIDEO_SI NULL
#define OPTION_VAL_RGB_SI NULL
#define OPTION_VAL_MONOCHROME_SI NULL
#define NESTOPIA_PALETTE_LABEL_SI NULL
#define NESTOPIA_PALETTE_INFO_0_SI NULL
#define OPTION_VAL_CXA2025AS_SI NULL
#define OPTION_VAL_CONSUMER_SI NULL
#define OPTION_VAL_CANONICAL_SI NULL
#define OPTION_VAL_ALTERNATIVE_SI NULL
#define OPTION_VAL_RGB_O3_SI NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_SI NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_SI NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_SI NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_SI NULL
#define OPTION_VAL_RAW_SI NULL
#define OPTION_VAL_CUSTOM_SI NULL
#define NESTOPIA_OVERSCAN_V_LABEL_SI NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_SI NULL
#define NESTOPIA_OVERSCAN_H_LABEL_SI NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_SI NULL
#define NESTOPIA_ASPECT_LABEL_SI NULL
#define NESTOPIA_ASPECT_INFO_0_SI NULL
#define OPTION_VAL_4_3_SI NULL
#define OPTION_VAL_UNCORRECTED_SI NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_SI NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_SI NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_SI NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_SI NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_SI NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_SI NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_SI NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_SI NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_SI NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_SI NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_SI NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_SI NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_SI NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_SI NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_SI NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_SI NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_SI NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_SI NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_SI NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_SI NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_SI NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_SI NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_SI NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_SI NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_SI NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_SI NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_SI NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_SI NULL
#define OPTION_VAL_MONO_SI NULL
#define OPTION_VAL_STEREO_SI NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_SI NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_SI NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_SI NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_SI NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_SI NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_SI NULL
#define OPTION_VAL_MOUSE_SI NULL
#define OPTION_VAL_POINTER_SI NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_SI NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_SI NULL
#define OPTION_VAL_LIGHTGUN_SI NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_SI NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_SI NULL
#define NESTOPIA_TURBO_PULSE_LABEL_SI NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_SI NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_SI NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_SI NULL
#define NESTOPIA_OVERCLOCK_LABEL_SI NULL
#define NESTOPIA_OVERCLOCK_INFO_0_SI NULL
#define OPTION_VAL_1X_SI NULL
#define OPTION_VAL_2X_SI NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_SI NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_SI NULL
#define OPTION_VAL_0X00_SI NULL
#define OPTION_VAL_0XFF_SI NULL
#define OPTION_VAL_RANDOM_SI NULL

struct retro_core_option_v2_category option_cats_si[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_SI,
      CATEGORY_SYSTEM_INFO_0_SI
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_SI,
      CATEGORY_VIDEO_INFO_0_SI
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_SI,
      CATEGORY_AUDIO_INFO_0_SI
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_SI,
      CATEGORY_INPUT_INFO_0_SI
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_SI,
      CATEGORY_HACKS_INFO_0_SI
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_si[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_SI,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_SI,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_SI },
         { "ntsc",    OPTION_VAL_NTSC_SI },
         { "pal",     OPTION_VAL_PAL_SI },
         { "famicom", OPTION_VAL_FAMICOM_SI },
         { "dendy",   OPTION_VAL_DENDY_SI },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_SI,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_SI,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_SI,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_SI,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_SI },
         { "svideo",     OPTION_VAL_SVIDEO_SI },
         { "rgb",        OPTION_VAL_RGB_SI },
         { "monochrome", OPTION_VAL_MONOCHROME_SI },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_SI,
      NULL,
      NESTOPIA_PALETTE_INFO_0_SI,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_SI },
         { "consumer",             OPTION_VAL_CONSUMER_SI },
         { "canonical",            OPTION_VAL_CANONICAL_SI },
         { "alternative",          OPTION_VAL_ALTERNATIVE_SI },
         { "rgb",                  OPTION_VAL_RGB_O3_SI },
         { "pal",                  OPTION_VAL_PAL_SI },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_SI },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_SI },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_SI },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_SI },
         { "raw",                  OPTION_VAL_RAW_SI },
         { "custom",               OPTION_VAL_CUSTOM_SI },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_SI,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_SI,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_SI,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_SI,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_SI,
      NULL,
      NESTOPIA_ASPECT_INFO_0_SI,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_SI },
         { "ntsc",        OPTION_VAL_NTSC_SI },
         { "pal",         OPTION_VAL_PAL_SI },
         { "4:3",         OPTION_VAL_4_3_SI },
         { "uncorrected", OPTION_VAL_UNCORRECTED_SI },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_SI,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_SI,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_SI,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_SI,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_SI,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_SI,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_SI,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_SI,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_SI,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_SI,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_SI,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_SI,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_SI,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_SI,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_SI,
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
      NESTOPIA_AUDIO_TYPE_LABEL_SI,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_SI,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_SI },
         { "stereo", OPTION_VAL_STEREO_SI },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_SI,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_SI,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_SI },
         { "ntsc",    OPTION_VAL_NTSC_SI },
         { "famicom", OPTION_VAL_FAMICOM_SI },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_SI,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_SI, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_SI,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_SI,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_SI },
         { "pointer", OPTION_VAL_POINTER_SI },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_SI,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_SI,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_SI },
         { "mouse", OPTION_VAL_MOUSE_SI },
         { "pointer", OPTION_VAL_POINTER_SI },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_SI,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_SI,
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
      NESTOPIA_TURBO_PULSE_LABEL_SI,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_SI,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_SI,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_SI,
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
      NESTOPIA_OVERCLOCK_LABEL_SI,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_SI,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_SI },
         { "2x", OPTION_VAL_2X_SI },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_SI,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_SI,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_SI },
         { "0xFF",   OPTION_VAL_0XFF_SI },
         { "random", OPTION_VAL_RANDOM_SI },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_si = {
   option_cats_si,
   option_defs_si
};

/* RETRO_LANGUAGE_SK */

#define CATEGORY_SYSTEM_LABEL_SK "Systém"
#define CATEGORY_SYSTEM_INFO_0_SK NULL
#define CATEGORY_VIDEO_LABEL_SK NULL
#define CATEGORY_VIDEO_INFO_0_SK NULL
#define CATEGORY_AUDIO_LABEL_SK "Zvuk"
#define CATEGORY_AUDIO_INFO_0_SK NULL
#define CATEGORY_INPUT_LABEL_SK "Vstup"
#define CATEGORY_INPUT_INFO_0_SK NULL
#define CATEGORY_HACKS_LABEL_SK "Emulačné hacky"
#define CATEGORY_HACKS_INFO_0_SK NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_SK "Systémový región"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_SK NULL
#define OPTION_VAL_AUTO_SK NULL
#define OPTION_VAL_NTSC_SK NULL
#define OPTION_VAL_PAL_SK NULL
#define OPTION_VAL_FAMICOM_SK NULL
#define OPTION_VAL_DENDY_SK NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_SK NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_SK NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_SK NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_SK NULL
#define OPTION_VAL_COMPOSITE_SK NULL
#define OPTION_VAL_SVIDEO_SK NULL
#define OPTION_VAL_RGB_SK NULL
#define OPTION_VAL_MONOCHROME_SK "Čiernobiele"
#define NESTOPIA_PALETTE_LABEL_SK "Paleta"
#define NESTOPIA_PALETTE_INFO_0_SK NULL
#define OPTION_VAL_CXA2025AS_SK NULL
#define OPTION_VAL_CONSUMER_SK "Spotrebiteľ"
#define OPTION_VAL_CANONICAL_SK NULL
#define OPTION_VAL_ALTERNATIVE_SK "Alternatívne"
#define OPTION_VAL_RGB_O3_SK NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_SK NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_SK NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_SK NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_SK NULL
#define OPTION_VAL_RAW_SK "Čisté"
#define OPTION_VAL_CUSTOM_SK "Vlastné"
#define NESTOPIA_OVERSCAN_V_LABEL_SK NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_SK NULL
#define NESTOPIA_OVERSCAN_H_LABEL_SK NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_SK NULL
#define NESTOPIA_ASPECT_LABEL_SK NULL
#define NESTOPIA_ASPECT_INFO_0_SK NULL
#define OPTION_VAL_4_3_SK NULL
#define OPTION_VAL_UNCORRECTED_SK "Neopravené"
#define NESTOPIA_GENIE_DISTORTION_LABEL_SK NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_SK NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_SK NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_SK NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_SK NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_SK NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_SK NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_SK NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_SK NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_SK NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_SK NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_SK NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_SK NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_SK NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_SK NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_SK NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_SK NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_SK NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_SK NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_SK NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_SK NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_SK NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_SK NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_SK NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_SK NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_SK NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_SK NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_SK NULL
#define OPTION_VAL_MONO_SK NULL
#define OPTION_VAL_STEREO_SK NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_SK NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_SK NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_SK NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_SK NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_SK NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_SK NULL
#define OPTION_VAL_MOUSE_SK "Myš"
#define OPTION_VAL_POINTER_SK NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_SK NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_SK NULL
#define OPTION_VAL_LIGHTGUN_SK "Svetelná pištoľ"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_SK NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_SK NULL
#define NESTOPIA_TURBO_PULSE_LABEL_SK NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_SK NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_SK NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_SK NULL
#define NESTOPIA_OVERCLOCK_LABEL_SK NULL
#define NESTOPIA_OVERCLOCK_INFO_0_SK NULL
#define OPTION_VAL_1X_SK NULL
#define OPTION_VAL_2X_SK NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_SK NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_SK NULL
#define OPTION_VAL_0X00_SK NULL
#define OPTION_VAL_0XFF_SK NULL
#define OPTION_VAL_RANDOM_SK "Náhodné"

struct retro_core_option_v2_category option_cats_sk[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_SK,
      CATEGORY_SYSTEM_INFO_0_SK
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_SK,
      CATEGORY_VIDEO_INFO_0_SK
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_SK,
      CATEGORY_AUDIO_INFO_0_SK
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_SK,
      CATEGORY_INPUT_INFO_0_SK
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_SK,
      CATEGORY_HACKS_INFO_0_SK
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_sk[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_SK,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_SK,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_SK },
         { "ntsc",    OPTION_VAL_NTSC_SK },
         { "pal",     OPTION_VAL_PAL_SK },
         { "famicom", OPTION_VAL_FAMICOM_SK },
         { "dendy",   OPTION_VAL_DENDY_SK },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_SK,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_SK,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_SK,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_SK,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_SK },
         { "svideo",     OPTION_VAL_SVIDEO_SK },
         { "rgb",        OPTION_VAL_RGB_SK },
         { "monochrome", OPTION_VAL_MONOCHROME_SK },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_SK,
      NULL,
      NESTOPIA_PALETTE_INFO_0_SK,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_SK },
         { "consumer",             OPTION_VAL_CONSUMER_SK },
         { "canonical",            OPTION_VAL_CANONICAL_SK },
         { "alternative",          OPTION_VAL_ALTERNATIVE_SK },
         { "rgb",                  OPTION_VAL_RGB_O3_SK },
         { "pal",                  OPTION_VAL_PAL_SK },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_SK },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_SK },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_SK },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_SK },
         { "raw",                  OPTION_VAL_RAW_SK },
         { "custom",               OPTION_VAL_CUSTOM_SK },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_SK,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_SK,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_SK,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_SK,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_SK,
      NULL,
      NESTOPIA_ASPECT_INFO_0_SK,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_SK },
         { "ntsc",        OPTION_VAL_NTSC_SK },
         { "pal",         OPTION_VAL_PAL_SK },
         { "4:3",         OPTION_VAL_4_3_SK },
         { "uncorrected", OPTION_VAL_UNCORRECTED_SK },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_SK,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_SK,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_SK,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_SK,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_SK,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_SK,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_SK,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_SK,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_SK,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_SK,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_SK,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_SK,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_SK,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_SK,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_SK,
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
      NESTOPIA_AUDIO_TYPE_LABEL_SK,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_SK,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_SK },
         { "stereo", OPTION_VAL_STEREO_SK },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_SK,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_SK,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_SK },
         { "ntsc",    OPTION_VAL_NTSC_SK },
         { "famicom", OPTION_VAL_FAMICOM_SK },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_SK,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_SK, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_SK,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_SK,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_SK },
         { "pointer", OPTION_VAL_POINTER_SK },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_SK,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_SK,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_SK },
         { "mouse", OPTION_VAL_MOUSE_SK },
         { "pointer", OPTION_VAL_POINTER_SK },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_SK,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_SK,
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
      NESTOPIA_TURBO_PULSE_LABEL_SK,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_SK,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_SK,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_SK,
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
      NESTOPIA_OVERCLOCK_LABEL_SK,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_SK,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_SK },
         { "2x", OPTION_VAL_2X_SK },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_SK,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_SK,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_SK },
         { "0xFF",   OPTION_VAL_0XFF_SK },
         { "random", OPTION_VAL_RANDOM_SK },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_sk = {
   option_cats_sk,
   option_defs_sk
};

/* RETRO_LANGUAGE_SR */

#define CATEGORY_SYSTEM_LABEL_SR NULL
#define CATEGORY_SYSTEM_INFO_0_SR NULL
#define CATEGORY_VIDEO_LABEL_SR NULL
#define CATEGORY_VIDEO_INFO_0_SR NULL
#define CATEGORY_AUDIO_LABEL_SR "Zvuk"
#define CATEGORY_AUDIO_INFO_0_SR NULL
#define CATEGORY_INPUT_LABEL_SR "Ulaz"
#define CATEGORY_INPUT_INFO_0_SR NULL
#define CATEGORY_HACKS_LABEL_SR NULL
#define CATEGORY_HACKS_INFO_0_SR NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_SR NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_SR NULL
#define OPTION_VAL_AUTO_SR NULL
#define OPTION_VAL_NTSC_SR NULL
#define OPTION_VAL_PAL_SR NULL
#define OPTION_VAL_FAMICOM_SR NULL
#define OPTION_VAL_DENDY_SR NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_SR NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_SR NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_SR NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_SR NULL
#define OPTION_VAL_COMPOSITE_SR NULL
#define OPTION_VAL_SVIDEO_SR NULL
#define OPTION_VAL_RGB_SR NULL
#define OPTION_VAL_MONOCHROME_SR NULL
#define NESTOPIA_PALETTE_LABEL_SR NULL
#define NESTOPIA_PALETTE_INFO_0_SR NULL
#define OPTION_VAL_CXA2025AS_SR NULL
#define OPTION_VAL_CONSUMER_SR NULL
#define OPTION_VAL_CANONICAL_SR NULL
#define OPTION_VAL_ALTERNATIVE_SR NULL
#define OPTION_VAL_RGB_O3_SR NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_SR NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_SR NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_SR NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_SR NULL
#define OPTION_VAL_RAW_SR NULL
#define OPTION_VAL_CUSTOM_SR NULL
#define NESTOPIA_OVERSCAN_V_LABEL_SR NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_SR NULL
#define NESTOPIA_OVERSCAN_H_LABEL_SR NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_SR NULL
#define NESTOPIA_ASPECT_LABEL_SR NULL
#define NESTOPIA_ASPECT_INFO_0_SR NULL
#define OPTION_VAL_4_3_SR NULL
#define OPTION_VAL_UNCORRECTED_SR NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_SR NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_SR NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_SR NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_SR NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_SR NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_SR NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_SR NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_SR NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_SR NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_SR NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_SR NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_SR NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_SR NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_SR NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_SR NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_SR NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_SR NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_SR NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_SR NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_SR NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_SR NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_SR NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_SR NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_SR NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_SR NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_SR NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_SR NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_SR NULL
#define OPTION_VAL_MONO_SR NULL
#define OPTION_VAL_STEREO_SR NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_SR NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_SR NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_SR NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_SR NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_SR NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_SR NULL
#define OPTION_VAL_MOUSE_SR NULL
#define OPTION_VAL_POINTER_SR NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_SR NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_SR NULL
#define OPTION_VAL_LIGHTGUN_SR NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_SR NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_SR NULL
#define NESTOPIA_TURBO_PULSE_LABEL_SR NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_SR NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_SR NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_SR NULL
#define NESTOPIA_OVERCLOCK_LABEL_SR NULL
#define NESTOPIA_OVERCLOCK_INFO_0_SR NULL
#define OPTION_VAL_1X_SR NULL
#define OPTION_VAL_2X_SR NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_SR NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_SR NULL
#define OPTION_VAL_0X00_SR NULL
#define OPTION_VAL_0XFF_SR NULL
#define OPTION_VAL_RANDOM_SR NULL

struct retro_core_option_v2_category option_cats_sr[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_SR,
      CATEGORY_SYSTEM_INFO_0_SR
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_SR,
      CATEGORY_VIDEO_INFO_0_SR
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_SR,
      CATEGORY_AUDIO_INFO_0_SR
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_SR,
      CATEGORY_INPUT_INFO_0_SR
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_SR,
      CATEGORY_HACKS_INFO_0_SR
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_sr[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_SR,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_SR,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_SR },
         { "ntsc",    OPTION_VAL_NTSC_SR },
         { "pal",     OPTION_VAL_PAL_SR },
         { "famicom", OPTION_VAL_FAMICOM_SR },
         { "dendy",   OPTION_VAL_DENDY_SR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_SR,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_SR,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_SR,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_SR,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_SR },
         { "svideo",     OPTION_VAL_SVIDEO_SR },
         { "rgb",        OPTION_VAL_RGB_SR },
         { "monochrome", OPTION_VAL_MONOCHROME_SR },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_SR,
      NULL,
      NESTOPIA_PALETTE_INFO_0_SR,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_SR },
         { "consumer",             OPTION_VAL_CONSUMER_SR },
         { "canonical",            OPTION_VAL_CANONICAL_SR },
         { "alternative",          OPTION_VAL_ALTERNATIVE_SR },
         { "rgb",                  OPTION_VAL_RGB_O3_SR },
         { "pal",                  OPTION_VAL_PAL_SR },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_SR },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_SR },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_SR },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_SR },
         { "raw",                  OPTION_VAL_RAW_SR },
         { "custom",               OPTION_VAL_CUSTOM_SR },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_SR,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_SR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_SR,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_SR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_SR,
      NULL,
      NESTOPIA_ASPECT_INFO_0_SR,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_SR },
         { "ntsc",        OPTION_VAL_NTSC_SR },
         { "pal",         OPTION_VAL_PAL_SR },
         { "4:3",         OPTION_VAL_4_3_SR },
         { "uncorrected", OPTION_VAL_UNCORRECTED_SR },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_SR,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_SR,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_SR,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_SR,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_SR,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_SR,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_SR,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_SR,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_SR,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_SR,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_SR,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_SR,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_SR,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_SR,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_SR,
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
      NESTOPIA_AUDIO_TYPE_LABEL_SR,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_SR,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_SR },
         { "stereo", OPTION_VAL_STEREO_SR },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_SR,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_SR,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_SR },
         { "ntsc",    OPTION_VAL_NTSC_SR },
         { "famicom", OPTION_VAL_FAMICOM_SR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_SR,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_SR, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_SR,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_SR,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_SR },
         { "pointer", OPTION_VAL_POINTER_SR },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_SR,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_SR,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_SR },
         { "mouse", OPTION_VAL_MOUSE_SR },
         { "pointer", OPTION_VAL_POINTER_SR },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_SR,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_SR,
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
      NESTOPIA_TURBO_PULSE_LABEL_SR,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_SR,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_SR,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_SR,
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
      NESTOPIA_OVERCLOCK_LABEL_SR,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_SR,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_SR },
         { "2x", OPTION_VAL_2X_SR },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_SR,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_SR,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_SR },
         { "0xFF",   OPTION_VAL_0XFF_SR },
         { "random", OPTION_VAL_RANDOM_SR },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_sr = {
   option_cats_sr,
   option_defs_sr
};

/* RETRO_LANGUAGE_SV */

#define CATEGORY_SYSTEM_LABEL_SV NULL
#define CATEGORY_SYSTEM_INFO_0_SV NULL
#define CATEGORY_VIDEO_LABEL_SV "Bild"
#define CATEGORY_VIDEO_INFO_0_SV NULL
#define CATEGORY_AUDIO_LABEL_SV "Ljud"
#define CATEGORY_AUDIO_INFO_0_SV NULL
#define CATEGORY_INPUT_LABEL_SV "Inmatning"
#define CATEGORY_INPUT_INFO_0_SV NULL
#define CATEGORY_HACKS_LABEL_SV NULL
#define CATEGORY_HACKS_INFO_0_SV NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_SV NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_SV NULL
#define OPTION_VAL_AUTO_SV "Automatiskt"
#define OPTION_VAL_NTSC_SV NULL
#define OPTION_VAL_PAL_SV NULL
#define OPTION_VAL_FAMICOM_SV NULL
#define OPTION_VAL_DENDY_SV NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_SV NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_SV NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_SV NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_SV NULL
#define OPTION_VAL_COMPOSITE_SV NULL
#define OPTION_VAL_SVIDEO_SV NULL
#define OPTION_VAL_RGB_SV NULL
#define OPTION_VAL_MONOCHROME_SV "Monokrom"
#define NESTOPIA_PALETTE_LABEL_SV "Palett"
#define NESTOPIA_PALETTE_INFO_0_SV NULL
#define OPTION_VAL_CXA2025AS_SV NULL
#define OPTION_VAL_CONSUMER_SV "Konsument"
#define OPTION_VAL_CANONICAL_SV NULL
#define OPTION_VAL_ALTERNATIVE_SV NULL
#define OPTION_VAL_RGB_O3_SV NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_SV NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_SV NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_SV NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_SV NULL
#define OPTION_VAL_RAW_SV NULL
#define OPTION_VAL_CUSTOM_SV "Anpassad"
#define NESTOPIA_OVERSCAN_V_LABEL_SV NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_SV NULL
#define NESTOPIA_OVERSCAN_H_LABEL_SV NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_SV NULL
#define NESTOPIA_ASPECT_LABEL_SV NULL
#define NESTOPIA_ASPECT_INFO_0_SV NULL
#define OPTION_VAL_4_3_SV NULL
#define OPTION_VAL_UNCORRECTED_SV NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_SV NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_SV NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_SV NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_SV NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_SV NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_SV NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_SV NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_SV NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_SV NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_SV NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_SV NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_SV NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_SV NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_SV NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_SV NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_SV NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_SV NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_SV NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_SV NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_SV NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_SV NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_SV NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_SV NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_SV NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_SV NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_SV NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_SV NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_SV NULL
#define OPTION_VAL_MONO_SV NULL
#define OPTION_VAL_STEREO_SV NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_SV NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_SV NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_SV NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_SV NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_SV NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_SV NULL
#define OPTION_VAL_MOUSE_SV "Mus"
#define OPTION_VAL_POINTER_SV "Pekare"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_SV NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_SV NULL
#define OPTION_VAL_LIGHTGUN_SV NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_SV NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_SV NULL
#define NESTOPIA_TURBO_PULSE_LABEL_SV NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_SV NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_SV NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_SV NULL
#define NESTOPIA_OVERCLOCK_LABEL_SV NULL
#define NESTOPIA_OVERCLOCK_INFO_0_SV NULL
#define OPTION_VAL_1X_SV NULL
#define OPTION_VAL_2X_SV NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_SV NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_SV NULL
#define OPTION_VAL_0X00_SV NULL
#define OPTION_VAL_0XFF_SV NULL
#define OPTION_VAL_RANDOM_SV NULL

struct retro_core_option_v2_category option_cats_sv[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_SV,
      CATEGORY_SYSTEM_INFO_0_SV
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_SV,
      CATEGORY_VIDEO_INFO_0_SV
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_SV,
      CATEGORY_AUDIO_INFO_0_SV
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_SV,
      CATEGORY_INPUT_INFO_0_SV
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_SV,
      CATEGORY_HACKS_INFO_0_SV
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_sv[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_SV,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_SV,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_SV },
         { "ntsc",    OPTION_VAL_NTSC_SV },
         { "pal",     OPTION_VAL_PAL_SV },
         { "famicom", OPTION_VAL_FAMICOM_SV },
         { "dendy",   OPTION_VAL_DENDY_SV },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_SV,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_SV,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_SV,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_SV,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_SV },
         { "svideo",     OPTION_VAL_SVIDEO_SV },
         { "rgb",        OPTION_VAL_RGB_SV },
         { "monochrome", OPTION_VAL_MONOCHROME_SV },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_SV,
      NULL,
      NESTOPIA_PALETTE_INFO_0_SV,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_SV },
         { "consumer",             OPTION_VAL_CONSUMER_SV },
         { "canonical",            OPTION_VAL_CANONICAL_SV },
         { "alternative",          OPTION_VAL_ALTERNATIVE_SV },
         { "rgb",                  OPTION_VAL_RGB_O3_SV },
         { "pal",                  OPTION_VAL_PAL_SV },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_SV },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_SV },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_SV },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_SV },
         { "raw",                  OPTION_VAL_RAW_SV },
         { "custom",               OPTION_VAL_CUSTOM_SV },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_SV,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_SV,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_SV,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_SV,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_SV,
      NULL,
      NESTOPIA_ASPECT_INFO_0_SV,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_SV },
         { "ntsc",        OPTION_VAL_NTSC_SV },
         { "pal",         OPTION_VAL_PAL_SV },
         { "4:3",         OPTION_VAL_4_3_SV },
         { "uncorrected", OPTION_VAL_UNCORRECTED_SV },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_SV,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_SV,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_SV,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_SV,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_SV,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_SV,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_SV,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_SV,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_SV,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_SV,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_SV,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_SV,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_SV,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_SV,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_SV,
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
      NESTOPIA_AUDIO_TYPE_LABEL_SV,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_SV,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_SV },
         { "stereo", OPTION_VAL_STEREO_SV },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_SV,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_SV,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_SV },
         { "ntsc",    OPTION_VAL_NTSC_SV },
         { "famicom", OPTION_VAL_FAMICOM_SV },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_SV,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_SV, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_SV,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_SV,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_SV },
         { "pointer", OPTION_VAL_POINTER_SV },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_SV,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_SV,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_SV },
         { "mouse", OPTION_VAL_MOUSE_SV },
         { "pointer", OPTION_VAL_POINTER_SV },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_SV,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_SV,
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
      NESTOPIA_TURBO_PULSE_LABEL_SV,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_SV,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_SV,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_SV,
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
      NESTOPIA_OVERCLOCK_LABEL_SV,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_SV,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_SV },
         { "2x", OPTION_VAL_2X_SV },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_SV,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_SV,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_SV },
         { "0xFF",   OPTION_VAL_0XFF_SV },
         { "random", OPTION_VAL_RANDOM_SV },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_sv = {
   option_cats_sv,
   option_defs_sv
};

/* RETRO_LANGUAGE_TR */

#define CATEGORY_SYSTEM_LABEL_TR "Sistem"
#define CATEGORY_SYSTEM_INFO_0_TR NULL
#define CATEGORY_VIDEO_LABEL_TR NULL
#define CATEGORY_VIDEO_INFO_0_TR "En boy oranını, görüntü kırpmayı, renk paletini ve video filtresi ayarlarını değiştirin."
#define CATEGORY_AUDIO_LABEL_TR "Ses"
#define CATEGORY_AUDIO_INFO_0_TR "Ses efektlerini ve kanal ses düzeylerini değiştirin."
#define CATEGORY_INPUT_LABEL_TR "Girdi"
#define CATEGORY_INPUT_INFO_0_TR "Girdi cihazlarını ve girdilerle ilgili diğer ayarları değiştirin."
#define CATEGORY_HACKS_LABEL_TR "Emülatör Geliştirmeleri"
#define CATEGORY_HACKS_INFO_0_TR "Düşük seviyeli performansı ve uyumluluğu etkileyen işlemci hız aşırtma ve taklit doğruluğu ayarlarını değiştirin."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_TR "Sistem Bölgesi"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_TR "'Otomatik', bölge otomatik algılaması için NstDatabase.xml veritabanı dosyasını kullanır. Mevcut bir veritabanı yoksa, varsayılan olarak NTSC olacaktır."
#define OPTION_VAL_AUTO_TR "Otomatik"
#define OPTION_VAL_NTSC_TR NULL
#define OPTION_VAL_PAL_TR NULL
#define OPTION_VAL_FAMICOM_TR NULL
#define OPTION_VAL_DENDY_TR NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_TR "FDS Otomatik Ekle"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_TR "Yeniden başlatmada ilk FDS diskini otomatik olarak yerleştirir."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_TR "Blargg NTSC Filtresi"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_TR "Blargg NTSC filtrelerini etkinleştirin."
#define OPTION_VAL_COMPOSITE_TR NULL
#define OPTION_VAL_SVIDEO_TR NULL
#define OPTION_VAL_RGB_TR NULL
#define OPTION_VAL_MONOCHROME_TR "Tek Renk"
#define NESTOPIA_PALETTE_LABEL_TR "Palet"
#define NESTOPIA_PALETTE_INFO_0_TR NULL
#define OPTION_VAL_CXA2025AS_TR NULL
#define OPTION_VAL_CONSUMER_TR "Kullanıcı"
#define OPTION_VAL_CANONICAL_TR "Standart"
#define OPTION_VAL_ALTERNATIVE_TR "Alternatif"
#define OPTION_VAL_RGB_O3_TR NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_TR "Composite Direk FBx"
#define OPTION_VAL_PVM_STYLE_D93_FBX_TR NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_TR "NTSC donanım FBx"
#define OPTION_VAL_NES_CLASSIC_FBX_FS_TR "NES Classic FBx (düzeltilmiş)"
#define OPTION_VAL_RAW_TR "İşlenmemiş"
#define OPTION_VAL_CUSTOM_TR "Özel"
#define NESTOPIA_OVERSCAN_V_LABEL_TR "Aşırı Taramayı Maskele (Dikey)"
#define NESTOPIA_OVERSCAN_V_INFO_0_TR "Standart tanımlı bir televizyon ekranının kenarındaki çerçeve tarafından gizlenmiş olabilecek olası rastgele arızalı video çıkışını (dikey olarak) maskeleyin."
#define NESTOPIA_OVERSCAN_H_LABEL_TR "Aşırı Taramayı Maskele (Yatay)"
#define NESTOPIA_OVERSCAN_H_INFO_0_TR "Standart tanımlı bir televizyon ekranının kenarındaki çerçeve tarafından gizlenmiş olabilecek olası rastgele arızalı video çıkışını (yatay olarak) maskeleyin."
#define NESTOPIA_ASPECT_LABEL_TR "Tercih Edilen En Boy Oranı"
#define NESTOPIA_ASPECT_INFO_0_TR NULL
#define OPTION_VAL_4_3_TR NULL
#define OPTION_VAL_UNCORRECTED_TR "Düzeltilmemiş"
#define NESTOPIA_GENIE_DISTORTION_LABEL_TR "Game Genie Ses Bozulması"
#define NESTOPIA_GENIE_DISTORTION_INFO_0_TR "Game Genie hile cihazı yanlışlıkla oyunlarda ses bozulmalarına neden olabilir. Bunu etkinleştirerek, oyun sesine ekleyeceği bozulmayı taklit edebilirsiniz."
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_TR "Gelişmiş Ses Ayarlarını Göster (Menüyü yeniden aç)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_TR "Düşük seviyeli ses kanalı parametrelerinin ayarlarını etkinleştirin."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_TR "Kare 1. Kanal Seviyesi %"
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_TR "Kare 1. Kanal Seviyesini Düzenle %."
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_TR "Kare 2. Kanal Seviyesi %"
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_TR "Kare 2. Kanal Seviyesini Düzenle %."
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_TR "Üçgen Kanal Seviyesi %"
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_TR "Üçgen Kanal Seviyesini Düzenle %."
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_TR "Gürültü Kanalı Seviyesi %"
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_TR "Gürültü Kanalı Seviyesini Düzenle %."
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_TR "DPCM Kanal Seviyesi %"
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_TR "DPCM Kanal Seviyesini Düzenle %."
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_TR "FDS Kanal Seviyesi %"
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_TR "FDS Kanal Seviyesini Düzenle %."
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_TR "MMC5 Kanal Seviyesi %"
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_TR "MMC5 Kanal Seviyesini Düzenle %."
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_TR "VRC6 Kanal Seviyesi %"
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_TR "VRC6 Kanal Seviyesini Düzenle %."
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_TR "VRC7 Kanal Seviyesi %"
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_TR "VRC7 Kanal Seviyesini Düzenle %."
#define NESTOPIA_AUDIO_VOL_N163_LABEL_TR "N163 Kanal Seviyesi %"
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_TR "N163 Kanal Seviyesini Düzenle %."
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_TR "S5B Kanal Seviyesi %"
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_TR NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_TR NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_TR NULL
#define OPTION_VAL_MONO_TR NULL
#define OPTION_VAL_STEREO_TR NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_TR "4 Oyuncu Adaptörü"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_TR "Gerekirse el ile bir 4 Oyuncu Adaptörü seçin. Bazı oyunlar bağdaştırıcıyı NstDatabase.xml veritabanı ile doğru bir şekilde tanıyamayabilir, bu seçenek bunu düzeltmeye yardımcı olur."
#define NESTOPIA_BUTTON_SHIFT_LABEL_TR "Shift Düğmeleri Saat Yönünde"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_TR "A/B/X/Y düğmelerini saat yönünde çevirin."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_TR "Arkanoid cihazı"
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_TR "Arkanoid kürek için kullanmak istediğiniz cihazı seçin."
#define OPTION_VAL_MOUSE_TR "Fare"
#define OPTION_VAL_POINTER_TR "İşaretçi"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_TR "Zapper cihazı"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_TR "Zapper için kullanmak istediğiniz cihazı seçin."
#define OPTION_VAL_LIGHTGUN_TR NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_TR "Nişangahı Göster"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_TR "Zapper kullanıldığında nişangahın gösterilip gösterilmeyeceğini ayarlayın."
#define NESTOPIA_TURBO_PULSE_LABEL_TR "Turbo Darbe Hızı"
#define NESTOPIA_TURBO_PULSE_INFO_0_TR "Turbo B ve Turbo A düğmeleri için turbo hızını ayarlayın."
#define NESTOPIA_NOSPRITELIMIT_LABEL_TR "Hareket Sınırını Kaldır"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_TR "Tarama başına 8 hareket donanım sınırını kaldır."
#define NESTOPIA_OVERCLOCK_LABEL_TR "CPU Hızı (Hız Aşırtma)"
#define NESTOPIA_OVERCLOCK_INFO_0_TR "Taklit edilmiş CPU için hız aşırtma uygula."
#define OPTION_VAL_1X_TR NULL
#define OPTION_VAL_2X_TR NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_TR "RAM Açılış Durumu"
#define NESTOPIA_RAM_POWER_STATE_INFO_0_TR "Açılışta RAM değerleri. Bazı oyunlar, örnek olarak rastgele sayı üretimi için ilk RAM değerlerine güvenir."
#define OPTION_VAL_0X00_TR NULL
#define OPTION_VAL_0XFF_TR NULL
#define OPTION_VAL_RANDOM_TR "Rastgele"

struct retro_core_option_v2_category option_cats_tr[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_TR,
      CATEGORY_SYSTEM_INFO_0_TR
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_TR,
      CATEGORY_VIDEO_INFO_0_TR
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_TR,
      CATEGORY_AUDIO_INFO_0_TR
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_TR,
      CATEGORY_INPUT_INFO_0_TR
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_TR,
      CATEGORY_HACKS_INFO_0_TR
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_tr[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_TR,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_TR,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_TR },
         { "ntsc",    OPTION_VAL_NTSC_TR },
         { "pal",     OPTION_VAL_PAL_TR },
         { "famicom", OPTION_VAL_FAMICOM_TR },
         { "dendy",   OPTION_VAL_DENDY_TR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_TR,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_TR,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_TR,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_TR,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_TR },
         { "svideo",     OPTION_VAL_SVIDEO_TR },
         { "rgb",        OPTION_VAL_RGB_TR },
         { "monochrome", OPTION_VAL_MONOCHROME_TR },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_TR,
      NULL,
      NESTOPIA_PALETTE_INFO_0_TR,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_TR },
         { "consumer",             OPTION_VAL_CONSUMER_TR },
         { "canonical",            OPTION_VAL_CANONICAL_TR },
         { "alternative",          OPTION_VAL_ALTERNATIVE_TR },
         { "rgb",                  OPTION_VAL_RGB_O3_TR },
         { "pal",                  OPTION_VAL_PAL_TR },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_TR },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_TR },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_TR },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_TR },
         { "raw",                  OPTION_VAL_RAW_TR },
         { "custom",               OPTION_VAL_CUSTOM_TR },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_TR,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_TR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_TR,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_TR,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_TR,
      NULL,
      NESTOPIA_ASPECT_INFO_0_TR,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_TR },
         { "ntsc",        OPTION_VAL_NTSC_TR },
         { "pal",         OPTION_VAL_PAL_TR },
         { "4:3",         OPTION_VAL_4_3_TR },
         { "uncorrected", OPTION_VAL_UNCORRECTED_TR },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_TR,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_TR,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_TR,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_TR,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_TR,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_TR,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_TR,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_TR,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_TR,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_TR,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_TR,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_TR,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_TR,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_TR,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_TR,
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
      NESTOPIA_AUDIO_TYPE_LABEL_TR,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_TR,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_TR },
         { "stereo", OPTION_VAL_STEREO_TR },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_TR,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_TR,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_TR },
         { "ntsc",    OPTION_VAL_NTSC_TR },
         { "famicom", OPTION_VAL_FAMICOM_TR },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_TR,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_TR, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_TR,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_TR,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_TR },
         { "pointer", OPTION_VAL_POINTER_TR },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_TR,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_TR,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_TR },
         { "mouse", OPTION_VAL_MOUSE_TR },
         { "pointer", OPTION_VAL_POINTER_TR },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_TR,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_TR,
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
      NESTOPIA_TURBO_PULSE_LABEL_TR,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_TR,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_TR,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_TR,
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
      NESTOPIA_OVERCLOCK_LABEL_TR,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_TR,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_TR },
         { "2x", OPTION_VAL_2X_TR },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_TR,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_TR,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_TR },
         { "0xFF",   OPTION_VAL_0XFF_TR },
         { "random", OPTION_VAL_RANDOM_TR },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_tr = {
   option_cats_tr,
   option_defs_tr
};

/* RETRO_LANGUAGE_UK */

#define CATEGORY_SYSTEM_LABEL_UK "Система"
#define CATEGORY_SYSTEM_INFO_0_UK NULL
#define CATEGORY_VIDEO_LABEL_UK "Відео"
#define CATEGORY_VIDEO_INFO_0_UK "Змінити співвідношення сторін, обрізання зображення, палітри кольорів та налаштування відео фільтрів."
#define CATEGORY_AUDIO_LABEL_UK "Аудіо"
#define CATEGORY_AUDIO_INFO_0_UK "Зміна гучності звукових ефектів і гучності каналів."
#define CATEGORY_INPUT_LABEL_UK "Ввід"
#define CATEGORY_INPUT_INFO_0_UK "Зміна пристроїв вводу та інші налаштування, пов'язані з керуванням."
#define CATEGORY_HACKS_LABEL_UK "Гаки емуляції"
#define CATEGORY_HACKS_INFO_0_UK "Налаштування розгону процесора та точності емуляції, що впливають на низькорівневу продуктивність та сумісність."
#define NESTOPIA_FAVORED_SYSTEM_LABEL_UK "Регіон системи"
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_UK "'Авто' буде використовувати файл бази даних NstDatabase.xml для автоматичного виявлення регіону. Якщо у базі даних немає, то буде використовуватися за замовчуванням NTSC."
#define OPTION_VAL_AUTO_UK "Авто"
#define OPTION_VAL_NTSC_UK NULL
#define OPTION_VAL_PAL_UK NULL
#define OPTION_VAL_FAMICOM_UK NULL
#define OPTION_VAL_DENDY_UK NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_UK "FDS Авто вставка дискети"
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_UK "Автоматично вставляти перший диск FDS після скидання."
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_UK "Фільтр Blargg NTSC"
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_UK "Увімкнути фільтри Blargg NTSC."
#define OPTION_VAL_COMPOSITE_UK "Композитне відео"
#define OPTION_VAL_SVIDEO_UK NULL
#define OPTION_VAL_RGB_UK NULL
#define OPTION_VAL_MONOCHROME_UK "Монотонні"
#define NESTOPIA_PALETTE_LABEL_UK "Палітра"
#define NESTOPIA_PALETTE_INFO_0_UK NULL
#define OPTION_VAL_CXA2025AS_UK NULL
#define OPTION_VAL_CONSUMER_UK "Користувацька"
#define OPTION_VAL_CANONICAL_UK "Канонічний"
#define OPTION_VAL_ALTERNATIVE_UK "Альтернатива"
#define OPTION_VAL_RGB_O3_UK NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_UK "Композитний FBx"
#define OPTION_VAL_PVM_STYLE_D93_FBX_UK "PVM стиль D93 FBx"
#define OPTION_VAL_NTSC_HARDWARE_FBX_UK NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_UK NULL
#define OPTION_VAL_RAW_UK NULL
#define OPTION_VAL_CUSTOM_UK "Користувацька"
#define NESTOPIA_OVERSCAN_V_LABEL_UK "Розгортка маски (вертикальна)"
#define NESTOPIA_OVERSCAN_V_INFO_0_UK NULL
#define NESTOPIA_OVERSCAN_H_LABEL_UK NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_UK NULL
#define NESTOPIA_ASPECT_LABEL_UK "Бажане співвідношення сторін"
#define NESTOPIA_ASPECT_INFO_0_UK NULL
#define OPTION_VAL_4_3_UK NULL
#define OPTION_VAL_UNCORRECTED_UK "Не коректне"
#define NESTOPIA_GENIE_DISTORTION_LABEL_UK NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_UK NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_UK "Показати додаткові параметри аудіо (перевідкрити меню)"
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_UK "Увімкнути конфігурацію низькорівневих параметрів аудіоканалів."
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_UK NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_UK NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_UK NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_UK NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_UK NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_UK NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_UK NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_UK NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_UK NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_UK NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_UK NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_UK NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_UK NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_UK NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_UK NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_UK NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_UK NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_UK NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_UK NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_UK NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_UK NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_UK NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_UK NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_UK NULL
#define OPTION_VAL_MONO_UK NULL
#define OPTION_VAL_STEREO_UK NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_UK "Адаптер на 4 гравці"
#define NESTOPIA_SELECT_ADAPTER_INFO_0_UK "Якщо потрібно, вручну виберіть адаптер на 4 гравці. Деякі ігри не розпізнають адаптер правильно через базу NstDatabase.xml, ця опція повинна допомогти це виправляти."
#define NESTOPIA_BUTTON_SHIFT_LABEL_UK "Зміна кнопок за годинниковою стрілкою"
#define NESTOPIA_BUTTON_SHIFT_INFO_0_UK "Обертати кнопки A/B/X/Y за годинниковою стрілкою."
#define NESTOPIA_ARKANOID_DEVICE_LABEL_UK NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_UK "Виберіть пристрій, який ви хочете використовувати для платформи Арканоїда."
#define OPTION_VAL_MOUSE_UK "Миша"
#define OPTION_VAL_POINTER_UK "Вказівник"
#define NESTOPIA_ZAPPER_DEVICE_LABEL_UK "Світлові пістолети"
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_UK "Виберіть пристрій, який ви хочете використовувати як світловий пістолет."
#define OPTION_VAL_LIGHTGUN_UK "Світловий пістолет"
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_UK "Показати приціл"
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_UK "Встановіть, чи показувати перехрестя прицілу під час використання світлового пістолета."
#define NESTOPIA_TURBO_PULSE_LABEL_UK NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_UK NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_UK "Видалити ліміт спрайтів"
#define NESTOPIA_NOSPRITELIMIT_INFO_0_UK NULL
#define NESTOPIA_OVERCLOCK_LABEL_UK "Швидкість ЦП (розгін)"
#define NESTOPIA_OVERCLOCK_INFO_0_UK "Розгін емульованого ЦП."
#define OPTION_VAL_1X_UK NULL
#define OPTION_VAL_2X_UK NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_UK NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_UK NULL
#define OPTION_VAL_0X00_UK NULL
#define OPTION_VAL_0XFF_UK NULL
#define OPTION_VAL_RANDOM_UK "Випадково"

struct retro_core_option_v2_category option_cats_uk[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_UK,
      CATEGORY_SYSTEM_INFO_0_UK
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_UK,
      CATEGORY_VIDEO_INFO_0_UK
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_UK,
      CATEGORY_AUDIO_INFO_0_UK
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_UK,
      CATEGORY_INPUT_INFO_0_UK
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_UK,
      CATEGORY_HACKS_INFO_0_UK
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_uk[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_UK,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_UK,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_UK },
         { "ntsc",    OPTION_VAL_NTSC_UK },
         { "pal",     OPTION_VAL_PAL_UK },
         { "famicom", OPTION_VAL_FAMICOM_UK },
         { "dendy",   OPTION_VAL_DENDY_UK },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_UK,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_UK,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_UK,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_UK,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_UK },
         { "svideo",     OPTION_VAL_SVIDEO_UK },
         { "rgb",        OPTION_VAL_RGB_UK },
         { "monochrome", OPTION_VAL_MONOCHROME_UK },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_UK,
      NULL,
      NESTOPIA_PALETTE_INFO_0_UK,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_UK },
         { "consumer",             OPTION_VAL_CONSUMER_UK },
         { "canonical",            OPTION_VAL_CANONICAL_UK },
         { "alternative",          OPTION_VAL_ALTERNATIVE_UK },
         { "rgb",                  OPTION_VAL_RGB_O3_UK },
         { "pal",                  OPTION_VAL_PAL_UK },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_UK },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_UK },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_UK },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_UK },
         { "raw",                  OPTION_VAL_RAW_UK },
         { "custom",               OPTION_VAL_CUSTOM_UK },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_UK,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_UK,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_UK,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_UK,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_UK,
      NULL,
      NESTOPIA_ASPECT_INFO_0_UK,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_UK },
         { "ntsc",        OPTION_VAL_NTSC_UK },
         { "pal",         OPTION_VAL_PAL_UK },
         { "4:3",         OPTION_VAL_4_3_UK },
         { "uncorrected", OPTION_VAL_UNCORRECTED_UK },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_UK,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_UK,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_UK,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_UK,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_UK,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_UK,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_UK,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_UK,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_UK,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_UK,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_UK,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_UK,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_UK,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_UK,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_UK,
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
      NESTOPIA_AUDIO_TYPE_LABEL_UK,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_UK,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_UK },
         { "stereo", OPTION_VAL_STEREO_UK },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_UK,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_UK,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_UK },
         { "ntsc",    OPTION_VAL_NTSC_UK },
         { "famicom", OPTION_VAL_FAMICOM_UK },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_UK,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_UK, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_UK,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_UK,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_UK },
         { "pointer", OPTION_VAL_POINTER_UK },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_UK,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_UK,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_UK },
         { "mouse", OPTION_VAL_MOUSE_UK },
         { "pointer", OPTION_VAL_POINTER_UK },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_UK,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_UK,
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
      NESTOPIA_TURBO_PULSE_LABEL_UK,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_UK,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_UK,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_UK,
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
      NESTOPIA_OVERCLOCK_LABEL_UK,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_UK,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_UK },
         { "2x", OPTION_VAL_2X_UK },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_UK,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_UK,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_UK },
         { "0xFF",   OPTION_VAL_0XFF_UK },
         { "random", OPTION_VAL_RANDOM_UK },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_uk = {
   option_cats_uk,
   option_defs_uk
};

/* RETRO_LANGUAGE_VAL */

#define CATEGORY_SYSTEM_LABEL_VAL "Sistema"
#define CATEGORY_SYSTEM_INFO_0_VAL NULL
#define CATEGORY_VIDEO_LABEL_VAL "Vídeo"
#define CATEGORY_VIDEO_INFO_0_VAL NULL
#define CATEGORY_AUDIO_LABEL_VAL "Àudio"
#define CATEGORY_AUDIO_INFO_0_VAL NULL
#define CATEGORY_INPUT_LABEL_VAL "Entrada"
#define CATEGORY_INPUT_INFO_0_VAL NULL
#define CATEGORY_HACKS_LABEL_VAL NULL
#define CATEGORY_HACKS_INFO_0_VAL NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_VAL NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_VAL NULL
#define OPTION_VAL_AUTO_VAL "Selecció automàtica"
#define OPTION_VAL_NTSC_VAL NULL
#define OPTION_VAL_PAL_VAL NULL
#define OPTION_VAL_FAMICOM_VAL NULL
#define OPTION_VAL_DENDY_VAL NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_VAL NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_VAL NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_VAL NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_VAL NULL
#define OPTION_VAL_COMPOSITE_VAL NULL
#define OPTION_VAL_SVIDEO_VAL NULL
#define OPTION_VAL_RGB_VAL NULL
#define OPTION_VAL_MONOCHROME_VAL NULL
#define NESTOPIA_PALETTE_LABEL_VAL NULL
#define NESTOPIA_PALETTE_INFO_0_VAL NULL
#define OPTION_VAL_CXA2025AS_VAL NULL
#define OPTION_VAL_CONSUMER_VAL NULL
#define OPTION_VAL_CANONICAL_VAL NULL
#define OPTION_VAL_ALTERNATIVE_VAL NULL
#define OPTION_VAL_RGB_O3_VAL NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_VAL NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_VAL NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_VAL NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_VAL NULL
#define OPTION_VAL_RAW_VAL NULL
#define OPTION_VAL_CUSTOM_VAL NULL
#define NESTOPIA_OVERSCAN_V_LABEL_VAL NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_VAL NULL
#define NESTOPIA_OVERSCAN_H_LABEL_VAL NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_VAL NULL
#define NESTOPIA_ASPECT_LABEL_VAL NULL
#define NESTOPIA_ASPECT_INFO_0_VAL NULL
#define OPTION_VAL_4_3_VAL NULL
#define OPTION_VAL_UNCORRECTED_VAL NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_VAL NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_VAL NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_VAL NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_VAL NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_VAL NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_VAL NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_VAL NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_VAL NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_VAL NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_VAL NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_VAL NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_VAL NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_VAL NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_VAL NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_VAL NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_VAL NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_VAL NULL
#define OPTION_VAL_MONO_VAL NULL
#define OPTION_VAL_STEREO_VAL NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_VAL NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_VAL NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_VAL NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_VAL NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_VAL NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_VAL NULL
#define OPTION_VAL_MOUSE_VAL NULL
#define OPTION_VAL_POINTER_VAL NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_VAL NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_VAL NULL
#define OPTION_VAL_LIGHTGUN_VAL NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_VAL NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_VAL NULL
#define NESTOPIA_TURBO_PULSE_LABEL_VAL NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_VAL NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_VAL NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_VAL NULL
#define NESTOPIA_OVERCLOCK_LABEL_VAL NULL
#define NESTOPIA_OVERCLOCK_INFO_0_VAL NULL
#define OPTION_VAL_1X_VAL NULL
#define OPTION_VAL_2X_VAL NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_VAL NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_VAL NULL
#define OPTION_VAL_0X00_VAL NULL
#define OPTION_VAL_0XFF_VAL NULL
#define OPTION_VAL_RANDOM_VAL NULL

struct retro_core_option_v2_category option_cats_val[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_VAL,
      CATEGORY_SYSTEM_INFO_0_VAL
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_VAL,
      CATEGORY_VIDEO_INFO_0_VAL
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_VAL,
      CATEGORY_AUDIO_INFO_0_VAL
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_VAL,
      CATEGORY_INPUT_INFO_0_VAL
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_VAL,
      CATEGORY_HACKS_INFO_0_VAL
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_val[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_VAL,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_VAL,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_VAL },
         { "ntsc",    OPTION_VAL_NTSC_VAL },
         { "pal",     OPTION_VAL_PAL_VAL },
         { "famicom", OPTION_VAL_FAMICOM_VAL },
         { "dendy",   OPTION_VAL_DENDY_VAL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_VAL,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_VAL,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_VAL,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_VAL,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_VAL },
         { "svideo",     OPTION_VAL_SVIDEO_VAL },
         { "rgb",        OPTION_VAL_RGB_VAL },
         { "monochrome", OPTION_VAL_MONOCHROME_VAL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_VAL,
      NULL,
      NESTOPIA_PALETTE_INFO_0_VAL,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_VAL },
         { "consumer",             OPTION_VAL_CONSUMER_VAL },
         { "canonical",            OPTION_VAL_CANONICAL_VAL },
         { "alternative",          OPTION_VAL_ALTERNATIVE_VAL },
         { "rgb",                  OPTION_VAL_RGB_O3_VAL },
         { "pal",                  OPTION_VAL_PAL_VAL },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_VAL },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_VAL },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_VAL },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_VAL },
         { "raw",                  OPTION_VAL_RAW_VAL },
         { "custom",               OPTION_VAL_CUSTOM_VAL },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_VAL,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_VAL,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_VAL,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_VAL,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_VAL,
      NULL,
      NESTOPIA_ASPECT_INFO_0_VAL,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_VAL },
         { "ntsc",        OPTION_VAL_NTSC_VAL },
         { "pal",         OPTION_VAL_PAL_VAL },
         { "4:3",         OPTION_VAL_4_3_VAL },
         { "uncorrected", OPTION_VAL_UNCORRECTED_VAL },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_VAL,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_VAL,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_VAL,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_VAL,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_VAL,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_VAL,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_VAL,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_VAL,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_VAL,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_VAL,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_VAL,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_VAL,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_VAL,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_VAL,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_VAL,
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
      NESTOPIA_AUDIO_TYPE_LABEL_VAL,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_VAL,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_VAL },
         { "stereo", OPTION_VAL_STEREO_VAL },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_VAL,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_VAL,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_VAL },
         { "ntsc",    OPTION_VAL_NTSC_VAL },
         { "famicom", OPTION_VAL_FAMICOM_VAL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_VAL,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_VAL, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_VAL,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_VAL,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_VAL },
         { "pointer", OPTION_VAL_POINTER_VAL },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_VAL,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_VAL,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_VAL },
         { "mouse", OPTION_VAL_MOUSE_VAL },
         { "pointer", OPTION_VAL_POINTER_VAL },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_VAL,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_VAL,
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
      NESTOPIA_TURBO_PULSE_LABEL_VAL,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_VAL,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_VAL,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_VAL,
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
      NESTOPIA_OVERCLOCK_LABEL_VAL,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_VAL,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_VAL },
         { "2x", OPTION_VAL_2X_VAL },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_VAL,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_VAL,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_VAL },
         { "0xFF",   OPTION_VAL_0XFF_VAL },
         { "random", OPTION_VAL_RANDOM_VAL },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_val = {
   option_cats_val,
   option_defs_val
};

/* RETRO_LANGUAGE_VN */

#define CATEGORY_SYSTEM_LABEL_VN "Hệ thống"
#define CATEGORY_SYSTEM_INFO_0_VN NULL
#define CATEGORY_VIDEO_LABEL_VN "Hình ảnh"
#define CATEGORY_VIDEO_INFO_0_VN NULL
#define CATEGORY_AUDIO_LABEL_VN "Âm thanh"
#define CATEGORY_AUDIO_INFO_0_VN NULL
#define CATEGORY_INPUT_LABEL_VN "Input Driver"
#define CATEGORY_INPUT_INFO_0_VN NULL
#define CATEGORY_HACKS_LABEL_VN "Hacks giả lập"
#define CATEGORY_HACKS_INFO_0_VN NULL
#define NESTOPIA_FAVORED_SYSTEM_LABEL_VN NULL
#define NESTOPIA_FAVORED_SYSTEM_INFO_0_VN NULL
#define OPTION_VAL_AUTO_VN "Tự động"
#define OPTION_VAL_NTSC_VN NULL
#define OPTION_VAL_PAL_VN NULL
#define OPTION_VAL_FAMICOM_VN NULL
#define OPTION_VAL_DENDY_VN NULL
#define NESTOPIA_FDS_AUTO_INSERT_LABEL_VN NULL
#define NESTOPIA_FDS_AUTO_INSERT_INFO_0_VN NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_LABEL_VN NULL
#define NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_VN NULL
#define OPTION_VAL_COMPOSITE_VN NULL
#define OPTION_VAL_SVIDEO_VN NULL
#define OPTION_VAL_RGB_VN NULL
#define OPTION_VAL_MONOCHROME_VN NULL
#define NESTOPIA_PALETTE_LABEL_VN NULL
#define NESTOPIA_PALETTE_INFO_0_VN NULL
#define OPTION_VAL_CXA2025AS_VN NULL
#define OPTION_VAL_CONSUMER_VN NULL
#define OPTION_VAL_CANONICAL_VN NULL
#define OPTION_VAL_ALTERNATIVE_VN NULL
#define OPTION_VAL_RGB_O3_VN NULL
#define OPTION_VAL_COMPOSITE_DIRECT_FBX_VN NULL
#define OPTION_VAL_PVM_STYLE_D93_FBX_VN NULL
#define OPTION_VAL_NTSC_HARDWARE_FBX_VN NULL
#define OPTION_VAL_NES_CLASSIC_FBX_FS_VN NULL
#define OPTION_VAL_RAW_VN NULL
#define OPTION_VAL_CUSTOM_VN "Tùy chỉnh"
#define NESTOPIA_OVERSCAN_V_LABEL_VN NULL
#define NESTOPIA_OVERSCAN_V_INFO_0_VN NULL
#define NESTOPIA_OVERSCAN_H_LABEL_VN NULL
#define NESTOPIA_OVERSCAN_H_INFO_0_VN NULL
#define NESTOPIA_ASPECT_LABEL_VN NULL
#define NESTOPIA_ASPECT_INFO_0_VN NULL
#define OPTION_VAL_4_3_VN NULL
#define OPTION_VAL_UNCORRECTED_VN NULL
#define NESTOPIA_GENIE_DISTORTION_LABEL_VN NULL
#define NESTOPIA_GENIE_DISTORTION_INFO_0_VN NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_VN NULL
#define NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_VN NULL
#define NESTOPIA_AUDIO_VOL_SQ1_LABEL_VN NULL
#define NESTOPIA_AUDIO_VOL_SQ1_INFO_0_VN NULL
#define NESTOPIA_AUDIO_VOL_SQ2_LABEL_VN NULL
#define NESTOPIA_AUDIO_VOL_SQ2_INFO_0_VN NULL
#define NESTOPIA_AUDIO_VOL_TRI_LABEL_VN NULL
#define NESTOPIA_AUDIO_VOL_TRI_INFO_0_VN NULL
#define NESTOPIA_AUDIO_VOL_NOISE_LABEL_VN NULL
#define NESTOPIA_AUDIO_VOL_NOISE_INFO_0_VN NULL
#define NESTOPIA_AUDIO_VOL_DPCM_LABEL_VN NULL
#define NESTOPIA_AUDIO_VOL_DPCM_INFO_0_VN NULL
#define NESTOPIA_AUDIO_VOL_FDS_LABEL_VN NULL
#define NESTOPIA_AUDIO_VOL_FDS_INFO_0_VN NULL
#define NESTOPIA_AUDIO_VOL_MMC5_LABEL_VN NULL
#define NESTOPIA_AUDIO_VOL_MMC5_INFO_0_VN NULL
#define NESTOPIA_AUDIO_VOL_VRC6_LABEL_VN NULL
#define NESTOPIA_AUDIO_VOL_VRC6_INFO_0_VN NULL
#define NESTOPIA_AUDIO_VOL_VRC7_LABEL_VN NULL
#define NESTOPIA_AUDIO_VOL_VRC7_INFO_0_VN NULL
#define NESTOPIA_AUDIO_VOL_N163_LABEL_VN NULL
#define NESTOPIA_AUDIO_VOL_N163_INFO_0_VN NULL
#define NESTOPIA_AUDIO_VOL_S5B_LABEL_VN NULL
#define NESTOPIA_AUDIO_VOL_S5B_INFO_0_VN NULL
#define NESTOPIA_AUDIO_TYPE_LABEL_VN NULL
#define NESTOPIA_AUDIO_TYPE_INFO_0_VN NULL
#define OPTION_VAL_MONO_VN NULL
#define OPTION_VAL_STEREO_VN NULL
#define NESTOPIA_SELECT_ADAPTER_LABEL_VN NULL
#define NESTOPIA_SELECT_ADAPTER_INFO_0_VN NULL
#define NESTOPIA_BUTTON_SHIFT_LABEL_VN NULL
#define NESTOPIA_BUTTON_SHIFT_INFO_0_VN NULL
#define NESTOPIA_ARKANOID_DEVICE_LABEL_VN NULL
#define NESTOPIA_ARKANOID_DEVICE_INFO_0_VN NULL
#define OPTION_VAL_MOUSE_VN NULL
#define OPTION_VAL_POINTER_VN NULL
#define NESTOPIA_ZAPPER_DEVICE_LABEL_VN NULL
#define NESTOPIA_ZAPPER_DEVICE_INFO_0_VN NULL
#define OPTION_VAL_LIGHTGUN_VN NULL
#define NESTOPIA_SHOW_CROSSHAIR_LABEL_VN NULL
#define NESTOPIA_SHOW_CROSSHAIR_INFO_0_VN NULL
#define NESTOPIA_TURBO_PULSE_LABEL_VN NULL
#define NESTOPIA_TURBO_PULSE_INFO_0_VN NULL
#define NESTOPIA_NOSPRITELIMIT_LABEL_VN NULL
#define NESTOPIA_NOSPRITELIMIT_INFO_0_VN NULL
#define NESTOPIA_OVERCLOCK_LABEL_VN NULL
#define NESTOPIA_OVERCLOCK_INFO_0_VN NULL
#define OPTION_VAL_1X_VN NULL
#define OPTION_VAL_2X_VN NULL
#define NESTOPIA_RAM_POWER_STATE_LABEL_VN NULL
#define NESTOPIA_RAM_POWER_STATE_INFO_0_VN NULL
#define OPTION_VAL_0X00_VN NULL
#define OPTION_VAL_0XFF_VN NULL
#define OPTION_VAL_RANDOM_VN NULL

struct retro_core_option_v2_category option_cats_vn[] = {
   {
      "system",
      CATEGORY_SYSTEM_LABEL_VN,
      CATEGORY_SYSTEM_INFO_0_VN
   },
   {
      "video",
      CATEGORY_VIDEO_LABEL_VN,
      CATEGORY_VIDEO_INFO_0_VN
   },
   {
      "audio",
      CATEGORY_AUDIO_LABEL_VN,
      CATEGORY_AUDIO_INFO_0_VN
   },
   {
      "input",
      CATEGORY_INPUT_LABEL_VN,
      CATEGORY_INPUT_INFO_0_VN
   },
   {
      "hacks",
      CATEGORY_HACKS_LABEL_VN,
      CATEGORY_HACKS_INFO_0_VN
   },
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_vn[] = {

/* System */

   {
      "nestopia_favored_system",
      NESTOPIA_FAVORED_SYSTEM_LABEL_VN,
      NULL,
      NESTOPIA_FAVORED_SYSTEM_INFO_0_VN,
      NULL,
      "system",
      {
         { "auto",    OPTION_VAL_AUTO_VN },
         { "ntsc",    OPTION_VAL_NTSC_VN },
         { "pal",     OPTION_VAL_PAL_VN },
         { "famicom", OPTION_VAL_FAMICOM_VN },
         { "dendy",   OPTION_VAL_DENDY_VN },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_fds_auto_insert",
      NESTOPIA_FDS_AUTO_INSERT_LABEL_VN,
      NULL,
      NESTOPIA_FDS_AUTO_INSERT_INFO_0_VN,
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },

/* Video */

   {
      "nestopia_blargg_ntsc_filter",
      NESTOPIA_BLARGG_NTSC_FILTER_LABEL_VN,
      NULL,
      NESTOPIA_BLARGG_NTSC_FILTER_INFO_0_VN,
      NULL,
      "video",
      {
         { "disabled",   NULL },
         { "composite",  OPTION_VAL_COMPOSITE_VN },
         { "svideo",     OPTION_VAL_SVIDEO_VN },
         { "rgb",        OPTION_VAL_RGB_VN },
         { "monochrome", OPTION_VAL_MONOCHROME_VN },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_palette",
      NESTOPIA_PALETTE_LABEL_VN,
      NULL,
      NESTOPIA_PALETTE_INFO_0_VN,
      NULL,
      "video",
      {
         { "cxa2025as",            OPTION_VAL_CXA2025AS_VN },
         { "consumer",             OPTION_VAL_CONSUMER_VN },
         { "canonical",            OPTION_VAL_CANONICAL_VN },
         { "alternative",          OPTION_VAL_ALTERNATIVE_VN },
         { "rgb",                  OPTION_VAL_RGB_O3_VN },
         { "pal",                  OPTION_VAL_PAL_VN },
         { "composite-direct-fbx", OPTION_VAL_COMPOSITE_DIRECT_FBX_VN },
         { "pvm-style-d93-fbx",    OPTION_VAL_PVM_STYLE_D93_FBX_VN },
         { "ntsc-hardware-fbx",    OPTION_VAL_NTSC_HARDWARE_FBX_VN },
         { "nes-classic-fbx-fs",   OPTION_VAL_NES_CLASSIC_FBX_FS_VN },
         { "raw",                  OPTION_VAL_RAW_VN },
         { "custom",               OPTION_VAL_CUSTOM_VN },
         { NULL, NULL },
      },
      "cxa2025as" /* TODO/FIXME - is this correct ? */
   },
   {
      "nestopia_overscan_v",
      NESTOPIA_OVERSCAN_V_LABEL_VN,
      NULL,
      NESTOPIA_OVERSCAN_V_INFO_0_VN,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "nestopia_overscan_h",
      NESTOPIA_OVERSCAN_H_LABEL_VN,
      NULL,
      NESTOPIA_OVERSCAN_H_INFO_0_VN,
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "nestopia_aspect",
      NESTOPIA_ASPECT_LABEL_VN,
      NULL,
      NESTOPIA_ASPECT_INFO_0_VN,
      NULL,
      "video",
      {
         { "auto",        OPTION_VAL_AUTO_VN },
         { "ntsc",        OPTION_VAL_NTSC_VN },
         { "pal",         OPTION_VAL_PAL_VN },
         { "4:3",         OPTION_VAL_4_3_VN },
         { "uncorrected", OPTION_VAL_UNCORRECTED_VN },
         { NULL, NULL },
      },
      "auto"
   },

/* Audio */

   {
      "nestopia_genie_distortion",
      NESTOPIA_GENIE_DISTORTION_LABEL_VN,
      NULL,
      NESTOPIA_GENIE_DISTORTION_INFO_0_VN,
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
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_LABEL_VN,
      NULL,
      NESTOPIA_SHOW_ADVANCED_AV_SETTINGS_INFO_0_VN,
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
      NESTOPIA_AUDIO_VOL_SQ1_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ1_INFO_0_VN,
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
      NESTOPIA_AUDIO_VOL_SQ2_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_VOL_SQ2_INFO_0_VN,
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
      NESTOPIA_AUDIO_VOL_TRI_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_VOL_TRI_INFO_0_VN,
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
      NESTOPIA_AUDIO_VOL_NOISE_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_VOL_NOISE_INFO_0_VN,
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
      NESTOPIA_AUDIO_VOL_DPCM_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_VOL_DPCM_INFO_0_VN,
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
      NESTOPIA_AUDIO_VOL_FDS_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_VOL_FDS_INFO_0_VN,
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
      NESTOPIA_AUDIO_VOL_MMC5_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_VOL_MMC5_INFO_0_VN,
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
      NESTOPIA_AUDIO_VOL_VRC6_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC6_INFO_0_VN,
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
      NESTOPIA_AUDIO_VOL_VRC7_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_VOL_VRC7_INFO_0_VN,
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
      NESTOPIA_AUDIO_VOL_N163_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_VOL_N163_INFO_0_VN,
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
      NESTOPIA_AUDIO_VOL_S5B_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_VOL_S5B_INFO_0_VN,
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
      NESTOPIA_AUDIO_TYPE_LABEL_VN,
      NULL,
      NESTOPIA_AUDIO_TYPE_INFO_0_VN,
      NULL,
      "audio",
      {
         { "mono", OPTION_VAL_MONO_VN },
         { "stereo", OPTION_VAL_STEREO_VN },
      },
      "stereo"
   },

/* Input */

   {
      "nestopia_select_adapter",
      NESTOPIA_SELECT_ADAPTER_LABEL_VN,
      NULL,
      NESTOPIA_SELECT_ADAPTER_INFO_0_VN,
      NULL,
      "input",
      {
         { "auto",    OPTION_VAL_AUTO_VN },
         { "ntsc",    OPTION_VAL_NTSC_VN },
         { "famicom", OPTION_VAL_FAMICOM_VN },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "nestopia_button_shift",
      NESTOPIA_BUTTON_SHIFT_LABEL_VN,
      NULL,
      NESTOPIA_BUTTON_SHIFT_INFO_0_VN, /* is this right? @gadsby */
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
      NESTOPIA_ARKANOID_DEVICE_LABEL_VN,
      NULL,
      NESTOPIA_ARKANOID_DEVICE_INFO_0_VN,
      NULL,
      "input",
      {
         { "mouse", OPTION_VAL_MOUSE_VN },
         { "pointer", OPTION_VAL_POINTER_VN },
      },
      "mouse"
   },
   {
      "nestopia_zapper_device",
      NESTOPIA_ZAPPER_DEVICE_LABEL_VN,
      NULL,
      NESTOPIA_ZAPPER_DEVICE_INFO_0_VN,
      NULL,
      "input",
      {
         { "lightgun", OPTION_VAL_LIGHTGUN_VN },
         { "mouse", OPTION_VAL_MOUSE_VN },
         { "pointer", OPTION_VAL_POINTER_VN },
         { NULL, NULL },
      },
      "lightgun",
   },
   {
      "nestopia_show_crosshair",
      NESTOPIA_SHOW_CROSSHAIR_LABEL_VN,
      NULL,
      NESTOPIA_SHOW_CROSSHAIR_INFO_0_VN,
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
      NESTOPIA_TURBO_PULSE_LABEL_VN,
      NULL,
      NESTOPIA_TURBO_PULSE_INFO_0_VN,
      NULL,
      "input",
      {
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
      NESTOPIA_NOSPRITELIMIT_LABEL_VN,
      NULL,
      NESTOPIA_NOSPRITELIMIT_INFO_0_VN,
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
      NESTOPIA_OVERCLOCK_LABEL_VN,
      NULL,
      NESTOPIA_OVERCLOCK_INFO_0_VN,
      NULL,
      "hacks",
      {
         { "1x", OPTION_VAL_1X_VN },
         { "2x", OPTION_VAL_2X_VN },
         { NULL, NULL },
      },
      "1x"
   },
   {
      "nestopia_ram_power_state",
      NESTOPIA_RAM_POWER_STATE_LABEL_VN,
      NULL,
      NESTOPIA_RAM_POWER_STATE_INFO_0_VN,
      NULL,
      "hacks",
      {
         { "0x00",   OPTION_VAL_0X00_VN },
         { "0xFF",   OPTION_VAL_0XFF_VN },
         { "random", OPTION_VAL_RANDOM_VN },
         { NULL, NULL },
      },
      "0x00"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_vn = {
   option_cats_vn,
   option_defs_vn
};


#ifdef __cplusplus
}
#endif

#endif