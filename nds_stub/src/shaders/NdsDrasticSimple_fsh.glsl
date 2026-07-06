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
} ndsParams;

const float kPi = 3.141592654;
const float kNdsScreenHeight = 192.0;

vec3 applyNdsColor(vec3 color)
{
    const float targetGamma = 1.91;
    const float displayGamma = 1.91;
    const float lum = 0.89;
    const mat3 ndsColor = mat3(
        0.87,  0.10, 0.10,
        0.255, 0.645, 0.17,
       -0.125, 0.255, 0.73);

    color = pow(max(color, vec3(0.0)), vec3(targetGamma));
    color = clamp(color * lum, 0.0, 1.0);
    color = ndsColor * color;
    return pow(clamp(color, 0.0, 1.0), vec3(1.0 / displayGamma));
}

vec3 applyNaturalVision(vec3 color)
{
    const float gammaIn = 1.91;
    const float gammaOut = 1.91;
    const float yStrength = 1.1;
    const float iStrength = 1.1;
    const float qStrength = 1.1;
    const mat3 rgbToYiq = mat3(
        0.299,  0.595716,  0.211456,
        0.587, -0.274453, -0.522591,
        0.114, -0.321263,  0.311135);
    const mat3 yiqToRgb = mat3(
        1.0,       1.0,        1.0,
        0.95629572,-0.27212210,-1.10698902,
        0.62102442,-0.64738060, 1.70461500);
    const vec3 yiqLo = vec3(0.0, -0.595716, -0.522591);
    const vec3 yiqHi = vec3(1.0,  0.595716,  0.522591);

    color = pow(max(color, vec3(0.0)), vec3(gammaIn));
    color = rgbToYiq * color;
    color = vec3(pow(max(color.x, 0.0), yStrength), color.y * iStrength, color.z * qStrength);
    color = clamp(color, yiqLo, yiqHi);
    color = yiqToRgb * color;
    return pow(clamp(color, 0.0, 1.0), vec3(1.0 / gammaOut));
}

vec3 sampleLinear(vec2 uv)
{
    ivec2 size = textureSize(inTexture, 0);
    vec2 pos = uv * vec2(size) - vec2(0.5);
    ivec2 base = ivec2(floor(pos));
    vec2 f = fract(pos);
    ivec2 p00 = clamp(base, ivec2(0), size - ivec2(1));
    ivec2 p10 = clamp(base + ivec2(1, 0), ivec2(0), size - ivec2(1));
    ivec2 p01 = clamp(base + ivec2(0, 1), ivec2(0), size - ivec2(1));
    ivec2 p11 = clamp(base + ivec2(1, 1), ivec2(0), size - ivec2(1));
    vec3 c00 = texelFetch(inTexture, p00, 0).rgb;
    vec3 c10 = texelFetch(inTexture, p10, 0).rgb;
    vec3 c01 = texelFetch(inTexture, p01, 0).rgb;
    vec3 c11 = texelFetch(inTexture, p11, 0).rgb;
    return mix(mix(c00, c10, f.x), mix(c01, c11, f.x), f.y);
}

vec3 applyQuilez(vec2 uv)
{
    vec2 texSize = vec2(textureSize(inTexture, 0));
    vec2 p = uv * texSize + vec2(0.5);
    vec2 i = floor(p);
    vec2 f = p - i;
    f = f * f * f * (f * (f * 6.0 - vec2(15.0)) + vec2(10.0));
    p = i + f;
    p = (p - vec2(0.5)) / texSize;
    return texture(inTexture, p).rgb;
}

float lcd1xWeight(vec2 uv)
{
    vec2 texSize = vec2(textureSize(inTexture, 0));
    vec2 angle = 2.0 * kPi * ((uv * texSize) * kNdsScreenHeight / texSize.y - 0.25);
    float yFactor = (16.0 + sin(angle.y)) / 17.0;
    float xFactor = (4.0 + sin(angle.x)) / 5.0;
    return yFactor * xFactor;
}

float zfastWeight(vec2 uv, bool brightness)
{
    vec2 texSize = vec2(textureSize(inTexture, 0));
    vec2 texcoordInPixels = uv * texSize * (kNdsScreenHeight / texSize.y);
    vec2 centerCoord = floor(texcoordInPixels) + vec2(0.5);
    vec2 distFromCenter = abs(centerCoord - texcoordInPixels);
    float y = max(distFromCenter.x, distFromCenter.y);
    float yy = y * y;
    float yyy = yy * y;
    float lineWeight = 1.0 - 14.0 * (yy - 2.7 * yyy);

    if (!brightness)
        return lineWeight;

    vec2 angle = kPi * (uv * texSize / texSize.y);
    float yFactor = (16.0 + sin(angle.y)) * (1.08 / 16.0);
    float xFactor = (4.0 + sin(angle.x)) * (1.08 / 4.0);
    return lineWeight * yFactor * xFactor;
}

float zfastPlainWeight(vec2 uv)
{
    vec2 texSize = vec2(textureSize(inTexture, 0));
    vec2 angle = kPi * (uv * texSize / texSize.y);
    float yFactor = (16.0 + sin(angle.y)) * (0.945 / 16.0);
    float xFactor = (4.0 + sin(angle.x)) * (0.945 / 4.0);
    return yFactor * xFactor;
}

vec3 applyScanlines(vec3 color, vec2 uv, int mode)
{
    float amount = 170.0;
    float intensity = 0.5;
    float pos0 = (uv.x + 1.0) * amount;
    float pos1 = (uv.y + 1.0) * amount;
    float pos2 = cos((fract(pos0 + pos1) - 0.5) * kPi * intensity);
    float pos3 = cos((fract(pos0 - pos1) - 0.5) * kPi * intensity);

    vec3 contrastColor = color * 0.6;
    if (mode == 18 || mode == 20)
        contrastColor = color * 0.4 + 0.24 * color * color;

    float mask = pos2 * (mode == 17 || mode == 18 ? 1.5 : 0.8);
    if (mode == 19 || mode == 20)
        mask = (pos2 + pos3) * 0.8;

    return mix(vec3(0.0), contrastColor, clamp(mask, 0.0, 1.0));
}

vec3 applyDot(vec2 uv, bool hv4)
{
    vec2 texSize = vec2(textureSize(inTexture, 0));
    vec2 texel = 1.0 / texSize;
    vec2 pixelNo = uv * texSize;
    float gammaValue = hv4 ? 2.0 : 1.8;
    float shine = 0.05;
    float blend = 0.65;

    vec3 mid = texture(inTexture, uv).rgb;
    vec3 color = vec3(0.0);
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            if (hv4 && abs(x) + abs(y) == 2)
                continue;
            vec3 sampleColor = texture(inTexture, uv + texel * vec2(x, y)).rgb;
            vec2 offset = vec2(float(x), float(y));
            vec2 delta = fract(pixelNo) - (offset + vec2(0.5));
            float bright = dot(sampleColor, vec3(0.30, 0.59, 0.11));
            float bloom = mix(1.0 + shine, 1.0 - shine, bright);
            color += sampleColor * exp(-gammaValue * sqrt(dot(delta, delta)) * bloom);
        }
    }
    vec2 centerDelta = fract(pixelNo) - vec2(0.5);
    vec3 midDot = mid * exp(-gammaValue * sqrt(dot(centerDelta, centerDelta)) *
                             mix(1.0 + shine, 1.0 - shine, dot(mid, vec3(0.30, 0.59, 0.11))));
    return mix(1.2 * midDot, color, blend);
}

void main()
{
    int mode = int(floor(ndsParams.param1.w + 0.5));
    vec4 sampled = texture(inTexture, inUV);
    vec3 rgb = sampled.rgb;

    if (mode == 0)
    {
        rgb = sampleLinear(inUV);
    }
    else if (mode == 1)
    {
        rgb = vec3(dot(rgb, vec3(0.299, 0.587, 0.114)));
    }
    else if (mode == 2)
    {
        rgb = applyNdsColor(rgb);
    }
    else if (mode == 3)
    {
        rgb = applyNaturalVision(rgb);
    }
    else if (mode == 4)
    {
        rgb = applyNaturalVision(applyNdsColor(rgb));
    }
    else if (mode >= 5 && mode <= 8)
    {
        if (mode == 6 || mode == 8)
            rgb = applyNdsColor(rgb);
        if (mode == 7 || mode == 8)
            rgb = applyNaturalVision(rgb);
        rgb *= lcd1xWeight(inUV);
    }
    else if (mode == 9)
    {
        rgb *= zfastPlainWeight(inUV);
    }
    else if (mode >= 10 && mode <= 14)
    {
        if (mode == 12 || mode == 14)
            rgb = applyNdsColor(rgb);
        if (mode == 13 || mode == 14)
            rgb = applyNaturalVision(rgb);
        rgb *= zfastWeight(inUV, mode == 11);
    }
    else if (mode == 15)
    {
        rgb = applyQuilez(inUV);
    }
    else if (mode >= 17 && mode <= 20)
    {
        rgb = applyScanlines(rgb, inUV, mode);
    }
    else if (mode == 21)
    {
        rgb = applyDot(inUV, false);
    }
    else if (mode == 22)
    {
        rgb = applyDot(inUV, true);
    }

    float alpha = sampled.a * inColor.a;
    alpha *= clamp(sqrt(coolTransparency.x), coolTransparency.y, coolTransparency.z);
    outColor = vec4(rgb * inColor.rgb, alpha);
}
