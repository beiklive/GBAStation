//
// Created by cpasjuste on 17/09/18.
//

const char *c2d_texture_shader = R"text(
#if defined(VERTEX)
#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#define COMPAT_TEXTURE texture
#define COMPAT_PRECISION
#else
// assume opengl es...
#define COMPAT_VARYING varying
#define COMPAT_ATTRIBUTE attribute
#define COMPAT_TEXTURE texture2D
#define COMPAT_PRECISION mediump
#endif

COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec4 COLOR;
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_VARYING vec4 COL0;
COMPAT_VARYING vec4 TEX0;

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
out vec4 fragColor;
#define COMPAT_PRECISION
#else
// assume opengl es...
#define COMPAT_VARYING varying
#define fragColor gl_FragColor
#define COMPAT_TEXTURE texture2D
#define COMPAT_PRECISION mediump
#endif

COMPAT_VARYING COMPAT_PRECISION vec4 COL0;
COMPAT_VARYING COMPAT_PRECISION vec4 TEX0;
uniform sampler2D tex;

void main()
{
    fragColor = COMPAT_TEXTURE(tex, TEX0.xy) * COL0;
}

#endif
)text";
