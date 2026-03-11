
#pragma parameter GAMMA "Gamma Emulated" 2.2 1.0 4.0 0.1 
#pragma parameter HT "Scanline Intensity" 0.7 0.0 0.95 0.05 
#define HT_INTENSITY HT
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
COMPAT_VARYING vec3 vParameter;
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
    vec4 b = vec4(InputSize,1.0/InputSize);
   vParameter.z = OutputSize.y*b.w*0.5;
   vParameter.x = (vParameter.z-1.0)*0.5;
   vParameter.y = b.w *floor((OutputSize.y*0.5-b.y)*0.5);
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
COMPAT_VARYING vec3 vParameter;
uniform COMPAT_PRECISION float GAMMA; 
uniform COMPAT_PRECISION float HT; 
#define aaa (InputSize/TextureSize)
#define bbb Texture
void main()
{
    vec2 a = TEX0.xy/aaa;
    vec4 b = vec4(InputSize,1.0/InputSize);
    float c = OutputSize.y*a.y;
    vec2 d;
    d.x = a.x*vParameter.z;
    d.y = c*b.w*0.5 +b.w*0.25;
    float e = mod(c,2.0);
    d -= vParameter.xy;
    d *= aaa;
    vec3 f = pow(COMPAT_TEXTURE(bbb, fract(d)).rgb, vec3(GAMMA));
    vec3 g = (f *HT_INTENSITY +(1.0 -HT_INTENSITY)) *2.0;
    vec3 h = clamp((g -e +0.5), 0.0, 1.0);
    h *= f / (  f*HT_INTENSITY +1.0 -HT_INTENSITY);
   FragColor.rgb = pow(h,vec3(0.454545));
}
#endif
