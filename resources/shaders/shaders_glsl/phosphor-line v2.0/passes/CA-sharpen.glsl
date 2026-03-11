
#pragma name ColorAdj

#pragma parameter SHARPEN "S-Video Resharpen" 0.3 0.0 2.0 0.1 
#pragma parameter NTSCintensity "NTSC TV Color intensity" 0.0 0.0 2.0 0.1
#pragma parameter BLK "Black Level Assumed(%)" 0.0 -17.5 7.5 0.5
#pragma parameter EXPOSURE "Exposure" 1.0 0.1 2.0 0.01
#pragma parameter CT "Temperature+ (*1000K)" 0.0 -2.8 2.8 0.1
#pragma parameter CORNER "Rounded Corners" 0.0 0.0 1.0 0.05 
#pragma parameter CPOY "Corner Position(Y)" 0.0 -50.0 0.0 0.5
#pragma parameter PPOY "Picture Position(Y)" 0.0 -50.0 50.0 1.0

#define DisplayCT 6500.0 

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
COMPAT_VARYING vec3 vt;


uniform mat4 MVPMatrix;
uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;

uniform COMPAT_PRECISION float CT; 

vec3 a(float b){
	mat3 m = (b > DisplayCT)?
		mat3(
		1745.0425298314172, 1216.6168361476490, -8257.7997278925690,
		-2666.3474220535695, -2173.1012343082230, 2575.2827530017594,
		0.55995389139931482, 0.70381203140554553, 1.8993753891711275):
		mat3(
		0.0, -2902.1955373783176, -8257.7997278925690,
		0.0, 1669.5803561666639, 2575.2827530017594,
		1.0, 1.3302673723350029, 1.8993753891711275);
	return mix(clamp(vec3(m[0] / (vec3(b) + m[1]) + m[2]), vec3(0.0), vec3(1.0)), vec3(1.0), smoothstep(1000.0, 0.0, b));
}

void main()
{
    gl_Position = VertexCoord.x * MVPMatrix[0] + VertexCoord.y * MVPMatrix[1] + VertexCoord.z * MVPMatrix[2] + VertexCoord.w * MVPMatrix[3];
    TEX0.xy = TexCoord.xy;
    
    if(abs(CT)>0.01) vt = a(DisplayCT+CT*1000.0);

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
uniform COMPAT_PRECISION vec2 OrigInputSize;
uniform COMPAT_PRECISION float OriginalAspect;
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;
COMPAT_VARYING vec3 vt;



uniform COMPAT_PRECISION float SHARPEN; 
uniform COMPAT_PRECISION float NTSCintensity;
uniform COMPAT_PRECISION float BLK; 
uniform COMPAT_PRECISION float EXPOSURE; 
uniform COMPAT_PRECISION float CT; 
uniform COMPAT_PRECISION float CORNER; 
uniform COMPAT_PRECISION float CPOY;
uniform COMPAT_PRECISION float PPOY;


#define Source Texture



vec3 c( vec3 c )
 { 
    const mat3 d = mat3(
        0.6068909,  0.1735011,  0.2003480, 
        0.2989164,  0.5865990,  0.1144845, 
        0.0000000,  0.0660957,  1.1162243);

    const mat3 e = mat3(
         3.2404542, -1.5371385, -0.4985314,
        -0.9692660,  1.8760108,  0.0415560,
         0.0556434, -0.2040259,  1.0572252);

    return pow(clamp(clamp(pow(c,vec3(2.2)) * d,0.0,1.0)*e,0.0,1.0), vec3(1.0/2.2)); 
 }



vec3 f(vec2 centerCoord) {
    float g = 1.0/TextureSize.x;
    vec2 h = InputSize/TextureSize;
    vec2 j = centerCoord*h;

    float k = COMPAT_TEXTURE(Source, j + vec2(-g, 0.0)).x;
    float l = COMPAT_TEXTURE(Source, j + vec2(g, 0.0)).x;
    vec3 m = COMPAT_TEXTURE(Source, j).xyz;
    
    m.x = m.x * (SHARPEN*2.0+1.0) - (k+l) * SHARPEN;
    return m;
}


vec3 n(vec3 o) {
    float p = o.y - 0.5;
    float q = o.z - 0.5;
    float R = o.x + 1.402 * q;
    float s = o.x - 0.344 * p - 0.714 * q;
    float t = o.x + 1.772 * p;
    return vec3(R, s, t);
}

void main()
{
    vec2 h = InputSize/TextureSize;
    vec2 vTexCoord = TEX0.xy/h;
    vec4 SourceSize = vec4(InputSize,1.0/InputSize);
    float OutputSize_w = 1.0/OutputSize.y;


    vec2 j = vTexCoord;
    j.y += PPOY *SourceSize.w;
    vec3 u = clamp(n(f(j)),0.0,1.0);

    if(NTSCintensity>0.01)
    u = mix(u.rgb, c(u.rgb), NTSCintensity*0.5);

    u = (u-(BLK *0.01)) *(100.0/(100.0-BLK)); 
    u =clamp(u*EXPOSURE,0.0,1.0);

    vec3 v = u;
    if(abs(CT)>0.01){
    v = u * vt; 
    v *= dot(u, vec3(0.2126, 0.7152, 0.0722)) / max(dot(v, vec3(0.2126, 0.7152, 0.0722)), 1e-5); 
    }

if (CORNER > 0.0) {
    vec2 w = vTexCoord - 0.5;
    float x = OriginalAspect<0.0001? OrigInputSize.x/OrigInputSize.y : OriginalAspect;
    w.x *= x;
    w = abs(w);
    
    float y = CORNER * 0.1;
    float z = 1.0+CPOY*0.01;
    vec2 m = vec2(x *0.5, 0.5)*vec2(1.0,z) - y;
    float bb = clamp((z*0.5-w.y)*SourceSize.y*0.5+0.5,0.0,1.0);
    v *= bb;
    
    float cc = step(m.x, w.x) * step(m.y, w.y);
    if(cc>0.5){
        float aa = (y + OutputSize_w*0.5 - distance(w, m)) / OutputSize_w;
        float dd = cc * clamp(aa, 0.0, 1.0) + (1.0 - cc);
        v *= dd;
    }  
      
}
    FragColor.rgb = v;
}
#endif