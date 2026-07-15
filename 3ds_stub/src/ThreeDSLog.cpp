#include "three_ds_stub/ThreeDSLog.hpp"

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <sys/stat.h>

namespace beiklive::three_ds_stub {
namespace {

std::mutex log_mutex;

FILE* OpenLog(const char* mode) {
    mkdir("sdmc:/GBAStation", 0777);
    mkdir("sdmc:/GBAStation/3ds", 0777);
    mkdir("sdmc:/GBAStation/3ds/log", 0777);
    return std::fopen("sdmc:/GBAStation/3ds/log/GBAStation3DSStub.log", mode);
}

} // namespace

void initializeLog() {
    std::scoped_lock lock{log_mutex};
    if (FILE* file = OpenLog("wb")) {
        std::fputs("GBAStation3DSStub: log session begin\n", file);
        std::fclose(file);
    }
}

void appendLog(const char* format, ...) {
    if (!format) {
        return;
    }

    char line[1024]{};
    va_list args;
    va_start(args, format);
    std::vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    std::scoped_lock lock{log_mutex};
    if (FILE* file = OpenLog("ab")) {
        std::fprintf(file, "%s\n", line);
        std::fflush(file);
        std::fclose(file);
    }
}

} // namespace beiklive::three_ds_stub

