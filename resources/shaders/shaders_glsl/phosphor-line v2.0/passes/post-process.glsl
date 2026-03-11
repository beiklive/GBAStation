
#pragma parameter MASK_INTENSITY "Shadow Mask Intensity" 0.7 0.0 0.9 0.1
#pragma parameter MASK_TYPE "Shadow Mask Type" 1.0 0.0 1.0 1.0
#pragma parameter MASK_SIZE "Shadow Mask Size" 1.0 0.0 10.0 0.1
#define kkk 2.2
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
#define COMPAT_HIGHP highp
#else
#define COMPAT_PRECISION
#define COMPAT_HIGHP
#endif
COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_VARYING vec4 TEX0;
COMPAT_VARYING vec2 vParameter;
uniform mat4 MVPMatrix;
uniform COMPAT_HIGHP vec2 InputSize;
uniform COMPAT_PRECISION vec2 OrigInputSize;
uniform COMPAT_PRECISION float PIC_SCALE_Y; 
uniform COMPAT_PRECISION float INT_TOL;
uniform COMPAT_PRECISION float MASK_SIZE;
void main()
{
    vec4 a = vec4(InputSize,1.0/InputSize);
    vec4 b = vec4(OrigInputSize,1.0/OrigInputSize);
    gl_Position = VertexCoord.x * MVPMatrix[0] + VertexCoord.y * MVPMatrix[1] + VertexCoord.z * MVPMatrix[2] + VertexCoord.w * MVPMatrix[3];
    TEX0.xy = TexCoord.xy;
   float c = a.y * b.w;
   float d = c * PIC_SCALE_Y;
   float e = step(d, 4.0);
   float f = max(floor(d * (1.0 + INT_TOL * e)), 1.0);
   float g = f / c;
   float h = g - 1.0;
   h = (h > 0.0 && h < INT_TOL * e) ? h : 0.0;
   g -= h;
   vParameter.y = g / PIC_SCALE_Y;
   float as = max(f,d);
   vParameter.x = as<3.0? ceil(as*MASK_SIZE) : floor(as *0.6 *MASK_SIZE +0.5);
   if(vParameter.x<2.0)vParameter.x=2.0;
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
#define COMPAT_HIGHP highp
#else
#define COMPAT_PRECISION
#define COMPAT_HIGHP
#endif
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_HIGHP vec2 TextureSize;
uniform COMPAT_HIGHP vec2 InputSize;
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;
COMPAT_VARYING vec2 vParameter;
uniform COMPAT_PRECISION float INT_SCALE;
uniform COMPAT_PRECISION float CURVE_X; 
uniform COMPAT_PRECISION float CURVE_Y; 
uniform COMPAT_PRECISION float MASK_TYPE;
uniform COMPAT_PRECISION float MASK_INTENSITY;
#define Source Texture
vec4 j(float x)
{
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w;
    w.x =  3.0*x2 -x3  - 3.0*x + 1.0;
    w.y =  3.0*x3 - 6.0*x2       + 4.0;
    w.z =  3.0*x2 -3.0*x3  + 3.0*x + 1.0;
    w.w =  x3;
    return w / 6.0;
}
vec3 k(sampler2D Source, vec2 l, vec2 m)
{
    l.y -= 0.5;
    float fy = fract(l.y);
    l.y -= fy;
    vec4 n = j(fy);
    vec2 c = vec2(l.y - 0.5, l.y + 1.5);
    vec2 s = vec2(n.x + n.y, n.z + n.w);
    vec2 o = c + vec2(n.y, n.w) / s;
    vec2 p = InputSize/TextureSize;
    vec3 q = COMPAT_TEXTURE(Source, fract(vec2(l.x, o.x) * m *p)).rgb;
    vec3 r = COMPAT_TEXTURE(Source, fract(vec2(l.x, o.y) * m *p)).rgb;
    float sy = s.x / (s.x + s.y);
    return mix(r, q, sy);
}
vec2 s(vec2 uv)
{
    if (CURVE_X < 0.005 && CURVE_Y < 0.005) return uv;
    float cx = clamp(CURVE_X * 2.0, 0.01, 1.0);
    float cy = clamp(CURVE_Y, 0.01, 1.0);
    float sin_cx = sin(cx);
    float sin_cy = sin(cy);
    float o = 0.50001;
    float t = (uv.x - o) * 2.0;
    float u = (uv.y - o) * 2.0;
    float argx = t * cx;
    float argy = u * cy;
    float sinc_x = (t * t * sin_cx) / sin(argx);
    float sinc_y = (u * u * sin_cy) / sin(argy);
    float arg2x = sinc_x * cx;
    float v = (1.0 / cx) * (1.0 - cos(arg2x));
    float w = sinc_y / (1.0 - 0.3 * v);
    float x = w * 0.5 + o;
    return vec2(uv.x, x);
}
void main()
{
    #define p (InputSize/TextureSize)
    vec2 y = TEX0.xy/p;
    vec4 a = vec4(InputSize,1.0/InputSize);
    vec2 z = y;
    if(INT_SCALE <= 0.25){
        z.y = (z.y-0.5) *vParameter.y +0.5;
        z = s(z);
    }
    vec2 l = z *a.xy;
    FragColor.rgb = k(Source, l, a.zw);
        #define MASK_SIZE vParameter.x
        vec2 bb = floor(y * OutputSize.xy) +0.5;
        float cc = floor(MASK_SIZE*0.5 +0.1);
        bb.x += floor(bb.y/cc)*cc *MASK_TYPE;
        vec3 dd;
        #define eee (floor(MASK_SIZE/2.0 +0.5))
        dd = mod(vec3(bb.x, bb.x+eee ,bb.x), MASK_SIZE);
        #define fff (dd -MASK_SIZE*0.5)
	    dd = abs(fff*2.0 -0.5);
        vec3 gg = FragColor.rgb*FragColor.rgb;
        vec3 hh = (gg *MASK_INTENSITY +(1.0 -MASK_INTENSITY)) *MASK_SIZE;
	    vec3 ii = clamp((hh -dd +0.5), 0.0, 1.0);
	    ii *= gg / (  gg*MASK_INTENSITY +1.0 -MASK_INTENSITY);
	    FragColor.rgb = pow(ii,vec3(1.0/kkk));
}
#endif