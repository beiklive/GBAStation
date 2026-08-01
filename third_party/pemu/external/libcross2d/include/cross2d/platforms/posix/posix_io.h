//
// Created by cpasjuste on 17/11/17.
//

#ifndef C2D_POSIX_IO_H
#define C2D_POSIX_IO_H

#include "cross2d/skeleton/io.h"

namespace c2d {
    class POSIXIo : public Io {
    public:
        File getFile(const std::string &path) override;

        bool exist(const std::string &path) override;

        size_t getSize(const std::string &file) override;

        Type getType(const std::string &file) override;

        bool create(const std::string &path) override;

        bool removeFile(const std::string &path) override;

        bool removeDir(const std::string &path) override;

        bool copy(const std::string &src, const std::string &dst,
                  const std::function<void(File, File, float)> &callback = nullptr) override;

        std::vector<Io::File> getDirList(const std::string &path, bool sort, bool showHidden) override;

        std::vector<File> findFiles(const std::string &path, const std::vector<std::string> &whitelist,
                                    const std::string &blacklist = "", bool stopOnFirst = true) override;

        size_t read(const std::string &file, char *out, size_t size = 0, size_t offset = 0) override;

        bool write(const std::string &file, const char *data, size_t size) override;

        std::string getDataPath() override;

        std::string getRomFsPath() override;

    private:
        bool _copy(const std::string &src, const std::string &dst,
                   const std::function<void(File, File, float)> &callback = nullptr);

        static bool _copyFile(const File &src, const File &dst,
                              const std::function<void(File, File, float)> &callback = nullptr);
    };
}

#endif //C2D_POSIX_IO_H
