#ifndef REALITYVP_H
#define REALITYVP_H


// libreality vertex program


//Param types
#define PARAM_FLOAT		0
#define PARAM_FLOAT2	1
#define PARAM_FLOAT3	2
#define PARAM_FLOAT4	3


#define PARAM_FLOAT4x4	4

#pragma pack(push,1)
//#pragma pack(1)
typedef struct _VPAttribute
{
	unsigned int NameOffset;
	unsigned int Index;
} rsxtiny_VertexProgramAttribute;

union _VPFloatUInt
{
	unsigned int u;
	float f;
};

typedef struct _VPConstant
{
	unsigned int NameOffset;
	unsigned int Index;
	unsigned char Type;
	unsigned char Internal;	//Is an internal constant? (unnamed but needs to be set)
	union _VPFloatUInt Values[4];
} rsxtiny_VertexProgramConstant;

typedef struct _rsxtiny_VertexProgram
{
	unsigned short Magic;		//Magic text 'VP'
	unsigned short NumInsts;	//Number of vertex program instructions
	unsigned int UCodeOffset;	//Microcode offset (16 bytes per instruction)
	unsigned short NumConstants;	//Number of named constants (parameters)
	unsigned short NumAttributes;	//Number of named attributes (inputs)
	unsigned int ConstantsOffset;
	unsigned int AttributesOffset;
	unsigned int InputMask;			//Used input attributes
	unsigned int OutputMask;		//Used output registers

	//data (constants, attributes, ucode)
} rsxtiny_VertexProgram;

#pragma pack(pop)




void *rsxtiny_VertexProgramGetUCode(rsxtiny_VertexProgram *vertexprogram);

unsigned int rsxtiny_VertexProgramGetInputMask(rsxtiny_VertexProgram *vertexprogram);

unsigned int rsxtiny_VertexProgramGetOutputMask(rsxtiny_VertexProgram *vertexprogram);

rsxtiny_VertexProgramAttribute *rsxtiny_VertexProgramGetAttributes(rsxtiny_VertexProgram *vertexprogram);

int rsxtiny_VertexProgramGetInputAttribute(rsxtiny_VertexProgram *vertexprogram,const char *name);

rsxtiny_VertexProgramConstant *rsxtiny_VertexProgramGetConstants(rsxtiny_VertexProgram *vertexprogram);

int rsxtiny_VertexProgramGetConstant(rsxtiny_VertexProgram *vertexprogram,const char *name);

#endif
