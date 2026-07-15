#pragma once

namespace beiklive::three_ds_stub {

enum class LogLevel {
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
    Off,
};

void initializeLog(const char* requested_level = nullptr);
void shutdownLog();
void setLogLevel(LogLevel level);
[[nodiscard]] LogLevel currentLogLevel();
[[nodiscard]] const char* logLevelName(LogLevel level);
void logMessage(LogLevel level, const char* format, ...);

// Compatibility entry point for existing informational messages.
void appendLog(const char* format, ...);

} // namespace beiklive::three_ds_stub
