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

#define BLEND_NONE 0
#define BLEND_NORMAL 1
#define BLEND_DOMINANT 2
#define LUMINANCE_WEIGHT 1.0
#define EQUAL_COLOR_TOLERANCE (30.0 / 255.0)
#define STEEP_DIRECTION_THRESHOLD 2.2
#define DOMINANT_DIRECTION_THRESHOLD 3.6

float distYCbCr(vec3 pixA, vec3 pixB)
{
    const vec3 w = vec3(0.2627, 0.6780, 0.0593);
    const float scaleB = 0.5 / (1.0 - w.b);
    const float scaleR = 0.5 / (1.0 - w.r);
    vec3 diff = pixA - pixB;
    float y = dot(diff.rgb, w);
    float cb = scaleB * (diff.b - y);
    float cr = scaleR * (diff.r - y);
    return sqrt(((LUMINANCE_WEIGHT * y) * (LUMINANCE_WEIGHT * y)) + (cb * cb) + (cr * cr));
}

bool isPixEqual(vec3 pixA, vec3 pixB)
{
    return distYCbCr(pixA, pixB) < EQUAL_COLOR_TOLERANCE;
}

float getLeftRatio(vec2 center, vec2 origin, vec2 direction, vec2 scale)
{
    vec2 p0 = center - origin;
    vec2 proj = direction * (dot(p0, direction) / dot(direction, direction));
    vec2 distv = p0 - proj;
    vec2 orth = vec2(-direction.y, direction.x);
    float side = sign(dot(p0, orth));
    float v = side * length(distv * scale);
    return smoothstep(-sqrt(2.0) / 2.0, sqrt(2.0) / 2.0, v);
}

bool eq(vec3 a, vec3 b) { return a == b; }
bool neq(vec3 a, vec3 b) { return a != b; }

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

vec3 samplePixel(vec2 coord, vec2 texel, vec2 offset)
{
    return texture(inTexture, coord + texel * offset).rgb;
}

void main()
{
    vec2 texSize = vec2(textureSize(inTexture, 0));
    float sourcePixelScale = max(ndsParams.runtime.x, 1.0);
    vec2 texel = vec2(sourcePixelScale) / texSize;
    vec2 pos = fract(inUV * texSize / sourcePixelScale) - vec2(0.5);
    vec2 coord = inUV - pos * texel;
    vec2 scale = 1.0 / max(fwidth(inUV * texSize / sourcePixelScale), vec2(0.001));

    vec3 A = samplePixel(coord, texel, vec2(-1.0, -1.0));
    vec3 B = samplePixel(coord, texel, vec2( 0.0, -1.0));
    vec3 C = samplePixel(coord, texel, vec2( 1.0, -1.0));
    vec3 D = samplePixel(coord, texel, vec2(-1.0,  0.0));
    vec3 E = samplePixel(coord, texel, vec2( 0.0,  0.0));
    vec3 F = samplePixel(coord, texel, vec2( 1.0,  0.0));
    vec3 G = samplePixel(coord, texel, vec2(-1.0,  1.0));
    vec3 H = samplePixel(coord, texel, vec2( 0.0,  1.0));
    vec3 I = samplePixel(coord, texel, vec2( 1.0,  1.0));

    ivec4 blendResult = ivec4(BLEND_NONE, BLEND_NONE, BLEND_NONE, BLEND_NONE);

    if (!((eq(E, F) && eq(H, I)) || (eq(E, H) && eq(F, I))))
    {
        float distHF = distYCbCr(G, E) + distYCbCr(E, C) +
                       distYCbCr(samplePixel(coord, texel, vec2(0.0, 2.0)), I) +
                       distYCbCr(I, samplePixel(coord, texel, vec2(2.0, 0.0))) +
                       4.0 * distYCbCr(H, F);
        float distEI = distYCbCr(D, H) +
                       distYCbCr(H, samplePixel(coord, texel, vec2(1.0, 2.0))) +
                       distYCbCr(B, F) +
                       distYCbCr(F, samplePixel(coord, texel, vec2(2.0, 1.0))) +
                       4.0 * distYCbCr(E, I);
        bool dominant = DOMINANT_DIRECTION_THRESHOLD * distHF < distEI;
        blendResult.z = (distHF < distEI && neq(E, F) && neq(E, H)) ? (dominant ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
    }

    if (!((eq(D, E) && eq(G, H)) || (eq(D, G) && eq(E, H))))
    {
        float distGE = distYCbCr(samplePixel(coord, texel, vec2(-2.0, 1.0)), D) +
                       distYCbCr(D, B) +
                       distYCbCr(samplePixel(coord, texel, vec2(-1.0, 2.0)), H) +
                       distYCbCr(H, F) +
                       4.0 * distYCbCr(G, E);
        float distDH = distYCbCr(samplePixel(coord, texel, vec2(-2.0, 0.0)), G) +
                       distYCbCr(G, samplePixel(coord, texel, vec2(0.0, 2.0))) +
                       distYCbCr(A, E) +
                       distYCbCr(E, I) +
                       4.0 * distYCbCr(D, H);
        bool dominant = DOMINANT_DIRECTION_THRESHOLD * distDH < distGE;
        blendResult.w = (distGE > distDH && neq(E, D) && neq(E, H)) ? (dominant ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
    }

    if (!((eq(B, C) && eq(E, F)) || (eq(B, E) && eq(C, F))))
    {
        float distEC = distYCbCr(D, B) +
                       distYCbCr(B, samplePixel(coord, texel, vec2(1.0, -2.0))) +
                       distYCbCr(H, F) +
                       distYCbCr(F, samplePixel(coord, texel, vec2(2.0, -1.0))) +
                       4.0 * distYCbCr(E, C);
        float distBF = distYCbCr(A, E) +
                       distYCbCr(E, I) +
                       distYCbCr(samplePixel(coord, texel, vec2(0.0, -2.0)), C) +
                       distYCbCr(C, samplePixel(coord, texel, vec2(2.0, 0.0))) +
                       4.0 * distYCbCr(B, F);
        bool dominant = DOMINANT_DIRECTION_THRESHOLD * distBF < distEC;
        blendResult.y = (distEC > distBF && neq(E, B) && neq(E, F)) ? (dominant ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
    }

    if (!((eq(A, B) && eq(D, E)) || (eq(A, D) && eq(B, E))))
    {
        float distDB = distYCbCr(samplePixel(coord, texel, vec2(-2.0, 0.0)), A) +
                       distYCbCr(A, samplePixel(coord, texel, vec2(0.0, -2.0))) +
                       distYCbCr(G, E) +
                       distYCbCr(E, C) +
                       4.0 * distYCbCr(D, B);
        float distAE = distYCbCr(samplePixel(coord, texel, vec2(-2.0, -1.0)), D) +
                       distYCbCr(D, H) +
                       distYCbCr(samplePixel(coord, texel, vec2(-1.0, -2.0)), B) +
                       distYCbCr(B, F) +
                       4.0 * distYCbCr(A, E);
        bool dominant = DOMINANT_DIRECTION_THRESHOLD * distDB < distAE;
        blendResult.x = (distDB < distAE && neq(E, D) && neq(E, B)) ? (dominant ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
    }

    vec3 res = E;

    if (blendResult.z != BLEND_NONE)
    {
        float distFG = distYCbCr(F, G);
        float distHC = distYCbCr(H, C);
        bool doLineBlend = blendResult.z == BLEND_DOMINANT ||
            !((blendResult.y != BLEND_NONE && !isPixEqual(E, G)) ||
              (blendResult.w != BLEND_NONE && !isPixEqual(E, C)) ||
              (isPixEqual(G, H) && isPixEqual(H, I) && isPixEqual(I, F) && isPixEqual(F, C) && !isPixEqual(E, I)));
        vec2 origin = vec2(0.0, 1.0 / sqrt(2.0));
        vec2 direction = vec2(1.0, -1.0);
        if (doLineBlend)
        {
            bool shallow = STEEP_DIRECTION_THRESHOLD * distFG <= distHC && neq(E, G) && neq(D, G);
            bool steep = STEEP_DIRECTION_THRESHOLD * distHC <= distFG && neq(E, C) && neq(B, C);
            origin = shallow ? vec2(0.0, 0.25) : vec2(0.0, 0.5);
            direction.x += shallow ? 1.0 : 0.0;
            direction.y -= steep ? 1.0 : 0.0;
        }
        vec3 blendPix = mix(H, F, step(distYCbCr(E, F), distYCbCr(E, H)));
        res = mix(res, blendPix, getLeftRatio(pos, origin, direction, scale));
    }

    if (blendResult.w != BLEND_NONE)
    {
        float distHA = distYCbCr(H, A);
        float distDI = distYCbCr(D, I);
        bool doLineBlend = blendResult.w == BLEND_DOMINANT ||
            !((blendResult.z != BLEND_NONE && !isPixEqual(E, A)) ||
              (blendResult.x != BLEND_NONE && !isPixEqual(E, I)) ||
              (isPixEqual(A, D) && isPixEqual(D, G) && isPixEqual(G, H) && isPixEqual(H, I) && !isPixEqual(E, G)));
        vec2 origin = vec2(-1.0 / sqrt(2.0), 0.0);
        vec2 direction = vec2(1.0, 1.0);
        if (doLineBlend)
        {
            bool shallow = STEEP_DIRECTION_THRESHOLD * distHA <= distDI && neq(E, A) && neq(B, A);
            bool steep = STEEP_DIRECTION_THRESHOLD * distDI <= distHA && neq(E, I) && neq(F, I);
            origin = shallow ? vec2(-0.25, 0.0) : vec2(-0.5, 0.0);
            direction.y += shallow ? 1.0 : 0.0;
            direction.x += steep ? 1.0 : 0.0;
        }
        vec3 blendPix = mix(H, D, step(distYCbCr(E, D), distYCbCr(E, H)));
        res = mix(res, blendPix, getLeftRatio(pos, origin, direction, scale));
    }

    if (blendResult.y != BLEND_NONE)
    {
        float distBI = distYCbCr(B, I);
        float distFA = distYCbCr(F, A);
        bool doLineBlend = blendResult.y == BLEND_DOMINANT ||
            !((blendResult.x != BLEND_NONE && !isPixEqual(E, I)) ||
              (blendResult.z != BLEND_NONE && !isPixEqual(E, A)) ||
              (isPixEqual(I, F) && isPixEqual(F, C) && isPixEqual(C, B) && isPixEqual(B, A) && !isPixEqual(E, C)));
        vec2 origin = vec2(1.0 / sqrt(2.0), 0.0);
        vec2 direction = vec2(-1.0, -1.0);
        if (doLineBlend)
        {
            bool shallow = STEEP_DIRECTION_THRESHOLD * distBI <= distFA && neq(E, I) && neq(H, I);
            bool steep = STEEP_DIRECTION_THRESHOLD * distFA <= distBI && neq(E, A) && neq(D, A);
            origin = shallow ? vec2(0.25, 0.0) : vec2(0.5, 0.0);
            direction.y -= shallow ? 1.0 : 0.0;
            direction.x -= steep ? 1.0 : 0.0;
        }
        vec3 blendPix = mix(F, B, step(distYCbCr(E, B), distYCbCr(E, F)));
        res = mix(res, blendPix, getLeftRatio(pos, origin, direction, scale));
    }

    if (blendResult.x != BLEND_NONE)
    {
        float distDC = distYCbCr(D, C);
        float distBG = distYCbCr(B, G);
        bool doLineBlend = blendResult.x == BLEND_DOMINANT ||
            !((blendResult.w != BLEND_NONE && !isPixEqual(E, C)) ||
              (blendResult.y != BLEND_NONE && !isPixEqual(E, G)) ||
              (isPixEqual(C, B) && isPixEqual(B, A) && isPixEqual(A, D) && isPixEqual(D, G) && !isPixEqual(E, A)));
        vec2 origin = vec2(0.0, -1.0 / sqrt(2.0));
        vec2 direction = vec2(-1.0, 1.0);
        if (doLineBlend)
        {
            bool shallow = STEEP_DIRECTION_THRESHOLD * distDC <= distBG && neq(E, C) && neq(F, C);
            bool steep = STEEP_DIRECTION_THRESHOLD * distBG <= distDC && neq(E, G) && neq(H, G);
            origin = shallow ? vec2(0.0, -0.25) : vec2(0.0, -0.5);
            direction.x -= shallow ? 1.0 : 0.0;
            direction.y += steep ? 1.0 : 0.0;
        }
        vec3 blendPix = mix(D, B, step(distYCbCr(E, B), distYCbCr(E, D)));
        res = mix(res, blendPix, getLeftRatio(pos, origin, direction, scale));
    }

    res = applyFinalAdjustments(res, E);

    float alpha = texture(inTexture, inUV).a * inColor.a;
    alpha *= clamp(sqrt(coolTransparency.x), coolTransparency.y, coolTransparency.z);
    outColor = vec4(res * inColor.rgb, alpha);
}
