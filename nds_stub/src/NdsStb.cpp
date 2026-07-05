#define STBI_ONLY_PNG
#define STBI_NO_SIMD
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_NO_THREAD_LOCALS
#define STBI_MAX_DIMENSIONS 4096
#define STBI_ASSERT(x) ((void)0)
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
