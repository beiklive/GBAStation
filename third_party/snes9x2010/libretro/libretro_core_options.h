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
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {
	/* These variable names and possible values constitute an ABI with ZMZ (ZSNES Libretro player).
	 * Changing "Show layer 1" is fine, but don't change "layer_1"/etc or the possible values ("Yes|No").
	 * Adding more variables and rearranging them is safe. */
   {
      "snes9x_2010_region",
      "Console Region (Restart)",
      NULL,
      "Specify which region the system is from. 'PAL' is 50hz, 'NTSC' is 60hz. Games will run faster or slower than normal if the incorrect region is selected.",
      NULL,
      NULL,
      {
         { "auto", "Auto" },
         { "ntsc", "NTSC" },
         { "pal",  "PAL" },
         { NULL, NULL},
      },
      "auto"
   },
   {
      "snes9x_2010_audio_interpolation",
      "Audio Interpolation (Inaccurate)",
      NULL,
      "Choose how each SNES voice's pitch is interpolated. 'Gaussian' matches real hardware. 'Cubic' is brighter and less muffled, but does not match hardware and does not undo BRR sample compression. 'Sinc' is the sharpest/brightest (8-tap) and the most CPU-intensive.",
      NULL,
      NULL,
      {
         { "gaussian", "Gaussian (Accurate)" },
         { "cubic",    "Cubic (Brighter)" },
         { "sinc",     "Sinc (Sharpest)" },
         { NULL, NULL},
      },
      "gaussian"
   },
   {
      "snes9x_2010_aspect",
      "Preferred Aspect Ratio",
      NULL,
      "Choose the preferred content aspect ratio. This will only apply when RetroArch's aspect ratio is set to 'Core provided' in the Video settings.",
      NULL,
      NULL,
      {
         { "4:3",         NULL },
         { "uncorrected", "Uncorrected" },
         { "auto",        "Auto" },
         { "ntsc",        "NTSC" },
         { "pal",         "PAL" },
         { NULL, NULL},
      },
      "auto"
   },
   {
      "snes9x_2010_turbodelay",
      "Set Autofire Pulse",
      NULL,
      "Fire interval: medium - 6 frames, fast - 4 frames, slow - 8 frames.",
      NULL,
      NULL,
      {
         { "medium", "Medium" },
         { "fast", "Fast" },
         { "slow", "Slow "},
         { NULL, NULL },
      },
      "medium"
   },
   {
      "snes9x_2010_blargg",
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
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "snes9x_2010_frameskip",
      "Frameskip",
      NULL,
      "Skip frames to avoid audio buffer under-run (crackling). Improves performance at the expense of visual smoothness. 'Auto' skips frames when advised by the frontend. 'Manual' utilises the 'Frameskip Threshold (%)' setting.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "auto",     "Auto" },
         { "manual",   "Manual" },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "snes9x_2010_frameskip_threshold",
      "Frameskip Threshold (%)",
      NULL,
      "When 'Frameskip' is set to 'Manual', specifies the audio buffer occupancy threshold (percentage) below which frames will be skipped. Higher values reduce the risk of crackling by causing frames to be dropped more frequently.",
      NULL,
      NULL,
      {
         { "15", NULL },
         { "18", NULL },
         { "21", NULL },
         { "24", NULL },
         { "27", NULL },
         { "30", NULL },
         { "33", NULL },
         { "36", NULL },
         { "39", NULL },
         { "42", NULL },
         { "45", NULL },
         { "48", NULL },
         { "51", NULL },
         { "54", NULL },
         { "57", NULL },
         { "60", NULL },
         { NULL, NULL },
      },
      "33"
   },
   {
      "snes9x_2010_overclock",
      "SuperFX Overclock",
      NULL,
      "Overclock or underclock the SuperFX chip. This may improve the framerate and playability of games that use SuperFX.",
      NULL,
      "hacks",
      {
         { "5 MHz (Underclock)", NULL },
         { "8 MHz (Underclock)", NULL },
         { "9 MHz (Underclock)", NULL },
         { "10 MHz (Default)",  NULL },
         { "11 MHz", NULL },
         { "12 MHz", NULL },
         { "13 MHz", NULL },
         { "14 MHz", NULL },
         { "15 MHz", NULL },
         { "20 MHz", NULL },
         { "30 MHz", NULL },
         { "40 MHz", NULL },
         { NULL, NULL},
      },
      "10 MHz (Default)"
   },
   {
      "snes9x_2010_overclock_cycles",
      "Reduce Slowdown (Hack, Unsafe)",
      NULL,
      "Overclock SNES CPU. May cause games to crash! Use 'Light' for shorter loading times, 'Compatible' for most games exhibiting slowdown and 'Max' only if absolutely required (Gradius 3, Super R-type...).",
      NULL,
      "hacks",
      {
         { "disabled",   NULL },
         { "light",      "Light" },
         { "compatible", "Compatible" },
         { "max",        "Max" },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "snes9x_2010_reduce_sprite_flicker",
      "Reduce Flickering (Hack, Unsafe)",
      NULL,
      "Increases number of sprites that can be drawn simultaneously on screen.",
      NULL,
      "hacks",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "snes9x_2010_block_invalid_vram_access",
      "Block Invalid VRAM Access (Restart)",
      NULL,
      "Some homebrew/ROM hacks require this option to be disabled for correct operation.",
      NULL,
      "hacks",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL},
      },
      "enabled"
   },
   {
      "snes9x_2010_mode7_hires",
      "Mode 7 - Hires (Restart)",
      NULL,
      "Render Mode 7 backgrounds at higher resolution. 2x renders at 512 px (smoother diagonals); 4x renders at 1024 px (smoothest, more expensive). The 2x_hv / 4x_hv variants additionally double the output vertically (448 rows) via an end-of-frame bilinear-Y average of adjacent rendered rows; HUD / sprites / non-M7 layers are row-duplicated. Smooths row-to-row transitions on Mode 7 ground textures but does not add per-line detail (the M7 plane still samples at 224 source rows). Affects only Mode 7 (driving / racing / overhead-rotation games). Other modes are unchanged. Default off.",
      NULL,
      "hacks",
      {
         { "disabled", NULL },
         { "2x",       NULL },
         { "4x",       NULL },
         { "2x_hv",    NULL },
         { "4x_hv",    NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "snes9x_2010_mode7_hires_bilinear",
      "Mode 7 - Bilinear Filter",
      NULL,
      "Apply bilinear filtering to Mode 7 backgrounds. Stable: blends along the X axis only with floor-Y sampling. Robust against high-contrast horizontal banding (rainbow rings, palette-rotation effects); recommended default. Smooth: 4-corner bilinear with a row-contrast guard that falls back to stable behaviour on high-contrast row pairs (avoids muddy seams on content like the Tiny Toons title screen). More aggressive smoothing along both axes; somewhat more expensive than stable. Independent of the Hires option; works at native, 2x and 4x.",
      NULL,
      "hacks",
      {
         { "disabled", NULL },
         { "stable",   NULL },
         { "smooth",   NULL },
         { NULL, NULL},
      },
      "stable"
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
   &options_us,    /* RETRO_LANGUAGE_ENGLISH */
   NULL,           /* RETRO_LANGUAGE_JAPANESE */
   NULL,           /* RETRO_LANGUAGE_FRENCH */
   NULL,           /* RETRO_LANGUAGE_SPANISH */
   NULL,           /* RETRO_LANGUAGE_GERMAN */
   NULL,           /* RETRO_LANGUAGE_ITALIAN */
   NULL,           /* RETRO_LANGUAGE_DUTCH */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,           /* RETRO_LANGUAGE_RUSSIAN */
   NULL,           /* RETRO_LANGUAGE_KOREAN */
   NULL,           /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,           /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,           /* RETRO_LANGUAGE_ESPERANTO */
   NULL,           /* RETRO_LANGUAGE_POLISH */
   NULL,           /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,           /* RETRO_LANGUAGE_ARABIC */
   NULL,           /* RETRO_LANGUAGE_GREEK */
   &options_tr,    /* RETRO_LANGUAGE_TURKISH */
   NULL,           /* RETRO_LANGUAGE_SLOVAK */
   NULL,           /* RETRO_LANGUAGE_PERSIAN */
   NULL,           /* RETRO_LANGUAGE_HEBREW */
   NULL,           /* RETRO_LANGUAGE_ASTURIAN */
   NULL,           /* RETRO_LANGUAGE_FINNISH */
};

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
            if (
                   (strcmp(key, "snes9x_show_advanced_av_settings") == 0)
               )
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

#endif
