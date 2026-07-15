#pragma once

#include <stdint.h>
#include <machine/endian.h>

#ifndef htobe16
#define htobe16(value) __builtin_bswap16(value)
#endif
#ifndef htobe32
#define htobe32(value) __builtin_bswap32(value)
#endif
#ifndef htobe64
#define htobe64(value) __builtin_bswap64(value)
#endif
#ifndef be16toh
#define be16toh(value) __builtin_bswap16(value)
#endif
#ifndef be32toh
#define be32toh(value) __builtin_bswap32(value)
#endif
#ifndef be64toh
#define be64toh(value) __builtin_bswap64(value)
#endif
#ifndef htole16
#define htole16(value) (value)
#endif
#ifndef htole32
#define htole32(value) (value)
#endif
#ifndef htole64
#define htole64(value) (value)
#endif
#ifndef le16toh
#define le16toh(value) (value)
#endif
#ifndef le32toh
#define le32toh(value) (value)
#endif
#ifndef le64toh
#define le64toh(value) (value)
#endif

