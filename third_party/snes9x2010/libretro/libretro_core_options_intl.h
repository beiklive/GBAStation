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

/* RETRO_LANGUAGE_JAPANESE */

/* RETRO_LANGUAGE_FRENCH */

/* RETRO_LANGUAGE_SPANISH */

/* RETRO_LANGUAGE_GERMAN */

/* RETRO_LANGUAGE_ITALIAN */

/* RETRO_LANGUAGE_DUTCH */

/* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */

/* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */

/* RETRO_LANGUAGE_RUSSIAN */

/* RETRO_LANGUAGE_KOREAN */

/* RETRO_LANGUAGE_CHINESE_TRADITIONAL */

/* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */

/* RETRO_LANGUAGE_ESPERANTO */

/* RETRO_LANGUAGE_POLISH */

/* RETRO_LANGUAGE_VIETNAMESE */

/* RETRO_LANGUAGE_ARABIC */

/* RETRO_LANGUAGE_GREEK */

/* RETRO_LANGUAGE_TURKISH */

struct retro_core_option_v2_category option_cats_tr[] = {
   {
      "hacks",
      NULL,
      NULL
   },
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_tr[] = {

   /* These variable names and possible values constitute an ABI with ZMZ (ZSNES Libretro player).
    * Changing "Show layer 1" is fine, but don't change "layer_1"/etc or the possible values ("Yes|No").
    * Adding more variables and rearranging them is safe. */
   {
      "snes9x_2010_overclock",
      "SuperFX Hız Aşırtma",
      NULL,
      "SuperFX işlemcisi frekans çarpanıdır. Kare hızını artırabilir veya zamanlama hatalarına neden olabilir. 10 altındaki değerler yavaş cihazlarda oyun performansını artırabilir.",
      NULL,
      NULL,
      {
         { "40 MHz", NULL },
         { "30 MHz", NULL },
         { "20 MHz", NULL },
         { "15 MHz", NULL },
         { "14 MHz", NULL },
         { "13 MHz", NULL },
         { "12 MHz", NULL },
         { "11 MHz", NULL },
         { "10 MHz (Default)",  NULL },
         { "9 MHz (Underclock)", NULL },
         { "8 MHz (Underclock)", NULL },
         { "5 MHz (Underclock)", NULL },
         { NULL, NULL},
      },
      NULL
   },
   {
      "snes9x_2010_overclock_cycles",
      "Yavaşlamayı Azalt (Hack, Güvensiz)",
      "Yavaslamayi Azalt (G�vensiz)",
      "SNES İşlemcisi için hız aşırtmadır. Oyunların çökmesine neden olabilir! Daha kısa yükleme süreleri için 'Hafif'i, yavaşlama gösteren oyunların çoğunda' Uyumlu 've yalnızca kesinlikle gerekliyse' Maks 'kullanın (Gradius 3, Süper R tipi ...).",
      NULL,
      NULL,
      {
         { "disabled",   NULL },
         { "light",      "Hafif" },
         { "compatible", "Uyumlu" },
         { "max",        "Maks" },
         { NULL, NULL},
      },
      NULL
   },
   {
      "snes9x_2010_reduce_sprite_flicker",
      "Kırılmayı Azalt (Hack, Güvensiz)",
      NULL,
      "Ekranda aynı anda çizilebilen sprite sayısını arttırır.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL},
      },
      NULL
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

struct retro_core_options_v2 options_tr = {
   option_cats_tr,
   option_defs_tr
};

/* RETRO_LANGUAGE_SLOVAK */

/* RETRO_LANGUAGE_PERSIAN */

/* RETRO_LANGUAGE_HEBREW */

/* RETRO_LANGUAGE_ASTURIAN */

/* RETRO_LANGUAGE_FINNISH */

#ifdef __cplusplus
}
#endif

#endif
