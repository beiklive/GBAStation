#pragma parameter W_STRETCH "Fit Width to Screen" 0.0 0.0 2.0 0.05
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

uniform mat4 MVPMatrix;
uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;


void main()
{
    gl_Position = VertexCoord.x * MVPMatrix[0] + VertexCoord.y * MVPMatrix[1] + VertexCoord.z * MVPMatrix[2] + VertexCoord.w * MVPMatrix[3];
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

uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;

uniform COMPAT_PRECISION float W_STRETCH; 

void main()
{
    vec2 uv_ratio = InputSize / TextureSize;
    vec2 pix_co = TEX0.xy / uv_ratio * OutputSize;
    vec2 TargetSize = InputSize / 1.5;
    TargetSize.x = mix(TargetSize.x,OutputSize.x,W_STRETCH);
    pix_co -= floor((OutputSize - TargetSize) /2.0);
    vec2 uv = (pix_co+0.5) / TargetSize;
    uv *= uv_ratio;
    if(uv.x<0.0 || uv.y<0.0)
	    FragColor = vec4(0.,0.,0.,1.);
    else
        FragColor.rgb = (uv.x<0.0 && uv.y<0.0)? vec3(0.0):COMPAT_TEXTURE(Texture, uv).rgb;
} 
#endif
