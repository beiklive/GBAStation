//
// Created by cpasjuste on 27/01/17.
//

#ifndef GL_SHADERS_H
#define GL_SHADERS_H

#include "cross2d/skeleton/shader_list.h"

namespace c2d {

    class GLShader : public ShaderList::Shader {

    public:

        GLShader(const std::string &name, const char *source, int size, const std::string &version);

        GLShader(const std::string &name, const char *vertex, const char *fragment,
                 int vertexSize = 0, int fragmentSize = 0, const std::string &version = "100");

        ~GLShader() override;

        void SetUniformMatrix(const GLchar *n, const GLfloat *v);

        void SetUniform(const GLchar *n, const Vector2f &v);

        void SetUniform(const GLchar *n, float v0, float v1);

        void SetUniform(const GLchar *n, int v);

        GLuint GetProgram();

    private:
        GLuint program = -1;
    };

    class GLShaderList : public ShaderList {

    public:

        GLShaderList();

        ~GLShaderList() override;
    };
}

#endif // GL_SHADERS_H
