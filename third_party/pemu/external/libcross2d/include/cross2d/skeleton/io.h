//
// Created by cpasjuste on 17/11/17.
//

#ifndef CROSS2D_IO_H
#define CROSS2D_IO_H

#include <string>
#include <vector>
#include "cross2d/skeleton/sfml/Color.hpp"
#include "texture.h"
#include "utility.h"

#define C2D_IO_COPY_BUFFER_SIZE (256 * 1024)

namespace c2d {
    class Io {
    public:
        enum class Type {
            Unknown = 0,
            File = 1,
            Directory = 2
        };

        class File {
        public:
            File() = default;

            File(const std::string &name, const std::string &path,
                 Type type = Type::File, const size_t &size = 0) {
                this->name = name;
                this->path = path;
                this->type = type;
                this->size = size;
            }

            bool isFile() const {
                return type == Type::File;
            }

            std::string name;
            std::string path;
            size_t size = 0;
            Type type = Type::Unknown;
        };

        Io() = default;

        virtual ~Io() = default;

        ///
        /// \return read only data path
        virtual std::string getRomFsPath() {
            return "./";
        }

        ///
        /// \return read write data path
        virtual std::string getDataPath() {
            if (!m_data_path.empty()) return m_data_path;
            return "./";
        }

        virtual void setDataPath(const std::string &path) {
            m_data_path = path;
        }

        virtual File getFile(const std::string &path) {
            return {};
        }

        virtual bool exist(const std::string &path) {
            return false;
        };

        virtual size_t getSize(const std::string &file) {
            return 0;
        };

        virtual Type getType(const std::string &file) {
            return Type::Unknown;
        };

        virtual bool create(const std::string &path) {
            return false;
        };

        virtual bool removeFile(const std::string &path) {
            return false;
        };

        virtual bool removeDir(const std::string &path) {
            return false;
        };

        virtual bool copy(const std::string &src, const std::string &dst,
                          const std::function<void(File, File, float)> &callback = nullptr) {
            return false;
        }

        virtual std::vector<Io::File> getDirList(const std::string &path, bool sort = false, bool showHidden = false) {
            return {};
        };

        virtual std::vector<File> findFiles(const std::string &path, const std::vector<std::string> &whitelist,
                                            const std::string &blacklist = "", bool stopOnFirst = true) {
            return {};
        }

        virtual size_t read(const std::string &file, char *out, size_t size = 0, size_t offset = 0) {
            return -1;
        }

        virtual bool write(const std::string &file, const char *data, size_t size) {
            return false;
        }

        static bool compare(const Io::File &a, const Io::File &b) {
            if (a.type == Type::Directory && b.type != Type::Directory) {
                return true;
            }
            if (a.type != Type::Directory && b.type == Type::Directory) {
                return false;
            }
            return Utility::toLower(a.name) < Utility::toLower(b.name);
        }

    protected:
        std::string m_data_path{};
    };
}

#endif //CROSS2D_IO_H
