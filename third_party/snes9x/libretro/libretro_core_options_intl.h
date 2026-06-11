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
   {
      "lightgun",
      NULL,
      NULL
   },
   {
      "advanced_av",
      NULL,
      NULL
   },
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_tr[] = {
   {
      "snes9x_region",
      "Konsol Bölgesi (Core Yenilenir)",
      NULL,
      "Sistemin hangi bölgeden olduğunu belirtir.. 'PAL' 50hz'dir, 'NTSC' ise 60hz. Yanlış bölge seçiliyse, oyunlar normalden daha hızlı veya daha yavaş çalışacaktır.",
      NULL,
      NULL,
      {
         { "auto", "Otomatik" },
         { "ntsc", "NTSC" },
         { "pal",  "PAL" },
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_aspect",
      "Tercih Edilen En Boy Oranı",
      NULL,
      "Tercih edilen içerik en boy oranını seçin. Bu, yalnızca RetroArch’ın en boy oranı Video ayarlarında 'Core tarafından' olarak ayarlandığında uygulanacaktır.",
      NULL,
      NULL,
      {
         { "4:3",         NULL },
         { "4:3 scaled",  "4:3 (Korunmuş)" },
         { "uncorrected", "Düzeltilmemiş" },
         { "auto",        "Otomatik" },
         { "ntsc",        "NTSC" },
         { "pal",         "PAL" },
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_overscan",
      "Aşırı Taramayı Kırp",
      NULL,
      "Ekranın üst ve alt kısmındaki sınırlarını, tipik olarak standart çözünürlüklü bir televizyondakini kaldırır. 'Otomatik (~8 piksel)' ise geçerli içeriğe bağlı olarak aşırı taramayı algılamaya ve kırpmaya çalışacaktır.",
      NULL,
      NULL,
      {
         { "enabled",      "~8 piksel" },
         { "12_pixels",    "12 piksel" },
         { "16_pixels",    "16 piksel" },
         { "auto",         "Otomatik (~8 piksel)" },
         { "disabled",     NULL },
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_hires_blend",
      "Hi-Res Karışımı",
      NULL,
      "Oyun hi-res moduna geçtiğinde pikselleri karıştırır (512x448). Şeffaflık efektleri üretmek için hi-res modunu kullanan bazı oyunlar için gereklidir (Kirby's Dream Land, Jurassic Park ...).",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "merge",    "Birlşetir" },
         { "blur",     "Bulanıklaştır" },
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_blargg",
      "Blargg NTSC Filtresi",
      NULL,
      "Çeşitli NTSC TV sinyallerini taklit etmek için bir video filtresi uygular.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_audio_interpolation",
      "Ses Enterpolasyonu",
      NULL,
      "Belirtilen ses filtresini uygular. 'Gaussian', orijinal donanımın bas ağırlıklı sesini üretir. 'Cubic' ve 'Sinc' daha az doğrudur ve daha fazla aralığı korur.",
      NULL,
      NULL,
      {
         { "gaussian", "Gaussian" },
         { "cubic",    "Cubic" },
         { "sinc",     "Sinc" },
         { "none",     "Hiçbiri" },
         { "linear",   "Linear" },
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_up_down_allowed",
      "Karşı Yönlere İzin Ver",
      NULL,
      "Bunu etkinleştirmek aynı anda hem sola hem de sağa (veya yukarı ve aşağı) yönlere basma / hızlı değiştirme / tutma imkanı sağlar. Bu harekete dayalı hatalara neden olabilir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_overclock_superfx",
      "SuperFX Hız Aşırtma",
      NULL,
      "SuperFX işlemcisi frekans çarpanıdır. Kare hızını artırabilir veya zamanlama hatalarına neden olabilir. % 100'ün altındaki değerler yavaş cihazlarda oyun performansını artırabilir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_overclock_cycles",
      "Yavaşlamayı Azalt (Hack, Güvensiz)",
      "Yavaşlamayı Azalt (Güvensiz)",
      "SNES İşlemcisi için hız aşırtmadır. Oyunların çökmesine neden olabilir! Daha kısa yükleme süreleri için 'Hafif'i, yavaşlama gösteren oyunların çoğunda' Uyumlu 've yalnızca kesinlikle gerekliyse' Maks 'kullanın (Gradius 3, Süper R tipi ...).",
      NULL,
      NULL,
      {
         { "disabled",   NULL },
         { "light",      "Hafif" },
         { "compatible", "Uyumlu" },
         { "max",        "Maks" },
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_reduce_sprite_flicker",
      "Kırılmayı Azalt (Hack, Güvensiz)",
      "Kırılmayı Azalt (Güvensiz)",
      "Ekranda aynı anda çizilebilen sprite sayısını arttırır.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_randomize_memory",
      "Belleği Rastgele Kıl (Güvensiz)",
      NULL,
      "Başlatıldığında sistem RAM'ını rastgele ayarlar. 'Super Off Road' gibi bazı oyunlar, oyunu daha öngörülemeyen hale getirmek için öğe yerleştirme ve AI davranışı için rastgele sayı üreticisi olarak sistem RAM'ini kullanır.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_block_invalid_vram_access",
      "Geçersiz VRAM Erişimini Engelle",
      NULL,
      "Bazı Homebrew/ROM'lar, doğru işlem için bu seçeneğin devre dışı bırakılmasını gerektirir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_echo_buffer_hack",
      "Eko Tampon Hack (Güvenli değil, yalnızca eski addmusic için etkinleştirin)",
      NULL,
      "Bazı Homebrew/ROM'lar, doğru işlem için bu seçeneğin devre dışı bırakılmasını gerektirir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_show_lightgun_settings",
      "Light Gun Ayarlarını Göster",
      NULL,
      "Super Scope / Justifier / M.A.C.S. için tüfek girişi yapılandırmasını etkinleştir. NOT: Bu ayarın etkili olabilmesi için Hızlı Menü’nün açılması gerekir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_lightgun_mode",
      "Light Gun Modu",
      "Modu",
      "Fare kontrollü 'Light Gun' veya 'Dokunmatik Ekran' girişini kullanın.",
      NULL,
      NULL,
      {
         { "Lightgun",    "Light Gun" },
         { "Touchscreen", "Dokunmatik Ekran" },
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_superscope_reverse_buttons",
      "Super Scope Ters Tetik Düğmeleri",
      NULL,
      "Süper Scope için 'Ateş' ve 'İmleç' butonlarının pozisyonlarını değiştir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_superscope_crosshair",
      "Super Scope İmkeç",
      NULL,
      "Ekrandaki imleç işaretini değiştirin.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_superscope_color",
      "Super Scope Rengi",
      NULL,
      "Ekrandaki imleç işaretinin rengini değiştirin.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_justifier1_crosshair",
      "Justifier 1 İmleci",
      NULL,
      "Ekrandaki imleç işaretinin boyutunu değiştirin.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_justifier1_color",
      "Justifier 1 Rengi",
      NULL,
      "Ekrandaki imleç işaretinin rengini değiştirin.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_justifier2_crosshair",
      "Justifier 2 İmleci",
      NULL,
      "Ekrandaki imleç işaretinin boyutunu değiştirin.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_justifier2_color",
      "Justifier 2 REngi",
      NULL,
      "Ekrandaki imleç işaretinin rengini değiştirin.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_rifle_crosshair",
      "M.A.C.S. Tüfek ",
      NULL,
      "Ekrandaki imleç işaretinin rengini değiştirin..",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_rifle_color",
      "M.A.C.S. Tüfek Rengi",
      NULL,
      "Ekrandaki imleç işaretinin rengini değiştirin.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_show_advanced_av_settings",
      "Gelişmiş Ses/Video Ayarlarını Göster",
      NULL,
      "Düşük seviye video katmanı / GFX etkisi / ses kanalı parametrelerinin yapılandırılmasını etkinleştirir. NOT: Bu ayarın etkili olabilmesi için Hızlı Menü’nün açılması gerekir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_layer_1",
      "1. Katmanı Göster",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_layer_2",
      "2. Katmanı Göster",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_layer_3",
      "3. Katmanı Göster",
      NULL,
      NULL,
      NULL,
      NULL,
      {
        { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_layer_4",
      "4. Katmanı Göster",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_layer_5",
      "Sprite Katmanını Göster",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_gfx_clip",
      "Grafik Klibi Pencerelerini Etkinleştir",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_gfx_transp",
      "Saydamlık Efektlerini Etkinleştir",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_sndchan_1",
      "Ses Kanalı 1'i etkinleştir",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_sndchan_2",
      "Ses Kanalı 2'yi etkinleştir",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_sndchan_3",
      "Ses Kanalı 3'ü etkinleştir",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_sndchan_4",
      "Ses Kanalı 4'ü etkinleştir",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_sndchan_5",
      "Ses Kanalı 5'i etkinleştir",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_sndchan_6",
      "Ses Kanalı 6'yı etkinleştir",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_sndchan_7",
      "Ses Kanalı 7'yi etkinleştir",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "snes9x_sndchan_8",
      "Ses Kanalı 8'i etkinleştir",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
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
