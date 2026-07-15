#pragma once

#include <stdarg.h>

#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7

#define LOG_CONS 0x02
#define LOG_PID 0x01
#define LOG_USER (1 << 3)
#define LOG_DAEMON (3 << 3)
#define LOG_LOCAL2 (18 << 3)

struct syslog_data {
    int log_stat;
    const char* log_tag;
    int log_fac;
    int log_mask;
};

#define SYSLOG_DATA_INIT {0, (const char*)0, LOG_USER, 0xff}

static inline void openlog(const char*, int, int) {}
static inline void closelog(void) {}
static inline void vsyslog(int, const char*, va_list) {}
static inline void syslog(int, const char*, ...) {}
static inline void vsyslog_r(int, struct syslog_data*, const char*, va_list) {}
static inline void syslog_r(int, struct syslog_data*, const char*, ...) {}
