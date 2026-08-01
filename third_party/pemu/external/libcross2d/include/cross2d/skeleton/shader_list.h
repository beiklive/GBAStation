//
// Created by cpasjuste on 27/01/17.
//

#ifndef C2D_SHADERLIST_H
#define C2D_SHADERLIST_H

#include <string>
#include <vector>

namespace c2d {

    class ShaderList {

    public:

        class Shader {
        public:
            Shader(const std::string &n) {
                name = n;
            }

            virtual ~Shader() = default;

            std::string name;
            bool available = false;
        };

        ShaderList() = default;

        virtual ~ShaderList() {
            printf("~ShaderList\n");
            delete (color);
            for (auto shader: list) {
                delete (shader);
            }
            list.clear();
        }

        virtual void add(Shader *shader) {
            list.emplace_back(shader);
        }

        virtual Shader *get(int index) {
            if ((size_t) index >= list.size()) {
                return nullptr;
            }
            return list[index];
        }

        virtual Shader *get(const std::string &name) {
            for (auto shader: list) {
                if (shader->name == name) {
                    return shader;
                }
            }
            return nullptr;
        }

        virtual std::vector<std::string> getNames() {
            std::vector<std::string> names;
            for (auto shader: list) {
                names.push_back(shader->name);
            }
            return names;
        }

        virtual int getCount() {
            return (int) list.size();
        }

        Shader *color = nullptr;

    private:
        std::vector<Shader *> list;
    };
}

#endif //C2D_SHADERLIST_H
