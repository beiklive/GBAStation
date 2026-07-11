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

const float targetGamma = 1.91;
const float displayGamma = 1.91;
const float sat = 1.0;
const float lum = 0.89;
const float contrast = 1.0;
const vec3 blackLift = vec3(0.0);

const mat4 ndsColor = mat4(
    0.87,  0.10,  0.10, 0.0,
    0.255, 0.645, 0.17, 0.0,
   -0.125, 0.255, 0.73, 0.0,
    0.0,   0.0,   0.0,  0.0);

float coeffsX(int i)
{
    const float c[7] = float[](1.0, -2.0 / 3.0, -1.0 / 5.0, 4.0 / 7.0, -1.0 / 9.0, -2.0 / 11.0, 1.0 / 13.0);
    return c[i];
}

float coeffsY(int i)
{
    const float c[7] = float[](1.0, 0.0, -4.0 / 5.0, 2.0 / 7.0, 4.0 / 9.0, -4.0 / 11.0, 1.0 / 13.0);
    return c[i];
}

float intsmearFunc(float z, bool useY)
{
    float z2 = z * z;
    float zn = z;
    float ret = 0.0;
    for (int i = 0; i < 7; ++i)
    {
        ret += zn * (useY ? coeffsY(i) : coeffsX(i));
        zn *= z2;
    }
    return ret;
}

float intsmear(float x, float dx, float d, bool useY)
{
    float zl = clamp((x - dx * 0.5) / d, -1.0, 1.0);
    float zh = clamp((x + dx * 0.5) / d, -1.0, 1.0);
    return d * (intsmearFunc(zh, useY) - intsmearFunc(zl, useY)) / max(dx, 0.0001);
}

vec3 fetchOffset(ivec2 coord, ivec2 offset, float sourceScale,
                 float gain, float gammaValue, float blackLevel, float ambient)
{
    ivec2 size = textureSize(inTexture, 0);
    ivec2 physicalCoord = ivec2((vec2(coord + offset) + vec2(0.5)) * sourceScale);
    ivec2 clampedCoord = clamp(physicalCoord, ivec2(0), size - ivec2(1));
    vec3 sampled = texelFetch(inTexture, clampedCoord, 0).rgb;
    return pow(vec3(gain) * sampled + vec3(blackLevel), vec3(gammaValue)) + vec3(ambient);
}

vec3 applyNdsColor(vec3 color)
{
    vec4 screen = pow(vec4(color, 1.0), vec4(targetGamma));
    screen = mix(screen, vec4(0.5), 1.0 - contrast);

    mat4 adjust = mat4(
        (1.0 - sat) * 0.3086 + sat, (1.0 - sat) * 0.3086,       (1.0 - sat) * 0.3086,       1.0,
        (1.0 - sat) * 0.6094,       (1.0 - sat) * 0.6094 + sat, (1.0 - sat) * 0.6094,       1.0,
        (1.0 - sat) * 0.0820,       (1.0 - sat) * 0.0820,       (1.0 - sat) * 0.0820 + sat, 1.0,
        0.0,                         0.0,                         0.0,                         1.0);
    mat4 colorMatrix = ndsColor * adjust;
    screen = clamp(screen * lum, 0.0, 1.0);
    screen = colorMatrix * screen + vec4(blackLift, 0.0);
    return pow(clamp(screen.rgb, 0.0, 1.0), vec3(1.0 / displayGamma));
}

void main()
{
    vec4 sampled = texture(inTexture, inUV);
    vec2 texSize = vec2(textureSize(inTexture, 0));
    float sourceScale = max(ndsParams.runtime.x, 1.0);
    vec2 texelSize = vec2(sourceScale) / texSize;
    vec2 sourcePerOutput = max(fwidth(inUV) * texSize / sourceScale, vec2(0.0001));

    float gain = max(ndsParams.param0.x, 0.01);
    float gammaValue = max(ndsParams.param0.y, 0.01);
    float blackLevel = clamp(ndsParams.param0.z, 0.0, 0.5);
    float ambient = clamp(ndsParams.param0.w, 0.0, 0.5);
    float bgr = ndsParams.param1.x;
    float colorStrength = clamp(ndsParams.param1.y, 0.0, 1.0);

    vec3 cred = pow(vec3(0.75, 0.0, 0.0), vec3(2.2));
    vec3 cgreen = pow(vec3(0.0, 0.75, 0.0), vec3(2.2));
    vec3 cblue = pow(vec3(0.0, 0.0, 0.75), vec3(2.2));

    ivec2 tli = ivec2(floor(inUV / texelSize - vec2(0.4999)));

    float subpix = (inUV.x / texelSize.x - 0.4999 - float(tli.x)) * 3.0;
    float rsubpix = sourcePerOutput.x * 3.0;
    vec3 lcol = vec3(
        intsmear(subpix + 1.0, rsubpix, 1.5, false),
        intsmear(subpix,       rsubpix, 1.5, false),
        intsmear(subpix - 1.0, rsubpix, 1.5, false));
    vec3 rcol = vec3(
        intsmear(subpix - 2.0, rsubpix, 1.5, false),
        intsmear(subpix - 3.0, rsubpix, 1.5, false),
        intsmear(subpix - 4.0, rsubpix, 1.5, false));

    if (bgr > 0.5)
    {
        lcol = lcol.bgr;
        rcol = rcol.bgr;
    }

    subpix = inUV.y / texelSize.y - 0.4999 - float(tli.y);
    rsubpix = sourcePerOutput.y;
    float tcol = intsmear(subpix,       rsubpix, 0.63, true);
    float bcol = intsmear(subpix - 1.0, rsubpix, 0.63, true);

    vec3 topLeft = fetchOffset(tli, ivec2(0, 0), sourceScale, gain, gammaValue, blackLevel, ambient) * lcol * vec3(tcol);
    vec3 bottomRight = fetchOffset(tli, ivec2(1, 1), sourceScale, gain, gammaValue, blackLevel, ambient) * rcol * vec3(bcol);
    vec3 bottomLeft = fetchOffset(tli, ivec2(0, 1), sourceScale, gain, gammaValue, blackLevel, ambient) * lcol * vec3(bcol);
    vec3 topRight = fetchOffset(tli, ivec2(1, 0), sourceScale, gain, gammaValue, blackLevel, ambient) * rcol * vec3(tcol);

    vec3 gridColor = mat3(cred, cgreen, cblue) * (topLeft + bottomRight + bottomLeft + topRight);
    gridColor = pow(max(gridColor, vec3(0.0)), vec3(1.0 / 2.2));
    vec3 colorized = applyNdsColor(gridColor);
    vec3 rgb = mix(gridColor, colorized, colorStrength) * inColor.rgb;

    float alpha = sampled.a * inColor.a;
    alpha *= clamp(sqrt(coolTransparency.x), coolTransparency.y, coolTransparency.z);
    outColor = vec4(rgb, alpha);
}
