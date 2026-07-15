#pragma once

#include <errno.h>
#include <sys/socket.h>

struct ifaddrs {
    struct ifaddrs* ifa_next;
    char* ifa_name;
    unsigned int ifa_flags;
    struct sockaddr* ifa_addr;
    struct sockaddr* ifa_netmask;
    struct sockaddr* ifa_broadaddr;
    void* ifa_data;
};

static inline int getifaddrs(struct ifaddrs** interfaces) {
    if (interfaces) {
        *interfaces = nullptr;
    }
    errno = ENOSYS;
    return -1;
}

static inline void freeifaddrs(struct ifaddrs*) {}
