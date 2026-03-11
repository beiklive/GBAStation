
#pragma name PhosphorLine
#pragma parameter GAMMA "Gamma Emulated" 2.2 1.0 4.0 0.1 
#pragma parameter LINE_PITCH_COEF "Scanline Pitch" 1.0 0.1 10.0 0.1
#pragma parameter LINE_THICKNESS "Scanline Thickness" 0.95 0.1 2.0 0.01
#pragma parameter LINE_OPACITY "Scanline Opacity" 0.95 0.0 0.95 0.05 
#pragma parameter LINEAR_X "Horizontal Blending" 0.8 0.0 1.0 0.1
#pragma parameter G_REV "Green Misaligned" 1.0 0.0 1.0 1.0
#pragma parameter PIC_SCALE_X "Picture Scale(X)" 1.0 -10.0 10.0 0.01
#pragma parameter PIC_SCALE_Y "Picture Scale(Y)" 1.0 -10.0 10.0 0.01
#pragma parameter INT_TOL "Allow Overscan for Integer Scaling" 0.05 0.0 0.3 0.01
#pragma parameter INT_SCALE "Keep Aspect Ratio" 0.0 0.0 1.0 1.0
#pragma parameter MASK_INTENSITY "Shadow Mask Intensity" 0.5 0.0 0.9 0.1
#pragma parameter MASK_TYPE "Shadow Mask Type" 1.0 0.0 1.0 1.0
#pragma parameter MASK_SIZE "Shadow Mask Size" 1.0 0.0 10.0 0.1
#define a 2.2
#define b 0.7
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
uniform COMPAT_PRECISION float MASK_SIZE;
void main()
{
	vec4 m = vec4(InputSize,1.0/InputSize);
	vec4 n = vec4(OrigInputSize,1.0/OrigInputSize);
	float OutputSize_w = 1.0/OutputSize.y;
gl_Position = VertexCoord.x * MVPMatrix[0] + VertexCoord.y * MVPMatrix[1] + VertexCoord.z * MVPMatrix[2] + VertexCoord.w * MVPMatrix[3];
    TEX0.xy = TexCoord.xy;
	vParameter.w = step(n.y+1.0, m.y);
	float c = OutputSize.y *n.w;
	float d = c*PIC_SCALE_Y;
	vParameter2.x = max(floor(d*(1.0 +INT_TOL)) ,1.0);
	vParameter.y = c/vParameter2.x;
	vParameter.x = INT_SCALE>0.75? c/vParameter2.x/PIC_SCALE_X : 1.0/PIC_SCALE_X;
	vParameter.zw = (1.0-1.0/vParameter.xy)*0.5;
	vParameter.w = floor(vParameter.w*OutputSize.y)*OutputSize_w;
	vParameter2.z = max(floor(vParameter2.x * LINE_PITCH_COEF +0.5) ,3.0);
	vParameter2.w = step(n.y+1.0, m.y);
	vParameter2.y = max(floor(d *MASK_SIZE*0.6 +0.5), 2.0);
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
uniform COMPAT_PRECISION float G_REV;
uniform COMPAT_PRECISION float MASK_INTENSITY;
uniform COMPAT_PRECISION float MASK_TYPE;
#define Source Texture
float luma(vec3 f)
{
	return f.x*0.3+f.y*0.6+f.z*0.1;
}
vec3 g(sampler2D Source, vec2 l, vec4 m, vec2 blur)
{
	vec2 h = l * m.xy -0.5;
	vec2 j = fract(h);
	j = clamp((j -0.5)/blur +0.5 ,0.0 ,1.0);
	vec2 p00 = floor(h) +0.5;
	vec2 p10 = p00 +vec2(1.0,0.0);
	vec2 p01 = p00 +vec2(0.0,1.0);
	vec2 p11 = p00 +1.0;
	vec2 k = InputSize/TextureSize;
	vec3 c00 = COMPAT_TEXTURE(Source, fract(p00 * m.zw *k)).rgb;
	vec3 c10 = COMPAT_TEXTURE(Source, fract(p10 * m.zw *k)).rgb;
	vec3 c01 = COMPAT_TEXTURE(Source, fract(p01 * m.zw *k)).rgb;
	vec3 c11 = COMPAT_TEXTURE(Source, fract(p11 * m.zw *k)).rgb;
	c00 *= c00;
	c10 *= c10;
	c01 *= c01;
	c11 *= c11;
	vec3 colx0 = mix(c00,c10,j.x);
	vec3 colx1 = mix(c01,c11,j.x);
	return sqrt(mix(colx0,colx1 ,j.y));
}
void main()
{
	vec2 k = InputSize/TextureSize;
	vec2 l = TEX0.xy/k;
	vec4 m = vec4(InputSize,1.0/InputSize);
	vec4 n = vec4(OrigInputSize,1.0/OrigInputSize);
	vec2 o = l;
	o -= vParameter.zw;
	float p = floor(o.y * OutputSize.y) +0.5;
	o *= vParameter.xy;
	vec2 q = vec2(LINEAR_X, vParameter2.w);
	vec2 r = vec2(0.0);
	if(m.x*n.z>1.5){
		vec3 s = COMPAT_TEXTURE(Source, (o-vec2(n.z*0.45,0.0)) *k).rgb;
		vec3 t = COMPAT_TEXTURE(Source, (o+vec2(n.z*0.45,0.0)) *k).rgb;
		#define u (luma(s) - luma(t))
		r.x = u*0.25 *n.z;
		r.x *= LINEAR_X;
		q.x = 1.0;
	}
    vec3 v = g(Source, o-r, m, q);
	v = pow(v,vec3(GAMMA));
	vec3 w = v;
	if(vParameter2.x*LINE_PITCH_COEF>1.9){
		#define PITCH vParameter2.z
		vec3 x = (w *b +(1.0 -b)) *PITCH;
		x *= LINE_THICKNESS;
		float y = mod(p, PITCH) - PITCH*0.5;
		vec3 z;
		z.rb = vec2(abs( y*2.0 -0.5));
		z.g = abs(-sign(G_REV-0.5)*y*2.0 -0.5);
		vec3 aa = clamp((x -z +0.5), 0.0, 1.0);
		aa *= w / (  w*b +1.0 -b);
		FragColor.rgb = mix(v, aa, LINE_OPACITY);	
	}
	else
	FragColor.rgb = v;	
	if(MASK_INTENSITY>0.001){
    #define MASK_SIZE vParameter2.y
    vec2 h;
	h.x = floor(l.x * OutputSize.x)+0.5;
	h.y = p;
    float bb = floor(MASK_SIZE*0.5 +0.1);
    h.x += floor(h.y/bb)*bb *MASK_TYPE;
    vec3 z;
    #define cc (floor(MASK_SIZE/2.0 +0.5))
    z = mod(vec3(h.x, h.x+cc ,h.x), MASK_SIZE);
    #define dd (z -MASK_SIZE*0.5)
    z = abs(dd*2.0 -0.5);
    w = FragColor.rgb;
    vec3 x = (w *MASK_INTENSITY +(1.0 -MASK_INTENSITY)) *MASK_SIZE;
    vec3 aa = clamp((x -z +0.5), 0.0, 1.0);
    aa *= w / (  w*MASK_INTENSITY +1.0 -MASK_INTENSITY);
	FragColor.rgb = aa;
	}
	FragColor.rgb = pow(FragColor.rgb,vec3(1.0/a));
}
#endif