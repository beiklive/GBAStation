
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
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_VARYING vec4 TEX0;
uniform mat4 MVPMatrix;
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
precision mediump int;
#endif
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform COMPAT_PRECISION vec2 PassPrev2TextureSize;
uniform COMPAT_PRECISION vec2 PassPrev2InputSize;
uniform sampler2D PassPrev2Texture;
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;
uniform COMPAT_PRECISION float GAMMA; 
vec3 a(vec3 v){
    return pow(v,vec3(GAMMA));
}
vec3 b(sampler2D Source, vec2 vTexCoord, vec4 SourceSize)
{
	vec2 c = vTexCoord * SourceSize.xy -0.5;
	vec2 d = fract(c);
	vec2 p00 = floor(c) +0.5;
	vec2 p10 = p00 +vec2(1.0,0.0);
	vec2 p01 = p00 +vec2(0.0,1.0);
	vec2 p11 = p00 +1.0;
    #define PassPrev2uvratio (PassPrev2InputSize/PassPrev2TextureSize)
    SourceSize.zw *= PassPrev2uvratio;
	vec3 c00 = COMPAT_TEXTURE(Source, fract(p00 * SourceSize.zw)).rgb;
	vec3 c10 = COMPAT_TEXTURE(Source, fract(p10 * SourceSize.zw)).rgb;
	vec3 c01 = COMPAT_TEXTURE(Source, fract(p01 * SourceSize.zw)).rgb;
	vec3 c11 = COMPAT_TEXTURE(Source, fract(p11 * SourceSize.zw)).rgb;
	c00 = a(c00);
	c10 = a(c10);
	c01 = a(c01);
	c11 = a(c11);
	#define colx0 mix(c00,c10,d.x)
	#define colx1 mix(c01,c11,d.x)
	return mix(colx0, colx1, d.y);
}
void main() {
    float f[5];
        f[0] = 0.218663;
        f[1] = 0.188942;
        f[2] = 0.121895;
        f[3] = 0.058715;
        f[4] = 0.021117;
    vec2 g = InputSize/TextureSize;
    vec2 vTexCoord = TEX0.xy/g;
    vec4 SourceSize = vec4(InputSize,1.0/InputSize);
    vec4 PassPrev2Size = vec4(PassPrev2InputSize,1.0/PassPrev2InputSize);
    float OutputSize_z = 1.0/OutputSize.x;
    #define Source Texture
    #define FinalViewportSize SourceSize
    #define ColorAdjSize PassPrev2Size
    #define ColorAdj PassPrev2Texture
    vec2 h = vTexCoord-0.5;
    h.x *=1.0527;
    float j = OutputSize_z* FinalViewportSize.y*FinalViewportSize.z*1.6;  
    j *= 1.0 + dot(h, h)*0.5; 
    h +=0.5;
    vec3 k = b(ColorAdj,h,ColorAdjSize).rgb * f[0];  
    for (int i = 1; i < 5; i += 1) {
        vec2 l = vec2(float(i) * j, 0.0);
        k += b(ColorAdj, h + l,ColorAdjSize).rgb * f[i];
        k += b(ColorAdj, h - l,ColorAdjSize).rgb * f[i];
    }

    if(TEX0.x<0.0)FragColor = COMPAT_TEXTURE(Texture,TEX0.xy);
    FragColor.rgb = sqrt(k);
}
#endif