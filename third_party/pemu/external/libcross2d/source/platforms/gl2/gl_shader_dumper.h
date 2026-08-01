//
// Created by cpasjuste on 27/01/2022.
//

#ifndef C2D_GL_SHADER_DUMPER_H
#define C2D_GL_SHADER_DUMPER_H
#ifdef GL_DUMP_SHADERS

namespace c2d {
    class GLShaderDumper {
    public:
        GLShaderDumper(const std::string &dumpPath);

        ~GLShaderDumper();

        void dump(int id, const std::string &name, const std::string &type);

    private:
        std::string dump_path;
        std::string header_path;
        const std::string header_name = "shaders.h";
    };
}

#endif //GL_DUMP_SHADERS
#endif //C2D_GL_SHADER_DUMPER_H
