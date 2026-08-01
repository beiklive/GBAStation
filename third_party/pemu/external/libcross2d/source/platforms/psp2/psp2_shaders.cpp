//
// Created by cpasjuste on 27/01/17.
//

#include <vita2d.h>
#include "cross2d/platforms/psp2/psp2_shaders.h"

#include "lcd3x_v.h"
#include "lcd3x_f.h"
#include "texture_v.h"
#include "texture_f.h"
#include "advanced_aa_v.h"
#include "advanced_aa_f.h"
#include "sharp_bilinear_f.h"
#include "sharp_bilinear_v.h"
#include "sharp_bilinear_simple_f.h"
#include "sharp_bilinear_simple_v.h"

/*
Shaders performance
====================
1 x 384x224 texture, scale x 2
    texture                 60
    lcd3x                   60
    crt_easymode            15
    sharp_bilinear          60
    sharp_bilinear_simple   60
    advanced_aa             60
    xbr_2x_fast             8
2 x 384x224 textures, scale x 2
    texture                 60
    lcd3x                   47
    crt_easymode            8
    sharp_bilinear          55
    sharp_bilinear_simple   60
    advanced_aa             34
    xbr_2x_fast             4
*/

using namespace c2d;

PSP2ShaderList::PSP2ShaderList() : ShaderList() {
    PSP2ShaderList::add(new PSP2Shader(
            "c2d-texture", (SceGxmProgram *) texture_v, (SceGxmProgram *) texture_f));
    PSP2ShaderList::add(new PSP2Shader(
            "lcd3x", (SceGxmProgram *) lcd3x_v, (SceGxmProgram *) lcd3x_f));
    PSP2ShaderList::add(new PSP2Shader(
            "sharp+scan", (SceGxmProgram *) sharp_bilinear_v, (SceGxmProgram *) sharp_bilinear_f));
    PSP2ShaderList::add(new PSP2Shader(
            "sharp", (SceGxmProgram *) sharp_bilinear_simple_v, (SceGxmProgram *) sharp_bilinear_simple_f));
    PSP2ShaderList::add(new PSP2Shader(
            "advanced-aa", (SceGxmProgram *) advanced_aa_v, (SceGxmProgram *) advanced_aa_f));
}

PSP2Shader::PSP2Shader(const std::string &name, const SceGxmProgram *v, const SceGxmProgram *f)
        : ShaderList::Shader(name) {
    SceGxmShaderPatcher *shaderPatcher = vita2d_get_shader_patcher();

    sceGxmProgramCheck(v);
    sceGxmProgramCheck(f);
    sceGxmShaderPatcherRegisterProgram(shaderPatcher, v, &vertexProgramId);
    sceGxmShaderPatcherRegisterProgram(shaderPatcher, f, &fragmentProgramId);

    paramPositionAttribute = sceGxmProgramFindParameterByName(v, "aPosition");
    paramTexcoordAttribute = sceGxmProgramFindParameterByName(v, "aTexcoord");
    vertexInput.texture_size = sceGxmProgramFindParameterByName(v, "IN.texture_size");
    vertexInput.output_size = sceGxmProgramFindParameterByName(v, "IN.output_size");
    vertexInput.video_size = sceGxmProgramFindParameterByName(v, "IN.video_size");
    fragmentInput.texture_size = sceGxmProgramFindParameterByName(f, "IN.texture_size");
    fragmentInput.output_size = sceGxmProgramFindParameterByName(f, "IN.output_size");
    fragmentInput.video_size = sceGxmProgramFindParameterByName(f, "IN.video_size");

    // create texture vertex format
    SceGxmVertexAttribute textureVertexAttributes[2];
    SceGxmVertexStream textureVertexStreams[1];
    /* x,y,z: 3 float 32 bits */
    textureVertexAttributes[0].streamIndex = 0;
    textureVertexAttributes[0].offset = 0;
    textureVertexAttributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
    textureVertexAttributes[0].componentCount = 3; // (x, y, z)
    textureVertexAttributes[0].regIndex = (unsigned short)
            sceGxmProgramParameterGetResourceIndex(paramPositionAttribute);
    /* u,v: 2 floats 32 bits */
    textureVertexAttributes[1].streamIndex = 0;
    textureVertexAttributes[1].offset = 12; // (x, y, z) * 4 = 11 bytes
    textureVertexAttributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
    textureVertexAttributes[1].componentCount = 2; // (u, v)
    textureVertexAttributes[1].regIndex = (unsigned short)
            sceGxmProgramParameterGetResourceIndex(paramTexcoordAttribute);
    // 16 bit (short) indices
    textureVertexStreams[0].stride = sizeof(vita2d_texture_vertex);
    textureVertexStreams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

    // create texture shaders
    sceGxmShaderPatcherCreateVertexProgram(
            shaderPatcher, vertexProgramId, textureVertexAttributes,
            2, textureVertexStreams, 1, &vertexProgram);

    // Fill SceGxmBlendInfo
    static const SceGxmBlendInfo blend_info = {
            SCE_GXM_COLOR_MASK_ALL,
            SCE_GXM_BLEND_FUNC_ADD,
            SCE_GXM_BLEND_FUNC_ADD,
            SCE_GXM_BLEND_FACTOR_SRC_ALPHA,
            SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            SCE_GXM_BLEND_FACTOR_ONE,
            SCE_GXM_BLEND_FACTOR_ZERO
    };

    sceGxmShaderPatcherCreateFragmentProgram(
            shaderPatcher, fragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4, SCE_GXM_MULTISAMPLE_NONE,
            &blend_info, v, &fragmentProgram);
}

void PSP2Shader::setVertexUniform(const char *param, const float *value, unsigned int length) {
    void *vertex_wvp_buffer;
    const SceGxmProgram *program = sceGxmVertexProgramGetProgram(vertexProgram);
    const SceGxmProgramParameter *program_parameter = sceGxmProgramFindParameterByName(program, param);
    sceGxmReserveVertexDefaultUniformBuffer(vita2d_get_context(), &vertex_wvp_buffer);
    sceGxmSetUniformDataF(vertex_wvp_buffer, program_parameter, 0, length, value);
}

void PSP2Shader::setFragmentUniform(const char *param, const float *value, unsigned int length) {
    void *fragment_wvp_buffer;
    const SceGxmProgram *program = sceGxmFragmentProgramGetProgram(fragmentProgram);
    const SceGxmProgramParameter *program_parameter = sceGxmProgramFindParameterByName(program, param);
    sceGxmReserveFragmentDefaultUniformBuffer(vita2d_get_context(), &fragment_wvp_buffer);
    sceGxmSetUniformDataF(fragment_wvp_buffer, program_parameter, 0, length, value);
}

void PSP2Shader::setVertexUniform(SceGxmProgramParameter *param, const float *value, unsigned int length) {
    void *vertex_wvp_buffer;
    sceGxmReserveVertexDefaultUniformBuffer(vita2d_get_context(), &vertex_wvp_buffer);
    sceGxmSetUniformDataF(vertex_wvp_buffer, param, 0, length, value);
}

void PSP2Shader::setFragmentUniform(SceGxmProgramParameter *param, const float *value, unsigned int length) {
    void *fragment_wvp_buffer;
    sceGxmReserveFragmentDefaultUniformBuffer(vita2d_get_context(), &fragment_wvp_buffer);
    sceGxmSetUniformDataF(fragment_wvp_buffer, param, 0, length, value);
}

PSP2Shader::~PSP2Shader() {
    SceGxmShaderPatcher *shaderPatcher = vita2d_get_shader_patcher();
    sceGxmShaderPatcherReleaseFragmentProgram(shaderPatcher, fragmentProgram);
    sceGxmShaderPatcherReleaseVertexProgram(shaderPatcher, vertexProgram);
    sceGxmShaderPatcherUnregisterProgram(shaderPatcher, fragmentProgramId);
    sceGxmShaderPatcherUnregisterProgram(shaderPatcher, vertexProgramId);
}
