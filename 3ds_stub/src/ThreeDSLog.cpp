#include "three_ds_stub/ThreeDSLog.hpp"

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include <switch.h>

namespace beiklive::three_ds_stub {
namespace {

constexpr const char* kLogPath = "sdmc:/GBAStation/3ds/log/GBAStation3DSStub.log";
constexpr const char* kLevelPath = "sdmc:/GBAStation/3ds/log/level.txt";
constexpr const char* kCrashPath = "sdmc:/GBAStation/3ds/log/GBAStation3DSStub.crash.log";

std::mutex log_mutex;
std::atomic<LogLevel> log_level{LogLevel::Debug};
FILE* log_file = nullptr;
std::chrono::steady_clock::time_point session_start{};

void CreateLogDirectories() {
    mkdir("sdmc:/GBAStation", 0777);
    mkdir("sdmc:/GBAStation/3ds", 0777);
    mkdir("sdmc:/GBAStation/3ds/log", 0777);
}

LogLevel ParseLogLevel(const char* value, LogLevel fallback) {
    if (!value) {
        return fallback;
    }

    std::string normalized{value};
    const auto begin = normalized.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return fallback;
    }
    const auto end = normalized.find_last_not_of(" \t\r\n");
    normalized = normalized.substr(begin, end - begin + 1);
    for (char& character : normalized) {
        if (character >= 'A' && character <= 'Z') {
            character = static_cast<char>(character - 'A' + 'a');
        }
    }

    if (normalized == "trace") return LogLevel::Trace;
    if (normalized == "debug") return LogLevel::Debug;
    if (normalized == "info") return LogLevel::Info;
    if (normalized == "warn" || normalized == "warning") return LogLevel::Warning;
    if (normalized == "error") return LogLevel::Error;
    if (normalized == "critical" || normalized == "fatal") return LogLevel::Critical;
    if (normalized == "off" || normalized == "none") return LogLevel::Off;
    return fallback;
}

void SyncFile(FILE* file) {
    if (!file) {
        return;
    }
    std::fflush(file);
    const int descriptor = fileno(file);
    if (descriptor >= 0) {
        fsync(descriptor);
    }
}

LogLevel ReadConfiguredLevel() {
    char value[64]{};
    if (FILE* file = std::fopen(kLevelPath, "rb")) {
        const std::size_t read = std::fread(value, 1, sizeof(value) - 1, file);
        value[read] = '\0';
        std::fclose(file);
        return ParseLogLevel(value, LogLevel::Debug);
    }

    if (FILE* file = std::fopen(kLevelPath, "wb")) {
        std::fputs("debug\n", file);
        SyncFile(file);
        std::fclose(file);
    }
    return LogLevel::Debug;
}

void WriteLineLocked(LogLevel level, const char* line) {
    if (!log_file || !line) {
        return;
    }

    const auto wall_now = std::chrono::system_clock::now();
    const auto wall_time = std::chrono::system_clock::to_time_t(wall_now);
    std::tm local_time{};
    localtime_r(&wall_time, &local_time);
    const auto wall_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             wall_now.time_since_epoch()) %
                         1000;
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - session_start)
                                .count();

    std::fprintf(log_file,
                 "%04d-%02d-%02d %02d:%02d:%02d.%03lld [+%09lldms] [CPU%u] [%-8s] %s\n",
                 local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday,
                 local_time.tm_hour, local_time.tm_min, local_time.tm_sec,
                 static_cast<long long>(wall_ms.count()), static_cast<long long>(elapsed_ms),
                 svcGetCurrentProcessorNumber(), logLevelName(level), line);
    SyncFile(log_file);
}

void VLogMessage(LogLevel level, const char* format, va_list args) {
    if (!format || level < log_level.load(std::memory_order_relaxed) ||
        log_level.load(std::memory_order_relaxed) == LogLevel::Off) {
        return;
    }

    char line[2048]{};
    std::vsnprintf(line, sizeof(line), format, args);

    std::scoped_lock lock{log_mutex};
    WriteLineLocked(level, line);
}

} // namespace

void initializeLog(const char* requested_level) {
    std::scoped_lock lock{log_mutex};
    CreateLogDirectories();
    if (log_file) {
        SyncFile(log_file);
        std::fclose(log_file);
        log_file = nullptr;
    }
    const LogLevel configured = requested_level && requested_level[0] != '\0'
                                    ? ParseLogLevel(requested_level, ReadConfiguredLevel())
                                    : ReadConfiguredLevel();
    log_level.store(configured, std::memory_order_relaxed);
    session_start = std::chrono::steady_clock::now();
    log_file = std::fopen(kLogPath, "wb");
    if (FILE* crash_file = std::fopen(kCrashPath, "wb")) {
        SyncFile(crash_file);
        std::fclose(crash_file);
    }
    WriteLineLocked(LogLevel::Info, "log session begin");
    char detail[256]{};
    std::snprintf(detail, sizeof(detail),
                  "level=%s level_file=%s crash_file=%s immediate_sync=true",
                  logLevelName(configured), kLevelPath, kCrashPath);
    WriteLineLocked(LogLevel::Info, detail);
}

void shutdownLog() {
    std::scoped_lock lock{log_mutex};
    if (log_file) {
        WriteLineLocked(LogLevel::Info, "log session end");
        std::fclose(log_file);
        log_file = nullptr;
    }
}

void setLogLevel(LogLevel level) {
    log_level.store(level, std::memory_order_relaxed);
}

LogLevel currentLogLevel() {
    return log_level.load(std::memory_order_relaxed);
}

const char* logLevelName(LogLevel level) {
    switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Critical: return "CRITICAL";
    case LogLevel::Off: return "OFF";
    }
    return "UNKNOWN";
}

void logMessage(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    VLogMessage(level, format, args);
    va_end(args);
}

void appendLog(const char* format, ...) {
    va_list args;
    va_start(args, format);
    VLogMessage(LogLevel::Info, format, args);
    va_end(args);
}

} // namespace beiklive::three_ds_stub
