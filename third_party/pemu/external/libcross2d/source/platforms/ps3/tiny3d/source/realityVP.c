#include "realityVP.h"
#include "string.h"

void *rsxtiny_VertexProgramGetUCode(rsxtiny_VertexProgram *vertexprogram)
{
	unsigned char *ptr=(unsigned char *)vertexprogram;

	return (void*)(ptr+vertexprogram->UCodeOffset);
}

unsigned int rsxtiny_VertexProgramGetInputMask(rsxtiny_VertexProgram *vertexprogram)
{
	return vertexprogram->InputMask;
}

unsigned int rsxtiny_VertexProgramGetOutputMask(rsxtiny_VertexProgram *vertexprogram)
{
	return vertexprogram->OutputMask;
}

rsxtiny_VertexProgramAttribute *rsxtiny_VertexProgramGetAttributes(rsxtiny_VertexProgram *vertexprogram)
{
	return (rsxtiny_VertexProgramAttribute*) (((unsigned char*)vertexprogram)+vertexprogram->AttributesOffset);
}

int rsxtiny_VertexProgramGetInputAttribute(rsxtiny_VertexProgram *vertexprogram,const char *name)
{
	int i;
	rsxtiny_VertexProgramAttribute *attributes = rsxtiny_VertexProgramGetAttributes(vertexprogram);
	for(i=0;i<vertexprogram->NumAttributes;++i)
	{
		char *namePtr;
		if(attributes[i].NameOffset==0)
			continue;
		namePtr=((char*)vertexprogram)+attributes[i].NameOffset;
		if(strcasecmp(name,namePtr)==0)
			return attributes[i].Index;
	}

	return -1;
}

rsxtiny_VertexProgramConstant *rsxtiny_VertexProgramGetConstants(rsxtiny_VertexProgram *vertexprogram)
{
	return (rsxtiny_VertexProgramConstant*) (((unsigned char*)vertexprogram)+vertexprogram->ConstantsOffset);
}

int rsxtiny_VertexProgramGetConstant(rsxtiny_VertexProgram *vertexprogram,const char *name)
{
	int i;
	rsxtiny_VertexProgramConstant *constants = rsxtiny_VertexProgramGetConstants(vertexprogram);
	for(i=0;i<vertexprogram->NumConstants;++i)
	{
		char *namePtr;
		if(constants[i].NameOffset==0)
			continue;
		namePtr=((char*)vertexprogram)+constants[i].NameOffset;
		if(strcasecmp(name,namePtr)==0)
			return constants[i].Index;
	}

	return -1;
}