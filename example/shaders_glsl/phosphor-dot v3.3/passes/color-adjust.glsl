#pragma name ColorAdj

//Shader parameters
#pragma parameter HEADER "↓↓↓ Phosphor Dot v3.3 by Roc-Y ↓↓↓" 0.0 0.0 0.0 1.0
#pragma parameter NTSCintensity "NTSC Color intensity" 0.0 0.0 1.0 0.1
#pragma parameter BLK "Black Level (IRE)" 2.0 -7.5 7.5 0.5
#pragma parameter EXPOSURE "Exposure" 1.0 0.1 2.0 0.05
#pragma parameter CT "Color Temperature Adjust (K)" 1000.0 -2800.0 2800.0 100.0
#pragma parameter DOT_GAMMA "Gamma Emulated" 2.2 1.0 4.0 0.1 

#define DisplayCT 6500.0 //Displayer color temperature estimated


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

#define SourceSize vec4(InputSize, 1.0 / InputSize) //either TextureSize or InputSize
#define SourceTextureSize vec4(TextureSize, 1.0 / TextureSize) //either TextureSize or InputSize

#define outsize vec4(OutputSize, 1.0 / OutputSize)

// All parameter floats need to have COMPAT_PRECISION in front of them
uniform COMPAT_PRECISION float NTSCintensity; 
uniform COMPAT_PRECISION float BLK; 
uniform COMPAT_PRECISION float EXPOSURE; 
uniform COMPAT_PRECISION float CT; 
uniform COMPAT_PRECISION float DOT_GAMMA; 


//NTSC Color
vec3 NTSCtoSRGB( vec3 c )
 { 
    const mat3 NTSCtoXYZ = mat3(
        0.6068909,  0.1735011,  0.2003480, 
        0.2989164,  0.5865990,  0.1144845, 
        0.0000000,  0.0660957,  1.1162243);

    const mat3 XYZtoSRGB = mat3(
         3.2404542, -1.5371385, -0.4985314,
        -0.9692660,  1.8760108,  0.0415560,
         0.0556434, -0.2040259,  1.0572252);

    return pow(clamp(clamp(pow(c,vec3(2.2)) * NTSCtoXYZ,0.0,1.0)*XYZtoSRGB,0.0,1.0), vec3(1.0/2.2)); 
 }

//Color Temperature
vec3 colorTemperatureToRGB(float temperature){
	mat3 m = (temperature > DisplayCT)?
		mat3(
		1745.0425298314172, 1216.6168361476490, -8257.7997278925690,
		-2666.3474220535695, -2173.1012343082230, 2575.2827530017594,
		0.55995389139931482, 0.70381203140554553, 1.8993753891711275):
		mat3(
		0.0, -2902.1955373783176, -8257.7997278925690,
		0.0, 1669.5803561666639, 2575.2827530017594,
		1.0, 1.3302673723350029, 1.8993753891711275);
	return mix(clamp(vec3(m[0] / (vec3(temperature) + m[1]) + m[2]), vec3(0.0), vec3(1.0)), vec3(1.0), smoothstep(1000.0, 0.0, temperature));
}


void main()
{
    vec3 col0 = COMPAT_TEXTURE(Source, vTexCoord).rgb;

    if(NTSCintensity<0.1);else
    //NTSC Color
    col0 = mix(col0.rgb, NTSCtoSRGB(col0.rgb), NTSCintensity);

    col0 = (col0-(BLK *0.01)) *(100.0/(100.0-BLK)); //Black Level Adjust
    col0 =clamp(col0*EXPOSURE,0.0,1.0);

    vec3 col1 = col0;
    if(CT==0.0);else
    {
    //Temperature
    col1 = col0 * colorTemperatureToRGB(DisplayCT+CT); 
    col1 *= dot(col0, vec3(0.2126, 0.7152, 0.0722)) / max(dot(col1, vec3(0.2126, 0.7152, 0.0722)), 1e-5);  //Luminance Preservation
    }

    FragColor.rgb = pow(col1,vec3(DOT_GAMMA));
} 
#endif
