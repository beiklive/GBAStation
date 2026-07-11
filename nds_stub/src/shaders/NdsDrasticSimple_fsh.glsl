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

const float kPi = 3.141592654;
const float kNdsScreenHeight = 192.0;

float sourcePixelScale()
{
    return max(ndsParams.runtime.x, 1.0);
}

vec2 logicalPixel(vec2 uv)
{
    return uv * vec2(textureSize(inTexture, 0)) / sourcePixelScale();
}

float effectStrength()
{
    return clamp(ndsParams.param0.x, 0.0, 1.0);
}

float brightnessValue()
{
    return max(ndsParams.param0.y, 0.0);
}

float contrastValue()
{
    return max(ndsParams.param0.z, 0.0);
}

float saturationValue()
{
    return max(ndsParams.param0.w, 0.0);
}

float shaderGammaValue()
{
    return max(ndsParams.param1.x, 0.01);
}

float patternStrength()
{
    return max(ndsParams.param1.y, 0.0);
}

float curvatureValue()
{
    return max(ndsParams.param1.z, 0.0);
}

vec3 applyFinalAdjustments(vec3 filtered, vec3 original)
{
    vec3 color = mix(original, filtered, effectStrength());
    color = (color - vec3(0.5)) * contrastValue() + vec3(0.5);
    color *= brightnessValue();
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luma), color, saturationValue());
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / shaderGammaValue()));
    return clamp(color, 0.0, 1.0);
}

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
    vec2 angle = 2.0 * kPi * (logicalPixel(uv) - 0.25);
    float yFactor = (16.0 + sin(angle.y)) / 17.0;
    float xFactor = (4.0 + sin(angle.x)) / 5.0;
    return yFactor * xFactor;
}

float zfastWeight(vec2 uv, bool brightness)
{
    vec2 texcoordInPixels = logicalPixel(uv);
    vec2 centerCoord = floor(texcoordInPixels) + vec2(0.5);
    vec2 distFromCenter = abs(centerCoord - texcoordInPixels);
    float y = max(distFromCenter.x, distFromCenter.y);
    float yy = y * y;
    float yyy = yy * y;
    float lineWeight = 1.0 - 14.0 * (yy - 2.7 * yyy);

    if (!brightness)
        return lineWeight;

    vec2 angle = kPi * (texcoordInPixels / kNdsScreenHeight);
    float yFactor = (16.0 + sin(angle.y)) * (1.08 / 16.0);
    float xFactor = (4.0 + sin(angle.x)) * (1.08 / 4.0);
    return lineWeight * yFactor * xFactor;
}

float zfastPlainWeight(vec2 uv)
{
    vec2 angle = kPi * (logicalPixel(uv) / kNdsScreenHeight);
    float yFactor = (16.0 + sin(angle.y)) * (0.945 / 16.0);
    float xFactor = (4.0 + sin(angle.x)) * (0.945 / 4.0);
    return yFactor * xFactor;
}

vec3 applyScanlines(vec3 color, vec2 uv, int mode, float pattern)
{
    vec2 pixel = logicalPixel(uv);
    float line = fract(pixel.y * (mode == 19 || mode == 20 ? 0.5 : 1.0));
    float scan = smoothstep(0.12, 0.48, line) * smoothstep(0.98, 0.58, line);
    float strength = (mode == 18 || mode == 20 ? 0.48 : 0.34) * pattern;
    vec3 outColor = color * mix(1.0 - strength, 1.08, scan);

    if (mode == 18 || mode == 20)
        outColor = mix(outColor, outColor * 0.86 + outColor * outColor * 0.22, clamp(pattern, 0.0, 1.0));
    if (mode == 19 || mode == 20)
    {
        float diagonal = smoothstep(0.0, 0.5, fract((pixel.x + pixel.y) * 0.5));
        outColor *= mix(1.0, mix(0.82, 1.12, diagonal), clamp(pattern, 0.0, 1.0));
    }
    return clamp(outColor, 0.0, 1.0);
}

vec3 applyDot(vec2 uv, bool hv4, float pattern)
{
    vec2 texSize = vec2(textureSize(inTexture, 0));
    vec2 texel = vec2(sourcePixelScale()) / texSize;
    vec2 pixelNo = logicalPixel(uv);
    float gammaValue = hv4 ? 2.6 : 2.35;
    float shine = 0.0;
    float blend = hv4 ? 0.32 : 0.36;

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
    vec3 dotColor = mix(1.05 * midDot, color * (hv4 ? 0.58 : 0.44), blend);
    return mix(mid, dotColor, clamp(pattern, 0.0, 1.0));
}

vec3 crtMask(vec2 uv, float strength)
{
    vec2 pixel = logicalPixel(uv);
    float phase = mod(floor(pixel.x), 3.0);
    vec3 mask = vec3(1.0 - strength);
    if (phase < 1.0)
        mask.r = 1.0;
    else if (phase < 2.0)
        mask.g = 1.0;
    else
        mask.b = 1.0;
    return mask;
}

vec3 applyCrt(vec2 uv, bool colorShift, float pattern, float curvature)
{
    vec2 texSize = vec2(textureSize(inTexture, 0));
    vec2 texel = vec2(sourcePixelScale()) / texSize;
    vec2 centered = uv * 2.0 - 1.0;
    vec2 warpedUv = uv + centered * dot(centered, centered) * 0.018 * curvature;
    if (warpedUv.x < 0.0 || warpedUv.x > 1.0 || warpedUv.y < 0.0 || warpedUv.y > 1.0)
        return vec3(0.0);

    vec3 color;
    if (colorShift)
    {
        color.r = texture(inTexture, warpedUv + vec2(texel.x * 0.45, 0.0)).r;
        color.g = texture(inTexture, warpedUv - vec2(texel.x * 0.25, 0.0)).g;
        color.b = texture(inTexture, warpedUv).b;
    }
    else
    {
        color = texture(inTexture, warpedUv).rgb;
    }

    color = pow(max(color, vec3(0.0)), vec3(1.15));
    vec2 pixel = logicalPixel(warpedUv);
    float line = fract(pixel.y);
    float scan = smoothstep(0.08, 0.42, line) * smoothstep(1.0, 0.58, line);
    color *= mix(1.0, mix(0.62, 1.12, scan), clamp(pattern, 0.0, 1.0));
    color *= crtMask(warpedUv, (colorShift ? 0.18 : 0.26) * pattern);

    float vignette = 1.0 - 0.34 * dot(centered, centered);
    color *= clamp(vignette, 0.58, 1.0);
    return clamp(color, 0.0, 1.0);
}

void main()
{
    int mode = int(floor(ndsParams.param1.w + 0.5));
    vec4 sampled = texture(inTexture, inUV);
    vec3 rgb = sampled.rgb;
    vec3 originalRgb = rgb;
    float pattern = patternStrength();

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
        rgb *= mix(1.0, lcd1xWeight(inUV), pattern);
    }
    else if (mode == 9)
    {
        rgb *= mix(1.0, zfastPlainWeight(inUV), pattern);
    }
    else if (mode >= 10 && mode <= 14)
    {
        if (mode == 12 || mode == 14)
            rgb = applyNdsColor(rgb);
        if (mode == 13 || mode == 14)
            rgb = applyNaturalVision(rgb);
        rgb *= mix(1.0, zfastWeight(inUV, mode == 11), pattern);
    }
    else if (mode == 15)
    {
        rgb = applyQuilez(inUV);
    }
    else if (mode >= 17 && mode <= 20)
    {
        rgb = applyScanlines(rgb, inUV, mode, pattern);
    }
    else if (mode == 21)
    {
        rgb = applyDot(inUV, false, pattern);
    }
    else if (mode == 22)
    {
        rgb = applyDot(inUV, true, pattern);
    }
    else if (mode == 24)
    {
        rgb = applyCrt(inUV, false, pattern, curvatureValue());
    }
    else if (mode == 25)
    {
        rgb = applyCrt(inUV, true, pattern, curvatureValue());
    }

    rgb = applyFinalAdjustments(rgb, originalRgb);

    float alpha = sampled.a * inColor.a;
    alpha *= clamp(sqrt(coolTransparency.x), coolTransparency.y, coolTransparency.z);
    outColor = vec4(rgb * inColor.rgb, alpha);
}
