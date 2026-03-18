


#define SCREEN_GAMMA 2.2
#define HALFTONE_STRENGTH 0.7


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
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec4 COLOR;
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_ATTRIBUTE vec4 PassPrev2TexCoord;


COMPAT_VARYING vec4 COL0;
COMPAT_VARYING vec4 TEX0;
COMPAT_VARYING vec4 TEX2;

vec4 _oPosition1; 
uniform mat4 MVPMatrix;

void main()
{
    gl_Position = MVPMatrix * VertexCoord;
    COL0 = COLOR;
    TEX0.xy = TexCoord.xy;
    TEX2.xy = PassPrev2TexCoord.xy;
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

uniform COMPAT_PRECISION vec2 OrigInputSize;
uniform COMPAT_PRECISION vec2 OrigTextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 PassPrev2InputSize;
uniform COMPAT_PRECISION vec2 PassPrev2TextureSize;
uniform COMPAT_PRECISION vec2 OutputSize;


uniform sampler2D Texture;
uniform sampler2D PassPrev2Texture;
COMPAT_VARYING vec4 TEX0;
COMPAT_VARYING vec4 TEX2;


// compatibility #defines
#define Source Texture
#define vTexCoord TEX0.xy
#define PassPrev2vTexCoord TEX2.xy

#define SourceSize vec4(InputSize, 1.0 / InputSize) //either TextureSize or InputSize
#define TextureSize vec4(TextureSize, 1.0 / TextureSize) //either TextureSize or InputSize
#define OriginalSize vec4(OrigInputSize, 1.0 / OrigInputSize) 

#define outsize vec4(OutputSize1, 1.0 / OutputSize1)

//uniform COMPAT_PRECISION float VERTICAL; 
uniform COMPAT_PRECISION float PIC_SCALE_X; 
uniform COMPAT_PRECISION float PIC_SCALE_Y; 
uniform COMPAT_PRECISION float DOT_PITCH_X;
uniform COMPAT_PRECISION float DOT_PITCH_Y;
uniform COMPAT_PRECISION float CURVE_X; 
uniform COMPAT_PRECISION float CURVE_Y; 
uniform COMPAT_PRECISION float DOT_GAMMA; 
uniform COMPAT_PRECISION float DIFFUSION; 

vec2 curve_x(vec2 uv)
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
        return vec2(uv2.x,uv.y);
    }
}


void main()
{
    //vec2 OutputSize1 = mix(OutputSize.xy, OutputSize.yx, VERTICAL);
	#define OutputSize1 OutputSize

	if(DIFFUSION>0.0){

	//Scaling factor
	vec2 scale0 = max(OutputSize.xy*OriginalSize.zw*vec2(PIC_SCALE_X,PIC_SCALE_Y),1.0);
	if(DOT_PITCH_X>0.999)scale0.x =floor(scale0.x+0.05);
	if(DOT_PITCH_Y>0.999)scale0.y =floor(scale0.y+0.05);
	vec2 scale = scale0*OrigInputSize.xy / SourceSize.xy; //scale factor offseted according previous shader passes

	//Color sampling
    vec2 uv_ratio = InputSize.xy/TextureSize.xy;

    vec2 pix_co = floor(vTexCoord /uv_ratio *OutputSize1.xy) + 0.5;

	pix_co += floor((SourceSize.xy*scale - OutputSize1.xy)/2.0 +0.5); //Center the graphic

	vec2 uv = pix_co/(InputSize.xy *scale);
    vec3 texColor;
    if(pix_co.x<0.0 || pix_co.y<0.0)texColor = vec3(0.0);else
    texColor = COMPAT_TEXTURE(Source, curve_x((uv-0.5)/1.0526+0.5)*uv_ratio).rgb;


	//Dot size
	vec2 dot_size = floor(scale0 * vec2(DOT_PITCH_X, DOT_PITCH_Y)+0.5);
	if(dot_size.x<1.5)dot_size.x = DOT_PITCH_X>0.001? 2.0:1.0;
	if(dot_size.y<0.5)dot_size.y = 1.0;

	//Dot shading
	float dot_radius_x = dot_size.x*0.207;
	float dot_ratio = dot_size.x/dot_size.y;

	vec2 center_offset = vec2(0.0);
	if(dot_size.x>2.5)center_offset.x = dot_size.x*0.5;
	else{
		dot_radius_x *= 2.0;
		dot_ratio *=2.0;
	}
	if(dot_size.y>2.5)center_offset.y = dot_size.y*0.5;
	else dot_ratio *=0.5;

	vec2 pix_co_sub = mod(pix_co+dot_size*0.5, dot_size) - center_offset;
	pix_co_sub.y *= dot_ratio;

	float distance = length(pix_co_sub);

	vec3 col2;

	if(distance-0.5>dot_size.x*0.212)
	col2 = vec3(0.0);
	else
	{
		float dot_radius_x = dot_size.x*0.207;
		vec2 s_pcs = pix_co_sub*pix_co_sub;
		float s_dr = dot_ratio*dot_ratio;
		float coef = sqrt((s_pcs.x*s_dr*s_dr + s_pcs.y*s_dr) / (s_pcs.x*s_dr*s_dr+s_pcs.y));
		col2.x = clamp((dot_radius_x -distance +0.5*coef)/coef, 0.0, 1.0);
		col2 = col2.x*texColor.rgb;
	}

	//Diffusion
	uv = uv*0.96+0.02;
	vec3 Diffusion = (uv.x<0.0 || uv.y<0.0)? vec3(0.0) :COMPAT_TEXTURE(Source, curve_x(uv)*uv_ratio).rgb;


	//Mix and output resault to screen
	#define Diff DIFFUSION*DIFFUSION//*THICKNESS*pow(1.0+DOT_BLOOM,2.0)
    FragColor.rgb = pow(COMPAT_TEXTURE(PassPrev2Texture, TEX2.xy).rgb, vec3(SCREEN_GAMMA)) + (col2 + Diffusion)*Diff;
	FragColor.rgb = pow(FragColor.rgb, vec3(1.0/SCREEN_GAMMA));
	}
	else
	FragColor.rgb = COMPAT_TEXTURE(PassPrev2Texture, TEX2.xy).rgb;
} 
#endif
