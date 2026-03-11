
//#pragma parameter SOFTEN "Softening Intensity" 1.0 0.0 1.0 0.5
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
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_VARYING vec4 TEX0;
uniform mat4 MVPMatrix;
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
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;
//uniform COMPAT_PRECISION float SOFTEN; 
#define aaa (InputSize/TextureSize)
#define bbb (1.0/OutputSize.x)
#define ccc Texture
void main() {
    float a[5];
        a[0] = 0.1043;
        a[1] = 0.3785;
        a[2] = 1.0;
        a[3] = 0.3785;
        a[4] = 0.1043;
    vec2 b = TEX0.xy/aaa;
    vec4 c = vec4(0.0);
    float d = 0.0;
    for (int i = -2; i <= 2; ++i) {
        float e = float(i) * bbb;// *SOFTEN;  
        vec2 f = b + vec2(e, 0.0);  
        float g = a[i+2];
        c += COMPAT_TEXTURE(ccc, f *aaa) * g;
        d += g;
    }
    if (d > 0.0) {
        c /= d;
    }
    FragColor = c;
}
#endif
