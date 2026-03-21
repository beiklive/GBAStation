
#pragma parameter MASK_INTENSITY "Shadow Mask Intensity" 0.7 0.0 0.9 0.1
#pragma parameter MASK_TYPE "Shadow Mask Type" 1.0 0.0 1.0 1.0
#pragma parameter MASK_SIZE "Shadow Mask Size" 1.0 0.0 10.0 0.1
#pragma parameter DIFFUSION "CRT Diffusion" 0.33 0.0 1.0 0.01
#pragma parameter DIFF_OVEREXP "Allow Diffusion Overexposed" 0.5 0.0 1.0 0.05
#pragma parameter REFLECTION "Reflection Intensity" 0.5 0.0 1.0 0.05
#pragma parameter E_OFFSET "Reflection Offset" 0.6 0.0 2.0 0.05
#pragma parameter B_CORNER "Bezel Corner Size" 0.5 0.0 1.0 0.1
#pragma parameter FILLET_D "Fillet Direction" -0.1 -0.5 0.5 0.05
#define ppppp 2.2
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
#define aaa OutputSize
#define bbb vec4(OrigInputSize,1.0/OrigInputSize)
void main()
{
    gl_Position = VertexCoord.x * MVPMatrix[0] + VertexCoord.y * MVPMatrix[1] + VertexCoord.z * MVPMatrix[2] + VertexCoord.w * MVPMatrix[3];
    TEX0.xy = TexCoord.xy;
   float a = aaa.y * bbb.w;
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
uniform COMPAT_PRECISION float PIC_SCALE_X;
uniform COMPAT_PRECISION float PIC_SCALE_Y;
uniform COMPAT_PRECISION float CURVE_X; 
uniform COMPAT_PRECISION float CURVE_Y; 
uniform COMPAT_PRECISION float MASK_TYPE;
uniform COMPAT_PRECISION float MASK_INTENSITY;
uniform COMPAT_PRECISION float DIFFUSION; 
uniform COMPAT_PRECISION float DIFF_OVEREXP; 
uniform COMPAT_PRECISION float CPOY;
uniform COMPAT_PRECISION float CORNER; 
uniform COMPAT_PRECISION float REFLECTION; 
uniform COMPAT_PRECISION float E_OFFSET;
uniform COMPAT_PRECISION float B_CORNER; 
uniform COMPAT_PRECISION float FILLET_D; 
#define ddd Texture
#define eee PassPrev3Texture
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
vec3 j(sampler2D ddd, vec2 k, vec2 l)
{
    k.y -= 0.5;
    float fy = fract(k.y);
    k.y -= fy;
    vec4 m = h(fy);
    vec2 c = vec2(k.y - 0.5, k.y + 1.5);
    vec2 s = vec2(m.x + m.y, m.z + m.w);
    vec2 n = c + vec2(m.y, m.w) / s;
    vec2 o = InputSize/TextureSize;
    vec2 PassPrev3uvratio = PassPrev3InputSize/PassPrev3TextureSize;
    vec3 p = COMPAT_TEXTURE(ddd, fract(vec2(k.x, n.x) * l *PassPrev3uvratio)).rgb;
    vec3 q = COMPAT_TEXTURE(ddd, fract(vec2(k.x, n.y) * l *PassPrev3uvratio)).rgb;
    float sy = s.x / (s.x + s.y);
    return mix(q, p, sy);
}
vec2 r(vec2 uv) {
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
    float sd = rx * (1.0 - cos_tx);
    float depth_y = ry * (1.0 - cos_ty);
    float div_x = 1.0 - depth_y * 0.3;
    ux /= div_x;
    float out_x = ux * 0.5 + 0.00001;
    return vec2(out_x, uv.y);
}
vec2 curve_y(vec2 uv)
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
    float sd = (1.0 / cx) * (1.0 - cos(arg2x));
    float t = sinc_y / (1.0 - 0.3 * sd);
    float final_y = t * 0.5 + n;
    return vec2(uv.x, final_y);
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
float x(float x, float t,float s) {
    return (x-t)*s+t;
}
vec2 y(vec2 A, vec2 B, vec2 O, float R) {
    vec2 D = B - A;
    vec2 C = A - O;
    float a = dot(D, D);
    float b = 2.0 * dot(C, D);
    float c = dot(C, C) - R * R;
    float delta = b * b - 4.0 * a * c;
    if (delta < 0.0) {
        return vec2(0.0);  
    }
    float sqrtDelta = sqrt(delta);
    float t1 = (-b + sqrtDelta) / (2.0 * a);
    vec2 P1 = A + t1 * D;
    vec2 V1 = P1 - O;
    if (t1 >= 0.0 && t1 <= 1.0 && P1.x >= O.x && P1.y >= O.y && 
        abs(length(V1) - R) < 1e-4) {
        return P1;
    }
    float t2 = (-b - sqrtDelta) / (2.0 * a);
    vec2 P2 = A + t2 * D;
    vec2 V2 = P2 - O;
    if (t2 >= 0.0 && t2 <= 1.0 && P2.x >= O.x && P2.y >= O.y && 
        abs(length(V2) - R) < 1e-4) {
        return P2;
    }
    return vec2(0.0);  
}
vec2 z(vec2 v, float t) {
    float theta = t * (3.14159265359 / 2.0);  
    float c = cos(theta);
    float s = sin(theta);
    return vec2(
        v.x * c - v.y * s,
        v.x * s + v.y * c
    );
}
void main()
{
    vec2 o = InputSize/TextureSize;
    vec2 bb = TEX0.xy/o;
    vec4 aaa = vec4(PassPrev3InputSize,1.0/PassPrev3InputSize);
    vec4 cc = vec4(OutputSize,1.0/OutputSize);
    vec2 dd = bb-0.5;
    vec2 ee = dd;
    if(INT_SCALE <= 0.25){
        dd.y *= vParameter.y;
        dd = curve_y(dd);
        ee = u(ee);
    }
    else ee = r(ee);
    ee /= vParameter.zw;
    #define fff ((dd+0.5) *aaa.xy)
    FragColor.rgb = j(eee, fff, aaa.zw);
    vec3 ff = FragColor.rgb*FragColor.rgb;
    vec3 gg = COMPAT_TEXTURE(ddd, fract((ee*0.957+0.5) *o)).rgb;
    gg *= gg;
    float hh = DIFFUSION*DIFFUSION *LINE_THICKNESS;
    ff += gg*hh;
    ff *= 1.0/(1.0 +hh*(1.0-DIFF_OVEREXP));
        #define MASK_SIZE vParameter.x
        vec2 ii = floor(bb * OutputSize.xy) +0.5;
        float jj = floor(MASK_SIZE*0.5 +0.1);
        ii.x += floor(ii.y/jj)*jj *MASK_TYPE;
        vec3 kk;
        #define lll (floor(MASK_SIZE/2.0 +0.5))
        kk = mod(vec3(ii.x, ii.x+lll ,ii.x), MASK_SIZE);
        #define mmm (kk -MASK_SIZE*0.5)
	    kk = abs(mmm*2.0 -0.5);
        vec3 ll = (ff *MASK_INTENSITY +(1.0 -MASK_INTENSITY)) *MASK_SIZE;
	    vec3 mm = clamp((ll -kk +0.5), 0.0, 1.0);
	    mm *= ff / (  ff*MASK_INTENSITY +1.0 -MASK_INTENSITY);
    float nn = cc.x*cc.w;
    float oo = nn*0.5;
    vec2 pp = ee;
    vec2 qq = sign(pp);
    pp.x *=nn;
    pp = abs(pp);
    vec2 rr = pp;
    vec2 ss = vec2(oo,0.5);
    ss.y *= 1.0+CPOY*0.01;
    vec2 tt = ss+E_OFFSET*0.01;
    // 画面边界以外（ss 之外）变为黑色，避免触发边界和画面边界之间出现游戏画面残影
    mm *= step(rr.x, ss.x) * step(rr.y, ss.y);
    if(pp.x>tt.x||pp.y>tt.y){
        vec2 uu = (pp-tt)*vec2(PIC_SCALE_X,PIC_SCALE_Y)*OutputSize.xy;
        if(pp.x>tt.x)pp.x = x(pp.x,tt.x,-1.5);
        if(pp.y>tt.y)pp.y = x(pp.y,tt.y,-1.5);
        pp.x /= nn;
        pp *= 0.95;
        pp *= qq;
        pp += 0.5;
        vec3 vv = COMPAT_TEXTURE(ddd, fract(pp *o)).rgb;
        vv *= vv;
        float ww = REFLECTION*REFLECTION;
        mm = mix(mm, vv*ww, clamp(max(uu.x,uu.y),0.0,1.0));
    }
    float xx = CORNER*0.1;
    vec2 yy = ss - xx;
    if(rr.x>yy.x&&rr.y>yy.y){
        float zz = xx+E_OFFSET*0.01;
        float aaaa = length(rr-yy)-zz;
        if(aaaa>0.0){
            vec2 bbbb= vec2(zz*5.0)*(1.0-B_CORNER);
            vec2 cccc =yy-z(bbbb,FILLET_D);
            vec2 dddd = y(rr, cccc, yy, zz);
            if(dddd.x>0.0){
                vec2 eeee = (dddd - yy)/zz;
                vec2 ffff =dddd - cccc;
                vec2 gggg = (eeee - 0.5) * -2.0;
                gggg *= length(ffff)/length(ffff*gggg);
                gggg *= 1.0 +eeee*0.5;
                rr.x = x(rr.x, dddd.x,gggg.x);
                rr.y = x(rr.y, dddd.y,gggg.y);
                if(abs(rr.x)>tt.x)rr.x = x(rr.x,tt.x,-1.5);
                if(abs(rr.y)>tt.y)rr.y = x(rr.y,tt.y,-1.5);
                rr *= 0.95;
                rr.x /= nn;
                rr *= qq;
                rr += 0.5;
                vec3 vv = COMPAT_TEXTURE(ddd, fract(rr *o)).rgb;
                vv *= 1.0+pow(eeee.x*eeee.y,2.0);
                vv *= vv;    
                float aa = clamp(aaaa*mix(OutputSize.x*PIC_SCALE_X, OutputSize.y*PIC_SCALE_Y, 0.5),0.0,1.0);
                vv *= REFLECTION*REFLECTION;
                mm = mix(mm,vv,aa);
            }
        }
    }
    FragColor.rgb = pow(mm,vec3(1.0/ppppp));
}
#endif