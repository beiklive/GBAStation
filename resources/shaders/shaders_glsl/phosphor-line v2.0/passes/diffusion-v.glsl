
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
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;
vec3 j(sampler2D Source, vec2 d, vec4 e)
{
    float a = d.y * e.y -0.5;
	float b = fract(a);
	float p0 = floor(a) +0.5;
	float p1 = p0 +1.0;
    vec2 c = InputSize/TextureSize;
	vec3 c0 = COMPAT_TEXTURE(Source, fract(vec2(d.x, p0 * e.w)*c)).rgb;
	vec3 c1 = COMPAT_TEXTURE(Source, fract(vec2(d.x, p1 * e.w)*c)).rgb;
	c0 *= c0;
	c1 *= c1;
	return mix(c0, c1, b);
}
void main() {
    float o[5];
        o[0] = 0.218663;
        o[1] = 0.188942;
        o[2] = 0.121895;
        o[3] = 0.058715;
        o[4] = 0.021117;
    vec2 c = InputSize/TextureSize;
    vec2 d = TEX0.xy/c;
    vec4 e = vec4(InputSize,1.0/InputSize);
    #define Source Texture
    vec2 f = d-0.5;
    f.y *=1.0527;
    float g = e.z *1.6;  
    g *= 1.0 + dot(f, f)*0.5; 
    f +=0.5;
    vec3 h = j(Source,f,e).rgb * o[0];  
    for (int i = 1; i < 5; i += 1) {
        vec2 k = vec2(0.0,float(i) * g);
        h += j(Source, f + k,e).rgb * o[i];
        h += j(Source, f - k,e).rgb * o[i];
    }
    FragColor.rgb = sqrt(h* vec3(0.56, 0.71, 1.0));
}
#endif