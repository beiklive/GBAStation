#version 460

layout(binding = 0) uniform sampler2D tex;

layout(std140, binding = 0) uniform frag {
    mat3 scissorMat;
    mat3 paintMat;
    vec4 innerCol;
    vec4 outerCol;
    vec2 scissorExt;
    vec2 scissorScale;
    vec2 extent;
    float radius;
    float feather;
    float strokeMult;
    float strokeThr;
    int texType;
    int type;
};

layout(location = 0) in vec2 ftcoord;
layout(location = 1) in vec2 fpos;
layout(location = 0) out vec4 outColor;

float sdroundrect(vec2 pt, vec2 ext, float rad) {
    vec2 ext2 = ext - vec2(rad,rad);
    vec2 d = abs(pt) - ext2;
    return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;
}

float roundedRectPerimeterPos(vec2 pt, vec2 ext, float rad) {
    const float PI = 3.14159265359;
    float r = max(rad, 0.001);
    vec2 b = max(ext - vec2(r,r), vec2(0.0,0.0));
    float q = PI * 0.5 * r;
    float h = b.x * 2.0;
    float vlen = b.y * 2.0;
    float pos = 0.0;

    if (pt.y <= -b.y && pt.x >= -b.x && pt.x <= b.x) {
        pos = pt.x + b.x;
    } else if (pt.x > b.x && pt.y < -b.y) {
        vec2 v = pt - vec2(b.x, -b.y); v /= max(length(v), 0.001);
        pos = h + clamp(atan(v.y, v.x) + PI * 0.5, 0.0, PI * 0.5) * r;
    } else if (pt.x >= b.x && pt.y >= -b.y && pt.y <= b.y) {
        pos = h + q + pt.y + b.y;
    } else if (pt.x > b.x && pt.y > b.y) {
        vec2 v = pt - vec2(b.x, b.y); v /= max(length(v), 0.001);
        pos = h + q + vlen + clamp(atan(v.y, v.x), 0.0, PI * 0.5) * r;
    } else if (pt.y >= b.y && pt.x >= -b.x && pt.x <= b.x) {
        pos = h + q * 2.0 + vlen + b.x - pt.x;
    } else if (pt.x < -b.x && pt.y > b.y) {
        vec2 v = pt - vec2(-b.x, b.y); v /= max(length(v), 0.001);
        pos = h * 2.0 + q * 2.0 + vlen + clamp(atan(v.y, v.x) - PI * 0.5, 0.0, PI * 0.5) * r;
    } else if (pt.x <= -b.x && pt.y >= -b.y && pt.y <= b.y) {
        pos = h * 2.0 + q * 3.0 + vlen + b.y - pt.y;
    } else {
        vec2 v = pt - vec2(-b.x, -b.y); v /= max(length(v), 0.001);
        float a = atan(v.y, v.x); if (a < 0.0) a += PI * 2.0;
        pos = h * 2.0 + vlen * 2.0 + q * 3.0 + clamp(a - PI, 0.0, PI * 0.5) * r;
    }

    return pos / max(h * 2.0 + vlen * 2.0 + q * 4.0, 1.0);
}

// Scissoring
float scissorMask(vec2 p) {
    vec2 sc = (abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);
    sc = vec2(0.5,0.5) - sc * scissorScale;
    return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);
}

// Stroke - from [0..1] to clipped pyramid, where the slope is 1px.
float strokeMask() {
    return min(1.0, (1.0-abs(ftcoord.x*2.0-1.0))*strokeMult) * min(1.0, ftcoord.y);
}

void main(void) {
    vec4 result;
    float scissor = scissorMask(fpos);
    float strokeAlpha = strokeMask();

    if (strokeAlpha < strokeThr) discard;

    if (type == 0) {			// Gradient
        // Calculate gradient color using box gradient
        vec2 pt = (paintMat * vec3(fpos,1.0)).xy;
        float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);
        vec4 color = mix(innerCol,outerCol,d);
        // Combine alpha
        color *= strokeAlpha * scissor;
        result = color;
    } else if (type == 1) {		// Image
        // Calculate color fron texture
        vec2 pt = (paintMat * vec3(fpos,1.0)).xy / extent;
        vec4 color = texture(tex, pt);

        if (texType == 1) color = vec4(color.xyz*color.w,color.w);
        if (texType == 2) color = vec4(color.x);
        // Apply color tint and alpha.
        color *= innerCol;
        // Combine alpha
        color *= strokeAlpha * scissor;
        result = color;
    } else if (type == 2) {		// Stencil fill
        result = vec4(1,1,1,1);
    } else if (type == 3) {		// Textured tris

        vec4 color = texture(tex, ftcoord);

        if (texType == 1) color = vec4(color.xyz*color.w,color.w);
        if (texType == 2) color = vec4(color.x);
        color *= scissor;
        result = color * innerCol;
    } else if (type == 4) {		// Gradient LUT
        vec2 pt = (paintMat * vec3(fpos,1.0)).xy;
        float d = sdroundrect(pt, extent, radius);
        float borderMask = 1.0 - smoothstep(max(feather - 1.0, 0.0), feather, abs(d));
        float u = fract(roundedRectPerimeterPos(pt, extent, radius) + outerCol.r);
        vec4 color = texture(tex, vec2(u, 0.5));

        if (texType == 1) color = vec4(color.xyz*color.w,color.w);
        if (texType == 2) color = vec4(color.x);
        color *= innerCol;
        color *= borderMask * strokeAlpha * scissor;
        result = color;
    }

    outColor = result;
};
