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

float toVideo(float l)
{
    return l < 0.00313 ? l * 12.9232102 : 1.055 * pow(l, 0.4166) - 0.055;
}

vec3 toVideo3(vec3 c)
{
    return vec3(toVideo(c.r), toVideo(c.g), toVideo(c.b));
}

void main()
{
    vec4 sampled = texture(inTexture, inUV);
    float screenGamma = max(ndsParams.param0.x, 0.01);
    float dotGamma = max(ndsParams.param0.y, 0.01);
    float dotScaleX = max(ndsParams.param0.z, 0.1);
    float dotScaleY = max(ndsParams.param0.w, 0.1);
    float dotOpacity = clamp(ndsParams.param1.x, 0.0, 1.0);
    float halftoneStrength = clamp(ndsParams.param1.y, 0.0, 1.0);
    vec3 texColor = pow(clamp(sampled.rgb, 0.0, 1.0), vec3(dotGamma));

    vec2 texSize = vec2(textureSize(inTexture, 0));
    vec2 pixelNo = inUV * texSize / max(ndsParams.runtime.x, 1.0);
    vec2 dotSize = vec2(max(dotScaleX, 0.1), max(dotScaleY, 0.1));
    vec2 local = fract(pixelNo) - vec2(0.5);
    local.x /= dotSize.x;
    local.y /= dotSize.y;

    vec3 linearColor = sqrt(max(texColor, vec3(0.0)));
    vec3 dotRadius = linearColor * halftoneStrength + (1.0 - halftoneStrength);
    float distanceFromCenter = length(local) * 2.0;
    vec3 dotMask = smoothstep(vec3(1.05), vec3(0.35), vec3(distanceFromCenter) / max(dotRadius, vec3(0.001)));
    vec3 dotColor = dotMask * pow(linearColor / max(linearColor * halftoneStrength + 1.0 - halftoneStrength, vec3(0.001)), vec3(2.0));

    vec3 outRgb = mix(texColor, dotColor, dotOpacity);
    outRgb = pow(max(outRgb, vec3(0.0)), vec3(1.0 / screenGamma)) * inColor.rgb;

    float alpha = sampled.a * inColor.a;
    alpha *= clamp(sqrt(coolTransparency.x), coolTransparency.y, coolTransparency.z);
    outColor = vec4(outRgb, alpha);
}
