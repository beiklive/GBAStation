#version 460

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 coolTransparency;

layout (binding = 0) uniform sampler2D inTexture;

layout (std140, binding = 1) uniform NdsShaderParams
{
    vec4 param0;
    vec4 param1;
    vec4 runtime;
} ndsParams;

float dist2(vec2 coord, vec2 source)
{
    vec2 delta = coord - source;
    return sqrt(dot(delta, delta));
}

float colorBloom(vec3 color)
{
    const vec3 grayCoeff = vec3(0.30, 0.59, 0.11);
    float bright = dot(color, grayCoeff);
    float shine = ndsParams.param0.y * 0.35;
    return mix(1.0 + shine, 1.0 - shine, bright);
}

vec3 lookup(vec2 pixelNo, vec2 sampleUV, vec2 offset)
{
    vec3 color = texture(inTexture, sampleUV).rgb;
    float delta = dist2(fract(pixelNo), offset + vec2(0.5, 0.5));
    float gammaValue = max(ndsParams.param0.x * 1.25, 0.01);
    return color * exp(-gammaValue * delta * colorBloom(color));
}

void main()
{
    vec2 texSize = vec2(textureSize(inTexture, 0));
    float sourceScale = max(ndsParams.runtime.x, 1.0);
    vec2 texel = vec2(sourceScale) / texSize;
    vec2 pixelNo = inUV * texSize / sourceScale;

    vec3 midColor = lookup(pixelNo, inUV, vec2(0.0, 0.0));
    vec3 color = vec3(0.0);
    color += lookup(pixelNo, inUV + texel * vec2(-1.0, -1.0), vec2(-1.0, -1.0));
    color += lookup(pixelNo, inUV + texel * vec2( 0.0, -1.0), vec2( 0.0, -1.0));
    color += lookup(pixelNo, inUV + texel * vec2( 1.0, -1.0), vec2( 1.0, -1.0));
    color += lookup(pixelNo, inUV + texel * vec2(-1.0,  0.0), vec2(-1.0,  0.0));
    color += midColor;
    color += lookup(pixelNo, inUV + texel * vec2( 1.0,  0.0), vec2( 1.0,  0.0));
    color += lookup(pixelNo, inUV + texel * vec2(-1.0,  1.0), vec2(-1.0,  1.0));
    color += lookup(pixelNo, inUV + texel * vec2( 0.0,  1.0), vec2( 0.0,  1.0));
    color += lookup(pixelNo, inUV + texel * vec2( 1.0,  1.0), vec2( 1.0,  1.0));

    float blend = clamp(ndsParams.param0.z * 0.55, 0.0, 1.0);
    vec3 outRgb = mix(1.05 * midColor, color * 0.42, blend) * inColor.rgb;
    float alpha = texture(inTexture, inUV).a * inColor.a;
    alpha *= clamp(sqrt(coolTransparency.x), coolTransparency.y, coolTransparency.z);
    outColor = vec4(outRgb, alpha);
}
