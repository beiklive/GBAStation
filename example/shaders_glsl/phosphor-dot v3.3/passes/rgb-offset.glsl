
//Shader parameters
#pragma parameter unalign "RGB Offset" 0.0 -1.0 1.0 0.1
#pragma parameter STRETCH_X "Stretch to Fullscreen(X) [slower if it works]" 1.0 0.0 2.0 0.1
#pragma parameter STRETCH_Y "Stretch to Fullscreen(Y) [slower if it works]" 1.0 0.0 2.0 0.1
#pragma parameter FOOTER "↑↑↑ Phosphor Dot v3.3 by Roc-Y ↑↑↑" 0.0 0.0 0.0 1.0


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

vec4 _oPosition1; 
uniform mat4 MVPMatrix;

void main()
{
    gl_Position = MVPMatrix * VertexCoord;
    COL0 = COLOR;
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
uniform COMPAT_PRECISION vec2 OrigInputSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 OutputSize;

//#define ColorAdj Texture
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;


// compatibility #defines
#define Source Texture
#define vTexCoord TEX0.xy

#define SourceSize vec4(InputSize, 1.0 / InputSize) //either TextureSize or InputSize
#define TextureSize vec4(TextureSize, 1.0 / TextureSize) //either TextureSize or InputSize
#define OriginalSize vec4(OrigInputSize, 1.0 / OrigInputSize) 

#define outsize vec4(OutputSize, 1.0 / OutputSize)

// All parameter floats need to have COMPAT_PRECISION in front of them
uniform COMPAT_PRECISION float PIC_SCALE_X; 
uniform COMPAT_PRECISION float PIC_SCALE_Y; 
uniform COMPAT_PRECISION float DOT_PITCH_X; 
uniform COMPAT_PRECISION float DOT_PITCH_Y; 
uniform COMPAT_PRECISION float CURVE_X; 
uniform COMPAT_PRECISION float CURVE_Y; 
uniform COMPAT_PRECISION float unalign; 
uniform COMPAT_PRECISION float STRETCH_X; 
uniform COMPAT_PRECISION float STRETCH_Y; 


vec4 cubic(float x)
{
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w;
    w.x =  3.0*x2 - x3 - 3.0*x + 1.0;
    w.y =  3.0*x3 - 6.0*x2       + 4.0;
    w.z =  3.0*x2 -3.0*x3 + 3.0*x + 1.0;
    w.w =  x3;
    return w / 6.0;
}

vec3 texture_bicubic(sampler2D Source, vec2 texcoord, vec2 texscale, float offset_pixco)
{
texcoord -= 0.5;//This cubic fillter assumed integer coordinates as the center of the output pixel
float fx = fract(texcoord.x);
float fy = fract(texcoord.y);
texcoord.x -= fx;
texcoord.y -= fy;

vec4 xcubic = cubic(fx);
vec4 ycubic = cubic(fy);

vec4 c = vec4(texcoord.x - 0.5, texcoord.x + 1.5, texcoord.y - 0.5, texcoord.y + 1.5);
vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);
vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) /s;

vec3 sample0;
    sample0.r = COMPAT_TEXTURE(Source, vec2(offset.x+offset_pixco, offset.z) *texscale).r;
    sample0.g = COMPAT_TEXTURE(Source, vec2(offset.x,              offset.z) *texscale).g;
    sample0.b = COMPAT_TEXTURE(Source, vec2(offset.x-offset_pixco, offset.z) *texscale).b;
vec3 sample1;
    sample1.r = COMPAT_TEXTURE(Source, vec2(offset.y+offset_pixco, offset.z) *texscale).r;
    sample1.g = COMPAT_TEXTURE(Source, vec2(offset.y,              offset.z) *texscale).g;
    sample1.b = COMPAT_TEXTURE(Source, vec2(offset.y-offset_pixco, offset.z) *texscale).b;
vec3 sample2;
    sample2.r = COMPAT_TEXTURE(Source, vec2(offset.x+offset_pixco, offset.w) *texscale).r;
    sample2.g = COMPAT_TEXTURE(Source, vec2(offset.x,              offset.w) *texscale).g;
    sample2.b = COMPAT_TEXTURE(Source, vec2(offset.x-offset_pixco, offset.w) *texscale).b;
vec3 sample3;
    sample3.r = COMPAT_TEXTURE(Source, vec2(offset.y+offset_pixco, offset.w) *texscale).r;
    sample3.g = COMPAT_TEXTURE(Source, vec2(offset.y,              offset.w) *texscale).g;
    sample3.b = COMPAT_TEXTURE(Source, vec2(offset.y-offset_pixco, offset.w) *texscale).b;

float sx = s.x / (s.x + s.y);
float sy = s.z / (s.z + s.w);

return mix(
mix(sample3, sample2, sx),
mix(sample1, sample0, sx), sy);
}


vec3 texture_cubic_y(sampler2D Source, vec2 texcoord, vec2 texscale, float offset_pixco)
{
texcoord.y -= 0.5;//This cubic fillter assumed integer coordinates as the center of the output pixel
float fy = fract(texcoord.y);
texcoord.y -= fy;

vec4 ycubic = cubic(fy);

vec2 c = vec2(texcoord.y - 0.5, texcoord.y + 1.5);
vec2 s = vec2(ycubic.x + ycubic.y, ycubic.z + ycubic.w);
vec2 offset = c + vec2(ycubic.y, ycubic.w) / s;

vec3 sample0;
sample0.r = COMPAT_TEXTURE(Source, vec2(texcoord.x+offset_pixco, offset.x) * texscale).r;
sample0.g = COMPAT_TEXTURE(Source, vec2(texcoord.x,              offset.x) * texscale).g;
sample0.b = COMPAT_TEXTURE(Source, vec2(texcoord.x-offset_pixco, offset.x) * texscale).b;
vec3 sample1;
sample1.r = COMPAT_TEXTURE(Source, vec2(texcoord.x+offset_pixco, offset.y) * texscale).r;
sample1.g = COMPAT_TEXTURE(Source, vec2(texcoord.x,              offset.y) * texscale).g;
sample1.b = COMPAT_TEXTURE(Source, vec2(texcoord.x-offset_pixco, offset.y) * texscale).b;

float sy = s.x / (s.x + s.y);

return mix(sample1, sample0, sy);
}


vec2 curve_y(vec2 uv)
{
    if(CURVE_X<0.005 && CURVE_Y<0.005) return uv;
    else
    {
        vec2 r = 1.0/ clamp(vec2(CURVE_X*2.0,CURVE_Y),0.01,1.0);
        vec2 uv2 = (uv-0.50001)*2.0;
        uv2 /= sin(uv2/r)/sin(1.0/r)/uv2;
        vec2 depth = r-cos(uv2/r)*r;
        uv2 /= (1.0 - depth.yx*0.3);
        uv2 = uv2/2.0+0.50001;
        return vec2(uv.x,uv2.y);
    }
}


#define OutputSize SourceSize
void main()
{
    vec2 uv_ratio = InputSize.xy/TextureSize.xy;

	vec2 scale0 = max(OutputSize.xy*OriginalSize.zw*vec2(PIC_SCALE_X,PIC_SCALE_Y),1.0);
	if(DOT_PITCH_X>0.999)scale0.x =floor(scale0.x+0.05);
	if(DOT_PITCH_Y>0.999)scale0.y =floor(scale0.y+0.05);

    //RGB Offset
    float offset_coef = OrigInputSize.x<480.0? 1.0 : 2.0;
    float offset_pixco = OutputSize.x / OrigInputSize.x *0.333 *clamp(unalign *offset_coef ,-1.0 ,1.0);
    offset_pixco = floor(offset_pixco+0.5);

    vec2 stretch_ratio;
        stretch_ratio =  clamp(scale0/(OutputSize.xy/OrigInputSize.xy),0.0,1.0);

    #define TH 0.95
    if((stretch_ratio.x>TH || STRETCH_X<0.001) && (stretch_ratio.y>TH || STRETCH_Y<0.001) && CURVE_Y<0.01 && CURVE_X<0.01 || scale0.y==2.0)
    {
        FragColor.r = COMPAT_TEXTURE(Source, vTexCoord +vec2(+SourceSize.z*uv_ratio.x *offset_pixco,0.0)).r;
        FragColor.g = COMPAT_TEXTURE(Source, vTexCoord).g;
        FragColor.b = COMPAT_TEXTURE(Source, vTexCoord +vec2(-SourceSize.z*uv_ratio.x *offset_pixco,0.0)).b;
    }
    else
    {
        stretch_ratio = (stretch_ratio -1.0) *vec2(STRETCH_X,STRETCH_Y) +1.0;
        vec2 texcoord = curve_y(vTexCoord/uv_ratio)*OutputSize.xy*stretch_ratio;
        texcoord += floor(OutputSize.xy*(1.0-stretch_ratio)/2.0+0.5);

        FragColor.rgb = (stretch_ratio.x > TH || STRETCH_X<0.001)?
            texture_cubic_y(Source, texcoord, uv_ratio/OutputSize.xy, offset_pixco):
            texture_bicubic(Source, texcoord, uv_ratio/OutputSize.xy, offset_pixco);
    }
}
#endif
