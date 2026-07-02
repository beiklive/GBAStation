#version 460

#cmakedefine OBJRegular
#cmakedefine OBJAffine

layout (location = 0) in ivec4 InBounds;
layout (location = 1) in uvec3 InAddrInfo;
layout (location = 2) in float InDepth;

layout (location = 0) out vec2 OutInSpritePosition;
layout (location = 1) out uvec3 OutAddrInfo;
layout (location = 2) out uvec2 OutSize;


const vec2 Edges[4] = vec2[]
(
    vec2(0, 0),
    vec2(1, 0),
    vec2(0, 1),
    vec2(1, 1)
);

layout (std140, binding = 0) uniform OBJUniform
{
    uint OBJVRAMMask;
    vec4 AffineTransforms[32];
};

void main()
{
    ivec2 position = InBounds.xy;
    ivec2 size = InBounds.zw;
    ivec2 bounds = size;

    vec2 edge = Edges[gl_VertexID];

#ifdef OBJRegular
    OutInSpritePosition = edge * vec2(size);
    if ((InAddrInfo.y & 0x40000000U) != 0)
        OutInSpritePosition.x = float(size.x) - OutInSpritePosition.x;
    if ((InAddrInfo.y & 0x80000000U) != 0)
        OutInSpritePosition.y = float(size.y) - OutInSpritePosition.y;
#endif
#ifdef OBJAffine
    vec4 affineTransform = AffineTransforms[InAddrInfo.y >> 24];
    if ((InAddrInfo.y & 0x800000U) != 0)
        bounds <<= 1U;

    vec2 polarEdge = edge * 2 - vec2(1);

    OutInSpritePosition =
        vec2(size) * 0.5
        + (bounds.x * 0.5) * polarEdge.x * affineTransform.xz
        + (bounds.y * 0.5) * polarEdge.y * affineTransform.yw;
#endif
    OutInSpritePosition *= 256.0;

    OutAddrInfo = InAddrInfo;
    OutAddrInfo.y &= 0xFF;

    OutSize = size << 8;

    gl_Position = vec4((((edge * vec2(bounds)) + vec2(position)) / vec2(256.0, 192.0) - 0.5) * vec2(2, -2), InDepth, 1.0);
}