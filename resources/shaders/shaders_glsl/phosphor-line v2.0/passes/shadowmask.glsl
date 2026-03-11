
#pragma parameter GAMMA "Gamma Emulated" 2.2 1.0 4.0 0.1 
#pragma parameter MASK_INTENSITY "Shadow Mask Intensity" 0.7 0.0 0.9 0.1
#pragma parameter MASK_TYPE "Shadow Mask Type" 1.0 0.0 1.0 1.0
#pragma parameter MASK_SIZE "Shadow Mask Size" 1.0 0.0 10.0 0.1
#define nnn 2.2
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
COMPAT_VARYING float vParameter;
uniform mat4 MVPMatrix;
uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform COMPAT_PRECISION vec2 OrigInputSize;
uniform COMPAT_PRECISION float MASK_SIZE;
#define OriginalSize vec4(OrigInputSize,1.0/OrigInputSize)
void main()
{
    gl_Position = VertexCoord.x * MVPMatrix[0] + VertexCoord.y * MVPMatrix[1] + VertexCoord.z * MVPMatrix[2] + VertexCoord.w * MVPMatrix[3];
    TEX0.xy = TexCoord.xy;
   float a = OutputSize.y*OriginalSize.w;
   vParameter = max(ceil(a *MASK_SIZE), 2.0);
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
COMPAT_VARYING float vParameter;
uniform COMPAT_PRECISION float GAMMA; 
uniform COMPAT_PRECISION float MASK_INTENSITY;
uniform COMPAT_PRECISION float MASK_TYPE;
#define Source Texture
void main()
{
    vec2 b = InputSize/TextureSize;
    vec2 c = TEX0.xy/b;
    vec4 d = vec4(InputSize,1.0/InputSize);
    vec3 e = COMPAT_TEXTURE(Source, TEX0.xy).rgb;
    #define MASK_SIZE vParameter
    vec2 g = c * OutputSize.xy;
    float h = floor(MASK_SIZE*0.5 +0.1);
    g.x += floor(g.y/h)*h *MASK_TYPE;
    vec3 j;
    #define kkk (floor(MASK_SIZE/2.0 +0.5))
    j = mod(vec3(g.x, g.x+kkk ,g.x), MASK_SIZE);
    #define lll (j -MASK_SIZE*0.5)
    j = abs(lll*2.0 -0.5);
    vec3 k = pow(e,vec3(GAMMA));
    vec3 l = (k *MASK_INTENSITY +(1.0 -MASK_INTENSITY)) *MASK_SIZE;
    vec3 m = clamp((l -j +0.5), 0.0, 1.0);
    m *= k / (  k*MASK_INTENSITY +1.0 -MASK_INTENSITY);
    FragColor.rgb = pow(m,vec3(1.0/nnn));
}
#endif