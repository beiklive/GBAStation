//first line must be empty，for libretro to add version
#pragma name PhosphorLine
#pragma parameter GAMMA "Gamma Emulated" 2.2 1.0 4.0 0.1 
#pragma parameter LINE_PITCH_COEF "Scanline Pitch" 1.0 0.1 10.0 0.1
#pragma parameter LINE_THICKNESS "Scanline Thickness" 0.95 0.1 2.0 0.01
#pragma parameter LINE_OPACITY "Scanline Opacity" 0.95 0.0 0.95 0.05 
#pragma parameter LINEAR_X "Horizontal Blending" 0.8 0.0 1.0 0.1
#pragma parameter PIC_SCALE_X "Picture Scale(X)" 1.0 -10.0 10.0 0.01
#pragma parameter PIC_SCALE_Y "Picture Scale(Y)" 1.0 -10.0 10.0 0.01
#pragma parameter INT_TOL "Allow Overscan for Clarity" 0.05 0.0 0.1 0.01
#pragma parameter INT_SCALE "Force Integer Scaling" 0.0 0.0 1.0 0.5
#pragma parameter CURVE_X "CRT Curvature(X)" 0.0 0.0 1.0 0.01
#pragma parameter CURVE_Y "CRT Curvature(Y)" 0.0 0.0 1.0 0.01
#define SCREEN_GAMMA 2.2
#define HT_INTENSITY 0.7
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
COMPAT_VARYING vec4 vParameter2;
uniform mat4 MVPMatrix;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform COMPAT_PRECISION vec2 OrigInputSize;
uniform COMPAT_PRECISION float GAMMA; 
uniform COMPAT_PRECISION float LINE_PITCH_COEF;
uniform COMPAT_PRECISION float LINE_OPACITY; 	
uniform COMPAT_PRECISION float PIC_SCALE_X; 
uniform COMPAT_PRECISION float PIC_SCALE_Y; 
uniform COMPAT_PRECISION float INT_TOL;
uniform COMPAT_PRECISION float INT_SCALE; 
void main()
{
	vec4 p = vec4(InputSize,1.0/InputSize);
	vec4 q = vec4(OrigInputSize,1.0/OrigInputSize);
	float OutputSize_w = 1.0/OutputSize.y;
gl_Position = VertexCoord.x * MVPMatrix[0] + VertexCoord.y * MVPMatrix[1] + VertexCoord.z * MVPMatrix[2] + VertexCoord.w * MVPMatrix[3];
    TEX0.xy = TexCoord.xy;
	vParameter.w = step(q.y+1.0, p.y);
	float multi_y0 = OutputSize.y *q.w;
	float multi_y1 = multi_y0*PIC_SCALE_Y;
	vParameter2.x = max(floor(multi_y1*(1.0 +INT_TOL*step(multi_y1,4.0) )) ,1.0);
	vParameter.y = multi_y0/vParameter2.x;
	vParameter.x = INT_SCALE>0.75? multi_y0/vParameter2.x : 1.0/PIC_SCALE_X;
	vParameter.zw = (1.0-1.0/vParameter.xy)*0.5;
	vParameter.w = floor(vParameter.w*OutputSize.y)*OutputSize_w;
	vParameter2.z = max(floor(vParameter2.x * LINE_PITCH_COEF +0.5) ,3.0);
	vParameter2.w = step(q.y+1.0, p.y);
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
uniform COMPAT_PRECISION vec2 OrigInputSize;
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;
COMPAT_VARYING vec4 vParameter;
COMPAT_VARYING vec4 vParameter2;
uniform COMPAT_PRECISION float GAMMA; 
uniform COMPAT_PRECISION float LINE_PITCH_COEF;
uniform COMPAT_PRECISION float LINE_THICKNESS; 
uniform COMPAT_PRECISION float LINE_OPACITY; 	
uniform COMPAT_PRECISION float LINEAR_X;
uniform COMPAT_PRECISION float CURVE_X; 
uniform COMPAT_PRECISION float CURVE_Y; 
#define Source Texture
float a(vec3 b)
{
	return b.r*0.3+b.g*0.6+b.b*0.1;
}
vec3 c(sampler2D Source, vec2 o, vec4 p, vec2 d)
{
	vec2 e = o * p.xy -0.5;
	vec2 f = fract(e);
	f = clamp((f -0.5)/d +0.5 ,0.0 ,1.0);
	vec2 p00 = floor(e) +0.5;
	vec2 p10 = p00 +vec2(1.0,0.0);
	vec2 p01 = p00 +vec2(0.0,1.0);
	vec2 p11 = p00 +1.0;
	vec2 g = InputSize/TextureSize;
	vec3 c00 = COMPAT_TEXTURE(Source, fract(p00 * p.zw *g)).rgb;
	vec3 c10 = COMPAT_TEXTURE(Source, fract(p10 * p.zw *g)).rgb;
	vec3 c01 = COMPAT_TEXTURE(Source, fract(p01 * p.zw *g)).rgb;
	vec3 c11 = COMPAT_TEXTURE(Source, fract(p11 * p.zw *g)).rgb;
	c00 *= c00;
	c10 *= c10;
	c01 *= c01;
	c11 *= c11;
	vec3 colx0 = mix(c00,c10,f.x);
	vec3 colx1 = mix(c01,c11,f.x);
	return sqrt(mix(colx0,colx1 ,f.y));
}
vec2 h(vec2 r) {
    if (CURVE_X < 0.005 && CURVE_Y < 0.005) return r;
    float rx = 1.0 / clamp(CURVE_X * 2.0, 0.01, 1.0);
    float ry = 1.0 / clamp(CURVE_Y, 0.01, 1.0);
    float s1x = sin(1.0 / rx);
    float s1y = sin(1.0 / ry);
    vec2 uv2 = (r - 0.50001) * 2.0;
    float ux = uv2.x;
    float uy = uv2.y;
    float theta_x = ux / rx;
    float theta_y = uy / ry;
    ux = ux * ux * s1x / sin(theta_x);
    uy = uy * uy * s1y / sin(theta_y);  
    float cos_tx = cos(theta_x);
    float cos_ty = cos(theta_y);
    float j = rx * (1.0 - cos_tx);
    float k = ry * (1.0 - cos_ty);
    float l = 1.0 - k * 0.3;
    ux /= l;
    float m = ux * 0.5 + 0.50001;
    return vec2(m, r.y);
}
vec3 n(vec3 t,float d) {
    t = clamp(t, 0.0, 1.0);
    float p0 = 0.0;
    float p3 = 1.0;
    vec3 u = 1.0 - t;  
    vec3 tt = t * t;   
    vec3 uu = u * u;   
    vec3 uuu = uu * u; 
    vec3 ttt = tt * t; 
    vec3 y = uuu * p0 + 3.0 * uu * t * (0.25+d) + 3.0 * u * tt * 0.75 + ttt * p3;
    return clamp(y, 0.0, 1.0);  
}
void main()
{
	float delta[5];
		delta[0] = 0.21;
		delta[1] = 0.15;
		delta[2] = 0.13;
		delta[3] = 0.12;
		delta[4] = 0.1;
	vec2 g = InputSize/TextureSize;
	vec2 o = TEX0.xy/g;
	vec4 p = vec4(InputSize,1.0/InputSize);
	vec4 q = vec4(OrigInputSize,1.0/OrigInputSize);
	vec2 r = h(o);
	r -= vParameter.zw;
	float s = floor(r.y * OutputSize.y) +0.5;
	r *= vParameter.xy;
	vec2 t = vec2(LINEAR_X, vParameter2.w);
	vec2 u = vec2(0.0);
	if(p.x*q.z>1.5){
		vec3 v = COMPAT_TEXTURE(Source, (r-vec2(q.z*0.45,0.0)) *g).rgb;
		vec3 w = COMPAT_TEXTURE(Source, (r+vec2(q.z*0.45,0.0)) *g).rgb;
		#define www (a(v) - a(w))
		u.x = www*0.25 *q.z;
		u.x *= LINEAR_X;
		t.x = 1.0;
	}
    vec3 x = c(Source, r-u, p, t);
	vec3 y = x;
	x = pow(x,vec3(GAMMA*0.5));
	if(vParameter2.x*LINE_PITCH_COEF>2.9){
		#define zzz vParameter2.z
		y = n(y,delta[int(min(zzz,7.0))-3]);
		y = pow(y,vec3(GAMMA));
		vec3 aa = (y *HT_INTENSITY +(1.0 -HT_INTENSITY)) *zzz;
		aa *= LINE_THICKNESS;
		float bb = mod(s, zzz) - zzz*0.5;
		vec3 cc;
		cc.rb = vec2(abs( bb*2.0 -0.5));
		cc.g = abs(-bb*2.0 -0.5);
		vec3 dd = clamp((aa -cc +0.5), 0.0, 1.0);
		dd *= y / (  y*HT_INTENSITY +1.0 -HT_INTENSITY);
		dd = sqrt(dd);
		FragColor.rgb = mix(x, dd, LINE_OPACITY);	
	}
	else
	FragColor.rgb = x;	
}
#endif