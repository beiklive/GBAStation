//
// Created by cpasjuste on 08/06/2021.
//

#ifndef C2D_GL1_H
#define C2D_GL1_H

#if __GLAD__

#include "glad/glad.h"

#elif __GLEW__

#include <GL/glew.h>

#endif

#include "cross2d/platforms/gl1/gl_renderer.h"
#include "cross2d/platforms/gl2/gl_texture.h"

#define C2DTexture GLTexture

#endif //C2D_GL2_H
