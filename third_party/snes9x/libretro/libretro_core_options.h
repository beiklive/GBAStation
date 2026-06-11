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
      "hacks",
      "Emulation Hacks",
      "Configure processor overclocking and emulation accuracy parameters affecting low-level performance and compatibility."
   },
   {
      "lightgun",
      "Light Gun",
      "Configure Super Scope / Justifier / M.A.C.S. rifle input."
   },
   {
      "advanced_av",
      "Advanced Audio/Video Settings",
      "Configure low-level video layer / GFX effect / audio channel parameters."
   },
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {

   /* These variable names and possible values constitute an ABI with ZMZ (ZSNES Libretro player).
    * Changing "Show layer 1" is fine, but don't change "layer_1"/etc or the possible values ("Yes|No").
    * Adding more variables and rearranging them is safe. */

   {
      "snes9x_region",
      "Console Region (Reload Core)",
      NULL,
      "Specify which region the system is from. 'PAL' is 50hz, 'NTSC' is 60hz. Games will run faster or slower than normal if the incorrect region is selected.",
      NULL,
      NULL,
      {
         { "auto", "Auto" },
         { "ntsc", "NTSC" },
         { "pal",  "PAL" },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "snes9x_aspect",
      "Preferred Aspect Ratio",
      NULL,
      "Choose the preferred content aspect ratio. This will only apply when RetroArch's aspect ratio is set to 'Core provided' in the Video settings.",
      NULL,
      NULL,
      {
         { "4:3",         NULL },
         { "4:3 scaled",  "4:3 (Preserved)" },
         { "uncorrected", "Uncorrected" },
         { "auto",        "Auto" },
         { "ntsc",        "NTSC" },
         { "pal",         "PAL" },
         { NULL, NULL },
      },
      "4:3"
   },
   {
      "snes9x_overscan",
      "Crop Overscan",
      NULL,
      "Remove the borders at the top and bottom of the screen, typically unused by games and hidden by the bezel of a standard-definition television. 'Auto' will attempt to detect and crop the ~8 pixel overscan based on the current content.",
      NULL,
      NULL,
      {
         { "enabled",     "~8 Pixels"},
         { "12_pixels",   "12 Pixels" },
         { "16_pixels",   "16 Pixels" },
         { "auto",        "Auto (~8 Pixels)" },
         { "disabled",    NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "snes9x_hires_blend",
      "Hi-Res Blending",
      NULL,
      "Blend adjacent pixels when game switches to hi-res mode (512x448). Required for certain games that use hi-res mode to produce transparency effects (Kirby's Dream Land, Jurassic Park...).",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "merge",    "Merge" },
         { "blur",     "Blur" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "snes9x_blargg",
      "Blargg NTSC Filter",
      NULL,
      "Apply a video filter to mimic various NTSC TV signals.",
      NULL,
      NULL,
      {
         { "disabled",   NULL },
         { "monochrome", "Monochrome" },
         { "rf",         "RF" },
         { "composite",  "Composite" },
         { "s-video",    "S-Video" },
         { "rgb",        "RGB" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "snes9x_audio_interpolation",
      "Audio Interpolation",
      NULL,
      "Apply an audio filter. 'Gaussian' reproduces the bass-heavy sound of the original hardware. 'Cubic' and 'Sinc' are less accurate, and preserve more of the high range.",
      NULL,
      NULL,
      {
         { "gaussian", "Gaussian" },
         { "cubic",    "Cubic" },
         { "sinc",     "Sinc" },
         { "none",     "None" },
         { "linear",   "Linear" },
         { NULL, NULL },
      },
      "gaussian"
   },
   {
      "snes9x_up_down_allowed",
      "Allow Opposing Directions",
      NULL,
      "Enabling this will allow pressing / quickly alternating / holding both left and right (or up and down) directions at the same time. This may cause movement-based glitches.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "snes9x_overclock_superfx",
      "SuperFX Overclocking",
      NULL,
      "SuperFX coprocessor frequency multiplier. Can improve frame rate or cause timing errors. Values under 100% can improve game performance on slow devices.",
      NULL,
      "hacks",
      {
         { "50%",  NULL },
         { "60%",  NULL },
         { "70%",  NULL },
         { "80%",  NULL },
         { "90%",  NULL },
         { "100%", NULL },
         { "150%", NULL },
         { "200%", NULL },
         { "250%", NULL },
         { "300%", NULL },
         { "350%", NULL },
         { "400%", NULL },
         { "450%", NULL },
         { "500%", NULL },
         { NULL, NULL },
      },
      "100%"
   },
   {
      "snes9x_overclock_cycles",
      "Reduce Slowdown (Hack, Unsafe)",
      "Reduce Slowdown (Unsafe)",
      "Overclock SNES CPU. May cause games to crash! Use 'Light' for shorter loading times, 'Compatible' for most games exhibiting slowdown and 'Max' only if absolutely required (Gradius 3, Super R-type...).",
      NULL,
      "hacks",
      {
         { "disabled",   NULL },
         { "light",      "Light" },
         { "compatible", "Compatible" },
         { "max",        "Max" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "snes9x_reduce_sprite_flicker",
      "Reduce Flickering (Hack, Unsafe)",
      "Reduce Flickering (Unsafe)",
      "Increases number of sprites that can be drawn simultaneously on screen.",
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
      "snes9x_randomize_memory",
      "Randomize Memory (Unsafe)",
      NULL,
      "Randomizes system RAM upon start-up. Some games such as 'Super Off Road' use system RAM as a random number generator for item placement and AI behavior, to make gameplay more unpredictable.",
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
      "snes9x_block_invalid_vram_access",
      "Block Invalid VRAM Access",
      NULL,
      "Some homebrew/ROM hacks require this option to be disabled for correct operation.",
      NULL,
      "hacks",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "snes9x_echo_buffer_hack",
      "Echo Buffer Hack (Unsafe, only enable for old addmusic hacks)",
      NULL,
      "Some homebrew/ROM hacks require this option to be enabled for correct operation.",
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
      "snes9x_show_lightgun_settings",
      "Show Light Gun Settings",
      NULL,
      "Enable configuration of Super Scope / Justifier / M.A.C.S. rifle input. NOTE: Quick Menu must be toggled for this setting to take effect.",
      NULL,
      NULL,
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "snes9x_lightgun_mode",
      "Light Gun Mode",
      "Mode",
      "Use a mouse-controlled 'Light Gun' or 'Touchscreen' input.",
      NULL,
      "lightgun",
      {
         { "Lightgun",    "Light Gun" },
         { "Touchscreen", NULL },
         { NULL, NULL },
      },
      "Lightgun"
   },
   {
      "snes9x_superscope_reverse_buttons",
      "Super Scope Reverse Trigger Buttons",
      NULL,
      "Swap the positions of the Super Scope 'Fire' and 'Cursor' buttons.",
      NULL,
      "lightgun",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "snes9x_superscope_crosshair",
      "Super Scope Crosshair",
      NULL,
      "Change the crosshair size on screen.",
      NULL,
      "lightgun",
      {
         { "0",  NULL },
         { "1",  NULL },
         { "2",  NULL },
         { "3",  NULL },
         { "4",  NULL },
         { "5",  NULL },
         { "6",  NULL },
         { "7",  NULL },
         { "8",  NULL },
         { "9",  NULL },
         { "10", NULL },
         { "11", NULL },
         { "12", NULL },
         { "13", NULL },
         { "14", NULL },
         { "15", NULL },
         { "16", NULL },
         { NULL, NULL },
      },
      "2"
   },
   {
      "snes9x_superscope_color",
      "Super Scope Color",
      NULL,
      "Change the crosshair color on screen.",
      NULL,
      "lightgun",
      {
         { "White",            NULL },
         { "White (blend)",    NULL },
         { "Red",              NULL },
         { "Red (blend)",      NULL },
         { "Orange",           NULL },
         { "Orange (blend)",   NULL },
         { "Yellow",           NULL },
         { "Yellow (blend)",   NULL },
         { "Green",            NULL },
         { "Green (blend)",    NULL },
         { "Cyan",             NULL },
         { "Cyan (blend)",     NULL },
         { "Sky",              NULL },
         { "Sky (blend)",      NULL },
         { "Blue",             NULL },
         { "Blue (blend)",     NULL },
         { "Violet",           NULL },
         { "Violet (blend)",   NULL },
         { "Pink",             NULL },
         { "Pink (blend)",     NULL },
         { "Purple",           NULL },
         { "Purple (blend)",   NULL },
         { "Black",            NULL },
         { "Black (blend)",    NULL },
         { "25% Grey",         NULL },
         { "25% Grey (blend)", NULL },
         { "50% Grey",         NULL },
         { "50% Grey (blend)", NULL },
         { "75% Grey",         NULL },
         { "75% Grey (blend)", NULL },
         { NULL, NULL },
      },
      "White"
   },
   {
      "snes9x_justifier1_crosshair",
      "Justifier 1 Crosshair",
      NULL,
      "Change the crosshair size on screen.",
      NULL,
      "lightgun",
      {
         { "0",  NULL },
         { "1",  NULL },
         { "2",  NULL },
         { "3",  NULL },
         { "4",  NULL },
         { "5",  NULL },
         { "6",  NULL },
         { "7",  NULL },
         { "8",  NULL },
         { "9",  NULL },
         { "10", NULL },
         { "11", NULL },
         { "12", NULL },
         { "13", NULL },
         { "14", NULL },
         { "15", NULL },
         { "16", NULL },
         { NULL, NULL },
      },
      "4"
   },
   {
      "snes9x_justifier1_color",
      "Justifier 1 Color",
      NULL,
      "Change the crosshair color on screen.",
      NULL,
      "lightgun",
      {
         { "Blue",             NULL },
         { "Blue (blend)",     NULL },
         { "Violet",           NULL },
         { "Violet (blend)",   NULL },
         { "Pink",             NULL },
         { "Pink (blend)",     NULL },
         { "Purple",           NULL },
         { "Purple (blend)",   NULL },
         { "Black",            NULL },
         { "Black (blend)",    NULL },
         { "25% Grey",         NULL },
         { "25% Grey (blend)", NULL },
         { "50% Grey",         NULL },
         { "50% Grey (blend)", NULL },
         { "75% Grey",         NULL },
         { "75% Grey (blend)", NULL },
         { "White",            NULL },
         { "White (blend)",    NULL },
         { "Red",              NULL },
         { "Red (blend)",      NULL },
         { "Orange",           NULL },
         { "Orange (blend)",   NULL },
         { "Yellow",           NULL },
         { "Yellow (blend)",   NULL },
         { "Green",            NULL },
         { "Green (blend)",    NULL },
         { "Cyan",             NULL },
         { "Cyan (blend)",     NULL },
         { "Sky",              NULL },
         { "Sky (blend)",      NULL },
         { NULL, NULL },
      },
      "Blue"
   },
   {
      "snes9x_justifier2_crosshair",
      "Justifier 2 Crosshair",
      NULL,
      "Change the crosshair size on screen.",
      NULL,
      "lightgun",
      {
         { "0",  NULL },
         { "1",  NULL },
         { "2",  NULL },
         { "3",  NULL },
         { "4",  NULL },
         { "5",  NULL },
         { "6",  NULL },
         { "7",  NULL },
         { "8",  NULL },
         { "9",  NULL },
         { "10", NULL },
         { "11", NULL },
         { "12", NULL },
         { "13", NULL },
         { "14", NULL },
         { "15", NULL },
         { "16", NULL },
         { NULL, NULL },
      },
      "4"
   },
   {
      "snes9x_justifier2_color",
      "Justifier 2 Color",
      NULL,
      "Change the crosshair color on screen.",
      NULL,
      "lightgun",
      {
         { "Pink",             NULL },
         { "Pink (blend)",     NULL },
         { "Purple",           NULL },
         { "Purple (blend)",   NULL },
         { "Black",            NULL },
         { "Black (blend)",    NULL },
         { "25% Grey",         NULL },
         { "25% Grey (blend)", NULL },
         { "50% Grey",         NULL },
         { "50% Grey (blend)", NULL },
         { "75% Grey",         NULL },
         { "75% Grey (blend)", NULL },
         { "White",            NULL },
         { "White (blend)",    NULL },
         { "Red",              NULL },
         { "Red (blend)",      NULL },
         { "Orange",           NULL },
         { "Orange (blend)",   NULL },
         { "Yellow",           NULL },
         { "Yellow (blend)",   NULL },
         { "Green",            NULL },
         { "Green (blend)",    NULL },
         { "Cyan",             NULL },
         { "Cyan (blend)",     NULL },
         { "Sky",              NULL },
         { "Sky (blend)",      NULL },
         { "Blue",             NULL },
         { "Blue (blend)",     NULL },
         { "Violet",           NULL },
         { "Violet (blend)",   NULL },
         { NULL, NULL },
      },
      "Pink"
   },
   {
      "snes9x_rifle_crosshair",
      "M.A.C.S. Rifle Crosshair",
      NULL,
      "Change the crosshair size on screen.",
      NULL,
      "lightgun",
      {
         { "0",  NULL },
         { "1",  NULL },
         { "2",  NULL },
         { "3",  NULL },
         { "4",  NULL },
         { "5",  NULL },
         { "6",  NULL },
         { "7",  NULL },
         { "8",  NULL },
         { "9",  NULL },
         { "10", NULL },
         { "11", NULL },
         { "12", NULL },
         { "13", NULL },
         { "14", NULL },
         { "15", NULL },
         { "16", NULL },
         { NULL, NULL },
      },
      "2"
   },
   {
      "snes9x_rifle_color",
      "M.A.C.S. Rifle Color",
      NULL,
      "Change the crosshair color on screen.",
      NULL,
      "lightgun",
      {
         { "White",            NULL },
         { "White (blend)",    NULL },
         { "Red",              NULL },
         { "Red (blend)",      NULL },
         { "Orange",           NULL },
         { "Orange (blend)",   NULL },
         { "Yellow",           NULL },
         { "Yellow (blend)",   NULL },
         { "Green",            NULL },
         { "Green (blend)",    NULL },
         { "Cyan",             NULL },
         { "Cyan (blend)",     NULL },
         { "Sky",              NULL },
         { "Sky (blend)",      NULL },
         { "Blue",             NULL },
         { "Blue (blend)",     NULL },
         { "Violet",           NULL },
         { "Violet (blend)",   NULL },
         { "Pink",             NULL },
         { "Pink (blend)",     NULL },
         { "Purple",           NULL },
         { "Purple (blend)",   NULL },
         { "Black",            NULL },
         { "Black (blend)",    NULL },
         { "25% Grey",         NULL },
         { "25% Grey (blend)", NULL },
         { "50% Grey",         NULL },
         { "50% Grey (blend)", NULL },
         { "75% Grey",         NULL },
         { "75% Grey (blend)", NULL },
         { NULL, NULL },
      },
      "White"
   },
   {
      "snes9x_show_advanced_av_settings",
      "Show Advanced Audio/Video Settings",
      NULL,
      "Enable configuration of low-level video layer / GFX effect / audio channel parameters. NOTE: Quick Menu must be toggled for this setting to take effect.",
      NULL,
      NULL,
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "snes9x_layer_1",
      "Show Layer 1",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "snes9x_layer_2",
      "Show Layer 2",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "snes9x_layer_3",
      "Show Layer 3",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "snes9x_layer_4",
      "Show Layer 4",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "snes9x_layer_5",
      "Show Sprite Layer",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "snes9x_gfx_clip",
      "Enable Graphic Clip Windows",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "snes9x_gfx_transp",
      "Enable Transparency Effects",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "snes9x_sndchan_volume_1",
      "Volume % for Sound Channel 1",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "0",   NULL },
         { "10",  NULL },
         { "20",  NULL },
         { "30",  NULL },
         { "40",  NULL },
         { "50",  NULL },
         { "60",  NULL },
         { "70",  NULL },
         { "80",  NULL },
         { "90",  NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "snes9x_sndchan_volume_2",
      "Volume % for Sound Channel 2",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "0",   NULL },
         { "10",  NULL },
         { "20",  NULL },
         { "30",  NULL },
         { "40",  NULL },
         { "50",  NULL },
         { "60",  NULL },
         { "70",  NULL },
         { "80",  NULL },
         { "90",  NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "snes9x_sndchan_volume_3",
      "Volume % for Sound Channel 3",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "0",   NULL },
         { "10",  NULL },
         { "20",  NULL },
         { "30",  NULL },
         { "40",  NULL },
         { "50",  NULL },
         { "60",  NULL },
         { "70",  NULL },
         { "80",  NULL },
         { "90",  NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "snes9x_sndchan_volume_4",
      "Volume % for Sound Channel 4",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "0",   NULL },
         { "10",  NULL },
         { "20",  NULL },
         { "30",  NULL },
         { "40",  NULL },
         { "50",  NULL },
         { "60",  NULL },
         { "70",  NULL },
         { "80",  NULL },
         { "90",  NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "snes9x_sndchan_volume_5",
      "Volume % for Sound Channel 5",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "0",   NULL },
         { "10",  NULL },
         { "20",  NULL },
         { "30",  NULL },
         { "40",  NULL },
         { "50",  NULL },
         { "60",  NULL },
         { "70",  NULL },
         { "80",  NULL },
         { "90",  NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "snes9x_sndchan_volume_6",
      "Volume % for Sound Channel 6",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "0",   NULL },
         { "10",  NULL },
         { "20",  NULL },
         { "30",  NULL },
         { "40",  NULL },
         { "50",  NULL },
         { "60",  NULL },
         { "70",  NULL },
         { "80",  NULL },
         { "90",  NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "snes9x_sndchan_volume_7",
      "Volume % for Sound Channel 7",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "0",   NULL },
         { "10",  NULL },
         { "20",  NULL },
         { "30",  NULL },
         { "40",  NULL },
         { "50",  NULL },
         { "60",  NULL },
         { "70",  NULL },
         { "80",  NULL },
         { "90",  NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "snes9x_sndchan_volume_8",
      "Volume % for Sound Channel 8",
      NULL,
      NULL,
      NULL,
      "advanced_av",
      {
         { "0",   NULL },
         { "10",  NULL },
         { "20",  NULL },
         { "30",  NULL },
         { "40",  NULL },
         { "50",  NULL },
         { "60",  NULL },
         { "70",  NULL },
         { "80",  NULL },
         { "90",  NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
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
   &options_us, /* RETRO_LANGUAGE_ENGLISH */
   NULL,        /* RETRO_LANGUAGE_JAPANESE */
   NULL,        /* RETRO_LANGUAGE_FRENCH */
   NULL,        /* RETRO_LANGUAGE_SPANISH */
   NULL,        /* RETRO_LANGUAGE_GERMAN */
   NULL,        /* RETRO_LANGUAGE_ITALIAN */
   NULL,        /* RETRO_LANGUAGE_DUTCH */
   NULL,        /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,        /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,        /* RETRO_LANGUAGE_RUSSIAN */
   NULL,        /* RETRO_LANGUAGE_KOREAN */
   NULL,        /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,        /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,        /* RETRO_LANGUAGE_ESPERANTO */
   NULL,        /* RETRO_LANGUAGE_POLISH */
   NULL,        /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,        /* RETRO_LANGUAGE_ARABIC */
   NULL,        /* RETRO_LANGUAGE_GREEK */
   &options_tr, /* RETRO_LANGUAGE_TURKISH */
   NULL,        /* RETRO_LANGUAGE_SLOVAK */
   NULL,        /* RETRO_LANGUAGE_PERSIAN */
   NULL,        /* RETRO_LANGUAGE_HEBREW */
   NULL,        /* RETRO_LANGUAGE_ASTURIAN */
   NULL,        /* RETRO_LANGUAGE_FINNISH */
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

            /* Skip options that are irrelevant when using the
             * old style core options interface */
            if ((strcmp(key, "snes9x_show_lightgun_settings") == 0) ||
                (strcmp(key, "snes9x_show_advanced_av_settings") == 0))
               continue;

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
