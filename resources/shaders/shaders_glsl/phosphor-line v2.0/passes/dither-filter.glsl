
#pragma parameter HEADER "#PhosphorLine v2.0 by Roc-k" 2.0 2.0 2.0 0.0
#pragma parameter SAT "Saturation" 1.0 0.0 2.0 0.05
#pragma parameter SOFTEN "S-Video Blur" 0.85 0.0 1.0 0.05
#if defined(VERTEX)
#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying 
#define COMPAT_ATTRIBUTE attribute 
#define COMPAT_TEXTURE texture2D
#endif
#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
precision mediump int;
#endif
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif
COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec4 COLOR;
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_VARYING vec4 COL0;
COMPAT_VARYING vec4 TEX0;
uniform mat4 MVPMatrix;
uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
void main()
{
    gl_Position = VertexCoord.x * MVPMatrix[0] + VertexCoord.y * MVPMatrix[1] + VertexCoord.z * MVPMatrix[2] + VertexCoord.w * MVPMatrix[3];
    TEX0.xy = TexCoord.xy;
}
#elif defined(FRAGMENT)
#if __VERSION__ >= 130
#define COMPAT_VARYING in
#define COMPAT_TEXTURE texture
out vec4 FragColor;
#else
#define COMPAT_VARYING varying
#define FragColor gl_FragColor
#define COMPAT_TEXTURE texture2D
#endif
#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
precision mediump int;
#endif
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif
uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;
uniform COMPAT_PRECISION float SAT; 
uniform COMPAT_PRECISION float SOFTEN; 
#define a Texture
#define bb (InputSize/TextureSize)
vec3 c(vec2 o) {    
    float d = 1.0/TextureSize.x;
    vec2 e = o*bb;
    vec3 f = COMPAT_TEXTURE(a, e + vec2(-d, 0.0)).xyz;
    vec3 g = COMPAT_TEXTURE(a, e + vec2(d, 0.0)).xyz;
    vec3 h = COMPAT_TEXTURE(a, e).xyz;
    return mix(h,h * 0.5 + f * 0.25 + g * 0.25,SOFTEN);
}
vec3 j(vec3 p) {
    float k = 0.299 * p.r + 0.587 * p.g + 0.114 * p.b;
    float l = -0.1687 * p.r - 0.3313 * p.g + 0.5000 * p.b + 0.5000;
    float m = 0.5000 * p.r - 0.4187 * p.g - 0.0813 * p.b + 0.5000;
    return vec3(k, l, m);
}
void main() {
    vec2 n = TEX0.xy/bb;
    FragColor.xyz = c(n);
    FragColor.xyz = j(FragColor.xyz);
    FragColor.yz = (FragColor.yz-0.5)*SAT+0.5;
}
#endif