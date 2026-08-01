//
// Created by cpasjuste on 27/01/2022.
//

#ifdef GL_DUMP_SHADERS

#include "cross2d/c2d.h"
#include "gl_shader_dumper.h"

#ifdef __PS4__

#include <orbis/Pigletv2VSH.h>

extern "C" void glPigletGetShaderBinarySCE(
        GLuint program, GLsizei bufSize, GLsizei *length,
        GLenum *binaryFormat, void *binary);
extern "C" void glGetProgramBinary(
        GLuint program, GLsizei bufSize, GLsizei *length,
        GLenum *binaryFormat, void *binary);
#endif

using namespace c2d;

GLShaderDumper::GLShaderDumper(const std::string &dumpPath) {
    dump_path = dumpPath;
    header_path = dump_path + "/" + header_name;

    FILE *fp = fopen(header_path.c_str(), "w");
    fprintf(fp, "#ifndef C2D_SHADERS_H\n#define C2D_SHADERS_H\n\n");
    fclose(fp);
}

GLShaderDumper::~GLShaderDumper() {
    FILE *fp = fopen(header_path.c_str(), "a");
    fprintf(fp, "\n#endif //C2D_SHADERS_H\n");
    fclose(fp);
}

void GLShaderDumper::dump(int id, const std::string &name, const std::string &type) {
    GLenum format = 0;
    int need_comma = 0, size;
    std::string path = dump_path + "/" + name + "_" + type + ".cpp";
    char *buf = (char *) malloc(0x5000);
    memset(buf, 0, 0x5000);

    // dump shader
#ifdef __PS4__
    glPigletGetShaderBinarySCE((GLuint) id, 0x5000, &size, &format, buf);
#else
    format = GL_SGX_PROGRAM_BINARY_IMG;
    glGetProgramBinaryOES((GLuint) id, 0x5000, &size, &format, buf);
#endif

    if (size > 0) {
        // add shader to cpp file
        printf("GLShaderDumper::dump: "
               "dumping shader to: %s, (size: %i)\n", path.c_str(), size);

        std::string array_name = "c2d_" + name + "_" + type;
        std::replace(array_name.begin(), array_name.end(), '-', '_');
        array_name.erase(std::remove(array_name.begin(), array_name.end(), '.'), array_name.end());
        FILE *fp = fopen(path.c_str(), "w");
#ifdef __VITA__
        fwrite(buf, size, 1, fp);
#else
        fprintf(fp, "#include \"%s\"\n\nconst char %s[%i] = {", header_name.c_str(), array_name.c_str(), size);
        for (int i = 0; i < size; ++i) {
            if (need_comma)
                fprintf(fp, ", ");
            else
                need_comma = 1;
            if ((i % 11) == 0)
                fprintf(fp, "\n\t");
            fprintf(fp, "0x%.2x", buf[i] & 0xff);
        }
        fprintf(fp, "\n};\n\n");
        fclose(fp);

        // add shader to header file
        fp = fopen(header_path.c_str(), "a");
        fprintf(fp, "extern const char %s[];\n", array_name.c_str());
        fprintf(fp, "const int %s_length = %i;\n", array_name.c_str(), size);
#endif
        fclose(fp);
    } else {
        printf("GLShaderDumper::dump: "
               "dumping shader failed...: %s\n", path.c_str());
    }

    free(buf);
}

#endif
