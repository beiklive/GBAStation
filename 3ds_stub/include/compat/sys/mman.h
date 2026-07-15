#pragma once

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define PROT_NONE 0x0
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

#define MAP_PRIVATE 0x2
#define MAP_ANON 0x20
#define MAP_ANONYMOUS MAP_ANON
#define MAP_FAILED ((void*)-1)

static inline void* mmap(void*, size_t length, int, int, int fd, off_t offset) {
    if (length == 0) {
        return NULL;
    }

    void* data = malloc(length);
    if (!data) {
        return NULL;
    }

    const off_t original_offset = lseek(fd, 0, SEEK_CUR);
    if (lseek(fd, offset, SEEK_SET) < 0) {
        free(data);
        return NULL;
    }

    size_t read_size = 0;
    while (read_size < length) {
        const ssize_t result = read(fd, (char*)data + read_size, length - read_size);
        if (result > 0) {
            read_size += (size_t)result;
            continue;
        }
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (original_offset >= 0) {
            lseek(fd, original_offset, SEEK_SET);
        }
        free(data);
        return NULL;
    }
    if (original_offset >= 0) {
        lseek(fd, original_offset, SEEK_SET);
    }
    return data;
}

static inline int munmap(void* address, size_t) {
    free(address);
    return 0;
}
