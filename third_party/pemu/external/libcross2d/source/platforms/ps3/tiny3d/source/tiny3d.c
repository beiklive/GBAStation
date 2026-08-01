/*
   TINY3D (c) 2010 Hermes  <www.elotrolado.net>
   This work is based in ElSemi, Phire, AerialX and may be, others PSL1GHT contributors

*/

#include "tiny3d.h"
#include "matrix.h"

//#include <sysutil/events.h>

#include <string.h>
#include <math.h>
#include <unistd.h>

#include "rsxutil.h"
#include "nv_shaders.h"

#include "vshader_text_normal.vcg.h"

#ifndef __CROSS2D__
#include "buffer.h"
#endif
#include "commands.h"
#include "rsxtiny.h"
#include "realityVP.h"
#include "old_nv40.h"

static struct {
    int target;
    u32 w, h, p;
} render_target;

#define EVENT_REQUEST_EXITAPP 	0x101
#define EVENT_DRAWING_BEGIN 	0x121
#define EVENT_DRAWING_END		0x122
#define EVENT_MENU_OPEN 		0x131
#define EVENT_MENU_CLOSE 		0x132

typedef enum sysEventSlot{
	EVENT_SLOT0,
	EVENT_SLOT1,
	EVENT_SLOT2,
	EVENT_SLOT3,
	EVENT_SLOTMAX,
} sysEventSlot;

typedef void (*sysCallback)(u64 status,	u64 param, void * userdata);

#ifdef OLD_TINY3D
    #define sysUtilRegisterCallback sysRegisterCallback
    #define sysUtilCheckCallback sysCheckCallback
    #define sysUtilUnregisterCallback sysUnregisterCallback
#endif

s32 sysUtilRegisterCallback(sysEventSlot slot, sysCallback func, void * userdata);
s32 sysUtilCheckCallback();
s32 sysUtilUnregisterCallback(sysEventSlot slot);


void __attribute__((weak)) tiny3d_alarm(int leds)
{
   // user can receive one status alarm externally

}


int tinyerror_incallback = 0;
int tinycallback_flag = 0;

static int tiny_3d_alarm = 0;
static int use_2d = 0; // MODE 2D/3D
static int enable_yuv = 0;


#define Z_SCALE 1.0/65536.0

int Video_currentBuffer = 0;

static u8 *rsx_vertex = NULL;           // vertex memory

static int size_rsx_vertex = 1024*1024; // size of vertex memory

static int pos_rsx_vertex = 0;          // offset to vertex memory

static int polygon = -1;

static u32 off_head_vertex = 0;  // index from the first vertex from shader list
static u32 off_start_vertex = 0; // memory offset of the first vertex of the polygon

static int min_vertex = 0x7fffffff; // min vertex for polygon requested
static int mod_vertex = 0x7fffffff; // module for the next vertex for polygon requested

static int n_vertex = 0; // number of vertex counted for the current polygon

#define VERTEX_LOCK      65536

#define VERTEX_SETCAMERA  4096
#define VERTEX_SETAMBIENT 2048
#define VERTEX_SETLIGHT 1024
#define VERTEX_SETMAT    512
#define VERTEX_SETPROJ   256
#define VERTEX_SETMATRIX 128

#define VERTEX_SETPOS      1
#define VERTEX_SETFCOL     2
#define VERTEX_SETCOL      4
#define VERTEX_NORMAL      8
#define VERTEX_SETTEXT     16
#define VERTEX_SETTEXT2    32


#define VERTEX_MASK (VERTEX_SETAMBIENT | VERTEX_SETCAMERA | VERTEX_SETLIGHT | VERTEX_SETMAT | VERTEX_SETPROJ | VERTEX_SETMATRIX)
#define VERTEX_MASKMATRIX (VERTEX_SETPROJ | VERTEX_SETMATRIX)

static int select_fp = 0; //  for alternative Pixel Shader

static int flag_vertex = 0; // flags to determine the shader and the datas used

static struct
{
    float x, y, z, w;

    float r, g, b, a;

    u32 rgba;

    float u, v, u2, v2;

    float nx, ny, nz;

} vertex_data;   // temp vertex data


static int current_shader = -1; // current shader ID

static struct _data_shader {

    int off_project;
    int off_modelv;
    int off_position;
    int off_color;
    int off_texture;
    int off_texture2;
    int off_normal;

    int off_lightAmbient;
    int off_lightColor;
    int off_lightPosition;

    int off_cameraPosition;
    int off_emissive;
    int off_ambient;
    int off_diffuse;
    int off_specular;

    void *vp;

    void *fp;
    void *fp_alt[2];
    void *fp_yuv[2];

    int size_vertex;
    int fixed_color;

} data_shader[15];

// marcador

static struct {

    float emissive[4];
    float ambient [4];
    float diffuse [4];
    float specular[4];

} material;

static struct {

    float pos  [4][4];
    float color[4][4];
    float ambient [4];
    float camera  [4];

} light;


static MATRIX matrix_ident =
{
    {
    {1.0, 0.0, 0.0, 0.0},
    {0.0, 1.0, 0.0, 0.0},
    {0.0, 0.0, 1.0, 0.0},
    {0.0, 0.0, 0.0, 1.0}
    }
};

// Projection Matrix (used in 3D)

static MATRIX project_mat;

// Model/View matrix

static MATRIX model_view;

#include "list_code.h"

#define CASE_POLYGON1(x, min, mod) case x: \
       min_vertex = min; mod_vertex = mod; break;

static gcmControlRegister *CtrlReg = NULL;


static void *fpshader_list[] = {
    &nv30_fp_color,
    &nv30_fp_texture,
    &nv30_fp_texture_color,
    &nv30_fp_texture2,
    &nv30_fp_texture2_alt,
    &nv30_fp_texture2_alt2,
    &nv30_fp_texture_color2,
    &nv30_fp_texture_color2_alt,
    &nv30_fp_texture_color2_alt2,
    &nv30_fp_yuv,
    &nv30_fp_yuv8,
    &nv30_fp_yuv_color,
    &nv30_fp_yuv_color8,
    NULL
};

static volatile int tiny3d_drawing_status = 0;
static volatile int tiny3d_menu_open = 0;


static void tiny3d_callback(u64 status,	u64 param, void * userdata)
{
    switch (status) {
        case EVENT_REQUEST_EXITAPP:

            sysUtilUnregisterCallback(EVENT_SLOT3);
            exit(0);
            //sysProcessExit(1);
            break;
        case EVENT_DRAWING_BEGIN:
            tiny3d_drawing_status = 1;
            break;
        case EVENT_DRAWING_END:
            tiny3d_drawing_status = 0;
            break;
        case EVENT_MENU_OPEN:
            tiny3d_menu_open = 1;
            break;
        case EVENT_MENU_CLOSE:
            tiny3d_menu_open = 0;
            break;
        default:
           break;

        }
}

static int inited= 0;

extern void *tiny_host_addr;

void tiny3d_Exit(void)
{

    if(!inited) return;

    gcmSetWaitFlip((gcmContextData *) context);
    rsxtiny_SetReferenceCommand(context, 1);
	rsxtiny_FlushBuffer(context);

	gcmControlRegister volatile *ctrl = gcmGetControlRegister((gcmContextData *) context);
	while(ctrl->ref != 1) usleep(30);


    //free(tiny_host_addr);

    inited= 0;

    tiny3d_drawing_status = 0;
    tiny3d_menu_open = 0;

    tinyerror_incallback = 0;
    tinycallback_flag = 0;

    tiny_3d_alarm = 0;
    use_2d = 0; // MODE 2D/3D
    enable_yuv = 0;

    //Video_currentBuffer = 0;

    rsx_vertex = NULL;           // vertex memory

    size_rsx_vertex = 1024*1024; // size of vertex memory

    pos_rsx_vertex = 0;          // offset to vertex memory

    polygon = -1;

    off_head_vertex = 0;  // index from the first vertex from shader list
    off_start_vertex = 0; // memory offset of the first vertex of the polygon

    min_vertex = 0x7fffffff; // min vertex for polygon requested
    mod_vertex = 0x7fffffff; // module for the next vertex for polygon requested

    n_vertex = 0; // number of vertex counted for the current polygon

    select_fp = 0; //  for alternative Pixel Shader

    flag_vertex = 0; // flags to determine the shader and the datas used

    current_shader = -1; // current shader ID
    CtrlReg = NULL;
}

/***********************************************************************************************************/
/* INIT                                                                                                    */
/***********************************************************************************************************/

int tiny3d_Init(u32 vertex_buff_size)
{


    int n;

    int use_Z32 = 1;

    if(vertex_buff_size & TINY3D_Z16) use_Z32 = 0;

    vertex_buff_size &= 0x3fffffff;

    flag_vertex = VERTEX_LOCK;

    if(inited) return TINY3D_CANNOTINIT;

    inited = 1;

    if(vertex_buff_size <= 1024*1024) vertex_buff_size = 1024*1024;

    size_rsx_vertex = vertex_buff_size;

    init_screen(vertex_buff_size * 2, use_Z32);

    CtrlReg = gcmGetControlRegister((gcmContextData *) context);

    n = 0;

    while(fpshader_list[n]) {

        // install fragments shaders in rsx memory
        u32 *frag_mem = rsxtiny_MemAlign(256, (((realityFragmentProgram_internal *) fpshader_list[n])->size * 4 + 255) & ~255);

        if(!frag_mem) return TINY3D_OUTMEMORY;

        rsxtiny_InstallFragmentProgram(context, fpshader_list[n], frag_mem);

        n++;
    }

    // shaders datas
    for(n = 0; n < 12; n++) {
        data_shader[n].off_project  = rsxtiny_VertexProgramGetConstant((rsxtiny_VertexProgram*)       vshader_text_normal_bin, "ProjMatrix");
        data_shader[n].off_modelv   = rsxtiny_VertexProgramGetConstant((rsxtiny_VertexProgram*)       vshader_text_normal_bin, "WorldMatrix");
        data_shader[n].off_position = rsxtiny_VertexProgramGetInputAttribute((rsxtiny_VertexProgram*) vshader_text_normal_bin, "inputvertex.vertex");
        data_shader[n].off_texture  = rsxtiny_VertexProgramGetInputAttribute((rsxtiny_VertexProgram*) vshader_text_normal_bin, "inputvertex.texcoord");
        data_shader[n].off_texture2 = rsxtiny_VertexProgramGetInputAttribute((rsxtiny_VertexProgram*) vshader_text_normal_bin, "inputvertex.texcoord2");
        data_shader[n].off_color    = rsxtiny_VertexProgramGetInputAttribute((rsxtiny_VertexProgram*) vshader_text_normal_bin, "inputvertex.color");
        data_shader[n].off_normal   = rsxtiny_VertexProgramGetInputAttribute((rsxtiny_VertexProgram*) vshader_text_normal_bin, "inputvertex.normal");
        data_shader[n].vp           = (void *) vshader_text_normal_bin;
        data_shader[n].fp_alt[0]    = (void *) NULL;
        data_shader[n].fp_alt[1]    = (void *) NULL;
        data_shader[n].fp_yuv[0]    = (void *) NULL;
        data_shader[n].fp_yuv[1]    = (void *) NULL;

        data_shader[n].off_lightAmbient   = rsxtiny_VertexProgramGetConstant((rsxtiny_VertexProgram*) vshader_text_normal_bin, "lightAmbient");
        data_shader[n].off_lightColor     = rsxtiny_VertexProgramGetConstant((rsxtiny_VertexProgram*) vshader_text_normal_bin, "lightColor");
        data_shader[n].off_lightPosition  = rsxtiny_VertexProgramGetConstant((rsxtiny_VertexProgram*) vshader_text_normal_bin, "lightPosition");
        data_shader[n].off_cameraPosition = rsxtiny_VertexProgramGetConstant((rsxtiny_VertexProgram*) vshader_text_normal_bin, "CameraPosition");
        data_shader[n].off_emissive       = rsxtiny_VertexProgramGetConstant((rsxtiny_VertexProgram*) vshader_text_normal_bin, "Memissive");
        data_shader[n].off_ambient        = rsxtiny_VertexProgramGetConstant((rsxtiny_VertexProgram*) vshader_text_normal_bin, "Mambient");
        data_shader[n].off_diffuse        = rsxtiny_VertexProgramGetConstant((rsxtiny_VertexProgram*) vshader_text_normal_bin, "Mdiffuse");
        data_shader[n].off_specular       = rsxtiny_VertexProgramGetConstant((rsxtiny_VertexProgram*) vshader_text_normal_bin, "Mspecular");

        data_shader[n].fixed_color = 0;
    }

    data_shader[0].off_texture  = -1; // colorf
    data_shader[0].off_texture2 = -1;
    data_shader[0].off_normal   = -1;
    data_shader[0].fp           = &nv30_fp_color;
    data_shader[0].size_vertex  = 16+16;

    data_shader[1].fixed_color  = 1; // colori
    data_shader[1].off_texture  = -1;
    data_shader[1].off_texture2 = -1;
    data_shader[1].off_normal   = -1;
    data_shader[1].fp           = &nv30_fp_color;
    data_shader[1].size_vertex  = 16+4;

    data_shader[2].off_texture2 = -1; // texture
    data_shader[2].off_color    = -1;
    data_shader[2].off_normal   = -1;
    data_shader[2].fp           = &nv30_fp_texture;
    data_shader[2].fp_yuv[0]    = &nv30_fp_yuv;
    data_shader[2].fp_yuv[1]    = &nv30_fp_yuv8;
    data_shader[2].size_vertex  = 16+8;

    data_shader[3].off_color    = -1; // texture + texture2
    data_shader[3].off_normal   = -1;
    data_shader[3].fp           = &nv30_fp_texture2;
    data_shader[3].fp_alt[0]    = &nv30_fp_texture2_alt;
    data_shader[3].fp_alt[1]    = &nv30_fp_texture2_alt2;
    data_shader[3].size_vertex  = 16+8+8;

    data_shader[4].off_texture2 = -1; // texture + colorf
    data_shader[4].off_normal   = -1;
    data_shader[4].fp           = &nv30_fp_texture_color;
    data_shader[4].fp_yuv[0]    = &nv30_fp_yuv_color;
    data_shader[4].fp_yuv[1]    = &nv30_fp_yuv_color8;
    data_shader[4].size_vertex  = 16+16+8;

    data_shader[5].off_normal   = -1; // texture + texture2 + colorf
    data_shader[5].fp           = &nv30_fp_texture_color2;
    data_shader[5].fp_alt[0]    = &nv30_fp_texture_color2_alt;
    data_shader[5].fp_alt[1]    = &nv30_fp_texture_color2_alt2;
    data_shader[5].size_vertex  = 16+16+8+8;

    data_shader[6].fixed_color  = 1; // texture + colori
    data_shader[6].off_texture2 = -1;
    data_shader[6].off_normal   = -1;
    data_shader[6].fp           = &nv30_fp_texture_color;
    data_shader[6].fp_yuv[0]    = &nv30_fp_yuv_color;
    data_shader[6].fp_yuv[1]    = &nv30_fp_yuv_color8;
    data_shader[6].size_vertex  = 16+4+8;

    data_shader[7].fixed_color  = 1; // texture + texture2 + colori
    data_shader[7].off_normal   = -1;
    data_shader[7].fp           = &nv30_fp_texture_color2;
    data_shader[7].fp_alt[0]    = &nv30_fp_texture_color2_alt;
    data_shader[7].fp_alt[1]    = &nv30_fp_texture_color2_alt2;
    data_shader[7].size_vertex  = 16+4+8+8;

    data_shader[8].off_texture  = -1; // normal
    data_shader[8].off_texture2 = -1;
    data_shader[8].off_color    = -1;
    data_shader[8].fp           = &nv30_fp_color;
    data_shader[8].size_vertex  = 16+12;

    data_shader[9].off_texture2 = -1; // normal + texture
    data_shader[9].off_color    = -1;
    data_shader[9].fp           = &nv30_fp_texture_color;
    data_shader[9].size_vertex  = 16+12+8;

    data_shader[10].off_color    = -1; // normal + texture + texture2
    data_shader[10].fp           = &nv30_fp_texture_color2;
    data_shader[10].fp_alt[0]    = &nv30_fp_texture_color2_alt;
    data_shader[10].fp_alt[1]    = &nv30_fp_texture_color2_alt2;
    data_shader[10].size_vertex  = 16+12+8+8;

    current_shader = -1;

    rsx_vertex = rsxtiny_MemAlign(64, vertex_buff_size);

    pos_rsx_vertex = 0;

    polygon = -1;
    off_head_vertex = off_start_vertex = 0;

    sysUtilUnregisterCallback(EVENT_SLOT3);
    sysUtilRegisterCallback(EVENT_SLOT3, tiny3d_callback, NULL);

    atexit(tiny3d_Exit);

    return TINY3D_OK;
}

/***********************************************************************************************************/
/* CHANGE SHADER CONTEXT / PUT_VERTEX                                                                      */
/***********************************************************************************************************/

static void rsxtiny_SetVertexProgramConstant3f(tiny_gcmContextData *context, int constant, float values[3])
{
    float _values[4];
    _values[0] = values[0]; _values[1] = values[1]; _values[2] = values[2]; _values[3] = 0.0f;
    rsxtiny_SetVertexProgramConstant4f(context, constant, _values);
}

/*
static void rsxtiny_SetVertexProgramConstant1f(tiny_gcmContextData *context, int constant, float values)
{
    float _values[4];
    _values[0] = values; _values[1] = _values[2] = _values[3] = 0.0f;
    rsxtiny_SetVertexProgramConstant4f(context, constant, _values);
}
*/


static void Update_With_Normal()
{
    //if(current_shader < 5 || current_shader > 6) return;

    if(data_shader[current_shader].off_normal < 0) return;

    if(flag_vertex & VERTEX_SETMAT) {
        rsxtiny_SetVertexProgramConstant4fBlock(context, data_shader[current_shader].off_emissive, 4, (float *) &material.emissive[0]);
        flag_vertex &= ~VERTEX_SETMAT;
    }

    if(flag_vertex & VERTEX_SETAMBIENT) {
        light.ambient[3] = 1.0f;
        rsxtiny_SetVertexProgramConstant4f(context, data_shader[current_shader].off_lightAmbient, (float *) light.ambient);
        flag_vertex &= ~VERTEX_SETAMBIENT;
    }

    if(flag_vertex & VERTEX_SETLIGHT) {
        int i;
        for(i = 0; i < 4; i++) {

            rsxtiny_SetVertexProgramConstant4f(context, data_shader[current_shader].off_lightPosition + i, (float *) &light.pos[i][0]);
            if(light.pos[i][3] == 0) continue;
            rsxtiny_SetVertexProgramConstant3f(context, data_shader[current_shader].off_lightColor + i, (float *) &light.color[i][0]);

        }

        flag_vertex &= ~VERTEX_SETLIGHT;
    }

    if(flag_vertex & VERTEX_SETCAMERA) {
        rsxtiny_SetVertexProgramConstant3f(context, data_shader[current_shader].off_cameraPosition, (float *) &light.camera[0]);
        flag_vertex &= ~VERTEX_SETCAMERA;
    }
}

static void set_shader_offsets()
{
    u32 text_off = 16;
    static u32 off_vtx = 0;

    if(current_shader < 0) exit(0);

    off_head_vertex = 0;

    //Bind the memory array to the input attributes
    //rsx requires the offset in his memory area
    rsxtiny_AddressToOffset(&rsx_vertex[off_start_vertex],&off_vtx);

    //stride is the distance (in bytes) from the attribute in a vertex to the same attribute in the next vertex (that is, the size of a single vertex struct)
    //elements is the number of components of this attribute that will be passed to this input parameter in the vertex program (max 4)
    rsxtiny_BindVertexBufferAttribute(context, data_shader[current_shader].off_position, off_vtx, data_shader[current_shader].size_vertex, 4,
        REALITY_BUFFER_DATATYPE_FLOAT, REALITY_RSX_MEMORY);

    if(data_shader[current_shader].off_normal >= 0) {
        text_off += 12;

            rsxtiny_BindVertexBufferAttribute(context, data_shader[current_shader].off_normal, off_vtx + 16, data_shader[current_shader].size_vertex, 3,
                REALITY_BUFFER_DATATYPE_FLOAT, REALITY_RSX_MEMORY);
    } else
    if(data_shader[current_shader].off_color >= 0) {
        //now the color

        if(data_shader[current_shader].fixed_color) {
            text_off += 4;
            rsxtiny_BindVertexBufferAttribute(context, data_shader[current_shader].off_color, off_vtx + 16, data_shader[current_shader].size_vertex, 4,
                REALITY_BUFFER_DATATYPE_BYTE, REALITY_RSX_MEMORY);
        } else {
            text_off += 16;
            rsxtiny_BindVertexBufferAttribute(context, data_shader[current_shader].off_color, off_vtx + 16, data_shader[current_shader].size_vertex, 4,
                REALITY_BUFFER_DATATYPE_FLOAT, REALITY_RSX_MEMORY);
        }
    }

    if(data_shader[current_shader].off_texture >= 0) {
        //now the texture coords

        rsxtiny_BindVertexBufferAttribute(context, data_shader[current_shader].off_texture, off_vtx + text_off, data_shader[current_shader].size_vertex, 2,
            REALITY_BUFFER_DATATYPE_FLOAT, REALITY_RSX_MEMORY);

        if(data_shader[current_shader].off_texture2 >= 0) {
            //now the texture coords2

            rsxtiny_BindVertexBufferAttribute(context, data_shader[current_shader].off_texture2, off_vtx + text_off + 8, data_shader[current_shader].size_vertex, 2,
                REALITY_BUFFER_DATATYPE_FLOAT, REALITY_RSX_MEMORY);
        }
    }


}

static void set_shader_context(int old_shader)
{

    if(current_shader < 0) exit(0);

    rsxtiny_MakeCmdSpace(context, 0x1000);

    rsxtiny_VertexInvalidate(context);
    rsxtiny_InvalidateTextureCache(context, GCM_INVALIDATE_VERTEX_TEXTURE);

    tinycallback_flag = 2;
    if(old_shader == -1)
        rsxtiny_LoadVertexProgram(context, (rsxtiny_VertexProgram*) data_shader[current_shader].vp);
    tinycallback_flag = 0;

    //Pass the matrix to the shader

    if(use_2d) {
        rsxtiny_SetVertexProgramConstant4fBlock(context, data_shader[current_shader].off_modelv,  4, (float*)(model_view.data));
        rsxtiny_SetVertexProgramConstant4fBlock(context, data_shader[current_shader].off_project, 4, (float*)(matrix_ident.data));
    } else {
        rsxtiny_SetVertexProgramConstant4fBlock(context, data_shader[current_shader].off_modelv,  4, (float*)(model_view.data));
        rsxtiny_SetVertexProgramConstant4fBlock(context, data_shader[current_shader].off_project,  4, (float*)(project_mat.data));
    }

    if(data_shader[current_shader].off_normal >= 0) {

        flag_vertex |= VERTEX_MASK; Update_With_Normal();

    } else {

        light.ambient[3] = 0.0f;
        rsxtiny_SetVertexProgramConstant4f(context, data_shader[current_shader].off_lightAmbient, (float *) light.ambient);
    }

    flag_vertex &= ~VERTEX_MASKMATRIX;

    tinycallback_flag = 1;

    if(data_shader[current_shader].fp_yuv[0] && enable_yuv)
        rsxtiny_LoadFragmentProgram(context, data_shader[current_shader].fp_yuv[enable_yuv - 1]);
    else if(data_shader[current_shader].fp_alt[0] == NULL)
        rsxtiny_LoadFragmentProgram(context, data_shader[current_shader].fp);
    else {

        select_fp &= 15;

        if(!select_fp)
            rsxtiny_LoadFragmentProgram(context, data_shader[current_shader].fp);
        else
            rsxtiny_LoadFragmentProgram(context, data_shader[current_shader].fp_alt[select_fp-1]);
    }

    tinycallback_flag = 0;

}


static void put_vertex()
{

//    if(n_vertex == 0 && off_head_vertex == 0 && off_start_vertex == pos_rsx_vertex) pos_rsx_vertex = (pos_rsx_vertex + 63) & ~63;

    float *f_address_v = NULL;
    u32 *u_address_v = NULL;
    if(pos_rsx_vertex > (size_rsx_vertex - 1024)) {tiny_3d_alarm = 1;flag_vertex =(flag_vertex & ~VERTEX_SETPOS) | VERTEX_LOCK; return;}

    if(flag_vertex & VERTEX_SETPOS) {

        // POSITION
        f_address_v = (float*) &(rsx_vertex[pos_rsx_vertex]);
        *f_address_v++ = vertex_data.x;
        *f_address_v++ = vertex_data.y;
        *f_address_v++ = vertex_data.z;
        *f_address_v++ = vertex_data.w;
        pos_rsx_vertex += 16;

        if(flag_vertex & VERTEX_NORMAL) {

            f_address_v = (float*) &(rsx_vertex[pos_rsx_vertex]);
            *f_address_v++ = vertex_data.nx;
            *f_address_v++ = vertex_data.ny;
            *f_address_v++ = vertex_data.nz;
            pos_rsx_vertex += 12;

        } else
        // FLOAT COLOR
        if(flag_vertex & VERTEX_SETFCOL) {

            f_address_v = (float*) &(rsx_vertex[pos_rsx_vertex]);
            *f_address_v++ = vertex_data.r;
            *f_address_v++ = vertex_data.g;
            *f_address_v++ = vertex_data.b;
            *f_address_v++ = vertex_data.a;
            pos_rsx_vertex += 16;

        } else if(flag_vertex & VERTEX_SETCOL) {

        // UINT32 COLOR
            u_address_v = (u32*) &(rsx_vertex[pos_rsx_vertex]);
            *u_address_v = vertex_data.rgba;
            pos_rsx_vertex += 4;

        } else if(!(flag_vertex & VERTEX_SETTEXT)){ // default

        // UINT32 COLOR (default auto color)
            vertex_data.rgba = 0xffffffff;
            flag_vertex |= VERTEX_SETCOL;
            u_address_v = (u32*) &(rsx_vertex[pos_rsx_vertex]);
            *u_address_v = vertex_data.rgba;
            pos_rsx_vertex += 4;

        }

        // TEXTURE
        if(flag_vertex & VERTEX_SETTEXT) {

            f_address_v = (float*) &(rsx_vertex[pos_rsx_vertex]);
            *f_address_v++ = vertex_data.u;
            *f_address_v++ = vertex_data.v;
            pos_rsx_vertex += 8;

             if(flag_vertex & VERTEX_SETTEXT2) {

                f_address_v = (float*) &(rsx_vertex[pos_rsx_vertex]);
                *f_address_v++ = vertex_data.u2;
                *f_address_v++ = vertex_data.v2;
                pos_rsx_vertex += 8;
            }
        }

        n_vertex++;
    }

   flag_vertex &= ~VERTEX_SETPOS;

}

/***********************************************************************************************************/
/* POLYGON / VERTEX                                                                                        */
/***********************************************************************************************************/

int tiny3d_SetPolygon(type_polygon type)
{
    n_vertex = 0;

    flag_vertex = (flag_vertex &  VERTEX_MASK) | VERTEX_LOCK;

    if(polygon > 0) return TINY3D_BUSY;

    if(type < TINY3D_POINTS || type > TINY3D_POLYGON) return TINY3D_INVALID;

    if(grab_list) {
        push_list_cmd(LIST_SETPOLYGON, 1); push_list_int(type);
        flag_vertex &= ~VERTEX_LOCK;
        return TINY3D_OK;
    }

    polygon = type;

    switch(polygon) {

        CASE_POLYGON1(TINY3D_POINTS,         1, 1);
        CASE_POLYGON1(TINY3D_LINES,          2, 2);
        CASE_POLYGON1(TINY3D_LINE_LOOP,      2, 1);
        CASE_POLYGON1(TINY3D_LINE_STRIP,     2, 1);
        CASE_POLYGON1(TINY3D_TRIANGLES,      3, 3);
        CASE_POLYGON1(TINY3D_TRIANGLE_STRIP, 3, 1);
        CASE_POLYGON1(TINY3D_TRIANGLE_FAN,   3, 1);
        CASE_POLYGON1(TINY3D_QUADS,          4, 4);
        CASE_POLYGON1(TINY3D_QUAD_STRIP,     4, 2);
        CASE_POLYGON1(TINY3D_POLYGON,        3, 1)
    }

    pos_rsx_vertex = (pos_rsx_vertex + 63) & ~63;
    off_start_vertex = pos_rsx_vertex;

    flag_vertex &= ~VERTEX_LOCK;

    return TINY3D_OK;

}

void tiny3d_VertexPos(float x, float y, float z)
{

    if(flag_vertex & VERTEX_LOCK) return;

    if(grab_list) {
        push_position_list(x, y, z, 1.0f);
        return;
    }

    put_vertex(); // previous vertex;

    vertex_data.x = x; vertex_data.y = y; vertex_data.z = z; vertex_data.w = 1.0f;

    flag_vertex |= VERTEX_SETPOS;
}

void tiny3d_VertexPos4(float x, float y, float z, float w)
{
    if(flag_vertex & VERTEX_LOCK) return;

    if(grab_list) {
        push_position_list(x, y, z, w);
        return;
    }

    put_vertex(); // previous vertex;

    vertex_data.x = x; vertex_data.y = y; vertex_data.z = z; vertex_data.w = w;

    flag_vertex |= VERTEX_SETPOS;
}

void tiny3d_VertexPosVector(VECTOR v)
{

    if(flag_vertex & VERTEX_LOCK) return;

    if(grab_list) {
        push_position_list(v.x, v.y, v.z, 1.0f);
        return;
    }

    put_vertex(); // previous vertex;

    vertex_data.x = v.x; vertex_data.y = v.y; vertex_data.z = v.z; vertex_data.w = 1.0f;

    flag_vertex |= VERTEX_SETPOS;
}

void tiny3d_VertexColor(u32 rgba)
{

    vertex_data.rgba = rgba;

    if(flag_vertex & VERTEX_NORMAL) return;

    if(grab_list) {
        push_list_cmd(LIST_VERTEXCOLOR, 1); push_list_int(rgba);
        return;
    }

    flag_vertex |= VERTEX_SETCOL;

}

void tiny3d_VertexFcolor(float r, float g, float b, float a)
{

    vertex_data.r = r; vertex_data.g = g; vertex_data.b = b; vertex_data.a = a;

    if(flag_vertex & VERTEX_NORMAL) return;

     if(grab_list) {
        push_list_cmd(LIST_VERTEXFCOLOR, 4); push_list_float(r); push_list_float(g); push_list_float(b); push_list_float(a);
        return;
    }

    flag_vertex |= VERTEX_SETFCOL;
}

void tiny3d_VertexTexture(float u, float v)
{

    if(grab_list) {
        push_list_cmd(LIST_VERTEXTEXTURE, 2); push_list_float(u); push_list_float(v);
        return;
    }

    vertex_data.u = u; vertex_data.v = v;

    flag_vertex |= VERTEX_SETTEXT;

}

void tiny3d_VertexTexture2(float u, float v)
{

    if(!(flag_vertex & VERTEX_SETTEXT)) return;

    if(grab_list) {
        push_list_cmd(LIST_VERTEXTEXTURE2, 2); push_list_float(u); push_list_float(v);
        return;
    }

    vertex_data.u2 = u; vertex_data.v2 = v;

    flag_vertex |= VERTEX_SETTEXT2;
}

void tiny3d_Normal(float x, float y, float z)
{
    if(grab_list) {
        push_normal_list(x, y, z);
        return;
    }

    vertex_data.nx = x; vertex_data.ny = y; vertex_data.nz = z;

    flag_vertex |= VERTEX_NORMAL;
}

void tiny3d_NormalVector(VECTOR v)
{
    if(grab_list) {
        push_normal_list(v.x, v.y, v.z);
        return;
    }

    vertex_data.nx = v.x; vertex_data.ny = v.y; vertex_data.nz = v.z;

    flag_vertex |= VERTEX_NORMAL;
}


int tiny3d_End()
{

     if(grab_list) {

        push_list_cmd(LIST_POLYGONEND, 0);

        n_vertex = 0;

        polygon = -1;
        flag_vertex = (flag_vertex & VERTEX_MASK) |  VERTEX_LOCK;

        return TINY3D_OK;
    }

    put_vertex(); // set the last vertex

    if(polygon > 0 && n_vertex >= min_vertex) {

        int temp_shader = 0;

        n_vertex -= ((n_vertex - min_vertex) % mod_vertex);

        if(flag_vertex & VERTEX_SETTEXT) { // texture
            temp_shader = 2;
            if(flag_vertex & VERTEX_NORMAL) { // with normal and texture
                temp_shader = 9;
                flag_vertex &= ~ (VERTEX_SETFCOL | VERTEX_SETCOL);
            } else
            if(flag_vertex & VERTEX_SETFCOL) { // with color float
                temp_shader = 4;
            } else if(flag_vertex & VERTEX_SETCOL) { // with color u32
                temp_shader = 6;
            }

        if(flag_vertex & VERTEX_SETTEXT2) temp_shader++;

        } else if(flag_vertex & VERTEX_NORMAL) { // with normal
                temp_shader = 8;
                flag_vertex &= ~ (VERTEX_SETFCOL | VERTEX_SETCOL);
        } else if(flag_vertex & VERTEX_SETCOL) {

            temp_shader = 1;  // with color u32
        }

        // force to change the shaders under YUV

        if(enable_yuv) {current_shader = -1;}

        if(temp_shader != current_shader) { // set the shader

            int old_shader = current_shader;
            current_shader = temp_shader;
            set_shader_context(old_shader);
        } else if(data_shader[current_shader].fp_yuv[0] && enable_yuv) {

            tinycallback_flag = 1;

            rsxtiny_LoadFragmentProgram(context, data_shader[current_shader].fp_yuv[enable_yuv - 1]);

            tinycallback_flag = 0;
        }

        else if(data_shader[current_shader].fp_alt[0] != NULL && (select_fp & 128)!=0){

            select_fp &= 15; // disable Pixel Shader Update

            tinycallback_flag = 1;

            if(!select_fp)
                rsxtiny_LoadFragmentProgram(context, data_shader[current_shader].fp);
            else
                rsxtiny_LoadFragmentProgram(context, data_shader[current_shader].fp_alt[select_fp-1]);

            tinycallback_flag = 0;

        }


        select_fp &= 15; // disable Pixel Shader Update

        if(flag_vertex & VERTEX_SETMATRIX) { // update matrix

            rsxtiny_SetVertexProgramConstant4fBlock(context, data_shader[current_shader].off_modelv,  4, (float*)(model_view.data));
            flag_vertex &= ~VERTEX_SETMATRIX;
        }

        if(flag_vertex & VERTEX_SETPROJ) { // update matrix

             if(!use_2d)
                rsxtiny_SetVertexProgramConstant4fBlock(context, data_shader[current_shader].off_project,  4, (float*)(project_mat.data));
             else
                rsxtiny_SetVertexProgramConstant4fBlock(context, data_shader[current_shader].off_project, 4, (float*)(matrix_ident.data));

            flag_vertex &= ~VERTEX_SETPROJ;
        }

        set_shader_offsets();

        Update_With_Normal();
        tinycallback_flag = 0;
        if(n_vertex != 0) {
            tinycallback_flag = 2;
            rsxtiny_DrawVertexBuffer(context, polygon, off_head_vertex, n_vertex);
            tinycallback_flag = 0;
        }
        else exit(0);

        off_head_vertex += n_vertex;

        n_vertex = 0;

        polygon = -1;
        flag_vertex = (flag_vertex & VERTEX_MASK) |  VERTEX_LOCK;

    }

    return TINY3D_OK;

}

/***********************************************************************************************************/
/* MATRIX                                                                                                  */
/***********************************************************************************************************/

void tiny3d_SetMatrixModelView(MATRIX *mat)
{
    if(grab_list) {

        set_apply_matrix = 0;

        push_list_cmd(LIST_MATRIX, 16);

        if(!mat)
            RSX_MEMCPY((void *) &curr_list[curr_list_index], (void *) &matrix_ident, 4*4*4);
        else
            RSX_MEMCPY((void *) &curr_list[curr_list_index], (void *) mat, 4*4*4);

        curr_list_index += 16;

        return;
    }

    if(!mat)
        model_view = matrix_ident;
    else
        model_view = *mat;

    flag_vertex |= VERTEX_SETMATRIX;
}


void tiny3d_SetProjectionMatrix(MATRIX *mat)
{
    if(!mat)
        project_mat= matrix_ident;
    else
        project_mat = *mat;

    if(!use_2d)
        flag_vertex |= VERTEX_SETPROJ;
}



/***********************************************************************************************************/
/* SCREEN CLEAR / FLIP / CHANGE 2D/3D MODE                                                                 */
/***********************************************************************************************************/

/*
void tiny3d_WaitRSX()
{
    int i = 0;

    tiny3d_End();

    if(!CtrlReg) exit(0);

	while (CtrlReg->get != CtrlReg->put  && (i < 5000)) {
		usleep(200);CtrlReg = gcmGetControlRegister((gcmContextData *) context);
        __asm __volatile__("sync");
        if(!CtrlReg) exit(0);
		i++;
	}


}
*/

void tiny3d_Flip()
{

    tiny3d_End();
    //tiny3d_WaitRSX();

    waitFlip();
    current_shader = 0;

    flip(Video_currentBuffer); // Flip buffer onto screen
    //tiny3d_WaitRSX();
    Video_currentBuffer = !Video_currentBuffer;

    if(tinyerror_incallback) tiny_3d_alarm = 1;
    rsxtiny_MakeCmdSpace(context, 0x4000);
    setupRenderTarget(Video_currentBuffer);

    sysUtilCheckCallback();

    set_shader_context(-1);

    pos_rsx_vertex = 0;
    current_shader = -1;
    polygon = -1;
    off_head_vertex = off_start_vertex = 0;
    flag_vertex = VERTEX_LOCK;

}


static int user_viewport = 0;
static float x_viewport_2D = 0.0f;
static float y_viewport_2D = 0.0f;
static float x_viewport_3D = 0.0f;
static float y_viewport_3D = 0.0f;
static float scale2D_x_viewport = 1.0f;
static float scale2D_y_viewport = 1.0f;
static float scale3D_x_viewport = 1.0f;
static float scale3D_y_viewport = 1.0f;

static float scale2D_x_surface = 848.0;
static float scale2D_y_surface = 512.0;

void tiny3d_UserViewport(int onoff, float pos_x, float pos_y, float scale2D_x, float scale2D_y, float scale3D_x, float scale3D_y)
{
    user_viewport = onoff;
    x_viewport_2D = pos_x;
    y_viewport_2D = pos_y;
    x_viewport_3D = 0.0f;
    y_viewport_3D = 0.0f;
    scale2D_x_viewport = scale2D_x;
    scale2D_y_viewport = scale2D_y;
    scale3D_x_viewport = scale3D_x;
    scale3D_y_viewport = scale3D_y;
}

void tiny3d_UserViewport2(int onoff, float pos2D_x, float pos2D_y, float scale2D_x, float scale2D_y,
                                    float correction3D_x, float correction3D_y, float scale3D_x, float scale3D_y)
{
    user_viewport = onoff;
    x_viewport_2D = pos2D_x;
    y_viewport_2D = pos2D_y;
    x_viewport_3D = correction3D_x;
    y_viewport_3D = correction3D_y;
    scale2D_x_viewport = scale2D_x;
    scale2D_y_viewport = scale2D_y;
    scale3D_x_viewport = scale3D_x;
    scale3D_y_viewport = scale3D_y;
}

void tiny3d_UserViewportSurface(int onoff, float width_2D, float height_2D)
{
    user_viewport = onoff;

    scale2D_x_surface = width_2D;
    scale2D_y_surface = height_2D;

}

void tiny3d_Project2D()
{
    use_2d = 1;

    flag_vertex |= VERTEX_SETPROJ;


    if(render_target.target) {

        rsxtiny_ViewportTranslate(context, 0.0, 0.0, 0.0, 0.0);

        if(user_viewport)
            rsxtiny_ViewportScale(context, ((float) render_target.w) / scale2D_x_surface ,
                (float) (render_target.h) / scale2D_y_surface, Z_SCALE, 1.0);
        else
            rsxtiny_ViewportScale(context, ((float) render_target.w)/848.0 ,  (float) (render_target.h)/512.0f, Z_SCALE, 1.0);

        return;
    }

    if(user_viewport) {

        rsxtiny_ViewportTranslate(context, x_viewport_2D, y_viewport_2D, 0.0, 0.0);
        rsxtiny_ViewportScale(context, scale2D_x_viewport,  scale2D_y_viewport, Z_SCALE, 1.0);

        return;
    }

    // 480P / 576P
    if(Video_Resolution.width < 1280) {

        rsxtiny_ViewportTranslate(context, 38.0 , 16.0, 0.0, 0.0);

        rsxtiny_ViewportScale(context, (float) (Video_Resolution.width - 72) / 848.0,
            (Video_Resolution.height == 480) ? (512.0) / 576.0 : 548.0 / 512.0, Z_SCALE, 1.0);

    } else if(Video_Resolution.width == 1280) {
    // 720P

        rsxtiny_ViewportTranslate(context, 54.0, 24.0, 0.0, 0.0);

        rsxtiny_ViewportScale(context, 848.0 / 611.0 ,  674.0 / 512.0, Z_SCALE, 1.0);

    } else {
    // 1080i

        rsxtiny_ViewportTranslate(context, 63.0, 40.0, 0.0, 0.0);

        rsxtiny_ViewportScale(context, 848.0 / 400.0 ,  952.0 / 512.0, Z_SCALE, 1.0);

    }
}

void tiny3d_Project3D()
{
    use_2d = 0;

    flag_vertex |= VERTEX_SETPROJ;

    //tiny_rsxContextCallback(context, 64 * 1024);

    if(render_target.target) {

        rsxtiny_ViewportTranslate(context, ((float) render_target.w) / 2.0f, (float) (render_target.h) / 2.0f, 0.0, 0.0);
        rsxtiny_ViewportScale(context, ((float) render_target.w) / 1920.0f ,  (float) (render_target.h)/ 1080.0f, Z_SCALE, 1.0);

        return;
    }


    if(user_viewport) {

        rsxtiny_ViewportTranslate(context, (float) Video_Resolution.width / 2.0f + x_viewport_3D,
            (float) Video_Resolution.height / 2.0 + y_viewport_3D, 0.0, 0.0);
        rsxtiny_ViewportScale(context, scale3D_x_viewport,  scale3D_y_viewport, Z_SCALE, 1.0);

    } else {
        rsxtiny_ViewportTranslate(context, (float) Video_Resolution.width / 2.0f, (float) Video_Resolution.height / 2.0, 0.0, 0.0);
        rsxtiny_ViewportScale(context, (float) Video_Resolution.width / 1920.0f, (float) Video_Resolution.height / 1080.0f , Z_SCALE, 1.0);
    }

}


void tiny3d_Clear(u32 color, clear_flags flags)
{

    flag_vertex |= VERTEX_LOCK;

    render_target.target = 0;

    if(flags & TINY3D_CLEAR_COLOR) {

        tiny3d_Project2D();

        tiny3d_SetProjectionMatrix(&matrix_ident);
        tiny3d_SetMatrixModelView(&matrix_ident);

        rsxtiny_ZControl(context, 0, 1, 1); // disable viewport culling

        rsxtiny_DepthTestFunc(context, REALITY_ZFUNC_LESSOREQUAL);
        rsxtiny_DepthWriteEnable(context, 1);
        rsxtiny_DepthTestEnable(context, 1);

        rsxtiny_AlphaEnable(context, 0);

        rsxtiny_BlendEnable(context, 0);

        rsxtiny_Viewport(context, Video_Resolution.width, Video_Resolution.height);
        rsxtiny_SetScissor(context, 0, 0,Video_Resolution.width, Video_Resolution.height);



        int n;

        for(n = 0; n < 8; n++)
            {

            rsxtiny_ViewportClip(context, n, Video_Resolution.width, Video_Resolution.height);
            }

        setupRenderTarget(Video_currentBuffer);

        rsxtiny_SetClearColor(context, color);

    }

    if(flags & TINY3D_CLEAR_ZBUFFER) {

        rsxtiny_SetClearDepthValue(context, 0xffffffff);
    }

    rsxtiny_ClearBuffers(context, flags);

    if((flags & TINY3D_CLEAR_COLOR) && tiny_3d_alarm) {

        tiny3d_SetPolygon(TINY3D_QUADS);

        tiny3d_VertexPos(848/2  , 0  ,  65535);
        tiny3d_VertexColor(0xff0000ff);

        tiny3d_VertexPos(847, 0  , 65535);

        tiny3d_VertexPos(847, 511, 65535);

        tiny3d_VertexPos(848/2 , 511, 65535);
        tiny3d_End();

        rsxtiny_Viewport(context, Video_Resolution.width*15/16, Video_Resolution.height);
        int n;
        for(n = 0; n < 8; n++)
            rsxtiny_ViewportClip(context, n, Video_Resolution.width*15/16, Video_Resolution.height);
        rsxtiny_ClearBuffers(context, flags);
    }
}

void tiny3d_ClearSurface(u32 color, clear_flags flags, u32 rsx_offset, u32 width, u32 height, u32 stride, u32 format)
{

    flag_vertex |= VERTEX_LOCK;

    render_target.target = 1;
    render_target.w = width;
    render_target.p = stride;
    render_target.h = height;


    if(flags & TINY3D_CLEAR_COLOR) {

        tiny3d_Project2D();

        tiny3d_SetProjectionMatrix(&matrix_ident);
        tiny3d_SetMatrixModelView(&matrix_ident);

        rsxtiny_ZControl(context, 0, 1, 1); // disable viewport culling

        rsxtiny_DepthTestFunc(context, REALITY_ZFUNC_LESSOREQUAL);
        rsxtiny_DepthWriteEnable(context, 1);
        rsxtiny_DepthTestEnable(context, 1);

        rsxtiny_AlphaEnable(context, 0);

        rsxtiny_BlendEnable(context, 0);

        rsxtiny_Viewport(context, width, height);

        int n;

        for(n = 0; n < 8; n++)
            rsxtiny_ViewportClip(context, n, width, height);

        setupRenderTarget2(rsx_offset, stride, width, height, format);

        rsxtiny_SetClearColor(context, color);

    }

    if(flags & TINY3D_CLEAR_ZBUFFER) {

        rsxtiny_SetClearDepthValue(context, 0xffffffff);
    }

    rsxtiny_ClearBuffers(context, flags);
}

/***********************************************************************************************************/
/* ALPHA / BLEND func                                                                                      */
/***********************************************************************************************************/

void tiny3d_AlphaTest (int enable, u8 ref, alpha_func func)
{
    rsxtiny_AlphaFunc(context, func);

    rsxtiny_AlphaRef(context, (u32) ref);

    rsxtiny_AlphaEnable(context, enable!=0);
}

void tiny3d_BlendFunc (int enable, blend_src_func src_fun, blend_dst_func dst_func, blend_func func)
{
    rsxtiny_BlendFunc(context, src_fun, dst_func);
    rsxtiny_BlendEquation(context, func);
    rsxtiny_BlendEnable(context, enable!=0);
}

/***********************************************************************************************************/
/* TEXTURES                                                                                                */
/***********************************************************************************************************/

void tiny3d_SetTexture(u32 unit, u32 offset, u32 width, u32 height, u32 stride, text_format fmt, int smooth)
{

    if(grab_list) {

        push_list_cmd(LIST_SETTEXTURE, 9); push_list_int(unit); push_list_int(offset); push_list_int(width); push_list_int(height);
        push_list_int(stride);push_list_int(fmt); push_list_int(TEXTWRAP_REPEAT); push_list_int(TEXTWRAP_REPEAT); push_list_int(smooth);

        return;
    }

    tiny3d_SetTextureWrap(unit, offset, width, height, stride, fmt, TEXTWRAP_REPEAT, TEXTWRAP_REPEAT, smooth);

}


void tiny3d_SetTextureWrap(u32 unit, u32 offset, u32 width, u32 height, u32 stride, text_format fmt, int wrap_u, int wrap_v, int smooth)
{
    rsxtiny_Texture tex;

    if(grab_list) {

        push_list_cmd(LIST_SETTEXTURE, 9); push_list_int(unit); push_list_int(offset); push_list_int(width); push_list_int(height);
        push_list_int(stride);push_list_int(fmt); push_list_int(wrap_u); push_list_int(wrap_v); push_list_int(smooth);
        return;
    }

    tex.swizzle =
    NV30_3D_TEX_SWIZZLE_S0_X_S1 | NV30_3D_TEX_SWIZZLE_S0_Y_S1 |
    NV30_3D_TEX_SWIZZLE_S0_Z_S1 | NV30_3D_TEX_SWIZZLE_S0_W_S1 |
    NV30_3D_TEX_SWIZZLE_S1_X_X | NV30_3D_TEX_SWIZZLE_S1_Y_Y |
    NV30_3D_TEX_SWIZZLE_S1_Z_Z | NV30_3D_TEX_SWIZZLE_S1_W_W ;

    tex.offset = offset;

    tex.format = fmt |
    NV40_3D_TEX_FORMAT_LINEAR  |
    NV30_3D_TEX_FORMAT_DIMS_2D |
    NV30_3D_TEX_FORMAT_DMA0 |
    NV30_3D_TEX_FORMAT_NO_BORDER | (0x8000) |
    (1 << NV40_3D_TEX_FORMAT_MIPMAP_COUNT__SHIFT);

    tex.wrap = NV30_3D_TEX_WRAP_R_REPEAT;

    switch(wrap_u) {

        case TEXTWRAP_CLAMP:
            tex.wrap |= NV30_3D_TEX_WRAP_S_CLAMP_TO_EDGE;
            break;
        case TEXTWRAP_MIRROR:
            tex.wrap |= NV40_3D_TEX_WRAP_S_MIRROR_CLAMP_TO_EDGE;
            break;
        default:
            tex.wrap |= NV30_3D_TEX_WRAP_S_REPEAT;
            break;
    }

    switch(wrap_v) {

        case TEXTWRAP_CLAMP:
            tex.wrap |= NV30_3D_TEX_WRAP_T_CLAMP_TO_EDGE;
            break;
        case TEXTWRAP_MIRROR:
            tex.wrap |= NV40_3D_TEX_WRAP_T_MIRROR_CLAMP_TO_EDGE;
            break;
        default:
            tex.wrap |= NV30_3D_TEX_WRAP_T_REPEAT;
            break;
    }

   // if(clamp_v) tex.wrap |= NV30_3D_TEX_WRAP_T_CLAMP_TO_EDGE; else tex.wrap |= NV30_3D_TEX_WRAP_T_REPEAT;

    tex.enable = NV40_3D_TEX_ENABLE_ENABLE;

    if(smooth)
    tex.filter = NV30_3D_TEX_FILTER_MIN_LINEAR |
           NV30_3D_TEX_FILTER_MAG_LINEAR | 0x3fd6;
    else
    tex.filter = NV30_3D_TEX_FILTER_MIN_NEAREST |
           NV30_3D_TEX_FILTER_MAG_NEAREST | 0x3fd6;

    tex.width = width;
    tex.height = height;
    tex.stride = stride;

    rsxtiny_InvalidateTextureCache(context, GCM_INVALIDATE_TEXTURE);
    rsxtiny_SetTexture(context, unit, &tex);
}

void * tiny3d_AllocTexture(u32 size)
{
   return rsxtiny_MemAlign(128, size);
}

u32 tiny3d_TextureOffset(void * text)
{
    u32 offset = 0;

    rsxtiny_AddressToOffset(text, &offset);

    return offset;
}

/***********************************************************************************************************/
/* LIGHTS & MATERIALS                                                                                      */
/***********************************************************************************************************/

void tiny3d_EmissiveMaterial(float r, float g, float b, float a)
{
    if(grab_list) {

        push_list_cmd(LIST_EMISSIVE_MAT, 4); push_list_float(r); push_list_float(g); push_list_float(b); push_list_float(a);

        return;
    }

    material.emissive[0] = r;
    material.emissive[1] = g;
    material.emissive[2] = b;
    material.emissive[3] = a;

    flag_vertex |= VERTEX_SETMAT;
}

void tiny3d_AmbientMaterial(float r, float g, float b, float a)
{
    if(grab_list) {

        push_list_cmd(LIST_AMBIENT_MAT, 4); push_list_float(r); push_list_float(g); push_list_float(b); push_list_float(a);

        return;
    }

    material.ambient[0] = r;
    material.ambient[1] = g;
    material.ambient[2] = b;
    material.ambient[3] = a;

    flag_vertex |= VERTEX_SETMAT;
}

void tiny3d_DiffuseMaterial(float r, float g, float b, float a)
{
    if(grab_list) {

        push_list_cmd(LIST_DIFFUSE_MAT, 4); push_list_float(r); push_list_float(g); push_list_float(b); push_list_float(a);

        return;
    }

    material.diffuse[0] = r;
    material.diffuse[1] = g;
    material.diffuse[2] = b;
    material.diffuse[3] = a;

    flag_vertex |= VERTEX_SETMAT;
}

void tiny3d_SpecularMaterial(float r, float g, float b, float shininess)
{
    if(grab_list) {

        push_list_cmd(LIST_SPECULAR_MAT, 4); push_list_float(r); push_list_float(g); push_list_float(b); push_list_float(shininess);

        return;
    }

    material.specular[0] = r;
    material.specular[1] = g;
    material.specular[2] = b;
    material.specular[3] = shininess;

    flag_vertex |= VERTEX_SETMAT;
}

void tiny3d_SetLightCameraPosition(float x, float y, float z)
{

    light.camera[0] = x;
    light.camera[1] = y;
    light.camera[2] = z;
    light.camera[3] = 0.0f;

    flag_vertex |= VERTEX_SETCAMERA;
}

void tiny3d_SetLight(int indx, float x, float y, float z, float r, float g, float b, int mode)
{

    if(indx < 0 || indx > 3) return;

    light.pos[indx][0] = x;
    light.pos[indx][1] = y;
    light.pos[indx][2] = z;
    light.pos[indx][3] = mode ? ((mode == LIGHT_DIFFUSE) ? 0.5f : 1.0f) : 0.0f;

    light.color[indx][0] = r;
    light.color[indx][1] = g;
    light.color[indx][2] = b;
    light.color[indx][3] = 0.0f;

    flag_vertex |= VERTEX_SETLIGHT;
}

void tiny3d_SetLightsOff()
{
    int n;

    for(n = 0; n < 4; n++)
        light.pos[n][3] = 0.0f;

    flag_vertex |= VERTEX_SETLIGHT;
}

void tiny3d_SetAmbientLight(float r, float g, float b)
{

    light.ambient[0] = r;
    light.ambient[1] = g;
    light.ambient[2] = b;
    light.ambient[3] = 0.0f;

    flag_vertex |= VERTEX_SETAMBIENT;
}

/***********************************************************************************************************/
/* SHADERS CONTROL                                                                                         */
/***********************************************************************************************************/

void tiny3d_SelMultiTexturesMethod(u32 method)
{
    if(grab_list) {

        push_list_cmd(LIST_MULTITEXTURE, 1); push_list_int(method);

        return;
    }

    if(method > 2) method = 2;

    select_fp = 128 | (method & 15);
}

/***********************************************************************************************************/
/* MISC                                                                                                    */
/***********************************************************************************************************/

int tiny3d_MenuActive() {

    return tiny3d_menu_open;

}

void tiny3d_Enable_YUV(int select) {

    current_shader = -1;
    enable_yuv =1 + (select & 1);
}

void tiny3d_Disable_YUV() {

    current_shader = -1;
    enable_yuv = 0;
}

void tiny3d_Dirty_Status() {

    rsxtiny_MakeCmdSpace(context, 0x4000);
    tiny3d_Disable_YUV();

    n_vertex = 0;

    polygon = -1;

    flag_vertex = (flag_vertex & VERTEX_MASK) |  VERTEX_LOCK;

    tiny3d_Project2D();

}

void * tiny3d_Get_GCM_Context(void) {
    return (void *) context;
}

void tiny3d_DoCmd_Space(int space) {
    rsxtiny_MakeCmdSpace(context, (space <= 0) ? 0x4000: space);
}
