
#pragma parameter MASK_INTENSITY "Shadow Mask Intensity" 0.7 0.0 0.9 0.1
#pragma parameter MASK_TYPE "Shadow Mask Type" 1.0 0.0 1.0 1.0
#pragma parameter MASK_SIZE "Shadow Mask Size" 1.0 0.0 10.0 0.1
#pragma parameter DIFFUSION "CRT Diffusion" 0.33 0.0 1.0 0.01
#pragma parameter DIFF_OVEREXP "Allow Diffusion Overexposed" 0.5 0.0 1.0 0.05
#define aaa 2.2
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
COMPAT_VARYING vec4 vParameter;
uniform mat4 MVPMatrix;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 OrigInputSize;
uniform COMPAT_PRECISION float PIC_SCALE_X; 
uniform COMPAT_PRECISION float PIC_SCALE_Y; 
uniform COMPAT_PRECISION float INT_TOL;
uniform COMPAT_PRECISION float INT_SCALE;
uniform COMPAT_PRECISION float MASK_SIZE;
#define bbb OutputSize
#define ccc vec4(OrigInputSize,1.0/OrigInputSize)
void main()
{
    gl_Position = VertexCoord.x * MVPMatrix[0] + VertexCoord.y * MVPMatrix[1] + VertexCoord.z * MVPMatrix[2] + VertexCoord.w * MVPMatrix[3];
    TEX0.xy = TexCoord.xy;
   float a = bbb.y * ccc.w;
   float b = a * PIC_SCALE_Y;
   float c = step(b, 4.0);
   float d = max(floor(b * (1.0 + INT_TOL * c)), 1.0);
   float e = d / a;
   float f = e;
   float g = f - 1.0;
   g = (g > 0.0 && g < INT_TOL * c) ? g : 0.0;
   f -= g;
   vParameter.y = f / PIC_SCALE_Y;
   float as = max(d,b);
   vParameter.x = as<3.0? ceil(as*MASK_SIZE) : floor(as *0.6 *MASK_SIZE +0.5);
   if(vParameter.x<2.0)vParameter.x=2.0;
   vParameter.w = INT_SCALE>0.25? e : PIC_SCALE_Y*(1.0+g);
   vParameter.z = INT_SCALE>0.75? vParameter.w : PIC_SCALE_X;
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
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform COMPAT_PRECISION vec2 OrigInputSize;
uniform COMPAT_HIGHP vec2 PassPrev3TextureSize;
uniform COMPAT_HIGHP vec2 PassPrev3InputSize;
uniform sampler2D PassPrev3Texture;
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;
COMPAT_VARYING vec4 vParameter;
uniform COMPAT_PRECISION float LINE_THICKNESS; 
uniform COMPAT_PRECISION float INT_SCALE;
uniform COMPAT_PRECISION float CURVE_X; 
uniform COMPAT_PRECISION float CURVE_Y; 
uniform COMPAT_PRECISION float MASK_TYPE;
uniform COMPAT_PRECISION float MASK_INTENSITY;
uniform COMPAT_PRECISION float DIFFUSION; 
uniform COMPAT_PRECISION float DIFF_OVEREXP; 
#define Source Texture
#define PhosphorLine PassPrev3Texture
vec4 h(float x)
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
vec3 j(sampler2D Source, vec2 k, vec2 l)
{
    k.y -= 0.5;
    float fy = fract(k.y);
    k.y -= fy;
    vec4 m = h(fy);
    vec2 c = vec2(k.y - 0.5, k.y + 1.5);
    vec2 s = vec2(m.x + m.y, m.z + m.w);
    vec2 n = c + vec2(m.y, m.w) / s;
    vec2 PassPrev3uvratio = PassPrev3InputSize/PassPrev3TextureSize;
    vec3 o = COMPAT_TEXTURE(Source, fract(vec2(k.x, n.x) * l *PassPrev3uvratio)).rgb;
    vec3 p = COMPAT_TEXTURE(Source, fract(vec2(k.x, n.y) * l *PassPrev3uvratio)).rgb;
    float sy = s.x / (s.x + s.y);
    return mix(p, o, sy);
}
vec2 q(vec2 uv) {
    if (CURVE_X < 0.005 && CURVE_Y < 0.005) return uv;
    float rx = 1.0 / clamp(CURVE_X * 2.0, 0.01, 1.0);
    float ry = 1.0 / clamp(CURVE_Y, 0.01, 1.0);
    float s1x = sin(1.0 / rx);
    float s1y = sin(1.0 / ry);
    vec2 w = (uv - 0.00001) * 2.0;
    float ux = w.x;
    float uy = w.y;
    float theta_x = ux / rx;
    float theta_y = uy / ry;
    ux = ux * ux * s1x / sin(theta_x);
    uy = uy * uy * s1y / sin(theta_y);  
    float cos_tx = cos(theta_x);
    float cos_ty = cos(theta_y);
    float r = rx * (1.0 - cos_tx);
    float depth_y = ry * (1.0 - cos_ty);
    float div_x = 1.0 - depth_y * 0.3;
    ux /= div_x;
    float out_x = ux * 0.5 + 0.00001;
    return vec2(out_x, uv.y);
}
vec2 bb(vec2 uv)
{
    if (CURVE_X < 0.005 && CURVE_Y < 0.005) return uv;
    float cx = clamp(CURVE_X * 2.0, 0.01, 1.0);
    float cy = clamp(CURVE_Y, 0.01, 1.0);
    float sin_cx = sin(cx);
    float sin_cy = sin(cy);
    float n = 0.00001;
    float uv2x = (uv.x - n) * 2.0;
    float uv2y = (uv.y - n) * 2.0;
    float argx = uv2x * cx;
    float argy = uv2y * cy;
    float sinc_x = (uv2x * uv2x * sin_cx) / sin(argx);
    float sinc_y = (uv2y * uv2y * sin_cy) / sin(argy);
    float arg2x = sinc_x * cx;
    float r = (1.0 / cx) * (1.0 - cos(arg2x));
    float s = sinc_y / (1.0 - 0.3 * r);
    float t = s * 0.5 + n;
    return vec2(uv.x, t);
}
vec2 u(vec2 uv)
{
    if(CURVE_X<0.005 && CURVE_Y<0.005) return uv;
    vec2 k = clamp(vec2(CURVE_X * 2.0, CURVE_Y), 0.01, 1.0);
    vec2 s1 = sin(k);
    vec2 w = (uv - 0.00001) * 2.0;
    vec2 arg = w * k;
    vec2 s = sin(arg);
    w = s1 * (w * w) / s;
    arg = w * k;
    vec2 co = cos(arg);
    vec2 v = (1.0 / k) * (1.0 - co);
    vec2 denom = 1.0 - v.yx * 0.3;
    w /= denom;
    w = w * 0.5 + 0.00001;
    return w;
}
void main()
{ 
    vec2 x = InputSize/TextureSize;
    vec2 y = TEX0.xy/x;
    vec4 bbb = vec4(PassPrev3InputSize,1.0/PassPrev3InputSize);
    vec2 z = y-0.5;
    vec2 aa = z;
    if(INT_SCALE <= 0.25){
        z.y *= vParameter.y;
        z = bb(z);
        aa = u(aa);
    }
    else aa = q(aa);
    aa /= vParameter.zw;
    #define ccc ((z+0.5) *bbb.xy)
    FragColor.rgb = j(PhosphorLine, ccc, bbb.zw);
    vec3 cc = FragColor.rgb*FragColor.rgb;
    vec3 dd = COMPAT_TEXTURE(Source, fract((aa*0.957+0.5)*x)).rgb;
    dd *= dd;
    float ee = DIFFUSION*DIFFUSION *LINE_THICKNESS;
    cc += dd*ee;
    cc *= 1.0/(1.0 +ee*(1.0-DIFF_OVEREXP));
        #define MASK_SIZE vParameter.x
        vec2 ff = floor(y * OutputSize.xy) +0.5;
        float gg = floor(MASK_SIZE*0.5 +0.1);
        ff.x += floor(ff.y/gg)*gg *MASK_TYPE;
        vec3 hh;
        #define iii (floor(MASK_SIZE/2.0 +0.5))
        hh = mod(vec3(ff.x, ff.x+iii ,ff.x), MASK_SIZE);
        #define jjj (hh -MASK_SIZE*0.5)
	    hh = abs(jjj*2.0 -0.5);
        vec3 kk = (cc *MASK_INTENSITY +(1.0 -MASK_INTENSITY)) *MASK_SIZE;
	    vec3 ll = clamp((kk -hh +0.5), 0.0, 1.0);
	    ll *= cc / (  cc*MASK_INTENSITY +1.0 -MASK_INTENSITY);
    FragColor.rgb = pow(ll,vec3(1.0/aaa));
}
#endif
