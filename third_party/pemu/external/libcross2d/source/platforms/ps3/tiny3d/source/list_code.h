#define LIST_SETPOLYGON      1
#define LIST_POLYGONEND      2
#define LIST_VERTEXPOS      16
#define LIST_VERTEXCOLOR    17
#define LIST_VERTEXFCOLOR   18
#define LIST_VERTEXTEXTURE  19
#define LIST_VERTEXTEXTURE2 20
#define LIST_VERTEXNORMAL   21

#define LIST_MATRIX         32
#define LIST_MATRIXDYN      33
#define LIST_SETTEXTURE     34
#define LIST_MULTITEXTURE   35
#define LIST_EMISSIVE_MAT   36
#define LIST_AMBIENT_MAT    37
#define LIST_DIFFUSE_MAT    38
#define LIST_SPECULAR_MAT   39

#define LIST_NEWLIST 444
#define LIST_STOP 555
#define LIST_NEXT 666

#define LIST_BLOCK_SIZE 65536

#ifndef RSX_MEMCPY
#define RSX_MEMCPY    __builtin_memcpy
#endif // RSX_MEMCPY

static int grab_list = 0;
static u32 *list_base;
static u32 *last_list;

static u32 *curr_list;
static int curr_list_index=0;

static int set_apply_matrix = 0;
static MATRIX apply_matrix;

static void push_list_cmd(u32 cmd, u32 params)
{
    if(grab_list !=1 ) return;

    if(curr_list_index >= (LIST_BLOCK_SIZE-72)/4) {
        u32 *p = malloc(LIST_BLOCK_SIZE);
        if(!p) {curr_list[curr_list_index++] = LIST_STOP<<16; grab_list = 2; return;}
        curr_list[curr_list_index++] = (LIST_NEXT<<16) | 1;
        last_list = &curr_list[curr_list_index];
        curr_list[curr_list_index++] = (u32) (u64) p;
        curr_list = p; curr_list_index = 0;
        curr_list[curr_list_index] = LIST_STOP<<16;
    }

    curr_list[curr_list_index++] = (cmd<<16) | params;
}

static inline  void push_list_int(int value)
{
    if(grab_list !=1 ) return;
    curr_list[curr_list_index++] = value;
}

static inline void push_list_float(float value)
{
    if(grab_list !=1 ) return;
    *((float *) &curr_list[curr_list_index++]) = value;
}

static void push_position_list(float x, float y, float z, float w)
{

    if(set_apply_matrix) {

        VECTOR v;

        v.x = x; v.y = y; v.z = z;

        v = MatrixVectorMultiply(apply_matrix, v);

        push_list_cmd(LIST_VERTEXPOS, 4); push_list_float(v.x); push_list_float(v.y); push_list_float(v.z); push_list_float(w);

    } else {

        push_list_cmd(LIST_VERTEXPOS, 4); push_list_float(x); push_list_float(y); push_list_float(z); push_list_float(w);

    }

}

static void push_normal_list(float x, float y, float z)
{

    if(set_apply_matrix) {

        VECTOR v;

        v.x = x; v.y = y; v.z = z;

        v = MatrixVectorMultiply3x3(apply_matrix, v);

        push_list_cmd(LIST_VERTEXNORMAL, 3); push_list_float(v.x); push_list_float(v.y); push_list_float(v.z);

    } else {

        push_list_cmd(LIST_VERTEXNORMAL, 3); push_list_float(x); push_list_float(y); push_list_float(z);

    }

}

int tiny3d_RecordList() {

    if(grab_list) return -1;

    set_apply_matrix = 0;

    list_base = malloc(LIST_BLOCK_SIZE);
    if(!list_base) return -1;

    last_list = NULL;

    curr_list = list_base; curr_list_index = 0;
    grab_list = 1;

    return 0;
}


void * tiny3d_StopList() {

    if(!grab_list) return NULL;

    if(grab_list == 1) {
        curr_list[curr_list_index++] = LIST_STOP<<16;
    }

    // optimize the list size

    if(curr_list_index < LIST_BLOCK_SIZE/4) {

        u32 *t = malloc(curr_list_index * 4);

        if(t) {
            RSX_MEMCPY((void *) t, (void *) curr_list, curr_list_index * 4);
            free(curr_list);
            if(last_list == NULL) list_base = t;
            else *last_list = (int) (long) t;
        }

    }


    curr_list_index = 0;
    grab_list = 0;
    return list_base;
}



#define get_list_int(x)  list[list_index+x]

#define  get_list_float(x)  *((float *) &list[list_index+x])

void tiny3d_DrawList(void * headlist)
{

    int list_index;

    if(!headlist) return;

    if(grab_list) {
        push_list_cmd(LIST_NEWLIST, 1);
        push_list_int((int)(long) headlist);
        return;
    }

    u32 *list = (u32 *) headlist;
    list_index = 0;

    MATRIX old_world = model_view;

    while(1) {
        u32 param = list[list_index] & 0xffff;
        u32 cmd = list[list_index++]>>16;
        switch(cmd) {

            case LIST_SETPOLYGON:
                tiny3d_SetPolygon(get_list_int(0));
                break;
            case LIST_POLYGONEND:
                tiny3d_End();
                break;
            case LIST_VERTEXPOS:
                tiny3d_VertexPos4(get_list_float(0), get_list_float(1), get_list_float(2), get_list_float(3));
                break;
            case LIST_VERTEXCOLOR:
                tiny3d_VertexColor(get_list_int(0));
                break;
            case LIST_VERTEXFCOLOR:
                tiny3d_VertexFcolor(get_list_float(0), get_list_float(1), get_list_float(2), get_list_float(3));
                break;
            case LIST_VERTEXTEXTURE:
                tiny3d_VertexTexture(get_list_float(0), get_list_float(1));
                break;
             case LIST_VERTEXTEXTURE2:
                tiny3d_VertexTexture2(get_list_float(0), get_list_float(1));
                break;
            case LIST_VERTEXNORMAL:
                tiny3d_Normal(get_list_float(0), get_list_float(1), get_list_float(2));
                break;

            case LIST_MATRIX:{
                MATRIX world;

                RSX_MEMCPY((void *) &world, (void *) &list[list_index], 4*4*4);
                world = MatrixMultiply(world, old_world);
                tiny3d_SetMatrixModelView(&world);
                break;
            }

            case LIST_MATRIXDYN:{
                MATRIX world;

                RSX_MEMCPY((void *) &world, (void *) (long) list[list_index], 4*4*4);
                world = MatrixMultiply(world, old_world);
                tiny3d_SetMatrixModelView(&world);
                break;
            }

            case LIST_SETTEXTURE:
                tiny3d_SetTextureWrap(get_list_int(0), get_list_int(1), get_list_int(2), get_list_int(3), get_list_int(4),
                get_list_int(5), get_list_int(6), get_list_int(7), get_list_int(8));
                break;

            case LIST_MULTITEXTURE:
                tiny3d_SelMultiTexturesMethod(get_list_int(0));
                break;
            case LIST_EMISSIVE_MAT:
                tiny3d_EmissiveMaterial(get_list_float(0), get_list_float(1), get_list_float(2), get_list_float(3));
                break;
            case LIST_AMBIENT_MAT:
                tiny3d_AmbientMaterial(get_list_float(0), get_list_float(1), get_list_float(2), get_list_float(3));
                break;
            case LIST_DIFFUSE_MAT:
                tiny3d_DiffuseMaterial(get_list_float(0), get_list_float(1), get_list_float(2), get_list_float(3));
                break;
            case LIST_SPECULAR_MAT:
                tiny3d_SpecularMaterial(get_list_float(0), get_list_float(1), get_list_float(2), get_list_float(3));
                break;

            case LIST_NEWLIST:

                tiny3d_DrawList((void * ) (long) get_list_int(0));
                break;

            case LIST_NEXT:
                list = (u32 *) (u64) get_list_int(0);
                list_index = 0; param = 0;
                break;

            default:
                goto end;
                break;

        }

        list_index += param;

    }

end:

    tiny3d_SetMatrixModelView(&old_world);
}

void tiny3d_FreeList(void * headlist)
{
    if(grab_list) return;

    int list_index;

    u32 *list = (u32 *) headlist;
    list_index = 0;

    while(1) {

        u32 param = list[list_index] & 0xffff;
        u32 cmd = list[list_index++]>>16;
        switch(cmd) {

            case LIST_NEXT: {
                u32 *list2 = (u32 *) (u64) get_list_int(0);
                free(list); list = list2;
                list_index = 0; param = 0;
                break;
            }

            case LIST_STOP:
                free(list);
                goto end;
                break;
            default:
                break;

        }

        list_index += param;

    }

end: ;
}

void tiny3d_DynamicMatrixList(MATRIX *mat)
{
    if(grab_list) {

        set_apply_matrix = 0;

        push_list_cmd(LIST_MATRIXDYN, 1);

        if(!mat)
            push_list_int((int)(long) &matrix_ident);
        else
            push_list_int((int)(long) mat);

        return;
    }


}

void tiny3d_ApplyMatrixList(MATRIX *mat)
{
    if(grab_list) {

        set_apply_matrix = 1;

        if(!mat)
           apply_matrix = matrix_ident;
        else
           apply_matrix =  *mat;

        return;
    }


}
