/*
This is a very simple, compact and very fast Blur shader based and optimized on the Gaussian Blur shader published at
https://www.shadertoy.com/view/Xltfzj
*/



//Shader parameters
#pragma parameter DIFFUSION "CRT Diffusion [slower]" 0.5 0.0 2.0 0.05

#define SCREEN_GAMMA 2.2



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
COMPAT_ATTRIBUTE vec4 PassPrev2TexCoord;
COMPAT_VARYING vec4 COL0;
COMPAT_VARYING vec4 TEX0;

vec4 _oPosition1; 
uniform mat4 MVPMatrix;

void main()
{
    gl_Position = MVPMatrix * VertexCoord;
    COL0 = COLOR;
    TEX0.xy = PassPrev2TexCoord.xy;
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
uniform COMPAT_PRECISION vec2 PassPrev2TextureSize;
uniform COMPAT_PRECISION vec2 PassPrev2InputSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 OutputSize;

//#define ColorAdj Texture
uniform sampler2D PassPrev2Texture;
COMPAT_VARYING vec4 TEX0;


// compatibility #defines
#define Source Texture
#define vTexCoord TEX0.xy

#define SourceSize vec4(InputSize, 1.0 / InputSize) //either TextureSize or InputSize
#define TextureSize vec4(TextureSize, 1.0 / TextureSize) //either TextureSize or InputSize

#define outsize vec4(OutputSize, 1.0 / OutputSize)

// All parameter floats need to have COMPAT_PRECISION in front of them
uniform COMPAT_PRECISION float DOT_GAMMA; 
uniform COMPAT_PRECISION float DIFFUSION; 



void main()
{
	if(DIFFUSION>0.0){

    vec2 uv_ratio = PassPrev2InputSize.xy/PassPrev2TextureSize.xy;

    #define Pi 6.28318530718 // Pi*2
    
    // GAUSSIAN BLUR SETTINGS {{{
	float Size = 8.0; // BLUR SIZE (Radius)
    #define Directions 16.0 // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
    #define Quality 5.0 // BLUR QUALITY (Default 4.0 - More is better but slower)
	Size *= 1.0 + pow(length(vTexCoord/uv_ratio-vec2(0.5))/5.0,2.0)*10.0;

    // GAUSSIAN BLUR SETTINGS }}}
    vec2 Radius = Size/vec2(InputSize.x/InputSize.y*240.0,240.0);
    
    // Pixel colour
	#define scale 0.95
	vec2 uv = (vTexCoord/uv_ratio-0.5)/scale+0.5;
    vec3 Color = (uv.x<0.0 || uv.y<0.0)? vec3(0.0) :COMPAT_TEXTURE(PassPrev2Texture, uv*uv_ratio).rgb;
    
    // Blur calculations
    for( float d=-Pi/Directions/2.0; d<Pi-Pi/Directions/2.0; d+=Pi/Directions)
    {
		for(float i=1.0/Quality; i<0.999; i+=1.0/Quality)
        {
            vec2 uv1 = (uv+vec2(cos(d),sin(d))*Radius*i)*uv_ratio;
			Color += (uv1.x<0.0 || uv1.y<0.0)? vec3(0.0) :COMPAT_TEXTURE(PassPrev2Texture, uv1).rgb*pow(1.0-i,2.0);		
        }
    }
    Color /= (Quality-1.0) * Directions +1.0;

    // Output to diffusion-mixer
    FragColor.rgb =  Color* vec3(0.56,0.71,1.0);
    }
    else
    FragColor = vec4(0.0);

} 
#endif
