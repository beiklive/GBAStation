#version 460

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor; 
layout (location = 2) in vec3 coolTransparency;

layout (binding = 0) uniform sampler2D inTexture;

void main()
{
    outColor = texture(inTexture, inUV) * inColor;

    // this cool transparency stuff is incredibly hacky
    // but it looks so cool!
    outColor.a *= clamp(sqrt(coolTransparency.x), coolTransparency.y, coolTransparency.z);
}