/*Phosphor Dot shader v3.3 by Roc-Y(脑浆油条).
*
*This shader reshade pixels to round dots, the diameter of dot change according pixel bright level.
*This will make low defination content looks cleaner but less sawtooth,just like CRT moniter. 
*Using in retro games are adviced.
*
*Jan 2024.
*
*Copyright (C) 2022-2023 Roc-Y - rocky02009@163.com
*/


//Shader parameters
//#pragma parameter VERTICAL "Vertical Game" 0.0 0.0 1.0 1.0
#pragma parameter LINEAR_X "Linear Filter(X) [-0.10=CRT-Mode]" 0.0 -0.1 1.0 0.1
#pragma parameter LINEAR_Y "Linear Filter(Y)" 0.0 0.0 1.0 0.1
#pragma parameter MT_SAMPLE "Dot Sampler Mode[0=Center(slower) 1=Multiple]" 1.0 0.0 1.0 1.0
#pragma parameter PIC_SCALE_X "Picture Scale(X)" 1.0 -10.0 10.0 0.01
#pragma parameter PIC_SCALE_Y "Picture Scale(Y)" 1.0 -10.0 10.0 0.01
#pragma parameter DOT_PITCH_X "Dot Pitch(X) [<1:turn off integer scaling]" 1.0 0.0 10.0 0.1
#pragma parameter DOT_PITCH_Y "Dot Pitch(Y) [<1:turn off integer scaling]" 1.0 0.1 10.0 0.1
#pragma parameter DOT_SCALE_X "Dot Scale(X)" 1.0 0.1 2.0 0.05
#pragma parameter DOT_SCALE_Y "Dot Scale(Y)" 1.0 0.1 2.0 0.05
#pragma parameter DOT_OPACITY "Dot Opacity" 1.0 0.0 1.0 0.05 
#pragma parameter DOT_BLOOM "Dot Bloom" 0.0 0.0 2.0 0.05 
#pragma parameter CURVE_X "CRT Curvature(X) [slower]" 0.0 0.0 1.0 0.01
#pragma parameter CURVE_Y "CRT Curvature(Y) [slower]" 0.0 0.0 1.0 0.01

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
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform COMPAT_PRECISION vec2 OrigInputSize;

uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;


// compatibility #defines
#define Source Texture
#define vTexCoord TEX0.xy

#define SourceSize vec4(InputSize, 1.0 / InputSize) 
#define TextureSize vec4(TextureSize, 1.0 / TextureSize) 
#define OriginalSize vec4(OrigInputSize, 1.0 / OrigInputSize) 

#define OutputSize vec4(OutputSize, 1.0 / OutputSize)

// All parameter floats need to have COMPAT_PRECISION in front of them
//uniform COMPAT_PRECISION float VERTICAL; 
uniform COMPAT_PRECISION float DOT_GAMMA; 
uniform COMPAT_PRECISION float LINEAR_X;
uniform COMPAT_PRECISION float LINEAR_Y;
uniform COMPAT_PRECISION float SCALE_OFFSET_X; 
uniform COMPAT_PRECISION float SCALE_OFFSET_Y; 
uniform COMPAT_PRECISION float PIC_SCALE_X; 
uniform COMPAT_PRECISION float PIC_SCALE_Y; 
uniform COMPAT_PRECISION float DOT_PITCH_X;
uniform COMPAT_PRECISION float DOT_PITCH_Y;
uniform COMPAT_PRECISION float DOT_SCALE_X; 
uniform COMPAT_PRECISION float DOT_SCALE_Y; 
uniform COMPAT_PRECISION float CURVE_X; 
uniform COMPAT_PRECISION float CURVE_Y; 
uniform COMPAT_PRECISION float MT_SAMPLE; 
uniform COMPAT_PRECISION float DOT_OPACITY; 
uniform COMPAT_PRECISION float DOT_BLOOM; 



float luma(vec3 col)
{
	return col.r*0.3+col.g*0.6+col.b*0.1;
}

//Variable Binear vTexCoord offset function
vec3 texture_blur(sampler2D Source, vec2 uv, vec4 InputSize, vec2 blur)
{
	vec2 pix_co = uv * InputSize.xy;
	pix_co = clamp(fract(pix_co+blur*0.5)/blur ,0.0 ,1.0) + floor(pix_co + blur*0.5) -0.5;
	return COMPAT_TEXTURE(Source, pix_co * InputSize.zw).rgb;
}



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

//sRGB LtoV
float LtoV(float l)
{
    return l<0.00313? l*12.9232102 :1.055*pow(l,0.4166)-0.055;
}

vec3 LtoV3(vec3 c)
{
	c.r = LtoV(c.r);
	c.g = LtoV(c.g);
	c.b = LtoV(c.b);
	return c;
}


void main()
{
	//Scaling factor
	vec2 scale0 = max(OutputSize.xy*OriginalSize.zw*vec2(PIC_SCALE_X,PIC_SCALE_Y),1.0);
	if(DOT_PITCH_X>0.999)scale0.x =floor(scale0.x+0.05);
	if(DOT_PITCH_Y>0.999)scale0.y =floor(scale0.y+0.05);
	vec2 PSSF = SourceSize.xy/OrigInputSize.xy;//Pre-shader scale factor
	vec2 scale =scale0 /PSSF; //scale factor offseted according previous shader passes

	//Color sampling
	vec2 pix_co = vTexCoord*TextureSize.xy/InputSize.xy * OutputSize.xy;

	pix_co += floor((SourceSize.xy*scale - OutputSize.xy)/2.0 +0.5); //Center the graphic

	vec2 uv_ratio = InputSize.xy/TextureSize.xy;
	vec2 uv = curve_x(pix_co/scale/SourceSize.xy)*uv_ratio;
	if(uv.x<0.0 || uv.y<0.0)
		FragColor = vec4(0.,0.,0.,1.);
	else{
	float uv_offset_x = 0.0;
	vec2 Linear = vec2(LINEAR_X,LINEAR_Y);
		if(LINEAR_X<-0.05)
			{
				//CRT horizontal fillter
				float EBW = (OriginalSize.w*DOT_SCALE_Y*OutputSize.y*PIC_SCALE_Y*OutputSize.z/PIC_SCALE_X);
				if(SourceSize.x*OriginalSize.z<2.0)//When no pre-upscale shader
					Linear.x = EBW*OriginalSize.x*0.9;
				else{
				vec3 col_l = texture_blur(Source, uv-vec2(TextureSize.z*PSSF.x*0.45,0.0), TextureSize, vec2(0.0));
				vec3 col_r = texture_blur(Source, uv+vec2(TextureSize.z*PSSF.x*0.45,0.0), TextureSize, vec2(0.0));
				float luma_diff = luma(col_l) - luma(col_r);
				uv_offset_x = luma_diff * EBW *0.1;//EBW(Electron beam width)
				Linear.x = 1.0;
				}
			}

    vec3 texColor = texture_blur(Source, uv-vec2(uv_offset_x,0.0), TextureSize, Linear);
	//texColor = pow( texColor, vec3(DOT_GAMMA) );


	//Dot size
	vec2 dot_size = floor(scale0 * vec2(DOT_PITCH_X, DOT_PITCH_Y)+0.5);
	if(dot_size.x<1.5)dot_size.x = DOT_PITCH_X>0.001? 2.0:1.0;
	if(dot_size.y<0.5)dot_size.y = 1.0;

	//Dot shading
	//STATEMENT:The following core algorithms are original, and anyone is prohibited from using them to obtain commercial benefits without the formal permission of the author.
	//声明：以下核心算法为独创，任何人未经书面允许，不得用于获取商业利益。

	vec3 col1;
	if(MT_SAMPLE==1.0)
	col1 = texColor;
	else
	{
		vec2 pix_co = floor(pix_co/dot_size)*dot_size +dot_size *0.5;
	    col1 = texture_blur(Source, curve_x(pix_co/scale/SourceSize.xy)*uv_ratio-vec2(uv_offset_x,0.0), TextureSize, Linear);
	}
	col1= sqrt(col1);

	vec3 dot_radius_x = (col1 *HALFTONE_STRENGTH +1.0 -HALFTONE_STRENGTH) *dot_size.x/2.0;
	dot_radius_x *= DOT_SCALE_X;

	float dot_ratio = (dot_size.x*DOT_SCALE_X)/(dot_size.y*DOT_SCALE_Y);

	vec2 center_offset = vec2(0.0);
	if(dot_size.x>2.5)center_offset.x = dot_size.x*0.5;
	else{
		dot_radius_x *= 2.0;
		dot_ratio *=2.0;
	}
	if(dot_size.y>2.5)center_offset.y = dot_size.y*0.5;
	else dot_ratio *=0.5;

	vec2 pix_co_sub = mod(pix_co, dot_size) - center_offset;
	pix_co_sub.y *= dot_ratio;

	float distance = length(pix_co_sub);

	float coef;

	vec3 col2;
	
	if(distance>0.1){
		vec2 s_pcs = pix_co_sub*pix_co_sub;
		float s_dr = dot_ratio*dot_ratio;
		coef = sqrt((s_pcs.x*s_dr*s_dr + s_pcs.y*s_dr) / (s_pcs.x*s_dr*s_dr+s_pcs.y));
		col2 = clamp((dot_radius_x -distance +0.5*coef)/coef, 0.0, 1.0);}
	else
		col2 = clamp((dot_radius_x*2.0-clamp(dot_radius_x*2.0-dot_ratio,0.0,1.0))*2.0*dot_radius_x/dot_ratio, 0.0, 1.0);

	col2 *= pow(col1 / (  col1*HALFTONE_STRENGTH +1.0 -HALFTONE_STRENGTH), vec3(2.0));

    
	//Final mix
	FragColor.rgb = mix(texColor, col2, DOT_OPACITY) +texColor*DOT_BLOOM;
	FragColor.rgb = LtoV3(FragColor.rgb);

	}
} 
#endif
