#include "commands.h"
#include  "old_nv40.h"
#include "buffer.h"
#include "rsxtiny.h"


#define NV406ETCL_SET_REF							0x00000050
#define NV406ETCL_SEMAPHORE_ACQUIRE					0x00000068
#define NV40TCL_SEMAPHORE_OFFSET					0x00001d6c
#define NV40TCL_SEMAPHORE_BACKENDWRITE_RELEASE		0x00001d70

void rsxtiny_MakeCmdSpace(tiny_gcmContextData *context, u32 space) {
	INTERNAL_COMMAND_LENGTH(context, space);
}

void rsxtiny_SetReferenceCommand(tiny_gcmContextData *context,u32 ref_value)
{
	INTERNAL_COMMAND_LENGTH(context, 2);
    internal_commandBufferPutCmd1(context, NV406ETCL_SET_REF, ref_value);
	
}

void rsxtiny_SetWriteBackendLabel(tiny_gcmContextData *context,u8 index,u32 value)
{
	INTERNAL_COMMAND_LENGTH(context, 4);

	u32 offset = 0x10*index;
	u32 wvalue = (value&0xff00ff00) | ((value>>16)&0xff) | ((value&0xff)<<16);
    internal_commandBufferPutCmd1(context, NV40TCL_SEMAPHORE_OFFSET, offset);
    internal_commandBufferPutCmd1(context, NV40TCL_SEMAPHORE_BACKENDWRITE_RELEASE, wvalue);
}

void rsxtiny_SetWaitLabel(tiny_gcmContextData *context,u8 index,u32 value)
{
	u32 offset = 0x10*index;

	INTERNAL_COMMAND_LENGTH(context, 4);
	internal_commandBufferPutCmd1(context, NV40TCL_SEMAPHORE_OFFSET, offset);
    internal_commandBufferPutCmd1(context, NV406ETCL_SEMAPHORE_ACQUIRE, value);
}

void rsxtiny_SetScissor(tiny_gcmContextData *context,u16 x,u16 y,u16 w,u16 h)
{
	INTERNAL_COMMAND_LENGTH(context, 3);

    internal_commandBufferPutCmd2(context, NV30_3D_SCISSOR_HORIZ, ((w<<16) | x), ((h<<16) | y));

}

void rsxtiny_SetClearColor(tiny_gcmContextData *context, uint32_t color) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_CLEAR_COLOR_VALUE, color);
}

void rsxtiny_SetClearDepthValue(tiny_gcmContextData *context, uint32_t value) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_CLEAR_DEPTH_VALUE, value);
}

void rsxtiny_Nop(tiny_gcmContextData *context) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, 0x100, 0);
}

void rsxtiny_ClearBuffers(tiny_gcmContextData *context, uint32_t buffers) {
	INTERNAL_COMMAND_LENGTH(context, 4);
	internal_commandBufferPutCmd1(context, NV30_3D_CLEAR_BUFFERS, buffers);
	internal_commandBufferPutCmd1(context, 0x100, 0); // Nop
}	

const static uint32_t offset_cmds[] = {NV30_3D_COLOR0_OFFSET, NV30_3D_COLOR1_OFFSET,
		NV40_3D_COLOR2_OFFSET, NV40_3D_COLOR3_OFFSET, NV30_3D_ZETA_OFFSET};
const static uint32_t dma_cmds[] = {NV30_3D_DMA_COLOR0, NV30_3D_DMA_COLOR1,
		NV40_3D_DMA_COLOR2, NV40_3D_DMA_COLOR3, NV30_3D_DMA_ZETA};
const static uint32_t pitch_cmds[] = {NV30_3D_COLOR0_PITCH, NV30_3D_COLOR1_PITCH,
		NV40_3D_COLOR2_PITCH, NV40_3D_COLOR3_PITCH, NV40_3D_ZETA_PITCH};

void rsxtiny_SetRenderSurface(tiny_gcmContextData *context, uint8_t surface, uint8_t location,
				uint32_t offset, uint32_t pitch) {
	INTERNAL_COMMAND_LENGTH(context, 6);
	internal_commandBufferPutCmd1(context, dma_cmds[surface], location | 0xFEED0000);
	internal_commandBufferPutCmd1(context, offset_cmds[surface], offset);
	internal_commandBufferPutCmd1(context, pitch_cmds[surface], pitch);
}

void rsxtiny_SelectRenderTarget(tiny_gcmContextData *context, uint8_t target, uint32_t format, 
				uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
	INTERNAL_COMMAND_LENGTH(context, 11);

	format |= (31 - __builtin_clz(width)) << 16;
	format |= (31 - __builtin_clz(height)) << 24;
	uint32_t conventions = height | 0x1000; // COORD_CONVENTIONS_ORIGIN_INVERTED
	
	internal_commandBufferPutCmd1(context, NV30_3D_RT_FORMAT, format); 
	internal_commandBufferPutCmd1(context, NV30_3D_RT_ENABLE, target);
	internal_commandBufferPutCmd1(context, NV30_3D_VIEWPORT_TX_ORIGIN, x | (y << 16));
	internal_commandBufferPutCmd1(context, NV30_3D_COORD_CONVENTIONS, conventions);
	internal_commandBufferPutCmd2(context, NV30_3D_RT_HORIZ, x | (width << 16), y | (height << 16));
}

void rsxtiny_Viewport(tiny_gcmContextData *context, uint16_t width, uint16_t height) {
	INTERNAL_COMMAND_LENGTH(context, 3);
	internal_commandBufferPutCmd2(context, NV30_3D_VIEWPORT_HORIZ, width << 16, height << 16);
}

void rsxtiny_ViewportClip(tiny_gcmContextData *context, u8 sel, uint16_t width, uint16_t height) {
    INTERNAL_COMMAND_LENGTH(context, 3);
	internal_commandBufferPutCmd2(context, NV30_3D_VIEWPORT_CLIP_HORIZ(sel), (width-1) << 16, (height-1) << 16);
}

void rsxtiny_VertexBegin(tiny_gcmContextData *context, uint32_t type) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, type);
}

void rsxtiny_VertexEnd(tiny_gcmContextData *context) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, NV30_3D_VERTEX_BEGIN_END_STOP);
}

void rsxtiny_Vertex4f(tiny_gcmContextData *context, float x, float y, float z, float w) {
	INTERNAL_COMMAND_LENGTH(context, 5);
	internal_commandBufferPutCmd4(context, NV30_3D_VTX_ATTR_4F(0),
				 ((rsxtiny_ieee32) x).u, 
				 ((rsxtiny_ieee32) y).u,
				 ((rsxtiny_ieee32) z).u, 
				 ((rsxtiny_ieee32) w).u);
}

void rsxtiny_TexCoord2f(tiny_gcmContextData *context, float s, float t) {
	INTERNAL_COMMAND_LENGTH(context, 3);
	internal_commandBufferPutCmd2(context, NV30_3D_VTX_ATTR_2F(8),
				 ((rsxtiny_ieee32) s).u, 
				 ((rsxtiny_ieee32) t).u);
}


void rsxtiny_LoadVertexProgram(tiny_gcmContextData *context, rsxtiny_VertexProgram *prog) {
	int inst;
	unsigned int *ucode = (unsigned int*)rsxtiny_VertexProgramGetUCode(prog);
	rsxtiny_VertexProgramConstant *constants;

    constants = rsxtiny_VertexProgramGetConstants(prog);


    INTERNAL_COMMAND_LENGTH(context, 3+3+5*prog->NumInsts + 6 * prog->NumConstants * (constants!=0) + 16); /* secure code*/
    //INTERNAL_COMMAND_LENGTH(context, 3);
	internal_commandBufferPutCmd2(context, NV30_3D_VP_UPLOAD_FROM_ID, 0, 0);
	
    for(inst = 0; inst < prog->NumInsts*4; inst += 4)
    {
    
        //INTERNAL_COMMAND_LENGTH(context, 5);
		internal_commandBufferPutCmd4(context, NV30_3D_VP_UPLOAD_INST(0/*inst & 31*/), 
					ucode[inst + 0],
					ucode[inst + 1],
					ucode[inst + 2],
					ucode[inst + 3]);
	}
    
   // INTERNAL_COMMAND_LENGTH(context, 2+3);
   // internal_commandBufferPutCmd1(context, NV30_3D_VP_START_FROM_ID, 0);
    internal_commandBufferPutCmd2(context, NV40_3D_VP_ATTRIB_EN, prog->InputMask, prog->OutputMask);
	
	if(constants)
	{
		int c;

		for(c = 0; c < prog->NumConstants; c++)
		{
            if(constants[c].Internal)
			    rsxtiny_SetVertexProgramConstant4f(context,constants[c].Index,(float*)constants[c].Values);
		}
	}
    
}


void rsxtiny_SetVertexProgramConstant4f(tiny_gcmContextData *context, int num, float values[4]){

	INTERNAL_COMMAND_LENGTH(context, 6);

	internal_commandBufferPutCmd5(context, NV30_3D_VP_UPLOAD_CONST_ID, 
							num,
							((rsxtiny_ieee32) (values[0])).u,
							((rsxtiny_ieee32) (values[1])).u,
							((rsxtiny_ieee32) (values[2])).u,
							((rsxtiny_ieee32) (values[3])).u
							);
}

void rsxtiny_SetVertexProgramConstant4fBlock(tiny_gcmContextData *context, int constant, int num4fConstants, float *values)
{
	int n;

    INTERNAL_COMMAND_LENGTH(context, 6 * num4fConstants);
	for(n=0;n<num4fConstants;++n)
	{
		rsxtiny_SetVertexProgramConstant4f(context,constant+n,values+4*n);
	}
}

void rsxtiny_InstallFragmentProgram(tiny_gcmContextData *context, rsxtiny_FragmentProgram *prog, uint32_t *addr) {
	// We don't actually need context, but if we leave it out people will forget.
	int i;
	for( i = 0; i < prog->size; ++i ) {
		addr[i] = (((prog->data[i] >> 16 ) & 0xffff) << 0) |
			    (((prog->data[i] >> 0 ) & 0xffff) << 16);
	}
    rsxtiny_AddressToOffset(addr, &prog->offset);
}

void rsxtiny_LoadFragmentProgram(tiny_gcmContextData *context, rsxtiny_FragmentProgram *prog) {
	INTERNAL_COMMAND_LENGTH(context, 4);
	
	internal_commandBufferPutCmd1(context, NV30_3D_FP_ACTIVE_PROGRAM,
				prog->offset | NV30_3D_FP_ACTIVE_PROGRAM_DMA0);
	internal_commandBufferPutCmd1(context, NV30_3D_FP_CONTROL, 
 				prog->num_regs << NV40_3D_FP_CONTROL_TEMP_COUNT__SHIFT);
}

void rsxtiny_InvalidateTextureCache(tiny_gcmContextData *context,u32 type)
{
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV40_3D_TEX_CACHE_CTL, type);
}

void rsxtiny_SetTexture(tiny_gcmContextData *context, uint32_t unit, rsxtiny_Texture *tex) {
	INTERNAL_COMMAND_LENGTH(context, 11);
	internal_commandBufferPutCmd8(context, NV30_3D_TEX_OFFSET(unit), tex->offset, tex->format,
		tex->wrap, tex->enable, tex->swizzle, tex->filter,
		(tex->width << 16) | tex->height, tex->borderColor);
	internal_commandBufferPutCmd1(context, NV40_3D_TEX_SIZE1(unit), tex->stride | 
		(1 << NV40_3D_TEX_SIZE1_DEPTH__SHIFT));
}

void rsxtiny_ViewportTranslate(tiny_gcmContextData *context, float x, float y, float z, float w) {
	INTERNAL_COMMAND_LENGTH(context, 5);
	internal_commandBufferPutCmd4(context, NV30_3D_VIEWPORT_TRANSLATE, 
				 ((rsxtiny_ieee32) x).u, 
				 ((rsxtiny_ieee32) y).u,
				 ((rsxtiny_ieee32) z).u, 
				 ((rsxtiny_ieee32) w).u);
}

void rsxtiny_ViewportScale(tiny_gcmContextData *context, float x, float y, float z, float w) {
	INTERNAL_COMMAND_LENGTH(context, 5);
	internal_commandBufferPutCmd4(context, NV30_3D_VIEWPORT_SCALE, 
				 ((rsxtiny_ieee32) x).u,
				 ((rsxtiny_ieee32) y).u,
				 ((rsxtiny_ieee32) z).u, 
				 ((rsxtiny_ieee32) w).u);
};

void rsxtiny_CullFace(tiny_gcmContextData *context, uint32_t face) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_CULL_FACE, face);
}

void rsxtiny_FrontFace(tiny_gcmContextData *context, uint32_t face) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_FRONT_FACE, face);
}

void rsxtiny_CullEnable(tiny_gcmContextData *context, uint32_t enable) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_CULL_FACE_ENABLE, enable);
}

void rsxtiny_AlphaEnable(tiny_gcmContextData *context, uint32_t enable) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_ALPHA_FUNC_ENABLE, enable);
}

void rsxtiny_AlphaRef(tiny_gcmContextData *context, uint32_t ref) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_ALPHA_FUNC_REF, ref);
}

void rsxtiny_AlphaFunc(tiny_gcmContextData *context, uint32_t func) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_ALPHA_FUNC_FUNC, func);
}

void rsxtiny_BlendEnable(tiny_gcmContextData *context, uint32_t enable) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_BLEND_FUNC_ENABLE, enable);
}

void rsxtiny_BlendEquation(tiny_gcmContextData *context, uint32_t equation) {
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV40_3D_BLEND_EQUATION, equation);
}

void rsxtiny_BlendFunc(tiny_gcmContextData *context, uint32_t src, uint32_t dest) {
	INTERNAL_COMMAND_LENGTH(context, 3);
	internal_commandBufferPutCmd2(context, NV30_3D_BLEND_FUNC_SRC, src, dest);
}

void rsxtiny_ZControl(tiny_gcmContextData *context, uint8_t cullNearFar, uint8_t zClampEnable, uint8_t cullIngnoreW) { // todo: documenent in nouveau
	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_DEPTH_CONTROL, cullNearFar | 
				(zClampEnable << 4) |
				(cullIngnoreW << 8));
}

void rsxtiny_BindVertexBufferAttribute(tiny_gcmContextData *context, uint8_t attribute, uint32_t offset, uint8_t stride, uint8_t elements, uint8_t dataType, uint8_t location)
{
	INTERNAL_COMMAND_LENGTH(context, 2+2);
	
	internal_commandBufferPutCmd1(context, NV30_3D_VTXBUF(attribute), offset|(location<<31));
	internal_commandBufferPutCmd1(context, NV30_3D_VTXFMT(attribute), (stride<<NV30_3D_VTXFMT_STRIDE__SHIFT)|(elements<<NV30_3D_VTXFMT_SIZE__SHIFT)|dataType);

}

void rsxtiny_VertexInvalidate(tiny_gcmContextData *context)
{
	INTERNAL_COMMAND_LENGTH(context, 4);
	internal_commandBufferPutCmd3(context, NV40_3D_VTX_CACHE_INVALIDATE, 0, 0, 0);
}

void rsxtiny_DrawVertexBuffer(tiny_gcmContextData *context, uint32_t type, uint32_t start, uint32_t count)
{
	INTERNAL_COMMAND_LENGTH(context, 6 + count * 2 + 2);

	internal_commandBufferPutCmd3(context, NV40_3D_VTX_CACHE_INVALIDATE, 0, 0, 0);
	internal_commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, type);

	while(count)
	{
		int num=count; //number of elements this call
		if(num>256)	//max 256 elements per call
			num=256;
		INTERNAL_COMMAND_LENGTH(context, 2);
		internal_commandBufferPutCmd1(context, NV30_3D_VB_VERTEX_BATCH, ((num-1)<<24)|(start & 0xffffff));
		count-=num;
		start+=num;
	}

	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, NV30_3D_VERTEX_BEGIN_END_STOP);
	

}

void rsxtiny_DrawVertexBufferIndex(tiny_gcmContextData *context, uint32_t type, uint32_t offset, uint32_t count, uint8_t dataType, uint8_t location)
{
	unsigned int current=0;

	INTERNAL_COMMAND_LENGTH(context, 9 + count * 2 + 2);

	internal_commandBufferPutCmd3(context, NV40_3D_VTX_CACHE_INVALIDATE, 0, 0, 0);

	internal_commandBufferPutCmd2(context, NV30_3D_IDXBUF_OFFSET, offset, (uint32_t)(dataType|location));

	internal_commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, type);

	while(count)
	{
		int num=count; //number of elements this call
		if(num>256)	//max 256 elements per call
			num=256;		
		INTERNAL_COMMAND_LENGTH(context, 2);
		internal_commandBufferPutCmd1(context, NV30_3D_VB_INDEX_BATCH, ((num-1)<<24)|current);
		count-=num;
		current+=num;
	}

	INTERNAL_COMMAND_LENGTH(context, 2);
	internal_commandBufferPutCmd1(context, NV30_3D_VERTEX_BEGIN_END, NV30_3D_VERTEX_BEGIN_END_STOP);
}

void rsxtiny_DepthTestEnable(tiny_gcmContextData *context, uint32_t enable)
{
	INTERNAL_COMMAND_LENGTH(context,2);
	internal_commandBufferPutCmd1(context, NV30_3D_DEPTH_TEST_ENABLE, enable);
}

void rsxtiny_DepthTestFunc(tiny_gcmContextData *context, uint32_t function)
{
	INTERNAL_COMMAND_LENGTH(context,2);
	internal_commandBufferPutCmd1(context, NV30_3D_DEPTH_FUNC, function);
}

void rsxtiny_DepthWriteEnable(tiny_gcmContextData *context, uint32_t enable)
{
	INTERNAL_COMMAND_LENGTH(context,2);
	internal_commandBufferPutCmd1(context, NV30_3D_DEPTH_WRITE_ENABLE, enable);
}
