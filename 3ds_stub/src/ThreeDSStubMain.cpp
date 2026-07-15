#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

#include <switch.h>

#include "three_ds_stub/ThreeDSLog.hpp"
#include "three_ds_stub/ThreeDSRuntime.hpp"

alignas(16) u8 __nx_exception_stack[0x4000];
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);

extern "C" char _start[];

extern "C" void __libnx_exception_handler(ThreadExceptionDump* context) {
    if (!context) {
        return;
    }

    FILE* file = std::fopen(
        "sdmc:/GBAStation/3ds/log/GBAStation3DSStub.crash.log", "wb");
    if (!file) {
        return;
    }

    const auto module_base = reinterpret_cast<std::uintptr_t>(_start);
    const auto pc = static_cast<std::uintptr_t>(context->pc.x);
    const auto lr = static_cast<std::uintptr_t>(context->lr.x);
    std::fprintf(file, "GBAStation3DSStub exception dump\n");
    std::fprintf(file, "error_desc=%#x aarch64=%s\n", context->error_desc,
                 threadExceptionIsAArch64(context) ? "true" : "false");
    std::fprintf(file, "module_base=%#llx\n",
                 static_cast<unsigned long long>(module_base));
    std::fprintf(file, "pc=%#llx pc_offset=%#llx\n",
                 static_cast<unsigned long long>(pc),
                 static_cast<unsigned long long>(pc - module_base));
    std::fprintf(file, "lr=%#llx lr_offset=%#llx\n",
                 static_cast<unsigned long long>(lr),
                 static_cast<unsigned long long>(lr - module_base));
    std::fprintf(file, "sp=%#llx fp=%#llx far=%#llx\n",
                 static_cast<unsigned long long>(context->sp.x),
                 static_cast<unsigned long long>(context->fp.x),
                 static_cast<unsigned long long>(context->far.x));
    std::fprintf(file, "pstate=%#x esr=%#x afsr0=%#x afsr1=%#x\n", context->pstate,
                 context->esr, context->afsr0, context->afsr1);
    for (int index = 0; index < 29; ++index) {
        std::fprintf(file, "x%-2d=%#018llx%s", index,
                     static_cast<unsigned long long>(context->cpu_gprs[index].x),
                     index % 2 == 1 ? "\n" : " ");
    }
    std::fputc('\n', file);
    std::fflush(file);
    const int descriptor = fileno(file);
    if (descriptor >= 0) {
        fsync(descriptor);
    }
    std::fclose(file);
}

namespace {

bool EndsWithNoCase(const std::string& value, const char* suffix) {
    const std::size_t suffix_length = std::strlen(suffix);
    if (value.size() < suffix_length) {
        return false;
    }
    const std::size_t offset = value.size() - suffix_length;
    for (std::size_t i = 0; i < suffix_length; ++i) {
        if (std::tolower(static_cast<unsigned char>(value[offset + i])) !=
            std::tolower(static_cast<unsigned char>(suffix[i]))) {
            return false;
        }
    }
    return true;
}

void SetReturnNro(const std::string& path) {
    if (path.empty() || !envHasNextLoad()) {
        return;
    }
    std::string args = "\"";
    for (const char character : path) {
        if (character == '\\' || character == '"') {
            args.push_back('\\');
        }
        args.push_back(character);
    }
    args.push_back('"');
    const Result rc = envSetNextLoad(path.c_str(), args.c_str());
    beiklive::three_ds_stub::logMessage(
        R_SUCCEEDED(rc) ? beiklive::three_ds_stub::LogLevel::Info
                        : beiklive::three_ds_stub::LogLevel::Error,
        "GBAStation3DSStub: envSetNextLoad rc=%#x path=%s", rc, path.c_str());
}

} // namespace

int main(int argc, char* argv[]) {
    using namespace beiklive::three_ds_stub;

    const char* requested_log_level = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (argv[i] && std::strcmp(argv[i], "--log-level") == 0 && i + 1 < argc) {
            requested_log_level = argv[i + 1];
            break;
        }
    }

    initializeLog(requested_log_level);
    logMessage(LogLevel::Info, "GBAStation3DSStub: process start argc=%d log_level=%s", argc,
               logLevelName(currentLogLevel()));
    for (int i = 0; i < argc; ++i) {
        logMessage(LogLevel::Debug, "GBAStation3DSStub: argv[%d]=%s", i,
                   argv[i] ? argv[i] : "<null>");
    }

    const Result core_mask_rc = svcSetThreadCoreMask(CUR_THREAD_HANDLE, 1, 1ULL << 1);
    logMessage(R_SUCCEEDED(core_mask_rc) ? LogLevel::Info : LogLevel::Warning,
               "GBAStation3DSStub: svcSetThreadCoreMask rc=%#x ideal_core=1 mask=%#llx current_cpu=%u",
               core_mask_rc, static_cast<unsigned long long>(1ULL << 1),
               svcGetCurrentProcessorNumber());

    std::string rom_path;
    std::string return_nro = "sdmc:/switch/GBAStation.nro";
    bool return_to_main = true;

    for (int i = 1; i < argc; ++i) {
        if (!argv[i]) {
            continue;
        }
        if (std::strcmp(argv[i], "--return") == 0 && i + 1 < argc) {
            return_nro = argv[++i];
        } else if (std::strcmp(argv[i], "--log-level") == 0) {
            if (i + 1 < argc) {
                ++i;
            } else {
                logMessage(LogLevel::Warning,
                           "GBAStation3DSStub: --log-level requires a value");
            }
        } else if (std::strcmp(argv[i], "--exit-to-home") == 0) {
            return_to_main = false;
        } else if (rom_path.empty() && !EndsWithNoCase(argv[i], ".nro")) {
            rom_path = argv[i];
        }
    }

    logMessage(LogLevel::Info, "GBAStation3DSStub: launch rom=%s return=%s return_to_main=%s",
               rom_path.c_str(), return_nro.c_str(), return_to_main ? "true" : "false");

    ThreeDSRuntime runtime;
    int result = 0;
    if (!runtime.Init() || !runtime.LoadGame(rom_path)) {
        logMessage(LogLevel::Error, "GBAStation3DSStub: initialization failed detail=%s",
                   runtime.LastError().c_str());
        result = 1;
    } else {
        while (appletMainLoop() && runtime.RunFrame()) {
        }
    }

    runtime.Shutdown();
    if (return_to_main) {
        SetReturnNro(return_nro);
    }
    logMessage(LogLevel::Info, "GBAStation3DSStub: exit result=%d", result);
    shutdownLog();
    return result;
}
