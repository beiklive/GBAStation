//
// Created by cpasjuste on 17/09/18.
//

#ifndef __C2D_SHADERS_H__
#define __C2D_SHADERS_H__

// default shaders
extern const char *c2d_color_shader;
extern const char *c2d_texture_shader;

// crt shaders
extern const char *c2d_crt_aperture_shader;     // scaling: integer | filter: point     |
extern const char *c2d_crt_caligari_shader;     // scaling: integer | filter: point     |
extern const char *c2d_crt_cgwg_fast_shader;    // scaling: integer | filter: point     |
extern const char *c2d_crt_easymode_shader;     // scaling: integer | filter: point     |
extern const char *c2d_crt_fakelottes_shader;   // scaling: integer | filter: linear    |
extern const char *c2d_crt_geom_shader;         // scaling: integer | filter: point     |
extern const char *c2d_crt_geom_flat_shader;    // scaling: integer | filter: point     |
extern const char *c2d_crt_hyllian_shader;      // scaling: integer | filter: point     |
extern const char *c2d_crt_lottes_fast_shader;  // scaling: integer | filter: linear    |
extern const char *c2d_crt_lottes_shader;       // scaling: integer | filter: point     |
extern const char *c2d_crt_mattias_shader;      // scaling: integer | filter: point     |
extern const char *c2d_crt_nes_mini_shader;     // scaling: integer | filter: any       |
extern const char *c2d_crt_pi_shader;           // scaling: integer | filter: linear    |
extern const char *c2d_crt_pi_flat_shader;      // scaling: integer | filter: linear    |
extern const char *c2d_crt_zfast_shader;        // scaling: integer | filter: linear    |

// handheld shaders

extern const char *c2d_handheld_bevel_shader;       // scaling: integer | filter: point     |
extern const char *c2d_handheld_dot_shader;         // scaling: integer | filter: point     |
extern const char *c2d_handheld_lcd1x_shader;       // scaling: integer | filter: point     |
extern const char *c2d_handheld_lcd3x_shader;       // scaling: integer | filter: point     |
extern const char *c2d_handheld_retro_v2_shader;    // scaling: integer | filter: point     |
extern const char *c2d_handheld_zfast_lcd_shader;   // scaling: integer | filter: linear    |

// interpolation shaders
extern const char *c2d_interpolation_aann_shader;                       // scaling: free    | filter: linear    |
extern const char *c2d_interpolation_pixellate_shader;                  // scaling: free    | filter: point     |
extern const char *c2d_interpolation_quilez_shader;                     // scaling: free    | filter: linear    |
extern const char *c2d_interpolation_sharp_bilinear_shader;             // scaling: free    | filter: linear    |
extern const char *c2d_interpolation_sharp_bilinear_scanlines_shader;   // scaling: integer | filter: linear    |

// scanlines
extern const char *c2d_scanline_simple_shader;          // scaling: free    | filter: point    |

// "sharp"
extern const char *c2d_sharp_2xsal_shader;              // scaling: free    | filter: point     |
extern const char *c2d_sharp_sabr_v30_shader;           // scaling: free    | filter: point    |
extern const char *c2d_sharp_supereagle_shader;         // scaling: free    | filter: point    |
extern const char *c2d_sharp_xbrz_freescale_shader;     // scaling: free    | filter: point    |

// for compatibility with vita/ps4 shaders binaries
const int c2d_color_shader_length = 0;
const int c2d_texture_shader_length = 0;
const int c2d_crt_aperture_shader_length = 0;
const int c2d_crt_caligari_shader_length = 0;
const int c2d_crt_cgwg_fast_shader_length = 0;
const int c2d_crt_easymode_shader_length = 0;
const int c2d_crt_fakelottes_shader_length = 0;
const int c2d_crt_geom_shader_length = 0;
const int c2d_crt_geom_flat_shader_length = 0;
const int c2d_crt_hyllian_shader_length = 0;
const int c2d_crt_lottes_fast_shader_length = 0;
const int c2d_crt_lottes_shader_length = 0;
const int c2d_crt_mattias_shader_length = 0;
const int c2d_crt_nes_mini_shader_length = 0;
const int c2d_crt_pi_shader_length = 0;
const int c2d_crt_pi_flat_shader_length = 0;
const int c2d_crt_zfast_shader_length = 0;
const int c2d_handheld_bevel_shader_length = 0;
const int c2d_handheld_dot_shader_length = 0;
const int c2d_handheld_lcd1x_shader_length = 0;
const int c2d_handheld_lcd3x_shader_length = 0;
const int c2d_handheld_retro_v2_shader_length = 0;
const int c2d_handheld_zfast_lcd_shader_length = 0;
const int c2d_interpolation_aann_shader_length = 0;
const int c2d_interpolation_pixellate_shader_length = 0;
const int c2d_interpolation_quilez_shader_length = 0;
const int c2d_interpolation_sharp_bilinear_shader_length = 0;
const int c2d_interpolation_sharp_bilinear_scanlines_shader_length = 0;
const int c2d_scanline_simple_shader_length = 0;
const int c2d_sharp_2xsal_shader_length = 0;
const int c2d_sharp_sabr_v30_shader_length = 0;
const int c2d_sharp_supereagle_shader_length = 0;
const int c2d_sharp_xbrz_freescale_shader_length = 0;
#endif //__C2D_SHADERS_H__
