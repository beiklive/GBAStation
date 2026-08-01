//
// Created by cpasjuste on 17/11/17.
//

#include <unistd.h>
#include <cstring>
#include <dirent.h>
#include "cross2d/c2d.h"

using namespace c2d;

Io::File DCIo::getFile(const std::string &path) {

    File file{};

    file_t f = fs_open(path.c_str(), O_DIR | O_RDONLY);
    if (f == FILEHND_INVALID) {
        f = fs_open(path.c_str(), O_RDONLY);
        if (f == FILEHND_INVALID) {
            return file;
        }
        file.type = Type::File;
    } else {
        file.type = Type::Directory;
    }

    file.name = Utility::baseName(path);
    file.path = path;
    file.size = fs_total(f);
    fs_close(f);

    return file;
}

size_t DCIo::getSize(const std::string &file) {

    file_t f = fs_open(file.c_str(), O_RDONLY);
    if (f == FILEHND_INVALID) {
        return -1;
    }

    size_t size = fs_total(f);
    fs_close(f);

    return size;
}

Io::Type DCIo::getType(const std::string &file) {

    Io::Type type;

    file_t f = fs_open(file.c_str(), O_DIR | O_RDONLY);
    if (f == FILEHND_INVALID) {
        f = fs_open(file.c_str(), O_RDONLY);
        if (f == FILEHND_INVALID) {
            return Type::Unknown;
        }
        type = Type::File;
    } else {
        type = Type::Directory;
    }

    fs_close(f);

    return type;
}

bool DCIo::existsFile(const std::string &path) {

    file_t f = fs_open(path.c_str(), O_RDONLY);
    if (f != FILEHND_INVALID) {
        fs_close(f);
        return true;
    }

    return false;
}

bool DCIo::existsDir(const std::string &path) {

    file_t f = fs_open(path.c_str(), O_DIR | O_RDONLY);
    if (f != FILEHND_INVALID) {
        fs_close(f);
        return true;
    }

    return false;
}

bool DCIo::exist(const std::string &path) {

    file_t f = fs_open(path.c_str(), O_RDONLY);
    if (f != FILEHND_INVALID) {
        fs_close(f);
        return true;
    }

    f = fs_open(path.c_str(), O_DIR | O_RDONLY);
    if (f != FILEHND_INVALID) {
        fs_close(f);
        return true;
    }

    return false;
}

bool DCIo::create(const std::string &path) {
    return fs_mkdir(path.c_str()) == 0;
}

bool DCIo::removeFile(const std::string &path) {
    return fs_unlink(path.c_str()) == 0;
}

bool DCIo::removeDir(const std::string &path) {

    dirent_t *ent;
    file_t fd;

    File file = getFile(path);
    if (file.type != Type::Directory) {
        return false;
    }

    if ((fd = fs_open(file.path.c_str(), O_RDONLY | O_DIR)) != FILEHND_INVALID) {
        while ((ent = fs_readdir(fd)) != nullptr) {

            if (strcmp(ent->name, ".") == 0 || strcmp(ent->name, "..") == 0)
                continue;

            std::string newPath =
                    file.path + (Utility::endsWith(file.path, "/") ? "" : "/") + std::string(ent->name);

            if (getType(newPath) == Type::Directory) {
                if (!removeDir(newPath)) {
                    fs_close(fd);
                    return false;
                }
            } else {
                if (!removeFile(newPath)) {
                    fs_close(fd);
                    return false;
                }
            }
        }
        fs_close(fd);
    } else {
        return false;
    }

    return fs_rmdir(path.c_str()) == 0;
}

size_t DCIo::read(const std::string &file, char *out, size_t size, const size_t offset) {
    file_t fd;
    size_t file_size;
    size_t read_size;

    fd = fs_open(file.c_str(), O_RDONLY);
    if (fd == FILEHND_INVALID) {
        printf("DCIo::read: can't open %s\n", file.c_str());
        return -1;
    }

    file_size = fs_total(fd);

    if (size <= 0) {
        size = file_size;
    }

    if (offset + size > file_size) {
        size = file_size - offset;
    }

    if (offset > 0) {
        fs_seek(fd, offset, SEEK_SET);
    }

    read_size = fs_read(fd, out, size);
    if (read_size != size) {
        fs_close(fd);
        printf("DCIo::read: can't read %s\n", file.c_str());
        return -1;
    }

    fs_close(fd);

    return read_size;
}

bool DCIo::write(const std::string &file, const char *data, size_t size) {

    file_t fd;

    fd = fs_open(file.c_str(), O_CREAT | O_TRUNC | O_WRONLY);
    if (fd == FILEHND_INVALID) {
        return false;
    }

    fs_write(fd, data, size);
    fs_close(fd);

    return true;
}

std::vector<Io::File> DCIo::getDirList(const std::string &path, bool sort, bool showHidden) {

    std::vector<Io::File> files;
    dirent_t *ent;
    file_t fd;

    if (path.empty()) {
        return files;
    }

    fd = fs_open(path.c_str(), O_RDONLY | O_DIR);
    if (fd == FILEHND_INVALID) {
        return files;
    }

    while ((ent = fs_readdir(fd)) != nullptr) {

        //printf("getDirList: %s\n", ent->name);

        // skip "."
        if (strlen(ent->name) == 1 && ent->name[0] == '.') {
            continue;
        }

        // skip ".." file in romdisk (?!)
        if (ent->attr != O_DIR && strlen(ent->name) == 2
            && ent->name[0] == '.' && ent->name[1] == '.') {
            continue;
        }

        // skip "hidden" files
        if (!showHidden && ent->name[0] == '.') {
            if (strlen(ent->name) != 2 && ent->name[1] != '.') {
                continue;
            }
        }

        // skip some stuff
        std::string lower = Utility::toLower(ent->name);
        if (strcmp(ent->name, "ram") == 0 || strcmp(ent->name, "pty") == 0
            || strcmp(ent->name, "rd") == 0 || strcmp(ent->name, "pc") == 0
            || lower == "recycler" || lower == "$recycle.bin" || lower == "system volume information") {
            continue;
        }

        File file;
        file.name = ent->name;
        file.path = Utility::removeLastSlash(path) + "/" + file.name;
        file.type = ent->attr == O_DIR ? Type::Directory : Type::File;
        file.size = ent->size;
        files.push_back(file);
    }

    fs_close(fd);

    if (sort) {
        std::sort(files.begin(), files.end(), compare);
    }

    return files;
}

std::vector<Io::File> DCIo::findFiles(const std::string &path, const std::vector<std::string> &whitelist,
                                         const std::string &blacklist, bool stopOnFirst) {
    std::vector<Io::File> files{};
    dirent_t *ent;
    file_t fd;

    if (path.empty()) {
        return files;
    }

    if ((fd = fs_open(path.c_str(), O_RDONLY | O_DIR)) != FILEHND_INVALID) {
        while ((ent = fs_readdir(fd)) != nullptr) {
            for (const auto &search : whitelist) {
                if (Utility::contains(ent->name, search)) {
                    if (!blacklist.empty() && Utility::contains(ent->name, blacklist)) {
                        continue;
                    }

                    File file;
                    file.name = ent->name;
                    file.path = Utility::removeLastSlash(path) + "/" + file.name;
                    file.type = ent->attr == O_DIR ? Type::Directory : Type::File;
                    file.size = ent->size;
                    files.emplace_back(file);

                    if (stopOnFirst) break;
                }
            }
        }
        fs_close(fd);
    }

    return files;
}

bool DCIo::copy(const std::string &src, const std::string &dst,
                const std::function<void(File, File, float)> &callback) {

    bool res = _copy(src, dst, callback);

    if (callback != nullptr) {
        callback(File{}, File{}, res ? 2 : -1);
    }

    return res;
}

bool DCIo::_copy(const std::string &src, const std::string &dst,
                 const std::function<void(File, File, float)> &callback) {

    File srcFile;
    File dstFile;
    dirent_t *ent;
    file_t fd;

    if (src == dst) {
        if (callback != nullptr) {
            callback(srcFile, dstFile, -1);
        }
        return false;
    }

    // The destination is a sub-folder of the source folder
    size_t size = src.size();
    if (src.compare(0, size, dst) == 0
        && (dst[size] == '/' || dst[size - 1] == '/')) {
        if (callback != nullptr) {
            callback(srcFile, dstFile, -1);
        }
        return false;
    }

    srcFile = getFile(src);
    dstFile = srcFile;
    dstFile.name = Utility::baseName(src);
    dstFile.path = dst;
    if (!Utility::endsWith(dstFile.path, "/")) {
        dstFile.path += "/";
    }
    dstFile.path += dstFile.name;

    if (srcFile.type == Type::File) {
        bool res = _copyFile(srcFile, dstFile, callback);
        if (callback != nullptr) {
            callback(srcFile, dstFile, 2);
        }
        return res;
    }

    if (!exist(dstFile.path) && !create(dstFile.path)) {
        if (callback != nullptr) {
            callback(srcFile, dstFile, -1);
        }
        return false;
    }

    if ((fd = fs_open(srcFile.path.c_str(), O_RDONLY | O_DIR)) != FILEHND_INVALID) {
        while ((ent = fs_readdir(fd)) != nullptr) {

            if (strcmp(ent->name, ".") == 0 || strcmp(ent->name, "..") == 0)
                continue;

            std::string newSrcPath =
                    srcFile.path + (Utility::endsWith(dstFile.path, "/") ? "" : "/") + std::string(ent->name);
            File newSrcFile = getFile(newSrcPath);
            File newDstFile = newSrcFile;
            newDstFile.path = dstFile.path;

            if (newSrcFile.type == Type::File) {
                newDstFile.path += (Utility::endsWith(dstFile.path, "/") ? "" : "/") + std::string(ent->name);
                bool success = _copyFile(newSrcFile, newDstFile, callback);
                if (!success) {
                    if (callback != nullptr) {
                        callback(srcFile, dstFile, -1);
                    }
                    fs_close(fd);
                    return false;
                }
            } else {
                bool success = _copy(newSrcFile.path, newDstFile.path, callback);
                if (!success) {
                    if (callback != nullptr) {
                        callback(srcFile, dstFile, -1);
                    }
                    fs_close(fd);
                    return false;
                }
            }
        }
        fs_close(fd);
    } else {
        return false;
    }

    return true;
}

bool DCIo::_copyFile(const File &src, const File &dst,
                     const std::function<void(File, File, float)> &callback) {

    if (src.path == dst.path) {
        if (callback != nullptr) {
            callback(src, dst, -1);
        }
        return false;
    }

    // The destination is a sub-folder of the source folder
    size_t len = src.path.size();
    if (src.path.compare(0, len, dst.path) == 0
        && (dst.path[len] == '/' || dst.path[len - 1] == '/')) {
        if (callback != nullptr) {
            callback(src, dst, -1);
        }
        return false;
    }

    file_t srcFd = fs_open(src.path.c_str(), O_RDONLY);
    if (srcFd == FILEHND_INVALID) {
        if (callback != nullptr) {
            callback(src, dst, -1);
        }
        return false;
    }

    file_t dstFd = fs_open(dst.path.c_str(), O_CREAT | O_WRONLY | O_TRUNC);
    if (dstFd == FILEHND_INVALID) {
        fs_close(srcFd);
        if (callback != nullptr) {
            callback(src, dst, -1);
        }
        return false;
    }

    auto buf = (unsigned char *) malloc(C2D_IO_COPY_BUFFER_SIZE);
    if (buf == nullptr) {
        fs_close(srcFd);
        fs_close(dstFd);
        if (callback != nullptr) {
            callback(src, dst, -1);
        }
        return false;
    }

    if (callback != nullptr) {
        callback(src, dst, 0);
    }

    size_t readBytes, writeBytes, totalBytes = 0;
    while ((readBytes = fs_read(srcFd, buf, C2D_IO_COPY_BUFFER_SIZE)) > 0) {
        writeBytes = fs_write(dstFd, buf, readBytes);
        if (writeBytes < 0) {
            free(buf);
            fs_close(srcFd);
            fs_close(dstFd);
            if (callback != nullptr) {
                callback(src, dst, -1);
            }
            return false;
        }

        if (callback != nullptr) {
            totalBytes += writeBytes;
            float progress = (float) totalBytes / (float) src.size;
            callback(src, dst, progress);
        }
    }

    free(buf);
    fs_close(srcFd);
    fs_close(dstFd);

    return true;
}
