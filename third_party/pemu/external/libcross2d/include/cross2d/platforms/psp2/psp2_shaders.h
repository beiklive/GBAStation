//
// Created by cpasjuste on 27/01/17.
//

#ifndef C2D_PSP2_SHADERS_H
#define C2D_PSP2_SHADERS_H

#include "cross2d/skeleton/shader_list.h"

typedef struct vita2d_shader_input {
    const SceGxmProgramParameter *video_size;
    const SceGxmProgramParameter *texture_size;
    const SceGxmProgramParameter *output_size;
} vita2d_shader_input;

namespace c2d {

    class PSP2Shader : public ShaderList::Shader {
    public:
        PSP2Shader(const std::string &name, const SceGxmProgram *v, const SceGxmProgram *f);

        ~PSP2Shader() override;

        void setVertexUniform(const char *param, const float *value, unsigned int length);

        void setFragmentUniform(const char *param, const float *value, unsigned int length);

        void setVertexUniform(SceGxmProgramParameter *param, const float *value, unsigned int length);

        void setFragmentUniform(SceGxmProgramParameter *param, const float *value, unsigned int length);

        SceGxmShaderPatcherId vertexProgramId;
        SceGxmShaderPatcherId fragmentProgramId;
        SceGxmVertexProgram *vertexProgram;
        SceGxmFragmentProgram *fragmentProgram;
        const SceGxmProgramParameter *paramPositionAttribute;
        const SceGxmProgramParameter *paramTexcoordAttribute;
        vita2d_shader_input vertexInput;
        vita2d_shader_input fragmentInput;
    };

    class PSP2ShaderList : public ShaderList {
    public:
        PSP2ShaderList();
    };
}

#endif //C2D_PSP2_SHADERS_H
