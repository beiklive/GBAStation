#version 450

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec2 inCoolTransparency;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec3 outCoolTransparency;

layout (std140, binding = 0) uniform Transformation
{
    mat4 Projection;
    float InvHeight;
} transform;

void main()
{
    gl_Position = transform.Projection * vec4(inPosition, 0.0, 1.0);

    outCoolTransparency = vec3(inPosition.y * transform.InvHeight, inCoolTransparency.x, inCoolTransparency.y);
    outUV = inUV;
    outColor = inColor;
}