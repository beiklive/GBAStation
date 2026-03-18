/*Phosphor Dot shader v3.2 by Roc-Y(脑浆油条).
*
*This shader reshade pixels to round dots, the diameter of dot change according pixel bright level.
*This will make low defination content looks cleaner but less sawtooth,just like CRT moniter. 
*Using in retro games are adviced.
*
*Jun 2023.
*
*Copyright (C) 2022-2023 Roc-Y - rocky02009@163.com
*/


//Shader parameters
#pragma parameter PIC_SCALE_X "Picture Scale(X)" 0.0 -10.0 10.0 1.0
#pragma parameter PIC_SCALE_Y "Picture Scale(Y)" 0.0 -10.0 10.0 1.0
#pragma parameter DOT_SCALE "Dot Scale" 1.0 0.1 2.0 0.05
#pragma parameter DOT_OPACITY "Dot Opacity" 0.5 0.0 1.0 0.05 
#pragma parameter DOT_BLOOM "Dot Bloom" 0.0 0.0 2.0 0.05 

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

//uniform COMPAT_PRECISION int FrameDirection;
//uniform COMPAT_PRECISION int FrameCount;
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
#define DOT_GAMMA 2.0; 
uniform COMPAT_PRECISION float PIC_SCALE_X; 
uniform COMPAT_PRECISION float PIC_SCALE_Y; 
uniform COMPAT_PRECISION float DOT_SCALE; 
uniform COMPAT_PRECISION float DOT_OPACITY; 
uniform COMPAT_PRECISION float DOT_BLOOM; 



void main()
{
	//Scaling factor
	vec2 scale0 = floor(OutputSize.xy*OriginalSize.zw+vec2(PIC_SCALE_X,PIC_SCALE_Y)+0.2);
	#define PSSF (InputSize/OrigInputSize)//Pre-shader scale factor
	vec2 scale =scale0 /PSSF; //scale factor offseted according previous shader passes

	//Color sampling
    vec2 uv_ratio = InputSize.xy/TextureSize.xy;
	vec2 pix_co = vTexCoord/uv_ratio * OutputSize.xy;

	pix_co += floor((SourceSize.xy*scale - OutputSize.xy)/2.0 +0.5); //Center the graphic

	vec2 uv = (pix_co/scale/SourceSize.xy)*uv_ratio;
	if(uv.x<0.0 || uv.y<0.0)
		FragColor = vec4(0.,0.,0.,1.);
	else{

    vec3 texColor = COMPAT_TEXTURE(Source, uv).rgb;


	//Dot size
	#define dot_size scale0


	//Dot shading
	//STATEMENT:The following core algorithms are original, and anyone is prohibited from using them to obtain commercial benefits without the formal permission of the author.
	//声明：以下核心算法为独创，任何人未经书面允许，不得用于获取商业利益。

	vec3 col1 = texColor;

	vec3 dot_radius_x = (col1 *HALFTONE_STRENGTH +1.0 -HALFTONE_STRENGTH) *dot_size.x/2.0 *DOT_SCALE;

	vec2 center_offset = vec2(0.0);
	if(dot_size.x>2.5 && dot_size.y>2.5)center_offset = dot_size*0.5;
	else dot_radius_x *= 2.0;


	vec2 pix_co_sub = mod(pix_co, dot_size) - center_offset;
	pix_co_sub.y *= dot_size.x/dot_size.y;

	float distance = length(pix_co_sub);

	vec3 col2 = distance>0.1? sqrt(clamp((dot_radius_x -distance +0.5), 0.0, 1.0)) * (col1 / (  col1*HALFTONE_STRENGTH +1.0 -HALFTONE_STRENGTH)):
	    sqrt(sqrt(clamp(dot_radius_x*2.0, 0.0, 1.0))) *(col1 / (  col1*HALFTONE_STRENGTH +1.0 -HALFTONE_STRENGTH));

    /*
    vec3 col2;
	if(distance>0.1)
        col2 = clamp((dot_radius_x -distance +0.5), 0.0, 1.0);
	else
		col2 = sqrt(clamp(dot_radius_x*2.0, 0.0, 1.0));

	col2 *= pow(col1 / (  col1*HALFTONE_STRENGTH +1.0 -HALFTONE_STRENGTH), vec3(2.0));
    col2 = sqrt(col2);
    //*/
    
	//Final mix
	FragColor.rgb = mix(texColor, col2, DOT_OPACITY) +texColor*DOT_BLOOM;
    //FragColor.rgb = texColor;
	}
} 
#endif
