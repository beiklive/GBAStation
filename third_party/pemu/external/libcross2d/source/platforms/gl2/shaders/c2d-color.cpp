//
// Created by cpasjuste on 17/09/18.
//

// vertex color shader
const char *c2d_color_shader = R"text(
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
COMPAT_VARYING vec4 COL0;

uniform mat4 MVPMatrix;

void main()
{
    gl_Position = MVPMatrix * VertexCoord;
    COL0 = COLOR;
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

void main()
{
    fragColor = vec4(COL0);
}
#endif
)text";
