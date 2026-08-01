/* 
   TINY3D (c) 2010 Hermes  <www.elotrolado.net>
   This work is based in ElSemi, Phire, AerialX and may be, others PSL1GHT contributors

*/

#ifndef TINY3D_H
#define TINY3D_H

#pragma once

#include <sysutil/video.h>

#ifdef OLD_TINY3D
    #include <rsx/gcm.h>
    #include <rsx/nv40.h>
#else
    #include <rsx/gcm_sys.h>
#endif

#include "matrix.h"

#ifndef RSX_MEMCPY
#define RSX_MEMCPY    __builtin_memcpy
#endif

#ifdef __cplusplus
extern "C" {
#endif


enum
{
    TINY3D_CANNOTINIT = -4,
    TINY3D_BUSY,
    TINY3D_OUTMEMORY,
    TINY3D_INVALID,
    TINY3D_OK = 0,

};

typedef enum 
{
    TINY3D_POINTS = 1,
    TINY3D_LINES,
    TINY3D_LINE_LOOP,
    TINY3D_LINE_STRIP,
    TINY3D_TRIANGLES,
    TINY3D_TRIANGLE_STRIP,
    TINY3D_TRIANGLE_FAN,
    TINY3D_QUADS,
    TINY3D_QUAD_STRIP,
    TINY3D_POLYGON

} type_polygon;

#define INT_REALITY_CLEAR_BUFFERS_DEPTH                 0x00000001
#define INT_REALITY_CLEAR_BUFFERS_STENCIL               0x00000002
#define INT_REALITY_CLEAR_BUFFERS_COLOR_R               0x00000010
#define INT_REALITY_CLEAR_BUFFERS_COLOR_G               0x00000020
#define INT_REALITY_CLEAR_BUFFERS_COLOR_B               0x00000040
#define INT_REALITY_CLEAR_BUFFERS_COLOR_A               0x00000080

#define INT_3D_CLEAR_BUFFERS_COLOR_A                    0x00000080
#define INT_3D_CLEAR_BUFFERS_STENCIL                    0x00000002

typedef enum 
{
    TINY3D_CLEAR_COLOR      = INT_REALITY_CLEAR_BUFFERS_COLOR_R | INT_REALITY_CLEAR_BUFFERS_COLOR_G 
                            | INT_REALITY_CLEAR_BUFFERS_COLOR_B | INT_3D_CLEAR_BUFFERS_COLOR_A,
    TINY3D_CLEAR_ZBUFFER    = INT_REALITY_CLEAR_BUFFERS_DEPTH,
    
    TINY3D_CLEAR_STENCIL    = INT_3D_CLEAR_BUFFERS_STENCIL,

    TINY3D_CLEAR_ALL        = TINY3D_CLEAR_COLOR | TINY3D_CLEAR_ZBUFFER | TINY3D_CLEAR_STENCIL

} clear_flags;

typedef enum 
{

    TINY3D_ALPHA_FUNC_NEVER     = 0x200,
    TINY3D_ALPHA_FUNC_LESS,
    TINY3D_ALPHA_FUNC_EQUAL,
    TINY3D_ALPHA_FUNC_LEQUAL,
    TINY3D_ALPHA_FUNC_GREATER,
    TINY3D_ALPHA_FUNC_NOTEQUAL,
    TINY3D_ALPHA_FUNC_GEQUAL,
    TINY3D_ALPHA_FUNC_ALWAYS

} alpha_func;

typedef enum 
{
    TINY3D_BLEND_FUNC_SRC_RGB_ZERO                       = 0x00000000,
    TINY3D_BLEND_FUNC_SRC_RGB_ONE,
    TINY3D_BLEND_FUNC_SRC_RGB_SRC_COLOR                  = 0x00000300,
    TINY3D_BLEND_FUNC_SRC_RGB_ONE_MINUS_SRC_COLOR,
    TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA,
    TINY3D_BLEND_FUNC_SRC_RGB_ONE_MINUS_SRC_ALPHA,
    TINY3D_BLEND_FUNC_SRC_RGB_DST_ALPHA,
    TINY3D_BLEND_FUNC_SRC_RGB_ONE_MINUS_DST_ALPHA,
    TINY3D_BLEND_FUNC_SRC_RGB_DST_COLOR,
    TINY3D_BLEND_FUNC_SRC_RGB_ONE_MINUS_DST_COLOR,
    TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA_SATURATE,
    TINY3D_BLEND_FUNC_SRC_RGB_CONSTANT_COLOR             = 0x00008001,
    TINY3D_BLEND_FUNC_SRC_RGB_ONE_MINUS_CONSTANT_COLOR,
    TINY3D_BLEND_FUNC_SRC_RGB_CONSTANT_ALPHA,
    TINY3D_BLEND_FUNC_SRC_RGB_ONE_MINUS_CONSTANT_ALPHA,
    
    TINY3D_BLEND_FUNC_SRC_ALPHA_ZERO                     = 0x00000000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_ONE                      = 0x00010000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_COLOR                = 0x03000000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_ONE_MINUS_SRC_COLOR      = 0x03010000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA                = 0x03020000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_ONE_MINUS_SRC_ALPHA      = 0x03030000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_DST_ALPHA                = 0x03040000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_ONE_MINUS_DST_ALPHA      = 0x03050000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_DST_COLOR                = 0x03060000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_ONE_MINUS_DST_COLOR      = 0x03070000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA_SATURATE       = 0x03080000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_CONSTANT_COLOR           = 0x80010000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_ONE_MINUS_CONSTANT_COLOR = 0x80020000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_CONSTANT_ALPHA           = 0x80030000,
    TINY3D_BLEND_FUNC_SRC_ALPHA_ONE_MINUS_CONSTANT_ALPHA = 0x80040000,

} blend_src_func;

typedef enum 
{

    TINY3D_BLEND_FUNC_DST_RGB_ZERO                       = 0x00000000,
    TINY3D_BLEND_FUNC_DST_RGB_ONE,
    TINY3D_BLEND_FUNC_DST_RGB_SRC_COLOR                  = 0x00000300,
    TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_COLOR,
    TINY3D_BLEND_FUNC_DST_RGB_SRC_ALPHA,
    TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA,
    TINY3D_BLEND_FUNC_DST_RGB_DST_ALPHA,
    TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_DST_ALPHA,
    TINY3D_BLEND_FUNC_DST_RGB_DST_COLOR,
    TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_DST_COLOR,
    TINY3D_BLEND_FUNC_DST_RGB_SRC_ALPHA_SATURATE,
    TINY3D_BLEND_FUNC_DST_RGB_CONSTANT_COLOR             = 0x00008001,
    TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_CONSTANT_COLOR,
    TINY3D_BLEND_FUNC_DST_RGB_CONSTANT_ALPHA,
    TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_CONSTANT_ALPHA,
   
    TINY3D_BLEND_FUNC_DST_ALPHA_ZERO                     = 0x00000000,
    TINY3D_BLEND_FUNC_DST_ALPHA_ONE                      = 0x00010000,
    TINY3D_BLEND_FUNC_DST_ALPHA_SRC_COLOR                = 0x03000000,
    TINY3D_BLEND_FUNC_DST_ALPHA_ONE_MINUS_SRC_COLOR      = 0x03010000,
    TINY3D_BLEND_FUNC_DST_ALPHA_SRC_ALPHA                = 0x03020000,
    TINY3D_BLEND_FUNC_DST_ALPHA_ONE_MINUS_SRC_ALPHA      = 0x03030000,
    TINY3D_BLEND_FUNC_DST_ALPHA_DST_ALPHA                = 0x03040000,
    TINY3D_BLEND_FUNC_DST_ALPHA_ONE_MINUS_DST_ALPHA      = 0x03050000,
    TINY3D_BLEND_FUNC_DST_ALPHA_DST_COLOR                = 0x03060000,
    TINY3D_BLEND_FUNC_DST_ALPHA_ONE_MINUS_DST_COLOR      = 0x03070000,
    TINY3D_BLEND_FUNC_DST_ALPHA_SRC_ALPHA_SATURATE       = 0x03080000,
    TINY3D_BLEND_FUNC_DST_ALPHA_CONSTANT_COLOR           = 0x80010000,
    TINY3D_BLEND_FUNC_DST_ALPHA_ONE_MINUS_CONSTANT_COLOR = 0x80020000,
    TINY3D_BLEND_FUNC_DST_ALPHA_CONSTANT_ALPHA           = 0x80030000,
    TINY3D_BLEND_FUNC_DST_ALPHA_ONE_MINUS_CONSTANT_ALPHA = 0x80040000

} blend_dst_func;


typedef enum 
{

    TINY3D_TEX_FORMAT_L8       = 0x00000100,
    TINY3D_TEX_FORMAT_A1R5G5B5 = 0x00000200,
    TINY3D_TEX_FORMAT_A4R4G4B4 = 0x00000300,
    TINY3D_TEX_FORMAT_R5G6B5   = 0x00000400,
    TINY3D_TEX_FORMAT_A8R8G8B8 = 0x00000500

} text_format;

typedef enum 
{

    TINY3D_BLEND_RGB_FUNC_ADD                = 0x00008006,
    TINY3D_BLEND_RGB_MIN,
    TINY3D_BLEND_RGB_MAX,
    TINY3D_BLEND_RGB_FUNC_SUBTRACT           = 0x0000800a,
    TINY3D_BLEND_RGB_FUNC_REVERSE_SUBTRACT,

    TINY3D_BLEND_ALPHA_FUNC_ADD              = 0x80060000,
    TINY3D_BLEND_ALPHA_MIN                   = 0x80070000,
    TINY3D_BLEND_ALPHA_MAX                   = 0x80080000,
    TINY3D_BLEND_ALPHA_FUNC_SUBTRACT         = 0x800a0000,
    TINY3D_BLEND_ALPHA_FUNC_REVERSE_SUBTRACT = 0x800b0000

} blend_func;


/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* INITIALIZE                                                                                                                                  */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

// initialize video and 3D environmment. vertex_buff_size is the size (in bytes) for the buffer that allocates the vertex datas of the polygons
// by default tiny3d_Init() uses Z32 as buffer but you can uses Z16 with tiny3d_Init(TINY3D_Z16 | vertex_buff_size)

#define TINY3D_Z16 0x40000000

int tiny3d_Init(u32 vertex_buff_size);

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* EXIT (it is also called from Exit() function)                                                                                               */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

void tiny3d_Exit(void);

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLEAR                                                                                                                                       */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

// clear the buffers. Usually call it with tiny3d_Clear(color, TINY3D_CLEAR_ALL)

void tiny3d_Clear(u32 color, clear_flags flags);

// clear the buffers. Usually call it with tiny3d_Clear2(color, TINY3D_CLEAR_ALL, ...
// it is used to render in one alternative surface (texture)

#define CLEARSURFACE_A1R5G5B5 0
#define CLEARSURFACE_A8R8G8B8 1

void tiny3d_ClearSurface(u32 color, clear_flags flags, u32 rsx_offset, u32 width, u32 height, u32 stride, u32 format);

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* CHANGE TO 2D/3D                                                                                                                             */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/


/* tiny3d_UserViewPort: change to user ViewPort.

This function allow to the user to change the viewport X/Y scale for 2D/3D modes and 2D position. You must call before tiny3d_Clear(),
tiny3d_Project2D(), tiny3d_Project3D() but it is not necessary to call one time per frame (one time can be sufficient).

You can use Video_Resolution.width and Video_Resolution.height to scale it. For example, in 3D mode by default is used 
((float) Video_Resolution.width) / 1920.0f and ((float) Video_Resolution.height) / 1080.0f for the scale.
For 2D mode it can be ((float) Video_Resolution.width) / 848.0f and ((float) Video_Resolution.height) / 512.0f and pos_x = pos_y = 0.0f
to support virtual 8x48x512 2D screen

if you HDTV support complete screen (some HDTV loses part of visible frame buffer area), you can use this method for the video output, but some 
HDTV (me HDTV) can be configured for "video" or "PC" the DVI (HDMI) port. By default, Tiny3D is adjusted for my  HDTV "video" mode, so if you can
see a black border in some samples, you can use tiny3d_UserViewPort() to adjust correctly it 

The onoff param is provide to switch to the internal (with 0) or user (not 0) viewports.

See userviewport sample.

*/

void tiny3d_UserViewport(int onoff, float pos_x, float pos_y, float scale2D_x, float scale2D_y, float scale3D_x, float scale3D_y);

/* tiny3d_UserViewPort: change to user ViewPort.

The same thing of tiny3d_UserViewport() but with coordinates correction (from center) for 3D mode

*/

void tiny3d_UserViewport2(int onoff, float pos2D_x, float pos2D_y, float scale2D_x, float scale2D_y, 
                                    float correction3D_x, float correction3D_y, float scale3D_x, float scale3D_y);

/*

tiny3d_UserViewportSurface: change to user ViewPort for surface render in 2D.

Usually, 2D virtual coordinates is based in 848.0 x 512.0. With this function you can change it for surfaces

In 3D it uses -1.0 to 1.0 range

*/

void tiny3d_UserViewportSurface(int onoff, float width_2D, float height_2D);

/*

tiny3d_Project2D(): change to 2D mode

*/

void tiny3d_Project2D();

/*

tiny3d_Project3D(): change to 3D mode

*/

void tiny3d_Project3D();


/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* FLIP                                                                                                                                        */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

// change the screen buffer, waits to vblank and reset some variables. 
// Polygons must be writen after tiny3d_Clear() and before tiny3d_Flip() functions

void tiny3d_Flip();

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* SYNC                                                                                                                                        */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

void tiny3d_WaitRSX();

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* CALLBACK                                                                                                                                    */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

// return 0 or 1 if one menu is active (pressing PS in the pad, for example)

int tiny3d_MenuActive();

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* TEXTURES                                                                                                                                    */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

// used to alloc RSX memory for the textures. The best idea is allocate a big area to work with textures, because you cannot deallocate the memory used

void * tiny3d_AllocTexture(u32 size);

// RSX use the offset to work with textures. This function return the texture offset from the pointer to the RSX memory allocated

u32 tiny3d_TextureOffset(void * text);

#define TEXTURE_NEAREST 0
#define TEXTURE_LINEAR  1

// set texture to draw (TEXTWRAP_REPEAT mode)

void tiny3d_SetTexture(u32 unit, u32 offset, u32 width, u32 height, u32 stride, text_format fmt, int smooth);

#define TEXTWRAP_REPEAT 0
#define TEXTWRAP_CLAMP  1
#define TEXTWRAP_MIRROR 2

// set texture to draw with wrap options

void tiny3d_SetTextureWrap(u32 unit, u32 offset, u32 width, u32 height, u32 stride, text_format fmt, int wrap_u, int wrap_v, int smooth);

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* SHADERS CONTROL FOR MULTI-TEXTURES                                                                                                          */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

#define MT_MULT_METHOD   0
#define MT_MADD_A_METHOD 1
#define MT_MADD_B_METHOD 2

// set method for merge multiple textures (it change the pixel shader)

void tiny3d_SelMultiTexturesMethod(u32 method);

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* ALPHA / BLEND                                                                                                                               */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

// enable/disable the alpha test

void tiny3d_AlphaTest(int enable, u8 ref, alpha_func func);

// enable/disable the blend function

void tiny3d_BlendFunc( int enable, blend_src_func src_fun, blend_dst_func dst_func, blend_func func);

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* MATRIX                                                                                                                                      */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

// set projection matrix (used in 3D context)

void tiny3d_SetProjectionMatrix(MATRIX *mat);

// set Model/View matrix: Call before tiny3d_SetPolygon()

void tiny3d_SetMatrixModelView(MATRIX *mat);


/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* DRAW POLYGONS                                                                                                                               */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

// set the polygon to draw. It must be called before to call tiny3d_Vertex* functions

int tiny3d_SetPolygon(type_polygon type);

// close the list of vertex, update shader and world matrix and write polygons

int tiny3d_End();

// position: it must be the first vertex function to call

void tiny3d_VertexPos(float x, float y, float z);
void tiny3d_VertexPos4(float x, float y, float z, float w);
void tiny3d_VertexPosVector(VECTOR v);

// color: fix color method

void tiny3d_VertexColor(u32 rgba);
void tiny3d_VertexFcolor(float r, float g, float b, float a);

// texture: texture coords

void tiny3d_VertexTexture(float u, float v);

// texture: texture coords2

void tiny3d_VertexTexture2(float u, float v);

// normal: normal coords 

void tiny3d_Normal(float x, float y, float z);
void tiny3d_NormalVector(VECTOR v);

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* LIGHTS & MATERIALS                                                                                                                          */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

// ambiental light multiplier

void tiny3d_SetAmbientLight(float r, float g, float b);

// camera position reference for position lights

void tiny3d_SetLightCameraPosition(float x, float y, float z);

// set positional light
// indx: 0 to 3 (4 lights supported)
// x,y,z: space position
// r,g,b: color components (0.0f to 1.0f)
// mode: 1 -> light diffuse, 2 -> light specular, 0-> light disable

#define LIGHT_DISABLED 0
#define LIGHT_DIFFUSE  1
#define LIGHT_SPECULAR 2

void tiny3d_SetLight(int indx, float x, float y, float z, float r, float g, float b, int mode);

// disable all lights
void tiny3d_SetLightsOff();

// EmissiveMaterial:
// r,g,b: color components (0.0f to 1.0f)
// a: Unused

void tiny3d_EmissiveMaterial(float r, float g, float b, float a);

// AmbientMaterial:
// r,g,b: color components (0.0f to 1.0f)
// a: Alpha component. Translucency control (0.0f to 1.0f)

void tiny3d_AmbientMaterial (float r, float g, float b, float a);

// DiffuseMaterial:
// r,g,b: color components (0.0f to 1.0f)
// a: 0.0 -> disable, other value -> enable

void tiny3d_DiffuseMaterial (float r, float g, float b, float a);

// SpecularMaterial:
// r,g,b: color components (0.0f to 1.0f)
// shininess: 0.0 -> disable specular, other value -> shininess

void tiny3d_SpecularMaterial(float r, float g, float b, float shininess);

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* LISTS                                                                                                                                       */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

// start recording list of polygon, vertex, textures, material, matrix propierties

int tiny3d_RecordList();

// use this instead tiny3d_SetMatrixModelView() to set one external-dynamic matrix

void tiny3d_DynamicMatrixList(MATRIX *mat);

// use this instead tiny3d_SetMatrixModelView() to apply the matrix changes directly

void tiny3d_ApplyMatrixList(MATRIX *mat);

// Record list stop and return the head of the list command

void * tiny3d_StopList();

// draw list. Also can link the list with the recording current list (hierarchy lists)

void tiny3d_DrawList(void * headlist);

// free list (is not recursive)

void tiny3d_FreeList(void * headlist);

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* SPECIAL MODES                                                                                                                               */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

// Enable YUV (Ycbcr) 32 bits Pixels shader for single texture. You need one A8R8G8B8 texture to work or 3 L8 textures for Y, U, V.
// Only for Polygons with single texture or texture + color attributes

// USE_AYUV_32BITS_TEXTURE works under A8R8G8B8 TEX0.
// USE_YUV_8BIT_TEXTURES works under L8 textures mapped as TEX0, TEX1 and TEXT2 but only use TEXTURE0 coordinates for all (tiny3d_VertexTexture())


#define USE_AYUV_32BITS_TEXTURE 0
#define USE_YUV_8BIT_TEXTURES   1

void tiny3d_Enable_YUV(int select);

// Disable YUV (Ycbcr) 32 bits Pixel shader

void tiny3d_Disable_YUV();

// disable YUV, changes to 2D context and reset others things. You can use this function if you changes the shaders or send polygons OUT of Tiny3D
// context and more later, you want return to the Tiny3D context (NOTE: Untested function)

void tiny3d_Dirty_Status();

// return tini3D gcmContextData

void * tiny3d_Get_GCM_Context(void);

// test and do space in the context if it is neccesary, to send rsx commands, shaders, etc, externally (1 space = 4 bytes). 
// if (space <=0) clear the context space
// it call to the gcm callback present in gcmContextData to work

void tiny3d_DoCmd_Space(int space);

/*---------------------------------------------------------------------------------------------------------------------------------------------*/
/* VIDEO                                                                                                                                       */
/*---------------------------------------------------------------------------------------------------------------------------------------------*/

extern int Video_currentBuffer;

#ifdef OLD_TINY3D
    extern VideoResolution Video_Resolution;
#else
    extern videoResolution Video_Resolution;
#endif

extern u32 * Video_buffer[2];

extern int Video_pitch;

extern u8 Video_aspect; // 1-> 4:3 2-> 16:9

#ifdef __cplusplus
}
#endif

#endif

